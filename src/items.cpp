bool isBreakable(char tile) {
    return tile == '0' || tile == 'i' || tile == 'T' || tile == ':' || tile == 'w' || tile == 'W' ||
           tile == '#' || tile == 'F' || tile == 'D' || tile == 'd' || tile == 'C' || tile == 'f' ||
           tile == 'B' || tile == 'M';
}

int damageAt(const Position& position) {
    auto damage = blockDamage.find(position);
    if (damage == blockDamage.end())
        return 0;

    return damage->second;
}

const char* itemName(ItemType item) {
    if (item == ItemType::Stone)
        return "Stone";
    if (item == ItemType::Wood)
        return "Wood";
    if (item == ItemType::RawMeat)
        return "Raw Meat";
    if (item == ItemType::CookedMeat)
        return "Cooked Meat";
    if (item == ItemType::DirtyWater)
        return "Dirty Water";
    if (item == ItemType::BoiledWater)
        return "Boiled Water";
    if (item == ItemType::Milk)
        return "Milk";
    if (item == ItemType::Workbench)
        return "Workbench";
    if (item == ItemType::Furnace)
        return "Furnace";
    if (item == ItemType::Door)
        return "Door";
    if (item == ItemType::Chest)
        return "Chest";
    if (item == ItemType::Spear)
        return "Spear";
    if (item == ItemType::Sand)
        return "Sand";
    if (item == ItemType::Wheat)
        return "Wheat";
    if (item == ItemType::WheatSeeds)
        return "Wheat Seeds";
    if (item == ItemType::Fence)
        return "Fence";
    if (item == ItemType::IronOre)
        return "Iron Ore";
    if (item == ItemType::IronIngot)
        return "Iron Ingot";
    if (item == ItemType::WoodPickaxe)
        return "Wood Pickaxe";
    if (item == ItemType::StonePickaxe)
        return "Stone Pickaxe";
    if (item == ItemType::IronPickaxe)
        return "Iron Pickaxe";
    if (item == ItemType::WoodAxe)
        return "Wood Axe";
    if (item == ItemType::StoneAxe)
        return "Stone Axe";
    if (item == ItemType::IronAxe)
        return "Iron Axe";
    if (item == ItemType::WoodShovel)
        return "Wood Shovel";
    if (item == ItemType::StoneShovel)
        return "Stone Shovel";
    if (item == ItemType::IronShovel)
        return "Iron Shovel";
    if (item == ItemType::Bread)
        return "Bread";
    if (item == ItemType::Shears)
        return "Shears";
    if (item == ItemType::Wool)
        return "Wool";
    if (item == ItemType::Bed)
        return "Bed";
    return "Empty";
}

const char* itemShortName(ItemType item) {
    if (item == ItemType::Stone)
        return "Stone";
    if (item == ItemType::Wood)
        return "Wood";
    if (item == ItemType::RawMeat)
        return "Raw";
    if (item == ItemType::CookedMeat)
        return "Cook";
    if (item == ItemType::DirtyWater)
        return "Dirty";
    if (item == ItemType::BoiledWater)
        return "Water";
    if (item == ItemType::Milk)
        return "Milk";
    if (item == ItemType::Workbench)
        return "Bench";
    if (item == ItemType::Furnace)
        return "Furn";
    if (item == ItemType::Door)
        return "Door";
    if (item == ItemType::Chest)
        return "Chest";
    if (item == ItemType::Spear)
        return "Spear";
    if (item == ItemType::Sand)
        return "Sand";
    if (item == ItemType::Wheat)
        return "Wheat";
    if (item == ItemType::WheatSeeds)
        return "Seeds";
    if (item == ItemType::Fence)
        return "Fence";
    if (item == ItemType::IronOre)
        return "Ore";
    if (item == ItemType::IronIngot)
        return "Iron";
    if (item == ItemType::WoodPickaxe)
        return "WPick";
    if (item == ItemType::StonePickaxe)
        return "SPick";
    if (item == ItemType::IronPickaxe)
        return "IPick";
    if (item == ItemType::WoodAxe)
        return "WAxe";
    if (item == ItemType::StoneAxe)
        return "SAxe";
    if (item == ItemType::IronAxe)
        return "IAxe";
    if (item == ItemType::WoodShovel)
        return "WShov";
    if (item == ItemType::StoneShovel)
        return "SShov";
    if (item == ItemType::IronShovel)
        return "IShov";
    if (item == ItemType::Bread)
        return "Bread";
    if (item == ItemType::Shears)
        return "Shears";
    if (item == ItemType::Wool)
        return "Wool";
    if (item == ItemType::Bed)
        return "Bed";
    return "Empty";
}

