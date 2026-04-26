#include <bits/stdc++.h>
using namespace std;

#define rep(i, n) for(int i = 0; i < (int)(n); i++)

namespace utility {
  struct Timer {
    chrono::steady_clock::time_point start;
    void CodeStart() {
      start = chrono::steady_clock::now();
    }
    double elapsed() const {
      using namespace chrono;
      return (double)duration_cast<milliseconds>(steady_clock::now() - start).count();
    }
  } mytm;
}

static constexpr int RMAX = 10;
static constexpr int DEP_CAP = 15;
static constexpr int SIDE_CAP = 20;
static constexpr int INIT_LEN = 10;
static constexpr int TOTAL = 100;

static constexpr int TIME_LIMIT_MS = 1900;

static constexpr int BEAM_WIDTH = 200;
static constexpr int TOP_TRANSITIONS = 10;

static constexpr long long EVAL_CORRECT_PREFIX_WEIGHT = 500'000'000LL;
static constexpr long long EVAL_BUILDABLE_ROWS_WEIGHT = 30'000'000LL;
static constexpr long long EVAL_SIDE_CARS_WEIGHT = 1'000'000LL;
static constexpr long long EVAL_REMAINING_BLOCKS_WEIGHT = 10'000LL;
static constexpr long long EVAL_ACCEPTOR_SHORTAGE_WEIGHT = 20'000LL;
static constexpr long long EVAL_SIDE_BALANCE_WEIGHT = 2'000LL;
static constexpr long long EVAL_BUCKET_QUALITY_WEIGHT = 30'000LL;
static constexpr long long EVAL_READY_OPS_WEIGHT = 5'000'000LL;
static constexpr long long EVAL_LAST_MOVED_OPS_WEIGHT = 2'000'000LL;
static constexpr long long EVAL_LAST_MOVED_CARS_WEIGHT = 100'000LL;
static constexpr long long EVAL_TURNS_WEIGHT = 1'000LL;

static constexpr long long LOCAL_MOVED_OPS_WEIGHT = 10'000'000LL;
static constexpr long long LOCAL_MOVED_CARS_WEIGHT = 2'000'000LL;
static constexpr long long LOCAL_RESTORE_OPS_BONUS = 50'000'000LL;
static constexpr long long LOCAL_RESTORE_CARS_BONUS = 5'000'000LL;
static constexpr long long LOCAL_CROSSING_PRESSURE_WEIGHT = 10'000LL;
static constexpr long long LOCAL_BUCKET_FIT_WEIGHT = 80'000LL;

static constexpr int SIDE_FRONT_ZERO_EXTRA_PENALTY = 200;
static constexpr int DEAD_ACCEPTOR_BASE_PENALTY = 100000;
static constexpr int DEAD_ACCEPTOR_TAIL_WEIGHT = 1000;
static constexpr int ACCEPTOR_SHORTAGE_DEAD_WEIGHT = 100000;

struct Operation {
  int type, i, j, k;
};

struct Turn {
  array<Operation, RMAX> ops{};
  unsigned char count = 0;
};

struct Trace {
  int parent;
  Turn turn;
};

vector<Trace> traces;

struct MoveCandidate {
  int type, i, j, k;
};

struct TurnCandidate {
  Turn turn;
  int moved_cars = 0;
  int moved_ops = 0;
  int restore_ops = 0;
  int restore_cars = 0;
  int crossing_pressure = 0;
  int bucket_fit = 0;
  long long local_score = 0;
};

struct State {
  array<array<unsigned char, DEP_CAP>, RMAX> dep{};
  array<array<unsigned char, SIDE_CAP>, RMAX> side{};
  array<unsigned char, RMAX> dep_len{};
  array<unsigned char, RMAX> side_len{};

  int turns = 0;
  int trace_id = -1;
  int last_moved_ops = 0;
  int last_moved_cars = 0;
};

struct BeamItem {
  long long score;
  State st;
  int parent_trace;
  Turn turn;

  bool operator<(const BeamItem& other) const {
    return score > other.score;
  }
};

struct Solver {
  int R;
  State initial;

  Solver() {
    input();
  }

  void input() {
    cin >> R;
    rep(i, R) {
      initial.dep_len[i] = INIT_LEN;
      initial.side_len[i] = 0;
      rep(j, INIT_LEN) {
        int x;
        cin >> x;
        initial.dep[i][j] = (unsigned char)x;
      }
    }
  }

