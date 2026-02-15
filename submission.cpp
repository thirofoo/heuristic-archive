#include <bits/stdc++.h>
#include "params.hpp"
using namespace std;

namespace utility {
  struct timer {
  chrono::steady_clock::time_point start;
  // 開始時間を記録
  void CodeStart() {
    start = chrono::steady_clock::now();
  }
  // 経過時間 (ms) を返す
  double elapsed() const {
    using namespace std::chrono;
    return duration<double, milli>(steady_clock::now() - start).count();
  }
  } mytm;
}

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty, ty = tz, tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline uint64_t rand_u64() {
  uint64_t hi = (uint64_t) rand_int();
  uint64_t lo = (uint64_t) rand_int();
  return (hi << 32) ^ lo;
}

inline double rand01() {
  return (double) rand_int() * (1.0 / 4294967296.0);
}

//-----------------以下から実装部分-----------------//

struct State {
  // --- 問題仕様に合わせた状態定義 ---
  // 盤面情報
  array<int, 100> owner; // O[100] 各マスの所有者
  array<int, 100> level; // L[100] 各マスのレベル
  // プレイヤー情報
  array<pair<int, int>, 8> pos; // 各プレイヤーの現在位置 pos[M] (先頭M要素を使用)
  // ターン数
  int t;
  // 評価値
  double score;
  array<long long, 8> score_p;
  uint64_t hash;

  State() : t(0), score(0.0), hash(0) {
    owner.fill(-1);
    level.fill(0);
    pos.fill({0, 0});
    score_p.fill(0);
  }
  // 必要に応じてコピーコンストラクタやmoveも追加可能

  bool operator<(const State& other) const {
    return score < other.score;
  }
  bool operator>(const State& other) const {
    return score > other.score;
  }
};

struct Solver {
  int N = 0;
  int M = 0;
  int T = 0;
  int U = 0;
  array<int, 100> V;
  int maxV = 0;
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
  vector<vector<uint64_t>> z_owner;
  vector<vector<uint64_t>> z_level;
  vector<vector<uint64_t>> z_pos;
  mutable vector<unordered_map<uint64_t, int>> enemy_cache;
  mutable vector<unordered_map<uint64_t, vector<int>>> move_cache;
  vector<array<double, 4>> enemy_candidates;
  vector<vector<double>> enemy_logp;
  static constexpr double kAssumedEps = 0.3;
  double value_cx = 0.0;
  double value_cy = 0.0;

  Solver() {
    this->input();
  }

