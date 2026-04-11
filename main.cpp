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
constexpr int SEARCH_TIME_LIMIT_MS = 1850;

constexpr int CUT_ENUM_MAX_DEPTH_HARD_MAX = 5;
constexpr int CUT_CANDIDATE_CAP = 64;
constexpr int CUT_KEEP_MIN_LEN = 5;
constexpr int INITIAL_SNAKE_LEN = 5;

constexpr long long SCORE_PREFIX_PRIMARY_WEIGHT = 25LL;
constexpr long long SCORE_PREFIX_PRIMARY_WEIGHT_LONG_BEST = 1'000'000LL;
constexpr int SCORE_PREFIX_PRIMARY_WEIGHT_LONG_BEST_THRESHOLD = 500;
constexpr long long SCORE_FOOD_WALL_PENALTY_WEIGHT = 10LL;
constexpr long long SCORE_FOOD_CORNER_EXTRA_PENALTY_WEIGHT = 20LL;

constexpr int BEAM_WIDTH = 5;
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
    5, 5, 4, 4, 4, 3, 2, 2, 2,
};
constexpr array<int, 17> WANDER_CANDIDATE_TOP_K_BY_N = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
};

constexpr uint16_t INVALID_NEXT_POS = 0xFFFFu;
constexpr uint8_t NO_OVERLAP_POS = 0xFFu;

// Global lookup tables — stay at max size 256, initialized per-N.
uint16_t gCellIdxByPos[256];
uint16_t gNextPos[256][4];
uint8_t gWallByPos[256];
uint8_t gCornerByPos[256];
uint32_t gDirOrderNonce = 0;

inline const int8_t* chooseDirOrder() {
    uint32_t x = ++gDirOrderNonce;
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    uint32_t idx = (uint32_t)(((uint64_t)x * DIR_ORDER_COUNT) >> 32);
    return DIR_ORDERS[idx];
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
        if ((++tick & 1u) != 0) return false;
        return chrono::steady_clock::now() >= deadline;
    }
};

