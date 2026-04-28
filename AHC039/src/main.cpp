#include <atcoder/all>
#include <bits/stdc++.h>
using namespace std;
using namespace atcoder;
#define rep(i, n) for (int i = 0; i < n; i++)

namespace utility {
struct timer {
  chrono::system_clock::time_point start;
  // 開始時間を記録
  void CodeStart() { start = chrono::system_clock::now(); }
  // 経過時間 (ms) を返す
  double elapsed() const {
    using namespace std::chrono;
    return (double)duration_cast<milliseconds>(system_clock::now() - start)
        .count();
  }
} mytm;
} // namespace utility

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629,
                      tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty, ty = tz, tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline double rand_double() { return (double)(rand_int() % (int)1e9) / 1e9; }

// 温度関数
#define TIME_LIMIT 2950
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) *
                          ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now, int start) {
  return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

using P = pair<int, int>;
const int GRID_LEN = 100000;

// right | down | left | up
#define DIR_NUM 4
vector<int> dx = {0, 1, 0, -1};
vector<int> dy = {1, 0, -1, 0};

inline bool outField(pair<int, int> now, int h, int w) {
  auto &&[x, y] = now;
  if (0 <= x && x < h && 0 <= y && y < w)
    return false;
  return true;
}

struct Solver {
  int N, one_len, best_score;
  vector<P> saba, iwashi, ans, best_ans;
  vector<vector<int>> saba_cnt, iwashi_cnt;

  Solver() {
    this->input();
    best_score = -1;
    return;
  }

  void input() {
    cin >> N;
    rep(i, N) {
      int x, y;
      cin >> x >> y;
      saba.push_back({x, y});
    }
    rep(i, N) {
      int x, y;
      cin >> x >> y;
      iwashi.push_back({x, y});
    }
    return;
  }

  void output() {
    cout << best_ans.size() << endl;
    for (auto &&[x, y] : best_ans) {
      cout << x << " " << y << endl;
    }
    return;
  } 

  void solve() {
    // ==== 貪欲解法 =====
    // saba - iwashi が多い 10 マスを接続するシュタイナー木が答え
    // ※ prim 法ベースでシュタイナー木を作成する
    using T = tuple<int, int, int>;
    int SPACE_LEN; // 1 マスの辺の長さ

    // vector<int> SPACE_LEN_LIST = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
    // vector<int> masu_cnt_list = {5, 10, 20, 50};
    
    vector<int> SPACE_LEN_LIST = {};
    vector<int> masu_cnt_list = {};

    rep(i, 100) SPACE_LEN_LIST.push_back(5000 + i * 50);
    rep(i, 50) masu_cnt_list.push_back(i + 1);

    rep(i, SPACE_LEN_LIST.size()) rep(j, masu_cnt_list.size()) {
      SPACE_LEN = SPACE_LEN_LIST[i];
      int masu_cnt = masu_cnt_list[j];
      // cerr << "SPACE_LEN: " << SPACE_LEN << " " << "masu_cnt: " << masu_cnt << endl;

      one_len = GRID_LEN / SPACE_LEN;
      saba_cnt.assign(one_len, vector<int>(one_len, 0));
      iwashi_cnt.assign(one_len, vector<int>(one_len, 0));

      // grid をリサイズする感じ
      rep(i, N) {
        auto [x, y] = saba[i];
        int nx = x / SPACE_LEN;
        int ny = y / SPACE_LEN;
        if(outField({nx, ny}, one_len, one_len)) continue;
        saba_cnt[nx][ny]++;
      }
      rep(i, N) {
        auto [x, y] = iwashi[i];
        int nx = x / SPACE_LEN;
        int ny = y / SPACE_LEN;
        if(outField({nx, ny}, one_len, one_len)) continue;
        iwashi_cnt[nx][ny]++;
      }

      // 1. 多いマスを探す & グラフ作成
      vector<T> max_v;
      vector graph(one_len, vector(one_len, vector<T>{}));
      int param = 1;
      rep(i, one_len) rep(j, one_len) {
        max_v.emplace_back(iwashi_cnt[i][j] * param - saba_cnt[i][j] + param * N, i, j);
        rep(d, DIR_NUM) {
          int nx = i + dx[d], ny = j + dy[d];
          if (outField({nx, ny}, one_len, one_len)) continue;
          graph[nx][ny].emplace_back(iwashi_cnt[i][j] * param - saba_cnt[i][j] + param * N, i, j);
        }
      }

      sort(max_v.begin(), max_v.end());
      while(max_v.size() > masu_cnt) max_v.pop_back();

      // 2. シュタイナー木を作成
      priority_queue<T, vector<T>, greater<T>> pq;
      vector<vector<bool>> in_steiner(one_len, vector<bool>(one_len, false));
      vector<vector<int>> dist;
      vector<vector<P>> prev;

      auto [_, x1, y1] = max_v[0];
      in_steiner[x1][y1] = true;

      for (int i = 1; i < masu_cnt; i++) {
        // dijkstra 法で最短経路復元
        auto [_, x, y] = max_v[i];
        pq.push({0, x, y});
        dist.assign(one_len, vector<int>(one_len, 1e9));
        prev.assign(one_len, vector<P>(one_len, {-1, -1}));
        dist[x][y] = 0;

        int hx = -1, hy = -1;
        while (!pq.empty()) {
          auto [cost, x, y] = pq.top();
          pq.pop();
          if(in_steiner[x][y]) {
            hx = x, hy = y;
            break;
          }
          if(dist[x][y] < cost) continue;
          for(auto [c, nx, ny] : graph[x][y]) {
            if(dist[nx][ny] <= cost + c) continue;
            dist[nx][ny] = cost + c;
            prev[nx][ny] = {x, y};
            pq.push({cost + c, nx, ny});
          }
        }
        while (!pq.empty()) pq.pop();

        // 経路復元
        while(hx != -1) {
          in_steiner[hx][hy] = true;
          auto [px, py] = prev[hx][hy];
          hx = px, hy = py;
        }
      }

      // 3. シュタイナー木の頂点をうまく出力
      vector<vector<int>> deg(one_len, vector<int>(one_len, 0));
      int sx = -1, sy = -1;
      rep(i, one_len) {
        rep(j, one_len) {
          if(!in_steiner[i][j]) continue;
          rep(d, DIR_NUM) {
            int nx = i + dx[d], ny = j + dy[d];
            if (outField({nx, ny}, one_len, one_len)) continue;
            if(in_steiner[nx][ny]) deg[i][j]++;
          }
        }
      }

      rep(i, one_len) {
        rep(j, one_len) {
          if(!in_steiner[i][j]) continue;
          if(deg[i][j] == 1) {
            sx = i, sy = j;
            break;
          }
        }
        if(sx != -1) break;
      }
      if(sx == -1) {
        rep(i, one_len) {
          rep(j, one_len) {
            if(!in_steiner[i][j]) continue;
            sx = i, sy = j;
            break;
          }
          if(sx != -1) break;
        }
      }
      // cerr << "sx: " << sx << " " << "sy: " << sy << endl;

      vector<vector<P>> push_perm = {
        {{0, 0}, {0, 1}, {1, 1}, {1, 0}}, // right
        {{0, 1}, {1, 1}, {1, 0}, {0, 0}}, // down
        {{1, 1}, {1, 0}, {0, 0}, {0, 1}}, // left
        {{1, 0}, {0, 0}, {0, 1}, {1, 1}} // up
      };
      // dfs の行きがけで ans を作成
      int score = 0;
      vector<vector<bool>> visited(one_len, vector<bool>(one_len, false));
      auto dfs = [&](auto &&f, int x, int y, int now_dir) -> void {
        visited[x][y] = true;
        score += saba_cnt[x][y] - iwashi_cnt[x][y];
        rep(d, DIR_NUM) {
          int next_dir = (now_dir + 3 + d) % DIR_NUM;
          auto [ndx, ndy] = push_perm[now_dir][d];

          int tx = x + ndx, ty = y + ndy, cnt = 0;
          bool taikaku = false;
          vector<P> ttd = {{0, 0}, {0, -1}, {-1, -1}, {-1, 0}};
          for(auto [tdx, tdy] : ttd) {
            if(outField({tx + tdx, ty + tdy}, one_len, one_len)) continue;
            cnt += in_steiner[tx + tdx][ty + tdy];
          }
          rep(k, 2) {
            auto [tdx1, tdy1] = ttd[k];
            auto [tdx2, tdy2] = ttd[k + 2];
            if(outField({tx + tdx1, ty + tdy1}, one_len, one_len)) continue;
            if(outField({tx + tdx2, ty + tdy2}, one_len, one_len)) continue;
            if(in_steiner[tx + tdx1][ty + tdy1] && in_steiner[tx + tdx2][ty + tdy2]) taikaku = true;
          }
          if(cnt % 2 == 1 || (cnt == 2 && taikaku)) ans.emplace_back(tx * SPACE_LEN + (ndx == 1 ? -1 : 0), ty * SPACE_LEN + (ndy == 1 ? -1 : 0));

          int nx = x + dx[next_dir], ny = y + dy[next_dir];
          if(outField({nx, ny}, one_len, one_len)) continue;
          if(visited[nx][ny]) continue;
          if(!in_steiner[nx][ny]) continue;
          f(f, nx, ny, next_dir);
        }
        return;
      };
      dfs(dfs, sx, sy, 0);

      vector<P> final_ans;
      rep(i, ans.size()) {
        if(ans[i] == ans[(i + 1) % ans.size()]) continue;
        final_ans.emplace_back(ans[i]);
      }
      swap(ans, final_ans);

      // 答えの制約を満たすかどうかの確認
      bool ok = true;
      int sourround = 0;
      rep(i, ans.size()) {
        auto [x1, y1] = ans[i];
        auto [x2, y2] = ans[(i + 1) % ans.size()];
        sourround += abs(x1 - x2) + abs(y1 - y2);
      }
      ok &= (sourround <= 400000);
      ok &= (ans.size() <= 1000);

      // 閉路ができてないか確認
      queue<P> q;
      vector<vector<bool>> visited2(one_len, vector<bool>(one_len, false));
      vector<P> delta = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}, {-1, 1}, {1, 1}, {1, -1}, {-1, -1}};
      rep(i, one_len) rep(j, one_len) {
        if(in_steiner[i][j] || visited2[i][j]) continue;
        q.push({i, j});
        bool check = false;
        while(!q.empty()) {
          auto [nx, ny] = q.front();
          q.pop();
          if(visited2[nx][ny]) continue;
          visited2[nx][ny] = true;
          rep(d, delta.size()) {
            int tx = nx + delta[d].first, ty = ny + delta[d].second;
            if(outField({tx, ty}, one_len, one_len)) {
              check = true;
              continue;
            }
            if(visited2[tx][ty]) continue;
            if(in_steiner[tx][ty]) continue;
            q.push({tx, ty});
          }
        }
        ok &= check;
      }

      // cerr << "score: " << score << " " << "sourround: " << sourround << " " << "size: " << ans.size() << endl;
      if(ok && score > best_score) {
        best_score = score;
        best_ans = ans;
        // rep(i, one_len) {
        //   rep(j, one_len) {
        //     cerr << (in_steiner[i][j] ? "#" : ".");
        //   }
        //   cerr << endl;
        // }
        // cerr << endl;
      }
      // cerr << endl;
      ans.clear();
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
