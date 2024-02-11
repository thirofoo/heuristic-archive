#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef pair<int, int> P;
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

// 温度関数
#define TIME_LIMIT 2800

//-----------------以下から実装部分-----------------//

// 左上右下の順番
#define DIR_NUM 4
vector<int> dx = {0,-1,0,1};
vector<int> dy = {-1,0,1,0};

int n, m, res;
double eps;

struct Mino {
    int max_x, max_y;
    vector<P> p;
    vector<vector<P>> valid_minos;
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
        valid_minos.clear();
        pattern.assign(n, vector<int>(n, 0));
        rep(sx,n-max_x) rep(sy,n-max_y) {
            bool ok = true;
            for(auto &&[x, y] : p) ok &= (field[sx+x][sy+y] != 0);
            if( !ok ) continue;

            valid_minos.emplace_back(vector<P>{});
            for(auto &&[x, y] : p) {
                pattern[sx+x][sy+y]++;
                valid_minos.back().emplace_back(P(sx+x,sy+y));
            }
        }
        return;
    }
};

struct Solver {
    int oil_cnt;
    bool all_search, mino_placable;
    vector<P> oil_place;
    vector<Mino> minos;
    vector<vector<int>> oil, field_cnt;

    double start, end, now;
    vector<vector<P>> cand;

    vector<vector<int>> fixed_field, field;
    vector<bool> fixed_mino;

    Solver() {
        this->input();
        utility::mytm.CodeStart();

        // oil[i][j] : (i,j) が -1:未探索, 0以上: 油田の数
        oil.assign(n, vector<int>(n, 1e9));
        oil_cnt = 0;
        all_search = false;

        // 初期状態の mino 配置場所 update
        rep(i,m) minos[i].updateMinoArea(oil);
        sort(minos.begin(), minos.end(), [](const Mino &a, const Mino &b) {
            return a.p.size() > b.p.size();
        });

        fixed_field.assign(n, vector<int>(n, 0));
        fixed_mino.assign(m, false);
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
        auto dfs = [&](auto self, int idx) -> void {
            if( idx == m ) {
                rep(i,n) rep(j,n) {
                    if( oil[i][j] == 1e9 ) continue;
                    if( field[i][j] != oil[i][j] ) return;
                }
                oil_place.clear();
                rep(i,n) rep(j,n) {
                    if( field[i][j] == 0 ) continue;
                    oil_place.emplace_back(P(i,j));
                }
                cand.emplace_back(oil_place);
                return;
            }
            
            // 位置固定済みの mino は skip
            if( fixed_mino[idx] ) return self(self, idx+1);
            
            for(auto &&minos_group : minos[idx].valid_minos) {
                mino_placable = true;
                for(auto &&[x, y] : minos_group) {
                    mino_placable &= (field[x][y] < oil[x][y]);
                    if( !mino_placable ) break;
                }
                if( !mino_placable ) continue;
                for(auto &&[x, y] : minos_group) field[x][y]++;
                self(self, idx+1);
                for(auto &&[x, y] : minos_group) field[x][y]--;

                // timeover
                if( now > end ) {
                    all_search = false;
                    return;
                }
                now = utility::mytm.elapsed();
            }
            return;
        };

        while( oil_cnt < n*n ) {
            // 占い part ( 選択肢を削減出来るマスを num 個選択 )
            vector point = searchPoint(oil,1);
            updateFortune(point);

            // dfs で mino 配置列挙
            start = utility::mytm.elapsed();
            end = start + (TIME_LIMIT-start)/(n*n-oil_cnt);
            now = start;

            cand.clear();
            all_search = true;
            field = fixed_field;
            dfs(dfs, 0);

            sort(cand.begin(), cand.end());
            cand.erase(unique(cand.begin(), cand.end()), cand.end());

            // 選択肢が 5 以上 or 全探索不可能な場合は skip
            if( !all_search || cand.size() > 5 ) continue;

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
            oil_place.clear();
            rep(i,n) rep(j,n) {
                if( oil[i][j] == 0 ) continue;
                oil_place.emplace_back(P(i,j));
            }
            cout << "a " << oil_place.size() << " ";
            for(auto &&[x, y] : oil_place) cout << x << " " << y << " ";
            cout << endl << flush;
            cin >> res;
        }
        return;
    }