// ============================================================
// Template: SHIFT=3,SIDE=8,CAP=64,MASK=63,BLOCKS=1 for N<=8
//           SHIFT=4,SIDE=16,CAP=256,MASK=255,BLOCKS=4 for N>8
// ============================================================
template<int SHIFT, int SIDE, int SCAP, int SMASK, int BLOCKS>
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
    int8_t grid[SCAP] = {};
    uint64_t bodyOcc[BLOCKS] = {};
    uint8_t bodyBuf[SCAP];
    uint8_t colorBuf[(SCAP + 1) >> 1] = {};
    int16_t bodyHead = 0, bodyLen = 0;
    int16_t colorHead = 0, colorLen = 0;
    uint8_t overlapPos = NO_OVERLAP_POS;
    int turn = 0;
    int16_t foodCount = 0;
    int16_t wallFoodCount = 0;
    int16_t cornerFoodCount = 0;
    uint64_t foodColorOcc[MAXC][BLOCKS] = {};

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

    void foodColorOccSetPos(int color, uint8_t pos) {
        uint16_t idx = gCellIdxByPos[pos];
        foodColorOcc[color][idx >> 6] |= 1ULL << (idx & 63);
    }
    void foodColorOccClearPos(int color, uint8_t pos) {
        uint16_t idx = gCellIdxByPos[pos];
        foodColorOcc[color][idx >> 6] &= ~(1ULL << (idx & 63));
    }
    void onFoodAddedPos(uint8_t pos, int color) {
        ++foodCount;
        foodColorOccSetPos(color, pos);
        wallFoodCount += gWallByPos[pos];
        cornerFoodCount += gCornerByPos[pos];
    }
    void onFoodRemovedPos(uint8_t pos, int color) {
        --foodCount;
        foodColorOccClearPos(color, pos);
        wallFoodCount -= gWallByPos[pos];
        cornerFoodCount -= gCornerByPos[pos];
    }
    bool bodyOccupiedPos(uint8_t pos) const {
        uint16_t idx = gCellIdxByPos[pos];
        return (bodyOcc[idx >> 6] & (1ULL << (idx & 63))) != 0;
    }
    void bodyOccSetPos(uint8_t pos) {
        uint16_t idx = gCellIdxByPos[pos];
        bodyOcc[idx >> 6] |= 1ULL << (idx & 63);
    }
    void bodyOccRemoveOnePos(uint8_t pos) {
        if (overlapPos == pos) {
            overlapPos = NO_OVERLAP_POS;
            return;
        }
        uint16_t idx = gCellIdxByPos[pos];
        bodyOcc[idx >> 6] &= ~(1ULL << (idx & 63));
    }

    int prefixLen(const int8_t* desired, int desiredLen) const {
        int lim = min((int)colorLen, desiredLen);
        if (lim == 0) return 0;
        int i = 0;
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

        bool eats = grid[npos] != 0;
        uint8_t tail = bodyBack();
        bool biteCandidate = bodyOccupiedPos(npos) && (eats || npos != tail || overlapPos == npos);

        if (__builtin_expect(!eats && !biteCandidate, 1)) {
            bodyPushFront(npos);
            bodyOccSetPos(npos);
            bodyPopBack();
            if (tail != npos) bodyOccRemoveOnePos(tail);
            overlapPos = (bodyBack() == npos) ? npos : NO_OVERLAP_POS;
            ++turn;
            ret.ok = true;
            return ret;
        }

        bodyPushFront(npos);
        bodyOccSetPos(npos);

        if (eats) {
            int eatenColor = grid[npos];
            grid[npos] = 0;
            colorPushBack((int8_t)eatenColor);
            onFoodRemovedPos(npos, eatenColor);
        } else {
            bodyPopBack();
            if (tail != npos) bodyOccRemoveOnePos(tail);
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
                        uint16_t bpIdx = gCellIdxByPos[bp];
                        int bpBlk = bpIdx >> 6;
                        uint64_t bpMask = 1ULL << (bpIdx & 63);
                        if (overlapPos == bp) {
                            overlapPos = NO_OVERLAP_POS;
                        } else {
                            bodyOcc[bpBlk] &= ~bpMask;
                        }
                        int color = colorAtRaw(restoreColorIdx);
                        grid[bp] = (int8_t)color;
                        ++restoredFood;
                        foodColorOcc[color][bpBlk] |= bpMask;
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
    long long eval = (long long)4e18;
};

array<vector<BeamNode>, BEAM_TURN_CAP_MAX + 1> layersBuf;
array<vector<uint64_t>, BEAM_TURN_CAP_MAX + 1> layerKeysBuf;
int layerRunStamp[BEAM_TURN_CAP_MAX + 1] = {};
int currentRunStamp = 1;

mutable uint16_t bfsSeenStamp[SCAP] = {};
mutable uint16_t bfsStamp = 1;
mutable uint16_t releaseStamp[SCAP] = {};
mutable uint16_t releaseCur = 1;
mutable int16_t releaseValue[SCAP] = {};
mutable uint8_t bfsParent[SCAP] = {};
mutable int8_t bfsPdir[SCAP] = {};

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
    for (int r = 0; r < SIDE; ++r) {
        for (int cc = 0; cc < SIDE; ++cc) {
            uint8_t pos = packPos(r, cc);
            if (r < N && cc < N) {
                int idx = r * N + cc;
                gCellIdxByPos[pos] = (uint16_t)idx;
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

    memset(state.grid, 0, sizeof(state.grid));
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
            state.grid[pos] = (int8_t)x;
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
    if ((unsigned)color >= MAXC) return false;
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
        releaseStamp[bp] = releaseCur;
        releaseValue[bp] = (int16_t)(st.bodyLen - i);
    }
}

inline int releaseAt(uint8_t pos) const {
    return releaseStamp[pos] == releaseCur ? releaseValue[pos] : 0;
}

int findDynamicStrictPathsToTarget(
    const SnakeState& st, int targetColor, int topK, FixedPath* out
) const {
    if (st.bodyLen == 0) return 0;
    if (!hasFoodColor(st, targetColor)) return 0;
    topK = max(1, topK);
    buildReleaseSparse(st);
    beginBfsVisit();

    uint8_t head = st.bodyFront();
    uint8_t neck = 255;
    if (st.bodyLen >= 2) neck = st.bodyAt(1);

    uint8_t bq[NCELLS];
    int16_t bqd[NCELLS];
    int qh = 0, qt = 0;
    bfsSeenStamp[head] = bfsStamp;
    bq[qt] = head;
    bqd[qt] = 0;
    ++qt;

    struct Goal { int16_t dist; uint8_t pos; };
    Goal goals[NCELLS];
    int gn = 0;
    int goalMaxDist = -1;
    const int8_t* dirOrder = chooseDirOrder();

    while (qh < qt) {
        uint8_t pos = bq[qh];
        int t = bqd[qh];
        ++qh;
        if (gn >= topK && t > goalMaxDist) break;
        if (pos != head && st.grid[pos] == targetColor) {
            goals[gn++] = {(int16_t)t, pos};
            goalMaxDist = t;
            continue;
        }
        for (int di = 0; di < 4; ++di) {
            int d = dirOrder[di];
            uint16_t next = gNextPos[pos][d];
            if (next == INVALID_NEXT_POS) continue;
            uint8_t npos = (uint8_t)next;
            if (pos == head && npos == neck) continue;
            int food = st.grid[npos];
            if (food != 0 && food != targetColor) continue;
            int nt = t + 1;
            if (nt < releaseAt(npos)) continue;
            if (bfsSeenStamp[npos] == bfsStamp) continue;
            bfsSeenStamp[npos] = bfsStamp;
            bfsParent[npos] = pos;
            bfsPdir[npos] = (int8_t)d;
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

// Permissive shortest BFS. When strict route is unavailable, prioritize speed
// and path length instead of minimizing the number of non-target foods crossed.
FixedPath findPreferFoodAvoidPathToTarget(const SnakeState& st, int targetColor) const {
    FixedPath result;
    if (st.bodyLen == 0) return result;
    if (!hasFoodColor(st, targetColor)) return result;
    buildReleaseSparse(st);
    beginBfsVisit();

    uint8_t head = st.bodyFront();
    uint8_t neck = 255;
    if (st.bodyLen >= 2) neck = st.bodyAt(1);

    uint8_t q[NCELLS];
    int16_t qd[NCELLS];
    int qh = 0, qt = 0;
    bfsSeenStamp[head] = bfsStamp;
    q[qt] = head;
    qd[qt] = 0;
    ++qt;
    const int8_t* dirOrder = chooseDirOrder();

    uint8_t goalPos = 0;
    bool goalFound = false;
    int goalDist = 0;
    while (qh < qt) {
        uint8_t pos = q[qh];
        int t = qd[qh];
        ++qh;
        if (pos != head && st.grid[pos] == targetColor) {
            goalPos = pos;
            goalFound = true;
            goalDist = t;
            break;
        }
        for (int di = 0; di < 4; ++di) {
            int d = dirOrder[di];
            uint16_t next = gNextPos[pos][d];
            if (next == INVALID_NEXT_POS) continue;
            uint8_t npos = (uint8_t)next;
            if (pos == head && npos == neck) continue;
            int nt = t + 1;
            if (nt < releaseAt(npos)) continue;
            if (bfsSeenStamp[npos] == bfsStamp) continue;
            bfsSeenStamp[npos] = bfsStamp;
            bfsParent[npos] = pos;
            bfsPdir[npos] = (int8_t)d;
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
        if (st.grid[np] == targetColor) return true;
    }
    return false;
}

void enumerateCutCandidatesDFS(
    const SnakeState& cur, int depth, int maxDepth,
    int baseL, int targetColor,
    int8_t seq[CUT_ENUM_MAX_DEPTH_HARD_MAX], int seqLen,
    CutCandidateList& out
) const {
    if (depth >= maxDepth) return;
    DirList dirs = cur.legalDirs();
    for (int di = 0; di < dirs.n; ++di) {
        int d = dirs.d[di];
        SnakeState nxt = cur;
        MoveResult ret = nxt.apply(d);
        if (!ret.ok) continue;
        seq[seqLen] = (int8_t)d;
        int newLen = seqLen + 1;
        if (ret.bite) {
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
            enumerateCutCandidatesDFS(nxt, depth + 1, maxDepth, baseL, targetColor, seq, newLen, out);
        }
    }
}

CutCandidateList enumerateCutCandidates(
    const SnakeState& src, int baseL, int targetColor, int maxDepth
) const {
    CutCandidateList out;
    int8_t seq[CUT_ENUM_MAX_DEPTH_HARD_MAX];
    enumerateCutCandidatesDFS(src, 0, maxDepth, baseL, targetColor, seq, 0, out);
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
    if ((unsigned)tc >= MAXC) return rem;
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
        uint64_t food0 = 0;
        for (int color = 1; color <= C; ++color) food0 |= st.foodColorOcc[color][0];
        h ^= food0 * 0xbf58476d1ce4e5b9ULL;
        h ^= st.bodyOcc[0] * 0xe7037ed1a0b428dbULL;
    } else {
        uint64_t food0 = 0, food1 = 0, food2 = 0, food3 = 0;
        for (int color = 1; color <= C; ++color) {
            food0 |= st.foodColorOcc[color][0];
            food1 |= st.foodColorOcc[color][1];
            food2 |= st.foodColorOcc[color][2];
            food3 |= st.foodColorOcc[color][3];
        }
        h ^= food0 * 0xbf58476d1ce4e5b9ULL;
        h ^= food1 * 0x94d049bb133111ebULL;
        h ^= food2 * 0xd6e8feb86659fd93ULL;
        h ^= food3 * 0xa0761d6478bd642fULL;
        h ^= st.bodyOcc[0] * 0xe7037ed1a0b428dbULL;
        h ^= st.bodyOcc[1] * 0x8ebc6af09c88c6e3ULL;
        h ^= st.bodyOcc[2] * 0x589965cc75374cc3ULL;
        h ^= st.bodyOcc[3] * 0x1d8e4e27c47d124fULL;
    }
    h ^= ((uint64_t)st.bodyFront() | ((uint64_t)(uint16_t)st.bodyLen << 8)) * 0x369dea0f31a53f85ULL;
    return h;
}

bool betterBeamNode(const BeamNode& a, const BeamNode& b) const {
    if (a.eval != b.eval) return a.eval < b.eval;
    if (a.maxPrefix != b.maxPrefix) return a.maxPrefix > b.maxPrefix;
    return a.st.turn < b.st.turn;
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

inline bool appendPath(BeamNode& node, const FixedPath& path) const {
    for (int i = 0; i < path.len; ++i) {
        if (!appendMove(node, path.d[i])) return false;
    }
    return true;
}

inline void finalizeBeamNodeAfterTransition(const BeamNode& cur, BeamNode& nxt) const {
    int np = nxt.st.prefixLen(desired, M);
    nxt.prefix = np;
    nxt.maxPrefix = max(cur.maxPrefix, np);
    nxt.eval = evaluateBeamStateFromPrefix(nxt.st, np);
}

inline bool pruneByIncumbent(BeamNode& node, int incumbentBestLen) const {
    if (incumbentBestLen >= INT_MAX) return false;
    if (node.optimisticLB < 0) {
        node.optimisticLB = safeOptimisticLowerBound(node.st, node.prefix);
    }
    return node.st.turn + node.optimisticLB >= incumbentBestLen;
}

int selectTopBeamIndices(const vector<BeamNode>& beam, int beamWidth, int* order) const {
    int n = (int)beam.size();
    struct SK { long long eval; int maxPrefix; int turn; int idx; };
    static SK sk[16384];
    int skN = min(n, 16384);
    for (int i = 0; i < skN; ++i) {
        sk[i] = {beam[i].eval, beam[i].maxPrefix, beam[i].st.turn, i};
    }
    auto better = [](const SK& a, const SK& b) {
        if (a.eval != b.eval) return a.eval < b.eval;
        if (a.maxPrefix != b.maxPrefix) return a.maxPrefix > b.maxPrefix;
        return a.turn < b.turn;
    };
    int ordN = 0;
    for (int i = 0; i < skN; ++i) {
        if (ordN < beamWidth) {
            int pos = ordN++;
            while (pos > 0 && better(sk[i], sk[order[pos - 1]])) {
                order[pos] = order[pos - 1];
                --pos;
            }
            order[pos] = i;
        } else if (better(sk[i], sk[order[ordN - 1]])) {
            int pos = ordN - 1;
            while (pos > 0 && better(sk[i], sk[order[pos - 1]])) {
                order[pos] = order[pos - 1];
                --pos;
            }
            order[pos] = i;
        }
    }
    return ordN;
}

// O(1) per direction: no SnakeState copy, no apply, just bitwise check
int pickWanderStepForState(const SnakeState& st) {
    if (st.bodyLen == 0) return -1;
    uint8_t head = st.bodyFront();
    uint8_t neck = (st.bodyLen >= 2) ? st.bodyAt(1) : 255;
    uint8_t tail = st.bodyBack();
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
        // (the tail will leave if we don't eat, and grid[npos]==0 means no eat)
        if (st.grid[npos] == 0) {
            // Normal move (no eat): tail leaves. Bite if npos is in body and npos != tail
            // (or if there's an overlap at tail)
            if (st.bodyOccupiedPos(npos) && (npos != tail || st.overlapPos == npos)) continue;
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

    FixedPath paths[8];
    int pathCount = 0;
    auto tryPush = [&](const FixedPath& path) {
        if (path.empty()) return;
        for (int i = 0; i < pathCount; ++i) if (paths[i] == path) return;
        if (pathCount < 8) paths[pathCount++] = path;
    };
    if (!prefBroken) {
        FixedPath fp[8];
        int fpn = findDynamicStrictPathsToTarget(cur.st, tc, min(topK, 8), fp);
        for (int i = 0; i < fpn; ++i) {
            tryPush(fp[i]);
            if (pathCount >= topK) break;
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
        if (!appendPath(nxt, path)) continue;
        finalizeBeamNodeAfterTransition(cur, nxt);
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
                memcpy(savedBodyBuf, nxt.st.bodyBuf, sizeof(savedBodyBuf));
                int16_t savedBodyHead = nxt.st.bodyHead;
                int16_t savedBodyLen = nxt.st.bodyLen;
                int pb = nxt.st.prefixLen(desired, M);
                MoveResult ret = nxt.st.apply(cut.seq[si]);
                if (!ret.ok || !ret.bite) { ok = false; break; }
                nxt.pathTail = gPool.add(cut.seq[si], nxt.pathTail);
                nxt.pathLen++;
                bitten = true;
                FixedPath recovery = buildRecoveryPathByOldBody(
                    savedBodyBuf, savedBodyHead, savedBodyLen, nxt.st, pb
                );
                if (!appendPath(nxt, recovery)) ok = false;
                break;
            }
            MoveResult ret = nxt.st.apply(cut.seq[si]);
            if (!ret.ok || ret.bite) { ok = false; break; }
            nxt.pathTail = gPool.add(cut.seq[si], nxt.pathTail);
            nxt.pathLen++;
        }
        if (!ok || !bitten) continue;
        finalizeBeamNodeAfterTransition(cur, nxt);
        insertFn(move(nxt));
    }
}

template<typename InsertFn>
void transitionWander(const BeamNode& cur, DeadlineChecker& timer, int topK, InsertFn& insertFn) {
    if (timer.isOver()) return;
    int p = cur.prefix;
    if (p >= M) return;
    const int wanderLen = WANDER_LEN_BY_N[N];
    FixedPath seen[WANDER_CANDIDATE_TOP_K_HARD_MAX];
    int seenCount = 0;
    for (int wi = 0; wi < topK; ++wi) {
        if (timer.isOver()) break;
        if (!gPool.hasSpace(wanderLen)) break;
        BeamNode out;
        initChildNode(cur, out);
        FixedPath path;
        int moved = 0;
        for (int t = 0; t < wanderLen; ++t) {
            int d = pickWanderStepForState(out.st);
            if (d == -1) break;
            if (!appendMove(out, (int8_t)d)) break;
            path.push_back((int8_t)d);
            ++moved;
            if (out.st.prefixLen(desired, M) >= M) break;
        }
        if (moved == 0) continue;
        bool dup = false;
        for (int i = 0; i < seenCount; ++i) {
            if (seen[i] == path) {
                dup = true;
                break;
            }
        }
        if (dup) continue;
        seen[seenCount++] = path;
        finalizeBeamNodeAfterTransition(cur, out);
        insertFn(move(out));
    }
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
    auto& layerKeys = layerKeysBuf;
    DeadlineChecker timer{deadline};
    const int goTargetCandidateTopK = BEAM_CANDIDATE_TOP_K_BY_N[N];
    const int cutApplyNodeLimit = CUT_APPLY_NODE_LIMIT_BY_N[N];
    const int wanderApplyNodeLimit = WANDER_APPLY_NODE_LIMIT_BY_N[N];
    const int cutCandidateTopK = BEAM_CUT_CANDIDATE_TOP_K_BY_N[N];
    const int wanderCandidateTopK = WANDER_CANDIDATE_TOP_K_BY_N[N];
    auto ensureLayer = [&](int t) {
        if (layerRunStamp[t] == runStamp) return;
        layerRunStamp[t] = runStamp;
        auto& vec = layers[t];
        auto& keys = layerKeys[t];
        vec.clear();
        keys.clear();
        int expected = beamWidth * (goTargetCandidateTopK + cutCandidateTopK + wanderCandidateTopK) * 2;
        expected = max(expected, beamWidth * 4);
        if ((int)vec.capacity() < expected) vec.reserve(expected);
        if ((int)keys.capacity() < expected) keys.reserve(expected);
    };

    auto ins = [&](BeamNode&& cand) {
        int t = cand.st.turn;
        if (t < startTurn || t > beamTurnCap) return;
        if (pruneByIncumbent(cand, incumbentBestLen)) return;
        ensureLayer(t);
        auto& vec = layers[t];
        auto& keys = layerKeys[t];
        uint64_t key = beamStateKey(cand.st);
        int dupIdx = -1;
        {
            const int sz = (int)keys.size();
            const uint64_t* kp = keys.data();
            // Scan only recent 32 entries — covers nearly all true dups
            // while cutting O(n) linear scan to O(1) amortized
            const int scanFrom = (sz > 32) ? sz - 32 : 0;
            for (int i = sz - 1; i >= scanFrom; --i) {
                if (kp[i] == key) { dupIdx = i; goto found; }
            }
        }
        found:
        if (dupIdx < 0) {
            if (__builtin_expect((int)vec.size() >= (int)vec.capacity(), 0)) return;
            keys.push_back(key);
            vec.push_back(move(cand));
        } else {
            if (betterBeamNode(cand, vec[dupIdx])) vec[dupIdx] = move(cand);
        }
    };

    ensureLayer(startTurn);
    layers[startTurn].push_back(init);
    layerKeys[startTurn].push_back(beamStateKey(init.st));

    bool hasDone = false; BeamNode bestDone, bestSeen = init;
    auto timeUp = [&]() -> bool {
        return timer.isOver();
    };

    for (int turn = startTurn; turn <= beamTurnCap; ++turn) {
        if (timeUp()) break;
        if (layerRunStamp[turn] != runStamp) continue;
        auto& beam = layers[turn];
        auto& keys = layerKeys[turn];
        if (beam.empty()) {
            keys.clear();
            continue;
        }

        int order[BEAM_WIDTH_HARD_MAX];
        int ordN = selectTopBeamIndices(beam, beamWidth, order);

        for (int oi = 0; oi < ordN; ++oi) {
            int bi = order[oi];
            if (timeUp()) break;
            auto& node = beam[bi];
            if (betterBeamNode(node, bestSeen)) bestSeen = node;
            int p = node.prefix;
            if (p >= M) { if (!hasDone || betterBeamNode(node, bestDone)) { hasDone = true; bestDone = node; } continue; }
            if (node.st.turn >= beamTurnCap) continue;
            if (pruneByIncumbent(node, incumbentBestLen)) continue;

            transitionGoTargetCandidates(node, timer, goTargetCandidateTopK, ins);
            if (oi < cutApplyNodeLimit) {
                transitionCutRecoverCandidates(node, timer, cutCandidateTopK, ins);
            }
            if (oi < wanderApplyNodeLimit) transitionWander(node, timer, wanderCandidateTopK, ins);
        }
        if (hasDone) break;
        keys.clear();
        beam.clear();
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
    if (N <= 8) {
        SolverT<3, 8, 64, 63, 1> solver;
        solver.readInput(N, M, C);
        solver.solve();
        solver.printAnswer();
    } else {
        SolverT<4, 16, 256, 255, 4> solver;
        solver.readInput(N, M, C);
        solver.solve();
        solver.printAnswer();
    }
    return 0;
}
