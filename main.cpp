#include <bits/stdc++.h>
using namespace std;

static constexpr int R = 10;
static constexpr int DEP_CAP = 15;
static constexpr int SIDE_CAP = 20;
static constexpr int N_CAR = 100;
static constexpr int MAX_TURN = 4000;

static constexpr int GREEDY_TURN_LIMIT = 650;
static constexpr int GREEDY_ANS_LIMIT = 100;
static constexpr int STAGNATION_LIMIT = 5;
static constexpr int FINAL_GUARD_LIMIT = 100;
static constexpr int SEEN_RESERVE = 10000;

static constexpr long long MERGE_DELTA_GOOD_W = 1000000LL;
static constexpr long long MERGE_CUT_SURFACE_W = 2000LL;

static constexpr long long EXPOSE_ENDPOINT_DIFF_W = 0LL;
static constexpr long long EXPOSE_CUT_SURFACE_W = 0LL;
static constexpr long long EXPOSE_INSIDE_W = 500LL;
static constexpr long long EXPOSE_K_W = 0LL;
static constexpr long long EXPOSE_DIST_W = 4LL;
static constexpr long long EXPOSE_EMPTY_DST_BONUS = 500LL;

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

struct Solver {
  vector<vector<int>> dep, side;
  vector<vector<Move>> ans;
  uint64_t zDep[R][DEP_CAP][N_CAR], zSide[R][SIDE_CAP][N_CAR];
  unordered_set<uint64_t> seen;
  vector<Move> last_turn;

