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
  // --- 問題仕様に合わせた状態定義 ---
  // 盤面情報
  array<int, 100> owner; // O[100] 各マスの所有者
  array<int, 100> level; // L[100] 各マスのレベル
  // プレイヤー情報
  vector<pair<int, int>> pos; // 各プレイヤーの現在位置 pos[M]
  // ターン数
  int t;
  // 評価値
  double score;

  State() : t(0), score(0.0) {
    owner.fill(-1);
    level.fill(0);
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
  vector<vector<double>> r;
  State current;
  bool initialized = false;

  const int BEAM_WIDTH = 1;
  const int MAX_DEPTH = 3;
  const int MAX_BRANCH = 30;
  const double TIME_LIMIT_MS = 1800.0;
  const double ALPHA = 1.0; // ratio term
  const double BETA = 1.0;  // opponent scale
  const double GAMMA = 0.01; // potential term
  const double OPP_SMOOTH_TAU = 2000.0;

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
    sx.assign(M, 0);
    sy.assign(M, 0);
    for(int p = 0; p < M; p++) {
      cin >> sx[p] >> sy[p];
    }
    enemy.assign(M, EnemyParam());
    for(int p = 1; p < M; p++) {
      cin >> enemy[p].wa >> enemy[p].wb >> enemy[p].wc >> enemy[p].wd >> enemy[p].eps;
    }
    r.assign(max(0, M - 1), vector<double>(2 * T, 0.0));
    for(int t = 0; t < T; t++) {
      for(int i = 0; i < M - 1; i++) {
        cin >> r[i][2 * t] >> r[i][2 * t + 1];
      }
    }

    current = State();
    current.pos.assign(M, {0, 0});
    for(int p = 0; p < M; p++) {
      int idx = sx[p] * N + sy[p];
      current.owner[idx] = p;
      current.level[idx] = 1;
      current.pos[p] = {sx[p], sy[p]};
    }
    current.t = 0;
    initialized = true;
  }

  void output() {
    return;
  }

  inline int idx(int x, int y) const {
    return x * N + y;
  }

  vector<pair<int, int>> get_candidates_ordered(const State& st, int p) const {
    vector<pair<int, int>> reachable;
    vector<vector<char>> visited(N, vector<char>(N, 0));
    queue<pair<int, int>> q;
    int sx = st.pos[p].first;
    int sy = st.pos[p].second;
    visited[sx][sy] = 1;
    q.push({sx, sy});
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {1, 0, -1, 0};
    while(!q.empty()) {
      auto [x, y] = q.front();
      q.pop();
      bool ok = true;
      for(int i = 0; i < M; i++) {
        if(i != p && st.pos[i].first == x && st.pos[i].second == y) {
          ok = false;
          break;
        }
      }
      if(ok) reachable.push_back({x, y});
      if(st.owner[idx(x, y)] == p) {
        for(int d = 0; d < 4; d++) {
          int nx = x + dx[d];
          int ny = y + dy[d];
          if(nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
          if(visited[nx][ny]) continue;
          visited[nx][ny] = 1;
          q.push({nx, ny});
        }
      }
    }
    return reachable;
  }

  void compute_reachable(const State& st, int p, array<char, 100>& reachable) const {
    reachable.fill(0);
    queue<int> q;
    int start = idx(st.pos[p].first, st.pos[p].second);
    reachable[start] = 1;
    q.push(start);
    while(!q.empty()) {
      int v = q.front();
      q.pop();
      int x = v / N;
      int y = v % N;
      const int dx[4] = {1, -1, 0, 0};
      const int dy[4] = {0, 0, 1, -1};
      for(int d = 0; d < 4; d++) {
        int nx = x + dx[d];
        int ny = y + dy[d];
        if(nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
        int ni = idx(nx, ny);
        if(reachable[ni]) continue;
        if(st.owner[ni] != p) continue;
        reachable[ni] = 1;
        q.push(ni);
      }
    }
  }

  vector<int> enumerate_moves(const State& st, int p) const {
    vector<int> moves;
    vector<pair<int, int>> cand = get_candidates_ordered(st, p);
    moves.reserve(cand.size());
    for(auto [x, y] : cand) moves.push_back(idx(x, y));
    if(moves.empty()) {
      int stay = idx(st.pos[p].first, st.pos[p].second);
      moves.push_back(stay);
      return moves;
    }
    if(p == 0 && (int) moves.size() > MAX_BRANCH) {
      vector<pair<double, int>> scored;
      scored.reserve(moves.size());
      for(int cell : moves) {
        int owner = st.owner[cell];
        int level = st.level[cell];
        double h = 0.0;
        if(owner == -1) h = V[cell];
        else if(owner == 0) h = 0.2 * V[cell];
        else if(level == 1) h = 1.5 * V[cell];
        else h = 0.7 * V[cell];
        scored.push_back({h, cell});
      }
      int k = MAX_BRANCH;
      nth_element(scored.begin(), scored.begin() + k, scored.end(),
                  [](const auto& a, const auto& b) {
                    if(a.first != b.first) return a.first > b.first;
                    return a.second < b.second;
                  });
      scored.resize(k);
      sort(scored.begin(), scored.end(),
           [](const auto& a, const auto& b) {
             if(a.first != b.first) return a.first > b.first;
             return a.second < b.second;
           });
      moves.clear();
      moves.reserve(k);
      for(auto& it : scored) moves.push_back(it.second);
    }
    return moves;
  }

  int decide_enemy_move_greedy(const State& st, int p) const {
    vector<int> moves = enumerate_moves(st, p);
    double best_score = -1e100;
    int best_move = moves[0];
    for(int i = 0; i < (int)moves.size(); i++) {
      int cell = moves[i];
      int owner = st.owner[cell];
      int level = st.level[cell];
      double a = 0.0;
      if(owner == -1) {
        a = V[cell] * enemy[p].wa;
      } else if(owner == p) {
        if(level < U) a = V[cell] * enemy[p].wb;
        else a = 0.0;
      } else {
        if(level == 1) a = V[cell] * enemy[p].wc;
        else a = V[cell] * enemy[p].wd;
      }
      if(a > best_score || (a == best_score && cell < best_move)) {
        best_score = a;
        best_move = cell;
      }
    }
    return best_move;
  }

  int decide_enemy_move_actual(const State& st, int p, int turn) const {
    int ai_idx = p - 1;
    vector<pair<int, int>> cand = get_candidates_ordered(st, p);
    vector<double> scores;
    scores.reserve(cand.size());
    for(auto [x, y] : cand) {
      int cell = idx(x, y);
      int owner = st.owner[cell];
      int level = st.level[cell];
      double a = 0.0;
      if(owner == -1) {
        a = V[cell] * enemy[p].wa;
      } else if(owner == p) {
        if(level < U) a = V[cell] * enemy[p].wb;
        else a = 0.0;
      } else {
        if(level == 1) a = V[cell] * enemy[p].wc;
        else a = V[cell] * enemy[p].wd;
      }
      scores.push_back(a);
    }

    double eps = enemy[p].eps;
    double r1 = r[ai_idx][2 * (turn % T)];
    double r2 = r[ai_idx][2 * (turn % T) + 1];
    if(r1 < eps) {
      size_t idx_sel = (size_t) floor(r2 * cand.size());
      if(idx_sel >= cand.size()) idx_sel = cand.size() - 1;
      return idx(cand[idx_sel].first, cand[idx_sel].second);
    }
    double max_score = -1e100;
    for(double v : scores) max_score = max(max_score, v);
    double tolerance = 1e-9 * max(fabs(max_score), 1.0);
    vector<int> best_ids;
    for(int i = 0; i < (int) scores.size(); i++) {
      if(scores[i] >= max_score - tolerance) best_ids.push_back(i);
    }
    size_t idx_sel = (size_t) floor(r2 * best_ids.size());
    if(idx_sel >= best_ids.size()) idx_sel = best_ids.size() - 1;
    int pick = best_ids[idx_sel];
    return idx(cand[pick].first, cand[pick].second);
  }

  State simulate_turn_greedy(const State& st, int my_move) const {
    State next = st;
    vector<int> target(M, -1);
    target[0] = my_move;
    for(int p = 1; p < M; p++) {
      target[p] = decide_enemy_move_greedy(st, p);
    }

    vector<pair<int, int>> start_pos = st.pos;
    vector<pair<int, int>> moved_pos = st.pos;
    for(int p = 0; p < M; p++) {
      int cell = target[p];
      moved_pos[p] = {cell / N, cell % N};
    }

    vector<char> returned(M, 0);
    array<vector<int>, 100> landing;
    for(int i = 0; i < N * N; i++) landing[i].clear();
    for(int p = 0; p < M; p++) {
      landing[target[p]].push_back(p);
    }

    for(int cell = 0; cell < N * N; cell++) {
      if(landing[cell].empty()) continue;
      if(landing[cell].size() == 1) continue;
      int cell_owner = st.owner[cell];
      bool owner_here = false;
      int owner_player = -1;
      if(cell_owner != -1) {
        for(int p : landing[cell]) {
          if(p == cell_owner) {
            owner_here = true;
            owner_player = p;
            break;
          }
        }
      }
      if(owner_here) {
        for(int p : landing[cell]) {
          if(p == owner_player) continue;
          returned[p] = 1;
        }
      } else {
        for(int p : landing[cell]) returned[p] = 1;
      }
    }

    // Territory update
    for(int p = 0; p < M; p++) {
      if(returned[p]) continue;
      int cell = target[p];
      int cell_owner = next.owner[cell];
      int cell_level = next.level[cell];
      if(cell_owner == -1) {
        next.owner[cell] = p;
        next.level[cell] = 1;
      } else if(cell_owner == p) {
        next.level[cell] = min(U, cell_level + 1);
      } else {
        // attack
        next.level[cell] = cell_level - 1;
        if(next.level[cell] <= 0) {
          next.owner[cell] = p;
          next.level[cell] = 1;
        } else {
          returned[p] = 1;
        }
      }
    }

    // Piece return
    for(int p = 0; p < M; p++) {
      if(returned[p]) next.pos[p] = start_pos[p];
      else next.pos[p] = moved_pos[p];
    }
    next.t = st.t + 1;
    return next;
  }

  State simulate_turn_actual(const State& st, int my_move, int turn) const {
    State next = st;
    vector<int> target(M, -1);
    target[0] = my_move;
    for(int p = 1; p < M; p++) {
      target[p] = decide_enemy_move_actual(st, p, turn);
    }

    vector<pair<int, int>> start_pos = st.pos;
    vector<pair<int, int>> moved_pos = st.pos;
    for(int p = 0; p < M; p++) {
      int cell = target[p];
      moved_pos[p] = {cell / N, cell % N};
    }

    vector<char> returned(M, 0);
    array<vector<int>, 100> landing;
    for(int i = 0; i < N * N; i++) landing[i].clear();
    for(int p = 0; p < M; p++) {
      landing[target[p]].push_back(p);
    }

    for(int cell = 0; cell < N * N; cell++) {
      if(landing[cell].empty()) continue;
      if(landing[cell].size() == 1) continue;
      int cell_owner = st.owner[cell];
      bool owner_here = false;
      int owner_player = -1;
      if(cell_owner != -1) {
        for(int p : landing[cell]) {
          if(p == cell_owner) {
            owner_here = true;
            owner_player = p;
            break;
          }
        }
      }
      if(owner_here) {
        for(int p : landing[cell]) {
          if(p == owner_player) continue;
          returned[p] = 1;
        }
      } else {
        for(int p : landing[cell]) returned[p] = 1;
      }
    }

    // Territory update
    for(int p = 0; p < M; p++) {
      if(returned[p]) continue;
      int cell = target[p];
      int cell_owner = next.owner[cell];
      int cell_level = next.level[cell];
      if(cell_owner == -1) {
        next.owner[cell] = p;
        next.level[cell] = 1;
      } else if(cell_owner == p) {
        next.level[cell] = min(U, cell_level + 1);
      } else {
        // attack
        next.level[cell] = cell_level - 1;
        if(next.level[cell] <= 0) {
          next.owner[cell] = p;
          next.level[cell] = 1;
        } else {
          returned[p] = 1;
        }
      }
    }

    // Piece return
    for(int p = 0; p < M; p++) {
      if(returned[p]) next.pos[p] = start_pos[p];
      else next.pos[p] = moved_pos[p];
    }
    next.t = st.t + 1;
    return next;
  }

  double smooth_max_opponent(const vector<long long>& score_p) const {
    if(M <= 1) return 0.0;
    double mx = 0.0;
    for(int p = 1; p < M; p++) mx = max(mx, (double) score_p[p]);
    double sum = 0.0;
    for(int p = 1; p < M; p++) {
      sum += exp(((double) score_p[p] - mx) / OPP_SMOOTH_TAU);
    }
    return mx + OPP_SMOOTH_TAU * log(sum);
  }

  double evaluate(const State& st) const {
    vector<long long> score_p(M, 0);
    for(int i = 0; i < N * N; i++) {
      int owner = st.owner[i];
      if(owner == -1) continue;
      score_p[owner] += (long long) V[i] * st.level[i];
    }
    long long s0 = score_p[0];
    double smax = smooth_max_opponent(score_p);

    // Potential: reachable own cells + frontier value sum
    array<char, 100> reachable;
    compute_reachable(st, 0, reachable);
    long long reach_cnt = 0;
    long long frontier_value = 0;
    for(int i = 0; i < N * N; i++) {
      if(!reachable[i]) continue;
      reach_cnt++;
      int x = i / N;
      int y = i % N;
      const int dx[4] = {1, -1, 0, 0};
      const int dy[4] = {0, 0, 1, -1};
      for(int d = 0; d < 4; d++) {
        int nx = x + dx[d];
        int ny = y + dy[d];
        if(nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
        int ni = idx(nx, ny);
        if(st.owner[ni] != 0) frontier_value += V[ni];
      }
    }
    double potential = (double) reach_cnt + 0.001 * (double) frontier_value;

    double ratio = log1p((double) s0 / (BETA * smax + 1.0));
    return ALPHA * ratio + GAMMA * potential;
  }

  pair<int, int> beam_search_decision(const State& st, double turn_end) const {
    struct Node {
      State st;
      int first_move;
    };

    vector<Node> beam;
    beam.reserve(BEAM_WIDTH);
    Node root;
    root.st = st;
    root.first_move = idx(st.pos[0].first, st.pos[0].second);
    beam.push_back(root);

    int best_move = root.first_move;
    double best_eval = -1e100;

    for(int depth = 0; depth < MAX_DEPTH; depth++) {
      if(utility::mytm.elapsed() > turn_end) break;
      vector<Node> next_beam;
      next_beam.reserve(BEAM_WIDTH * 4);
      for(const Node& node : beam) {
        vector<int> moves = enumerate_moves(node.st, 0);
        for(int mv : moves) {
          if(utility::mytm.elapsed() > turn_end) break;
          State ns = simulate_turn_greedy(node.st, mv);
          ns.score = evaluate(ns);
          Node child;
          child.st = ns;
          child.first_move = (depth == 0 ? mv : node.first_move);
          next_beam.push_back(child);
        }
        if(utility::mytm.elapsed() > turn_end) break;
      }
      if(next_beam.empty()) break;
      int k = min((int) next_beam.size(), BEAM_WIDTH);
      if(k < (int) next_beam.size()) {
        nth_element(next_beam.begin(),
                    next_beam.begin() + k,
                    next_beam.end(),
                    [](const Node& a, const Node& b) { return a.st.score > b.st.score; });
        next_beam.resize(k);
      }
      beam.swap(next_beam);

      for(const Node& node : beam) {
        if(node.st.score > best_eval) {
          best_eval = node.st.score;
          best_move = node.first_move;
        }
      }
    }

    return {best_move / N, best_move % N};
  }

  void solve() {
    if(!initialized) return;
    utility::mytm.CodeStart();
    for(int turn = 0; turn < T; turn++) {
      double turn_end = TIME_LIMIT_MS * (double) (turn + 1) / (double) T;
      pair<int, int> decision = beam_search_decision(current, turn_end);
      cout << decision.first << " " << decision.second << "\n" << flush;
      int move_idx = idx(decision.first, decision.second);
      current = simulate_turn_actual(current, move_idx, turn);
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