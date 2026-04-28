void initColors() {
    if (!has_colors())
        return;

    start_color();
    use_default_colors();
    init_pair(C_EMPTY, COLOR_GREEN, -1);
    init_pair(C_COBBLESTONE, COLOR_BLACK, -1);
    init_pair(C_WATER, COLOR_BLUE, -1);
    init_pair(C_BEACH, COLOR_YELLOW, -1);
    init_pair(C_TREE, COLOR_GREEN, -1);
    init_pair(C_COW, COLOR_WHITE, -1);
    init_pair(C_PLAYER, COLOR_GREEN, -1);
    init_pair(C_STATUS, COLOR_WHITE, COLOR_BLACK);
    init_pair(C_DAMAGE_1, COLOR_WHITE, -1);
    init_pair(C_DAMAGE_2, COLOR_WHITE, -1);
    init_pair(C_WORKBENCH, COLOR_YELLOW, -1);
    init_pair(C_FURNACE, COLOR_RED, -1);
    init_pair(C_DOOR, COLOR_CYAN, -1);
    init_pair(C_COW_BLACK, COLOR_BLACK, -1);
    init_pair(C_SHEEP_BLACK, COLOR_BLACK, -1);
    init_pair(C_BED, COLOR_MAGENTA, -1);
    init_pair(C_BAG, COLOR_YELLOW, -1);
    init_pair(C_POISON, COLOR_MAGENTA, -1);
}

void enableMouseTracking() {
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mouseinterval(0);
    std::printf("\033[?1000h\033[?1003h\033[?1006h");
    std::fflush(stdout);
}

void disableMouseTracking() {
    std::printf("\033[?1006l\033[?1003l\033[?1000l");
    std::fflush(stdout);
}

char lookGlyph(const LookDirection& look) {
    if (look.dx == 0 && look.dy < 0)
        return '^';
    if (look.dx == 0 && look.dy > 0)
        return 'v';
    if (look.dx < 0 && look.dy == 0)
        return '<';
    if (look.dx > 0 && look.dy == 0)
        return '>';
    if (look.dx == look.dy)
        return '\\';
    return '/';
}

const char* selectedItemName(const Inventory& inventory) {
    return itemName(inventory.slots[inventory.selectedSlot].item);
}

int selectedItemCount(const Inventory& inventory) {
    return inventory.slots[inventory.selectedSlot].count;
}

void drawRoundedBox(int y, int x, int height, int width) {
    if (height < 2 || width < 2)
        return;

    mvaddch(y, x, useUnicode ? ACS_ULCORNER : '+');
    mvhline(y, x + 1, ACS_HLINE, width - 2);
    mvaddch(y, x + width - 1, useUnicode ? ACS_URCORNER : '+');

    for (int row = 1; row < height - 1; ++row) {
        mvaddch(y + row, x, ACS_VLINE);
        mvhline(y + row, x + 1, ' ', width - 2);
        mvaddch(y + row, x + width - 1, ACS_VLINE);
    }

    mvaddch(y + height - 1, x, useUnicode ? ACS_LLCORNER : '+');
    mvhline(y + height - 1, x + 1, ACS_HLINE, width - 2);
    mvaddch(y + height - 1, x + width - 1, useUnicode ? ACS_LRCORNER : '+');
}

void drawTopHud(const Inventory& inventory, const PlayerStats& stats, long long tick) {
    int width = 0;
    width = getmaxx(stdscr);

    int boxWidth = width < 34 ? width - 2 : 32;
    int boxHeight = 5;
    if (boxWidth < 24)
        return;

    int x = 2;
    int y = 1;
    drawRoundedBox(y, x, boxHeight, boxWidth);
    mvprintw(y + 1, x + 2, "Tool: %s", selectedItemName(inventory));

    if (inventory.slots[inventory.selectedSlot].item != ItemType::None)
        mvprintw(y + 2, x + 2, "Amount: %d", selectedItemCount(inventory));
    else
        mvprintw(y + 2, x + 2, "Amount: -");
    if (stats.poisonTicks > 0)
        mvprintw(y + 3, x + 2, "%s  Poison %ds", dayPhase(tick), (stats.poisonTicks + tickRate - 1) / tickRate);
    else
        mvprintw(y + 3, x + 2, "%s", dayPhase(tick));
}

