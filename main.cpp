#include <bits/stdc++.h>
using namespace std;
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

// 温度関数
#define TIME_LIMIT 2600

//-----------------以下から実装部分-----------------//

// 左上右下の順番
#define DIR_NUM 4
vector<int> dx = {0,-1,0,1};
vector<int> dy = {-1,0,1,0};

int n, m, res;
double eps;

struct Mino {
    int max_x, max_y;
    vector<P> p, valid_area;
    vector<vector<int>> pattern;

    Mino(): max_x(0), max_y(0), p({}) {}
    explicit Mino(const vector<P> &p): max_x(0), max_y(0), p(p) {
        pattern.assign(n, vector<int>(n, 0));
        for(auto &&[x, y] : p) {
            max_x = max(max_x, x);
            max_y = max(max_y, y);
        }
    }

    inline void updateMinoArea(const vector<vector<int>> &field){
        pattern.assign(n, vector<int>(n, 0));
        valid_area.clear();
        rep(sx,n-max_x) rep(sy,n-max_y) {
            bool ok = true;
            for(auto &&[x, y] : p) {
                if( field[sx+x][sy+y] != 0 ) continue;
                ok = false;
                break;
            }
            if( !ok ) continue;
            valid_area.emplace_back(P(sx,sy));
            for(auto &&[x, y] : p) pattern[sx+x][sy+y]++;
        }
        return;
    }
};

struct Solver {
    int oil_cnt;
    bool all_search;
    vector<Mino> minos;
    vector<vector<int>> oil, field, field_cnt;

    Solver() {
        this->input();
        utility::mytm.CodeStart();

        // oil[i][j] : (i,j) が -1:未探索, 0以上: 油田の数
        oil.assign(n, vector<int>(n, 1e9));
        oil_cnt = 0;
        all_search = false;
        // 初期状態の mino 配置場所 update
        rep(i,m) minos[i].updateMinoArea(oil);
        return;
    }

    void input(){
        cin >> n >> m >> eps;
        rep(i,m) {
            int d; cin >> d;
            vector<P> p;
            int max_x = 0, max_y = 0;
            rep(j,d) {
                int x, y; cin >> x >> y;
                p.emplace_back(P(x,y));
            }
            minos.emplace_back(Mino(p));
        }
        return;
    }

    void solve() {
        while( oil_cnt < n*n ) {
            // 占い part ( 最も選択肢を削減出来るマスを選択 )
            auto [sx, sy] = searchPoint(oil);
            if( oil[sx][sy] != 1e9 ) continue;
            cout << "q 1 " << sx << " " << sy << endl << flush;
            cin >> res;
            oil[sx][sy] = res;
            rep(i,m) minos[i].updateMinoArea(oil);
            oil_cnt++;

            // 枝狩りしても探索が無理そうなケースは continue
            long double all_cnt = 1;
            rep(i,m) {
                all_cnt *= minos[i].valid_area.size();
                if( all_cnt > 1e30 ) break;
            }
            if( all_cnt > 1e30 ) continue;

            // dfs で mino 配置列挙
            double start = utility::mytm.elapsed();
            double end = start + (TIME_LIMIT-start)/(n*n-oil_cnt);
            double now = start;

            all_search = true;
            vector<vector<P>> cand;
            field.assign(n, vector<int>(n, 0));

            auto dfs = [&](auto self, int idx) -> void {
                if( idx == m ) {
                    vector<P> ans;
                    bool flag = true;
                    rep(i,n) rep(j,n) {
                        if( field[i][j] > 0 ) ans.emplace_back(P(i,j));
                        if( oil[i][j] == 1e9 ) continue;
                        flag &= (field[i][j] == oil[i][j]);
                        if( !flag ) break;
                    }
                    if( !flag ) return;
                    cand.emplace_back(ans);
                    return;
                }
                for(auto &&[x, y] : minos[idx].valid_area) {
                    bool ok = true;
                    for(auto &&[dx, dy] : minos[idx].p) {
                        if( field[x+dx][y+dy] < oil[x+dx][y+dy] ) continue;
                        ok = false;
                        break;
                    }
                    if( !ok ) continue;
                    for(auto &&[dx, dy] : minos[idx].p) field[x+dx][y+dy]++;
                    self(self, idx+1);
                    for(auto &&[dx, dy] : minos[idx].p) field[x+dx][y+dy]--;

                    // timeover
                    if( now > end ) {
                        all_search = false;
                        return;
                    }
                    now = utility::mytm.elapsed();
                }
                return;
            };
            dfs(dfs, 0);
            if( !all_search ) continue;

            sort(cand.begin(), cand.end());
            cand.erase(unique(cand.begin(), cand.end()), cand.end());
            if( cand.size() > 5 ) continue;

            rep(i,cand.size()) {
                cout << "a " << cand[i].size() << " ";
                for(auto &&[x, y] : cand[i]) cout << x << " " << y << " ";
                cout << endl << flush;
                cin >> res;
                if( res == 1 ) return;
            }
        }

        // 最後まで分からなかった場合 ⇒ 結果から答えを出力
        if( oil_cnt == n*n ) {
            vector<P> ans;
            rep(i,n) rep(j,n) if( oil[i][j] > 0 ) ans.emplace_back(P(i,j));
            cout << "a " << ans.size() << " ";
            for(auto &&[x, y] : ans) cout << x << " " << y << " ";
            cout << endl << flush;
            cin >> res;
        }
        return;
    }

