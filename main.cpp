#include <bits/stdc++.h>
using namespace std;

struct Pos {
    int r;
    int c;
};

inline int dist(const Pos &a, const Pos &b) {
    return abs(a.r - b.r) + abs(a.c - b.c);
}

struct Timer {
    using clk = chrono::steady_clock;
    clk::time_point st;
    Timer() : st(clk::now()) {}
    double sec() const {
        return chrono::duration<double>(clk::now() - st).count();
    }
};

struct XorShift64 {
    uint64_t x = 88172645463393265ull;
    uint64_t next_u64() {
        x ^= x << 7;
        x ^= x >> 9;
        return x;
    }
    int next_int(int lo, int hi) {
        return lo + static_cast<int>(next_u64() % (uint64_t)(hi - lo + 1));
    }
    double next_double() {
        return (next_u64() >> 11) * (1.0 / 9007199254740992.0);
    }
};

struct Tree {
    int M;
    int root;
    vector<int> parent;
    vector<vector<int>> children;
};

enum MoveType {
    MOVE_SWAP = 0,
    MOVE_REPARENT = 1,
    MOVE_LABEL_SWAP = 2
};

struct Move {
    int type = -1;
    int p = -1;
    int i = -1;
    int j = -1;
    int v = -1;
    int old_parent = -1;
    int old_index = -1;
    int new_parent = -1;
    int new_index = -1;
};

static vector<Pos> center2;
static Pos root_center2;

struct SegNode {
    Pos entry[2];
    Pos exit[2];
    long long cost[2][2];
};

static void fill_leaf_node(SegNode &node, int child, const vector<array<Pos, 2>> &pos,
                           const vector<array<long long, 2>> &cost) {
    node.entry[0] = pos[child][0];
    node.entry[1] = pos[child][1];
    node.exit[0] = pos[child][1];
    node.exit[1] = pos[child][0];
    const long long INF = (1LL << 60);
    node.cost[0][0] = cost[child][0];
    node.cost[1][1] = cost[child][1];
    node.cost[0][1] = INF;
    node.cost[1][0] = INF;
}

static SegNode merge_nodes(const SegNode &a, const SegNode &b) {
    SegNode res;
    res.entry[0] = a.entry[0];
    res.entry[1] = a.entry[1];
    res.exit[0] = b.exit[0];
    res.exit[1] = b.exit[1];
    const long long INF = (1LL << 60);
    for (int i = 0; i < 2; ++i) {
        for (int k = 0; k < 2; ++k) {
            long long best = INF;
            for (int j = 0; j < 2; ++j) {
                for (int l = 0; l < 2; ++l) {
                    long long cand = a.cost[i][j] + dist(a.exit[j], b.entry[l]) + b.cost[l][k];
                    if (cand < best) best = cand;
                }
            }
            res.cost[i][k] = best;
        }
    }
    return res;
}

struct RootSegTree {
    int n = 0;
    vector<SegNode> seg;
    vector<int> order;
    vector<int> index_of;

    void build(const vector<int> &children, const vector<array<Pos, 2>> &pos,
               const vector<array<long long, 2>> &cost) {
        order = children;
        n = (int)order.size();
        seg.assign(n ? 4 * n : 1, SegNode());
        index_of.assign((int)cost.size(), -1);
        for (int i = 0; i < n; ++i) index_of[order[i]] = i;
        if (n == 0) return;
        build_rec(1, 0, n - 1, pos, cost);
    }

    void build_rec(int idx, int l, int r, const vector<array<Pos, 2>> &pos,
                   const vector<array<long long, 2>> &cost) {
        if (l == r) {
            fill_leaf_node(seg[idx], order[l], pos, cost);
            return;
        }
        int mid = (l + r) / 2;
        build_rec(idx * 2, l, mid, pos, cost);
        build_rec(idx * 2 + 1, mid + 1, r, pos, cost);
        seg[idx] = merge_nodes(seg[idx * 2], seg[idx * 2 + 1]);
    }