  void input() {
    if(!(cin >> N >> M >> T >> U)) return;
    maxV = 0;
    for(int i = 0; i < N; i++) {
      for(int j = 0; j < N; j++) {
        cin >> V[i * N + j];
        if(V[i * N + j] > maxV) maxV = V[i * N + j];
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
    for(int p = 1; p < M; p++) {
      enemy[p].wa = 0.3 + 0.7 * rand01();
      enemy[p].wb = 0.3 + 0.7 * rand01();
      enemy[p].wc = 0.3 + 0.7 * rand01();
      enemy[p].wd = 0.3 + 0.7 * rand01();
      enemy[p].eps = 0.1 + 0.4 * rand01();
    }

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
    init_zobrist();
    init_scores_and_hash(current);
    enemy_cache.assign(M, {});
    move_cache.assign(M, {});
    for(auto& mp : enemy_cache) mp.reserve(4096);
    for(auto& mp : move_cache) mp.reserve(4096);
    init_enemy_estimator();
  }

  void output() {
    return;
  }

  bool read_turn_state(State& st) {
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
    update_enemy_params(prev, selected);
    st.t += 1;
    init_scores_and_hash(st);
    return true;
  }

  void init_enemy_estimator() {
    const array<double, 4> grid = {0.3, 0.5, 0.7, 0.9};
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
    enemy_logp.assign(max(0, M - 1), vector<double>(enemy_candidates.size(), 0.0));
    for(int p = 1; p < M; p++) {
      enemy[p].wa = 0.65;
      enemy[p].wb = 0.65;
      enemy[p].wc = 0.65;
      enemy[p].wd = 0.65;
      enemy[p].eps = kAssumedEps;
    }
  }

  void update_enemy_params(const State& prev, const array<int, 8>& selected) {
    if(enemy_candidates.empty()) return;
    for(int p = 1; p < M; p++) {
      int obs = selected[p];
      if(obs < 0) continue;
      const vector<int>& moves = enumerate_moves(prev, p);
      if(moves.empty()) continue;
      int ai_idx = p - 1;
      vector<double>& logp = enemy_logp[ai_idx];
      for(size_t ci = 0; ci < enemy_candidates.size(); ci++) {
        const auto& cand = enemy_candidates[ci];
        double max_score = -1e100;
        int best_count = 0;
        bool obs_best = false;
        for(int cell : moves) {
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
        if(obs_best) {
          logp[ci] += -log((double) max(1, best_count));
        } else {
          logp[ci] += log(1e-6);
        }
      }

      size_t best_ci = 0;
      for(size_t ci = 1; ci < logp.size(); ci++) {
        if(logp[ci] > logp[best_ci]) best_ci = ci;
      }
      const auto& best = enemy_candidates[best_ci];
      enemy[p].wa = best[0];
      enemy[p].wb = best[1];
      enemy[p].wc = best[2];
      enemy[p].wd = best[3];
      enemy[p].eps = kAssumedEps;
    }
    cerr.setf(ios::fixed);
    cerr << setprecision(3) << "t " << (prev.t + 1);
    for(int p = 1; p < M; p++) {
      cerr << " p" << p << ":" << enemy[p].wa << "," << enemy[p].wb << "," << enemy[p].wc
           << "," << enemy[p].wd;
    }
    cerr << "\n";
  }

  inline int idx(int x, int y) const {
    return x * N + y;
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

  void init_zobrist() {
    z_owner.assign(9, vector<uint64_t>(N * N, 0));
    z_level.assign(U + 1, vector<uint64_t>(N * N, 0));
    z_pos.assign(M, vector<uint64_t>(N * N, 0));
    for(int o = 0; o < (int) z_owner.size(); o++) {
      for(int i = 0; i < N * N; i++) z_owner[o][i] = rand_u64();
    }
    for(int l = 0; l < (int) z_level.size(); l++) {
      for(int i = 0; i < N * N; i++) z_level[l][i] = rand_u64();
    }
    for(int p = 0; p < M; p++) {
      for(int i = 0; i < N * N; i++) z_pos[p][i] = rand_u64();
    }
  }

  void init_scores_and_hash(State& st) const {
    st.score_p.fill(0);
    st.hash = 0;
    for(int i = 0; i < N * N; i++) {
      int owner = st.owner[i];
      int level = st.level[i];
      if(owner != -1) st.score_p[owner] += (long long) V[i] * level;
      st.hash ^= z_owner[owner + 1][i];
      st.hash ^= z_level[level][i];
    }
    for(int p = 0; p < M; p++) {
      int cell = idx(st.pos[p].first, st.pos[p].second);
      st.hash ^= z_pos[p][cell];
    }
  }

  void update_cell(State& st, int cell, int new_owner, int new_level) const {
    int old_owner = st.owner[cell];
    int old_level = st.level[cell];
    if(old_owner != -1) st.score_p[old_owner] -= (long long) V[cell] * old_level;
    if(new_owner != -1) st.score_p[new_owner] += (long long) V[cell] * new_level;
    st.hash ^= z_owner[old_owner + 1][cell];
    st.hash ^= z_level[old_level][cell];
    st.hash ^= z_owner[new_owner + 1][cell];
    st.hash ^= z_level[new_level][cell];
    st.owner[cell] = new_owner;
    st.level[cell] = new_level;
  }

  void update_pos(State& st, int p, int new_cell) const {
    int old_cell = idx(st.pos[p].first, st.pos[p].second);
    if(old_cell == new_cell) return;
    st.hash ^= z_pos[p][old_cell];
    st.hash ^= z_pos[p][new_cell];
    st.pos[p] = {new_cell / N, new_cell % N};
  }

  const vector<int>& enumerate_moves(const State& st, int p) const {
    uint64_t key = st.hash ^ (0x3c6ef372fe94f82aULL * (uint64_t) (p + 1));
    auto& cache = move_cache[p];
    auto it = cache.find(key);
    if(it != cache.end()) return it->second;

    array<char, 100> visited;
    visited.fill(0);
    array<char, 100> blocked;
    blocked.fill(0);
    for(int i = 0; i < M; i++) {
      if(i == p) continue;
      int cell = idx(st.pos[i].first, st.pos[i].second);
      blocked[cell] = 1;
    }

    int start = idx(st.pos[p].first, st.pos[p].second);
    int qbuf[100];
    int head = 0, tail = 0;
    visited[start] = 1;
    qbuf[tail++] = start;

    vector<int> moves;
    moves.reserve(64);
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {1, 0, -1, 0};
    while(head < tail) {
      int v = qbuf[head++];
      if(!blocked[v]) moves.push_back(v);
      if(st.owner[v] != p) continue;
      int x = v / N;
      int y = v % N;
      for(int d = 0; d < 4; d++) {
        int nx = x + dx[d];
        int ny = y + dy[d];
        if(nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
        int ni = idx(nx, ny);
        if(visited[ni]) continue;
        visited[ni] = 1;
        qbuf[tail++] = ni;
      }
    }

    if(moves.empty()) moves.push_back(start);
    auto [inserted_it, _] = cache.emplace(key, std::move(moves));
    return inserted_it->second;
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

  double move_heuristic(
      const State& st,
      int cell,
      const array<uint8_t, 100>& enemy_target_cnt,
      const array<uint16_t, 100>& enemy_target_mask) const {
    int x = cell / N;
    int y = cell % N;
    int owner = st.owner[cell];
    int level = st.level[cell];
    double v = (double) V[cell];
    double h = 0.0;

    // Base gain model
    if(owner == -1) {
      h += 1.00 * v;
    } else if(owner == 0) {
      if(level < U) h += 0.30 * v;
      else h -= 0.25 * v;
    } else {
      h += (level == 1 ? 2.10 : 0.70) * v;
    }

    // Local next-turn potential around destination.
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    double local_potential = 0.0;
    for(int d = 0; d < 4; d++) {
      int nx = x + dx[d];
      int ny = y + dy[d];
      if(nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
      int c = idx(nx, ny);
      int o = st.owner[c];
      int l = st.level[c];
      double nv = (double) V[c];
      if(o == -1) local_potential += 0.22 * nv;
      else if(o != 0 && l == 1) local_potential += 0.42 * nv;
      else if(o == 0 && l >= U) local_potential -= 0.08 * nv;
    }
    h += local_potential;

    int d1 = 100, d2 = 100;
    int near2 = 0;
    int near4 = 0;
    for(int p = 1; p < M; p++) {
      int md = abs(st.pos[p].first - x) + abs(st.pos[p].second - y);
      if(md < d1) {
        d2 = d1;
        d1 = md;
      } else if(md < d2) {
        d2 = md;
      }
      if(md <= 2) near2++;
      if(md <= 4) near4++;
    }

    if(M >= 4) {
      int edge_dist = min(min(x, N - 1 - x), min(y, N - 1 - y));
      int max_edge_dist = max(1, (N - 1) / 2);
      double edge_pref = (double) (max_edge_dist - edge_dist) / (double) max_edge_dist;
      h += 0.55 * v * edge_pref;

      double pressure = 0.0;
      if(d1 <= 1) pressure += 2.0;
      else if(d1 == 2) pressure += 1.0;
      else if(d1 == 3) pressure += 0.4;
      if(d2 <= 2) pressure += 0.9;
      else if(d2 == 3) pressure += 0.3;
      pressure += 0.25 * near2 + 0.08 * near4;
      h -= 0.55 * v * pressure;

      double d_value = fabs(x - value_cx) + fabs(y - value_cy);
      h += 0.20 * v * (d_value / max(1.0, 2.0 * (N - 1)));
    } else {
      // In low-player games, Lv1 sniping is highly efficient (+2 swing).
      if(owner > 0 && level == 1) h += 1.20 * v;
      if(d1 < 100) h += 0.45 * v / (1.0 + d1);
    }

    // This turn's predicted enemy targets (for branch pruning quality).
    int hit = enemy_target_cnt[cell];
    int neigh_hit = 0;
    const int dx2[4] = {1, -1, 0, 0};
    const int dy2[4] = {0, 0, 1, -1};
    for(int d = 0; d < 4; d++) {
      int nx = x + dx2[d];
      int ny = y + dy2[d];
      if(nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
      neigh_hit += enemy_target_cnt[idx(nx, ny)];
    }
    bool owner_defends = (owner > 0) && ((enemy_target_mask[cell] & (uint16_t) (1u << owner)) != 0);
    if(M >= 4) {
      h -= 0.85 * v * hit;
      h -= 0.22 * v * neigh_hit;
      if(owner_defends) h -= 1.10 * v;
      if(hit == 0 && neigh_hit == 0) h += 0.12 * v;
    } else {
      if(owner > 0 && level == 1 && !owner_defends) h += 0.55 * v;
      if(owner_defends) h -= 0.70 * v;
      h -= 0.25 * v * hit;
    }

    return h;
  }

  void enumerate_moves_my(const State& st, vector<int>& moves) const {
    const vector<int>& base_moves = enumerate_moves(st, 0);
    moves.clear();
    if((int) moves.capacity() < (int) base_moves.size()) moves.reserve(base_moves.size());
    for(int cell : base_moves) {
      if(st.owner[cell] == 0 && st.level[cell] >= U) continue;
      moves.push_back(cell);
    }
    if(moves.empty()) moves.assign(base_moves.begin(), base_moves.end());
    if((int) moves.size() <= params.max_branch) return;

    array<uint8_t, 100> enemy_target_cnt;
    enemy_target_cnt.fill(0);
    array<uint16_t, 100> enemy_target_mask;
    enemy_target_mask.fill(0);
    for(int p = 1; p < M; p++) {
      int c = decide_enemy_move_greedy(st, p);
      enemy_target_cnt[c]++;
      enemy_target_mask[c] |= (uint16_t) (1u << p);
    }

    array<pair<double, int>, 100> scored;
    int scored_size = 0;
    for(int cell : moves) {
      scored[scored_size++] = {move_heuristic(st, cell, enemy_target_cnt, enemy_target_mask), cell};
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

  int decide_enemy_move_greedy(const State& st, int p) const {
    uint64_t key = st.hash ^ (0x9e3779b97f4a7c15ULL * (uint64_t) (p + 1));
    if(p < (int) enemy_cache.size()) {
      auto it = enemy_cache[p].find(key);
      if(it != enemy_cache[p].end()) return it->second;
    }
    const vector<int>& moves = enumerate_moves(st, p);
    double best_score = -1e100;
    int best_move = moves[0];
    for(int i = 0; i < (int)moves.size(); i++) {
      int cell = moves[i];
      double a = enemy_eval_cell(st, p, cell, enemy[p].wa, enemy[p].wb, enemy[p].wc, enemy[p].wd);
      if(a > best_score || (a == best_score && cell < best_move)) {
        best_score = a;
        best_move = cell;
      }
    }
    if(p < (int) enemy_cache.size()) enemy_cache[p][key] = best_move;
    return best_move;
  }

  void build_enemy_target_prob(const State& st, array<array<double, 100>, 8>& prob) const {
    for(int p = 0; p < 8; p++) prob[p].fill(0.0);
    for(int p = 1; p < M; p++) {
      const vector<int>& moves = enumerate_moves(st, p);
      int msz = (int) moves.size();
      if(msz <= 0) continue;
      double eps = enemy[p].eps;
      if(eps < 0.0) eps = 0.0;
      if(eps > 0.5) eps = 0.5;
      double inv_m = 1.0 / (double) msz;
      array<double, 100> score;
      int n = 0;
      double max_score = -1e100;
      for(int cell : moves) {
        double a = enemy_eval_cell(st, p, cell, enemy[p].wa, enemy[p].wb, enemy[p].wc, enemy[p].wd);
        score[n++] = a;
        if(a > max_score) max_score = a;
      }
      double tol = 1e-12 * max(1.0, fabs(max_score));
      int best_count = 0;
      for(int i = 0; i < n; i++) {
        if(fabs(score[i] - max_score) <= tol) best_count++;
      }
      double add_rand = eps * inv_m;
      for(int cell : moves) prob[p][cell] += add_rand;
      if(best_count > 0) {
        double add_greedy = (1.0 - eps) / (double) best_count;
        for(int i = 0; i < n; i++) {
          if(fabs(score[i] - max_score) <= tol) {
            prob[p][moves[i]] += add_greedy;
          }
        }
      }
    }
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

  pair<int, int> beam_search_decision(const State& root, double turn_start, double turn_end) const {
    struct BeamNode {
      State st;
      int first_action;
      double score;
    };

    vector<BeamNode> layer;
    layer.reserve(params.beam_width);
    layer.push_back({root, -1, evaluate(root)});
    int best_move = idx(root.pos[0].first, root.pos[0].second);
    double best_eval = -1e100;

    auto better = [](const BeamNode& a, const BeamNode& b) {
      if(a.score != b.score) return a.score > b.score;
      return a.first_action < b.first_action;
    };

    double total_budget = max(1.0, turn_end - turn_start);
    for(int depth = 0; depth < params.max_depth; depth++) {
      double now = utility::mytm.elapsed();
      if(now > turn_end) break;
      double remaining = max(0.0, turn_end - now);
      int beam_limit = max(1, (int) (params.beam_width * (remaining / total_budget)));
      if(beam_limit > params.beam_width) beam_limit = params.beam_width;
      vector<BeamNode> next_layer;
      next_layer.reserve(beam_limit);
      int worst_idx = -1;
      vector<int> moves;
      moves.reserve(100);
      auto recompute_worst = [&]() {
        if(next_layer.empty()) {
          worst_idx = -1;
          return;
        }
        worst_idx = 0;
        for(int i = 1; i < (int) next_layer.size(); i++) {
          if(better(next_layer[worst_idx], next_layer[i])) {
            worst_idx = i;
          }
        }
      };
      for(const auto& parent : layer) {
        if(utility::mytm.elapsed() > turn_end) break;
        enumerate_moves_my(parent.st, moves);
        bool use_root_ev = (depth == 0);
        array<int, 8> enemy_target;
        enemy_target.fill(-1);
        for(int p = 1; p < M; p++) {
          enemy_target[p] = decide_enemy_move_greedy(parent.st, p);
        }
        array<array<double, 100>, 8> enemy_prob;
        array<double, 100> conflict_prob;
        if(use_root_ev) {
          build_enemy_target_prob(parent.st, enemy_prob);
          for(int cell = 0; cell < N * N; cell++) {
            double p_none = 1.0;
            for(int p = 1; p < M; p++) {
              p_none *= max(0.0, 1.0 - enemy_prob[p][cell]);
            }
            conflict_prob[cell] = 1.0 - p_none;
          }
        }
        for(int mv : moves) {
          if(utility::mytm.elapsed() > turn_end) break;
          BeamNode child = {parent.st, parent.first_action == -1 ? mv : parent.first_action, 0.0};
          array<int, 8> target = enemy_target;
          target[0] = mv;
          apply_turn_targets_inplace(child.st, target);
          child.score = evaluate(child.st);
          if(use_root_ev) {
            int owner = parent.st.owner[mv];
            int level = parent.st.level[mv];
            double v = (double) V[mv];
            double p_conf = conflict_prob[mv];
            if(p_conf < 0.0) p_conf = 0.0;
            if(p_conf > 1.0) p_conf = 1.0;
            double p_success = 1.0 - p_conf;
            double gain_if_success = 0.0;
            if(owner == -1) {
              gain_if_success = 1.00 * v;
            } else if(owner == 0) {
              gain_if_success = (level < U) ? 1.00 * v : 0.0;
            } else {
              gain_if_success = (level == 1) ? 1.25 * v : 0.18 * v;
            }
            double fail_cost = (owner <= 0 ? 0.32 * v : 0.44 * v);
            double ev_delta = p_success * gain_if_success - p_conf * fail_cost;
            if(owner > 0 && owner < M) {
              ev_delta -= 0.42 * v * p_success * enemy_prob[owner][mv];
            }
            int x = mv / N;
            int y = mv % N;
            const int dx[4] = {1, -1, 0, 0};
            const int dy[4] = {0, 0, 1, -1};
            double neigh_conf = 0.0;
            for(int d = 0; d < 4; d++) {
              int nx = x + dx[d];
              int ny = y + dy[d];
              if(nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
              neigh_conf += conflict_prob[idx(nx, ny)];
            }
            double risk_scale = (M >= 4 ? 0.56 : 0.32);
            double neigh_scale = (M >= 4 ? 0.11 : 0.06);
            child.score += risk_scale * ev_delta;
            child.score -= neigh_scale * v * neigh_conf;
          }
          if((int) next_layer.size() < beam_limit) {
            next_layer.push_back(std::move(child));
            int new_idx = (int) next_layer.size() - 1;
            if(worst_idx == -1 || better(next_layer[worst_idx], next_layer[new_idx])) {
              worst_idx = new_idx;
            }
          } else if(better(child, next_layer[worst_idx])) {
            next_layer[worst_idx] = std::move(child);
            recompute_worst();
          }
        }
      }

      if(next_layer.empty()) break;
      layer.swap(next_layer);

      for(const auto& node : layer) {
        if(node.score > best_eval && node.first_action != -1) {
          best_eval = node.score;
          best_move = node.first_action;
        }
      }
    }

    return {best_move / N, best_move % N};
  }

  void solve() {
    if(!initialized) return;
    utility::mytm.CodeStart();
    constexpr double kSafetyMs = 60.0;
    constexpr double kPostMoveMs = 5.0;
    double hard_deadline = params.time_limit_ms - kSafetyMs;
    if(hard_deadline < 0.0) hard_deadline = 0.0;
    for(int turn = 0; turn < T; turn++) {
      for(auto& mp : enemy_cache) mp.clear();
      for(auto& mp : move_cache) mp.clear();
      double turn_start = utility::mytm.elapsed();
      double turn_end = params.time_limit_ms * (double) (turn + 1) / (double) T;
      turn_end = min(turn_end, hard_deadline) - kPostMoveMs;
      if(turn_end < 0.0) turn_end = 0.0;
      if(turn_start > turn_end) turn_end = turn_start;
      pair<int, int> decision = beam_search_decision(current, turn_start, turn_end);
      cout << decision.first << " " << decision.second << "\n" << flush;
      if(!read_turn_state(current)) break;
    }
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