void drawStatusBar(int y, int x, const char* label, int value, int maxValue, int barWidth) {
    if (barWidth < 4 || maxValue <= 0)
        return;

    if (value < 0)
        value = 0;
    if (value > maxValue)
        value = maxValue;

    int filled = (value * barWidth + maxValue - 1) / maxValue;
    mvprintw(y, x, "%s [", label);
    int barX = x + static_cast<int>(std::strlen(label)) + 2;
    for (int i = 0; i < barWidth; ++i)
        mvaddch(y, barX + i, i < filled ? '#' : '-');
    mvprintw(y, barX + barWidth, "] %02d/%02d", value, maxValue);
}

void drawSurvivalBars(const PlayerStats& stats, int y, int width) {
    int barWidth = width >= 100 ? 18 : 12;
    int hpWidth = static_cast<int>(std::strlen("HP")) + barWidth + 9;
    int hungerWidth = static_cast<int>(std::strlen("FOOD")) + barWidth + 9;
    int thirstWidth = static_cast<int>(std::strlen("WATER")) + barWidth + 9;

    drawStatusBar(y, 1, "HP", stats.hearts, 10, barWidth);

    int hungerX = (width - hungerWidth) / 2;
    if (hungerX > hpWidth + 2)
        drawStatusBar(y, hungerX, "FOOD", stats.hunger, 20, barWidth);

    int thirstX = width - thirstWidth - 1;
    if (thirstX > hungerX + hungerWidth + 2 || hungerX <= hpWidth + 2)
        drawStatusBar(y, thirstX, "WATER", stats.thirst, 20, barWidth);
}

void drawHotbar(const Inventory& inventory, int y, int width) {
    int slotWidth = 12;
    int firstRowSlots = 5;
    int firstRowWidth = firstRowSlots * slotWidth;
    int secondRowWidth = (hotbarSlots - firstRowSlots) * slotWidth;
    int firstX = (width - firstRowWidth) / 2;
    int secondX = (width - secondRowWidth) / 2;
    if (firstX < 0) firstX = 0;
    if (secondX < 0) secondX = 0;

    for (int i = 0; i < hotbarSlots; ++i) {
        const Slot& slot = inventory.slots[i];
        int row = i < firstRowSlots ? 0 : 1;
        int col = i < firstRowSlots ? i : i - firstRowSlots;
        int x = (row == 0 ? firstX : secondX) + col * slotWidth;

        if (i == inventory.selectedSlot)
            attron(A_REVERSE);

        if (slot.item == ItemType::None)
            mvprintw(y + row, x, "[%d Empty]", i + 1);
        else {
            mvprintw(y + row, x, "[%d ", i + 1);
            mvaddwstr(y + row, x + 3, itemIcon(slot.item));
            mvprintw(y + row, x + 5, "%02d]", slot.count);
        }

        if (i == inventory.selectedSlot)
            attroff(A_REVERSE);
    }
}

std::string clippedName(ItemType item, int maxLength) {
    std::string name = itemName(item);
    if (static_cast<int>(name.size()) <= maxLength)
        return name;
    if (maxLength <= 1)
        return name.substr(0, maxLength);
    return name.substr(0, maxLength - 1) + ".";
}

