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

// 周囲のdxdy
constexpr int SURROUND_DIR_NUM = 9;
vector<int> sdx = {-1, -1, -1,  0, 0, 0,  1, 1, 1};
vector<int> sdy = {-1,  0,  1, -1, 0, 1, -1, 0, 1};

int t, h, w, i0, k, nx, ny;

struct PlantedCrops {
    int idx;   // 作物番号
    int x, y;  // 作物場所
    int start; // 植えた時期
    PlantedCrops(int _idx, int _x, int _y, int _start) {
        idx = _idx;
        x = _x;
        y = _y;
        start = _start;
    }
};

struct State {
    // 盤面情報を持つ State
    int rx, ry, now_score;
    queue<P> todo;
    queue<vector<P>> plan;
    vector<int> crops_score;
    vector<T> rest_crops, rest_cache;
    vector<vector<bool>> visited;
    vector<vector<vector<bool>>> water;
    vector<vector<vector<PlantedCrops>>> crops_info, cache;

    State() : water({{{}}}){}
    State(const vector<vector<vector<PlantedCrops>>> &_crops_info, vector<vector<vector<bool>>> &_water, vector<T> &_rest_crops, vector<int> &_crops_score){
        cache.assign(h, vector(w, vector<PlantedCrops>{}));
        crops_score = _crops_score;
        crops_info = _crops_info;
        rest_crops = _rest_crops;
        water = _water;

        // feat : ここで貪欲解を作るよう移行
        now_score = 0;
        rep(th, h) {
            rep(tw, w) {
                rep(i, crops_info[th][tw].size()) {
                    now_score += crops_score[crops_info[th][tw][i].idx];
                }
            }
        }
    }

    inline void query() {
        // 矩形破壊 & 再構築

        // (rx,ry) を中心とした 3*3 矩形破壊
        rx = rand_int()%(h-2) + 1, ry = rand_int()%(w-2) + 1;
        rest_cache = rest_crops;

        rep(sd,SURROUND_DIR_NUM) {
            if( (sd == 1 && water[rx][ry][3]) || (sd == 3 && water[rx][ry][2]) || (sd == 5 && water[rx][ry][0]) || (sd == 7 && water[rx][ry][1]) ) continue;
            nx = rx + sdx[sd], ny = ry + sdy[sd];
            rep(i,crops_info[nx][ny].size()) {
                rest_crops.emplace_back(crops_info[nx][ny][i]);
                now_score -= crops_score[crops_info[nx][ny][i].idx];
            }

            // 真っ新に生まれ変わって人生一から始めようか
            swap(cache[nx][ny], crops_info[nx][ny]);
        }

        // 先に rest で作物計画を作っておく ( 最大の9個分まで )
        while( !plan.empty() ) plan.pop();
        while( plan.size() < 9 ) {
            auto [nd, ns, idx] = rest_crops.back();

            vector<P> tmp;
            tmp.emplace_back(P(idx,ns));
            rest_crops.pop_back();

            int now = ns;
            while( true ) {
                auto itr = lower_bound(rest_crops.begin(), rest_crops.end(), T(now, -1e8, -1e8));
                if( itr == rest_crops.begin() ) break;
                itr--;
                auto [td, ts, nidx] = *itr;
                tmp.emplace_back(P(nidx,ts));
                rest_crops.erase(itr);
                now = ts;
            }
            plan.push(tmp);
        }

        // bit全探索で各配置を試して一番置ける数が多いやつを採用
        int max_count = -1, max_state = -1;
        rep(i,(1 << SURROUND_DIR_NUM)) {
            int pop_count = 0;
            rep(sd, SURROUND_DIR_NUM) {
                if( (i & (1 << sd)) > 0 ) {
                    pop_count++;
                    nx = rx + sdx[sd], ny = ry + sdy[sd];
                    crops_info[nx][ny].emplace_back(T(-1,-1,-1));
                }
            }
            vector accesible = getAccesible();
            bool f = true;
            rep(th,h) {
                rep(tw,w) {
                    if( !crops_info.empty() && accesible[th][tw] ) {
                        f = false;
                        break;
                    }
                }
            }

            if( f && (max_count < pop_count) ) {
                max_count = pop_count;
                max_state = i;
            }
            rep(sd, SURROUND_DIR_NUM) {
                if( (i & (1 << sd)) > 0 ) {
                    nx = rx + sdx[sd], ny = ry + sdy[sd];
                    crops_info[nx][ny].pop_back();
                }
            }
        }

        rep(sd, SURROUND_DIR_NUM) {
            if( max_state & (1 << sd) ) {
                nx = rx + sdx[sd], ny = ry + sdy[sd];
                vector tmp = plan.front(); plan.pop();
                while( !tmp.empty() ) {
                    auto [idx, ts] = tmp.back(); tmp.pop_back();
                    crops_info[nx][ny].emplace_back(PlantedCrops(idx, nx, ny, ts));
                    now_score += crops_score[idx];
                }
            }
        }
    }

