#include <bits/stdc++.h>
using namespace std;
#if __has_include(<atcoder/all>)
    #include <atcoder/all>
using namespace atcoder;
#endif
typedef long long ll;
typedef pair<int, int> P;
typedef tuple<int, int, int> T;
typedef tuple<int, int, int, int> T2;
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

//温度関数
#define TIME_LIMIT 2950
inline double temp(double start) {
    double start_temp = 100,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

// 右下左上の順番
constexpr int DIR_NUM = 4;
vector<int> dx = {0,1,0,-1};
vector<int> dy = {1,0,-1,0};

struct State{

    State(){
        
    }
};

struct Solver{
    int t, h, w, i0, k, total_crops = 0, start_time;
    vector<int> s, d;
    vector<vector<int>> used;
    vector<vector<bool>> visited;
    vector<vector<vector<bool>>> water;
    vector<vector<vector<T2>>> crops_info;

    Solver(){
        utility::mytm.CodeStart();
        start_time = utility::mytm.elapsed();

        input();
        crops_info.assign(h,vector(w,vector<T2>{}));
    }

    void input(){
        cin >> t >> h >> w >> i0;
        water.assign(h,vector(w,vector<bool>(DIR_NUM,false)));
        rep(i,h-1) {
            string s; cin >> s;
            rep(j,w) {
                if( s[j] == '1' ) {
                    water[i][j][1] = true;
                    water[i+1][j][3] = true;
                }
            }
        }
        rep(i,h) {
            string s; cin >> s;
            rep(j,w-1) {
                if( s[j] == '1' ) {
                    water[i][j][0] = true;
                    water[i][j+1][2] = true;
                }
            }
        }
        cin >> k;
        s.assign(k,0);
        d.assign(k,0);
        rep(i,k) cin >> s[i] >> d[i];
    }

    void solve(){
        int noko_num = 0;
        used.assign(h,vector<int>(w,0));
        queue<P> todo, place;
        queue<T> bfs;
        stack<T> st;

        vector<vector<int>> distance(h,vector<int>(w,-1));
        bfs.push(T(0,i0,0));
        while( !bfs.empty() ) {
            auto [dis, x, y] = bfs.front(); bfs.pop();
            if( distance[x][y] != -1 ) continue;
            distance[x][y] = dis;
            st.push(T(dis, x, y));
            rep(d,DIR_NUM) {
                if( water[x][y][d] ) continue;
                int nx = x + dx[d], ny = y + dy[d];
                if( outField(nx,ny) || distance[nx][ny] != -1 ) continue;
                bfs.push(T(dis+1, nx, ny));
            }
        }

        // 入口から距離が遠い順に農耕地に出来るか確認
        while( !st.empty() ) {
            auto [_, x, y] = st.top(); st.pop();

            // 実際に農耕地にして条件を満たすか
            used[x][y] = 1;
            int reachable_num = 0;
            vector<vector<bool>> reachable(h,vector<bool>(w,false)), v(h,vector<bool>(w,false));
            todo.push(P(i0,0));
            while( !todo.empty() ) {
                auto [tx, ty] = todo.front(); todo.pop();
                if( v[tx][ty] || used[tx][ty] == 1 ) continue;
                v[tx][ty] = true;
                reachable[tx][ty] = true;
                rep(d,DIR_NUM) {
                    if( water[tx][ty][d] ) continue;
                    int nx = tx + dx[d], ny = ty + dy[d];
                    if( outField(nx,ny) ) continue;
                    reachable[nx][ny] = true;
                    if( v[nx][ny] || used[nx][ny] == 1 ) continue;
                    todo.push(P(nx,ny));
                }
            }
            rep(i,h)rep(j,w) reachable_num += (reachable[i][j] && (used[i][j] == 1));

            if( reachable_num == noko_num+1 ) noko_num++;
            else used[x][y] = -1;
        }

        vector<T> crops;
        rep(i,k) crops.emplace_back(T(d[i],s[i], i));
        sort(crops.begin(), crops.end());

        rep(i,h) {
            rep(j,w) {
                if( used[i][j] != 1 || crops.empty() ) continue;

                auto [nd, ns, idx] = crops.back();
                crops_info[i][j].emplace_back(T2(idx,i,j,ns));
                crops.pop_back();
                total_crops++;

                int now = ns;
                while( true ) {
                    auto itr = lower_bound(crops.begin(), crops.end(), T(now, -1e8, -1e8));
                    if( itr == crops.begin() ) break;
                    itr--;

                    auto [td, ts, nidx] = *itr;
                    crops_info[i][j].emplace_back(T2(nidx,i,j,ts));
                    crops.erase(itr);

                    now = ts;
                    total_crops++;
                }
            }
        }

        // ~~~~ ↑ ここまで貪欲解 ↑ ~~~~ //
        // ~~~~ ↓  ここから改善  ↓ ~~~~ //

        // 山登り法
        // while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            // 候補1. 部分破壊 × 再構築 (3*3矩形破壊)
            // 候補2. 

        // }
        

    }

    void output(){
        cout << total_crops << endl;
        rep(i,h) {
            rep(j,w) {
                for(auto [tk,ti,tj,ts]:crops_info[i][j]) {
                    cout << tk+1 << " " << ti << " " << tj << " " << ts << endl;
                }
            }
        }
        rep(i,h) {
            rep(j,w) {
                cerr << (used[i][j] == 1 ? 1 : 0) << " ";
            }
            cerr << endl;
        }
    }

    inline bool outField(int x,int y){
        if(0 <= x && x < h && 0 <= y && y < w)return false;
        return true;
    }
};

int main(){
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.solve();
    solver.output();
    
    return 0;
}