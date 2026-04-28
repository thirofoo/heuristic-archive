#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;

#define rep(i, n) for (int i = 0; i < (n); i++)

namespace utility {
  struct timer {
    chrono::system_clock::time_point start;
    void CodeStart() { start = chrono::system_clock::now(); }
    double elapsed() const {
      using namespace std::chrono;
      return (double)duration_cast<milliseconds>(system_clock::now() - start).count();
    }
  } mytm;
}

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty;
  ty = tz;
  tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline double rand_double() {
  return (double)(rand_int() % (int)1e9) / 1e9;
}

#define TIME_LIMIT 2950
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

inline double prob(int best, int now, int start) {
  return exp((double)(now - best) / temp(start));
}

using P = pair<int, int>;

#define DIR_NUM 4
vector<P> arr4 = { P(0, 1), P(1, 0), P(0, -1), P(-1, 0) };

static int POS_SIZE = 8;

struct C_shape {
  vector<P> ok_positions, ng_positions;
  int sdx, gdx, sdy, gdy;
};

static vector<C_shape> C_positions = {
  C_shape{
    .ok_positions = { P(-2, 0), P(-1, 1), P(0, -1), P(0, 1), P(1, 0) },
    .ng_positions = { P(-1, 0), P(0, 0), P(-1, -1) },
    .sdx = -3, .gdx = 2, .sdy = -2, .gdy = 2
  },
  C_shape{
    .ok_positions = { P(-1, 0), P(0, -1), P(0, 1), P(1, 1), P(2, 0) },
    .ng_positions = { P(0, 0), P(1, 0), P(1, -1) },
    .sdx = -2, .gdx = 3, .sdy = -2, .gdy = 2
  },
  C_shape{
    .ok_positions = { P(-1, 0), P(0, -1), P(0, 2), P(1, 0), P(-1, 1) },
    .ng_positions = { P(0, 0), P(0, 1), P(1, 1) },
    .sdx = -2, .gdx = 2, .sdy = -2, .gdy = 3
  },
  C_shape{
    .ok_positions = { P(-1, -1), P(-1, 0), P(0, -2), P(0, 1), P(1, 0) },
    .ng_positions = { P(0, 0), P(0, -1), P(1, -1) },
    .sdx = -2, .gdx = 2, .sdy = -3, .gdy = 2
  },
  C_shape{
    .ok_positions = { P(-2, 0), P(-1, -1), P(0, -1), P(0, 1), P(1, 0) },
    .ng_positions = { P(-1, 0), P(0, 0), P(-1, 1) },
    .sdx = -3, .gdx = 2, .sdy = -2, .gdy = 2
  },
  C_shape{
    .ok_positions = { P(-1, 0), P(0, -1), P(0, 1), P(1, -1), P(2, 0) },
    .ng_positions = { P(0, 0), P(1, 0), P(1, 1) },
    .sdx = -2, .gdx = 3, .sdy = -2, .gdy = 2
  },
  C_shape{
    .ok_positions = { P(-1, 0), P(0, -1), P(0, 2), P(1, 0), P(1, 1) },
    .ng_positions = { P(0, 0), P(0, 1), P(-1, 1) },
    .sdx = -2, .gdx = 2, .sdy = -2, .gdy = 3
  },
  C_shape{
    .ok_positions = { P(1, -1), P(-1, 0), P(0, -2), P(0, 1), P(1, 0) },
    .ng_positions = { P(0, 0), P(0, -1), P(-1, -1) },
    .sdx = -2, .gdx = 2, .sdy = -3, .gdy = 2
  },
};

inline bool outField(P now, int h, int w) {
  auto&& [x, y] = now;
  return !(0 <= x && x < h && 0 <= y && y < w);
}

struct Solver {
  int N;
  vector<string> initial_board;
  P flower_pos;

  Solver() { input(); }

  void input() {
    int ti, tj;
    cin >> N >> ti >> tj;
    flower_pos = P(ti, tj);
    initial_board.resize(N);
    rep(i, N) cin >> initial_board[i];
  }

  vector<P> C_list;

