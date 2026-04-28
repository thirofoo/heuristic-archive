#include <bits/stdc++.h>
using namespace std;
typedef pair<int, int> P;
typedef tuple<int, int, int, int> T;
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
            return (double)duration_cast<milliseconds>(system_clock::now() - start).count();
        }
    } mytm;
}

#define TIME_LIMIT 950
//-----------------以下から実装部分-----------------//

static constexpr int MOD = 998244353;
int n, m, k;

// 下行・右列の最適化時の最大の Stamp の数
int MAX_OPT = 3;

struct Answer {
    long long score;
    vector<tuple<int,int,int>> stamps;
    vector<vector<int>> now_a;

    Answer() : score(0), stamps({}) {}
    explicit Answer(long long score, vector<tuple<int,int,int>> stamps, vector<vector<int>> now_a) : score(score), stamps(stamps), now_a(now_a) {}

    bool operator < (const Answer &a) const {return score < a.score;}
    bool operator <= (const Answer &a) const {return score <= a.score;}
    bool operator > (const Answer &a) const {return score > a.score;}
    bool operator >= (const Answer &a) const {return score >= a.score;}
    bool operator == (const Answer &a) const {return score == a.score;}
};

struct Stamp {
    int op, op_num;
    vector<int> stamp_idx;
    vector<vector<int>> stamp;

    Stamp() : op(-1), stamp({}) {}
    Stamp(vector<int> idx, vector<vector<int>> stamp) : stamp_idx(idx), stamp(stamp) , op(0) {}
    Stamp(int op, vector<vector<int>> stamp) : op(op), stamp(stamp) { 
        op_num = __builtin_popcount(op);
        rep(i,m) {
            if( !((op >> i) & 1) ) continue;
            stamp_idx.emplace_back(i);
        }
    }
};

struct Solver{
    int rest;
    vector<vector<int>> a;
    vector<vector<vector<int>>> stamp;

    vector<Stamp> all_stamps, opt_stamps;
    priority_queue<Answer> pq;

    vector<pair<long long, Stamp>> bottom_stamp, right_stamp;

    Answer now;

    Solver() {
        this->input();
        rest = k;
        vector<vector<int>> s(3, vector<int>(3));

        // 最大で 5 個の Stamp を重ねたものを全列挙
        auto dfs = [&](auto self, int cnt, vector<vector<int>> v, vector<int> idx) -> void {
            if( cnt == 3 ) return;
            rep(i,m) {
                idx.emplace_back(i);
                rep(x,3) rep(y,3) {
                    v[x][y] += stamp[i][x][y];
                    v[x][y] %= MOD;
                }

                all_stamps.emplace_back(Stamp(idx, v));
                self(self, cnt+1, v, idx);

                idx.pop_back();
                rep(x,3) rep(y,3) {
                    v[x][y] -= stamp[i][x][y];
                    v[x][y] += MOD;
                    v[x][y] %= MOD;
                }
            }
            return;
        };
        vector<int> tidx;
        vector<vector<int>> ts(3, vector<int>(3,0));
        dfs(dfs, 0, ts, tidx);

        // 最大 10 個 Stamp を重ねたものを全列挙
        // rep(i, (1LL << m)) {
        //     s.assign(3, vector<int>(3, 0));
        //     rep(j,m) {
        //         if( !(i & (1LL << j)) ) continue;
        //         rep(x,3) rep(y,3) {
        //             s[x][y] += stamp[j][x][y];
        //             s[x][y] %= MOD;
        //         }
        //     }
        //     all_stamps.emplace_back(Stamp(i, s));
        //     // 微調整用の Stamp 配列
        //     if( __builtin_popcount(i) <= MAX_OPT-1 ) opt_stamps.emplace_back(Stamp(i, s));
        // }
        return;
    }

    void input() {
        cin >> n >> m >> k;  
        a.resize(n, vector<int>(n));      
        rep(i,n) rep(j,n) cin >> a[i][j];
        rep(i,m) {
            vector<vector<int>> s(3, vector<int>(3));
            rep(j,3) rep(k,3) cin >> s[j][k];
            stamp.emplace_back(s);
        }
        return;
    }

