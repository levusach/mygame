void updateLookFromMouse(const Position& player, int mouseX, int mouseY, LookDirection& look) {
    Position target = screenToWorld(mouseX, mouseY, player);
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;

    if (dx == 0 && dy == 0)
        return;

    look.dx = sign(dx);
    look.dy = sign(dy);
}

void updateLookFromMouseState(const Position& player, const MouseState& mouse, LookDirection& look) {
    if (!mouse.known)
        return;

    updateLookFromMouse(player, mouse.x, mouse.y, look);
}

void handleMouseEvent(MouseState& mouse) {
    MEVENT event;
    if (getmouse(&event) != OK)
        return;

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);
    int worldBottom = height - bottomRows;
    if (event.x < 0 || event.x >= width || event.y < 0 || event.y >= worldBottom)
        return;

    mouse.x = event.x;
    mouse.y = event.y;
    mouse.known = true;

    auto now = Clock::now();
    auto sinceAttack = std::chrono::duration_cast<std::chrono::milliseconds>(now - mouse.lastAttack);
    if ((event.bstate & BUTTON3_CLICKED) || (event.bstate & BUTTON3_PRESSED)) {
        mouse.placeQueued = true;
        return;
    }

    if ((event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_RELEASED)) &&
        sinceAttack.count() >= attackDebounceMs) {
        mouse.attackQueued = true;
        mouse.lastAttack = now;
    }
}

void applyMouseButton(MouseState& mouse, int button, bool pressed) {
    int baseButton = button & 3;

    if (pressed && baseButton == 2) {
        mouse.placeQueued = true;
        return;
    }

    if (baseButton == 0) {
        mouse.attackQueued = true;
        mouse.lastAttack = Clock::now();
    }
}

bool parseSgrMouseSequence(const std::vector<int>& sequence, MouseState& mouse) {
    if (sequence.size() < 6 || sequence[0] != '[' || sequence[1] != '<')
        return false;

    int button = 0;
    int x = 0;
    int y = 0;
    size_t index = 2;

    auto readNumber = [&](int& out) {
        if (index >= sequence.size() || sequence[index] < '0' || sequence[index] > '9')
            return false;

        out = 0;
        while (index < sequence.size() && sequence[index] >= '0' && sequence[index] <= '9') {
            out = out * 10 + (sequence[index] - '0');
            index++;
        }
        return true;
    };

    if (!readNumber(button) || index >= sequence.size() || sequence[index++] != ';')
        return false;
    if (!readNumber(x) || index >= sequence.size() || sequence[index++] != ';')
        return false;
    if (!readNumber(y) || index >= sequence.size())
        return false;

    int suffix = sequence[index];
    if (suffix != 'M' && suffix != 'm')
        return false;

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);
    int sx = x - 1;
    int sy = y - 1;
    int worldBottom = height - bottomRows;
    if (sx < 0 || sx >= width || sy < 0 || sy >= worldBottom)
        return true;

    mouse.x = sx;
    mouse.y = sy;
    mouse.known = true;

    bool pressed = suffix == 'M';
    bool motion = (button & 32) != 0;
    int baseButton = button & 3;
    if (!motion || baseButton == 0 || baseButton == 2)
        applyMouseButton(mouse, button, pressed);

    return true;
}

bool parseX10MouseSequence(const std::vector<int>& sequence, MouseState& mouse) {
    if (sequence.size() < 5 || sequence[0] != '[' || sequence[1] != 'M')
        return false;

    int button = sequence[2] - 32;
    int sx = sequence[3] - 33;
    int sy = sequence[4] - 33;

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);
    int worldBottom = height - bottomRows;
    if (sx < 0 || sx >= width || sy < 0 || sy >= worldBottom)
        return true;

    mouse.x = sx;
    mouse.y = sy;
    mouse.known = true;

    bool release = (button & 3) == 3;
    if (!release)
        applyMouseButton(mouse, button, true);

    return true;
}

bool parseUrxvtMouseSequence(const std::vector<int>& sequence, MouseState& mouse) {
    if (sequence.size() < 6 || sequence[0] != '[' || sequence[1] == '<' || sequence[1] == 'M')
        return false;

    int button = 0;
    int x = 0;
    int y = 0;
    size_t index = 1;

    auto readNumber = [&](int& out) {
        if (index >= sequence.size() || sequence[index] < '0' || sequence[index] > '9')
            return false;

        out = 0;
        while (index < sequence.size() && sequence[index] >= '0' && sequence[index] <= '9') {
            out = out * 10 + (sequence[index] - '0');
            index++;
        }
        return true;
    };

    if (!readNumber(button) || index >= sequence.size() || sequence[index++] != ';')
        return false;
    if (!readNumber(x) || index >= sequence.size() || sequence[index++] != ';')
        return false;
    if (!readNumber(y) || index >= sequence.size() || sequence[index] != 'M')
        return false;

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);
    int sx = x - 1;
    int sy = y - 1;
    int worldBottom = height - bottomRows;
    if (sx < 0 || sx >= width || sy < 0 || sy >= worldBottom)
        return true;

    mouse.x = sx;
    mouse.y = sy;
    mouse.known = true;
    applyMouseButton(mouse, button, true);
    return true;
}

