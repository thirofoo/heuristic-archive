#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>
using namespace std;

const int N = 20;
int AK, AM, AW;
string vwalls[N];
string hwalls[N - 1];

const int DI[] = {-1, 0, 1, 0};
const int DJ[] = {0, 1, 0, -1};
const char DIR_CHAR[] = {'U', 'R', 'D', 'L'};

// Precomputed wall info for fast access
bool wall_cache[N][N][4];

void precompute_walls() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int d = 0; d < 4; d++) {
                int ni = i + DI[d], nj = j + DJ[d];
                if (ni < 0 || ni >= N || nj < 0 || nj >= N) {
                    wall_cache[i][j][d] = true;
                    continue;
                }
                if (i == ni)
                    wall_cache[i][j][d] = vwalls[i][min(j, nj)] == '1';
                else
                    wall_cache[i][j][d] = hwalls[min(i, ni)][j] == '1';
            }
}

struct RobotOut {
    int m, si, sj;
    char sd;
    vector<array<int, 4>> trans;
};

int segment_solve(vector<RobotOut>& robots) {
    auto try_dir = [](bool col) -> pair<int, vector<RobotOut>> {
        vector<RobotOut> result;
        int total = 0;
        if (col) {
            for (int j = 0; j < N; j++) {
                int start = 0;
                for (int i = 0; i < N; i++) {
                    bool is_end = (i == N - 1) || (hwalls[i][j] == '1');
                    if (is_end) {
                        int len = i - start + 1;
                        RobotOut r;
                        r.si = start;
                        r.sj = j;
                        r.sd = 'D';
                        if (len == 1) {
                            r.m = 1;
                            r.trans = {{1, 0, 1, 0}};
                        } else {
                            r.m = 2;
                            r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}};
                        }
                        total += r.m;
                        result.push_back(r);
                        start = i + 1;
                    }
                }
            }
        } else {
            for (int i = 0; i < N; i++) {
                int start = 0;
                for (int j = 0; j < N; j++) {
                    bool is_end = (j == N - 1) || (vwalls[i][j] == '1');
                    if (is_end) {
                        int len = j - start + 1;
                        RobotOut r;
                        r.si = i;
                        r.sj = start;
                        r.sd = 'R';
                        if (len == 1) {
                            r.m = 1;
                            r.trans = {{1, 0, 1, 0}};
                        } else {
                            r.m = 2;
                            r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}};
                        }
                        total += r.m;
                        result.push_back(r);
                        start = j + 1;
                    }
                }
            }
        }
        return {total, result};
    };
    auto [rm, rr] = try_dir(false);
    auto [cm, cr] = try_dir(true);
    if (rm <= cm) {
        robots = rr;
        return rm;
    }
    robots = cr;
    return cm;
}

// Fast uncovered segment count (just returns M cost, no robot construction)
int segment_uncovered_cost(const bool covered[N][N]) {
    // Try both row and col, return min
    int row_m = 0;
    for (int i = 0; i < N; i++) {
        int start = 0;
        for (int j = 0; j < N; j++) {
            bool is_end = (j == N - 1) || (vwalls[i][j] == '1');
            if (is_end) {
                bool need = false;
                for (int jj = start; jj <= j; jj++)
                    if (!covered[i][jj]) {
                        need = true;
                        break;
                    }
                if (need) {
                    int len = j - start + 1;
                    row_m += (len == 1) ? 1 : 2;
                }
                start = j + 1;
            }
        }
    }
    int col_m = 0;
    for (int j = 0; j < N; j++) {
        int start = 0;
        for (int i = 0; i < N; i++) {
            bool is_end = (i == N - 1) || (hwalls[i][j] == '1');
            if (is_end) {
                bool need = false;
                for (int ii = start; ii <= i; ii++)
                    if (!covered[ii][j]) {
                        need = true;
                        break;
                    }
                if (need) {
                    int len = i - start + 1;
                    col_m += (len == 1) ? 1 : 2;
                }
                start = i + 1;
            }
        }
    }
    return min(row_m, col_m);
}

