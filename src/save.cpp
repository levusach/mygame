void clearWorldState() {
    brokenBlocks.clear();
    placedBlocks.clear();
    blockDamage.clear();
    worldTileCache.clear();
    seenCowSpawns.clear();
    killedCowSpawns.clear();
    seenSpiderSpawns.clear();
    killedSpiderSpawns.clear();
    activeSpiderNight = -1;
    seenSheepSpawns.clear();
    killedSheepSpawns.clear();
    droppedSpears.clear();
    droppedItems.clear();
    chests.clear();
    furnaces.clear();
    hasRespawnPoint = false;
    respawnPoint = {};
    hasDeathBag = false;
    deathBagPos = {};
}

std::vector<std::string> listSaveFiles() {
    namespace fs = std::filesystem;
    std::vector<std::string> saves;

    if (fs::exists(legacySavePath))
        saves.push_back(legacySavePath);

    if (fs::exists(saveDirectory)) {
        for (const auto& entry : fs::directory_iterator(saveDirectory)) {
            if (!entry.is_regular_file())
                continue;
            if (entry.path().extension() == ".sav")
                saves.push_back(entry.path().string());
        }
    }

    std::sort(saves.begin(), saves.end(), [](const std::string& left, const std::string& right) {
        if (left == legacySavePath)
            return false;
        if (right == legacySavePath)
            return true;
        return left > right;
    });
    return saves;
}

std::string saveLabel(const std::string& path) {
    if (path == legacySavePath)
        return "old world.sav";

    std::filesystem::path file(path);
    std::string name = file.stem().string();
    const std::string prefix = "world_";
    if (name.rfind(prefix, 0) == 0)
        name = name.substr(prefix.size());
    return name;
}

std::string makeNewSavePath() {
    namespace fs = std::filesystem;
    fs::create_directories(saveDirectory);

    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    char timePart[32];
    std::strftime(timePart, sizeof(timePart), "%Y%m%d_%H%M%S", &localTime);
    uint64_t suffix = mix(makeRandomWorldSeed()) & 0xffffULL;

    std::ostringstream path;
    path << saveDirectory << "/world_" << timePart << "_" << std::hex << suffix << ".sav";
    return path.str();
}

void writeSlot(std::ostream& out, const Slot& slot) {
    out << static_cast<int>(slot.item) << ' ' << slot.count << '\n';
}

void readSlot(std::istream& in, Slot& slot) {
    int item = 0;
    in >> item >> slot.count;
    slot.item = static_cast<ItemType>(item);
    if (slot.count <= 0)
        slot = {};
}