    inline vector<P> searchPoint(const vector<vector<int>> &oil, int num) {
        // 各マスにおいて置ける or 置けない時の差分が小さい ( 情報を得ると選択肢を大幅削減出来る ) マスを選択
        // 1. 配置全列挙出来る時 ⇒ dfs で全探索
        // 2. 配置全列挙出来ない時 ⇒ 各ミノの置き方の総積 (overflow しないよう対数の和で計算)
        double min_score = 1e9, score1, score2;
        vector<pair<double,P>> result;
        if( !all_search ) {
            rep(i,n) rep(j,n) {
                if( oil[i][j] != 1e9 ) continue;
                score1 = 0.0, score2 = 0.0;
                rep(k,m) {
                    score1 += log(minos[k].pattern[i][j] + 1);
                    score2 += log(minos[k].valid_minos.size()-minos[k].pattern[i][j] + 1);
                }
                result.emplace_back(pair(abs(score1-score2), P(i,j)));
            }
        }
        else {
            int total = 0;
            field = fixed_field;
            field_cnt.assign(n, vector<int>(n, 0));
            
            auto dfs = [&](auto self, int idx) -> void {
                if( idx == m ) {
                    rep(i,n) rep(j,n) {
                        if( oil[i][j] == 1e9 ) continue;
                        if( field[i][j] != oil[i][j] ) return;
                    }
                    oil_place.clear();
                    rep(i,n) rep(j,n) {
                        if( field[i][j] == 0 ) continue;
                        oil_place.emplace_back(P(i,j));
                        field_cnt[i][j]++;
                    }
                    total++;
                    return;
                }

                // 位置固定済みの mino は skip
                if( fixed_mino[idx] ) return self(self, idx+1);

                for(auto &&minos_group : minos[idx].valid_minos) {
                    mino_placable = true;
                    for(auto &&[x, y] : minos_group) {
                        mino_placable &= (field[x][y] < oil[x][y]);
                        if( !mino_placable ) break;
                    }
                    if( !mino_placable ) continue;
                    for(auto &&[x, y] : minos_group) field[x][y]++;
                    self(self, idx+1);
                    for(auto &&[x, y] : minos_group) field[x][y]--;
                }
                return;
            };
            dfs(dfs, 0);
            rep(i,n) rep(j,n) {
                if( oil[i][j] != 1e9 ) continue;
                // 差分: total - field_cnt[i][j] - (field_cnt[i][j])
                result.emplace_back(pair(abs(total - field_cnt[i][j]*2), P(i,j)));
            }
        }
        vector<P> res;
        sort(result.begin(), result.end());
        rep(i,min(num,(int)result.size())) res.emplace_back(result[i].second);
        return res;
    }

    inline void updateFortune(const vector<P> &point) {
        // 占い & 場面更新をする関数
        // 1. 最初の 1 回は全 point で一括占い
        // 2. 残り size-1 回は 1 回のみ占って、占ってない 1 マスは 1 の結果 - 2 の結果 で更新
        cout << "q " << point.size() << " ";
        for(auto &&[sx, sy] : point) cout << sx << " " << sy << " ";
        cout << endl << flush;
        int tmp_res, other_cnt = 0; cin >> tmp_res;

        rep(i,point.size()-1) {
            auto &&[sx, sy] = point[i];
            cout << "q 1 " << sx << " " << sy << endl << flush;
            cin >> res;
            oil[sx][sy] = res;
            other_cnt += res;
        }

        auto &&[sx, sy] = point.back();
        oil[sx][sy] = tmp_res - other_cnt;

        rep(i,m) {
            minos[i].updateMinoArea(oil);
            if( minos[i].valid_minos.size() > 1 || fixed_mino[i] ) continue;
            // 今回の update で配置場所が確定した場合は位置固定
            fixed_mino[i] = true;
            for(auto &&[x, y] : minos[i].valid_minos[0]) fixed_field[x][y]++;
        }
        oil_cnt += point.size();
        return;
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
