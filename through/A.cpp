#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for (int i = 0; i < n; i++)

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
  tx = ty; ty = tz; tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline double rand_double01() {
  return static_cast<double>(rand_int()) / (static_cast<double>(UINT_MAX) + 1.0);
}

#define TIME_LIMIT 1950

inline double temp(double elapsed_time, double total_duration) {
  double start_temp = 10000;
  double end_temp = 0.1;
  if (total_duration <= 0) return end_temp;
  double progress = elapsed_time / total_duration;
  progress = min(1.0, max(0.0, progress));
  return start_temp + (end_temp - start_temp) * progress;
}

inline double acceptance_probability(double current_cost, double next_cost, double temperature) {
  if (next_cost < current_cost) return 1.0;
  if (temperature <= 1e-9) return 0.0;
  double delta_cost = current_cost - next_cost;
  double exponent = delta_cost / temperature;
  if (exponent < -700.0) return 0.0;
  return exp(exponent);
}

const double INF_COST = -1.0;

struct Point {
  int x, y;
  Point() : x(0), y(0) {}
  Point(int _x, int _y) : x(_x), y(_y) {}
  Point operator-(const Point &other) const { return Point(x - other.x, y - other.y); }
  bool operator==(const Point &other) const { return x == other.x && y == other.y; }
};

double dist(Point p1, Point p2) {
  long long dx = static_cast<long long>(p1.x) - p2.x;
  long long dy = static_cast<long long>(p1.y) - p2.y;
  return sqrt(static_cast<double>(dx * dx + dy * dy));
}

bool is_on_segment(Point A, Point B, Point P) {
  long long cross_product = (long long)(P.y - A.y) * (B.x - A.x) - (long long)(P.x - A.x) * (B.y - A.y);
  if (cross_product != 0) return false;
  bool within_x = (P.x >= min(A.x, B.x)) && (P.x <= max(A.x, B.x));
  bool within_y = (P.y >= min(A.y, B.y)) && (P.y <= max(A.y, B.y));
  return within_x && within_y;
}

struct Solver {
  int X, Y, Z;
  vector<Point> px;
  vector<Point> py;
  vector<Point> pz;
  vector<vector<Point>> history;
  vector<vector<bool>> segment_valid;
  
  Solver() {
    utility::mytm.CodeStart();
    input();
  }
  
  void input() {
    cin >> X >> Y >> Z;
    px.resize(X);
    py.resize(Y);
    pz.resize(Z);
    rep(i, X) cin >> px[i].x >> px[i].y;
    rep(i, Y) cin >> py[i].x >> py[i].y;
    rep(i, Z) cin >> pz[i].x >> pz[i].y;
  }
  
  bool check_segment_idx_validity(int idx1, int idx2) {
    if (idx1 == idx2) return true;
    Point A = px[idx1], B = px[idx2];
    for (const auto &r : pz) {
      if (is_on_segment(A, B, r)) return false;
    }
    return true;
  }
  
  void precompute_validity() {
    if (X <= 1) return;
    segment_valid.assign(X, vector<bool>(X, false));
    rep(i, X) {
      for (int j = i; j < X; j++) {
        bool valid = check_segment_idx_validity(i, j);
        segment_valid[i][j] = segment_valid[j][i] = valid;
      }
    }
    cerr << "Segment validity precomputation finished at " << utility::mytm.elapsed() << " ms" << endl;
  }
  
  double calculate_total_cost_fast(const vector<int> &order) {
    if (X <= 1) return 0.0;
    double total_cost = 0.0;
    for (int i = 0; i < X - 1; i++) {
      int u_idx = order[i], v_idx = order[i + 1];
      if (!segment_valid[u_idx][v_idx]) return INF_COST;
      total_cost += 2.0 * dist(px[u_idx], px[v_idx]);
    }
    return total_cost;
  }
  
