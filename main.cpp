#include <bits/stdc++.h>
#include <atcoder/all>

using namespace std;

namespace utility {
struct Timer {
  chrono::steady_clock::time_point start;

  void begin() { start = chrono::steady_clock::now(); }

  double elapsed_ms() const {
    using namespace std::chrono;
    return (double)duration_cast<milliseconds>(steady_clock::now() - start)
        .count();
  }
} timer;
}  // namespace utility

inline unsigned int rand_int() {
  static unsigned int tx = 123456789;
  static unsigned int ty = 362436069;
  static unsigned int tz = 521288629;
  static unsigned int tw = 88675123;
  unsigned int tt = tx ^ (tx << 11);
  tx = ty;
  ty = tz;
  tz = tw;
  return tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8));
}

inline double rand_double() {
  return (double)(rand_int() % 1000000000U) / 1000000000.0;
}

struct Node {
  int vid;
  int priority;
  int sz;
  bool rev;
  long long weight;
  long long sum_weight;
  long long sum_index_weight;
  Node *l, *r, *p;

  Node(int vid_, long long weight_)
      : vid(vid_),
        priority((int)rand_int()),
        sz(1),
        rev(false),
        weight(weight_),
        sum_weight(weight_),
        sum_index_weight(0),
        l(nullptr),
        r(nullptr),
        p(nullptr) {}
};

struct Move {
  int old1;
  int old2;
  int new1;
  int new2;
};

struct PatternTemplate {
  array<pair<int, int>, 4> points;
  array<pair<int, int>, 2> old_pairs;
  array<pair<int, int>, 2> new_pairs;
};

static constexpr array<pair<int, int>, 100> BLOCK_DROP_PATH = {{
    {0, 0}, {0, 1}, {1, 0}, {2, 0}, {1, 1}, {0, 2}, {0, 3}, {1, 2}, {2, 1},
    {3, 0}, {4, 0}, {3, 1}, {2, 2}, {1, 3}, {0, 4}, {0, 5}, {1, 4}, {2, 3},
    {3, 2}, {4, 1}, {5, 0}, {6, 0}, {5, 1}, {4, 2}, {3, 3}, {2, 4}, {1, 5},
    {0, 6}, {0, 7}, {1, 6}, {2, 5}, {3, 4}, {4, 3}, {5, 2}, {6, 1}, {7, 0},
    {8, 0}, {9, 0}, {9, 1}, {9, 2}, {8, 1}, {7, 1}, {6, 2}, {5, 3}, {4, 4},
    {3, 5}, {2, 6}, {1, 7}, {0, 8}, {0, 9}, {1, 9}, {1, 8}, {2, 9}, {3, 9},
    {2, 8}, {2, 7}, {3, 6}, {4, 5}, {5, 4}, {6, 3}, {7, 2}, {8, 2}, {9, 3},
    {9, 4}, {8, 3}, {7, 3}, {6, 4}, {5, 5}, {4, 6}, {3, 7}, {3, 8}, {4, 9},
    {5, 9}, {4, 8}, {4, 7}, {5, 6}, {5, 7}, {5, 8}, {6, 9}, {7, 9}, {6, 8},
    {6, 7}, {7, 8}, {8, 9}, {9, 8}, {9, 7}, {9, 6}, {9, 5}, {8, 4}, {7, 4},
    {6, 5}, {6, 6}, {7, 5}, {8, 5}, {7, 6}, {8, 6}, {7, 7}, {8, 7}, {8, 8},
    {9, 9},
}};

inline int node_size(Node *t) { return t == nullptr ? 0 : t->sz; }

inline long long node_sum_weight(Node *t) {
  return t == nullptr ? 0LL : t->sum_weight;
}

inline long long node_sum_index_weight(Node *t) {
  return t == nullptr ? 0LL : t->sum_index_weight;
}

void toggle_reverse(Node *t) {
  if (t == nullptr) return;
  t->rev = !t->rev;
  swap(t->l, t->r);
  t->sum_index_weight =
      1LL * (t->sz - 1) * t->sum_weight - t->sum_index_weight;
}

void push(Node *t) {
  if (t == nullptr || !t->rev) return;
  toggle_reverse(t->l);
  toggle_reverse(t->r);
  t->rev = false;
}

