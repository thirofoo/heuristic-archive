#include <bits/stdc++.h>
#pragma GCC target("avx2,bmi,bmi2,popcnt,sse4.2")
#pragma GCC optimize("Ofast,omit-frame-pointer,inline,unroll-all-loops,no-stack-protector")
#include <immintrin.h>
using namespace std;

namespace {

constexpr int MAXN = 16;
constexpr int MAXC = 8;
constexpr int MAXBODY = 256;
constexpr int BODY_MASK = MAXBODY - 1;
constexpr int MAXCELLS = MAXN * MAXN;
constexpr int DR[4] = {-1, 1, 0, 0};
constexpr int DC[4] = {0, 0, -1, 1};
constexpr char DCHAR[4] = {'U', 'D', 'L', 'R'};
constexpr int DIR_ORDER_UDLR[4] = {0, 1, 2, 3};
constexpr int DIR_ORDER_RLDU[4] = {3, 2, 1, 0};
constexpr int MAX_TURN_LIMIT = 100000;
constexpr int BEAM_WIDTH = 5;
constexpr int WANDER_LEN = 2;
constexpr int SEARCH_TIME_LIMIT_MS = 1850;
constexpr int BEAM_CANDIDATE_TOP_K = 3;

constexpr int CUT_ENUM_MAX_DEPTH = 3;
constexpr int CUT_CANDIDATE_CAP = 64;
constexpr int CUT_KEEP_MIN_LEN = 3;
constexpr long long CUT_BEAM_WRONG_SUFFIX_WEIGHT = 2'000'000LL;
constexpr long long CUT_BEAM_PREFIX_LOSS_WEIGHT = 120'000LL;
constexpr long long CUT_BEAM_NO_WRONG_REDUCTION_PENALTY = 2'500'000LL;
constexpr long long CUT_BEAM_BITE_TURN_WEIGHT = 20'000LL;
constexpr long long CUT_BEAM_NOT_ADJ_PENALTY = 90'000LL;

constexpr long long SCORE_PREFIX_PRIMARY_WEIGHT = 1'000'000LL;
constexpr long long SCORE_FOOD_WALL_PENALTY_WEIGHT = 1LL;
constexpr long long SCORE_FOOD_CORNER_EXTRA_PENALTY_WEIGHT = 2LL;

constexpr int BEAM_TURN_CAP_MAX = 5000;
constexpr int BEAM_WIDTH_MAX = 80;
constexpr int BEAM_CUT_CANDIDATE_TOP_K = 2;
constexpr int CUT_APPLY_NODE_LIMIT = 12;
constexpr int BEAM_BAN_PREFIX_TURNS = 80;
constexpr uint16_t INVALID_NEXT_POS = 0xFFFFu;

inline uint8_t packPos(int r, int c) { return (uint8_t)((r << 4) | c); }
inline int posR(uint8_t p) { return p >> 4; }
inline int posC(uint8_t p) { return p & 15; }

inline int prefixMatchContiguous(const int8_t* a, const int8_t* b, int len) {
    int p = 0;
#ifdef __AVX2__
    while (p + 32 <= len) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + p));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + p));
        unsigned mask = (unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(va, vb));
        if (mask != 0xFFFFFFFFu) return p + __builtin_ctz(~mask);
        p += 32;
    }
#endif
#ifdef __SSE2__
    while (p + 16 <= len) {
        __m128i va = _mm_loadu_si128((const __m128i*)(a + p));
        __m128i vb = _mm_loadu_si128((const __m128i*)(b + p));
        unsigned mask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(va, vb));
        if (mask != 0xFFFFu) return p + __builtin_ctz(~mask);
        p += 16;
    }
#endif
    while (p < len && a[p] == b[p]) ++p;
    return p;
}

uint8_t gPosRow[MAXBODY];
uint8_t gPosCol[MAXBODY];
uint16_t gCellIdxByPos[MAXBODY];
uint16_t gNextPos[MAXBODY][4];
uint8_t gCellBlockByPos[MAXBODY];
uint64_t gCellMaskByPos[MAXBODY];
uint8_t gWallByPos[MAXBODY];
uint8_t gCornerByPos[MAXBODY];
uint32_t gDirOrderNonce = 0;

inline const int* chooseDirOrder() {
    uint32_t x = ++gDirOrderNonce;
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    return (x & 1U) ? DIR_ORDER_RLDU : DIR_ORDER_UDLR;
}

struct FixedPath {
    int8_t d[MAXCELLS];
    int16_t len = 0;
    bool empty() const { return len == 0; }
    int size() const { return len; }
    void push_back(int8_t v) { d[len++] = v; }
    bool operator==(const FixedPath& o) const {
        return len == o.len && memcmp(d, o.d, len) == 0;
    }
};

