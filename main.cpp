#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for (int i = 0; i < n; i++)

namespace utility {
  struct timer {
    chrono::system_clock::time_point start;
    void CodeStart() { start = chrono::system_clock::now(); }
    double elapsed() const {
      using namespace chrono;
      return (double)duration_cast<milliseconds>(system_clock::now() - start)
          .count();
    }
  } mytm;
}

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629,
                      tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty, ty = tz, tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline double rand_double() { return (double)(rand_int() % (int)1e9) / 1e9; }

inline double gaussian(double mean, double stddev) {
  // 標準正規分布からの乱数生成（Box-Muller変換
  double z0 = sqrt(-2.0 * log(rand_double())) * cos(2.0 * M_PI * rand_double());
  // 平均と標準偏差の変換
  return mean + z0 * stddev;
}

//-----------------以下から実装部分-----------------//

using ll = long long;
using P = pair<int, int>;

struct Op {
  int box_id;   // 箱の番号
  bool rotate;  // 0 : 素, 1 : 90°回転
  char dir;     // L : 無限遠右から左, U : 無限遠下から上
  int prev_id;  // 差し込む基準となる箱の番号
  string op_str;

  Op(int box_id, bool rotate, char dir, int prev_id)
      : box_id(box_id), rotate(rotate), dir(dir), prev_id(prev_id) {
    this->update_str();
    return;      
  }

  inline void update_str() {
    this->op_str = to_string(box_id) + " " + to_string(rotate) + " " + dir + " " + to_string(prev_id);
    return;
  }

  bool operator<(const Op& other) const {
    // op_str の辞書順比較 (一意性)
    return op_str < other.op_str;
  }
};

struct Solver {
  int N, T, sigma, output_cnt;
  vector<P> boxes;
  map<vector<Op>, P> memo;

  Solver() {
    this->input();
    output_cnt = 0;
    return;
  }

  void input() {
    cin >> N >> T >> sigma;
    boxes.resize(N);
    rep(i, N) {
      int w, h;
      cin >> w >> h;
      boxes[i] = P(w, h);
    }
    return;
  }

  void solve() {
    // Greedy 解法
    ll area_sum = 0;
    for(auto [w, h] : boxes) area_sum += (ll)w * (ll)h;
    int threshold = sqrt(area_sum);
    int ignore_cnt = sqrt(N) + 1;
    vector<pair<int, vector<Op>>> ops_list;

    while(threshold < sqrt(area_sum) * 1.2) {
      int now_idx = 0, max_height = 0;
      vector<Op> ops;

      bool is_valid;
      while(now_idx < N) {
        int total_h = 0, start_idx = now_idx;
        bool measure_flag = false;
        while(true) {
          auto && [w, h] = boxes[now_idx];
          if(now_idx - start_idx >= ignore_cnt && !measure_flag) {
            vector<Op> tmp_ops = vector<Op>(ops.begin() + start_idx, ops.end());
            auto [_, in_h] = output(tmp_ops);
            total_h = max(total_h, in_h);
            if(output_cnt >= T) return;
            measure_flag = true;
          }

          // サイズ引き延ばしができるか否か
          if(now_idx < N && total_h + min(w, h) <= threshold) {
            Op op(now_idx, (w < h), 'L', (now_idx == start_idx ? -1 : now_idx - 1));
            ops.emplace_back(op);
            total_h += min(w, h);
            now_idx++;
          }
          else break;
        }
        if(now_idx >= N) {
          is_valid = (total_h * 2 >= threshold || ops_list.empty());
        }
      }
      if(is_valid) {
        auto [th, tw] = output(ops);
        ops_list.emplace_back(tw + th, ops);
        if(output_cnt >= T) return;
      }
      threshold += 100;
    }
    sort(ops_list.begin(), ops_list.end());

    // ========== 残りの操作回数で最良解の箱をランダムに方向反転をして改良を狙う ========== 
    int cnt = 0, ops_idx = 0;
    while(output_cnt < T && cnt <= 1000) {
      cnt++;
      vector<int> flip_idx;
      int time = 1;
      auto && [best_score, best_ops] = ops_list[ops_idx];
      while(time > 0) {
        int idx = rand_int() % N;
        auto && [w, h] = boxes[idx];
        flip_idx.emplace_back(idx);
        time--;
      }
      
      rep(i, flip_idx.size()) {
        // 方向反転
        best_ops[flip_idx[i]].rotate = !best_ops[flip_idx[i]].rotate;
        best_ops[flip_idx[i]].update_str();
      }

      bool updated = false;
      if(!memo.count(best_ops)) {
        auto [th, tw] = output(best_ops);
        if(tw + th < best_score) {
          best_score = tw + th;
          updated = true;
          cnt = 0;
        }
      }
      if(updated) continue;
      rep(i, flip_idx.size()) {
        best_ops[flip_idx[i]].rotate = !best_ops[flip_idx[i]].rotate;
        best_ops[flip_idx[i]].update_str();
      }

      if(cnt >= 2 * N) {
        cnt = 0;
        ops_idx++;
        if(ops_idx >= ops_list.size()) {
          break;
        }
      }
    }

    // ※ あまりの手数は浪費
    while(output_cnt < T) {
      cout << ops_list[0].second.size() << endl;
      for(auto op : ops_list[0].second) cout << op.op_str << endl;
      cout << flush;
      output_cnt++;
      int tw, th;
      cin >> tw >> th;
    }

    return;
  }

  inline P output(const vector<Op> &ops) {
    if(memo.count(ops)) return memo[ops];
    cout << ops.size() << endl;
    for(auto op : ops) cout << op.op_str << endl;
    cout << flush;

    int tw, th;
    cin >> tw >> th;

    output_cnt++;
    memo[ops] = P(tw, th);
    return P(tw, th);
  }
};

int main() {
  cin.tie(0);
  ios_base::sync_with_stdio(false);

  Solver solver;
  solver.solve();

  return 0;
}
