Position targetInLook(const Position& player, const LookDirection& look, long long distance = 1) {
    return {player.x + look.dx * distance, player.y + look.dy * distance};
}

bool isDoorInLook(const Position& player, const LookDirection& look) {
    char tile = tileAt(player.x + look.dx, player.y + look.dy);
    return tile == 'D' || tile == 'd';
}

Position nearbyTilePosition(const Position& player, char wanted) {
    Position lookTarget = targetInLook(player, {0, 0});
    (void)lookTarget;
    for (long long dy = -1; dy <= 1; ++dy) {
        for (long long dx = -1; dx <= 1; ++dx) {
            Position pos{player.x + dx, player.y + dy};
            if (tileAt(pos.x, pos.y) == wanted)
                return pos;
        }
    }
    return {player.x, player.y};
}

bool isNearChest(const Position& player) {
    for (long long dy = -1; dy <= 1; ++dy) {
        for (long long dx = -1; dx <= 1; ++dx) {
            if (tileAt(player.x + dx, player.y + dy) == 'C')
                return true;
        }
    }
    return false;
}

bool openNearbyChest(Inventory& inventory, const Position& player, const LookDirection& look) {
    Position target = targetInLook(player, look);
    char targetTile = tileAt(target.x, target.y);
    if (targetTile != 'C' && targetTile != 'M') {
        target = nearbyTilePosition(player, 'C');
        targetTile = tileAt(target.x, target.y);
        if (targetTile != 'C') {
            target = nearbyTilePosition(player, 'M');
            targetTile = tileAt(target.x, target.y);
        }
    }
    if (targetTile != 'C' && targetTile != 'M')
        return false;

    chests[target];
    inventory.open = true;
    inventory.chestOpen = true;
    inventory.actionMenuOpen = false;
    inventory.activeChest = target;
    inventory.cursor = 0;
    inventory.held = {};
    return true;
}

bool openNearbyFurnace(Inventory& inventory, const Position& player, const LookDirection& look) {
    Position target = targetInLook(player, look);
    if (tileAt(target.x, target.y) != 'F')
        target = nearbyTilePosition(player, 'F');
    if (tileAt(target.x, target.y) != 'F')
        return false;

    furnaces[target];
    inventory.open = true;
    inventory.furnaceOpen = true;
    inventory.chestOpen = false;
    inventory.actionMenuOpen = false;
    inventory.craftMenuOpen = false;
    inventory.activeFurnace = target;
    inventory.cursor = 0;
    inventory.held = {};
    return true;
}

bool isNearTile(const Position& player, char wanted) {
    for (long long dy = -1; dy <= 1; ++dy) {
        for (long long dx = -1; dx <= 1; ++dx) {
            if (tileAt(player.x + dx, player.y + dy) == wanted)
                return true;
        }
    }
    return false;
}

bool isNearWater(const Position& player) {
    for (long long dy = -1; dy <= 1; ++dy) {
        for (long long dx = -1; dx <= 1; ++dx) {
            if (tileAt(player.x + dx, player.y + dy) == '~')
                return true;
        }
    }
    return false;
}

bool isPlantableTile(char tile) {
    return tile == '.' || tile == ':';
}

bool placeSelectedBlock(const Position& player, const LookDirection& look, Inventory& inventory) {
    Slot& slot = inventory.slots[inventory.selectedSlot];
    Position target = targetInLook(player, look);
    char tile = tileFromItem(slot.item);
    if (tile == '\0' || slot.count <= 0)
        return false;

    char targetTile = tileAt(target.x, target.y);
    bool plant = slot.item == ItemType::WheatSeeds || slot.item == ItemType::Sapling;
    if (target == player || (plant ? !isPlantableTile(targetTile) : targetTile != '.'))
        return false;

    placedBlocks[target] = tile;
    blockDamage.erase(target);
    brokenBlocks.erase(target);
    slot.count--;
    if (slot.count <= 0)
        slot = {};
    return true;
}