    void update_child(int child, const vector<array<Pos, 2>> &pos,
                      const vector<array<long long, 2>> &cost) {
        if (n == 0) return;
        int idx = (child >= 0 && child < (int)index_of.size()) ? index_of[child] : -1;
        if (idx < 0) return;
        update_rec(1, 0, n - 1, idx, pos, cost);
    }

    void update_rec(int node, int l, int r, int idx, const vector<array<Pos, 2>> &pos,
                    const vector<array<long long, 2>> &cost) {
        if (l == r) {
            fill_leaf_node(seg[node], order[l], pos, cost);
            return;
        }
        int mid = (l + r) / 2;
        if (idx <= mid) update_rec(node * 2, l, mid, idx, pos, cost);
        else update_rec(node * 2 + 1, mid + 1, r, idx, pos, cost);
        seg[node] = merge_nodes(seg[node * 2], seg[node * 2 + 1]);
    }

    long long total_cost() const {
        if (n == 0) return 0;
        const SegNode &root = seg[1];
        const long long INF = (1LL << 60);
        Pos start{0, 0};
        long long best = INF;
        for (int s = 0; s < 2; ++s) {
            for (int t = 0; t < 2; ++t) {
                long long cand = dist(start, root.entry[s]) + root.cost[s][t];
                if (cand < best) best = cand;
            }
        }
        return best;
    }
};

static vector<int> build_greedy_order(int M, const vector<array<Pos, 2>> &pos) {
    vector<int> order;
    order.reserve(M);
    vector<char> used(M, 0);
    Pos cur{0, 0};

    for (int step = 0; step < M; ++step) {
        int best = -1;
        int best_cost = INT_MAX;
        int best_end = 0;
        for (int i = 0; i < M; ++i) {
            if (used[i]) continue;
            Pos a = pos[i][0];
            Pos b = pos[i][1];
            int c0 = dist(cur, a) + dist(a, b); // end at b
            int c1 = dist(cur, b) + dist(b, a); // end at a
            int c = min(c0, c1);
            if (c < best_cost) {
                best_cost = c;
                best = i;
                best_end = (c0 <= c1) ? 1 : 0;
            }
        }
        if (best == -1) break;
        order.push_back(best);
        used[best] = 1;
        cur = (best_end == 1) ? pos[best][1] : pos[best][0];
    }

    for (int i = 0; i < M; ++i) {
        if (!used[i]) order.push_back(i);
    }
    return order;
}

static Tree build_initial_tree(int M, const vector<array<Pos, 2>> &pos) {
    Tree tr;
    tr.M = M;
    tr.root = M;
    tr.parent.assign(M, tr.root);
    tr.children.assign(M + 1, {});

    vector<int> order = build_greedy_order(M, pos);
    tr.children[tr.root] = order;
    return tr;
}

static long long evaluate_tree(const Tree &tr, const vector<array<Pos, 2>> &pos,
                               vector<array<long long, 2>> &cost) {
    int M = tr.M;
    int root = tr.root;
    cost.assign(M, {0LL, 0LL});

    vector<int> post;
    post.reserve(M);
    function<void(int)> dfs = [&](int v) {
        for (int u : tr.children[v]) dfs(u);
        if (v != root) post.push_back(v);
    };
    dfs(root);

    const long long INF = (1LL << 60);
    for (int v : post) {
        const auto &ch = tr.children[v];
        for (int s = 0; s < 2; ++s) {
            Pos open = pos[v][s];
            Pos close = pos[v][1 - s];
            if (ch.empty()) {
                cost[v][s] = dist(open, close);
                continue;
            }
            long long dp_prev[2] = {INF, INF};
            long long dp_cur[2] = {INF, INF};
            Pos exit_prev[2];
            for (size_t i = 0; i < ch.size(); ++i) {
                int u = ch[i];
                for (int cs = 0; cs < 2; ++cs) {
                    Pos entry = pos[u][cs];
                    long long base;
                    if (i == 0) {
                        base = dist(open, entry);
                    } else {
                        long long c0 = dp_prev[0] + dist(exit_prev[0], entry);
                        long long c1 = dp_prev[1] + dist(exit_prev[1], entry);
                        base = min(c0, c1);
                    }
                    dp_cur[cs] = base + cost[u][cs];
                }
                dp_prev[0] = dp_cur[0];
                dp_prev[1] = dp_cur[1];
                exit_prev[0] = pos[u][1];
                exit_prev[1] = pos[u][0];
            }
            long long total0 = dp_prev[0] + dist(exit_prev[0], close);
            long long total1 = dp_prev[1] + dist(exit_prev[1], close);
            cost[v][s] = min(total0, total1);
        }
    }

    const auto &rch = tr.children[root];
    if (rch.empty()) return 0;

    long long dp_prev[2] = {INF, INF};
    long long dp_cur[2] = {INF, INF};
    Pos exit_prev[2];
    Pos start{0, 0};
    for (size_t i = 0; i < rch.size(); ++i) {
        int u = rch[i];
        for (int cs = 0; cs < 2; ++cs) {
            Pos entry = pos[u][cs];
            long long base;
            if (i == 0) {
                base = dist(start, entry);
            } else {
                long long c0 = dp_prev[0] + dist(exit_prev[0], entry);
                long long c1 = dp_prev[1] + dist(exit_prev[1], entry);
                base = min(c0, c1);
            }
            dp_cur[cs] = base + cost[u][cs];
        }
        dp_prev[0] = dp_cur[0];
        dp_prev[1] = dp_cur[1];
        exit_prev[0] = pos[u][1];
        exit_prev[1] = pos[u][0];
    }
    return min(dp_prev[0], dp_prev[1]);
}