void drawInventoryOverlay(const Inventory& inventory, const Position& player, const std::vector<Cow>& cows) {
    if (!inventory.open)
        return;

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    int cellWidth = width >= 92 ? 15 : 12;
    int nameWidth = cellWidth - 7;
    int invPanelWidth = inventoryColumns * cellWidth + 4;
    int chestColumns = 3;
    int chestPanelWidth = chestColumns * cellWidth + 4;
    int furnacePanelWidth = cellWidth + 14;
    bool sidePanelOpen = inventory.chestOpen || inventory.furnaceOpen;
    int gap = sidePanelOpen ? 3 : 0;
    int sidePanelWidth = inventory.chestOpen ? chestPanelWidth : (inventory.furnaceOpen ? furnacePanelWidth : 0);
    int boxWidth = sidePanelOpen ? invPanelWidth + sidePanelWidth + gap : invPanelWidth;
    if (boxWidth > width - 2)
        boxWidth = width - 2;
    int inventoryRows = (inventorySlots + inventoryColumns - 1) / inventoryColumns;
    int chestRows = (chestSlots + chestColumns - 1) / chestColumns;
    int shownRows = inventory.chestOpen ? std::max(inventoryRows, chestRows) : inventoryRows;
    int boxHeight = shownRows * 2 + 5;
    if (boxWidth < 24 || height < boxHeight + 2)
        return;

    int y = (height - boxHeight) / 2;
    int x = (width - boxWidth) / 2;

    drawRoundedBox(y, x, boxHeight, boxWidth);
    mvprintw(y + 1, x + 2, "Inventory");
    int sideX = x + invPanelWidth + gap;
    if (sidePanelOpen) {
        mvvline(y + 1, x + invPanelWidth + gap / 2, ACS_VLINE, boxHeight - 2);
        mvprintw(y + 1, sideX + 2, inventory.chestOpen ? "Chest" : "Furnace");
    }
    if (inventory.held.item == ItemType::None)
        mvprintw(y + 1, x + boxWidth - 16, "Held: Empty");
    else {
        int heldX = x + boxWidth - 18;
        mvprintw(y + 1, heldX, "Held: ");
        mvaddwstr(y + 1, heldX + 6, itemIcon(inventory.held.item));
        mvprintw(y + 1, heldX + 8, " %02d", inventory.held.count);
    }

    std::array<Slot, chestSlots> emptyChest{};
    const auto& chestSlotsRef = inventory.chestOpen ? chests[inventory.activeChest] : emptyChest;
    Slot emptySlot;
    const Slot& selectedSlot = inventory.cursor < inventorySlots
        ? inventory.slots[inventory.cursor]
        : (inventory.chestOpen ? chestSlotsRef[inventory.cursor - inventorySlots] :
           (inventory.furnaceOpen && inventory.cursor == inventorySlots ? furnaces[inventory.activeFurnace].slot :
            (inventory.furnaceOpen && inventory.cursor == inventorySlots + 1 ? furnaces[inventory.activeFurnace].fuel : emptySlot)));
    if (selectedSlot.item != ItemType::None)
        mvprintw(y + 2, x + 2, "Selected: %s x%d", itemName(selectedSlot.item), selectedSlot.count);

    for (int i = 0; i < inventorySlots; ++i) {
        int row = i / inventoryColumns;
        int col = i % inventoryColumns;
        int cx = x + 2 + col * cellWidth;
        int cy = y + 3 + row * 2;
        const Slot& slot = inventory.slots[i];

        if (i == inventory.cursor)
            attron(A_REVERSE);

        if (slot.item == ItemType::None)
            mvprintw(cy, cx, "[ %-*s ]", nameWidth, "Empty");
        else {
            std::string name = clippedName(slot.item, nameWidth);
            mvprintw(cy, cx, "[%-*s %02d]", nameWidth, name.c_str(), slot.count);
        }

        if (i == inventory.cursor)
            attroff(A_REVERSE);
    }

    if (inventory.chestOpen) {
        for (int i = 0; i < chestSlots; ++i) {
            int globalIndex = inventorySlots + i;
            int row = i / chestColumns;
            int col = i % chestColumns;
            int cx = sideX + 2 + col * cellWidth;
            int cy = y + 3 + row * 2;
            const Slot& slot = chestSlotsRef[i];

            if (globalIndex == inventory.cursor)
                attron(A_REVERSE);

            if (slot.item == ItemType::None)
                mvprintw(cy, cx, "[ %-*s ]", nameWidth, "Empty");
            else {
                std::string name = clippedName(slot.item, nameWidth);
                mvprintw(cy, cx, "[%-*s %02d]", nameWidth, name.c_str(), slot.count);
            }

            if (globalIndex == inventory.cursor)
                attroff(A_REVERSE);
        }
    }

    if (inventory.furnaceOpen) {
        const FurnaceState& furnace = furnaces[inventory.activeFurnace];
        int cx = sideX + 2;
        int cy = y + 3;
        mvprintw(cy - 1, cx, "Input");
        if (inventory.cursor == inventorySlots)
            attron(A_REVERSE);
        if (furnace.slot.item == ItemType::None)
            mvprintw(cy, cx, "[ %-*s ]", nameWidth, "Empty");
        else {
            std::string name = clippedName(furnace.slot.item, nameWidth);
            mvprintw(cy, cx, "[%-*s %02d]", nameWidth, name.c_str(), furnace.slot.count);
        }
        if (inventory.cursor == inventorySlots)
            attroff(A_REVERSE);
        mvprintw(cy + 2, cx, "Fuel");
        if (inventory.cursor == inventorySlots + 1)
            attron(A_REVERSE);
        if (furnace.fuel.item == ItemType::None)
            mvprintw(cy + 3, cx, "[ %-*s ]", nameWidth, "Empty");
        else {
            std::string name = clippedName(furnace.fuel.item, nameWidth);
            mvprintw(cy + 3, cx, "[%-*s %02d]", nameWidth, name.c_str(), furnace.fuel.count);
        }
        if (inventory.cursor == inventorySlots + 1)
            attroff(A_REVERSE);
        mvprintw(cy + 5, cx, "Progress: %3d%%", furnace.slot.item == ItemType::None ? 0 :
                 (furnace.progress * 100 / furnaceProcessTicks));
        mvprintw(cy + 6, cx, "Burn: %3d%%", furnace.burnTicks <= 0 ? 0 : std::min(100, furnace.burnTicks * 100 / (tickRate * 10)));
    }

    mvprintw(y + boxHeight - 2, x + 2, "E move stack  C craft  arrows/WASD move  I close");

    if (inventory.craftMenuOpen) {
        std::vector<Action> actions = buildCraftActions(inventory, player);
        int menuWidth = 34;
        int menuHeight = static_cast<int>(actions.size()) + 3;
        if (menuHeight < 5)
            menuHeight = 5;

        int menuX = x + boxWidth - menuWidth - 2;
        int menuY = y + 2;
        if (menuX < 1)
            menuX = 1;
        drawRoundedBox(menuY, menuX, menuHeight, menuWidth);

        if (actions.empty()) {
            mvprintw(menuY + 1, menuX + 2, "No recipes");
        } else {
            for (size_t i = 0; i < actions.size(); ++i) {
                if (static_cast<int>(i) == inventory.craftCursor)
                    attron(A_REVERSE);
                mvprintw(menuY + 1 + static_cast<int>(i), menuX + 2, "%s", actions[i].label.c_str());
                if (static_cast<int>(i) == inventory.craftCursor)
                    attroff(A_REVERSE);
            }
            mvprintw(menuY + menuHeight - 2, menuX + 2, "E one  Shift+E all");
        }
    } else if (inventory.actionMenuOpen) {
        std::vector<Action> actions = buildActions(inventory, player, cows);
        int menuWidth = 34;
        int menuHeight = static_cast<int>(actions.size()) + 2;
        if (menuHeight < 4)
            menuHeight = 4;

        int menuX = x + boxWidth - menuWidth - 2;
        int menuY = y + 2;
        if (menuX < 1)
            menuX = 1;
        drawRoundedBox(menuY, menuX, menuHeight, menuWidth);

        if (actions.empty()) {
            mvprintw(menuY + 1, menuX + 2, "No actions");
        } else {
            for (size_t i = 0; i < actions.size(); ++i) {
                if (static_cast<int>(i) == inventory.actionCursor)
                    attron(A_REVERSE);
                mvprintw(menuY + 1 + static_cast<int>(i), menuX + 2, "%s", actions[i].label.c_str());
                if (static_cast<int>(i) == inventory.actionCursor)
                    attroff(A_REVERSE);
            }
        }
    }
}