void pull(Node *t) {
  if (t == nullptr) return;
  if (t->l != nullptr) t->l->p = t;
  if (t->r != nullptr) t->r->p = t;
  const int left_size = node_size(t->l);
  const long long right_sum_weight = node_sum_weight(t->r);
  t->sz = 1 + node_size(t->l) + node_size(t->r);
  t->sum_weight = node_sum_weight(t->l) + t->weight + right_sum_weight;
  t->sum_index_weight = node_sum_index_weight(t->l) + 1LL * left_size * t->weight
      + node_sum_index_weight(t->r)
      + 1LL * (left_size + 1) * right_sum_weight;
}

Node *merge(Node *a, Node *b) {
  if (a == nullptr) {
    if (b != nullptr) b->p = nullptr;
    return b;
  }
  if (b == nullptr) {
    if (a != nullptr) a->p = nullptr;
    return a;
  }
  if (a->priority > b->priority) {
    push(a);
    a->r = merge(a->r, b);
    if (a->r != nullptr) a->r->p = a;
    pull(a);
    a->p = nullptr;
    return a;
  }
  push(b);
  b->l = merge(a, b->l);
  if (b->l != nullptr) b->l->p = b;
  pull(b);
  b->p = nullptr;
  return b;
}

void split(Node *t, int left_count, Node *&a, Node *&b) {
  if (t == nullptr) {
    a = nullptr;
    b = nullptr;
    return;
  }
  push(t);
  if (node_size(t->l) >= left_count) {
    split(t->l, left_count, a, t->l);
    if (t->l != nullptr) t->l->p = t;
    b = t;
    b->p = nullptr;
    pull(b);
    return;
  }
  split(t->r, left_count - node_size(t->l) - 1, t->r, b);
  if (t->r != nullptr) t->r->p = t;
  a = t;
  a->p = nullptr;
  pull(a);
}

int index_of(Node *x) {
  vector<Node *> ancestors;
  for (Node *cur = x; cur != nullptr; cur = cur->p) ancestors.push_back(cur);
  for (int i = (int)ancestors.size() - 1; i >= 0; --i) push(ancestors[i]);

  int idx = node_size(x->l);
  Node *cur = x;
  while (cur->p != nullptr) {
    if (cur == cur->p->r) idx += node_size(cur->p->l) + 1;
    cur = cur->p;
  }
  return idx;
}

void collect_order(Node *t, vector<int> &order) {
  if (t == nullptr) return;
  push(t);
  collect_order(t->l, order);
  order.push_back(t->vid);
  collect_order(t->r, order);
}

struct Solver {
  static constexpr double TIME_LIMIT_MS = 2850.0;
  static constexpr double START_TEMP = 2.0e0;
  static constexpr double END_TEMP = 1.0e0;
  static constexpr int MAX_REVERSE_LEN = 100;
  static constexpr int INITIAL_BLOCK_SIZE = 10;
  static constexpr int OPTIMIZE_TAIL_LEN = 400;

  int N;
  int V;
  vector<int> A;
  vector<int> row_id;
  vector<int> col_id;
  vector<vector<int>> neighbors;
  vector<array<int, 9>> edge_id_dir;
  vector<pair<int, int>> edges;
  vector<Move> moves;
  vector<vector<int>> incident_moves;
  vector<char> edge_used;
  vector<int> active_moves;
  vector<int> active_pos;
  vector<int> order;
  vector<int> pos_of;
  vector<int> path_edge;
  vector<int> edge_pos;
  long long current_objective = 0;
  long long best_objective = 0;
  long long iterations = 0;
  vector<int> best_order;