static void recompute_node(const Tree &tr, const vector<array<Pos, 2>> &pos,
                           vector<array<long long, 2>> &cost, int v) {
    const long long INF = (1LL << 60);
    const auto &ch = tr.children[v];
    for (int s = 0; s < 2; ++s) {
        Pos open = pos[v][s];
        Pos close = pos[v][1 - s];
        if (ch.empty()) {
            cost[v][s] = dist(open, close);
            continue;
        }
        long long dp_prev[2] = {INF, INF};
        long long dp_cur[2] = {INF, INF};
        Pos exit_prev[2];
        for (size_t i = 0; i < ch.size(); ++i) {
            int u = ch[i];
            for (int cs = 0; cs < 2; ++cs) {
                Pos entry = pos[u][cs];
                long long base;
                if (i == 0) {
                    base = dist(open, entry);
                } else {
                    long long c0 = dp_prev[0] + dist(exit_prev[0], entry);
                    long long c1 = dp_prev[1] + dist(exit_prev[1], entry);
                    base = min(c0, c1);
                }
                dp_cur[cs] = base + cost[u][cs];
            }
            dp_prev[0] = dp_cur[0];
            dp_prev[1] = dp_cur[1];
            exit_prev[0] = pos[u][1];
            exit_prev[1] = pos[u][0];
        }
        long long total0 = dp_prev[0] + dist(exit_prev[0], close);
        long long total1 = dp_prev[1] + dist(exit_prev[1], close);
        cost[v][s] = min(total0, total1);
    }
}

static int recompute_path(const Tree &tr, const vector<array<Pos, 2>> &pos,
                          vector<array<long long, 2>> &cost, int v) {
    int last = -1;
    while (v != tr.root) {
        last = v;
        recompute_node(tr, pos, cost, v);
        v = tr.parent[v];
    }
    return last;
}

static int apply_depth_delta(const Tree &tr, vector<int> &depth, int v, int delta) {
    if (delta == 0) return 0;
    int cnt = 0;
    vector<int> stack;
    stack.push_back(v);
    while (!stack.empty()) {
        int u = stack.back();
        stack.pop_back();
        depth[u] += delta;
        ++cnt;
        for (int c : tr.children[u]) stack.push_back(c);
    }
    return cnt;
}

static bool is_descendant(const Tree &tr, int node, int potential_parent) {
    int cur = potential_parent;
    while (cur != tr.root) {
        if (cur == node) return true;
        cur = tr.parent[cur];
    }
    return false;
}

