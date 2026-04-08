#include <bits/stdc++.h>
#pragma GCC target("avx2,bmi,bmi2,popcnt,sse4.2")
#pragma GCC optimize("O3,unroll-loops")
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
constexpr int MAX_TURN_LIMIT = 100000;
constexpr int BEAM_WIDTH = 5;
constexpr int WANDER_LEN = 2;
constexpr int SEARCH_TIME_LIMIT_MS = 1850;
constexpr int BEAM_CANDIDATE_TOP_K = 3;

constexpr int CUT_ENUM_MAX_DEPTH = 3;
constexpr int CUT_KEEP_MIN_LEN = 3;
constexpr int CUT_LEN_DEVIATION_WEIGHT = 100;
constexpr int CUT_LEN_OVER_HALF_WEIGHT = 300;
constexpr long long CUT_EVAL_BITE_TURN_WEIGHT = 60'000LL;
constexpr long long CUT_EVAL_ADJ_BONUS = 120'000LL;
constexpr int CUT_EVAL_DIRECT_UNREACHABLE = 20'000;
constexpr int CUT_EVAL_DIRECT_DISTANCE_WEIGHT = 220;
constexpr int CUT_EVAL_LOOSE_UNREACHABLE = 10'000;
constexpr int CUT_EVAL_LOOSE_DISTANCE_WEIGHT = 30;
constexpr long long CUT_EVAL_PREFIX_AFTER_WEIGHT = 300LL;
constexpr long long CUT_BEAM_WRONG_SUFFIX_WEIGHT = 2'000'000LL;
constexpr long long CUT_BEAM_PREFIX_LOSS_WEIGHT = 120'000LL;
constexpr long long CUT_BEAM_NO_WRONG_REDUCTION_PENALTY = 2'500'000LL;
constexpr long long CUT_BEAM_BITE_TURN_WEIGHT = 20'000LL;
constexpr long long CUT_BEAM_NOT_ADJ_PENALTY = 90'000LL;
constexpr int CUT_BEAM_DIRECT_UNREACHABLE = 400'000;
constexpr int CUT_BEAM_DIRECT_DISTANCE_WEIGHT = 120;
constexpr int CUT_BEAM_LOOSE_UNREACHABLE = 200'000;
constexpr int CUT_BEAM_LOOSE_DISTANCE_WEIGHT = 25;
constexpr long long CUT_BEAM_NOT_REACHABLE_PENALTY = 4'000'000LL;

constexpr long long SCORE_PREFIX_PRIMARY_WEIGHT = 1'000'000LL;
constexpr long long SCORE_FOOD_WALL_PENALTY_WEIGHT = 1LL;
constexpr long long SCORE_FOOD_CORNER_EXTRA_PENALTY_WEIGHT = 2LL;

constexpr int BEAM_TURN_CAP_MAX = 5000;
constexpr int BEAM_WIDTH_MAX = 80;
constexpr int BEAM_CUT_CANDIDATE_TOP_K = 1;
constexpr int CUT_APPLY_NODE_LIMIT = 12;

inline uint8_t packPos(int r, int c) { return (uint8_t)((r << 4) | c); }
inline int posR(uint8_t p) { return p >> 4; }
inline int posC(uint8_t p) { return p & 15; }

struct FixedPath {
    int8_t d[MAXCELLS];
    int16_t len = 0;
    bool empty() const { return len == 0; }
    int size() const { return len; }
    int8_t front() const { return d[0]; }
    int8_t operator[](int i) const { return d[i]; }
    void push_back(int8_t v) { d[len++] = v; }
    void reverse_path() { std::reverse(d, d + len); }
    bool operator==(const FixedPath& o) const {
        return len == o.len && memcmp(d, o.d, len) == 0;
    }
};

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

struct SnakeState {
    int8_t N = 0;
    int8_t grid[MAXN][MAXN] = {};
    uint8_t bodyOccCnt[MAXN][MAXN] = {};
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

    bool inBounds(int r, int c) const {
        return (unsigned)r < (unsigned)N && (unsigned)c < (unsigned)N;
    }

