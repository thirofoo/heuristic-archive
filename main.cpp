#include <bits/stdc++.h>
using namespace std;

#define rep(i,n) for (int i = 0; i < (int)(n); i++)

struct Rule {
  int c, q, A, S;
  char D;
};

struct AutomatonSolution {
  int C = 1;
  int Q = 1;
  vector<vector<int>> s;
  vector<Rule> rules;

  int score() const {
    return C + Q;
  }
};

struct Solver {
  int N, K, T;
  vector<string> vwall; // v[i][j]: (i,j)-(i,j+1) 間の壁 (0/1)
  vector<string> hwall; // h[i][j]: (i,j)-(i+1,j) 間の壁 (0/1)
  vector<pair<int,int>> targets;

  mt19937_64 rng;

  Solver() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    rng.seed(chrono::steady_clock::now().time_since_epoch().count());
    input();
  }

  void input() {
    cin >> N >> K >> T;
    vwall.resize(N);
    rep(i, N) cin >> vwall[i];
    hwall.resize(N-1);
    rep(i, N-1) cin >> hwall[i];
    targets.resize(K);
    rep(i, K) {
      int x, y;
      cin >> x >> y;
      targets[i] = {x, y};
    }
  }

  inline bool can_up(int i, int j) const {
    return (i > 0 && hwall[i-1][j] == '0');
  }
  inline bool can_down(int i, int j) const {
    return (i+1 < N && hwall[i][j] == '0');
  }
  inline bool can_left(int i, int j) const {
    return (j > 0 && vwall[i][j-1] == '0');
  }
  inline bool can_right(int i, int j) const {
    return (j+1 < N && vwall[i][j] == '0');
  }

  // dirOrder: {0,1,2,3} の並び替え
  // 0=U,1=D,2=L,3=R
  vector<pair<int,int>> bfs_path_with_order(pair<int,int> from,
                                            pair<int,int> to,
                                            const array<int,4>& dirOrder) {
    const int INF = 1e9;
    vector<vector<int>> dist(N, vector<int>(N, INF));
    vector<vector<pair<int,int>>> par(
      N, vector<pair<int,int>>(N, make_pair(-1, -1))
    );
    queue<pair<int,int>> q;
    auto [sx, sy] = from;
    auto [gx, gy] = to;
    dist[sx][sy] = 0;
    q.push({sx, sy});

    while (!q.empty()) {
      auto [x, y] = q.front(); q.pop();
      int d = dist[x][y];
      if (x == gx && y == gy) break;

      for (int k = 0; k < 4; k++) {
        int code = dirOrder[k];
        int nx = x, ny = y;
        bool ok = false;
        if (code == 0) { // U
          if (can_up(x,y)) { nx = x-1; ny = y; ok = true; }
        } else if (code == 1) { // D
          if (can_down(x,y)) { nx = x+1; ny = y; ok = true; }
        } else if (code == 2) { // L
          if (can_left(x,y)) { nx = x; ny = y-1; ok = true; }
        } else if (code == 3) { // R
          if (can_right(x,y)) { nx = x; ny = y+1; ok = true; }
        }
        if (!ok) continue;
        if (dist[nx][ny] > d+1) {
          dist[nx][ny] = d+1;
          par[nx][ny] = {x, y};
          q.push({nx, ny});
        }
      }
    }

    vector<pair<int,int>> path;
    int x = to.first, y = to.second;
    path.push_back({x, y});
    while (!(x == from.first && y == from.second)) {
      auto p = par[x][y];
      if (p.first == -1) {
        // あり得ないはずだが保険
        path.clear();
        path.push_back(from);
        break;
      }
      x = p.first;
      y = p.second;
      path.push_back({x, y});
    }
    reverse(path.begin(), path.end());
    return path;
  }

  // from->to の最短経路を固定順 (U,D,L,R) で
  vector<pair<int,int>> bfs_path(pair<int,int> from, pair<int,int> to) {
    array<int,4> order = {0,1,2,3};
    return bfs_path_with_order(from, to, order);
  }

  string cells_to_dirs(const vector<pair<int,int>>& cells) {
    string res;
    for (int i = 0; i+1 < (int)cells.size(); i++) {
      auto [x1,y1] = cells[i];
      auto [x2,y2] = cells[i+1];
      if (x2 == x1-1 && y2 == y1) res.push_back('U');
      else if (x2 == x1+1 && y2 == y1) res.push_back('D');
      else if (x2 == x1 && y2 == y1-1) res.push_back('L');
      else if (x2 == x1 && y2 == y1+1) res.push_back('R');
      else res.push_back('S'); // 本来は起きない
    }
    return res;
  }

  // need 個の状態を (C,Q) に分解 (C*Q>=need かつ C+Q 最小)
  pair<int,int> choose_CQ(long long need) {
    long long N4 = 1LL * N * N * N * N;
    int bestC = 1;
    int bestQ = (int)need;
    int bestScore = bestC + bestQ;
    for (int Ccand = 1; 1LL * Ccand * Ccand <= need && 1LL * Ccand <= N4; Ccand++) {
      long long Qll = (need + Ccand - 1) / Ccand;
      if (Qll > N4) continue;
      int Qcand = (int)Qll;
      int score = Ccand + Qcand;
      if (score < bestScore) {
        bestScore = score;
        bestC = Ccand;
        bestQ = Qcand;
      }
    }
    return {bestC, bestQ};
  }

  // Q=1 色圧縮方式でオートマトン構成
  AutomatonSolution build_color_automaton(const vector<pair<int,int>>& pathCells,
                                          const string& moves) {
    AutomatonSolution sol;
    int L = (int)moves.size();
    if (K <= 1) {
      sol.C = 1;
      sol.Q = 1;
      sol.s.assign(N, vector<int>(N, 0));
      sol.rules.clear();
      sol.rules.push_back({0, 0, 0, 0, 'S'});
      return sol;
    }

    int totalCells = N * N;
    auto vid = [&](int x, int y){ return x * N + y; };

    vector<vector<int>> visits(totalCells);
    for (int t = 0; t < L; t++) {
      auto [x,y] = pathCells[t];
      int id = vid(x,y);
      visits[id].push_back(t);
    }

    vector<int> stepColor(L, 0);
    vector<int> cellStage0Color(totalCells, 0);

    unordered_map<long long,int> mp;
    mp.reserve(L * 2 + 10);

    auto dir_index = [&](char d) {
      if (d == 'U') return 0;
      if (d == 'D') return 1;
      if (d == 'L') return 2;
      if (d == 'R') return 3;
      return 4;
    };

    vector<int> colorDir;   // dirIndex
    vector<int> colorNext;  // nextColor

    colorDir.push_back(4);  // color 0: dummy
    colorNext.push_back(0);

    int colorCount = 1;
    vector<int> tmpStages;

    for (int id = 0; id < totalCells; id++) {
      auto &v = visits[id];
      int m = (int)v.size();
      if (m == 0) continue;

      tmpStages.assign(m, 0);
      int nextColor = 0; // 最後のステージの次色は 0

      for (int j = m-1; j >= 0; j--) {
        int t = v[j];
        char d = moves[t];
        int di = dir_index(d);
        if (di >= 4) di = 4;
        long long key = ( (long long)di << 32 ) | (unsigned long long)nextColor;
        int col;
        auto it = mp.find(key);
        if (it != mp.end()) {
          col = it->second;
        } else {
          col = colorCount++;
          mp[key] = col;
          colorDir.push_back(di);
          colorNext.push_back(nextColor);
        }
        tmpStages[j] = col;
        stepColor[t] = col;
        nextColor = col;
      }
      cellStage0Color[id] = tmpStages[0];
    }

    sol.C = colorCount;
    sol.Q = 1;

    sol.s.assign(N, vector<int>(N, 0));
    rep(x, N) rep(y, N) {
      int id = vid(x,y);
      sol.s[x][y] = cellStage0Color[id];
    }

    sol.rules.clear();
    sol.rules.reserve(sol.C);

    // 色0: とりあえずその場に留まる
    sol.rules.push_back({0, 0, 0, 0, 'S'});

    for (int c = 1; c < sol.C; c++) {
      int di = colorDir[c];
      int nc = colorNext[c];
      char Dch = 'S';
      if (di == 0) Dch = 'U';
      else if (di == 1) Dch = 'D';
      else if (di == 2) Dch = 'L';
      else if (di == 3) Dch = 'R';
      sol.rules.push_back({c, 0, nc, 0, Dch});
    }

    return sol;
  }

  // タイムライン方式（時刻ごとに固有状態）でオートマトン構成
  AutomatonSolution build_timeline_automaton(const vector<pair<int,int>>& pathCells,
                                             const string& moves) {
    AutomatonSolution sol;
    int L = (int)moves.size();
    if (K <= 1) {
      sol.C = 1;
      sol.Q = 1;
      sol.s.assign(N, vector<int>(N, 0));
      sol.rules.clear();
      sol.rules.push_back({0, 0, 0, 0, 'S'});
      return sol;
    }

    int totalCells = N * N;
    auto vid = [&](int x, int y){ return x * N + y; };

    vector<int> firstVisit(totalCells, -1);
    vector<int> lastVisit(totalCells, -1);
    vector<int> nextVisit(L+1, -1);

    for (int t = 0; t <= L; t++) {
      auto [x,y] = pathCells[t];
      int id = vid(x,y);
      if (firstVisit[id] == -1) firstVisit[id] = t;
      if (lastVisit[id] != -1) {
        nextVisit[lastVisit[id]] = t;
      }
      lastVisit[id] = t;
    }

    auto cq = choose_CQ((long long)L + 1);
    sol.C = cq.first;
    sol.Q = cq.second;

    vector<int> ct(L+1), qt(L+1);
    for (int t = 0; t <= L; t++) {
      long long idt = t;
      ct[t] = (int)(idt / sol.Q);
      qt[t] = (int)(idt % sol.Q);
    }

    sol.s.assign(N, vector<int>(N, 0));
    rep(x, N) rep(y, N) {
      int id = vid(x,y);
      int t0 = firstVisit[id];
      if (t0 != -1) sol.s[x][y] = ct[t0];
      else sol.s[x][y] = 0;
    }

    vector<int> At(L), St(L);
    for (int t = 0; t < L; t++) {
      int nxt = nextVisit[t];
      if (nxt != -1) At[t] = ct[nxt];
      else At[t] = 0;
      St[t] = qt[t+1];
    }

    sol.rules.clear();
    sol.rules.reserve(L);
    for (int t = 0; t < L; t++) {
      sol.rules.push_back({ct[t], qt[t], At[t], St[t], moves[t]});
    }

    return sol;
  }

  // fixed 順序 (U,D,L,R) で最初のパス作成
  void build_initial_path(vector<pair<int,int>>& pathCells, string& moves) {
    pathCells.clear();
    pathCells.push_back(targets[0]);
    moves.clear();
    for (int i = 0; i+1 < K; i++) {
      auto seg = bfs_path(targets[i], targets[i+1]);
      string segm = cells_to_dirs(seg);
      for (int k = 1; k < (int)seg.size(); k++) {
        pathCells.push_back(seg[k]);
      }
      moves += segm;
    }
  }

  // ランダムな順序で最短路パス生成
  void build_random_path(vector<pair<int,int>>& pathCells, string& moves) {
    array<int,4> order = {0,1,2,3};
    shuffle(order.begin(), order.end(), rng);

    pathCells.clear();
    pathCells.push_back(targets[0]);
    moves.clear();
    for (int i = 0; i+1 < K; i++) {
      auto seg = bfs_path_with_order(targets[i], targets[i+1], order);
      string segm = cells_to_dirs(seg);
      for (int k = 1; k < (int)seg.size(); k++) {
        pathCells.push_back(seg[k]);
      }
      moves += segm;
    }
  }

  void solve() {
    // K<=1 は即終了で良い
    if (K <= 1) {
      AutomatonSolution sol;
      sol.C = 1;
      sol.Q = 1;
      sol.s.assign(N, vector<int>(N, 0));
      sol.rules.clear();
      sol.rules.push_back({0, 0, 0, 0, 'S'});
      output(sol);
      return;
    }

    vector<pair<int,int>> basePath;
    string baseMoves;
    build_initial_path(basePath, baseMoves);
    int L = (int)baseMoves.size();
    // 問題の保証から L <= T

    // ベースライン：色圧縮方式とタイムライン方式の両方を作る
    AutomatonSolution best = build_color_automaton(basePath, baseMoves);
    AutomatonSolution timeSol = build_timeline_automaton(basePath, baseMoves);
    if (timeSol.score() < best.score()) {
      best = timeSol;
    }

    // 時間いっぱいまでランダムな BFS パスを試す
    using Clock = chrono::steady_clock;
    auto startTime = Clock::now();
    const double TIME_LIMIT = 1.9;

    vector<pair<int,int>> pathCells;
    string moves;

    int iter = 0;
    while (true) {
      auto now = Clock::now();
      double elapsed = chrono::duration<double>(now - startTime).count();
      if (elapsed > TIME_LIMIT) break;

      iter++;

      build_random_path(pathCells, moves);
      // 最短路を使っているので長さは常に L
      AutomatonSolution cand = build_color_automaton(pathCells, moves);
      if (cand.score() < best.score()) {
        best = cand;
      }
    }

    output(best);
  }

  void output(const AutomatonSolution& sol) {
    int C = sol.C;
    int Q = sol.Q;
    int M = (int)sol.rules.size();

    cout << C << " " << Q << " " << M << "\n";
    rep(i, N) {
      rep(j, N) {
        if (j) cout << ' ';
        cout << sol.s[i][j];
      }
      cout << "\n";
    }
    for (auto &r : sol.rules) {
      cout << r.c << " " << r.q << " " << r.A << " " << r.S << " " << r.D << "\n";
    }
  }
};

int main() {
  Solver solver;
  solver.solve();
  return 0;
}