chtype tileAttrs(char tile, const Position& position, short colorPair, chtype baseAttrs) {
    int damage = isBreakable(tile) ? damageAt(position) : 0;
    if (damage == 1)
        return has_colors() ? COLOR_PAIR(C_DAMAGE_1) : A_NORMAL;
    if (damage >= 2)
        return has_colors() ? (COLOR_PAIR(C_DAMAGE_2) | A_BOLD) : A_BOLD;

    return has_colors() ? (COLOR_PAIR(colorPair) | baseAttrs) : baseAttrs;
}

int daySlice(long long tick) {
    long long phase = dayPhaseTick(tick);
    return static_cast<int>(phase * 8 / dayCycleTicks);
}

chtype atmosphereAttrs(long long tick, chtype baseAttrs) {
    int slice = daySlice(tick);
    if (slice == 0 || slice == 4 || slice == 5)
        return baseAttrs | A_NORMAL;
    if (slice == 6 || slice == 7)
        return (baseAttrs & ~A_BOLD) | A_DIM;
    return baseAttrs | A_BOLD;
}

char skyGlyphAt(const Position& position, long long tick) {
    int slice = daySlice(tick);
    if (slice == 0)
        return ',';
    if (slice >= 1 && slice <= 3)
        return '.';
    if (slice == 4)
        return '`';
    if (slice == 5)
        return '\'';

    uint64_t stars = mix(static_cast<uint64_t>(position.x) * 0x9e3779b97f4a7c15ULL ^
                         static_cast<uint64_t>(position.y) * 0xc2b2ae3d27d4eb4fULL ^
                         static_cast<uint64_t>(tick / (tickRate * 2)));
    return stars % 23 == 0 ? '*' : ' ';
}

