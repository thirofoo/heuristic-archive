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
  static constexpr double START_TEMP = 2.0e7;
  static constexpr double END_TEMP = 1.0e4;

  int N;
  int V;
  vector<int> A;
  vector<int> row_id;
  vector<int> col_id;
  vector<vector<int>> neighbors;
  vector<array<int, 9>> edge_id_dir;
  vector<pair<int, int>> edges;
  vector<pair<int, int>> moves;
  vector<vector<int>> incident_moves;
  vector<char> edge_used;
  vector<int> active_moves;
  vector<int> active_pos;
  vector<Node *> node_of;
  Node *root = nullptr;
  long long current_score = 0;
  long long best_score = 0;
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

  vector<int> build_initial_order() const {
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

  void build_moves() {
    incident_moves.assign(edges.size(), {});
    vector<int> partners;
    for (int eid = 0; eid < (int)edges.size(); ++eid) {
      partners.clear();
      const auto [u, v] = edges[eid];
      for (int orientation = 0; orientation < 2; ++orientation) {
        const int a = orientation == 0 ? u : v;
        const int b = orientation == 0 ? v : u;
        for (int c : neighbors[a]) {
          if (c == a || c == b) continue;
          for (int d : neighbors[b]) {
            if (d == a || d == b || d == c) continue;
            const int fid = find_edge_id(c, d);
            if (fid < 0 || fid == eid) continue;
            partners.push_back(fid);
          }
        }
      }
      sort(partners.begin(), partners.end());
      partners.erase(unique(partners.begin(), partners.end()), partners.end());
      for (int fid : partners) {
        if (eid > fid) continue;
        const int mid = (int)moves.size();
        moves.push_back({eid, fid});
        incident_moves[eid].push_back(mid);
        incident_moves[fid].push_back(mid);
      }
    }
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

  void refresh_move(int mid) {
    const auto [e1, e2] = moves[mid];
    if (edge_used[e1] && edge_used[e2]) {
      activate_move(mid);
    } else {
      deactivate_move(mid);
    }
  }

  void refresh_incident(int eid) {
    for (int mid : incident_moves[eid]) refresh_move(mid);
  }

  void build_initial_state(const vector<int> &order) {
    node_of.assign(V, nullptr);
    root = nullptr;
    current_score = 0;
    for (int pos = 0; pos < V; ++pos) {
      const int vid = order[pos];
      current_score += 1LL * pos * A[vid];
      Node *node = new Node(vid, A[vid]);
      node_of[vid] = node;
      root = merge(root, node);
    }

    edge_used.assign(edges.size(), false);
    for (int i = 0; i + 1 < V; ++i) {
      const int eid = find_edge_id(order[i], order[i + 1]);
      edge_used[eid] = true;
    }

    active_pos.assign(moves.size(), -1);
    active_moves.clear();
    for (int mid = 0; mid < (int)moves.size(); ++mid) refresh_move(mid);

    best_score = current_score;
    best_order = order;
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
    const auto [e1, e2] = moves[mid];
    const auto [u1, v1] = edges[e1];
    const auto [u2, v2] = edges[e2];

    int pu1 = index_of(node_of[u1]);
    int pv1 = index_of(node_of[v1]);
    int pu2 = index_of(node_of[u2]);
    int pv2 = index_of(node_of[v2]);

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
    if (!is_adjacent(a, c) || !is_adjacent(b, d)) return false;

    const int new_e1 = find_edge_id(a, c);
    const int new_e2 = find_edge_id(b, d);
    if (new_e1 < 0 || new_e2 < 0) return false;

    Node *left = nullptr;
    Node *mid_seg = nullptr;
    Node *right = nullptr;
    Node *rest = nullptr;
    split(root, i + 1, left, rest);
    split(rest, j - i, mid_seg, right);

    const long long delta = 1LL * (node_size(mid_seg) - 1) *
            node_sum_weight(mid_seg) -
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
    current_score += delta;

    vector<int> changed = {e1, e2, new_e1, new_e2};
    sort(changed.begin(), changed.end());
    changed.erase(unique(changed.begin(), changed.end()), changed.end());

    for (int eid : changed) {
      const bool should_use = (eid == new_e1 || eid == new_e2);
      if (edge_used[eid] == should_use) continue;
      edge_used[eid] = should_use;
      refresh_incident(eid);
    }

    if (current_score > best_score) {
      best_score = current_score;
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

    while (utility::timer.elapsed_ms() < TIME_LIMIT_MS && !active_moves.empty()) {
      const int mid = active_moves[rand_int() % active_moves.size()];
      apply_move(mid);
    }
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
