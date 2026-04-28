#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <random>
#include <iomanip>
#include <limits>
#include <set>
#include <tuple>

using CoordType = long long;
#define rep(i, n) for (int i = 0; i < (n); ++i)

namespace utility {
struct timer {
  std::chrono::system_clock::time_point start;
  timer() { start = std::chrono::system_clock::now(); }
  double elapsed() const {
    using namespace std::chrono;
    return static_cast<double>(duration_cast<milliseconds>(system_clock::now() - start).count());
  }
} mytm;
}

struct Point {
  CoordType x, y;
  Point() : x(0), y(0) {}
  Point(CoordType _x, CoordType _y) : x(_x), y(_y) {}
  Point operator-(const Point &other) const { return Point(x - other.x, y - other.y); }
  bool operator==(const Point &other) const { return x == other.x && y == other.y; }
  bool operator!=(const Point &other) const { return !(*this == other); }
};

double dist(Point p1, Point p2) {
  CoordType dx = p1.x - p2.x, dy = p1.y - p2.y;
  if (dx == 0 && dy == 0) return 0.0;
  return std::hypot(static_cast<double>(dx), static_cast<double>(dy));
}

struct TrashInfo {
    CoordType x, y;
    int type;
    int original_index;
};

struct SortByXAsc {
    bool operator()(const TrashInfo& a, const TrashInfo& b) const {
        if (a.x != b.x) return a.x < b.x;
        return a.y < b.y;
    }
};
struct SortByXDesc {
    bool operator()(const TrashInfo& a, const TrashInfo& b) const {
        if (a.x != b.x) return a.x > b.x;
        return a.y < b.y;
    }
};
struct SortByYAsc {
    bool operator()(const TrashInfo& a, const TrashInfo& b) const {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    }
};
struct SortByYDesc {
    bool operator()(const TrashInfo& a, const TrashInfo& b) const {
        if (a.y != b.y) return a.y > b.y;
        return a.x < b.x;
    }
};

struct SweepResult {
    std::vector<std::vector<Point>> history;
    double total_cost = std::numeric_limits<double>::max();
};

struct Solver {
  int X, Y, Z;
  std::vector<Point> px, py, pz;
  std::vector<std::vector<Point>> best_history;
  const CoordType M = 1000000;

  Solver() { input(); }

  void input() {
    std::cin >> X >> Y >> Z;
    px.resize(X); py.resize(Y); pz.resize(Z);
    rep(i, X) std::cin >> px[i].x >> px[i].y;
    rep(i, Y) std::cin >> py[i].x >> py[i].y;
    rep(i, Z) std::cin >> pz[i].x >> pz[i].y;
  }

  double calculate_total_cost(const std::vector<std::vector<Point>>& history_local) {
      double total_cost = 0.0;
      if (history_local.empty()) return 0.0;
      Point pt_l_first = history_local[0][0], pt_r_first = history_local[0][1];
      Point pa_l_first = history_local[0][2], pa_r_first = history_local[0][3];
      Point pt_l_implicit_start, pt_r_implicit_start, pa_l_implicit_start, pa_r_implicit_start;
      if (pt_l_first.y == 0 && pt_r_first.y == M) {
            pt_l_implicit_start = {pt_l_first.x == 0 ? M : 0, 0};
            pt_r_implicit_start = {pt_r_first.x == 0 ? M : 0, M};
            pa_l_implicit_start = {pa_l_first.x == 0 ? M : 0, 0};
            pa_r_implicit_start = {pa_r_first.x == 0 ? M : 0, M};
            if (pt_l_first.x == 0 || pa_l_first.x == 0) {
                 pt_l_implicit_start.x = 0; pt_r_implicit_start.x = 0;
                 pa_l_implicit_start.x = 0; pa_r_implicit_start.x = 0;
            } else if (pt_l_first.x == M || pa_l_first.x == M){
                 pt_l_implicit_start.x = M; pt_r_implicit_start.x = M;
                 pa_l_implicit_start.x = M; pa_r_implicit_start.x = M;
            } else {
                 pt_l_implicit_start = {0, 0}; pt_r_implicit_start = {0, M};
                 pa_l_implicit_start = {0, 0}; pa_r_implicit_start = {0, M};
            }

      } else if (pt_l_first.x == 0 && pt_r_first.x == M) {
            pt_l_implicit_start = {0, pt_l_first.y == 0 ? M : 0};
            pt_r_implicit_start = {M, pt_r_first.y == 0 ? M : 0};
            pa_l_implicit_start = {0, pa_l_first.y == 0 ? M : 0};
            pa_r_implicit_start = {M, pa_r_first.y == 0 ? M : 0};
             if (pt_l_first.y == 0 || pa_l_first.y == 0) {
                  pt_l_implicit_start.y = 0; pt_r_implicit_start.y = 0;
                  pa_l_implicit_start.y = 0; pa_r_implicit_start.y = 0;
             } else if (pt_l_first.y == M || pa_l_first.y == M){
                  pt_l_implicit_start.y = M; pt_r_implicit_start.y = M;
                  pa_l_implicit_start.y = M; pa_r_implicit_start.y = M;
             } else {
                  pt_l_implicit_start = {0, 0}; pt_r_implicit_start = {M, 0};
                  pa_l_implicit_start = {0, 0}; pa_r_implicit_start = {M, 0};
             }
      } else {
           pt_l_implicit_start = {0, 0}; pt_r_implicit_start = {0, M};
           pa_l_implicit_start = {0, 0}; pa_r_implicit_start = {0, M};
      }
      double first_cost_t = dist(pt_l_implicit_start, history_local[0][0]) + dist(pt_r_implicit_start, history_local[0][1]);
      double first_cost_a = dist(pa_l_implicit_start, history_local[0][2]) + dist(pa_r_implicit_start, history_local[0][3]);
      total_cost += std::max(first_cost_t, first_cost_a);
      for (size_t i = 1; i < history_local.size(); ++i) {
           const auto& prev = history_local[i-1];
           const auto& curr = history_local[i];
           double cost_t = dist(prev[0], curr[0]) + dist(prev[1], curr[1]);
           double cost_a = dist(prev[2], curr[2]) + dist(prev[3], curr[3]);
           total_cost += std::max(cost_t, cost_a);
      }
      return total_cost;
  }