inline void reconstructPathBackward(
    FixedPath& path,
    int len,
    uint8_t goalPos,
    const uint8_t* parentPacked,
    const int8_t* pdir
) {
    path.len = (int16_t)len;
    uint8_t pos = goalPos;
    for (int idx = len - 1; idx >= 0; --idx) {
        path.d[idx] = pdir[pos];
        pos = parentPacked[pos];
    }
}

struct PathPool {
    struct Node { int8_t dir; int32_t parent; };
    static constexpr int CAP = 1 << 22; // 4M entries ~20MB
    Node nodes[CAP];
    int sz = 0;
    void clear() { sz = 0; }
    bool hasSpace(int need = 1) const { return sz + need <= CAP; }
    int add(int8_t dir, int parent) {
        if (sz >= CAP) return -1;
        nodes[sz] = {dir, parent};
        return sz++;
    }
    void extract(int tail, int len, int8_t* out) const {
        int idx = tail;
        for (int i = len - 1; i >= 0; --i) {
            out[i] = nodes[idx].dir;
            idx = nodes[idx].parent;
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
    bool ate = false;
    bool bite = false;
    int8_t eatenColor = 0;
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
        if ((++tick & 15u) != 0) return false;
        return chrono::steady_clock::now() >= deadline;
    }
};

struct SnakeState {
    int8_t N = 0;
    int8_t grid[MAXBODY] = {};
    uint8_t bodyOccCnt[MAXBODY] = {};
    uint64_t bodyOcc[4] = {};
    uint8_t bodyBuf[MAXBODY];
    int8_t colorBuf[MAXBODY];
    int16_t bodyHead = 0, bodyLen = 0;
    int16_t colorHead = 0, colorLen = 0;
    int turn = 0;
    int16_t foodCount = 0;
    int16_t wallFoodCount = 0;
    int16_t cornerFoodCount = 0;
    uint64_t foodOcc[4] = {};
    uint64_t foodColorOcc[MAXC][4] = {};

    uint8_t bodyAt(int i) const { return bodyBuf[(bodyHead + i) & BODY_MASK]; }
    int8_t colorAt(int i) const { return colorBuf[(colorHead + i) & BODY_MASK]; }

    void bodyPushFront(uint8_t v) {
        bodyHead = (bodyHead - 1) & BODY_MASK;
        bodyBuf[bodyHead] = v;
        ++bodyLen;
    }
    void bodyPopBack() { --bodyLen; }
    uint8_t bodyFront() const { return bodyBuf[bodyHead]; }
    uint8_t bodyBack() const { return bodyBuf[(bodyHead + bodyLen - 1) & BODY_MASK]; }

    void colorPushBack(int8_t v) {
        colorBuf[(colorHead + colorLen) & BODY_MASK] = v;
        ++colorLen;
    }

    void foodOccSetPos(uint8_t pos) {
        foodOcc[gCellBlockByPos[pos]] |= gCellMaskByPos[pos];
    }
    void foodOccClearPos(uint8_t pos) {
        foodOcc[gCellBlockByPos[pos]] &= ~gCellMaskByPos[pos];
    }
    void foodColorOccSetPos(int color, uint8_t pos) {
        foodColorOcc[color][gCellBlockByPos[pos]] |= gCellMaskByPos[pos];
    }
    void foodColorOccClearPos(int color, uint8_t pos) {
        foodColorOcc[color][gCellBlockByPos[pos]] &= ~gCellMaskByPos[pos];
    }
    void onFoodAddedPos(uint8_t pos, int color) {
        ++foodCount;
        foodOccSetPos(pos);
        foodColorOccSetPos(color, pos);
        wallFoodCount += gWallByPos[pos];
        cornerFoodCount += gCornerByPos[pos];
    }
    void onFoodRemovedPos(uint8_t pos, int color) {
        --foodCount;
        foodOccClearPos(pos);
        foodColorOccClearPos(color, pos);
        wallFoodCount -= gWallByPos[pos];
        cornerFoodCount -= gCornerByPos[pos];
    }
    void bodyOccIncPos(uint8_t pos) {
        uint8_t& cnt = bodyOccCnt[pos];
        ++cnt;
        if (cnt == 1) {
            bodyOcc[gCellBlockByPos[pos]] |= gCellMaskByPos[pos];
        }
    }
    void bodyOccDecPos(uint8_t pos) {
        uint8_t& cnt = bodyOccCnt[pos];
        --cnt;
        if (cnt == 0) {
            bodyOcc[gCellBlockByPos[pos]] &= ~gCellMaskByPos[pos];
        }
    }

