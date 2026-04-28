// Your code here
#include <bits/stdc++.h>
using namespace std;

using u64 = unsigned long long;
using u128 = unsigned __int128;

static constexpr int N = 10;
static constexpr int L = 4;
static constexpr int T = 500;
static constexpr uint8_t NOOP = 255;

struct Timer {
    using clk = chrono::steady_clock;
    clk::time_point st;
    Timer() : st(clk::now()) {}
    inline double sec() const { return chrono::duration<double>(clk::now() - st).count(); }
};

struct XorShift64 {
    u64 x = 88172645463393265ull;
    inline u64 next_u64() { x ^= x << 7; x ^= x >> 9; return x; }
    inline uint32_t next_u32() { return (uint32_t)next_u64(); }
    inline int next_int(int lo, int hi) {
        return lo + (int)(next_u32() % (uint32_t)(hi - lo + 1));
    }
    inline double next_double01() {
        u64 r = next_u64();
        r = (r >> 11) | 1ULL; 
        return (double)r * (1.0 / 9007199254740992.0);
    }
};

struct NegLogTable {
    static constexpr int SZ = 1 << 16;
    array<float, SZ> tab;
    NegLogTable() {
        for (int i = 0; i < SZ; i++) {
            double u = (i + 0.5) / (double)SZ;
            tab[i] = (float)(-log(u));
        }
    }
    inline float sample(XorShift64 &rng) const { return tab[rng.next_u32() >> 16]; }
};

struct Input {
    u64 K;
    u64 A[N];
    u64 C[L][N];
    u64 c2[T + 1], c3[T + 1], c4[T + 1];
};

struct State {
    u128 apples;
    u64 b0[N], b1[N], b2[N];
    uint16_t p[L][N];
    uint16_t active_mask; 
};
static_assert(std::is_trivially_copyable_v<State>);

static inline void init_state(State &st, u64 K) {
    st.apples = (u128)K;
    st.active_mask = 0;
    for (int j = 0; j < N; j++) {
        st.b0[j] = 1;
        st.b1[j] = 1;
        st.b2[j] = 1;
        st.p[0][j] = st.p[1][j] = st.p[2][j] = st.p[3][j] = 0;
    }
}

static inline int msb_u128(u128 x) {
    if (x == 0) return -1;
    u64 hi = (u64)(x >> 64);
    if (hi) return 64 + (63 - __builtin_clzll(hi));
    u64 lo = (u64)x;
    return (63 - __builtin_clzll(lo));
}

static inline double log2_u128(u128 x) {
    if (x == 0) return -1e300;
    int msb = msb_u128(x);
    int shift = msb - 52;
    u64 mant;
    if (shift >= 0) mant = (u64)(x >> shift);
    else mant = (u64)(x << (-shift));
    return std::log2((double)mant) + (double)shift;
}

static inline double u128_to_double(u128 v) {
    u64 hi = (u64)(v >> 64);
    if (hi == 0) return (double)(u64)v;
    return (double)hi * 1.8446744073709552e19 + (double)(u64)v;
}

static inline bool apply_action(const Input &in, State &st, uint8_t code) {
    if (code == NOOP) return true;
    int lev = code / 10;
    int j = code % 10;
    u128 cost = (u128)in.C[lev][j] * (u128)(st.p[lev][j] + 1);
    if (st.apples < cost) return false;
    st.apples -= cost;
    st.p[lev][j]++;
    st.active_mask |= (1 << j);
    return true;
}

static inline void do_production(const Input &in, State &st) {
    uint16_t mask = st.active_mask;
    if (mask == 0) return;
    while (mask) {
        int j = __builtin_ctz(mask);
        mask &= mask - 1; 

        uint16_t p0 = st.p[0][j];
        if (p0) st.apples += (u128)in.A[j] * (u128)st.b0[j] * (u128)p0;

        uint16_t p1 = st.p[1][j];
        if (p1) st.b0[j] += (u64)st.b1[j] * (u64)p1;

        uint16_t p2 = st.p[2][j];
        if (p2) st.b1[j] += (u64)st.b2[j] * (u64)p2;

        uint16_t p3 = st.p[3][j];
        if (p3) st.b2[j] += (u64)p3;
    }
}

