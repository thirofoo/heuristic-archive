#include <bits/stdc++.h>
#pragma GCC optimize("Ofast,omit-frame-pointer,inline,unroll-all-loops,no-stack-protector")
using namespace std;

namespace {

constexpr int MAXC = 8;
constexpr int DR[4] = {-1, 1, 0, 0};
constexpr int DC[4] = {0, 0, -1, 1};
constexpr char DCHAR[4] = {'U', 'D', 'L', 'R'};
constexpr int DIR_ORDER_COUNT = 2;
alignas(64) constexpr int8_t DIR_ORDERS[DIR_ORDER_COUNT][4] = {
    {0, 1, 2, 3}, {3, 2, 1, 0},
};
constexpr int MAX_TURN_LIMIT = 100000;
constexpr int SEARCH_TIME_LIMIT_MS = 1900;

constexpr int CUT_ENUM_MAX_DEPTH_HARD_MAX = 7;
constexpr int CUT_CANDIDATE_CAP = 64;
constexpr int CUT_KEEP_MIN_LEN = 5;
constexpr int INITIAL_SNAKE_LEN = 5;

constexpr long long SCORE_PREFIX_PRIMARY_WEIGHT = 25LL;
constexpr long long SCORE_PREFIX_PRIMARY_WEIGHT_LONG_BEST = 1'000'000LL;
constexpr int SCORE_PREFIX_PRIMARY_WEIGHT_LONG_BEST_THRESHOLD = 1000;
constexpr long long SCORE_FOOD_WALL_PENALTY_WEIGHT = 10LL;
constexpr long long SCORE_FOOD_CORNER_EXTRA_PENALTY_WEIGHT = 20LL;

constexpr int BEAM_WIDTH = 20;
constexpr int BEAM_TURN_CAP_MAX = 4000;
constexpr int BEAM_WIDTH_HARD_MAX = 160;
constexpr int WANDER_CANDIDATE_TOP_K_HARD_MAX = 1;
constexpr array<int, 17> BEAM_CANDIDATE_TOP_K_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    3, 3, 3, 3, 3, 3, 3, 3, 3,
};
constexpr array<int, 17> WANDER_LEN_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    1, 2, 2, 2, 2, 2, 4, 4, 6,
};
constexpr array<int, 17> CUT_ENUM_MAX_DEPTH_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    5, 5, 5, 5, 4, 4, 3, 3, 3,
};
constexpr array<int, 17> BEAM_WIDTH_MAX_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    160, 160, 160, 160, 160, 160, 160, 80, 80,
};
constexpr array<int, 17> CUT_APPLY_NODE_LIMIT_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    40, 40, 40, 40, 40, 40, 40, 20, 20,
};
constexpr array<int, 17> WANDER_APPLY_NODE_LIMIT_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    20, 20, 20, 20, 20, 20, 20, 20, 20,
};
constexpr array<int, 17> BEAM_CUT_CANDIDATE_TOP_K_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    5, 5, 4, 4, 2, 1, 1, 1, 1,
};
constexpr array<int, 17> WANDER_CANDIDATE_TOP_K_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
};

constexpr array<int, 17> NEAR_P_ALLOW_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    0, 4, 4, 4, 4, 4, 4, 4, 4,
};
constexpr array<int, 17> PCUT_APPLY_NODE_LIMIT_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    0, 40, 40, 40, 40, 40, 40, 40, 40,
};
constexpr array<int, 17> PCUT_CANDIDATE_TOP_K_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    0, 3, 3, 3, 3, 3, 3, 3, 5,
};

constexpr uint16_t INVALID_NEXT_POS = 0xFFFFu;
constexpr uint8_t NO_OVERLAP_POS = 0xFFu;

// Global lookup tables — stay at max size 256, initialized per-N.
uint16_t gCellIdxByPos[256];
uint16_t gNextPos[256][4];
uint8_t gWallByPos[256];
uint8_t gCornerByPos[256];
uint8_t gBlkByPos[256];
uint64_t gMaskByPos[256];
uint32_t gDirOrderNonce = 0;

inline const int8_t* chooseDirOrder() {
    return DIR_ORDERS[++gDirOrderNonce & 1];
}

struct PathPool {
    static constexpr int CAP = 1 << 22;
    uint32_t data[CAP];
    int sz = 0;
    void clear() { sz = 0; }
    bool hasSpace(int need = 1) const { return sz + need <= CAP; }
    int add(int8_t dir, int parent) {
        if (__builtin_expect(sz >= CAP, 0)) return -1;
        data[sz] = ((uint32_t)(parent + 1) << 2) | (uint32_t)(dir & 3);
        return sz++;
    }
    void extract(int tail, int len, int8_t* out) const {
        int idx = tail;
        for (int i = len - 1; i >= 0; --i) {
            out[i] = (int8_t)(data[idx] & 3);
            idx = (int)(data[idx] >> 2) - 1;
        }
    }
};

static PathPool gPool;

struct DirList {
    int8_t d[4];
    int8_t n = 0;
};

struct MoveResult {
    bool ok = false;
    bool bite = false;
};

struct MoveSeq {
    int8_t d[MAX_TURN_LIMIT];
    int len = 0;
    void clear() { len = 0; }
    int size() const { return len; }
};

struct DeadlineChecker {
    chrono::steady_clock::time_point deadline;
    uint32_t tick = 0;

    bool isOver() {
        // if ((++tick & 15u) != 0) return false;
        return chrono::steady_clock::now() >= deadline;
    }
};

// ============================================================
// Template: SHIFT=3,SIDE=8,CAP=64,MASK=63,BLOCKS=1 for N<=8
//           SHIFT=4,SIDE=16,CAP=256,MASK=255,BLOCKS=4 for N>8
// ============================================================
template<int SHIFT, int SIDE, int SCAP, int SMASK, int BLOCKS, int CMAXP1>
struct SolverT {

static inline uint8_t packPos(int r, int c) { return (uint8_t)((r << SHIFT) | c); }
static inline int posR(uint8_t p) { return p >> SHIFT; }
static inline int posC(uint8_t p) { return p & (SIDE - 1); }
static constexpr int NCELLS = SIDE * SIDE;

struct FixedPath {
    int8_t d[NCELLS];
    int16_t len = 0;
    bool empty() const { return len == 0; }
    void push_back(int8_t v) { d[len++] = v; }
    bool operator==(const FixedPath& o) const {
        return len == o.len && memcmp(d, o.d, len) == 0;
    }
};

static inline void reconstructPathBackward(
    FixedPath& path, int len, uint8_t goalPos,
    const uint8_t* parentPacked, const int8_t* pdir
) {
    path.len = (int16_t)len;
    uint8_t pos = goalPos;
    for (int idx = len - 1; idx >= 0; --idx) {
        path.d[idx] = pdir[pos];
        pos = parentPacked[pos];
    }
}

struct SnakeState {
    // Hot fields (accessed on every apply) — grouped at start for cache locality
    uint64_t bodyOcc[BLOCKS] = {};
    uint64_t foodOcc[BLOCKS] = {};
    int16_t bodyHead = 0, bodyLen = 0;
    int16_t colorLen = 0;
    int16_t colorHead = 0;
    int16_t foodCount = 0;
    int16_t wallFoodCount = 0;
    int16_t cornerFoodCount = 0;
    uint8_t overlapPos = NO_OVERLAP_POS;
    // 3 bytes padding here to align int
    int turn = 0;
    // Large arrays
    uint8_t bodyBuf[SCAP];
    uint8_t colorBuf[(SCAP + 1) >> 1] = {};
    uint64_t foodColorOcc[CMAXP1][BLOCKS] = {};

    SnakeState() = default;

    uint8_t bodyAt(int i) const { return bodyBuf[(bodyHead + i) & SMASK]; }
    int8_t colorAtRaw(int idx) const {
        uint8_t x = colorBuf[idx >> 1];
        return (int8_t)((idx & 1) ? (x >> 4) : (x & 15));
    }
    void colorSetRaw(int idx, int8_t v) {
        uint8_t& x = colorBuf[idx >> 1];
        uint8_t c = (uint8_t)v & 15;
        if (idx & 1) {
            x = (uint8_t)((x & 0x0F) | (c << 4));
        } else {
            x = (uint8_t)((x & 0xF0) | c);
        }
    }
    int8_t colorAt(int i) const { return colorAtRaw((colorHead + i) & SMASK); }

    void bodyPushFront(uint8_t v) {
        bodyHead = (bodyHead - 1) & SMASK;
        bodyBuf[bodyHead] = v;
        ++bodyLen;
    }
    void bodyPopBack() { --bodyLen; }
    uint8_t bodyFront() const { return bodyBuf[bodyHead]; }
    uint8_t bodyBack() const { return bodyBuf[(bodyHead + bodyLen - 1) & SMASK]; }

    void colorPushBack(int8_t v) {
        colorSetRaw((colorHead + colorLen) & SMASK, v);
        ++colorLen;
    }

