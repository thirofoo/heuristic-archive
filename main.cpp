#include <bits/stdc++.h>
using namespace std;
#if __has_include(<atcoder/all>)
    #include <atcoder/all>
using namespace atcoder;
#endif
typedef long long ll;
typedef long double ld;
typedef pair<int, int> P;
typedef tuple<int, int, int> T;
typedef tuple<int, int, int, int, int> T2;
#define rep(i, n) for(int i = 0; i < n; i++)

namespace utility {
    struct timer {
        chrono::system_clock::time_point start;
        // 開始時間を記録
        void CodeStart() {
            start = chrono::system_clock::now();
        }
        // 経過時間 (s) を返す
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
#define TIME_LIMIT 5850
inline double temp(int start) {
    double start_temp = 100,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int now,int next,int start) {
    return exp((double)(next - now) / temp(start));
}

//-----------------以下から実装部分-----------------//

constexpr int DIR_NUM = 6;
int turn = 0,pre_erase = -1;
queue<T> erase_block;
vector<vector<vector<int>>> dig1,dig2;

// 面の方向 (サイコロの出目と同じ)
//     
//    ・ーーーーー ・
//   /　　１　　 / |
//  /　　　　　 /　|　　　　y方向
// ◎ーーーーー・ 2｜   　 ⇗
// ｜　　　　　｜  ・　　　◎⇒ x方向
// ｜　　０　　｜ /　　　 ⇓
// ｜　　　　　｜/　　　　z方向
// ・ーーーーー・

vector<vector<vector<int>>> b_dir = {
    // b_dir[i] : 面iを0と見立てた時の軸の方向
    // { z正,z負,y正,y負,x正,x負 }
    {
        { 1,-1, 0, 0, 0, 0}, // dz
        { 0, 0, 1,-1, 0, 0}, // dy
        { 0, 0, 0, 0, 1,-1}, // dx
    },
    {
        { 0, 0, 1,-1, 0, 0},
        {-1, 1, 0, 0, 0, 0},
        { 0, 0, 0, 0, 1,-1},
    },
    {
        { 0, 0, 0, 0, 1,-1},
        {-1, 1, 0, 0, 0, 0},
        { 0, 0,-1, 1, 0, 0},
    },
    {
        { 0, 0,-1, 1, 0, 0},
        {-1, 1, 0, 0, 0, 0},
        { 0, 0, 0, 0,-1, 1},
    },
    {
        { 0, 0, 0, 0,-1, 1},
        {-1, 1, 0, 0, 0, 0},
        { 0, 0, 1,-1, 0, 0},
    },
    {
        {-1, 1, 0, 0, 0, 0},
        { 0, 0,-1, 1, 0, 0},
        { 0, 0, 0, 0, 1,-1},
    },
};
priority_queue<T> pq1,pq2;

struct Block{
    bool one;
    int size, d, id, dir1, dir2;
    vector<T> place1,place2;
    stack<T> cache1,cache2;

    Block(T b1, T b2, int _dir1, int _dir2, int _d, int _id){
        d = _d;
        id = _id;
        dir1 = _dir1; // 基準面1
        dir2 = _dir2; // 基準面2
        size = 1;
        place1.emplace_back(b1);
        place2.emplace_back(b2);
        one = (place1[0] == T(-1,-1,-1) || place2[0] == T(-1,-1,-1));
        // 回転は実装せず、全消し & 方向転換でとりあえずやる。(回転きつすぎ)
    }