bool saveGame(const Position& player, const LookDirection& look, const Inventory& inventory,
              const PlayerStats& stats, const std::vector<Cow>& cows, const std::vector<Spider>& spiders,
              const std::vector<Sheep>& sheep, long long tick,
              long long lastMoveTick, long long lastAttackTick) {
    std::ofstream out(currentSavePath);
    if (!out)
        return false;

    out << "RATIT_SAVE 1\n";
    out << "PLAYER " << player.x << ' ' << player.y << '\n';
    out << "LOOK " << look.dx << ' ' << look.dy << '\n';
    out << "SEED " << worldSeed << '\n';
    out << "TICK " << tick << ' ' << lastMoveTick << ' ' << lastAttackTick << '\n';
    out << "STATS " << stats.hearts << ' ' << stats.hunger << ' ' << stats.thirst << '\n';
    out << "RESPAWN " << hasRespawnPoint << ' ' << respawnPoint.x << ' ' << respawnPoint.y << '\n';
    out << "DEATH_BAG " << hasDeathBag << ' ' << deathBagPos.x << ' ' << deathBagPos.y << '\n';
    out << "INV " << inventory.selectedSlot << ' ' << inventory.cursor << '\n';
    writeSlot(out, inventory.held);
    for (const Slot& slot : inventory.slots)
        writeSlot(out, slot);

    out << "PLACED " << placedBlocks.size() << '\n';
    for (const auto& placed : placedBlocks)
        out << placed.first.x << ' ' << placed.first.y << ' ' << static_cast<int>(placed.second) << '\n';

    out << "BROKEN " << brokenBlocks.size() << '\n';
    for (const Position& pos : brokenBlocks)
        out << pos.x << ' ' << pos.y << '\n';

    out << "DAMAGE " << blockDamage.size() << '\n';
    for (const auto& damage : blockDamage)
        out << damage.first.x << ' ' << damage.first.y << ' ' << damage.second << '\n';

    out << "SEEN_COWS " << seenCowSpawns.size() << '\n';
    for (const Position& pos : seenCowSpawns)
        out << pos.x << ' ' << pos.y << '\n';

    out << "KILLED_COWS " << killedCowSpawns.size() << '\n';
    for (const Position& pos : killedCowSpawns)
        out << pos.x << ' ' << pos.y << '\n';

    out << "DROPS " << droppedItems.size() << '\n';
    for (const auto& drop : droppedItems)
        out << drop.first.x << ' ' << drop.first.y << ' ' << static_cast<int>(drop.second.item) << ' ' << drop.second.count << '\n';

    out << "SPEARS " << droppedSpears.size() << '\n';
    for (const Position& pos : droppedSpears)
        out << pos.x << ' ' << pos.y << '\n';

    out << "CHESTS " << chests.size() << '\n';
    for (const auto& chest : chests) {
        out << chest.first.x << ' ' << chest.first.y << '\n';
        for (const Slot& slot : chest.second)
            writeSlot(out, slot);
    }

    out << "FURNACES " << furnaces.size() << '\n';
    for (const auto& furnace : furnaces) {
        out << furnace.first.x << ' ' << furnace.first.y << ' ' << furnace.second.progress << ' ' << furnace.second.burnTicks << '\n';
        writeSlot(out, furnace.second.slot);
        writeSlot(out, furnace.second.fuel);
    }

    out << "COWS " << cows.size() << '\n';
    for (const Cow& cow : cows)
        out << cow.pos.x << ' ' << cow.pos.y << ' ' << cow.spawn.x << ' ' << cow.spawn.y << ' ' << cow.milked << '\n';

    out << "SEEN_SPIDERS " << seenSpiderSpawns.size() << '\n';
    for (const Position& pos : seenSpiderSpawns)
        out << pos.x << ' ' << pos.y << '\n';

    out << "KILLED_SPIDERS " << killedSpiderSpawns.size() << '\n';
    for (const Position& pos : killedSpiderSpawns)
        out << pos.x << ' ' << pos.y << '\n';

    out << "SPIDERS " << spiders.size() << '\n';
    for (const Spider& spider : spiders)
        out << spider.pos.x << ' ' << spider.pos.y << ' ' << spider.spawn.x << ' ' << spider.spawn.y << '\n';

    out << "SEEN_SHEEP " << seenSheepSpawns.size() << '\n';
    for (const Position& pos : seenSheepSpawns)
        out << pos.x << ' ' << pos.y << '\n';

    out << "KILLED_SHEEP " << killedSheepSpawns.size() << '\n';
    for (const Position& pos : killedSheepSpawns)
        out << pos.x << ' ' << pos.y << '\n';

    out << "SHEEP " << sheep.size() << '\n';
    for (const Sheep& oneSheep : sheep)
        out << oneSheep.pos.x << ' ' << oneSheep.pos.y << ' ' << oneSheep.spawn.x << ' ' << oneSheep.spawn.y << ' ' << oneSheep.sheared << '\n';

    return true;
}