  void output() {
    rep(i, N) {
      rep(j, N) {
        if (initial_board[i][j] == 'C') C_list.emplace_back(i, j);
      }
    }
    cout << C_list.size() << " ";
    for (const auto& [x, y] : C_list) cout << x << " " << y << " ";
    cout << endl << flush;

    int pi, pj, next;
    cin >> pi >> pj >> next;
    rep(i, next) {
      int x, y;
      cin >> x >> y;
    }
    cout << -1 << endl << flush;
  }

  queue<P> que;
  vector<P> visitable_positions;

  inline bool can_place_C(int x, int y, const vector<P>& ng_positions, const vector<P>& ok_positions) {
    for (const auto& [dx, dy] : ng_positions) {
      int nx = x + dx, ny = y + dy;
      if (outField(P(nx, ny), N, N) || initial_board[nx][ny] != '.') return false;
    }

    for (const auto& [dx, dy] : ok_positions) {
      int nx = x + dx, ny = y + dy;
      if (nx == flower_pos.first && ny == flower_pos.second) return false;
      if (nx == 0 && ny == N / 2) return false;
    }

    if (visitable_positions.empty()) {
      if (initial_board[0][0] == '.') visitable_positions.push_back(P(0, 0));
      if (initial_board[0][N - 1] == '.') visitable_positions.push_back(P(0, N - 1));
      if (initial_board[N - 1][0] == '.') visitable_positions.push_back(P(N - 1, 0));
      if (initial_board[N - 1][N - 1] == '.') visitable_positions.push_back(P(N - 1, N - 1));
      visitable_positions.push_back(flower_pos);
    }
    visitable_positions.push_back(P(x, y));

    dsu uf(N * N);
    bool ng;
    rep(i, N) {
      rep(j, N) {
        if (initial_board[i][j] != '.') continue;
        ng = false;
        for (const auto& [dx, dy] : ok_positions) {
          if (i == x + dx && j == y + dy) ng = true;
        }
        if (ng) continue;

        for (const auto& [dx, dy] : arr4) {
          int ni = i + dx, nj = j + dy;
          ng = outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.';
          for (const auto& [odx, ody] : ok_positions) {
            if (ni == x + odx && nj == y + ody) ng = true;
          }
          if (ng) continue;
          uf.merge(i * N + j, ni * N + nj);
        }
      }
    }

    for (const auto& [vx, vy] : visitable_positions) {
      if (uf.same(vx * N + vy, 0 * N + N / 2)) continue;
      visitable_positions.pop_back();
      return false;
    }
    return true;
  }

