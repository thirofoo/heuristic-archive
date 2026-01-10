#include <bits/stdc++.h>
using namespace std;

struct Pos {
    int r;
    int c;
};

inline int manhattan(const Pos &a, const Pos &b) {
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
};

long long compute_cost(const vector<int> &order, const vector<array<Pos, 2>> &pos) {
    const int m = static_cast<int>(order.size());
    if (m == 0) return 0;

    Pos start{0, 0};
    Pos a0 = pos[order[0]][0];
    Pos b0 = pos[order[0]][1];

    long long dp0 = manhattan(start, b0) + manhattan(b0, a0); // end at a0
    long long dp1 = manhattan(start, a0) + manhattan(a0, b0); // end at b0

    Pos prev_a = a0;
    Pos prev_b = b0;

    for (int i = 1; i < m; ++i) {
        Pos a = pos[order[i]][0];
        Pos b = pos[order[i]][1];

        long long ndp0 = min(dp0 + manhattan(prev_a, b) + manhattan(b, a),
                             dp1 + manhattan(prev_b, b) + manhattan(b, a));
        long long ndp1 = min(dp0 + manhattan(prev_a, a) + manhattan(a, b),
                             dp1 + manhattan(prev_b, a) + manhattan(a, b));

        dp0 = ndp0;
        dp1 = ndp1;
        prev_a = a;
        prev_b = b;
    }

    return min(dp0, dp1);
}

long long compute_orientations(const vector<int> &order,
                               const vector<array<Pos, 2>> &pos,
                               vector<int> &end_state) {
    const int m = static_cast<int>(order.size());
    end_state.assign(m, 0);
    if (m == 0) return 0;

    vector<long long> dp0(m, 0), dp1(m, 0);
    vector<array<int, 2>> prev_choice(m);

    Pos start{0, 0};
    Pos a0 = pos[order[0]][0];
    Pos b0 = pos[order[0]][1];
    dp0[0] = manhattan(start, b0) + manhattan(b0, a0);
    dp1[0] = manhattan(start, a0) + manhattan(a0, b0);

    for (int i = 1; i < m; ++i) {
        Pos pa = pos[order[i - 1]][0];
        Pos pb = pos[order[i - 1]][1];
        Pos a = pos[order[i]][0];
        Pos b = pos[order[i]][1];

        long long c0 = dp0[i - 1] + manhattan(pa, b) + manhattan(b, a);
        long long c1 = dp1[i - 1] + manhattan(pb, b) + manhattan(b, a);
        if (c0 <= c1) {
            dp0[i] = c0;
            prev_choice[i][0] = 0;
        } else {
            dp0[i] = c1;
            prev_choice[i][0] = 1;
        }

        long long d0 = dp0[i - 1] + manhattan(pa, a) + manhattan(a, b);
        long long d1 = dp1[i - 1] + manhattan(pb, a) + manhattan(a, b);
        if (d0 <= d1) {
            dp1[i] = d0;
            prev_choice[i][1] = 0;
        } else {
            dp1[i] = d1;
            prev_choice[i][1] = 1;
        }
    }

    int end = (dp0[m - 1] <= dp1[m - 1]) ? 0 : 1;
    for (int i = m - 1; i >= 0; --i) {
        end_state[i] = end;
        if (i == 0) break;
        end = prev_choice[i][end];
    }
    return min(dp0[m - 1], dp1[m - 1]);
}

void move_to(Pos &cur, const Pos &target, string &ops) {
    if (cur.r < target.r) {
        ops.append(target.r - cur.r, 'D');
        cur.r = target.r;
    } else if (cur.r > target.r) {
        ops.append(cur.r - target.r, 'U');
        cur.r = target.r;
    }

    if (cur.c < target.c) {
        ops.append(target.c - cur.c, 'R');
        cur.c = target.c;
    } else if (cur.c > target.c) {
        ops.append(cur.c - target.c, 'L');
        cur.c = target.c;
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    if (!(cin >> N)) return 0;
    const int M = N * N / 2;

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

    vector<int> order;
    order.reserve(M);
    vector<char> used(M, false);

    Pos cur{0, 0};
    for (int step = 0; step < M; ++step) {
        int best = -1;
        int best_cost = INT_MAX;
        int best_end = 0;
        for (int i = 0; i < M; ++i) {
            if (used[i]) continue;
            Pos p0 = pos[i][0];
            Pos p1 = pos[i][1];
            int c0 = manhattan(cur, p0) + manhattan(p0, p1); // end at p1
            int c1 = manhattan(cur, p1) + manhattan(p1, p0); // end at p0
            int c = min(c0, c1);
            if (c < best_cost) {
                best_cost = c;
                best = i;
                best_end = (c0 <= c1) ? 1 : 0;
            }
        }
        if (best == -1) break;
        order.push_back(best);
        used[best] = true;
        cur = (best_end == 1) ? pos[best][1] : pos[best][0];
    }
    if ((int)order.size() != M) {
        for (int i = 0; i < M; ++i) {
            if (!used[i]) order.push_back(i);
        }
    }

    Timer timer;
    XorShift64 rng;

    vector<int> best_order = order;
    long long best_cost = compute_cost(best_order, pos);
    vector<int> cur_order = best_order;
    long long cur_cost = best_cost;

    const double time_limit = 1.8;
    while (timer.sec() < time_limit) {
        int a = rng.next_int(0, M - 1);
        int b = rng.next_int(0, M - 1);
        if (a == b) continue;

        if (rng.next_u64() & 1ULL) {
            swap(cur_order[a], cur_order[b]);
            long long cost = compute_cost(cur_order, pos);
            if (cost <= cur_cost) {
                cur_cost = cost;
                if (cost < best_cost) {
                    best_cost = cost;
                    best_order = cur_order;
                }
            } else {
                swap(cur_order[a], cur_order[b]);
            }
        } else {
            if (a > b) swap(a, b);
            reverse(cur_order.begin() + a, cur_order.begin() + b + 1);
            long long cost = compute_cost(cur_order, pos);
            if (cost <= cur_cost) {
                cur_cost = cost;
                if (cost < best_cost) {
                    best_cost = cost;
                    best_order = cur_order;
                }
            } else {
                reverse(cur_order.begin() + a, cur_order.begin() + b + 1);
            }
        }
    }

    vector<int> end_state;
    compute_orientations(best_order, pos, end_state);

    string ops;
    ops.reserve(static_cast<size_t>(best_cost + 2LL * M + 16));
    cur = {0, 0};
    for (int i = 0; i < M; ++i) {
        int idx = best_order[i];
        Pos a = pos[idx][0];
        Pos b = pos[idx][1];
        Pos first = (end_state[i] == 0) ? b : a;
        Pos second = (end_state[i] == 0) ? a : b;
        move_to(cur, first, ops);
        ops.push_back('Z');
        move_to(cur, second, ops);
        ops.push_back('Z');
    }

    for (char op : ops) {
        cout << op << '\n';
    }
    return 0;
}
