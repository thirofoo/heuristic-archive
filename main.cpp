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

using Answer = tuple<int, int, int>;

// 入力の基本パラメータ
int N, M, money, T;
#define LINE_COST 100
#define STATION_COST 5000

struct Point {
  int x, y;
  Point(): x(0), y(0) {}
  Point(int _x, int _y): x(_x), y(_y) {}
  Point operator+(const Point& other) const {
    return Point(x + other.x, y + other.y);
  }
  Point operator-(const Point& other) const {
    return Point(x - other.x, y - other.y);
  }
  Point operator*(const int& k) const {
    return Point(x * k, y * k);
  }
  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const Point& other) const {
    return x != other.x || y != other.y;
  }
  bool operator<(const Point& other) const {
    return x != other.x ? x < other.x : y < other.y;
  }
  bool operator>(const Point& other) const {
    return x != other.x ? x > other.x : y > other.y;
  }
  bool operator<=(const Point& other) const {
    return x != other.x ? x < other.x : y <= other.y;
  }
  bool operator>=(const Point& other) const {
    return x != other.x ? x > other.x : y >=
    other.y;
  }
};

// right | down | left | up
#define DIR_NUM 4
vector<Point> delta = {
  Point(0, 1),
  Point(1, 0),
  Point(0, -1),
  Point(-1, 0)
};

enum Direction {
  RIGHT = 0,
  DOWN = 1,
  LEFT = 2,
  UP = 3
};

inline bool out_field(Point &p) {
  return p.x < 0 || p.x >= N || p.y < 0 || p.y >= N;
}

#define DIR_LESS2_NUM 13
vector<Point> delta_less2 = {
  Point(0, 0),
  delta[RIGHT],
  delta[DOWN],
  delta[LEFT],
  delta[UP],
  delta[RIGHT] + delta[RIGHT],
  delta[RIGHT] + delta[UP],
  delta[RIGHT] + delta[DOWN],
  delta[LEFT] + delta[LEFT],
  delta[LEFT] + delta[UP],
  delta[LEFT] + delta[DOWN],
  delta[UP] + delta[UP],
  delta[DOWN] + delta[DOWN],
};

enum LineType {
  STATION = 0, // 0 : 駅
  LR = 1,      // 1 : 左 ⇔ 右
  UD = 2,      // 2 : 上 ⇔ 下
  LD = 3,      // 3 : 左 ⇔ 下
  LU = 4,      // 4 : 左 ⇔ 上
  RU = 5,      // 5 : 右 ⇔ 上
  RD = 6,      // 6 : 右 ⇔ 下
  NONE = 7     // 7 : NONE
};

// from, now, to から線路の種類を返す
inline LineType get_line_type(Point &from, Point &now, Point &to) {
  Point delta1 = from - now, delta2 = to - now;
  if((delta1 == delta[LEFT] && delta2 == delta[RIGHT]) || (delta1 == delta[RIGHT] && delta2 == delta[LEFT])) return LineType::LR;
  if((delta1 == delta[UP] && delta2 == delta[DOWN]) || (delta1 == delta[DOWN] && delta2 == delta[UP])) return LineType::UD;
  if((delta1 == delta[LEFT] && delta2 == delta[DOWN]) || (delta1 == delta[DOWN] && delta2 == delta[LEFT])) return LineType::LD;
  if((delta1 == delta[LEFT] && delta2 == delta[UP]) || (delta1 == delta[UP] && delta2 == delta[LEFT])) return LineType::LU;
  if((delta1 == delta[RIGHT] && delta2 == delta[UP]) || (delta1 == delta[UP] && delta2 == delta[RIGHT])) return LineType::RU;
  if((delta1 == delta[RIGHT] && delta2 == delta[DOWN]) || (delta1 == delta[DOWN] && delta2 == delta[RIGHT])) return LineType::RD;
  // Error
  cerr << "Error: get_line_type" << '\n';
  cerr << "from: " << from.x << " " << from.y << '\n';
  cerr << "now: " << now.x << " " << now.y << '\n';
  cerr << "to: " << to.x << " " << to.y << '\n';
  assert(false);
  return LineType::NONE;
}
















struct Solver {
  vector<pair<Point, Point>> points;
  vector<Answer> answers;
  vector<vector<int>> setted;

  vector<vector<bool>> near_station;
  vector<vector<vector<Point>>> another_side;
  vector<vector<int>> house_cnt, work_cnt;

  int income = 0;

  Solver() {
    this->input();
    setted.resize(N, vector<int>(N, LineType::NONE));
    near_station.resize(N, vector<bool>(N, false));
    return;
  }

  void input() {
    cin >> N >> M >> money >> T;
    another_side.resize(N, vector<vector<Point>>(N, vector<Point>{}));
    house_cnt.resize(N, vector<int>(N, 0));
    work_cnt.resize(N, vector<int>(N, 0));
    rep(i, M) {
      int x1, y1, x2, y2;
      cin >> x1 >> y1 >> x2 >> y2;
      points.emplace_back(Point(x1, y1), Point(x2, y2));
      another_side[x1][y1].emplace_back(Point(x2, y2));
      another_side[x2][y2].emplace_back(Point(x1, y1));
      house_cnt[x1][y1]++;
      work_cnt[x2][y2]++;
    }
    return;
  }