  bool dep_empty(const State& st) const {
    rep(i, R) {
      if(st.dep_len[i] != 0) return false;
    }
    return true;
  }

  bool side_empty(const State& st) const {
    rep(j, R) {
      if(st.side_len[j] != 0) return false;
    }
    return true;
  }

  bool is_buildable_row(const State& st, int i) const {
    int len = st.dep_len[i];
    rep(p, len) {
      if((int)st.dep[i][p] != 10 * i + p) return false;
    }
    return true;
  }

  int buildable_rows(const State& st) const {
    int cnt = 0;
    rep(i, R) {
      if(is_buildable_row(st, i)) cnt++;
    }
    return cnt;
  }

  bool all_buildable(const State& st) const {
    rep(i, R) {
      if(!is_buildable_row(st, i)) return false;
    }
    return true;
  }

  bool is_complete(const State& st) const {
    rep(i, R) {
      if((int)st.dep_len[i] != INIT_LEN) return false;
      rep(j, INIT_LEN) {
        if((int)st.dep[i][j] != 10 * i + j) return false;
      }
    }
    rep(j, R) {
      if(st.side_len[j] != 0) return false;
    }
    return true;
  }

  void apply_op(State& st, const Operation& op) const {
    int type = op.type;
    int i = op.i;
    int j = op.j;
    int k = op.k;

    if(type == 0) {
      int dl = st.dep_len[i];
      int sl = st.side_len[j];

      for(int p = sl - 1; p >= 0; p--) {
        st.side[j][p + k] = st.side[j][p];
      }

      rep(t, k) {
        st.side[j][t] = st.dep[i][dl - k + t];
      }

      st.dep_len[i] = (unsigned char)(dl - k);
      st.side_len[j] = (unsigned char)(sl + k);
    } else {
      int dl = st.dep_len[i];
      int sl = st.side_len[j];

      rep(t, k) {
        st.dep[i][dl + t] = st.side[j][t];
      }

      for(int p = k; p < sl; p++) {
        st.side[j][p - k] = st.side[j][p];
      }

      st.dep_len[i] = (unsigned char)(dl + k);
      st.side_len[j] = (unsigned char)(sl - k);
    }
  }

  void apply_turn(State& st, const Turn& turn) const {
    int moved_ops = 0;
    int moved_cars = 0;

    rep(idx, turn.count) {
      const auto& op = turn.ops[idx];
      moved_ops++;
      moved_cars += op.k;
      apply_op(st, op);
    }

    st.turns++;
    st.last_moved_ops = moved_ops;
    st.last_moved_cars = moved_cars;
  }

  int dep_cars(const State& st) const {
    int res = 0;
    rep(i, R) res += st.dep_len[i];
    return res;
  }

  int side_cars(const State& st) const {
    int res = 0;
    rep(j, R) res += st.side_len[j];
    return res;
  }

  int correct_side_cars(const State& st) const {
    int res = 0;
    rep(j, R) {
      rep(p, st.side_len[j]) {
        int car = st.side[j][p];
        if(car % 10 == j) res++;
      }
    }
    return res;
  }

  int correct_dep_prefix_cars(const State& st) const {
    int res = 0;
    rep(i, R) {
      int len = st.dep_len[i];
      rep(p, len) {
        if((int)st.dep[i][p] == 10 * i + p) res++;
        else break;
      }
    }
    return res;
  }

  int dep_mod_blocks(const State& st) const {
    int blocks = 0;
    rep(i, R) {
      int len = st.dep_len[i];
      if(len == 0) continue;

      int prefix_len = 0;
      rep(p, len) {
        if((int)st.dep[i][p] == 10 * i + p) prefix_len++;
        else break;
      }
      if(prefix_len == len) continue;

      blocks++;
      for(int p = len - 2; p >= prefix_len; p--) {
        int a = (int)st.dep[i][p] % 10;
        int b = (int)st.dep[i][p + 1] % 10;
        if(a != b) blocks++;
      }
    }
    return blocks;
  }

