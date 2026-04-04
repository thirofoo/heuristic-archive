#include <bits/stdc++.h>
using namespace std;

// Direction: 0=U, 1=D, 2=L, 3=R
const int DR[] = {-1, 1, 0, 0};
const int DC[] = {0, 0, -1, 1};
const char DCHAR[] = "UDLR";

int N, M, C;
vector<int> desired;
vector<vector<int>> grid;

// Snake state
// body[0] = head, body[k-1] = tail
// colors[i] = color at logical position i (head=0, tail=k-1)
deque<pair<int,int>> body;
deque<int> colors;
string moves;

bool inBounds(int r, int c) {
    return 0 <= r && r < N && 0 <= c && c < N;
}

int recomputeProgress() {
    int lim = min((int)colors.size(), M);
    int p = 0;
    while (p < lim && colors[p] == desired[p]) p++;
    return p;
}

// Execute one move in direction d, update all state.
// Returns true iff a bite happened.
bool doMove(int d) {
    auto [hr, hc] = body.front();
    int nr = hr + DR[d], nc = hc + DC[d];
    moves += DCHAR[d];

    // Step 1: Move
    body.push_front({nr, nc});

    // Step 2: Eat
    if (grid[nr][nc] != 0) {
        colors.push_back(grid[nr][nc]);
        grid[nr][nc] = 0;
        return false; // eating and biting cannot coexist in this implementation
    }

    // Step 3: Normal movement
    body.pop_back();

    // Step 4: Bite check — head collides with body (excluding tail)
    int k = (int)body.size();
    for (int h = 1; h <= k - 2; h++) {
        if (body[0] == body[h]) {
            // Bite off segments h+1 .. k-1, restore as food
            while ((int)body.size() > h + 1) {
                auto [bi, bj] = body.back();
                grid[bi][bj] = colors.back();
                body.pop_back();
                colors.pop_back();
            }
            return true;
        }
    }
    return false;
}

// BFS to an arbitrary target cell.
// avoidFood: skip cells with food (except target)
// avoidBody: skip snake body cells
// allowedBodyTarget: if target itself is a body cell, allow stepping onto that target even when avoidBody=true
vector<int> findPathCell(pair<int,int> tgt, bool avoidFood, bool avoidBody, bool allowedBodyTarget = false) {
    auto [tr, tc] = tgt;
    auto [hr, hc] = body.front();
    pair<int,int> neck = (body.size() >= 2 ? body[1] : make_pair(-1, -1));

    vector<vector<char>> vis(N, vector<char>(N, 0));
    vector<vector<pair<int,int>>> par(N, vector<pair<int,int>>(N, {-1, -1}));
    vector<vector<int>> pdir(N, vector<int>(N, -1));
    vector<vector<char>> blocked(N, vector<char>(N, 0));

    if (avoidBody) {
        for (int i = 1; i < (int)body.size(); i++) {
            auto [r, c] = body[i];
            blocked[r][c] = 1;
        }
        if (allowedBodyTarget) blocked[tr][tc] = 0;
    }

    queue<pair<int,int>> q;
    q.push({hr, hc});
    vis[hr][hc] = 1;

    while (!q.empty()) {
        auto [r, c] = q.front();
        q.pop();
        if (r == tr && c == tc) break;

        for (int d = 0; d < 4; d++) {
            int nr = r + DR[d], nc = c + DC[d];
            if (!inBounds(nr, nc) || vis[nr][nc]) continue;

            // no immediate U-turn from head
            if (r == hr && c == hc && make_pair(nr, nc) == neck) continue;

            if (avoidBody && blocked[nr][nc]) continue;
            if (avoidFood && grid[nr][nc] != 0 && make_pair(nr, nc) != tgt) continue;

            vis[nr][nc] = 1;
            par[nr][nc] = {r, c};
            pdir[nr][nc] = d;
            q.push({nr, nc});
        }
    }

    if (!vis[tr][tc]) return {};

    vector<int> path;
    int cr = tr, cc = tc;
    while (!(cr == hr && cc == hc)) {
        path.push_back(pdir[cr][cc]);
        auto [pr, pc] = par[cr][cc];
        cr = pr;
        cc = pc;
    }
    reverse(path.begin(), path.end());
    return path;
}

// Find all cells containing a color.
vector<pair<int,int>> findFoodsOfColor(int color) {
    vector<pair<int,int>> res;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (grid[i][j] == color) res.push_back({i, j});
        }
    }
    return res;
}

// Greedy direct plan to targetColor.
// mode 0: avoid food + avoid body
// mode 1: avoid food + allow body (bite may happen)
// returns best path/cell under that mode
pair<pair<int,int>, vector<int>> planToColor(int targetColor, int mode) {
    auto foods = findFoodsOfColor(targetColor);
    if (foods.empty()) return {{-1, -1}, {}};

    auto [hr, hc] = body.front();
    pair<int,int> bestCell = {-1, -1};
    vector<int> bestPath;
    int bestScore = INT_MAX;

    for (auto cell : foods) {
        vector<int> path;
        if (mode == 0) path = findPathCell(cell, true, true, false);
        else          path = findPathCell(cell, true, false, false);

        if (path.empty()) continue;

        // modestly better than Manhattan-only:
        // prioritize short actual paths, tie-break by Manhattan
        int man = abs(cell.first - hr) + abs(cell.second - hc);
        int score = (int)path.size() * 1000 + man;
        if (score < bestScore) {
            bestScore = score;
            bestCell = cell;
            bestPath = move(path);
        }
    }
    return {bestCell, bestPath};
}