bool loadGame(Position& player, LookDirection& look, Inventory& inventory, PlayerStats& stats,
              std::vector<Cow>& cows, std::vector<Spider>& spiders, std::vector<Sheep>& sheep, long long& tick, long long& lastMoveTick,
              long long& lastAttackTick) {
    std::ifstream in(currentSavePath);
    if (!in)
        return false;

    std::string tag;
    int version = 0;
    in >> tag >> version;
    if (tag != "RATIT_SAVE" || version != 1)
        return false;

    clearWorldState();
    inventory = {};
    stats = {};
    cows.clear();
    spiders.clear();
    sheep.clear();

    in >> tag >> player.x >> player.y;
    in >> tag >> look.dx >> look.dy;

    in >> tag;
    if (tag == "SEED") {
        in >> worldSeed;
        setupWorldGeneration(worldSeed);
        in >> tag;
    } else {
        setupWorldGeneration(0);
    }

    if (tag != "TICK")
        return false;
    in >> tick >> lastMoveTick >> lastAttackTick;
    in >> tag;
    if (tag != "STATS")
        return false;
    std::string statsLine;
    std::getline(in, statsLine);
    std::istringstream statsIn(statsLine);
    statsIn >> stats.hearts >> stats.hunger >> stats.thirst;
    in >> tag;
    if (tag == "RESPAWN") {
        in >> hasRespawnPoint >> respawnPoint.x >> respawnPoint.y;
        in >> tag;
    }
    if (tag == "DEATH_BAG") {
        in >> hasDeathBag >> deathBagPos.x >> deathBagPos.y;
        in >> tag;
    }
    if (tag != "INV")
        return false;
    in >> inventory.selectedSlot >> inventory.cursor;
    readSlot(in, inventory.held);
    for (Slot& slot : inventory.slots)
        readSlot(in, slot);

    size_t count = 0;
    in >> tag >> count;
    for (size_t i = 0; i < count; ++i) {
        Position pos;
        int tile = 0;
        in >> pos.x >> pos.y >> tile;
        placedBlocks[pos] = static_cast<char>(tile);
    }

    in >> tag >> count;
    for (size_t i = 0; i < count; ++i) {
        Position pos;
        in >> pos.x >> pos.y;
        brokenBlocks.insert(pos);
    }

    in >> tag >> count;
    for (size_t i = 0; i < count; ++i) {
        Position pos;
        int damage = 0;
        in >> pos.x >> pos.y >> damage;
        if (damage > 0)
            blockDamage[pos] = damage;
    }

    in >> tag >> count;
    if (tag == "CANALS" || tag == "WATER_DEPTH") {
        for (size_t i = 0; i < count; ++i) {
            Position pos;
            int ignored = 0;
            in >> pos.x >> pos.y;
            if (tag == "WATER_DEPTH")
                in >> ignored;
        }
        in >> tag >> count;
        if (tag == "WATER_DEPTH") {
            for (size_t i = 0; i < count; ++i) {
                Position pos;
                int ignored = 0;
                in >> pos.x >> pos.y >> ignored;
            }
            in >> tag >> count;
        }
    }

    for (size_t i = 0; i < count; ++i) {
        Position pos;
        in >> pos.x >> pos.y;
        seenCowSpawns.insert(pos);
    }

    in >> tag >> count;
    for (size_t i = 0; i < count; ++i) {
        Position pos;
        in >> pos.x >> pos.y;
        killedCowSpawns.insert(pos);
    }

    in >> tag >> count;
    for (size_t i = 0; i < count; ++i) {
        Position pos;
        int item = 0;
        Slot slot;
        in >> pos.x >> pos.y >> item >> slot.count;
        slot.item = static_cast<ItemType>(item);
        if (slot.item != ItemType::None && slot.count > 0)
            droppedItems[pos] = slot;
    }

    in >> tag >> count;
    for (size_t i = 0; i < count; ++i) {
        Position pos;
        in >> pos.x >> pos.y;
        droppedSpears.insert(pos);
    }

    in >> tag >> count;
    if (tag == "CHESTS") {
        for (size_t i = 0; i < count; ++i) {
            Position pos;
            in >> pos.x >> pos.y;
            auto& slots = chests[pos];
            for (Slot& slot : slots)
                readSlot(in, slot);
        }
        in >> tag >> count;
    }

    if (tag == "FURNACES") {
        for (size_t i = 0; i < count; ++i) {
            Position pos;
            FurnaceState state;
            in >> pos.x >> pos.y >> state.progress;
            bool hasFuelSlot = false;
            if (in.peek() == ' ') {
                in >> state.burnTicks;
                hasFuelSlot = true;
            }
            readSlot(in, state.slot);
            if (hasFuelSlot)
                readSlot(in, state.fuel);
            furnaces[pos] = state;
        }
        in >> tag >> count;
    }

    for (size_t i = 0; i < count; ++i) {
        Cow cow;
        in >> cow.pos.x >> cow.pos.y >> cow.spawn.x >> cow.spawn.y >> cow.milked;
        cow.previousPos = cow.pos;
        cow.moveStartedTick = tick;
        cows.push_back(cow);
    }

    in >> tag >> count;
    if (tag == "SEEN_SPIDERS") {
        for (size_t i = 0; i < count; ++i) {
            Position pos;
            in >> pos.x >> pos.y;
            seenSpiderSpawns.insert(pos);
        }
        in >> tag >> count;
    }

    if (tag == "KILLED_SPIDERS") {
        for (size_t i = 0; i < count; ++i) {
            Position pos;
            in >> pos.x >> pos.y;
            killedSpiderSpawns.insert(pos);
        }
        in >> tag >> count;
    }

    if (tag == "SPIDERS") {
        std::string line;
        std::getline(in, line);
        for (size_t i = 0; i < count; ++i) {
            Spider spider;
            std::getline(in, line);
            std::istringstream spiderIn(line);
            spiderIn >> spider.pos.x >> spider.pos.y >> spider.spawn.x >> spider.spawn.y;
            spider.previousPos = spider.pos;
            spider.moveStartedTick = tick;
            spiders.push_back(spider);
        }
        in >> tag >> count;
    }

    if (tag == "SEEN_SHEEP") {
        for (size_t i = 0; i < count; ++i) {
            Position pos;
            in >> pos.x >> pos.y;
            seenSheepSpawns.insert(pos);
        }
        in >> tag >> count;
    }

    if (tag == "KILLED_SHEEP") {
        for (size_t i = 0; i < count; ++i) {
            Position pos;
            in >> pos.x >> pos.y;
            killedSheepSpawns.insert(pos);
        }
        in >> tag >> count;
    }

    if (tag == "SHEEP") {
        for (size_t i = 0; i < count; ++i) {
            Sheep oneSheep;
            in >> oneSheep.pos.x >> oneSheep.pos.y >> oneSheep.spawn.x >> oneSheep.spawn.y >> oneSheep.sheared;
            oneSheep.previousPos = oneSheep.pos;
            oneSheep.moveStartedTick = tick;
            sheep.push_back(oneSheep);
        }
    }

    inventory.open = false;
    inventory.actionMenuOpen = false;
    inventory.craftMenuOpen = false;
    inventory.chestOpen = false;
    inventory.furnaceOpen = false;
    return in.good() || in.eof();
}

