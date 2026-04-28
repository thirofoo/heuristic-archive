#include <bits/stdc++.h>
using namespace std;

static constexpr int R = 10;
static constexpr int DEP_CAP = 15;
static constexpr int SIDE_CAP = 20;
static constexpr int N_CAR = 100;
static constexpr int MAX_TURN = 4000;

static constexpr int GREEDY_TURN_LIMIT = 60;
static constexpr int GREEDY_ANS_LIMIT = 100;
static constexpr int STAGNATION_LIMIT = 5;
static constexpr int FINAL_GUARD_LIMIT = 100;
static constexpr int SEEN_RESERVE = 10000;

static constexpr int BEAM_WIDTH = 300;
static constexpr int BEAM_BRANCH = 20;
static constexpr int BEAM_CAND_LIMIT = 300;
static constexpr int BEAM_ROOT_LIMIT = 50;
static constexpr long long BEAM_BLOCK_W = 10000000LL;
static constexpr long long BEAM_ENDPOINT_W = 0LL;
static constexpr long long BEAM_SURFACE_W = 40000LL;
static constexpr long long BEAM_TURN_W = 300LL;

static constexpr long long MERGE_DELTA_GOOD_W = 1000000LL;
static constexpr long long MERGE_CUT_SURFACE_W = 2000LL;

static constexpr long long EXPOSE_ENDPOINT_DIFF_W = 0LL;
static constexpr long long EXPOSE_CUT_SURFACE_W = 0LL;
static constexpr long long EXPOSE_INSIDE_W = 500LL;
static constexpr long long EXPOSE_K_W = 0LL;
static constexpr long long EXPOSE_DIST_W = 4LL;
static constexpr long long EXPOSE_EMPTY_DST_BONUS = 50000000LL;

static constexpr long long BALANCE_FREE_AFTER_W = 18LL;
static constexpr long long BALANCE_NEAR_FULL_PENALTY_W = 0LL;
static constexpr long long BALANCE_DST_AFTER_SQ_PENALTY_W = 2LL;
static constexpr int BALANCE_MIN_FREE = 0;

static constexpr long long FINAL_DIRECT_BASE_W = 0LL;
static constexpr long long FINAL_DIRECT_K_W = 100LL;
static constexpr long long FINAL_DIRECT_DIST_W = 0LL;

static constexpr int FINAL_BLOCKED_BASE = 0;
static constexpr int FINAL_BLOCKED_K_W = 0;
static constexpr int FINAL_BLOCKED_PENALTY_P = 0;
static constexpr long long FINAL_BLOCKED_BATCH_W = 0LL;

static constexpr int FINAL_FALLBACK_K_W = 0;
static constexpr int FINAL_FALLBACK_PENALTY_P = 0;
static constexpr int FINAL_FALLBACK_DIRECT_BONUS = 0;

static constexpr int UNLOAD_K_W = 0;
static constexpr int UNLOAD_FREE_W = 0;
static constexpr int UNLOAD_DIST_W = 0;

static constexpr uint64_t ZOBRIST_SEED = 1234567891234567ULL;

struct Move { int type, i, j, k; };
struct Cand { Move mv; long long score; };

struct Board {
  uint8_t dsz[R], ssz[R];
  uint8_t dep[R][DEP_CAP];
  uint8_t side[R][SIDE_CAP];
};

struct Turn {
  int n = 0;
  Move mv[R];
};

struct Solver {
  Board bd;
  vector<vector<Move>> ans;
  uint64_t zDep[R][DEP_CAP][N_CAR], zSide[R][SIDE_CAP][N_CAR];
  unordered_set<uint64_t> seen;
  vector<Move> last_turn;

  struct BeamNode {
    Board b;
    int parent;
    int depth;
    int blocks;
    long long eval;
    uint64_t h;
    uint8_t last_n;
    Move last[R];
    uint8_t ops_n;
    Move ops[R];
  };

  Solver(const vector<vector<int>>& y) {
    memset(&bd, 0, sizeof(bd));
    for (int i = 0; i < R; i++) {
      bd.dsz[i] = 10;
      for (int j = 0; j < 10; j++) bd.dep[i][j] = (uint8_t)y[i][j];
    }
    init_zobrist();
    seen.reserve(SEEN_RESERVE);
    seen.insert(current_hash());
  }

  static uint64_t splitmix64(uint64_t& x) {
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  }

  void init_zobrist() {
    uint64_t seed = ZOBRIST_SEED;
    for (int i = 0; i < R; i++)
      for (int p = 0; p < DEP_CAP; p++)
        for (int x = 0; x < N_CAR; x++)
          zDep[i][p][x] = splitmix64(seed);
    for (int j = 0; j < R; j++)
      for (int p = 0; p < SIDE_CAP; p++)
        for (int x = 0; x < N_CAR; x++)
          zSide[j][p][x] = splitmix64(seed);
  }

