#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
using SockLen = int;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SockLen = socklen_t;
#endif

constexpr uint16_t defaultNetworkPort = 41731;
constexpr int networkSendTicks = tickRate / 10;
constexpr int networkTimeoutTicks = tickRate * 8;

std::string networkModeName(const NetworkSession& session) {
    if (session.mode == NetworkMode::Host)
        return "host";
    if (session.mode == NetworkMode::Client)
        return "client";
    return "offline";
}

void closeNetworkSocket(NetworkSession& session) {
    if (session.socketFd < 0)
        return;
#if defined(_WIN32)
    closesocket(session.socketFd);
    if (session.winsockStarted)
        WSACleanup();
    session.winsockStarted = false;
#else
    close(session.socketFd);
#endif
    session.socketFd = -1;
}

uint32_t makeNetworkId() {
    uint64_t seed = makeRandomWorldSeed();
    uint32_t id = static_cast<uint32_t>(seed ^ (seed >> 32));
    return id == 0 ? 1 : id;
}

bool setNonBlockingSocket(int fd) {
#if defined(_WIN32)
    u_long mode = 1;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool openUdpSocket(NetworkSession& session, bool bindSocket) {
#if defined(_WIN32)
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return false;
    session.winsockStarted = true;
#endif

    session.socketFd = static_cast<int>(socket(AF_INET, SOCK_DGRAM, 0));
    if (session.socketFd < 0)
        return false;

    int yes = 1;
    setsockopt(session.socketFd, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&yes), sizeof(yes));
    setNonBlockingSocket(session.socketFd);

    if (!bindSocket)
        return true;

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(session.port);
    if (bind(session.socketFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        closeNetworkSocket(session);
        return false;
    }
    return true;
}

sockaddr_in makeAddress(const std::string& host, uint16_t port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) == 1)
        return address;

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &result) == 0 && result) {
        address.sin_addr = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr;
        freeaddrinfo(result);
    } else {
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    return address;
}

std::string endpointKey(const sockaddr_in& address) {
    char host[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &address.sin_addr, host, sizeof(host));
    return std::string(host) + ":" + std::to_string(ntohs(address.sin_port));
}

struct NetworkPeer {
    uint32_t id = 0;
    sockaddr_in address{};
    std::string key;
};

std::vector<NetworkPeer> networkPeers;

void rememberPeer(uint32_t id, const sockaddr_in& address) {
    std::string key = endpointKey(address);
    for (NetworkPeer& peer : networkPeers) {
        if (peer.key == key || (id != 0 && peer.id == id)) {
            peer.id = id;
            peer.address = address;
            peer.key = key;
            return;
        }
    }
    networkPeers.push_back({id, address, key});
}

void sendPacket(NetworkSession& session, const sockaddr_in& address, const std::string& packet) {
    if (session.socketFd < 0)
        return;
    sendto(session.socketFd, packet.c_str(), static_cast<int>(packet.size()), 0,
           reinterpret_cast<const sockaddr*>(&address), sizeof(address));
}

void sendToHost(NetworkSession& session, const std::string& packet) {
    sockaddr_in address = makeAddress(session.connectHost, session.port);
    sendPacket(session, address, packet);
}

void sendToClients(NetworkSession& session, const std::string& packet) {
    for (const NetworkPeer& peer : networkPeers)
        sendPacket(session, peer.address, packet);
}

NetworkPlayer* findNetworkPlayer(NetworkSession& session, uint32_t id) {
    for (NetworkPlayer& player : session.players) {
        if (player.id == id)
            return &player;
    }
    return nullptr;
}

void upsertNetworkPlayer(NetworkSession& session, const NetworkPlayer& incoming) {
    if (incoming.id == session.localId)
        return;

    NetworkPlayer* existing = findNetworkPlayer(session, incoming.id);
    if (existing) {
        *existing = incoming;
        return;
    }
    session.players.push_back(incoming);
}

