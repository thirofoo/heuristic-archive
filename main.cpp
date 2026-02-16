#include <bits/stdc++.h>
#include "params.hpp"
using namespace std;

namespace utility {
  struct timer {
  chrono::steady_clock::time_point start;
  void CodeStart() {
    start = chrono::steady_clock::now();
  }
  double elapsed() const {
    using namespace std::chrono;
    return duration<double, milli>(steady_clock::now() - start).count();
  }
  } mytm;
}

struct State {
  array<int, 100> owner;
  array<int, 100> level;
  array<pair<int, int>, 8> pos;
  int t;
  array<long long, 8> score_p;

  State() : t(0) {
    owner.fill(-1);
    level.fill(0);
    pos.fill({0, 0});
    score_p.fill(0);
  }
};

struct Solver {
  struct MoveList {
    int n = 0;
    array<int, 100> a{};
  };

  int N = 0;
  int M = 0;
  int T = 0;
  int U = 0;
  array<int, 100> V;
  struct EnemyParam {
    double wa = 0.0;
    double wb = 0.0;
    double wc = 0.0;
    double wd = 0.0;
    double eps = 0.0;
  };
  vector<EnemyParam> enemy;
  State current;
  bool initialized = false;

  Params params = {};
  vector<array<double, 4>> enemy_candidates;
  vector<double> enemy_eps_candidates;
  vector<vector<double>> enemy_logp;
  static constexpr int kPosteriorTop = 16;
  static constexpr double kPosteriorTemp = 0.75;
  vector<array<int, kPosteriorTop>> enemy_post_idx;
  vector<array<double, kPosteriorTop>> enemy_post_cdf;
  vector<uint8_t> enemy_post_size;
  mutable array<uint32_t, 100> visit_stamp{};
  mutable array<uint32_t, 100> block_stamp{};
  mutable uint32_t visit_epoch = 1;
  mutable uint32_t block_epoch = 1;
  array<array<int, 4>, 100> neighbors{};
  array<uint8_t, 100> neighbor_deg{};

  Solver() {
    this->input();
  }

  void input() {
    if(!(cin >> N >> M >> T >> U)) return;
    for(int i = 0; i < N; i++) {
      for(int j = 0; j < N; j++) {
        cin >> V[i * N + j];
      }
    }
    enemy.assign(M, EnemyParam());

    current = State();
    for(int p = 0; p < M; p++) {
      int x, y;
      cin >> x >> y;
      int idx = x * N + y;
      current.owner[idx] = p;
      current.level[idx] = 1;
      current.pos[p] = {x, y};
    }
    current.t = 0;
    initialized = true;
    params = params_for_m(M);
    build_neighbors();
    init_scores(current);
    init_enemy_estimator();
  }

  bool read_turn_state(State& st, bool update_enemy_model = true) {
    int tx, ty;
    State prev = st;
    array<int, 8> selected;
    selected.fill(-1);
    for(int p = 0; p < M; p++) {
      if(!(cin >> tx >> ty)) return false;
      selected[p] = idx(tx, ty);
    }
    for(int p = 0; p < M; p++) {
      if(!(cin >> tx >> ty)) return false;
      st.pos[p] = {tx, ty};
    }
    for(int i = 0; i < N * N; i++) {
      if(!(cin >> st.owner[i])) return false;
    }
    for(int i = 0; i < N * N; i++) {
      if(!(cin >> st.level[i])) return false;
    }
    if(update_enemy_model) update_enemy_params(prev, selected);
    st.t += 1;
    init_scores(st);
    return true;
  }

