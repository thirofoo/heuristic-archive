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
#define TIME_LIMIT 2950
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

  void solve() {
    // 現在の状態を管理する変数
    vector<long long> current_H = H;
    vector<int> current_C = C;
    vector<bool> is_opened(N, false);
    int opened_count = 0;

    // 全ての宝箱が開くまでループ
    while (opened_count < N) {
      // Step 1: 利用可能な武器の中で、最も攻撃力の高い攻撃を探す
      int best_w = -1; // 最適な武器
      int best_b = -1; // 最適なターゲット宝箱
      int max_power = 0; // 最大攻撃力

      rep(w, N) {
        // 武器wが利用可能か (宝箱wが開いていて、耐久値が残っている)
        if (is_opened[w] && current_C[w] > 0) {
          rep(b, N) {
            // 宝箱bがまだ開いていないか
            if (!is_opened[b]) {
              if (A[w][b] > max_power) {
                max_power = A[w][b];
                best_w = w;
                best_b = b;
              }
            }
          }
        }
      }

      // Step 2: 状況に応じて行動を決定
      // 攻撃力が1より大きい効率的な武器攻撃が見つかった場合
      if (max_power > 1) {
        plan.emplace_back(best_w, best_b);
        current_H[best_b] -= A[best_w][best_b];
        current_C[best_w]--;

        // 攻撃によって宝箱が開いたかチェック
        if (current_H[best_b] <= 0 && !is_opened[best_b]) {
          is_opened[best_b] = true;
          opened_count++;
        }
      } else { 
        // 有効な武器攻撃がない場合、素手で攻撃する宝箱を決める
        // 残り硬さが最も少ない未開封の宝箱を探す
        int target_b = -1;
        long long min_rem_h = 2e18; // 十分に大きい値で初期化

        rep(b, N) {
          if (!is_opened[b]) {
            if (current_H[b] < min_rem_h) {
              min_rem_h = current_H[b];
              target_b = b;
            }
          }
        }

        // 対象の宝箱が見つかったら、それを開ける
        if (target_b != -1) {
          // もし硬さがプラスなら、その回数だけ素手で攻撃
          if (current_H[target_b] > 0) {
            rep(k, current_H[target_b]) {
              plan.emplace_back(-1, target_b);
            }
            current_H[target_b] = 0;
          }
          
          // 宝箱を開ける
          if (!is_opened[target_b]) {
            is_opened[target_b] = true;
            opened_count++;
          }
        }
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
