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

inline double gaussian(double mean, double stddev) {
  // 標準正規分布からの乱数生成（Box-Muller変換
  double z0 = sqrt(-2.0 * log(rand_double())) * cos(2.0 * M_PI * rand_double());
  // 平均と標準偏差の変換
  return mean + z0 * stddev;
}

// 温度関数
#define TIME_LIMIT 1950
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now, int start) {
  return exp((double) (now - best) / temp(start));
}

// ビームサーチの結果復元用の構造体
struct Trace {
  char op;
  int parend_id;
  Trace(): parend_id(0) {}
  explicit Trace(char _op, int _parend_id): op(_op), parend_id(_parend_id) {}
};

vector<Trace> traces;
string restore(int best_id, int initial_id) {
  string ops;
  int current_id = best_id;
  while(current_id != initial_id) {
    ops += traces[current_id].op;
    current_id = traces[current_id].parend_id;
  }
  reverse(ops.begin(), ops.end());
  return ops;
}

//-----------------以下から実装部分-----------------//

struct Solver {
  int N;
  vector<long long> H;
  vector<int> C;
  vector<vector<int>> A;
  vector<pair<int, int>> plan; // 実行する攻撃の計画

  Solver() {
    this->input();
  }

  void input() {
    cin >> N;
    H.resize(N);
    C.resize(N);
    A.assign(N, vector<int>(N));
    rep(i, N) cin >> H[i];
    rep(i, N) cin >> C[i];
    rep(i, N) rep(j, N) cin >> A[i][j];
  }

  void output() {
    for (const auto& p : plan) {
      cout << p.first << " " << p.second << endl;
    }
  }

  long long simulate_global(vector<pair<int, int>>* out_plan) const {
    vector<long long> current_H = H;
    vector<int> current_C = C;
    vector<bool> is_opened(N, false);
    int opened_count = 0;
    long long attacks = 0;
    vector<pair<int, int>> local_plan;
    if (out_plan) local_plan.reserve(accumulate(H.begin(), H.end(), 0LL));

    while (opened_count < N) {
      int best_w = -1;
      int best_b = -1;
      int max_power = 1;

      rep(w, N) {
        if (!is_opened[w] || current_C[w] <= 0) continue;
        rep(b, N) {
          if (is_opened[b]) continue;
          if (A[w][b] > max_power) {
            max_power = A[w][b];
            best_w = w;
            best_b = b;
          }
        }
      }

      if (best_w != -1) {
        attacks++;
        if (out_plan) local_plan.emplace_back(best_w, best_b);
        current_H[best_b] -= A[best_w][best_b];
        current_C[best_w]--;
        if (current_H[best_b] <= 0 && !is_opened[best_b]) {
          is_opened[best_b] = true;
          opened_count++;
        }
      } else {
        int target_b = -1;
        long long min_rem_h = (long long) 4e18;
        rep(b, N) {
          if (is_opened[b]) continue;
          if (current_H[b] < min_rem_h) {
            min_rem_h = current_H[b];
            target_b = b;
          }
        }
        if (target_b != -1) {
          long long hits = max(0LL, current_H[target_b]);
          attacks += hits;
          if (out_plan) {
            for (long long k = 0; k < hits; k++) {
              local_plan.emplace_back(-1, target_b);
            }
          }
          current_H[target_b] = 0;
          if (!is_opened[target_b]) {
            is_opened[target_b] = true;
            opened_count++;
          }
        } else {
          break;
        }
      }
    }

    if (out_plan) *out_plan = move(local_plan);
    return attacks;
  }

  long long simulate_with_priority(const vector<int>& priority_raw, vector<pair<int, int>>* out_plan) const {
    vector<int> order;
    order.reserve(N);
    vector<char> used(N, 0);
    for (int v : priority_raw) {
      if (v < 0 || v >= N) continue;
      if (used[v]) continue;
      used[v] = 1;
      order.push_back(v);
    }
    rep(i, N) {
      if (!used[i]) order.push_back(i);
    }

    vector<long long> current_H = H;
    vector<int> current_C = C;
    vector<char> is_opened(N, 0);
    vector<int> rank(N);
    rep(i, N) rank[order[i]] = i;

    long long attacks = 0;
    vector<pair<int, int>> local_plan;
    if (out_plan) local_plan.reserve(accumulate(H.begin(), H.end(), 0LL));

    int order_ptr = 0;

    auto next_target = [&](vector<char>& opened_flags) -> int {
      while (order_ptr < N) {
        int cand = order[order_ptr++];
        if (!opened_flags[cand]) return cand;
      }
      int best_b = -1;
      long long best_h = (long long) 4e18;
      rep(b, N) {
        if (opened_flags[b]) continue;
        if (current_H[b] < best_h) {
          best_h = current_H[b];
          best_b = b;
        }
      }
      return best_b;
    };

    int opened_count = 0;

    while (opened_count < N) {
      int best_w = -1;
      int best_b = -1;
      int max_power = 1;

      rep(w, N) {
        if (!is_opened[w] || current_C[w] <= 0) continue;
        rep(b, N) {
          if (is_opened[b]) continue;
          int damage = A[w][b];
          if (damage > max_power) {
            max_power = damage;
            best_w = w;
            best_b = b;
          } else if (damage == max_power && damage > 1 && best_b != -1) {
            if (rank[b] < rank[best_b]) {
              best_w = w;
              best_b = b;
            }
          }
        }
      }

      if (best_w != -1) {
        attacks++;
        if (out_plan) local_plan.emplace_back(best_w, best_b);
        current_H[best_b] -= A[best_w][best_b];
        current_C[best_w]--;
        if (current_H[best_b] <= 0 && !is_opened[best_b]) {
          is_opened[best_b] = 1;
          opened_count++;
        }
      } else {
        int target = next_target(is_opened);
        if (target == -1) break;
        long long hits = max(0LL, current_H[target]);
        attacks += hits;
        if (out_plan) {
          for (long long k = 0; k < hits; k++) {
            local_plan.emplace_back(-1, target);
          }
        }
        current_H[target] = 0;
        if (!is_opened[target]) {
          is_opened[target] = 1;
          opened_count++;
        }
      }
    }

    if (out_plan) *out_plan = move(local_plan);
    return attacks;
  }