void drawTile(int y, int x, const Position& position, char tile, long long tick) {
    char drawnTile = tile == '.' ? skyGlyphAt(position, tick) : tile;
    if (has_colors()) {
        if (tile == '#') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_WORKBENCH, A_BOLD));
            attron(attrs);
            if (useUnicode)
                mvaddwstr(y, x, L"⚒");
            else
                mvaddch(y, x, '#');
            attroff(attrs);
            return;
        }

        if (tile == 'F') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_FURNACE, A_BOLD));
            attron(attrs);
            if (useUnicode)
                mvaddwstr(y, x, L"♨");
            else
                mvaddch(y, x, 'F');
            attroff(attrs);
            return;
        }

        if (tile == 'D' || tile == 'd') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_DOOR, A_BOLD));
            attron(attrs);
            if (useUnicode)
                mvaddwstr(y, x, tile == 'D' ? L"▥" : L"▯");
            else
                mvaddch(y, x, tile);
            attroff(attrs);
            return;
        }

        if (tile == 'C') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_WORKBENCH, A_BOLD));
            attron(attrs);
            mvaddch(y, x, 'C');
            attroff(attrs);
            return;
        }

        if (tile == 'M') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_BAG, A_BOLD));
            attron(attrs);
            mvaddch(y, x, 'M');
            attroff(attrs);
            return;
        }

        if (tile == 'B') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_BED, A_BOLD));
            attron(attrs);
            mvaddch(y, x, 'Z');
            attroff(attrs);
            return;
        }

        if (tile == 'f') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_DOOR, A_BOLD));
            attron(attrs);
            mvaddch(y, x, 'H');
            attroff(attrs);
            return;
        }

        if (tile == '0' || tile == 'i') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_COBBLESTONE, A_BOLD));
            attron(attrs);
            mvaddch(y, x, tile == 'i' ? 'o' : tile);
            attroff(attrs);
            return;
        }

        if (tile == '~') {
            chtype attrs = atmosphereAttrs(tick, COLOR_PAIR(C_WATER) | A_BOLD);
            attron(attrs);
            mvaddch(y, x, tile);
            attroff(attrs);
            return;
        }

        if (tile == ':') {
            chtype attrs = atmosphereAttrs(tick, COLOR_PAIR(C_BEACH) | A_DIM);
            attron(attrs);
            mvaddch(y, x, tile);
            attroff(attrs);
            return;
        }

        if (tile == 'T') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_TREE, A_BOLD));
            attron(attrs);
            mvaddch(y, x, tile);
            attroff(attrs);
            return;
        }

        if (tile == 'w') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_TREE, A_BOLD));
            attron(attrs);
            mvaddch(y, x, 'w');
            attroff(attrs);
            return;
        }

        if (tile == 'W') {
            chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_BEACH, A_BOLD));
            attron(attrs);
            mvaddch(y, x, 'W');
            attroff(attrs);
            return;
        }

        chtype attrs = atmosphereAttrs(tick, COLOR_PAIR(C_EMPTY) | A_DIM);
        attron(attrs);
        mvaddch(y, x, drawnTile);
        attroff(attrs);
        return;
    }

    if (isBreakable(tile)) {
        chtype attrs = atmosphereAttrs(tick, tileAttrs(tile, position, C_COBBLESTONE, A_NORMAL));
        attron(attrs);
        mvaddch(y, x, tile);
        attroff(attrs);
        return;
    }

    chtype attrs = atmosphereAttrs(tick, A_NORMAL);
    attron(attrs);
    mvaddch(y, x, drawnTile);
    attroff(attrs);
}

