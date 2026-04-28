uint64_t mix(uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31;
    return value;
}

double seedUnit(uint64_t salt) {
    uint64_t value = mix(worldSeed ^ salt);
    return static_cast<double>(value >> 11) / static_cast<double>(1ULL << 53);
}

long long seedRange(uint64_t salt, long long minValue, long long maxValue) {
    double unit = seedUnit(salt);
    return minValue + static_cast<long long>(unit * static_cast<double>(maxValue - minValue + 1));
}

double seedRangeDouble(uint64_t salt, double minValue, double maxValue) {
    return minValue + seedUnit(salt) * (maxValue - minValue);
}

uint64_t makeRandomWorldSeed() {
    std::random_device device;
    uint64_t a = static_cast<uint64_t>(device());
    uint64_t b = static_cast<uint64_t>(device());
    uint64_t now = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return mix((a << 32) ^ b ^ now);
}

void setupWorldGeneration(uint64_t seed) {
    worldSeed = seed;
    worldGeneration = {};
    worldTileCache.clear();
    if (worldSeed == 0)
        return;

    worldGeneration.terrainNoiseScale = seedRange(0x101, 12, 28);
    worldGeneration.lakeNoiseScale = seedRange(0x102, 24, 58);
    worldGeneration.forestNoiseScale = seedRange(0x103, 14, 38);
    worldGeneration.lakeOffsetX = seedRange(0x201, -200000, 200000);
    worldGeneration.lakeOffsetY = seedRange(0x202, -200000, 200000);
    worldGeneration.forestOffsetX = seedRange(0x203, -200000, 200000);
    worldGeneration.forestOffsetY = seedRange(0x204, -200000, 200000);
    worldGeneration.stoneOffsetX = seedRange(0x205, -200000, 200000);
    worldGeneration.stoneOffsetY = seedRange(0x206, -200000, 200000);
    worldGeneration.wheatOffsetX = seedRange(0x207, -200000, 200000);
    worldGeneration.wheatOffsetY = seedRange(0x208, -200000, 200000);
    worldGeneration.cobblestoneLimit = seedRangeDouble(0x301, 0.72, 0.84);
    worldGeneration.waterLimit = seedRangeDouble(0x302, 0.76, 0.88);
    worldGeneration.treeLimit = seedRangeDouble(0x303, 0.70, 0.83);
    worldGeneration.wheatLimit = seedRangeDouble(0x304, 0.68, 0.82);
    worldGeneration.ironLimit = seedRangeDouble(0x305, 0.72, 0.84);
}

long long floorDiv(long long value, long long divisor) {
    long long result = value / divisor;
    long long rest = value % divisor;
    return (rest != 0 && ((rest < 0) != (divisor < 0))) ? result - 1 : result;
}

double randomAt(long long x, long long y) {
    uint64_t ux = static_cast<uint64_t>(x);
    uint64_t uy = static_cast<uint64_t>(y);
    uint64_t noise = mix(ux * 0x9e3779b97f4a7c15ULL ^ uy * 0xc2b2ae3d27d4eb4fULL ^ worldSeed);
    return static_cast<double>(noise >> 11) / static_cast<double>(1ULL << 53);
}

double smoothstep(double value) {
    return value * value * (3.0 - 2.0 * value);
}

double lerp(double a, double b, double amount) {
    return a + (b - a) * amount;
}

double valueNoise(long long x, long long y, long long scale) {
    long long cellX = floorDiv(x, scale);
    long long cellY = floorDiv(y, scale);
    double localX = static_cast<double>(x - cellX * scale) / scale;
    double localY = static_cast<double>(y - cellY * scale) / scale;

    double topLeft = randomAt(cellX, cellY);
    double topRight = randomAt(cellX + 1, cellY);
    double bottomLeft = randomAt(cellX, cellY + 1);
    double bottomRight = randomAt(cellX + 1, cellY + 1);

    double sx = smoothstep(localX);
    double sy = smoothstep(localY);
    double top = lerp(topLeft, topRight, sx);
    double bottom = lerp(bottomLeft, bottomRight, sx);

    return lerp(top, bottom, sy);
}

bool isStartArea(long long x, long long y) {
    return std::llabs(x) <= emptyStartRadius && std::llabs(y) <= emptyStartRadius;
}

bool isWaterAt(long long x, long long y) {
    if (isStartArea(x, y))
        return false;

    double lakeShape = valueNoise(x + worldGeneration.lakeOffsetX, y + worldGeneration.lakeOffsetY,
                                  worldGeneration.lakeNoiseScale);
    double lakeDetail = valueNoise(x + worldGeneration.lakeOffsetX / 2, y - worldGeneration.lakeOffsetY / 2, 11);
    double water = lakeShape * 0.88 + lakeDetail * 0.12;

    return water > worldGeneration.waterLimit;
}

