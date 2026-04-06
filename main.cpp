#pragma GCC target("avx2,bmi,bmi2,popcnt,sse4.2")
#pragma GCC optimize("O3,unroll-loops")
#include <bits/stdc++.h>
#include <immintrin.h>
using namespace std;

namespace {

constexpr int MAXN = 16;
constexpr int MAXBODY = 256;
constexpr int BODY_MASK = MAXBODY - 1;
constexpr int MAXCELLS = MAXN * MAXN;
constexpr int DR[4] = {-1, 1, 0, 0};
constexpr int DC[4] = {0, 0, -1, 1};
constexpr char DCHAR[4] = {'U', 'D', 'L', 'R'};
constexpr int MAX_TURN_LIMIT = 100000;
constexpr int INITIAL_GREEDY_TURN_LIMIT = 100;
constexpr int BEAM_WIDTH = 5;
constexpr int WANDER_LEN = 2;
constexpr int SEARCH_TIME_LIMIT_MS = 1800;
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
constexpr long long SCORE_MISMATCH_WEIGHT = 10'000LL;
constexpr int SCORE_MISSING_LENGTH_FACTOR = 2;
constexpr double WANDER_RAND_WEIGHT = 5.0;
constexpr double WANDER_EMPTY_SCORE = 1.0;
constexpr double WANDER_FOOD_SCORE = 2.0;
constexpr double WANDER_MOBILITY_WEIGHT = 0.5;
constexpr double WANDER_TARGET_FOOD_BONUS = 3.0;
constexpr double WANDER_BITE_PENALTY = 5.0;
constexpr double WANDER_PREFIX_BREAK_PENALTY = 8.0;
constexpr double WANDER_TARGET_DISTANCE_WEIGHT = 0.05;
constexpr int WANDER_CHOICE_TOP_K = 3;
constexpr int LEGACY_NEAR_FINISH_REMAINING = 2;
constexpr int LEGACY_TAIL_PATH_STAGNATION_LIMIT = 60;
constexpr int LEGACY_WANDER_TRIGGER_STAGNATION = 10;
constexpr int LEGACY_WANDER_BUDGET = 5;
constexpr int LEGACY_CUT_THRESHOLD_NEAR_FIN = 120;
constexpr int LEGACY_CUT_THRESHOLD_DEFAULT = 30;
constexpr int LEGACY_CUT_THRESHOLD_PREF_BROKEN_CAP = 18;
constexpr int LEGACY_CUT_STAGNATION_REDUCE = 15;
constexpr int LEGACY_PENALIZED_TRIGGER_STAGNATION = 80;
constexpr int LEGACY_PENALIZED_FOOD_PENALTY_FACTOR = 3;
constexpr double EXPLORE_RANDOM_MOVE_PROB = 0.15;
constexpr double EXPLORE_LAND_EMPTY_SCORE = 2.5;
constexpr double EXPLORE_LAND_TARGET_SCORE = 8.0;
constexpr double EXPLORE_LAND_OTHER_PENALTY = 8.0;
constexpr double EXPLORE_NON_TARGET_EMPTY_SCORE = 2.0;
constexpr double EXPLORE_NON_TARGET_FOOD_SCORE = 1.5;
constexpr double EXPLORE_BITE_BONUS = 0.5;
constexpr double EXPLORE_PREFIX_BREAK_PENALTY = 2000.0;
constexpr double EXPLORE_MOBILITY_WEIGHT = 0.6;
constexpr double EXPLORE_TARGET_DISTANCE_WEIGHT = 0.4;
constexpr int EXPLORE_CHOICE_TOP_K = 3;
constexpr double EXPLORE_TOP_CHOICE_PROB = 0.75;
constexpr int GREEDY_STEP_BOUND_SMALL = 1000;
constexpr int GREEDY_STEP_BOUND_MIDDLE = 10000;
constexpr int GREEDY_STEP_SMALL = 100;
constexpr int GREEDY_STEP_MIDDLE = 1000;
constexpr int GREEDY_STEP_LARGE = 10000;
constexpr int BEAM_TURN_CAP_MAX = 5000;
constexpr int BEAM_WIDTH_MAX = 1'000'000;
constexpr int BEAM_CUT_CANDIDATE_TOP_K = 1;

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
    int add(int8_t dir, int parent) {
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
    uint8_t bodyBuf[MAXBODY];
    int8_t colorBuf[MAXBODY];
    int16_t bodyHead = 0, bodyLen = 0;
    int16_t colorHead = 0, colorLen = 0;
    int turn = 0;
    int16_t foodCount = 0;

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

        if (grid[nr][nc] != 0) {
            ret.ate = true;
            ret.eatenColor = grid[nr][nc];
            grid[nr][nc] = 0;
            colorPushBack(ret.eatenColor);
            --foodCount;
        } else {
            bodyPopBack();
        }

        int k = bodyLen;
        for (int h = 1; h <= k - 2; ++h) {
            if (bodyAt(h) == npos) {
                for (int p = h + 1; p < k; ++p) {
                    uint8_t bp = bodyAt(p);
                    grid[posR(bp)][posC(bp)] = colorAt(p);
                    ++foodCount;
                }
                bodyLen = h + 1;
                colorLen = h + 1;
                ret.bite = true;
                break;
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

    SnakeState state;
    vector<int8_t> movesDirs;
    uint32_t tx = 123456789u;
    uint32_t ty = 362436069u;
    uint32_t tz = 521288629u;
    uint32_t tw = 88675123u;
    int8_t recoveryBuf[MAXCELLS];
    int recoveryHead = 0, recoveryLen = 0;
    int wanderBudget = 0;
    int activeTurnLimit = MAX_TURN_LIMIT;

    struct BeamNode {
        SnakeState st;
        int pathTail = -1;
        int pathLen = 0;
        int maxPrefix = 0;
        long long eval = (long long)4e18;
    };

    Solver() {
        uint64_t seed = (uint64_t)chrono::steady_clock::now().time_since_epoch().count();
        seed ^= (uint64_t)(uintptr_t)this;
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
        for (int i = 0; i < M; ++i) {
            int x; cin >> x;
            desired[i] = (int8_t)x;
        }

        state.N = (int8_t)N;
        memset(state.grid, 0, sizeof(state.grid));
        state.foodCount = 0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int x; cin >> x;
                state.grid[i][j] = (int8_t)x;
                if (x != 0) ++state.foodCount;
            }
        }

        state.bodyHead = 0;
        state.bodyLen = 5;
        state.bodyBuf[0] = packPos(4, 0);
        state.bodyBuf[1] = packPos(3, 0);
        state.bodyBuf[2] = packPos(2, 0);
        state.bodyBuf[3] = packPos(1, 0);
        state.bodyBuf[4] = packPos(0, 0);

        state.colorHead = 0;
        state.colorLen = 5;
        for (int i = 0; i < 5; ++i) state.colorBuf[i] = 1;

        state.turn = 0;
        movesDirs.clear();
        recoveryHead = 0;
        recoveryLen = 0;
        wanderBudget = 0;
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

    bool executeDir(int d) {
        int prefixBefore = 0;
        SnakeState before;
        bool needBackup = (state.bodyLen >= 3);
        if (needBackup) {
            before = state;
            prefixBefore = before.prefixLen(desired, M);
        }
        MoveResult ret = state.apply(d);
        if (!ret.ok) return false;
        movesDirs.push_back((int8_t)d);
        if (ret.bite && needBackup) {
            auto recovery = buildRecoveryPathByOldBody(before, state, prefixBefore);
            recoveryHead = 0;
            recoveryLen = recovery.len;
            memcpy(recoveryBuf, recovery.d, recovery.len);
        }
        return true;
    }

    int manhattanToNearestTarget(const SnakeState& st, int targetColor) const {
        if (st.bodyLen == 0) return INT_MAX;
        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        int best = INT_MAX;
        for (int i = 0; i < st.N; ++i) {
            for (int j = 0; j < st.N; ++j) {
                if (st.grid[i][j] == targetColor) {
                    best = min(best, abs(i - hr) + abs(j - hc));
                }
            }
        }
        return best;
    }

    // Bucket queue BFS structures (reusable, stack-allocated per call)
    static constexpr int BQ_MAX_DIST = MAXCELLS + 1;

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
            release[r][c] = max(release[r][c], (int)st.bodyLen - i);
        }

        int dist[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        memset(dist, 0x3f, sizeof(dist));
        memset(pdir, -1, sizeof(pdir));

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        pair<int8_t,int8_t> buckets_arr[BQ_MAX_DIST][MAXCELLS];
        int bucket_sz[BQ_MAX_DIST] = {};
        dist[hr][hc] = 0;
        buckets_arr[0][0] = {(int8_t)hr, (int8_t)hc};
        bucket_sz[0] = 1;

        for (int t = 0; t < BQ_MAX_DIST; ++t) {
            for (int bi = 0; bi < bucket_sz[t]; ++bi) {
                auto [r, c] = buckets_arr[t][bi];
                if (dist[r][c] != t) continue;
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
                        if (nt < BQ_MAX_DIST) {
                            buckets_arr[nt][bucket_sz[nt]++] = {(int8_t)nr, (int8_t)nc};
                        }
                    }
                }
            }
        }

