#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
#define rep(i, n) for (int i = 0; i < (int)(n); i++)

namespace utility {
  struct timer {
    chrono::system_clock::time_point start;
    void CodeStart() { start = chrono::system_clock::now(); }
    double elapsed() const {
      using namespace std::chrono;
      return (double)chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start
      ).count();
    }
  } mytm;
}

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty; ty = tz; tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline double rand_double() {
  return (double)(rand_int() % (int)1e9) / 1e9;
}

inline double gaussian(double mean, double stddev) {
  double z0 = sqrt(-2.0 * log(rand_double())) * cos(2.0 * M_PI * rand_double());
  return mean + z0 * stddev;
}

#define TIME_LIMIT 2950
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}
inline double prob(int best, int now, int start) {
  return exp((double)(now - best) / temp(start));
}

// テンプレのまま残してあるが今回ロジックには使っていない
struct Trace {
  char op;
  int parend_id;
  Trace() : op('S'), parend_id(0) {}
  explicit Trace(char _op, int _parend_id) : op(_op), parend_id(_parend_id) {}
};
vector<Trace> traces;
string restore(int best_id, int initial_id) {
  string ops;
  int current_id = best_id;
  while (current_id != initial_id) {
    ops += traces[current_id].op;
    current_id = traces[current_id].parend_id;
  }
  reverse(ops.begin(), ops.end());
  return ops;
}

struct State {
  int id;
  long long score;
  State() : id(generate_id()), score(0LL) {}
  explicit State(long long _score) : id(generate_id()), score(_score) {}
  bool operator<(const State& other) const { return score < other.score; }
  bool operator>(const State& other) const { return score > other.score; }
private:
  static int generate_id() {
    static int id_counter = 0;
    return id_counter++;
  }
};

struct Solver {
  int N, K, T;
  vector<string> vwall;   // v[i][j]: (i,j)-(i,j+1) に壁があるか
  vector<string> hwall;   // h[i][j]: (i,j)-(i+1,j) に壁があるか
  vector<pair<int,int>> targets;

  int C, Q, M;
  vector<vector<int>> s;

  struct Rule {
    int c, q, A, S;
    char D;
  };
  vector<Rule> rules;

  Solver() { input(); }

  void input() {
    cin >> N >> K >> T;
    vwall.resize(N);
    rep(i, N) {
      string t; cin >> t;
      vwall[i] = t;
    }
    hwall.resize(N-1);
    rep(i, N-1) {
      string t; cin >> t;
      hwall[i] = t;
    }
    targets.resize(K);
    rep(i, K) {
      int x, y; cin >> x >> y;
      targets[i] = {x, y};
    }
  }

  // (sx,sy) -> (tx,ty) の最短経路（マス列）を BFS で取得
  vector<pair<int,int>> bfs_cells(pair<int,int> s0, pair<int,int> t0) {
    vector<pair<int,int>> path;
    if (s0 == t0) {
      path.push_back(s0);
      return path;
    }

    const int INF = 1e9;
    vector<vector<int>> dist(N, vector<int>(N, INF));
    vector<vector<pair<int,int>>> par(N, vector<pair<int,int>>(N, {-1,-1}));

    auto can_up = [&](int i, int j) {
      return (i > 0 && hwall[i-1][j] == '0');
    };
    auto can_down = [&](int i, int j) {
      return (i+1 < N && hwall[i][j] == '0');
    };
    auto can_left = [&](int i, int j) {
      return (j > 0 && vwall[i][j-1] == '0');
    };
    auto can_right = [&](int i, int j) {
      return (j+1 < N && vwall[i][j] == '0');
    };

    queue<pair<int,int>> q;
    dist[s0.first][s0.second] = 0;
    q.push(s0);

    while (!q.empty()) {
      auto [x, y] = q.front(); q.pop();
      int d = dist[x][y];
      if (make_pair(x,y) == t0) break;

      if (can_up(x,y) && dist[x-1][y] > d+1) {
        dist[x-1][y] = d+1;
        par[x-1][y] = {x,y};
        q.push({x-1,y});
      }
      if (can_down(x,y) && dist[x+1][y] > d+1) {
        dist[x+1][y] = d+1;
        par[x+1][y] = {x,y};
        q.push({x+1,y});
      }
      if (can_left(x,y) && dist[x][y-1] > d+1) {
        dist[x][y-1] = d+1;
        par[x][y-1] = {x,y};
        q.push({x,y-1});
      }
      if (can_right(x,y) && dist[x][y+1] > d+1) {
        dist[x][y+1] = d+1;
        par[x][y+1] = {x,y};
        q.push({x,y+1});
      }
    }

    if (dist[t0.first][t0.second] == INF) {
      path.push_back(s0);
      return path;
    }

    pair<int,int> cur = t0;
    while (cur != s0) {
      path.push_back(cur);
      cur = par[cur.first][cur.second];
    }
    path.push_back(s0);
    reverse(path.begin(), path.end());
    return path;
  }

