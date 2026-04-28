#include <bits/stdc++.h>
using namespace std;
#if __has_include(<atcoder/all>)
    #include <atcoder/all>
using namespace atcoder;
#endif
typedef long long ll;
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

inline unsigned int rand_int() {
    static unsigned int tx = 123456789, ty=362436069, tz=521288629, tw=88675123;
    unsigned int tt = (tx^(tx<<11));
    tx = ty; ty = tz; tz = tw;
    return ( tw=(tw^(tw>>19))^(tt^(tt>>8)) );
}

inline double rand_double() {
    return (double)(rand_int()%(int)1e9)/1e9;
}

inline double gaussian(double mean, double stddev) {
    // 標準正規分布からの乱数生成（Box-Muller変換
    double z0 = sqrt(-2.0 * log(rand_double())) * cos(2.0 * M_PI * rand_double());
    // 平均と標準偏差の変換
    return mean + z0 * stddev;
}

// 温度関数
#define TIME_LIMIT 1800
inline double temp(double start) {
    double start_temp = 0.1,end_temp = 0.01;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now,int start) {
    return exp((double)(best - now) / temp(start));
}

//-----------------以下から実装部分-----------------//

struct Solver{
    int n, m, sx, sy;
    vector<string> key;
    vector<string> word;
    vector<vector<P>> place;
    vector<P> ans;
    vector<vector<int>> score;

    Solver() {
        // timer start
        utility::mytm.CodeStart();

        this->input();
        place.assign(26,{});
        rep(i,n) rep(j,n) place[key[i][j]-'A'].push_back({i,j});
        score.assign(m, vector<int>(m, 0));

        int cnt = 0;
        rep(i,m) rep(j,m) {
            // 先頭と尻の一致度合いを計算
            for(int k=5; k>=1; k--) {
                bool flag = true;
                for(int l=0; l<k; l++) flag &= (word[i][5-k+l] == word[j][l]);
                if( flag ) {
                    score[i][j] = k;
                    break;
                }
            }
        }
        return;
    }

    void input() {
        cin >> n >> m >> sx >> sy;
        key.assign(n, "");
        word.assign(m, "");
        rep(i,n) cin >> key[i];
        rep(i,m) cin >> word[i];
        return;
    }

    void output() {
        rep(i,ans.size()) {
            auto [x, y] = ans[i];
            cout << x << " " << y << endl;
        }
        return;
    }

    void solve() {
        // 文字生成 part
        // ~~ 省略度が高いものから貪欲に結合 ~~

        string ans_str = createString();
        auto [best_score, best] = calcScore(ans_str);
        cerr << "First Score: " << 10000-best_score << endl;

        int itration = 0;
        while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            string cand_str = createString();
            auto [cand_score, cand] = calcScore(cand_str);
            if( best_score > cand_score ) {
                best_score = cand_score;
                ans_str = cand_str;
                ans = cand;
            }
            itration++;
        }
        cerr << "Last Score: " << 10000-best_score << endl;
        cerr << "Itaration: " << itration << endl;
        return;
    }

    inline string createString() {
        dsu uf(m);
        vector<int> head(m,-1), tail(m,-1);
        rep(i,m-1) {
            int best_score = -1;
            vector<P> cand;
            rep(j,m) rep(k,m) {
                if( j == k || tail[j] != -1 || head[k] != -1 || uf.same(j,k) ) continue;
                if( best_score < score[j][k] ) {
                    cand.clear();
                    cand.push_back({j,k});
                    best_score = score[j][k];
                }
                else if( best_score == score[j][k] ) {
                    cand.push_back({j,k});
                }
            }
            auto [best_j, best_k] = cand[rand_int()%cand.size()];
            tail[best_j] = best_k;
            head[best_k] = best_j;
            uf.merge(best_j, best_k);
        }
        int now = 0;
        while( head[now] != -1 ) now = head[now];
        string ans_str = word[now];
        vector<int> idx;
        while( tail[now] != -1 ) {
            int cnt = score[now][tail[now]];
            while( cnt-- ) ans_str.pop_back();
            ans_str += word[tail[now]];
            idx.push_back(now);
            now = tail[now];
        }
        idx.push_back(now);
        return ans_str;
    }

    inline pair<int, vector<P>> calcScore(const string &str) {
        int tsx = sx, tsy = sy;
        vector<P> cand;

        // dp[i][j][k] : i文字目まで見て マス (j,k) にいるときの最小コスト
        vector<vector<vector<int>>> dp(str.size()+1, vector<vector<int>>(n, vector<int>(n, 1e9))), pre(str.size()+1, vector<vector<int>>(n, vector<int>(n, -1)));
        dp[0][sx][sy] = 0;
        rep(i,str.size()) {
            rep(j,n) rep(k,n) {
                if( dp[i][j][k] == 1e9 ) continue;
                for(auto [nx, ny]: place[str[i]-'A']) {
                    if( dp[i+1][nx][ny] > dp[i][j][k] + abs(nx-j) + abs(ny-k) ) {
                        dp[i+1][nx][ny] = dp[i][j][k] + abs(nx-j) + abs(ny-k) + 1;
                        pre[i+1][nx][ny] = j*n+k;
                    }
                }
            }
        }
        // 一番最小コストのマスから pre を用いて復元
        int best_score = 1e9;
        rep(i,n) rep(j,n) {
            if( best_score > dp[str.size()][i][j] ) {
                best_score = dp[str.size()][i][j];
                tsx = i;
                tsy = j;
            }
        }
        for(int i=str.size(); i>=1; i--) {
            int prex = pre[i][tsx][tsy]/n;
            int prey = pre[i][tsx][tsy]%n;
            cand.push_back({tsx, tsy});
            tsx = prex;
            tsy = prey;
        }
        reverse(cand.begin(), cand.end());
        return {best_score, cand};
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