void drawCow(int y, int x) {
    if (has_colors())
        attron(COLOR_PAIR(C_COW_BLACK) | A_BOLD);
    mvaddch(y, x - 1, '&');
    if (has_colors()) {
        attroff(COLOR_PAIR(C_COW_BLACK) | A_BOLD);
        attron(COLOR_PAIR(C_COW) | A_BOLD);
    }
    mvaddch(y, x, '&');
    if (has_colors()) {
        attroff(COLOR_PAIR(C_COW) | A_BOLD);
        attron(COLOR_PAIR(C_COW_BLACK) | A_BOLD);
    }
    mvaddch(y, x + 1, '&');
    if (has_colors())
        attroff(COLOR_PAIR(C_COW_BLACK) | A_BOLD);
}

void drawSpider(int y, int x) {
    if (has_colors())
        attron(COLOR_PAIR(C_COW_BLACK) | A_BOLD);
    mvaddch(y, x, 'X');
    if (has_colors())
        attroff(COLOR_PAIR(C_COW_BLACK) | A_BOLD);
}

void drawSpider(int y, int x, bool poisonous) {
    if (!poisonous) {
        drawSpider(y, x);
        return;
    }
    if (has_colors())
        attron(COLOR_PAIR(C_POISON) | A_BOLD);
    mvaddch(y, x, 'P');
    if (has_colors())
        attroff(COLOR_PAIR(C_POISON) | A_BOLD);
}

void drawSheep(int y, int x, bool sheared) {
    if (has_colors())
        attron(COLOR_PAIR(C_SHEEP_BLACK) | A_BOLD);
    mvaddch(y, x - 1, '&');
    if (has_colors()) {
        attroff(COLOR_PAIR(C_SHEEP_BLACK) | A_BOLD);
        attron((sheared ? COLOR_PAIR(C_BEACH) : COLOR_PAIR(C_COW)) | A_BOLD);
    }
    mvaddch(y, x, '&');
    if (has_colors())
        attroff((sheared ? COLOR_PAIR(C_BEACH) : COLOR_PAIR(C_COW)) | A_BOLD);
}

double movementProgress(long long currentTick, long long startedTick, long long durationTicks) {
    if (durationTicks <= 0 || currentTick <= startedTick)
        return 0.0;
    double progress = static_cast<double>(currentTick - startedTick) / static_cast<double>(durationTicks);
    if (progress > 1.0)
        return 1.0;
    return progress * progress * (3.0 - 2.0 * progress);
}

int visualScreenCoord(long long previous, long long current, long long camera,
                      int center, long long startedTick, long long currentTick,
                      long long durationTicks) {
    double progress = movementProgress(currentTick, startedTick, durationTicks);
    double visual = static_cast<double>(previous) +
                    static_cast<double>(current - previous) * progress;
    return static_cast<int>(std::llround(visual - static_cast<double>(camera))) + center;
}