  void solve() {
    vector<pair<int, int>> best_plan_local;
    long long best_attacks = simulate_global(&best_plan_local);
    plan = best_plan_local;

    if (N <= 1) return;

    vector<int> base_order(N);
    iota(base_order.begin(), base_order.end(), 0);

    vector<int> best_priority = base_order;
    long long best_priority_attacks = simulate_with_priority(base_order, nullptr);

    auto consider_order = [&](const vector<int>& order) {
      long long attacks = simulate_with_priority(order, nullptr);
      if (attacks < best_priority_attacks) {
        best_priority_attacks = attacks;
        best_priority = order;
      }
    };

    vector<int> order_by_H = base_order;
    sort(order_by_H.begin(), order_by_H.end(), [&](int lhs, int rhs) {
      if (H[lhs] != H[rhs]) return H[lhs] < H[rhs];
      return lhs < rhs;
    });
    consider_order(order_by_H);

    vector<int> order_by_C = base_order;
    sort(order_by_C.begin(), order_by_C.end(), [&](int lhs, int rhs) {
      if (C[lhs] != C[rhs]) return C[lhs] > C[rhs];
      return lhs < rhs;
    });
    consider_order(order_by_C);

    vector<int> order_by_ratio = base_order;
    vector<double> value_ratio(N);
    rep(i, N) {
      int max_damage = 0;
      long long total_damage = 0;
      rep(j, N) {
        max_damage = max(max_damage, A[i][j]);
        total_damage += A[i][j];
      }
      value_ratio[i] = (max_damage == 0 ? 1.0 : (double) H[i] / (double) max_damage);
      if (total_damage == 0) value_ratio[i] += 1e6;
    };
    sort(order_by_ratio.begin(), order_by_ratio.end(), [&](int lhs, int rhs) {
      if (value_ratio[lhs] != value_ratio[rhs]) return value_ratio[lhs] < value_ratio[rhs];
      return lhs < rhs;
    });
    consider_order(order_by_ratio);

    vector<int> order_by_strength = base_order;
    vector<long long> strength(N, 0);
    rep(i, N) {
      long long max_damage = 0;
      long long sum_damage = 0;
      rep(j, N) {
        max_damage = max<long long>(max_damage, A[i][j]);
        sum_damage += A[i][j];
      }
      strength[i] = max_damage * 1000 + sum_damage;
    }
    sort(order_by_strength.begin(), order_by_strength.end(), [&](int lhs, int rhs) {
      if (strength[lhs] != strength[rhs]) return strength[lhs] > strength[rhs];
      return lhs < rhs;
    });
    consider_order(order_by_strength);

    const int random_trials = min(40, N * 2 + 10);
    vector<int> random_order = base_order;
    rep(iter, random_trials) {
      rep(k, N) {
        int i = rand_int() % N;
        int j = rand_int() % N;
        swap(random_order[i], random_order[j]);
      }
      consider_order(random_order);
    }

    vector<int> current_order = best_priority;
    long long current_attacks = simulate_with_priority(current_order, nullptr);
    best_priority_attacks = current_attacks;
    best_priority = current_order;

    double anneal_start = utility::mytm.elapsed();
    if (current_attacks < (long long) 4e18) {
      while (utility::mytm.elapsed() < TIME_LIMIT - 5) {
        vector<int> next_order = current_order;
        int move_type = rand_int() % 3;
        if (move_type == 0) {
          int i = rand_int() % N;
          int j = rand_int() % N;
          if (i != j) swap(next_order[i], next_order[j]);
        } else if (move_type == 1) {
          int l = rand_int() % N;
          int r = rand_int() % N;
          if (l > r) swap(l, r);
          reverse(next_order.begin() + l, next_order.begin() + r + 1);
        } else {
          int from = rand_int() % N;
          int to = rand_int() % N;
          if (from != to) {
            int val = next_order[from];
            next_order.erase(next_order.begin() + from);
            next_order.insert(next_order.begin() + to, val);
          }
        }

        long long next_attacks = simulate_with_priority(next_order, nullptr);
        bool accept = false;
        if (next_attacks <= current_attacks) {
          accept = true;
        } else {
          double temperature = max(1.0, temp(anneal_start));
          double probability = exp((double) (current_attacks - next_attacks) / temperature);
          if (probability > rand_double()) accept = true;
        }

        if (accept) {
          current_order.swap(next_order);
          current_attacks = next_attacks;
          if (current_attacks < best_priority_attacks) {
            best_priority_attacks = current_attacks;
            best_priority = current_order;
          }
        }

        if (utility::mytm.elapsed() - anneal_start > TIME_LIMIT) break;
      }
    }

    if (best_priority_attacks < best_attacks) {
      vector<pair<int, int>> new_plan;
      simulate_with_priority(best_priority, &new_plan);
      plan = move(new_plan);
      best_attacks = best_priority_attacks;
    }
  }
};

int main() {
  cin.tie(0);
  ios_base::sync_with_stdio(false);
  utility::mytm.CodeStart();

  Solver solver;
  solver.solve();
  solver.output();

  return 0;
}
