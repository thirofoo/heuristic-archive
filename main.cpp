#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for (int i = 0; i < n; i++)

namespace utility {
struct timer {
  chrono::system_clock::time_point start;
  void CodeStart() { start = chrono::system_clock::now(); }
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

#define TIME_LIMIT 28000

//-----------------以下から実装部分-----------------//

// 入力の基本パラメータ
int M, K, T;
constexpr int N = 50;
#define LINE_COST 100
#define STATION_COST 5000

using Answer = tuple<int, int, int>;

struct Point {
  int x, y;
  Point() : x(0), y(0) {}
  Point(int _x, int _y) : x(_x), y(_y) {}
  Point operator+(const Point &other) const {
    return Point(x + other.x, y + other.y);
  }
  Point operator-(const Point &other) const {
    return Point(x - other.x, y - other.y);
  }
  Point operator*(const int &k) const { return Point(x * k, y * k); }
  bool operator==(const Point &other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const Point &other) const {
    return x != other.x || y != other.y;
  }
  bool operator<(const Point &other) const {
    return x != other.x ? x < other.x : y < other.y;
  }
  bool operator>(const Point &other) const {
    return x != other.x ? x > other.x : y > other.y;
  }
  bool operator<=(const Point &other) const {
    return x != other.x ? x < other.x : y <= other.y;
  }
  bool operator>=(const Point &other) const {
    return x != other.x ? x > other.x : y >= other.y;
  }
};

// right | down | left | up
#define DIR_NUM 4
vector<Point> delta = {Point(0, 1), Point(1, 0), Point(0, -1), Point(-1, 0)};
enum Direction { RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3 };
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

// from, now, to から線路の種類を返す関数
inline LineType get_line_type(Point &from, Point &now, Point &to) {
  Point delta1 = from - now, delta2 = to - now;
  if ((delta1 == delta[LEFT] && delta2 == delta[RIGHT]) ||
      (delta1 == delta[RIGHT] && delta2 == delta[LEFT]))
    return LineType::LR;
  if ((delta1 == delta[UP] && delta2 == delta[DOWN]) ||
      (delta1 == delta[DOWN] && delta2 == delta[UP]))
    return LineType::UD;
  if ((delta1 == delta[LEFT] && delta2 == delta[DOWN]) ||
      (delta1 == delta[DOWN] && delta2 == delta[LEFT]))
    return LineType::LD;
  if ((delta1 == delta[LEFT] && delta2 == delta[UP]) ||
      (delta1 == delta[UP] && delta2 == delta[LEFT]))
    return LineType::LU;
  if ((delta1 == delta[RIGHT] && delta2 == delta[UP]) ||
      (delta1 == delta[UP] && delta2 == delta[RIGHT]))
    return LineType::RU;
  if ((delta1 == delta[RIGHT] && delta2 == delta[DOWN]) ||
      (delta1 == delta[DOWN] && delta2 == delta[RIGHT]))
    return LineType::RD;
  // Error
  cerr << "Error: get_line_type" << '\n';
  assert(false);
  return LineType::NONE;
}

struct Solver {
  vector<Answer> answers, best_answers, final_answers;
  vector<vector<int>> setted;

  vector<vector<int>> house_idx, work_idx;
  vector<Point> house_points, work_points;
  vector<vector<int>> house_cnt, work_cnt;

  vector<vector<bool>> planned_station;
  vector<vector<int>> near_station;
  vector<vector<vector<Point>>> another_side;

  int money = 0, best_money = 0, final_money = 0;
  int income = 0;
  int _dummy;

  // BFS 用の配列
  int current_time = 0;
  vector<int> visit_time;

  vector<vector<Point>> prev;
  queue<Point> que;

  Solver() {
    this->input();
    setted.resize(N, vector<int>(N, LineType::NONE));
    near_station.resize(N, vector<int>(N, false));
    planned_station.resize(N, vector<bool>(N, false));

    // BFS 用の配列
    prev.assign(N, vector<Point>(N, Point(-1, -1)));
    visit_time.assign(N * N, -1);

    utility::mytm.CodeStart();
    return;
  }

