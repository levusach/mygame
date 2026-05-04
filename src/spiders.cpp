bool shouldSpiderSpawnAt(const Position& pos) {
    if (isStartArea(pos.x, pos.y) || killedSpiderSpawns.count(pos) > 0)
        return false;
    if (!canWalkTo(pos))
        return false;

    uint64_t noise = mix(static_cast<uint64_t>(pos.x) * 0x7f4a7c159e3779b9ULL ^
                         static_cast<uint64_t>(pos.y) * 0x94d049bb133111ebULL ^
                         worldSeed);
    return noise % 520 == 0;
}

bool spiderExistsAt(const std::vector<Spider>& spiders, const Position& pos) {
    for (const Spider& spider : spiders) {
        if (spider.pos == pos)
            return true;
    }
    return false;
}

bool spawnSpidersAroundPlayer(std::vector<Spider>& spiders, const Position& player) {
    (void)spiders;
    (void)player;
    return false;
}

bool updateSpiderDayCycle(std::vector<Spider>& spiders, long long tick) {
    if (!isNightTick(tick)) {
        bool hadSpiders = !spiders.empty();
        spiders.clear();
        activeSpiderNight = -1;
        return hadSpiders;
    }

    long long night = nightIndexForTick(tick);
    if (night != activeSpiderNight) {
        activeSpiderNight = night;
        seenSpiderSpawns.clear();
    }
    return false;
}

bool spawnNightSpidersAroundPlayer(std::vector<Spider>& spiders, const Position& player, long long tick) {
    if (!isNightTick(tick))
        return false;

    bool spawned = false;
    for (long long y = player.y - spiderScanRadius; y <= player.y + spiderScanRadius; ++y) {
        for (long long x = player.x - spiderScanRadius; x <= player.x + spiderScanRadius; ++x) {
            Position spawn{x, y};
            if (seenSpiderSpawns.count(spawn) > 0)
                continue;

            seenSpiderSpawns.insert(spawn);
            if (shouldSpiderSpawnAt(spawn)) {
                spiders.push_back({spawn, spawn, spawn, 0});
                spawned = true;
            }
        }
    }
    return spawned;
}

bool updateSpiders(std::vector<Spider>& spiders, const Position& player, PlayerStats& stats, long long tick) {
    bool changed = false;
    for (Spider& spider : spiders) {
        long long dxToPlayer = player.x - spider.pos.x;
        long long dyToPlayer = player.y - spider.pos.y;
        long long dist2 = dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer;

        if (dist2 <= 2 && tick > 0 && tick % spiderDamageTicks == 0 && stats.hearts > 0) {
            stats.hearts--;
            changed = true;
        }

        if (tick % spiderMoveTicks != 0)
            continue;

        int dx = 0;
        int dy = 0;
        if (dist2 <= 144) {
            dx = sign(dxToPlayer);
            dy = sign(dyToPlayer);
        } else {
            uint64_t choice = mix(static_cast<uint64_t>(spider.pos.x + tick) ^
                                  static_cast<uint64_t>(spider.pos.y - tick) * 0x632be59bd9b4e019ULL ^
                                  worldSeed);
            int dir = static_cast<int>(choice % 5);
            if (dir == 1) dx = 1;
            else if (dir == 2) dx = -1;
            else if (dir == 3) dy = 1;
            else if (dir == 4) dy = -1;
        }

        Position next{spider.pos.x + dx, spider.pos.y + dy};
        if (!(next == player) && !spiderExistsAt(spiders, next) && canWalkTo(next)) {
            spider.previousPos = spider.pos;
            spider.pos = next;
            spider.moveStartedTick = tick;
            changed = true;
        }
    }
    return changed;
}

bool killSpiderAt(std::vector<Spider>& spiders, const Position& target) {
    for (auto it = spiders.begin(); it != spiders.end(); ++it) {
        if (it->pos == target) {
            killedSpiderSpawns.insert(it->spawn);
            spiders.erase(it);
            return true;
        }
    }
    return false;
}

bool attackSpiderBlockAt(const Position& player, const Position& target, std::vector<Spider>& spiders) {
    long long dx = target.x - player.x;
    long long dy = target.y - player.y;
    if (dx * dx + dy * dy > attackRadius * attackRadius)
        return false;

    return killSpiderAt(spiders, target);
}

bool attackSpiderInLookDirection(const Position& player, const LookDirection& look, std::vector<Spider>& spiders) {
    for (long long distance = 1; distance <= attackRadius; ++distance) {
        Position target{player.x + look.dx * distance, player.y + look.dy * distance};
        if (killSpiderAt(spiders, target))
            return true;
    }
    return false;
}

bool attackSpiderTowardTarget(const Position& player, const Position& target, std::vector<Spider>& spiders) {
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
        if (killSpiderAt(spiders, current))
            return true;
    }
    return false;
}