    bool hasFoodAtIdx(int blk, uint64_t mask) const {
        return (foodOcc[blk] & mask) != 0;
    }
    int foodColorAtIdx(int blk, uint64_t mask) const {
        for (int c = 1; c < CMAXP1; ++c)
            if (foodColorOcc[c][blk] & mask) return c;
        return 0;
    }
    bool isFoodColor(int blk, uint64_t mask, int tc) const {
        return (foodColorOcc[tc][blk] & mask) != 0;
    }

    void foodColorOccSetPos(int color, uint8_t pos) {
        foodColorOcc[color][gBlkByPos[pos]] |= gMaskByPos[pos];
    }
    void foodColorOccClearPos(int color, uint8_t pos) {
        foodColorOcc[color][gBlkByPos[pos]] &= ~gMaskByPos[pos];
    }
    void onFoodAddedPos(uint8_t pos, int color) {
        ++foodCount;
        int blk = gBlkByPos[pos]; uint64_t mask = gMaskByPos[pos];
        foodOcc[blk] |= mask;
        foodColorOcc[color][blk] |= mask;
        wallFoodCount += gWallByPos[pos];
        cornerFoodCount += gCornerByPos[pos];
    }
    void onFoodRemovedPos(uint8_t pos, int color) {
        --foodCount;
        int blk = gBlkByPos[pos]; uint64_t mask = gMaskByPos[pos];
        foodOcc[blk] &= ~mask;
        foodColorOcc[color][blk] &= ~mask;
        wallFoodCount -= gWallByPos[pos];
        cornerFoodCount -= gCornerByPos[pos];
    }
    bool bodyOccupiedPos(uint8_t pos) const {
        return (bodyOcc[gBlkByPos[pos]] & gMaskByPos[pos]) != 0;
    }
    void bodyOccSetPos(uint8_t pos) {
        bodyOcc[gBlkByPos[pos]] |= gMaskByPos[pos];
    }
    void bodyOccRemoveOnePos(uint8_t pos) {
        if (overlapPos == pos) {
            overlapPos = NO_OVERLAP_POS;
            return;
        }
        bodyOcc[gBlkByPos[pos]] &= ~gMaskByPos[pos];
    }

    int prefixLen(const int8_t* desired, int desiredLen, int knownPrefix = 0) const {
        int lim = min((int)colorLen, desiredLen);
        if (lim <= knownPrefix) return lim;
        int i = knownPrefix;
        // Bulk path: when rawIdx is even (byte-aligned) and no wrap, unpack 8 nibbles at once
        while (i + 8 <= lim) {
            int rawIdx = (colorHead + i) & SMASK;
            if ((rawIdx & 1) == 0 && rawIdx + 8 <= SCAP) {
                uint32_t packed;
                memcpy(&packed, &colorBuf[rawIdx >> 1], 4);
                // Compare 8 nibbles via single uint64 XOR
                uint64_t des8;
                memcpy(&des8, &desired[i], 8);
                uint64_t unp =
                    ((uint64_t)(packed & 15)) |
                    ((uint64_t)((packed >> 4) & 15) << 8) |
                    ((uint64_t)((packed >> 8) & 15) << 16) |
                    ((uint64_t)((packed >> 12) & 15) << 24) |
                    ((uint64_t)((packed >> 16) & 15) << 32) |
                    ((uint64_t)((packed >> 20) & 15) << 40) |
                    ((uint64_t)((packed >> 24) & 15) << 48) |
                    ((uint64_t)((packed >> 28) & 15) << 56);
                uint64_t diff = unp ^ des8;
                if (diff) {
                    // Find first differing byte
                    return i + (__builtin_ctzll(diff) >> 3);
                }
                i += 8;
            } else {
                break;
            }
        }
        // Scalar tail
        for (; i < lim; ++i) {
            if (colorAt(i) != desired[i]) return i;
        }
        return lim;
    }

    DirList legalDirs() const {
        DirList dl;
        if (bodyLen == 0) return dl;
        uint8_t head = bodyFront();
        uint8_t neck = 255;
        if (bodyLen >= 2) neck = bodyAt(1);
        const int8_t* dirOrder = chooseDirOrder();
        for (int di = 0; di < 4; ++di) {
            int d = dirOrder[di];
            uint16_t next = gNextPos[head][d];
            if (next == INVALID_NEXT_POS) continue;
            uint8_t npos = (uint8_t)next;
            if (npos == neck) continue;
            dl.d[dl.n++] = (int8_t)d;
        }
        return dl;
    }

    MoveResult apply(int d) {
        MoveResult ret;
        if (__builtin_expect(bodyLen == 0, 0)) return ret;

        uint8_t head = bodyFront();
        uint16_t next = gNextPos[head][d];
        if (__builtin_expect(next == INVALID_NEXT_POS, 0)) return ret;
        uint8_t npos = (uint8_t)next;
        if (bodyLen >= 2 && npos == bodyAt(1)) return ret;

        int nposBlk = gBlkByPos[npos];
        uint64_t nposMask = gMaskByPos[npos];
        bool occupied = (bodyOcc[nposBlk] & nposMask) != 0;
        bool eats = (foodOcc[nposBlk] & nposMask) != 0;
        uint8_t tail = bodyBack();
        bool biteCandidate = occupied && (eats || npos != tail || overlapPos == npos);

        if (__builtin_expect(!eats && !biteCandidate, 1)) {
            bodyPushFront(npos);
            bodyOcc[nposBlk] |= nposMask;
            bodyPopBack();
            if (tail != npos) {
                if (overlapPos == tail) {
                    overlapPos = NO_OVERLAP_POS;
                } else {
                    bodyOcc[gBlkByPos[tail]] &= ~gMaskByPos[tail];
                }
            }
            overlapPos = (bodyBack() == npos) ? npos : NO_OVERLAP_POS;
            ++turn;
            ret.ok = true;
            return ret;
        }

        bodyPushFront(npos);
        bodyOcc[nposBlk] |= nposMask;

        if (eats) {
            int eatenColor = foodColorAtIdx(nposBlk, nposMask);
            colorPushBack((int8_t)eatenColor);
            --foodCount;
            foodOcc[nposBlk] &= ~nposMask;
            foodColorOcc[eatenColor][nposBlk] &= ~nposMask;
            wallFoodCount -= gWallByPos[npos];
            cornerFoodCount -= gCornerByPos[npos];
        } else {
            bodyPopBack();
            if (tail != npos) {
                if (overlapPos == tail) {
                    overlapPos = NO_OVERLAP_POS;
                } else {
                    bodyOcc[gBlkByPos[tail]] &= ~gMaskByPos[tail];
                }
            }
        }

        if (__builtin_expect(!biteCandidate, 1)) {
            ++turn;
            ret.ok = true;
            return ret;
        }

        int k = bodyLen;
        bool bitten = false;
        if (__builtin_expect(biteCandidate, 0)) {
            int h = 1;
            int bi = (bodyHead + 1) & SMASK;
            for (; h <= k - 2; ++h) {
                if (bodyBuf[bi] == npos) {
                    int restoreBodyIdx = (bi + 1) & SMASK;
                    int restoreColorIdx = (colorHead + h + 1) & SMASK;
                    int restoredFood = 0;
                    int restoredWall = 0;
                    int restoredCorner = 0;
                    for (int p = h + 1; p < k; ++p) {
                        uint8_t bp = bodyBuf[restoreBodyIdx];
                        int bpBlk = gBlkByPos[bp];
                        uint64_t bpMask = gMaskByPos[bp];
                        if (overlapPos == bp) {
                            overlapPos = NO_OVERLAP_POS;
                        } else {
                            bodyOcc[bpBlk] &= ~bpMask;
                        }
                        int color = colorAtRaw(restoreColorIdx);
                        foodOcc[bpBlk] |= bpMask;
                        foodColorOcc[color][bpBlk] |= bpMask;
                        ++restoredFood;
                        restoredWall += gWallByPos[bp];
                        restoredCorner += gCornerByPos[bp];
                        restoreBodyIdx = (restoreBodyIdx + 1) & SMASK;
                        restoreColorIdx = (restoreColorIdx + 1) & SMASK;
                    }
                    foodCount += (int16_t)restoredFood;
                    wallFoodCount += (int16_t)restoredWall;
                    cornerFoodCount += (int16_t)restoredCorner;
                    bodyLen = (int16_t)(h + 1);
                    colorLen = (int16_t)(h + 1);
                    overlapPos = npos;
                    ret.bite = true;
                    bitten = true;
                    break;
                }
                bi = (bi + 1) & SMASK;
            }
        }

        if (__builtin_expect(!bitten && !eats, 1)) {
            // After a normal move the old tail disappears, so the only possible
            // duplicate is the new head sharing the new tail's cell.
            overlapPos = (bodyBack() == npos) ? npos : NO_OVERLAP_POS;
        }

        ++turn;
        ret.ok = true;
        return ret;
    }
};

struct CutCandidate {
    int8_t seq[CUT_ENUM_MAX_DEPTH_HARD_MAX];
    int8_t seqLen = 0;
    int16_t afterBodyLen = 0;
    bool adjacentToPrefix = false;
};

struct CutCandidateList {
    CutCandidate data[CUT_CANDIDATE_CAP];
    int size = 0;