  int side_row_blocks(const State& st) const {
    int blocks = 0;

    rep(j, R) {
      int len = st.side_len[j];
      if(len == 0) continue;

      blocks++;
      for(int p = 1; p < len; p++) {
        int a = (int)st.side[j][p - 1] / 10;
        int b = (int)st.side[j][p] / 10;
        if(a != b) blocks++;
      }
    }

    return blocks;
  }

  int side_row_inversions(const State& st) const {
    int inv = 0;
    rep(j, R) {
      int len = st.side_len[j];
      rep(a, len) {
        for(int b = a + 1; b < len; b++) {
          int ra = (int)st.side[j][a] / 10;
          int rb = (int)st.side[j][b] / 10;
          if(ra > rb) inv++;
        }
      }
    }
    return inv;
  }

  int side_front_block_penalty(const State& st) const {
    int penalty = 0;
    rep(j, R) {
      if(st.side_len[j] == 0) continue;
      int front_mod = st.side[j][0] % 10;
      int room = SIDE_CAP - st.side_len[j];

      penalty += (9 - front_mod) * (room + 1);
      if(front_mod == 0 && room > 0) penalty += SIDE_FRONT_ZERO_EXTRA_PENALTY;
    }
    return penalty;
  }

  int next_block_dead_penalty(const State& st) const {
    int penalty = 0;

    rep(i, R) {
      int len = st.dep_len[i];
      if(len == 0) continue;

      int prefix_len = 0;
      rep(p, len) {
        if((int)st.dep[i][p] == 10 * i + p) prefix_len++;
        else break;
      }
      if(prefix_len == len) continue;

      int k = 1;
      while(prefix_len + k < len) {
        int prev = st.dep[i][len - 1 - k] % 10;
        int cur = st.dep[i][len - k] % 10;
        if(prev > cur) break;
        k++;
      }

      int block_tail_mod = st.dep[i][len - 1] % 10;
      int acceptors = 0;

      rep(j, R) {
        if((int)st.side_len[j] + k > SIDE_CAP) continue;

        bool ok = false;
        if(st.side_len[j] == 0) ok = true;
        else {
          int front_mod = st.side[j][0] % 10;
          if(block_tail_mod <= front_mod) ok = true;
        }

        if(j == block_tail_mod) ok = true;

        if(ok) acceptors++;
      }

      if(acceptors == 0) {
        penalty += DEAD_ACCEPTOR_BASE_PENALTY + block_tail_mod * block_tail_mod * DEAD_ACCEPTOR_TAIL_WEIGHT;
      } else {
        int lack = 10 - min(10, acceptors);
        penalty += lack * lack * (block_tail_mod + 1) * (block_tail_mod + 1);
      }
    }

    return penalty;
  }

  int high_col_acceptor_penalty(const State& st) const {
    int penalty = 0;
    for(int col = 7; col < R; col++) {
      int acceptors = 0;
      rep(j, R) {
        if(st.side_len[j] >= SIDE_CAP) continue;
        if(st.side_len[j] == 0 || col <= (int)st.side[j][0] % 10 || j == col) {
          acceptors++;
        }
      }
      int lack = 10 - min(10, acceptors);
      penalty += lack * lack * (col + 1) * (col + 1);
    }
    return penalty;
  }

  int acceptor_shortage_penalty(const State& st) const {
    array<int, RMAX> rem{};
    rep(i, R) {
      int prefix_len = 0;
      rep(p, st.dep_len[i]) {
        if((int)st.dep[i][p] == 10 * i + p) prefix_len++;
        else break;
      }
      for(int p = prefix_len; p < (int)st.dep_len[i]; p++) {
        rem[st.dep[i][p] % 10]++;
      }
    }

    int penalty = 0;
    rep(col, R) {
      if(rem[col] == 0) continue;

      int acceptors = 0;
      rep(j, R) {
        if(st.side_len[j] >= SIDE_CAP) continue;
        if(col < j) continue;
        if(st.side_len[j] > 0) {
          int front_col = st.side[j][0] % 10;
          if(col > front_col) continue;
        }
        acceptors++;
      }

      int weight = (col + 1) * (col + 1);
      if(acceptors == 0) {
        penalty += ACCEPTOR_SHORTAGE_DEAD_WEIGHT * weight;
      } else {
        int scarcity = 10 - min(10, acceptors);
        penalty += rem[col] * scarcity * scarcity * weight;
      }
    }
    return penalty;
  }