std::string cleanNetworkName(std::string name) {
    if (name.empty())
        return "Player";
    for (char& ch : name) {
        if (ch == '|' || ch == '\n' || ch == '\r')
            ch = '_';
    }
    if (name.size() > 20)
        name.resize(20);
    return name;
}

std::string playerPacket(const NetworkSession& session, const Position& player,
                         const LookDirection& look, long long tick) {
    std::ostringstream out;
    out << "PLAYER " << session.localId << ' ' << player.x << ' ' << player.y << ' '
        << look.dx << ' ' << look.dy << ' ' << tick << ' ' << cleanNetworkName(session.name);
    return out.str();
}

std::string statePacket(const NetworkSession& session, const Position& hostPlayer,
                        const LookDirection& hostLook, long long tick) {
    std::ostringstream out;
    out << "STATE " << worldSeed << ' ' << tick << ' ' << (session.players.size() + 1)
        << ' ' << session.localId << ' ' << hostPlayer.x << ' ' << hostPlayer.y << ' '
        << hostLook.dx << ' ' << hostLook.dy << ' ' << cleanNetworkName(session.name);
    for (const NetworkPlayer& player : session.players) {
        out << '|' << player.id << ' ' << player.pos.x << ' ' << player.pos.y << ' '
            << player.look.dx << ' ' << player.look.dy << ' ' << cleanNetworkName(player.name);
    }
    return out.str();
}

bool startNetwork(NetworkSession& session, ChatState& chat) {
    if (session.mode == NetworkMode::Offline)
        return true;

    session.localId = makeNetworkId();
    session.name = cleanNetworkName(session.name);
    bool bindSocket = session.mode == NetworkMode::Host;
    if (!openUdpSocket(session, bindSocket)) {
        addChatMessage(chat, "Network: failed to open UDP socket.");
        session.mode = NetworkMode::Offline;
        return false;
    }

    session.ready = session.mode == NetworkMode::Host;
    if (session.mode == NetworkMode::Host) {
        networkPeers.clear();
        addChatMessage(chat, "Network: hosting LAN on UDP port " + std::to_string(session.port) + ".");
    } else {
        addChatMessage(chat, "Network: joining " + session.connectHost + ":" + std::to_string(session.port) + ".");
    }
    return true;
}

void parsePlayerLine(NetworkSession& session, std::istream& in, long long tick) {
    NetworkPlayer player;
    in >> player.id >> player.pos.x >> player.pos.y >> player.look.dx >> player.look.dy >> player.lastSeenTick;
    std::getline(in, player.name);
    if (!player.name.empty() && player.name[0] == ' ')
        player.name.erase(player.name.begin());
    player.name = cleanNetworkName(player.name);
    player.lastSeenTick = tick;
    upsertNetworkPlayer(session, player);
}

void parseStatePacket(NetworkSession& session, const std::string& packet, long long tick) {
    std::istringstream in(packet);
    std::string tag;
    uint64_t seed = 0;
    long long hostTick = 0;
    size_t count = 0;
    in >> tag >> seed >> hostTick >> count;
    if (seed != 0 && seed != worldSeed)
        setupWorldGeneration(seed);

    std::string rest;
    std::getline(in, rest);
    if (!rest.empty() && rest[0] == ' ')
        rest.erase(rest.begin());

    std::vector<NetworkPlayer> parsed;
    std::stringstream players(rest);
    std::string part;
    while (std::getline(players, part, '|')) {
        std::istringstream playerIn(part);
        NetworkPlayer player;
        playerIn >> player.id >> player.pos.x >> player.pos.y >> player.look.dx >> player.look.dy;
        std::getline(playerIn, player.name);
        if (!player.name.empty() && player.name[0] == ' ')
            player.name.erase(player.name.begin());
        player.name = cleanNetworkName(player.name);
        player.lastSeenTick = tick;
        if (player.id != session.localId)
            parsed.push_back(player);
    }
    session.players = parsed;
    session.ready = true;
}