    inline void rollBack() {
        // 時を戻そう…！
        rep(sd,SURROUND_DIR_NUM) {
            if( (sd == 1 && water[rx][ry][3]) || (sd == 3 && water[rx][ry][2]) || (sd == 5 && water[rx][ry][0]) || (sd == 7 && water[rx][ry][1]) ) continue;
            nx = rx + sdx[sd], ny = ry + sdy[sd];
            swap(cache[nx][ny], crops_info[nx][ny]);
            cache.assign({});
        }
    }

    inline void deleteCache() {
        // 時を忘れよう…！
        rep(sd,SURROUND_DIR_NUM) {
            if( (sd == 1 && water[rx][ry][3]) || (sd == 3 && water[rx][ry][2]) || (sd == 5 && water[rx][ry][0]) || (sd == 7 && water[rx][ry][1]) ) continue;
            nx = rx + sdx[sd], ny = ry + sdy[sd];
            cache.assign({});
        }
    }

    inline int calcScore() {
        // 評価関数
    }

    inline vector<int> getEssentialTimeList(int x, int y) {
        // (x,y) を完全に封じたら行けなくなる場所の、行けてほしい時間帯の list
        vector<int> list;
        vector accesible = getAccesible();
        rep(i,h) {
            rep(j,w) {
                if( !accesible[i][j] && !crops_info[i][j].empty() ) {
                    // access不可能だけど、作物がある場合
                    // ⇒ そのマスの作物の start, end を取得

                    // for(auto [idx,])
                }
            }
        }
    }

    inline vector<vector<bool>> getAccesible() {
        // access 可能なマスを bool として返す
        vector res(h,vector<bool>(w,false));
        visited.assign(h,vector<bool>(w,false));

        todo.push(P(i0,0));
        while( !todo.empty() ) {
            auto [tx, ty] = todo.front(); todo.pop();
            if( visited[tx][ty] || !crops_info[tx][ty].empty() ) continue;
            visited[tx][ty] = true;
            res[tx][ty] = true;
            rep(d,DIR_NUM) {
                if( water[tx][ty][d] ) continue;
                nx = tx + dx[d], ny = ty + dy[d];
                if( outField(nx,ny) ) continue;
                res[nx][ny] = true;
                if( visited[nx][ny] || !crops_info[tx][ty].empty() ) continue;
                todo.push(P(nx,ny));
            }
        }

        return res;
    }

    inline bool outField(int x,int y){
        // 場外判定
        if(0 <= x && x < h && 0 <= y && y < w)return false;
        return true;
    }
};

struct Solver{
    int total_crops = 0, start_time, best_score, now_score;
    vector<int> s, d, crops_score;
    vector<vector<int>> used;
    vector<vector<bool>> visited;
    vector<vector<vector<bool>>> water;
    vector<vector<vector<PlantedCrops>>> crops_info;

    State state;

    Solver(){
        utility::mytm.CodeStart();
        start_time = utility::mytm.elapsed();

        input();
        crops_info.assign(h,vector(w,vector<PlantedCrops>{}));
        crops_score.assign(k, 0);
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
        rep(i,k) {
            cin >> s[i] >> d[i];
            crops_score[i] = d[i] - s[i];
        }
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
                nx = x + dx[d], ny = y + dy[d];
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
                    nx = tx + dx[d], ny = ty + dy[d];
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

        vector<T> rest_crops;
        rep(i,k) rest_crops.emplace_back(T(d[i],s[i], i));
        sort(rest_crops.begin(), rest_crops.end());

        rep(i,h) {
            rep(j,w) {
                if( used[i][j] != 1 || rest_crops.empty() ) continue;

                auto [nd, ns, idx] = rest_crops.back();
                crops_info[i][j].emplace_back(PlantedCrops(idx,i,j,ns));
                rest_crops.pop_back();
                total_crops++;

                int now = ns;
                while( true ) {
                    auto itr = lower_bound(rest_crops.begin(), rest_crops.end(), T(now, -1e8, -1e8));
                    if( itr == rest_crops.begin() ) break;
                    itr--;

                    auto [td, ts, nidx] = *itr;
                    crops_info[i][j].emplace_back(PlantedCrops(nidx, i, i, ts));
                    rest_crops.erase(itr);

                    now = ts;
                    total_crops++;
                }
            }
        }

        // 貪欲会から State 作成
        state = State(crops_info, water, rest_crops, crops_score);

        // ~~~~ ↑ ここまで貪欲解 ↑ ~~~~ //
        // ~~~~ ↓  ここから改善  ↓ ~~~~ //
        best_score = state.now_score;

        // 山登り法
        while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            // 候補1. 部分破壊 × 再構築 (3*3矩形破壊)
            // ・破壊は簡単だが、再構築が難しい
            // ・育てる場所は bit全探索 で網羅して、作物選抜は従来通り締め切り遅い順(?)
            // ↑ とりあえずこれで一回やってみる

            state.query();

            now_score = state.now_score;
            if( now_score > best_score ) {
                best_score = now_score;
                state.deleteCache();
            }
            else {
                state.rollBack();
            }
        }
    }

    void output(){
        cout << total_crops << endl;
        rep(i,h) {
            rep(j,w) {
                for(auto crops:crops_info[i][j]) {
                    cout << crops.idx << " " << crops.x << " " << crops.y << " " << crops.start << endl;
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

