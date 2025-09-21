#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
#define rep(i, n) for(int i = 0; i < n; i++)

namespace utility {
  struct timer {
  chrono::system_clock::time_point start;
  // 開始時間を記録
  void CodeStart() {
    start = chrono::system_clock::now();
  }
  // 経過時間 (ms) を返す
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

// 温度関数
#define TIME_LIMIT 2950
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now, int start) {
  return exp((double) (now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//
using P = pair<int, int>;

// right | down | left | up
#define DIR_NUM 4
vector<P> arr4 = {P(0, 1), P(1, 0), P(0, -1), P(-1, 0)};

// C 型配置全列挙
static int POS_SIZE = 8;
struct C_shape {
  vector<P> ok_positions, ng_positions;
};

static vector<C_shape> C_positions = {
  /*
      .#.
      ..#
      #x#
      .#.
  */
  C_shape{
    .ok_positions = {P(-2, 0), P(-1, 1), P(0, -1), P(0, 1), P(1, 0)},
    .ng_positions = {P(-1, 0), P(0, 0), P(-1, -1)}
  },

  /*
      .#.
      #x#
      ..#
      .#.
  */
  C_shape{
    .ok_positions = {P(-1, 0), P(0, -1), P(0, 1), P(1, 1), P(2, 0)},
    .ng_positions = {P(0, 0), P(1, 0), P(1, -1)}
  },


  /*
      .##.
      #x.#
      .#..
  */
  C_shape{
    .ok_positions = {P(-1, 0), P(0, -1), P(0, 2), P(1, 0), P(-1, 1)},
    .ng_positions = {P(0, 0), P(0, 1), P(1, 1)}
  },

  /*
      .##.
      #.x#
      ..#.
  */
  C_shape{
    .ok_positions = {P(-1, -1), P(-1, 0), P(0, -2), P(0, 1), P(1, 0)},
    .ng_positions = {P(0, 0), P(0, -1), P(1, -1)}
  },

  /*
      .#.
      #..
      #x#
      .#.
  */
  C_shape{
    .ok_positions = {P(-2, 0), P(-1, -1), P(0, -1), P(0, 1), P(1, 0)},
    .ng_positions = {P(-1, 0), P(0, 0), P(-1, 1)}
  },

  /*
      .#.
      #x#
      #..
      .#.
  */
  C_shape{
    .ok_positions = {P(-1, 0), P(0, -1), P(0, 1), P(1, -1), P(2, 0)},
    .ng_positions = {P(0, 0), P(1, 0), P(1, 1)}
  },


  /*
      .#..
      #x.#
      .##.
  */
  C_shape{
    .ok_positions = {P(-1, 0), P(0, -1), P(0, 2), P(1, 0), P(1, 1)},
    .ng_positions = {P(0, 0), P(0, 1), P(-1, 1)}
  },

  /*
      ..#.
      #.x#
      .##.
  */
  C_shape{
    .ok_positions = {P(1, -1), P(-1, 0), P(0, -2), P(0, 1), P(1, 0)},
    .ng_positions = {P(0, 0), P(0, -1), P(-1, -1)}
  },
};

inline bool outField(P now, int h, int w) {
  auto &&[x, y] = now;
  if(0 <= x && x < h && 0 <= y && y < w) return false;
  return true;
}

struct Solver {
  int N;
  vector<string> initial_board;
  P flower_pos;

  Solver() {
    this->input();
  }

  void input() {
    int ti, tj;
    cin >> N >> ti >> tj;
    flower_pos = P(ti, tj);
    initial_board.resize(N);
    rep(i, N) cin >> initial_board[i];
    return;
  }

  void output() {
    vector<P> C_list;
    rep(i, N) {
      rep(j, N) {
        if(initial_board[i][j] == 'C') {
          C_list.emplace_back(i, j);
        }
      }
    }
    cout << C_list.size() << " ";
    for(const auto& [x, y] : C_list) cout << x << " " << y << " ";
    cout << endl << flush;

    int pi, pj, next;
    cin >> pi >> pj >> next;
    rep(i, next) {
      int x, y;
      cin >> x >> y;
    }
    
    cout << -1 << endl << flush;
    return;
  }

  queue<P> que;
  vector<P> visitable_positions;
  inline bool can_place_C(int x, int y, const vector<P>& ng_positions, const vector<P>& ok_positions) {
    // 通り道を塞がれていない判定
    for(const auto& [dx, dy] : ng_positions) {
      int nx = x + dx, ny = y + dy;
      if(outField(P(nx, ny), N, N) || initial_board[nx][ny] != '.') return false;
    }

    // 花の位置 or start 位置は避ける判定
    for(const auto& [dx, dy] : ok_positions) {
      int nx = x + dx, ny = y + dy;
      if(nx == flower_pos.first && ny == flower_pos.second) return false;
      if(nx == 0 && ny == N / 2) return false;
    }

    // visitable_positions ⇔ (0, N / 2) 間に道がある判定
    visitable_positions.push_back(P(x, y));
    dsu uf(N * N);
    bool ng;
    rep(i, N) {
      rep(j, N) {
        if(initial_board[i][j] != '.') continue;
        ng = false;
        for(const auto& [dx, dy] : ok_positions) {
          if(i == x + dx && j == y + dy) ng = true;
        }
        if(ng) continue;

        for(const auto& [dx, dy] : arr4) {
          int ni = i + dx, nj = j + dy;
          ng = outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.';
          for(const auto& [odx, ody] : ok_positions) {
            if(ni == x + odx && nj == y + ody) ng = true;
          }
          if(ng) continue;
          uf.merge(i * N + j, ni * N + nj);
        }
      }
    }
    for(const auto& [vx, vy] : visitable_positions) {
      if(uf.same(vx * N + vy, 0 * N + N / 2)) continue;
      visitable_positions.pop_back();
      return false;
    }
    return true;
  }

  void solve() {
    // まず花の周りに C を配置する
    auto [fx, fy] = flower_pos;

    int initial_start_idx;
    if(fy < N / 2) initial_start_idx = 0;
    else initial_start_idx = 4;

    rep(i, POS_SIZE) {
      const auto& pos = C_positions[(initial_start_idx + i) % POS_SIZE];
      if(!can_place_C(fx, fy, pos.ng_positions, pos.ok_positions)) continue;
      for(const auto& [dx, dy] : pos.ok_positions) {
        int ni = fx + dx, nj = fy + dy;
        if(outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.') continue;
        initial_board[ni][nj] = 'C';
      }
      break;
    }

    // 他の場所に出来るだけ C を配置する
    rep(i, N) {
      rep(j, N) {
        if(initial_board[i][j] != '.') continue;

        int start_idx;
        if(j < N / 2) start_idx = 0;
        else start_idx = 4;

        rep(k, POS_SIZE) {
          const auto& pos = C_positions[(start_idx + k) % POS_SIZE];
          if(!can_place_C(i, j, pos.ng_positions, pos.ok_positions)) continue;
          for(const auto& [dx, dy] : pos.ok_positions) {
            int ni = i + dx, nj = j + dy;
            if(outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.') continue;
            initial_board[ni][nj] = 'C';
          }
          break;
        }
      }
    }
    return;
  }
};

int main() {
  cin.tie(0);
  ios_base::sync_with_stdio(false);

  Solver solver;
  solver.solve();
  solver.output();

  return 0;
}