    inline P searchPoint(const vector<vector<int>> &oil) {
        // 各マスにおいて置ける or 置けない時の差分が小さい ( 情報を得ると選択肢を大幅削減出来る ) マスを選択
        // 1. 配置全列挙出来る時 ⇒ dfs で全探索
        // 2. 配置全列挙出来ない時 ⇒ 各ミノの置き方の総積 (overflow しないよう対数の和で計算)
        double min_score = 1e9, score1, score2;
        P res;
        if( !all_search ) {
            rep(i,n) rep(j,n) {
                if( oil[i][j] != 1e9 ) continue;
                score1 = 0.0, score2 = 0.0;
                rep(k,m) {
                    score1 += log(minos[k].pattern[i][j]+1);
                    score2 += log(minos[k].valid_area.size()-minos[k].pattern[i][j]+1);
                }
                if( min_score > abs(score1-score2) ) {
                    min_score = abs(score1-score2);
                    res = P(i,j);
                }
            }
        }
        else {
            long long total = 0;
            field.assign(n, vector<int>(n, 0));
            field_cnt.assign(n, vector<int>(n, 0));
            
            auto dfs = [&](auto self, int idx) -> void {
                if( idx == m ) {
                    vector<P> ans;
                    bool flag = true;
                    rep(i,n) {
                        rep(j,n) {
                            if( field[i][j] > 0 ) ans.emplace_back(P(i,j));
                            if( oil[i][j] == 1e9 ) continue;
                            flag &= (field[i][j] == oil[i][j]);
                            if( !flag ) break;
                        }
                        if( !flag ) break;
                    }
                    if( !flag ) return;
                    
                    rep(i,n) rep(j,n) field_cnt[i][j] += (field[i][j] > 0);
                    total++;
                    return;
                }
                for(auto &&[x, y] : minos[idx].valid_area) {
                    bool mino_placeable = true;
                    for(auto &&[dx, dy] : minos[idx].p) {
                        mino_placeable &= (field[x+dx][y+dy] < oil[x+dx][y+dy]);
                        if( !mino_placeable ) break;
                    }
                    if( !mino_placeable ) continue;
                    for(auto &&[dx, dy] : minos[idx].p) field[x+dx][y+dy]++;
                    self(self, idx+1);
                    for(auto &&[dx, dy] : minos[idx].p) field[x+dx][y+dy]--;
                }
                return;
            };

            dfs(dfs, 0);
            rep(i,n) rep(j,n) {
                if( oil[i][j] != 1e9 ) continue;
                double score1 = 0, score2 = 0;
                rep(k,m) {
                    score1 += log(field_cnt[i][j]+1);
                    score2 += log(total-field_cnt[i][j]+1);
                }
                if( min_score > abs(score1-score2) ) {
                    min_score = abs(score1-score2);
                    res = P(i,j);
                }
            }
        }
        return res;
    }

    inline bool outField(int x,int y,int h,int w){
        if(0 <= x && x < h && 0 <= y && y < w)return false;
        return true;
    }
};

int main(){
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.solve();
    
    return 0;
}
