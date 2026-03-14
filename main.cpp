#include <bits/stdc++.h>

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
} // namespace utility

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
  Node *l;
  Node *r;
  Node *p;

  Node(int vid_, long long weight_)
      : vid(vid_), priority((int)rand_int()), sz(1), rev(false),
        weight(weight_), sum_weight(weight_), sum_index_weight(0), l(nullptr),
        r(nullptr), p(nullptr) {}
};

inline int node_size(Node *t) { return t == nullptr ? 0 : t->sz; }

inline long long node_sum_weight(Node *t) {
  return t == nullptr ? 0LL : t->sum_weight;
}

inline long long node_sum_index_weight(Node *t) {
  return t == nullptr ? 0LL : t->sum_index_weight;
}

void toggle_reverse(Node *t) {
  if (t == nullptr)
    return;
  t->rev = !t->rev;
  swap(t->l, t->r);
  t->sum_index_weight = 1LL * (t->sz - 1) * t->sum_weight - t->sum_index_weight;
}

void push(Node *t) {
  if (t == nullptr || !t->rev)
    return;
  toggle_reverse(t->l);
  toggle_reverse(t->r);
  t->rev = false;
}

void pull(Node *t) {
  if (t == nullptr)
    return;
  if (t->l != nullptr)
    t->l->p = t;
  if (t->r != nullptr)
    t->r->p = t;
  const int left_size = node_size(t->l);
  const long long right_sum_weight = node_sum_weight(t->r);
  t->sz = 1 + node_size(t->l) + node_size(t->r);
  t->sum_weight = node_sum_weight(t->l) + t->weight + right_sum_weight;
  t->sum_index_weight =
      node_sum_index_weight(t->l) + 1LL * left_size * t->weight +
      node_sum_index_weight(t->r) + 1LL * (left_size + 1) * right_sum_weight;
}

Node *merge(Node *a, Node *b) {
  if (a == nullptr) {
    if (b != nullptr)
      b->p = nullptr;
    return b;
  }
  if (b == nullptr) {
    if (a != nullptr)
      a->p = nullptr;
    return a;
  }
  if (a->priority > b->priority) {
    push(a);
    a->r = merge(a->r, b);
    if (a->r != nullptr)
      a->r->p = a;
    pull(a);
    a->p = nullptr;
    return a;
  }
  push(b);
  b->l = merge(a, b->l);
  if (b->l != nullptr)
    b->l->p = b;
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
    if (t->l != nullptr)
      t->l->p = t;
    b = t;
    b->p = nullptr;
    pull(b);
    return;
  }
  split(t->r, left_count - node_size(t->l) - 1, t->r, b);
  if (t->r != nullptr)
    t->r->p = t;
  a = t;
  a->p = nullptr;
  pull(a);
}

int index_of(Node *x) {
  vector<Node *> ancestors;
  for (Node *cur = x; cur != nullptr; cur = cur->p)
    ancestors.push_back(cur);
  for (int i = (int)ancestors.size() - 1; i >= 0; --i)
    push(ancestors[i]);

  int idx = node_size(x->l);
  Node *cur = x;
  while (cur->p != nullptr) {
    if (cur == cur->p->r)
      idx += node_size(cur->p->l) + 1;
    cur = cur->p;
  }
  return idx;
}

void collect_order(Node *t, vector<int> &order) {
  if (t == nullptr)
    return;
  push(t);
  collect_order(t->l, order);
  order.push_back(t->vid);
  collect_order(t->r, order);
}

struct Move {
  int old1;
  int old2;
  int new1;
  int new2;
};

struct Solver {
  static constexpr double TIME_LIMIT_MS = 2850.0;
  static constexpr double START_TEMP = 3.0e8;
  static constexpr double END_TEMP = 3.0e6;

  int N = 0;
  int V = 0;
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

  vector<Node *> node_of;
  Node *root = nullptr;

  long long current_objective = 0;
  long long best_objective = LLONG_MIN;
  long long iterations = 0;
  vector<int> best_order;

