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

static constexpr int TIME_LIMIT_MS = 190000;

static constexpr int BEAM_WIDTH_CLASSIFY = 2000;
static constexpr int BEAM_WIDTH_RESTORE  = 2000;
static constexpr int TOP_TRANSITIONS = 100;

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
  bool restore_phase = false;
};

struct BeamItem {
  long long score;
  State st;
  int parent_trace;
  Turn turn;

  bool operator<(const BeamItem& other) const {
    return score > other.score; // score 大きい方が良い
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
      // dep[i] の末尾 k 両を順序を保って side[j] の先頭へ入れる。
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
      // side[j] の先頭 k 両を順序を保って dep[i] の末尾へ戻す。
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

      blocks++;
      for(int p = len - 2; p >= 0; p--) {
        int a = (int)st.dep[i][p] % 10;
        int b = (int)st.dep[i][p + 1] % 10;
        if(a != b) blocks++;
      }
    }
    return blocks;
  }

  int side_row_blocks(const State& st) const {
    // 待避線 j の front -> tail に見た row = car / 10 のブロック数。
    // 復元時には side の front からしか出せないため、row ブロックが少ない方が戻しやすい。
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
    // 復元しやすさの軽い proxy。
    // 小さい row から順に出る必要があるわけではないが、
    // ぐちゃぐちゃ度を見るために入れる。
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

      // The next prepended block must end with mod <= front_mod. A small
      // front_mod, especially 0, almost closes this siding for future blocks.
      penalty += (9 - front_mod) * (room + 1);
      if(front_mod == 0 && room > 0) penalty += 200;
    }
    return penalty;
  }

  int next_block_dead_penalty(const State& st) const {
    int penalty = 0;

    rep(i, R) {
      int len = st.dep_len[i];
      if(len == 0) continue;

      int k = 1;
      while(k < len) {
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

        // Canonical escape: keep the original mod bucket route alive.
        if(j == block_tail_mod) ok = true;

        if(ok) acceptors++;
      }

      if(acceptors == 0) {
        penalty += 100000 + block_tail_mod * block_tail_mod * 1000;
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
      rep(p, st.dep_len[i]) {
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
        penalty += 100000 * weight;
      } else {
        int scarcity = 10 - min(10, acceptors);
        penalty += rem[col] * scarcity * scarcity * weight;
      }
    }
    return penalty;
  }

  bool preserves_future_acceptors_after_classify(const State& st, int i, int j, int k) const {
    array<int, RMAX> rem{};
    rep(r, R) {
      rep(p, st.dep_len[r]) {
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
    // 現在すぐ正しい prefix として戻せる type=1 操作数。
    int ready = 0;

    rep(i, R) {
      int need_col = st.dep_len[i];
      if(need_col >= INIT_LEN) continue;

      int j = need_col;
      if(st.side_len[j] == 0) continue;

      int car = st.side[j][0];
      if(car == 10 * i + need_col) ready++;
    }

    return ready;
  }

  int restore_ready_cars(const State& st) const {
    // 今すぐ戻せる車両数。
    // 現状、正しい prefix 復元なので基本 k=1 だが、将来拡張用。
    int ready = 0;

    rep(i, R) {
      int need_col = st.dep_len[i];
      if(need_col >= INIT_LEN) continue;

      int j = need_col;
      if(st.side_len[j] == 0) continue;

      int car = st.side[j][0];
      if(car == 10 * i + need_col) ready++;
    }

    return ready;
  }

  long long evaluate_classify(const State& st) const {
    int ok_side = side_cars(st);
    int remaining_blocks = dep_mod_blocks(st);
    int remaining_cars = dep_cars(st);
    int shortage_penalty = acceptor_shortage_penalty(st);

    long long score = 0;

    // 分類フェーズでは、復元の点は一切入れない。
    // 「mod 10 に対応する正しい待避線へ入った車両数」だけを主目的にする。
    score += 1'000'000LL * ok_side;

    // 同じ ok_side なら、残り車両数と残りブロック数が少ない方を優先。
    score -= 100'000LL * remaining_cars;
    score -= 10'000LL * remaining_blocks;
    score -= 20'000LL * shortage_penalty;

    // 直前ターンで複数操作している状態をかなり優遇。
    // これを入れないと同じ進捗の単発手が残りやすい。
    score += 2'000'000LL * st.last_moved_ops;
    score += 100'000LL * st.last_moved_cars;

    // ターン数は少しだけ罰する。
    // 同じ深さの beam 内ではほぼ効かないが、念のため。
    score -= 1'000LL * st.turns;

    return score;
  }

  long long evaluate_restore(const State& st) const {
    int correct_prefix = correct_dep_prefix_cars(st);
    int remaining_side = side_cars(st);
    int ready = restore_ready_ops(st);

    long long score = 0;

    score += 1'000'000'000'000LL;

    // 復元フェーズだけで加算する。分類側の 1000 倍程度の重み。
    score += 1'000'000'000LL * correct_prefix;

    // 残り side 車両が少ないほど良い。
    score -= 20'000'000LL * remaining_side;

    // すぐ戻せる候補が多いほど良い。
    score += 5'000'000LL * ready;

    // 直前ターンの複数操作を強く評価。
    score += 30'000'000LL * st.last_moved_ops;
    score += 1'000'000LL * st.last_moved_cars;

    score -= 10'000LL * st.turns;

    return score;
  }

  long long evaluate(const State& st) const {
    if(st.restore_phase) return evaluate_restore(st);
    return evaluate_classify(st);
  }

  uint64_t hash_state(const State& st) const {
    uint64_t h = 1469598103934665603ULL;

    auto add = [&](uint64_t x) {
      h ^= x;
      h *= 1099511628211ULL;
    };

    add(st.restore_phase ? 1234567 : 7654321);

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

      int col = st.dep[i][len - 1] % 10;

      int k = 1;
      while(k < len) {
        int car = st.dep[i][len - 1 - k];
        if(car % 10 != col) break;
        k++;
      }

      rep(j, R) {
        int sl = st.side_len[j];
        if(sl + k > SIDE_CAP) continue;

        // Hybrid bucket rule: side[j] accepts only mod >= j.
        if(col < j) continue;

        if(sl > 0) {
          int front_col = st.side[j][0] % 10;
          if(col > front_col) continue;
        }

        if(!preserves_future_acceptors_after_classify(st, i, j, k)) continue;

        moves.push_back(MoveCandidate{0, i, j, k});
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
      int i = car / 10;
      int col = car % 10;
    
      // The siding index is only a bucket id in the hybrid policy.
      // Restore readiness is determined by the front car's actual column.
      if((int)st.dep_len[i] == col && (int)st.dep_len[i] < INIT_LEN) {
        moves.push_back(MoveCandidate{1, i, j, 1});
      }
    }
  
    sort(moves.begin(), moves.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
      if(a.i != b.i) return a.i < b.i;
      return a.j < b.j;
    });
  
    return moves;
  }

  long long local_turn_score(int moved_ops, int moved_cars, int crossing_pressure, bool restore_phase) const {
    long long score = 0;

    // 1ターン複数操作を強く評価。
    // ここをかなり強くしないと、単発巨大 k が残りすぎる。
    if(!restore_phase) {
      score += 10'000'000LL * moved_ops;
      score += 500'000LL * moved_cars;
    } else {
      // 復元フェーズでは 1 両ずつになりやすいので、ops 数がほぼそのままターン短縮。
      score += 100'000'000LL * moved_ops;
      score += 1'000'000LL * moved_cars;
    }

    // 経路が遠いものは他候補を潰しやすいので軽く減点。
    score -= 10'000LL * crossing_pressure;

    return score;
  }

  vector<TurnCandidate> enumerate_turns(const vector<MoveCandidate>& moves, bool restore_phase) const {
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
        tc.crossing_pressure += abs(mv.i - mv.j);
        tc.bucket_fit += mv.j * mv.j * mv.k;
        tc.local_score = local_turn_score(tc.moved_ops, tc.moved_cars, tc.crossing_pressure, restore_phase);
        if(!restore_phase) tc.local_score += 20'000LL * tc.bucket_fit;
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

      vector<TurnCandidate> turns = enumerate_turns(moves, true);
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

    // 元 Greedy と同じ復元。
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
    State best_classify_state = initial;
    long long best_classify_score = evaluate_classify(initial);
    State best_progress_state = initial;
    int best_progress_side_cars = 0;
    long long best_progress_score = best_classify_score;
    State best_restore_state = initial;
    int best_restore_side_cars = TOTAL + 1;
    long long best_restore_score = -(1LL << 60);

    while(true) {
      if(utility::mytm.elapsed() > TIME_LIMIT_MS) break;

      for(const auto& st: beam) {
        if(is_complete(st)) {
          return reconstruct(st.trace_id);
        }
        if(st.restore_phase) {
          long long sc = evaluate_restore(st);
          int sc_side_cars = side_cars(st);
          if(sc_side_cars < best_restore_side_cars ||
             (sc_side_cars == best_restore_side_cars && sc > best_restore_score)) {
            best_restore_side_cars = sc_side_cars;
            best_restore_score = sc;
            best_restore_state = st;
          }
          vector<Turn> suffix;
          if(finish_restore_greedy(st, suffix)) {
            vector<vector<Operation>> result = reconstruct(st.trace_id);
            vector<vector<Operation>> tail = turn_suffix_to_output(suffix);
            result.insert(result.end(), tail.begin(), tail.end());
            return result;
          }
        }
        if(!st.restore_phase) {
          long long sc = evaluate_classify(st);
          if(sc > best_classify_score) {
            best_classify_score = sc;
            best_classify_state = st;
          }
          int sc_side_cars = side_cars(st);
          if(sc_side_cars > best_progress_side_cars ||
             (sc_side_cars == best_progress_side_cars && sc > best_progress_score)) {
            best_progress_side_cars = sc_side_cars;
            best_progress_score = sc;
            best_progress_state = st;
          }
        }
      }

      bool restore_phase = false;
      bool all_restore_phase = true;
      for(const auto& st: beam) {
        if(!st.restore_phase) {
          all_restore_phase = false;
          break;
        }
      }
      restore_phase = all_restore_phase;

      cerr << "Depth = " << beam[0].turns << (restore_phase ? " (restore)" : " (classify)") << '\n';

      vector<BeamItem> cand;
      cand.reserve((size_t)beam.size() * TOP_TRANSITIONS);

      for(const auto& st: beam) {
        if(utility::mytm.elapsed() > TIME_LIMIT_MS) break;

        vector<MoveCandidate> moves;
        bool st_restore = st.restore_phase;

        if(st_restore) {
          moves = generate_restore_moves(st);
        } else {
          moves = generate_classify_moves(st);
        }

        if(moves.empty()) {
          continue;
        }

        vector<TurnCandidate> turns = enumerate_turns(moves, st_restore);

        for(const auto& tc: turns) {
          State ns = st;
          apply_turn(ns, tc.turn);
          if(!ns.restore_phase && dep_empty(ns)) {
            ns.restore_phase = true;
          }

          long long sc = evaluate(ns);

          // local turn score も足す。
          // これにより、同じような状態評価なら複数同時移動が残る。
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

      int beam_width = restore_phase ? BEAM_WIDTH_RESTORE : BEAM_WIDTH_CLASSIFY;

      int keep = min((int)cand.size(), max(beam_width * 8, beam_width + 100));
      if((int)cand.size() > keep) {
        nth_element(cand.begin(), cand.begin() + keep, cand.end());
        cand.resize(keep);
      }
      sort(cand.begin(), cand.end());

      vector<State> next_beam;
      next_beam.reserve(beam_width);

      unordered_set<uint64_t> used;
      used.reserve(beam_width * 4);

      for(auto& item: cand) {
        if((int)next_beam.size() >= beam_width) break;

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

    cerr << "Fallback: best classify score = " << best_classify_score
         << ", turns = " << best_classify_state.turns
         << ", dep_cars = " << dep_cars(best_classify_state)
         << ", side_cars = " << side_cars(best_classify_state)
         << '\n';
    rep(j, R) {
      cerr << "side[" << j << "] len=" << (int)best_classify_state.side_len[j] << " mods:";
      rep(p, best_classify_state.side_len[j]) {
        cerr << ' ' << ((int)best_classify_state.side[j][p] % 10);
      }
      cerr << " cars:";
      rep(p, best_classify_state.side_len[j]) {
        cerr << ' ' << (int)best_classify_state.side[j][p];
      }
      cerr << '\n';
    }
    rep(i, R) {
      cerr << "dep[" << i << "] len=" << (int)best_classify_state.dep_len[i] << " mods:";
      rep(p, best_classify_state.dep_len[i]) {
        cerr << ' ' << ((int)best_classify_state.dep[i][p] % 10);
      }
      cerr << " cars:";
      rep(p, best_classify_state.dep_len[i]) {
        cerr << ' ' << (int)best_classify_state.dep[i][p];
      }
      cerr << '\n';
    }

    cerr << "Fallback: best progress score = " << best_progress_score
         << ", turns = " << best_progress_state.turns
         << ", dep_cars = " << dep_cars(best_progress_state)
         << ", side_cars = " << side_cars(best_progress_state)
         << '\n';
    rep(j, R) {
      cerr << "progress side[" << j << "] len=" << (int)best_progress_state.side_len[j] << " mods:";
      rep(p, best_progress_state.side_len[j]) {
        cerr << ' ' << ((int)best_progress_state.side[j][p] % 10);
      }
      cerr << " cars:";
      rep(p, best_progress_state.side_len[j]) {
        cerr << ' ' << (int)best_progress_state.side[j][p];
      }
      cerr << '\n';
    }
    rep(i, R) {
      cerr << "progress dep[" << i << "] len=" << (int)best_progress_state.dep_len[i] << " mods:";
      rep(p, best_progress_state.dep_len[i]) {
        cerr << ' ' << ((int)best_progress_state.dep[i][p] % 10);
      }
      cerr << " cars:";
      rep(p, best_progress_state.dep_len[i]) {
        cerr << ' ' << (int)best_progress_state.dep[i][p];
      }
      cerr << '\n';
    }

    if(best_restore_side_cars <= TOTAL) {
      cerr << "Fallback: best restore score = " << best_restore_score
           << ", turns = " << best_restore_state.turns
           << ", dep_cars = " << dep_cars(best_restore_state)
           << ", side_cars = " << side_cars(best_restore_state)
           << '\n';
      rep(j, R) {
        cerr << "restore side[" << j << "] len=" << (int)best_restore_state.side_len[j] << " mods:";
        rep(p, best_restore_state.side_len[j]) {
          int car = best_restore_state.side[j][p];
          cerr << ' ' << (car % 10) << '(' << (car / 10) << ')';
        }
        cerr << " cars:";
        rep(p, best_restore_state.side_len[j]) {
          cerr << ' ' << (int)best_restore_state.side[j][p];
        }
        cerr << '\n';
      }
      rep(i, R) {
        cerr << "restore dep[" << i << "] len=" << (int)best_restore_state.dep_len[i] << " mods:";
        rep(p, best_restore_state.dep_len[i]) {
          cerr << ' ' << ((int)best_restore_state.dep[i][p] % 10);
        }
        cerr << " cars:";
        rep(p, best_restore_state.dep_len[i]) {
          cerr << ' ' << (int)best_restore_state.dep[i][p];
        }
        cerr << '\n';
      }
      vector<MoveCandidate> ready = generate_restore_moves(best_restore_state);
      cerr << "restore ready moves:";
      for(const auto& mv: ready) {
        cerr << " (i=" << mv.i << ",j=" << mv.j << ",k=" << mv.k << ")";
      }
      cerr << '\n';
    }

    // 時間切れ・詰まり対策。
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