static inline void fast_forward_noop(const Input &in, State &st, int k) {
    if (k <= 0) return;
    if (st.active_mask == 0) return; 

    const u64 C2 = in.c2[k];
    const u64 C3 = in.c3[k];
    const u64 C4 = in.c4[k];
    const u128 kk = (u128)k;

    uint16_t mask = st.active_mask;
    while (mask) {
        int j = __builtin_ctz(mask);
        mask &= mask - 1;

        const uint16_t p0 = st.p[0][j];
        const uint16_t p1 = st.p[1][j];
        const uint16_t p2 = st.p[2][j];
        const uint16_t p3 = st.p[3][j];

        const u128 b0 = (u128)st.b0[j];
        const u128 b1 = (u128)st.b1[j];
        const u128 b2 = (u128)st.b2[j];
        
        // Optimizations based on zero values
        if (p3 == 0) {
            u128 b2n = b2;
            u128 sumB2 = kk * b2; // p3 is 0

            if (p2 == 0) {
                u128 b1n = b1;
                u128 sumB1 = kk * b1; 

                if (p1 == 0) {
                     u128 sumB0 = kk * b0;
                     if (p0) st.apples += (u128)in.A[j] * (u128)p0 * sumB0;
                     // b0n, b1n, b2n unchanged for b1/b2, b0 += 0
                     // Actually b0,b1,b2 only change if p>0 from level above
                     // p1=0 => b0 constant
                     // p2=0 => b1 constant
                     // p3=0 => b2 constant
                     // So no updates to b0,b1,b2 needed here if p1,p2,p3 all 0
                } else {
                    // p1 != 0, p2=0, p3=0
                    u128 sumB0 = kk * b0 + (u128)p1 * (b1 * (u128)C2);
                    if (p0) st.apples += (u128)in.A[j] * (u128)p0 * sumB0;
                    st.b0[j] = (u64)(b0 + (u128)p1 * sumB1);
                }
            } else {
                // p2 != 0, p3=0
                u128 b1n = b1 + (u128)p2 * sumB2;
                u128 sumB1 = kk * b1 + (u128)p2 * (b2 * (u128)C2);
                
                u128 sumB0 = kk * b0 + (u128)p1 * (b1 * (u128)C2 + (u128)p2 * (b2 * (u128)C3));
                
                if (p0) st.apples += (u128)in.A[j] * (u128)p0 * sumB0;
                st.b1[j] = (u64)b1n;
                st.b0[j] = (u64)(b0 + (u128)p1 * sumB1);
            }
        } else {
            // p3 != 0 (Full case)
            u128 b2n = b2 + (u128)p3 * kk;
            u128 sumB2 = kk * b2 + (u128)p3 * (u128)C2;
            
            u128 b1n = b1 + (u128)p2 * sumB2;
            u128 sumB1 = kk * b1 + (u128)p2 * (b2 * (u128)C2 + (u128)p3 * (u128)C3);

            u128 sumB0 = kk * b0 + (u128)p1 * (b1 * (u128)C2 + (u128)p2 * (b2 * (u128)C3 + (u128)p3 * (u128)C4));
            
            if (p0) st.apples += (u128)in.A[j] * (u128)p0 * sumB0;

            st.b2[j] = (u64)b2n;
            st.b1[j] = (u64)b1n;
            st.b0[j] = (u64)(b0 + (u128)p1 * sumB1);
        }
    }
}

static inline bool simulate_tail_fast(const Input &in, const State &start, int start_t,
                                      const uint8_t actions[T], u128 &final_apples) {
    static constexpr int NOOP_BATCH_TH = 3; 
    State st;
    memcpy(&st, &start, sizeof(State));

    int t = start_t;
    while (t < T) {
        uint8_t a = actions[t];
        if (a == NOOP) {
            int t2 = t + 1;
            while (t2 < T && actions[t2] == NOOP) ++t2;
            int k = t2 - t;
            if (k >= NOOP_BATCH_TH) fast_forward_noop(in, st, k);
            else {
                for (int i = 0; i < k; i++) do_production(in, st);
            }
            t = t2;
            continue;
        }
        if (!apply_action(in, st, a)) return false;
        do_production(in, st);
        ++t;
    }
    final_apples = st.apples;
    return true;
}

static inline void rebuild_history_from(const Input &in, int start_t,
                                        const uint8_t actions[T], State hist[T + 1]) {
    State st;
    memcpy(&st, &hist[start_t], sizeof(State));
    for (int t = start_t; t < T; t++) {
        (void)apply_action(in, st, actions[t]);
        do_production(in, st);
        memcpy(&hist[t + 1], &st, sizeof(State));
    }
}

struct Profile {
    int theta_code = 2; 
    double w[L] = {1.0, 1.3, 1.9, 2.7};
    int virt[L] = {2, 1, 1, 0};
    double min_gain_over_cost = 0.0;
    uint32_t focus_mask = 0xFFFF;
};