bool handleEscapeSequence(MouseState& mouse) {
    std::vector<int> sequence;
    int key = 0;

    timeout(20);
    for (int i = 0; i < 32; ++i) {
        key = getch();
        if (key == ERR)
            break;

        sequence.push_back(key);
        if ((sequence.size() >= 2 && sequence[0] == '[' && sequence[1] == 'M' && sequence.size() >= 5) ||
            (sequence.size() >= 2 && sequence[0] == '[' && sequence[1] == '<' && (key == 'M' || key == 'm')) ||
            key == '~')
            break;
    }
    timeout(0);

    if (sequence.empty())
        return false;

    bool parsed = parseSgrMouseSequence(sequence, mouse) ||
                  parseX10MouseSequence(sequence, mouse) ||
                  parseUrxvtMouseSequence(sequence, mouse);
    return parsed;
}

void handleChatKey(ChatState& chat, int key) {
    if (key == 27) {
        chat.open = false;
        chat.input.clear();
        return;
    }

    if (key == '\n' || key == '\r' || key == KEY_ENTER) {
        chat.pendingLine = chat.input;
        chat.input.clear();
        chat.open = false;
        return;
    }

    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        if (!chat.input.empty()) {
            chat.input.pop_back();
            while (!chat.input.empty() &&
                   (static_cast<unsigned char>(chat.input.back()) & 0xc0) == 0x80)
                chat.input.pop_back();
        }
        return;
    }

    if (key >= 32 && key <= 255 && chat.input.size() < 360)
        chat.input.push_back(static_cast<char>(key));
}

void addKeyToHeldInput(int key, HeldInput& input, Clock::time_point now) {
    if (key == 'q' || key == 'Q') {
        input.quit = true;
        return;
    }

    if (key == ' ') {
        input.attackQueued = true;
        return;
    }

    if (key == 'i' || key == 'I') {
        input.inventoryToggleQueued = true;
        return;
    }

    if (key == 't' || key == 'T') {
        return;
    }

    if (key == 'c' || key == 'C') {
        input.craftToggleQueued = true;
        return;
    }

    if (key == '\n' || key == '\r' || key == KEY_ENTER || key == 'e' || key == 'E') {
        input.craftAllQueued = key == 'E';
        input.interactQueued = true;
        return;
    }

    if (key >= '1' && key <= '9') {
        input.selectedSlotQueued = key - '1';
        return;
    }

    if (key == KEY_UP || key == 'w' || key == 'W') {
        refreshHeldKey(input.up, now);
        input.queuedMove.dy = -1;
        input.queuedMove.moved = true;
    } else if (key == KEY_DOWN || key == 's' || key == 'S') {
        refreshHeldKey(input.down, now);
        input.queuedMove.dy = 1;
        input.queuedMove.moved = true;
    } else if (key == KEY_LEFT || key == 'a' || key == 'A') {
        refreshHeldKey(input.left, now);
        input.queuedMove.dx = -1;
        input.queuedMove.moved = true;
    } else if (key == KEY_RIGHT || key == 'd' || key == 'D') {
        refreshHeldKey(input.right, now);
        input.queuedMove.dx = 1;
        input.queuedMove.moved = true;
    } else if (key == KEY_A1 || key == 'y' || key == 'Y') {
        refreshHeldKey(input.up, now);
        refreshHeldKey(input.left, now);
        input.queuedMove.dy = -1;
        input.queuedMove.dx = -1;
        input.queuedMove.moved = true;
    } else if (key == KEY_A3 || key == 'u' || key == 'U') {
        refreshHeldKey(input.up, now);
        refreshHeldKey(input.right, now);
        input.queuedMove.dy = -1;
        input.queuedMove.dx = 1;
        input.queuedMove.moved = true;
    } else if (key == KEY_C1 || key == 'b' || key == 'B') {
        refreshHeldKey(input.down, now);
        refreshHeldKey(input.left, now);
        input.queuedMove.dy = 1;
        input.queuedMove.dx = -1;
        input.queuedMove.moved = true;
    } else if (key == KEY_C3 || key == 'n' || key == 'N') {
        refreshHeldKey(input.down, now);
        refreshHeldKey(input.right, now);
        input.queuedMove.dy = 1;
        input.queuedMove.dx = 1;
        input.queuedMove.moved = true;
    }
}

void readHeldInput(HeldInput& input, MouseState& mouse, ChatState& chat) {
    auto now = Clock::now();
    int key = 0;

    while ((key = getch()) != ERR) {
        if (chat.open) {
            handleChatKey(chat, key);
            continue;
        }

        if (key == KEY_MOUSE) {
            handleMouseEvent(mouse);
            continue;
        }

        if (key == 27 && handleEscapeSequence(mouse))
            continue;

        if (key == 't' || key == 'T') {
            chat.open = true;
            chat.input.clear();
            continue;
        }

        addKeyToHeldInput(key, input, now);
    }
}

