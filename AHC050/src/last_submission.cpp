#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;

constexpr int N  = 40;
constexpr int SZ = N * N;
constexpr int BEAM_W = 16;
#define rep(i, n) for (int i = 0; i < (n); ++i)

inline unsigned int rnd() {
  static unsigned int x = 123456789, y = 362436069, z = 521288629, w = 88675123;
  unsigned int t = x ^ (x << 11);
  x = y; y = z; z = w;
  return w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
}

struct Timer {
  chrono::steady_clock::time_point st;
  void start() { st = chrono::steady_clock::now(); }
  double ms() const {
    return chrono::duration_cast<chrono::milliseconds>(
             chrono::steady_clock::now() - st).count();
  }
} g_t;

class Runner {
public:
  Runner(const vector<string>& S_, const vector<pair<int, int>>& emp_)
      : S(S_), empty(emp_) {}
  pair<double, vector<pair<int, int>>> run(int base_pool);

private:
  const vector<string>& S;
  const vector<pair<int, int>>& empty;
  array<bool, SZ> rock{};
  array<int, SZ> up_st, down_st, left_st, right_st;
  array<double, SZ> P[2];
  vector<int> active_cur, active_nxt;
  array<int, SZ> mark{};
  int stamp = 1;
  vector<int> alive, pos_in_alive;
  static inline int id(int r, int c) { return r * N + c; }
  static inline int row(int x) { return x / N; }
  static inline int col(int x) { return x % N; }
  void buildStops();
  void updateStops(int r, int c);
  void sparseDiffuse(int cur, int nxt, vector<int>& src, vector<int>& dst);
};

struct Solver {
  int M;
  vector<string> board;
  vector<pair<int, int>> empty;
  Solver() {
    int dummy;
    cin >> dummy >> M;
    board.resize(N);
    rep(i, N) {
      cin >> board[i];
      rep(j, N) if (board[i][j] == '.') empty.emplace_back(i, j);
    }
  }
  void solve() {
    g_t.start();
    Runner runner(board, empty);
    double bestE = -1;
    vector<pair<int, int>> bestP;
    long long iter = 0;
    while (g_t.ms() < 1970) {
      auto [E, P] = runner.run(100);
      if (E > bestE) {
        bestE = E;
        bestP = move(P);
      }
      ++iter;
    }
    cerr << "Iter " << iter << '\n';
    for (auto [r, c] : bestP) cout << r << ' ' << c << '\n';
  }
};

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);
  cout.setf(ios::fixed);
  cout.precision(12);
  Solver().solve();
}