static inline void gains_virtual_4(const Input &in, const Profile &pf, const State &st,
                                   int j, int rem, u128 out_gain[L]) {
    const u64 Aj = in.A[j];
    const u64 C2 = in.c2[rem], C3 = in.c3[rem], C4 = in.c4[rem];

    int p0 = max<int>(st.p[0][j], pf.virt[0]);
    int p1 = max<int>(st.p[1][j], pf.virt[1]);
    int p2 = max<int>(st.p[2][j], pf.virt[2]);
    int p3 = max<int>(st.p[3][j], pf.virt[3]);

    u128 b0 = (u128)st.b0[j];
    u128 b1 = (u128)st.b1[j];
    u128 b2 = (u128)st.b2[j];

    u128 Y = b2 * (u128)C3 + (u128)p3 * (u128)C4;
    u128 X = b1 * (u128)C2 + (u128)p2 * Y;

    out_gain[3] = (u128)Aj * (u128)p0 * (u128)p1 * (u128)p2 * (u128)C4;
    out_gain[2] = (u128)Aj * (u128)p0 * (u128)p1 * Y;
    out_gain[1] = (u128)Aj * (u128)p0 * X;
    out_gain[0] = (u128)Aj * ((u128)rem * b0 + (u128)p1 * X);
}

static inline double score_by_profile(const Profile &pf, u128 gain, u128 cost, int lev) {
    double g = u128_to_double(gain);
    double c = u128_to_double(cost);
    if (c < 0.5) c = 0.5;
    if (pf.min_gain_over_cost > 0.0) {
        if (g < c * pf.min_gain_over_cost) return -1e300;
    }
    double base;
    if (pf.theta_code == 0) base = g;
    else if (pf.theta_code == 1) {
        if (c < 1e-9) c = 1e-9;
        base = g / sqrt(c);
    } else {
        base = g / c;
    }
    return base * pf.w[lev];
}

template<int K>
static inline void topk_insert(array<double, K> &sc, array<uint8_t, K> &code,
                               int &sz, double v, uint8_t c) {
    if (sz < K) {
        int pos = sz++;
        sc[pos] = v; code[pos] = c;
        while (pos > 0 && sc[pos] > sc[pos - 1]) {
            swap(sc[pos], sc[pos - 1]);
            swap(code[pos], code[pos - 1]);
            --pos;
        }
        return;
    }
    if (v <= sc[K - 1]) return;
    sc[K - 1] = v; code[K - 1] = c;
    int pos = K - 1;
    while (pos > 0 && sc[pos] > sc[pos - 1]) {
        swap(sc[pos], sc[pos - 1]);
        swap(code[pos], code[pos - 1]);
        --pos;
    }
}

static inline uint8_t choose_greedy_action(const Input &in, const Profile &pf, XorShift64 &rng,
                                           const State &st, int turn, double eps, bool randomized) {
    int rem = T - turn;
    static constexpr int K = 8;
    array<double, K> top_sc;
    array<uint8_t, K> top_code;
    int top_sz = 0;
    for (int k = 0; k < K; k++) { top_sc[k] = -1e300; top_code[k] = NOOP; }

    double best_sc = -1e300;

    for (int j = 0; j < N; j++) {
        if (!((pf.focus_mask >> j) & 1u)) continue;
        u128 gains[L];
        gains_virtual_4(in, pf, st, j, rem, gains);

        for (int lev = 0; lev < L; lev++) {
            u128 cost = (u128)in.C[lev][j] * (u128)(st.p[lev][j] + 1);
            if (st.apples < cost) continue;
            double sc = score_by_profile(pf, gains[lev], cost, lev);
            if (randomized && lev >= 2) sc += 1e-6; 
            best_sc = max(best_sc, sc);
            topk_insert<K>(top_sc, top_code, top_sz, sc, (uint8_t)(lev * 10 + j));
        }
    }

    if (top_sz == 0 || best_sc <= 0.0) return NOOP;

    uint8_t chosen;
    if (randomized && rng.next_double01() < eps && top_sz > 1) chosen = top_code[rng.next_int(0, top_sz - 1)];
    else chosen = top_code[0];

    if (randomized && (rng.next_u32() & 8191u) == 0u) chosen = NOOP;
    return chosen;
}

static inline u128 run_greedy_plan(const Input &in, const Profile &pf, XorShift64 &rng,
                                   uint8_t out_actions[T], bool randomized, double eps) {
    State st;
    init_state(st, in.K);
    for (int t = 0; t < T; t++) {
        uint8_t a = choose_greedy_action(in, pf, rng, st, t, eps, randomized);
        out_actions[t] = a;
        if (!apply_action(in, st, a)) out_actions[t] = NOOP;
        do_production(in, st);
    }
    return st.apples;
}