  SweepResult run_single_sweep_direction(const std::vector<TrashInfo>& sorted_trash, int move_axis, bool increasing_order) {
      SweepResult result;
      if (sorted_trash.empty()) { result.total_cost = 0; return result; }
      const TrashInfo& first_trash = sorted_trash[0];
      CoordType first_active_coord = move_axis == 0 ? first_trash.x : first_trash.y;
      int first_active_type = first_trash.type;
      Point pt_l_init, pt_r_init, pa_l_init, pa_r_init;
      CoordType first_inactive_target_coord = increasing_order ? (first_active_coord - 1) : (first_active_coord + 1);
      first_inactive_target_coord = std::max((CoordType)0, std::min(M, first_inactive_target_coord));
      if (move_axis == 0) {
          pt_l_init = {first_active_type == 0 ? first_active_coord : first_inactive_target_coord, 0};
          pt_r_init = {first_active_type == 0 ? first_active_coord : first_inactive_target_coord, M};
          pa_l_init = {first_active_type == 1 ? first_active_coord : first_inactive_target_coord, 0};
          pa_r_init = {first_active_type == 1 ? first_active_coord : first_inactive_target_coord, M};
      } else {
          pt_l_init = {0, first_active_type == 0 ? first_active_coord : first_inactive_target_coord};
          pt_r_init = {M, first_active_type == 0 ? first_active_coord : first_inactive_target_coord};
          pa_l_init = {0, first_active_type == 1 ? first_active_coord : first_inactive_target_coord};
          pa_r_init = {M, first_active_type == 1 ? first_active_coord : first_inactive_target_coord};
      }
      result.history.push_back({pt_l_init, pt_r_init, pa_l_init, pa_r_init});
      Point pt_l_cur = pt_l_init, pt_r_cur = pt_r_init, pa_l_cur = pa_l_init, pa_r_cur = pa_r_init;
      for (size_t i = 1; i < sorted_trash.size(); ++i) {
          const auto& trash = sorted_trash[i];
          CoordType active_target_coord = move_axis == 0 ? trash.x : trash.y;
          int active_player_type = trash.type;
          Point pt_l_next = pt_l_cur, pt_r_next = pt_r_cur, pa_l_next = pa_l_cur, pa_r_next = pa_r_cur;
          double dist_active = 0.0, dist_inactive = 0.0;
          CoordType inactive_current_coord = 0, final_inactive_target_coord = 0;
          if (active_player_type == 0) {
              Point target_l = pt_l_cur, target_r = pt_r_cur;
              if (move_axis == 0) { target_l.x = active_target_coord; target_r.x = active_target_coord; }
              else { target_l.y = active_target_coord; target_r.y = active_target_coord; }
              dist_active = (target_l != pt_l_cur || target_r != pt_r_cur) ? (dist(pt_l_cur, target_l) + dist(pt_r_cur, target_r)) : 0.0;
              pt_l_next = target_l; pt_r_next = target_r;
              inactive_current_coord = move_axis == 0 ? pa_l_cur.x : pa_l_cur.y;
              CoordType ideal_inactive_target = increasing_order ? (active_target_coord - 1) : (active_target_coord + 1);
              ideal_inactive_target = std::max((CoordType)0, std::min(M, ideal_inactive_target));
              final_inactive_target_coord = increasing_order ? std::max(ideal_inactive_target, inactive_current_coord)
                                                            : std::min(ideal_inactive_target, inactive_current_coord);
              if (final_inactive_target_coord != inactive_current_coord) {
                   Point inactive_target_l = pa_l_cur, inactive_target_r = pa_r_cur;
                   if (move_axis == 0) { inactive_target_l.x = final_inactive_target_coord; inactive_target_r.x = final_inactive_target_coord; }
                   else { inactive_target_l.y = final_inactive_target_coord; inactive_target_r.y = final_inactive_target_coord; }
                   dist_inactive = dist(pa_l_cur, inactive_target_l) + dist(pa_r_cur, inactive_target_r);
                   if (dist_inactive <= dist_active + 1e-9) {
                       pa_l_next = inactive_target_l; pa_r_next = inactive_target_r;
                   }
              }
          } else {
              Point target_l = pa_l_cur, target_r = pa_r_cur;
              if (move_axis == 0) { target_l.x = active_target_coord; target_r.x = active_target_coord; }
              else { target_l.y = active_target_coord; target_r.y = active_target_coord; }
              dist_active = (target_l != pa_l_cur || target_r != pa_r_cur) ? (dist(pa_l_cur, target_l) + dist(pa_r_cur, target_r)) : 0.0;
              pa_l_next = target_l; pa_r_next = target_r;
              inactive_current_coord = move_axis == 0 ? pt_l_cur.x : pt_l_cur.y;
              CoordType ideal_inactive_target = increasing_order ? (active_target_coord - 1) : (active_target_coord + 1);
              ideal_inactive_target = std::max((CoordType)0, std::min(M, ideal_inactive_target));
              final_inactive_target_coord = increasing_order ? std::max(ideal_inactive_target, inactive_current_coord)
                                                            : std::min(ideal_inactive_target, inactive_current_coord);
              if (final_inactive_target_coord != inactive_current_coord) {
                  Point inactive_target_l = pt_l_cur, inactive_target_r = pt_r_cur;
                  if (move_axis == 0) { inactive_target_l.x = final_inactive_target_coord; inactive_target_r.x = final_inactive_target_coord; }
                  else { inactive_target_l.y = final_inactive_target_coord; inactive_target_r.y = final_inactive_target_coord; }
                  dist_inactive = dist(pt_l_cur, inactive_target_l) + dist(pt_r_cur, inactive_target_r);
                  if (dist_inactive <= dist_active + 1e-9) {
                      pt_l_next = inactive_target_l; pt_r_next = inactive_target_r;
                  }
              }
          }
          if (pt_l_next != pt_l_cur || pt_r_next != pt_r_cur ||
              pa_l_next != pa_l_cur || pa_r_next != pa_r_cur)
          {
             if (result.history.empty() ||
                 pt_l_next != result.history.back()[0] || pt_r_next != result.history.back()[1] ||
                 pa_l_next != result.history.back()[2] || pa_r_next != result.history.back()[3])
             {
                   result.history.push_back({pt_l_next, pt_r_next, pa_l_next, pa_r_next});
             }
          }
          pt_l_cur = pt_l_next; pt_r_cur = pt_r_next;
          pa_l_cur = pa_l_next; pa_r_cur = pa_r_next;
      }
      result.total_cost = calculate_total_cost(result.history);
      return result;
  }