    bool empty() const { return size == 0; }
    void push_back(const CutCandidate& cand) {
        if (size < CUT_CANDIDATE_CAP) data[size++] = cand;
    }
};

// --- Solver data ---
int N = 0, M = 0, C = 0;
int8_t desired[NCELLS];
int nnCells = 0;
uint8_t manhattanDist[NCELLS][NCELLS] = {};

SnakeState state;
MoveSeq movesDirs;
uint32_t tx = 123456789u;
uint32_t ty = 362436069u;
uint32_t tz = 521288629u;
uint32_t tw = 88675123u;
int activeTurnLimit = MAX_TURN_LIMIT;
long long activePrefixPrimaryWeight = SCORE_PREFIX_PRIMARY_WEIGHT_LONG_BEST;

struct BeamNode {
    SnakeState st;
    int pathTail = -1;
    int pathLen = 0;
    int prefix = 0;
    int maxPrefix = 0;
    int optimisticLB = -1;
    uint64_t stateKey = 0;
    long long eval = (long long)4e18;
};

struct BeamKey { long long eval; int maxPrefix; int turn; uint64_t stateKey; };

array<vector<BeamNode>, BEAM_TURN_CAP_MAX + 1> layersBuf;
array<vector<BeamKey>, BEAM_TURN_CAP_MAX + 1> layerKeysBuf;
int layerRunStamp[BEAM_TURN_CAP_MAX + 1] = {};
int currentRunStamp = 1;

mutable uint16_t bfsSeenStamp[NCELLS] = {};
mutable uint16_t bfsStamp = 1;
mutable uint16_t releaseStamp[NCELLS] = {};
mutable uint16_t releaseCur = 1;
mutable int16_t releaseValue[NCELLS] = {};
mutable uint8_t bfsParent[NCELLS] = {};
mutable int8_t bfsPdir[NCELLS] = {};

SolverT() {
    constexpr uint64_t seed = 0x8f3a2b1c7d9e5a61ULL;
    uint32_t s = (uint32_t)(seed ^ (seed >> 32));
    tx ^= s;
    ty ^= s * 0x9e3779b9u + 0x7f4a7c15u;
    tz ^= s * 0x85ebca6bu + 0xc2b2ae35u;
    tw ^= s * 0x27d4eb2fu + 0x165667b1u;
    for (int i = 0; i < 16; ++i) (void)randInt();
}

inline uint32_t randInt() {
    uint32_t tt = (tx ^ (tx << 11));
    tx = ty;
    ty = tz;
    tz = tw;
    tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8));
    return tw;
}

inline int randRange(int l, int r) {
    return l + (int)(randInt() % (uint32_t)(r - l + 1));
}

void readInput(int n, int m, int c) {
    N = n; M = m; C = c;
    nnCells = N * N;
    memset(gCellIdxByPos, 0xFF, sizeof(gCellIdxByPos));
    memset(gNextPos, 0xFF, sizeof(gNextPos));
    memset(gWallByPos, 0, sizeof(gWallByPos));
    memset(gCornerByPos, 0, sizeof(gCornerByPos));
    memset(gBlkByPos, 0, sizeof(gBlkByPos));
    memset(gMaskByPos, 0, sizeof(gMaskByPos));
    for (int r = 0; r < SIDE; ++r) {
        for (int cc = 0; cc < SIDE; ++cc) {
            uint8_t pos = packPos(r, cc);
            if (r < N && cc < N) {
                int idx = r * N + cc;
                gCellIdxByPos[pos] = (uint16_t)idx;
                gBlkByPos[pos] = (uint8_t)(idx >> 6);
                gMaskByPos[pos] = 1ULL << (idx & 63);
                for (int d = 0; d < 4; ++d) {
                    int nr = r + DR[d], nc = cc + DC[d];
                    if ((unsigned)nr < (unsigned)N && (unsigned)nc < (unsigned)N) {
                        gNextPos[pos][d] = packPos(nr, nc);
                    }
                }
                gWallByPos[pos] = (uint8_t)(r == 0 || cc == 0 || r == N - 1 || cc == N - 1);
                gCornerByPos[pos] = (uint8_t)((r == 0 || r == N - 1) && (cc == 0 || cc == N - 1));
            }
        }
    }
    for (int i = 0; i < nnCells; ++i) {
        int r1 = i / N, c1 = i - r1 * N;
        for (int j = 0; j < nnCells; ++j) {
            int r2 = j / N, c2 = j - r2 * N;
            manhattanDist[i][j] = (uint8_t)(abs(r1 - r2) + abs(c1 - c2));
        }
    }
    for (int i = 0; i < M; ++i) {
        int x; cin >> x;
        desired[i] = (int8_t)x;
    }

    memset(state.foodOcc, 0, sizeof(state.foodOcc));
    memset(state.bodyOcc, 0, sizeof(state.bodyOcc));
    state.overlapPos = NO_OVERLAP_POS;
    state.foodCount = 0;
    state.wallFoodCount = 0;
    state.cornerFoodCount = 0;
    memset(state.foodColorOcc, 0, sizeof(state.foodColorOcc));
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int x; cin >> x;
            uint8_t pos = packPos(i, j);
            if (x != 0) {
                state.onFoodAddedPos(pos, x);
            }
        }
    }

    state.bodyHead = 0;
    state.bodyLen = INITIAL_SNAKE_LEN;
    state.bodyBuf[0] = packPos(4, 0);
    state.bodyBuf[1] = packPos(3, 0);
    state.bodyBuf[2] = packPos(2, 0);
    state.bodyBuf[3] = packPos(1, 0);
    state.bodyBuf[4] = packPos(0, 0);
    for (int i = 0; i < INITIAL_SNAKE_LEN; ++i) {
        state.bodyOccSetPos(state.bodyBuf[i]);
    }

    state.colorHead = 0;
    state.colorLen = 0;
    memset(state.colorBuf, 0, sizeof(state.colorBuf));
    for (int i = 0; i < INITIAL_SNAKE_LEN; ++i) state.colorPushBack(1);

    state.turn = 0;
    movesDirs.clear();
    activeTurnLimit = MAX_TURN_LIMIT;
}

int dirBetween(uint8_t a, uint8_t b) const {
    int ar = posR(a), ac = posC(a), br = posR(b), bc = posC(b);
    for (int d = 0; d < 4; ++d) {
        if (ar + DR[d] == br && ac + DC[d] == bc) return d;
    }
    return -1;
}

FixedPath buildRecoveryPathByOldBody(
    const uint8_t* savedBodyBuf,
    int16_t savedBodyHead,
    int16_t savedBodyLen,
    const SnakeState& afterBite,
    int prefixBefore
) const {
    FixedPath plan;
    int lenAfter = afterBite.bodyLen;
    if (prefixBefore <= lenAfter) return plan;
    if (savedBodyLen == 0 || afterBite.bodyLen == 0) return plan;
    int startIdx = lenAfter - 1;
    int endIdx = prefixBefore - 2;
    if (startIdx < 0 || endIdx < startIdx || endIdx >= savedBodyLen) return plan;
    uint8_t cur = afterBite.bodyFront();
    for (int idx = startIdx; idx <= endIdx; ++idx) {
        uint8_t nxt = savedBodyBuf[(savedBodyHead + idx) & SMASK];
        int d = dirBetween(cur, nxt);
        if (d < 0) { plan.len = 0; return plan; }
        plan.push_back((int8_t)d);
        cur = nxt;
    }
    return plan;
}

bool hasFoodColor(const SnakeState& st, int color) const {
    if ((unsigned)color >= CMAXP1) return false;
    for (int blk = 0; blk < BLOCKS; ++blk) {
        if (st.foodColorOcc[color][blk]) return true;
    }
    return false;
}

void beginBfsVisit() const {
    ++bfsStamp;
    if (__builtin_expect(bfsStamp == 0, 0)) {
        memset(bfsSeenStamp, 0, sizeof(bfsSeenStamp));
        bfsStamp = 1;
    }
}

void buildReleaseSparse(const SnakeState& st) const {
    ++releaseCur;
    if (__builtin_expect(releaseCur == 0, 0)) {
        memset(releaseStamp, 0, sizeof(releaseStamp));
        releaseCur = 1;
    }
    for (int i = 1; i < st.bodyLen; ++i) {
        uint8_t bp = st.bodyAt(i);
        int rel = st.bodyLen - 1 - i;
        if (rel < 1) rel = 1;
        if (releaseStamp[bp] != releaseCur) {
            releaseStamp[bp] = releaseCur;
            releaseValue[bp] = (int16_t)rel;
        } else if (releaseValue[bp] < rel) {
            releaseValue[bp] = (int16_t)rel;
        }
    }
}

inline int releaseAt(uint8_t pos) const {
    return releaseStamp[pos] == releaseCur ? releaseValue[pos] : 0;
}