bool isBeachAt(long long x, long long y) {
    if (isStartArea(x, y) || isWaterAt(x, y))
        return false;

    for (long long dy = -beachRadius; dy <= beachRadius; ++dy) {
        for (long long dx = -beachRadius; dx <= beachRadius; ++dx) {
            if (dx == 0 && dy == 0)
                continue;

            if (dx * dx + dy * dy > beachRadius * beachRadius)
                continue;

            if (isWaterAt(x + dx, y + dy))
                return true;
        }
    }

    return false;
}

bool isTreeAt(long long x, long long y) {
    if (isStartArea(x, y) || isWaterAt(x, y) || isBeachAt(x, y))
        return false;

    double forestShape = valueNoise(x + worldGeneration.forestOffsetX, y + worldGeneration.forestOffsetY,
                                    worldGeneration.forestNoiseScale);
    double forestDetail = valueNoise(x - worldGeneration.forestOffsetY, y + worldGeneration.forestOffsetX, 9);
    double forest = forestShape * 0.84 + forestDetail * 0.16;

    return forest > worldGeneration.treeLimit;
}

bool isWheatAt(long long x, long long y) {
    if (isStartArea(x, y) || isWaterAt(x, y) || isBeachAt(x, y) || isTreeAt(x, y))
        return false;

    double fieldShape = valueNoise(x + worldGeneration.wheatOffsetX, y + worldGeneration.wheatOffsetY, 31);
    double fieldDetail = valueNoise(x - worldGeneration.wheatOffsetY, y + worldGeneration.wheatOffsetX, 6);
    return fieldShape > worldGeneration.wheatLimit && fieldDetail > 0.58;
}

bool isIronOreAt(long long x, long long y) {
    double mountain = valueNoise(x + worldGeneration.stoneOffsetX, y + worldGeneration.stoneOffsetY, 22);
    double vein = valueNoise(x - worldGeneration.stoneOffsetY, y + worldGeneration.stoneOffsetX, 5);
    return mountain > worldGeneration.ironLimit && vein > 0.62;
}

char generateBaseTileAt(long long x, long long y) {
    if (isStartArea(x, y))
        return '.';

    if (isWaterAt(x, y))
        return '~';

    if (isBeachAt(x, y))
        return ':';

    if (isTreeAt(x, y))
        return 'T';

    if (isWheatAt(x, y))
        return 'W';

    double largeBlobs = valueNoise(x + worldGeneration.stoneOffsetX, y + worldGeneration.stoneOffsetY,
                                   worldGeneration.terrainNoiseScale);
    double detail = valueNoise(x - worldGeneration.stoneOffsetY, y + worldGeneration.stoneOffsetX, 7);
    double terrain = largeBlobs * 0.82 + detail * 0.18;

    return (terrain > worldGeneration.cobblestoneLimit) ? '0' : '.';
}

bool rawEmptyForOre(long long x, long long y) {
    Position pos{x, y};
    auto placed = placedBlocks.find(pos);
    if (placed != placedBlocks.end())
        return placed->second == '.' || placed->second == '~';
    if (brokenBlocks.count(pos) > 0)
        return true;
    return generateBaseTileAt(x, y) == '.';
}

bool oreIsExposed(long long x, long long y) {
    const Position neighbors[4] = {{x + 1, y}, {x - 1, y}, {x, y + 1}, {x, y - 1}};
    for (const Position& neighbor : neighbors) {
        if (rawEmptyForOre(neighbor.x, neighbor.y))
            return true;
    }
    return false;
}

char cachedBaseTileAt(long long x, long long y) {
    long long chunkX = floorDiv(x, worldChunkSize);
    long long chunkY = floorDiv(y, worldChunkSize);
    Position chunkPos{chunkX, chunkY};
    auto found = worldTileCache.find(chunkPos);
    if (found == worldTileCache.end()) {
        std::array<char, worldChunkSize * worldChunkSize> tiles{};
        long long baseX = chunkX * worldChunkSize;
        long long baseY = chunkY * worldChunkSize;
        for (int localY = 0; localY < worldChunkSize; ++localY) {
            for (int localX = 0; localX < worldChunkSize; ++localX)
                tiles[localY * worldChunkSize + localX] = generateBaseTileAt(baseX + localX, baseY + localY);
        }
        found = worldTileCache.emplace(chunkPos, tiles).first;
    }

    int localX = static_cast<int>(x - chunkX * worldChunkSize);
    int localY = static_cast<int>(y - chunkY * worldChunkSize);
    return found->second[localY * worldChunkSize + localX];
}

