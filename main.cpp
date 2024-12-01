#include <atcoder/all>
#include <bits/stdc++.h>
using namespace std;
using namespace atcoder;
#define rep(i, n) for (int i = 0; i < n; i++)

namespace utility {
struct timer {
  chrono::system_clock::time_point start;
  // 開始時間を記録
  void CodeStart() { start = chrono::system_clock::now(); }
  // 経過時間 (ms) を返す
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

inline double rand_double() { return (double)(rand_int() % (int)1e9) / 1e9; }

inline double gaussian(double mean, double stddev) {
  // 標準正規分布からの乱数生成（Box-Muller変換
  double z0 = sqrt(-2.0 * log(rand_double())) * cos(2.0 * M_PI * rand_double());
  // 平均と標準偏差の変換
  return mean + z0 * stddev;
}

//-----------------以下から実装部分-----------------//

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
    int sum = 0;
    for(auto [w, h] : boxes) sum += w + h;

    rep(i, T) {
      cout << N << endl;
      int threshold = sum / (i + 1);
      int now_h = 0, pre_box = -1;
      rep(i, N) {
        auto [w, h] = boxes[i];
        cout << i << " " << (w < h ? "1" : "0") << " L " << pre_box << endl << flush;
        pre_box = i;
        now_h += h;
        if(now_h > threshold) {
          now_h = 0;
          pre_box = -1;
        }
      }
      int input;
      cin >> input;
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