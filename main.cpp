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
int turn = 0;
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
    int size, d, id, surf1, surf2;
    vector<vector<vector<int>>> blo_idx1,blo_idx2;
    vector<T> place1,place2;
    stack<T> cache1,cache2;

    Block(T b1, T b2, int _surf1, int _surf2, int _d, int _id){
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

    inline void expand(int nz1,int ny1,int nx1,int nz2,int ny2,int nx2,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        // 次数更新
        rep(i,DIR_NUM){
            int tz1 = nz1 + b_dir[surf1][0][i];
            int ty1 = ny1 + b_dir[surf1][1][i];
            int tx1 = nx1 + b_dir[surf1][2][i];
            if(!outField_3d(tz1,ty1,tx1) && state1[tz1][ty1][tx1] == id){
                dig1[nz1][ny1][nx1]++;
                dig1[tz1][ty1][tx1]++;
            }
        }
        rep(i,DIR_NUM){
            int tz2 = nz2 + b_dir[surf2][0][i];
            int ty2 = ny2 + b_dir[surf2][1][i];
            int tx2 = nx2 + b_dir[surf2][2][i];
            if(!outField_3d(tz2,ty2,tx2) && state2[tz2][ty2][tx2] == id){
                dig2[nz2][ny2][nx2]++;
                dig2[tz2][ty2][tx2]++;
            }
        }

        // blo_idx更新
        blo_idx1[nz1][ny1][nx1] = size;
        blo_idx2[nz2][ny2][nx2] = size;

        // add_block
        place1.emplace_back(T(nz1,ny1,nx1));
        place2.emplace_back(T(nz2,ny2,nx2));
        state1[nz1][ny1][nx1] = id;
        state2[nz2][ny2][nx2] = id;
        size++;
        return;
    }

    inline void erase(int nz1,int ny1,int nx1,int nz2,int ny2,int nx2,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        int idx1 = blo_idx1[nz1][ny1][nx1];
        int idx2 = blo_idx2[nz2][ny2][nx2];
        
        auto [z1,y1,x1] = place1[idx1];
        auto [z2,y2,x2] = place2[idx2];
        place1.erase(place1.begin()+idx1);
        place2.erase(place2.begin()+idx2);

        // 次数更新
        dig1[z1][y1][x1] = 0;
        dig2[z2][y2][x2] = 0;
        rep(i,DIR_NUM){
            int tz1 = z1 + b_dir[surf1][0][i];
            int ty1 = y1 + b_dir[surf1][1][i];
            int tx1 = x1 + b_dir[surf1][2][i];
            if(!!outField_3d(tz1,ty1,tx1) && state1[tz1][ty1][tx1] == id){
                dig1[tz1][ty1][tx1]--;
            }
        }
        rep(i,DIR_NUM){
            int tz2 = z2 + b_dir[surf2][0][i];
            int ty2 = y2 + b_dir[surf2][1][i];
            int tx2 = x2 + b_dir[surf2][2][i];
            if(!outField_3d(tz2,ty2,tx2) && state2[tz2][ty2][tx2] == id){
                dig2[tz2][ty2][tx2]--;
            }
        }

        // blo_idx更新
        blo_idx1[z1][y1][x1] = -1;
        blo_idx2[z2][y2][x2] = -1;

        state1[z1][y1][x1] = 0;
        state2[z2][y2][x2] = 0;
        size--;

        // while(!place1.empty() && num--){
        //     auto [z1,y1,x1] = place1.back(); place1.pop_back();
        //     auto [z2,y2,x2] = place2.back(); place2.pop_back();

        //     // 次数確認
        //     dig1[z1][y1][x1] = 0;
        //     dig2[z2][y2][x2] = 0;
        //     rep(i,DIR_NUM){
        //         int tz1 = z1 + b_dir[surf1][0][i];
        //         int ty1 = y1 + b_dir[surf1][1][i];
        //         int tx1 = x1 + b_dir[surf1][2][i];
        //         if(!!outField_3d(tz1,ty1,tx1) && state1[tz1][ty1][tx1] == id){
        //             dig1[tz1][ty1][tx1]--;
        //         }
        //     }
        //     rep(i,DIR_NUM){
        //         int tz2 = z2 + b_dir[surf2][0][i];
        //         int ty2 = y2 + b_dir[surf2][1][i];
        //         int tx2 = x2 + b_dir[surf2][2][i];
        //         if(!outField_3d(tz2,ty2,tx2) && state2[tz2][ty2][tx2] == id){
        //             dig2[tz2][ty2][tx2]--;
        //         }
        //     }

        //     state1[z1][y1][x1] = 0;
        //     state2[z2][y2][x2] = 0;
        //     size--;
        // }
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
    int d;
    ll best_score;
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
        dig1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        dig2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
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
            rep(y,d)rep(x,d)if(!sil1_f[z][x] || !sil1_r[z][y]){
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
            rep(y,d)rep(x,d)if(!sil2_f[z][x] || !sil2_r[z][y]){
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
        best_score = calcScore();
        while(utility::mytm.elapsed() <= TIME_LIMIT){
            // query候補
            // 1. 1Block延長
            // 2. 1Block削除
            // 3. 合併
            // 4. 削除
            // 5. 同型同士ペア交換
            // 6. 進捗無し → 何手か戻す (archiveが大事になりそう)

            int query = rand_int()%100;
            if     (query <= 10)query1(); // 方向転換
            else if(query <= 20)query2(); // ブロック削除
            else                query3(); // ブロック拡張
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

        // for(auto block:Block_list){
        //     rep(z,d){
        //         rep(y,d){
        //             rep(x,d){
        //                 cerr << block.blo_idx1[z][y][x] << " ";
        //             }
        //             cerr << endl;
        //         }
        //         cerr << endl;
        //     }
        //     cerr << endl;
        // }
        // for(auto block:Block_list){
        //     rep(z,d){
        //         rep(y,d){
        //             rep(x,d){
        //                 cerr << block.blo_idx2[z][y][x] << " ";
        //             }
        //             cerr << endl;
        //         }
        //         cerr << endl;
        //     }
        //     cerr << endl;
        // }

        // rep(z,d){
        //     rep(y,d){
        //         rep(x,d)cerr << dig1[z][y][x] << " ";
        //         cerr << endl;
        //     }
        //     cerr << endl;
        // }
        // cerr << endl;
        // rep(z,d){
        //     rep(y,d){
        //         rep(x,d)cerr << dig2[z][y][x] << " ";
        //         cerr << endl;
        //     }
        //     cerr << endl;
        // }
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
        // ブロック削除query

        // 削除ブロック決定
        int r1 = rand_int()%Block_list.size();

        for(auto [nz1,ny1,nx1]:Block_list[r1].place1){
            if(nz1 == -1)continue;
            ans1[nz1][ny1][nx1] = 0;
        }
        for(auto [nz2,ny2,nx2]:Block_list[r1].place2){
            if(nz2 == -1)continue;
            ans2[nz2][ny2][nx2] = 0;
        }
        bool f = true;
        rep(z,d)rep(x,d){
            if(!sil1_f[z][x])continue;
            int cnt = 0;
            rep(y,d)cnt += (ans1[z][y][x] > 0);
            f &= (cnt > 0);
        }
        rep(z,d)rep(y,d){
            if(!sil1_r[z][y])continue;
            int cnt = 0;
            rep(x,d)cnt += (ans1[z][y][x] > 0);
            f &= (cnt > 0);
        }
        rep(z,d)rep(x,d){
            if(!sil2_f[z][x])continue;
            int cnt = 0;
            rep(y,d)cnt += (ans2[z][y][x] > 0);
            f &= (cnt > 0);
        }
        rep(z,d)rep(y,d){
            if(!sil2_r[z][y])continue;
            int cnt = 0;
            rep(x,d)cnt += (ans2[z][y][x] > 0);
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
                ans1[nz1][ny1][nx1] = Block_list[r1].id;
            }
            for(auto [nz2,ny2,nx2]:Block_list[r1].place2){
                if(nz2 == -1)continue;
                ans2[nz2][ny2][nx2] = Block_list[r1].id;
            }
        }
    }

    inline void query3(){
        // どのブロックを拡張するか
        int r1 = rand_int()%Block_list.size();
        // 何ブロック目を起点とするか
        int r2 = rand_int()%Block_list[r1].size;
        // どの方向に伸ばしていくか
        int r3 = rand_int()%DIR_NUM;

        auto [z1,y1,x1] = Block_list[r1].place1[r2];
        int nz1 = z1 + b_dir[Block_list[r1].surf1][0][r3];
        int ny1 = y1 + b_dir[Block_list[r1].surf1][1][r3];
        int nx1 = x1 + b_dir[Block_list[r1].surf1][2][r3];

        auto [z2,y2,x2] = Block_list[r1].place2[r2];
        int nz2 = z2 + b_dir[Block_list[r1].surf2][0][r3];
        int ny2 = y2 + b_dir[Block_list[r1].surf2][1][r3];
        int nx2 = x2 + b_dir[Block_list[r1].surf2][2][r3];

        if(outField_3d(nz1,ny1,nx1) || ans1[nz1][ny1][nx1])return;
        if(outField_3d(nz2,ny2,nx2) || ans2[nz2][ny2][nx2])return;

        // expand先が他ブロックの場合
        // 1. 消去可能か。(他ブロックの連結条件が途切れないか)
        // 2. 1がtrueならerase
        // 3. expand
        // 4. score改善ならそのまま、改悪ならerase → expand(復元)

        

        Block_list[r1].expand(nz1,ny1,nx1,nz2,ny2,nx2,ans1,ans2);

        ll now_score = calcScore();
        cerr << best_score << " " << now_score << endl;
        if(now_score < best_score){
            best_score = now_score;
            if(turn <= 10)output();
            turn++;
        }
        else{
            Block_list[r1].erase(nz1,ny1,nx1,nz2,ny2,nx2,ans1,ans2);
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