  Solver(const vector<vector<int>>& y) : dep(y), side(R) {
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

  uint64_t hash_of(const vector<vector<int>>& D, const vector<vector<int>>& S) const {
    uint64_t h = 0;
    for (int i = 0; i < R; i++)
      for (int p = 0; p < (int)D[i].size(); p++)
        h ^= zDep[i][p][D[i][p]];
    for (int j = 0; j < R; j++)
      for (int p = 0; p < (int)S[j].size(); p++)
        h ^= zSide[j][p][S[j][p]];
    return h;
  }

  uint64_t current_hash() const { return hash_of(dep, side); }
  bool good_adj(int a, int b) const { return a / 10 == b / 10 && b == a + 1; }

  bool forbidden_inverse(const Move& m) const {
    for (auto& p : last_turn)
      if (p.i == m.i && p.j == m.j && p.type != m.type) return true;
    return false;
  }

  int block_count_of(const vector<vector<int>>& D, const vector<vector<int>>& S) const {
    int good = 0;
    for (int i = 0; i < R; i++) {
      for (int p = 0; p + 1 < (int)D[i].size(); p++)
        if (good_adj(D[i][p], D[i][p + 1])) good++;
      for (int p = 0; p + 1 < (int)S[i].size(); p++)
        if (good_adj(S[i][p], S[i][p + 1])) good++;
    }
    return N_CAR - good;
  }

  int block_count() const { return block_count_of(dep, side); }

  int endpoint_match_count_of(const vector<vector<int>>& D, const vector<vector<int>>& S) const {
    int cnt = 0;
    for (int i = 0; i < R; i++) {
      if (D[i].empty()) continue;
      for (int j = 0; j < R; j++)
        if (!S[j].empty() && good_adj(D[i].back(), S[j][0])) cnt++;
    }
    return cnt;
  }

  int endpoint_match_count() const { return endpoint_match_count_of(dep, side); }

  int seg_good_dep_suffix(int i, int k) const {
    int n = dep[i].size(), res = 0;
    for (int p = n - k; p + 1 < n; p++)
      if (good_adj(dep[i][p], dep[i][p + 1])) res++;
    return res;
  }

  int seg_good_side_prefix(int j, int k) const {
    int res = 0;
    for (int p = 0; p + 1 < k; p++)
      if (good_adj(side[j][p], side[j][p + 1])) res++;
    return res;
  }

  bool cuts_good_edge(const Move& m) const {
    if (m.type == 0) {
      int n = dep[m.i].size();
      return n > m.k && good_adj(dep[m.i][n - m.k - 1], dep[m.i][n - m.k]);
    }
    int n = side[m.j].size();
    return n > m.k && good_adj(side[m.j][m.k - 1], side[m.j][m.k]);
  }

  int delta_good(const Move& m) const {
    if (m.type == 0) {
      int n = dep[m.i].size();
      int lost = (n > m.k && good_adj(dep[m.i][n - m.k - 1], dep[m.i][n - m.k])) ? 1 : 0;
      int gain = (!side[m.j].empty() && good_adj(dep[m.i].back(), side[m.j][0])) ? 1 : 0;
      return gain - lost;
    }
    int n = side[m.j].size();
    int lost = (n > m.k && good_adj(side[m.j][m.k - 1], side[m.j][m.k])) ? 1 : 0;
    int gain = (!dep[m.i].empty() && good_adj(dep[m.i].back(), side[m.j][0])) ? 1 : 0;
    return gain - lost;
  }

  void simulate_one(vector<vector<int>>& D, vector<vector<int>>& S, const Move& m) const {
    if (m.type == 0) {
      S[m.j].insert(S[m.j].begin(), D[m.i].end() - m.k, D[m.i].end());
      D[m.i].resize(D[m.i].size() - m.k);
    } else {
      D[m.i].insert(D[m.i].end(), S[m.j].begin(), S[m.j].begin() + m.k);
      S[m.j].erase(S[m.j].begin(), S[m.j].begin() + m.k);
    }
  }

  bool would_be_seen_single(const Move& m) const {
    auto D = dep, S = side;
    simulate_one(D, S, m);
    return seen.count(hash_of(D, S));
  }

  int endpoint_match_after(const Move& m) const {
    auto D = dep, S = side;
    simulate_one(D, S, m);
    return endpoint_match_count_of(D, S);
  }

  int cut_surface_potential_of(const vector<vector<int>>& D, const vector<vector<int>>& S) const {
    int score = 0;
    vector<int> dep_tail(N_CAR, 0), side_head(N_CAR, 0);
    for (int i = 0; i < R; i++)
      if (!D[i].empty()) dep_tail[D[i].back()]++;
    for (int j = 0; j < R; j++)
      if (!S[j].empty()) side_head[S[j][0]]++;
    for (int i = 0; i < R; i++) {
      if (D[i].empty()) continue;
      int x = D[i].back();
      if (x + 1 < N_CAR && x / 10 == (x + 1) / 10) score += side_head[x + 1];
    }
    for (int j = 0; j < R; j++) {
      if (S[j].empty()) continue;
      int x = S[j][0];
      if (x - 1 >= 0 && x / 10 == (x - 1) / 10) score += dep_tail[x - 1];
    }
    return score;
  }

  int cut_surface_potential_delta(const Move& m) const {
    int before = cut_surface_potential_of(dep, side);
    auto D = dep, S = side;
    simulate_one(D, S, m);
    return cut_surface_potential_of(D, S) - before;
  }

  long long dst_balance_score(const Move& m) const {
    int before = m.type == 0 ? (int)side[m.j].size() : (int)dep[m.i].size();
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

  bool inverse_against(const vector<Move>& ops, const vector<Move>& prv) const {
    for (auto& m : ops)
      for (auto& p : prv)
        if (m.i == p.i && m.j == p.j && m.type != p.type) return true;
    return false;
  }

  bool simulate_turn_local(
      vector<vector<int>>& D, vector<vector<int>>& S,
      vector<Move>& local_last, unordered_set<uint64_t>& local_seen,
      vector<Move>& ops) const {
    if (!normalize_turn(ops)) return false;
    if (inverse_against(ops, local_last)) return false;
    for (auto& m : ops) {
      if (m.type == 0) {
        if ((int)D[m.i].size() < m.k || (int)S[m.j].size() + m.k > SIDE_CAP) return false;
      } else {
        if ((int)S[m.j].size() < m.k || (int)D[m.i].size() + m.k > DEP_CAP) return false;
      }
    }
    for (auto& m : ops) simulate_one(D, S, m);
    uint64_t h = hash_of(D, S);
    if (local_seen.count(h)) return false;
    local_seen.insert(h);
    local_last = ops;
    return true;
  }

  bool apply_sequence(const vector<vector<Move>>& turns) {
    if (turns.empty() || (int)ans.size() + (int)turns.size() > MAX_TURN) return false;
    auto D = dep, S = side;
    auto local_last = last_turn;
    auto local_seen = seen;
    vector<vector<Move>> normalized;
    for (auto ops : turns) {
      if (!simulate_turn_local(D, S, local_last, local_seen, ops)) return false;
      normalized.push_back(ops);
    }
    dep = move(D);
    side = move(S);
    last_turn = move(local_last);
    seen = move(local_seen);
    for (auto& ops : normalized) ans.push_back(ops);
    return true;
  }

  bool apply_turn(vector<Move> ops) {
    return apply_sequence({ops});
  }

  vector<Move> select_parallel_chain(const vector<Cand>& cand) const {
    vector<vector<char>> has(R, vector<char>(R, 0));
    vector<vector<Cand>> best(R, vector<Cand>(R));
    for (auto& c : cand) {
      int i = c.mv.i, j = c.mv.j;
      if (!has[i][j] || c.score > best[i][j].score) {
        has[i][j] = 1;
        best[i][j] = c;
      }
    }
    const long long NEG = -(1LL << 60);
    vector<vector<long long>> dp(R + 1, vector<long long>(R + 1, NEG));
    vector<vector<int>> ch(R, vector<int>(R + 1, -1));
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
    if (dp[0][0] <= 0) return {};
    vector<Move> ops;
    for (int i = 0, lj = 0; i < R; i++) {
      int j = ch[i][lj];
      if (j == -1) continue;
      ops.push_back(best[i][j].mv);
      lj = j + 1;
    }
    return ops;
  }

  bool try_apply_candidates(vector<Cand> cand) {
    if (cand.empty()) return false;
    auto ops = select_parallel_chain(cand);
    if (!ops.empty() && apply_turn(ops)) return true;
    sort(cand.begin(), cand.end(), [](const Cand& a, const Cand& b) {
      return make_tuple(-a.score, -a.mv.k, a.mv.i, a.mv.j)
           < make_tuple(-b.score, -b.mv.k, b.mv.i, b.mv.j);
    });
    for (auto& c : cand)
      if (apply_turn({c.mv})) return true;
    return false;
  }

  vector<Cand> generate_merge_candidates() const {
    vector<Cand> cand;
    for (int type = 0; type < 2; type++) {
      for (int i = 0; i < R; i++) {
        for (int j = 0; j < R; j++) {
          int src_n = type == 0 ? (int)dep[i].size() : (int)side[j].size();
          int dst_n = type == 0 ? (int)side[j].size() : (int)dep[i].size();
          int cap = type == 0 ? SIDE_CAP : DEP_CAP;
          int max_k = min(src_n, cap - dst_n);
          for (int k = 1; k <= max_k; k++) {
            Move m{type, i, j, k};
            if (forbidden_inverse(m) || would_be_seen_single(m)) continue;
            int d = delta_good(m);
            if (d <= 0) continue;
            long long score = MERGE_DELTA_GOOD_W * d
                            + MERGE_CUT_SURFACE_W * cut_surface_potential_delta(m)
                            + dst_balance_score(m);
            cand.push_back({m, score});
          }
        }
      }
    }
    return cand;
  }

  vector<Cand> generate_expose_candidates() const {
    vector<Cand> cand;
    int before_endpoint = endpoint_match_count();
    for (int type = 0; type < 2; type++) {
      for (int i = 0; i < R; i++) {
        for (int j = 0; j < R; j++) {
          const auto& dst = type == 0 ? side[j] : dep[i];
          int src_n = type == 0 ? (int)dep[i].size() : (int)side[j].size();
          int cap = type == 0 ? SIDE_CAP : DEP_CAP;
          int max_k = min(src_n, cap - (int)dst.size());
          for (int k = 1; k <= max_k; k++) {
            Move m{type, i, j, k};
            if (forbidden_inverse(m) || would_be_seen_single(m)) continue;
            if (cuts_good_edge(m)) continue;
            if (delta_good(m) != 0) continue;
            int endpoint_diff = endpoint_match_after(m) - before_endpoint;
            int pot = cut_surface_potential_delta(m);
            if (endpoint_diff <= 0 && pot <= 0) continue;
            int inside = type == 0 ? seg_good_dep_suffix(i, k) : seg_good_side_prefix(j, k);
            long long score = EXPOSE_ENDPOINT_DIFF_W * endpoint_diff
                            + EXPOSE_CUT_SURFACE_W * pot
                            + EXPOSE_INSIDE_W * inside
                            + EXPOSE_K_W * k
                            - EXPOSE_DIST_W * abs(i - j)
                            + dst_balance_score(m);
            if (dst.empty()) score += EXPOSE_EMPTY_DST_BONUS;
            cand.push_back({m, score});
          }
        }
      }
    }
    return cand;
  }

  bool greedy_step() {
    auto mc = generate_merge_candidates();
    if (try_apply_candidates(mc)) return true;
    auto ec = generate_expose_candidates();
    return try_apply_candidates(ec);
  }

  bool final_ok() const {
    for (int r = 0; r < R; r++) {
      if ((int)dep[r].size() != 10) return false;
      for (int c = 0; c < 10; c++)
        if (dep[r][c] != 10 * r + c) return false;
    }
    for (int j = 0; j < R; j++)
      if (!side[j].empty()) return false;
    return true;
  }

  bool unload_all_dep_to_side() {
    for (int i = 0; i < R; i++) {
      while (!dep[i].empty()) {
        vector<pair<long long, Move>> tries;
        for (int j = 0; j < R; j++) {
          int free = SIDE_CAP - (int)side[j].size();
          if (free <= 0) continue;
          int k = min((int)dep[i].size(), free);
          Move m{0, i, j, k};
          if (forbidden_inverse(m) || would_be_seen_single(m)) continue;
          long long score = UNLOAD_K_W * 1LL * k
                          + UNLOAD_FREE_W * 1LL * free
                          - UNLOAD_DIST_W * 1LL * abs(i - j)
                          + dst_balance_score(m);
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

  int need(int r) const { return dep[r].size(); }

  pair<int, int> find_car_in_side(int x) const {
    for (int j = 0; j < R; j++)
      for (int p = 0; p < (int)side[j].size(); p++)
        if (side[j][p] == x) return {j, p};
    return {-1, -1};
  }

  int side_run_len(int j, int p, int r) const {
    int d = need(r), k = 0;
    while (d + k < 10 && p + k < (int)side[j].size() && side[j][p + k] == 10 * r + d + k) k++;
    return k;
  }

  bool final_direct_batch() {
    vector<Cand> cand;
    for (int r = 0; r < R; r++) {
      int d = need(r);
      if (d >= 10) continue;
      for (int j = 0; j < R; j++) {
        if (!side[j].empty() && side[j][0] == 10 * r + d) {
          int k = side_run_len(j, 0, r);
          Move m{1, r, j, k};
          if (forbidden_inverse(m) || would_be_seen_single(m)) continue;
          long long score = FINAL_DIRECT_BASE_W
                          + FINAL_DIRECT_K_W * k
                          - FINAL_DIRECT_DIST_W * abs(r - j)
                          + dst_balance_score(m);
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
    vector<int> free_cap(R);
    for (int i = 0; i < R; i++) free_cap[i] = DEP_CAP - (int)dep[i].size();
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
      vector<int> fc(R);
      for (int i = 0; i < R; i++) fc[i] = DEP_CAP - (int)dep[i].size();
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
    int stag = 0, prev = block_count();
    for (int turn = 0; turn < GREEDY_TURN_LIMIT; turn++) {
      if (block_count() <= R || (int)ans.size() >= GREEDY_ANS_LIMIT) break;
      if (!greedy_step()) break;
      int cur = block_count();
      stag = cur < prev ? 0 : stag + 1;
      prev = cur;
      if (stag >= STAGNATION_LIMIT) break;
    }
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
