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

    long long score = 0;

    // 分類フェーズでは、復元の点は一切入れない。
    // 「mod 10 に対応する正しい待避線へ入った車両数」だけを主目的にする。
    score += 1'000'000LL * ok_side;

    // 同じ ok_side なら、残り車両数と残りブロック数が少ない方を優先。
    score -= 100'000LL * remaining_cars;
    score -= 10'000LL * remaining_blocks;

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

      int tail = st.dep[i][len - 1];
      int j = tail % 10;

      int k = 1;
      while(k < len) {
        int car = st.dep[i][len - 1 - k];
        if(car % 10 != j) break;
        k++;
      }

      if((int)st.side_len[j] + k <= SIDE_CAP) {
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
    
      // j 列の車両を戻すには、dep[i] がちょうど j 個積まれている必要がある。
      // つまり次に置くべき列が j。
      if((int)st.dep_len[i] == j && (int)st.dep_len[i] < INIT_LEN) {
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
    vector<TurnCandidate> result;
    int n = (int)moves.size();
    result.reserve(min((1 << n) - 1, TOP_TRANSITIONS * 2));

    for(int mask = 1; mask < (1 << n); mask++) {
      int last_j = -1;
      TurnCandidate tc;
      bool ok = true;

      rep(idx, n) {
        if(((mask >> idx) & 1) == 0) continue;

        const auto& mv = moves[idx];
        if(mv.j <= last_j) {
          ok = false;
          break;
        }
        last_j = mv.j;

        Operation op{mv.type, mv.i, mv.j, mv.k};
        tc.turn.ops[tc.turn.count++] = op;
        tc.moved_ops++;
        tc.moved_cars += mv.k;
        tc.crossing_pressure += abs(mv.i - mv.j);
      }

      if(!ok) continue;

      tc.local_score = local_turn_score(tc.moved_ops, tc.moved_cars, tc.crossing_pressure, restore_phase);
      result.push_back(tc);
    }

    sort(result.begin(), result.end(), [](const TurnCandidate& a, const TurnCandidate& b) {
      if(a.local_score != b.local_score) return a.local_score > b.local_score;
      if(a.moved_ops != b.moved_ops) return a.moved_ops > b.moved_ops;
      return a.moved_cars > b.moved_cars;
    });

    if((int)result.size() > TOP_TRANSITIONS) {
      result.resize(TOP_TRANSITIONS);
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

    while(true) {
      if(utility::mytm.elapsed() > TIME_LIMIT_MS) break;

      for(const auto& st: beam) {
        if(is_complete(st)) {
          return reconstruct(st.trace_id);
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