// Quick check: is target food adjacent to head and reachable in 1 step?
// Returns number of 1-step paths found (0 if none).
int findAdjacentTargetPaths(
    const SnakeState& st, int targetColor, int topK, FixedPath* out
) const {
    if (st.bodyLen == 0) return 0;
    uint8_t head = st.bodyFront();
    uint8_t neck = (st.bodyLen >= 2) ? st.bodyAt(1) : 255;
    int cnt = 0;
    for (int d = 0; d < 4 && cnt < topK; ++d) {
        uint16_t next = gNextPos[head][d];
        if (next == INVALID_NEXT_POS) continue;
        uint8_t npos = (uint8_t)next;
        if (npos == neck) continue;
        if (!st.isFoodColor(gBlkByPos[npos], gMaskByPos[npos], targetColor)) continue;
        // Check body collision: if body occupies npos and release time > 1, skip
        if (st.bodyOccupiedPos(npos)) {
            if (releaseAt(npos) > 1) continue;
        }
        out[cnt].len = 1;
        out[cnt].d[0] = (int8_t)d;
        ++cnt;
    }
    return cnt;
}

int findDynamicStrictPathsToTarget(
    const SnakeState& st, int targetColor, int topK, FixedPath* out
) const {
    if (st.bodyLen == 0) return 0;
    if (!hasFoodColor(st, targetColor)) return 0;
    topK = max(1, topK);
    // buildReleaseSparse must be called by the caller before this
    beginBfsVisit();

    uint8_t head = st.bodyFront();
    uint8_t neck = 255;
    if (st.bodyLen >= 2) neck = st.bodyAt(1);

    uint8_t bq[NCELLS];
    int16_t bqd[NCELLS];
    int qt = 0;
    bfsSeenStamp[head] = bfsStamp;

    struct Goal { int16_t dist; uint8_t pos; };
    Goal goals[NCELLS];
    int gn = 0;
    int goalMaxDist = -1;
    const int8_t* dirOrder = chooseDirOrder();

    // Precompute non-target food bitmask for fast inner loop filtering
    uint64_t nonTargetFood[BLOCKS];
    for (int b = 0; b < BLOCKS; ++b)
        nonTargetFood[b] = st.foodOcc[b] & ~st.foodColorOcc[targetColor][b];

    // Expand head neighbors separately (skip neck, no neck check in main loop)
    for (int di = 0; di < 4; ++di) {
        int d = dirOrder[di];
        uint16_t next = gNextPos[head][d];
        if (next == INVALID_NEXT_POS) continue;
        uint8_t npos = (uint8_t)next;
        if (npos == neck) continue;
        if (nonTargetFood[gBlkByPos[npos]] & gMaskByPos[npos]) continue;
        if (1 < releaseAt(npos)) continue;
        if (bfsSeenStamp[npos] == bfsStamp) continue;
        bfsSeenStamp[npos] = bfsStamp;
        bfsParent[npos] = head;
        bfsPdir[npos] = (int8_t)d;
        // Check if this neighbor is a goal
        if (st.isFoodColor(gBlkByPos[npos], gMaskByPos[npos], targetColor)) {
            goals[gn++] = {1, npos};
            goalMaxDist = 1;
        } else {
            bq[qt] = npos;
            bqd[qt] = 1;
            ++qt;
        }
    }

    int qh = 0;
    while (qh < qt) {
        uint8_t pos = bq[qh];
        int t = bqd[qh];
        ++qh;
        if (gn >= topK && t > goalMaxDist) break;
        for (int di = 0; di < 4; ++di) {
            int d = dirOrder[di];
            uint16_t next = gNextPos[pos][d];
            if (next == INVALID_NEXT_POS) continue;
            uint8_t npos = (uint8_t)next;
            if (nonTargetFood[gBlkByPos[npos]] & gMaskByPos[npos]) continue;
            int nt = t + 1;
            if (nt < releaseAt(npos)) continue;
            if (bfsSeenStamp[npos] == bfsStamp) continue;
            bfsSeenStamp[npos] = bfsStamp;
            bfsParent[npos] = pos;
            bfsPdir[npos] = (int8_t)d;
            if (st.isFoodColor(gBlkByPos[npos], gMaskByPos[npos], targetColor)) {
                goals[gn++] = {(int16_t)nt, npos};
                goalMaxDist = nt;
            } else {
                bq[qt] = npos;
                bqd[qt] = (int16_t)nt;
                ++qt;
            }
        }
    }
    if (gn == 0) return 0;

    int take = min(topK, gn);
    int cnt = 0;
    for (int gi = 0; gi < take; ++gi) {
        FixedPath& path = out[cnt];
        reconstructPathBackward(path, goals[gi].dist, goals[gi].pos, bfsParent, bfsPdir);
        if (path.len > 0) ++cnt;
    }
    return cnt;
}

// Permissive shortest BFS. When strict route is unavailable, prioritize speed
// and path length instead of minimizing the number of non-target foods crossed.
FixedPath findPreferFoodAvoidPathToTarget(const SnakeState& st, int targetColor) const {
    FixedPath result;
    if (st.bodyLen == 0) return result;
    if (!hasFoodColor(st, targetColor)) return result;
    // buildReleaseSparse must be called by the caller before this
    beginBfsVisit();

    uint8_t head = st.bodyFront();
    uint8_t neck = 255;
    if (st.bodyLen >= 2) neck = st.bodyAt(1);

    uint8_t q[NCELLS];
    int16_t qd[NCELLS];
    int qt = 0;
    bfsSeenStamp[head] = bfsStamp;
    const int8_t* dirOrder = chooseDirOrder();

    uint8_t goalPos = 0;
    int goalDist = 0;
    bool goalFound = false;

    // Expand head neighbors separately (skip neck, remove neck check from main loop)
    for (int di = 0; di < 4; ++di) {
        int d = dirOrder[di];
        uint16_t next = gNextPos[head][d];
        if (next == INVALID_NEXT_POS) continue;
        uint8_t npos = (uint8_t)next;
        if (npos == neck) continue;
        if (1 < releaseAt(npos)) continue;
        if (bfsSeenStamp[npos] == bfsStamp) continue;
        bfsSeenStamp[npos] = bfsStamp;
        bfsParent[npos] = head;
        bfsPdir[npos] = (int8_t)d;
        if (st.isFoodColor(gBlkByPos[npos], gMaskByPos[npos], targetColor)) {
            goalPos = npos;
            goalDist = 1;
            goalFound = true;
            break;
        }
        q[qt] = npos;
        qd[qt] = 1;
        ++qt;
    }

    int qh = 0;
    while (!goalFound && qh < qt) {
        uint8_t pos = q[qh];
        int t = qd[qh];
        ++qh;
        for (int di = 0; di < 4; ++di) {
            int d = dirOrder[di];
            uint16_t next = gNextPos[pos][d];
            if (next == INVALID_NEXT_POS) continue;
            uint8_t npos = (uint8_t)next;
            int nt = t + 1;
            if (nt < releaseAt(npos)) continue;
            if (bfsSeenStamp[npos] == bfsStamp) continue;
            bfsSeenStamp[npos] = bfsStamp;
            bfsParent[npos] = pos;
            bfsPdir[npos] = (int8_t)d;
            if (st.isFoodColor(gBlkByPos[npos], gMaskByPos[npos], targetColor)) {
                goalPos = npos;
                goalDist = nt;
                goalFound = true;
                break;
            }
            q[qt] = npos;
            qd[qt] = (int16_t)nt;
            ++qt;
        }
    }

    if (!goalFound) return result;
    reconstructPathBackward(result, goalDist, goalPos, bfsParent, bfsPdir);
    return result;
}

bool isTargetAdjacentToPrefixSegment(const SnakeState& st, int anchorLen, int targetColor) const {
    if (anchorLen <= 0 || anchorLen > st.bodyLen) return false;
    uint8_t bp = st.bodyAt(anchorLen - 1);
    for (int d = 0; d < 4; ++d) {
        uint16_t next = gNextPos[bp][d];
        if (next == INVALID_NEXT_POS) continue;
        uint8_t np = (uint8_t)next;
        if (st.isFoodColor(gBlkByPos[np], gMaskByPos[np], targetColor)) return true;
    }
    return false;
}