static inline u128 simulate_full_sanitize_fast(const Input &in, const uint8_t actions[T], uint8_t out_sanitized[T]) {
    static constexpr int NOOP_BATCH_TH = 3;
    State st;
    init_state(st, in.K);

    int t = 0;
    while (t < T) {
        uint8_t a = actions[t];
        if (a == NOOP) {
            out_sanitized[t] = NOOP;
            int t2 = t + 1;
            while (t2 < T && actions[t2] == NOOP) {
                out_sanitized[t2] = NOOP;
                ++t2;
            }
            int k = t2 - t;
            if (k >= NOOP_BATCH_TH) fast_forward_noop(in, st, k);
            else for (int i = 0; i < k; i++) do_production(in, st);
            t = t2;
            continue;
        }

        int lev = a / 10, j = a % 10;
        u128 cost = (u128)in.C[lev][j] * (u128)(st.p[lev][j] + 1);
        if (st.apples < cost) a = NOOP;

        out_sanitized[t] = a;
        (void)apply_action(in, st, a);
        do_production(in, st);
        ++t;
    }
    return st.apples;
}

template<int K>
static inline int collect_top_actions_here(const Input &in, const Profile &pf, const State &st, int turn,
                                           array<uint8_t, K> &out) {
    int rem = T - turn;
    array<double, K> sc;
    int sz = 0;
    for (int i = 0; i < K; i++) { sc[i] = -1e300; out[i] = NOOP; }

    out[0] = NOOP;
    sc[0] = 0.0;
    sz = 1;

    for (int j = 0; j < N; j++) {
        if (!((pf.focus_mask >> j) & 1u)) continue;
        u128 gains[L];
        gains_virtual_4(in, pf, st, j, rem, gains);

        for (int lev = 0; lev < L; lev++) {
            u128 cost = (u128)in.C[lev][j] * (u128)(st.p[lev][j] + 1);
            if (st.apples < cost) continue;
            double s = score_by_profile(pf, gains[lev], cost, lev);
            if (s <= -1e200) continue;
            topk_insert<K>(sc, out, sz, s, (uint8_t)(lev * 10 + j));
        }
    }
    return sz;
}

static inline void unique_push(vector<int> &v, int x) {
    for (int y : v) if (y == x) return;
    v.push_back(x);
}

