void interactInventory(Inventory& inventory) {
    Slot* slot = nullptr;
    if (inventory.furnaceOpen && inventory.cursor >= inventorySlots) {
        int furnaceIndex = inventory.cursor - inventorySlots;
        if (furnaceIndex == 0)
            slot = &furnaces[inventory.activeFurnace].slot;
        else if (furnaceIndex == 1)
            slot = &furnaces[inventory.activeFurnace].fuel;
        else
            return;
    } else if (inventory.chestOpen && inventory.cursor >= inventorySlots) {
        int chestIndex = inventory.cursor - inventorySlots;
        if (chestIndex < 0 || chestIndex >= chestSlots)
            return;
        slot = &chests[inventory.activeChest][chestIndex];
    } else {
        if (inventory.cursor < 0 || inventory.cursor >= inventorySlots)
            return;
        slot = &inventory.slots[inventory.cursor];
    }

    if (inventory.held.item == ItemType::None) {
        if (slot->item != ItemType::None) {
            inventory.held = *slot;
            *slot = {};
        }
        return;
    }

    if (slot->item == ItemType::None) {
        *slot = inventory.held;
        inventory.held = {};
        return;
    }

    if (slot->item == inventory.held.item) {
        slot->count += inventory.held.count;
        inventory.held = {};
        return;
    }

    std::swap(*slot, inventory.held);
}

void eatSelected(Inventory& inventory) {
    eatSelectedItem(inventory);
}

bool eatSelected(Inventory& inventory, PlayerStats& stats) {
    ItemType eaten = inventory.slots[inventory.selectedSlot].item;
    if (!eatSelectedItem(inventory))
        return false;

    stats.hunger += eaten == ItemType::Bread ? 5 : 6;
    if (stats.hunger > 20)
        stats.hunger = 20;
    return true;
}

bool drinkSelected(Inventory& inventory, PlayerStats& stats) {
    bool milk = inventory.slots[inventory.selectedSlot].item == ItemType::Milk;
    if (!drinkSelectedItem(inventory))
        return false;

    stats.thirst += milk ? 4 : 8;
    if (stats.thirst > 20)
        stats.thirst = 20;
    if (milk) {
        stats.hunger += 2;
        if (stats.hunger > 20)
            stats.hunger = 20;
    }
    return true;
}

bool updateSurvival(PlayerStats& stats, long long tick) {
    bool changed = false;
    if (stats.poisonTicks > 0) {
        stats.poisonTicks--;
        if (stats.poisonDamageLeft > 0) {
            stats.poisonDamageCooldown--;
            if (stats.poisonDamageCooldown <= 0) {
                if (stats.hearts > 0)
                    stats.hearts--;
                stats.poisonDamageLeft--;
                stats.poisonDamageCooldown = poisonDamageIntervalTicks;
                changed = true;
            }
        }
        if (stats.poisonTicks <= 0) {
            stats.poisonTicks = 0;
            stats.poisonDamageLeft = 0;
            stats.poisonDamageCooldown = 0;
            changed = true;
        }
    }
    if (tick > 0 && tick % hungerDrainTicks == 0 && stats.hunger > 0) {
        stats.hunger--;
        changed = true;
    }
    if (tick > 0 && tick % thirstDrainTicks == 0 && stats.thirst > 0) {
        stats.thirst--;
        changed = true;
    }
    if (tick > 0 && tick % damageTicks == 0 && (stats.hunger == 0 || stats.thirst == 0) && stats.hearts > 0) {
        stats.hearts--;
        changed = true;
    }
    if (tick > 0 && tick % healTicks == 0 && stats.hunger == 20 && stats.thirst > 0 && stats.hearts > 0 && stats.hearts < 10) {
        stats.hearts++;
        changed = true;
    }
    return changed;
}

bool handleDeath(Position& player, Inventory& inventory, PlayerStats& stats) {
    if (stats.hearts > 0)
        return false;

    if (hasDeathBag) {
        auto placed = placedBlocks.find(deathBagPos);
        if (placed != placedBlocks.end() && placed->second == 'M')
            placedBlocks.erase(placed);
        chests.erase(deathBagPos);
    }

    deathBagPos = player;
    hasDeathBag = true;
    placedBlocks[deathBagPos] = 'M';
    brokenBlocks.erase(deathBagPos);
    blockDamage.erase(deathBagPos);
    auto& bag = chests[deathBagPos];
    int bagIndex = 0;
    if (inventory.held.item != ItemType::None && inventory.held.count > 0) {
        bag[bagIndex++] = inventory.held;
        inventory.held = {};
    }
    for (Slot& slot : inventory.slots) {
        if (slot.item == ItemType::None || slot.count <= 0)
            continue;
        if (bagIndex < chestSlots)
            bag[bagIndex++] = slot;
        else
            dropSlotNear(deathBagPos, slot);
        slot = {};
    }

    inventory.open = false;
    inventory.actionMenuOpen = false;
    inventory.craftMenuOpen = false;
    inventory.chestOpen = false;
    inventory.furnaceOpen = false;
    inventory.cursor = inventory.selectedSlot;
    player = hasRespawnPoint ? respawnPoint : Position{};
    stats.hearts = 10;
    stats.hunger = 14;
    stats.thirst = 14;
    stats.poisonTicks = 0;
    stats.poisonDamageLeft = 0;
    stats.poisonDamageCooldown = 0;
    return true;
}