        struct Goal { int dist, r, c; };
        Goal goals[MAXCELLS];
        int gn = 0;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == hr && j == hc) continue;
                if (st.grid[i][j] != targetColor) continue;
                if (dist[i][j] >= INF) continue;
                goals[gn++] = {dist[i][j], i, j};
            }
        }
        sort(goals, goals + gn, [](const Goal& a, const Goal& b) { return a.dist < b.dist; });
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

    FixedPath findDynamicStrictPathToTarget(const SnakeState& st, int targetColor) const {
        FixedPath out[1];
        int cnt = findDynamicStrictPathsToTarget(st, targetColor, 1, out);
        return cnt > 0 ? out[0] : FixedPath{};
    }

    FixedPath findDynamicStrictPathToTail(const SnakeState& st, int targetColor) const {
        FixedPath result;
        if (st.bodyLen == 0) return result;
        const int n = st.N;
        const int INF = 0x3f3f3f3f;

        int release[MAXN][MAXN] = {};
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            release[posR(bp)][posC(bp)] = max(release[posR(bp)][posC(bp)], (int)st.bodyLen - i);
        }

        uint8_t tail = st.bodyBack();
        int gr = posR(tail), gc = posC(tail);

        int dist[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        memset(dist, 0x3f, sizeof(dist));
        memset(pdir, -1, sizeof(pdir));

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        // Bucket queue
        pair<int8_t,int8_t> buckets_arr[BQ_MAX_DIST][MAXCELLS];
        int bucket_sz[BQ_MAX_DIST] = {};
        dist[hr][hc] = 0;
        buckets_arr[0][0] = {(int8_t)hr, (int8_t)hc};
        bucket_sz[0] = 1;

        for (int t = 0; t < BQ_MAX_DIST; ++t) {
            for (int bi = 0; bi < bucket_sz[t]; ++bi) {
                auto [r, c] = buckets_arr[t][bi];
                if (dist[r][c] != t) continue;
                if (!(r == hr && c == hc) && r == gr && c == gc) goto found_tail;
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
                        if (nt < BQ_MAX_DIST) {
                            buckets_arr[nt][bucket_sz[nt]++] = {(int8_t)nr, (int8_t)nc};
                        }
                    }
                }
            }
        }
        found_tail:

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

    FixedPath findPenalizedPathToTarget(const SnakeState& st, int targetColor, int foodPenalty) const {
        FixedPath result;
        if (st.bodyLen == 0) return result;
        const int n = st.N;
        const int INF = 0x3f3f3f3f;

        int release[MAXN][MAXN] = {};
        for (int i = 1; i < st.bodyLen; ++i) {
            uint8_t bp = st.bodyAt(i);
            release[posR(bp)][posC(bp)] = max(release[posR(bp)][posC(bp)], (int)st.bodyLen - i);
        }

        int dist[MAXN][MAXN];
        uint8_t parentPacked[MAXN][MAXN];
        int8_t pdir[MAXN][MAXN];
        memset(dist, 0x3f, sizeof(dist));
        memset(pdir, -1, sizeof(pdir));

        uint8_t head = st.bodyFront();
        int hr = posR(head), hc = posC(head);
        uint8_t neck = 255;
        if (st.bodyLen >= 2) neck = st.bodyAt(1);

        using Node = tuple<int, int, int>;
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        dist[hr][hc] = 0;
        pq.push({0, hr, hc});

        int goalR = -1, goalC = -1;
        while (!pq.empty()) {
            auto [t, r, c] = pq.top(); pq.pop();
            if (t != dist[r][c]) continue;
            if (!(r == hr && c == hc) && st.grid[r][c] == targetColor) {
                goalR = r; goalC = c; break;
            }
            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d], nc = c + DC[d];
                if ((unsigned)nr >= (unsigned)n || (unsigned)nc >= (unsigned)n) continue;
                if (r == hr && c == hc && packPos(nr, nc) == neck) continue;
                int nt = t + 1;
                if (nt < release[nr][nc]) continue;
                int food = st.grid[nr][nc];
                int ncost = nt;
                if (food != 0 && food != targetColor) ncost += foodPenalty;
                if (ncost < dist[nr][nc]) {
                    dist[nr][nc] = ncost;
                    parentPacked[nr][nc] = packPos(r, c);
                    pdir[nr][nc] = (int8_t)d;
                    pq.push({ncost, nr, nc});
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
        memset(pdir, -1, sizeof(pdir));

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
                    auto lp = bfsToTargetColor(nxt, targetColor, false, true);
                    cand.directPathLen = dp.empty() ? INT_MAX : dp.size();
                    cand.loosePathLen = lp.empty() ? INT_MAX : lp.size();
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
        int8_t seq[CUT_ENUM_MAX_DEPTH];
        enumerateCutCandidatesDFS(src, 0, maxDepth, baseL, targetColor, seq, 0, out);
        return out;
    }

    optional<CutCandidate> chooseBestCutCandidate(const SnakeState& src, int baseL, int targetColor) {
        auto cands = enumerateCutCandidates(src, baseL, targetColor, CUT_ENUM_MAX_DEPTH);
        if (cands.empty()) return nullopt;

        int curLen = src.bodyLen;
        int floorHalf = max(1, curLen / 2);
        int halfOdd = floorHalf;
        if (halfOdd % 2 == 0) --halfOdd;
        halfOdd = max(CUT_KEEP_MIN_LEN, halfOdd);

        auto cutLenCost = [&](const CutCandidate& c) {
            int la = c.after.bodyLen;
            int cost = abs(la - halfOdd) * CUT_LEN_DEVIATION_WEIGHT;
            if (la > floorHalf) cost += (la - floorHalf) * CUT_LEN_OVER_HALF_WEIGHT;
            return cost;
        };

        int bestLC = INT_MAX;
        for (auto& c : cands) bestLC = min(bestLC, cutLenCost(c));

        auto eval = [&](const CutCandidate& c) -> long long {
            if (!(c.adjacentToPrefix || c.directPathLen != INT_MAX || c.loosePathLen != INT_MAX)) return (long long)4e18;
            long long s = CUT_EVAL_BITE_TURN_WEIGHT * c.biteTurn;
            if (c.adjacentToPrefix) s -= CUT_EVAL_ADJ_BONUS;
            s += (c.directPathLen == INT_MAX ? CUT_EVAL_DIRECT_UNREACHABLE : c.directPathLen * CUT_EVAL_DIRECT_DISTANCE_WEIGHT);
            s += (c.loosePathLen == INT_MAX ? CUT_EVAL_LOOSE_UNREACHABLE : c.loosePathLen * CUT_EVAL_LOOSE_DISTANCE_WEIGHT);
            s -= CUT_EVAL_PREFIX_AFTER_WEIGHT * c.prefixAfter;
            return s;
        };

        auto pick = [&](bool reqAdj) -> int {
            long long bs = (long long)4e18; int bi = -1;
            for (int i = 0; i < (int)cands.size(); ++i) {
                if (cutLenCost(cands[i]) != bestLC) continue;
                if (reqAdj && !cands[i].adjacentToPrefix) continue;
                long long s = eval(cands[i]);
                if (s < bs) { bs = s; bi = i; }
            }
            return bi;
        };

        int bi = pick(true);
        if (bi == -1) bi = pick(false);
        if (bi == -1) return nullopt;
        return cands[bi];
    }

    optional<CutCandidate> chooseRandomHalfCutCandidate(const SnakeState& src, int baseL, int targetColor) {
        auto cands = enumerateCutCandidates(src, baseL, targetColor, CUT_ENUM_MAX_DEPTH);
        if (cands.empty()) return nullopt;
        int curLen = src.bodyLen, floorHalf = max(1, curLen / 2);
        int halfOdd = floorHalf; if (halfOdd % 2 == 0) --halfOdd; halfOdd = max(CUT_KEEP_MIN_LEN, halfOdd);
        auto clc = [&](const CutCandidate& c) {
            int la = c.after.bodyLen;
            int cost = abs(la - halfOdd) * CUT_LEN_DEVIATION_WEIGHT;
            if (la > floorHalf) cost += (la - floorHalf) * CUT_LEN_OVER_HALF_WEIGHT;
            return cost;
        };
        int bestLC = INT_MAX;
        for (auto& c : cands) bestLC = min(bestLC, clc(c));
        vector<int> idxs;
        for (int i = 0; i < (int)cands.size(); ++i) if (clc(cands[i]) == bestLC) idxs.push_back(i);
        if (idxs.empty()) return nullopt;
        return cands[idxs[randRange(0, (int)idxs.size() - 1)]];
    }

    vector<CutCandidate> chooseTopCutCandidatesForBeam(
        const SnakeState& src, int baseL, int targetColor, int topK
    ) const {
        topK = max(1, topK);
        auto cands = enumerateCutCandidates(src, baseL, targetColor, CUT_ENUM_MAX_DEPTH);
        if (cands.empty()) return {};
        int wrongSuffix = max(0, (int)src.bodyLen - baseL);
        struct R { long long s; int i; };
        vector<R> ranked; ranked.reserve(cands.size());
        for (int i = 0; i < (int)cands.size(); ++i) {
            auto& c = cands[i];
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
            ranked.push_back({s, i});
        }
        sort(ranked.begin(), ranked.end(), [](const R& a, const R& b) { return a.s != b.s ? a.s < b.s : a.i < b.i; });
        vector<CutCandidate> out;
        int take = min(topK, (int)ranked.size());
        for (int i = 0; i < take; ++i) out.push_back(cands[ranked[i].i]);
        return out;
    }

    long long evaluateBeamState(const SnakeState& st, int maxPrefix) const {
        int k = st.bodyLen, pref = min(maxPrefix, k), E = k - pref;
        return st.turn + SCORE_MISMATCH_WEIGHT * (E + SCORE_MISSING_LENGTH_FACTOR * (M - k));
    }

    uint64_t beamStateKey(const SnakeState& st, int maxPrefix) const {
        uint64_t h = 1469598103934665603ULL;
        auto mix = [&](uint64_t x) { h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); };
        mix((uint64_t)st.turn);
        mix((uint64_t)st.bodyLen);
        mix((uint64_t)min(maxPrefix, M));
        const int8_t* gp = &st.grid[0][0];
        int gs = st.N * st.N;
        for (int i = 0; i < gs; i += 8) {
            uint64_t chunk = 0;
            memcpy(&chunk, gp + i, min(8, gs - i));
            mix(chunk);
        }
        // Batch body+color: interleave for better cache behavior
        for (int i = 0; i < st.bodyLen; ++i) {
            mix(((uint64_t)st.bodyAt(i) << 8) | (uint64_t)(uint8_t)st.colorAt(i));
        }
        return h;
    }

    bool betterBeamNode(const BeamNode& a, const BeamNode& b) const {
        if (a.eval != b.eval) return a.eval < b.eval;
        if (a.maxPrefix != b.maxPrefix) return a.maxPrefix > b.maxPrefix;
        return a.st.turn < b.st.turn;
    }

    int pickWanderStepForState(const SnakeState& st, int targetColor, int baseL) {
        DirList dirs = st.legalDirs();
        if (dirs.n == 0) return -1;
        struct SM { double s; int d; };
        SM sc[4]; int sn = 0;
        for (int di = 0; di < dirs.n; ++di) {
            int d = dirs.d[di];
            SnakeState nxt = st;
            MoveResult ret = nxt.apply(d);
            if (!ret.ok) continue;
            uint8_t head = st.bodyFront();
            int nr = posR(head) + DR[d], nc = posC(head) + DC[d];
            int food = st.grid[nr][nc];
            double s = randDouble() * WANDER_RAND_WEIGHT + randDouble() * WANDER_RAND_WEIGHT +
                       (food == 0 ? WANDER_EMPTY_SCORE : WANDER_FOOD_SCORE) +
                       nxt.legalDirs().n * WANDER_MOBILITY_WEIGHT;
            if (food == targetColor) s += WANDER_TARGET_FOOD_BONUS;
            if (ret.bite) s -= WANDER_BITE_PENALTY;
            if (nxt.prefixLen(desired, M) < baseL) s -= WANDER_PREFIX_BREAK_PENALTY;
            int nt = manhattanToNearestTarget(nxt, targetColor);
            if (nt != INT_MAX) s -= nt * WANDER_TARGET_DISTANCE_WEIGHT;
            sc[sn++] = {s, d};
        }
        if (sn == 0) return dirs.d[0];
        sort(sc, sc + sn, [](const SM& a, const SM& b) { return a.s > b.s; });
        return sc[randRange(0, min(WANDER_CHOICE_TOP_K, sn) - 1)].d;
    }

    void transitionGoTargetCandidates(
        const BeamNode& cur, chrono::steady_clock::time_point dl, int topK,
        BeamNode* out, int& outN
    ) {
        outN = 0;
        if (chrono::steady_clock::now() >= dl) return;
        int p = cur.st.prefixLen(desired, M);
        if (p >= M || p < cur.st.colorLen) return;
        int tc = desired[p];

        FixedPath paths[8];
        int pathCount = 0;

        auto tryPush = [&](const FixedPath& path) {
            if (path.empty()) return;
            for (int i = 0; i < pathCount; ++i) if (paths[i] == path) return;
            if (pathCount < 8) paths[pathCount++] = path;
        };
        tryPush(findDynamicStrictPathToTarget(cur.st, tc));
        if (pathCount < topK) {
            FixedPath fp[4];
            int fpn = findDynamicStrictPathsToTarget(cur.st, tc, topK, fp);
            for (int i = 0; i < fpn; ++i) { tryPush(fp[i]); if (pathCount >= topK) break; }
        }
        if (pathCount == 0) return;
        if (pathCount > topK) pathCount = topK;

        for (int pi = 0; pi < pathCount; ++pi) {
            if (chrono::steady_clock::now() >= dl) break;
            const FixedPath& path = paths[pi];
            BeamNode nxt;
            nxt.st = cur.st;
            nxt.pathTail = cur.pathTail;
            nxt.pathLen = cur.pathLen;
            bool ok = true;
            for (int i = 0; i < path.len; ++i) {
                if (chrono::steady_clock::now() >= dl) { ok = false; break; }
                if (nxt.st.turn >= activeTurnLimit) break;
                MoveResult ret = nxt.st.apply(path.d[i]);
                if (!ret.ok) { ok = false; break; }
                nxt.pathTail = gPool.add(path.d[i], nxt.pathTail);
                nxt.pathLen++;
            }
            if (!ok) continue;
            int np = nxt.st.prefixLen(desired, M);
            nxt.maxPrefix = max(cur.maxPrefix, np);
            nxt.eval = evaluateBeamState(nxt.st, nxt.maxPrefix);
            out[outN++] = move(nxt);
        }
    }

    void transitionCutRecoverCandidates(
        const BeamNode& cur, chrono::steady_clock::time_point dl, int topK,
        BeamNode* out, int& outN
    ) {
        outN = 0;
        if (chrono::steady_clock::now() >= dl) return;
        int p = cur.st.prefixLen(desired, M);
        if (p >= M) return;
        auto cuts = chooseTopCutCandidatesForBeam(cur.st, p, desired[p], topK);
        if (cuts.empty()) return;

        for (auto& cut : cuts) {
            if (chrono::steady_clock::now() >= dl) break;
            BeamNode nxt;
            nxt.st = cur.st;
            nxt.pathTail = cur.pathTail;
            nxt.pathLen = cur.pathLen;
            bool bitten = false, ok = true;
            for (int si = 0; si < cut.seqLen; ++si) {
                if (chrono::steady_clock::now() >= dl) { ok = false; break; }
                if (nxt.st.turn >= activeTurnLimit) break;
                SnakeState bef = nxt.st;
                int pb = bef.prefixLen(desired, M);
                MoveResult ret = nxt.st.apply(cut.seq[si]);
                if (!ret.ok) { ok = false; break; }
                nxt.pathTail = gPool.add(cut.seq[si], nxt.pathTail);
                nxt.pathLen++;
                if (ret.bite) {
                    bitten = true;
                    auto recovery = buildRecoveryPathByOldBody(bef, nxt.st, pb);
                    for (int ri = 0; ri < recovery.len; ++ri) {
                        if (chrono::steady_clock::now() >= dl) { ok = false; break; }
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
            nxt.maxPrefix = max(cur.maxPrefix, np);
            nxt.eval = evaluateBeamState(nxt.st, nxt.maxPrefix);
            out[outN++] = move(nxt);
        }
    }

    bool transitionWander(const BeamNode& cur, chrono::steady_clock::time_point dl, BeamNode& out) {
        if (chrono::steady_clock::now() >= dl) return false;
        int p = cur.st.prefixLen(desired, M);
        if (p >= M) return false;
        int tc = desired[p];
        out.st = cur.st;
        out.pathTail = cur.pathTail;
        out.pathLen = cur.pathLen;
        int moved = 0;
        for (int t = 0; t < WANDER_LEN; ++t) {
            if (chrono::steady_clock::now() >= dl || out.st.turn >= activeTurnLimit) break;
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
        out.maxPrefix = max(cur.maxPrefix, np);
        out.eval = evaluateBeamState(out.st, out.maxPrefix);
        return true;
    }

    bool isDoneNode(const BeamNode& node) const { return node.st.prefixLen(desired, M) >= M; }
    bool betterFinalNode(const BeamNode& a, const BeamNode& b) const {
        bool ad = isDoneNode(a), bd = isDoneNode(b);
        if (ad != bd) return ad;
        return betterBeamNode(a, b);
    }

    BeamNode runLegacyFallbackPolicy(const SnakeState& initial, chrono::steady_clock::time_point deadline) {
        state = initial;
        movesDirs.clear();
        recoveryHead = 0;
        recoveryLen = 0;
        wanderBudget = 0;

        int bestPrefix = state.prefixLen(desired, M);
        int stagnation = 0;
        auto execCut = [&](const int8_t* seq, int len) -> bool {
            for (int i = 0; i < len; ++i) {
                if (state.turn >= activeTurnLimit || chrono::steady_clock::now() >= deadline) break;
                if (!executeDir(seq[i])) return false;
            }
            return true;
        };

        while (state.turn < activeTurnLimit && chrono::steady_clock::now() < deadline) {
            int p = state.prefixLen(desired, M);
            if (p >= M) break;
            bool prefBroken = p < state.colorLen;

            if (recoveryLen > 0) {
                int d = recoveryBuf[recoveryHead];
                ++recoveryHead;
                --recoveryLen;
                if (!executeDir(d)) { recoveryLen = 0; }
                continue;
            }

            if (p > bestPrefix) { bestPrefix = p; stagnation = 0; wanderBudget = 0; }
            else ++stagnation;

            int tc = desired[p];
            int rem = M - p;
            bool nearFin = rem <= LEGACY_NEAR_FINISH_REMAINING;
            bool empty = state.foodCount == 0;

            if (empty) {
                auto co = chooseBestCutCandidate(state, p, tc);
                if (!co) co = chooseRandomHalfCutCandidate(state, p, tc);
                if (co) { if (!execCut(co->seq, co->seqLen)) break; stagnation = 0; wanderBudget = 0; continue; }
                DirList dirs = state.legalDirs();
                if (dirs.n == 0) break;
                if (!executeDir(dirs.d[0])) break;
                continue;
            }

            if (!prefBroken) {
                auto direct = findDynamicStrictPathToTarget(state, tc);
                if (!direct.empty()) { if (!executeDir(direct.front())) break; continue; }
                auto tail = findDynamicStrictPathToTail(state, tc);
                if (!tail.empty() && (nearFin || stagnation < LEGACY_TAIL_PATH_STAGNATION_LIMIT)) { if (!executeDir(tail.front())) break; continue; }
            }

            if (wanderBudget == 0 && (stagnation >= LEGACY_WANDER_TRIGGER_STAGNATION || prefBroken)) {
                wanderBudget = LEGACY_WANDER_BUDGET;
            }
            if (wanderBudget > 0) {
                int d = prefBroken ? pickExploreMove(tc, p, true, false) : pickWanderStepForState(state, tc, p);
                if (d == -1) { DirList dirs = state.legalDirs(); if (dirs.n == 0) break; d = dirs.d[0]; }
                if (!executeDir(d)) break;
                --wanderBudget;
                continue;
            }

            int ct = nearFin ? LEGACY_CUT_THRESHOLD_NEAR_FIN : LEGACY_CUT_THRESHOLD_DEFAULT;
            if (prefBroken) ct = min(ct, LEGACY_CUT_THRESHOLD_PREF_BROKEN_CAP);
            if (stagnation >= ct) {
                auto co = chooseBestCutCandidate(state, p, tc);
                if (co) {
                    if (!execCut(co->seq, co->seqLen)) break;
                    stagnation = max(0, stagnation - LEGACY_CUT_STAGNATION_REDUCE);
                    continue;
                }
            }

            if (!prefBroken && stagnation >= LEGACY_PENALIZED_TRIGGER_STAGNATION) {
                auto pen = findPenalizedPathToTarget(state, tc, N * LEGACY_PENALIZED_FOOD_PENALTY_FACTOR);
                if (!pen.empty()) { if (!executeDir(pen.front())) break; continue; }
            }

            int fb = pickExploreMove(tc, p, true, !prefBroken);
            if (fb == -1) { DirList dirs = state.legalDirs(); if (dirs.n == 0) break; fb = dirs.d[0]; }
            if (!executeDir(fb)) {
                DirList dirs = state.legalDirs();
                if (dirs.n == 0) break;
                if (!executeDir(dirs.d[0])) break;
            }
        }

        BeamNode out;
        out.st = state;
        out.maxPrefix = max(bestPrefix, state.prefixLen(desired, M));
        out.eval = evaluateBeamState(out.st, out.maxPrefix);
        out.pathTail = -1;
        out.pathLen = (int)movesDirs.size();
        return out;
    }

    optional<BeamNode> runBeamSearch(
        const SnakeState& initial, chrono::steady_clock::time_point deadline,
        int beamWidth, int beamTurnCap
    ) {
        BeamNode init;
        init.st = initial;
        init.maxPrefix = init.st.prefixLen(desired, M);
        init.eval = evaluateBeamState(init.st, init.maxPrefix);
        init.pathTail = -1;
        init.pathLen = 0;
        if (init.st.turn > beamTurnCap) return nullopt;

        int startTurn = init.st.turn;
        vector<vector<BeamNode>> layers(beamTurnCap + 1);
        vector<unordered_map<uint64_t, int>> lki(beamTurnCap + 1);

        auto ins = [&](BeamNode&& cand) {
            int t = cand.st.turn;
            if (t < startTurn || t > beamTurnCap) return;
            uint64_t key = beamStateKey(cand.st, cand.maxPrefix);
            auto& mp = lki[t]; auto& vec = layers[t];
            auto it = mp.find(key);
            if (it == mp.end()) { mp[key] = (int)vec.size(); vec.push_back(move(cand)); }
            else { if (betterBeamNode(cand, vec[it->second])) vec[it->second] = move(cand); }
        };

        layers[startTurn].push_back(init);
        lki[startTurn][beamStateKey(init.st, init.maxPrefix)] = 0;

        bool hasDone = false; BeamNode bestDone, bestSeen = init;

        for (int turn = startTurn; turn <= beamTurnCap; ++turn) {
            if (chrono::steady_clock::now() >= deadline) break;
            auto& beam = layers[turn];
            if (beam.empty()) continue;
            sort(beam.begin(), beam.end(), [&](const BeamNode& a, const BeamNode& b) { return betterBeamNode(a, b); });
            if ((int)beam.size() > beamWidth) beam.resize(beamWidth);

            for (auto& node : beam) {
                if (chrono::steady_clock::now() >= deadline) break;
                if (betterBeamNode(node, bestSeen)) bestSeen = node;
                int p = node.st.prefixLen(desired, M);
                if (p >= M) { if (!hasDone || betterBeamNode(node, bestDone)) { hasDone = true; bestDone = node; } continue; }
                if (node.st.turn >= beamTurnCap) continue;

                BeamNode cands[8]; int cn;
                transitionGoTargetCandidates(node, deadline, BEAM_CANDIDATE_TOP_K, cands, cn);
                for (int i = 0; i < cn; ++i) ins(move(cands[i]));
                if (chrono::steady_clock::now() >= deadline) break;
                transitionCutRecoverCandidates(node, deadline, BEAM_CUT_CANDIDATE_TOP_K, cands, cn);
                for (int i = 0; i < cn; ++i) ins(move(cands[i]));
                if (chrono::steady_clock::now() >= deadline) break;
                BeamNode wn;
                if (transitionWander(node, deadline, wn)) ins(move(wn));
            }
            if (hasDone) break;
            lki[turn].clear();
        }
        return hasDone ? optional(bestDone) : optional(bestSeen);
    }

    int pickExploreMove(int tc, int baseL, bool randomized, bool preferTC = true) {
        DirList dirs = state.legalDirs();
        if (dirs.n == 0) return -1;
        if (randomized && randDouble() < EXPLORE_RANDOM_MOVE_PROB) return dirs.d[randRange(0, dirs.n - 1)];

        struct SM { double s; int d; };
        SM sc[4]; int sn = 0;
        for (int di = 0; di < dirs.n; ++di) {
            int d = dirs.d[di];
            uint8_t head = state.bodyFront();
            int nr = posR(head) + DR[d], nc = posC(head) + DC[d];
            int lf = state.grid[nr][nc];
            SnakeState nxt = state;
            MoveResult ret = nxt.apply(d);
            if (!ret.ok) continue;
            int pref = nxt.prefixLen(desired, M);
            int nt = manhattanToNearestTarget(nxt, tc);
            int mob = nxt.legalDirs().n;
            double s = 0;
            if (preferTC) {
                if (lf == 0) s += EXPLORE_LAND_EMPTY_SCORE;
                else if (lf == tc) s += EXPLORE_LAND_TARGET_SCORE;
                else s -= EXPLORE_LAND_OTHER_PENALTY;
            } else {
                s += (lf == 0 ? EXPLORE_NON_TARGET_EMPTY_SCORE : EXPLORE_NON_TARGET_FOOD_SCORE);
            }
            if (ret.bite) s += EXPLORE_BITE_BONUS;
            if (pref < baseL) s -= EXPLORE_PREFIX_BREAK_PENALTY;
            s += mob * EXPLORE_MOBILITY_WEIGHT;
            if (preferTC && nt != INT_MAX) s -= nt * EXPLORE_TARGET_DISTANCE_WEIGHT;
            if (randomized) s += randDouble();
            sc[sn++] = {s, d};
        }
        if (sn == 0) return dirs.d[0];
        sort(sc, sc + sn, [](const SM& a, const SM& b) { return a.s > b.s; });
        if (!randomized) return sc[0].d;
        int topK = min(EXPLORE_CHOICE_TOP_K, sn);
        if (randDouble() < EXPLORE_TOP_CHOICE_PROB) return sc[randRange(0, topK - 1)].d;
        return sc[randRange(0, sn - 1)].d;
    }

    int greedyTurnStep(int cap) const {
        return cap < GREEDY_STEP_BOUND_SMALL ? GREEDY_STEP_SMALL
             : cap < GREEDY_STEP_BOUND_MIDDLE ? GREEDY_STEP_MIDDLE
                                              : GREEDY_STEP_LARGE;
    }

    void solve() {
        auto start = chrono::steady_clock::now();
        auto totalDL = start + chrono::milliseconds(SEARCH_TIME_LIMIT_MS);
        SnakeState initial = state;
        BeamNode greedy; bool hasGreedy = false; int selTL = INITIAL_GREEDY_TURN_LIMIT;
        vector<int8_t> greedyPath;

        for (int cap = INITIAL_GREEDY_TURN_LIMIT; cap <= MAX_TURN_LIMIT;) {
            if (chrono::steady_clock::now() >= totalDL) break;
            activeTurnLimit = cap;
            BeamNode cand = runLegacyFallbackPolicy(initial, totalDL);
            selTL = cap;
            if (!hasGreedy || betterFinalNode(cand, greedy)) {
                greedy = cand;
                greedyPath = movesDirs; // save the greedy path
                hasGreedy = true;
            }
            if (isDoneNode(cand)) { greedy = move(cand); greedyPath = movesDirs; break; }
            int step = greedyTurnStep(cap);
            if (cap > MAX_TURN_LIMIT - step) break;
            cap += step;
        }

        if (!hasGreedy) {
            activeTurnLimit = INITIAL_GREEDY_TURN_LIMIT;
            greedy.st = initial;
            greedy.pathTail = -1;
            greedy.pathLen = 0;
            greedy.maxPrefix = initial.prefixLen(desired, M);
            greedy.eval = evaluateBeamState(greedy.st, greedy.maxPrefix);
            selTL = activeTurnLimit;
            greedyPath.clear();
        } else { activeTurnLimit = selTL; greedy.eval = evaluateBeamState(greedy.st, greedy.maxPrefix); }

        int btc = min({activeTurnLimit, (int)greedyPath.size(), BEAM_TURN_CAP_MAX});
        BeamNode best = greedy;
        vector<int8_t> bestPath = greedyPath;
        int bw = BEAM_WIDTH;

        while (chrono::steady_clock::now() < totalDL) {
            gPool.clear();
            auto bb = runBeamSearch(initial, totalDL, bw, btc);
            if (bb && betterFinalNode(*bb, best)) {
                best = move(*bb);
                // Extract path from pool
                bestPath.resize(best.pathLen);
                if (best.pathTail >= 0 && best.pathLen > 0) {
                    gPool.extract(best.pathTail, best.pathLen, bestPath.data());
                }
            }
            if (chrono::steady_clock::now() >= totalDL || bw >= BEAM_WIDTH_MAX) break;
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
