#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef pair<int, int> P;
typedef tuple<int, int, int, int> T;
#define rep(i, n) for(int i = 0; i < n; i++)
#define reps(i, l, r) for(ll i = l; i < r; i++)

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

#define TIME_LIMIT 1950

//-----------------以下から実装部分-----------------//

const int N = 50;

// right | down | left | up
#define DIR_NUM 4
const vector<int> dx = { 0, 1, 0,-1};
const vector<int> dy = { 1, 0,-1, 0};
map<P, char> delta_to_char = {{{ 0, 1}, 'R'}, {{ 1, 0}, 'D'}, {{ 0,-1}, 'L'}, {{-1, 0}, 'U'}};

struct Solver {
    int sx, sy;
    vector<vector<int>> tile, point;
    vector<vector<bool>> used;
    vector<vector<P>> group;

    int ans_score;
    string ans_str;
    vector<P> ans_point, now_point;

    // 解に多様性を持たせる parameter
    int back_cnt;

    // query の時に保存しておく系変数
    vector<P> pre_route;

    Solver() {
        this->input();
        used.resize(N, vector<bool>(N, false));
        now_point.assign({});
    }

    void input() {
        cin >> sx >> sy;
        tile.resize(N, vector<int>(N));
        point.resize(N, vector<int>(N));
        group.resize(N*N, {});
        rep(i,N) rep(j,N) {
            cin >> tile[i][j];
            group[tile[i][j]].emplace_back(P(i,j));
        }
        rep(i,N) rep(j,N) cin >> point[i][j];
        return;
    }

    void output() {
        ans_str = calcString(ans_point);
        cout << ans_str << endl;
        return;
    }

    void solve() {
        utility::mytm.CodeStart();
        
        // 1. dfs で貪欲初期解生成
        // ※ (TIME_LIMIT / 10) ms までで初期解生成
        ans_score = 0;
        for(auto &&[i, j] : group[tile[sx][sy]]) used[i][j] = true;
        firstDFS(sx, sy, 0, TIME_LIMIT / 10);

        // used 更新
        used.assign(N, vector<bool>(N, false));
        for(auto &&[x,y]: ans_point) for(auto &&[i, j] : group[tile[x][y]]) used[i][j] = true;

        int iteration = 0, l, r, now_score, next_score;
        vector<P> cand_point;

        // 2. 部分破壊 & 再構築 の山登り
        while( utility::mytm.elapsed() < TIME_LIMIT ) {
            // 近傍 1. (l,r] の経路破壊 ⇒ dfs で再構築 ※ 破壊する経路長は 15 固定
            l = rand_int() % ((int)ans_point.size()-1) + 1; // Start 以外
            r = min( l+15, (int)ans_point.size()-1); // End 以外

            now_score = calcScore(l,r);
            next_score = query1(l,r);
            
            // cerr << "Now: " << now_score << endl;
            // cerr << "Next: " << next_score << endl << endl;

            if( next_score > now_score ) {
                // score 更新
                ans_score -= now_score;
                ans_score += next_score;
                
                // point 更新
                cand_point.clear();
                reps(i,0,l) cand_point.emplace_back(ans_point[i]);
                for(auto &&[i, j] : best_point) cand_point.emplace_back(P(i,j));
                reps(i,r+1,ans_point.size()) cand_point.emplace_back(ans_point[i]);
                swap(ans_point, cand_point);

                output();

                // used 更新
                used.assign(N, vector<bool>(N, false));
                for(auto &&[x,y]: ans_point) for(auto &&[i, j] : group[tile[x][y]]) used[i][j] = true;

                pre_route.clear();
            }
            else rollback();
            iteration++;
        }
        cerr << "iteration: " << iteration << endl;
        return;
    }

