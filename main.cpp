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

struct Solver {
  int N, T, sigma;
  vector<P> boxes;

  Solver() { this->input(); }

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
    ll sum = 0;
    for(auto [w, h] : boxes) sum += (ll)w * (ll)h;
    set<int> threshold_set, used_threshold;
    set<vector<string>> hash_set;
    int threshold = sqrt(sum);
    int turn = 0, th, tw;

    // ========== 1. 再起関数を用いて折り返し地点を探索 ========== 
    constexpr int TURN_MAX = 1;
    vector<string> answer, best_answer;
    int output_num = 0, best_score = 1e9;
    vector<int> rotate_flag(N, -1);
    rep(i, N) {
      auto &&[w, h] = boxes[i];
      rotate_flag[i] = (w < h);
    }

    auto dfs = [&](auto self, int idx, int carry_cnt) -> void {
      if(output_num >= T) return;

      int total_height = 0, pre_box = -1, pile_cnt = 0;
      while(idx < N && total_height + boxes[idx].second <= threshold) {
        auto &&[w, h] = boxes[idx];
        bool rotate = rotate_flag[idx];
        total_height += (rotate ? w : h);
        answer.emplace_back(to_string(idx) + " " + to_string(rotate) + " L " + to_string(pre_box));
        pre_box = idx;
        idx++;
        pile_cnt++;
      }

      for(int i = 0; i + carry_cnt <= TURN_MAX && idx + i < N; i++) {
        rep(j, i) {
          auto &&[w, h] = boxes[idx + j];
          answer.emplace_back(to_string(idx + j) + " " + to_string(w < h) + " L " + to_string(pre_box));
          pre_box = idx + j;
        }
        self(self, idx + i, carry_cnt + i);
        if(output_num >= T) return;
        rep(j, i) {
          auto &&[w, h] = boxes[idx + j];
          answer.pop_back();
          pre_box -= 1;
        }
      }
      if(idx == N && !hash_set.count(answer) && (total_height * 4 >= threshold * 3 || hash_set.empty())) {
        // あまりにも列を無駄にしてる場合も無視
        hash_set.insert(answer);
        cout << answer.size() << endl << flush;
        for(auto s : answer) cout << s << endl << flush;
        cin >> tw >> th;
        if(tw + th < best_score) {
          best_score = tw + th;
          best_answer = answer;
        }
        output_num++;
        if(output_num >= T) return;
      }
      while(pile_cnt-- > 0) answer.pop_back();
      return;
    };
    while(output_num < T && threshold <= sqrt(sum * 2LL)) {
      dfs(dfs, 0, 0);
      threshold += sigma;
    }

    // ========== 2. 残りの操作回数で最良解の箱をランダムに方向反転をして改良を狙う ========== 
    while(output_num < T) {
      vector<int> flip_idx;
      int time = 1;
      while(time--) flip_idx.emplace_back(rand_int() % best_answer.size());
      // 方向反転
      rep(i, flip_idx.size()) {
        int idx = flip_idx[i];
        rep(j, best_answer[idx].size()) {
          if(best_answer[idx][j] != ' ') continue;
          best_answer[idx][j + 1] = (best_answer[idx][j + 1] == '0' ? '1' : '0');
          break;
        }
      }
      cout << best_answer.size() << endl << flush;
      for(auto s : best_answer) cout << s << endl << flush;
      cin >> tw >> th;
      if(tw + th < best_score) best_score = tw + th;
      else {
        rep(i, flip_idx.size()) {
          int idx = flip_idx[i];
          rep(j, best_answer[idx].size()) {
            if(best_answer[idx][j] != ' ') continue;
            best_answer[idx][j + 1] = (best_answer[idx][j + 1] == '0' ? '1' : '0');
            break;
          }
        }
      }
      output_num++;
    }

    return;
  }
};

int main() {
  cin.tie(0);
  ios_base::sync_with_stdio(false);

  Solver solver;
  solver.solve();

  return 0;
}
