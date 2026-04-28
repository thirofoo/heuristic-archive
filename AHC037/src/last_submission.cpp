#include <bits/stdc++.h>
using namespace std;
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
} // namespace utility

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
#define TIME_LIMIT 1850
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now, int start) {
  return exp((double) (now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

using Point = pair<int, int>;

struct Answer {
  int x1, y1, x2, y2;
  Answer() : x1(0), y1(0), x2(0), y2(0) {}
  Answer(int x1, int y1, int x2, int y2) : x1(x1), y1(y1), x2(x2), y2(y2) {}
};

// right | down | left | up
#define DIR_NUM 4
vector<int> dx = { 0, 1, 0,-1};
vector<int> dy = { 1, 0,-1, 0};

inline bool outField(pair<int, int> now, int h, int w) {
  auto &&[x, y] = now;
  if(0 <= x && x < h && 0 <= y && y < w) return false;
  return true;
}

struct Solver {
  int n;
  vector<int> px, py, cpx, cpy, comp_x, comp_y;
  vector<vector<bool>> need_soda;
  vector<Point> points;
  vector<Answer> answers;

  Solver() {
    this->input(); // 入力受付
    sort(comp_x.begin(), comp_x.end());
    sort(comp_y.begin(), comp_y.end());
    comp_x.erase(unique(comp_x.begin(), comp_x.end()), comp_x.end());
    comp_y.erase(unique(comp_y.begin(), comp_y.end()), comp_y.end());

    cpx.assign(n, 0);
    cpy.assign(n, 0);
    need_soda.assign(comp_x.size(), vector<bool>(comp_y.size(), false));
    rep(i, n) {
      cpx[i] = lower_bound(comp_x.begin(), comp_x.end(), px[i]) - comp_x.begin();
      cpy[i] = lower_bound(comp_y.begin(), comp_y.end(), py[i]) - comp_y.begin();
      need_soda[cpx[i]][cpy[i]] = true;
    }
    return;
  }

  void input() {
    cin >> n;
    px.assign(n, 0);
    py.assign(n, 0);
    rep(i, n) {
      cin >> px[i] >> py[i];
      points.emplace_back(px[i], py[i]);
      comp_x.emplace_back(px[i]);
      comp_y.emplace_back(py[i]);
    }
    return;
  }

  void output() {
    cout << answers.size() << endl;
    for(auto &&ans : answers) {
      cout << ans.x1 << " " << ans.y1 << " " << ans.x2 << " " << ans.y2 << endl;
    }
    return;
  }

  void solve() {
    vector<Point> used_points;
    rep(i, n) used_points.emplace_back(cpx[i], cpy[i]);

    

    return;
  }

  vector<vector<int>> createSteiner(vector<vector<int>> &graph, vector<Point> &need) {
    int v = graph.size();
    vector steiner(v, vector<int>{});
    vector dist(v, vector<int>(v, 1e9));
    vector prev(v, vector<Point>(v, {-1, -1}));
    vector visited(v, vector<bool>(v, false));
    priority_queue<pair<int, Point>, vector<pair<int, Point>>, greater<pair<int, Point>>> pq;

    sort(need.begin(), need.end(), [&](Point &a, Point &b) {
      auto &&[ax, ay] = a;
      auto &&[bx, by] = b;
      return ax + ay < bx + by;
    });
    rep(i, v) dist[i][i] = 0;
    auto [sx, sy] = need[0];
    dist[sx][sy] = 0;
    visited[sx][sy] = true;
    int start_x, start_y;

    for(int i = 1; i < need.size(); i++) {
      auto &&[x, y] = need[i];
      if(visited[x][y]) continue;

      pq.push({0, need[0]});
      dist[x][y] = 0;
      while(!pq.empty()) {
        auto [cost, now] = pq.top();
        auto [now_x, now_y] = now;
        pq.pop();
        if(visited[now_x][now_y]) {
          start_x = now_x;
          start_y = now_y;
          break;
        }
        rep(i, DIR_NUM) {
          int nx = now_x + dx[i], ny = now_y + dy[i];
          if(outField({nx, ny}, v, v)) continue;
          if(dist[nx][ny] > dist[now_x][now_y] + abs(nx - now_x) + abs(ny - now_y)) {
            dist[nx][ny] = dist[now_x][now_y] + abs(nx - now_x) + abs(ny - now_y);
            prev[nx][ny] = Point(now_x, now_y);
            pq.push({dist[nx][ny], {nx, ny}});
          }
        }
      }
      // 経路復元
      while(prev[start_x][start_y] != -1) {
        auto [prev_x, prev_y] = prev[start_x][start_y];
        steiner[start_x].emplace_back(start_y);
        start_x = prev_x;
      }
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