void enumerateCutCandidatesDFS(
    SnakeState& cur, int depth, int maxDepth,
    int baseL, int targetColor,
    int8_t seq[CUT_ENUM_MAX_DEPTH_HARD_MAX], int seqLen,
    CutCandidateList& out
) const {
    if (depth >= maxDepth) return;
    uint8_t head = cur.bodyFront();
    uint8_t neck = (cur.bodyLen >= 2) ? cur.bodyAt(1) : 255;
    uint8_t tail = cur.bodyBack();
    for (int d = 0; d < 4; ++d) {
        seq[seqLen] = (int8_t)d;
        int newLen = seqLen + 1;

        uint16_t next = gNextPos[head][d];
        if (next == INVALID_NEXT_POS) continue;
        uint8_t npos = (uint8_t)next;
        if (npos == neck) continue;
        int nposBlk = gBlkByPos[npos];
        uint64_t nposMask = gMaskByPos[npos];
        bool occupied = cur.bodyOccupiedPos(npos);
        bool eats = cur.hasFoodAtIdx(nposBlk, nposMask);
        bool biteCandidate = occupied && (eats || npos != tail || cur.overlapPos == npos);

        if (biteCandidate) {
            // Bite is terminal — use full copy (no recursion needed)
            SnakeState nxt = cur;
            MoveResult ret = nxt.apply(d);
            if (!ret.ok || !ret.bite) continue;
            int lenAfter = nxt.bodyLen;
            if (lenAfter >= CUT_KEEP_MIN_LEN && (lenAfter % 2 == 1)) {
                int anchorLen = min(baseL, lenAfter);
                CutCandidate cand;
                memcpy(cand.seq, seq, newLen);
                cand.seqLen = (int8_t)newLen;
                cand.afterBodyLen = (int16_t)lenAfter;
                cand.adjacentToPrefix = isTargetAdjacentToPrefixSegment(nxt, anchorLen, targetColor);
                out.push_back(cand);
            }
        } else {
            // Non-bite: apply in-place, recurse, then undo
            // Save state needed for undo
            int16_t svBodyHead = cur.bodyHead;
            int16_t svBodyLen = cur.bodyLen;
            uint8_t svOverlapPos = cur.overlapPos;
            int svTurn = cur.turn;
            // Save bodyOcc blocks
            uint64_t svBodyOcc[BLOCKS];
            memcpy(svBodyOcc, cur.bodyOcc, sizeof(svBodyOcc));
            // Save the bodyBuf slot that will be overwritten by bodyPushFront
            int16_t newHeadSlot = (svBodyHead - 1) & SMASK;
            uint8_t svBodyBufAtNewHead = cur.bodyBuf[newHeadSlot];

            // Eat-specific saves
            uint64_t svFoodOccBlk = cur.foodOcc[nposBlk];
            int16_t svColorLen = cur.colorLen;
            int16_t svFoodCount = cur.foodCount;
            int16_t svWallFoodCount = cur.wallFoodCount;
            int16_t svCornerFoodCount = cur.cornerFoodCount;
            // Save colorBuf byte and foodColorOcc block that may change
            int colorBufSlot = ((cur.colorHead + cur.colorLen) & SMASK) >> 1;
            uint8_t svColorBufByte = cur.colorBuf[colorBufSlot];
            uint64_t svFoodColorOccBlk = 0;
            int svEatenColor = eats ? cur.foodColorAtIdx(nposBlk, nposMask) : 0;
            if (eats && (unsigned)svEatenColor < CMAXP1) {
                svFoodColorOccBlk = cur.foodColorOcc[svEatenColor][nposBlk];
            }

            MoveResult ret = cur.apply(d);
            if (ret.ok && !ret.bite) {
                enumerateCutCandidatesDFS(cur, depth + 1, maxDepth, baseL, targetColor, seq, newLen, out);
            }

            // Undo: restore all modified fields
            cur.bodyHead = svBodyHead;
            cur.bodyLen = svBodyLen;
            cur.overlapPos = svOverlapPos;
            cur.turn = svTurn;
            memcpy(cur.bodyOcc, svBodyOcc, sizeof(svBodyOcc));
            cur.bodyBuf[newHeadSlot] = svBodyBufAtNewHead;

            if (eats) {
                cur.foodOcc[nposBlk] = svFoodOccBlk;
                cur.colorLen = svColorLen;
                cur.colorBuf[colorBufSlot] = svColorBufByte;
                cur.foodCount = svFoodCount;
                cur.wallFoodCount = svWallFoodCount;
                cur.cornerFoodCount = svCornerFoodCount;
                if ((unsigned)svEatenColor < CMAXP1) {
                    cur.foodColorOcc[svEatenColor][nposBlk] = svFoodColorOccBlk;
                }
            }
        }
    }
}

CutCandidateList enumerateCutCandidates(
    const SnakeState& src, int baseL, int targetColor, int maxDepth
) const {
    CutCandidateList out;
    int8_t seq[CUT_ENUM_MAX_DEPTH_HARD_MAX];
    SnakeState mutableSrc = src;
    enumerateCutCandidatesDFS(mutableSrc, 0, maxDepth, baseL, targetColor, seq, 0, out);
    return out;
}

CutCandidateList chooseTopCutCandidatesForBeam(
    const SnakeState& src, int baseL, int targetColor, int topK
) const {
    topK = max(1, topK);
    auto cands = enumerateCutCandidates(src, baseL, targetColor, CUT_ENUM_MAX_DEPTH_BY_N[N]);
    if (cands.empty()) return {};
    int wrongSuffix = max(0, (int)src.bodyLen - baseL);

    // Rule-based comparator: a is better than b?
    // 1) rmw > 0 preferred (actually removes wrong suffix)
    // 2) rw smaller (less wrong suffix remaining)
    // 3) lp smaller (less prefix loss)
    // 4) adj true preferred
    // 5) seqLen smaller
    auto betterCut = [&](const CutCandidate& a, const CutCandidate& b) {
        int a_rw = max(0, (int)a.afterBodyLen - baseL);
        int b_rw = max(0, (int)b.afterBodyLen - baseL);
        int a_lp = max(0, baseL - (int)a.afterBodyLen);
        int b_lp = max(0, baseL - (int)b.afterBodyLen);
        int a_rmw = max(0, wrongSuffix - a_rw);
        int b_rmw = max(0, wrongSuffix - b_rw);
        bool a_removes = (a_rmw > 0);
        bool b_removes = (b_rmw > 0);
        if (a_removes != b_removes) return a_removes;
        if (a_rw != b_rw) return a_rw < b_rw;
        if (a_lp != b_lp) return a_lp < b_lp;
        if (a.adjacentToPrefix != b.adjacentToPrefix) return a.adjacentToPrefix;
        return a.seqLen < b.seqLen;
    };

    // Selection sort for top-k
    int indices[CUT_CANDIDATE_CAP];
    for (int i = 0; i < cands.size; ++i) indices[i] = i;
    CutCandidateList out;
    int take = min(topK, cands.size);
    for (int k = 0; k < take; ++k) {
        int bestJ = k;
        for (int j = k + 1; j < cands.size; ++j) {
            if (betterCut(cands.data[indices[j]], cands.data[indices[bestJ]])) {
                bestJ = j;
            }
        }
        if (bestJ != k) { int tmp = indices[k]; indices[k] = indices[bestJ]; indices[bestJ] = tmp; }
        out.push_back(cands.data[indices[k]]);
    }
    return out;
}

// BFS to find shortest paths from head to body segments whose bite gives
// afterBodyLen in [p - NEAR_P_ALLOW_BY_N[N], p] (i.e., afterBodyLen <= p, close to p).
// Key formula: biting original bodyAt(j) at BFS distance d → afterBodyLen = j + d.
// So at distance d, target body index = (desired afterBodyLen) - d.
// Soft BFS: only avoids body cells with index < p (prefix side to keep).
// Body cells with index >= p (tail side to cut) and food cells are freely passable.
int findPrefixBrokenCutPaths(const SnakeState& st, int p, int topK, FixedPath* out) const {
    if (st.bodyLen == 0 || p >= st.colorLen || p < 3) return 0;
    topK = max(1, topK);

    // buildReleaseSparse must be called by caller
    // releaseAt threshold: body index i has release = bodyLen-1-i.
    // index < p  → release > bodyLen-1-p  (must avoid)
    // index >= p → release <= bodyLen-1-p  (can pass)
    int releaseThresh = st.bodyLen - 1 - p; // cells with releaseAt > this are prefix-side → block

    beginBfsVisit();

    uint8_t head = st.bodyFront();
    uint8_t neck = (st.bodyLen >= 2) ? st.bodyAt(1) : 255;

    // Check if npos is a valid bite target at BFS distance d.
    auto isGoal = [&](uint8_t npos, int d) -> bool {
        const int nearPAllow = NEAR_P_ALLOW_BY_N[N];
        for (int delta = 0; delta <= nearPAllow; ++delta) {
            int targetL = p - delta;
            if (targetL < CUT_KEEP_MIN_LEN) continue;
            int j = targetL - d;
            if (j < 2 || j >= st.bodyLen) continue;
            if (st.bodyAt(j) == npos) return true;
        }
        return false;
    };

    // Block check: only block prefix-side body (releaseAt > releaseThresh)
    auto isBlocked = [&](uint8_t npos) -> bool {
        int r = releaseAt(npos);
        return r > releaseThresh;
    };

    int maxDist = p - 2; // need j = p - d >= 2

    struct Goal { int16_t dist; uint8_t pos; };
    Goal goals[NCELLS];
    int gn = 0;
    int goalMaxDist = -1;

    uint8_t bq[NCELLS];
    int16_t bqd[NCELLS];
    int qt = 0;
    bfsSeenStamp[head] = bfsStamp;

    // Expand head neighbors (skip neck)
    for (int di = 0; di < 4; ++di) {
        uint16_t next = gNextPos[head][di];
        if (next == INVALID_NEXT_POS) continue;
        uint8_t npos = (uint8_t)next;
        if (npos == neck) continue;

        if (isGoal(npos, 1)) {
            bfsSeenStamp[npos] = bfsStamp;
            bfsParent[npos] = head;
            bfsPdir[npos] = (int8_t)di;
            goals[gn++] = {1, npos};
            goalMaxDist = 1;
            continue;
        }

        if (isBlocked(npos)) continue;
        if (bfsSeenStamp[npos] == bfsStamp) continue;
        bfsSeenStamp[npos] = bfsStamp;
        bfsParent[npos] = head;
        bfsPdir[npos] = (int8_t)di;
        bq[qt] = npos;
        bqd[qt] = 1;
        ++qt;
    }

    int qh = 0;
    while (qh < qt) {
        uint8_t pos = bq[qh];
        int t = bqd[qh];
        ++qh;
        if (t >= maxDist) break;
        if (gn >= topK && t > goalMaxDist) break;

        for (int di = 0; di < 4; ++di) {
            uint16_t next = gNextPos[pos][di];
            if (next == INVALID_NEXT_POS) continue;
            uint8_t npos = (uint8_t)next;
            int nt = t + 1;

            if (bfsSeenStamp[npos] == bfsStamp) continue;

            if (isGoal(npos, nt)) {
                bfsSeenStamp[npos] = bfsStamp;
                bfsParent[npos] = pos;
                bfsPdir[npos] = (int8_t)di;
                goals[gn++] = {(int16_t)nt, npos};
                goalMaxDist = nt;
                continue;
            }

            if (isBlocked(npos)) continue;
            bfsSeenStamp[npos] = bfsStamp;
            bfsParent[npos] = pos;
            bfsPdir[npos] = (int8_t)di;
            bq[qt] = npos;
            bqd[qt] = (int16_t)nt;
            ++qt;
        }
    }

    if (gn == 0) return 0;
    int take = min(topK, gn);
    int cnt = 0;
    for (int gi = 0; gi < take; ++gi) {
        FixedPath& path = out[cnt];
        reconstructPathBackward(path, goals[gi].dist, goals[gi].pos, bfsParent, bfsPdir);
        if (path.len > 0) ++cnt;
    }
    return cnt;
}