int segment_for_uncovered(const bool covered[N][N], vector<RobotOut>& robots) {
    auto try_dir = [&](bool col) -> pair<int, vector<RobotOut>> {
        vector<RobotOut> result;
        int total = 0;
        bool cov[N][N];
        memcpy(cov, covered, sizeof(cov));
        if (col) {
            for (int j = 0; j < N; j++) {
                int start = 0;
                for (int i = 0; i < N; i++) {
                    bool is_end = (i == N - 1) || (hwalls[i][j] == '1');
                    if (is_end) {
                        bool need = false;
                        for (int ii = start; ii <= i; ii++)
                            if (!cov[ii][j]) need = true;
                        if (need) {
                            int len = i - start + 1;
                            RobotOut r;
                            r.si = start;
                            r.sj = j;
                            r.sd = 'D';
                            if (len == 1) {
                                r.m = 1;
                                r.trans = {{1, 0, 1, 0}};
                            } else {
                                r.m = 2;
                                r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}};
                            }
                            total += r.m;
                            result.push_back(r);
                            for (int ii = start; ii <= i; ii++) cov[ii][j] = true;
                        }
                        start = i + 1;
                    }
                }
            }
        } else {
            for (int i = 0; i < N; i++) {
                int start = 0;
                for (int j = 0; j < N; j++) {
                    bool is_end = (j == N - 1) || (vwalls[i][j] == '1');
                    if (is_end) {
                        bool need = false;
                        for (int jj = start; jj <= j; jj++)
                            if (!cov[i][jj]) need = true;
                        if (need) {
                            int len = j - start + 1;
                            RobotOut r;
                            r.si = i;
                            r.sj = start;
                            r.sd = 'R';
                            if (len == 1) {
                                r.m = 1;
                                r.trans = {{1, 0, 1, 0}};
                            } else {
                                r.m = 2;
                                r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}};
                            }
                            total += r.m;
                            result.push_back(r);
                            for (int jj = start; jj <= j; jj++) cov[i][jj] = true;
                        }
                        start = j + 1;
                    }
                }
            }
        }
        return {total, result};
    };
    auto [rm, rr] = try_dir(false);
    auto [cm, cr] = try_dir(true);
    if (rm <= cm) {
        robots = rr;
        return rm;
    }
    robots = cr;
    return cm;
}

// Fast simulation that returns covered cell count and fills coverage array
// Uses static arrays with timestamp to avoid clearing
static int sim_vis[N * N * 4 * 1601];
static int sim_ts = 0;