  int side_balance_penalty(const State& st) const {
    int total = side_cars(st);
    int penalty = 0;
    rep(j, R) {
      int diff = (int)st.side_len[j] * R - total;
      penalty += diff * diff;
    }
    return penalty;
  }

  int side_bucket_quality(const State& st) const {
    int quality = 0;
    rep(j, R) {
      rep(p, st.side_len[j]) {
        int col = st.side[j][p] % 10;
        quality += j * j;
        quality -= (col - j) * (col - j);
      }
    }
    return quality;
  }

  bool preserves_future_acceptors_after_classify(const State& st, int i, int j, int k) const {
    array<int, RMAX> rem{};
    rep(r, R) {
      int prefix_len = 0;
      rep(p, st.dep_len[r]) {
        if((int)st.dep[r][p] == 10 * r + p) prefix_len++;
        else break;
      }
      for(int p = prefix_len; p < (int)st.dep_len[r]; p++) {
        rem[st.dep[r][p] % 10]++;
      }
    }
    int len = st.dep_len[i];
    for(int p = len - k; p < len; p++) {
      rem[st.dep[i][p] % 10]--;
    }

    rep(col, R) {
      if(rem[col] == 0) continue;

      bool exists = false;
      rep(s, R) {
        int sl = st.side_len[s];
        if(s == j) sl += k;
        if(sl >= SIDE_CAP) continue;
        if(col < s) continue;

        int front_col;
        if(s == j) {
          front_col = st.dep[i][len - k] % 10;
        } else if(st.side_len[s] > 0) {
          front_col = st.side[s][0] % 10;
        } else {
          front_col = R - 1;
        }

        if(col <= front_col) {
          exists = true;
          break;
        }
      }

      if(!exists) return false;
    }

    return true;
  }

  int restore_ready_ops(const State& st) const {
    int ready = 0;
    rep(j, R) {
      if(st.side_len[j] == 0) continue;
      int car = st.side[j][0];
      int row = car / 10;
      int col = car % 10;
      if(!is_buildable_row(st, row)) continue;
      if((int)st.dep_len[row] == col && (int)st.dep_len[row] < INIT_LEN) ready++;
    }
    return ready;
  }

  int restore_ready_cars(const State& st) const {
    int ready = 0;
    rep(j, R) {
      if(st.side_len[j] == 0) continue;
      int car = st.side[j][0];
      int row = car / 10;
      int col = car % 10;
      if(!is_buildable_row(st, row)) continue;
      if((int)st.dep_len[row] != col || (int)st.dep_len[row] >= INIT_LEN) continue;
      int k = 1;
      while(k < (int)st.side_len[j]) {
        int nxt = st.side[j][k];
        if(nxt / 10 != row || nxt % 10 != col + k) break;
        if((int)st.dep_len[row] + k >= DEP_CAP) break;
        k++;
      }
      ready += k;
    }
    return ready;
  }

  long long evaluate(const State& st) const {
    int ok_side = side_cars(st);
    int remaining_blocks = dep_mod_blocks(st);
    int shortage_penalty = acceptor_shortage_penalty(st);
    int balance_penalty = side_balance_penalty(st);
    int bucket_quality = side_bucket_quality(st);
    int correct_prefix = correct_dep_prefix_cars(st);
    int b_rows = buildable_rows(st);
    int ready = restore_ready_ops(st);

    long long score = 0;
    score += EVAL_CORRECT_PREFIX_WEIGHT * correct_prefix;
    score += EVAL_BUILDABLE_ROWS_WEIGHT * b_rows;
    score += EVAL_SIDE_CARS_WEIGHT * ok_side;
    score -= EVAL_REMAINING_BLOCKS_WEIGHT * remaining_blocks;
    score -= EVAL_ACCEPTOR_SHORTAGE_WEIGHT * shortage_penalty;
    score -= EVAL_SIDE_BALANCE_WEIGHT * balance_penalty;
    score += EVAL_BUCKET_QUALITY_WEIGHT * bucket_quality;
    score += EVAL_READY_OPS_WEIGHT * ready;
    score += EVAL_LAST_MOVED_OPS_WEIGHT * st.last_moved_ops;
    score += EVAL_LAST_MOVED_CARS_WEIGHT * st.last_moved_cars;
    score -= EVAL_TURNS_WEIGHT * st.turns;
    return score;
  }