static bool apply_random_move(Tree &tr, XorShift64 &rng, Move &mv,
                              vector<array<Pos, 2>> &pos) {
    int M = tr.M;
    int roll = rng.next_int(0, 99);
    const int LABEL_SWAP_RATE = 80;

    if (roll < 40) { // swap siblings or labels
        if (rng.next_int(0, 99) < LABEL_SWAP_RATE) {
            for (int tries = 0; tries < 8; ++tries) {
                int a = rng.next_int(0, M - 1);
                int b = rng.next_int(0, M - 1);
                if (a == b) continue;
                swap(pos[a], pos[b]);
                swap(center2[a], center2[b]);
                mv.type = MOVE_LABEL_SWAP;
                mv.p = a;
                mv.v = b;
                return true;
            }
        }
        for (int tries = 0; tries < 8; ++tries) {
            int p = rng.next_int(0, M); // include root
            if (tr.children[p].size() < 2) continue;
            int n = (int)tr.children[p].size();
            int i = rng.next_int(0, n - 1);
            int j = rng.next_int(0, n - 1);
            if (i == j) continue;
            swap(tr.children[p][i], tr.children[p][j]);
            mv.type = MOVE_SWAP;
            mv.p = p;
            mv.i = i;
            mv.j = j;
            return true;
        }
        return false;
    }

    // move subtree
    int v = rng.next_int(0, M - 1);
    int old_parent = tr.parent[v];
    int best_parent = -1;
    int best_dist = INT_MAX;
    for (int tries = 0; tries < 12; ++tries) {
        int cand = rng.next_int(0, M);
        if (cand == v || cand == old_parent) continue;
        if (is_descendant(tr, v, cand)) continue;
        Pos ca = center2[v];
        Pos cb = (cand == tr.root) ? root_center2 : center2[cand];
        int d = abs(ca.r - cb.r) + abs(ca.c - cb.c);
        if (d < best_dist) {
            best_dist = d;
            best_parent = cand;
        }
    }
    if (best_parent == -1) return false;
    int new_parent = best_parent;
    auto &old_ch = tr.children[old_parent];
    int idx = -1;
    for (int i = 0; i < (int)old_ch.size(); ++i) {
        if (old_ch[i] == v) {
            idx = i;
            break;
        }
    }
    if (idx == -1) return false;

    old_ch.erase(old_ch.begin() + idx);
    auto &new_ch = tr.children[new_parent];
    int ins = rng.next_int(0, (int)new_ch.size());
    new_ch.insert(new_ch.begin() + ins, v);
    tr.parent[v] = new_parent;
    mv.type = MOVE_REPARENT;
    mv.v = v;
    mv.old_parent = old_parent;
    mv.old_index = idx;
    mv.new_parent = new_parent;
    mv.new_index = ins;
    return true;
}

static void undo_move(Tree &tr, const Move &mv, vector<array<Pos, 2>> &pos) {
    if (mv.type == MOVE_SWAP) {
        swap(tr.children[mv.p][mv.i], tr.children[mv.p][mv.j]);
        return;
    }
    if (mv.type == MOVE_REPARENT) {
        auto &new_ch = tr.children[mv.new_parent];
        new_ch.erase(new_ch.begin() + mv.new_index);
        auto &old_ch = tr.children[mv.old_parent];
        old_ch.insert(old_ch.begin() + mv.old_index, mv.v);
        tr.parent[mv.v] = mv.old_parent;
        return;
    }
    if (mv.type == MOVE_LABEL_SWAP) {
        swap(pos[mv.p], pos[mv.v]);
        swap(center2[mv.p], center2[mv.v]);
    }
}

struct NodeTrace {
    vector<array<int, 2>> prev_state[2];
    int last_state[2];
};

struct RootTrace {
    vector<array<int, 2>> prev_state;
    int last_state;
};