bool placeSlotBlock(const Position& player, const LookDirection& look, Inventory& inventory, int slotIndex) {
    if (slotIndex < 0 || slotIndex >= inventorySlots)
        return false;

    int oldSelected = inventory.selectedSlot;
    inventory.selectedSlot = slotIndex;
    bool placed = placeSelectedBlock(player, look, inventory);
    inventory.selectedSlot = oldSelected;
    return placed;
}

bool toggleDoorAt(const Position& target) {
    auto placed = placedBlocks.find(target);
    if (placed == placedBlocks.end())
        return false;

    if (placed->second == 'D') {
        placed->second = 'd';
        return true;
    }

    if (placed->second == 'd') {
        placed->second = 'D';
        return true;
    }

    return false;
}

bool toggleDoorInLook(const Position& player, const LookDirection& look) {
    return toggleDoorAt(targetInLook(player, look));
}

bool toggleNearbyDoor(const Position& player, const LookDirection& look) {
    if (toggleDoorInLook(player, look))
        return true;

    const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    for (const auto& dir : dirs) {
        if (toggleDoorAt({player.x + dir[0], player.y + dir[1]}))
            return true;
    }

    return false;
}

int countPlanks(const Inventory& inventory) {
    return countItem(inventory, ItemType::Planks) + countItem(inventory, ItemType::Wood);
}

bool removePlanks(Inventory& inventory, int amount) {
    if (countPlanks(inventory) < amount)
        return false;

    int fromPlanks = std::min(countItem(inventory, ItemType::Planks), amount);
    if (fromPlanks > 0) {
        removeItem(inventory, ItemType::Planks, fromPlanks);
        amount -= fromPlanks;
    }
    if (amount > 0)
        removeItem(inventory, ItemType::Wood, amount);
    return true;
}

bool craftPlanks(Inventory& inventory) {
    if (!removeItem(inventory, ItemType::Log, 1))
        return false;
    addItem(inventory, ItemType::Planks, 4);
    return true;
}

bool craftWorkbench(Inventory& inventory) {
    if (!removePlanks(inventory, 4))
        return false;
    addItem(inventory, ItemType::Workbench);
    return true;
}

bool craftFurnace(Inventory& inventory) {
    if (!removeItem(inventory, ItemType::Stone, 8))
        return false;
    addItem(inventory, ItemType::Furnace);
    return true;
}

bool craftDoor(Inventory& inventory) {
    if (!removePlanks(inventory, 3))
        return false;
    addItem(inventory, ItemType::Door);
    return true;
}

bool craftChest(Inventory& inventory) {
    if (!removePlanks(inventory, 8))
        return false;
    addItem(inventory, ItemType::Chest);
    return true;
}

bool craftSpear(Inventory& inventory) {
    if (countPlanks(inventory) < 1 || countItem(inventory, ItemType::Stone) < 1)
        return false;
    removePlanks(inventory, 1);
    removeItem(inventory, ItemType::Stone, 1);
    addItem(inventory, ItemType::Spear);
    return true;
}

bool craftFence(Inventory& inventory) {
    if (!removePlanks(inventory, 2))
        return false;
    addItem(inventory, ItemType::Fence, 4);
    return true;
}

bool craftShears(Inventory& inventory) {
    if (!removeItem(inventory, ItemType::IronIngot, 2))
        return false;
    addItem(inventory, ItemType::Shears);
    return true;
}

bool craftBed(Inventory& inventory) {
    if (countPlanks(inventory) < 3 || countItem(inventory, ItemType::Wool) < 3)
        return false;
    removePlanks(inventory, 3);
    removeItem(inventory, ItemType::Wool, 3);
    addItem(inventory, ItemType::Bed);
    return true;
}

bool craftTool(Inventory& inventory, ItemType material, int materialCount, ItemType result) {
    if (countPlanks(inventory) < 1 || countItem(inventory, material) < materialCount)
        return false;
    removePlanks(inventory, 1);
    removeItem(inventory, material, materialCount);
    addItem(inventory, result);
    return true;
}