void handleNetworkPacket(NetworkSession& session, ChatState& chat, const std::string& packet,
                         const sockaddr_in& from, long long tick) {
    std::istringstream in(packet);
    std::string tag;
    in >> tag;

    if (tag == "HELLO" && session.mode == NetworkMode::Host) {
        uint32_t id = 0;
        in >> id;
        rememberPeer(id, from);
        std::string welcome = "WELCOME " + std::to_string(worldSeed) + " " + std::to_string(session.localId);
        sendPacket(session, from, welcome);
        return;
    }

    if (tag == "WELCOME" && session.mode == NetworkMode::Client) {
        uint64_t seed = 0;
        in >> seed;
        if (seed != 0 && seed != worldSeed)
            setupWorldGeneration(seed);
        session.ready = true;
        return;
    }

    if (tag == "PLAYER" && session.mode == NetworkMode::Host) {
        uint32_t id = 0;
        std::istringstream idReader(packet);
        idReader >> tag >> id;
        rememberPeer(id, from);
        std::istringstream playerIn(packet);
        playerIn >> tag;
        parsePlayerLine(session, playerIn, tick);
        return;
    }

    if (tag == "STATE" && session.mode == NetworkMode::Client) {
        parseStatePacket(session, packet, tick);
        return;
    }

    if (tag == "CHAT") {
        uint32_t id = 0;
        in >> id;
        std::string message;
        std::getline(in, message);
        if (!message.empty() && message[0] == ' ')
            message.erase(message.begin());
        if (!message.empty())
            addChatMessage(chat, message);
        if (session.mode == NetworkMode::Host && id != session.localId)
            sendToClients(session, packet);
    }
}

void pollNetwork(NetworkSession& session, ChatState& chat, const Position& player,
                 const LookDirection& look, long long tick) {
    if (session.mode == NetworkMode::Offline || session.socketFd < 0)
        return;

    while (true) {
        char buffer[4096];
        sockaddr_in from{};
        SockLen fromLen = sizeof(from);
        int received = static_cast<int>(recvfrom(session.socketFd, buffer, sizeof(buffer) - 1, 0,
                                                reinterpret_cast<sockaddr*>(&from), &fromLen));
        if (received <= 0)
            break;
        buffer[received] = '\0';
        handleNetworkPacket(session, chat, std::string(buffer), from, tick);
    }

    session.players.erase(std::remove_if(session.players.begin(), session.players.end(),
        [&](const NetworkPlayer& other) {
            return tick - other.lastSeenTick > networkTimeoutTicks;
        }), session.players.end());

    if (tick - session.lastSendTick < networkSendTicks)
        return;
    session.lastSendTick = tick;

    if (session.mode == NetworkMode::Client) {
        if (!session.announcedJoin || !session.ready) {
            sendToHost(session, "HELLO " + std::to_string(session.localId) + " " + session.name);
            session.announcedJoin = true;
        }
        sendToHost(session, playerPacket(session, player, look, tick));
    } else if (session.mode == NetworkMode::Host) {
        sendToClients(session, statePacket(session, player, look, tick));
    }
}

void sendNetworkChat(NetworkSession& session, const std::string& message) {
    if (session.mode == NetworkMode::Offline || message.empty())
        return;

    std::string packet = "CHAT " + std::to_string(session.localId) + " <" +
                         cleanNetworkName(session.name) + "> " + message;
    if (session.mode == NetworkMode::Host)
        sendToClients(session, packet);
    else
        sendToHost(session, packet);
}

bool parseHostPort(const std::string& value, std::string& host, uint16_t& port) {
    size_t colon = value.rfind(':');
    if (colon == std::string::npos) {
        host = value;
        return true;
    }

    host = value.substr(0, colon);
    int parsedPort = std::atoi(value.substr(colon + 1).c_str());
    if (parsedPort <= 0 || parsedPort > 65535)
        return false;
    port = static_cast<uint16_t>(parsedPort);
    return !host.empty();
}