  uint64_t hash_state(const State& st) const {
    uint64_t h = 1469598103934665603ULL;

    auto add = [&](uint64_t x) {
      h ^= x;
      h *= 1099511628211ULL;
    };

    rep(i, R) {
      add(st.dep_len[i] + 1);
      rep(p, st.dep_len[i]) add((int)st.dep[i][p] + 7);
    }

    rep(j, R) {
      add(st.side_len[j] + 101);
      rep(p, st.side_len[j]) add((int)st.side[j][p] + 1009);
    }

    return h;
  }

  vector<MoveCandidate> generate_classify_moves(const State& st) const {
    vector<MoveCandidate> moves;

    rep(i, R) {
      int len = st.dep_len[i];
      if(len == 0) continue;

      int prefix_len = 0;
      rep(p, len) {
        if((int)st.dep[i][p] == 10 * i + p) prefix_len++;
        else break;
      }
      if(prefix_len == len) continue;

      int max_k = 1;
      while(prefix_len + max_k < len) {
        int prev = st.dep[i][len - 1 - max_k] % 10;
        int cur = st.dep[i][len - max_k] % 10;
        if(prev > cur) break;
        max_k++;
      }

      for(int k = 1; k <= max_k; k++) {
        int block_front_col = st.dep[i][len - k] % 10;
        int block_tail_col = st.dep[i][len - 1] % 10;

        rep(j, R) {
          int sl = st.side_len[j];
          if(sl + k > SIDE_CAP) continue;

          if(block_front_col < j) continue;

          if(sl > 0) {
            int front_col = st.side[j][0] % 10;
            if(block_tail_col > front_col) continue;
          }

          if(!preserves_future_acceptors_after_classify(st, i, j, k)) continue;

          moves.push_back(MoveCandidate{0, i, j, k});
        }
      }
    }

    sort(moves.begin(), moves.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
      if(a.i != b.i) return a.i < b.i;
      return a.j < b.j;
    });