    int prefixLen(const int8_t* desired, int desiredLen) const {
        int lim = min((int)colorLen, desiredLen);
        if (lim == 0) return 0;
        int start = colorHead & BODY_MASK;
        if (start + lim <= MAXBODY) {
            return prefixMatchContiguous(colorBuf + start, desired, lim);
        }
        int firstLen = MAXBODY - start;
        int matched = prefixMatchContiguous(colorBuf + start, desired, firstLen);
        if (matched != firstLen) return matched;
        return matched + prefixMatchContiguous(colorBuf, desired + matched, lim - matched);
    }

    DirList legalDirs() const {
        DirList dl;
        if (bodyLen == 0) return dl;
        uint8_t head = bodyFront();
        uint8_t neck = 255;
        if (bodyLen >= 2) neck = bodyAt(1);
        const int* dirOrder = chooseDirOrder();
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

        bodyPushFront(npos);
        bodyOccIncPos(npos);

        if (grid[npos] != 0) {
            ret.ate = true;
            ret.eatenColor = grid[npos];
            grid[npos] = 0;
            colorPushBack(ret.eatenColor);
            onFoodRemovedPos(npos, ret.eatenColor);
        } else {
            uint8_t tail = bodyBack();
            bodyPopBack();
            bodyOccDecPos(tail);
        }

        int k = bodyLen;
        if (__builtin_expect(bodyOccCnt[npos] >= 2, 0)) {
            int h = 1;
            int bi = (bodyHead + 1) & BODY_MASK;
            for (; h <= k - 2; ++h) {
                if (bodyBuf[bi] == npos) {
                    int restoreBodyIdx = (bi + 1) & BODY_MASK;
                    int restoreColorIdx = (colorHead + h + 1) & BODY_MASK;
                    int restoredFood = 0;
                    int restoredWall = 0;
                    int restoredCorner = 0;
                    for (int p = h + 1; p < k; ++p) {
                        uint8_t bp = bodyBuf[restoreBodyIdx];
                        uint8_t& cnt = bodyOccCnt[bp];
                        --cnt;
                        if (cnt == 0) {
                            bodyOcc[gCellBlockByPos[bp]] &= ~gCellMaskByPos[bp];
                        }
                        int color = colorBuf[restoreColorIdx];
                        grid[bp] = (int8_t)color;
                        ++restoredFood;
                        foodOcc[gCellBlockByPos[bp]] |= gCellMaskByPos[bp];
                        foodColorOcc[color][gCellBlockByPos[bp]] |= gCellMaskByPos[bp];
                        restoredWall += gWallByPos[bp];
                        restoredCorner += gCornerByPos[bp];
                        restoreBodyIdx = (restoreBodyIdx + 1) & BODY_MASK;
                        restoreColorIdx = (restoreColorIdx + 1) & BODY_MASK;
                    }
                    foodCount += (int16_t)restoredFood;
                    wallFoodCount += (int16_t)restoredWall;
                    cornerFoodCount += (int16_t)restoredCorner;
                    bodyLen = (int16_t)(h + 1);
                    colorLen = (int16_t)(h + 1);
                    ret.bite = true;
                    break;
                }
                bi = (bi + 1) & BODY_MASK;
            }
        }

        ++turn;
        ret.ok = true;
        return ret;
    }
};

struct CutCandidate {
    int8_t seq[CUT_ENUM_MAX_DEPTH];
    int8_t seqLen = 0;
    int16_t afterBodyLen = 0;
    bool adjacentToPrefix = false;
};

struct CutCandidateList {
    CutCandidate data[CUT_CANDIDATE_CAP];
    int size = 0;

    bool empty() const { return size == 0; }
    void clear() { size = 0; }
    void push_back(const CutCandidate& cand) {
        if (size < CUT_CANDIDATE_CAP) data[size++] = cand;
    }
};

struct Solver {
    int N = 0, M = 0, C = 0;
    int8_t desired[MAXCELLS];
    int nnCells = 0;
    uint8_t idxToR[MAXCELLS] = {};
    uint8_t idxToC[MAXCELLS] = {};
    uint8_t manhattanDist[MAXCELLS][MAXCELLS] = {};

    SnakeState state;
    MoveSeq movesDirs;
    uint32_t tx = 123456789u;
    uint32_t ty = 362436069u;
    uint32_t tz = 521288629u;
    uint32_t tw = 88675123u;
    int activeTurnLimit = MAX_TURN_LIMIT;

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
    uint64_t bannedPathStateKey[BEAM_TURN_CAP_MAX + 1] = {};
    uint8_t bannedPathUsed[BEAM_TURN_CAP_MAX + 1] = {};