long long foodWallPenalty(const SnakeState& st) const {
    if (st.foodCount <= 0) return 0LL;
    return SCORE_FOOD_WALL_PENALTY_WEIGHT * (long long)st.wallFoodCount
         + SCORE_FOOD_CORNER_EXTRA_PENALTY_WEIGHT * (long long)st.cornerFoodCount;
}

inline long long evaluateBeamStateFromPrefix(const SnakeState& st, int pref) const {
    return -activePrefixPrimaryWeight * pref + foodWallPenalty(st);
}

int safeOptimisticLowerBound(const SnakeState& st, int p) const {
    int rem = M - p;
    if (rem <= 0) return 0;
    if (st.bodyLen <= 0) return rem;
    int tc = desired[p];
    if ((unsigned)tc >= CMAXP1) return rem;
    uint8_t head = st.bodyFront();
    int headIdx = gCellIdxByPos[head];
    int best = INT_MAX;
    for (int blk = 0; blk < BLOCKS; ++blk) {
        uint64_t bits = st.foodColorOcc[tc][blk];
        while (bits) {
            int b = __builtin_ctzll(bits);
            int idx = (blk << 6) + b;
            if (idx >= nnCells) break;
            int d = manhattanDist[headIdx][idx];
            if (d < best) best = d;
            bits &= bits - 1;
        }
    }
    if (best == INT_MAX) return rem;
    return best + (rem - 1);
}

uint64_t beamStateKey(const SnakeState& st) const {
    uint64_t h = 0x9e3779b97f4a7c15ULL;
    if constexpr (BLOCKS == 1) {
        h ^= st.foodOcc[0] * 0xbf58476d1ce4e5b9ULL;
    } else {
        h ^= st.foodOcc[0] * 0xbf58476d1ce4e5b9ULL;
        h ^= st.foodOcc[1] * 0x94d049bb133111ebULL;
        h ^= st.foodOcc[2] * 0xd6e8feb86659fd93ULL;
        h ^= st.foodOcc[3] * 0xa0761d6478bd642fULL;
    }
    h ^= (uint64_t)st.bodyFront() * 0x369dea0f31a53f85ULL;
    return h;
}

bool betterBeamNode(const BeamNode& a, const BeamNode& b) const {
    if (a.eval != b.eval) return a.eval < b.eval;
    if (a.maxPrefix != b.maxPrefix) return a.maxPrefix > b.maxPrefix;
    return a.st.turn < b.st.turn;
}

static bool betterKey(const BeamKey& a, const BeamKey& b) {
    if (a.eval != b.eval) return a.eval < b.eval;
    if (a.maxPrefix != b.maxPrefix) return a.maxPrefix > b.maxPrefix;
    return a.turn < b.turn;
}


inline void initChildNode(const BeamNode& parent, BeamNode& child) const {
    child.st = parent.st;
    child.pathTail = parent.pathTail;
    child.pathLen = parent.pathLen;
}

inline bool appendMove(BeamNode& node, int8_t dir) const {
    if (node.st.turn >= activeTurnLimit) return false;
    MoveResult ret = node.st.apply(dir);
    if (!ret.ok) return false;
    node.pathTail = gPool.add(dir, node.pathTail);
    node.pathLen++;
    return true;
}

inline bool appendMoveNoBite(BeamNode& node, int8_t dir) const {
    if (node.st.turn >= activeTurnLimit) return false;
    MoveResult ret = node.st.apply(dir);
    if (!ret.ok || ret.bite) return false;
    node.pathTail = gPool.add(dir, node.pathTail);
    node.pathLen++;
    return true;
}

inline bool appendPath(BeamNode& node, const FixedPath& path) const {
    for (int i = 0; i < path.len; ++i) {
        if (!appendMove(node, path.d[i])) return false;
    }
    return true;
}

inline bool appendPathNoBite(BeamNode& node, const FixedPath& path) const {
    for (int i = 0; i < path.len; ++i) {
        if (!appendMoveNoBite(node, path.d[i])) return false;
    }
    return true;
}

inline void finalizeBeamNodeAfterTransition(const BeamNode& cur, BeamNode& nxt, int prefixHint = 0) const {
    int np = nxt.st.prefixLen(desired, M, prefixHint);
    nxt.prefix = np;
    nxt.maxPrefix = max(cur.maxPrefix, np);
    nxt.stateKey = beamStateKey(nxt.st);
    nxt.eval = evaluateBeamStateFromPrefix(nxt.st, np);
}

inline bool pruneByIncumbent(BeamNode& node, int incumbentBestLen) const {
    if (incumbentBestLen >= INT_MAX) return false;
    if (node.optimisticLB < 0) {
        node.optimisticLB = safeOptimisticLowerBound(node.st, node.prefix);
    }
    return node.st.turn + node.optimisticLB >= incumbentBestLen;
}

static constexpr int SELECT_BUF_CAP = 16384;
mutable int selectBuf[SELECT_BUF_CAP];

int selectTopBeamIndices(const vector<BeamKey>& keys, int beamWidth, int* order) const {
    int n = min((int)keys.size(), SELECT_BUF_CAP);
    if (n == 0) return 0;
    int take = min(n, beamWidth);
    for (int i = 0; i < n; ++i) selectBuf[i] = i;
    auto better = [&](int a, int b) {
        if (keys[a].eval != keys[b].eval) return keys[a].eval < keys[b].eval;
        if (keys[a].maxPrefix != keys[b].maxPrefix) return keys[a].maxPrefix > keys[b].maxPrefix;
        return keys[a].turn < keys[b].turn;
    };
    if (take < n) {
        nth_element(selectBuf, selectBuf + take, selectBuf + n, better);
    }
    sort(selectBuf, selectBuf + take, better);
    memcpy(order, selectBuf, take * sizeof(int));
    return take;
}

// O(1) per direction: no SnakeState copy, no apply, just bitwise check
int pickWanderStepForState(const SnakeState& st) {
    if (st.bodyLen == 0) return -1;
    uint8_t head = st.bodyFront();
    uint8_t neck = (st.bodyLen >= 2) ? st.bodyAt(1) : 255;
    uint8_t tail = st.bodyBack();
    uint8_t tailPrev = (st.bodyLen >= 2) ? st.bodyAt(st.bodyLen - 2) : 255;
    int8_t candidates[4];
    int candN = 0;
    const int8_t* dirOrder = chooseDirOrder();
    for (int di = 0; di < 4; ++di) {
        int d = dirOrder[di];
        uint16_t next = gNextPos[head][d];
        if (next == INVALID_NEXT_POS) continue;
        uint8_t npos = (uint8_t)next;
        if (npos == neck) continue;
        // Skip if moving here would cause bite:
        // A bite happens when npos is occupied by body AND it's not just the tail
        // (the tail will leave if we don't eat, and no food means no eat)
        bool hasFood = st.hasFoodAtIdx(gBlkByPos[npos], gMaskByPos[npos]);
        if (!hasFood) {
            // Normal move (no eat):
            // - old tail is safe
            // - old tailPrev is also safe because it becomes the new tail
            // - but if old tail overlaps some earlier body, entering that tail cell is
            //   unsafe unless the overlap is exactly tailPrev==tail.
            if (st.bodyOccupiedPos(npos)) {
                bool safeTail = (npos == tail) && (st.overlapPos != npos || tailPrev == tail);
                bool safeTailPrev = (npos == tailPrev);
                if (!safeTail && !safeTailPrev) continue;
            }
        } else {
            // Would eat: tail stays. Bite if npos is in body at all
            if (st.bodyOccupiedPos(npos)) continue;
        }
        candidates[candN++] = (int8_t)d;
    }
    if (candN == 0) return -1;
    return candidates[randRange(0, candN - 1)];
}