  uint64_t hash_of(const Board& b) const {
    uint64_t h = 0;
    for (int i = 0; i < R; i++)
      for (int p = 0; p < b.dsz[i]; p++)
        h ^= zDep[i][p][b.dep[i][p]];
    for (int j = 0; j < R; j++)
      for (int p = 0; p < b.ssz[j]; p++)
        h ^= zSide[j][p][b.side[j][p]];
    return h;
  }

  uint64_t current_hash() const { return hash_of(bd); }
  bool good_adj(int a, int b) const { return a / 10 == b / 10 && b == a + 1; }

  bool forbidden_inverse(const Move& m) const {
    for (auto& p : last_turn)
      if (p.i == m.i && p.j == m.j && p.type != m.type) return true;
    return false;
  }

  bool forbidden_inverse_arr(const Move& m, const Move* last, int n) const {
    for (int t = 0; t < n; t++)
      if (last[t].i == m.i && last[t].j == m.j && last[t].type != m.type) return true;
    return false;
  }

  int block_count_of(const Board& b) const {
    int good = 0;
    for (int i = 0; i < R; i++) {
      for (int p = 0; p + 1 < b.dsz[i]; p++)
        if (good_adj(b.dep[i][p], b.dep[i][p + 1])) good++;
      for (int p = 0; p + 1 < b.ssz[i]; p++)
        if (good_adj(b.side[i][p], b.side[i][p + 1])) good++;
    }
    return N_CAR - good;
  }

  int block_count() const { return block_count_of(bd); }

  int endpoint_match_count_of(const Board& b) const {
    int cnt = 0;
    for (int i = 0; i < R; i++) {
      if (!b.dsz[i]) continue;
      int tail = b.dep[i][b.dsz[i] - 1];
      for (int j = 0; j < R; j++)
        if (b.ssz[j] && good_adj(tail, b.side[j][0])) cnt++;
    }
    return cnt;
  }

  int endpoint_match_count() const { return endpoint_match_count_of(bd); }

  int seg_good_dep_suffix(const Board& b, int i, int k) const {
    int n = b.dsz[i], res = 0;
    for (int p = n - k; p + 1 < n; p++)
      if (good_adj(b.dep[i][p], b.dep[i][p + 1])) res++;
    return res;
  }

  int seg_good_side_prefix(const Board& b, int j, int k) const {
    int res = 0;
    for (int p = 0; p + 1 < k; p++)
      if (good_adj(b.side[j][p], b.side[j][p + 1])) res++;
    return res;
  }

  bool cuts_good_edge(const Board& b, const Move& m) const {
    if (m.type == 0) {
      int n = b.dsz[m.i];
      return n > m.k && good_adj(b.dep[m.i][n - m.k - 1], b.dep[m.i][n - m.k]);
    }
    int n = b.ssz[m.j];
    return n > m.k && good_adj(b.side[m.j][m.k - 1], b.side[m.j][m.k]);
  }

  int delta_good(const Board& b, const Move& m) const {
    if (m.type == 0) {
      int n = b.dsz[m.i];
      int lost = (n > m.k && good_adj(b.dep[m.i][n - m.k - 1], b.dep[m.i][n - m.k])) ? 1 : 0;
      int gain = (b.ssz[m.j] && good_adj(b.dep[m.i][n - 1], b.side[m.j][0])) ? 1 : 0;
      return gain - lost;
    }
    int n = b.ssz[m.j];
    int lost = (n > m.k && good_adj(b.side[m.j][m.k - 1], b.side[m.j][m.k])) ? 1 : 0;
    int gain = (b.dsz[m.i] && good_adj(b.dep[m.i][b.dsz[m.i] - 1], b.side[m.j][0])) ? 1 : 0;
    return gain - lost;
  }

  void simulate_one(Board& b, const Move& m) const {
    if (m.type == 0) {
      int dn = b.dsz[m.i], sn = b.ssz[m.j], k = m.k;
      for (int p = sn - 1; p >= 0; p--) b.side[m.j][p + k] = b.side[m.j][p];
      for (int t = 0; t < k; t++) b.side[m.j][t] = b.dep[m.i][dn - k + t];
      b.dsz[m.i] -= k;
      b.ssz[m.j] += k;
    } else {
      int dn = b.dsz[m.i], sn = b.ssz[m.j], k = m.k;
      for (int t = 0; t < k; t++) b.dep[m.i][dn + t] = b.side[m.j][t];
      for (int p = k; p < sn; p++) b.side[m.j][p - k] = b.side[m.j][p];
      b.dsz[m.i] += k;
      b.ssz[m.j] -= k;
    }
  }