// Intentional bite plan:
// go to a body cell on purpose (without stepping on other food),
// so that the suffix is restored and the snake gets shorter.
// We score candidates by:
//   - smaller path to bite
//   - closer to next desired food after bite point
//   - keep enough body length (avoid over-shrinking)
vector<int> planIntentionalBite(int targetColor) {
    auto foods = findFoodsOfColor(targetColor);
    if (foods.empty()) return {};

    int k = (int)body.size();
    if (k <= 3) return {}; // too short to bite usefully

    int bestScore = INT_MAX;
    vector<int> bestPath;

    // Choose a bite target among body segments.
    // Avoid very shallow bites that do almost nothing, and very deep bites that destroy too much.
    for (int idx = 2; idx <= k - 2; idx++) {
        auto biteCell = body[idx];

        // Path to the chosen body cell, while avoiding food and all other body cells.
        vector<int> path = findPathCell(biteCell, true, true, true);
        if (path.empty()) continue;

        // Estimate distance from bite cell to nearest target-color food.
        int nearFood = INT_MAX;
        for (auto f : foods) {
            nearFood = min(nearFood, abs(f.first - biteCell.first) + abs(f.second - biteCell.second));
        }

        // Remaining logical length after bite:
        // after moving into body[idx], collision happens and suffix after collision point is removed.
        // Exact remaining size depends on the shifted indices, but idx is still a reasonable proxy.
        int remainingApprox = idx + 1;

        // Prefer:
        // - short path to bite
        // - bite cell near target food
        // - not over-shrinking
        // strong penalty if too short
        int penaltyShort = (remainingApprox < 4 ? 100000 : 0);

        int score = (int)path.size() * 1000 + nearFood * 20 + penaltyShort - remainingApprox * 3;
        if (score < bestScore) {
            bestScore = score;
            bestPath = move(path);
        }
    }

    return bestPath;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> N >> M >> C;
    desired.resize(M);
    for (auto& x : desired) cin >> x;

    grid.assign(N, vector<int>(N, 0));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            cin >> grid[i][j];
        }
    }

    // Initial snake: head=(4,0), tail=(0,0), all colors 1
    body = {{4,0},{3,0},{2,0},{1,0},{0,0}};
    colors = {1,1,1,1,1};

    int p = recomputeProgress();
    int stuckCnt = 0;

    while (p < M && (int)moves.size() < 99000) {
        int targetColor = desired[p];

        // 1) Safe direct path
        auto [best0, path0] = planToColor(targetColor, 0);

        if (!path0.empty()) {
            stuckCnt = 0;
            bool bitten = false;
            for (int d : path0) {
                if ((int)moves.size() >= 99000) break;
                bool bite = doMove(d);
                if (bite) {
                    bitten = true;
                    break;
                }
            }
            p = recomputeProgress();
            if (!bitten && best0.first != -1 && grid[best0.first][best0.second] == 0) {
                p = recomputeProgress();
            }
            continue;
        }

        // 2) Direct path allowing bite en route
        auto [best1, path1] = planToColor(targetColor, 1);

        if (!path1.empty()) {
            stuckCnt = 0;
            for (int d : path1) {
                if ((int)moves.size() >= 99000) break;
                doMove(d);
                // re-evaluate immediately after any structural change
                p = recomputeProgress();
                if (p >= M) break;
                // stop early if we already ate the target cell
                if (best1.first != -1 && grid[best1.first][best1.second] == 0) break;
            }
            p = recomputeProgress();
            continue;
        }

        // 3) Intentional bite as a greedy state-reset
        vector<int> bitePlan = planIntentionalBite(targetColor);
        if (!bitePlan.empty()) {
            stuckCnt = 0;
            for (int d : bitePlan) {
                if ((int)moves.size() >= 99000) break;
                bool bite = doMove(d);
                p = recomputeProgress();
                if (bite) break; // intentional bite succeeded; replan from new state
            }
            continue;
        }

        // 4) Last-resort fallback: choose a food-avoiding move that keeps freedom
        auto [hr, hc] = body.front();
        pair<int,int> neck = (body.size() >= 2 ? body[1] : make_pair(-1, -1));
        int chosen = -1;
        int bestScore = -1;

        for (int d = 0; d < 4; d++) {
            int nr = hr + DR[d], nc = hc + DC[d];
            if (!inBounds(nr, nc) || make_pair(nr, nc) == neck) continue;

            // Prefer empty cells
            int score = 0;
            if (grid[nr][nc] == 0) score += 1000;

            // Prefer moves near target foods
            auto foods = findFoodsOfColor(targetColor);
            int nearFood = INT_MAX;
            for (auto f : foods) {
                nearFood = min(nearFood, abs(f.first - nr) + abs(f.second - nc));
            }
            if (nearFood != INT_MAX) score -= nearFood * 10;

            // Prefer staying in bounds with more local freedom
            for (int dd = 0; dd < 4; dd++) {
                int rr = nr + DR[dd], cc = nc + DC[dd];
                if (inBounds(rr, cc)) score++;
            }

            if (score > bestScore) {
                bestScore = score;
                chosen = d;
            }
        }

        if (chosen == -1 || stuckCnt++ >= 500) break;
        doMove(chosen);
        p = recomputeProgress();
    }

    for (char c : moves) {
        cout << c << '\n';
    }
    return 0;
}