char tileAt(long long x, long long y) {
    auto placed = placedBlocks.find({x, y});
    if (placed != placedBlocks.end())
        return placed->second;

    if (brokenBlocks.count({x, y}) > 0)
        return '.';

    char base = cachedBaseTileAt(x, y);
    if (base == '0' && isIronOreAt(x, y) && oreIsExposed(x, y))
        return 'i';
    return base;
}

bool updateWheatGrowth(long long tick) {
    if (tick <= 0 || tick % wheatGrowTicks != 0)
        return false;

    bool changed = false;
    for (auto& placed : placedBlocks) {
        if (placed.second != 'w')
            continue;

        const Position& pos = placed.first;
        uint64_t roll = mix(static_cast<uint64_t>(pos.x) * 0x51ed2705ULL ^
                            static_cast<uint64_t>(pos.y) * 0x9e3779b97f4a7c15ULL ^
                            static_cast<uint64_t>(tick / wheatGrowTicks) ^
                            worldSeed);
        if (roll % 3 == 0) {
            placed.second = 'W';
            blockDamage.erase(pos);
            changed = true;
        }
    }
    return changed;
}

bool isWaterFlowTarget(const Position& pos) {
    (void)pos;
    return false;
}

bool updateWaterPhysics(const Position& player, long long tick) {
    (void)player;
    (void)tick;
    return false;
}

bool canWalkTo(const Position& next) {
    char tile = tileAt(next.x, next.y);
    return tile != '0' && tile != 'i' && tile != '~' && tile != '#' && tile != 'F' &&
           tile != 'D' && tile != 'C' && tile != 'f' && tile != 'B' && tile != 'M';
}

int moveDelayFor(const Position& player, const MoveInput& move) {
    (void)player;
    (void)move;
    return ticksPerMove;
}

bool cowCanWalkTo(const Position& next) {
    char tile = tileAt(next.x, next.y);
    return tile == '.' || tile == ':' || tile == 'T' || tile == 'w' || tile == 'W';
}

bool tryMove(Position& player, const MoveInput& move) {
    Position next = {player.x + move.dx, player.y + move.dy};
    if (canWalkTo(next)) {
        player = next;
        return true;
    }

    if (move.dx != 0 && move.dy != 0) {
        Position horizontal = {player.x + move.dx, player.y};
        if (canWalkTo(horizontal)) {
            player = horizontal;
            return true;
        }

        Position vertical = {player.x, player.y + move.dy};
        if (canWalkTo(vertical)) {
            player = vertical;
            return true;
        }
    }

    return false;
}

bool pickupAtPlayer(const Position& player, Inventory& inventory) {
    auto item = droppedItems.find(player);
    if (item != droppedItems.end()) {
        addItem(inventory, item->second.item, item->second.count);
        droppedItems.erase(item);
        return true;
    }

    auto spear = droppedSpears.find(player);
    if (spear != droppedSpears.end()) {
        droppedSpears.erase(spear);
        addItem(inventory, ItemType::Spear);
        return true;
    }

    return false;
}

void pruneOldInput(HeldInput& input, Clock::time_point now) {
    if (!isHeld(input.up, now)) input.up = Clock::time_point::min();
    if (!isHeld(input.down, now)) input.down = Clock::time_point::min();
    if (!isHeld(input.left, now)) input.left = Clock::time_point::min();
    if (!isHeld(input.right, now)) input.right = Clock::time_point::min();
}

void consumeMoveInput(HeldInput& input, const MoveInput& move) {
    input.queuedMove = {};
    if (move.dy < 0) input.up = Clock::time_point::min();
    if (move.dy > 0) input.down = Clock::time_point::min();
    if (move.dx < 0) input.left = Clock::time_point::min();
    if (move.dx > 0) input.right = Clock::time_point::min();
}

void refreshHeldKey(Clock::time_point& keyUntil, Clock::time_point now) {
    bool alreadyHeld = keyUntil != Clock::time_point::min() && keyUntil >= now;
    int duration = alreadyHeld ? heldKeyMs : initialHeldKeyMs;
    keyUntil = now + std::chrono::milliseconds(duration);
}

int sign(long long value) {
    if (value < 0)
        return -1;
    if (value > 0)
        return 1;
    return 0;
}

Position screenToWorld(int sx, int sy, const Position& player) {
    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    int worldHeight = height - bottomRows;
    int centerY = worldHeight / 2;
    int centerX = width / 2;

    return {player.x + sx - centerX, player.y + sy - centerY};
}
