#include <bits/stdc++.h>
using namespace std;

namespace {

constexpr int DR[4] = {-1, 1, 0, 0};
constexpr int DC[4] = {0, 0, -1, 1};
constexpr char DCHAR[4] = {'U', 'D', 'L', 'R'};
constexpr int TURN_LIMIT = 100000;
constexpr int RANDOM_SEARCH_LIMIT = 100000;
constexpr int BEAM_WIDTH = 20;
constexpr int WANDER_LEN = 2;
constexpr int BEAM_TIME_LIMIT_MS = 1900;
constexpr int BEAM_PHASE_LIMIT_MS = 1600;
constexpr int BEAM_CANDIDATE_TOP_K = 2;

struct MoveResult {
    bool ok = false;
    bool ate = false;
    bool bite = false;
    int eatenColor = 0;
};

struct SnakeState {
    int N = 0;
    vector<vector<int>> grid;
    deque<pair<int, int>> body;   // head=0
    deque<int> colors;            // logical index colors
    int turn = 0;

    bool inBounds(int r, int c) const {
        return 0 <= r && r < N && 0 <= c && c < N;
    }

    int prefixLen(const vector<int>& desired) const {
        int lim = min((int)colors.size(), (int)desired.size());
        int p = 0;
        while (p < lim && colors[p] == desired[p]) {
            ++p;
        }
        return p;
    }

    vector<int> legalDirs() const {
        vector<int> dirs;
        if (body.empty()) {
            return dirs;
        }
        auto [hr, hc] = body.front();
        pair<int, int> neck = {-1, -1};
        if (body.size() >= 2) {
            neck = body[1];
        }
        for (int d = 0; d < 4; ++d) {
            int nr = hr + DR[d];
            int nc = hc + DC[d];
            if (!inBounds(nr, nc)) {
                continue;
            }
            if (make_pair(nr, nc) == neck) {
                continue;
            }
            dirs.push_back(d);
        }
        return dirs;
    }