  bool path_has_hash(const vector<BeamNode>& pool, int idx, uint64_t h) const {
    for (int p = idx; p != -1; p = pool[p].parent)
      if (pool[p].h == h) return true;
    return false;
  }

  bool would_be_seen_single(const Move& m) const {
    Board b = bd;
    simulate_one(b, m);
    return seen.count(hash_of(b));
  }

  int endpoint_match_after(const Board& b, const Move& m) const {
    Board nb = b;
    simulate_one(nb, m);
    return endpoint_match_count_of(nb);
  }

  int cut_surface_potential_of(const Board& b) const {
    int score = 0;
    int dep_tail[N_CAR] = {};
    int side_head[N_CAR] = {};
    for (int i = 0; i < R; i++)
      if (b.dsz[i]) dep_tail[b.dep[i][b.dsz[i] - 1]]++;
    for (int j = 0; j < R; j++)
      if (b.ssz[j]) side_head[b.side[j][0]]++;
    for (int i = 0; i < R; i++) {
      if (!b.dsz[i]) continue;
      int x = b.dep[i][b.dsz[i] - 1];
      if (x + 1 < N_CAR && x / 10 == (x + 1) / 10) score += side_head[x + 1];
    }
    for (int j = 0; j < R; j++) {
      if (!b.ssz[j]) continue;
      int x = b.side[j][0];
      if (x - 1 >= 0 && x / 10 == (x - 1) / 10) score += dep_tail[x - 1];
    }
    return score;
  }

  int cut_surface_potential_delta(const Board& b, const Move& m) const {
    int before = cut_surface_potential_of(b);
    Board nb = b;
    simulate_one(nb, m);
    return cut_surface_potential_of(nb) - before;
  }

  long long dst_balance_score(const Board& b, const Move& m) const {
    int before = m.type == 0 ? (int)b.ssz[m.j] : (int)b.dsz[m.i];
    int after = before + m.k;
    int cap = m.type == 0 ? SIDE_CAP : DEP_CAP;
    int free_after = cap - after;
    int lack = max(0, BALANCE_MIN_FREE - free_after);
    return BALANCE_FREE_AFTER_W * free_after
         - BALANCE_NEAR_FULL_PENALTY_W * 1LL * lack * lack
         - BALANCE_DST_AFTER_SQ_PENALTY_W * 1LL * after * after;
  }

  bool normalize_turn(vector<Move>& ops) const {
    if (ops.empty()) return false;
    sort(ops.begin(), ops.end(), [](const Move& a, const Move& b) {
      return tie(a.i, a.j) < tie(b.i, b.j);
    });
    for (auto& m : ops)
      if (m.type < 0 || m.type > 1 || m.i < 0 || m.i >= R || m.j < 0 || m.j >= R || m.k <= 0)
        return false;
    for (int t = 1; t < (int)ops.size(); t++)
      if (ops[t - 1].i >= ops[t].i || ops[t - 1].j >= ops[t].j) return false;
    return true;
  }

  bool normalize_turn(Turn& t) const {
    if (t.n <= 0) return false;
    sort(t.mv, t.mv + t.n, [](const Move& a, const Move& b) {
      return tie(a.i, a.j) < tie(b.i, b.j);
    });
    for (int x = 0; x < t.n; x++) {
      auto& m = t.mv[x];
      if (m.type < 0 || m.type > 1 || m.i < 0 || m.i >= R || m.j < 0 || m.j >= R || m.k <= 0)
        return false;
    }
    for (int x = 1; x < t.n; x++)
      if (t.mv[x - 1].i >= t.mv[x].i || t.mv[x - 1].j >= t.mv[x].j) return false;
    return true;
  }

  bool inverse_against(const vector<Move>& ops, const vector<Move>& prv) const {
    for (auto& m : ops)
      for (auto& p : prv)
        if (m.i == p.i && m.j == p.j && m.type != p.type) return true;
    return false;
  }

  bool inverse_against(const Turn& ops, const Move* prv, int prv_n) const {
    for (int a = 0; a < ops.n; a++)
      for (int b = 0; b < prv_n; b++)
        if (ops.mv[a].i == prv[b].i && ops.mv[a].j == prv[b].j && ops.mv[a].type != prv[b].type) return true;
    return false;
  }

  bool simulate_turn_local(
      Board& b,
      vector<Move>& local_last,
      unordered_set<uint64_t>& local_seen,
      vector<Move>& ops) const {
    if (!normalize_turn(ops)) return false;
    if (inverse_against(ops, local_last)) return false;
    for (auto& m : ops) {
      if (m.type == 0) {
        if ((int)b.dsz[m.i] < m.k || (int)b.ssz[m.j] + m.k > SIDE_CAP) return false;
      } else {
        if ((int)b.ssz[m.j] < m.k || (int)b.dsz[m.i] + m.k > DEP_CAP) return false;
      }
    }
    for (auto& m : ops) simulate_one(b, m);
    uint64_t h = hash_of(b);
    if (local_seen.count(h)) return false;
    local_seen.insert(h);
    local_last = ops;
    return true;
  }