  static constexpr int DR[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  static constexpr int DC[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

  Solver() { input(); }

  void input() {
    cin >> N;
    V = N * N;
    A.resize(V);
    row_id.resize(V);
    col_id.resize(V);
    for (int r = 0; r < N; ++r) {
      for (int c = 0; c < N; ++c) {
        const int id = vertex_id(r, c);
        cin >> A[id];
        row_id[id] = r;
        col_id[id] = c;
      }
    }
  }

  int vertex_id(int r, int c) const { return r * N + c; }

  bool in_board(int r, int c) const {
    return 0 <= r && r < N && 0 <= c && c < N;
  }

  static int dir_code(int dr, int dc) { return (dr + 1) * 3 + (dc + 1); }

  bool is_adjacent(int u, int v) const {
    if (u == v) return false;
    return abs(row_id[u] - row_id[v]) <= 1 && abs(col_id[u] - col_id[v]) <= 1;
  }

  int find_edge_id(int u, int v) const {
    if (!is_adjacent(u, v)) return -1;
    return edge_id_dir[u][dir_code(row_id[v] - row_id[u], col_id[v] - col_id[u])];
  }

  vector<int> build_plain_snake_order() const {
    vector<int> order;
    order.reserve(V);
    for (int r = 0; r < N; ++r) {
      if ((r & 1) == 0) {
        for (int c = 0; c < N; ++c) order.push_back(vertex_id(r, c));
      } else {
        for (int c = N - 1; c >= 0; --c) order.push_back(vertex_id(r, c));
      }
    }
    return order;
  }

  void append_block_horizontal(vector<int> &initial_order, int base_r,
                               int base_c, bool mirror_h) const {
    for (int c = 0; c < INITIAL_BLOCK_SIZE; ++c) {
      const int cc = mirror_h ? (INITIAL_BLOCK_SIZE - 1 - c) : c;
      if ((c & 1) == 0) {
        for (int r = 0; r < INITIAL_BLOCK_SIZE; ++r) {
          initial_order.push_back(vertex_id(base_r + r, base_c + cc));
        }
      } else {
        for (int r = INITIAL_BLOCK_SIZE - 1; r >= 0; --r) {
          initial_order.push_back(vertex_id(base_r + r, base_c + cc));
        }
      }
    }
  }

  void append_block_drop(vector<int> &initial_order, int base_r, int base_c,
                         bool mirror_h) const {
    for (auto [r, c] : BLOCK_DROP_PATH) {
      const int cc = mirror_h ? (INITIAL_BLOCK_SIZE - 1 - c) : c;
      initial_order.push_back(vertex_id(base_r + r, base_c + cc));
    }
  }

  vector<int> build_initial_order() const {
    if (N < INITIAL_BLOCK_SIZE || N % INITIAL_BLOCK_SIZE != 0) {
      return build_plain_snake_order();
    }

    vector<int> initial_order;
    initial_order.reserve(V);
    const int block_rows = N / INITIAL_BLOCK_SIZE;
    const int block_cols = N / INITIAL_BLOCK_SIZE;

    for (int br = 0; br < block_rows; ++br) {
      const int base_r = br * INITIAL_BLOCK_SIZE;
      if ((br & 1) == 0) {
        for (int bc = 0; bc < block_cols; ++bc) {
          const int base_c = bc * INITIAL_BLOCK_SIZE;
          const bool is_last = (bc + 1 == block_cols);
          if (is_last) {
            append_block_drop(initial_order, base_r, base_c, false);
          } else {
            append_block_horizontal(initial_order, base_r, base_c, false);
          }
        }
      } else {
        for (int bc = block_cols - 1; bc >= 0; --bc) {
          const int base_c = bc * INITIAL_BLOCK_SIZE;
          const bool is_last = (bc == 0);
          if (is_last) {
            append_block_drop(initial_order, base_r, base_c, true);
          } else {
            append_block_horizontal(initial_order, base_r, base_c, true);
          }
        }
      }
    }
    return initial_order;
  }

  void build_board_graph() {
    neighbors.assign(V, {});
    edge_id_dir.assign(V, {});
    for (int v = 0; v < V; ++v) edge_id_dir[v].fill(-1);

    for (int r = 0; r < N; ++r) {
      for (int c = 0; c < N; ++c) {
        const int u = vertex_id(r, c);
        for (int dir = 0; dir < 8; ++dir) {
          const int nr = r + DR[dir];
          const int nc = c + DC[dir];
          if (!in_board(nr, nc)) continue;
          neighbors[u].push_back(vertex_id(nr, nc));
        }
      }
    }

    for (int u = 0; u < V; ++u) {
      for (int v : neighbors[u]) {
        if (u > v) continue;
        const int eid = (int)edges.size();
        edges.push_back({u, v});
        edge_id_dir[u][dir_code(row_id[v] - row_id[u], col_id[v] - col_id[u])] =
            eid;
        edge_id_dir[v][dir_code(row_id[u] - row_id[v], col_id[u] - col_id[v])] =
            eid;
      }
    }
  }

  static pair<int, int> rotate_point(pair<int, int> p, int rot) {
    int r = p.first;
    int c = p.second;
    for (int k = 0; k < rot; ++k) {
      const int nr = c;
      const int nc = -r;
      r = nr;
      c = nc;
    }
    return {r, c};
  }

  static pair<int, int> transform_point(pair<int, int> p, int rot,
                                        bool mirror) {
    auto q = rotate_point(p, rot);
    if (mirror) q.second = -q.second;
    return q;
  }

  static PatternTemplate transform_pattern(const PatternTemplate &base, int rot,
                                           bool mirror) {
    PatternTemplate res = base;
    int min_r = numeric_limits<int>::max();
    int min_c = numeric_limits<int>::max();
    for (int i = 0; i < 4; ++i) {
      res.points[i] = transform_point(base.points[i], rot, mirror);
      min_r = min(min_r, res.points[i].first);
      min_c = min(min_c, res.points[i].second);
    }
    for (int i = 0; i < 4; ++i) {
      res.points[i].first -= min_r;
      res.points[i].second -= min_c;
    }
    return res;
  }

  static string pattern_signature(const PatternTemplate &pattern) {
    string key;
    for (int i = 0; i < 4; ++i) {
      key += to_string(pattern.points[i].first);
      key += ',';
      key += to_string(pattern.points[i].second);
      key += ';';
    }
    for (int i = 0; i < 2; ++i) {
      key += 'o';
      key += char('0' + pattern.old_pairs[i].first);
      key += char('0' + pattern.old_pairs[i].second);
      key += 'n';
      key += char('0' + pattern.new_pairs[i].first);
      key += char('0' + pattern.new_pairs[i].second);
    }
    return key;
  }

  void add_move_instance(int old1, int old2, int new1, int new2,
                         unordered_set<string> &seen_moves) {
    if (old1 == old2 || new1 == new2) return;
    if (old1 > old2) swap(old1, old2);
    if (new1 > new2) swap(new1, new2);
    const string key = to_string(old1) + ":" + to_string(old2) + "->" +
        to_string(new1) + ":" + to_string(new2);
    if (!seen_moves.insert(key).second) return;
    const int mid = (int)moves.size();
    moves.push_back({old1, old2, new1, new2});
    incident_moves[old1].push_back(mid);
    incident_moves[old2].push_back(mid);
  }

  void add_pattern_family(const PatternTemplate &base,
                          unordered_set<string> &seen_patterns,
                          unordered_set<string> &seen_moves) {
    for (int rot = 0; rot < 4; ++rot) {
      for (int mirror = 0; mirror < 2; ++mirror) {
        const PatternTemplate pattern =
            transform_pattern(base, rot, mirror != 0);
        if (!seen_patterns.insert(pattern_signature(pattern)).second) continue;

        int max_r = 0;
        int max_c = 0;
        for (const auto &p : pattern.points) {
          max_r = max(max_r, p.first);
          max_c = max(max_c, p.second);
        }

        for (int br = 0; br + max_r < N; ++br) {
          for (int bc = 0; bc + max_c < N; ++bc) {
            array<int, 4> vids;
            for (int i = 0; i < 4; ++i) {
              vids[i] = vertex_id(br + pattern.points[i].first,
                                  bc + pattern.points[i].second);
            }

            array<int, 2> old_edges;
            array<int, 2> new_edges;
            bool ok = true;
            for (int i = 0; i < 2; ++i) {
              old_edges[i] = find_edge_id(
                  vids[pattern.old_pairs[i].first],
                  vids[pattern.old_pairs[i].second]);
              new_edges[i] = find_edge_id(
                  vids[pattern.new_pairs[i].first],
                  vids[pattern.new_pairs[i].second]);
              if (old_edges[i] < 0 || new_edges[i] < 0) ok = false;
            }
            if (!ok) continue;

            add_move_instance(old_edges[0], old_edges[1], new_edges[0],
                              new_edges[1], seen_moves);
            add_move_instance(new_edges[0], new_edges[1], old_edges[0],
                              old_edges[1], seen_moves);
          }
        }
      }
    }
  }

  void build_moves() {
    incident_moves.assign(edges.size(), {});
    moves.clear();

    unordered_set<string> seen_patterns;
    unordered_set<string> seen_moves;

    add_pattern_family(
        {{{{0, 0}, {0, 1}, {1, 0}, {1, 1}}}, {{{0, 1}, {2, 3}}},
         {{{0, 3}, {1, 2}}}},
        seen_patterns, seen_moves);

    add_pattern_family(
        {{{{0, 0}, {1, 1}, {0, 1}, {1, 2}}}, {{{0, 1}, {2, 3}}},
         {{{0, 2}, {1, 3}}}},
        seen_patterns, seen_moves);

    add_pattern_family(
        {{{{0, 0}, {0, 1}, {0, 2}, {1, 1}}}, {{{0, 1}, {2, 3}}},
         {{{0, 3}, {1, 2}}}},
        seen_patterns, seen_moves);

    add_pattern_family(
        {{{{1, 0}, {0, 1}, {1, 2}, {2, 1}}}, {{{0, 1}, {2, 3}}},
         {{{1, 2}, {0, 3}}}},
        seen_patterns, seen_moves);
  }

  void activate_move(int mid) {
    if (active_pos[mid] != -1) return;
    active_pos[mid] = (int)active_moves.size();
    active_moves.push_back(mid);
  }

  void deactivate_move(int mid) {
    const int pos = active_pos[mid];
    if (pos == -1) return;
    const int back = active_moves.back();
    active_moves[pos] = back;
    active_pos[back] = pos;
    active_moves.pop_back();
    active_pos[mid] = -1;
  }

  int optimize_start_pos() const { return max(0, V - OPTIMIZE_TAIL_LEN); }

  void refresh_move(int mid) {
    const Move &move = moves[mid];
    if (!edge_used[move.old1] || !edge_used[move.old2]) {
      deactivate_move(mid);
      return;
    }

    int i = edge_pos[move.old1];
    int j = edge_pos[move.old2];
    if (i > j) swap(i, j);

    if (j - i > 1 && j - i <= MAX_REVERSE_LEN &&
        i + 1 >= optimize_start_pos()) {
      activate_move(mid);
    } else {
      deactivate_move(mid);
    }
  }

  void refresh_incident(int eid) {
    for (int mid : incident_moves[eid]) refresh_move(mid);
  }

  long long calc_reverse_delta(int l, int r) const {
    long long delta = 0;
    for (int offset = 0; l + offset <= r; ++offset) {
      const int left_pos = l + offset;
      const int right_pos = r - offset;
      if (left_pos >= right_pos) break;
      const int left_vid = order[left_pos];
      const int right_vid = order[right_pos];
      delta += 1LL * left_pos * (A[right_vid] - A[left_vid]);
      delta += 1LL * right_pos * (A[left_vid] - A[right_vid]);
    }
    return delta;
  }

  void apply_reverse_segment(int l, int r) {
    reverse(order.begin() + l, order.begin() + r + 1);
    for (int pos = l; pos <= r; ++pos) pos_of[order[pos]] = pos;
  }

  void build_initial_state(const vector<int> &initial_order) {
    order = initial_order;
    pos_of.assign(V, -1);
    current_objective = 0;
    for (int pos = 0; pos < V; ++pos) {
      const int vid = initial_order[pos];
      current_objective += 1LL * pos * A[vid];
      pos_of[vid] = pos;
    }

    edge_used.assign(edges.size(), false);
    edge_pos.assign(edges.size(), -1);
    path_edge.assign(V - 1, -1);
    for (int i = 0; i + 1 < V; ++i) {
      const int eid = find_edge_id(initial_order[i], initial_order[i + 1]);
      path_edge[i] = eid;
      edge_used[eid] = true;
      edge_pos[eid] = i;
    }

    active_pos.assign(moves.size(), -1);
    active_moves.clear();
    for (int mid = 0; mid < (int)moves.size(); ++mid) refresh_move(mid);

    best_objective = current_objective;
    best_order = this->order;
  }

  long long score_from_objective(long long objective) const {
    return (objective + V / 2) / V;
  }

  bool is_better_objective(long long lhs, long long rhs) const {
    const long long lhs_score = score_from_objective(lhs);
    const long long rhs_score = score_from_objective(rhs);
    if (lhs_score != rhs_score) return lhs_score > rhs_score;
    return lhs > rhs;
  }

  double temperature() const {
    const double progress =
        min(1.0, utility::timer.elapsed_ms() / TIME_LIMIT_MS);
    return exp(log(START_TEMP) * (1.0 - progress) + log(END_TEMP) * progress);
  }

  bool accept(long long delta, double temp) const {
    if (delta >= 0) return true;
    return exp((double)delta / temp) > rand_double();
  }

  bool apply_move(int mid) {
    const Move &move = moves[mid];
    const int e1 = move.old1;
    const int e2 = move.old2;
    const auto [u1, v1] = edges[e1];
    const auto [u2, v2] = edges[e2];

    const int pu1 = pos_of[u1];
    const int pv1 = pos_of[v1];
    const int pu2 = pos_of[u2];
    const int pv2 = pos_of[v2];

    if (abs(pu1 - pv1) != 1 || abs(pu2 - pv2) != 1) return false;

    int i = pu1 < pv1 ? pu1 : pv1;
    int j = pu2 < pv2 ? pu2 : pv2;
    int a = pu1 < pv1 ? u1 : v1;
    int b = pu1 < pv1 ? v1 : u1;
    int c = pu2 < pv2 ? u2 : v2;
    int d = pu2 < pv2 ? v2 : u2;

    if (i > j) {
      swap(i, j);
      swap(a, c);
      swap(b, d);
    }

    if (j <= i + 1) return false;
    if (j - i > MAX_REVERSE_LEN) return false;
    if (i + 1 < optimize_start_pos()) return false;
    if (!is_adjacent(a, c) || !is_adjacent(b, d)) return false;

    const int new_e1 = find_edge_id(a, c);
    const int new_e2 = find_edge_id(b, d);
    if (new_e1 < 0 || new_e2 < 0) return false;

    int actual1 = new_e1;
    int actual2 = new_e2;
    if (actual1 > actual2) swap(actual1, actual2);
    int target1 = move.new1;
    int target2 = move.new2;
    if (target1 > target2) swap(target1, target2);
    if (actual1 != target1 || actual2 != target2) return false;

    const int l = i + 1;
    const int r = j;
    const long long objective_delta = calc_reverse_delta(l, r);
    const double temp = temperature();
    if (!accept(objective_delta, temp)) return false;

    vector<int> affected_edges;
    affected_edges.reserve(2 * MAX_REVERSE_LEN + 8);
    for (int pos = i; pos <= j; ++pos) affected_edges.push_back(path_edge[pos]);

    apply_reverse_segment(l, r);
    current_objective += objective_delta;

    for (int pos = i; pos <= j; ++pos) {
      path_edge[pos] = find_edge_id(order[pos], order[pos + 1]);
      affected_edges.push_back(path_edge[pos]);
    }

    sort(affected_edges.begin(), affected_edges.end());
    affected_edges.erase(unique(affected_edges.begin(), affected_edges.end()),
                         affected_edges.end());

    for (int eid : affected_edges) {
      edge_used[eid] = false;
      edge_pos[eid] = -1;
    }
    for (int pos = i; pos <= j; ++pos) {
      const int eid = path_edge[pos];
      edge_used[eid] = true;
      edge_pos[eid] = pos;
    }
    for (int eid : affected_edges) {
      refresh_incident(eid);
    }

    if (is_better_objective(current_objective, best_objective)) {
      best_objective = current_objective;
      best_order = order;
    }
    return true;
  }

  void solve() {
    build_board_graph();
    build_moves();
    build_initial_state(build_initial_order());

    while (utility::timer.elapsed_ms() < TIME_LIMIT_MS && !active_moves.empty()) {
      ++iterations;
      const int mid = active_moves[rand_int() % active_moves.size()];
      apply_move(mid);
    }
    cerr << "iterations: " << iterations << '\n';
    cerr << "best score: " << score_from_objective(best_objective) << '\n';
    cerr << "best objective: " << best_objective << '\n';
  }

  void output() const {
    for (int vid : best_order) {
      cout << row_id[vid] << ' ' << col_id[vid] << '\n';
    }
  }
};

constexpr int Solver::DR[8];
constexpr int Solver::DC[8];

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  utility::timer.begin();
  Solver solver;
  solver.solve();
  solver.output();
  return 0;
}