  // マス列 -> U/D/L/R
  string cells_to_dirs(const vector<pair<int,int>>& cells) {
    string res;
    for (int i = 0; i+1 < (int)cells.size(); i++) {
      auto [x1,y1] = cells[i];
      auto [x2,y2] = cells[i+1];
      if (x2 == x1-1 && y2 == y1) res.push_back('U');
      else if (x2 == x1+1 && y2 == y1) res.push_back('D');
      else if (x2 == x1 && y2 == y1-1) res.push_back('L');
      else if (x2 == x1 && y2 == y1+1) res.push_back('R');
      else res.push_back('S'); // ありえないはず
    }
    return res;
  }

  void solve() {
    // 1. 全目的地を結ぶ最短経路を作る
    vector<pair<int,int>> pathCells; // 長さ L+1
    string moves;                    // 長さ L

    pathCells.clear();
    pathCells.push_back(targets[0]);
    moves.clear();

    for (int i = 0; i+1 < K; i++) {
      auto seg_cells = bfs_cells(targets[i], targets[i+1]);
      string seg_moves = cells_to_dirs(seg_cells);
      for (int k = 1; k < (int)seg_cells.size(); k++) {
        pathCells.push_back(seg_cells[k]);
      }
      moves += seg_moves;
    }

    int L = (int)moves.size(); // 最短移動回数 X
    if (L == 0) {
      // 動かないケース（ほぼ来ない）
      C = 1;
      Q = 1;
      M = 0;
      s.assign(N, vector<int>(N, 0));
      return;
    }

    // 2. 各マスの訪問時刻列を使って「次に同じマスに来る時刻」を前計算
    int totalCells = N * N;
    vector<int> firstVisit(totalCells, -1);
    vector<int> lastVisit(totalCells, -1);
    vector<int> nextVisit(L+1, -1);

    auto vid = [&](int x, int y) { return x * N + y; };

    for (int t = 0; t <= L; t++) {
      auto [x, y] = pathCells[t];
      int id = vid(x,y);
      if (firstVisit[id] == -1) firstVisit[id] = t;
      if (lastVisit[id] != -1) {
        nextVisit[lastVisit[id]] = t;
      }
      lastVisit[id] = t;
    }

    // 3. C, Q を決める (C*Q >= L+1 かつ C+Q が最小)
    long long need = (long long)L + 1;
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

    C = bestC;
    Q = bestQ;

    // 4. 時刻 t=0..L に一意な (c_t, q_t) を割り当て
    vector<int> ct(L+1), qt(L+1);
    for (int t = 0; t <= L; t++) {
      long long idt = t;
      ct[t] = (int)(idt / Q);   // 0..C-1
      qt[t] = (int)(idt % Q);   // 0..Q-1
    }

    // 5. 初期色 s[i][j]: そのマスの「最初の訪問時刻」の色をセット
    s.assign(N, vector<int>(N, 0));
    rep(x, N) rep(y, N) {
      int id = vid(x,y);
      int t0 = firstVisit[id];
      if (t0 != -1) {
        s[x][y] = ct[t0];
      } else {
        s[x][y] = 0; // 一度も通らないマスは何色でもよい
      }
    }

    // 6. 各ステップ t の A_t, S_t を決める
    vector<int> At(L), St(L);
    for (int t = 0; t < L; t++) {
      int nxt = nextVisit[t];
      if (nxt != -1) {
        At[t] = ct[nxt];  // 次回訪問時刻の色にしておく
      } else {
        At[t] = 0;        // もう使わないマスは適当でよい
      }
      St[t] = qt[t+1];    // 状態は次の時刻の状態へ
    }

    // 7. 遷移規則を作る
    rules.clear();
    rules.reserve(L);
    for (int t = 0; t < L; t++) {
      int c0 = ct[t];
      int q0 = qt[t];
      int A0 = At[t];
      int S0 = St[t];
      char D0 = moves[t];
      rules.push_back({c0, q0, A0, S0, D0});
    }

    M = (int)rules.size();
    // L = X ≤ T なので M = L ≤ T
  }

  void output() {
    cout << C << " " << Q << " " << M << "\n";
    rep(i, N) {
      rep(j, N) {
        if (j) cout << ' ';
        cout << s[i][j];
      }
      cout << "\n";
    }
    for (auto &r : rules) {
      cout << r.c << " " << r.q << " " << r.A << " " << r.S << " " << r.D << "\n";
    }
  }
};

int main() {
  cin.tie(nullptr);
  ios::sync_with_stdio(false);

  Solver solver;
  solver.solve();
  solver.output();
  return 0;
}