const wchar_t* itemIcon(ItemType item) {
    if (!useUnicode) {
        if (item == ItemType::Stone)
            return L"O";
        if (item == ItemType::Wood)
            return L"W";
        if (item == ItemType::RawMeat)
            return L"r";
        if (item == ItemType::CookedMeat)
            return L"M";
        if (item == ItemType::DirtyWater)
            return L"w";
        if (item == ItemType::BoiledWater)
            return L"V";
        if (item == ItemType::Milk)
            return L"M";
        if (item == ItemType::Workbench)
            return L"B";
        if (item == ItemType::Furnace)
            return L"F";
        if (item == ItemType::Door)
            return L"D";
        if (item == ItemType::Chest)
            return L"C";
        if (item == ItemType::Spear)
            return L">";
        if (item == ItemType::Sand)
            return L":";
        if (item == ItemType::Wheat)
            return L"w";
        if (item == ItemType::WheatSeeds)
            return L"s";
        if (item == ItemType::Fence)
            return L"H";
        if (item == ItemType::IronOre)
            return L"o";
        if (item == ItemType::IronIngot)
            return L"I";
        if (item == ItemType::WoodPickaxe || item == ItemType::StonePickaxe || item == ItemType::IronPickaxe)
            return L"P";
        if (item == ItemType::WoodAxe || item == ItemType::StoneAxe || item == ItemType::IronAxe)
            return L"A";
        if (item == ItemType::WoodShovel || item == ItemType::StoneShovel || item == ItemType::IronShovel)
            return L"S";
        if (item == ItemType::Bread)
            return L"b";
        if (item == ItemType::Shears)
            return L"X";
        if (item == ItemType::Wool)
            return L"u";
        if (item == ItemType::Bed)
            return L"Z";
        return L".";
    }

    if (item == ItemType::Stone)
        return L"●";
    if (item == ItemType::Wood)
        return L"♣";
    if (item == ItemType::RawMeat)
        return L"m";
    if (item == ItemType::CookedMeat)
        return L"M";
    if (item == ItemType::DirtyWater)
        return L"≈";
    if (item == ItemType::BoiledWater)
        return L"◌";
    if (item == ItemType::Milk)
        return L"m";
    if (item == ItemType::Workbench)
        return L"⚒";
    if (item == ItemType::Furnace)
        return L"♨";
    if (item == ItemType::Door)
        return L"▥";
    if (item == ItemType::Chest)
        return L"C";
    if (item == ItemType::Spear)
        return L"➤";
    if (item == ItemType::Sand)
        return L":";
    if (item == ItemType::Wheat)
        return L"w";
    if (item == ItemType::WheatSeeds)
        return L"s";
    if (item == ItemType::Fence)
        return L"H";
    if (item == ItemType::IronOre)
        return L"o";
    if (item == ItemType::IronIngot)
        return L"I";
    if (item == ItemType::WoodPickaxe || item == ItemType::StonePickaxe || item == ItemType::IronPickaxe)
        return L"⛏";
    if (item == ItemType::WoodAxe || item == ItemType::StoneAxe || item == ItemType::IronAxe)
        return L"Y";
    if (item == ItemType::WoodShovel || item == ItemType::StoneShovel || item == ItemType::IronShovel)
        return L"⌞";
    if (item == ItemType::Bread)
        return L"b";
    if (item == ItemType::Shears)
        return L"X";
    if (item == ItemType::Wool)
        return L"u";
    if (item == ItemType::Bed)
        return L"Z";
    return L"·";
}

ItemType itemFromPlacedTile(char tile) {
    if (tile == '0')
        return ItemType::Stone;
    if (tile == 'i')
        return ItemType::IronOre;
    if (tile == 'T')
        return ItemType::Wood;
    if (tile == '#')
        return ItemType::Workbench;
    if (tile == 'F')
        return ItemType::Furnace;
    if (tile == 'D' || tile == 'd')
        return ItemType::Door;
    if (tile == 'C')
        return ItemType::Chest;
    if (tile == ':')
        return ItemType::Sand;
    if (tile == 'w')
        return ItemType::WheatSeeds;
    if (tile == 'W')
        return ItemType::Wheat;
    if (tile == 'f')
        return ItemType::Fence;
    if (tile == 'B')
        return ItemType::Bed;
    return ItemType::None;
}