  void output() {
    // 不足分の操作は全て待機 (-1)
    while(answers.size() < T) answers.emplace_back(-1, -1, -1);
    rep(i, T) {
      auto [type, x, y] = answers[i];
      if(type == -1) cout << -1 << '\n';
      else cout << type << " " << x << " " << y << '\n';
    }
    return;
  }

  void solve() {
    // 貪欲解法
    // 1. 最も家・職場が含まれる箇所を前計算し、最も家を包含する箇所に駅を 1 つ設置 (初期駅)
    // 2. 職場 → 家 → 職場 .. と新しい駅を設置していき、全連結にする
    // 3. 2 を T - 200 (600) ターンまで繰り返す

    // 1.
    vector<Point> station_places;
    vector<vector<bool>> already_near_station(N, vector<bool>(N, false));
    set<Point> planned_places;
    // WIP : 仮として家・職場が多い上位 10 箇所を前計算 (もっと必要なら今後増やす)
    int time = 10;
    while(time--) {
      int max_cnt = 0;
      Point max_p;
      rep(i, N) rep(j, N) {
        if(planned_places.count(Point(i, j))) continue;
        int cand_cnt = 0;
        for(auto &delta : delta_less2) {
          Point next = Point(i, j) + delta;
          if(out_field(next)) continue;
          if(!already_near_station[next.x][next.y]) {
            cand_cnt += (time % 2 == 1 ? house_cnt[next.x][next.y] : work_cnt[next.x][next.y]);
          }
        }
        if(cand_cnt > max_cnt) {
          max_cnt = cand_cnt;
          max_p = Point(i, j);
        }
      }
      if(max_cnt == 0) break;
      for(auto &delta : delta_less2) {
        Point next = max_p + delta;
        if(out_field(next)) continue;
        already_near_station[next.x][next.y] = true;
      }
      planned_places.insert(max_p);
      station_places.emplace_back(max_p);
    }
    set_station(station_places[0]);

    // 2, 3.
    for(int i = 1; i < station_places.size(); i++) {
      if(answers.size() > T - 200) break;
      calc_route(station_places[i]);
      cerr << "Income: " << income << '\n';
      cerr << "Money: " << money << "\n\n";
    }
    cerr << "Final Income: " << income << '\n';
    cerr << "Final Money: " << money << '\n';
    return;
  }

  // BFS + 復元で Point(x, y) に駅設置 + 他の駅に連結させる関数
  inline bool calc_route(Point start) {
    // 条件 : Start は更地であること
    assert(setted[start.x][start.y] == LineType::NONE);

    vector<vector<int>> dist(N, vector<int>(N, -1));
    vector<vector<Point>> prev(N, vector<Point>(N, Point(-1, -1)));
    queue<Point> que;
    que.push(start);
    dist[start.x][start.y] = 0;
    bool arrived = false;
    Point goal = Point(-1, -1);

    while(!que.empty()) {
      Point current = que.front();
      que.pop();
      if(arrived) break; // ゴール到達可能であれば終了
      if(setted[current.x][current.y] != LineType::NONE) continue;

      for(int i = 0; i < DIR_NUM; i++) {
        Point next = current + delta[i];
        if(out_field(next)) continue;
        
        // 行先が駅であれば終了
        if(setted[next.x][next.y] == LineType::STATION) {
          arrived = true;
          prev[next.x][next.y] = current;
          goal = next;
          break;
        }

        if(dist[next.x][next.y] != -1 || setted[next.x][next.y] != LineType::NONE) continue;
        dist[next.x][next.y] = dist[current.x][current.y] + 1;
        prev[next.x][next.y] = current;
        que.push(next);
      }
    }
    if(goal == Point(-1, -1)) return false; // 他の駅に到達不可能

    // 経路復元 + 操作列生成 (駅設置込み)
    vector<Point> route;
    Point current = goal;
    while(current != Point(-1, -1)) {
      route.emplace_back(current);
      current = prev[current.x][current.y];
    }
    reverse(route.begin(), route.end());
    for(int i = 1; i < route.size() - 1; i++) {
      set_line(route[i - 1], route[i], route[i + 1]);
    }
    set_station(start);
    return true;
  }

  inline void set_station(Point &p) {
    assert(setted[p.x][p.y] == LineType::NONE);

    // お金が足りない場合は待機
    while(money < STATION_COST) {
      money += income;
      answers.emplace_back(-1, -1, -1);
    }

    answers.emplace_back(LineType::STATION, p.x, p.y);
    setted[p.x][p.y] = LineType::STATION;
    money -= STATION_COST;
    money += income;
    
    // 駅設置によるインカム更新 (駅は全連結であるという仮定あり)
    for(auto &[dx, dy] : delta_less2) {
      Point next = p + Point(dx, dy);
      if(out_field(next)) continue;
      if(!near_station[next.x][next.y]) {
        near_station[next.x][next.y] = true;
        for(auto &another : another_side[next.x][next.y]) {
          if(!near_station[another.x][another.y]) continue;
          income += abs(next.x - another.x) + abs(next.y - another.y);
        }
      }
    }
    return;
  }

  inline void set_line(Point &from, Point &now, Point &to) {
    assert(setted[now.x][now.y] == LineType::NONE);

    // お金が足りない場合は待機
    while(money < LINE_COST) {
      money += income;
      answers.emplace_back(-1, -1, -1);
    }

    LineType line_type = get_line_type(from, now, to);
    answers.emplace_back(line_type, now.x, now.y);
    setted[now.x][now.y] = line_type;
    money -= LINE_COST;
    money += income;
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