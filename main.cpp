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

// ==================== 差分更新用 State ==================== //
static vector<Point> prev_p(N * N, Point(-1, -1));
static vector<int> visited(N * N, 0);
static int current_time = 0; // 今何回目の BFS かのカウント方式で VISITED 初期化のオーバヘッド削減
static queue<Point> que;

struct State {
  vector<Answer> answers;

  int money, income;
  vector<int> setted, near_station_cnt, house_near_cnt, work_near_cnt;

  vector<vector<Point>> histories; // 線路・駅設置の履歴 (rollback 用)
  vector<int> money_histories, income_histories; // 駅を置くタイミングでの情報 (rollback 用)

  State() {
    money = K;
    income = 0;
    setted.assign(N * N, LineType::NONE);
    near_station_cnt.assign(N * N, 0);
    house_near_cnt.assign(M, false);
    work_near_cnt.assign(M, false);

    histories.clear();
    money_histories.clear();
    income_histories.clear();
    return;
  }

  inline void next(
    Point &next_station,                         // 次に設置する駅の座標
    const vector<vector<bool>> &planned_station, // planned_station[x][y] : (x, y) に駅を設置予定かどうか
    const vector<vector<int>> &house_idx,        // house_idx[x]        : x に存在する家の index の集合
    const vector<vector<int>> &work_idx,         // work_idx[x]         : x に存在する職場の index の集合
    const vector<Point> &house_points,           // house_points[idx]   : idx 番目の家の座標
    const vector<Point> &work_points             // work_points[idx]    : idx 番目の職場の座標
  ) {
    vector<Point> history;
    if(setted[next_station.x * N + next_station.y] != LineType::NONE) {
      // 空の操作を行ったとする
      histories.emplace_back(history);
      return;
    }

    que = queue<Point>();
    que.push(next_station);

    bool arrived = false;
    Point goal = Point(-1, -1);
    current_time++;
    visited[next_station.x * N + next_station.y] = current_time;

    while(!que.empty()) {
      Point current = que.front();
      que.pop();
      for(auto &d : delta) {
        Point next = current + d;
        if(out_field(next)) continue;

        // 駅に到達した場合
        if(setted[next.x * N + next.y] == LineType::STATION) {
          arrived = true;
          goal = next;
          prev_p[next.x * N + next.y] = current;
          break;
        }

        // 既に設置済み or 今後設置予定の箇所は通らないようにする
        if(visited[next.x * N + next.y] == current_time ||
          setted[next.x * N + next.y] != LineType::NONE ||
          (income != 0 && planned_station[next.x][next.y])
        ) continue;

        visited[next.x * N + next.y] = current_time;
        prev_p[next.x * N + next.y] = current;
        que.push(next);
      }
      if(arrived) break;
    }

    if(income == 0 && goal == Point(-1, -1)) {
      // 一番最初の初期駅設置の場合
      money_histories.emplace_back(money);
      income_histories.emplace_back(income);
      set_station(
        next_station,
        house_idx,
        work_idx,
        house_points,
        work_points,
        history
      );
      histories.emplace_back(history);
      return;
    }
    // 駅に到達不可能な場合
    if(goal == Point(-1, -1)) {
      // 空の操作を行ったとする
      histories.emplace_back(history);
      return;
    }

    // 情報保持
    money_histories.emplace_back(money);
    income_histories.emplace_back(income);

    // 経路復元
    vector<Point> route;
    Point current = goal;
    do {
      route.emplace_back(current);
      current = prev_p[current.x * N + current.y];
    } while(current != next_station);
    route.emplace_back(next_station);
    reverse(route.begin(), route.end());
    
    // 線路・駅設置
    for(int i = 1; i < route.size() - 1; i++) {
      set_line(route[i - 1], route[i], route[i + 1], history);
    }
    set_station(
      next_station,
      house_idx,
      work_idx,
      house_points,
      work_points,
      history
    );

    // history 更新
    histories.emplace_back(history);
    return;
  }