    MoveResult apply(int d) {
        MoveResult ret;
        if (body.empty()) {
            return ret;
        }

        auto [hr, hc] = body.front();
        int nr = hr + DR[d];
        int nc = hc + DC[d];

        if (!inBounds(nr, nc)) {
            return ret;
        }
        if (body.size() >= 2 && make_pair(nr, nc) == body[1]) {
            return ret;
        }

        body.push_front({nr, nc});

        if (grid[nr][nc] != 0) {
            ret.ate = true;
            ret.eatenColor = grid[nr][nc];
            grid[nr][nc] = 0;
            colors.push_back(ret.eatenColor);
        } else {
            body.pop_back();
        }

        int k = (int)body.size();
        for (int h = 1; h <= k - 2; ++h) {
            if (body[h] == make_pair(nr, nc)) {
                for (int p = h + 1; p < (int)body.size(); ++p) {
                    auto [ri, ci] = body[p];
                    grid[ri][ci] = colors[p];
                }
                body.resize(h + 1);
                colors.resize(h + 1);
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
    vector<int> seq;      // 1..3 moves, bite at end
    SnakeState after;
    int biteTurn = 0;
    int prefixAfter = 0;
    int anchorLen = 0;
    bool adjacentToPrefix = false;
    int directPathLen = INT_MAX;  // avoid body, avoid non-target foods
    int loosePathLen = INT_MAX;   // allow body, avoid non-target foods
};

struct Solver {
    int N = 0, M = 0, C = 0;
    vector<int> desired;

    SnakeState state;
    string moves;
    mt19937 rng;
    deque<int> recoveryMoves;
    int recoveryTargetPrefix = 0;
    vector<vector<int>> visitCount;
    int wanderBudget = 0;

    struct BeamNode {
        SnakeState st;
        vector<int> path;
        int maxPrefix = 0;
        long long eval = (long long)4e18;
    };

    Solver() {
        uint64_t seed = (uint64_t)chrono::steady_clock::now().time_since_epoch().count();
        seed ^= (uint64_t)(uintptr_t)this;
        rng.seed((uint32_t)(seed ^ (seed >> 32)));
    }

    void readInput() {
        ios::sync_with_stdio(false);
        cin.tie(nullptr);

        cin >> N >> M >> C;
        desired.resize(M);
        for (int i = 0; i < M; ++i) {
            cin >> desired[i];
        }

        state.N = N;
        state.grid.assign(N, vector<int>(N, 0));
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                cin >> state.grid[i][j];
            }
        }

        state.body = {{4, 0}, {3, 0}, {2, 0}, {1, 0}, {0, 0}};
        state.colors = {1, 1, 1, 1, 1};
        state.turn = 0;
        moves.clear();
        recoveryMoves.clear();
        recoveryTargetPrefix = 0;
        visitCount.assign(N, vector<int>(N, 0));
        auto [sr, sc] = state.body.front();
        visitCount[sr][sc] = 1;
        wanderBudget = 0;
    }

    int dirBetween(pair<int, int> a, pair<int, int> b) const {
        for (int d = 0; d < 4; ++d) {
            if (a.first + DR[d] == b.first && a.second + DC[d] == b.second) {
                return d;
            }
        }
        return -1;
    }

    vector<int> buildRecoveryPathByOldBody(
        const SnakeState& beforeBite,
        const SnakeState& afterBite,
        int prefixBefore
    ) const {
        int lenAfter = (int)afterBite.body.size();
        if (prefixBefore <= lenAfter) {
            return {};
        }
        if (beforeBite.body.empty() || afterBite.body.empty()) {
            return {};
        }

        int startIdx = lenAfter - 1;
        int endIdx = prefixBefore - 2;
        if (startIdx < 0 || endIdx < startIdx || endIdx >= (int)beforeBite.body.size()) {
            return {};
        }

        vector<int> plan;
        auto cur = afterBite.body.front();
        for (int idx = startIdx; idx <= endIdx; ++idx) {
            auto nxt = beforeBite.body[idx];
            int d = dirBetween(cur, nxt);
            if (d < 0) {
                return {};
            }
            plan.push_back(d);
            cur = nxt;
        }
        return plan;
    }

    bool executeDir(int d) {
        SnakeState before = state;
        int prefixBefore = before.prefixLen(desired);
        MoveResult ret = state.apply(d);
        if (!ret.ok) {
            return false;
        }
        moves.push_back(DCHAR[d]);
        auto [hr, hc] = state.body.front();
        if (0 <= hr && hr < N && 0 <= hc && hc < N) {
            ++visitCount[hr][hc];
        }
        if (ret.bite) {
            auto recovery = buildRecoveryPathByOldBody(before, state, prefixBefore);
            recoveryMoves.clear();
            for (int mv : recovery) {
                recoveryMoves.push_back(mv);
            }
            recoveryTargetPrefix = prefixBefore;
        }
        return true;
    }

    int manhattanToNearestTarget(const SnakeState& st, int targetColor) const {
        if (st.body.empty()) {
            return INT_MAX;
        }
        auto [hr, hc] = st.body.front();
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

    int remainingFoodCount(const SnakeState& st) const {
        int cnt = 0;
        for (int i = 0; i < st.N; ++i) {
            for (int j = 0; j < st.N; ++j) {
                if (st.grid[i][j] != 0) {
                    ++cnt;
                }
            }
        }
        return cnt;
    }

    bool lastTwoSwappedAtPrefix(int p) const {
        if (p + 1 >= M) {
            return false;
        }
        if ((int)state.colors.size() <= p + 1) {
            return false;
        }
        return state.colors[p] == desired[p + 1] && state.colors[p + 1] == desired[p];
    }

    vector<vector<int>> findDynamicStrictPathsToTarget(
        const SnakeState& st,
        int targetColor,
        int topK
    ) const {
        if (st.body.empty()) {
            return {};
        }
        topK = max(1, topK);

        const int n = st.N;
        const int INF = 1e9;
        vector<vector<int>> release(n, vector<int>(n, 0));
        int k = (int)st.body.size();
        for (int i = 1; i < k; ++i) {
            auto [r, c] = st.body[i];
            release[r][c] = max(release[r][c], k - i);
        }

        vector<vector<int>> dist(n, vector<int>(n, INF));
        vector<vector<pair<int, int>>> parent(n, vector<pair<int, int>>(n, {-1, -1}));
        vector<vector<int>> pdir(n, vector<int>(n, -1));

        auto [hr, hc] = st.body.front();
        pair<int, int> neck = {-1, -1};
        if (st.body.size() >= 2) {
            neck = st.body[1];
        }

        using Node = tuple<int, int, int>;  // time, r, c
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        dist[hr][hc] = 0;
        pq.push({0, hr, hc});

        while (!pq.empty()) {
            auto [t, r, c] = pq.top();
            pq.pop();
            if (t != dist[r][c]) {
                continue;
            }

            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d];
                int nc = c + DC[d];
                if (!st.inBounds(nr, nc)) {
                    continue;
                }
                if (r == hr && c == hc && make_pair(nr, nc) == neck) {
                    continue;
                }
                int food = st.grid[nr][nc];
                if (food != 0 && food != targetColor) {
                    continue;
                }

                int nt = t + 1;
                if (nt < release[nr][nc]) {
                    continue;
                }
                if (nt < dist[nr][nc]) {
                    dist[nr][nc] = nt;
                    parent[nr][nc] = {r, c};
                    pdir[nr][nc] = d;
                    pq.push({nt, nr, nc});
                }
            }
        }

        vector<tuple<int, int, int>> goals;  // dist, r, c
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == hr && j == hc) {
                    continue;
                }
                if (st.grid[i][j] != targetColor) {
                    continue;
                }
                if (dist[i][j] >= INF) {
                    continue;
                }
                goals.push_back({dist[i][j], i, j});
            }
        }
        sort(goals.begin(), goals.end());
        if (goals.empty()) {
            return {};
        }

        int take = min(topK, (int)goals.size());
        vector<vector<int>> out;
        out.reserve(take);
        for (int gi = 0; gi < take; ++gi) {
            int gr = get<1>(goals[gi]);
            int gc = get<2>(goals[gi]);
            vector<int> path;
            int cr = gr;
            int cc = gc;
            while (!(cr == hr && cc == hc)) {
                path.push_back(pdir[cr][cc]);
                auto [pr, pc] = parent[cr][cc];
                cr = pr;
                cc = pc;
            }
            reverse(path.begin(), path.end());
            if (!path.empty()) {
                out.push_back(move(path));
            }
        }
        return out;
    }

    vector<int> findDynamicStrictPathToTarget(const SnakeState& st, int targetColor) const {
        auto paths = findDynamicStrictPathsToTarget(st, targetColor, 1);
        if (paths.empty()) {
            return {};
        }
        return paths[0];
    }

    vector<int> findDynamicStrictPathToTail(const SnakeState& st, int targetColor) const {
        if (st.body.empty()) {
            return {};
        }

        const int n = st.N;
        const int INF = 1e9;
        vector<vector<int>> release(n, vector<int>(n, 0));
        int k = (int)st.body.size();
        for (int i = 1; i < k; ++i) {
            auto [r, c] = st.body[i];
            release[r][c] = max(release[r][c], k - i);
        }

        auto goal = st.body.back();
        auto [gr, gc] = goal;

        vector<vector<int>> dist(n, vector<int>(n, INF));
        vector<vector<pair<int, int>>> parent(n, vector<pair<int, int>>(n, {-1, -1}));
        vector<vector<int>> pdir(n, vector<int>(n, -1));

        auto [hr, hc] = st.body.front();
        pair<int, int> neck = {-1, -1};
        if (st.body.size() >= 2) {
            neck = st.body[1];
        }

        using Node = tuple<int, int, int>;  // time, r, c
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        dist[hr][hc] = 0;
        pq.push({0, hr, hc});

        while (!pq.empty()) {
            auto [t, r, c] = pq.top();
            pq.pop();
            if (t != dist[r][c]) {
                continue;
            }
            if (!(r == hr && c == hc) && r == gr && c == gc) {
                break;
            }

            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d];
                int nc = c + DC[d];
                if (!st.inBounds(nr, nc)) {
                    continue;
                }
                if (r == hr && c == hc && make_pair(nr, nc) == neck) {
                    continue;
                }
                int food = st.grid[nr][nc];
                if (food != 0 && food != targetColor) {
                    continue;
                }

                int nt = t + 1;
                if (nt < release[nr][nc]) {
                    continue;
                }
                if (nt < dist[nr][nc]) {
                    dist[nr][nc] = nt;
                    parent[nr][nc] = {r, c};
                    pdir[nr][nc] = d;
                    pq.push({nt, nr, nc});
                }
            }
        }

        if (dist[gr][gc] == INF) {
            return {};
        }

        vector<int> path;
        int cr = gr;
        int cc = gc;
        while (!(cr == hr && cc == hc)) {
            path.push_back(pdir[cr][cc]);
            auto [pr, pc] = parent[cr][cc];
            cr = pr;
            cc = pc;
        }
        reverse(path.begin(), path.end());
        return path;
    }

    vector<int> findPenalizedPathToTarget(const SnakeState& st, int targetColor, int foodPenalty) const {
        if (st.body.empty()) {
            return {};
        }

        const int n = st.N;
        const int INF = 1e9;
        vector<vector<int>> release(n, vector<int>(n, 0));
        int k = (int)st.body.size();
        for (int i = 1; i < k; ++i) {
            auto [r, c] = st.body[i];
            release[r][c] = max(release[r][c], k - i);
        }

        vector<vector<int>> dist(n, vector<int>(n, INF));
        vector<vector<pair<int, int>>> parent(n, vector<pair<int, int>>(n, {-1, -1}));
        vector<vector<int>> pdir(n, vector<int>(n, -1));

        auto [hr, hc] = st.body.front();
        pair<int, int> neck = {-1, -1};
        if (st.body.size() >= 2) {
            neck = st.body[1];
        }

        using Node = tuple<int, int, int>;
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        dist[hr][hc] = 0;
        pq.push({0, hr, hc});

        pair<int, int> goal = {-1, -1};
        while (!pq.empty()) {
            auto [t, r, c] = pq.top();
            pq.pop();
            if (t != dist[r][c]) {
                continue;
            }
            if (!(r == hr && c == hc) && st.grid[r][c] == targetColor) {
                goal = {r, c};
                break;
            }

            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d];
                int nc = c + DC[d];
                if (!st.inBounds(nr, nc)) {
                    continue;
                }
                if (r == hr && c == hc && make_pair(nr, nc) == neck) {
                    continue;
                }

                int nt = t + 1;
                if (nt < release[nr][nc]) {
                    continue;
                }

                int food = st.grid[nr][nc];
                int ncost = nt;
                if (food != 0 && food != targetColor) {
                    ncost += foodPenalty;
                }

                if (ncost < dist[nr][nc]) {
                    dist[nr][nc] = ncost;
                    parent[nr][nc] = {r, c};
                    pdir[nr][nc] = d;
                    pq.push({ncost, nr, nc});
                }
            }
        }

        if (goal.first == -1) {
            return {};
        }

        vector<int> path;
        int cr = goal.first;
        int cc = goal.second;
        while (!(cr == hr && cc == hc)) {
            path.push_back(pdir[cr][cc]);
            auto [pr, pc] = parent[cr][cc];
            cr = pr;
            cc = pc;
        }
        reverse(path.begin(), path.end());
        return path;
    }

    vector<int> bfsToTargetColor(
        const SnakeState& st,
        int targetColor,
        bool avoidBody,
        bool avoidNonTargetFood
    ) const {
        if (st.body.empty()) {
            return {};
        }

        const int n = st.N;
        vector<vector<int>> dist(n, vector<int>(n, -1));
        vector<vector<pair<int, int>>> parent(n, vector<pair<int, int>>(n, {-1, -1}));
        vector<vector<int>> pdir(n, vector<int>(n, -1));
        vector<vector<char>> blocked(n, vector<char>(n, 0));

        if (avoidBody) {
            for (int i = 1; i < (int)st.body.size(); ++i) {
                auto [r, c] = st.body[i];
                blocked[r][c] = 1;
            }
        }

        auto [hr, hc] = st.body.front();
        pair<int, int> neck = {-1, -1};
        if (st.body.size() >= 2) {
            neck = st.body[1];
        }

        queue<pair<int, int>> q;
        q.push({hr, hc});
        dist[hr][hc] = 0;

        pair<int, int> goal = {-1, -1};

        while (!q.empty()) {
            auto [r, c] = q.front();
            q.pop();

            if (!(r == hr && c == hc) && st.grid[r][c] == targetColor) {
                goal = {r, c};
                break;
            }

            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d];
                int nc = c + DC[d];
                if (!st.inBounds(nr, nc) || dist[nr][nc] != -1) {
                    continue;
                }
                if (r == hr && c == hc && make_pair(nr, nc) == neck) {
                    continue;
                }
                if (avoidBody && blocked[nr][nc]) {
                    continue;
                }
                int food = st.grid[nr][nc];
                if (avoidNonTargetFood && food != 0 && food != targetColor) {
                    continue;
                }

                dist[nr][nc] = dist[r][c] + 1;
                parent[nr][nc] = {r, c};
                pdir[nr][nc] = d;
                q.push({nr, nc});
            }
        }

        if (goal.first == -1) {
            return {};
        }

        vector<int> path;
        int cr = goal.first;
        int cc = goal.second;
        while (!(cr == hr && cc == hc)) {
            path.push_back(pdir[cr][cc]);
            auto [pr, pc] = parent[cr][cc];
            cr = pr;
            cc = pc;
        }
        reverse(path.begin(), path.end());
        return path;
    }

    bool isTargetAdjacentToPrefixSegment(const SnakeState& st, int anchorLen, int targetColor) const {
        if (anchorLen <= 0) {
            return false;
        }
        if (anchorLen > (int)st.body.size()) {
            return false;
        }

        auto [r, c] = st.body[anchorLen - 1];
        for (int d = 0; d < 4; ++d) {
            int nr = r + DR[d];
            int nc = c + DC[d];
            if (!st.inBounds(nr, nc)) {
                continue;
            }
            if (st.grid[nr][nc] == targetColor) {
                return true;
            }
        }
        return false;
    }

    vector<CutCandidate> enumerateCutCandidates(
        const SnakeState& src,
        int baseL,
        int targetColor,
        int maxDepth = 3
    ) const {
        vector<CutCandidate> out;
        vector<int> seq;

        function<void(const SnakeState&, int)> dfs = [&](const SnakeState& cur, int depth) {
            if (depth >= maxDepth) {
                return;
            }

            auto dirs = cur.legalDirs();
            for (int d : dirs) {
                SnakeState nxt = cur;
                MoveResult ret = nxt.apply(d);
                if (!ret.ok) {
                    continue;
                }

                seq.push_back(d);

                if (ret.bite) {
                    int pref = nxt.prefixLen(desired);
                    int lenAfter = (int)nxt.body.size();
                    if (lenAfter >= 3 && (lenAfter % 2 == 1)) {
                        int anchorLen = min(baseL, lenAfter);
                        CutCandidate cand;
                        cand.seq = seq;
                        cand.after = nxt;
                        cand.biteTurn = (int)seq.size();
                        cand.prefixAfter = pref;
                        cand.anchorLen = anchorLen;
                        cand.adjacentToPrefix = isTargetAdjacentToPrefixSegment(nxt, anchorLen, targetColor);

                        auto directPath = findDynamicStrictPathToTarget(nxt, targetColor);
                        auto loosePath = bfsToTargetColor(nxt, targetColor, false, true);
                        cand.directPathLen = directPath.empty() ? INT_MAX : (int)directPath.size();
                        cand.loosePathLen = loosePath.empty() ? INT_MAX : (int)loosePath.size();
                        out.push_back(move(cand));
                    }
                } else {
                    dfs(nxt, depth + 1);
                }

                seq.pop_back();
            }
        };

        dfs(src, 0);
        return out;
    }

    optional<CutCandidate> chooseBestCutCandidate(
        const SnakeState& src,
        int baseL,
        int targetColor
    ) {
        auto cands = enumerateCutCandidates(src, baseL, targetColor, 3);
        if (cands.empty()) {
            return nullopt;
        }

        int curLen = (int)src.body.size();
        int floorHalf = max(1, curLen / 2);
        int halfOdd = floorHalf;
        if (halfOdd % 2 == 0) {
            --halfOdd;
        }
        halfOdd = max(3, halfOdd);

        auto cutLenCost = [&](const CutCandidate& c) -> int {
            int lenAfter = (int)c.after.body.size();
            int cost = abs(lenAfter - halfOdd) * 100;
            if (lenAfter > floorHalf) {
                cost += (lenAfter - floorHalf) * 300;
            }
            return cost;
        };

        int bestLenCost = INT_MAX;
        for (const auto& c : cands) {
            bestLenCost = min(bestLenCost, cutLenCost(c));
        }

        auto eval = [&](const CutCandidate& c) -> long long {
            bool reachable = c.adjacentToPrefix || c.directPathLen != INT_MAX || c.loosePathLen != INT_MAX;
            if (!reachable) {
                return (long long)4e18;
            }

            long long score = 0;
            // Cut length preference is enforced before this scoring step.
            score += 60'000LL * c.biteTurn;
            if (c.adjacentToPrefix) {
                score -= 120'000LL;
            }
            score += (c.directPathLen == INT_MAX ? 20'000 : c.directPathLen * 220);
            score += (c.loosePathLen == INT_MAX ? 10'000 : c.loosePathLen * 30);
            // Keep as much matched prefix as possible, but this is secondary to cut length.
            score -= 300LL * c.prefixAfter;
            return score;
        };

        auto pickBestIdx = [&](bool requireAdj) -> int {
            long long bestScore = (long long)4e18;
            vector<int> bestIdxs;
            for (int i = 0; i < (int)cands.size(); ++i) {
                if (cutLenCost(cands[i]) != bestLenCost) {
                    continue;
                }
                if (requireAdj && !cands[i].adjacentToPrefix) {
                    continue;
                }
                long long s = eval(cands[i]);
                if (s < bestScore) {
                    bestScore = s;
                    bestIdxs.clear();
                    bestIdxs.push_back(i);
                } else if (s == bestScore) {
                    bestIdxs.push_back(i);
                }
            }
            if (bestIdxs.empty()) {
                return -1;
            }
            uniform_int_distribution<int> uid(0, (int)bestIdxs.size() - 1);
            return bestIdxs[uid(rng)];
        };

        int bestIdx = pickBestIdx(true);
        if (bestIdx == -1) {
            bestIdx = pickBestIdx(false);
        }
        if (bestIdx == -1) {
            return nullopt;
        }
        return cands[bestIdx];
    }

    optional<CutCandidate> chooseRandomHalfCutCandidate(
        const SnakeState& src,
        int baseL,
        int targetColor
    ) {
        auto cands = enumerateCutCandidates(src, baseL, targetColor, 3);
        if (cands.empty()) {
            return nullopt;
        }

        int curLen = (int)src.body.size();
        int floorHalf = max(1, curLen / 2);
        int halfOdd = floorHalf;
        if (halfOdd % 2 == 0) {
            --halfOdd;
        }
        halfOdd = max(3, halfOdd);

        auto cutLenCost = [&](const CutCandidate& c) -> int {
            int lenAfter = (int)c.after.body.size();
            int cost = abs(lenAfter - halfOdd) * 100;
            if (lenAfter > floorHalf) {
                cost += (lenAfter - floorHalf) * 300;
            }
            return cost;
        };

        int bestLenCost = INT_MAX;
        for (const auto& c : cands) {
            bestLenCost = min(bestLenCost, cutLenCost(c));
        }

        vector<int> idxs;
        for (int i = 0; i < (int)cands.size(); ++i) {
            if (cutLenCost(cands[i]) == bestLenCost) {
                idxs.push_back(i);
            }
        }
        if (idxs.empty()) {
            return nullopt;
        }
        uniform_int_distribution<int> uid(0, (int)idxs.size() - 1);
        return cands[idxs[uid(rng)]];
    }

    vector<CutCandidate> chooseTopCutCandidatesForBeam(
        const SnakeState& src,
        int baseL,
        int targetColor,
        int topK
    ) const {
        topK = max(1, topK);
        auto cands = enumerateCutCandidates(src, baseL, targetColor, 3);
        if (cands.empty()) {
            return {};
        }

        int curLen = (int)src.body.size();
        int wrongSuffix = max(0, curLen - baseL);

        struct Ranked {
            long long score = (long long)4e18;
            int idx = -1;
        };
        vector<Ranked> ranked;
        ranked.reserve(cands.size());

        for (int i = 0; i < (int)cands.size(); ++i) {
            const auto& c = cands[i];
            int lenAfter = (int)c.after.body.size();
            int remainingWrong = max(0, lenAfter - baseL);
            int lostPrefix = max(0, baseL - lenAfter);
            int removedWrong = max(0, wrongSuffix - remainingWrong);
            bool removesWrong = removedWrong > 0;
            bool reachable = c.adjacentToPrefix || c.directPathLen != INT_MAX || c.loosePathLen != INT_MAX;

            long long s = 0;
            // Primary: remove mismatch suffix while preserving prefix.
            s += 2'000'000LL * remainingWrong;
            s += 120'000LL * lostPrefix;
            if (!removesWrong && wrongSuffix > 0) {
                s += 2'500'000LL;
            }
            // Secondary: practical recoverability and speed.
            s += 20'000LL * c.biteTurn;
            if (!c.adjacentToPrefix) {
                s += 90'000LL;
            }
            s += (c.directPathLen == INT_MAX ? 400'000 : 120LL * c.directPathLen);
            s += (c.loosePathLen == INT_MAX ? 200'000 : 25LL * c.loosePathLen);
            if (!reachable) {
                s += 4'000'000LL;
            }
            ranked.push_back({s, i});
        }

        sort(ranked.begin(), ranked.end(), [](const Ranked& a, const Ranked& b) {
            if (a.score != b.score) {
                return a.score < b.score;
            }
            return a.idx < b.idx;
        });

        vector<CutCandidate> out;
        int take = min(topK, (int)ranked.size());
        out.reserve(take);
        for (int i = 0; i < take; ++i) {
            out.push_back(cands[ranked[i].idx]);
        }
        return out;
    }

    long long evaluateBeamState(const SnakeState& st, int maxPrefix) const {
        int k = (int)st.body.size();
        int pref = min(maxPrefix, k);
        int E = k - pref;
        return st.turn + 10000LL * (E + 2LL * (M - k));
    }

    uint64_t beamStateKey(const SnakeState& st, int maxPrefix) const {
        uint64_t h = 1469598103934665603ULL;
        auto mix = [&](uint64_t x) {
            h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        };
        mix((uint64_t)st.turn);
        mix((uint64_t)st.body.size());
        mix((uint64_t)min(maxPrefix, M));
        for (int i = 0; i < st.N; ++i) {
            for (int j = 0; j < st.N; ++j) {
                mix((uint64_t)st.grid[i][j] + 1ULL);
            }
        }
        int k = (int)st.body.size();
        for (int i = 0; i < k; ++i) {
            auto [r, c] = st.body[i];
            mix((uint64_t)(r * st.N + c + 1));
            mix((uint64_t)st.colors[i] + 1ULL);
        }
        return h;
    }

    bool betterBeamNode(const BeamNode& a, const BeamNode& b) const {
        if (a.eval != b.eval) {
            return a.eval < b.eval;
        }
        if (a.maxPrefix != b.maxPrefix) {
            return a.maxPrefix > b.maxPrefix;
        }
        return a.st.turn < b.st.turn;
    }

    int pickWanderStepForState(const SnakeState& st, int targetColor, int baseL) {
        auto dirs = st.legalDirs();
        if (dirs.empty()) {
            return -1;
        }

        uniform_real_distribution<double> urand(0.0, 1.0);
        struct ScoredMove {
            double score;
            int dir;
        };
        vector<ScoredMove> scored;
        scored.reserve(dirs.size());

        for (int d : dirs) {
            auto [hr, hc] = st.body.front();
            int nr = hr + DR[d];
            int nc = hc + DC[d];
            SnakeState nxt = st;
            MoveResult ret = nxt.apply(d);
            if (!ret.ok) {
                continue;
            }

            int pref = nxt.prefixLen(desired);
            int nearTarget = manhattanToNearestTarget(nxt, targetColor);
            int food = st.grid[nr][nc];

            double s = 0.0;
            s += urand(rng) * 5.0;
            s += urand(rng) * 5.0;
            s += (food == 0 ? 1.0 : 2.0);
            s += (int)nxt.legalDirs().size() * 0.5;
            if (food == targetColor) {
                s += 3.0;
            }
            if (ret.bite) {
                s -= 5.0;
            }
            if (pref < baseL) {
                s -= 8.0;
            }
            if (nearTarget != INT_MAX) {
                s -= nearTarget * 0.05;
            }
            scored.push_back({s, d});
        }

        if (scored.empty()) {
            return dirs[0];
        }
        sort(scored.begin(), scored.end(), [](const ScoredMove& a, const ScoredMove& b) {
            return a.score > b.score;
        });
        int topK = min(3, (int)scored.size());
        uniform_int_distribution<int> uid(0, topK - 1);
        return scored[uid(rng)].dir;
    }

    vector<BeamNode> transitionGoTargetCandidates(
        const BeamNode& cur,
        chrono::steady_clock::time_point deadline,
        int topK
    ) {
        if (chrono::steady_clock::now() >= deadline) {
            return {};
        }
        int p = cur.st.prefixLen(desired);
        if (p >= M) {
            return {};
        }
        // When prefix is already broken, avoid greedily collecting the same target color.
        if (p < (int)cur.st.colors.size()) {
            return {};
        }
        int targetColor = desired[p];
        auto paths = findDynamicStrictPathsToTarget(cur.st, targetColor, topK);
        if (paths.empty()) {
            return {};
        }

        vector<BeamNode> out;
        out.reserve(paths.size());
        for (const auto& path : paths) {
            if (chrono::steady_clock::now() >= deadline) {
                break;
            }
            BeamNode nxt;
            nxt.st = cur.st;
            nxt.path = cur.path;
            nxt.path.reserve(cur.path.size() + path.size());

            bool ok = true;
            for (int d : path) {
                if (chrono::steady_clock::now() >= deadline) {
                    ok = false;
                    break;
                }
                if (nxt.st.turn >= TURN_LIMIT) {
                    break;
                }
                MoveResult ret = nxt.st.apply(d);
                if (!ret.ok) {
                    ok = false;
                    break;
                }
                nxt.path.push_back(d);
            }
            if (!ok) {
                continue;
            }
            int newPrefix = nxt.st.prefixLen(desired);
            nxt.maxPrefix = max(cur.maxPrefix, newPrefix);
            nxt.eval = evaluateBeamState(nxt.st, nxt.maxPrefix);
            out.push_back(move(nxt));
        }
        return out;
    }

    vector<BeamNode> transitionCutRecoverCandidates(
        const BeamNode& cur,
        chrono::steady_clock::time_point deadline,
        int topK
    ) {
        if (chrono::steady_clock::now() >= deadline) {
            return {};
        }
        int p = cur.st.prefixLen(desired);
        if (p >= M) {
            return {};
        }
        int targetColor = desired[p];
        auto cutCands = chooseTopCutCandidatesForBeam(cur.st, p, targetColor, topK);
        if (cutCands.empty()) {
            return {};
        }

        vector<BeamNode> out;
        out.reserve(cutCands.size());
        for (const auto& cut : cutCands) {
            if (chrono::steady_clock::now() >= deadline) {
                break;
            }
            BeamNode nxt;
            nxt.st = cur.st;
            nxt.path = cur.path;

            bool bitten = false;
            bool ok = true;
            for (int d : cut.seq) {
                if (chrono::steady_clock::now() >= deadline) {
                    ok = false;
                    break;
                }
                if (nxt.st.turn >= TURN_LIMIT) {
                    break;
                }
                SnakeState beforeStep = nxt.st;
                int prefixBeforeStep = beforeStep.prefixLen(desired);
                MoveResult ret = nxt.st.apply(d);
                if (!ret.ok) {
                    ok = false;
                    break;
                }
                nxt.path.push_back(d);
                if (ret.bite) {
                    bitten = true;
                    auto recovery = buildRecoveryPathByOldBody(beforeStep, nxt.st, prefixBeforeStep);
                    for (int rd : recovery) {
                        if (chrono::steady_clock::now() >= deadline) {
                            ok = false;
                            break;
                        }
                        if (nxt.st.turn >= TURN_LIMIT) {
                            break;
                        }
                        MoveResult rr = nxt.st.apply(rd);
                        if (!rr.ok) {
                            ok = false;
                            break;
                        }
                        nxt.path.push_back(rd);
                    }
                    break;
                }
            }
            if (!ok || !bitten) {
                continue;
            }

            int newPrefix = nxt.st.prefixLen(desired);
            nxt.maxPrefix = max(cur.maxPrefix, newPrefix);
            nxt.eval = evaluateBeamState(nxt.st, nxt.maxPrefix);
            out.push_back(move(nxt));
        }
        return out;
    }

    optional<BeamNode> transitionWander(
        const BeamNode& cur,
        chrono::steady_clock::time_point deadline
    ) {
        if (chrono::steady_clock::now() >= deadline) {
            return nullopt;
        }
        int p = cur.st.prefixLen(desired);
        if (p >= M) {
            return nullopt;
        }
        int targetColor = desired[p];

        BeamNode nxt;
        nxt.st = cur.st;
        nxt.path = cur.path;

        int moved = 0;
        for (int t = 0; t < WANDER_LEN; ++t) {
            if (chrono::steady_clock::now() >= deadline) {
                break;
            }
            if (nxt.st.turn >= TURN_LIMIT) {
                break;
            }
            int d = pickWanderStepForState(nxt.st, targetColor, p);
            if (d == -1) {
                break;
            }
            MoveResult ret = nxt.st.apply(d);
            if (!ret.ok) {
                break;
            }
            nxt.path.push_back(d);
            ++moved;
            if (nxt.st.prefixLen(desired) >= M) {
                break;
            }
        }
        if (moved == 0) {
            return nullopt;
        }

        int newPrefix = nxt.st.prefixLen(desired);
        nxt.maxPrefix = max(cur.maxPrefix, newPrefix);
        nxt.eval = evaluateBeamState(nxt.st, nxt.maxPrefix);
        return nxt;
    }

    bool isDoneNode(const BeamNode& node) const {
        return node.st.prefixLen(desired) >= M;
    }

    bool betterFinalNode(const BeamNode& a, const BeamNode& b) const {
        bool ad = isDoneNode(a);
        bool bd = isDoneNode(b);
        if (ad != bd) {
            return ad;
        }
        return betterBeamNode(a, b);
    }

    vector<int> buildMaxPrefixCurveFromPath(const SnakeState& initial, const vector<int>& path) const {
        SnakeState st = initial;
        int best = st.prefixLen(desired);
        vector<int> curve;
        curve.reserve(path.size() + 1);
        curve.push_back(best);
        for (int d : path) {
            MoveResult ret = st.apply(d);
            if (!ret.ok) {
                break;
            }
            best = max(best, st.prefixLen(desired));
            curve.push_back(best);
            if (st.turn >= TURN_LIMIT) {
                break;
            }
        }
        return curve;
    }

    int greedyPrefixLowerBoundAtTurn(const vector<int>& curve, int turn) const {
        if (curve.empty()) {
            return 0;
        }
        if (turn <= 0) {
            return curve[0];
        }
        if (turn >= (int)curve.size()) {
            return curve.back();
        }
        return curve[turn];
    }

    BeamNode runLegacyFallbackPolicy(const SnakeState& initial, chrono::steady_clock::time_point deadline) {
        state = initial;
        moves.clear();
        recoveryMoves.clear();
        recoveryTargetPrefix = 0;
        visitCount.assign(N, vector<int>(N, 0));
        if (!state.body.empty()) {
            auto [sr, sc] = state.body.front();
            visitCount[sr][sc] = 1;
        }
        wanderBudget = 0;

        int bestPrefix = state.prefixLen(desired);
        int stagnation = 0;

        while (state.turn < TURN_LIMIT && chrono::steady_clock::now() < deadline) {
            int p = state.prefixLen(desired);
            if (p >= M) {
                break;
            }
            bool prefixBroken = p < (int)state.colors.size();

            if (!recoveryMoves.empty()) {
                int d = recoveryMoves.front();
                recoveryMoves.pop_front();
                if (!executeDir(d)) {
                    recoveryMoves.clear();
                }
                continue;
            }

            if (p > bestPrefix) {
                bestPrefix = p;
                stagnation = 0;
                wanderBudget = 0;
            } else {
                ++stagnation;
            }

            int targetColor = desired[p];
            int remaining = M - p;
            bool nearFinish = remaining <= 2;
            bool boardEmpty = remainingFoodCount(state) == 0;

            // Emergency: if no foods remain on board but sequence is unfinished,
            // force a bite to recreate foods from the mismatched suffix.
            if (boardEmpty) {
                auto cutOpt = chooseBestCutCandidate(state, p, targetColor);
                if (!cutOpt.has_value()) {
                    cutOpt = chooseRandomHalfCutCandidate(state, p, targetColor);
                }
                if (cutOpt.has_value()) {
                    bool ok = true;
                    for (int d : cutOpt->seq) {
                        if (state.turn >= TURN_LIMIT || chrono::steady_clock::now() >= deadline) {
                            break;
                        }
                        if (!executeDir(d)) {
                            ok = false;
                            break;
                        }
                    }
                    if (!ok) {
                        break;
                    }
                    stagnation = 0;
                    wanderBudget = 0;
                    continue;
                }

                // Even if we cannot find a bite sequence now, never freeze.
                auto dirs = state.legalDirs();
                if (dirs.empty()) {
                    break;
                }
                if (!executeDir(dirs[0])) {
                    break;
                }
                continue;
            }

            if (!prefixBroken) {
                // Strategy 1: Direct strict path (dynamic release, avoid non-target food)
                auto direct = findDynamicStrictPathToTarget(state, targetColor);
                if (!direct.empty()) {
                    if (!executeDir(direct.front())) {
                        break;
                    }
                    continue;
                }

                // Strategy 2: Tail path (with stagnation gate to avoid infinite tail-chasing)
                auto tailPath = findDynamicStrictPathToTail(state, targetColor);
                if (!tailPath.empty() && (nearFinish || stagnation < 60)) {
                    if (!executeDir(tailPath.front())) {
                        break;
                    }
                    continue;
                }
            }

            // Strategy 3: Wander to reposition
            if (wanderBudget == 0 && (stagnation >= 10 || prefixBroken)) {
                wanderBudget = 5;
            }
            if (wanderBudget > 0) {
                int d = -1;
                if (prefixBroken) {
                    d = pickExploreMove(targetColor, p, true, false);
                } else {
                    d = pickWanderMove(targetColor, p);
                }
                if (d == -1) {
                    auto dirs = state.legalDirs();
                    if (dirs.empty()) {
                        break;
                    }
                    d = dirs[0];
                }
                if (!executeDir(d)) {
                    break;
                }
                --wanderBudget;
                continue;
            }

            // Strategy 4: Cut (threshold reduced from 70 to 30)
            int cutThreshold = nearFinish ? 120 : 30;
            if (prefixBroken) {
                cutThreshold = min(cutThreshold, 18);
            }
            if (stagnation >= cutThreshold) {
                auto cutOpt = chooseBestCutCandidate(state, p, targetColor);
                if (cutOpt.has_value()) {
                    bool ok = true;
                    for (int d : cutOpt->seq) {
                        if (state.turn >= TURN_LIMIT || chrono::steady_clock::now() >= deadline) {
                            break;
                        }
                        if (!executeDir(d)) {
                            ok = false;
                            break;
                        }
                    }
                    if (!ok) {
                        break;
                    }
                    stagnation = max(0, stagnation - 15);
                    continue;
                }
            }

            // Strategy 5: Penalized path (allow non-target food with penalty, very high stagnation)
            if (!prefixBroken && stagnation >= 80) {
                auto penalized = findPenalizedPathToTarget(state, targetColor, N * 3);
                if (!penalized.empty()) {
                    if (!executeDir(penalized.front())) {
                        break;
                    }
                    continue;
                }
            }

            // Strategy 6: Explore
            int fallback = pickExploreMove(targetColor, p, true, !prefixBroken);
            if (fallback == -1) {
                auto dirs = state.legalDirs();
                if (dirs.empty()) {
                    break;
                }
                fallback = dirs[0];
            }
            if (!executeDir(fallback)) {
                auto dirs = state.legalDirs();
                if (dirs.empty()) {
                    break;
                }
                if (!executeDir(dirs[0])) {
                    break;
                }
            }
        }

        BeamNode out;
        out.st = state;
        out.maxPrefix = max(bestPrefix, out.st.prefixLen(desired));
        out.eval = evaluateBeamState(out.st, out.maxPrefix);
        out.path.reserve(moves.size());
        for (char c : moves) {
            if (c == 'U') {
                out.path.push_back(0);
            } else if (c == 'D') {
                out.path.push_back(1);
            } else if (c == 'L') {
                out.path.push_back(2);
            } else if (c == 'R') {
                out.path.push_back(3);
            }
        }
        return out;
    }

    optional<BeamNode> runBeamSearch(
        const SnakeState& initial,
        chrono::steady_clock::time_point deadline,
        const vector<int>& greedyPrefixCurve
    ) {
        BeamNode init;
        init.st = initial;
        init.maxPrefix = init.st.prefixLen(desired);
        init.eval = evaluateBeamState(init.st, init.maxPrefix);

        vector<BeamNode> beam;
        beam.push_back(move(init));

        bool hasBestDone = false;
        BeamNode bestDone;

        for (int depth = 0; depth < TURN_LIMIT && !beam.empty(); ++depth) {
            if (chrono::steady_clock::now() >= deadline) {
                break;
            }
            vector<BeamNode> expanded;
            expanded.reserve(beam.size() * 3);

            for (const auto& node : beam) {
                if (chrono::steady_clock::now() >= deadline) {
                    break;
                }
                if (node.maxPrefix < greedyPrefixLowerBoundAtTurn(greedyPrefixCurve, node.st.turn)) {
                    continue;
                }
                int p = node.st.prefixLen(desired);
                if (p >= M) {
                    if (!hasBestDone || betterBeamNode(node, bestDone)) {
                        hasBestDone = true;
                        bestDone = node;
                    }
                    continue;
                }

                auto nxtAs = transitionGoTargetCandidates(node, deadline, BEAM_CANDIDATE_TOP_K);
                for (auto& nxt : nxtAs) {
                    if (nxt.maxPrefix >= greedyPrefixLowerBoundAtTurn(greedyPrefixCurve, nxt.st.turn)) {
                        expanded.push_back(move(nxt));
                    }
                }

                if (chrono::steady_clock::now() >= deadline) {
                    break;
                }
                auto nxtBs = transitionCutRecoverCandidates(node, deadline, 1);
                for (auto& nxt : nxtBs) {
                    if (nxt.maxPrefix >= greedyPrefixLowerBoundAtTurn(greedyPrefixCurve, nxt.st.turn)) {
                        expanded.push_back(move(nxt));
                    }
                }

                if (chrono::steady_clock::now() >= deadline) {
                    break;
                }
                auto nxtC = transitionWander(node, deadline);
                if (nxtC.has_value()) {
                    if (nxtC->maxPrefix >= greedyPrefixLowerBoundAtTurn(greedyPrefixCurve, nxtC->st.turn)) {
                        expanded.push_back(move(nxtC.value()));
                    }
                }
            }

            if (hasBestDone) {
                break;
            }
            if (expanded.empty()) {
                break;
            }

            unordered_map<uint64_t, int> bestByKey;
            vector<BeamNode> uniq;
            uniq.reserve(expanded.size());

            for (auto& cand : expanded) {
                uint64_t key = beamStateKey(cand.st, cand.maxPrefix);
                auto it = bestByKey.find(key);
                if (it == bestByKey.end()) {
                    bestByKey[key] = (int)uniq.size();
                    uniq.push_back(move(cand));
                } else {
                    int idx = it->second;
                    if (betterBeamNode(cand, uniq[idx])) {
                        uniq[idx] = move(cand);
                    }
                }
            }

            sort(uniq.begin(), uniq.end(), [&](const BeamNode& a, const BeamNode& b) {
                return betterBeamNode(a, b);
            });
            if ((int)uniq.size() > BEAM_WIDTH) {
                uniq.resize(BEAM_WIDTH);
            }
            beam = move(uniq);
        }

        if (hasBestDone) {
            return bestDone;
        }
        if (beam.empty()) {
            return nullopt;
        }
        BeamNode best = beam[0];
        for (int i = 1; i < (int)beam.size(); ++i) {
            if (betterBeamNode(beam[i], best)) {
                best = beam[i];
            }
        }
        return best;
    }

    int pickExploreMove(int targetColor, int baseL, bool randomized, bool preferTargetColor = true) {
        auto dirs = state.legalDirs();
        if (dirs.empty()) {
            return -1;
        }

        uniform_real_distribution<double> urand(0.0, 1.0);
        if (randomized && urand(rng) < 0.15) {
            uniform_int_distribution<int> uid(0, (int)dirs.size() - 1);
            return dirs[uid(rng)];
        }

        struct ScoredMove {
            double score;
            int dir;
        };
        vector<ScoredMove> scored;
        scored.reserve(dirs.size());

        for (int d : dirs) {
            auto [hr, hc] = state.body.front();
            int nr = hr + DR[d];
            int nc = hc + DC[d];
            int landingFood = state.grid[nr][nc];

            SnakeState nxt = state;
            MoveResult ret = nxt.apply(d);
            if (!ret.ok) {
                continue;
            }

            int pref = nxt.prefixLen(desired);
            int nearTarget = manhattanToNearestTarget(nxt, targetColor);
            int mobility = (int)nxt.legalDirs().size();

            double s = 0.0;
            if (preferTargetColor) {
                if (landingFood == 0) {
                    s += 2.5;
                } else if (landingFood == targetColor) {
                    s += 8.0;
                } else {
                    s -= 8.0;
                }
            } else {
                // Prefix-broken mode: any color is acceptable; prioritize exploration over color fixation.
                if (landingFood == 0) {
                    s += 2.0;
                } else {
                    s += 1.5;
                }
            }
            if (ret.bite) {
                s += 0.5;
            }
            if (pref < baseL) {
                s -= 2000.0;
            }
            s += mobility * 0.6;
            if (preferTargetColor && nearTarget != INT_MAX) {
                s -= nearTarget * 0.4;
            }
            if (randomized) {
                s += urand(rng);
            }

            scored.push_back({s, d});
        }

        if (scored.empty()) {
            return dirs[0];
        }

        sort(scored.begin(), scored.end(), [](const ScoredMove& a, const ScoredMove& b) {
            return a.score > b.score;
        });

        if (!randomized) {
            return scored.front().dir;
        }

        int topK = min(3, (int)scored.size());
        if (urand(rng) < 0.75) {
            uniform_int_distribution<int> uid(0, topK - 1);
            return scored[uid(rng)].dir;
        }
        uniform_int_distribution<int> uid(0, (int)scored.size() - 1);
        return scored[uid(rng)].dir;
    }

    int pickWanderMove(int targetColor, int baseL) {
        auto dirs = state.legalDirs();
        if (dirs.empty()) {
            return -1;
        }

        uniform_real_distribution<double> urand(0.0, 1.0);
        struct ScoredMove {
            double score;
            int dir;
        };
        vector<ScoredMove> scored;
        scored.reserve(dirs.size());

        for (int d : dirs) {
            auto [hr, hc] = state.body.front();
            int nr = hr + DR[d];
            int nc = hc + DC[d];

            SnakeState nxt = state;
            MoveResult ret = nxt.apply(d);
            if (!ret.ok) {
                continue;
            }

            int pref = nxt.prefixLen(desired);
            int nearTarget = manhattanToNearestTarget(nxt, targetColor);
            int food = state.grid[nr][nc];
            int visits = visitCount[nr][nc];

            double s = 0.0;
            s += urand(rng) * 6.0;
            s += urand(rng) * 6.0;
            s += (food == 0 ? 0.5 : 2.5);
            s += max(0, 5 - visits) * 0.8;
            s += (int)nxt.legalDirs().size() * 0.4;
            if (food == targetColor) {
                s += 3.0;
            }
            if (ret.bite) {
                s -= 6.0;
            }
            if (pref < baseL) {
                s -= 12.0;
            }
            if (nearTarget != INT_MAX) {
                s -= nearTarget * 0.05;
            }

            scored.push_back({s, d});
        }

        if (scored.empty()) {
            return dirs[0];
        }
        sort(scored.begin(), scored.end(), [](const ScoredMove& a, const ScoredMove& b) {
            return a.score > b.score;
        });
        int topK = min(3, (int)scored.size());
        uniform_int_distribution<int> uid(0, topK - 1);
        return scored[uid(rng)].dir;
    }

    bool randomSearch(int baseL) {
        int used = 0;

        while (used < RANDOM_SEARCH_LIMIT && state.turn < TURN_LIMIT) {
            int p = state.prefixLen(desired);
            if (p >= M) {
                return true;
            }
            int targetColor = desired[p];

            auto direct = bfsToTargetColor(state, targetColor, true, true);
            if (!direct.empty()) {
                return true;
            }
            auto cutOpt = chooseBestCutCandidate(state, baseL, targetColor);
            if (cutOpt.has_value()) {
                return true;
            }

            auto randomCut = chooseRandomHalfCutCandidate(state, baseL, targetColor);
            if (randomCut.has_value()) {
                uniform_int_distribution<int> prob(0, 99);
                if (prob(rng) < 40) {
                    const auto& cand = randomCut.value();
                    for (int d : cand.seq) {
                        if (used >= RANDOM_SEARCH_LIMIT || state.turn >= TURN_LIMIT) {
                            break;
                        }
                        if (!executeDir(d)) {
                            break;
                        }
                        ++used;
                    }
                    continue;
                }
            }

            int d = pickExploreMove(targetColor, baseL, true);
            if (d == -1) {
                return false;
            }
            if (!executeDir(d)) {
                return false;
            }
            ++used;
        }

        return false;
    }

    void solve() {
        auto start = chrono::steady_clock::now();
        auto totalDeadline = start + chrono::milliseconds(BEAM_TIME_LIMIT_MS);
        auto beamDeadline = min(totalDeadline, start + chrono::milliseconds(BEAM_PHASE_LIMIT_MS));

        SnakeState initial = state;
        BeamNode greedy = runLegacyFallbackPolicy(initial, beamDeadline);
        vector<int> greedyPrefixCurve = buildMaxPrefixCurveFromPath(initial, greedy.path);

        BeamNode best = greedy;
        if (chrono::steady_clock::now() < totalDeadline) {
            auto beamBest = runBeamSearch(initial, totalDeadline, greedyPrefixCurve);
            if (beamBest.has_value() && betterFinalNode(beamBest.value(), best)) {
                best = move(beamBest.value());
            }
        }

        moves.clear();
        for (int d : best.path) {
            moves.push_back(DCHAR[d]);
        }
    }

    void printAnswer() const {
        for (char c : moves) {
            cout << c << '\n';
        }
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