  void solve_4_sweeps_enhanced() {
    double min_total_cost = std::numeric_limits<double>::max();
    std::vector<TrashInfo> all_trash;
    all_trash.reserve(X + Y);
    rep(i, X) all_trash.push_back({px[i].x, px[i].y, 0, i});
    rep(i, Y) all_trash.push_back({py[i].x, py[i].y, 1, i + X});
    
    std::vector<TrashInfo> sorted_l_to_r = all_trash;
    std::sort(sorted_l_to_r.begin(), sorted_l_to_r.end(), SortByXAsc());
    SweepResult res1 = run_single_sweep_direction(sorted_l_to_r, 0, true);
    if (res1.total_cost < min_total_cost) { 
      min_total_cost = res1.total_cost; 
      best_history = res1.history; 
    }
    
    std::vector<TrashInfo> sorted_r_to_l = all_trash;
    std::sort(sorted_r_to_l.begin(), sorted_r_to_l.end(), SortByXDesc());
    SweepResult res2 = run_single_sweep_direction(sorted_r_to_l, 0, false);
    if (res2.total_cost < min_total_cost) { 
      min_total_cost = res2.total_cost; 
      best_history = res2.history; 
    }
    
    std::vector<TrashInfo> sorted_b_to_t = all_trash;
    std::sort(sorted_b_to_t.begin(), sorted_b_to_t.end(), SortByYAsc());
    SweepResult res3 = run_single_sweep_direction(sorted_b_to_t, 1, true);
    if (res3.total_cost < min_total_cost) { 
      min_total_cost = res3.total_cost; 
      best_history = res3.history; 
    }
    
    std::vector<TrashInfo> sorted_t_to_b = all_trash;
    std::sort(sorted_t_to_b.begin(), sorted_t_to_b.end(), SortByYDesc());
    SweepResult res4 = run_single_sweep_direction(sorted_t_to_b, 1, false);
    if (res4.total_cost < min_total_cost) { 
      min_total_cost = res4.total_cost; 
      best_history = res4.history; 
    }
    
    if (X + Y > 0 && best_history.empty())
       best_history.push_back({{0,0},{0,0},{0,0},{0,0}});
  }