  bool apply_sequence(const vector<vector<Move>>& turns) {
    if (turns.empty() || (int)ans.size() + (int)turns.size() > MAX_TURN) return false;
    Board b = bd;
    auto local_last = last_turn;
    auto local_seen = seen;
    vector<vector<Move>> normalized;
    for (auto ops : turns) {
      if (!simulate_turn_local(b, local_last, local_seen, ops)) return false;
      normalized.push_back(ops);
    }
    bd = b;
    last_turn = move(local_last);
    seen = move(local_seen);
    for (auto& ops : normalized) ans.push_back(ops);
    return true;
  }

  bool apply_turn(vector<Move> ops) {
    return apply_sequence({ops});
  }

  Turn select_parallel_chain(const vector<Cand>& cand) const {
    Cand best[R][R];
    bool has[R][R] = {};
    for (auto& c : cand) {
      int i = c.mv.i, j = c.mv.j;
      if (!has[i][j] || c.score > best[i][j].score) {
        has[i][j] = true;
        best[i][j] = c;
      }
    }
    const long long NEG = -(1LL << 60);
    long long dp[R + 1][R + 1];
    int ch[R][R + 1];
    for (int i = 0; i <= R; i++)
      for (int j = 0; j <= R; j++)
        dp[i][j] = NEG;
    memset(ch, -1, sizeof(ch));
    for (int lj = 0; lj <= R; lj++) dp[R][lj] = 0;
    for (int i = R - 1; i >= 0; i--) {
      for (int lj = 0; lj <= R; lj++) {
        long long bs = dp[i + 1][lj];
        int bj = -1;
        for (int j = lj; j < R; j++) {
          if (!has[i][j]) continue;
          long long val = best[i][j].score + dp[i + 1][j + 1];
          if (val > bs) { bs = val; bj = j; }
        }
        dp[i][lj] = bs;
        ch[i][lj] = bj;
      }
    }
    Turn t;
    if (dp[0][0] <= 0) return t;
    for (int i = 0, lj = 0; i < R; i++) {
      int j = ch[i][lj];
      if (j == -1) continue;
      t.mv[t.n++] = best[i][j].mv;
      lj = j + 1;
    }
    return t;
  }

  bool noncross_ok(const Turn& ops, const Move& m) const {
    for (int a = 0; a < ops.n; a++) {
      const Move& x = ops.mv[a];
      if (x.i == m.i || x.j == m.j) return false;
      if ((x.i < m.i) != (x.j < m.j)) return false;
    }
    return true;
  }