  // 1 回分の差分更新用 rollback 関数
  inline void rollback(
    vector<vector<int>> &house_idx, // house_idx[x]        : x に存在する家の index の集合
    vector<vector<int>> &work_idx,  // work_idx[x]         : x に存在する職場の index の集合
    const vector<Point> &house_points, // house_points[idx]   : idx 番目の家の座標
    const vector<Point> &work_points   // work_points[idx]    : idx 番目の職場の座標
  ) {
    assert(histories.size() > 0);
    auto history = histories.back();
    histories.pop_back();
    if(history.empty()) return; // 空の操作の場合

    for(auto &p : history) {
      if(setted[p.x * N + p.y] == LineType::STATION) { // 駅の場合
        for(auto &d : delta_less2) {
          Point next = p + d;
          if(out_field(next)) continue;
          for(auto &idx : house_idx[next.x * N + next.y]) {
            if(work_near_cnt[idx] != 0 && house_near_cnt[idx] == 1) {
              income -= abs(next.x - work_points[idx].x) + abs(next.y - work_points[idx].y);
            }
            house_near_cnt[idx]--;
          }
          for(auto &idx : work_idx[next.x * N + next.y]) {
            if(house_near_cnt[idx] != 0 && work_near_cnt[idx] == 1) {
              income -= abs(next.x - house_points[idx].x) + abs(next.y - house_points[idx].y);
            }
            work_near_cnt[idx]--;
          }
        }
        setted[p.x * N + p.y] = LineType::NONE;
        money -= income;
      } else { // 線路の場合
        setted[p.x * N + p.y] = LineType::NONE;
      }
    }

    // money, income, turn, answers の rollback
    money = money_histories.back();
    money_histories.pop_back();
    income = income_histories.back();
    income_histories.pop_back();

    assert(!answers.empty());
    answers.pop_back(); // 駅設置動作を削除
    while(!answers.empty()) {
      auto [type, x, y] = answers.back();
      if(type == LineType::STATION) break;
      answers.pop_back();
    }
    return;
  }
  
  private:

  // 駅設置用関数
  inline void set_station(
    Point &next_station,                  // 次に設置する駅の座標
    const vector<vector<int>> &house_idx, // house_idx[x]        : x に存在する家の index の集合
    const vector<vector<int>> &work_idx,  // work_idx[x]         : x に存在する職場の index の集合
    const vector<Point> &house_points,    // house_points[idx]   : idx 番目の家の座標
    const vector<Point> &work_points,     // work_points[idx]    : idx 番目の職場の座標
    vector<Point> &history
  ) {
    assert(setted[next_station.x * N + next_station.y] == LineType::NONE);
    assert(!(income == 0 && money < STATION_COST));

    // お金調達
    while (money + income < STATION_COST) {
      money += income;
      answers.emplace_back(-1, -1, -1);
    }

    answers.emplace_back(LineType::STATION, next_station.x, next_station.y);
    setted[next_station.x * N + next_station.y] = LineType::STATION;
    money -= STATION_COST;
    money += income;
    history.emplace_back(next_station);

    for(auto &d : delta_less2) {
      Point next = next_station + d;
      if(out_field(next)) continue;
      for(auto &idx : house_idx[next.x * N + next.y]) {
        if(work_near_cnt[idx] != 0 && house_near_cnt[idx] == 0) {
          income += abs(next.x - work_points[idx].x) + abs(next.y - work_points[idx].y);
        }
        house_near_cnt[idx]++;
      }
      for(auto &idx : work_idx[next.x * N + next.y]) {
        if(house_near_cnt[idx] != 0 && work_near_cnt[idx] == 0) {
          income += abs(next.x - house_points[idx].x) + abs(next.y - house_points[idx].y);
        }
        work_near_cnt[idx]++;
      }
    }
    return;
  }

  // 線路設置用関数
  inline void set_line(Point &from, Point &now, Point &to, vector<Point> &history) {
    assert(setted[now.x * N + now.y] == LineType::NONE);
    assert(!(income == 0 && money < LINE_COST));

    // お金が足りない場合は待機
    while (money + income < LINE_COST) {
      money += income;
      answers.emplace_back(-1, -1, -1);
    }

    LineType line_type = get_line_type(from, now, to);
    answers.emplace_back(line_type, now.x, now.y);
    setted[now.x * N + now.y] = line_type;
    money -= LINE_COST;
    money += income;
    
    history.emplace_back(now);
    return;
  }
};

struct Solver {
  vector<Answer> answers, best_answers, final_answers;
  vector<vector<int>> setted;

  vector<vector<int>> house_idx, work_idx;
  vector<Point> house_points, work_points;
  vector<vector<int>> house_cnt, work_cnt;

  vector<vector<bool>> planned_station;
  vector<vector<int>> near_station;
  vector<vector<vector<Point>>> another_side;

  int best_money = 0, final_money = 0;
  int _dummy;

  // BFS 用の配列
  int current_time = 0;
  vector<int> visit_time;

  vector<vector<Point>> prev;
  queue<Point> que;