const char* dayPhase(long long tick) {
    long long phase = dayPhaseTick(tick);
    long long dawnEnd = dayCycleTicks / 10;
    long long morningEnd = dayCycleTicks * 4 / 10;
    long long noonEnd = dayCycleTicks * 6 / 10;
    long long eveningEnd = dayCycleTicks * 7 / 10;
    long long duskEnd = nightStartTicks;
    long long nightEnd = dayCycleTicks * 9 / 10;
    if (phase < dawnEnd)
        return "Dawn";
    if (phase < morningEnd)
        return "Morning";
    if (phase < noonEnd)
        return "Noon";
    if (phase < eveningEnd)
        return "Evening";
    if (phase < duskEnd)
        return "Dusk";
    if (phase < nightEnd)
        return "Night";
    return "Midnight";
}

struct Ingredient {
    ItemType item;
    int count;
};

struct CraftRecipe {
    ActionType action;
    const char* label;
    bool needsWorkbench;
    std::vector<Ingredient> ingredients;
    ItemType result;
    int resultCount;
};

std::vector<CraftRecipe> allCraftRecipes() {
    return {
        {ActionType::CraftWorkbench, "Workbench (4 wood)", false, {{ItemType::Wood, 4}}, ItemType::Workbench, 1},
        {ActionType::CraftFurnace, "Furnace (8 stone)", true, {{ItemType::Stone, 8}}, ItemType::Furnace, 1},
        {ActionType::CraftDoor, "Door (3 wood)", true, {{ItemType::Wood, 3}}, ItemType::Door, 1},
        {ActionType::CraftChest, "Chest (8 wood)", true, {{ItemType::Wood, 8}}, ItemType::Chest, 1},
        {ActionType::CraftSpear, "Spear (wood + stone)", true, {{ItemType::Wood, 1}, {ItemType::Stone, 1}}, ItemType::Spear, 1},
        {ActionType::CraftFence, "Fence x4 (2 wood)", true, {{ItemType::Wood, 2}}, ItemType::Fence, 4},
        {ActionType::CraftWoodPickaxe, "Wood pickaxe (4 wood)", true, {{ItemType::Wood, 4}}, ItemType::WoodPickaxe, 1},
        {ActionType::CraftStonePickaxe, "Stone pickaxe", true, {{ItemType::Wood, 1}, {ItemType::Stone, 3}}, ItemType::StonePickaxe, 1},
        {ActionType::CraftIronPickaxe, "Iron pickaxe", true, {{ItemType::Wood, 1}, {ItemType::IronIngot, 3}}, ItemType::IronPickaxe, 1},
        {ActionType::CraftWoodAxe, "Wood axe (4 wood)", true, {{ItemType::Wood, 4}}, ItemType::WoodAxe, 1},
        {ActionType::CraftStoneAxe, "Stone axe", true, {{ItemType::Wood, 1}, {ItemType::Stone, 3}}, ItemType::StoneAxe, 1},
        {ActionType::CraftIronAxe, "Iron axe", true, {{ItemType::Wood, 1}, {ItemType::IronIngot, 3}}, ItemType::IronAxe, 1},
        {ActionType::CraftWoodShovel, "Wood shovel (3 wood)", true, {{ItemType::Wood, 3}}, ItemType::WoodShovel, 1},
        {ActionType::CraftStoneShovel, "Stone shovel", true, {{ItemType::Wood, 1}, {ItemType::Stone, 2}}, ItemType::StoneShovel, 1},
        {ActionType::CraftIronShovel, "Iron shovel", true, {{ItemType::Wood, 1}, {ItemType::IronIngot, 2}}, ItemType::IronShovel, 1},
        {ActionType::CraftShears, "Shears (2 iron)", true, {{ItemType::IronIngot, 2}}, ItemType::Shears, 1},
        {ActionType::CraftBed, "Bed (3 wool + 3 wood)", true, {{ItemType::Wool, 3}, {ItemType::Wood, 3}}, ItemType::Bed, 1}
    };
}

bool canCraftRecipe(const CraftRecipe& recipe, const Inventory& inventory, const Position& player) {
    if (recipe.needsWorkbench && !isNearTile(player, '#'))
        return false;

    if (!canAddItem(inventory, recipe.result))
        return false;

    for (const Ingredient& ingredient : recipe.ingredients) {
        if (countItem(inventory, ingredient.item) < ingredient.count)
            return false;
    }
    return true;
}