size_t utf8Next(const std::string& text, size_t index) {
    if (index >= text.size())
        return index;

    unsigned char lead = static_cast<unsigned char>(text[index]);
    size_t length = 1;
    if ((lead & 0xe0) == 0xc0)
        length = 2;
    else if ((lead & 0xf0) == 0xe0)
        length = 3;
    else if ((lead & 0xf8) == 0xf0)
        length = 4;

    if (index + length > text.size())
        return index + 1;
    return index + length;
}

std::vector<std::string> wrapUtf8Line(const std::string& text, int maxColumns) {
    std::vector<std::string> lines;
    if (maxColumns <= 4) {
        lines.push_back(text);
        return lines;
    }

    std::string current;
    int columns = 0;
    for (size_t index = 0; index < text.size();) {
        size_t next = utf8Next(text, index);
        std::string glyph = text.substr(index, next - index);
        if (columns >= maxColumns) {
            lines.push_back(current);
            current.clear();
            columns = 0;
        }
        current += glyph;
        columns++;
        index = next;
    }
    if (!current.empty() || lines.empty())
        lines.push_back(current);
    return lines;
}

void drawChat(const ChatState& chat, int height, int width) {
    int y = height - 8;
    if (y < 1)
        y = 1;

    std::vector<std::string> wrapped;
    for (const std::string& message : chat.messages) {
        std::vector<std::string> lines = wrapUtf8Line(message, width - 2);
        wrapped.insert(wrapped.end(), lines.begin(), lines.end());
    }

    int maxLines = 4;
    int start = static_cast<int>(wrapped.size()) - maxLines;
    if (start < 0)
        start = 0;

    int shown = 0;
    for (int i = start; i < static_cast<int>(wrapped.size()); ++i) {
        if (has_colors())
            attron(COLOR_PAIR(C_STATUS));
        mvprintw(y + shown, 1, "%s", wrapped[i].c_str());
        if (has_colors())
            attroff(COLOR_PAIR(C_STATUS));
        shown++;
    }

    if (!chat.open)
        return;

    std::string input = "> " + chat.input;
    std::vector<std::string> inputLines = wrapUtf8Line(input, width - 2);
    if (!inputLines.empty())
        input = inputLines.back();
    if (has_colors())
        attron(COLOR_PAIR(C_STATUS) | A_BOLD);
    else
        attron(A_REVERSE);
    mvhline(height - 3, 0, ' ', width);
    mvprintw(height - 3, 1, "%s", input.c_str());
    if (has_colors())
        attroff(COLOR_PAIR(C_STATUS) | A_BOLD);
    else
        attroff(A_REVERSE);
}