  Solver() {
    input();
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
    State state;

    bool log_flag = true;

    // ==================== Part 1. 初期駅選択 Part ====================
    vector<vector<bool>> used(N, vector<bool>(N, false));
    vector<tuple<int, Point, Point>> start_cand;
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
          if (dist - 1 > (K - 5000 * 2) / 100)
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
          start_cand.emplace_back(cand_income, next1, next2);
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
    initial_station_places = {get<1>(start_cand[0]), get<2>(start_cand[0])};
    for (auto &p : initial_station_places) {
      for (auto &delta : delta_less2) {
        Point next = p + delta;
        if (out_field(next))
          continue;
        already_near_station[next.x][next.y] = true;
      }
      planned_station[p.x][p.y] = true;
    }
    state.next(initial_station_places[0], planned_station, house_idx, work_idx, house_points, work_points);
    state.next(initial_station_places[1], planned_station, house_idx, work_idx, house_points, work_points);



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
    reverse(station_places.begin(), station_places.end());
    station_places.emplace_back(initial_station_places[1]);
    station_places.emplace_back(initial_station_places[0]);
    reverse(station_places.begin(), station_places.end());
    cerr << "Station Places Size: " << station_places.size() << '\n';


    // ==================== Part 3. 山登り法 Part ====================
    best_money = state.money + state.income * (T - state.answers.size() + 1);
    best_answers = answers;

    int iteration = 0, e1, e2, query;
    int kick_cnt = 0;
    Point next;

    int kick_threshold = 1000;
    int start_idx = 0;
    State pre_state = state;

    vector<Point> best_station_places, final_station_places;

    while (utility::mytm.elapsed() < TIME_LIMIT) {
      // Todo 近傍 : 1 点を選択して最も income が増加する点に変更
      e1 = rand_int() % (station_places.size() - 2) + 2;
      e2 = rand_int() % (station_places.size() - 2) + 2;
      while (e1 == e2 && min(e1, e2) > state.histories.size()) 
        e2 = rand_int() % (station_places.size() - 2) + 2;
      swap(station_places[e1], station_places[e2]);
      
      int pre_size1 = state.histories.size();
      // pre_state = state;

      bool updated = false;
      while(!state.histories.empty() && state.histories.size() >= min(e1, e2)) {
        // min(e1, e2) まで rollback
        state.rollback(house_idx, work_idx, house_points, work_points);
      }
      int pre_size2 = state.histories.size();

      int start = state.histories.size();
      int renzoku_non_update = 0;
      for(int i = start; i < station_places.size(); i++) {
        if (state.answers.size() > T - 50)
          break;
        
        state.next(station_places[i], planned_station, house_idx, work_idx, house_points, work_points);

        int cand_money = state.money + state.income * (T - state.answers.size() + 1);
        // 山登り法
        if (cand_money > best_money) {
          updated = true;
          best_money = cand_money;
          best_answers = state.answers;
          best_station_places = station_places;
          renzoku_non_update = 0;
        } else {
          renzoku_non_update++;
          if(renzoku_non_update > 5 && updated) break;
        }
        if(renzoku_non_update > 5 && updated) break;
      }

      if (!updated) {
        kick_cnt++;

        // state = pre_state;
        while(state.histories.size() > pre_size2) {
          state.rollback(house_idx, work_idx, house_points, work_points);
        }
        swap(station_places[e1], station_places[e2]);
        while(state.histories.size() < pre_size1) {
          state.next(station_places[state.histories.size()], planned_station, house_idx, work_idx, house_points, work_points);
        }

        if(kick_cnt > 3000) {
          state = State();
          auto [_, next_point1, next_point2] = start_cand[++start_idx % 3];
          state.next(next_point1, planned_station, house_idx, work_idx, house_points, work_points);
          state.next(next_point2, planned_station, house_idx, work_idx, house_points, work_points);
          
          if(final_money < best_money) {
            final_money = best_money;
            final_answers = best_answers;
            final_station_places = best_station_places;
          }
          best_money = state.money + state.income * (T - state.answers.size() + 1);
          best_answers = answers;
          kick_cnt = 0;
          // cerr << "Kick Apply" << '\n';
        }
      } else {
        if(log_flag) {
          cerr << "Updated: " << best_money << '\n';
          // cerr << "Query: " << (query == 1 ? "Insertion" : "Swap") << '\n';
        }
        kick_cnt -= 10;
      }
      iteration++;
    }
    if(final_money < best_money) {
      final_money = best_money;
      final_answers = best_answers;
    }

    cerr << "Iteration: " << iteration << '\n';
    cerr << "Final Money: " << final_money << '\n';

    rep(i, final_station_places.size()) {
      cerr << final_station_places[i].x << " " << final_station_places[i].y << '\n';
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
