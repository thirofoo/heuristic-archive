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

struct State {
  int id;
  long long score;

  State(): score(0LL) {
    id = generate_id();
  }
  explicit State(long long _score): score(_score) {
    id = generate_id();
  }

  bool operator<(const State& other) const {
    return score < other.score;
  }
  bool operator>(const State& other) const {
    return score > other.score;
  }

  private:
  static int generate_id() {
    static int id_counter = 0;
    return id_counter++;
  }
};

struct Solver {
  struct Operation {
    int type, i, j, k;
  };

  int R;
  vector<vector<int>> dep;
  vector<deque<int>> side;
  vector<vector<Operation>> turns;

  Solver() {
    this->input();
    
  }

  void input() {
    cin >> R;
    dep.assign(R, {});
    side.assign(R, {});
    rep(r, R) {
      dep[r].resize(10);
      rep(c, 10) cin >> dep[r][c];
    }
  }

  void output() {
    if(is_complete()) {
      cerr << "Score = " << 100 * R + 4000 - (int)turns.size() << '\n';
    }
    cout << turns.size() << '\n';
    for(const auto& turn: turns) {
      cout << turn.size() << '\n';
      for(const auto& op: turn) {
        cout << op.type << ' ' << op.i << ' ' << op.j << ' ' << op.k << '\n';
      }
    }
    return;
  }

  void add_op(int type, int i, int j, int k = 1) {
    Operation op{type, i, j, k};
    turns.push_back({op});

    if(type == 0) {
      vector<int> moved;
      rep(_, k) {
        moved.push_back(dep[i].back());
        dep[i].pop_back();
      }
      reverse(moved.begin(), moved.end());
      for(int x = (int)moved.size() - 1; x >= 0; x--) side[j].push_front(moved[x]);
    } else {
      vector<int> moved;
      rep(_, k) {
        moved.push_back(side[j].front());
        side[j].pop_front();
      }
      for(int x: moved) dep[i].push_back(x);
    }
  }

  bool is_complete() const {
    rep(r, R) {
      if((int)dep[r].size() != 10) return false;
      rep(c, 10) {
        if(dep[r][c] != 10 * r + c) return false;
      }
    }
    return true;
  }

  void solve() {
    // Put each railcar into the siding corresponding to its final position.
    rep(i, R) {
      while(!dep[i].empty()) {
        int car = dep[i].back();
        int pos = car % 10;
        int k = 1;
        while(k < (int)dep[i].size() && dep[i][(int)dep[i].size() - 1 - k] % 10 == pos) {
          k++;
        }
        add_op(0, i, pos, k);
      }
    }

    // Restore positions from front to tail. Siding p contains exactly the
    // railcars whose final position is p, so any order inside the siding works.
    rep(pos, 10) {
      while(!side[pos].empty()) {
        int car = side[pos].front();
        add_op(1, car / 10, pos);
      }
    }

    assert((int)turns.size() <= 4000);
    assert(is_complete());
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
