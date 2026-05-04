int main(int argc, char** argv) {
    NetworkSession network;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--unicode") {
            useUnicode = true;
        } else if (arg == "--host") {
            network.mode = NetworkMode::Host;
            if (i + 1 < argc && argv[i + 1][0] >= '0' && argv[i + 1][0] <= '9') {
                int port = std::atoi(argv[++i]);
                if (port > 0 && port <= 65535)
                    network.port = static_cast<uint16_t>(port);
            }
        } else if (arg == "--join" && i + 1 < argc) {
            network.mode = NetworkMode::Client;
            network.port = defaultNetworkPort;
            parseHostPort(argv[++i], network.connectHost, network.port);
        } else if (arg == "--port" && i + 1 < argc) {
            int port = std::atoi(argv[++i]);
            if (port > 0 && port <= 65535)
                network.port = static_cast<uint16_t>(port);
        } else if (arg == "--name" && i + 1 < argc) {
            network.name = argv[++i];
        }
    }

    setlocale(LC_ALL, "");
    initscr();
    raw();
    noecho();
    keypad(stdscr, true);
    curs_set(0);
    leaveok(stdscr, true);
    idlok(stdscr, true);
    set_escdelay(25);
    timeout(0);
    initColors();

    Position player;
    LookDirection look;
    MouseState mouse;
    HeldInput input;
    Inventory inventory;
    PlayerStats stats;
    ChatState chat;
    VisualState visual;
    std::vector<Cow> cows;
    std::vector<Sheep> sheep;
    std::vector<Spider> spiders;
    long long tick = 0;
    long long lastMoveTick = -ticksPerMove;
    long long lastAttackTick = -attackCooldownTicks;
    auto nextTick = Clock::now();
    const auto tickDuration = std::chrono::microseconds(1000000 / tickRate);
    bool dirty = true;
    bool warnedLowHearts = false;
    bool warnedNoFood = false;
    bool warnedNoWater = false;

    if (network.mode == NetworkMode::Client) {
        clearWorldState();
        setupWorldGeneration(0);
        currentSavePath = makeNewSavePath();
        addChatMessage(chat, "Waiting for LAN host world seed...");
    } else {
        std::vector<std::string> saves = listSaveFiles();
        int menuChoice = showMainMenu(!saves.empty());
        if (menuChoice == 0) {
            endwin();
            return 0;
        }

        if (menuChoice == 2) {
            int saveChoice = showLoadMenu(saves);
            if (saveChoice < 0 || saveChoice >= static_cast<int>(saves.size())) {
                endwin();
                return 0;
            }
            currentSavePath = saves[saveChoice];
        } else if (menuChoice == 1) {
            currentSavePath = makeNewSavePath();
            setupWorldGeneration(makeRandomWorldSeed());
            clearWorldState();
        }

        if (menuChoice == 2 && !loadGame(player, look, inventory, stats, cows, spiders, sheep, tick, lastMoveTick, lastAttackTick)) {
            clearWorldState();
            currentSavePath = makeNewSavePath();
            setupWorldGeneration(makeRandomWorldSeed());
            player = {};
            look = {};
            inventory = {};
            stats = {};
            cows.clear();
            sheep.clear();
            spiders.clear();
            tick = 0;
            lastMoveTick = -ticksPerMove;
            lastAttackTick = -attackCooldownTicks;
        }
    }

    startNetwork(network, chat);

    visual.playerPrevious = player;
    visual.playerMoveStartedTick = tick;

    enableMouseTracking();

    while (true) {
        pollNetwork(network, chat, player, look, tick);
        readHeldInput(input, mouse, chat);
        if (!chat.pendingLine.empty()) {
            std::string line = chat.pendingLine;
            if (!line.empty() && line[0] != '/' && network.mode != NetworkMode::Offline) {
                chat.pendingLine.clear();
                addChatMessage(chat, "<" + cleanNetworkName(network.name) + "> " + line);
                sendNetworkChat(network, line);
            } else {
                processChatLine(chat, player, inventory);
            }
            dirty = true;
        }
        if (spawnCowsAroundPlayer(cows, player))
            dirty = true;
        if (spawnSheepAroundPlayer(sheep, player))
            dirty = true;
        if (updateSpiderDayCycle(spiders, tick))
            dirty = true;
        if (spawnNightSpidersAroundPlayer(spiders, player, tick))
            dirty = true;
        if (updateCows(cows, player, tick, inventory))
            dirty = true;
        if (updateSheep(sheep, player, tick, inventory))
            dirty = true;
        if (updateSpiders(spiders, player, stats, tick))
            dirty = true;
        if (updateWheatGrowth(tick))
            dirty = true;
        if (updateFurnaces(tick))
            dirty = true;
        if (updateWaterPhysics(player, tick))
            dirty = true;
        if (updateSurvival(stats, tick))
            dirty = true;
        if (stats.hearts <= 3 && !warnedLowHearts) {
            addChatMessage(chat, "Warning: low HP.");
            warnedLowHearts = true;
            dirty = true;
        } else if (stats.hearts > 3) {
            warnedLowHearts = false;
        }
        if (stats.hunger == 0 && !warnedNoFood) {
            addChatMessage(chat, "Warning: starving. Eat something.");
            warnedNoFood = true;
            dirty = true;
        } else if (stats.hunger > 0) {
            warnedNoFood = false;
        }
        if (stats.thirst == 0 && !warnedNoWater) {
            addChatMessage(chat, "Warning: dehydrated. Drink boiled water or milk.");
            warnedNoWater = true;
            dirty = true;
        } else if (stats.thirst > 0) {
            warnedNoWater = false;
        }
        if (handleDeath(player, inventory, stats)) {
            addChatMessage(chat, "You died. Your inventory is in death bag M at " +
                                std::to_string(deathBagPos.x) + "," +
                                std::to_string(deathBagPos.y) + ". Press E near it.");
            warnedLowHearts = false;
            warnedNoFood = false;
            warnedNoWater = false;
            visual.playerPrevious = player;
            visual.playerMoveStartedTick = tick;
            dirty = true;
        }
        if (pickupAtPlayer(player, inventory))
            dirty = true;
        if (tick % tickRate == 0)
            dirty = true;
        if (tick - visual.playerMoveStartedTick <= ticksPerMove)
            dirty = true;
        for (const Cow& cow : cows) {
            if (tick - cow.moveStartedTick <= cowMoveTicks) {
                dirty = true;
                break;
            }
        }
        for (const Spider& spider : spiders) {
            if (tick - spider.moveStartedTick <= spiderMoveTicks) {
                dirty = true;
                break;
            }
        }
        for (const Sheep& oneSheep : sheep) {
            if (tick - oneSheep.moveStartedTick <= sheepMoveTicks) {
                dirty = true;
                break;
            }
        }
        LookDirection oldLook = look;
        updateLookFromMouseState(player, mouse, look);
        if (!(look == oldLook))
            dirty = true;

        auto now = Clock::now();
        pruneOldInput(input, now);

        MoveInput move = currentMove(input);
        if (move.quit)
            break;

        if (input.selectedSlotQueued >= 0) {
            inventory.selectedSlot = input.selectedSlotQueued;
            input.selectedSlotQueued = -1;
            dirty = true;
        }

        if (input.inventoryToggleQueued) {
            inventory.open = !inventory.open;
            inventory.actionMenuOpen = false;
            inventory.craftMenuOpen = false;
            if (!inventory.open) {
                inventory.chestOpen = false;
                inventory.furnaceOpen = false;
                if (inventory.cursor >= inventorySlots)
                    inventory.cursor = inventory.selectedSlot;
            }
            input.inventoryToggleQueued = false;
            dirty = true;
        }

        if (input.craftToggleQueued) {
            if (!inventory.open) {
                inventory.open = true;
                inventory.cursor = inventory.selectedSlot;
            }
            inventory.actionMenuOpen = false;
            inventory.craftMenuOpen = !inventory.craftMenuOpen;
            inventory.craftCursor = 0;
            input.craftToggleQueued = false;
            dirty = true;
        }

        if (!inventory.open && mouse.placeQueued) {
            if (placeSelectedBlock(player, look, inventory))
                dirty = true;
            mouse.placeQueued = false;
        } else if (inventory.open) {
            mouse.placeQueued = false;
        }

        if (inventory.open && input.interactQueued) {
            if (inventory.craftMenuOpen) {
                std::vector<Action> recipes = buildCraftActions(inventory, player);
                if (!recipes.empty()) {
                    if (inventory.craftCursor >= static_cast<int>(recipes.size()))
                        inventory.craftCursor = static_cast<int>(recipes.size()) - 1;

                    if (input.craftAllQueued) {
                        for (int crafted = 0; crafted < 256; ++crafted) {
                            if (!executeAction(recipes[inventory.craftCursor].type, inventory, stats, player, look, cows))
                                break;
                        }
                    } else {
                        executeAction(recipes[inventory.craftCursor].type, inventory, stats, player, look, cows);
                    }
                }
                input.craftAllQueued = false;
            } else if (!inventory.actionMenuOpen) {
                interactInventory(inventory);
            } else {
                std::vector<Action> actions = buildActions(inventory, player, cows);
                if (!actions.empty()) {
                    if (inventory.actionCursor >= static_cast<int>(actions.size()))
                        inventory.actionCursor = static_cast<int>(actions.size()) - 1;
                    executeAction(actions[inventory.actionCursor].type, inventory, stats, player, look, cows);
                }
                inventory.actionMenuOpen = false;
            }
            input.interactQueued = false;
            dirty = true;
        } else if (!inventory.open && input.interactQueued) {
            if (openNearbyChest(inventory, player, look) || openNearbyFurnace(inventory, player, look)) {
                input.interactQueued = false;
                dirty = true;
            } else if (setRespawnAtNearbyBed(player, look)) {
                input.interactQueued = false;
                dirty = true;
            } else if (toggleDoorInLook(player, look) || toggleNearbyDoor(player, look)) {
                input.interactQueued = false;
                dirty = true;
            } else if (eatSelected(inventory, stats) || drinkSelected(inventory, stats) ||
                       milkNearbyCow(player, cows, inventory)) {
                input.interactQueued = false;
                dirty = true;
            } else {
                input.interactQueued = false;
                dirty = true;
            }
            input.craftAllQueued = false;
        }

        if (!inventory.open && (mouse.attackQueued || input.attackQueued)) {
            if (tick - lastAttackTick < attackCooldownTicks) {
                mouse.attackQueued = false;
                input.attackQueued = false;
            } else {
                lastAttackTick = tick;
                bool brokeBlock = false;
                ItemType tool = inventory.slots[inventory.selectedSlot].item;
                if (tool == ItemType::Spear)
                    brokeBlock = throwSpear(player, look, cows, inventory);
                if (mouse.known) {
                    Position target = screenToWorld(mouse.x, mouse.y, player);
                    if (!brokeBlock)
                        brokeBlock = breedCowAt(player, target, cows, inventory);
                    if (!brokeBlock)
                        brokeBlock = attackSpiderBlockAt(player, target, spiders);
                    if (!brokeBlock)
                        brokeBlock = attackSpiderTowardTarget(player, target, spiders);
                    if (!brokeBlock)
                        brokeBlock = attackCowBlockAt(player, target, cows, inventory);
                    if (!brokeBlock)
                        brokeBlock = attackCowTowardTarget(player, target, cows, inventory);
                    if (!brokeBlock)
                        brokeBlock = attackSheepBlockAt(player, target, sheep, inventory);
                    if (!brokeBlock)
                        brokeBlock = attackSheepTowardTarget(player, target, sheep, inventory);
                    if (!brokeBlock)
                        brokeBlock = attackBlockAt(player, target, tool, &inventory);
                    if (!brokeBlock)
                        brokeBlock = attackTowardTarget(player, target, tool, &inventory);
                }

                if (!brokeBlock)
                    brokeBlock = attackSpiderInLookDirection(player, look, spiders);
                if (!brokeBlock)
                    brokeBlock = attackCowInLookDirection(player, look, cows, inventory);
                if (!brokeBlock)
                    brokeBlock = attackSheepInLookDirection(player, look, sheep, inventory);
                if (!brokeBlock)
                    brokeBlock = attackInLookDirection(player, look, tool, &inventory);

                mouse.attackQueued = false;
                input.attackQueued = false;
                dirty = true;
            }
        } else if (inventory.open) {
            mouse.attackQueued = false;
            input.attackQueued = false;
        }

        if (inventory.open && move.moved) {
            if (inventory.craftMenuOpen) {
                std::vector<Action> recipes = buildCraftActions(inventory, player);
                moveCraftCursor(inventory, move, static_cast<int>(recipes.size()));
            } else if (inventory.actionMenuOpen) {
                std::vector<Action> actions = buildActions(inventory, player, cows);
                moveActionCursor(inventory, move, static_cast<int>(actions.size()));
            } else {
                moveInventoryCursor(inventory, move);
            }
            consumeMoveInput(input, move);
            dirty = true;
        } else if (move.moved && tick - lastMoveTick >= moveDelayFor(player, move)) {
            Position oldPlayer = player;
            if (tryMove(player, move)) {
                visual.playerPrevious = oldPlayer;
                visual.playerMoveStartedTick = tick;
                if (pickupAtPlayer(player, inventory))
                    dirty = true;
                lastMoveTick = tick;
                dirty = true;
            }
            consumeMoveInput(input, move);
        }

        if (dirty && tick % renderEveryTicks == 0) {
            drawWorld(player, look, inventory, cows, spiders, sheep, stats, chat, visual, network, tick);
            dirty = false;
        }

        tick++;
        nextTick += tickDuration;
        std::this_thread::sleep_until(nextTick);

        if (Clock::now() > nextTick + tickDuration)
            nextTick = Clock::now();
    }

    if (network.mode != NetworkMode::Client)
        saveGame(player, look, inventory, stats, cows, spiders, sheep, tick, lastMoveTick, lastAttackTick);
    closeNetworkSocket(network);
    disableMouseTracking();
    endwin();
    return 0;
}