static inline uint8_t suggest_fix_chain(const Input &in, const State &st, int turn) {
    Profile pf;
    pf.theta_code = 2;
    pf.w[0] = 1.0; pf.w[1] = 1.4; pf.w[2] = 2.1; pf.w[3] = 3.2;
    pf.virt[0] = 4; pf.virt[1] = 1; pf.virt[2] = 1; pf.virt[3] = 0;
    pf.min_gain_over_cost = 0.0;
    pf.focus_mask = st.active_mask; 

    int rem = T - turn;
    double best = -1e300;
    uint8_t best_code = NOOP;

    uint16_t mask = st.active_mask;
    while (mask) {
        int j = __builtin_ctz(mask);
        mask &= mask - 1;

        int lev = -1;
        if (st.p[3][j] > 0 && st.p[2][j] == 0) lev = 2;
        else if (st.p[2][j] > 0 && st.p[1][j] == 0) lev = 1;
        else if (st.p[1][j] > 0 && st.p[0][j] == 0) lev = 0;
        if (lev < 0) continue;

        u128 cost = (u128)in.C[lev][j] * (u128)(st.p[lev][j] + 1);
        if (st.apples < cost) continue;

        u128 gains[L];
        gains_virtual_4(in, pf, st, j, rem, gains);
        double sc = score_by_profile(pf, gains[lev], cost, lev);
        if (sc > best) {
            best = sc;
            best_code = (uint8_t)(lev * 10 + j);
        }
    }
    return best_code;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Input in;
    int n_in, l_in, t_in;
    cin >> n_in >> l_in >> t_in >> in.K;
    for (int j = 0; j < N; j++) cin >> in.A[j];
    for (int i = 0; i < L; i++) for (int j = 0; j < N; j++) cin >> in.C[i][j];

    for (int r = 0; r <= T; r++) {
        in.c2[r] = (r >= 2) ? (u64)r * (r - 1) / 2 : 0;
        in.c3[r] = (r >= 3) ? (u64)r * (r - 1) * (r - 2) / 6 : 0;
        in.c4[r] = (r >= 4) ? (u64)r * (r - 1) * (r - 2) * (r - 3) / 24 : 0;
    }

    Timer timer;
    XorShift64 rng;
    NegLogTable neglog;

    const double BUILD_T = 0.28; 
    const double SA_MAIN_END = 1.95; 
    const double END_TIME = 1.98;

    vector<Profile> base_profiles;
    base_profiles.push_back({2, {1.0, 1.3, 1.9, 2.7}, {2, 1, 1, 0}, 0.0, 0xFFFF});
    base_profiles.push_back({2, {1.0, 1.35, 1.95, 2.8}, {2, 1, 1, 0}, 0.0, 0xFFFF});
    base_profiles.push_back({1, {1.0, 1.35, 2.0, 3.2}, {3, 1, 1, 0}, 0.0, 0xFFFF});
    base_profiles.push_back({2, {1.0, 1.45, 2.2, 3.6}, {5, 2, 1, 0}, 0.0, 0xFFFF});
    base_profiles.push_back({2, {1.0, 1.2, 1.5, 1.8}, {0, 0, 0, 0}, 1.2, 0xFFFF});
    base_profiles.push_back({0, {1.25, 1.35, 1.55, 1.75}, {1, 1, 1, 0}, 0.0, 0xFFFF});
    base_profiles.push_back({2, {1.35, 1.15, 1.05, 1.05}, {0, 0, 0, 0}, 0.9, 0xFFFF});

    static uint8_t best_actions[T], tmp_actions[T];
    u128 best_apples = 0;

    auto commit_best = [&](u128 apples, const uint8_t acts[T]) {
        if (apples > best_apples) {
            best_apples = apples;
            memcpy(best_actions, acts, T);
        }
    };

    vector<uint32_t> focus_masks;
    for(int j=0; j<N; ++j) focus_masks.push_back(1u << j);
    for(int j=1; j<N; ++j) focus_masks.push_back((1u << j) | 1u);
    
    // Add {j, best_A} masks
    int j_maxA = 0;
    for(int j=1; j<N; ++j) if(in.A[j] > in.A[j_maxA]) j_maxA = j;
    for(int j=0; j<N; ++j) if(j != j_maxA) focus_masks.push_back((1u << j) | (1u << j_maxA));

    // Dedup
    sort(focus_masks.begin(), focus_masks.end());
    focus_masks.erase(unique(focus_masks.begin(), focus_masks.end()), focus_masks.end());

    for (uint32_t fm : focus_masks) {
        Profile pf = base_profiles[0];
        pf.focus_mask = fm;
        u128 ap = run_greedy_plan(in, pf, rng, tmp_actions, false, 0.0);
        commit_best(ap, tmp_actions);
    }
    
    for (auto &pf : base_profiles) {
        pf.focus_mask = 0xFFFF;
        u128 ap = run_greedy_plan(in, pf, rng, tmp_actions, false, 0.0);
        commit_best(ap, tmp_actions);
    }

    while (timer.sec() < BUILD_T) {
        Profile pf = base_profiles[rng.next_int(0, (int)base_profiles.size() - 1)];
        for (int i = 0; i < L; i++) pf.w[i] *= (0.85 + 0.30 * rng.next_double01());

        if ((rng.next_u32() & 3u) == 0u) {
            pf.virt[0] = rng.next_int(0, 8);
            pf.virt[1] = rng.next_int(0, 4);
            pf.virt[2] = rng.next_int(0, 3);
            pf.virt[3] = rng.next_int(0, 2);
        }

        uint32_t r = rng.next_u32();
        if ((r & 3u) == 0u) {
            int j = rng.next_int(0, N - 1);
            pf.focus_mask = (rng.next_u32() & 1u) ? ((1u << j) | 1u) : (1u << j);
        } else if ((r & 3u) == 1u) {
            int j1 = rng.next_int(0, N - 1);
            int j2 = rng.next_int(0, N - 1);
            pf.focus_mask = (1u << j1) | (1u << j2);
            if (rng.next_u32() & 1u) pf.focus_mask |= 1u;
        } else {
            pf.focus_mask = 0xFFFF;
        }

        pf.min_gain_over_cost = (rng.next_u32() & 1u) ? (0.6 + 0.8 * rng.next_double01()) : 0.0;
        double eps = 0.05 + 0.20 * rng.next_double01();

        u128 ap = run_greedy_plan(in, pf, rng, tmp_actions, true, eps);
        commit_best(ap, tmp_actions);
    }

    // ----- SA -----
    static uint8_t cur_actions[T];
    memcpy(cur_actions, best_actions, T);

    static State hist[T + 1];
    init_state(hist[0], in.K);
    rebuild_history_from(in, 0, cur_actions, hist);

    u128 cur_apples = hist[T].apples;
    double cur_obj = log2_u128(cur_apples + 1);

    double T0 = 0.10, Tend = 0.0010;
    // Calibrate T0
    {
        vector<double> neg;
        neg.reserve(64);
        Profile pf = base_profiles[0];
        for (int k = 0; k < 50; k++) {
            int t = rng.next_int(0, T - 10);
            array<uint8_t, 8> top;
            int sz = collect_top_actions_here<8>(in, pf, hist[t], t, top);
            if (sz <= 1) continue;
            uint8_t old = cur_actions[t];
            uint8_t nw = top[rng.next_int(0, sz - 1)];
            if (nw == old) continue;
            cur_actions[t] = nw;
            u128 fa;
            bool ok = simulate_tail_fast(in, hist[t], t, cur_actions, fa);
            cur_actions[t] = old;
            if (ok) {
                double diff = log2_u128(fa + 1) - cur_obj;
                if (diff < 0) neg.push_back(-diff);
            }
        }
        if (!neg.empty()) {
            nth_element(neg.begin(), neg.begin() + neg.size() / 2, neg.end());
            T0 = max(0.01, neg[neg.size() / 2] * 2.0);
            Tend = T0 * 0.005;
        }
    }
    T0 *= 0.9700000000000001;

    const double sa_start = timer.sec();
    const double ln_ratio = log(Tend / T0);
    double temp = T0;

    double now_cached = timer.sec();
    double prog_cached = 0.0;

    static uint8_t cand_actions[T];

    auto accept_move = [&](double diff) -> bool {
        if (diff >= 0) return true;
        float nl = neglog.sample(rng);
        return (-diff) <= (double)nl * temp;
    };

    int iter = 0;
    while (true) {
        if ((iter & 1023) == 0) {
            now_cached = timer.sec();
            if (now_cached >= SA_MAIN_END) break;
            prog_cached = (now_cached - sa_start) / (SA_MAIN_END - sa_start);
            prog_cached = min(1.0, max(0.0, prog_cached));
            temp = T0 * exp(ln_ratio * prog_cached);
        }
        iter++;

        double prog = prog_cached;
        int type = 0;
        double r = rng.next_double01();

        if (prog > 0.85) {
            if (r < 0.55) type = 0;          // micro
            else if (r < 0.85) type = 4;     // shift
            else if (r < 0.95) type = 1;     // swap
            else if (r < 0.985) type = 6;    // fixchain
            else type = 3;                   // small window
        } else {
            if (r < 0.34) type = 0;
            else if (r < 0.47) type = 1;
            else if (r < 0.78) type = 4;
            else if (r < 0.845) type = 3;
            else if (r < 0.975) type = 6;    // fixchain
            else type = 5;
        }

        if (type == 0) { // MicroChange
            double u = rng.next_double01();
            int t = (rng.next_u32() & 1u) ? (int)(u * u * (double)T) : (int)(u * (double)T);
            t = max(0, min(T - 1, t));

            Profile pf = base_profiles[rng.next_int(0, (int)base_profiles.size() - 1)];
            // Increase randomness to 35%
            if (rng.next_double01() < 0.0) pf.focus_mask = (1u << rng.next_int(0, N - 1));

            array<uint8_t, 12> top;
            int sz = collect_top_actions_here<12>(in, pf, hist[t], t, top);
            if (sz <= 0) continue;

            uint8_t old = cur_actions[t];
            uint8_t nw = top[rng.next_int(0, sz - 1)];
            if (nw == old) continue;

            cur_actions[t] = nw;
            u128 fa;
            bool ok = simulate_tail_fast(in, hist[t], t, cur_actions, fa);
            if (!ok) { cur_actions[t] = old; continue; }

            double obj = log2_u128(fa + 1);
            double diff = obj - cur_obj;
            if (accept_move(diff)) {
                cur_apples = fa;
                cur_obj = obj;
                rebuild_history_from(in, t, cur_actions, hist);
                if (cur_apples > best_apples) {
                    best_apples = cur_apples;
                    memcpy(best_actions, cur_actions, T);
                }
            } else {
                cur_actions[t] = old;
            }
        }
        else if (type == 1) { // Swap
            int t1 = rng.next_int(0, T - 2);
            int t2 = min(T - 1, t1 + rng.next_int(1, 14));
            if (cur_actions[t1] == cur_actions[t2]) continue;

            swap(cur_actions[t1], cur_actions[t2]);
            u128 fa;
            bool ok = simulate_tail_fast(in, hist[t1], t1, cur_actions, fa);
            if (!ok) { swap(cur_actions[t1], cur_actions[t2]); continue; }

            double obj = log2_u128(fa + 1);
            double diff = obj - cur_obj;
            if (accept_move(diff)) {
                cur_apples = fa;
                cur_obj = obj;
                rebuild_history_from(in, t1, cur_actions, hist);
                if (cur_apples > best_apples) {
                    best_apples = cur_apples;
                    memcpy(best_actions, cur_actions, T);
                }
            } else {
                swap(cur_actions[t1], cur_actions[t2]);
            }
        }
        else if (type == 3) { // WindowRebuild
            double u = rng.next_double01();
            int l0 = (int)(u * u * (double)(T - 12));
            l0 = max(0, min(T - 12, l0));

            double v = rng.next_double01();
            double vp = v * v * v * v;
            int len = 12 + (int)(120.0 * vp);
            int r0 = min(T, l0 + len);

            Profile pf = base_profiles[rng.next_int(0, (int)base_profiles.size() - 1)];
            for (int i = 0; i < L; i++) pf.w[i] *= (0.9 + 0.2 * rng.next_double01());
            if (rng.next_u32() & 1u) pf.focus_mask = (1u << rng.next_int(0, N - 1));

            memcpy(cand_actions, cur_actions, T);
            State st; memcpy(&st, &hist[l0], sizeof(State));

            for (int t = l0; t < r0; t++) {
                uint8_t a = choose_greedy_action(in, pf, rng, st, t, 0.05, false);
                cand_actions[t] = a;
                if (!apply_action(in, st, a)) cand_actions[t] = NOOP;
                do_production(in, st);
            }

            u128 fa;
            bool ok = simulate_tail_fast(in, st, r0, cand_actions, fa);
            if (!ok) continue;

            double obj = log2_u128(fa + 1);
            double diff = obj - cur_obj;
            if (accept_move(diff)) {
                cur_apples = fa;
                cur_obj = obj;
                memcpy(cur_actions, cand_actions, T);
                rebuild_history_from(in, l0, cur_actions, hist);
                if (cur_apples > best_apples) {
                    best_apples = cur_apples;
                    memcpy(best_actions, cur_actions, T);
                }
            }
        }
        else if (type == 4) { // Shift
            int t1 = rng.next_int(0, T - 1);
            int dist = rng.next_int(1, 28);
            int t2 = (rng.next_u32() & 1u) ? min(T - 1, t1 + dist) : max(0, t1 - dist);
            if (t1 == t2) continue;
            int l0 = min(t1, t2);

            uint8_t val = cur_actions[t1];
            if (t1 < t2) {
                for (int k = t1; k < t2; ++k) cur_actions[k] = cur_actions[k + 1];
                cur_actions[t2] = val;
            } else {
                for (int k = t1; k > t2; --k) cur_actions[k] = cur_actions[k - 1];
                cur_actions[t2] = val;
            }

            u128 fa;
            bool ok = simulate_tail_fast(in, hist[l0], l0, cur_actions, fa);
            if (!ok) {
                if (t1 < t2) {
                    for (int k = t2; k > t1; --k) cur_actions[k] = cur_actions[k - 1];
                    cur_actions[t1] = val;
                } else {
                    for (int k = t2; k < t1; ++k) cur_actions[k] = cur_actions[k + 1];
                    cur_actions[t1] = val;
                }
                continue;
            }

            double obj = log2_u128(fa + 1);
            double diff = obj - cur_obj;
            if (accept_move(diff)) {
                cur_apples = fa;
                cur_obj = obj;
                rebuild_history_from(in, l0, cur_actions, hist);
                if (cur_apples > best_apples) {
                    best_apples = cur_apples;
                    memcpy(best_actions, cur_actions, T);
                }
            } else {
                if (t1 < t2) {
                    for (int k = t2; k > t1; --k) cur_actions[k] = cur_actions[k - 1];
                    cur_actions[t1] = val;
                } else {
                    for (int k = t2; k < t1; ++k) cur_actions[k] = cur_actions[k + 1];
                    cur_actions[t1] = val;
                }
            }
        }
        else if (type == 5) { // Full Suffix Rebuild
            int l0 = rng.next_int(0, T - 10);
            Profile pf = base_profiles[rng.next_int(0, (int)base_profiles.size() - 1)];
            if (rng.next_u32() & 1u) pf.focus_mask = (1u << rng.next_int(0, N - 1));

            memcpy(cand_actions, cur_actions, T);
            State st; memcpy(&st, &hist[l0], sizeof(State));

            for (int t = l0; t < T; t++) {
                uint8_t a = choose_greedy_action(in, pf, rng, st, t, 0.10, true);
                cand_actions[t] = a;
                if (!apply_action(in, st, a)) cand_actions[t] = NOOP;
                do_production(in, st);
            }

            u128 fa = st.apples;
            double obj = log2_u128(fa + 1);
            double diff = obj - cur_obj;
            if (accept_move(diff)) {
                cur_apples = fa;
                cur_obj = obj;
                memcpy(cur_actions, cand_actions, T);
                rebuild_history_from(in, l0, cur_actions, hist);
                if (cur_apples > best_apples) {
                    best_apples = cur_apples;
                    memcpy(best_actions, cur_actions, T);
                }
            }
        }
        else if (type == 6) { // FixChain
            double u = rng.next_double01();
            int t = (int)(u * u * (double)T);
            t = max(0, min(T - 1, t));
            uint8_t cand = suggest_fix_chain(in, hist[t], t);
            if (cand == NOOP) continue;
            uint8_t old = cur_actions[t];
            if (cand == old) continue;

            cur_actions[t] = cand;
            u128 fa;
            bool ok = simulate_tail_fast(in, hist[t], t, cur_actions, fa);
            if (!ok) { cur_actions[t] = old; continue; }

            double obj = log2_u128(fa + 1);
            double diff = obj - cur_obj;
            if (accept_move(diff)) {
                cur_apples = fa;
                cur_obj = obj;
                rebuild_history_from(in, t, cur_actions, hist);
                if (cur_apples > best_apples) {
                    best_apples = cur_apples;
                    memcpy(best_actions, cur_actions, T);
                }
            } else {
                cur_actions[t] = old;
            }
        }
    }

    // ----- Final Polish -----
    memcpy(cur_actions, best_actions, T);
    rebuild_history_from(in, 0, cur_actions, hist);
    double best_obj = log2_u128(best_apples + 1);

    while (timer.sec() < END_TIME) {
        if (rng.next_u32() & 1u) {
            double u = rng.next_double01();
            int t = (int)(u * u * (double)T);
            t = max(0, min(T - 1, t));
            Profile pf = base_profiles[0];

            array<uint8_t, 10> top;
            int sz = collect_top_actions_here<10>(in, pf, hist[t], t, top);
            if (sz <= 0) continue;
            uint8_t nw = top[0];
            uint8_t old = cur_actions[t];
            if (nw == old) continue;

            cur_actions[t] = nw;
            u128 fa;
            bool ok = simulate_tail_fast(in, hist[t], t, cur_actions, fa);
            if (!ok) { cur_actions[t] = old; continue; }

            double obj = log2_u128(fa + 1);
            if (obj > best_obj) {
                best_apples = fa;
                best_obj = obj;
                memcpy(best_actions, cur_actions, T);
                rebuild_history_from(in, t, cur_actions, hist);
            } else {
                cur_actions[t] = old;
            }
        } else {
            int t1 = rng.next_int(0, T - 1);
            int dist = rng.next_int(1, 18);
            int t2 = (rng.next_u32() & 1u) ? min(T - 1, t1 + dist) : max(0, t1 - dist);
            if (t1 == t2) continue;
            int l0 = min(t1, t2);
            uint8_t val = cur_actions[t1];

            if (t1 < t2) {
                for (int k = t1; k < t2; ++k) cur_actions[k] = cur_actions[k + 1];
                cur_actions[t2] = val;
            } else {
                for (int k = t1; k > t2; --k) cur_actions[k] = cur_actions[k - 1];
                cur_actions[t2] = val;
            }

            u128 fa;
            bool ok = simulate_tail_fast(in, hist[l0], l0, cur_actions, fa);
            if (!ok) {
                 if (t1 < t2) {
                    for (int k = t2; k > t1; --k) cur_actions[k] = cur_actions[k - 1];
                    cur_actions[t1] = val;
                } else {
                    for (int k = t2; k < t1; ++k) cur_actions[k] = cur_actions[k + 1];
                    cur_actions[t1] = val;
                }
                continue;
            }

            double obj = log2_u128(fa + 1);
            if (obj > best_obj) {
                best_apples = fa;
                best_obj = obj;
                memcpy(best_actions, cur_actions, T);
                rebuild_history_from(in, l0, cur_actions, hist);
            } else {
                 if (t1 < t2) {
                    for (int k = t2; k > t1; --k) cur_actions[k] = cur_actions[k - 1];
                    cur_actions[t1] = val;
                } else {
                    for (int k = t2; k < t1; ++k) cur_actions[k] = cur_actions[k + 1];
                    cur_actions[t1] = val;
                }
            }
        }
    }

    static uint8_t final_out[T];
    simulate_full_sanitize_fast(in, best_actions, final_out);

    for (int t = 0; t < T; t++) {
        if (final_out[t] == NOOP) cout << "-1\n";
        else cout << (int)(final_out[t] / 10) << " " << (int)(final_out[t] % 10) << "\n";
    }
    return 0;
}