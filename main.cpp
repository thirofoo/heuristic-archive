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

  const int BEAM_WIDTH = 100;
  const int MAX_DEPTH = 10;
  const int MAX_BRANCH = 100;
  const double TIME_LIMIT_MS = 1900.0;
  const double ALPHA = 1.0;
  const double BETA = 1.0;
  const double GAMMA = 0.02;

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
    }
    return moves;
  }

  double move_heuristic(const State& st, int cell) const {
    int owner = st.owner[cell];
    int level = st.level[cell];
    if(owner == -1) return 1.0 * V[cell];
    if(owner == 0) return (level < U ? 0.2 : 0.05) * V[cell];
    if(level == 1) return 1.3 * V[cell];
    return 0.6 * V[cell];
  }

  vector<int> enumerate_moves_my(const State& st) const {
    vector<int> moves = enumerate_moves(st, 0);
    if((int) moves.size() <= MAX_BRANCH) return moves;
    vector<pair<double, int>> scored;
    scored.reserve(moves.size());
    for(int cell : moves) scored.push_back({move_heuristic(st, cell), cell});
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
    for(const auto& it : scored) moves.push_back(it.second);
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

  struct CellChange {
    int idx;
    int owner;
    int level;
  };

  struct Undo {
    vector<CellChange> changes;
    vector<pair<int, int>> prev_pos;
    int prev_t = 0;
  };

  void apply_turn_greedy(State& st, int my_move, Undo& undo) const {
    undo.changes.clear();
    undo.prev_pos = st.pos;
    undo.prev_t = st.t;
    array<char, 100> touched;
    touched.fill(0);
    auto mark = [&](int cell) {
      if(touched[cell]) return;
      touched[cell] = 1;
      undo.changes.push_back({cell, st.owner[cell], st.level[cell]});
    };

    vector<int> target(M, -1);
    target[0] = my_move;
    for(int p = 1; p < M; p++) target[p] = decide_enemy_move_greedy(st, p);

    vector<pair<int, int>> moved_pos = st.pos;
    for(int p = 0; p < M; p++) {
      int cell = target[p];
      moved_pos[p] = {cell / N, cell % N};
    }

    vector<char> returned(M, 0);
    array<vector<int>, 100> landing;
    for(int i = 0; i < N * N; i++) landing[i].clear();
    for(int p = 0; p < M; p++) landing[target[p]].push_back(p);

    for(int cell = 0; cell < N * N; cell++) {
      if(landing[cell].size() <= 1) continue;
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
        for(int p : landing[cell]) if(p != owner_player) returned[p] = 1;
      } else {
        for(int p : landing[cell]) returned[p] = 1;
      }
    }

    for(int p = 0; p < M; p++) {
      if(returned[p]) continue;
      int cell = target[p];
      mark(cell);
      int cell_owner = st.owner[cell];
      int cell_level = st.level[cell];
      if(cell_owner == -1) {
        st.owner[cell] = p;
        st.level[cell] = 1;
      } else if(cell_owner == p) {
        st.level[cell] = min(U, cell_level + 1);
      } else {
        st.level[cell] = cell_level - 1;
        if(st.level[cell] <= 0) {
          st.owner[cell] = p;
          st.level[cell] = 1;
        } else {
          returned[p] = 1;
        }
      }
    }

    for(int p = 0; p < M; p++) {
      st.pos[p] = returned[p] ? undo.prev_pos[p] : moved_pos[p];
    }
    st.t = undo.prev_t + 1;
  }

  void undo_turn(State& st, const Undo& undo) const {
    for(const auto& ch : undo.changes) {
      st.owner[ch.idx] = ch.owner;
      st.level[ch.idx] = ch.level;
    }
    st.pos = undo.prev_pos;
    st.t = undo.prev_t;
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

  double evaluate(const State& st) const {
    vector<long long> score_p(M, 0);
    for(int i = 0; i < N * N; i++) {
      int owner = st.owner[i];
      if(owner == -1) continue;
      score_p[owner] += (long long) V[i] * st.level[i];
    }
    long long s0 = score_p[0];
    long long smax = 0;
    for(int p = 1; p < M; p++) smax = max(smax, score_p[p]);

    long long own_cnt = 0;
    long long frontier_value = 0;
    for(int i = 0; i < N * N; i++) {
      if(st.owner[i] != 0) continue;
      own_cnt++;
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
    double potential = (double) own_cnt + 0.001 * (double) frontier_value;

    return ALPHA * (double) s0 - BETA * (double) smax + GAMMA * potential;
  }

  pair<int, int> beam_search_decision(const State& root, double turn_end) const {
    struct Node {
      int parent;
      int action;
      int first_action;
      double score;
    };

    vector<Node> nodes;
    nodes.reserve(BEAM_WIDTH * MAX_DEPTH * 4);
    nodes.push_back({-1, -1, -1, evaluate(root)});
    vector<int> layer = {0};
    int best_move = idx(root.pos[0].first, root.pos[0].second);
    double best_eval = -1e100;

    for(int depth = 0; depth < MAX_DEPTH; depth++) {
      if(utility::mytm.elapsed() > turn_end) break;
      vector<int> next_layer;
      next_layer.reserve(BEAM_WIDTH * MAX_BRANCH);
      for(int node_idx : layer) {
        if(utility::mytm.elapsed() > turn_end) break;
        State st = root;
        vector<int> path;
        int cur = node_idx;
        while(cur > 0) {
          path.push_back(nodes[cur].action);
          cur = nodes[cur].parent;
        }
        reverse(path.begin(), path.end());
        for(int action : path) {
          Undo u;
          apply_turn_greedy(st, action, u);
        }

        vector<int> moves = enumerate_moves_my(st);
        for(int mv : moves) {
          if(utility::mytm.elapsed() > turn_end) break;
          Undo u;
          apply_turn_greedy(st, mv, u);
          double sc = evaluate(st);
          int first_action = (nodes[node_idx].first_action == -1 ? mv : nodes[node_idx].first_action);
          nodes.push_back({node_idx, mv, first_action, sc});
          next_layer.push_back((int) nodes.size() - 1);
          undo_turn(st, u);
        }
      }

      if(next_layer.empty()) break;
      int k = min((int) next_layer.size(), BEAM_WIDTH);
      if(k < (int) next_layer.size()) {
        nth_element(next_layer.begin(), next_layer.begin() + k, next_layer.end(),
                    [&](int a, int b) { return nodes[a].score > nodes[b].score; });
        next_layer.resize(k);
      }
      layer.swap(next_layer);

      for(int node_idx : layer) {
        const Node& node = nodes[node_idx];
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