  void input() {
    cin >> _dummy >> M >> K >> T;
    money = K;
    another_side.resize(N, vector<vector<Point>>(N, vector<Point>{}));
    house_cnt.resize(N, vector<int>(N, 0));
    work_cnt.resize(N, vector<int>(N, 0));
    house_idx.resize(N * N);
    work_idx.resize(N * N);

    rep(i, M) {
      int x1, y1, x2, y2;
      cin >> x1 >> y1 >> x2 >> y2;
      another_side[x1][y1].emplace_back(Point(x2, y2));
      another_side[x2][y2].emplace_back(Point(x1, y1));

      house_cnt[x1][y1]++;
      work_cnt[x2][y2]++;
      house_idx[x1 * N + y1].emplace_back(i);
      work_idx[x2 * N + y2].emplace_back(i);
      house_points.emplace_back(Point(x1, y1));
      work_points.emplace_back(Point(x2, y2));
    }
    return;
  }

  void output() {
    // 不足分の操作は全て待機 (-1)
    while (final_answers.size() < T)
      final_answers.emplace_back(-1, -1, -1);
    rep(i, T) {
      auto [type, x, y] = final_answers[i];
      if (type == -1)
        cout << -1 << '\n';
      else
        cout << type << " " << x << " " << y << '\n';
    }
    return;
  }