  pair<vector<P>, vector<P>> carve_and_get_path(P start_pos, const vector<string>& initial_grid) {
    int h = (int)initial_grid.size();
    int w = (int)initial_grid[0].size();

    vector<string> grid = initial_grid;
    auto inb = [&](int r, int c) { return 0 <= r && r < h && 0 <= c && c < w; };

    int dr[4] = { -1, 0, 1, 0 };
    int dc[4] = { 0, -1, 0, 1 };

    vector<vector<char>> vis(h, vector<char>(w, 0));

    function<bool(int, int)> dfs = [&](int r, int c) {
      vis[r][c] = 1;
      grid[r][c] = '.';

      for (int d = 0; d < 4; d++) {
        int nr = r + dr[d], nc = c + dc[d];
        if (!inb(nr, nc)) continue;
        if (vis[nr][nc]) continue;
        if (grid[nr][nc] == 'T' || grid[nr][nc] == 'C') continue;

        vector<P> changed;

        auto push_wall = [&](int wr, int wc) {
          if (!inb(wr, wc)) return;
          if (vis[wr][wc]) return;

          if (wr == flower_pos.first && wc == flower_pos.second - N / 4) return;
          if (wr == 0 && wc == N / 2 - N / 4) return;

          if (grid[wr][wc] != 'C' && grid[wr][wc] != 'T') {
            grid[wr][wc] = 'T';
            changed.emplace_back(wr, wc);
          }
        };

        for (int i = 0; i < 4; i++) {
          int wr = r + dr[i], wc = c + dc[i];
          if (wr == nr && wc == nc) continue;
          push_wall(wr, wc);
        }

        if (dfs(nr, nc)) return true;

        for (auto& p : changed) grid[p.first][p.second] = '.';
      }

      if (c == w - 1) {
        grid[r][c] = '.';
        return true;
      }

      if (!(flower_pos.first == r && flower_pos.second - N / 4 == c) &&
          !(r == 0 && c == N / 2 - N / 4)) {
        grid[r][c] = 'T';
      }

      vis[r][c] = 0;
      return false;
    };

    if (inb(start_pos.first, start_pos.second) && initial_grid[start_pos.first][start_pos.second] != 'T') {
      dfs(start_pos.first, start_pos.second);
    }

    vector<P> added_t;
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        if (initial_grid[i][j] == '.' && grid[i][j] == 'T') added_t.emplace_back(i, j);
      }
    }

    vector<int> dist(h * w, INT_MAX), par(h * w, -1);
    auto id = [&](int r, int c) { return r * w + c; };

    queue<P> q;
    if (inb(start_pos.first, start_pos.second) && grid[start_pos.first][start_pos.second] != 'T') {
      dist[id(start_pos.first, start_pos.second)] = 0;
      q.push(start_pos);
    }

    while (!q.empty()) {
      auto [r, c] = q.front(); q.pop();
      for (int k = 0; k < 4; k++) {
        int nr = r + dr[k], nc = c + dc[k];
        if (!inb(nr, nc)) continue;
        if (grid[nr][nc] == 'T' || grid[nr][nc] == 'C') continue;
        int u = id(nr, nc), v = id(r, c);
        if (dist[u] != INT_MAX) continue;
        dist[u] = dist[v] + 1;
        par[u] = v;
        q.emplace(nr, nc);
      }
    }

    int best_goal = -1, best_dist = INT_MAX;
    for (int r = 0; r < h; r++) {
      int v = id(r, w - 1);
      if (grid[r][w - 1] != 'T' && grid[r][w - 1] != 'C' && dist[v] < best_dist) {
        best_dist = dist[v];
        best_goal = v;
      }
    }

    vector<P> shortest_path;
    if (best_goal != -1) {
      int cur = best_goal;
      while (cur != -1) {
        int r = cur / w, c = cur % w;
        shortest_path.emplace_back(r, c);
        cur = par[cur];
      }
      reverse(shortest_path.begin(), shortest_path.end());
    }

    return { shortest_path, added_t };
  }

  void solve() {
    auto [fx, fy] = flower_pos;

    int initial_start_idx;
    if (fy < N / 2) initial_start_idx = 0;
    else initial_start_idx = 4;

    int flowe_arr_idx;

    rep(i, POS_SIZE) {
      const auto& pos = C_positions[(initial_start_idx + i) % POS_SIZE];
      if (!can_place_C(fx, fy, pos.ng_positions, pos.ok_positions)) continue;
      for (const auto& [dx, dy] : pos.ok_positions) {
        int ni = fx + dx, nj = fy + dy;
        if (outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.') continue;
        initial_board[ni][nj] = 'C';
        flowe_arr_idx = (initial_start_idx + i) % POS_SIZE;
      }
      break;
    }

    double t_rate = 0.0;
    rep(i, N) rep(j, N) if (initial_board[i][j] == 'T') t_rate += 1.0;
    t_rate /= (double)(N * N);

    bool ng = true;
    if (t_rate < 1) {
      vector<string> temp_board(N, string(3 * N / 4 - N / 4 + 1, '.'));
      for (int i = 0; i < N; i++) {
        for (int j = N / 4; j <= 3 * N / 4; j++) {
          temp_board[i][j - N / 4] = initial_board[i][j];
        }
      }

      auto [path, added_t] = carve_and_get_path(P(0, 0), temp_board);
      for (const auto& [tx, ty] : added_t) {
        initial_board[tx][ty + N / 4] = 'C';
      }

      rep(d, DIR_NUM) {
        int nx = 0 + arr4[d].first, ny = N / 2 + arr4[d].second;
        if (outField(P(nx, ny), N, N) || initial_board[nx][ny] != '.') continue;
        ng = false;
      }

      if (ng) {
        for (const auto& [px, py] : added_t) {
          initial_board[px][py + N / 4] = '.';
        }
      }
    }

    if (ng) {
      dsu tmp_uf(N * N);
      for (int j = 4; j < N - 8; j += 5) {
        if (abs((j / 4) - (fy / 4)) == 0) continue;
        if (j == N / 2 && ((N / 2) / 4) % 2 == 0) j += 2;

        for (int i = (j % 2 == 0 ? 0 : N - 1); (j % 2 == 0 ? i < N : i >= 0); (j % 2 == 0 ? i++ : i--)) {
          if (initial_board[i][j] != '.') continue;
          if (i == flower_pos.first && j == flower_pos.second) continue;
          if (i == 0 && j == N / 2) continue;

          tmp_uf = dsu(N * N);
          rep(x, N) rep(y, N) {
            if (initial_board[x][y] != '.' || (x == i && y == j)) continue;
            rep(d, DIR_NUM) {
              int nx = x + arr4[d].first, ny = y + arr4[d].second;
              if (outField(P(nx, ny), N, N) || initial_board[nx][ny] != '.' || (nx == i && ny == j)) continue;
              tmp_uf.merge(x * N + y, nx * N + ny);
            }
          }
          if (initial_board[0][0] == '.' && !tmp_uf.same(0 * N + 0, 0 * N + N / 2)) continue;
          if (initial_board[0][N - 1] == '.' && !tmp_uf.same(0 * N + (N - 1), 0 * N + N / 2)) continue;
          if (initial_board[N - 1][0] == '.' && !tmp_uf.same((N - 1) * N + 0, 0 * N + N / 2)) continue;
          if (initial_board[N - 1][N - 1] == '.' && !tmp_uf.same((N - 1) * N + (N - 1), 0 * N + N / 2)) continue;
          if (!tmp_uf.same(flower_pos.first * N + flower_pos.second, 0 * N + N / 2)) continue;

          initial_board[i][j] = 'C';
        }
      }
    }

    auto f_pos = C_positions[flowe_arr_idx];
    for (int ddx = f_pos.sdx; ddx <= f_pos.gdx; ddx++) {
      for (int ddy = f_pos.sdy; ddy <= f_pos.gdy; ddy++) {
        int nx = fx + ddx, ny = fy + ddy;
        if (outField(P(nx, ny), N, N) || initial_board[nx][ny] == 'T') continue;
        initial_board[nx][ny] = '.';
      }
    }

    rep(i, POS_SIZE) {
      const auto& pos = C_positions[(initial_start_idx + i) % POS_SIZE];
      if (!can_place_C(fx, fy, pos.ng_positions, pos.ok_positions)) continue;
      for (const auto& [dx, dy] : pos.ok_positions) {
        int ni = fx + dx, nj = fy + dy;
        if (outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.') continue;
        initial_board[ni][nj] = 'C';
      }
      break;
    }

    rep(i, N) {
      rep(j, N) {
        if (initial_board[i][j] != '.') continue;

        int start_idx;
        if (j < N / 2) start_idx = 0;
        else start_idx = 4;

        rep(k, POS_SIZE) {
          const auto& pos = C_positions[(start_idx + k) % POS_SIZE];
          if (!can_place_C(i, j, pos.ng_positions, pos.ok_positions)) continue;
          for (const auto& [dx, dy] : pos.ok_positions) {
            int ni = i + dx, nj = j + dy;
            if (outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.') continue;
            initial_board[ni][nj] = 'C';
          }
          break;
        }
      }
    }

    for (int ddx = f_pos.sdx; ddx <= f_pos.gdx; ddx++) {
      for (int ddy = f_pos.sdy; ddy <= f_pos.gdy; ddy++) {
        int nx = fx + ddx, ny = fy + ddy;
        if (outField(P(nx, ny), N, N) || initial_board[nx][ny] == 'T') continue;
        initial_board[nx][ny] = '.';
      }
    }

    rep(i, POS_SIZE) {
      const auto& pos = C_positions[(initial_start_idx + i) % POS_SIZE];
      if (!can_place_C(fx, fy, pos.ng_positions, pos.ok_positions)) continue;
      for (const auto& [dx, dy] : pos.ok_positions) {
        int ni = fx + dx, nj = fy + dy;
        if (outField(P(ni, nj), N, N) || initial_board[ni][nj] != '.') continue;
        initial_board[ni][nj] = 'C';
      }
      break;
    }
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