  static constexpr int DR[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  static constexpr int DC[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

  void input() {
    cin >> N;
    V = N * N;
    A.assign(V, 0);
    row_id.assign(V, 0);
    col_id.assign(V, 0);
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

  pair<int, int> rc(int v) const { return {v / N, v % N}; }

  bool in_board(int r, int c) const {
    return 0 <= r && r < N && 0 <= c && c < N;
  }

  static int dir_code(int dr, int dc) { return (dr + 1) * 3 + (dc + 1); }

  bool is_adjacent(int u, int v) const {
    if (u == v)
      return false;
    return abs(row_id[u] - row_id[v]) <= 1 && abs(col_id[u] - col_id[v]) <= 1;
  }

  int find_edge_id(int u, int v) const {
    if (!is_adjacent(u, v))
      return -1;
    return edge_id_dir[u]
                      [dir_code(row_id[v] - row_id[u], col_id[v] - col_id[u])];
  }

  long long calc_objective(const vector<int> &order) const {
    long long objective = 0;
    for (int i = 0; i < V; ++i)
      objective += 1LL * i * A[order[i]];
    return objective;
  }

  bool is_valid_hamilton_path(const vector<int> &order) const {
    if ((int)order.size() != V)
      return false;
    vector<char> used(V, false);
    for (int i = 0; i < V; ++i) {
      const int v = order[i];
      if (v < 0 || v >= V || used[v])
        return false;
      used[v] = true;
      if (i > 0 && !is_adjacent(order[i - 1], v))
        return false;
    }
    return true;
  }

  vector<int> build_plain_snake_order() const {
    vector<int> order;
    order.reserve(V);
    for (int r = 0; r < N; ++r) {
      if ((r & 1) == 0) {
        for (int c = 0; c < N; ++c)
          order.push_back(vertex_id(r, c));
      } else {
        for (int c = N - 1; c >= 0; --c)
          order.push_back(vertex_id(r, c));
      }
    }
    return order;
  }

  vector<int> build_two_column_round_trip_order(bool start_from_bottom) const {
    if ((N & 1) != 0 || N < 4)
      return build_plain_snake_order();

    vector<int> order;
    order.reserve(V);
    vector<char> used(V, false);
    bool ok = true;
    const int strips = N / 2;

    auto add_cell = [&](int r, int c) {
      if (!ok)
        return;
      if (r < 0 || r >= N || c < 0 || c >= N) {
        ok = false;
        return;
      }
      const int v = vertex_id(r, c);
      if (used[v]) {
        ok = false;
        return;
      }
      if (!order.empty() && !is_adjacent(order.back(), v)) {
        ok = false;
        return;
      }
      used[v] = true;
      order.push_back(v);
    };

    auto choose_outbound_col = [&](int cL, int cR, int r, int forced_col) {
      if (forced_col != -1)
        return forced_col;
      return (A[vertex_id(r, cL)] <= A[vertex_id(r, cR)]) ? cL : cR;
    };

    function<void(int)> from_top;
    function<void(int)> from_bottom;

    from_top = [&](int s) {
      const int cL = 2 * s;
      const int cR = cL + 1;
      const bool is_last = (s + 1 == strips);

      add_cell(0, cL);
      add_cell(0, cR);

      vector<int> inbound_col(N, -1);
      for (int r = 1; r < N; ++r) {
        int forced = -1;
        if (r == 1)
          forced = cR;
        if (!is_last && r == N - 2)
          forced = cL;
        if (r == N - 1)
          forced = cL;
        const int out_col = choose_outbound_col(cL, cR, r, forced);
        inbound_col[r] = (out_col == cL) ? cR : cL;
        add_cell(r, out_col);
      }

      add_cell(N - 1, cR);
      if (!is_last) {
        from_bottom(s + 1);
        add_cell(N - 2, cR);
        for (int r = N - 3; r >= 2; --r)
          add_cell(r, inbound_col[r]);
      } else {
        for (int r = N - 2; r >= 2; --r)
          add_cell(r, inbound_col[r]);
      }
      add_cell(1, cL);
    };

    from_bottom = [&](int s) {
      const int cL = 2 * s;
      const int cR = cL + 1;
      const bool is_last = (s + 1 == strips);

      add_cell(N - 1, cL);
      add_cell(N - 1, cR);

      vector<int> inbound_col(N, -1);
      for (int r = N - 2; r >= 0; --r) {
        int forced = -1;
        if (r == N - 2)
          forced = cR;
        if (!is_last && r == 1)
          forced = cL;
        if (r == 0)
          forced = cL;
        const int out_col = choose_outbound_col(cL, cR, r, forced);
        inbound_col[r] = (out_col == cL) ? cR : cL;
        add_cell(r, out_col);
      }

      add_cell(0, cR);
      if (!is_last) {
        from_top(s + 1);
        add_cell(1, cR);
        for (int r = 2; r <= N - 3; ++r)
          add_cell(r, inbound_col[r]);
      } else {
        for (int r = 1; r <= N - 3; ++r)
          add_cell(r, inbound_col[r]);
      }
      add_cell(N - 2, cL);
    };

    if (start_from_bottom) {
      from_bottom(0);
    } else {
      from_top(0);
    }

    if (!ok || (int)order.size() != V)
      return build_plain_snake_order();
    return order;
  }

  pair<int, int> transform_rc(int r, int c, int rot, bool mirror) const {
    if (mirror)
      c = N - 1 - c;
    for (int k = 0; k < rot; ++k) {
      const int nr = c;
      const int nc = N - 1 - r;
      r = nr;
      c = nc;
    }
    return {r, c};
  }

  vector<int> transform_order(const vector<int> &base, int rot, bool mirror,
                              bool reverse_path) const {
    vector<int> transformed;
    transformed.reserve(V);
    if (!reverse_path) {
      for (int v : base) {
        const auto [r, c] = rc(v);
        const auto [nr, nc] = transform_rc(r, c, rot, mirror);
        transformed.push_back(vertex_id(nr, nc));
      }
    } else {
      for (int i = V - 1; i >= 0; --i) {
        const auto [r, c] = rc(base[i]);
        const auto [nr, nc] = transform_rc(r, c, rot, mirror);
        transformed.push_back(vertex_id(nr, nc));
      }
    }
    return transformed;
  }

  vector<int> build_initial_order() const {
    vector<int> best = build_plain_snake_order();
    long long best_obj = calc_objective(best);

    auto consider = [&](const vector<int> &cand) {
      if (!is_valid_hamilton_path(cand))
        return;
      const long long obj = calc_objective(cand);
      if (obj > best_obj) {
        best_obj = obj;
        best = cand;
      }
    };

    vector<int> b0 = build_two_column_round_trip_order(true);
    vector<int> b1 = build_two_column_round_trip_order(false);
    for (const vector<int> *base_ptr : {&b0, &b1}) {
      const auto &base = *base_ptr;
      for (int rot = 0; rot < 4; ++rot) {
        for (int mirror = 0; mirror < 2; ++mirror) {
          for (int rev = 0; rev < 2; ++rev) {
            consider(transform_order(base, rot, mirror != 0, rev != 0));
          }
        }
      }
    }
    return best;
  }

  void build_board_graph() {
    neighbors.assign(V, {});
    edge_id_dir.assign(V, {});
    for (int v = 0; v < V; ++v)
      edge_id_dir[v].fill(-1);

    for (int r = 0; r < N; ++r) {
      for (int c = 0; c < N; ++c) {
        const int u = vertex_id(r, c);
        for (int dir = 0; dir < 8; ++dir) {
          const int nr = r + DR[dir];
          const int nc = c + DC[dir];
          if (!in_board(nr, nc))
            continue;
          neighbors[u].push_back(vertex_id(nr, nc));
        }
      }
    }

    for (int u = 0; u < V; ++u) {
      for (int v : neighbors[u]) {
        if (u > v)
          continue;
        const int eid = (int)edges.size();
        edges.push_back({u, v});
        edge_id_dir[u][dir_code(row_id[v] - row_id[u], col_id[v] - col_id[u])] =
            eid;
        edge_id_dir[v][dir_code(row_id[u] - row_id[v], col_id[u] - col_id[v])] =
            eid;
      }
    }
  }

  void add_move_instance(int old1, int old2, int new1, int new2,
                         unordered_set<string> &seen) {
    if (old1 < 0 || old2 < 0 || new1 < 0 || new2 < 0)
      return;
    if (old1 == old2 || new1 == new2)
      return;

    if (old1 > old2)
      swap(old1, old2);
    if (new1 > new2)
      swap(new1, new2);

    const string key = to_string(old1) + ":" + to_string(old2) + "->" +
                       to_string(new1) + ":" + to_string(new2);
    if (!seen.insert(key).second)
      return;

    const int mid = (int)moves.size();
    moves.push_back({old1, old2, new1, new2});
    incident_moves[old1].push_back(mid);
    incident_moves[old2].push_back(mid);
  }

  void build_moves() {
    moves.clear();
    incident_moves.assign(edges.size(), {});
    unordered_set<string> seen;

    // Only use 2x2 neighborhoods across strip boundaries: (1,2), (3,4), ...
    for (int c = 1; c + 1 < N; c += 2) {
      for (int r = 0; r + 1 < N; ++r) {
        const int v00 = vertex_id(r, c);
        const int v10 = vertex_id(r + 1, c);
        const int v01 = vertex_id(r, c + 1);
        const int v11 = vertex_id(r + 1, c + 1);

        const int e_v1 = find_edge_id(v00, v10);
        const int e_v2 = find_edge_id(v01, v11);
        const int e_h1 = find_edge_id(v00, v01);
        const int e_h2 = find_edge_id(v10, v11);
        const int e_d1 = find_edge_id(v00, v11);
        const int e_d2 = find_edge_id(v01, v10);

        // Vertical-parallel <-> diagonal cross.
        add_move_instance(e_v1, e_v2, e_d1, e_d2, seen);
        add_move_instance(e_d1, e_d2, e_v1, e_v2, seen);

        // Horizontal-parallel <-> diagonal cross.
        add_move_instance(e_h1, e_h2, e_d1, e_d2, seen);
        add_move_instance(e_d1, e_d2, e_h1, e_h2, seen);
      }
    }
  }

  void activate_move(int mid) {
    if (active_pos[mid] != -1)
      return;
    active_pos[mid] = (int)active_moves.size();
    active_moves.push_back(mid);
  }

  void deactivate_move(int mid) {
    const int pos = active_pos[mid];
    if (pos == -1)
      return;
    const int back = active_moves.back();
    active_moves[pos] = back;
    active_pos[back] = pos;
    active_moves.pop_back();
    active_pos[mid] = -1;
  }

  void refresh_move(int mid) {
    const Move &move = moves[mid];
    if (edge_used[move.old1] && edge_used[move.old2]) {
      activate_move(mid);
    } else {
      deactivate_move(mid);
    }
  }

  void refresh_incident(int eid) {
    for (int mid : incident_moves[eid])
      refresh_move(mid);
  }

  void build_initial_state(const vector<int> &order) {
    node_of.assign(V, nullptr);
    root = nullptr;
    current_objective = 0;

    for (int pos = 0; pos < V; ++pos) {
      const int v = order[pos];
      current_objective += 1LL * pos * A[v];
      Node *node = new Node(v, A[v]);
      node_of[v] = node;
      root = merge(root, node);
    }

    edge_used.assign(edges.size(), false);
    for (int i = 0; i + 1 < V; ++i) {
      const int eid = find_edge_id(order[i], order[i + 1]);
      edge_used[eid] = true;
    }

    active_pos.assign(moves.size(), -1);
    active_moves.clear();
    for (int mid = 0; mid < (int)moves.size(); ++mid)
      refresh_move(mid);

    best_objective = current_objective;
    best_order = order;
  }

  double temperature() const {
    const double progress =
        min(1.0, utility::timer.elapsed_ms() / TIME_LIMIT_MS);
    return exp(log(START_TEMP) * (1.0 - progress) + log(END_TEMP) * progress);
  }

  bool accept(long long delta, double temp) const {
    if (delta >= 0)
      return true;
    return exp((double)delta / temp) > rand_double();
  }

  bool apply_move(int mid) {
    const Move &move = moves[mid];
    const auto [u1, v1] = edges[move.old1];
    const auto [u2, v2] = edges[move.old2];

    int pu1 = index_of(node_of[u1]);
    int pv1 = index_of(node_of[v1]);
    int pu2 = index_of(node_of[u2]);
    int pv2 = index_of(node_of[v2]);

    if (abs(pu1 - pv1) != 1 || abs(pu2 - pv2) != 1)
      return false;

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

    if (j <= i + 1)
      return false;
    if (!is_adjacent(a, c) || !is_adjacent(b, d))
      return false;

    const int new_e1 = find_edge_id(a, c);
    const int new_e2 = find_edge_id(b, d);
    if (new_e1 < 0 || new_e2 < 0)
      return false;

    int actual1 = new_e1;
    int actual2 = new_e2;
    if (actual1 > actual2)
      swap(actual1, actual2);
    int target1 = move.new1;
    int target2 = move.new2;
    if (target1 > target2)
      swap(target1, target2);
    if (actual1 != target1 || actual2 != target2)
      return false;

    Node *left = nullptr;
    Node *mid_seg = nullptr;
    Node *right = nullptr;
    Node *rest = nullptr;
    split(root, i + 1, left, rest);
    split(rest, j - i, mid_seg, right);

    const long long delta =
        1LL * (node_size(mid_seg) - 1) * node_sum_weight(mid_seg) -
        2LL * node_sum_index_weight(mid_seg);
    const double temp = temperature();
    if (!accept(delta, temp)) {
      root = merge(left, mid_seg);
      root = merge(root, right);
      return false;
    }

    toggle_reverse(mid_seg);
    root = merge(left, mid_seg);
    root = merge(root, right);
    current_objective += delta;

    vector<int> changed = {move.old1, move.old2, new_e1, new_e2};
    sort(changed.begin(), changed.end());
    changed.erase(unique(changed.begin(), changed.end()), changed.end());

    for (int eid : changed) {
      const bool should_use = (eid == new_e1 || eid == new_e2);
      if (edge_used[eid] == should_use)
        continue;
      edge_used[eid] = should_use;
      refresh_incident(eid);
    }

    if (current_objective > best_objective) {
      best_objective = current_objective;
      best_order.clear();
      best_order.reserve(V);
      collect_order(root, best_order);
    }
    return true;
  }

  void solve() {
    build_board_graph();
    build_moves();
    build_initial_state(build_initial_order());

    while (utility::timer.elapsed_ms() < TIME_LIMIT_MS &&
           !active_moves.empty()) {
      ++iterations;
      const int mid = active_moves[rand_int() % active_moves.size()];
      apply_move(mid);
    }
  }

  void output() const {
    for (int v : best_order) {
      cout << row_id[v] << ' ' << col_id[v] << '\n';
    }
  }

  void report() const {
    const long long score = (best_objective + V / 2) / V;
    cerr << "mode: sa-2x2-boundary\n";
    cerr << "iterations: " << iterations << '\n';
    cerr << "best score: " << score << '\n';
    cerr << "best objective: " << best_objective << '\n';
    cerr << "moves: " << moves.size() << '\n';
  }
};

constexpr int Solver::DR[8];
constexpr int Solver::DC[8];

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  utility::timer.begin();

  Solver solver;
  solver.input();
  solver.solve();
  solver.report();
  solver.output();
  return 0;
}
