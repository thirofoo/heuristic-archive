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
  vector<int> sx;
  vector<int> sy;
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
  mutable array<uint32_t, 100> visit_stamp{};
  mutable array<uint32_t, 100> block_stamp{};
  mutable uint32_t visit_epoch = 1;
  mutable uint32_t block_epoch = 1;
  array<array<int, 4>, 100> neighbors{};
  array<uint8_t, 100> neighbor_deg{};
  static constexpr double kDefaultEps = 0.3;
  double value_cx = 0.0;
  double value_cy = 0.0;

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
    {
      double wsum = 0.0;
      double sxw = 0.0;
      double syw = 0.0;
      for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
          double w = (double) V[i * N + j];
          wsum += w;
          sxw += w * i;
          syw += w * j;
        }
      }
      if(wsum > 0.0) {
        value_cx = sxw / wsum;
        value_cy = syw / wsum;
      } else {
        value_cx = (N - 1) * 0.5;
        value_cy = (N - 1) * 0.5;
      }
    }
    sx.assign(M, 0);
    sy.assign(M, 0);
    for(int p = 0; p < M; p++) {
      cin >> sx[p] >> sy[p];
    }
    enemy.assign(M, EnemyParam());

    current = State();
    current.pos.fill({0, 0});
    for(int p = 0; p < M; p++) {
      int idx = sx[p] * N + sy[p];
      current.owner[idx] = p;
      current.level[idx] = 1;
      current.pos[p] = {sx[p], sy[p]};
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
    st.pos.fill({0, 0});
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
    for(int p = 1; p < M; p++) {
      enemy[p].wa = 0.65;
      enemy[p].wb = 0.65;
      enemy[p].wc = 0.65;
      enemy[p].wd = 0.65;
      enemy[p].eps = kDefaultEps;
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
    if((int) moves.size() <= params.max_branch) return;

    array<pair<double, int>, 100> scored;
    int scored_size = 0;
    for(int cell : moves) {
      scored[scored_size++] = {move_heuristic(st, cell), cell};
    }
    int k = params.max_branch;
    nth_element(scored.begin(), scored.begin() + k, scored.begin() + scored_size,
                [](const pair<double, int>& a, const pair<double, int>& b) {
                  if(a.first != b.first) return a.first > b.first;
                  return a.second < b.second;
                });
    sort(scored.begin(), scored.begin() + k,
         [](const pair<double, int>& a, const pair<double, int>& b) {
           if(a.first != b.first) return a.first > b.first;
           return a.second < b.second;
         });
    moves.clear();
    moves.reserve(k);
    for(int i = 0; i < k; i++) moves.push_back(scored[i].second);
  }

  double evaluate(const State& st) const {
    long long s0 = st.score_p[0];
    long long smax = 0;
    for(int p = 1; p < M; p++) smax = max(smax, st.score_p[p]);
    double s0d = max(1.0, (double) s0);
    double sad = (double) smax;
    double abs_score = 1e5 * log2(1.0 + sad / s0d);
    double score = -abs_score;
    double phase = (T <= 1) ? 1.0 : (double) st.t / (double) (T - 1);

    int mx = st.pos[0].first;
    int my = st.pos[0].second;
    const double max_md = max(1.0, 2.0 * (N - 1));

    if(M >= 4) {
      int edge_dist = min(min(mx, N - 1 - mx), min(my, N - 1 - my));
      int max_edge_dist = max(1, (N - 1) / 2);
      double edge_pref = (double) (max_edge_dist - edge_dist) / (double) max_edge_dist;
      double edge_w = (3400.0 + 2200.0 * (1.0 - phase)) * (double) (M - 3);
      score += edge_w * edge_pref;

      int near2 = 0;
      int near4 = 0;
      array<char, 4> quadrant;
      quadrant.fill(0);
      double ex_sum = 0.0;
      double ey_sum = 0.0;
      int enemy_cnt = 0;
      for(int p = 1; p < M; p++) {
        int ex = st.pos[p].first;
        int ey = st.pos[p].second;
        int md = abs(ex - mx) + abs(ey - my);
        if(md <= 2) near2++;
        if(md <= 4) near4++;
        int q = (ex >= mx ? 1 : 0) | (ey >= my ? 2 : 0);
        quadrant[q] = 1;
        ex_sum += ex;
        ey_sum += ey;
        enemy_cnt++;
      }
      int qcnt = quadrant[0] + quadrant[1] + quadrant[2] + quadrant[3];
      score -= (2400.0 + 500.0 * phase) * near2;
      score -= (650.0 + 250.0 * phase) * near4;
      if(qcnt >= 3) score -= (1700.0 + 800.0 * phase) * (qcnt - 2);

      int pair_cnt = 0;
      double pair_sum = 0.0;
      for(int a = 1; a < M; a++) {
        for(int b = a + 1; b < M; b++) {
          pair_sum += abs(st.pos[a].first - st.pos[b].first) + abs(st.pos[a].second - st.pos[b].second);
          pair_cnt++;
        }
      }
      if(pair_cnt > 0) {
        double avg_pair = pair_sum / pair_cnt;
        double cluster = 1.0 - avg_pair / max_md;
        score += (3200.0 + 1200.0 * phase) * cluster;
      }

      if(enemy_cnt > 0) {
        double ecx = ex_sum / enemy_cnt;
        double ecy = ey_sum / enemy_cnt;
        double d_enemy = fabs(mx - ecx) + fabs(my - ecy);
        score += (2200.0 + 700.0 * (1.0 - phase)) * (d_enemy / max_md);
      }

      double d_value = fabs(mx - value_cx) + fabs(my - value_cy);
      score += (1200.0 + 900.0 * (1.0 - phase)) * (d_value / max_md);
    } else {
      int nearest_enemy = 100;
      for(int p = 1; p < M; p++) {
        int md = abs(st.pos[p].first - mx) + abs(st.pos[p].second - my);
        nearest_enemy = min(nearest_enemy, md);
      }
      if(nearest_enemy < 100) {
        double desired_dist = (phase < 0.7) ? 3.5 : 2.0;
        score -= 380.0 * fabs((double) nearest_enemy - desired_dist);
      }

      if(phase < 0.75) {
        double lead = ((double) s0 - (double) smax) / max(1.0, (double) (s0 + smax));
        if(lead > 0.0) {
          score -= 14000.0 * lead * (0.75 - phase);
        }
      }
    }

    return score;
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

  int sample_enemy_move_rollout(const State& st, int p, double r_eps, double r_sel) const {
    MoveList moves = enumerate_moves(st, p);
    int msz = moves.n;
    if(msz <= 1) return moves.a[0];

    double eps = enemy[p].eps;
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
      double a = enemy_eval_cell(st, p, cell, enemy[p].wa, enemy[p].wb, enemy[p].wc, enemy[p].wd);
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

    int best_cell = cand[0];
    double best_h = move_heuristic(st, best_cell);
    for(int i = 1; i < n; i++) {
      int cell = cand[i];
      double h = move_heuristic(st, cell);
      if(h > best_h || (h == best_h && cell < best_cell)) {
        best_h = h;
        best_cell = cell;
      }
    }
    return best_cell;
  }

  double rollout_once(const State& root, int first_move, int horizon, uint64_t sample_id) const {
    State st = root;
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
        target[p] = sample_enemy_move_rollout(st, p, r1, r2);
      }
      apply_turn_targets_inplace(st, target);
    }
    return evaluate(st);
  }

  pair<int, int> monte_carlo_rollout_decision(const State& root, double /*turn_start*/, double turn_end) const {
    vector<int> moves;
    enumerate_moves_my(root, moves);
    if(moves.empty()) {
      MoveList base = enumerate_moves(root, 0);
      moves.reserve(base.n);
      for(int i = 0; i < base.n; i++) moves.push_back(base.a[i]);
    }
    if(moves.empty()) {
      return {root.pos[0].first, root.pos[0].second};
    }

    int remaining_turns = T - root.t;
    int root_cap;
    if(remaining_turns >= 70) root_cap = 20;
    else if(remaining_turns >= 40) root_cap = 16;
    else if(remaining_turns >= 20) root_cap = 12;
    else root_cap = 9;
    root_cap = min(root_cap, params.max_branch);
    if(root_cap < 1) root_cap = 1;

    if((int) moves.size() > root_cap) {
      array<pair<double, int>, 100> scored;
      int n = 0;
      for(int mv : moves) scored[n++] = {move_heuristic(root, mv), mv};
      nth_element(scored.begin(), scored.begin() + root_cap, scored.begin() + n,
                  [](const pair<double, int>& a, const pair<double, int>& b) {
                    if(a.first != b.first) return a.first > b.first;
                    return a.second < b.second;
                  });
      sort(scored.begin(), scored.begin() + root_cap,
           [](const pair<double, int>& a, const pair<double, int>& b) {
             if(a.first != b.first) return a.first > b.first;
             return a.second < b.second;
           });
      moves.clear();
      moves.reserve(root_cap);
      for(int i = 0; i < root_cap; i++) moves.push_back(scored[i].second);
    }

    int K = (int) moves.size();
    vector<double> sum(K, 0.0), sq_sum(K, 0.0);
    vector<int> cnt(K, 0);

    int horizon = params.rollout_horizon;
    if(horizon < 1) horizon = 1;
    horizon = min(horizon, max(1, remaining_turns));

    int best_idx = 0;
    double best_value = -1e100;
    auto robust_value = [&](int i) {
      if(cnt[i] <= 0) return -1e100;
      double mean = sum[i] / cnt[i];
      double var = sq_sum[i] / cnt[i] - mean * mean;
      if(var < 0.0) var = 0.0;
      double phase = (T <= 1) ? 1.0 : (double) root.t / (double) (T - 1);
      double lambda = 0.18 - 0.10 * phase;
      if(lambda < 0.05) lambda = 0.05;
      return mean - lambda * sqrt(var);
    };

    uint64_t sample_round = 0;
    while(utility::mytm.elapsed() <= turn_end) {
      bool progressed = false;
      for(int i = 0; i < K; i++) {
        if(utility::mytm.elapsed() > turn_end) break;
        double val = rollout_once(root, moves[i], horizon, sample_round);
        sum[i] += val;
        sq_sum[i] += val * val;
        cnt[i]++;
        progressed = true;
      }
      if(!progressed) break;
      sample_round++;
    }

    for(int i = 0; i < K; i++) {
      double rv = robust_value(i);
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

      pair<int, int> decision = monte_carlo_rollout_decision(current, turn_start, turn_end);
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
