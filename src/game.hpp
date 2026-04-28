#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <locale.h>
#include <ncurses.h>
#include <random>
#include <sstream>
#include <cwchar>
#include <string>
#include <thread>
#include <utility>
#include <unordered_map>
#include <vector>
#include <unordered_set>

constexpr int emptyStartRadius = 3;
constexpr int beachRadius = 4;
constexpr int tickRate = 128;
constexpr int renderEveryTicks = 1;
constexpr int ticksPerMove = 10;
constexpr int waterMoveTicks = ticksPerMove * 2;
constexpr int heldKeyMs = 95;
constexpr int initialHeldKeyMs = 120;
constexpr int bottomRows = 6;
constexpr int hotbarSlots = 9;
constexpr int inventorySlots = 27;
constexpr int chestSlots = 18;
constexpr int furnaceProcessTicks = tickRate * 4;
constexpr int worldChunkSize = 32;
constexpr int inventoryColumns = 6;
constexpr int attackDebounceMs = 140;
constexpr long long attackCooldownTicks = tickRate / 3;
constexpr int blockHitsToBreak = 3;
constexpr int cowScanRadius = 28;
constexpr int cowMoveTicks = 96;
constexpr int spiderScanRadius = 30;
constexpr int spiderMoveTicks = 48;
constexpr int spiderDamageTicks = tickRate;
constexpr int poisonDurationTicks = tickRate * 4;
constexpr int poisonDamageIntervalTicks = tickRate;
constexpr int sheepScanRadius = 28;
constexpr int sheepMoveTicks = 100;
constexpr int dayCycleTicks = tickRate * 240;
constexpr int nightStartTicks = dayCycleTicks * 3 / 4;
constexpr int hungerDrainTicks = tickRate * 28;
constexpr int thirstDrainTicks = tickRate * 8;
constexpr int damageTicks = tickRate * 3;
constexpr int healTicks = tickRate * 5;
constexpr int wheatGrowTicks = tickRate * 20;
constexpr int waterFlowTicks = tickRate / 2;
constexpr int waterFlowRadius = 40;
constexpr int maxWaterDepth = 4;
constexpr long long attackRadius = 4;
constexpr double terrainScale = 18.0;
constexpr double lakeScale = 34.0;
constexpr double forestScale = 24.0;
constexpr double cobblestoneLimit = 0.78;
constexpr double waterLimit = 0.82;
constexpr double treeLimit = 0.76;
const std::string saveDirectory = "worlds";
const std::string legacySavePath = "world.sav";
std::string currentSavePath = legacySavePath;
uint64_t worldSeed = 0;

struct WorldGeneration {
    long long terrainNoiseScale = static_cast<long long>(terrainScale);
    long long lakeNoiseScale = static_cast<long long>(lakeScale);
    long long forestNoiseScale = static_cast<long long>(forestScale);
    long long lakeOffsetX = -30000;
    long long lakeOffsetY = 30000;
    long long forestOffsetX = 7777;
    long long forestOffsetY = -7777;
    long long stoneOffsetX = 0;
    long long stoneOffsetY = 0;
    long long wheatOffsetX = -12345;
    long long wheatOffsetY = 9876;
    double cobblestoneLimit = ::cobblestoneLimit;
    double waterLimit = ::waterLimit;
    double treeLimit = ::treeLimit;
    double wheatLimit = 0.74;
    double ironLimit = 0.76;
};

WorldGeneration worldGeneration;

enum ColorPair {
    C_EMPTY = 1,
    C_COBBLESTONE,
    C_WATER,
    C_BEACH,
    C_TREE,
    C_COW,
    C_PLAYER,
    C_STATUS,
    C_DAMAGE_1,
    C_DAMAGE_2,
    C_WORKBENCH,
    C_FURNACE,
    C_DOOR,
    C_COW_BLACK,
    C_SHEEP_BLACK,
    C_BED,
    C_BAG,
    C_POISON
};

struct Position {
    long long x = 0;
    long long y = 0;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct MoveInput {
    int dx = 0;
    int dy = 0;
    bool quit = false;
    bool moved = false;
};

struct PositionHash {
    size_t operator()(const Position& position) const {
        uint64_t ux = static_cast<uint64_t>(position.x);
        uint64_t uy = static_cast<uint64_t>(position.y);
        return static_cast<size_t>(ux * 0x9e3779b97f4a7c15ULL ^
                                   uy * 0xc2b2ae3d27d4eb4fULL);
    }
};

using Clock = std::chrono::steady_clock;

std::unordered_set<Position, PositionHash> brokenBlocks;
std::unordered_map<Position, char, PositionHash> placedBlocks;
std::unordered_map<Position, int, PositionHash> blockDamage;
std::unordered_map<Position, std::array<char, worldChunkSize * worldChunkSize>, PositionHash> worldTileCache;
std::unordered_set<Position, PositionHash> seenCowSpawns;
std::unordered_set<Position, PositionHash> killedCowSpawns;
std::unordered_set<Position, PositionHash> seenSpiderSpawns;
std::unordered_set<Position, PositionHash> killedSpiderSpawns;
long long activeSpiderNight = -1;
std::unordered_set<Position, PositionHash> seenSheepSpawns;
std::unordered_set<Position, PositionHash> killedSheepSpawns;
std::unordered_set<Position, PositionHash> droppedSpears;
bool hasRespawnPoint = false;
Position respawnPoint;
bool hasDeathBag = false;
Position deathBagPos;
bool useUnicode = false;

struct HeldInput {
    Clock::time_point up = Clock::time_point::min();
    Clock::time_point down = Clock::time_point::min();
    Clock::time_point left = Clock::time_point::min();
    Clock::time_point right = Clock::time_point::min();
    MoveInput queuedMove;
    bool quit = false;
    bool attackQueued = false;
    bool inventoryToggleQueued = false;
    bool craftToggleQueued = false;
    bool craftAllQueued = false;
    bool interactQueued = false;
    int selectedSlotQueued = -1;
};

struct LookDirection {
    int dx = 1;
    int dy = 0;