int simulate(const int trans[][4], int m, int si, int sj, int sd, bool cov[N][N]) {
    int total = N * N * 4 * m;
    sim_ts++;
    int ts = sim_ts;
    int ci = si, cj = sj, cd = sd, cs = 0;

    // We need to find cycle and track cells in cycle only
    // Use two arrays: one for state visitation, one for route
    static int route_i[N * N * 4 * 1601 + 1];
    static int route_j[N * N * 4 * 1601 + 1];
    int rlen = 0;

    for (int t = 0; t <= total; t++) {
        int key = ((ci * N + cj) * 4 + cd) * m + cs;
        if (sim_vis[key] == ts) {
            // Find cycle start by re-simulating
            int ci2 = si, cj2 = sj, cd2 = sd, cs2 = 0;
            int cycle_start = 0;
            for (int k = 0; k < t; k++) {
                int k2 = ((ci2 * N + cj2) * 4 + cd2) * m + cs2;
                if (k2 == key) {
                    cycle_start = k;
                    break;
                }
                int w = wall_cache[ci2][cj2][cd2] ? 1 : 0;
                int a, ns;
                if (w == 0) {
                    a = trans[cs2][0];
                    ns = trans[cs2][1];
                } else {
                    a = trans[cs2][2];
                    ns = trans[cs2][3];
                }
                if (a == 0) {
                    ci2 += DI[cd2];
                    cj2 += DJ[cd2];
                } else if (a == 1)
                    cd2 = (cd2 + 1) % 4;
                else
                    cd2 = (cd2 + 3) % 4;
                cs2 = ns;
            }
            int cnt = 0;
            memset(cov, 0, sizeof(bool) * N * N);
            for (int k = cycle_start; k < rlen; k++) {
                if (!cov[route_i[k]][route_j[k]]) {
                    cov[route_i[k]][route_j[k]] = true;
                    cnt++;
                }
            }
            return cnt;
        }
        sim_vis[key] = ts;
        route_i[rlen] = ci;
        route_j[rlen] = cj;
        rlen++;

        int w = wall_cache[ci][cj][cd] ? 1 : 0;
        int a, ns;
        if (w == 0) {
            a = trans[cs][0];
            ns = trans[cs][1];
        } else {
            a = trans[cs][2];
            ns = trans[cs][3];
        }
        if (a == 0) {
            ci += DI[cd];
            cj += DJ[cd];
        } else if (a == 1)
            cd = (cd + 1) % 4;
        else
            cd = (cd + 3) % 4;
        cs = ns;
    }
    memset(cov, 0, sizeof(bool) * N * N);
    return 0;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    int n_unused;
    cin >> n_unused >> AK >> AM >> AW;
    for (int i = 0; i < N; i++) cin >> vwalls[i];
    for (int i = 0; i < N - 1; i++) cin >> hwalls[i];
    precompute_walls();

    auto start_time = chrono::steady_clock::now();
    mt19937 rng(42);
    auto elapsed_ms = [&]() {
        return chrono::duration_cast<chrono::milliseconds>(
                   chrono::steady_clock::now() - start_time)
            .count();
    };

    int best_M = INT_MAX;
    vector<RobotOut> best_robots;
    {
        vector<RobotOut> seg;
        best_M = segment_solve(seg);
        best_robots = seg;
    }

    // ============================================================
    // Phase 1: Try zigzag automaton variants with all start positions
    // The base zigzag: go straight, bounce at wall, shift one column, repeat
    // ============================================================
    {
        // Base zigzag: S0=turn, S1=go straight, S2=shift+corner, S3=transition
        // Original (vertical zigzag, shift right):
        //   S0: w0→R,1  w1→R,1
        //   S1: w0→F,3  w1→L,2
        //   S2: w0→F,0  w1→R,2
        //   S3: w0→L,2  w1→L,1
        // We generate 8 variants: 4 rotations x 2 mirrors (shift left vs right)

        // Action encoding: 0=F, 1=R, 2=L
        // To rotate an automaton by 90 degrees CW, we swap R<->L in actions
        // and rotate the start direction.
        // Actually rotation doesn't change the automaton itself - it only changes
        // the start direction. Mirror (swap R<->L) gives shift-left variant.

        int zigzag_base[4][4] = {
            {1, 1, 1, 1},  // S0: always R, goto S1
            {0, 3, 2, 2},  // S1: w0→F,S3  w1→L,S2
            {0, 0, 1, 2},  // S2: w0→F,S0  w1→R,S2
            {2, 2, 2, 1},  // S3: w0→L,S2  w1→L,S1
        };

        // Mirror variant: swap R(1)<->L(2) in actions
        int zigzag_mirror[4][4];
        for (int s = 0; s < 4; s++) {
            for (int k = 0; k < 4; k += 2) {
                int a = zigzag_base[s][k];
                if (a == 1)
                    a = 2;
                else if (a == 2)
                    a = 1;
                zigzag_mirror[s][k] = a;
                zigzag_mirror[s][k + 1] = zigzag_base[s][k + 1];
            }
        }

        int variants[2][4][4];
        memcpy(variants[0], zigzag_base, sizeof(zigzag_base));
        memcpy(variants[1], zigzag_mirror, sizeof(zigzag_mirror));

        for (int v = 0; v < 2; v++) {
            for (int sd = 0; sd < 4; sd++) {
                for (int si = 0; si < N; si++) {
                    for (int sj = 0; sj < N; sj++) {
                        bool cov[N][N];
                        int cover = simulate(variants[v], 4, si, sj, sd, cov);
                        if (cover == N * N) {
                            if (4 < best_M) {
                                best_M = 4;
                                best_robots.clear();
                                RobotOut r;
                                r.m = 4;
                                r.si = si;
                                r.sj = sj;
                                r.sd = DIR_CHAR[sd];
                                r.trans.resize(4);
                                for (int s = 0; s < 4; s++)
                                    r.trans[s] = {variants[v][s][0], variants[v][s][1],
                                                  variants[v][s][2], variants[v][s][3]};
                                best_robots = {r};
                            }
                        } else {
                            int fb = segment_uncovered_cost(cov);
                            if (4 + fb < best_M) {
                                best_M = 4 + fb;
                                best_robots.clear();
                                RobotOut r;
                                r.m = 4;
                                r.si = si;
                                r.sj = sj;
                                r.sd = DIR_CHAR[sd];
                                r.trans.resize(4);
                                for (int s = 0; s < 4; s++)
                                    r.trans[s] = {variants[v][s][0], variants[v][s][1],
                                                  variants[v][s][2], variants[v][s][3]};
                                best_robots = {r};
                                bool cov2[N][N];
                                memcpy(cov2, cov, sizeof(cov));
                                vector<RobotOut> fb_r;
                                segment_for_uncovered(cov2, fb_r);
                                for (auto& fr : fb_r) best_robots.push_back(fr);
                            }
                        }
                    }
                }
            }
        }
    }

    // ============================================================
    // Phase 1.5: Two zigzag robots with different start positions (M=8)
    // When single zigzag can't cover all, try pairs
    // ============================================================
    if (best_M > 8) {
        int zigzag_base[4][4] = {
            {1, 1, 1, 1},
            {0, 3, 2, 2},
            {0, 0, 1, 2},
            {2, 2, 2, 1},
        };
        int zigzag_mirror[4][4];
        for (int s = 0; s < 4; s++) {
            for (int k = 0; k < 4; k += 2) {
                int a = zigzag_base[s][k];
                if (a == 1)
                    a = 2;
                else if (a == 2)
                    a = 1;
                zigzag_mirror[s][k] = a;
                zigzag_mirror[s][k + 1] = zigzag_base[s][k + 1];
            }
        }
        int vars[2][4][4];
        memcpy(vars[0], zigzag_base, sizeof(zigzag_base));
        memcpy(vars[1], zigzag_mirror, sizeof(zigzag_mirror));

        // Precompute coverage for all (variant, dir, i, j)
        // Store which cells each config covers
        struct ZConfig {
            int v, sd, si, sj;
            bool cov[N][N];
            int cover;
        };

        // Too many configs to store all, so use greedy:
        // Find best single zigzag, then find best complement
        int best1_v = -1, best1_sd = -1, best1_si = -1, best1_sj = -1, best1_cover = 0;
        bool best1_cov[N][N];

        for (int v = 0; v < 2; v++)
            for (int sd = 0; sd < 4; sd++)
                for (int si = 0; si < N; si++)
                    for (int sj = 0; sj < N; sj++) {
                        bool cov[N][N];
                        int cover = simulate(vars[v], 4, si, sj, sd, cov);
                        if (cover > best1_cover) {
                            best1_cover = cover;
                            best1_v = v;
                            best1_sd = sd;
                            best1_si = si;
                            best1_sj = sj;
                            memcpy(best1_cov, cov, sizeof(cov));
                        }
                    }

        // Now find best second robot to complement
        if (best1_cover < N * N) {
            int best_pair_uncov = N * N;
            int b2_v = -1, b2_sd = -1, b2_si = -1, b2_sj = -1;

            for (int v = 0; v < 2; v++)
                for (int sd = 0; sd < 4; sd++)
                    for (int si = 0; si < N; si++)
                        for (int sj = 0; sj < N; sj++) {
                            bool cov[N][N];
                            simulate(vars[v], 4, si, sj, sd, cov);
                            // Count uncovered cells (not covered by either robot)
                            int uncov = 0;
                            for (int i = 0; i < N; i++)
                                for (int j = 0; j < N; j++)
                                    if (!best1_cov[i][j] && !cov[i][j]) uncov++;
                            if (uncov < best_pair_uncov) {
                                best_pair_uncov = uncov;
                                b2_v = v;
                                b2_sd = sd;
                                b2_si = si;
                                b2_sj = sj;
                            }
                        }

            // Compute actual M for this pair
            bool combined[N][N];
            {
                bool cov2[N][N];
                simulate(vars[b2_v], 4, b2_si, b2_sj, b2_sd, cov2);
                for (int i = 0; i < N; i++)
                    for (int j = 0; j < N; j++)
                        combined[i][j] = best1_cov[i][j] || cov2[i][j];
            }
            int fb = segment_uncovered_cost(combined);
            int total = 8 + fb;  // 4 states x 2 robots + fallback

            if (total < best_M) {
                best_M = total;
                best_robots.clear();

                RobotOut r1;
                r1.m = 4;
                r1.si = best1_si;
                r1.sj = best1_sj;
                r1.sd = DIR_CHAR[best1_sd];
                r1.trans.resize(4);
                for (int s = 0; s < 4; s++)
                    r1.trans[s] = {vars[best1_v][s][0], vars[best1_v][s][1],
                                   vars[best1_v][s][2], vars[best1_v][s][3]};
                best_robots.push_back(r1);

                RobotOut r2;
                r2.m = 4;
                r2.si = b2_si;
                r2.sj = b2_sj;
                r2.sd = DIR_CHAR[b2_sd];
                r2.trans.resize(4);
                for (int s = 0; s < 4; s++)
                    r2.trans[s] = {vars[b2_v][s][0], vars[b2_v][s][1],
                                   vars[b2_v][s][2], vars[b2_v][s][3]};
                best_robots.push_back(r2);

                vector<RobotOut> fb_r;
                segment_for_uncovered(combined, fb_r);
                for (auto& fr : fb_r) best_robots.push_back(fr);
            }
        }
    }

    // ============================================================
    // Phase 2: SA starting from zigzag base (4-6 states) to handle wall cases
    // Also try pure random SA for larger state counts
    // ============================================================
    int aut_trans[1600][4];

    // SA with zigzag seed for small state counts (4-8)
    {
        int zigzag_base[4][4] = {
            {1, 1, 1, 1},
            {0, 3, 2, 2},
            {0, 0, 1, 2},
            {2, 2, 2, 1},
        };
        int zigzag_mirror[4][4];
        for (int s = 0; s < 4; s++) {
            for (int k = 0; k < 4; k += 2) {
                int a = zigzag_base[s][k];
                if (a == 1)
                    a = 2;
                else if (a == 2)
                    a = 1;
                zigzag_mirror[s][k] = a;
                zigzag_mirror[s][k + 1] = zigzag_base[s][k + 1];
            }
        }

        for (int num_states = 4; num_states <= 8 && elapsed_ms() < 1000; num_states++) {
            for (int restart = 0; restart < 200 && elapsed_ms() < 1000; restart++) {
                // Init with zigzag base + random extra states
                int base_var = restart % 2;
                for (int s = 0; s < 4; s++) {
                    if (base_var == 0)
                        memcpy(aut_trans[s], zigzag_base[s], sizeof(int) * 4);
                    else
                        memcpy(aut_trans[s], zigzag_mirror[s], sizeof(int) * 4);
                }
                for (int s = 4; s < num_states; s++) {
                    aut_trans[s][0] = rng() % 3;
                    aut_trans[s][1] = rng() % num_states;
                    aut_trans[s][2] = 1 + rng() % 2;
                    aut_trans[s][3] = rng() % num_states;
                }
                int si = rng() % N, sj = rng() % N, sd = rng() % 4;

                bool cov[N][N];
                simulate(aut_trans, num_states, si, sj, sd, cov);
                int fb_m = segment_uncovered_cost(cov);
                int total_m = num_states + fb_m;

                int best_trans[1600][4];
                memcpy(best_trans, aut_trans, sizeof(int) * 4 * num_states);
                int bsi = si, bsj = sj, bsd = sd;
                int best_total = total_m;

                double temp = 3.0;
                for (int it = 0; it < 3000 && elapsed_ms() < 1000; it++) {
                    int old_trans[1600][4];
                    memcpy(old_trans, aut_trans, sizeof(int) * 4 * num_states);
                    int osi = si, osj = sj, osd = sd;

                    int mv = rng() % 15;
                    if (mv < 5) {
                        int s = rng() % num_states, w = rng() % 2;
                        if (w == 0) {
                            if (rng() % 2)
                                aut_trans[s][0] = rng() % 3;
                            else
                                aut_trans[s][1] = rng() % num_states;
                        } else {
                            if (rng() % 2)
                                aut_trans[s][2] = 1 + rng() % 2;
                            else
                                aut_trans[s][3] = rng() % num_states;
                        }
                    } else if (mv < 8) {
                        int s = rng() % num_states;
                        aut_trans[s][0] = rng() % 3;
                        aut_trans[s][1] = rng() % num_states;
                        aut_trans[s][2] = 1 + rng() % 2;
                        aut_trans[s][3] = rng() % num_states;
                    } else if (mv < 11) {
                        si = rng() % N;
                        sj = rng() % N;
                        sd = rng() % 4;
                    } else {
                        int cnt = 2 + rng() % 2;
                        for (int c = 0; c < cnt; c++) {
                            int s = rng() % num_states, w = rng() % 2;
                            if (w == 0) {
                                if (rng() % 2)
                                    aut_trans[s][0] = rng() % 3;
                                else
                                    aut_trans[s][1] = rng() % num_states;
                            } else {
                                if (rng() % 2)
                                    aut_trans[s][2] = 1 + rng() % 2;
                                else
                                    aut_trans[s][3] = rng() % num_states;
                            }
                        }
                    }

                    bool nc[N][N];
                    simulate(aut_trans, num_states, si, sj, sd, nc);
                    int nfb = segment_uncovered_cost(nc);
                    int nt = num_states + nfb;

                    double delta = nt - total_m;
                    if (delta < 0 || (rng() % 10000 < 10000 * exp(-delta / temp))) {
                        total_m = nt;
                        if (nt < best_total) {
                            best_total = nt;
                            memcpy(best_trans, aut_trans, sizeof(int) * 4 * num_states);
                            bsi = si;
                            bsj = sj;
                            bsd = sd;
                        }
                    } else {
                        memcpy(aut_trans, old_trans, sizeof(int) * 4 * num_states);
                        si = osi;
                        sj = osj;
                        sd = osd;
                    }
                    temp *= 0.999;
                }

                if (best_total < best_M) {
                    best_M = best_total;
                    bool fcov[N][N];
                    simulate(best_trans, num_states, bsi, bsj, bsd, fcov);
                    best_robots.clear();
                    RobotOut mr;
                    mr.m = num_states;
                    mr.si = bsi;
                    mr.sj = bsj;
                    mr.sd = DIR_CHAR[bsd];
                    mr.trans.resize(num_states);
                    for (int s = 0; s < num_states; s++)
                        mr.trans[s] = {best_trans[s][0], best_trans[s][1], best_trans[s][2], best_trans[s][3]};
                    best_robots = {mr};
                    vector<RobotOut> fb;
                    segment_for_uncovered(fcov, fb);
                    for (auto& r : fb) best_robots.push_back(r);
                }
            }
        }
    }

    // Phase 3: Random SA (remaining time)
    for (int num_states = 2; num_states <= 30 && elapsed_ms() < 1900; num_states++) {
        int max_restarts = (num_states <= 10) ? 100 : 30;
        for (int restart = 0; restart < max_restarts && elapsed_ms() < 1900; restart++) {
            // Random init
            for (int s = 0; s < num_states; s++) {
                aut_trans[s][0] = rng() % 3;
                aut_trans[s][1] = rng() % num_states;
                aut_trans[s][2] = 1 + rng() % 2;
                aut_trans[s][3] = rng() % num_states;
            }
            int si = rng() % N, sj = rng() % N, sd = rng() % 4;

            bool cov[N][N];
            int cover = simulate(aut_trans, num_states, si, sj, sd, cov);
            int fb_m = segment_uncovered_cost(cov);
            int total_m = num_states + fb_m;

            int best_trans[1600][4];
            memcpy(best_trans, aut_trans, sizeof(int) * 4 * num_states);
            int bsi = si, bsj = sj, bsd = sd;
            int best_total = total_m;

            double temp = 5.0;
            int iters = (num_states <= 5) ? 5000 : 2000;

            for (int it = 0; it < iters && elapsed_ms() < 1900; it++) {
                int old_trans[1600][4];
                memcpy(old_trans, aut_trans, sizeof(int) * 4 * num_states);
                int osi = si, osj = sj, osd = sd;

                int mv = rng() % 20;
                if (mv < 5) {
                    // Single entry change
                    int s = rng() % num_states, w = rng() % 2;
                    if (w == 0) {
                        if (rng() % 2)
                            aut_trans[s][0] = rng() % 3;
                        else
                            aut_trans[s][1] = rng() % num_states;
                    } else {
                        if (rng() % 2)
                            aut_trans[s][2] = 1 + rng() % 2;
                        else
                            aut_trans[s][3] = rng() % num_states;
                    }
                } else if (mv < 8) {
                    // Reinitialize one state completely
                    int s = rng() % num_states;
                    aut_trans[s][0] = rng() % 3;
                    aut_trans[s][1] = rng() % num_states;
                    aut_trans[s][2] = 1 + rng() % 2;
                    aut_trans[s][3] = rng() % num_states;
                } else if (mv < 10) {
                    // Change multiple entries (2-3)
                    int cnt = 2 + rng() % 2;
                    for (int c = 0; c < cnt; c++) {
                        int s = rng() % num_states, w = rng() % 2;
                        if (w == 0) {
                            if (rng() % 2)
                                aut_trans[s][0] = rng() % 3;
                            else
                                aut_trans[s][1] = rng() % num_states;
                        } else {
                            if (rng() % 2)
                                aut_trans[s][2] = 1 + rng() % 2;
                            else
                                aut_trans[s][3] = rng() % num_states;
                        }
                    }
                } else if (mv < 13) {
                    // Change start position
                    si = rng() % N;
                    sj = rng() % N;
                    sd = rng() % 4;
                } else if (mv < 15 && num_states >= 2) {
                    // Swap two states
                    int s1 = rng() % num_states, s2 = rng() % num_states;
                    int tmp[4];
                    memcpy(tmp, aut_trans[s1], sizeof(tmp));
                    memcpy(aut_trans[s1], aut_trans[s2], sizeof(tmp));
                    memcpy(aut_trans[s2], tmp, sizeof(tmp));
                    for (int s = 0; s < num_states; s++)
                        for (int idx : {1, 3}) {
                            if (aut_trans[s][idx] == s1)
                                aut_trans[s][idx] = s2;
                            else if (aut_trans[s][idx] == s2)
                                aut_trans[s][idx] = s1;
                        }
                } else if (mv < 17) {
                    // Redirect all transitions pointing to state X to state Y
                    int x = rng() % num_states, y = rng() % num_states;
                    for (int s = 0; s < num_states; s++) {
                        if (aut_trans[s][1] == x) aut_trans[s][1] = y;
                        if (aut_trans[s][3] == x) aut_trans[s][3] = y;
                    }
                } else if (mv < 19) {
                    // Reinitialize 2-3 states
                    int cnt = 2 + rng() % 2;
                    for (int c = 0; c < cnt; c++) {
                        int s = rng() % num_states;
                        aut_trans[s][0] = rng() % 3;
                        aut_trans[s][1] = rng() % num_states;
                        aut_trans[s][2] = 1 + rng() % 2;
                        aut_trans[s][3] = rng() % num_states;
                    }
                } else {
                    // Start position + one entry change
                    si = rng() % N;
                    sj = rng() % N;
                    sd = rng() % 4;
                    int s = rng() % num_states;
                    aut_trans[s][0] = rng() % 3;
                    aut_trans[s][1] = rng() % num_states;
                    aut_trans[s][2] = 1 + rng() % 2;
                    aut_trans[s][3] = rng() % num_states;
                }

                bool nc[N][N];
                simulate(aut_trans, num_states, si, sj, sd, nc);
                int nfb = segment_uncovered_cost(nc);
                int nt = num_states + nfb;

                double delta = nt - total_m;
                if (delta < 0 || (rng() % 10000 < 10000 * exp(-delta / temp))) {
                    total_m = nt;
                    memcpy(cov, nc, sizeof(cov));
                    if (nt < best_total) {
                        best_total = nt;
                        memcpy(best_trans, aut_trans, sizeof(int) * 4 * num_states);
                        bsi = si;
                        bsj = sj;
                        bsd = sd;
                    }
                } else {
                    memcpy(aut_trans, old_trans, sizeof(int) * 4 * num_states);
                    si = osi;
                    sj = osj;
                    sd = osd;
                }

                temp *= 0.9995;
            }

            if (best_total < best_M) {
                best_M = best_total;
                // Reconstruct robots
                bool fcov[N][N];
                simulate(best_trans, num_states, bsi, bsj, bsd, fcov);
                best_robots.clear();
                RobotOut mr;
                mr.m = num_states;
                mr.si = bsi;
                mr.sj = bsj;
                mr.sd = DIR_CHAR[bsd];
                mr.trans.resize(num_states);
                for (int s = 0; s < num_states; s++)
                    mr.trans[s] = {best_trans[s][0], best_trans[s][1], best_trans[s][2], best_trans[s][3]};
                best_robots = {mr};
                vector<RobotOut> fb;
                segment_for_uncovered(fcov, fb);
                for (auto& r : fb) best_robots.push_back(r);
            }
        }
    }

    const char* as[] = {"F", "R", "L"};
    cout << best_robots.size() << "\n";
    for (auto& r : best_robots) {
        cout << r.m << " " << r.si << " " << r.sj << " " << r.sd << "\n";
        for (int s = 0; s < r.m; s++)
            cout << as[r.trans[s][0]] << " " << r.trans[s][1] << " "
                 << as[r.trans[s][2]] << " " << r.trans[s][3] << "\n";
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N - 1; j++) cout << '0';
        cout << "\n";
    }
    for (int i = 0; i < N - 1; i++) {
        for (int j = 0; j < N; j++) cout << '0';
        cout << "\n";
    }
    return 0;
}
