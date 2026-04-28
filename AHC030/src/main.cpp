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
#define TIME_LIMIT 2800

//-----------------以下から実装部分-----------------//

#define DIR_NUM 4
vector<int> dx = { 0,-1, 0, 1};
vector<int> dy = {-1, 0, 1, 0};

inline bool outField(int x,int y,int h,int w){
    if(0 <= x && x < h && 0 <= y && y < w)return false;
    return true;
}

// gloabal info (N, M, res, eps)
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

    inline void updateMinoArea(vector<vector<int>> &field){
        valid_minos.clear();
        pattern.assign(n, vector<int>(n, 0));
        rep(sx,n-max_x) rep(sy,n-max_y) {
            // 置けるかどうかの判定
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
    private:
    // flag 系統
    bool all_search, mino_placable;
    // 時間系統
    double start, end, now; 
    
    // 石油系統
    int oil_cnt;
    vector<Mino> minos; // 油田情報
    vector<vector<int>> oil; // oil[i][j] : (i,j) が -1:未探索, 0以上: 油田の数

    // 位置固定された油田系統
    vector<bool> fixed_mino;
    vector<vector<int>> fixed_oil;
    
    // "答え候補の座標配列" の配列
    vector<vector<P>> cand_array;

    vector<vector<int>> field;

    public:
    Solver() {
        this->input();
        utility::mytm.CodeStart(); // Program Start

        fixed_mino.assign(m, false);
        fixed_oil.assign(n, vector<int>(n, 0));
        oil.assign(n, vector<int>(n, 1e9));
        oil_cnt = 0;
        all_search = false;

        // 初期状態の mino 配置場所 update
        rep(i,m) minos[i].updateMinoArea(oil);
        sort(minos.begin(), minos.end(), [](const Mino &a, const Mino &b) {
            return a.p.size() > b.p.size();
        });
        return;
    }

    void input(){
        int x, y, d;
        cin >> n >> m >> eps;
        rep(i,m) {
            cin >> d;
            vector<P> p;
            rep(j,d) {
                cin >> x >> y;
                p.emplace_back(P(x,y));
            }
            minos.emplace_back(Mino(p));
        }
        return;
    }

    void solve() {
        int env_cnt = 0;
        auto dfs = [&](auto self, int idx) -> void {
            if( idx == m ) {
                env_cnt++;
                rep(i,n) rep(j,n) {
                    if( oil[i][j] == 1e9 ) continue;
                    if( field[i][j] != oil[i][j] ) return;
                }
                cand_array.emplace_back(vector<P>{});
                rep(i,n) rep(j,n) {
                    if( field[i][j] == 0 ) continue;
                    cand_array.back().emplace_back(P(i,j));
                }
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

            env_cnt = 0;
            cand_array.clear();
            all_search = true;
            field = fixed_oil;
            dfs(dfs, 0);

            // ---------- DFS iterattion, valid_minos, all_search, cand_array の情報 log ---------- //
            // rep(i,m) cerr << "valid_minos[" << i << "] : " << minos[i].valid_minos.size() << '\n';
            // cerr << "env_cnt: " << env_cnt << '\n';
            // cerr << "all_search : " << all_search << '\n';
            // cerr << "cand_array.size() : " << cand_array.size() << '\n' << '\n';

            sort(cand_array.begin(), cand_array.end());
            cand_array.erase(unique(cand_array.begin(), cand_array.end()), cand_array.end());

            // 選択肢が 5 以上 or 全探索不可能な場合は skip
            if( !all_search || cand_array.size() > 5 ) continue;

            rep(i,cand_array.size()) {
                cout << "a " << cand_array[i].size() << " ";
                for(auto &&[x, y] : cand_array[i]) cout << x << " " << y << " ";
                cout << '\n' << flush; cin >> res;
                if( res == 1 ) return;
            }
        }

        // 最後まで分からなかった場合 ⇒ 結果から答えを出力
        if( oil_cnt == n*n ) {
            vector<P> oil_place;
            rep(i,n) rep(j,n) {
                if( oil[i][j] == 0 ) continue;
                oil_place.emplace_back(P(i,j));
            }
            cout << "a " << oil_place.size() << " ";
            for(auto &&[x, y] : oil_place) cout << x << " " << y << " ";
            cout << '\n' << flush;
            cin >> res;
        }
        return;
    }

    // num 個の占いを返す関数
    inline vector<P> searchPoint(const vector<vector<int>> &oil, int num) {
        // 配置全列挙出来る　 : dfs で全探索 ⇒ 選択肢を多く削げる座標を返す
        // 配置全列挙出来ない : 選択肢が多い mino を削げる座標を返す

        long double min_score = 1e9, score1, score2;
        vector<pair<double,P>> result;
        if( !all_search ) {
            rep(i,n) rep(j,n) {
                if( oil[i][j] != 1e9 ) continue;
                int rest = 0;
                rep(k,m) {
                    if( rest < minos[k].valid_minos.size()*minos[k].p.size() ) {
                        rest = minos[k].valid_minos.size()*minos[k].p.size();
                        score1 = minos[k].pattern[i][j];
                        score2 = minos[k].valid_minos.size()-minos[k].pattern[i][j];
                    }
                }

                // 回りに探索済みがあるところは penalty
                double pena = 1.0;
                rep(k,DIR_NUM) {
                    int nx = i + dx[k], ny = j + dy[k];
                    if( outField(nx,ny,n,n) || oil[nx][ny] != 1e9 ) pena++;
                }
                if( pena >= 2.0 ) pena = 1e3;
                // ---------- 各マスの score 表示 log ---------- //
                // cerr << "(" << i << "," << j << ") : " << score1 << " " << score2 << '\n';
                result.emplace_back(pair(abs(score1-score2) * pena, P(i,j)));
            }
        }
        else {
            int total = 0;
            field = fixed_oil;
            vector field_cnt(n,vector(n,vector<int>(m+1,0))); // cnt[i][j][k] : (i,j) が v = k となる場合の数
            
            auto dfs = [&](auto self, int idx) -> void {
                if( idx == m ) {
                    rep(i,n) rep(j,n) if( oil[i][j] != 1e9 && field[i][j] != oil[i][j] ) return;
                    rep(i,n) rep(j,n) field_cnt[i][j][field[i][j]]++;
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
                int max_cnt = 0;
                rep(k,m+1) max_cnt = max(max_cnt, field_cnt[i][j][k]);
                result.emplace_back(pair(max_cnt, P(i,j)));
            }
        }
        vector<P> res;
        sort(result.begin(), result.end());
        rep(i,min(num,(int)result.size())) res.emplace_back(result[i].second);

        // ---------- 今回の pointSearch にて score 上位5つの頂点の log ---------- //
        // rep(i,min(5,(int)result.size())) {
        //     auto &&[score, place] = result[i];
        //     auto &&[tx, ty] = place;
        //     cerr << "Top" << i+1 << " : " << score << " " << tx << " " << ty << '\n';
        // }
        // cerr << '\n';
        return res;
    }

    // 占い & 場面更新をする関数
    inline void updateFortune(const vector<P> &point) {
        // 1. 最初の 1 回は全 point で一括占い
        // 2. 残り size-1 回は 1 回のみ占って、占ってない 1 マスは 1 の結果 - 2 の結果 で更新

        cout << "q " << point.size() << " ";
        for(auto &&[sx, sy] : point) cout << sx << " " << sy << " ";
        cout << '\n' << flush;
        int tmp_res, other_cnt = 0; cin >> tmp_res;

        rep(i,point.size()-1) {
            auto &&[sx, sy] = point[i];
            cout << "q 1 " << sx << " " << sy << '\n' << flush;
            cin >> res;
            oil[sx][sy] = res;
            other_cnt += res;
        }

        auto &&[sx, sy] = point.back();
        oil[sx][sy] = tmp_res - other_cnt;

        vector<vector<vector<P>>> reachable(n, vector(n, vector<P>{}));
        rep(i,m) {
            minos[i].updateMinoArea(oil);
            rep(j,minos[i].valid_minos.size()) {
                auto &&mino_g = minos[i].valid_minos[j];
                for(auto &&[x, y] : mino_g) reachable[x][y].emplace_back(P(i,j));
            }

            if( minos[i].valid_minos.size() > 1 || fixed_mino[i] ) continue;
            // 今回の update で配置場所が確定した場合は位置固定
            fixed_mino[i] = true;
            for(auto &&[x, y] : minos[i].valid_minos[0]) fixed_oil[x][y]++;
        }
        oil_cnt += point.size();

        // どのミノも到達不可能な場所は 0 に更新
        rep(i,n) rep(j,n) {
            if( reachable[i][j].size() == 0 && oil[i][j] == 1e9 ) {
                oil[i][j] = 0;
                oil_cnt++;
            }
        }

        // 到達可能なミノが 1 つ & oil が 1 以上の場合はその mino で確定
        bool flag = true;
        while( flag ) {
            flag = false;
            rep(i,n) rep(j,n) {
                int reach_cnt = 0, reach_idx = -1;
                rep(k,reachable[i][j].size()) {
                    auto &&[x, y] = reachable[i][j][k];
                    if( fixed_mino[x] ) continue;
                    reach_cnt++;
                    reach_idx = k;
                }
                if( reach_cnt == 1 && oil[i][j]-fixed_oil[i][j] > 0 && oil[i][j] != 1e9 && !fixed_mino[reachable[i][j][reach_idx].first] ) {
                    auto &&[x, y] = reachable[i][j][reach_idx];
                    // ---------- 位置固定がされた mino の log 出力 ---------- //
                    // auto &&[tx, ty] = minos[x].valid_minos[y][0];
                    // cerr << "fixed_mino[" << x << "] : " << tx << " " << ty << "\n\n";
                    for(auto &&[tx, ty] : minos[x].valid_minos[y]) fixed_oil[tx][ty]++;
                    fixed_mino[x] = true;
                    flag = true;
                }
            }
        }
        // ---------- update 後の既知の油田情報の log 出力 ---------- //
        // rep(i,n) {
        //     rep(j,n) cerr << (oil[i][j] == 1e9 ? (char)'?' : (char)('0' + oil[i][j])) << '\t';
        //     cerr << '\n';
        // }
        return;
    }
};

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.solve();
    
    return 0;
}