    int foodIdx(int r, int c) const { return r * (int)N + c; }
    bool isWallCell(int r, int c) const {
        int edge = (int)N - 1;
        return r == 0 || c == 0 || r == edge || c == edge;
    }
    bool isCornerCell(int r, int c) const {
        int edge = (int)N - 1;
        return (r == 0 || r == edge) && (c == 0 || c == edge);
    }
    void foodOccSet(int r, int c) {
        int idx = foodIdx(r, c);
        foodOcc[idx >> 6] |= (1ULL << (idx & 63));
    }
    void foodOccClear(int r, int c) {
        int idx = foodIdx(r, c);
        foodOcc[idx >> 6] &= ~(1ULL << (idx & 63));
    }
    void foodColorOccSet(int color, int r, int c) {
        int idx = foodIdx(r, c);
        foodColorOcc[color][idx >> 6] |= (1ULL << (idx & 63));
    }
    void foodColorOccClear(int color, int r, int c) {
        int idx = foodIdx(r, c);
        foodColorOcc[color][idx >> 6] &= ~(1ULL << (idx & 63));
    }
    void onFoodAdded(int r, int c, int color) {
        ++foodCount;
        foodOccSet(r, c);
        foodColorOccSet(color, r, c);
        if (isWallCell(r, c)) ++wallFoodCount;
        if (isCornerCell(r, c)) ++cornerFoodCount;
    }
    void onFoodRemoved(int r, int c, int color) {
        --foodCount;
        foodOccClear(r, c);
        foodColorOccClear(color, r, c);
        if (isWallCell(r, c)) --wallFoodCount;
        if (isCornerCell(r, c)) --cornerFoodCount;
    }
    void bodyOccInc(int r, int c) {
        uint8_t& cnt = bodyOccCnt[r][c];
        ++cnt;
        if (cnt == 1) {
            int idx = foodIdx(r, c);
            bodyOcc[idx >> 6] |= (1ULL << (idx & 63));
        }
    }
    void bodyOccDec(int r, int c) {
        uint8_t& cnt = bodyOccCnt[r][c];
        --cnt;
        if (cnt == 0) {
            int idx = foodIdx(r, c);
            bodyOcc[idx >> 6] &= ~(1ULL << (idx & 63));
        }
    }

    int prefixLen(const int8_t* desired, int desiredLen) const {
        int lim = min((int)colorLen, desiredLen);
        if (lim == 0) return 0;
        int p = 0;
        int start = colorHead & BODY_MASK;
        if (start + lim <= MAXBODY) {
            const int8_t* cols = colorBuf + start;
#ifdef __SSE2__
            while (p + 16 <= lim) {
                __m128i a = _mm_loadu_si128((const __m128i*)(cols + p));
                __m128i b = _mm_loadu_si128((const __m128i*)(desired + p));
                __m128i cmp = _mm_cmpeq_epi8(a, b);
                int mask = _mm_movemask_epi8(cmp);
                if (mask != 0xFFFF) {
                    return p + __builtin_ctz(~mask);
                }
                p += 16;
            }
#endif
            while (p < lim && cols[p] == desired[p]) ++p;
        } else {
            while (p < lim && colorAt(p) == desired[p]) ++p;
        }
        return p;
    }

    DirList legalDirs() const {
        DirList dl;
        if (bodyLen == 0) return dl;
        uint8_t head = bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (bodyLen >= 2) neck = bodyAt(1);
        for (int d = 0; d < 4; ++d) {
            int nr = hr + DR[d], nc = hc + DC[d];
            if ((unsigned)nr >= (unsigned)N || (unsigned)nc >= (unsigned)N) continue;
            if (packPos(nr, nc) == neck) continue;
            dl.d[dl.n++] = (int8_t)d;
        }
        return dl;
    }