void drawWorld(const Position& player, const LookDirection& look,
               const Inventory& inventory, const std::vector<Cow>& cows,
               const std::vector<Spider>& spiders, const std::vector<Sheep>& sheep,
               const PlayerStats& stats, const ChatState& chat,
               const VisualState& visual, long long tick) {
    erase();

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    int worldHeight = height - bottomRows;
    if (worldHeight < 1)
        return;

    int centerY = worldHeight / 2;
    int centerX = width / 2;

    for (int sy = 0; sy < worldHeight; ++sy) {
        for (int sx = 0; sx < width; ++sx) {
            long long wx = player.x + sx - centerX;
            long long wy = player.y + sy - centerY;
            drawTile(sy, sx, {wx, wy}, tileAt(wx, wy), tick);
        }
    }

    for (const auto& dropped : droppedItems) {
        const Position& itemPos = dropped.first;
        int sx = static_cast<int>(itemPos.x - player.x + centerX);
        int sy = static_cast<int>(itemPos.y - player.y + centerY);
        if (sx >= 0 && sx < width && sy >= 0 && sy < worldHeight) {
            if (has_colors())
                attron(COLOR_PAIR(C_STATUS) | A_BOLD);
            mvaddch(sy, sx, '*');
            if (has_colors())
                attroff(COLOR_PAIR(C_STATUS) | A_BOLD);
        }
    }

    for (const Position& spear : droppedSpears) {
        int sx = static_cast<int>(spear.x - player.x + centerX);
        int sy = static_cast<int>(spear.y - player.y + centerY);
        if (sx >= 0 && sx < width && sy >= 0 && sy < worldHeight) {
            if (has_colors())
                attron(COLOR_PAIR(C_STATUS) | A_BOLD);
            mvaddch(sy, sx, '>');
            if (has_colors())
                attroff(COLOR_PAIR(C_STATUS) | A_BOLD);
        }
    }

    for (const Cow& cow : cows) {
        int sx = visualScreenCoord(cow.previousPos.x, cow.pos.x, player.x, centerX,
                                   cow.moveStartedTick, tick, cowMoveTicks);
        int sy = visualScreenCoord(cow.previousPos.y, cow.pos.y, player.y, centerY,
                                   cow.moveStartedTick, tick, cowMoveTicks);
        if (sx > 0 && sx < width - 1 && sy >= 0 && sy < worldHeight)
            drawCow(sy, sx);
    }

    for (const Spider& spider : spiders) {
        int sx = visualScreenCoord(spider.previousPos.x, spider.pos.x, player.x, centerX,
                                   spider.moveStartedTick, tick, spiderMoveTicks);
        int sy = visualScreenCoord(spider.previousPos.y, spider.pos.y, player.y, centerY,
                                   spider.moveStartedTick, tick, spiderMoveTicks);
        if (sx >= 0 && sx < width && sy >= 0 && sy < worldHeight)
            drawSpider(sy, sx, spider.poisonous);
    }

    for (const Sheep& oneSheep : sheep) {
        int sx = visualScreenCoord(oneSheep.previousPos.x, oneSheep.pos.x, player.x, centerX,
                                   oneSheep.moveStartedTick, tick, sheepMoveTicks);
        int sy = visualScreenCoord(oneSheep.previousPos.y, oneSheep.pos.y, player.y, centerY,
                                   oneSheep.moveStartedTick, tick, sheepMoveTicks);
        if (sx > 0 && sx < width && sy >= 0 && sy < worldHeight)
            drawSheep(sy, sx, oneSheep.sheared);
    }

    if (has_colors())
        attron(COLOR_PAIR(C_PLAYER));
    int arrowY = centerY + look.dy;
    int arrowX = centerX + look.dx;
    if (arrowY >= 0 && arrowY < worldHeight && arrowX >= 0 && arrowX < width)
        mvaddch(arrowY, arrowX, lookGlyph(look));

    int playerY = visualScreenCoord(visual.playerPrevious.y, player.y, player.y, centerY,
                                    visual.playerMoveStartedTick, tick, ticksPerMove);
    int playerX = visualScreenCoord(visual.playerPrevious.x, player.x, player.x, centerX,
                                    visual.playerMoveStartedTick, tick, ticksPerMove);
    attron(A_BOLD);
    if (playerY >= 0 && playerY < worldHeight && playerX >= 0 && playerX < width)
        mvaddch(playerY, playerX, '@');
    attroff(A_BOLD);
    if (has_colors())
        attroff(COLOR_PAIR(C_PLAYER));

    drawTopHud(inventory, stats, tick);
    drawSurvivalBars(stats, height - 5, width);
    drawHotbar(inventory, height - 4, width);

    if (has_colors())
        attron(COLOR_PAIR(C_STATUS));
    else
        attron(A_REVERSE);
    mvhline(height - 2, 0, ' ', width);
    mvprintw(height - 2, 0, "LMB hit/spear  RMB place  E item/menu  I inventory  T chat  q quit pos:%lld,%lld",
             player.x, player.y);
    if (has_colors())
        attroff(COLOR_PAIR(C_STATUS));
    else
        attroff(A_REVERSE);

    if (useUnicode)
        mvprintw(height - 1, 0, ". 0 ~ : sand T tree w wheat H fence &&& cow && sheep X spider * item > spear @ you");
    else
        mvprintw(height - 1, 0, ". 0 ~ : sand T tree w wheat H fence &&& cow && sheep X spider * item > spear @ you");
    drawChat(chat, height, width);
    drawInventoryOverlay(inventory, player, cows);
    wnoutrefresh(stdscr);
    doupdate();
}