static long long compute_trace(const Tree &tr, const vector<array<Pos, 2>> &pos,
                               vector<array<long long, 2>> &cost,
                               vector<NodeTrace> &trace,
                               RootTrace &rtrace) {
    int M = tr.M;
    int root = tr.root;
    cost.assign(M, {0LL, 0LL});
    trace.assign(M, NodeTrace());

    vector<int> post;
    post.reserve(M);
    function<void(int)> dfs = [&](int v) {
        for (int u : tr.children[v]) dfs(u);
        if (v != root) post.push_back(v);
    };
    dfs(root);

    const long long INF = (1LL << 60);
    for (int v : post) {
        const auto &ch = tr.children[v];
        for (int s = 0; s < 2; ++s) {
            Pos open = pos[v][s];
            Pos close = pos[v][1 - s];
            trace[v].prev_state[s].clear();
            trace[v].prev_state[s].resize(ch.size());
            if (ch.empty()) {
                cost[v][s] = dist(open, close);
                trace[v].last_state[s] = -1;
                continue;
            }
            long long dp_prev[2] = {INF, INF};
            long long dp_cur[2] = {INF, INF};
            Pos exit_prev[2];
            for (size_t i = 0; i < ch.size(); ++i) {
                int u = ch[i];
                for (int cs = 0; cs < 2; ++cs) {
                    Pos entry = pos[u][cs];
                    long long base;
                    int arg = -1;
                    if (i == 0) {
                        base = dist(open, entry);
                    } else {
                        long long c0 = dp_prev[0] + dist(exit_prev[0], entry);
                        long long c1 = dp_prev[1] + dist(exit_prev[1], entry);
                        if (c0 <= c1) {
                            base = c0;
                            arg = 0;
                        } else {
                            base = c1;
                            arg = 1;
                        }
                    }
                    dp_cur[cs] = base + cost[u][cs];
                    trace[v].prev_state[s][i][cs] = arg;
                }
                dp_prev[0] = dp_cur[0];
                dp_prev[1] = dp_cur[1];
                exit_prev[0] = pos[u][1];
                exit_prev[1] = pos[u][0];
            }
            long long total0 = dp_prev[0] + dist(exit_prev[0], close);
            long long total1 = dp_prev[1] + dist(exit_prev[1], close);
            if (total0 <= total1) {
                cost[v][s] = total0;
                trace[v].last_state[s] = 0;
            } else {
                cost[v][s] = total1;
                trace[v].last_state[s] = 1;
            }
        }
    }

    const auto &rch = tr.children[root];
    rtrace.prev_state.clear();
    rtrace.prev_state.resize(rch.size());
    if (rch.empty()) {
        rtrace.last_state = -1;
        return 0;
    }

    long long dp_prev[2] = {INF, INF};
    long long dp_cur[2] = {INF, INF};
    Pos exit_prev[2];
    Pos start{0, 0};
    for (size_t i = 0; i < rch.size(); ++i) {
        int u = rch[i];
        for (int cs = 0; cs < 2; ++cs) {
            Pos entry = pos[u][cs];
            long long base;
            int arg = -1;
            if (i == 0) {
                base = dist(start, entry);
            } else {
                long long c0 = dp_prev[0] + dist(exit_prev[0], entry);
                long long c1 = dp_prev[1] + dist(exit_prev[1], entry);
                if (c0 <= c1) {
                    base = c0;
                    arg = 0;
                } else {
                    base = c1;
                    arg = 1;
                }
            }
            dp_cur[cs] = base + cost[u][cs];
            rtrace.prev_state[i][cs] = arg;
        }
        dp_prev[0] = dp_cur[0];
        dp_prev[1] = dp_cur[1];
        exit_prev[0] = pos[u][1];
        exit_prev[1] = pos[u][0];
    }
    if (dp_prev[0] <= dp_prev[1]) {
        rtrace.last_state = 0;
        return dp_prev[0];
    }
    rtrace.last_state = 1;
    return dp_prev[1];
}

static void move_to(Pos &cur, const Pos &target, string &ops) {
    while (cur.r < target.r) {
        ops.push_back('D');
        cur.r++;
    }
    while (cur.r > target.r) {
        ops.push_back('U');
        cur.r--;
    }
    while (cur.c < target.c) {
        ops.push_back('R');
        cur.c++;
    }
    while (cur.c > target.c) {
        ops.push_back('L');
        cur.c--;
    }
}