    void output() {
        Answer ans = now;
        cout << ans.stamps.size() << endl;
        for(auto [x,y,z] : ans.stamps) cout << x << " " << y << " " << z << endl;
        return;
    }

    void solve() {
        utility::mytm.CodeStart();

        // ==================== 貪欲解法 ====================
        // 1. 左上から 1 Stamp で一番スコアが高くなるものを押す
        // 2. 右と下の 3 列を最小化するように Stamp を押す
        // 3. 一番右下とその上・左の 3 つの 3 x 3 を最大化する Stamp を探索
        // =================================================

        op1();
        if( rest == 0 ) return;
        
        op2();
        if( rest == 0 ) return;

        op3();

        return;
    }

    void op1() {
        // 1. 左上から 1 Stamp で一番スコアが高くなるものを押す
        // ========== 左上の最適化をビームサーチで行う ========== //
        now = Answer(0, {}, a);

        int beam_width = 100;
        priority_queue<Answer, vector<Answer>, greater<Answer>> beam_search;
        beam_search.push(now);

        rep(i,n-3) rep(j,n-3) {
            priority_queue<Answer, vector<Answer>, greater<Answer>> next_beam_search;
            while( !beam_search.empty() ) {
                Answer cand = beam_search.top(); beam_search.pop();

                cand.score += cand.now_a[i][j];
                next_beam_search.push(cand); // Stamp を使わないケース
                cand.score -= cand.now_a[i][j];

                // m 個の stamp 全探索して次のビームサーチに追加
                rep(idx,m) {
                    Answer next = cand;
                    next.stamps.emplace_back(tuple(idx,i,j));
                    rep(x,3) rep(y,3) {
                        next.now_a[i+x][j+y] += stamp[idx][x][y];
                        next.now_a[i+x][j+y] %= MOD;
                    }
                    next.score += next.now_a[i][j];
                    next_beam_search.push(next);
                    if( next_beam_search.size() > beam_width ) next_beam_search.pop();
                }
            }
            swap(beam_search, next_beam_search);
        }

        if( !beam_search.empty() ) {
            now = beam_search.top();
            a = now.now_a;
            rest -= now.stamps.size();
        }
        else cerr << "No Answer\n";
        return;
    }

    void op2() {
        // 2. 右と下の最適化
        // ========== 右 3 列の最適化 ========== //
        rep(i,n-3) {
            long long max_v = 0;
            rep(y,3) max_v += a[i][n-3+y];
            Stamp best_stp = Stamp();

            for(auto &&stp: all_stamps) {
                if( stp.op_num > MAX_OPT ) continue;

                long long tmp = 0;
                rep(y,3) tmp += (a[i][n-3+y] + stp.stamp[0][y]) % MOD;
                if( max_v < tmp ) {
                    max_v = tmp;
                    best_stp = stp;
                }
                if( i == n-4 ) right_stamp.emplace_back(pair(tmp, stp));
            }
            if( i == n-4 ) continue;
            if( best_stp.op == -1 ) continue;
            for(auto stp_idx: best_stp.stamp_idx) {
                now.stamps.emplace_back(tuple(stp_idx,i,n-3));
                rep(x,3) rep(y,3) {
                    now.score -= a[i+x][n-3+y];
                    a[i+x][n-3+y] += stamp[stp_idx][x][y];
                    a[i+x][n-3+y] %= MOD;
                    now.score += a[i+x][n-3+y];
                }
                rest--;
                if( rest == 0 ) return;
            }
        }

        // ========== 下 3 列の最適化 ========== //
        rep(j,n-3) {
            long long max_v = 0;
            rep(x,3) max_v += a[n-3+x][j];
            Stamp best_stp = Stamp();

            for(auto &&stp: all_stamps) {
                if( stp.op_num > MAX_OPT ) continue;

                long long tmp = 0;
                rep(x,3) tmp += (a[n-3+x][j] + stp.stamp[x][0]) % MOD;
                if( max_v < tmp ) {
                    max_v = tmp;
                    best_stp = stp;
                }
                if( j == n-4 ) bottom_stamp.emplace_back(pair(tmp, stp));
            }
            if( j == n-4 ) continue;
            if( best_stp.op == -1 ) continue;
            for(auto stp_idx: best_stp.stamp_idx) {
                now.stamps.emplace_back(tuple(stp_idx,n-3,j));
                rep(x,3) rep(y,3) {
                    now.score -= a[n-3+x][j+y];
                    a[n-3+x][j+y] += stamp[stp_idx][x][y];
                    a[n-3+x][j+y] %= MOD;
                    now.score += a[n-3+x][j+y];
                }
                rest--;
                if( rest == 0 ) return;
            }
        }
        pq.push(now);
        return;
    }