bool isHeld(Clock::time_point pressedAt, Clock::time_point now) {
    if (pressedAt == Clock::time_point::min())
        return false;

    return pressedAt >= now;
}

MoveInput currentMove(const HeldInput& input) {
    MoveInput move = input.queuedMove;
    auto now = Clock::now();
    if (!move.moved) {
        if (isHeld(input.left, now) && !isHeld(input.right, now))
            move.dx = -1;
        else if (isHeld(input.right, now) && !isHeld(input.left, now))
            move.dx = 1;

        if (isHeld(input.up, now) && !isHeld(input.down, now))
            move.dy = -1;
        else if (isHeld(input.down, now) && !isHeld(input.up, now))
            move.dy = 1;

        move.moved = move.dx != 0 || move.dy != 0;
    }
    move.quit = input.quit;
    return move;
}

void moveInventoryCursor(Inventory& inventory, const MoveInput& move) {
    if (inventory.actionMenuOpen || inventory.craftMenuOpen)
        return;

    if (inventory.furnaceOpen) {
        if (inventory.cursor >= inventorySlots) {
            int local = inventory.cursor - inventorySlots;
            if (move.dx < 0)
                inventory.cursor = inventory.selectedSlot;
            else if (move.dy < 0)
                inventory.cursor = inventorySlots;
            else if (move.dy > 0)
                inventory.cursor = inventorySlots + 1;
            else
                inventory.cursor = inventorySlots + (local == 0 ? 0 : 1);
            return;
        }

        int row = inventory.cursor / inventoryColumns;
        int col = inventory.cursor % inventoryColumns;
        int rows = (inventorySlots + inventoryColumns - 1) / inventoryColumns;

        if (move.dx < 0 && col > 0)
            col--;
        else if (move.dx > 0 && col < inventoryColumns - 1)
            col++;
        else if (move.dx > 0)
            inventory.cursor = inventorySlots;
        if (inventory.cursor == inventorySlots)
            return;

        if (move.dy < 0 && row > 0)
            row--;
        if (move.dy > 0 && row < rows - 1)
            row++;

        int next = row * inventoryColumns + col;
        if (next >= inventorySlots)
            next = inventorySlots - 1;
        inventory.cursor = next;
        return;
    }

    if (!inventory.chestOpen) {
        int row = inventory.cursor / inventoryColumns;
        int col = inventory.cursor % inventoryColumns;
        int rows = (inventorySlots + inventoryColumns - 1) / inventoryColumns;

        if (move.dx < 0 && col > 0) col--;
        if (move.dx > 0 && col < inventoryColumns - 1) col++;
        if (move.dy < 0 && row > 0) row--;
        if (move.dy > 0 && row < rows - 1) row++;

        int next = row * inventoryColumns + col;
        if (next >= inventorySlots)
            next = inventorySlots - 1;
        inventory.cursor = next;
        return;
    }

    constexpr int chestColumns = 3;
    bool inChest = inventory.cursor >= inventorySlots;
    int local = inChest ? inventory.cursor - inventorySlots : inventory.cursor;
    int columns = inChest ? chestColumns : inventoryColumns;
    int slots = inChest ? chestSlots : inventorySlots;
    int row = local / columns;
    int col = local % columns;
    int rows = (slots + columns - 1) / columns;

    if (move.dx < 0) {
        if (col > 0)
            col--;
        else if (inChest) {
            int invRow = row;
            int invCol = inventoryColumns - 1;
            int next = invRow * inventoryColumns + invCol;
            if (next >= inventorySlots)
                next = inventorySlots - 1;
            inventory.cursor = next;
            return;
        }
    }
    if (move.dx > 0) {
        if (col < columns - 1 && local + 1 < slots)
            col++;
        else if (!inChest) {
            int chestRow = row;
            int next = chestRow * chestColumns;
            if (next >= chestSlots)
                next = chestSlots - 1;
            inventory.cursor = inventorySlots + next;
            return;
        }
    }
    if (move.dy < 0 && row > 0)
        row--;
    if (move.dy > 0 && row < rows - 1)
        row++;

    int next = row * columns + col;
    if (next >= slots)
        next = slots - 1;
    inventory.cursor = inChest ? inventorySlots + next : next;
}

void moveActionCursor(Inventory& inventory, const MoveInput& move, int actionCount) {
    if (!inventory.actionMenuOpen || actionCount <= 0)
        return;

    if (move.dy < 0 && inventory.actionCursor > 0)
        inventory.actionCursor--;
    if (move.dy > 0 && inventory.actionCursor < actionCount - 1)
        inventory.actionCursor++;
}

void moveCraftCursor(Inventory& inventory, const MoveInput& move, int recipeCount) {
    if (!inventory.craftMenuOpen || recipeCount <= 0)
        return;

    if (move.dy < 0 && inventory.craftCursor > 0)
        inventory.craftCursor--;
    if (move.dy > 0 && inventory.craftCursor < recipeCount - 1)
        inventory.craftCursor++;
}