std::vector<Action> buildCraftActions(const Inventory& inventory, const Position& player) {
    std::vector<Action> actions;
    for (const CraftRecipe& recipe : allCraftRecipes()) {
        if (canCraftRecipe(recipe, inventory, player))
            actions.push_back({recipe.action, recipe.label});
    }
    return actions;
}

bool craftRecipe(const CraftRecipe& recipe, Inventory& inventory, const Position& player) {
    if (!canCraftRecipe(recipe, inventory, player))
        return false;

    for (const Ingredient& ingredient : recipe.ingredients)
        removeItem(inventory, ingredient.item, ingredient.count);
    addItem(inventory, recipe.result, recipe.resultCount);
    return true;
}

bool craftByAction(ActionType action, Inventory& inventory, const Position& player) {
    for (const CraftRecipe& recipe : allCraftRecipes()) {
        if (recipe.action == action)
            return craftRecipe(recipe, inventory, player);
    }
    return false;
}

bool isCraftAction(ActionType action) {
    for (const CraftRecipe& recipe : allCraftRecipes()) {
        if (recipe.action == action)
            return true;
    }
    return false;
}

std::vector<Action> buildActions(const Inventory& inventory, const Position& player, const std::vector<Cow>& cows) {
    std::vector<Action> actions;
    Slot empty;
    const Slot& slot = (inventory.cursor >= 0 && inventory.cursor < inventorySlots) ? inventory.slots[inventory.cursor] : empty;

    if (slot.item != ItemType::None || inventory.held.item != ItemType::None)
        actions.push_back({ActionType::MoveStack, "Move / swap stack"});

    if (slot.item == ItemType::CookedMeat || slot.item == ItemType::Bread)
        actions.push_back({ActionType::Eat, slot.item == ItemType::Bread ? "Eat bread" : "Eat cooked meat"});
    if (slot.item == ItemType::BoiledWater || slot.item == ItemType::Milk)
        actions.push_back({ActionType::Drink, slot.item == ItemType::Milk ? "Drink milk" : "Drink boiled water"});
    if (tileFromItem(slot.item) != '\0')
        actions.push_back({ActionType::Place, "Place selected block"});

    if (cowCanBeMilked(player, cows))
        actions.push_back({ActionType::MilkCow, "Milk nearby cow"});

    if (isDoorInLook(player, {1,0}) || isDoorInLook(player, {-1,0}) ||
        isDoorInLook(player, {0,1}) || isDoorInLook(player, {0,-1}) ||
        isDoorInLook(player, {1,1}) || isDoorInLook(player, {-1,-1}) ||
        isDoorInLook(player, {1,-1}) || isDoorInLook(player, {-1,1}))
        actions.push_back({ActionType::ToggleDoor, "Open / close nearby door"});

    if (slot.item == ItemType::Spear)
        actions.push_back({ActionType::ThrowSpear, "Throw spear"});

    return actions;
}

bool executeAction(ActionType action, Inventory& inventory, PlayerStats& stats,
                   const Position& player, const LookDirection& look, std::vector<Cow>& cows) {
    if (isCraftAction(action))
        return craftByAction(action, inventory, player);

    switch (action) {
        case ActionType::MoveStack:
            interactInventory(inventory);
            return true;
        case ActionType::Eat:
            return eatSelected(inventory, stats);
        case ActionType::Drink:
            return drinkSelected(inventory, stats);
        case ActionType::Place:
            return placeSlotBlock(player, look, inventory, inventory.cursor);
        case ActionType::CraftWorkbench:
        case ActionType::CraftFurnace:
        case ActionType::CraftDoor:
        case ActionType::CraftChest:
        case ActionType::CraftSpear:
        case ActionType::CraftFence:
        case ActionType::CraftWoodPickaxe:
        case ActionType::CraftStonePickaxe:
        case ActionType::CraftIronPickaxe:
        case ActionType::CraftWoodAxe:
        case ActionType::CraftStoneAxe:
        case ActionType::CraftIronAxe:
        case ActionType::CraftWoodShovel:
        case ActionType::CraftStoneShovel:
        case ActionType::CraftIronShovel:
        case ActionType::CraftShears:
        case ActionType::CraftBed:
            return false;
        case ActionType::CookMeat:
        case ActionType::BoilWater:
        case ActionType::SmeltIron:
            return false;
        case ActionType::MilkCow:
            return milkNearbyCow(player, cows, inventory);
        case ActionType::ToggleDoor:
            return toggleNearbyDoor(player, look);
        case ActionType::ThrowSpear:
            return throwSpear(player, look, cows, inventory);
    }

    return false;
}