  void init_enemy_estimator() {
    const array<double, 8> grid = {0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    enemy_candidates.clear();
    for(double wa : grid) {
      for(double wb : grid) {
        for(double wc : grid) {
          for(double wd : grid) {
            enemy_candidates.push_back({wa, wb, wc, wd});
          }
        }
      }
    }
    enemy_eps_candidates = {0.10, 0.15, 0.20, 0.25, 0.30, 0.35, 0.40, 0.45, 0.50};
    int wcnt = (int) enemy_candidates.size();
    int ecnt = (int) enemy_eps_candidates.size();
    enemy_logp.assign(max(0, M - 1), vector<double>(wcnt * ecnt, 0.0));
    enemy_post_idx.assign(max(0, M - 1), array<int, kPosteriorTop>{});
    enemy_post_cdf.assign(max(0, M - 1), array<double, kPosteriorTop>{});
    enemy_post_size.assign(max(0, M - 1), 0);

    constexpr double mu_w = 0.65;
    constexpr double mu_e = 0.30;
    constexpr double inv_2sig2_w = 1.0 / (2.0 * 0.22 * 0.22);
    constexpr double inv_2sig2_e = 1.0 / (2.0 * 0.13 * 0.13);
    for(int ai = 0; ai < max(0, M - 1); ai++) {
      vector<double>& logp = enemy_logp[ai];
      for(int wi = 0; wi < wcnt; wi++) {
        const auto& cand = enemy_candidates[wi];
        double dw2 = (cand[0] - mu_w) * (cand[0] - mu_w) + (cand[1] - mu_w) * (cand[1] - mu_w)
                     + (cand[2] - mu_w) * (cand[2] - mu_w) + (cand[3] - mu_w) * (cand[3] - mu_w);
        for(int ei = 0; ei < ecnt; ei++) {
          double de = enemy_eps_candidates[ei] - mu_e;
          logp[wi * ecnt + ei] = -dw2 * inv_2sig2_w - de * de * inv_2sig2_e;
        }
      }
    }

    for(int p = 1; p < M; p++) {
      int ai_idx = p - 1;
      int best_idx = 0;
      const vector<double>& logp = enemy_logp[ai_idx];
      for(int i = 1; i < (int) logp.size(); i++) {
        if(logp[i] > logp[best_idx]) best_idx = i;
      }
      int best_wi = best_idx / ecnt;
      int best_ei = best_idx % ecnt;
      const auto& best = enemy_candidates[best_wi];
      enemy[p].wa = best[0];
      enemy[p].wb = best[1];
      enemy[p].wc = best[2];
      enemy[p].wd = best[3];
      enemy[p].eps = enemy_eps_candidates[best_ei];
      rebuild_enemy_posterior(ai_idx);
    }
  }

  void rebuild_enemy_posterior(int ai_idx) {
    if(ai_idx < 0 || ai_idx >= (int) enemy_logp.size()) return;
    const vector<double>& logp = enemy_logp[ai_idx];
    if(logp.empty()) {
      enemy_post_size[ai_idx] = 0;
      return;
    }

    array<int, kPosteriorTop> best_idx{};
    array<double, kPosteriorTop> best_log{};
    int sz = 0;
    for(int i = 0; i < (int) logp.size(); i++) {
      double v = logp[i];
      int pos = sz;
      if(pos < kPosteriorTop) {
        best_idx[pos] = i;
        best_log[pos] = v;
        sz++;
      } else if(v > best_log[sz - 1]) {
        pos = sz - 1;
        best_idx[pos] = i;
        best_log[pos] = v;
      } else {
        continue;
      }
      while(pos > 0 && best_log[pos] > best_log[pos - 1]) {
        swap(best_log[pos], best_log[pos - 1]);
        swap(best_idx[pos], best_idx[pos - 1]);
        pos--;
      }
    }
    if(sz <= 0) {
      enemy_post_size[ai_idx] = 0;
      return;
    }

    enemy_post_size[ai_idx] = (uint8_t) sz;
    double mx = best_log[0];
    double inv_temp = 1.0 / kPosteriorTemp;
    double acc = 0.0;
    for(int j = 0; j < sz; j++) {
      enemy_post_idx[ai_idx][j] = best_idx[j];
      double z = (best_log[j] - mx) * inv_temp;
      if(z < -60.0) z = -60.0;
      acc += exp(z);
      enemy_post_cdf[ai_idx][j] = acc;
    }
    if(acc <= 0.0) {
      enemy_post_size[ai_idx] = 1;
      enemy_post_idx[ai_idx][0] = best_idx[0];
      enemy_post_cdf[ai_idx][0] = 1.0;
      return;
    }
    for(int j = 0; j < sz; j++) {
      enemy_post_cdf[ai_idx][j] /= acc;
    }
  }

  void update_enemy_params(const State& prev, const array<int, 8>& selected) {
    if(enemy_candidates.empty() || enemy_eps_candidates.empty()) return;
    int wcnt = (int) enemy_candidates.size();
    int ecnt = (int) enemy_eps_candidates.size();
    for(int p = 1; p < M; p++) {
      int obs = selected[p];
      if(obs < 0) continue;
      MoveList moves = enumerate_moves(prev, p);
      if(moves.n <= 0) continue;
      int ai_idx = p - 1;
      vector<double>& logp = enemy_logp[ai_idx];
      double inv_m = 1.0 / (double) moves.n;
      for(int wi = 0; wi < wcnt; wi++) {
        const auto& cand = enemy_candidates[wi];
        double max_score = -1e100;
        int best_count = 0;
        bool obs_best = false;
        for(int mi = 0; mi < moves.n; mi++) {
          int cell = moves.a[mi];
          double a = enemy_eval_cell(prev, p, cell, cand[0], cand[1], cand[2], cand[3]);
          if(a > max_score + 1e-12) {
            max_score = a;
            best_count = 1;
            obs_best = (cell == obs);
          } else if(fabs(a - max_score) <= 1e-12) {
            best_count++;
            if(cell == obs) obs_best = true;
          }
        }
        best_count = max(1, best_count);
        for(int ei = 0; ei < ecnt; ei++) {
          double eps = enemy_eps_candidates[ei];
          double prob = eps * inv_m;
          if(obs_best) prob += (1.0 - eps) / (double) best_count;
          if(prob < 1e-15) prob = 1e-15;
          logp[wi * ecnt + ei] += log(prob);
        }
      }

      int best_idx = 0;
      for(int i = 1; i < (int) logp.size(); i++) {
        if(logp[i] > logp[best_idx]) best_idx = i;
      }
      int best_wi = best_idx / ecnt;
      int best_ei = best_idx % ecnt;
      const auto& best = enemy_candidates[best_wi];
      enemy[p].wa = best[0];
      enemy[p].wb = best[1];
      enemy[p].wc = best[2];
      enemy[p].wd = best[3];
      enemy[p].eps = enemy_eps_candidates[best_ei];
      rebuild_enemy_posterior(ai_idx);
    }
  }

  inline int idx(int x, int y) const {
    return x * N + y;
  }

  void build_neighbors() {
    for(int x = 0; x < N; x++) {
      for(int y = 0; y < N; y++) {
        int v = idx(x, y);
        uint8_t deg = 0;
        if(y + 1 < N) neighbors[v][deg++] = idx(x, y + 1);
        if(x + 1 < N) neighbors[v][deg++] = idx(x + 1, y);
        if(y - 1 >= 0) neighbors[v][deg++] = idx(x, y - 1);
        if(x - 1 >= 0) neighbors[v][deg++] = idx(x - 1, y);
        neighbor_deg[v] = deg;
      }
    }
  }

  inline double enemy_eval_cell(
      const State& st,
      int p,
      int cell,
      double wa,
      double wb,
      double wc,
      double wd) const {
    int owner = st.owner[cell];
    int level = st.level[cell];
    if(owner == -1) return V[cell] * wa;
    if(owner == p) return (level < U) ? (V[cell] * wb) : 0.0;
    return (level == 1) ? (V[cell] * wc) : (V[cell] * wd);
  }

  void init_scores(State& st) const {
    st.score_p.fill(0);
    for(int i = 0; i < N * N; i++) {
      int owner = st.owner[i];
      int level = st.level[i];
      if(owner != -1) st.score_p[owner] += (long long) V[i] * level;
    }
  }

  void update_cell(State& st, int cell, int new_owner, int new_level) const {
    int old_owner = st.owner[cell];
    int old_level = st.level[cell];
    if(old_owner != -1) st.score_p[old_owner] -= (long long) V[cell] * old_level;
    if(new_owner != -1) st.score_p[new_owner] += (long long) V[cell] * new_level;
    st.owner[cell] = new_owner;
    st.level[cell] = new_level;
  }

  void update_pos(State& st, int p, int new_cell) const {
    int old_cell = idx(st.pos[p].first, st.pos[p].second);
    if(old_cell == new_cell) return;
    st.pos[p] = {new_cell / N, new_cell % N};
  }

  MoveList enumerate_moves(const State& st, int p) const {
    uint32_t ve = ++visit_epoch;
    if(ve == 0) {
      visit_stamp.fill(0);
      visit_epoch = 1;
      ve = 1;
    }
    uint32_t be = ++block_epoch;
    if(be == 0) {
      block_stamp.fill(0);
      block_epoch = 1;
      be = 1;
    }
    for(int i = 0; i < M; i++) {
      if(i == p) continue;
      int cell = idx(st.pos[i].first, st.pos[i].second);
      block_stamp[cell] = be;
    }

    int start = idx(st.pos[p].first, st.pos[p].second);
    int qbuf[100];
    int head = 0, tail = 0;
    visit_stamp[start] = ve;
    qbuf[tail++] = start;

    MoveList moves;
    while(head < tail) {
      int v = qbuf[head++];
      if(block_stamp[v] != be) moves.a[moves.n++] = v;
      if(st.owner[v] != p) continue;
      int deg = (int) neighbor_deg[v];
      for(int di = 0; di < deg; di++) {
        int ni = neighbors[v][di];
        if(visit_stamp[ni] == ve) continue;
        visit_stamp[ni] = ve;
        qbuf[tail++] = ni;
      }
    }

    if(moves.n <= 0) moves.a[moves.n++] = start;
    return moves;
  }

  template <class TargetContainer>
  void compute_returned(const State& st, const TargetContainer& target, array<char, 8>& returned) const {
    returned.fill(0);
    array<uint8_t, 100> cnt;
    array<uint16_t, 100> mask;
    cnt.fill(0);
    mask.fill(0);
    int touched_buf[100];
    int touched_size = 0;
    for(int p = 0; p < M; p++) {
      int cell = target[p];
      if(cnt[cell] == 0) touched_buf[touched_size++] = cell;
      cnt[cell]++;
      mask[cell] |= (uint16_t) (1u << p);
    }
    for(int i = 0; i < touched_size; i++) {
      int cell = touched_buf[i];
      if(cnt[cell] <= 1) continue;
      uint16_t m = mask[cell];
      int cell_owner = st.owner[cell];
      if(cell_owner != -1 && (m & (1u << cell_owner))) {
        m &= (uint16_t) ~(1u << cell_owner);
      }
      while(m) {
        int p = __builtin_ctz(m);
        returned[p] = 1;
        m &= (uint16_t) (m - 1);
      }
    }
  }

  void apply_turn_targets_inplace(State& st, const array<int, 8>& target) const {
    array<pair<int, int>, 8> start_pos = st.pos;
    array<pair<int, int>, 8> moved_pos = st.pos;
    for(int p = 0; p < M; p++) {
      int cell = target[p];
      moved_pos[p] = {cell / N, cell % N};
    }

    array<char, 8> returned;
    compute_returned(st, target, returned);

    for(int p = 0; p < M; p++) {
      if(returned[p]) continue;
      int cell = target[p];
      int cell_owner = st.owner[cell];
      int cell_level = st.level[cell];
      if(cell_owner == -1) {
        update_cell(st, cell, p, 1);
      } else if(cell_owner == p) {
        update_cell(st, cell, p, min(U, cell_level + 1));
      } else {
        int new_level = cell_level - 1;
        if(new_level <= 0) {
          update_cell(st, cell, p, 1);
        } else {
          update_cell(st, cell, cell_owner, new_level);
          returned[p] = 1;
        }
      }
    }

    for(int p = 0; p < M; p++) {
      int new_cell = returned[p] ? idx(start_pos[p].first, start_pos[p].second)
                                 : idx(moved_pos[p].first, moved_pos[p].second);
      update_pos(st, p, new_cell);
    }
    st.t += 1;
  }

  double move_heuristic(const State& st, int cell) const {
    int owner = st.owner[cell];
    int level = st.level[cell];
    double v = (double) V[cell];
    if(owner == -1) return v;
    if(owner == 0) return (level < U) ? v : 0.0;
    return (level == 1) ? (2.0 * v) : v;
  }

  static bool better_scored_cell(const pair<double, int>& a, const pair<double, int>& b) {
    if(a.first != b.first) return a.first > b.first;
    return a.second < b.second;
  }

  void cap_moves_by_heuristic(const State& st, vector<int>& moves, int cap) const {
    if(cap < 1) cap = 1;
    if((int) moves.size() <= cap) return;
    array<pair<double, int>, 100> scored;
    int scored_size = 0;
    for(int cell : moves) {
      scored[scored_size++] = {move_heuristic(st, cell), cell};
    }
    nth_element(scored.begin(), scored.begin() + cap, scored.begin() + scored_size, better_scored_cell);
    sort(scored.begin(), scored.begin() + cap, better_scored_cell);
    moves.clear();
    moves.reserve(cap);
    for(int i = 0; i < cap; i++) moves.push_back(scored[i].second);
  }

  int root_candidate_cap(int remaining_turns) const {
    int root_cap;
    if(remaining_turns >= 70) root_cap = 20;
    else if(remaining_turns >= 40) root_cap = 16;
    else if(remaining_turns >= 20) root_cap = 12;
    else root_cap = 9;
    root_cap = min(root_cap, params.max_branch);
    if(root_cap < 1) root_cap = 1;
    return root_cap;
  }

  void enumerate_root_moves(const State& root, vector<int>& moves) const {
    enumerate_moves_my(root, moves);
    if(!moves.empty()) return;
    MoveList base = enumerate_moves(root, 0);
    moves.reserve(base.n);
    for(int i = 0; i < base.n; i++) moves.push_back(base.a[i]);
  }

  double robust_root_value(double sum, double sq_sum, int cnt, int turn) const {
    if(cnt <= 0) return -1e100;
    double mean = sum / cnt;
    double var = sq_sum / cnt - mean * mean;
    if(var < 0.0) var = 0.0;
    double phase = (T <= 1) ? 1.0 : (double) turn / (double) (T - 1);
    double lambda = 0.18 - 0.10 * phase;
    if(lambda < 0.05) lambda = 0.05;
    return mean - lambda * sqrt(var);
  }

  void enumerate_moves_my(const State& st, vector<int>& moves) const {
    MoveList base_moves = enumerate_moves(st, 0);
    moves.clear();
    if((int) moves.capacity() < base_moves.n) moves.reserve(base_moves.n);
    for(int i = 0; i < base_moves.n; i++) {
      int cell = base_moves.a[i];
      if(st.owner[cell] == 0 && st.level[cell] >= U) continue;
      moves.push_back(cell);
    }
    if(moves.empty()) {
      for(int i = 0; i < base_moves.n; i++) moves.push_back(base_moves.a[i]);
    }
    cap_moves_by_heuristic(st, moves, params.max_branch);
  }

  double evaluate(const State& st) const {
    long long s0 = st.score_p[0];
    long long smax = 0;
    int strongest = 1;
    for(int p = 1; p < M; p++) {
      if(st.score_p[p] > smax) {
        smax = st.score_p[p];
        strongest = p;
      }
    }

    int remaining = T - st.t;

    // At game end or near end, use exact score only
    if(remaining <= 0 || params.eval_potential_w <= 0.0) {
      double s0d = max(1.0, (double) s0);
      double sad = max(1.0, (double) smax);
      return -1e5 * log2(1.0 + sad / s0d);
    }

    // Compute territory potential for player 0 and strongest enemy
    double my_reinforce = 0.0, my_expand = 0.0;
    double en_reinforce = 0.0, en_expand = 0.0;

    for(int i = 0; i < N * N; i++) {
      int owner = st.owner[i];
      if(owner == 0) {
        // Our cell: reinforcement potential
        if(st.level[i] < U) {
          my_reinforce += (double) V[i] * (U - st.level[i]);
        }
      } else if(owner == strongest) {
        // Strongest enemy cell: reinforcement potential
        if(st.level[i] < U) {
          en_reinforce += (double) V[i] * (U - st.level[i]);
        }
      } else if(owner == -1) {
        // Unowned cell: check adjacency for expansion potential
        int deg = (int) neighbor_deg[i];
        bool adj_me = false, adj_en = false;
        for(int d = 0; d < deg; d++) {
          int ni = neighbors[i][d];
          if(st.owner[ni] == 0) adj_me = true;
          if(st.owner[ni] == strongest) adj_en = true;
        }
        if(adj_me) my_expand += (double) V[i];
        if(adj_en) en_expand += (double) V[i];
      }
    }

    double phase = (double) remaining / (double) T;
    double w = phase * params.eval_potential_w;

    double s0_adj = max(1.0, (double) s0 + w * (my_reinforce + my_expand));
    double sa_adj = max(1.0, (double) smax + w * (en_reinforce + en_expand));

    return -1e5 * log2(1.0 + sa_adj / s0_adj);
  }

  inline uint64_t splitmix64(uint64_t x) const {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }

  inline double rand01_det(uint64_t seed) const {
    uint64_t z = splitmix64(seed);
    return (double) (z >> 11) * (1.0 / 9007199254740992.0);
  }

  inline uint64_t state_seed(const State& st) const {
    uint64_t h = 0x9e3779b97f4a7c15ULL ^ (uint64_t) (st.t + 1);
    for(int p = 0; p < M; p++) {
      int cell = idx(st.pos[p].first, st.pos[p].second);
      h ^= splitmix64((uint64_t) (cell + 1) ^ (0xbf58476d1ce4e5b9ULL * (uint64_t) (p + 1)));
    }
    return h;
  }

  EnemyParam sample_enemy_param_rollout(int p, double r_model) const {
    if(p <= 0 || p >= M) return EnemyParam();
    int ai_idx = p - 1;
    if(ai_idx < 0 || ai_idx >= (int) enemy_post_size.size()) return enemy[p];
    int sz = (int) enemy_post_size[ai_idx];
    if(sz <= 0) return enemy[p];

    int pick = 0;
    while(pick + 1 < sz && r_model > enemy_post_cdf[ai_idx][pick]) pick++;
    int flat_idx = enemy_post_idx[ai_idx][pick];
    int ecnt = (int) enemy_eps_candidates.size();
    if(ecnt <= 0) return enemy[p];
    int wi = flat_idx / ecnt;
    int ei = flat_idx % ecnt;
    if(wi < 0 || wi >= (int) enemy_candidates.size() || ei < 0 || ei >= ecnt) return enemy[p];

    EnemyParam model;
    model.wa = enemy_candidates[wi][0];
    model.wb = enemy_candidates[wi][1];
    model.wc = enemy_candidates[wi][2];
    model.wd = enemy_candidates[wi][3];
    model.eps = enemy_eps_candidates[ei];
    return model;
  }

  int sample_enemy_move_rollout(const State& st, int p, const EnemyParam& model, double r_eps, double r_sel) const {
    MoveList moves = enumerate_moves(st, p);
    int msz = moves.n;
    if(msz <= 1) return moves.a[0];

    double eps = model.eps;
    if(eps < 0.0) eps = 0.0;
    if(eps > 0.5) eps = 0.5;
    if(r_eps < eps) {
      int j = (int) (r_sel * msz);
      if(j < 0) j = 0;
      if(j >= msz) j = msz - 1;
      return moves.a[j];
    }

    array<int, 100> best_cells;
    int best_count = 0;
    double max_score = -1e100;
    for(int i = 0; i < msz; i++) {
      int cell = moves.a[i];
      double a = enemy_eval_cell(st, p, cell, model.wa, model.wb, model.wc, model.wd);
      if(best_count == 0 || a > max_score) {
        max_score = a;
        best_cells[0] = cell;
        best_count = 1;
      } else {
        double tol = 1e-12 * max(1.0, fabs(max_score));
        if(fabs(a - max_score) <= tol) {
          best_cells[best_count++] = cell;
        }
      }
    }
    if(best_count <= 0) return moves.a[0];
    int pick = (int) (r_sel * best_count);
    if(pick < 0) pick = 0;
    if(pick >= best_count) pick = best_count - 1;
    return best_cells[pick];
  }

  int choose_my_rollout_move(const State& st, uint64_t sample_id, int depth) const {
    MoveList base_moves = enumerate_moves(st, 0);
    int fallback = idx(st.pos[0].first, st.pos[0].second);
    if(base_moves.n <= 0) return fallback;

    array<int, 100> cand;
    int n = 0;
    for(int i = 0; i < base_moves.n; i++) {
      int cell = base_moves.a[i];
      if(st.owner[cell] == 0 && st.level[cell] >= U) continue;
      cand[n++] = cell;
    }
    if(n == 0) {
      for(int i = 0; i < base_moves.n; i++) cand[n++] = base_moves.a[i];
    }
    if(n == 1) return cand[0];

    uint64_t seed = state_seed(st);
    seed ^= 0x632be59bd9b4e019ULL * (sample_id + 1);
    seed ^= 0x9e3779b97f4a7c15ULL * (uint64_t) (depth + 1);
    double r_mode = rand01_det(seed ^ 0x243f6a8885a308d3ULL);
    double r_sel = rand01_det(seed ^ 0x13198a2e03707344ULL);

    double self_eps = params.self_random_eps;
    if(self_eps < 0.0) self_eps = 0.0;
    if(self_eps > 1.0) self_eps = 1.0;
    if(r_mode < self_eps) {
      int j = (int) (r_sel * n);
      if(j < 0) j = 0;
      if(j >= n) j = n - 1;
      return cand[j];
    }

    // Softmax selection over move_heuristic scores (normalized)
    double temp = params.softmax_temp;
    if(temp < 1e-6) temp = 1e-6;
    double inv_temp = 1.0 / temp;

    array<double, 100> h_scores;
    double max_h = -1e100;
    double min_h = 1e100;
    for(int i = 0; i < n; i++) {
      h_scores[i] = move_heuristic(st, cand[i]);
      if(h_scores[i] > max_h) max_h = h_scores[i];
      if(h_scores[i] < min_h) min_h = h_scores[i];
    }

    // Normalize scores to [0, 1] range before applying temperature
    double range = max_h - min_h;
    if(range < 1e-12) {
      // All scores are equal, pick uniformly
      int j = (int) (r_sel * n);
      if(j < 0) j = 0;
      if(j >= n) j = n - 1;
      return cand[j];
    }
    double inv_range = 1.0 / range;

    // Compute cumulative softmax probabilities
    array<double, 100> cum_prob;
    double acc = 0.0;
    for(int i = 0; i < n; i++) {
      double z = (h_scores[i] - max_h) * inv_range * inv_temp;
      if(z < -60.0) z = -60.0;
      acc += exp(z);
      cum_prob[i] = acc;
    }

    // Sample from the distribution
    double threshold = r_sel * acc;
    for(int i = 0; i < n; i++) {
      if(cum_prob[i] >= threshold) return cand[i];
    }
    return cand[n - 1];
  }

  double rollout_once(const State& root, int first_move, int horizon, uint64_t sample_id) const {
    State st = root;
    uint64_t root_seed = state_seed(root);
    array<EnemyParam, 8> sampled_enemy;
    for(int p = 1; p < M; p++) {
      uint64_t z = root_seed ^ (0x9e3779b97f4a7c15ULL * (sample_id + 1)) ^ (0xbf58476d1ce4e5b9ULL * (uint64_t) (p + 1));
      double r_model = rand01_det(z ^ 0x94d049bb133111ebULL);
      sampled_enemy[p] = sample_enemy_param_rollout(p, r_model);
    }
    for(int d = 0; d < horizon && st.t < T; d++) {
      uint64_t step_seed = state_seed(st);
      array<int, 8> target;
      if(d == 0) {
        target[0] = first_move;
      } else {
        target[0] = choose_my_rollout_move(st, sample_id, d);
      }
      for(int p = 1; p < M; p++) {
        uint64_t base = step_seed;
        base ^= 0x9e3779b97f4a7c15ULL * (sample_id + 1);
        base ^= 0xbf58476d1ce4e5b9ULL * (uint64_t) (d + 1);
        base ^= 0x94d049bb133111ebULL * (uint64_t) (p + 1);
        double r1 = rand01_det(base ^ 0x243f6a8885a308d3ULL);
        double r2 = rand01_det(base ^ 0x13198a2e03707344ULL);
        target[p] = sample_enemy_move_rollout(st, p, sampled_enemy[p], r1, r2);
      }
      apply_turn_targets_inplace(st, target);
    }
    return evaluate(st);
  }

  pair<int, int> monte_carlo_rollout_decision(const State& root, double turn_end) const {
    vector<int> moves;
    enumerate_root_moves(root, moves);
    if(moves.empty()) {
      return {root.pos[0].first, root.pos[0].second};
    }

    int remaining_turns = T - root.t;
    int root_cap = root_candidate_cap(remaining_turns);
    cap_moves_by_heuristic(root, moves, root_cap);

    int K = (int) moves.size();
    vector<double> sum(K, 0.0), sq_sum(K, 0.0);
    vector<int> cnt(K, 0);

    int horizon = params.rollout_horizon;
    if(horizon < 1) horizon = 1;
    horizon = min(horizon, max(1, remaining_turns));

    int best_idx = 0;
    double best_value = -1e100;

    double C = params.ucb_c;
    if(C < 0.0) C = 0.0;
    int total_cnt = 0;

    // Phase 1: Initialize each arm with one rollout
    for(int i = 0; i < K; i++) {
      if(utility::mytm.elapsed() > turn_end) break;
      double val = rollout_once(root, moves[i], horizon, 0);
      sum[i] += val;
      sq_sum[i] += val * val;
      cnt[i]++;
      total_cnt++;
    }

    // Compute reward scale for UCB exploration normalization
    // Use pooled stddev; fallback to range-based estimate if too few samples
    double ucb_scale = 1.0;
    if(total_cnt >= 2) {
      double total_sum_all = 0.0, total_sq_all = 0.0;
      double val_min = 1e100, val_max = -1e100;
      for(int i = 0; i < K; i++) {
        if(cnt[i] <= 0) continue;
        total_sum_all += sum[i];
        total_sq_all += sq_sum[i];
        double m = sum[i] / cnt[i];
        if(m < val_min) val_min = m;
        if(m > val_max) val_max = m;
      }
      double overall_mean = total_sum_all / total_cnt;
      double overall_var = total_sq_all / total_cnt - overall_mean * overall_mean;
      if(overall_var < 0.0) overall_var = 0.0;
      ucb_scale = sqrt(overall_var);
      // Fallback: if stddev is tiny, use range
      if(ucb_scale < 1.0) {
        ucb_scale = max(1.0, val_max - val_min);
      }
    }

    // Phase 2: UCB1-based arm selection with scaled exploration
    while(utility::mytm.elapsed() <= turn_end) {
      // Select arm with highest UCB1 value
      int sel = -1;
      double best_ucb = -1e100;
      double log_total = log((double) total_cnt);
      for(int i = 0; i < K; i++) {
        if(cnt[i] <= 0) {
          // Uninitialized arm gets highest priority
          sel = i;
          break;
        }
        double mean_i = sum[i] / cnt[i];
        double ucb = mean_i + C * ucb_scale * sqrt(log_total / cnt[i]);
        if(ucb > best_ucb) {
          best_ucb = ucb;
          sel = i;
        }
      }
      if(sel < 0) sel = 0;

      double val = rollout_once(root, moves[sel], horizon, (uint64_t) cnt[sel]);
      sum[sel] += val;
      sq_sum[sel] += val * val;
      cnt[sel]++;
      total_cnt++;

      // Periodically update scale (every 32 rollouts)
      if((total_cnt & 31) == 0 && total_cnt >= 4) {
        double ts = 0.0, tsq = 0.0;
        for(int i = 0; i < K; i++) {
          ts += sum[i];
          tsq += sq_sum[i];
        }
        double om = ts / total_cnt;
        double ov = tsq / total_cnt - om * om;
        if(ov < 0.0) ov = 0.0;
        double new_scale = sqrt(ov);
        if(new_scale >= 1.0) ucb_scale = new_scale;
      }
    }

    for(int i = 0; i < K; i++) {
      double rv = robust_root_value(sum[i], sq_sum[i], cnt[i], root.t);
      if(rv > best_value || (rv == best_value && moves[i] < moves[best_idx])) {
        best_value = rv;
        best_idx = i;
      }
    }
    int best_move = moves[best_idx];
    return {best_move / N, best_move % N};
  }

  void solve() {
    if(!initialized) return;
    utility::mytm.CodeStart();
    constexpr double kHarnessOverheadMs = 120.0;
    constexpr double kSafetyMs = 80.0;
    constexpr double kPostMoveMs = 5.0;
    double hard_deadline = params.time_limit_ms - kSafetyMs - kHarnessOverheadMs;
    if(hard_deadline < 0.0) hard_deadline = 0.0;
    double ema_read_ms = 7.0;
    for(int turn = 0; turn < T; turn++) {
      double turn_start = utility::mytm.elapsed();
      int rem_turns = T - turn;
      int rem_reads = max(0, rem_turns - 1);
      double reserve_reads = ema_read_ms * (double) rem_reads;
      double remaining_budget = hard_deadline - turn_start - reserve_reads;
      if(remaining_budget < 0.0) remaining_budget = 0.0;
      double sumw = (double) rem_turns * (double) (rem_turns + 1) * 0.5;
      double alloc = (sumw > 0.0) ? (remaining_budget * rem_turns / sumw) : 0.0;
      double turn_end = turn_start + alloc - kPostMoveMs;
      if(turn_end > hard_deadline - kPostMoveMs) turn_end = hard_deadline - kPostMoveMs;
      if(turn_end < 0.0) turn_end = 0.0;
      if(turn_end < turn_start) turn_end = turn_start;

      pair<int, int> decision = monte_carlo_rollout_decision(current, turn_end);
      cout << decision.first << " " << decision.second << "\n" << flush;
      if(turn + 1 >= T) break;

      double read_start = utility::mytm.elapsed();
      bool update_enemy_model = (read_start + 8.0 < hard_deadline);
      if(!read_turn_state(current, update_enemy_model)) break;
      double read_ms = utility::mytm.elapsed() - read_start;
      ema_read_ms = 0.9 * ema_read_ms + 0.1 * read_ms;
    }
  }
};

int main() {
  cin.tie(0);
  ios_base::sync_with_stdio(false);

  Solver solver;
  solver.solve();

  return 0;
}
