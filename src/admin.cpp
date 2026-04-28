void addChatMessage(ChatState& chat, const std::string& message) {
    chat.messages.push_back(message);
    if (chat.messages.size() > 8)
        chat.messages.erase(chat.messages.begin());
}

bool adminIsAllowedTile(char tile) {
    return tile == '.' || tile == '0' || tile == 'i' || tile == 'T' || tile == '~' || tile == ':' ||
           tile == 'w' || tile == '#' || tile == 'F' || tile == 'D' || tile == 'd' ||
           tile == 'C' || tile == 'f' || tile == 'B' || tile == 'M';
}

Position adminFirstPoint;
Position adminSecondPoint;
bool adminHasFirstPoint = false;
bool adminHasSecondPoint = false;

void adminSetTile(const Position& position, char tile) {
    blockDamage.erase(position);
    chests.erase(position);

    if (tile == '.') {
        placedBlocks.erase(position);
        brokenBlocks.insert(position);
        return;
    }

    brokenBlocks.erase(position);
    placedBlocks[position] = tile;
    if (tile == 'C' || tile == 'M')
        chests[position];
}

bool adminFill(char tile) {
    if (!adminHasFirstPoint || !adminHasSecondPoint)
        return false;
    if (!adminIsAllowedTile(tile))
        return false;

    long long minX = std::min(adminFirstPoint.x, adminSecondPoint.x);
    long long maxX = std::max(adminFirstPoint.x, adminSecondPoint.x);
    long long minY = std::min(adminFirstPoint.y, adminSecondPoint.y);
    long long maxY = std::max(adminFirstPoint.y, adminSecondPoint.y);
    long long width = maxX - minX + 1;
    long long height = maxY - minY + 1;
    if (width <= 0 || height <= 0 || width * height > 200000)
        return false;

    for (long long y = minY; y <= maxY; ++y) {
        for (long long x = minX; x <= maxX; ++x)
            adminSetTile({x, y}, tile);
    }
    return true;
}

void chatHelp(ChatState& chat) {
#ifdef PODVOH_ADMIN
    addChatMessage(chat, "Commands: /items /give <id> [n] /tp <x> <y> /pos1 here|x y /pos2 here|x y /blocks /fill <tile>");
#else
    addChatMessage(chat, "Chat is enabled. Host commands are only available in worldpodvoh.");
#endif
}

void chatItems(ChatState& chat) {
    addChatMessage(chat, "1 Stone, 2 Wood, 3 Raw Meat, 4 Cooked Meat, 8 Workbench, 9 Furnace, 10 Door, 11 Chest");
    addChatMessage(chat, "12 Spear, 13 Sand, 14 Wheat, 15 Seeds, 16 Fence, 17 Ore, 18 Ingot, 30 Bread");
    addChatMessage(chat, "21-23 Pickaxes, 24-26 Axes, 27-29 Shovels, 31 Shears, 32 Wool, 33 Bed");
}

void chatBlocks(ChatState& chat) {
    addChatMessage(chat, "Blocks: . empty, 0 stone, i ore, T tree, ~ water, : sand, w wheat, # bench, F furnace, D/d door, C chest, f fence, B bed, M bag");
}

bool parsePoint(std::istringstream& in, const Position& player, Position& point) {
    std::string firstArg;
    if (!(in >> firstArg))
        return false;

    if (firstArg == "here") {
        point = player;
        return true;
    }

    try {
        point.x = std::stoll(firstArg);
    } catch (...) {
        return false;
    }

    return static_cast<bool>(in >> point.y);
}

void executeHostCommand(const std::string& line, ChatState& chat, Position& player, Inventory& inventory) {
    std::istringstream in(line);
    std::string command;
    in >> command;

    if (command == "/help") {
        chatHelp(chat);
        return;
    }
    if (command == "/items") {
        chatItems(chat);
        return;
    }
    if (command == "/blocks") {
        chatBlocks(chat);
        return;
    }

#ifndef PODVOH_ADMIN
    (void)player;
    (void)inventory;
    addChatMessage(chat, "Only host worldpodvoh can use commands.");
    return;
#else
    if (command == "/give") {
        int item = 0;
        int amount = 999;
        if (!(in >> item)) {
            addChatMessage(chat, "Usage: /give <item-id> [amount]");
            return;
        }
        in >> amount;
        if (item <= static_cast<int>(ItemType::None) || item > static_cast<int>(ItemType::Bed) || amount <= 0) {
            addChatMessage(chat, "Bad item id or amount.");
            return;
        }
        addItem(inventory, static_cast<ItemType>(item), amount);
        addChatMessage(chat, "Gave " + std::to_string(amount) + " x " + itemName(static_cast<ItemType>(item)));
        return;
    }

    if (command == "/tp") {
        Position target;
        if (!(in >> target.x >> target.y)) {
            addChatMessage(chat, "Usage: /tp <x> <y>");
            return;
        }
        player = target;
        addChatMessage(chat, "Teleported to " + std::to_string(player.x) + "," + std::to_string(player.y));
        return;
    }

    if (command == "/pos1" || command == "/pos2") {
        Position point;
        if (!parsePoint(in, player, point)) {
            addChatMessage(chat, "Usage: " + command + " here OR " + command + " <x> <y>");
            return;
        }

        if (command == "/pos1") {
            adminFirstPoint = point;
            adminHasFirstPoint = true;
        } else {
            adminSecondPoint = point;
            adminHasSecondPoint = true;
        }
        addChatMessage(chat, command.substr(1) + " = " + std::to_string(point.x) + "," + std::to_string(point.y));
        return;
    }

    if (command == "/fill") {
        char tile = '\0';
        if (!(in >> tile)) {
            addChatMessage(chat, "Usage: /fill <tile>");
            return;
        }
        if (adminFill(tile))
            addChatMessage(chat, std::string("Filled selection with ") + tile);
        else
            addChatMessage(chat, "Fill failed. Set /pos1 and /pos2, max area is 200000.");
        return;
    }

    addChatMessage(chat, "Unknown command. Type /help.");
#endif
}

void processChatLine(ChatState& chat, Position& player, Inventory& inventory) {
    if (chat.pendingLine.empty())
        return;

    std::string line = chat.pendingLine;
    chat.pendingLine.clear();

    if (line.empty())
        return;

    if (line[0] == '/') {
        executeHostCommand(line, chat, player, inventory);
        return;
    }

    addChatMessage(chat, "<Host> " + line);
}