pair<double, vector<pair<int, int>>> Runner::run(int base_pool) {
  rock.fill(false);
  P[0].fill(0);
  P[1].fill(0);
  active_cur.clear();
  active_nxt.clear();
  rep(r, N) rep(c, N) if (S[r][c] == '#') rock[id(r, c)] = true;
  if (!empty.empty()) {
    double p0 = 1.0 / empty.size();
    for (auto [r, c] : empty) {
      int d = id(r, c);
      P[0][d] = p0;
      active_cur.push_back(d);
    }
  }
  buildStops();
  alive.resize(empty.size());
  pos_in_alive.resize(empty.size());
  iota(alive.begin(), alive.end(), 0);
  iota(pos_in_alive.begin(), pos_in_alive.end(), 0);
  vector<pair<int, int>> placed;
  placed.reserve(empty.size());
  double expect = 0, prAlive = 1;
  int cur = 0, nxt = 1;
  rep(step, (int)empty.size()) {
    int remain_step = (int)empty.size() - step;
    int pool = 1 + remain_step * (base_pool - 1) / (int)empty.size();
    sparseDiffuse(cur, nxt, active_cur, active_nxt);
    swap(cur, nxt);
    swap(active_cur, active_nxt);
    double minp = 2.0;
    vector<int> cand;
    cand.reserve(32);
    for (int ei : alive) {
      auto [r, c] = empty[ei];
      double p = P[cur][id(r, c)];
      if (p < minp - 1e-12) {
        minp = p;
        cand.clear();
        cand.push_back(ei);
      } else if (abs(p - minp) < 1e-12) cand.push_back(ei);
    }
    struct Node {
      double score;
      int ei;
    };
    vector<Node> list;
    list.reserve(cand.size());
    for (int ei : cand) {
      auto [r, c] = empty[ei];
      int d = id(r, c);
      int upL = r - row(up_st[d]), dnL = row(down_st[d]) - r;
      int lfL = c - col(left_st[d]), rtL = col(right_st[d]) - c;
      double pen = 1.0 * upL * dnL + 1.0 * lfL * rtL;
      double destroyed = P[cur][d];
      double remainProb = 1.0 - destroyed;
      double spread = active_cur.size();
      if (remainProb > 1e-12) spread *= remainProb;
      else pen -= 1000;
      list.push_back({pen + 0.01 * spread, ei});
    }
    int k = min(pool, (int)list.size());
    nth_element(list.begin(), list.begin() + k, list.end(),
                [](auto& a, auto& b) { return a.score < b.score; });
    int choose = list[rnd() % k].ei;
    auto [br, bc] = empty[choose];
    int idPl = id(br, bc);
    placed.push_back({br, bc});
    int pos = find(alive.begin(), alive.end(), choose) - alive.begin();
    alive[pos] = alive.back();
    alive.pop_back();
    double destr = P[cur][idPl];
    P[cur][idPl] = 0;
    double remainProb = 1.0 - destr;
    if (remainProb > 1e-12) {
      double inv = 1.0 / remainProb;
      for (int idv : active_cur) P[cur][idv] *= inv;
    }
    prAlive *= remainProb;
    expect += prAlive;
    rock[idPl] = true;
    updateStops(br, bc);
  }
  return {expect, move(placed)};
}

void Runner::sparseDiffuse(int cur, int nxt, vector<int>& src, vector<int>& dst) {
  dst.clear();
  ++stamp;
  fill(P[nxt].begin(), P[nxt].end(), 0);
  auto push = [&](int to, double v) {
    if (mark[to] != stamp) {
      mark[to] = stamp;
      dst.push_back(to);
    }
    P[nxt][to] += v;
  };
  for (int idv : src) {
    double p = P[cur][idv];
    if (p <= 1e-12) continue;
    double q = 0.25 * p;
    push(up_st[idv], q);
    push(down_st[idv], q);
    push(left_st[idv], q);
    push(right_st[idv], q);
    P[cur][idv] = 0;
  }
}

void Runner::buildStops() {
  rep(r, N) {
    int last = -1;
    rep(c, N) {
      left_st[id(r, c)] = id(r, last + 1);
      if (rock[id(r, c)]) last = c;
    }
    last = N;
    for (int c = N - 1; c >= 0; --c) {
      right_st[id(r, c)] = id(r, last - 1);
      if (rock[id(r, c)]) last = c;
    }
  }
  rep(c, N) {
    int last = -1;
    rep(r, N) {
      up_st[id(r, c)] = id(last + 1, c);
      if (rock[id(r, c)]) last = r;
    }
    last = N;
    for (int r = N - 1; r >= 0; --r) {
      down_st[id(r, c)] = id(last - 1, c);
      if (rock[id(r, c)]) last = r;
    }
  }
}

void Runner::updateStops(int r, int c) {
  for (int i = r - 1; i >= 0 && !rock[id(i, c)]; --i)
    down_st[id(i, c)] = id(r - 1, c);
  for (int i = r + 1; i < N && !rock[id(i, c)]; ++i)
    up_st[id(i, c)] = id(r + 1, c);
  for (int j = c - 1; j >= 0 && !rock[id(r, j)]; --j)
    right_st[id(r, j)] = id(r, c - 1);
  for (int j = c + 1; j < N && !rock[id(r, j)]; ++j)
    left_st[id(r, j)] = id(r, c + 1);
}