    Solver() {
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

    void readInput() {
        ios::sync_with_stdio(false);
        cin.tie(nullptr);

        cin >> N >> M >> C;
        nnCells = N * N;
        for (int i = 0; i < MAXBODY; ++i) gCellIdxByPos[i] = INVALID_NEXT_POS;
        memset(gNextPos, 0xFF, sizeof(gNextPos));
        memset(gCellBlockByPos, 0, sizeof(gCellBlockByPos));
        memset(gCellMaskByPos, 0, sizeof(gCellMaskByPos));
        memset(gWallByPos, 0, sizeof(gWallByPos));
        memset(gCornerByPos, 0, sizeof(gCornerByPos));
        for (int r = 0; r < MAXN; ++r) {
            for (int c = 0; c < MAXN; ++c) {
                uint8_t pos = packPos(r, c);
                gPosRow[pos] = (uint8_t)r;
                gPosCol[pos] = (uint8_t)c;
                if (r < N && c < N) {
                    int idx = r * N + c;
                    gCellIdxByPos[pos] = (uint16_t)idx;
                    gCellBlockByPos[pos] = (uint8_t)(idx >> 6);
                    gCellMaskByPos[pos] = 1ULL << (idx & 63);
                    gWallByPos[pos] = (uint8_t)(r == 0 || c == 0 || r == N - 1 || c == N - 1);
                    gCornerByPos[pos] = (uint8_t)((r == 0 || r == N - 1) && (c == 0 || c == N - 1));
                    for (int d = 0; d < 4; ++d) {
                        int nr = r + DR[d], nc = c + DC[d];
                        if ((unsigned)nr < (unsigned)N && (unsigned)nc < (unsigned)N) {
                            gNextPos[pos][d] = packPos(nr, nc);
                        }
                    }
                }
            }
        }
        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                int idx = r * N + c;
                idxToR[idx] = (uint8_t)r;
                idxToC[idx] = (uint8_t)c;
            }
        }
        for (int i = 0; i < nnCells; ++i) {
            int r1 = idxToR[i], c1 = idxToC[i];
            for (int j = 0; j < nnCells; ++j) {
                int r2 = idxToR[j], c2 = idxToC[j];
                manhattanDist[i][j] = (uint8_t)(abs(r1 - r2) + abs(c1 - c2));
            }
        }
        for (int i = 0; i < M; ++i) {
            int x; cin >> x;
            desired[i] = (int8_t)x;
        }

        state.N = (int8_t)N;
        memset(state.grid, 0, sizeof(state.grid));
        memset(state.bodyOccCnt, 0, sizeof(state.bodyOccCnt));
        memset(state.bodyOcc, 0, sizeof(state.bodyOcc));
        state.foodCount = 0;
        state.wallFoodCount = 0;
        state.cornerFoodCount = 0;
        memset(state.foodOcc, 0, sizeof(state.foodOcc));
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
        state.bodyLen = 5;
        state.bodyBuf[0] = packPos(4, 0);
        state.bodyBuf[1] = packPos(3, 0);
        state.bodyBuf[2] = packPos(2, 0);
        state.bodyBuf[3] = packPos(1, 0);
        state.bodyBuf[4] = packPos(0, 0);
        for (int i = 0; i < 5; ++i) {
            uint8_t p = state.bodyBuf[i];
            state.bodyOccIncPos(p);
        }

        state.colorHead = 0;
        state.colorLen = 5;
        for (int i = 0; i < 5; ++i) state.colorBuf[i] = 1;

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
        const SnakeState& beforeBite,
        const SnakeState& afterBite,
        int prefixBefore
    ) const {
        FixedPath plan;
        int lenAfter = afterBite.bodyLen;
        if (prefixBefore <= lenAfter) return plan;
        if (beforeBite.bodyLen == 0 || afterBite.bodyLen == 0) return plan;

        int startIdx = lenAfter - 1;
        int endIdx = prefixBefore - 2;
        if (startIdx < 0 || endIdx < startIdx || endIdx >= beforeBite.bodyLen) return plan;

        uint8_t cur = afterBite.bodyFront();
        for (int idx = startIdx; idx <= endIdx; ++idx) {
            uint8_t nxt = beforeBite.bodyAt(idx);
            int d = dirBetween(cur, nxt);
            if (d < 0) { plan.len = 0; return plan; }
            plan.push_back((int8_t)d);
            cur = nxt;
        }
        return plan;
    }

    int findDynamicStrictPathsToTarget(
        const SnakeState& st,
        int targetColor,
        int topK,
        FixedPath* out
    ) const {
        if (st.bodyLen == 0) return 0;
        topK = max(1, topK);
        int16_t release[MAXBODY];
        memset(release, 0, sizeof(release));
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            release[bp] = (int16_t)(st.bodyLen - i);
        }

        int16_t dist[MAXBODY];
        uint8_t parentPacked[MAXBODY];
        int8_t pdir[MAXBODY];
        memset(dist, 0x3f, sizeof(dist));