bool craftAtWorkbench(Inventory& inventory, const Position& player) {
    if (!isNearTile(player, '#'))
        return craftPlanks(inventory) || craftWorkbench(inventory);

    if (craftPlanks(inventory))
        return true;
    if (craftFurnace(inventory))
        return true;
    if (craftDoor(inventory))
        return true;
    if (craftChest(inventory))
        return true;
    if (craftSpear(inventory))
        return true;
    if (craftFence(inventory))
        return true;
    if (craftShears(inventory))
        return true;
    if (craftBed(inventory))
        return true;
    return craftWorkbench(inventory);
}

bool craftFromInventory(Inventory& inventory, const Position& player) {
    if (!isNearTile(player, '#'))
        return craftPlanks(inventory) || craftWorkbench(inventory);
    return craftAtWorkbench(inventory, player);
}

ItemType furnaceResult(ItemType item) {
    if (item == ItemType::RawMeat)
        return ItemType::CookedMeat;
    if (item == ItemType::IronOre)
        return ItemType::IronIngot;
    if (item == ItemType::Wheat)
        return ItemType::Bread;
    if (item == ItemType::DirtyWater)
        return ItemType::BoiledWater;
    return ItemType::None;
}

bool furnaceAccepts(ItemType item) {
    return furnaceResult(item) != ItemType::None;
}

int fuelTicks(ItemType item) {
    if (item == ItemType::Log)
        return tickRate * 9;
    if (item == ItemType::Planks || item == ItemType::Wood)
        return tickRate * 6;
    if (item == ItemType::Door || item == ItemType::Workbench || item == ItemType::Chest || item == ItemType::Bed)
        return tickRate * 10;
    if (item == ItemType::Fence || item == ItemType::Wheat || item == ItemType::WheatSeeds)
        return tickRate * 3;
    if (item == ItemType::Spear || item == ItemType::WoodPickaxe || item == ItemType::WoodAxe || item == ItemType::WoodShovel)
        return tickRate * 5;
    return 0;
}

bool isFuel(ItemType item) {
    return fuelTicks(item) > 0;
}

bool updateFurnaces(long long tick) {
    (void)tick;
    bool changed = false;
    for (auto& furnace : furnaces) {
        Slot& slot = furnace.second.slot;
        if (slot.item == ItemType::None || slot.count <= 0 || !furnaceAccepts(slot.item)) {
            furnace.second.progress = 0;
            continue;
        }

        if (furnace.second.burnTicks <= 0) {
            int burn = fuelTicks(furnace.second.fuel.item);
            if (burn <= 0 || furnace.second.fuel.count <= 0) {
                furnace.second.progress = 0;
                continue;
            }
            furnace.second.fuel.count--;
            if (furnace.second.fuel.count <= 0)
                furnace.second.fuel = {};
            furnace.second.burnTicks = burn;
            changed = true;
        }

        furnace.second.burnTicks--;
        furnace.second.progress++;
        if (furnace.second.progress >= furnaceProcessTicks) {
            slot.item = furnaceResult(slot.item);
            furnace.second.progress = 0;
            changed = true;
        }
    }
    return changed;
}

bool setRespawnAtNearbyBed(const Position& player, const LookDirection& look) {
    Position target = targetInLook(player, look);
    if (tileAt(target.x, target.y) != 'B')
        target = nearbyTilePosition(player, 'B');
    if (tileAt(target.x, target.y) != 'B')
        return false;

    respawnPoint = player;
    hasRespawnPoint = true;
    return true;
}

bool throwSpear(const Position& player, const LookDirection& look,
                std::vector<Cow>& cows, Inventory& inventory) {
    if (!removeItem(inventory, ItemType::Spear, 1))
        return false;

    Position landing = targetInLook(player, look);
    for (long long distance = 1; distance <= 6; ++distance) {
        Position target = targetInLook(player, look, distance);

        // Копье пронзает корову: убивает цель, но не останавливается на ней.
        killCowAt(cows, target, inventory);

        if (!canWalkTo(target)) {
            droppedSpears.insert(landing);
            return true;
        }
        landing = target;
    }

    droppedSpears.insert(landing);
    return true;
}