  void output() {
    if (best_history.empty()) {
        std::cout << "0 0 0 0 0 0 0 0\n";
        return;
    }
    std::cout << std::fixed << std::setprecision(0);
    Point pt_l_first = best_history[0][0], pt_r_first = best_history[0][1];
    Point pa_l_first = best_history[0][2], pa_r_first = best_history[0][3];
    Point pt_l_implicit_start = {0, 0}, pt_r_implicit_start = {0, M};
    Point pa_l_implicit_start = {0, 0}, pa_r_implicit_start = {0, M};
    if (pt_l_first.y == 0 && pt_r_first.y == M && pa_l_first.y == 0 && pa_r_first.y == M) {
        if (pt_l_first.x == 0 && pa_l_first.x == 0) {
            pt_l_implicit_start = {0, 0}; pt_r_implicit_start = {0, M};
            pa_l_implicit_start = {0, 0}; pa_r_implicit_start = {0, M};
        } else {
            pt_l_implicit_start = {M, 0}; pt_r_implicit_start = {M, M};
            pa_l_implicit_start = {M, 0}; pa_r_implicit_start = {M, M};
        }
    }
    else if (pt_l_first.x == 0 && pt_r_first.x == M && pa_l_first.x == 0 && pa_r_first.x == M) {
         if (pt_l_first.y == 0 && pa_l_first.y == 0) {
              pt_l_implicit_start = {0, 0}; pt_r_implicit_start = {M, 0};
              pa_l_implicit_start = {0, 0}; pa_r_implicit_start = {M, 0};
         } else {
              pt_l_implicit_start = {0, M}; pt_r_implicit_start = {M, M};
              pa_l_implicit_start = {0, M}; pa_r_implicit_start = {M, M};
         }
    }
    for (size_t i = 0; i < best_history.size(); ++i) {
         bool is_duplicate = false;
         if (i == 0) {
              if (best_history[0][0] == pt_l_implicit_start && best_history[0][1] == pt_r_implicit_start &&
                  best_history[0][2] == pa_l_implicit_start && best_history[0][3] == pa_r_implicit_start)
                  is_duplicate = true;
         } else {
             if (best_history[i] == best_history[i-1])
                 is_duplicate = true;
         }
         if (is_duplicate)
             continue;
         std::cout << best_history[i][0].x << " " << best_history[i][0].y << " "
                   << best_history[i][1].x << " " << best_history[i][1].y << " "
                   << best_history[i][2].x << " " << best_history[i][2].y << " "
                   << best_history[i][3].x << " " << best_history[i][3].y << "\n";
         if (i == 0)
             std::cout << best_history[i][0].x << " " << best_history[i][0].y << " "
                       << best_history[i][1].x << " " << best_history[i][1].y << " "
                       << best_history[i][2].x << " " << best_history[i][2].y << " "
                       << best_history[i][3].x << " " << best_history[i][3].y << "\n";
    }
    std::cout.flush();
  }
};

int main() {
  std::cin.tie(nullptr);
  std::ios_base::sync_with_stdio(false);
  std::cout << std::fixed << std::setprecision(0);
  Solver solver;
  solver.solve_4_sweeps_enhanced();
  solver.output();
  std::cerr << "Total execution time: " << utility::mytm.elapsed() << " ms" << std::endl;
  return 0;
}