  uint64_t turn_key(Turn t) const {
    if (!normalize_turn(t)) return 0;
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < t.n; i++) {
      uint64_t x = (((uint64_t)t.mv[i].type * 10 + t.mv[i].i) * 10 + t.mv[i].j) * 32 + t.mv[i].k;
      h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    }
    return h ? h : 1;
  }

  bool try_apply_candidates(vector<Cand> cand) {
    if (cand.empty()) return false;
    Turn top = select_parallel_chain(cand);
    if (top.n) {
      vector<Move> ops(top.mv, top.mv + top.n);
      if (apply_turn(ops)) return true;
    }
    sort(cand.begin(), cand.end(), [](const Cand& a, const Cand& b) {
      return make_tuple(-a.score, -a.mv.k, a.mv.i, a.mv.j)
           < make_tuple(-b.score, -b.mv.k, b.mv.i, b.mv.j);
    });
    for (auto& c : cand)
      if (apply_turn({c.mv})) return true;
    return false;
  }

  vector<Cand> generate_merge_candidates_state(const Board& b, const Move* last, int last_n, int parent_idx, const vector<BeamNode>* pool) const {
    vector<Cand> cand;
    cand.reserve(160);
    for (int type = 0; type < 2; type++) {
      for (int i = 0; i < R; i++) {
        for (int j = 0; j < R; j++) {
          int src_n = type == 0 ? (int)b.dsz[i] : (int)b.ssz[j];
          int dst_n = type == 0 ? (int)b.ssz[j] : (int)b.dsz[i];
          int cap = type == 0 ? SIDE_CAP : DEP_CAP;
          int max_k = min(src_n, cap - dst_n);
          for (int k = 1; k <= max_k; k++) {
            Move m{type, i, j, k};
            if (forbidden_inverse_arr(m, last, last_n)) continue;
            int d = delta_good(b, m);
            if (d <= 0) continue;
            long long score = MERGE_DELTA_GOOD_W * d
                            + MERGE_CUT_SURFACE_W * cut_surface_potential_delta(b, m)
                            + dst_balance_score(b, m);
            cand.push_back({m, score});
          }
        }
      }
    }
    return cand;
  }

  vector<Cand> generate_expose_candidates_state(const Board& b, const Move* last, int last_n, int parent_idx, const vector<BeamNode>* pool) const {
    vector<Cand> cand;
    cand.reserve(160);
    int before_endpoint = endpoint_match_count_of(b);
    for (int type = 0; type < 2; type++) {
      for (int i = 0; i < R; i++) {
        for (int j = 0; j < R; j++) {
          int src_n = type == 0 ? (int)b.dsz[i] : (int)b.ssz[j];
          int dst_n = type == 0 ? (int)b.ssz[j] : (int)b.dsz[i];
          int cap = type == 0 ? SIDE_CAP : DEP_CAP;
          int max_k = min(src_n, cap - dst_n);
          for (int k = 1; k <= max_k; k++) {
            Move m{type, i, j, k};
            if (forbidden_inverse_arr(m, last, last_n)) continue;
            if (cuts_good_edge(b, m)) continue;
            if (delta_good(b, m) != 0) continue;
            int endpoint_diff = endpoint_match_after(b, m) - before_endpoint;
            int pot = cut_surface_potential_delta(b, m);
            if (endpoint_diff <= 0 && pot <= 0) continue;
            int inside = type == 0 ? seg_good_dep_suffix(b, i, k) : seg_good_side_prefix(b, j, k);
            long long score = EXPOSE_ENDPOINT_DIFF_W * endpoint_diff
                            + EXPOSE_CUT_SURFACE_W * pot
                            + EXPOSE_INSIDE_W * inside
                            + EXPOSE_K_W * k
                            - EXPOSE_DIST_W * abs(i - j)
                            + dst_balance_score(b, m);
            if (dst_n == 0) score += EXPOSE_EMPTY_DST_BONUS;
            cand.push_back({m, score});
          }
        }
      }
    }
    return cand;
  }

  long long evaluate_board(const Board& b, int depth) const {
    int blocks = block_count_of(b);
    int endpoint = endpoint_match_count_of(b);
    int surface = cut_surface_potential_of(b);
    long long bal = 0;
    for (int i = 0; i < R; i++) {
      bal -= 10LL * (int)b.dsz[i] * (int)b.dsz[i];
      bal -= 8LL * (int)b.ssz[i] * (int)b.ssz[i];
    }
    return -BEAM_BLOCK_W * blocks
         + BEAM_ENDPOINT_W * endpoint
         + BEAM_SURFACE_W * surface
         - BEAM_TURN_W * depth
         + bal;
  }

  bool simulate_node_turn(const vector<BeamNode>& pool, int idx, Turn ops, BeamNode& nxt) const {
    if (!normalize_turn(ops)) return false;
    const BeamNode& nd = pool[idx];
    if (inverse_against(ops, nd.last, nd.last_n)) return false;

    Board b = nd.b;

    for (int x = 0; x < ops.n; x++) {
      Move& m = ops.mv[x];
      if (m.type == 0) {
        if ((int)b.dsz[m.i] < m.k || (int)b.ssz[m.j] + m.k > SIDE_CAP) return false;
      } else {
        if ((int)b.ssz[m.j] < m.k || (int)b.dsz[m.i] + m.k > DEP_CAP) return false;
      }
    }

    for (int x = 0; x < ops.n; x++) simulate_one(b, ops.mv[x]);

    uint64_t h = hash_of(b);
    if (path_has_hash(pool, idx, h)) return false;

    nxt.b = b;
    nxt.parent = idx;
    nxt.depth = nd.depth + 1;
    nxt.blocks = block_count_of(b);
    nxt.eval = evaluate_board(b, nxt.depth);
    nxt.h = h;
    nxt.last_n = ops.n;
    nxt.ops_n = ops.n;
    for (int i = 0; i < ops.n; i++) {
      nxt.last[i] = ops.mv[i];
      nxt.ops[i] = ops.mv[i];
    }
    return true;
  }

  vector<Turn> make_turn_candidates(vector<Cand> cand) const {
    vector<Turn> res;
    vector<uint64_t> used;

    sort(cand.begin(), cand.end(), [](const Cand& a, const Cand& b) {
      return make_tuple(-a.score, -a.mv.k, a.mv.i, a.mv.j)
           < make_tuple(-b.score, -b.mv.k, b.mv.i, b.mv.j);
    });

    if ((int)cand.size() > BEAM_CAND_LIMIT) cand.resize(BEAM_CAND_LIMIT);

    auto add_turn = [&](Turn t) {
      uint64_t key = turn_key(t);
      if (!key) return;
      for (auto x : used) if (x == key) return;
      used.push_back(key);
      normalize_turn(t);
      res.push_back(t);
    };

    add_turn(select_parallel_chain(cand));

    int roots = min(BEAM_ROOT_LIMIT, (int)cand.size());

    for (int s = 0; s < roots; s++) {
      Turn t;
      t.mv[t.n++] = cand[s].mv;
      for (int u = 0; u < (int)cand.size(); u++) {
        if (u == s) continue;
        if (noncross_ok(t, cand[u].mv)) t.mv[t.n++] = cand[u].mv;
      }
      add_turn(t);
    }

    for (int u = 0; u < roots; u++) {
      Turn t;
      t.mv[t.n++] = cand[u].mv;
      add_turn(t);
    }

    sort(res.begin(), res.end(), [](const Turn& a, const Turn& b) {
      if (a.n != b.n) return a.n > b.n;
      int ka = 0, kb = 0;
      for (int i = 0; i < a.n; i++) ka += a.mv[i].k;
      for (int i = 0; i < b.n; i++) kb += b.mv[i].k;
      return ka > kb;
    });

    if ((int)res.size() > BEAM_BRANCH) res.resize(BEAM_BRANCH);
    return res;
  }

  void beam_search() {
    vector<BeamNode> pool;
    pool.reserve(1 + GREEDY_TURN_LIMIT * BEAM_WIDTH * BEAM_BRANCH);

    BeamNode root;
    memset(&root, 0, sizeof(root));
    root.b = bd;
    root.parent = -1;
    root.depth = 0;
    root.blocks = block_count_of(root.b);
    root.eval = evaluate_board(root.b, 0);
    root.h = current_hash();

    pool.push_back(root);

    vector<int> beam = {0};
    int best_idx = 0;

    for (int turn = 0; turn < GREEDY_TURN_LIMIT; turn++) {
      if ((int)pool[best_idx].depth >= GREEDY_ANS_LIMIT) break;
      if (pool[best_idx].blocks <= R) break;

      vector<int> nxts;
      nxts.reserve(BEAM_WIDTH * BEAM_BRANCH);

      for (int idx : beam) {
        const BeamNode& nd = pool[idx];
        if (nd.depth >= GREEDY_ANS_LIMIT) continue;

        vector<Cand> cand = generate_merge_candidates_state(nd.b, nd.last, nd.last_n, idx, &pool);
        if (cand.empty()) cand = generate_expose_candidates_state(nd.b, nd.last, nd.last_n, idx, &pool);
        if (cand.empty()) continue;

        auto turns = make_turn_candidates(move(cand));

        for (auto& ops : turns) {
          BeamNode nxt;
          if (!simulate_node_turn(pool, idx, ops, nxt)) continue;
          pool.push_back(nxt);
          nxts.push_back((int)pool.size() - 1);
        }
      }

      if (nxts.empty()) break;

      sort(nxts.begin(), nxts.end(), [&](int a, int b) {
        const BeamNode& x = pool[a];
        const BeamNode& y = pool[b];
        if (x.eval != y.eval) return x.eval > y.eval;
        if (x.blocks != y.blocks) return x.blocks < y.blocks;
        return x.depth < y.depth;
      });

      vector<int> nb;
      nb.reserve(BEAM_WIDTH);
      unordered_set<uint64_t> layer_seen;
      layer_seen.reserve(nxts.size() * 2 + 10);

      for (int idx : nxts) {
        uint64_t h = pool[idx].h;
        if (layer_seen.count(h)) continue;
        layer_seen.insert(h);
        nb.push_back(idx);
        if ((int)nb.size() >= BEAM_WIDTH) break;
      }

      beam = move(nb);

      for (int idx : beam) {
        const BeamNode& nd = pool[idx];
        const BeamNode& best = pool[best_idx];
        if (nd.blocks < best.blocks || (nd.blocks == best.blocks && nd.eval > best.eval)) {
          best_idx = idx;
        }
      }
    }

    vector<int> path;
    for (int p = best_idx; p != -1; p = pool[p].parent) path.push_back(p);
    reverse(path.begin(), path.end());

    bd = pool[best_idx].b;
    ans.clear();
    last_turn.clear();
    seen.clear();
    seen.reserve(SEEN_RESERVE + path.size() * 2);

    for (int idx : path) seen.insert(pool[idx].h);

    for (int t = 1; t < (int)path.size(); t++) {
      const BeamNode& nd = pool[path[t]];
      vector<Move> ops;
      for (int i = 0; i < nd.ops_n; i++) ops.push_back(nd.ops[i]);
      ans.push_back(ops);
    }

    const BeamNode& last = pool[best_idx];
    for (int i = 0; i < last.last_n; i++) last_turn.push_back(last.last[i]);
  }

  bool final_ok() const {
    for (int r = 0; r < R; r++) {
      if ((int)bd.dsz[r] != 10) return false;
      for (int c = 0; c < 10; c++)
        if (bd.dep[r][c] != 10 * r + c) return false;
    }
    for (int j = 0; j < R; j++)
      if (bd.ssz[j]) return false;
    return true;
  }

  bool unload_all_dep_to_side() {
    for (int i = 0; i < R; i++) {
      while (bd.dsz[i]) {
        vector<pair<long long, Move>> tries;
        for (int j = 0; j < R; j++) {
          int free = SIDE_CAP - (int)bd.ssz[j];
          if (free <= 0) continue;
          int k = min((int)bd.dsz[i], free);
          Move m{0, i, j, k};
          if (forbidden_inverse(m) || would_be_seen_single(m)) continue;
          long long score = UNLOAD_K_W * 1LL * k
                          + UNLOAD_FREE_W * 1LL * free
                          - UNLOAD_DIST_W * 1LL * abs(i - j)
                          + dst_balance_score(bd, m);
          tries.push_back({score, m});
        }
        sort(tries.begin(), tries.end(), [](auto& a, auto& b) { return a.first > b.first; });
        bool moved = false;
        for (auto& [s, m] : tries)
          if (apply_turn({m})) { moved = true; break; }
        if (!moved) return false;
      }
    }
    return true;
  }

  int need(int r) const { return bd.dsz[r]; }

  pair<int, int> find_car_in_side(int x) const {
    for (int j = 0; j < R; j++)
      for (int p = 0; p < bd.ssz[j]; p++)
        if (bd.side[j][p] == x) return {j, p};
    return {-1, -1};
  }

  int side_run_len(int j, int p, int r) const {
    int d = need(r), k = 0;
    while (d + k < 10 && p + k < bd.ssz[j] && bd.side[j][p + k] == 10 * r + d + k) k++;
    return k;
  }

  bool final_direct_batch() {
    vector<Cand> cand;
    for (int r = 0; r < R; r++) {
      int d = need(r);
      if (d >= 10) continue;
      for (int j = 0; j < R; j++) {
        if (bd.ssz[j] && bd.side[j][0] == 10 * r + d) {
          int k = side_run_len(j, 0, r);
          Move m{1, r, j, k};
          if (forbidden_inverse(m) || would_be_seen_single(m)) continue;
          long long score = FINAL_DIRECT_BASE_W
                          + FINAL_DIRECT_K_W * k
                          - FINAL_DIRECT_DIST_W * abs(r - j)
                          + dst_balance_score(bd, m);
          cand.push_back({m, score});
          break;
        }
      }
    }
    return try_apply_candidates(cand);
  }

  struct BlockedJob { int r, j, p, k, tmp; };

  vector<BlockedJob> select_final_blocked_batch() const {
    struct Job { int r, j, p, k, score; };
    vector<Job> jobs;
    for (int r = 0; r < R; r++) {
      int d = need(r);
      if (d >= 10) continue;
      auto [j, p] = find_car_in_side(10 * r + d);
      if (j == -1 || p <= 0) continue;
      int k = side_run_len(j, p, r);
      jobs.push_back({r, j, p, k, FINAL_BLOCKED_BASE + FINAL_BLOCKED_K_W * k - FINAL_BLOCKED_PENALTY_P * p});
    }
    int m = jobs.size();
    if (m == 0) return {};
    int free_cap[R];
    for (int i = 0; i < R; i++) free_cap[i] = DEP_CAP - (int)bd.dsz[i];
    long long best_score = LLONG_MIN;
    vector<BlockedJob> best;
    for (int mask = 1; mask < (1 << m); mask++) {
      vector<Job> sub;
      int used_side = 0, target_dep = 0;
      long long score = 0;
      bool ok = true;
      for (int b = 0; b < m && ok; b++) {
        if (!(mask >> b & 1)) continue;
        auto& jb = jobs[b];
        if (used_side >> jb.j & 1) { ok = false; break; }
        used_side |= 1 << jb.j;
        target_dep |= 1 << jb.r;
        score += jb.score;
        sub.push_back(jb);
      }
      if (!ok) continue;
      auto by_r = sub;
      sort(by_r.begin(), by_r.end(), [](auto& a, auto& b) { return a.r < b.r; });
      for (int i = 1; i < (int)by_r.size() && ok; i++)
        if (by_r[i - 1].j >= by_r[i].j) ok = false;
      if (!ok) continue;
      auto by_j = sub;
      sort(by_j.begin(), by_j.end(), [](auto& a, auto& b) { return a.j < b.j; });
      vector<int> tmp((int)by_j.size(), -1);
      function<bool(int, int)> dfs = [&](int idx, int last_dep) -> bool {
        if (idx == (int)by_j.size()) return true;
        for (int i = last_dep + 1; i < R; i++) {
          if (target_dep >> i & 1) continue;
          if (free_cap[i] < by_j[idx].p) continue;
          Move expose{1, i, by_j[idx].j, by_j[idx].p};
          if (forbidden_inverse(expose) || would_be_seen_single(expose)) continue;
          tmp[idx] = i;
          if (dfs(idx + 1, i)) return true;
        }
        tmp[idx] = -1;
        return false;
      };
      if (!dfs(0, -1)) continue;
      score += FINAL_BLOCKED_BATCH_W * (int)sub.size();
      if (score > best_score) {
        best_score = score;
        best.clear();
        for (int i = 0; i < (int)by_j.size(); i++)
          best.push_back({by_j[i].r, by_j[i].j, by_j[i].p, by_j[i].k, tmp[i]});
      }
    }
    return best;
  }

  bool execute_final_blocked_batch(const vector<BlockedJob>& batch) {
    if (batch.empty()) return false;
    vector<Move> expose, take, restore;
    for (auto& jb : batch) expose.push_back({1, jb.tmp, jb.j, jb.p});
    auto by_r = batch;
    sort(by_r.begin(), by_r.end(), [](auto& a, auto& b) { return a.r < b.r; });
    for (auto& jb : by_r) take.push_back({1, jb.r, jb.j, jb.k});
    for (auto& jb : batch) restore.push_back({0, jb.tmp, jb.j, jb.p});
    return apply_sequence({expose, take, restore});
  }

  bool final_fallback_one() {
    struct Job { int r, j, p, k, score; };
    vector<Job> jobs;
    for (int r = 0; r < R; r++) {
      int d = need(r);
      if (d >= 10) continue;
      auto [j, p] = find_car_in_side(10 * r + d);
      if (j == -1) continue;
      int k = side_run_len(j, p, r);
      jobs.push_back({r, j, p, k,
        FINAL_FALLBACK_K_W * k - FINAL_FALLBACK_PENALTY_P * p + (p == 0 ? FINAL_FALLBACK_DIRECT_BONUS : 0)});
    }
    sort(jobs.begin(), jobs.end(), [](auto& a, auto& b) { return a.score > b.score; });
    for (auto& jb : jobs) {
      if (jb.p == 0) {
        Move m{1, jb.r, jb.j, jb.k};
        if (!forbidden_inverse(m) && !would_be_seen_single(m) && apply_turn({m})) return true;
        continue;
      }
      int rem = jb.p;
      int fc[R];
      for (int i = 0; i < R; i++) fc[i] = DEP_CAP - (int)bd.dsz[i];
      vector<pair<int, int>> chunks;
      bool ok = true;
      while (rem > 0) {
        int bi = -1, bf = -1;
        for (int i = 0; i < R; i++)
          if (i != jb.r && fc[i] > bf) { bf = fc[i]; bi = i; }
        if (bi == -1 || bf <= 0) { ok = false; break; }
        int tk = min(rem, bf);
        chunks.push_back({bi, tk});
        fc[bi] -= tk;
        rem -= tk;
      }
      if (!ok) continue;
      vector<vector<Move>> seq;
      for (auto& [i, len] : chunks) seq.push_back({{1, i, jb.j, len}});
      seq.push_back({{1, jb.r, jb.j, jb.k}});
      for (int idx = (int)chunks.size() - 1; idx >= 0; idx--)
        seq.push_back({{0, chunks[idx].first, jb.j, chunks[idx].second}});
      if (apply_sequence(seq)) return true;
    }
    return false;
  }

  void final_assemble_from_side() {
    for (int guard = 0; !final_ok() && (int)ans.size() < MAX_TURN && guard < FINAL_GUARD_LIMIT; guard++) {
      bool prog = false;
      while (!final_ok() && final_direct_batch()) prog = true;
      if (final_ok()) break;
      auto batch = select_final_blocked_batch();
      if (!batch.empty() && execute_final_blocked_batch(batch)) continue;
      if (final_fallback_one()) continue;
      if (!prog) break;
    }
  }

  void solve() {
    beam_search();
    unload_all_dep_to_side();
    final_assemble_from_side();
  }
};

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);
  int input_R;
  cin >> input_R;
  vector<vector<int>> y(R, vector<int>(10));
  for (int r = 0; r < R; r++)
    for (int c = 0; c < 10; c++)
      cin >> y[r][c];
  Solver solver(y);
  solver.solve();
  cout << solver.ans.size() << '\n';
  for (auto& turn : solver.ans) {
    cout << turn.size() << '\n';
    for (auto& m : turn)
      cout << m.type << ' ' << m.i << ' ' << m.j << ' ' << m.k << '\n';
  }
}