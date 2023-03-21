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

const vector<vector<vector<int>>> b_dir = {
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
constexpr int DIR_NUM = 6;

int turn = 0;
queue<T> erase_block;
priority_queue<T> pq1,pq2;
vector<vector<vector<int>>> dig1,dig2;

struct Block{
    bool one;
    int size, d, id, surf1, surf2;
    vector<vector<vector<int>>> blo_idx1,blo_idx2;
    vector<T> place1,place2;

    Block(const T &b1,const T &b2,const int &_surf1,const int &_surf2,const int &_d,const int &_id){
        d = _d;
        id = _id;
        surf1 = _surf1; // 基準面1
        surf2 = _surf2; // 基準面2
        size = 1;
        place1.emplace_back(b1);
        place2.emplace_back(b2);
        blo_idx1.assign(d,vector<vector<int>>(d,vector<int>(d,-1)));
        blo_idx2.assign(d,vector<vector<int>>(d,vector<int>(d,-1)));
        auto [z1,y1,x1] = b1;
        auto [z2,y2,x2] = b2;
        if(z1 != -1)blo_idx1[z1][y1][x1] = size-1;
        if(z2 != -1)blo_idx2[z2][y2][x2] = size-1;
        one = (place1[0] == T(-1,-1,-1) || place2[0] == T(-1,-1,-1));
        // 回転は実装せず、全消し & 方向転換でとりあえずやる。(回転きつすぎ)
    }

    inline void expand(const int &nz1,const int &ny1,const int &nx1,const int &nz2,const int &ny2,const int &nx2,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        if(nz1 != -1){
            rep(i,DIR_NUM){
                int tz1 = nz1 + b_dir[surf1][0][i];
                int ty1 = ny1 + b_dir[surf1][1][i];
                int tx1 = nx1 + b_dir[surf1][2][i];
                if(!outField_3d(tz1,ty1,tx1) && state1[tz1][ty1][tx1] == id){
                    dig1[nz1][ny1][nx1]++;
                    dig1[tz1][ty1][tx1]++;
                }
            }
            blo_idx1[nz1][ny1][nx1] = size;
            place1.emplace_back(T(nz1,ny1,nx1));
            state1[nz1][ny1][nx1] = id;
        }

        if(nz2 != -1){
            rep(i,DIR_NUM){
                int tz2 = nz2 + b_dir[surf2][0][i];
                int ty2 = ny2 + b_dir[surf2][1][i];
                int tx2 = nx2 + b_dir[surf2][2][i];
                if(!outField_3d(tz2,ty2,tx2) && state2[tz2][ty2][tx2] == id){
                    dig2[nz2][ny2][nx2]++;
                    dig2[tz2][ty2][tx2]++;
                }
            }
            blo_idx2[nz2][ny2][nx2] = size;
            place2.emplace_back(T(nz2,ny2,nx2));
            state2[nz2][ny2][nx2] = id;
        }

        size++;
        return;
    }

    inline void erase(const int &nz1,const int &ny1,const int &nx1,const int &nz2,const int &ny2,const int &nx2,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        int idx1 = -1,idx2 = -1;
        if(nz1 != -1)idx1 = blo_idx1[nz1][ny1][nx1];
        if(nz2 != -1)idx2 = blo_idx2[nz2][ny2][nx2];
         
        if(idx1 >= 0)place1.erase(place1.begin()+idx1);
        if(idx2 >= 0)place2.erase(place2.begin()+idx2);

        // 次数 & blo_idx 更新
        if(nz1 != -1){
            dig1[nz1][ny1][nx1] = 0;
            blo_idx1[nz1][ny1][nx1] = -1;
            rep(i,DIR_NUM){
                int tz1 = nz1 + b_dir[surf1][0][i];
                int ty1 = ny1 + b_dir[surf1][1][i];
                int tx1 = nx1 + b_dir[surf1][2][i];
                if(!outField_3d(tz1,ty1,tx1) && state1[tz1][ty1][tx1] == id){
                    dig1[tz1][ty1][tx1]--;
                }
            }
            state1[nz1][ny1][nx1] = 0;
        }
        if(nz2 != -1){
            dig2[nz2][ny2][nx2] = 0;
            blo_idx2[nz2][ny2][nx2] = -1;
            rep(i,DIR_NUM){
                int tz2 = nz2 + b_dir[surf2][0][i];
                int ty2 = ny2 + b_dir[surf2][1][i];
                int tx2 = nx2 + b_dir[surf2][2][i];
                if(!outField_3d(tz2,ty2,tx2) && state2[tz2][ty2][tx2] == id){
                    dig2[tz2][ty2][tx2]--;
                }
            }
            state2[nz2][ny2][nx2] = 0;
        }
        size--;
        return;
    }

    inline bool outField_2d(int x,int y){
        return (x < 0 || x >= d || y < 0 || y >= d);
    }

    inline bool outField_3d(int x,int y,int z){
        return (x < 0 || x >= d || y < 0 || y >= d || z < 0 || z >= d);
    }
};



struct Solver{
    int d, r1, r2, r3, ;
    ll best_score, cand_score;
    vector<vector<bool>> sil1_f, sil1_r, sil2_f, sil2_r;
    vector<vector<vector<int>>> cand1, cand2, ans1, ans2, init1, init2;
    vector<Block> Block_list,init_list, ans_list;
    priority_queue<P> pq1, pq2;

    Solver(){}

    void input(){
        cin >> d;
        sil1_f.assign(d,vector<bool>(d,false));
        sil1_r.assign(d,vector<bool>(d,false));
        sil2_f.assign(d,vector<bool>(d,false));
        sil2_r.assign(d,vector<bool>(d,false));
        rep(i,4){
            rep(z,d){
                string s; cin >> s;
                if     (i == 0)rep(x,d)sil1_f[z][x] = (s[x] == '1');
                else if(i == 1)rep(y,d)sil1_r[z][y] = (s[y] == '1');
                else if(i == 2)rep(x,d)sil2_f[z][x] = (s[x] == '1');
                else           rep(y,d)sil2_r[z][y] = (s[y] == '1');
            }
        }
        dig1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        dig2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        cand1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        cand2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        rep(z,d){
            rep(y,d)rep(x,d)if(!sil1_f[z][x] || !sil1_r[z][y]){
                cand1[z][y][x] = -1;
                dig1[z][y][x] = 1e7;
            }
            rep(y,d)rep(x,d)if(!sil2_f[z][x] || !sil2_r[z][y]){
                cand2[z][y][x] = -1;
                dig2[z][y][x] = 1e7;
            }
        }
    }

    void solve(){
        utility::mytm.CodeStart();

        // 初期解生成
        int id = 1;
        while(utility::mytm.elapsed() <= TIME_LIMIT){
            vector<T> cand1_v,cand2_v;
            rep(z,d)rep(y,d)rep(x,d){
                if(sil1_f[z][x] && sil1_r[z][y]){
                    bool f = true;
                    rep(ty,d)f &= (cand1[z][ty][x] <= 0);
                    if(f){
                        cand1_v.emplace_back(T(z,y,x));
                        break;
                    }
                    f = true;
                    rep(tx,d)f &= (cand1[z][y][tx] <= 0);
                    if(f){
                        cand1_v.emplace_back(T(z,y,x));
                        break;
                    }
                }
                if(sil2_f[z][x] && sil2_r[z][y]){
                    bool f = true;
                    rep(ty,d)f &= (cand2[z][ty][x] <= 0);
                    if(f){
                        cand2_v.emplace_back(T(z,y,x));
                        break;
                    }
                    f = true;
                    rep(tx,d)f &= (cand2[z][y][tx] <= 0);
                    if(f){
                        cand2_v.emplace_back(T(z,y,x));
                        break;
                    }
                }
            }

            auto [nz1,ny1,nx1] = (cand1_v.empty() ? T(-1,-1,-1) : cand1_v[rand_int()%cand1_v.size()]);
            auto [nz2,ny2,nx2] = (cand2_v.empty() ? T(-1,-1,-1) : cand2_v[rand_int()%cand2_v.size()]);

            if(nz1 == -1 && nz2 == -1)break;
            if(nz1 == -1){
                vector<T> cv;
                rep(z,d)rep(y,d)rep(x,d)if(!cand1[z][y][x])cv.emplace_back(T(z,y,x));
                if(!cv.empty()){
                    auto [z,y,x] = cv[rand_int()%cv.size()];
                    nz1 = z, ny1 = y, nx1 = x;
                }
                else{
                    int rnd = rand_int()%Block_list.size();
                    while(!Block_list[rnd].place1.empty()){
                        auto [z1,y1,x1] = Block_list[rnd].place1.back();
                        auto [z2,y2,x2] = Block_list[rnd].place2.back();
                        Block_list[rnd].erase(z1,y1,x1,z2,y2,x2,cand1,cand2);
                    }
                    Block_list.erase(Block_list.begin()+rnd);
                    continue;
                }
            }
            if(nz2 == -1){
                vector<T> cv;
                rep(z,d)rep(y,d)rep(x,d)if(!cand2[z][y][x])cv.emplace_back(T(z,y,x));
                if(!cv.empty()){
                    auto [z,y,x] = cv[rand_int()%cv.size()];
                    nz2 = z, ny2 = y, nx2 = x;
                }
                else{
                    int rnd = rand_int()%Block_list.size();
                    while(!Block_list[rnd].place1.empty()){
                        auto [z1,y1,x1] = Block_list[rnd].place1.back();
                        auto [z2,y2,x2] = Block_list[rnd].place2.back();
                        Block_list[rnd].erase(z1,y1,x1,z2,y2,x2,cand1,cand2);
                    }
                    Block_list.erase(Block_list.begin()+rnd);
                    continue;
                }
            }

            Block_list.emplace_back(Block(T(nz1,ny1,nx1),T(nz2,ny2,nx2),rand_int()%DIR_NUM,rand_int()%DIR_NUM,d,id));
            best_score = LLONG_MAX;
            cand1[nz1][ny1][nx1] = id;
            cand2[nz2][ny2][nx2] = id;
            init1 = cand1;
            init2 = cand2;
            init_list = Block_list;

            // Block_list.back() を方向を変えて探索
            rep(d1,DIR_NUM){
                rep(d2,DIR_NUM){
                    cand_score = LLONG_MAX-1;
                    Block_list.back().surf1 = d1;
                    Block_list.back().surf2 = d2;
                    while(turn <= 100){
                        query3(Block_list.size()-1);
                        turn++;
                    }
                    // cerr << cand_score << " " << best_score << endl;
                    if(cand_score < best_score){
                        best_score = cand_score;
                        ans1 = cand1;
                        ans2 = cand2;
                        ans_list = Block_list;
                    }
                    cand1 = init1;
                    cand2 = init2;
                    Block_list = init_list;
                    turn = 0;
                }
            }

            // シルエット完成しているか
            bool f = true;
            rep(z,d){
                rep(y,d){
                    int cnt = 0;
                    rep(x,d)cnt += (cand1[z][y][x] > 0);
                    f &= (cnt > 0);
                }
                rep(x,d){
                    int cnt = 0;
                    rep(y,d)cnt += (cand2[z][y][x] > 0);
                    f &= (cnt > 0);
                }
            }
            if(f)break;

            cand1 = ans1;
            cand2 = ans2;
            Block_list = ans_list;
            id++;
        }
        return;
    }

    inline void query1(){
        // size1 方向転換
        int r1 = rand_int()%Block_list.size();
        if(Block_list[r1].size == 1){
            if(rand_int()%2){
                Block_list[r1].surf1 = rand_int()%DIR_NUM;
                Block_list[r1].surf2 = rand_int()%DIR_NUM;
            }
        }
    }

    inline void query2(){
        // 削除ブロック決定
        int r1 = rand_int()%Block_list.size();

        for(auto [nz1,ny1,nx1]:Block_list[r1].place1){
            if(nz1 == -1)continue;
            cand1[nz1][ny1][nx1] = 0;
        }
        for(auto [nz2,ny2,nx2]:Block_list[r1].place2){
            if(nz2 == -1)continue;
            cand2[nz2][ny2][nx2] = 0;
        }
        bool f = true;
        rep(z,d)rep(x,d){
            if(!sil1_f[z][x])continue;
            int cnt = 0;
            rep(y,d)cnt += (cand1[z][y][x] > 0);
            f &= (cnt > 0);
        }
        rep(z,d)rep(y,d){
            if(!sil1_r[z][y])continue;
            int cnt = 0;
            rep(x,d)cnt += (cand1[z][y][x] > 0);
            f &= (cnt > 0);
        }
        rep(z,d)rep(x,d){
            if(!sil2_f[z][x])continue;
            int cnt = 0;
            rep(y,d)cnt += (cand2[z][y][x] > 0);
            f &= (cnt > 0);
        }
        rep(z,d)rep(y,d){
            if(!sil2_r[z][y])continue;
            int cnt = 0;
            rep(x,d)cnt += (cand2[z][y][x] > 0);
            f &= (cnt > 0);
        }

        if(f){
            for(auto [nz1,ny1,nx1]:Block_list[r1].place1){
                if(nz1 == -1)continue;
                dig1[nz1][ny1][nx1] = 0;
            }
            for(auto [nz2,ny2,nx2]:Block_list[r1].place2){
                if(nz2 == -1)continue;
                dig2[nz2][ny2][nx2] = 0;
            }
            Block_list.erase(Block_list.begin()+r1);
        }
        else{
            for(auto [nz1,ny1,nx1]:Block_list[r1].place1){
                if(nz1 == -1)continue;
                cand1[nz1][ny1][nx1] = Block_list[r1].id;
            }
            for(auto [nz2,ny2,nx2]:Block_list[r1].place2){
                if(nz2 == -1)continue;
                cand2[nz2][ny2][nx2] = Block_list[r1].id;
            }
        }
    }

    inline void query3(int idx){
        // r1 : どのブロックを拡張するか
        // r2 : 何ブロック目を起点とするか
        // r3 : どの方向に伸ばしていくか
        r1 = (idx >= 0 ? idx :rand_int()%Block_list.size());
        r2 = rand_int()%Block_list[r1].size;
        r3 = rand_int()%DIR_NUM;

        auto [z1,y1,x1] = Block_list[r1].place1[r2];
        int nz1 = z1 + b_dir[Block_list[r1].surf1][0][r3];
        int ny1 = y1 + b_dir[Block_list[r1].surf1][1][r3];
        int nx1 = x1 + b_dir[Block_list[r1].surf1][2][r3];

        auto [z2,y2,x2] = Block_list[r1].place2[r2];
        int nz2 = z2 + b_dir[Block_list[r1].surf2][0][r3];
        int ny2 = y2 + b_dir[Block_list[r1].surf2][1][r3];
        int nx2 = x2 + b_dir[Block_list[r1].surf2][2][r3];

        if(outField_3d(nz1,ny1,nx1) || cand1[nz1][ny1][nx1])return;
        if(outField_3d(nz2,ny2,nx2) || cand2[nz2][ny2][nx2])return;

        Block_list[r1].expand(nz1,ny1,nx1,nz2,ny2,nx2,cand1,cand2);

        ll now_score = calcScore();
        if(now_score < cand_score){
            cand_score = now_score;
            turn = 0;
        }
        else{
            Block_list[r1].erase(nz1,ny1,nx1,nz2,ny2,nx2,cand1,cand2);
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

    inline bool outField_2d(int x,int y){
        return (x < 0 || x >= d || y < 0 || y >= d);
    }

    inline bool outField_3d(int x,int y,int z){
        return (x < 0 || x >= d || y < 0 || y >= d || z < 0 || z >= d);
    }

    void idxCompression(){
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

    void output(){
        idxCompression();
        int t1 = 0,t2 = 0;
        rep(x,d)rep(y,d)rep(z,d)t1 = max(ans1[z][y][x],t1);
        rep(x,d)rep(y,d)rep(z,d)t2 = max(ans2[z][y][x],t2);
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