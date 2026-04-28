#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

const int N = 20;
int AK, AM, AW;
string vwalls[N];
string hwalls[N - 1];

// U=0, R=1, D=2, L=3
const int DI[] = {-1, 0, 1, 0};
const int DJ[] = {0, 1, 0, -1};
const char DIR_CHAR[] = {'U', 'R', 'D', 'L'};

// 壁判定の事前計算テーブル
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

// ロボット出力用構造体
// trans[s] = {壁なし行動, 壁なし遷移先, 壁あり行動, 壁あり遷移先}
// 行動: 0=F(前進), 1=R(右回転), 2=L(左回転)
struct RobotOut {
    int m, si, sj;
    char sd;
    vector<array<int, 4>> trans;
};

// ヘルパー: ロボット出力を生成
RobotOut make_robot(const int tmpl[][4], int m, int si, int sj, int sd) {
    RobotOut r;
    r.m = m;
    r.si = si;
    r.sj = sj;
    r.sd = DIR_CHAR[sd];
    r.trans.resize(m);
    for (int s = 0; s < m; s++)
        r.trans[s] = {tmpl[s][0], tmpl[s][1], tmpl[s][2], tmpl[s][3]};
    return r;
}

// セグメント分解: 壁で区切られた行/列ごとに往復ロボットを配置
// 行分解と列分解のうちM合計が小さい方を採用
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
                        r.si = start; r.sj = j; r.sd = 'D';
                        if (len == 1) { r.m = 1; r.trans = {{1, 0, 1, 0}}; }
                        else { r.m = 2; r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}}; }
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
                        r.si = i; r.sj = start; r.sd = 'R';
                        if (len == 1) { r.m = 1; r.trans = {{1, 0, 1, 0}}; }
                        else { r.m = 2; r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}}; }
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
    if (rm <= cm) { robots = rr; return rm; }
    robots = cr; return cm;
}

// 未カバーマスのセグメント分解コスト (M合計のみ返す高速版)
int segment_uncovered_cost(const bool covered[N][N]) {
    int row_m = 0;
    for (int i = 0; i < N; i++) {
        int start = 0;
        for (int j = 0; j < N; j++) {
            bool is_end = (j == N - 1) || (vwalls[i][j] == '1');
            if (is_end) {
                bool need = false;
                for (int jj = start; jj <= j; jj++)
                    if (!covered[i][jj]) { need = true; break; }
                if (need) row_m += (j - start + 1 == 1) ? 1 : 2;
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
                    if (!covered[ii][j]) { need = true; break; }
                if (need) col_m += (i - start + 1 == 1) ? 1 : 2;
                start = i + 1;
            }
        }
    }
    return min(row_m, col_m);
}

// 未カバーマスのセグメント分解 (ロボットも構築する版)
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
                            RobotOut r;
                            r.si = start; r.sj = j; r.sd = 'D';
                            int len = i - start + 1;
                            if (len == 1) { r.m = 1; r.trans = {{1, 0, 1, 0}}; }
                            else { r.m = 2; r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}}; }
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
                            RobotOut r;
                            r.si = i; r.sj = start; r.sd = 'R';
                            int len = j - start + 1;
                            if (len == 1) { r.m = 1; r.trans = {{1, 0, 1, 0}}; }
                            else { r.m = 2; r.trans = {{0, 0, 1, 1}, {1, 0, 1, 0}}; }
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
    if (rm <= cm) { robots = rr; return rm; }
    robots = cr; return cm;
}

// オートマトンシミュレーション
// 周期サイクルに入ったマスのみをカバーとしてカウントする
// タイムスタンプ方式で毎回のクリアを不要にしている
static int sim_vis[N * N * 4 * 1601];
static int sim_ts = 0;