template<typename InsertFn>
void transitionGoTargetCandidates(
    const BeamNode& cur, DeadlineChecker& timer, int topK, InsertFn& insertFn
) {
    if (timer.isOver()) return;
    int p = cur.prefix;
    if (p >= M) return;
    int tc = desired[p];
    bool prefBroken = (p < cur.st.colorLen);

    // Build release schedule once for all BFS calls on this node
    buildReleaseSparse(cur.st);

    FixedPath paths[8];
    int pathCount = 0;
    auto tryPush = [&](const FixedPath& path) {
        if (path.empty()) return;
        for (int i = 0; i < pathCount; ++i) if (paths[i] == path) return;
        if (pathCount < 8) paths[pathCount++] = path;
    };

    if (!prefBroken) {
        // Quick adjacent check first — O(4) instead of O(N²) BFS
        FixedPath adjPaths[4];
        int adjn = findAdjacentTargetPaths(cur.st, tc, min(topK, 4), adjPaths);
        for (int i = 0; i < adjn; ++i) {
            tryPush(adjPaths[i]);
            if (pathCount >= topK) break;
        }
        // Only run full BFS if we need more paths
        if (pathCount < topK) {
            FixedPath fp[8];
            int fpn = findDynamicStrictPathsToTarget(cur.st, tc, min(topK, 8), fp);
            for (int i = 0; i < fpn; ++i) {
                tryPush(fp[i]);
                if (pathCount >= topK) break;
            }
        }
    }
    if (prefBroken || pathCount == 0) {
        tryPush(findPreferFoodAvoidPathToTarget(cur.st, tc));
    }
    if (pathCount == 0) return;
    if (pathCount > topK) pathCount = topK;

    for (int pi = 0; pi < pathCount; ++pi) {
        if (timer.isOver()) break;
        const FixedPath& path = paths[pi];
        if (!gPool.hasSpace(path.len)) continue;
        BeamNode nxt;
        initChildNode(cur, nxt);
        if (!appendPathNoBite(nxt, path)) continue;
        finalizeBeamNodeAfterTransition(cur, nxt, cur.prefix);
        insertFn(move(nxt));
    }
}

template<typename InsertFn>
void transitionCutRecoverCandidates(
    const BeamNode& cur, DeadlineChecker& timer, int topK, InsertFn& insertFn
) {
    if (timer.isOver()) return;
    int p = cur.prefix;
    if (p >= M) return;
    if (p >= cur.st.colorLen) return;
    auto cuts = chooseTopCutCandidatesForBeam(cur.st, p, desired[p], topK);
    if (cuts.empty()) return;

    for (int ci = 0; ci < cuts.size; ++ci) {
        const CutCandidate& cut = cuts.data[ci];
        if (timer.isOver()) break;
        if (!gPool.hasSpace(cut.seqLen + NCELLS)) continue;
        BeamNode nxt;
        initChildNode(cur, nxt);
        bool bitten = false, ok = true;
        for (int si = 0; si < cut.seqLen; ++si) {
            if (si + 1 == cut.seqLen) {
                uint8_t savedBodyBuf[SCAP];
                int16_t savedBodyHead = nxt.st.bodyHead;
                int16_t savedBodyLen = nxt.st.bodyLen;
                {
                    int h = savedBodyHead, len = savedBodyLen;
                    if (h + len <= SCAP) {
                        memcpy(savedBodyBuf + h, nxt.st.bodyBuf + h, len);
                    } else {
                        int first = SCAP - h;
                        memcpy(savedBodyBuf + h, nxt.st.bodyBuf + h, first);
                        memcpy(savedBodyBuf, nxt.st.bodyBuf, len - first);
                    }
                }
                int pb = nxt.st.prefixLen(desired, M, cur.prefix);
                MoveResult ret = nxt.st.apply(cut.seq[si]);
                if (!ret.ok || !ret.bite) { ok = false; break; }
                nxt.pathTail = gPool.add(cut.seq[si], nxt.pathTail);
                nxt.pathLen++;
                bitten = true;
                int hintAfterBite = min((int)pb, (int)nxt.st.colorLen);
                FixedPath recovery = buildRecoveryPathByOldBody(
                    savedBodyBuf, savedBodyHead, savedBodyLen, nxt.st, pb
                );
                if (!appendPath(nxt, recovery)) ok = false;
                if (ok) {
                    finalizeBeamNodeAfterTransition(cur, nxt, hintAfterBite);
                    insertFn(move(nxt));
                }
                break;
            }
            MoveResult ret = nxt.st.apply(cut.seq[si]);
            if (!ret.ok || ret.bite) { ok = false; break; }
            nxt.pathTail = gPool.add(cut.seq[si], nxt.pathTail);
            nxt.pathLen++;
        }
        // finalize/insert already done inside bite branch
    }
}

// Prefix-broken specialized cut: BFS to body segment near index p, bite there.
// afterBodyLen in [p - NEAR_P_ALLOW_BY_N[N], p], recovery if afterBodyLen < p.
template<typename InsertFn>
void transitionPrefixBrokenCutRecover(
    const BeamNode& cur, DeadlineChecker& timer, int topK, InsertFn& insertFn
) {
    if (timer.isOver()) return;
    int p = cur.prefix;
    if (p >= M) return;
    if (p >= cur.st.colorLen) return;

    buildReleaseSparse(cur.st);
    FixedPath paths[8];
    int pathCount = findPrefixBrokenCutPaths(cur.st, p, min(topK, 8), paths);
    if (pathCount == 0) return;

    for (int pi = 0; pi < pathCount; ++pi) {
        if (timer.isOver()) break;
        const FixedPath& path = paths[pi];
        if (!gPool.hasSpace(path.len + NCELLS)) continue;

        BeamNode nxt;
        initChildNode(cur, nxt);
        bool ok = true;
        for (int si = 0; si < path.len; ++si) {
            // Save body state before every step (needed if this step causes a bite)
            uint8_t savedBodyBuf[SCAP];
            int16_t savedBodyHead = nxt.st.bodyHead;
            int16_t savedBodyLen = nxt.st.bodyLen;
            {
                int h = savedBodyHead, len = savedBodyLen;
                if (h + len <= SCAP) {
                    memcpy(savedBodyBuf + h, nxt.st.bodyBuf + h, len);
                } else {
                    int first = SCAP - h;
                    memcpy(savedBodyBuf + h, nxt.st.bodyBuf + h, first);
                    memcpy(savedBodyBuf, nxt.st.bodyBuf, len - first);
                }
            }
            int pb = nxt.st.prefixLen(desired, M, cur.prefix);
            MoveResult ret = nxt.st.apply(path.d[si]);
            if (!ret.ok) { ok = false; break; }
            nxt.pathTail = gPool.add(path.d[si], nxt.pathTail);
            nxt.pathLen++;

            if (ret.bite) {
                // Bite happened (intended final or mid-path) — do recovery and emit
                int hintAfterBite = min((int)pb, (int)nxt.st.colorLen);
                FixedPath recovery = buildRecoveryPathByOldBody(
                    savedBodyBuf, savedBodyHead, savedBodyLen, nxt.st, pb
                );
                if (!appendPath(nxt, recovery)) { ok = false; break; }
                finalizeBeamNodeAfterTransition(cur, nxt, hintAfterBite);
                insertFn(move(nxt));
                ok = false; // signal: already handled, don't emit again
                break;
            }
            // No bite — continue to next step (eat is fine, just keep going)
        }
    }
}

template<typename InsertFn>
void transitionWander(const BeamNode& cur, DeadlineChecker& timer, int topK, InsertFn& insertFn) {
    if (timer.isOver()) return;
    int p = cur.prefix;
    if (p >= M) return;
    if (topK <= 0) return;
    const int wanderLen = WANDER_LEN_BY_N[N];
    if (!gPool.hasSpace(wanderLen)) return;
    BeamNode out;
    initChildNode(cur, out);
    int moved = 0;
    int knownPrefix = cur.prefix;
    for (int t = 0; t < wanderLen; ++t) {
        int d = pickWanderStepForState(out.st);
        if (d == -1) break;
        int16_t prevColorLen = out.st.colorLen;
        if (!appendMove(out, (int8_t)d)) break;
        ++moved;
        if (out.st.colorLen > prevColorLen) {
            knownPrefix = out.st.prefixLen(desired, M, knownPrefix);
            if (knownPrefix >= M) break;
        }
    }
    if (moved == 0) return;
    finalizeBeamNodeAfterTransition(cur, out, knownPrefix);
    insertFn(move(out));
}

bool isDoneNode(const BeamNode& node) const { return node.prefix >= M; }
bool betterFinalNode(const BeamNode& a, const BeamNode& b) const {
    bool ad = isDoneNode(a), bd = isDoneNode(b);
    if (ad != bd) return ad;
    if (ad && bd) return a.st.turn < b.st.turn;
    return betterBeamNode(a, b);
}