  void solve() {
    vector<Point> station_places, initial_station_places;
    vector<vector<bool>> already_near_station(N, vector<bool>(N, false));

    bool log_flag = true;

    // ==================== Part 1. 初期駅選択 Part ====================
    vector<vector<bool>> used(N, vector<bool>(N, false));
    vector<tuple<int, int, Point, Point>> start_cand;
    int best_income = 0;
    rep(i, M) {
      Point p1 = house_points[i], p2 = work_points[i];
      for (auto &delta1 : delta_less2) {
        Point next1 = p1 + delta1;
        if (out_field(next1))
          continue;
        for (auto &delta2 : delta_less2) {
          Point next2 = p2 + delta2;
          if (out_field(next2))
            continue;

          int dist = abs(next1.x - next2.x) + abs(next1.y - next2.y);
          if (dist - 1 > (money - 5000 * 2) / 100)
            continue;

          for (auto &delta : delta_less2) {
            Point next = next1 + delta;
            if (!out_field(next) && !used[next.x][next.y]) {
              que.push(next);
              used[next.x][next.y] = true;
            }
            next = next2 + delta;
            if (!out_field(next) && !used[next.x][next.y]) {
              que.push(next);
              used[next.x][next.y] = true;
            }
          }
          int cand_income = 0;
          for (auto &delta : delta_less2) {
            Point next = next1 + delta;
            if (out_field(next))
              continue;
            for (auto &p : another_side[next.x][next.y]) {
              if (used[p.x][p.y]) {
                cand_income += abs(next.x - p.x) + abs(next.y - p.y);
              }
            }
          }
          cand_income *= 100 - dist;
          start_cand.emplace_back(cand_income, dist, next1, next2);
          while (!que.empty()) {
            Point current = que.front();
            que.pop();
            used[current.x][current.y] = false;
          }
        }
      }
    }
    assert(start_cand.size() > 0);
    sort(start_cand.begin(), start_cand.end(), greater<>());
    initial_station_places = {get<2>(start_cand[0]), get<3>(start_cand[0])};
    for (auto &p : initial_station_places) {
      for (auto &delta : delta_less2) {
        Point next = p + delta;
        if (out_field(next))
          continue;
        already_near_station[next.x][next.y] = true;
      }
      planned_station[p.x][p.y] = true;
    }
    // 初期駅設置
    set_station(initial_station_places[0]);
    calc_route(initial_station_places[1]);
    initial_station_places.clear();

    // ==================== Part 2. 駅設置箇所固定 Part ====================
    while (true) {
      bool new_station = false;
      int max_cnt = 0;
      Point max_p;
      rep(i, N) rep(j, N) {
        if (planned_station[i][j])
          continue;
        int cand_cnt = 0;
        for (auto &delta : delta_less2) {
          Point next = Point(i, j) + delta;
          if (out_field(next))
            continue;
          cand_cnt += (house_cnt[next.x][next.y] + work_cnt[next.x][next.y]) *
                      (already_near_station[next.x][next.y] ? 1 : 1000);
          if (!already_near_station[next.x][next.y] &&
              (house_cnt[next.x][next.y] + work_cnt[next.x][next.y]) > 0) {
            new_station = true;
          }
        }
        if (cand_cnt > max_cnt) {
          max_cnt = cand_cnt;
          max_p = Point(i, j);
        }
      }
      if (!new_station)
        break;
      for (auto &delta : delta_less2) {
        Point next = max_p + delta;
        if (out_field(next))
          continue;
        already_near_station[next.x][next.y] = true;
      }
      planned_station[max_p.x][max_p.y] = true;
      station_places.emplace_back(max_p);
    }
    sort(station_places.begin(), station_places.end(), greater<>());
    cerr << "Station Places Size: " << station_places.size() << '\n';

    // ==================== Part 3. 山登り法 Part ====================
    int start_money = money;
    int start_income = income;
    vector<Answer> start_answers = answers;
    vector<vector<int>> start_setted = setted;
    vector<vector<int>> start_near_station = near_station;

    best_money = money + income * (T - answers.size() + 1);
    best_answers = answers;

    int iteration = 0, e1, e2, query;
    int kick_cnt = 0;
    Point next;

    int kick_threshold = 1000;
    int inital_idx = 0;

    int max_block = min(3, (int) station_places.size() / 2);
    int block_size, start_idx, dest_idx;
    vector<Point> backup;

    while (utility::mytm.elapsed() < TIME_LIMIT) {
      // Todo 近傍 : 1 点を選択して最も income が増加する点に変更
      e1 = rand_int() % station_places.size();
      e2 = rand_int() % station_places.size();
      while (e1 == e2)
        e2 = rand_int() % station_places.size();
      swap(station_places[e1], station_places[e2]);

      // 初期化
      money = start_money;
      answers = start_answers;
      setted = start_setted;
      income = start_income;
      near_station = start_near_station;

      bool updated = false;
      int renzoku_non_update = 0;
      for (int i = 0; i < station_places.size(); i++) {
        if (answers.size() > T - 50)
          break;
        if (setted[station_places[i].x][station_places[i].y] != LineType::NONE)
          continue;

        if (calc_route(station_places[i])) {
          int cand_money = money + income * (T - answers.size() + 1);
          // 山登り法
          if (cand_money > best_money) {
            updated = true;
            best_money = cand_money;
            best_answers = answers;
            renzoku_non_update = 0;
          } else {
            // Best 更新後に連続で更新がなかったら終了
            renzoku_non_update++;
            if (renzoku_non_update > 3 && updated)
              break;
          }
        }
        if (renzoku_non_update > 3 && updated)
          break;
      }

      if (!updated) {
        kick_cnt++;
        swap(station_places[e1], station_places[e2]);

        // 一定以上更新がなかったら Kick (初期駅を変更)
        if (kick_cnt > kick_threshold) {
          money = K;
          answers.clear();
          setted.assign(N, vector<int>(N, LineType::NONE));
          income = 0;
          near_station.assign(N, vector<int>(N, 0));

          auto [_, __, next_point1, next_point2] = start_cand[++inital_idx % 3];
          set_station(next_point1);
          calc_route(next_point2);
          start_money = money;
          start_income = income;
          start_answers = answers;
          start_setted = setted;
          start_near_station = near_station;

          if (final_money < best_money) {
            final_money = best_money;
            final_answers = best_answers;
          }
          best_money = money + income * (T - answers.size() + 1);
          best_answers = answers;
          kick_cnt = 0;
          if(log_flag) {
            cerr << "========== Kick Apply ==========" << '\n';
          }
        }
      } else {
        if(log_flag) {
          cerr << "Updated: " << best_money << '\n';
          // cerr << "Query: " << (query == 1 ? "Insertion" : "Swap") << '\n';
        }
        kick_cnt = max(0, kick_cnt - 100); // Kick の閾値を下げる
      }

      iteration++;
    }
    if (final_money < best_money) {
      final_money = best_money;
      final_answers = best_answers;
    }

    cerr << "Iteration: " << iteration << '\n';
    cerr << "Final Money: " << final_money << '\n';
    return;
  }