char tileFromItem(ItemType item) {
    if (item == ItemType::Stone)
        return '0';
    if (item == ItemType::Wood)
        return 'T';
    if (item == ItemType::Workbench)
        return '#';
    if (item == ItemType::Furnace)
        return 'F';
    if (item == ItemType::Door)
        return 'D';
    if (item == ItemType::Chest)
        return 'C';
    if (item == ItemType::Sand)
        return ':';
    if (item == ItemType::Fence)
        return 'f';
    if (item == ItemType::WheatSeeds)
        return 'w';
    if (item == ItemType::Bed)
        return 'B';
    return '\0';
}

bool canAddItem(const Inventory& inventory, ItemType item) {
    if (item == ItemType::None)
        return false;

    for (const Slot& slot : inventory.slots) {
        if (slot.item == item || slot.item == ItemType::None || slot.count <= 0)
            return true;
    }
    return false;
}

void addItem(Inventory& inventory, ItemType item, int amount) {
    if (item == ItemType::None || amount <= 0)
        return;

    for (Slot& slot : inventory.slots) {
        if (slot.item == item) {
            slot.count += amount;
            return;
        }
    }

    for (Slot& slot : inventory.slots) {
        if (slot.item == ItemType::None || slot.count <= 0) {
            slot.item = item;
            slot.count = amount;
            return;
        }
    }
}

int countItem(const Inventory& inventory, ItemType item) {
    int total = 0;
    for (const Slot& slot : inventory.slots) {
        if (slot.item == item)
            total += slot.count;
    }
    return total;
}

bool removeItem(Inventory& inventory, ItemType item, int amount = 1) {
    if (countItem(inventory, item) < amount)
        return false;

    for (Slot& slot : inventory.slots) {
        if (slot.item != item)
            continue;

        int taken = slot.count < amount ? slot.count : amount;
        slot.count -= taken;
        amount -= taken;
        if (slot.count <= 0)
            slot = {};
        if (amount == 0)
            return true;
    }

    return true;
}

void dropBrokenTile(const Position& position, char tile) {
    ItemType item = itemFromPlacedTile(tile);
    if (tile == '0' && isIronOreAt(position.x, position.y))
        item = ItemType::IronOre;
    if (item == ItemType::None)
        return;

    Slot& drop = droppedItems[position];
    if (drop.item == ItemType::None || drop.count <= 0) {
        drop.item = item;
        drop.count = tile == 'W' ? 2 : 1;
        return;
    }

    if (drop.item == item)
        drop.count += tile == 'W' ? 2 : 1;

    if (tile == 'W') {
        Slot& seeds = droppedItems[{position.x + 1, position.y}];
        if (seeds.item == ItemType::None || seeds.count <= 0)
            seeds = {ItemType::WheatSeeds, tile == 'W' ? 2 : 1};
        else if (seeds.item == ItemType::WheatSeeds)
            seeds.count += tile == 'W' ? 2 : 1;
    }
}

void dropSlotNear(const Position& position, const Slot& slot) {
    if (slot.item == ItemType::None || slot.count <= 0)
        return;

    for (long long radius = 0; radius <= 4; ++radius) {
        for (long long dy = -radius; dy <= radius; ++dy) {
            for (long long dx = -radius; dx <= radius; ++dx) {
                if (std::llabs(dx) != radius && std::llabs(dy) != radius)
                    continue;

                Position target{position.x + dx, position.y + dy};
                Slot& drop = droppedItems[target];
                if (drop.item == ItemType::None || drop.count <= 0) {
                    drop = slot;
                    return;
                }
                if (drop.item == slot.item) {
                    drop.count += slot.count;
                    return;
                }
            }
        }
    }
}

bool isPickaxe(ItemType item) {
    return item == ItemType::WoodPickaxe || item == ItemType::StonePickaxe || item == ItemType::IronPickaxe;
}