bool runBeamSearch(
    const SnakeState& initial, chrono::steady_clock::time_point deadline,
    int beamWidth, int beamTurnCap, int incumbentBestLen, BeamNode& outBest
) {
    BeamNode init;
    init.st = initial;
    init.prefix = init.st.prefixLen(desired, M);
    init.maxPrefix = init.prefix;
    init.optimisticLB = safeOptimisticLowerBound(init.st, init.prefix);
    init.stateKey = beamStateKey(init.st);
    init.eval = evaluateBeamStateFromPrefix(init.st, init.prefix);
    init.pathTail = -1;
    init.pathLen = 0;
    if (init.st.turn > beamTurnCap) return false;

    int startTurn = init.st.turn;
    ++currentRunStamp;
    if (currentRunStamp == INT_MAX) {
        memset(layerRunStamp, 0, sizeof(layerRunStamp));
        currentRunStamp = 1;
    }
    const int runStamp = currentRunStamp;
    auto& layers = layersBuf;
    DeadlineChecker timer{deadline};
    const int goTargetCandidateTopK = BEAM_CANDIDATE_TOP_K_BY_N[N];
    const int cutApplyNodeLimit = CUT_APPLY_NODE_LIMIT_BY_N[N];
    const int pcutApplyNodeLimit = PCUT_APPLY_NODE_LIMIT_BY_N[N];
    const int pcutCandidateTopK = PCUT_CANDIDATE_TOP_K_BY_N[N];
    const int wanderApplyNodeLimit = WANDER_APPLY_NODE_LIMIT_BY_N[N];
    const int cutCandidateTopK = BEAM_CUT_CANDIDATE_TOP_K_BY_N[N];
    const int wanderCandidateTopK = WANDER_CANDIDATE_TOP_K_BY_N[N];
    auto ensureLayer = [&](int t) {
        if (layerRunStamp[t] == runStamp) return;
        layerRunStamp[t] = runStamp;
        auto& vec = layers[t];
        auto& kvec = layerKeysBuf[t];
        vec.clear();
        kvec.clear();
        int expected = beamWidth * (goTargetCandidateTopK + cutCandidateTopK + 1 + wanderCandidateTopK) * 2;
        expected = max(expected, beamWidth * 4);
        if ((int)vec.capacity() < expected) { vec.reserve(expected); kvec.reserve(expected); }
    };

    auto ins = [&](BeamNode&& cand) {
        int t = cand.st.turn;
        if (t < startTurn || t > beamTurnCap) return;
        if (pruneByIncumbent(cand, incumbentBestLen)) return;
        ensureLayer(t);
        auto& vec = layers[t];
        auto& kvec = layerKeysBuf[t];
        int dupIdx = -1;
        {
            const int sz = (int)kvec.size();
            const int scanFrom = (sz > 32) ? sz - 32 : 0;
            for (int i = sz - 1; i >= scanFrom; --i) {
                if (kvec[i].stateKey == cand.stateKey) { dupIdx = i; break; }
            }
        }
        if (dupIdx < 0) {
            if (__builtin_expect((int)vec.size() >= (int)vec.capacity(), 0)) return;
            kvec.push_back({cand.eval, cand.maxPrefix, cand.st.turn, cand.stateKey});
            vec.push_back(move(cand));
        } else {
            BeamKey candKey = {cand.eval, cand.maxPrefix, cand.st.turn, cand.stateKey};
            if (betterKey(candKey, kvec[dupIdx])) {
                kvec[dupIdx] = candKey;
                vec[dupIdx] = move(cand);
            }
        }
    };

    ensureLayer(startTurn);
    layers[startTurn].push_back(init);
    layerKeysBuf[startTurn].push_back({init.eval, init.maxPrefix, init.st.turn, init.stateKey});

    bool hasDone = false; BeamNode bestDone, bestSeen = init;
    BeamKey bestSeenKey = {init.eval, init.maxPrefix, init.st.turn, init.stateKey};
    BeamKey bestDoneKey;

    for (int turn = startTurn; turn <= beamTurnCap; ++turn) {
        if (timer.isOver()) break;
        if (layerRunStamp[turn] != runStamp) continue;
        auto& beam = layers[turn];
        if (beam.empty()) continue;

        int order[BEAM_WIDTH_HARD_MAX];
        int ordN = selectTopBeamIndices(layerKeysBuf[turn], beamWidth, order);
        auto& kbeam = layerKeysBuf[turn];

        for (int oi = 0; oi < ordN; ++oi) {
            int bi = order[oi];
            if (timer.isOver()) break;
            auto& node = beam[bi];
            auto& nkey = kbeam[bi];
            if (betterKey(nkey, bestSeenKey)) { bestSeenKey = nkey; bestSeen = node; }
            int p = node.prefix;
            if (p >= M) {
                if (!hasDone || betterKey(nkey, bestDoneKey)) { hasDone = true; bestDoneKey = nkey; bestDone = node; }
                continue;
            }
            if (node.st.turn >= beamTurnCap) continue;
            if (pruneByIncumbent(node, incumbentBestLen)) continue;

            transitionGoTargetCandidates(node, timer, goTargetCandidateTopK, ins);
            if (oi < cutApplyNodeLimit)
                transitionCutRecoverCandidates(node, timer, cutCandidateTopK, ins);
            if (oi < pcutApplyNodeLimit)
                transitionPrefixBrokenCutRecover(node, timer, pcutCandidateTopK, ins);
            if (oi < wanderApplyNodeLimit) transitionWander(node, timer, wanderCandidateTopK, ins);
        }
        if (hasDone) break;
        beam.clear();
        layerKeysBuf[turn].clear();
    }
    outBest = hasDone ? bestDone : bestSeen;
    return true;
}

void solve() {
    auto totalDL = chrono::steady_clock::now() + chrono::milliseconds(SEARCH_TIME_LIMIT_MS);
    DeadlineChecker totalTimer{totalDL};
    SnakeState initial = state;
    activeTurnLimit = MAX_TURN_LIMIT;

    BeamNode best;
    best.st = initial;
    best.pathTail = -1;
    best.pathLen = 0;
    best.prefix = initial.prefixLen(desired, M);
    best.maxPrefix = best.prefix;
    best.eval = evaluateBeamStateFromPrefix(best.st, best.prefix);
    MoveSeq bestPath;
    int btc = BEAM_TURN_CAP_MAX;
    const int beamWidthMax = BEAM_WIDTH_MAX_BY_N[N];
    int bw = BEAM_WIDTH;
    int itr = 0;
    while (true) {
        if (totalTimer.isOver()) break;
        gPool.clear();
        int incumbentBestLen = isDoneNode(best) ? bestPath.size() : INT_MAX;
        activePrefixPrimaryWeight =
            (incumbentBestLen > SCORE_PREFIX_PRIMARY_WEIGHT_LONG_BEST_THRESHOLD)
                ? SCORE_PREFIX_PRIMARY_WEIGHT_LONG_BEST
                : SCORE_PREFIX_PRIMARY_WEIGHT;
        BeamNode candidate;
        bool ran = runBeamSearch(initial, totalDL, bw, btc, incumbentBestLen, candidate);
        if (ran && betterFinalNode(candidate, best)) {
            best = move(candidate);
            bestPath.len = best.pathLen;
            if (best.pathTail >= 0 && best.pathLen > 0) {
                gPool.extract(best.pathTail, best.pathLen, bestPath.d);
            }
            if (isDoneNode(best) && best.pathLen > 0) {
                btc = min(btc, best.pathLen);
            }
            // cerr << "best_update"
            //          << " bw=" << bw
            //          << " btc=" << btc
            //          << " turn=" << best.st.turn
            //          << " len=" << best.pathLen
            //          << " prefix=" << best.prefix << "/" << M
            //          << " maxPrefix=" << best.maxPrefix
            //          << " eval=" << best.eval
            //          << " done=" << (isDoneNode(best) ? 1 : 0)
            //          << '\n';
        }
        // cerr << "itr=" << itr << " bw=" << bw << " Finish\n";
        if (totalTimer.isOver()) break;
        bw = min(beamWidthMax, bw * 2);
        itr++;
    }

    movesDirs = bestPath;
}

void printAnswer() const {
    static char buf[MAX_TURN_LIMIT * 2];
    int pos = 0;
    for (int i = 0; i < movesDirs.len; ++i) {
        buf[pos++] = DCHAR[(int)movesDirs.d[i]];
        buf[pos++] = '\n';
    }
    fwrite(buf, 1, pos, stdout);
}

}; // SolverT

}  // namespace

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int N, M, C;
    cin >> N >> M >> C;
    auto run = [&](auto& solver) {
        solver.readInput(N, M, C);
        solver.solve();
        solver.printAnswer();
    };
    if (N <= 8) {
        if (C <= 4) { SolverT<3, 8, 64, 63, 1, 5> s; run(s); }
        else { SolverT<3, 8, 64, 63, 1, 8> s; run(s); }
    } else if (N <= 11) {
        if (C <= 4) { SolverT<4, 16, 128, 127, 4, 5> s; run(s); }
        else { SolverT<4, 16, 128, 127, 4, 8> s; run(s); }
    } else {
        if (C <= 4) { SolverT<4, 16, 256, 255, 4, 5> s; run(s); }
        else { SolverT<4, 16, 256, 255, 4, 8> s; run(s); }
    }
    return 0;
}