  void solve() {
    precompute_validity();
    vector<int> current_order(X);
    if (X > 0) iota(current_order.begin(), current_order.end(), 0);
    double current_cost = calculate_total_cost_fast(current_order);
    mt19937 rng(rand_int());
    int shuffle_attempts = 0;
    const int MAX_SHUFFLE_ATTEMPTS = 3000;
    const double INITIAL_SEARCH_TIME_LIMIT = 800;
    while (current_cost == INF_COST && X > 1 &&
         utility::mytm.elapsed() < INITIAL_SEARCH_TIME_LIMIT &&
         shuffle_attempts < MAX_SHUFFLE_ATTEMPTS)
    {
      shuffle(current_order.begin(), current_order.end(), rng);
      current_cost = calculate_total_cost_fast(current_order);
      shuffle_attempts++;
    }
    if (current_cost == INF_COST && X > 1) {
      cerr << "Warning: Could not find an initial valid path after " << shuffle_attempts 
         << " attempts and " << utility::mytm.elapsed() << " ms." << endl;
      Point origin(0, 0);
      history.push_back({origin, origin, origin, origin});
      return;
    }
    if (X > 1)
      cerr << "Found initial valid path with cost: " << current_cost 
         << " after " << shuffle_attempts << " shuffles." << endl;
    else
      cerr << "Path has " << X << " points. Initial cost: " << current_cost << endl;
    
    vector<int> best_order = current_order;
    double best_cost = current_cost;
    double sa_start_time = utility::mytm.elapsed();
    double sa_duration = max(0.0, TIME_LIMIT - sa_start_time);
    int iterations = 0;
    while (utility::mytm.elapsed() < TIME_LIMIT && X > 1) {
      iterations++;
      int i = rand_int() % X;
      int j = rand_int() % X;
      if (i == j) continue;
      if (i > j) swap(i, j);
      vector<int> next_order = current_order;
      reverse(next_order.begin() + i + 1, next_order.begin() + j + 1);
      double next_cost = calculate_total_cost_fast(next_order);
      if (next_cost == INF_COST) continue;
      double current_elapsed_sa_time = utility::mytm.elapsed() - sa_start_time;
      double T = temp(current_elapsed_sa_time, sa_duration);
      double P = acceptance_probability(current_cost, next_cost, T);
      if (P >= rand_double01()) {
        current_order = next_order;
        current_cost = next_cost;
        if (current_cost < best_cost) {
          best_cost = current_cost;
          best_order = current_order;
        }
      }
    }
    if (X > 1)
      cerr << "SA finished after " << iterations << " iterations." << endl;
    history.clear();
    Point aoki_pos(0, 0);
    if (X == 0) {
      history.push_back({Point(0, 0), Point(0, 0), aoki_pos, aoki_pos});
    } else {
      Point initial_p1 = px[best_order[0]];
      Point initial_q1 = px[best_order[0]];
      history.push_back({initial_p1, initial_q1, aoki_pos, aoki_pos});
      for (int i = 0; i < X - 1; i++) {
        Point next_pos = px[best_order[i + 1]];
        history.push_back({next_pos, next_pos, aoki_pos, aoki_pos});
      }
    }
  }
  
  void output() {
    if (history.empty()) {
      cout << "0 0 0 0 0 0 0 0" << endl;
      return;
    }
    cout << history[0][0].x << " " << history[0][0].y << " " 
       << history[0][1].x << " " << history[0][1].y << " " 
       << history[0][2].x << " " << history[0][2].y << " " 
       << history[0][3].x << " " << history[0][3].y << endl;
    for (size_t i = 1; i < history.size(); i++) {
      cout << history[i][0].x << " " << history[i][0].y << " " 
         << history[i][1].x << " " << history[i][1].y << " " 
         << history[i][2].x << " " << history[i][2].y << " " 
         << history[i][3].x << " " << history[i][3].y << endl;
    }
  }
};

int main() {
  cin.tie(0);
  ios_base::sync_with_stdio(false);
  Solver solver;
  solver.solve();
  solver.output();
  cerr << "Total time: " << utility::mytm.elapsed() << " ms" << endl;
  return 0;
}
