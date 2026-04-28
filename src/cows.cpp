bool shouldCowSpawnAt(const Position& pos) {
    if (isStartArea(pos.x, pos.y) || killedCowSpawns.count(pos) > 0)
        return false;

    if (!cowCanWalkTo(pos))
        return false;

    uint64_t noise = mix(static_cast<uint64_t>(pos.x) * 0x632be59bd9b4e019ULL ^
                         static_cast<uint64_t>(pos.y) * 0x9e3779b97f4a7c15ULL ^
                         worldSeed);
    return noise % 420 == 0;
}

bool cowExistsAt(const std::vector<Cow>& cows, const Position& pos) {
    for (const Cow& cow : cows) {
        if (cow.pos == pos)
            return true;
    }

    return false;
}

bool spawnCowsAroundPlayer(std::vector<Cow>& cows, const Position& player) {
    bool spawned = false;
    for (long long y = player.y - cowScanRadius; y <= player.y + cowScanRadius; ++y) {
        for (long long x = player.x - cowScanRadius; x <= player.x + cowScanRadius; ++x) {
            Position spawn{x, y};
            if (seenCowSpawns.count(spawn) > 0)
                continue;

            seenCowSpawns.insert(spawn);
            if (shouldCowSpawnAt(spawn)) {
                cows.push_back({spawn, spawn, false, spawn, 0});
                spawned = true;
            }
        }
    }

    return spawned;
}

bool updateCows(std::vector<Cow>& cows, const Position& player, long long tick, const Inventory& inventory) {
    if (tick % cowMoveTicks != 0)
        return false;

    bool luring = inventory.slots[inventory.selectedSlot].item == ItemType::Wheat &&
                  inventory.slots[inventory.selectedSlot].count > 0;
    bool moved = false;
    for (Cow& cow : cows) {
        long long dxToPlayer = player.x - cow.pos.x;
        long long dyToPlayer = player.y - cow.pos.y;
        bool nearPlayer = dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer <= 25;
        bool seesWheat = luring && dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer <= 100;

        int dx = 0;
        int dy = 0;
        if (seesWheat) {
            dx = sign(dxToPlayer);
            dy = sign(dyToPlayer);
        } else if (nearPlayer) {
            dx = -sign(dxToPlayer);
            dy = -sign(dyToPlayer);
        } else {
            uint64_t choice = mix(static_cast<uint64_t>(cow.pos.x + tick) ^
                                  static_cast<uint64_t>(cow.pos.y - tick) * 0x94d049bb133111ebULL);
            int dir = static_cast<int>(choice % 5);
            if (dir == 1) dx = 1;
            else if (dir == 2) dx = -1;
            else if (dir == 3) dy = 1;
            else if (dir == 4) dy = -1;
        }

        Position next{cow.pos.x + dx, cow.pos.y + dy};
        if (!(next == player) && !cowExistsAt(cows, next) && cowCanWalkTo(next)) {
            cow.previousPos = cow.pos;
            cow.pos = next;
            cow.moveStartedTick = tick;
            moved = true;
        }
    }

    return moved;
}

bool killCowAt(std::vector<Cow>& cows, const Position& target, Inventory& inventory) {
    for (auto it = cows.begin(); it != cows.end(); ++it) {
        if (it->pos == target) {
            killedCowSpawns.insert(it->spawn);
            cows.erase(it);
            addItem(inventory, ItemType::RawMeat);
            return true;
        }
    }

    return false;
}

bool cowCanBeMilked(const Position& player, const std::vector<Cow>& cows) {
    for (const Cow& cow : cows) {
        long long dx = cow.pos.x - player.x;
        long long dy = cow.pos.y - player.y;
        if (!cow.milked && dx * dx + dy * dy <= 2)
            return true;
    }

    return false;
}

bool milkNearbyCow(const Position& player, std::vector<Cow>& cows, Inventory& inventory) {
    for (Cow& cow : cows) {
        long long dx = cow.pos.x - player.x;
        long long dy = cow.pos.y - player.y;
        if (!cow.milked && dx * dx + dy * dy <= 2) {
            cow.milked = true;
            addItem(inventory, ItemType::Milk);
            return true;
        }
    }

    return false;
}

bool isNearFence(const Position& position) {
    for (long long dy = -1; dy <= 1; ++dy) {
        for (long long dx = -1; dx <= 1; ++dx) {
            if (tileAt(position.x + dx, position.y + dy) == 'f')
                return true;
        }
    }
    return false;
}

bool breedCowAt(const Position& player, const Position& target, std::vector<Cow>& cows, Inventory& inventory) {
    Slot& slot = inventory.slots[inventory.selectedSlot];
    if (slot.item != ItemType::Wheat || slot.count <= 0)
        return false;

    for (Cow& cow : cows) {
        if (!(cow.pos == target))
            continue;

        long long dx = cow.pos.x - player.x;
        long long dy = cow.pos.y - player.y;
        if (dx * dx + dy * dy > attackRadius * attackRadius || !isNearFence(cow.pos))
            return false;

        const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        for (const auto& dir : dirs) {
            Position baby{cow.pos.x + dir[0], cow.pos.y + dir[1]};
            if (!(baby == player) && !cowExistsAt(cows, baby) && cowCanWalkTo(baby)) {
                cows.push_back({baby, baby, false, baby, 0});
                slot.count--;
                if (slot.count <= 0)
                    slot = {};
                return true;
            }
        }
        return false;
    }

    return false;
}

bool attackCowBlockAt(const Position& player, const Position& target,
                      std::vector<Cow>& cows, Inventory& inventory) {
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    if (dx * dx + dy * dy > attackRadius * attackRadius)
        return false;

    return killCowAt(cows, target, inventory);
}

bool attackCowInLookDirection(const Position& player, const LookDirection& look,
                              std::vector<Cow>& cows, Inventory& inventory) {
    for (long long distance = 1; distance <= attackRadius; ++distance) {
        Position target{player.x + look.dx * distance, player.y + look.dy * distance};
        if (killCowAt(cows, target, inventory))
            return true;
    }

    return false;
}

bool attackCowTowardTarget(const Position& player, const Position& target,
                           std::vector<Cow>& cows, Inventory& inventory) {
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    long long distance = std::llabs(dx) > std::llabs(dy) ? std::llabs(dx) : std::llabs(dy);
    if (distance == 0)
        return false;

    if (distance > attackRadius) {
        double scale = static_cast<double>(attackRadius) / static_cast<double>(distance);
        dx = static_cast<long long>(std::llround(static_cast<double>(dx) * scale));
        dy = static_cast<long long>(std::llround(static_cast<double>(dy) * scale));
        distance = attackRadius;
    }

    long long steps = std::llabs(dx) > std::llabs(dy) ? std::llabs(dx) : std::llabs(dy);
    for (long long step = 1; step <= steps; ++step) {
        Position current{
            player.x + static_cast<long long>(std::llround(static_cast<double>(dx) * step / steps)),
            player.y + static_cast<long long>(std::llround(static_cast<double>(dy) * step / steps))
        };

        if (killCowAt(cows, current, inventory))
            return true;
    }

    return false;
}