    MoveResult apply(int d) {
        MoveResult ret;
        if (bodyLen == 0) return ret;

        uint8_t head = bodyFront();
        int hr = posR(head), hc = posC(head);
        int nr = hr + DR[d], nc = hc + DC[d];

        if ((unsigned)nr >= (unsigned)N || (unsigned)nc >= (unsigned)N) return ret;
        uint8_t npos = packPos(nr, nc);
        if (bodyLen >= 2 && npos == bodyAt(1)) return ret;

        bodyPushFront(npos);
        bodyOccInc(nr, nc);

        if (grid[nr][nc] != 0) {
            ret.ate = true;
            ret.eatenColor = grid[nr][nc];
            grid[nr][nc] = 0;
            colorPushBack(ret.eatenColor);
            onFoodRemoved(nr, nc, ret.eatenColor);
        } else {
            uint8_t tail = bodyBack();
            bodyPopBack();
            bodyOccDec(posR(tail), posC(tail));
        }

        int k = bodyLen;
        if (bodyOccCnt[nr][nc] >= 2) {
            for (int h = 1; h <= k - 2; ++h) {
                if (bodyAt(h) == npos) {
                    for (int p = h + 1; p < k; ++p) {
                        uint8_t bp = bodyAt(p);
                        int br = posR(bp), bc = posC(bp);
                        bodyOccDec(br, bc);
                        int col = colorAt(p);
                        grid[br][bc] = col;
                        onFoodAdded(br, bc, col);
                    }
                    bodyLen = h + 1;
                    colorLen = h + 1;
                    ret.bite = true;
                    break;
                }
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
    SnakeState after;
    int biteTurn = 0;
    int prefixAfter = 0;
    int anchorLen = 0;
    bool adjacentToPrefix = false;
    int directPathLen = INT_MAX;
    int loosePathLen = INT_MAX;
};

struct Solver {
    int N = 0, M = 0, C = 0;
    int8_t desired[MAXCELLS];
    int nnCells = 0;
    uint8_t idxToR[MAXCELLS] = {};
    uint8_t idxToC[MAXCELLS] = {};
    uint8_t manhattanDist[MAXCELLS][MAXCELLS] = {};

    SnakeState state;
    vector<int8_t> movesDirs;
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

    inline double randDouble() {
        return (double)(randInt() % 1000000000u) / 1e9;
    }

    void readInput() {
        ios::sync_with_stdio(false);
        cin.tie(nullptr);

        cin >> N >> M >> C;
        nnCells = N * N;
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
                state.grid[i][j] = (int8_t)x;
                if (x != 0) {
                    state.onFoodAdded(i, j, x);
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
            state.bodyOccInc(posR(p), posC(p));
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
        const int n = st.N;
        const int INF = 0x3f3f3f3f;

        int release[MAXN][MAXN] = {};
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            int r = posR(bp), c = posC(bp);
            release[r][c] = (int)st.bodyLen - i;
        }

        int dist[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        memset(dist, 0x3f, sizeof(dist));

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        // Simple BFS queue with early exit when topK goals found
        uint8_t bq[MAXCELLS];
        int qh = 0, qt = 0;
        dist[hr][hc] = 0;
        bq[qt++] = packPos(hr, hc);

        struct Goal { int dist, r, c; };
        Goal goals[MAXCELLS];
        int gn = 0;
        int goalMaxDist = -1;

        while (qh < qt) {
            uint8_t pos = bq[qh++];
            int r = posR(pos), c = posC(pos);
            int t = dist[r][c];
            // Early exit: found enough goals and moved past their distance
            if (gn >= topK && t > goalMaxDist) break;
            if (!(r == hr && c == hc) && st.grid[r][c] == targetColor) {
                goals[gn++] = {t, r, c};
                goalMaxDist = t;
                continue; // don't expand past goal cells
            }
            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d], nc = c + DC[d];
                if ((unsigned)nr >= (unsigned)n || (unsigned)nc >= (unsigned)n) continue;
                if (r == hr && c == hc && packPos(nr, nc) == neck) continue;
                int food = st.grid[nr][nc];
                if (food != 0 && food != targetColor) continue;
                int nt = t + 1;
                if (nt < release[nr][nc]) continue;
                if (nt < dist[nr][nc]) {
                    dist[nr][nc] = nt;
                    parentPacked[nr][nc] = packPos(r, c);
                    pdir[nr][nc] = (int8_t)d;
                    bq[qt++] = packPos(nr, nc);
                }
            }
        }
        if (gn == 0) return 0;

        int take = min(topK, gn);
        int cnt = 0;
        for (int gi = 0; gi < take; ++gi) {
            FixedPath& path = out[cnt];
            path.len = 0;
            int cr = goals[gi].r, cc = goals[gi].c;
            while (!(cr == hr && cc == hc)) {
                path.push_back(pdir[cr][cc]);
                uint8_t pp = parentPacked[cr][cc];
                cr = posR(pp); cc = posC(pp);
            }
            path.reverse_path();
            if (path.len > 0) ++cnt;
        }
        return cnt;
    }

    // Dedicated single-target BFS with early exit (no bucket queue overhead)
    FixedPath findDynamicStrictPathToTarget(const SnakeState& st, int targetColor) const {
        FixedPath result;
        if (st.bodyLen == 0) return result;
        const int n = st.N;

        int release[MAXN][MAXN] = {};
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            release[posR(bp)][posC(bp)] = (int)st.bodyLen - i;
        }

        int dist[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        memset(dist, 0x3f, sizeof(dist));

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        uint8_t bq[MAXCELLS];
        int qh = 0, qt = 0;
        dist[hr][hc] = 0;
        bq[qt++] = packPos(hr, hc);

        int goalR = -1, goalC = -1;
        while (qh < qt) {
            uint8_t pos = bq[qh++];
            int r = posR(pos), c = posC(pos);
            int t = dist[r][c];
            if (!(r == hr && c == hc) && st.grid[r][c] == targetColor) {
                goalR = r; goalC = c; break;
            }
            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d], nc = c + DC[d];
                if ((unsigned)nr >= (unsigned)n || (unsigned)nc >= (unsigned)n) continue;
                if (r == hr && c == hc && packPos(nr, nc) == neck) continue;
                int food = st.grid[nr][nc];
                if (food != 0 && food != targetColor) continue;
                int nt = t + 1;
                if (nt < release[nr][nc]) continue;
                if (nt < dist[nr][nc]) {
                    dist[nr][nc] = nt;
                    parentPacked[nr][nc] = packPos(r, c);
                    pdir[nr][nc] = (int8_t)d;
                    bq[qt++] = packPos(nr, nc);
                }
            }
        }

        if (goalR == -1) return result;
        int cr = goalR, cc = goalC;
        while (!(cr == hr && cc == hc)) {
            result.push_back(pdir[cr][cc]);
            uint8_t pp = parentPacked[cr][cc];
            cr = posR(pp); cc = posC(pp);
        }
        result.reverse_path();
        return result;
    }

    // Lexicographic shortest path:
    //   1) minimize count of traversed non-target foods
    //   2) minimize path length
    // while respecting dynamic body release timing.
    FixedPath findPreferFoodAvoidPathToTarget(const SnakeState& st, int targetColor) const {
        FixedPath result;
        if (st.bodyLen == 0) return result;
        const int n = st.N;
        const int INF = 0x3f3f3f3f;

        int release[MAXN][MAXN] = {};
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            release[posR(bp)][posC(bp)] = (int)st.bodyLen - i;
        }

