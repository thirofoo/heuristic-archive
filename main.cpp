#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
#define rep(i, n) for(int i = 0; i < (int)(n); i++)

namespace utility {
  struct timer {
    chrono::system_clock::time_point start;
    void CodeStart() { start = chrono::system_clock::now(); }
    double elapsed() const {
      using namespace std::chrono;
      return (double) duration_cast<milliseconds>(system_clock::now() - start).count();
    }
  } mytm;
}

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty, ty = tz, tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline double rand_double() {
  return (double) (rand_int() % (int) 1e9) / 1e9;
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
  return exp((double) (now - best) / temp(start));
}

struct Trace {
  char op;
  int parend_id;
  Trace(): op('S'), parend_id(0) {}
  explicit Trace(char _op, int _parend_id): op(_op), parend_id(_parend_id) {}
};
vector<Trace> traces;
string restore(int best_id, int initial_id) {
  string ops;
  int current_id = best_id;
  while(current_id != initial_id) {
    ops += traces[current_id].op;
    current_id = traces[current_id].parend_id;
  }
  reverse(ops.begin(), ops.end());
  return ops;
}

struct State {
  int id;
  long long score;
  State(): score(0LL) { id = generate_id(); }
  explicit State(long long _score): score(_score) { id = generate_id(); }
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
  vector<string> vwall; // v[i][j] : between (i,j) and (i,j+1), length N-1, rows N
  vector<string> hwall; // h[i][j] : between (i,j) and (i+1,j), length N,   rows N-1
  vector<pair<int,int>> targets;

  // 出力用
  int C = 1;
  int Q = 1;
  int M = 0;
  vector<vector<int>> s;   // N x N, all 0
  vector<tuple<int,int,int,int,char>> rules; // (c,q,A,S,D)

  // 作る移動列
  string path;

  Solver() { input(); }

  void input() {
    cin >> N >> K >> T;
    vwall.resize(N);
    rep(i, N) {
      string t; cin >> t; // length N-1
      vwall[i] = t;
    }
    hwall.resize(N-1);
    rep(i, N-1) {
      string t; cin >> t; // length N
      hwall[i] = t;
    }
    targets.resize(K);
    rep(i, K) {
      int x, y; cin >> x >> y;
      targets[i] = {x, y};
    }
  }

  // (sx,sy) -> (tx,ty) の最短路（U/D/L/R）をBFSで返す
  string bfs_path(pair<int,int> s, pair<int,int> t) {
    if (s == t) return "";
    const int INF = 1e9;
    vector<vector<int>> dist(N, vector<int>(N, INF));
    vector<vector<pair<int,int>>> par(N, vector<pair<int,int>>(N, {-1,-1}));
    vector<vector<char>> how(N, vector<char>(N, '?'));
    queue<pair<int,int>> q;
    auto push = [&](int x, int y, int px, int py, char c, int nd){
      if (dist[x][y] > nd) {
        dist[x][y] = nd;
        par[x][y] = {px, py};
        how[x][y] = c;
        q.push({x,y});
      }
    };

    auto can_up = [&](int i, int j){ return i > 0 && hwall[i-1][j] == '0'; };
    auto can_down = [&](int i, int j){ return i+1 < N && hwall[i][j] == '0'; };
    auto can_left = [&](int i, int j){ return j > 0 && vwall[i][j-1] == '0'; };
    auto can_right = [&](int i, int j){ return j+1 < N && vwall[i][j] == '0'; };

    dist[s.first][s.second] = 0;
    q.push(s);

    while(!q.empty()){
      auto [x,y] = q.front(); q.pop();
      int d = dist[x][y];
      if (make_pair(x,y) == t) break;

      if (can_up(x,y))    push(x-1,y, x,y, 'U', d+1);
      if (can_down(x,y))  push(x+1,y, x,y, 'D', d+1);
      if (can_left(x,y))  push(x,y-1, x,y, 'L', d+1);
      if (can_right(x,y)) push(x,y+1, x,y, 'R', d+1);
    }

    // 復元
    string res;
    if (dist[t.first][t.second] == INF) {
      // 全到達可能と保証されているので通常ここには来ない
      // 念のため「動かない」で返す（後段で安全に短縮される）
      return res;
    }
    pair<int,int> cur = t;
    while (cur != s) {
      char c = how[cur.first][cur.second];
      res.push_back(c);
      cur = par[cur.first][cur.second];
    }
    reverse(res.begin(), res.end());
    return res;
  }

  void build_path() {
    path.clear();
    for (int i = 0; i+1 < K; i++) {
      auto seg = bfs_path(targets[i], targets[i+1]);
      path += seg;
    }
  }

  void build_rules() {
    // 使う手数（ルール行数）は T に切り詰め（安全策）
    int L = (int)path.size();
    if (L > T) L = T;

    // 色は 1 色（0）で固定
    C = 1;
    // 状態数は「使う最大状態 + 1」。ここでは q=0..L を使い、遷移は 0..L-1 まで出す
    // 制約 Q <= N^4 を守る（N<=20 なので十分余裕）
    long long N4 = 1LL*N*N*N*N;
    Q = (int)min<long long>(N4, (long long)L + 1);

    M = L;
    s.assign(N, vector<int>(N, 0));
    rules.clear();
    for (int t = 0; t < L; t++) {
      // (c=0, q=t) -> A=0, S=t+1, D=path[t]
      rules.emplace_back(0, t, 0, t+1, path[t]);
    }
  }

  void solve() {
    build_path();
    build_rules();
  }

  void output() {
    // 1行目: C Q M
    cout << C << " " << Q << " " << M << "\n";
    // N 行: 初期色
    rep(i, N) {
      rep(j, N) {
        if (j) cout << ' ';
        cout << s[i][j];
      }
      cout << "\n";
    }
    // M 行: c q A S D
    for (auto &r : rules) {
      int c, q, A, S; char D;
      tie(c, q, A, S, D) = r;
      cout << c << " " << q << " " << A << " " << S << " " << D << "\n";
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