bool isAxe(ItemType item) {
    return item == ItemType::WoodAxe || item == ItemType::StoneAxe || item == ItemType::IronAxe;
}

bool isShovel(ItemType item) {
    return item == ItemType::WoodShovel || item == ItemType::StoneShovel || item == ItemType::IronShovel;
}

bool canInstantBreak(char tile, ItemType tool) {
    if (tile == '0' || tile == 'i' || tile == '#' || tile == 'F')
        return isPickaxe(tool);
    if (tile == 'T' || tile == 'D' || tile == 'd' || tile == 'C' || tile == 'f' || tile == 'B' || tile == 'M')
        return isAxe(tool);
    if (tile == ':')
        return isShovel(tool);
    return tile == 'w';
}

bool eatSelectedItem(Inventory& inventory) {
    Slot& slot = inventory.slots[inventory.selectedSlot];
    if ((slot.item != ItemType::CookedMeat && slot.item != ItemType::Bread) || slot.count <= 0)
        return false;

    slot.count--;
    if (slot.count <= 0)
        slot = {};

    return true;
}

bool drinkSelectedItem(Inventory& inventory) {
    Slot& slot = inventory.slots[inventory.selectedSlot];
    if ((slot.item != ItemType::BoiledWater && slot.item != ItemType::Milk) || slot.count <= 0)
        return false;

    slot.count--;
    if (slot.count <= 0)
        slot = {};
    return true;
}

bool breakBlockAt(const Position& target, ItemType tool = ItemType::None) {
    char tile = tileAt(target.x, target.y);
    if (!isBreakable(tile))
        return false;

    int& damage = blockDamage[target];
    damage += canInstantBreak(tile, tool) ? blockHitsToBreak : 1;
    if (damage < blockHitsToBreak)
        return true;

    blockDamage.erase(target);
    dropBrokenTile(target, tile);
    if (tile == 'C' || tile == 'M') {
        auto chest = chests.find(target);
        if (chest != chests.end()) {
            for (const Slot& slot : chest->second)
                dropSlotNear(target, slot);
            chests.erase(chest);
        }
    }
    if (tile == 'F') {
        auto furnace = furnaces.find(target);
        if (furnace != furnaces.end()) {
            dropSlotNear(target, furnace->second.slot);
            dropSlotNear(target, furnace->second.fuel);
            furnaces.erase(furnace);
        }
    }

    auto placed = placedBlocks.find(target);
    if (placed != placedBlocks.end()) {
        placedBlocks.erase(placed);
        brokenBlocks.erase(target);
        return true;
    }

    if (brokenBlocks.insert(target).second)
        return true;

    return false;
}

bool attackBlockAt(const Position& player, const Position& target, ItemType tool = ItemType::None) {
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    if (dx * dx + dy * dy > attackRadius * attackRadius)
        return false;

    return breakBlockAt(target, tool);
}

bool attackInLookDirection(const Position& player, const LookDirection& look, ItemType tool = ItemType::None) {
    if (look.dx == 0 && look.dy == 0)
        return false;

    for (long long distance = 1; distance <= attackRadius; ++distance) {
        Position target = {
            player.x + look.dx * distance,
            player.y + look.dy * distance
        };

        if (breakBlockAt(target, tool))
            return true;
    }

    return false;
}

bool attackTowardTarget(const Position& player, const Position& target, ItemType tool = ItemType::None) {
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    if (dx == 0 && dy == 0)
        return false;

    long long distance = std::llabs(dx) > std::llabs(dy) ? std::llabs(dx) : std::llabs(dy);
    if (distance > attackRadius) {
        double scale = static_cast<double>(attackRadius) / static_cast<double>(distance);
        dx = static_cast<long long>(std::llround(static_cast<double>(dx) * scale));
        dy = static_cast<long long>(std::llround(static_cast<double>(dy) * scale));
    }

    long long steps = std::llabs(dx) > std::llabs(dy) ? std::llabs(dx) : std::llabs(dy);
    if (steps == 0)
        return false;

    for (long long step = 1; step <= steps; ++step) {
        Position current = {
            player.x + static_cast<long long>(std::llround(static_cast<double>(dx) * step / steps)),
            player.y + static_cast<long long>(std::llround(static_cast<double>(dy) * step / steps))
        };

        if (breakBlockAt(current, tool))
            return true;
    }

    return false;
}
