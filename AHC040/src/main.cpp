#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
#define rep(i, n) for (int i = 0; i < n; i++)

namespace utility {
  struct timer {
    chrono::system_clock::time_point start;
    void CodeStart() {
      start = chrono::system_clock::now();
    }
    double elapsed() const {
      using namespace std::chrono;
      return (double) duration_cast<milliseconds>(system_clock::now() - start).count();
    }
  } mytm;
}

using ll = long long;
using P = pair<int, int>;

// ↓ ========== Lazy Segment Tree Config ========== ↓ //
using S = struct {
  ll max;
  ll size;
};
using F = optional<ll>;
S op(S l, S r) {return {max(l.max, r.max), l.size + r.size};}
S mapping(F f, S x) {
  if(f.has_value()) return {f.value(), x.size};
  return x;
}
F composition(F f, F g) {
  if(f.has_value()) return f;
  return g;
}
S e() {return {0, 0};}
F id() {return nullopt;}
// ↑ ========== Lazy Segment Tree Config ========== ↑ //

#define TIME_LIMIT 2800

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
    for (int i = 0; i < N; i++) {
        int w, h;
        cin >> w >> h;
        w = max(10000, min(100000, w));
        h = max(10000, min(100000, h));
        boxes[i] = {w, h};
    }
    utility::mytm.CodeStart();
    return;
  }

  void solve() {
    ll area_sum = 0;
    for(auto [w, h] : boxes) area_sum += (ll)w * (ll)h;
    int threshold = sqrt(area_sum);
    int ignore_cnt = sqrt(N) + 1;
    vector<pair<int, vector<Op>>> ops_list;

    while(threshold < sqrt(area_sum) * 1.2) {
      int now_idx = 0, max_height = 0;
      vector<Op> ops;
      while(now_idx < N) {
        int total_h = 0, start_idx = now_idx, pre_idx = -1, pre_pre_idx = -1;
        int max_w = 0, prev_w = 0, now_h = 0;
        bool measure_flag = false;
        while(now_idx < N) {
          // 余裕がある or 分散が大きい時は高さを計測
          bool allowance = (sigma >= 5000);
          if(now_idx - start_idx >= ignore_cnt && !measure_flag && allowance) {
            vector<Op> tmp_ops = vector<Op>(ops.begin() + start_idx, ops.end());
            auto [_, in_h] = output(tmp_ops);
            total_h = in_h;
            if(output_cnt >= T) return;
            measure_flag = true;
          }
          auto && [w, h] = boxes[now_idx];
          bool rotate = (w < h);
          auto [tw, th] = (rotate ? P(h, w) : P(w, h));

          if(max_w - prev_w >= tw) {
            Op op(now_idx, rotate, 'L', pre_pre_idx);
            ops.emplace_back(op);

            if(now_h < th) {
              now_h = th;
              pre_idx = now_idx;
            }

            now_idx = now_idx + 1;
            prev_w += tw;
          }
          else if(total_h + th <= threshold) {
            Op op(now_idx, rotate, 'L', pre_idx);
            ops.emplace_back(op);
            now_h = th;
            total_h += now_h;

            pre_pre_idx = pre_idx;
            pre_idx = now_idx;
            
            now_idx = now_idx + 1;
            prev_w = tw;
            max_w = max(max_w, prev_w);
          }
          else break;
        }

        // rotate を反転して高さ調整
        vector<int> perm;
        for(int i = start_idx; i < now_idx; i++) perm.emplace_back(i);
        sort(perm.begin(), perm.end(), [&](int i, int j) {
          auto && [w1, h1] = boxes[i];
          auto && [w2, h2] = boxes[j];
          return abs(w1 - h1) > abs(w2 - h2);
        });
        int flip_miss_cnt = 0;
        for(int i = start_idx; i < now_idx; i++) {
          int idx = perm[i - start_idx];
          auto && [w, h] = boxes[idx];
          auto [tw, th] = (ops[idx].rotate ? P(h, w) : P(w, h));
          if(total_h - th + tw >= threshold) {
            flip_miss_cnt++;
            if(flip_miss_cnt * sigma >= 10000) break;
            else continue;
          }
          ops[idx].rotate = !ops[idx].rotate;
          total_h = total_h - th + tw;
          ops[idx].update_str();
        }
      }

      auto [th, tw] = output(ops);
      ops_list.emplace_back(tw + th, ops);
      if(output_cnt >= T) return;
      threshold += 100;
    }
    sort(ops_list.begin(), ops_list.end());

    // ========== 残りの操作回数で最良解の箱をランダムに方向反転をして改良を狙う ========== 
    int cnt = 0, ops_idx = 0;
    lazy_segtree<S, op, e, F, mapping, composition, id> seg(2000000);
    auto [best_score, best_ops] = ops_list[0];

    int cache_hit = 0, iter = 0;
    while(output_cnt < T && utility::mytm.elapsed() < TIME_LIMIT) {
      // 一つの向きを flip した時にランダム誤差で期待値が一番高い操作を行う
      // cerr << "# output_cnt : " << output_cnt << endl;
      // cerr << "# iter : " << iter++ << endl;

      int best_operate = -1;
      ll best_expect = 1e16;
      
      rep(i, best_ops.size()) {
        // 操作後が既に検証済みの場合は skip
        best_ops[i].rotate = !best_ops[i].rotate;
        best_ops[i].update_str();
        bool flag = (memo.count(best_ops) > 0);
        best_ops[i].rotate = !best_ops[i].rotate;
        best_ops[i].update_str();
        if(flag) continue;

        ll expect_sum = 0;
        ll start_h = 0;
        ll max_h = -1, max_w = -1;
        seg.apply(0, 2000000, 0);
        rep(j, best_ops.size()) {
          if(best_ops[j].prev_id == -1) {
            max_h = max(max_h, start_h);
            start_h = 0;
          }
          auto [w, h] = boxes[j];
          bool true_rotate = best_ops[j].rotate ^ (i == j);
          auto [tw, th] = (true_rotate ? P(h, w) : P(w, h));
          int next_w = seg.prod(start_h, start_h + th).max + tw;
          seg.apply(start_h, start_h + th, next_w);
          start_h += th;
        }
        max_h = max(max_h, start_h);
        max_w = seg.prod(0, 2000000).max;
        if(max_h + max_w < best_expect) {
          best_expect = max_h + max_w;
          best_operate = i;
        }
      }

      // cout << "# best_expect : " << best_expect << endl;
      // cout << "# best_expect / SAMPLE_NUM : " << best_expect / SAMPLE_NUM << endl;
      // cout << "# best_operate : " << best_operate << endl;
      // cerr << "# cache_hit : " << cache_hit << endl;
      cache_hit += (memo.count(best_ops) > 0);
      if(best_operate == -1) break;

      best_ops[best_operate].rotate = !best_ops[best_operate].rotate;
      best_ops[best_operate].update_str();

      auto [th, tw] = output(best_ops);
      if(tw + th < best_score + sigma) {
        best_score = tw + th;
      } else {
        best_ops[best_operate].rotate = !best_ops[best_operate].rotate;
        best_ops[best_operate].update_str();
      }
    }

    // ※ あまりの手数は浪費
    while(output_cnt < T) {
      cout << best_ops.size() << endl;
      for(auto op : best_ops) cout << op.op_str << endl;
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