    void op3() {
        // 3. 一番右下とその上・左の 3 つの 3 x 3 を最大化する Stamp を探索
        long long max_v = 0;
        rep(x,3) rep(y,3) max_v += a[n-3+x][n-3+y];
        Stamp best_stp1 = Stamp(), best_stp2 = Stamp(), best_stp3 = Stamp();

        // ========== 初めに左・上の置き方を全探索 ========== //
        sort(bottom_stamp.begin(), bottom_stamp.end(), [](auto &a, auto &b) {return a.first > b.first;});
        sort(right_stamp.begin(), right_stamp.end(), [](auto &a, auto &b) {return a.first > b.first;});

        if( rest-2 < m ) {
            // all_stamps を改変して計算量を減らす
            vector<Stamp> tmp_stamps;
            for(auto &&stp: all_stamps) {
                if( stp.op_num > rest ) continue;
                tmp_stamps.emplace_back(stp);
            }
            swap(all_stamps, tmp_stamps);
        }

        // それぞれ上位 10 通りを全探索
        rep(bs,min(3,(int)bottom_stamp.size())) rep(rs,min(3,(int)right_stamp.size())) {
            Stamp stp1 = bottom_stamp[bs].second, stp2 = right_stamp[rs].second;
            vector<vector<int>> now_a = a;
            rep(x,3) rep(y,3) now_a[n-3+x][n-4+y] = (now_a[n-3+x][n-4+y] + stp1.stamp[x][y]) % MOD;
            rep(x,3) rep(y,3) now_a[n-4+x][n-3+y] = (now_a[n-4+x][n-3+y] + stp2.stamp[x][y]) % MOD;

            int now_rest = rest - stp1.op_num - stp2.op_num;
            if( now_rest < 0 ) continue;

            // ここで一番右下の Stamp を全探索
            for(auto &&stp: all_stamps) {
                if( now_rest < stp.op_num ) continue;
                long long tmp = 0;
                rep(x,3) rep(y,3) tmp += (now_a[n-3+x][n-3+y] + stp.stamp[x][y]) % MOD;
                if( max_v < tmp ) {
                    max_v = tmp;
                    best_stp1 = stp1;
                    best_stp2 = stp2;
                    best_stp3 = stp;
                }
            }
        }
        
        if( best_stp1.op != -1 ) {
            rest -= best_stp1.op_num + best_stp2.op_num + best_stp3.op_num;
            for(auto stp_idx: best_stp1.stamp_idx) {
                now.stamps.emplace_back(tuple(stp_idx,n-3,n-4));
                rep(x,3) rep(y,3) {
                    now.score -= a[n-3+x][n-4+y];
                    a[n-3+x][n-4+y] += stamp[stp_idx][x][y];
                    a[n-3+x][n-4+y] %= MOD;
                    now.score += a[n-3+x][n-4+y];
                }
            }
            for(auto stp_idx: best_stp2.stamp_idx) {
                now.stamps.emplace_back(tuple(stp_idx,n-4,n-3));
                rep(x,3) rep(y,3) {
                    now.score -= a[n-4+x][n-3+y];
                    a[n-4+x][n-3+y] += stamp[stp_idx][x][y];
                    a[n-4+x][n-3+y] %= MOD;
                    now.score += a[n-4+x][n-3+y];
                }
            }
            for(auto stp_idx: best_stp3.stamp_idx) {
                now.stamps.emplace_back(tuple(stp_idx,n-3,n-3));
                rep(x,3) rep(y,3) {
                    now.score -= a[n-3+x][n-3+y];
                    a[n-3+x][n-3+y] += stamp[stp_idx][x][y];
                    a[n-3+x][n-3+y] %= MOD;
                    now.score += a[n-3+x][n-3+y];
                }
            }
            pq.push(now);
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