        int bestFood[MAXN][MAXN];
        int bestStep[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        memset(bestFood, 0x3f, sizeof(bestFood));
        memset(bestStep, 0x3f, sizeof(bestStep));

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        struct SoftNode {
            int foodCost;
            int step;
            uint8_t pos;
        };
        static constexpr int SOFT_HEAP_CAP = MAXCELLS * MAXCELLS * 4;
        static thread_local SoftNode heap[SOFT_HEAP_CAP];
        int heapSize = 0;
        auto betterNode = [](const SoftNode& a, const SoftNode& b) {
            return (a.foodCost < b.foodCost) || (a.foodCost == b.foodCost && a.step < b.step);
        };
        auto heapPush = [&](int foodCost, int step, int r, int c) {
            if (heapSize >= SOFT_HEAP_CAP) return;
            SoftNode x{foodCost, step, packPos(r, c)};
            int i = heapSize++;
            while (i > 0) {
                int p = (i - 1) >> 1;
                if (!betterNode(x, heap[p])) break;
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
                if (r < heapSize && betterNode(heap[r], heap[l])) ch = r;
                if (!betterNode(heap[ch], last)) break;
                heap[i] = heap[ch];
                i = ch;
            }
            if (heapSize > 0) heap[i] = last;
            return ret;
        };
        bestFood[hr][hc] = 0;
        bestStep[hr][hc] = 0;
        heapPush(0, 0, hr, hc);

        int goalR = -1, goalC = -1;
        while (heapSize > 0) {
            SoftNode cur = heapPop();
            int fc = cur.foodCost;
            int step = cur.step;
            int r = posR(cur.pos);
            int c = posC(cur.pos);
            if (fc != bestFood[r][c] || step != bestStep[r][c]) continue;
            if (!(r == hr && c == hc) && st.grid[r][c] == targetColor) {
                goalR = r;
                goalC = c;
                break;
            }
            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d], nc = c + DC[d];
                if ((unsigned)nr >= (unsigned)n || (unsigned)nc >= (unsigned)n) continue;
                if (r == hr && c == hc && packPos(nr, nc) == neck) continue;
                int nstep = step + 1;
                if (nstep < release[nr][nc]) continue;
                int food = st.grid[nr][nc];
                int nfc = fc + ((food != 0 && food != targetColor) ? 1 : 0);
                if (nfc < bestFood[nr][nc] || (nfc == bestFood[nr][nc] && nstep < bestStep[nr][nc])) {
                    bestFood[nr][nc] = nfc;
                    bestStep[nr][nc] = nstep;
                    parentPacked[nr][nc] = packPos(r, c);
                    pdir[nr][nc] = (int8_t)d;
                    heapPush(nfc, nstep, nr, nc);
                }
            }
        }

        if (goalR == -1) return result;
        if (bestFood[goalR][goalC] >= INF || bestStep[goalR][goalC] >= INF) return result;
        int cr = goalR, cc = goalC;
        while (!(cr == hr && cc == hc)) {
            result.push_back(pdir[cr][cc]);
            uint8_t pp = parentPacked[cr][cc];
            cr = posR(pp); cc = posC(pp);
        }
        result.reverse_path();
        return result;
    }

    FixedPath findDynamicStrictPathToTail(const SnakeState& st, int targetColor) const {
        FixedPath result;
        if (st.bodyLen == 0) return result;
        const int n = st.N;
        const int INF = 0x3f3f3f3f;

        int release[MAXN][MAXN] = {};
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            release[posR(bp)][posC(bp)] = (int)st.bodyLen - i;
        }

        uint8_t tail = st.bodyBack();
        int gr = posR(tail), gc = posC(tail);

        int dist[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        memset(dist, 0x3f, sizeof(dist));

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        uint8_t bq[MAXCELLS];
        int qh = 0, qt = 0;
        dist[hr][hc] = 0;
        bq[qt++] = packPos(hr, hc);

        while (qh < qt) {
            uint8_t pos = bq[qh++];
            int r = posR(pos), c = posC(pos);
            int t = dist[r][c];
            if (!(r == hr && c == hc) && r == gr && c == gc) break;
            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d], nc = c + DC[d];
                if ((unsigned)nr >= (unsigned)n || (unsigned)nc >= (unsigned)n) continue;
                if (r == hr && c == hc && packPos(nr, nc) == neck) continue;
                int food = st.grid[nr][nc];
                if (food != 0 && food != targetColor) continue;
                int nt = t + 1;
                if (nt < release[nr][nc]) continue;
                if (nt < dist[nr][nc]) {
                    dist[nr][nc] = nt;
                    parentPacked[nr][nc] = packPos(r, c);
                    pdir[nr][nc] = (int8_t)d;
                    bq[qt++] = packPos(nr, nc);
                }
            }
        }

        if (dist[gr][gc] >= INF) return result;
        int cr = gr, cc = gc;
        while (!(cr == hr && cc == hc)) {
            result.push_back(pdir[cr][cc]);
            uint8_t pp = parentPacked[cr][cc];
            cr = posR(pp); cc = posC(pp);
        }
        result.reverse_path();
        return result;
    }

    FixedPath bfsToTargetColor(
        const SnakeState& st,
        int targetColor,
        bool avoidBody,
        bool avoidNonTargetFood
    ) const {
        FixedPath result;
        if (st.bodyLen == 0) return result;
        const int n = st.N;

        int dist[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        char blocked[MAXN][MAXN] = {};
        memset(dist, -1, sizeof(dist));

        if (avoidBody) {
            for (int i = 1; i < st.bodyLen; ++i) {
                uint8_t bp = st.bodyAt(i);
                blocked[posR(bp)][posC(bp)] = 1;
            }
        }

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        pair<int8_t,int8_t> q[MAXCELLS];
        int qh = 0, qt = 0;
        q[qt++] = {(int8_t)hr, (int8_t)hc};
        dist[hr][hc] = 0;

        int goalR = -1, goalC = -1;
        while (qh < qt) {
            auto [r, c] = q[qh++];
            if (!(r == hr && c == hc) && st.grid[r][c] == targetColor) {
                goalR = r; goalC = c; break;
            }
            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d], nc = c + DC[d];
                if ((unsigned)nr >= (unsigned)n || (unsigned)nc >= (unsigned)n) continue;
                if (dist[nr][nc] != -1) continue;
                if (r == hr && c == hc && packPos(nr, nc) == neck) continue;
                if (avoidBody && blocked[nr][nc]) continue;
                int food = st.grid[nr][nc];
                if (avoidNonTargetFood && food != 0 && food != targetColor) continue;
                dist[nr][nc] = dist[r][c] + 1;
                parentPacked[nr][nc] = packPos(r, c);
                pdir[nr][nc] = (int8_t)d;
                q[qt++] = {(int8_t)nr, (int8_t)nc};
            }
        }

        if (goalR == -1) return result;
        int cr = goalR, cc = goalC;
        while (!(cr == hr && cc == hc)) {
            result.push_back(pdir[cr][cc]);
            uint8_t pp = parentPacked[cr][cc];
            cr = posR(pp); cc = posC(pp);
        }
        result.reverse_path();
        return result;
    }

    bool isTargetAdjacentToPrefixSegment(const SnakeState& st, int anchorLen, int targetColor) const {
        if (anchorLen <= 0 || anchorLen > st.bodyLen) return false;
        uint8_t bp = st.bodyAt(anchorLen - 1);
        int r = posR(bp), c = posC(bp);
        for (int d = 0; d < 4; ++d) {
            int nr = r + DR[d], nc = c + DC[d];
            if (st.inBounds(nr, nc) && st.grid[nr][nc] == targetColor) return true;
        }
        return false;
    }

    void enumerateCutCandidatesDFS(
        const SnakeState& cur,
        int depth, int maxDepth,
        int baseL, int targetColor,
        int8_t seq[CUT_ENUM_MAX_DEPTH], int seqLen,
        vector<CutCandidate>& out
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
                int pref = nxt.prefixLen(desired, M);
                int lenAfter = nxt.bodyLen;
                if (lenAfter >= CUT_KEEP_MIN_LEN && (lenAfter % 2 == 1)) {
                    int anchorLen = min(baseL, lenAfter);
                    CutCandidate cand;
                    memcpy(cand.seq, seq, newLen);
                    cand.seqLen = (int8_t)newLen;
                    cand.after = nxt;
                    cand.biteTurn = newLen;
                    cand.prefixAfter = pref;
                    cand.anchorLen = anchorLen;
                    cand.adjacentToPrefix = isTargetAdjacentToPrefixSegment(nxt, anchorLen, targetColor);
                    auto dp = findDynamicStrictPathToTarget(nxt, targetColor);
                    cand.directPathLen = dp.empty() ? INT_MAX : dp.size();
                    // Skip expensive loose BFS if direct path found or adjacent
                    if (cand.directPathLen != INT_MAX || cand.adjacentToPrefix) {
                        cand.loosePathLen = cand.directPathLen; // approximate
                    } else {
                        auto lp = bfsToTargetColor(nxt, targetColor, false, true);
                        cand.loosePathLen = lp.empty() ? INT_MAX : lp.size();
                    }
                    out.push_back(move(cand));
                }
            } else {
                enumerateCutCandidatesDFS(nxt, depth + 1, maxDepth, baseL, targetColor, seq, newLen, out);
            }
        }
    }

    vector<CutCandidate> enumerateCutCandidates(
        const SnakeState& src, int baseL, int targetColor, int maxDepth = CUT_ENUM_MAX_DEPTH
    ) const {
        vector<CutCandidate> out;
        out.reserve(64);
        int8_t seq[CUT_ENUM_MAX_DEPTH];
        enumerateCutCandidatesDFS(src, 0, maxDepth, baseL, targetColor, seq, 0, out);
        return out;
    }

    vector<CutCandidate> chooseTopCutCandidatesForBeam(
        const SnakeState& src, int baseL, int targetColor, int topK
    ) const {
        topK = max(1, topK);
        auto cands = enumerateCutCandidates(src, baseL, targetColor, CUT_ENUM_MAX_DEPTH);
        if (cands.empty()) return {};
        int wrongSuffix = max(0, (int)src.bodyLen - baseL);
        auto evalScore = [&](const CutCandidate& c) -> long long {
            int la = c.after.bodyLen, rw = max(0, la - baseL), lp = max(0, baseL - la);
            int rmw = max(0, wrongSuffix - rw);
            bool reach = c.adjacentToPrefix || c.directPathLen != INT_MAX || c.loosePathLen != INT_MAX;
            long long s = CUT_BEAM_WRONG_SUFFIX_WEIGHT * rw + CUT_BEAM_PREFIX_LOSS_WEIGHT * lp;
            if (!rmw && wrongSuffix > 0) s += CUT_BEAM_NO_WRONG_REDUCTION_PENALTY;
            s += CUT_BEAM_BITE_TURN_WEIGHT * c.biteTurn;
            if (!c.adjacentToPrefix) s += CUT_BEAM_NOT_ADJ_PENALTY;
            s += (c.directPathLen == INT_MAX ? CUT_BEAM_DIRECT_UNREACHABLE : CUT_BEAM_DIRECT_DISTANCE_WEIGHT * c.directPathLen);
            s += (c.loosePathLen == INT_MAX ? CUT_BEAM_LOOSE_UNREACHABLE : CUT_BEAM_LOOSE_DISTANCE_WEIGHT * c.loosePathLen);
            if (!reach) s += CUT_BEAM_NOT_REACHABLE_PENALTY;
            return s;
        };
        if (topK == 1) {
            int bestIdx = 0;
            long long bestScore = evalScore(cands[0]);
            for (int i = 1; i < (int)cands.size(); ++i) {
                long long s = evalScore(cands[i]);
                if (s < bestScore) {
                    bestScore = s;
                    bestIdx = i;
                }
            }
            vector<CutCandidate> out;
            out.reserve(1);
            out.push_back(cands[bestIdx]);
            return out;
        }
        struct R { long long s; int i; };
        vector<R> ranked; ranked.reserve(cands.size());
        for (int i = 0; i < (int)cands.size(); ++i) {
            ranked.push_back({evalScore(cands[i]), i});
        }
        sort(ranked.begin(), ranked.end(), [](const R& a, const R& b) { return a.s != b.s ? a.s < b.s : a.i < b.i; });
        vector<CutCandidate> out;
        int take = min(topK, (int)ranked.size());
        for (int i = 0; i < take; ++i) out.push_back(cands[ranked[i].i]);
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

    long long evaluateBeamState(const SnakeState& st, int maxPrefix) const {
        (void)maxPrefix;
        return evaluateBeamStateFromPrefix(st, st.prefixLen(desired, M));
    }

    int safeOptimisticLowerBound(const SnakeState& st, int p) const {
        int rem = M - p;
        if (rem <= 0) return 0;
        if (st.bodyLen <= 0) return rem;

        int tc = desired[p];
        if ((unsigned)tc >= MAXC) return rem;
        uint8_t head = st.bodyFront();
        int headIdx = posR(head) * N + posC(head);
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

    int pickWanderStepForState(const SnakeState& st, int /*targetColor*/, int /*baseL*/) {
        DirList dirs = st.legalDirs();
        if (dirs.n == 0) return -1;
        return dirs.d[randRange(0, dirs.n - 1)];
    }

    void transitionGoTargetCandidates(
        const BeamNode& cur, chrono::steady_clock::time_point dl, int topK,
        BeamNode* out, int& outN
    ) {
        outN = 0;
        if (chrono::steady_clock::now() >= dl) return;
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
        // If suffix is broken, or strict routes are unavailable, use soft route
        // that minimizes crossing non-target foods (then length).
        if (prefBroken || pathCount == 0) {
            tryPush(findPreferFoodAvoidPathToTarget(cur.st, tc));
        }
        if (pathCount == 0) return;
        if (pathCount > topK) pathCount = topK;

        for (int pi = 0; pi < pathCount; ++pi) {
            if (chrono::steady_clock::now() >= dl) break;
            const FixedPath& path = paths[pi];
            if (!gPool.hasSpace(path.len)) continue;
            BeamNode nxt;
            nxt.st = cur.st;
            nxt.pathTail = cur.pathTail;
            nxt.pathLen = cur.pathLen;
            bool ok = true;
            for (int i = 0; i < path.len; ++i) {
                if (nxt.st.turn >= activeTurnLimit) break;
                MoveResult ret = nxt.st.apply(path.d[i]);
                if (!ret.ok) { ok = false; break; }
                nxt.pathTail = gPool.add(path.d[i], nxt.pathTail);
                nxt.pathLen++;
            }
            if (!ok) continue;
            int np = nxt.st.prefixLen(desired, M);
            nxt.prefix = np;
            nxt.maxPrefix = max(cur.maxPrefix, np);
            nxt.eval = evaluateBeamStateFromPrefix(nxt.st, np);
            out[outN++] = move(nxt);
        }
    }

    void transitionCutRecoverCandidates(
        const BeamNode& cur, chrono::steady_clock::time_point dl, int topK,
        BeamNode* out, int& outN
    ) {
        outN = 0;
        if (chrono::steady_clock::now() >= dl) return;
        int p = cur.prefix;
        if (p >= M) return;
        // Cut-recover is useful only when suffix is broken (wrong colors exist).
        if (p >= cur.st.colorLen) return;
        auto cuts = chooseTopCutCandidatesForBeam(cur.st, p, desired[p], topK);
        if (cuts.empty()) return;

        for (auto& cut : cuts) {
            if (chrono::steady_clock::now() >= dl) break;
            if (!gPool.hasSpace(cut.seqLen + MAXCELLS)) continue;
            BeamNode nxt;
            nxt.st = cur.st;
            nxt.pathTail = cur.pathTail;
            nxt.pathLen = cur.pathLen;
            bool bitten = false, ok = true;
            for (int si = 0; si < cut.seqLen; ++si) {
                if (nxt.st.turn >= activeTurnLimit) break;
                SnakeState bef = nxt.st;
                MoveResult ret = nxt.st.apply(cut.seq[si]);
                if (!ret.ok) { ok = false; break; }
                nxt.pathTail = gPool.add(cut.seq[si], nxt.pathTail);
                nxt.pathLen++;
                if (ret.bite) {
                    bitten = true;
                    int pb = bef.prefixLen(desired, M);
                    auto recovery = buildRecoveryPathByOldBody(bef, nxt.st, pb);
                    for (int ri = 0; ri < recovery.len; ++ri) {
                        if (nxt.st.turn >= activeTurnLimit) break;
                        MoveResult rr = nxt.st.apply(recovery.d[ri]);
                        if (!rr.ok) { ok = false; break; }
                        nxt.pathTail = gPool.add(recovery.d[ri], nxt.pathTail);
                        nxt.pathLen++;
                    }
                    break;
                }
            }
            if (!ok || !bitten) continue;
            int np = nxt.st.prefixLen(desired, M);
            nxt.prefix = np;
            nxt.maxPrefix = max(cur.maxPrefix, np);
            nxt.eval = evaluateBeamStateFromPrefix(nxt.st, np);
            out[outN++] = move(nxt);
        }
    }

    bool transitionWander(const BeamNode& cur, chrono::steady_clock::time_point dl, BeamNode& out) {
        if (chrono::steady_clock::now() >= dl) return false;
        int p = cur.prefix;
        if (p >= M) return false;
        if (!gPool.hasSpace(WANDER_LEN)) return false;
        int tc = desired[p];
        out.st = cur.st;
        out.pathTail = cur.pathTail;
        out.pathLen = cur.pathLen;
        int moved = 0;
        for (int t = 0; t < WANDER_LEN; ++t) {
            if (out.st.turn >= activeTurnLimit) break;
            int d = pickWanderStepForState(out.st, tc, p);
            if (d == -1) break;
            MoveResult ret = out.st.apply(d);
            if (!ret.ok) break;
            out.pathTail = gPool.add((int8_t)d, out.pathTail);
            out.pathLen++;
            ++moved;
            if (out.st.prefixLen(desired, M) >= M) break;
        }
        if (moved == 0) return false;
        int np = out.st.prefixLen(desired, M);
        out.prefix = np;
        out.maxPrefix = max(cur.maxPrefix, np);
        out.eval = evaluateBeamStateFromPrefix(out.st, np);
        return true;
    }

    bool isDoneNode(const BeamNode& node) const { return node.prefix >= M; }
    bool betterFinalNode(const BeamNode& a, const BeamNode& b) const {
        bool ad = isDoneNode(a), bd = isDoneNode(b);
        if (ad != bd) return ad;
        return betterBeamNode(a, b);
    }

    optional<BeamNode> runBeamSearch(
        const SnakeState& initial, chrono::steady_clock::time_point deadline,
        int beamWidth, int beamTurnCap, int incumbentBestLen
    ) {
        BeamNode init;
        init.st = initial;
        init.prefix = init.st.prefixLen(desired, M);
        init.maxPrefix = init.prefix;
        init.optimisticLB = safeOptimisticLowerBound(init.st, init.prefix);
        init.eval = evaluateBeamStateFromPrefix(init.st, init.prefix);
        init.pathTail = -1;
        init.pathLen = 0;
        if (init.st.turn > beamTurnCap) return nullopt;

        int startTurn = init.st.turn;
        ++currentRunStamp;
        if (currentRunStamp == INT_MAX) {
            memset(layerRunStamp, 0, sizeof(layerRunStamp));
            currentRunStamp = 1;
        }
        const int runStamp = currentRunStamp;
        auto& layers = layersBuf;
        auto& layerKeys = layerKeysBuf;
        auto ensureLayer = [&](int t) {
            if (layerRunStamp[t] == runStamp) return;
            layerRunStamp[t] = runStamp;
            auto& vec = layers[t];
            auto& keys = layerKeys[t];
            vec.clear();
            keys.clear();
            int expected = beamWidth * (BEAM_CANDIDATE_TOP_K + BEAM_CUT_CANDIDATE_TOP_K + 1);
            expected = max(expected, beamWidth);
            if ((int)vec.capacity() < expected) vec.reserve(expected);
            if ((int)keys.capacity() < expected) keys.reserve(expected);
        };

        auto ins = [&](BeamNode&& cand) {
            int t = cand.st.turn;
            if (t < startTurn || t > beamTurnCap) return;
            if (incumbentBestLen < INT_MAX) {
                if (cand.optimisticLB < 0) {
                    cand.optimisticLB = safeOptimisticLowerBound(cand.st, cand.prefix);
                }
                if (cand.st.turn + cand.optimisticLB >= incumbentBestLen) return;
            }
            ensureLayer(t);
            auto& vec = layers[t];
            auto& keys = layerKeys[t];
            uint64_t key = beamStateKey(cand.st);
            int dupIdx = -1;
            for (int i = (int)keys.size() - 1; i >= 0; --i) {
                if (keys[i] == key) { dupIdx = i; break; }
            }
            if (dupIdx < 0) {
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
        int deadlineTick = 0;
        auto timeUp = [&]() -> bool {
            if ((++deadlineTick & 7) != 0) return false;
            return chrono::steady_clock::now() >= deadline;
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
            int ordN = 0;
            for (int i = 0; i < (int)beam.size(); ++i) {
                if (ordN < beamWidth) {
                    int pos = ordN++;
                    while (pos > 0 && betterBeamNode(beam[i], beam[order[pos - 1]])) {
                        order[pos] = order[pos - 1];
                        --pos;
                    }
                    order[pos] = i;
                } else if (betterBeamNode(beam[i], beam[order[ordN - 1]])) {
                    int pos = ordN - 1;
                    while (pos > 0 && betterBeamNode(beam[i], beam[order[pos - 1]])) {
                        order[pos] = order[pos - 1];
                        --pos;
                    }
                    order[pos] = i;
                }
            }

            for (int oi = 0; oi < ordN; ++oi) {
                int bi = order[oi];
                if (timeUp()) break;
                auto& node = beam[bi];
                if (betterBeamNode(node, bestSeen)) bestSeen = node;
                int p = node.prefix;
                if (p >= M) { if (!hasDone || betterBeamNode(node, bestDone)) { hasDone = true; bestDone = node; } continue; }
                if (node.st.turn >= beamTurnCap) continue;
                if (incumbentBestLen < INT_MAX) {
                    if (node.optimisticLB < 0) {
                        node.optimisticLB = safeOptimisticLowerBound(node.st, node.prefix);
                    }
                    if (node.st.turn + node.optimisticLB >= incumbentBestLen) continue;
                }

                BeamNode cands[8]; int cn;
                transitionGoTargetCandidates(node, deadline, BEAM_CANDIDATE_TOP_K, cands, cn);
                for (int i = 0; i < cn; ++i) ins(move(cands[i]));
                if (oi < CUT_APPLY_NODE_LIMIT) {
                    transitionCutRecoverCandidates(node, deadline, BEAM_CUT_CANDIDATE_TOP_K, cands, cn);
                    for (int i = 0; i < cn; ++i) ins(move(cands[i]));
                }
                BeamNode wn;
                if (transitionWander(node, deadline, wn)) ins(move(wn));
            }
            if (hasDone) break;
            keys.clear();
            beam.clear();
        }
        return hasDone ? optional(bestDone) : optional(bestSeen);
    }

    void solve() {
        auto start = chrono::steady_clock::now();
        auto totalDL = start + chrono::milliseconds(SEARCH_TIME_LIMIT_MS);
        SnakeState initial = state;
        activeTurnLimit = MAX_TURN_LIMIT;

        BeamNode best;
        best.st = initial;
        best.pathTail = -1;
        best.pathLen = 0;
        best.prefix = initial.prefixLen(desired, M);
        best.maxPrefix = best.prefix;
        best.eval = evaluateBeamStateFromPrefix(best.st, best.prefix);
        vector<int8_t> bestPath;
        int btc = BEAM_TURN_CAP_MAX;
        int bw = BEAM_WIDTH;

        while (chrono::steady_clock::now() < totalDL) {
            gPool.clear();
            int incumbentBestLen = isDoneNode(best) ? (int)bestPath.size() : INT_MAX;
            auto bb = runBeamSearch(initial, totalDL, bw, btc, incumbentBestLen);
            if (bb && betterFinalNode(*bb, best)) {
                best = move(*bb);
                // Extract path from pool
                bestPath.resize(best.pathLen);
                if (best.pathTail >= 0 && best.pathLen > 0) {
                    gPool.extract(best.pathTail, best.pathLen, bestPath.data());
                }
                if (isDoneNode(best) && best.pathLen > 0) {
                    btc = min(btc, best.pathLen);
                }
            }
            if (chrono::steady_clock::now() >= totalDL) break;
            bw = min(BEAM_WIDTH_MAX, bw * 2);
        }

        // Output
        movesDirs = move(bestPath);
    }

    void printAnswer() const {
        for (int8_t d : movesDirs) cout << DCHAR[(int)d] << '\n';
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