int showMainMenu(bool hasSave) {
    timeout(-1);
    clear();
    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);
    int boxWidth = 34;
    int boxHeight = 8;
    int y = (height - boxHeight) / 2;
    int x = (width - boxWidth) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    drawRoundedBox(y, x, boxHeight, boxWidth);
    mvprintw(y + 1, x + 2, "The World");
    mvprintw(y + 3, x + 2, "N - new game");
    mvprintw(y + 4, x + 2, hasSave ? "C - continue" : "C - continue (no save)");
    mvprintw(y + 5, x + 2, "Q - quit");
    refresh();

    while (true) {
        int key = getch();
        if (key == 'n' || key == 'N') {
            timeout(0);
            return 1;
        }
        if ((key == 'c' || key == 'C') && hasSave) {
            timeout(0);
            return 2;
        }
        if (key == 'q' || key == 'Q') {
            timeout(0);
            return 0;
        }
    }
}

int showLoadMenu(const std::vector<std::string>& saves) {
    timeout(-1);
    int cursor = 0;

    while (true) {
        clear();
        int height = 0;
        int width = 0;
        getmaxyx(stdscr, height, width);
        int visible = static_cast<int>(saves.size());
        if (visible > 9)
            visible = 9;
        int boxWidth = 46;
        int boxHeight = visible + 5;
        int y = (height - boxHeight) / 2;
        int x = (width - boxWidth) / 2;
        if (x < 0) x = 0;
        if (y < 0) y = 0;

        drawRoundedBox(y, x, boxHeight, boxWidth);
        mvprintw(y + 1, x + 2, "Choose world");
        for (int i = 0; i < visible; ++i) {
            if (i == cursor)
                attron(A_REVERSE);
            mvprintw(y + 3 + i, x + 2, "%d - %s", i + 1, saveLabel(saves[i]).c_str());
            if (i == cursor)
                attroff(A_REVERSE);
        }
        mvprintw(y + boxHeight - 1, x + 2, "Enter load  Esc back");
        refresh();

        int key = getch();
        if (key == 27) {
            timeout(0);
            return -1;
        }
        if (key == KEY_UP || key == 'w' || key == 'W') {
            if (cursor > 0)
                cursor--;
        } else if (key == KEY_DOWN || key == 's' || key == 'S') {
            if (cursor < visible - 1)
                cursor++;
        } else if (key >= '1' && key <= '9') {
            int picked = key - '1';
            if (picked < visible) {
                timeout(0);
                return picked;
            }
        } else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            timeout(0);
            return cursor;
        }
    }
}