    return moves;
  }

  vector<MoveCandidate> generate_restore_moves(const State& st) const {
    vector<MoveCandidate> moves;

    rep(j, R) {
      if(st.side_len[j] == 0) continue;

      int car = st.side[j][0];
      int row = car / 10;
      int col = car % 10;

      if(!is_buildable_row(st, row)) continue;
      if((int)st.dep_len[row] != col || (int)st.dep_len[row] >= INIT_LEN) continue;

      int k = 1;
      while(k < (int)st.side_len[j]) {
        int nxt = st.side[j][k];
        if(nxt / 10 != row || nxt % 10 != col + k) break;
        if((int)st.dep_len[row] + k >= DEP_CAP) break;
        k++;
      }

      moves.push_back(MoveCandidate{1, row, j, k});
    }

    sort(moves.begin(), moves.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
      if(a.i != b.i) return a.i < b.i;
      return a.j < b.j;
    });

    return moves;
  }

  long long local_turn_score(const TurnCandidate& tc) const {
    long long score = 0;
    score += LOCAL_MOVED_OPS_WEIGHT * tc.moved_ops;
    score += LOCAL_MOVED_CARS_WEIGHT * tc.moved_cars;
    score += LOCAL_RESTORE_OPS_BONUS * tc.restore_ops;
    score += LOCAL_RESTORE_CARS_BONUS * tc.restore_cars;
    score -= LOCAL_CROSSING_PRESSURE_WEIGHT * tc.crossing_pressure;
    score += LOCAL_BUCKET_FIT_WEIGHT * tc.bucket_fit;
    return score;
  }

  vector<TurnCandidate> enumerate_turns(const vector<MoveCandidate>& moves) const {
    auto better = [](const TurnCandidate& a, const TurnCandidate& b) {
      if(a.local_score != b.local_score) return a.local_score > b.local_score;
      if(a.moved_ops != b.moved_ops) return a.moved_ops > b.moved_ops;
      return a.moved_cars > b.moved_cars;
    };

    vector<TurnCandidate> partial(1);
    for(const auto& mv: moves) {
      vector<TurnCandidate> next = partial;

      for(const auto& base: partial) {
        if(base.turn.count > 0) {
          const auto& last = base.turn.ops[base.turn.count - 1];
          if(mv.i <= last.i || mv.j <= last.j) continue;
        }

        TurnCandidate tc = base;
        Operation op{mv.type, mv.i, mv.j, mv.k};
        tc.turn.ops[tc.turn.count++] = op;
        tc.moved_ops++;
        tc.moved_cars += mv.k;
        if(mv.type == 1) {
          tc.restore_ops++;
          tc.restore_cars += mv.k;
        }
        tc.crossing_pressure += abs(mv.i - mv.j);
        if(mv.type == 0) tc.bucket_fit += mv.j * mv.j * mv.k;
        tc.local_score = local_turn_score(tc);
        next.push_back(tc);
      }

      sort(next.begin(), next.end(), better);
      if((int)next.size() > TOP_TRANSITIONS + 1) next.resize(TOP_TRANSITIONS + 1);
      partial.swap(next);
    }

    vector<TurnCandidate> result;
    result.reserve(TOP_TRANSITIONS);
    for(const auto& tc: partial) {
      if(tc.turn.count == 0) continue;
      result.push_back(tc);
      if((int)result.size() >= TOP_TRANSITIONS) break;
    }
    return result;
  }

  vector<vector<Operation>> reconstruct(int trace_id) const {
    vector<Turn> rev;

    int cur = trace_id;
    while(cur != -1) {
      rev.push_back(traces[cur].turn);
      cur = traces[cur].parent;
    }

    reverse(rev.begin(), rev.end());

    vector<vector<Operation>> turns;
    turns.reserve(rev.size());
    for(const auto& turn: rev) {
      vector<Operation> ops;
      ops.reserve(turn.count);
      rep(i, turn.count) ops.push_back(turn.ops[i]);
      turns.push_back(std::move(ops));
    }
    return turns;
  }

  vector<vector<Operation>> turn_suffix_to_output(const vector<Turn>& suffix) const {
    vector<vector<Operation>> turns;
    turns.reserve(suffix.size());
    for(const auto& turn: suffix) {
      vector<Operation> ops;
      ops.reserve(turn.count);
      rep(i, turn.count) ops.push_back(turn.ops[i]);
      turns.push_back(std::move(ops));
    }
    return turns;
  }

  bool finish_restore_greedy(State st, vector<Turn>& suffix) const {
    suffix.clear();
    rep(_, 1000) {
      if(is_complete(st)) return true;

      vector<MoveCandidate> moves = generate_restore_moves(st);
      if(moves.empty()) return false;

      vector<TurnCandidate> turns = enumerate_turns(moves);
      if(turns.empty()) return false;

      Turn turn = turns[0].turn;
      apply_turn(st, turn);
      suffix.push_back(turn);
    }
    return is_complete(st);
  }

  vector<vector<Operation>> greedy_fallback() const {
    vector<vector<int>> dep(R);
    vector<deque<int>> side(R);

    rep(i, R) {
      dep[i].resize(INIT_LEN);
      rep(j, INIT_LEN) dep[i][j] = initial.dep[i][j];
    }

    vector<vector<Operation>> turns;

    auto add_one = [&](int type, int i, int j, int k) {
      turns.push_back(vector<Operation>{Operation{type, i, j, k}});

      if(type == 0) {
        vector<int> moved;
        rep(_, k) {
          moved.push_back(dep[i].back());
          dep[i].pop_back();
        }
        reverse(moved.begin(), moved.end());
        for(int x = (int)moved.size() - 1; x >= 0; x--) {
          side[j].push_front(moved[x]);
        }
      } else {
        vector<int> moved;
        rep(_, k) {
          moved.push_back(side[j].front());
          side[j].pop_front();
        }
        for(int x: moved) dep[i].push_back(x);
      }
    };

    rep(i, R) {
      while(!dep[i].empty()) {
        int car = dep[i].back();
        int pos = car % 10;
        int k = 1;
        while(k < (int)dep[i].size() && dep[i][(int)dep[i].size() - 1 - k] % 10 == pos) {
          k++;
        }
        add_one(0, i, pos, k);
      }
    }

    rep(pos, R) {
      while(!side[pos].empty()) {
        int car = side[pos].front();
        add_one(1, car / 10, pos, 1);
      }
    }

    return turns;
  }

  vector<vector<Operation>> solve_beam() {
    traces.clear();

    vector<State> beam;
    beam.push_back(initial);
    State best_state = initial;
    long long best_score = evaluate(initial);

    while(true) {
      if(utility::mytm.elapsed() > TIME_LIMIT_MS) break;

      for(const auto& st: beam) {
        if(is_complete(st)) {
          return reconstruct(st.trace_id);
        }

        long long sc = evaluate(st);
        if(sc > best_score) {
          best_score = sc;
          best_state = st;
        }

        if(all_buildable(st) && side_cars(st) > 0) {
          vector<Turn> suffix;
          if(finish_restore_greedy(st, suffix)) {
            vector<vector<Operation>> result = reconstruct(st.trace_id);
            vector<vector<Operation>> tail = turn_suffix_to_output(suffix);
            result.insert(result.end(), tail.begin(), tail.end());
            return result;
          }
        }
      }

      cerr << "Depth = " << beam[0].turns
           << " buildable=" << buildable_rows(beam[0])
           << " prefix=" << correct_dep_prefix_cars(beam[0])
           << " side=" << side_cars(beam[0])
           << '\n';

      vector<BeamItem> cand;
      cand.reserve((size_t)beam.size() * TOP_TRANSITIONS);

      for(const auto& st: beam) {
        if(utility::mytm.elapsed() > TIME_LIMIT_MS) break;

        vector<MoveCandidate> moves = generate_classify_moves(st);
        {
          auto restore = generate_restore_moves(st);
          moves.insert(moves.end(), restore.begin(), restore.end());
        }

        if(moves.empty()) continue;

        sort(moves.begin(), moves.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
          if(a.i != b.i) return a.i < b.i;
          return a.j < b.j;
        });

        vector<TurnCandidate> turns = enumerate_turns(moves);

        for(const auto& tc: turns) {
          State ns = st;
          apply_turn(ns, tc.turn);

          long long sc = evaluate(ns);
          sc += tc.local_score;

          BeamItem item;
          item.score = sc;
          item.st = ns;
          item.parent_trace = st.trace_id;
          item.turn = tc.turn;
          cand.push_back(std::move(item));
        }
      }

      if(cand.empty()) break;

      int keep = min((int)cand.size(), max(BEAM_WIDTH * 8, BEAM_WIDTH + 100));
      if((int)cand.size() > keep) {
        nth_element(cand.begin(), cand.begin() + keep, cand.end());
        cand.resize(keep);
      }
      sort(cand.begin(), cand.end());

      vector<State> next_beam;
      next_beam.reserve(BEAM_WIDTH);

      unordered_set<uint64_t> used;
      used.reserve(BEAM_WIDTH * 4);

      for(auto& item: cand) {
        if((int)next_beam.size() >= BEAM_WIDTH) break;

        uint64_t h = hash_state(item.st);
        if(used.find(h) != used.end()) continue;
        used.insert(h);

        int tid = (int)traces.size();
        traces.push_back(Trace{item.parent_trace, item.turn});
        item.st.trace_id = tid;

        if(is_complete(item.st)) {
          return reconstruct(tid);
        }

        next_beam.push_back(item.st);
      }

      if(next_beam.empty()) break;
      beam.swap(next_beam);
    }

    cerr << "Fallback: best score = " << best_score
         << ", turns = " << best_state.turns
         << ", dep_cars = " << dep_cars(best_state)
         << ", side_cars = " << side_cars(best_state)
         << ", buildable = " << buildable_rows(best_state)
         << ", prefix = " << correct_dep_prefix_cars(best_state)
         << '\n';

    return greedy_fallback();
  }

  void output(const vector<vector<Operation>>& turns) const {
    cerr << "Turns = " << turns.size() << '\n';
    cerr << "Score = " << 100 * R + 4000 - (int)turns.size() << '\n';

    cout << turns.size() << '\n';
    for(const auto& turn: turns) {
      cout << turn.size() << '\n';
      for(const auto& op: turn) {
        cout << op.type << ' ' << op.i << ' ' << op.j << ' ' << op.k << '\n';
      }
    }
  }

  void solve() {
    utility::mytm.CodeStart();

    vector<vector<Operation>> turns = solve_beam();

    assert((int)turns.size() <= 4000);

    output(turns);
  }
};

int main() {
  cin.tie(nullptr);
  ios::sync_with_stdio(false);

  Solver solver;
  solver.solve();

  return 0;
}
