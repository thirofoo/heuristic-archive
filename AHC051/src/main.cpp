#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <numeric>
#include <tuple>
#include <limits>
#include <map>

// --- ユーティリティ & 幾何学関連 ---
namespace utility {
    struct timer {
        std::chrono::system_clock::time_point start;
        void CodeStart() { start = std::chrono::system_clock::now(); }
        double elapsed() const {
            using namespace std::chrono;
            return (double)duration_cast<milliseconds>(system_clock::now() - start).count();
        }
    };
}
const double TSP_TIME_LIMIT = 1000.0; // DAG構築に使う時間(ms)

struct Point { long long x, y; int id; };
const Point INLET_POS = {0, 5000, -1};
struct Segment { Point p1, p2; };
long long distSq(const Point& p1, const Point& p2) {
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}
int sign(long long val) { return (val > 0) - (val < 0); }
int orientation(Point a, Point b, Point c) {
    return sign((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
}
bool segments_intersect(Point p1, Point p2, Point q1, Point q2) {
    if ((p1.x == q1.x && p1.y == q1.y) || (p1.x == q2.x && p1.y == q2.y) ||
        (p2.x == q1.x && p2.y == q1.y) || (p2.x == q2.x && p2.y == q2.y)) return false;
    if (std::max(p1.x, p2.x) < std::min(q1.x, q2.x) || std::max(q1.x, q2.x) < std::min(p1.x, p2.x) ||
        std::max(p1.y, p2.y) < std::min(q1.y, q2.y) || std::max(q1.y, q2.y) < std::min(p1.y, p2.y)) return false;
    int o1 = orientation(p1, p2, q1), o2 = orientation(p1, p2, q2), o3 = orientation(q1, q2, p1), o4 = orientation(q1, q2, p2);
    return (o1 * o2 < 0 && o3 * o4 < 0);
}
struct SorterConfig { int type = -1, exit1_to = -1, exit2_to = -1; };

double calculate_variance(const std::vector<double>& probs) {
    if (probs.empty()) return 0.0;
    double sum = 0, sum_sq = 0;
    int count = 0;
    for (double p : probs) {
        if (p > 1e-9) { // 確率が0に近いものは無視
            sum += p;
            sum_sq += p * p;
            count++;
        }
    }
    if (count == 0) return 0.0;
    double mean = sum / count;
    return (sum_sq / count) - (mean * mean);
}

struct Solver {
    int N, M, K;
    std::vector<Point> processor_locations;
    std::vector<Point> sorter_locations;
    std::vector<std::vector<double>> probabilities;
    std::map<int, int> sorter_to_proc;

    std::vector<int> final_proc_assign;
    int final_inlet_to;
    std::vector<SorterConfig> final_sorter_configs;

    Solver() {
        std::cin.tie(0); std::ios_base::sync_with_stdio(false);
        input();
    }

    void input() {
        std::cin >> N >> M >> K;
        processor_locations.resize(N);
        sorter_locations.resize(M);
        probabilities.resize(K, std::vector<double>(N));
        for (int i = 0; i < N; ++i) { std::cin >> processor_locations[i].x >> processor_locations[i].y; processor_locations[i].id = i; }
        for (int i = 0; i < M; ++i) { std::cin >> sorter_locations[i].x >> sorter_locations[i].y; sorter_locations[i].id = i; }
        for (int i = 0; i < K; ++i) { for (int j = 0; j < N; ++j) { std::cin >> probabilities[i][j]; } }
    }
    
    int count_intersections(const std::vector<int>& tour) {
        if (tour.empty() || tour.size() != (size_t)N) return 1e9;
        std::vector<Segment> belts;
        belts.push_back({INLET_POS, sorter_locations[tour[0]]});
        for (size_t i = 0; i < tour.size(); ++i) {
            int s_idx = tour[i];
            int p_idx = sorter_to_proc.at(s_idx);
            belts.push_back({sorter_locations[s_idx], processor_locations[p_idx]});
            if (i < tour.size() - 1) {
                belts.push_back({sorter_locations[s_idx], sorter_locations[tour[i+1]]});
            }
        }
        int intersections = 0;
        for (size_t i = 0; i < belts.size(); ++i) {
            for (size_t j = i + 1; j < belts.size(); ++j) {
                if (segments_intersect(belts[i].p1, belts[i].p2, belts[j].p1, belts[j].p2)) {
                    intersections++;
                }
            }
        }
        return intersections;
    }

    void solve() {
        utility::timer local_timer;
        local_timer.CodeStart();
        
        // --- フェーズ1: ペアリング & TSPによるDAG構築 ---
        std::vector<int> paired_sorter_indices;
        std::vector<bool> sorter_is_paired(M, false);
        for (int i = 0; i < N; ++i) {
            long long min_d = -1; int best_s_idx = -1;
            for (int j = 0; j < M; ++j) {
                if (!sorter_is_paired[j]) {
                    long long d = distSq(processor_locations[i], sorter_locations[j]);
                    if (best_s_idx == -1 || d < min_d) { min_d = d; best_s_idx = j; }
                }
            }
            if (best_s_idx != -1) {
                sorter_to_proc[best_s_idx] = i;
                paired_sorter_indices.push_back(best_s_idx);
                sorter_is_paired[best_s_idx] = true;
            }
        }

        std::vector<int> tour;
        std::vector<bool> visited(M, false);
        Point current_pos = INLET_POS;
        for (int i = 0; i < N; ++i) {
            long long min_d = -1; int next_s_idx = -1;
            for (int s_idx : paired_sorter_indices) {
                if (!visited[s_idx]) {
                    long long d = distSq(current_pos, sorter_locations[s_idx]);
                    if (next_s_idx == -1 || d < min_d) { min_d = d; next_s_idx = s_idx; }
                }
            }
            if (next_s_idx != -1) {
                tour.push_back(next_s_idx);
                visited[next_s_idx] = true;
                current_pos = sorter_locations[next_s_idx];
            }
        }
        
        int current_intersections = count_intersections(tour);
        while (local_timer.elapsed() < TSP_TIME_LIMIT && current_intersections > 0) {
            bool updated = false;
            for (int i = 0; i < N - 1; ++i) {
                for (int j = i + 1; j < N; ++j) {
                    std::vector<int> new_tour = tour;
                    std::reverse(new_tour.begin() + i, new_tour.begin() + j + 1);
                    int new_intersections = count_intersections(new_tour);
                    if (new_intersections < current_intersections) {
                        tour = new_tour;
                        current_intersections = new_intersections;
                        updated = true;
                        goto next_iteration;
                    }
                }
            }
            next_iteration:;
            if (!updated) break;
        }
        
        // --- フェーズ2: 確率伝播シミュレーションによる割り当て ---
        final_proc_assign.assign(N, -1);
        final_sorter_configs.resize(M);
        std::vector<double> current_waste_probs(N, 1.0);
        std::vector<bool> waste_type_is_assigned(N, false);

        final_inlet_to = N + tour[0];

        for (size_t i = 0; i < tour.size(); ++i) {
            int s_idx = tour[i];
            int p_idx = sorter_to_proc.at(s_idx);

            int best_k = -1;
            double max_variance_sum = -1.0;
            for (int k = 0; k < K; ++k) {
                std::vector<double> exit1_probs, exit2_probs;
                for(int j=0; j<N; ++j){
                    exit1_probs.push_back(current_waste_probs[j] * probabilities[k][j]);
                    exit2_probs.push_back(current_waste_probs[j] * (1.0 - probabilities[k][j]));
                }
                double current_variance = calculate_variance(exit1_probs) + calculate_variance(exit2_probs);
                if(current_variance > max_variance_sum){
                    max_variance_sum = current_variance;
                    best_k = k;
                }
            }
            final_sorter_configs[s_idx].type = best_k;
            
            int waste_type_to_assign = -1;
            double max_prob_val = -1.0;
            for(int j=0; j<N; ++j){
                if(!waste_type_is_assigned[j] && current_waste_probs[j] > max_prob_val){
                    max_prob_val = current_waste_probs[j];
                    waste_type_to_assign = j;
                }
            }
            if(waste_type_to_assign == -1) {
                for(int j=0; j<N; ++j) if(!waste_type_is_assigned[j]) { waste_type_to_assign = j; break; }
            }

            final_proc_assign[p_idx] = waste_type_to_assign;
            waste_type_is_assigned[waste_type_to_assign] = true;
            
            int clean_dest = p_idx;
            int waste_dest = (i < tour.size() - 1) ? (N + tour[i+1]) : p_idx;
            
            double prob_for_target_at_exit1 = probabilities[best_k][waste_type_to_assign];
            
            std::vector<double> next_waste_probs(N);
            if (prob_for_target_at_exit1 > 0.5) { // 出口1に目的のゴミが行きやすいと判断
                final_sorter_configs[s_idx].exit1_to = clean_dest;
                final_sorter_configs[s_idx].exit2_to = waste_dest;
                for(int j=0; j<N; ++j) next_waste_probs[j] = current_waste_probs[j] * (1.0 - probabilities[best_k][j]);
            } else {
                final_sorter_configs[s_idx].exit1_to = waste_dest;
                final_sorter_configs[s_idx].exit2_to = clean_dest;
                for(int j=0; j<N; ++j) next_waste_probs[j] = current_waste_probs[j] * probabilities[best_k][j];
            }
            current_waste_probs = next_waste_probs;
        }
    }
    
    void output() {
        for (int i = 0; i < N; ++i) { std::cout << final_proc_assign[i] << (i == N - 1 ? "" : " "); }
        std::cout << std::endl;
        std::cout << final_inlet_to << std::endl;
        for (int i = 0; i < M; ++i) {
            if (final_sorter_configs[i].type == -1) { std::cout << -1 << std::endl; }
            else { std::cout << final_sorter_configs[i].type << " " << final_sorter_configs[i].exit1_to << " " << final_sorter_configs[i].exit2_to << std::endl; }
        }
    }
};

int main() {
    Solver solver;
    solver.solve();
    solver.output();
    return 0;
}