  // BFS + 復元で Point(x, y) に駅設置 + 他の駅に連結させる関数
  inline bool calc_route(Point start) {
    // 条件 : Start は更地であること
    // assert(setted[start.x][start.y] == LineType::NONE);

    current_time++;
    visit_time[start.x * N + start.y] = current_time;
    que = queue<Point>();
    que.push(start);

    bool arrived = false;
    Point goal = Point(-1, -1);

    while (!que.empty()) {
      Point current = que.front();
      que.pop();

      rep(i, DIR_NUM) {
        Point next = current + delta[i];
        if (out_field(next))
          continue;

        // 行先が駅であれば終了
        if (setted[next.x][next.y] == LineType::STATION) {
          arrived = true;
          prev[next.x][next.y] = current;
          goal = next;
          break;
        }

        if (visit_time[next.x * N + next.y] == current_time ||
            setted[next.x][next.y] != LineType::NONE ||
            (income != 0 && planned_station[next.x][next.y]))
          continue;
        visit_time[next.x * N + next.y] = current_time;
        prev[next.x][next.y] = current;
        que.push(next);
      }
      if (arrived)
        break; // ゴール到達可能であれば終了
    }
    if (!arrived)
      return false; // 駅に到達不可能
    if (goal == Point(-1, -1))
      return false; // 他の駅に到達不可能

    // 経路復元 + 操作列生成 (駅設置込み)
    vector<Point> route;
    Point current = goal;
    do {
      route.emplace_back(current);
      current = prev[current.x][current.y];
    } while (current != start);
    route.emplace_back(start);
    reverse(route.begin(), route.end());
    for (int i = 1; i < route.size() - 1; i++) {
      set_line(route[i - 1], route[i], route[i + 1]);
    }
    set_station(start);
    return true;
  }

  inline void set_station(Point &p) {
    assert(setted[p.x][p.y] == LineType::NONE);
    assert(!(income == 0 && money < STATION_COST));

    // お金が足りない場合は待機
    while (money + income < STATION_COST) {
      money += income;
      answers.emplace_back(-1, -1, -1);
    }

    answers.emplace_back(LineType::STATION, p.x, p.y);
    setted[p.x][p.y] = LineType::STATION;
    money -= STATION_COST;
    money += income;

    // 駅設置によるインカム更新
    for (auto &[dx, dy] : delta_less2) {
      Point next = p + Point(dx, dy);
      if (out_field(next))
        continue;
      if (near_station[next.x][next.y] == 0) {
        for (auto &another : another_side[next.x][next.y]) {
          if (!near_station[another.x][another.y])
            continue;
          income += abs(next.x - another.x) + abs(next.y - another.y);
        }
      }
      near_station[next.x][next.y]++;
    }
    return;
  }

  inline void set_line(Point &from, Point &now, Point &to) {
    assert(setted[now.x][now.y] == LineType::NONE);
    assert(!(income == 0 && money < LINE_COST));

    // お金が足りない場合は待機
    while (money + income < LINE_COST) {
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