    inline bool expand(vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        int rnd = rand_int()%place1.size(); // 起点ブロック
        int d = rand_int()%DIR_NUM; // 拡張方向

        auto [z1,y1,x1] = place1[rnd];
        int nz1 = z1 + b_dir[dir1][0][d];
        int ny1 = y1 + b_dir[dir1][1][d];
        int nx1 = x1 + b_dir[dir1][2][d];
        if(outField_3d(nz1,ny1,nx1) || state1[nz1][ny1][nx1])return false;
        // if(outField_3d(nz1,ny1,nx1) || dig1[nz1][ny1][nx1] <= 1)return false;

        auto [z2,y2,x2] = place2[rnd];
        int nz2 = z2 + b_dir[dir2][0][d];
        int ny2 = y2 + b_dir[dir2][1][d];
        int nx2 = x2 + b_dir[dir2][2][d];
        if(outField_3d(nz2,ny2,nx2) || state2[nz2][ny2][nx2])return false;
        // if(outField_3d(nz2,ny2,nx2) || dig2[nz2][ny2][nx2] <= 1)return false;

        // 次数を管理
        // rep(i,DIR_NUM){
        //     int lz1 = nz1 + b_dir[0][0][i];
        //     int ly1 = ny1 + b_dir[0][1][i];
        //     int lx1 = nx1 + b_dir[0][2][i];
        //     if(!outField_3d(lz1,ly1,lx1) && state1[lz1][ly1][lx1] == id){
        //         dig1[nz1][ny1][nx1]++;
        //         dig1[lz1][ly1][lx1]++;
        //     }
        //     int lz2 = nz2 + b_dir[0][0][i];
        //     int ly2 = ny2 + b_dir[0][1][i];
        //     int lx2 = nx2 + b_dir[0][2][i];
        //     if(!outField_3d(lz2,ly2,lx2) && state2[lz2][ly2][lx2] == id){
        //         dig2[nz2][ny2][nx2]++;
        //         dig2[lz2][ly2][lx2]++;
        //     }
        // }

        // add_block
        place1.emplace_back(T(nz1,ny1,nx1));
        place2.emplace_back(T(nz2,ny2,nx2));
        state1[nz1][ny1][nx1] = id;
        state2[nz2][ny2][nx2] = id;
        size++;

        return true;
    }

    inline bool erase(int num,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        pre_erase = id;
        while(!place1.empty() && num--){
            auto [z1,y1,x1] = place1.back(); place1.pop_back();
            auto [z2,y2,x2] = place2.back(); place2.pop_back();
            state1[z1][y1][x1] = 0;
            state2[z2][y2][x2] = 0;
            cache1.push(T(z1,y1,x1));
            cache2.push(T(z2,y2,x2));
            size--;
        }
        return place1.empty();
    }

    inline bool outField_2d(int x,int y){
        return (x < 0 || x >= d || y < 0 || y >= d);
    }

    inline bool outField_3d(int x,int y,int z){
        return (x < 0 || x >= d || y < 0 || y >= d || z < 0 || z >= d);
    }
};

struct Solver{
    int d;
    ll best_score1, best_score2;
    vector<vector<bool>> sil1_f,sil1_r,sil2_f,sil2_r;
    vector<vector<vector<int>>> ans1,ans2;
    vector<Block> Block_list;
    priority_queue<P> pq1,pq2;

    Solver(){}

    void input(){
        cin >> d;
        sil1_f.assign(d,vector<bool>(d,false));
        sil1_r.assign(d,vector<bool>(d,false));
        sil2_f.assign(d,vector<bool>(d,false));
        sil2_r.assign(d,vector<bool>(d,false));
        rep(z,d){
            string s; cin >> s;
            rep(x,d)sil1_f[z][x] = (s[x] == '1');
        }
        rep(z,d){
            string s; cin >> s;
            rep(y,d)sil1_r[z][y] = (s[y] == '1');
        }
        rep(z,d){
            string s; cin >> s;
            rep(x,d)sil2_f[z][x] = (s[x] == '1');
        }
        rep(z,d){
            string s; cin >> s;
            rep(y,d)sil2_r[z][y] = (s[y] == '1');
        }
        dig1.assign(d,vector<vector<int>>(d,vector<int>(d,1e7)));
        dig2.assign(d,vector<vector<int>>(d,vector<int>(d,1e7)));
        ans1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        ans2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
    }