    inline void firstDFS(int x, int y, int score, const int &end_time) {
        score += point[x][y];
        now_point.emplace_back(P(x,y));

        // dx, dy を壁に近い順に変更
        vector<ll> perm(DIR_NUM); // 訪問順序
        iota(perm.begin(), perm.end(), 0);
        sort(perm.begin(), perm.end(), [&](int a, int b) {
            int dis1 = abs(N/2 - (x+dx[a])) + abs(N/2 - (y+dy[a]));
            int dis2 = abs(N/2 - (x+dx[b])) + abs(N/2 - (y+dy[b]));
            return dis1 > dis2;
        });

        rep(d,DIR_NUM) {
            int nx = x + dx[perm[d]], ny = y + dy[perm[d]];
            if( outField(nx, ny, N, N) || used[nx][ny] ) continue;

            for(auto &&[i, j] : group[tile[nx][ny]]) used[i][j] = true;

            firstDFS(nx, ny, score, end_time);
            if( utility::mytm.elapsed() > end_time ) return;

            for(auto &&[i, j] : group[tile[nx][ny]]) used[i][j] = false;

            if( back_cnt ) {
                back_cnt--;
                score -= point[x][y];
                now_point.pop_back();
                return;
            }
        }
        if( score > ans_score ) {
            cerr << "score: " << score << endl;
            ans_score = score;
            ans_point = now_point;
        }
        score -= point[x][y];
        now_point.pop_back();
        return;
    }

    // ※ 0 ≦ left < right ≦ ans_str.size() 
    inline int query1(int left, int right) {
        // 1. (l,r] の経路破壊
        reps(pi,left+1,right+1) {
            auto &&[x, y] = ans_point[pi];
            for(auto &&[i, j] : group[tile[x][y]]) used[i][j] = false;
            pre_route.emplace_back(P(x,y));
        }

        // 2. (l,r] の経路再構築
        best_score = 0;
        now_point = {};
        auto [tsx, tsy] = ans_point[left];
        auto [gx, gy] = ans_point[right];
        query_start_time = utility::mytm.elapsed();
        // ※ 矩形範囲内で再構築
        min_x = max(0, min(tsx, gx) - 5);
        min_y = max(0, min(tsy, gy) - 5);
        max_x = min(N, max(tsx, gx) + 5);
        max_y = min(N, max(tsy, gy) + 5);

        rep(i,N) {
            rep(j,N) {
                if( i == tsx && j == tsy ) cerr << "S";
                else if( i == gx && j == gy ) cerr << "G";
                else cerr << (used[i][j] ? "#" : ".");
            }
            cerr << endl;
        }
        cerr << endl;

        query1DFS(tsx, tsy, best_score, gx, gy);
        return best_score;
    }

    // query1 の時のみに使う変数
    int best_score, min_x, min_y, max_x, max_y;
    int query_start_time, time;
    vector<P> best_point;

    inline void query1DFS(int x, int y, int score, const int &gx, const int &gy) {
        score += point[x][y];
        now_point.emplace_back(P(x,y));

        if( x == gx && y == gy && score > best_score ) {
            best_score = score;
            best_point = now_point;
        }

        rep(d,DIR_NUM) {
            int nx = x + dx[d], ny = y + dy[d];
            if( nx < min_x || max_x <= nx || ny < min_y || max_y <= ny ) continue;
            if( used[nx][ny] ) continue;

            for(auto &&[i, j] : group[tile[nx][ny]]) used[i][j] = true;

            query1DFS(nx, ny, score, gx, gy);
            time = utility::mytm.elapsed();
            if( time > TIME_LIMIT || time - query_start_time > 100 ) return;

            for(auto &&[i, j] : group[tile[nx][ny]]) used[i][j] = false;
        }
        score -= point[x][y];
        now_point.pop_back();
        return;
    }

    inline int calcScore(int left, int right) {
        int res = 0;
        reps(i,left,right+1) {
            auto &&[x, y] = ans_point[i];
            res += point[x][y];
        }
        return res;
    }

    inline string calcString(const vector<P> &point) {
        string res = "";
        reps(i,1,point.size()) {
            auto &&[x1, y1] = point[i-1];
            auto &&[x2, y2] = point[i];
            res += delta_to_char[P(x2-x1, y2-y1)];
        }
        return res;
    }

    inline void rollback() {
        while( !pre_route.empty() ) {
            auto &&[x, y] = pre_route.back();
            for(auto &&[i, j] : group[tile[x][y]]) used[i][j] = true;
            pre_route.pop_back();
        }
        return;
    }

    inline bool outField(int x,int y,int h,int w) {
        if(0 <= x && x < h && 0 <= y && y < w)return false;
        return true;
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