    bool operator==(const LookDirection& other) const {
        return dx == other.dx && dy == other.dy;
    }
};

struct MouseState {
    int x = 0;
    int y = 0;
    bool known = false;
    bool attackQueued = false;
    bool placeQueued = false;
    Clock::time_point lastAttack = Clock::time_point::min();
};

enum class ItemType {
    None,
    Stone,
    Wood,
    RawMeat,
    CookedMeat,
    DirtyWater,
    BoiledWater,
    Milk,
    Workbench,
    Furnace,
    Door,
    Chest,
    Spear,
    Sand,
    Wheat,
    WheatSeeds,
    Fence,
    IronOre,
    IronIngot,
    Bucket,
    WaterBucket,
    WoodPickaxe,
    StonePickaxe,
    IronPickaxe,
    WoodAxe,
    StoneAxe,
    IronAxe,
    WoodShovel,
    StoneShovel,
    IronShovel,
    Bread,
    Shears,
    Wool,
    Bed,
    Log,
    Planks,
    Sapling
};

struct Slot {
    ItemType item = ItemType::None;
    int count = 0;
};

struct Inventory {
    std::array<Slot, inventorySlots> slots{};
    Slot held;
    bool open = false;
    bool actionMenuOpen = false;
    bool craftMenuOpen = false;
    bool chestOpen = false;
    bool furnaceOpen = false;
    Position activeChest;
    Position activeFurnace;
    int selectedSlot = 0;
    int cursor = 0;
    int actionCursor = 0;
    int craftCursor = 0;
};

std::unordered_map<Position, Slot, PositionHash> droppedItems;
std::unordered_map<Position, std::array<Slot, chestSlots>, PositionHash> chests;

struct FurnaceState {
    Slot slot;
    Slot fuel;
    int progress = 0;
    int burnTicks = 0;
};

std::unordered_map<Position, FurnaceState, PositionHash> furnaces;

struct Cow {
    Position pos;
    Position spawn;
    bool milked = false;
    Position previousPos;
    long long moveStartedTick = 0;
};

struct Spider {
    Position pos;
    Position spawn;
    bool poisonous = false;
    Position previousPos;
    long long moveStartedTick = 0;
};

struct Sheep {
    Position pos;
    Position spawn;
    bool sheared = false;
    Position previousPos;
    long long moveStartedTick = 0;
};

struct VisualState {
    Position playerPrevious;
    long long playerMoveStartedTick = 0;
};

struct PlayerStats {
    int hearts = 10;
    int hunger = 20;
    int thirst = 20;
    int poisonTicks = 0;
    int poisonDamageLeft = 0;
    int poisonDamageCooldown = 0;
};

struct ChatState {
    bool open = false;
    std::string input;
    std::string pendingLine;
    std::vector<std::string> messages;
};

enum class ActionType {
    MoveStack,
    Eat,
    Drink,
    Place,
    CraftPlanks,
    CraftWorkbench,
    CraftFurnace,
    CraftDoor,
    CraftChest,
    CraftSpear,
    CraftFence,
    CraftWoodPickaxe,
    CraftStonePickaxe,
    CraftIronPickaxe,
    CraftWoodAxe,
    CraftStoneAxe,
    CraftIronAxe,
    CraftWoodShovel,
    CraftStoneShovel,
    CraftIronShovel,
    CraftShears,
    CraftBed,
    CookMeat,
    BoilWater,
    SmeltIron,
    MilkCow,
    ToggleDoor,
    ThrowSpear
};

struct Action {
    ActionType type;
    std::string label;
};

bool isHeld(Clock::time_point pressedAt, Clock::time_point now);
void addItem(Inventory& inventory, ItemType item, int amount = 1);
void drawRoundedBox(int y, int x, int height, int width);

long long dayPhaseTick(long long tick) {
    long long phase = tick % dayCycleTicks;
    return phase < 0 ? phase + dayCycleTicks : phase;
}

bool isNightTick(long long tick) {
    return dayPhaseTick(tick) >= nightStartTicks;
}

long long nightIndexForTick(long long tick) {
    long long phase = dayPhaseTick(tick);
    long long day = tick / dayCycleTicks;
    if (tick < 0 && phase != 0)
        day--;
    return phase >= nightStartTicks ? day : -1;
}