    void solve(){
        utility::mytm.CodeStart();

        // 必要最低限の1*1のみで構成 (初期解)
        int now1 = 1,now2 = 1;
        rep(z,d){
            vector<bool> r(d,false),c(d,false);
            vector<int> ri,ci;
            rep(y,d)rep(x,d)if(!sil1_f[z][x] | !sil1_r[z][y]){
                ans1[z][y][x] = -1;
                dig1[z][y][x] = 1e7;
            }
            rep(x,d)if(sil1_f[z][x])ci.emplace_back(x);
            rep(y,d)if(sil1_r[z][y])ri.emplace_back(y);
            rep(i,max(ri.size(),ci.size())){
                ans1[z][ri[(ri.size() <= i ? ri.size()-1 : i)]][ci[(ci.size() <= i ? ci.size()-1 : i)]] = now1;
                now1++;
            }
        }
        rep(z,d){
            vector<bool> r(d,false),c(d,false);
            vector<int> ri,ci;
            rep(y,d)rep(x,d)if(!sil2_f[z][x] | !sil2_r[z][y]){
                ans2[z][y][x] = -1;
                dig2[z][y][x] = 1e7;
            }
            rep(x,d)if(sil2_f[z][x])ci.emplace_back(x);
            rep(y,d)if(sil2_r[z][y])ri.emplace_back(y);
            rep(i,max(ri.size(),ci.size())){
                ans2[z][ri[(ri.size() <= i ? ri.size()-1 : i)]][ci[(ci.size() <= i ? ci.size()-1 : i)]] = now2;
                now2++;
            }
        }
        int now = max(now1,now2);
        vector<vector<T>> block_info(now,vector<T>(2,T{-1,-1,-1}));
        rep(z,d)rep(y,d)rep(x,d){
            if(ans1[z][y][x] > 0)block_info[ans1[z][y][x]][0] = T(z,y,x);
            if(ans2[z][y][x] > 0)block_info[ans2[z][y][x]][1] = T(z,y,x);
        }
        rep(i,now){
            Block_list.emplace_back(Block(block_info[i][0],block_info[i][1],rand_int()%DIR_NUM,rand_int()%DIR_NUM,d,i));
        }

        // 山登り法
        bool f;
        ll best_score = calcScore();
        while(utility::mytm.elapsed() <= TIME_LIMIT){
            // query候補
            // 1. 1Block延長
            // 2. 1Block削除
            // 3. 合併
            // 4. 削除
            // 5. 同型同士ペア交換

            int rnd = rand_int()%Block_list.size();
            f = Block_list[rnd].expand(ans1,ans2);
            if(!f)continue;
            
            ll now_score = calcScore();
            cerr << best_score << " " << now_score << endl;
            if(now_score < best_score){
                best_score = now_score;
                if(turn <= 10)output();
                turn++;
            }
            else{
                Block_list[rnd].erase(1,ans1,ans2);
            }
        }

        // index圧縮
        vector<int> idx;
        rep(i,d)rep(j,d)rep(k,d)if(ans1[i][j][k] > 0)idx.emplace_back(ans1[i][j][k]);
        rep(i,d)rep(j,d)rep(k,d)if(ans2[i][j][k] > 0)idx.emplace_back(ans2[i][j][k]);
        sort(idx.begin(),idx.end());
        idx.erase(unique(idx.begin(),idx.end()),idx.end());
        rep(i,d)rep(j,d)rep(k,d){
            if(ans1[i][j][k] > 0){
                ans1[i][j][k] = lower_bound(idx.begin(),idx.end(),ans1[i][j][k])-idx.begin()+1;
            }
            if(ans2[i][j][k] > 0){
                ans2[i][j][k] = lower_bound(idx.begin(),idx.end(),ans2[i][j][k])-idx.begin()+1;
            }
        }
    }

    ll calcScore(){
        ld score = 0;
        for(auto b:Block_list){
            if(b.size > 0)score += 1/(ld)b.size;
            if(b.one)score += (ld)b.size;
        }
        return round(score*1e9);
    }

    void output(){
        int t1 = 0,t2 = 0;
        rep(x,d)rep(y,d)rep(z,d)t1 = max(ans1[x][y][z],t1);
        rep(x,d)rep(y,d)rep(z,d)t2 = max(ans2[x][y][z],t2);
        cout << max(t1,t2) << endl;
        rep(x,d)rep(y,d)rep(z,d)cout << (ans1[z][y][x] <= 0 ? 0 : ans1[z][y][x]) << " ";
        cout << endl;
        rep(x,d)rep(y,d)rep(z,d)cout << (ans2[z][y][x] <= 0 ? 0 : ans2[z][y][x]) << " ";
        cout << endl;
    }
};

int main(){
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.input();
    solver.solve();
    solver.output();
    
    return 0;
}