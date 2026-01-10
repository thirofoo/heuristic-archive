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

static bool is_descendant(const Tree &tr, int node, int potential_parent) {
    int cur = potential_parent;
    while (cur != tr.root) {
        if (cur == node) return true;
        cur = tr.parent[cur];
    }
    return false;
}

static bool apply_random_move(Tree &tr, XorShift64 &rng) {
    int M = tr.M;
    int root = tr.root;
    int move_type = rng.next_int(0, 2);

    if (move_type == 0) { // swap siblings
        for (int tries = 0; tries < 8; ++tries) {
            int p = rng.next_int(0, M); // include root
            if (tr.children[p].size() < 2) continue;
            int n = (int)tr.children[p].size();
            int i = rng.next_int(0, n - 1);
            int j = rng.next_int(0, n - 1);
            if (i == j) continue;
            swap(tr.children[p][i], tr.children[p][j]);
            return true;
        }
        return false;
    }

    if (move_type == 1) { // reverse segment
        for (int tries = 0; tries < 8; ++tries) {
            int p = rng.next_int(0, M);
            int n = (int)tr.children[p].size();
            if (n < 2) continue;
            int l = rng.next_int(0, n - 1);
            int r = rng.next_int(0, n - 1);
            if (l > r) swap(l, r);
            if (l == r) continue;
            reverse(tr.children[p].begin() + l, tr.children[p].begin() + r + 1);
            return true;
        }
        return false;
    }

    // move subtree
    int v = rng.next_int(0, M - 1);
    int new_parent = rng.next_int(0, M);
    if (new_parent == v) return false;
    if (is_descendant(tr, v, new_parent)) return false;

    int old_parent = tr.parent[v];
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
    int ins = (new_ch.empty() ? 0 : rng.next_int(0, (int)new_ch.size()));
    new_ch.insert(new_ch.begin() + ins, v);
    tr.parent[v] = new_parent;
    return true;
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

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

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

    Tree cur = build_initial_tree(M, pos);
    Tree best = cur;

    vector<array<long long, 2>> cost;
    long long cur_cost = evaluate_tree(cur, pos, cost);
    long long best_cost = cur_cost;

    Timer timer;
    XorShift64 rng;
    const double TIME_LIMIT = 1.85;
    const double T0 = 50.0;
    const double T1 = 1.0;

    int iter = 0;
    while (timer.sec() < TIME_LIMIT) {
        ++iter;
        Tree cand = cur;
        if (!apply_random_move(cand, rng)) continue;
        long long cand_cost = evaluate_tree(cand, pos, cost);

        bool accept = false;
        if (cand_cost <= cur_cost) {
            accept = true;
        } else {
            double t = timer.sec() / TIME_LIMIT;
            double temp = T0 * pow(T1 / T0, t);
            double prob = exp((double)(cur_cost - cand_cost) / temp);
            if (rng.next_double() < prob) accept = true;
        }

        if (accept) {
            cur = std::move(cand);
            cur_cost = cand_cost;
            if (cur_cost < best_cost) {
                best_cost = cur_cost;
                best = cur;
            }
        }
    }

    string ops = build_solution(best, pos);
    for (char ch : ops) {
        cout << ch << '\n';
    }
    return 0;
}