static string build_solution(const Tree &tr, const vector<array<Pos, 2>> &pos) {
    int M = tr.M;
    int root = tr.root;

    vector<array<long long, 2>> cost;
    vector<NodeTrace> trace;
    RootTrace rtrace;
    compute_trace(tr, pos, cost, trace, rtrace);

    vector<int> orient(M, 0);
    const auto &rch = tr.children[root];
    if (!rch.empty()) {
        int state = rtrace.last_state;
        for (int i = (int)rch.size() - 1; i >= 0; --i) {
            int child = rch[i];
            orient[child] = state;
            int prev = rtrace.prev_state[i][state];
            state = prev;
        }

        function<void(int, int)> assign_orient = [&](int v, int s) {
            const auto &ch = tr.children[v];
            if (ch.empty()) return;
            int k = (int)ch.size();
            int st = trace[v].last_state[s];
            for (int i = k - 1; i >= 0; --i) {
                int child = ch[i];
                orient[child] = st;
                int prev = trace[v].prev_state[s][i][st];
                st = prev;
            }
            for (int child : ch) assign_orient(child, orient[child]);
        };

        for (int child : rch) assign_orient(child, orient[child]);
    }

    string ops;
    ops.reserve(10000);
    Pos cur{0, 0};

    function<void(int)> dfs_emit = [&](int v) {
        int s = orient[v];
        Pos open = pos[v][s];
        move_to(cur, open, ops);
        ops.push_back('Z');
        for (int u : tr.children[v]) dfs_emit(u);
        Pos close = pos[v][1 - s];
        move_to(cur, close, ops);
        ops.push_back('Z');
    };

    for (int child : rch) dfs_emit(child);
    return ops;
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    bool log_stats = (argc > 1);

    int N;
    if (!(cin >> N)) return 0;
    int M = (N * N) / 2;

    vector<array<Pos, 2>> pos(M);
    vector<int> cnt(M, 0);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            int v;
            cin >> v;
            if (v < 0 || v >= M) continue;
            int k = cnt[v]++;
            if (k < 2) pos[v][k] = {r, c};
        }
    }
    center2.assign(M, {});
    for (int i = 0; i < M; ++i) {
        center2[i] = {pos[i][0].r + pos[i][1].r, pos[i][0].c + pos[i][1].c};
    }
    root_center2 = {N - 1, N - 1};

    Tree cur = build_initial_tree(M, pos);
    Tree best = cur;
    vector<array<Pos, 2>> best_pos = pos;

    vector<array<long long, 2>> cost;
    long long cur_cost = evaluate_tree(cur, pos, cost);
    long long best_cost = cur_cost;
    RootSegTree root_seg;
    root_seg.build(cur.children[cur.root], pos, cost);
    vector<int> depth(M, 0);
    long long sum_depth = 0;
    {
        vector<int> stack;
        for (int child : cur.children[cur.root]) {
            depth[child] = 1;
            stack.push_back(child);
        }
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            sum_depth += depth[v];
            for (int c : cur.children[v]) {
                depth[c] = depth[v] + 1;
                stack.push_back(c);
            }
        }
    }

    Timer timer;
    XorShift64 rng;
    const double TIME_LIMIT = 1.95;
    const double T0 = 10.0;
    const double T1 = 0.05;
    const double DEPTH_WEIGHT = 0.05;
    double cur_obj = cur_cost + DEPTH_WEIGHT * (double)sum_depth;

    long long total_iters = 0;
    long long valid_moves = 0;
    long long accepted_moves = 0;
    long long improved_moves = 0;
    array<long long, 3> type_valid{};
    array<long long, 3> type_accepted{};
    array<long long, 3> type_improved{};
    while (timer.sec() < TIME_LIMIT) {
        ++total_iters;
        Move mv;
        if (!apply_random_move(cur, rng, mv, pos)) continue;
        ++valid_moves;
        if (mv.type >= 0 && mv.type < 3) {
            ++type_valid[mv.type];
        }
        int p1 = -1;
        int p2 = -1;
        if (mv.type == MOVE_REPARENT) {
            p1 = mv.old_parent;
            p2 = mv.new_parent;
        } else if (mv.type == MOVE_LABEL_SWAP) {
            p1 = mv.p;
            p2 = mv.v;
        } else {
            p1 = mv.p;
        }
        bool root_changed = false;
        if (mv.type == MOVE_SWAP) {
            root_changed = (mv.p == cur.root);
        } else if (mv.type == MOVE_REPARENT) {
            root_changed = (mv.old_parent == cur.root || mv.new_parent == cur.root);
        }

        int top1 = -1;
        int top2 = -1;
        if (p1 != -1) top1 = recompute_path(cur, pos, cost, p1);
        if (p2 != -1 && p2 != p1) top2 = recompute_path(cur, pos, cost, p2);

        if (root_changed) {
            root_seg.build(cur.children[cur.root], pos, cost);
        } else {
            if (top1 != -1) root_seg.update_child(top1, pos, cost);
            if (top2 != -1 && top2 != top1) root_seg.update_child(top2, pos, cost);
        }
        long long cand_cost = root_seg.total_cost();
        if (mv.type == MOVE_REPARENT) {
            int new_depth = depth[mv.new_parent] + 1;
            int delta = new_depth - depth[mv.v];
            if (delta != 0) {
                int cnt = apply_depth_delta(cur, depth, mv.v, delta);
                sum_depth += 1LL * delta * cnt;
            }
        }
        double cand_obj = cand_cost + DEPTH_WEIGHT * (double)sum_depth;

        bool accept = false;
        if (cand_obj <= cur_obj) {
            accept = true;
        } else {
            double t = timer.sec() / TIME_LIMIT;
            double temp = T0 * pow(T1 / T0, t);
            double prob = exp((cur_obj - cand_obj) / temp);
            if (rng.next_double() < prob) accept = true;
        }

        if (accept) {
            // cerr << "Iter " << iter << ": Cost " << cand_cost << ", Best Cost " << best_cost << "\n";
            // cerr << "Best Update? : " << (cand_cost < best_cost ? "Yes" : "No") << "\n";
            cur_cost = cand_cost;
            cur_obj = cand_obj;
            ++accepted_moves;
            if (mv.type >= 0 && mv.type < 3) {
                ++type_accepted[mv.type];
            }
            if (cur_cost < best_cost) {
                best_cost = cur_cost;
                best = cur;
                best_pos = pos;
                ++improved_moves;
                if (mv.type >= 0 && mv.type < 3) {
                    ++type_improved[mv.type];
                }
            }
        } else {
            undo_move(cur, mv, pos);
            top1 = -1;
            top2 = -1;
            if (p1 != -1) top1 = recompute_path(cur, pos, cost, p1);
            if (p2 != -1 && p2 != p1) top2 = recompute_path(cur, pos, cost, p2);
            if (root_changed) {
                root_seg.build(cur.children[cur.root], pos, cost);
            } else {
                if (top1 != -1) root_seg.update_child(top1, pos, cost);
                if (top2 != -1 && top2 != top1) root_seg.update_child(top2, pos, cost);
            }
            cur_cost = root_seg.total_cost();
            if (mv.type == MOVE_REPARENT) {
                int target_depth = depth[mv.old_parent] + 1;
                int delta = target_depth - depth[mv.v];
                if (delta != 0) {
                    int cnt = apply_depth_delta(cur, depth, mv.v, delta);
                    sum_depth += 1LL * delta * cnt;
                }
            }
            cur_obj = cur_cost + DEPTH_WEIGHT * (double)sum_depth;
        }
    }

    if (log_stats) {
        auto rate = [](long long a, long long b) -> double {
            if (b == 0) return 0.0;
            return 100.0 * (double)a / (double)b;
        };
        cerr << "iters=" << total_iters << " valid=" << valid_moves
             << " accepted=" << accepted_moves << " improved=" << improved_moves << "\n";
        cerr << "accept_rate=" << fixed << setprecision(2) << rate(accepted_moves, valid_moves) << "% "
             << "improve_rate=" << rate(improved_moves, valid_moves) << "%\n";
        const char *names[3] = {"swap", "reparent", "lswap"};
        for (int t = 0; t < 3; ++t) {
            cerr << names[t] << ": valid=" << type_valid[t]
                 << " accepted=" << type_accepted[t]
                 << " improved=" << type_improved[t] << "\n";
        }
    }

    string ops = build_solution(best, best_pos);
    for (char ch : ops) {
        cout << ch << '\n';
    }
    return 0;
}