        uint8_t head = st.bodyFront();
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        // Simple BFS queue with early exit when topK goals found
        uint8_t bq[MAXCELLS];
        int qh = 0, qt = 0;
        dist[head] = 0;
        bq[qt++] = head;

        struct Goal { int16_t dist; uint8_t pos; };
        Goal goals[MAXCELLS];
        int gn = 0;
        int goalMaxDist = -1;
        const int* dirOrder = chooseDirOrder();

        while (qh < qt) {
            uint8_t pos = bq[qh++];
            int t = dist[pos];
            // Early exit: found enough goals and moved past their distance
            if (gn >= topK && t > goalMaxDist) break;
            if (pos != head && st.grid[pos] == targetColor) {
                goals[gn++] = {(int16_t)t, pos};
                goalMaxDist = t;
                continue; // don't expand past goal cells
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
                if (nt < release[npos]) continue;
                if (nt < dist[npos]) {
                    dist[npos] = (int16_t)nt;
                    parentPacked[npos] = pos;
                    pdir[npos] = (int8_t)d;
                    bq[qt++] = npos;
                }
            }
        }
        if (gn == 0) return 0;

        int take = min(topK, gn);
        int cnt = 0;
        for (int gi = 0; gi < take; ++gi) {
            FixedPath& path = out[cnt];
            reconstructPathBackward(path, goals[gi].dist, goals[gi].pos, parentPacked, pdir);
            if (path.len > 0) ++cnt;
        }
        return cnt;
    }

    // Lexicographic shortest path:
    //   1) minimize count of traversed non-target foods
    //   2) minimize path length
    // while respecting dynamic body release timing.
    FixedPath findPreferFoodAvoidPathToTarget(const SnakeState& st, int targetColor) const {
        FixedPath result;
        if (st.bodyLen == 0) return result;

        int16_t release[MAXBODY];
        memset(release, 0, sizeof(release));
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            release[bp] = (int16_t)(st.bodyLen - i);
        }

        // Pack (foodCost, step) into uint64_t for single-compare lexicographic ordering
        uint64_t bestFS[MAXBODY];
        memset(bestFS, 0x3f, sizeof(bestFS));
        uint8_t parentPacked[MAXBODY];
        int8_t pdir[MAXBODY];

        uint8_t head = st.bodyFront();
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        struct SoftNode {
            uint64_t packed; // (foodCost << 32) | step
            uint8_t pos;
        };
        static constexpr int SOFT_HEAP_CAP = MAXCELLS * MAXCELLS * 4;
        static thread_local SoftNode heap[SOFT_HEAP_CAP];
        int heapSize = 0;
        auto heapPush = [&](uint64_t packed, uint8_t pos) {
            if (heapSize >= SOFT_HEAP_CAP) return;
            SoftNode x{packed, pos};
            int i = heapSize++;
            while (i > 0) {
                int p = (i - 1) >> 1;
                if (x.packed >= heap[p].packed) break;
                heap[i] = heap[p];
                i = p;
            }
            heap[i] = x;
        };
        auto heapPop = [&]() {
            SoftNode ret = heap[0];
            SoftNode last = heap[--heapSize];
            int i = 0;
            while (true) {
                int l = (i << 1) + 1;
                if (l >= heapSize) break;
                int r = l + 1;
                int ch = l;
                if (r < heapSize && heap[r].packed < heap[l].packed) ch = r;
                if (last.packed <= heap[ch].packed) break;
                heap[i] = heap[ch];
                i = ch;
            }
            if (heapSize > 0) heap[i] = last;
            return ret;
        };
        bestFS[head] = 0;
        heapPush(0, head);
        const int* dirOrder = chooseDirOrder();

        uint8_t goalPos = 0;
        bool goalFound = false;
        while (heapSize > 0) {
            SoftNode cur = heapPop();
            if (cur.packed != bestFS[cur.pos]) continue;
            if (cur.pos != head && st.grid[cur.pos] == targetColor) {
                goalPos = cur.pos;
                goalFound = true;
                break;
            }
            int fc = (int)(cur.packed >> 32);
            int step = (int)(cur.packed & 0xFFFFFFFFU);
            for (int di = 0; di < 4; ++di) {
                int d = dirOrder[di];
                uint16_t next = gNextPos[cur.pos][d];
                if (next == INVALID_NEXT_POS) continue;
                uint8_t npos = (uint8_t)next;
                if (cur.pos == head && npos == neck) continue;
                int nstep = step + 1;
                if (nstep < release[npos]) continue;
                int food = st.grid[npos];
                int nfc = fc + ((food != 0 && food != targetColor) ? 1 : 0);
                uint64_t npacked = ((uint64_t)(unsigned)nfc << 32) | (unsigned)nstep;
                if (npacked < bestFS[npos]) {
                    bestFS[npos] = npacked;
                    parentPacked[npos] = cur.pos;
                    pdir[npos] = (int8_t)d;
                    heapPush(npacked, npos);
                }
            }
        }

        if (!goalFound) return result;
        if (bestFS[goalPos] >= 0x3f3f3f3f00000000ULL) return result;
        int goalStep = (int)(bestFS[goalPos] & 0xFFFFFFFFU);
        reconstructPathBackward(result, goalStep, goalPos, parentPacked, pdir);
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
        const SnakeState& cur,
        int depth, int maxDepth,
        int baseL, int targetColor,
        int8_t seq[CUT_ENUM_MAX_DEPTH], int seqLen,
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
        const SnakeState& src, int baseL, int targetColor, int maxDepth = CUT_ENUM_MAX_DEPTH
    ) const {
        CutCandidateList out;
        int8_t seq[CUT_ENUM_MAX_DEPTH];
        enumerateCutCandidatesDFS(src, 0, maxDepth, baseL, targetColor, seq, 0, out);
        return out;
    }

    CutCandidateList chooseTopCutCandidatesForBeam(
        const SnakeState& src, int baseL, int targetColor, int topK
    ) const {
        topK = max(1, topK);
        auto cands = enumerateCutCandidates(src, baseL, targetColor, CUT_ENUM_MAX_DEPTH);
        if (cands.empty()) return {};
        int wrongSuffix = max(0, (int)src.bodyLen - baseL);
        auto evalScore = [&](const CutCandidate& c) -> long long {
            int la = c.afterBodyLen, rw = max(0, la - baseL), lp = max(0, baseL - la);
            int rmw = max(0, wrongSuffix - rw);
            long long s = CUT_BEAM_WRONG_SUFFIX_WEIGHT * rw + CUT_BEAM_PREFIX_LOSS_WEIGHT * lp;
            if (!rmw && wrongSuffix > 0) s += CUT_BEAM_NO_WRONG_REDUCTION_PENALTY;
            s += CUT_BEAM_BITE_TURN_WEIGHT * c.seqLen;
            if (!c.adjacentToPrefix) s += CUT_BEAM_NOT_ADJ_PENALTY;
            return s;
        };
        if (topK == 1) {
            int bestIdx = 0;
            long long bestScore = evalScore(cands.data[0]);
            for (int i = 1; i < cands.size; ++i) {
                long long s = evalScore(cands.data[i]);
                if (s < bestScore) {
                    bestScore = s;
                    bestIdx = i;
                }
            }
            CutCandidateList out;
            out.push_back(cands.data[bestIdx]);
            return out;
        }
        struct R { long long s; int i; };
        R ranked[CUT_CANDIDATE_CAP];
        int rankedSize = cands.size;
        for (int i = 0; i < rankedSize; ++i) {
            ranked[i] = {evalScore(cands.data[i]), i};
        }
        CutCandidateList out;
        int take = min(topK, rankedSize);
        for (int k = 0; k < take; ++k) {
            int bestJ = k;
            for (int j = k + 1; j < rankedSize; ++j) {
                if (ranked[j].s < ranked[bestJ].s || (ranked[j].s == ranked[bestJ].s && ranked[j].i < ranked[bestJ].i)) {
                    bestJ = j;
                }
            }
            if (bestJ != k) { R tmp = ranked[k]; ranked[k] = ranked[bestJ]; ranked[bestJ] = tmp; }
            out.push_back(cands.data[ranked[k].i]);
        }
        return out;
    }

    long long foodWallPenalty(const SnakeState& st) const {
        if (st.foodCount <= 0) return 0LL;
        return SCORE_FOOD_WALL_PENALTY_WEIGHT * (long long)st.wallFoodCount
             + SCORE_FOOD_CORNER_EXTRA_PENALTY_WEIGHT * (long long)st.cornerFoodCount;
    }

    inline long long evaluateBeamStateFromPrefix(const SnakeState& st, int pref) const {
        return -SCORE_PREFIX_PRIMARY_WEIGHT * pref + foodWallPenalty(st);
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
        for (int blk = 0; blk < 4; ++blk) {
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
        h ^= st.foodOcc[0] * 0xbf58476d1ce4e5b9ULL;
        h ^= st.foodOcc[1] * 0x94d049bb133111ebULL;
        h ^= st.foodOcc[2] * 0xd6e8feb86659fd93ULL;
        h ^= st.foodOcc[3] * 0xa0761d6478bd642fULL;
        h ^= st.bodyOcc[0] * 0xe7037ed1a0b428dbULL;
        h ^= st.bodyOcc[1] * 0x8ebc6af09c88c6e3ULL;
        h ^= st.bodyOcc[2] * 0x589965cc75374cc3ULL;
        h ^= st.bodyOcc[3] * 0x1d8e4e27c47d124fULL;
        h ^= (uint64_t)(unsigned)st.bodyLen * 0x2545f4914f6cdd1dULL;
        if (st.bodyLen > 0) {
            h ^= (uint64_t)st.bodyFront() * 0x369dea0f31a53f85ULL;
            h ^= (uint64_t)st.bodyBack() * 0xdb4f0b9175ae2165ULL;
        }
        return h;
    }

    bool betterBeamNode(const BeamNode& a, const BeamNode& b) const {
        if (a.eval != b.eval) return a.eval < b.eval;
        if (a.maxPrefix != b.maxPrefix) return a.maxPrefix > b.maxPrefix;
        return a.st.turn < b.st.turn;
    }

    void clearBannedPathStates() {
        memset(bannedPathUsed, 0, sizeof(bannedPathUsed));
    }

    void rebuildBannedPathStates(const SnakeState& initial, const MoveSeq& path) {
        clearBannedPathStates();
        if (path.len <= BEAM_BAN_PREFIX_TURNS) return;
        SnakeState st = initial;
        for (int i = 0; i < path.len; ++i) {
            MoveResult ret = st.apply(path.d[i]);
            if (!ret.ok) break;
            int t = st.turn;
            if (t <= BEAM_BAN_PREFIX_TURNS || t > BEAM_TURN_CAP_MAX) continue;
            bannedPathUsed[t] = 1;
            bannedPathStateKey[t] = beamStateKey(st);
        }
    }

    inline void initChildNode(const BeamNode& parent, BeamNode& child) const {
        child.st = parent.st;
        child.pathTail = parent.pathTail;
        child.pathLen = parent.pathLen;
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
        // Extract compact sort keys for cache-friendly sorting
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

    int pickWanderStepForState(const SnakeState& st) {
        DirList dirs = st.legalDirs();
        if (dirs.n == 0) return -1;
        return dirs.d[randRange(0, dirs.n - 1)];
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
            bool ok = true;
            for (int i = 0; i < path.len; ++i) {
                if (nxt.st.turn >= activeTurnLimit) break;
                MoveResult ret = nxt.st.apply(path.d[i]);
                if (!ret.ok) { ok = false; break; }
                nxt.pathTail = gPool.add(path.d[i], nxt.pathTail);
                nxt.pathLen++;
            }
            if (!ok) continue;
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
            if (!gPool.hasSpace(cut.seqLen + MAXCELLS)) continue;
            BeamNode nxt;
            initChildNode(cur, nxt);
            bool bitten = false, ok = true;
            for (int si = 0; si < cut.seqLen; ++si) {
                if (nxt.st.turn >= activeTurnLimit) break;
                if (si + 1 == cut.seqLen) {
                    // Save only body data instead of full SnakeState copy
                    uint8_t savedBodyBuf[MAXBODY];
                    memcpy(savedBodyBuf, nxt.st.bodyBuf, sizeof(savedBodyBuf));
                    int16_t savedBodyHead = nxt.st.bodyHead;
                    int16_t savedBodyLen = nxt.st.bodyLen;
                    int pb = nxt.st.prefixLen(desired, M);
                    MoveResult ret = nxt.st.apply(cut.seq[si]);
                    if (!ret.ok || !ret.bite) { ok = false; break; }
                    nxt.pathTail = gPool.add(cut.seq[si], nxt.pathTail);
                    nxt.pathLen++;
                    bitten = true;
                    // Build recovery from saved body data
                    {
                        FixedPath recovery;
                        int lenAfter = nxt.st.bodyLen;
                        if (pb > lenAfter && savedBodyLen > 0 && nxt.st.bodyLen > 0) {
                            int startIdx = lenAfter - 1;
                            int endIdx = pb - 2;
                            if (startIdx >= 0 && endIdx >= startIdx && endIdx < savedBodyLen) {
                                uint8_t rcur = nxt.st.bodyFront();
                                bool rok = true;
                                for (int idx = startIdx; idx <= endIdx; ++idx) {
                                    uint8_t rnxt = savedBodyBuf[(savedBodyHead + idx) & BODY_MASK];
                                    int rd = dirBetween(rcur, rnxt);
                                    if (rd < 0) { rok = false; break; }
                                    recovery.push_back((int8_t)rd);
                                    rcur = rnxt;
                                }
                                if (!rok) recovery.len = 0;
                            }
                        }
                        for (int ri = 0; ri < recovery.len; ++ri) {
                            if (nxt.st.turn >= activeTurnLimit) break;
                            MoveResult rr = nxt.st.apply(recovery.d[ri]);
                            if (!rr.ok) { ok = false; break; }
                            nxt.pathTail = gPool.add(recovery.d[ri], nxt.pathTail);
                            nxt.pathLen++;
                        }
                    }
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
    void transitionWander(const BeamNode& cur, DeadlineChecker& timer, InsertFn& insertFn) {
        if (timer.isOver()) return;
        int p = cur.prefix;
        if (p >= M) return;
        if (!gPool.hasSpace(WANDER_LEN)) return;
        BeamNode out;
        initChildNode(cur, out);
        int moved = 0;
        for (int t = 0; t < WANDER_LEN; ++t) {
            if (out.st.turn >= activeTurnLimit) break;
            int d = pickWanderStepForState(out.st);
            if (d == -1) break;
            MoveResult ret = out.st.apply(d);
            if (!ret.ok) break;
            out.pathTail = gPool.add((int8_t)d, out.pathTail);
            out.pathLen++;
            ++moved;
            if (out.st.prefixLen(desired, M) >= M) break;
        }
        if (moved == 0) return;
        finalizeBeamNodeAfterTransition(cur, out);
        insertFn(move(out));
    }

    bool isDoneNode(const BeamNode& node) const { return node.prefix >= M; }
    bool betterFinalNode(const BeamNode& a, const BeamNode& b) const {
        bool ad = isDoneNode(a), bd = isDoneNode(b);
        if (ad != bd) return ad;
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
        auto ensureLayer = [&](int t) {
            if (layerRunStamp[t] == runStamp) return;
            layerRunStamp[t] = runStamp;
            auto& vec = layers[t];
            auto& keys = layerKeys[t];
            vec.clear();
            keys.clear();
            int expected = beamWidth * (BEAM_CANDIDATE_TOP_K + BEAM_CUT_CANDIDATE_TOP_K + 1) * 2;
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
            if (bannedPathUsed[t] && bannedPathStateKey[t] == key) return;
            int dupIdx = -1;
            {
                const int sz = (int)keys.size();
                const uint64_t* kp = keys.data();
#ifdef __AVX2__
                __m256i vkey = _mm256_set1_epi64x((long long)key);
                int i = 0;
                for (; i + 4 <= sz; i += 4) {
                    __m256i vdata = _mm256_loadu_si256((const __m256i*)(kp + i));
                    int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi64(vkey, vdata));
                    if (mask) {
                        dupIdx = i + (__builtin_ctz(mask) >> 3);
                        goto found;
                    }
                }
                for (; i < sz; ++i) {
                    if (kp[i] == key) { dupIdx = i; goto found; }
                }
#else
                for (int i = sz - 1; i >= 0; --i) {
                    if (kp[i] == key) { dupIdx = i; goto found; }
                }
#endif
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

            int order[BEAM_WIDTH_MAX];
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

                transitionGoTargetCandidates(node, timer, BEAM_CANDIDATE_TOP_K, ins);
                if (oi < CUT_APPLY_NODE_LIMIT) {
                    transitionCutRecoverCandidates(node, timer, BEAM_CUT_CANDIDATE_TOP_K, ins);
                }
                transitionWander(node, timer, ins);
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
        int bw = BEAM_WIDTH;
        clearBannedPathStates();

        while (true) {
            if (totalTimer.isOver()) break;
            gPool.clear();
            int incumbentBestLen = isDoneNode(best) ? bestPath.size() : INT_MAX;
            BeamNode candidate;
            bool ran = runBeamSearch(initial, totalDL, bw, btc, incumbentBestLen, candidate);
            if (ran && betterFinalNode(candidate, best)) {
                best = move(candidate);
                // Extract path from pool
                bestPath.len = best.pathLen;
                if (best.pathTail >= 0 && best.pathLen > 0) {
                    gPool.extract(best.pathTail, best.pathLen, bestPath.d);
                }
                if (isDoneNode(best) && best.pathLen > 0) {
                    btc = min(btc, best.pathLen);
                    rebuildBannedPathStates(initial, bestPath);
                }
                // cerr << "best_update"
                //      << " bw=" << bw
                //      << " btc=" << btc
                //      << " turn=" << best.st.turn
                //      << " len=" << best.pathLen
                //      << " prefix=" << best.prefix << "/" << M
                //      << " maxPrefix=" << best.maxPrefix
                //      << " eval=" << best.eval
                //      << " done=" << (isDoneNode(best) ? 1 : 0)
                //      << '\n';
            }
            // cerr << bw << " Finish\n";
            if (totalTimer.isOver()) break;
            bw = min(BEAM_WIDTH_MAX, bw * 2);
        }

        // Output
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
};

}  // namespace

int main() {
    Solver solver;
    solver.readInput();
    solver.solve();
    solver.printAnswer();
    return 0;
}