int simulate(const int trans[][4], int m, int si, int sj, int sd, bool cov[N][N]) {
    int total = N * N * 4 * m;
    sim_ts++;
    int ts = sim_ts;
    int ci = si, cj = sj, cd = sd, cs = 0;

    static int route_i[N * N * 4 * 1601 + 1];
    static int route_j[N * N * 4 * 1601 + 1];
    int rlen = 0;

    for (int t = 0; t <= total; t++) {
        int key = ((ci * N + cj) * 4 + cd) * m + cs;
        if (sim_vis[key] == ts) {
            // サイクル検出: 再シミュレーションでサイクル開始位置を特定
            int ci2 = si, cj2 = sj, cd2 = sd, cs2 = 0;
            int cycle_start = 0;
            for (int k = 0; k < t; k++) {
                int k2 = ((ci2 * N + cj2) * 4 + cd2) * m + cs2;
                if (k2 == key) { cycle_start = k; break; }
                int w = wall_cache[ci2][cj2][cd2] ? 1 : 0;
                int a = trans[cs2][w * 2], ns = trans[cs2][w * 2 + 1];
                if (a == 0) { ci2 += DI[cd2]; cj2 += DJ[cd2]; }
                else if (a == 1) cd2 = (cd2 + 1) % 4;
                else cd2 = (cd2 + 3) % 4;
                cs2 = ns;
            }
            // サイクル部分のカバーマスを集計
            int cnt = 0;
            memset(cov, 0, sizeof(bool) * N * N);
            for (int k = cycle_start; k < rlen; k++)
                if (!cov[route_i[k]][route_j[k]]) {
                    cov[route_i[k]][route_j[k]] = true;
                    cnt++;
                }
            return cnt;
        }
        sim_vis[key] = ts;
        route_i[rlen] = ci;
        route_j[rlen] = cj;
        rlen++;

        int w = wall_cache[ci][cj][cd] ? 1 : 0;
        int a = trans[cs][w * 2], ns = trans[cs][w * 2 + 1];
        if (a == 0) { ci += DI[cd]; cj += DJ[cd]; }
        else if (a == 1) cd = (cd + 1) % 4;
        else cd = (cd + 3) % 4;
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

    int best_M = INT_MAX;
    vector<RobotOut> best_robots;
    {
        vector<RobotOut> seg;
        best_M = segment_solve(seg);
        best_robots = seg;
    }

    // ジグザグ4状態オートマトン
    // 動作: 直進→壁で折り返し→隣の列/行へ移動→逆向き直進 を繰り返す
    // 鏡像 (R↔L入替) で左右反転パターンも試す
    // 開始方向を4通り試すことで縦横の蛇行を全てカバー
    int zigzag_base[4][4] = {
        {1, 1, 1, 1},  // S0: 常にR → S1 (方向転換の起点)
        {0, 3, 2, 2},  // S1: 壁なし→F,S3 / 壁あり→L,S2 (直進 or 折り返し)
        {0, 0, 1, 2},  // S2: 壁なし→F,S0 / 壁あり→R,S2 (隣の列へ移動)
        {2, 2, 2, 1},  // S3: 壁なし→L,S2 / 壁あり→L,S1 (方向調整)
    };

    // 鏡像: R(1)↔L(2) を入れ替え
    int zigzag_mirror[4][4];
    for (int s = 0; s < 4; s++)
        for (int k = 0; k < 4; k += 2) {
            int a = zigzag_base[s][k];
            if (a == 1) a = 2; else if (a == 2) a = 1;
            zigzag_mirror[s][k] = a;
            zigzag_mirror[s][k + 1] = zigzag_base[s][k + 1];
        }

    int variants[2][4][4];
    memcpy(variants[0], zigzag_base, sizeof(zigzag_base));
    memcpy(variants[1], zigzag_mirror, sizeof(zigzag_mirror));

    // 1台で全マスカバーを試みる (M=4)
    for (int v = 0; v < 2; v++)
        for (int sd = 0; sd < 4; sd++)
            for (int si = 0; si < N; si++)
                for (int sj = 0; sj < N; sj++) {
                    bool cov[N][N];
                    int cover = simulate(variants[v], 4, si, sj, sd, cov);
                    if (cover == N * N) {
                        if (4 < best_M) {
                            best_M = 4;
                            best_robots = {make_robot(variants[v], 4, si, sj, sd)};
                        }
                    } else {
                        int fb = segment_uncovered_cost(cov);
                        if (4 + fb < best_M) {
                            best_M = 4 + fb;
                            best_robots = {make_robot(variants[v], 4, si, sj, sd)};
                            bool cov2[N][N];
                            memcpy(cov2, cov, sizeof(cov));
                            vector<RobotOut> fb_r;
                            segment_for_uncovered(cov2, fb_r);
                            for (auto& fr : fb_r) best_robots.push_back(fr);
                        }
                    }
                }

    // 2台ペアで補完を試みる (M=8)
    // 貪欲: カバー数最大の1台目を選び、残りを最も埋める2台目を選ぶ
    if (best_M > 8) {
        int best1_v = -1, best1_sd = -1, best1_si = -1, best1_sj = -1, best1_cover = 0;
        bool best1_cov[N][N];

        for (int v = 0; v < 2; v++)
            for (int sd = 0; sd < 4; sd++)
                for (int si = 0; si < N; si++)
                    for (int sj = 0; sj < N; sj++) {
                        bool cov[N][N];
                        int cover = simulate(variants[v], 4, si, sj, sd, cov);
                        if (cover > best1_cover) {
                            best1_cover = cover;
                            best1_v = v; best1_sd = sd; best1_si = si; best1_sj = sj;
                            memcpy(best1_cov, cov, sizeof(cov));
                        }
                    }

        if (best1_cover < N * N) {
            int best_pair_uncov = N * N;
            int b2_v = -1, b2_sd = -1, b2_si = -1, b2_sj = -1;

            for (int v = 0; v < 2; v++)
                for (int sd = 0; sd < 4; sd++)
                    for (int si = 0; si < N; si++)
                        for (int sj = 0; sj < N; sj++) {
                            bool cov[N][N];
                            simulate(variants[v], 4, si, sj, sd, cov);
                            int uncov = 0;
                            for (int i = 0; i < N; i++)
                                for (int j = 0; j < N; j++)
                                    if (!best1_cov[i][j] && !cov[i][j]) uncov++;
                            if (uncov < best_pair_uncov) {
                                best_pair_uncov = uncov;
                                b2_v = v; b2_sd = sd; b2_si = si; b2_sj = sj;
                            }
                        }

            bool combined[N][N];
            {
                bool cov2[N][N];
                simulate(variants[b2_v], 4, b2_si, b2_sj, b2_sd, cov2);
                for (int i = 0; i < N; i++)
                    for (int j = 0; j < N; j++)
                        combined[i][j] = best1_cov[i][j] || cov2[i][j];
            }
            int fb = segment_uncovered_cost(combined);
            if (8 + fb < best_M) {
                best_M = 8 + fb;
                best_robots = {
                    make_robot(variants[best1_v], 4, best1_si, best1_sj, best1_sd),
                    make_robot(variants[b2_v], 4, b2_si, b2_sj, b2_sd),
                };
                vector<RobotOut> fb_r;
                segment_for_uncovered(combined, fb_r);
                for (auto& fr : fb_r) best_robots.push_back(fr);
            }
        }
    }

    // 出力
    const char* act_str[] = {"F", "R", "L"};
    cout << best_robots.size() << "\n";
    for (auto& r : best_robots) {
        cout << r.m << " " << r.si << " " << r.sj << " " << r.sd << "\n";
        for (int s = 0; s < r.m; s++)
            cout << act_str[r.trans[s][0]] << " " << r.trans[s][1] << " "
                 << act_str[r.trans[s][2]] << " " << r.trans[s][3] << "\n";
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