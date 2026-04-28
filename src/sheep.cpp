bool shouldSheepSpawnAt(const Position& pos) {
    if (isStartArea(pos.x, pos.y) || killedSheepSpawns.count(pos) > 0)
        return false;
    if (!cowCanWalkTo(pos))
        return false;

    uint64_t noise = mix(static_cast<uint64_t>(pos.x) * 0x94d049bb133111ebULL ^
                         static_cast<uint64_t>(pos.y) * 0xbf58476d1ce4e5b9ULL ^
                         (worldSeed + 0x51ed2705ULL));
    return noise % 520 == 0;
}

bool sheepExistsAt(const std::vector<Sheep>& sheep, const Position& pos) {
    for (const Sheep& oneSheep : sheep) {
        if (oneSheep.pos == pos)
            return true;
    }
    return false;
}

bool spawnSheepAroundPlayer(std::vector<Sheep>& sheep, const Position& player) {
    bool spawned = false;
    for (long long y = player.y - sheepScanRadius; y <= player.y + sheepScanRadius; ++y) {
        for (long long x = player.x - sheepScanRadius; x <= player.x + sheepScanRadius; ++x) {
            Position spawn{x, y};
            if (seenSheepSpawns.count(spawn) > 0)
                continue;

            seenSheepSpawns.insert(spawn);
            if (shouldSheepSpawnAt(spawn)) {
                sheep.push_back({spawn, spawn, false, spawn, 0});
                spawned = true;
            }
        }
    }
    return spawned;
}

bool updateSheep(std::vector<Sheep>& sheep, const Position& player, long long tick, const Inventory& inventory) {
    if (tick % sheepMoveTicks != 0)
        return false;

    bool luring = inventory.slots[inventory.selectedSlot].item == ItemType::Wheat &&
                  inventory.slots[inventory.selectedSlot].count > 0;
    bool moved = false;
    for (Sheep& oneSheep : sheep) {
        long long dxToPlayer = player.x - oneSheep.pos.x;
        long long dyToPlayer = player.y - oneSheep.pos.y;
        bool seesWheat = luring && dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer <= 100;

        int dx = 0;
        int dy = 0;
        if (seesWheat) {
            dx = sign(dxToPlayer);
            dy = sign(dyToPlayer);
        } else {
            uint64_t choice = mix(static_cast<uint64_t>(oneSheep.pos.x + tick) ^
                                  static_cast<uint64_t>(oneSheep.pos.y - tick) * 0x632be59bd9b4e019ULL);
            int dir = static_cast<int>(choice % 5);
            if (dir == 1) dx = 1;
            else if (dir == 2) dx = -1;
            else if (dir == 3) dy = 1;
            else if (dir == 4) dy = -1;
        }

        Position next{oneSheep.pos.x + dx, oneSheep.pos.y + dy};
        if (!(next == player) && !sheepExistsAt(sheep, next) && cowCanWalkTo(next)) {
            oneSheep.previousPos = oneSheep.pos;
            oneSheep.pos = next;
            oneSheep.moveStartedTick = tick;
            moved = true;
        }
    }
    return moved;
}

bool shearSheepAt(const Position& player, const Position& target, std::vector<Sheep>& sheep, Inventory& inventory) {
    Slot& selected = inventory.slots[inventory.selectedSlot];
    if (selected.item != ItemType::Shears || selected.count <= 0)
        return false;

    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    if (dx * dx + dy * dy > attackRadius * attackRadius)
        return false;

    for (Sheep& oneSheep : sheep) {
        if (oneSheep.pos == target && !oneSheep.sheared) {
            oneSheep.sheared = true;
            addItem(inventory, ItemType::Wool, 2);
            return true;
        }
    }
    return false;
}

bool killSheepAt(std::vector<Sheep>& sheep, const Position& target, Inventory& inventory) {
    for (auto it = sheep.begin(); it != sheep.end(); ++it) {
        if (it->pos == target) {
            killedSheepSpawns.insert(it->spawn);
            bool sheared = it->sheared;
            sheep.erase(it);
            addItem(inventory, ItemType::RawMeat);
            if (!sheared)
                addItem(inventory, ItemType::Wool);
            return true;
        }
    }
    return false;
}

bool attackSheepBlockAt(const Position& player, const Position& target,
                        std::vector<Sheep>& sheep, Inventory& inventory) {
    if (shearSheepAt(player, target, sheep, inventory))
        return true;
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    if (dx * dx + dy * dy > attackRadius * attackRadius)
        return false;
    return killSheepAt(sheep, target, inventory);
}

bool attackSheepInLookDirection(const Position& player, const LookDirection& look,
                                std::vector<Sheep>& sheep, Inventory& inventory) {
    for (long long distance = 1; distance <= attackRadius; ++distance) {
        Position target{player.x + look.dx * distance, player.y + look.dy * distance};
        if (attackSheepBlockAt(player, target, sheep, inventory))
            return true;
    }
    return false;
}

bool attackSheepTowardTarget(const Position& player, const Position& target,
                             std::vector<Sheep>& sheep, Inventory& inventory) {
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    long long distance = std::llabs(dx) > std::llabs(dy) ? std::llabs(dx) : std::llabs(dy);
    if (distance == 0)
        return false;

    if (distance > attackRadius) {
        double scale = static_cast<double>(attackRadius) / static_cast<double>(distance);
        dx = static_cast<long long>(std::llround(static_cast<double>(dx) * scale));
        dy = static_cast<long long>(std::llround(static_cast<double>(dy) * scale));
    }

    long long steps = std::llabs(dx) > std::llabs(dy) ? std::llabs(dx) : std::llabs(dy);
    for (long long step = 1; step <= steps; ++step) {
        Position current{
            player.x + static_cast<long long>(std::llround(static_cast<double>(dx) * step / steps)),
            player.y + static_cast<long long>(std::llround(static_cast<double>(dy) * step / steps))
        };
        if (attackSheepBlockAt(player, current, sheep, inventory))
            return true;
    }
    return false;
}
