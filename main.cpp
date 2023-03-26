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
typedef tuple<int, int, int, int, int, int> T2;
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
#define TIME_LIMIT 5800
inline double temp(int start) {
    double start_temp = 500,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}
 
//焼きなましの採用確率
inline double prob(ll best,ll now,int start) {
    return exp((double)(best - now) / temp(start));
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
const vector<int> dx = { 1,-1, 0, 0, 0, 0};
const vector<int> dy = { 0, 0, 1,-1, 0, 0};
const vector<int> dz = { 0, 0, 0, 0, 1,-1};
constexpr int DIR_NUM = 6;
vector<vector<int>> exist_f1, exist_r1, exist_f2, exist_r2;

struct Block{
    int size, d, id, surf1, surf2;
    vector<T> place1,place2;

    Block(const T &b1,const T &b2,const int &_surf1,const int &_surf2,const int &_d,const int &_id){
        d = _d;
        id = _id;
        surf1 = _surf1; // 基準面1
        surf2 = _surf2; // 基準面2
        size = 1;
        place1.emplace_back(b1);
        place2.emplace_back(b2);
    }

    inline void expand(const int &nz1,const int &ny1,const int &nx1,const int &nz2,const int &ny2,const int &nx2,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        place1.emplace_back(T(nz1,ny1,nx1));
        place2.emplace_back(T(nz2,ny2,nx2));
        state1[nz1][ny1][nx1] = id;
        state2[nz2][ny2][nx2] = id;
        size++;
        return;
    }

    inline void erase(const int &nz1,const int &ny1,const int &nx1,const int &nz2,const int &ny2,const int &nx2,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        int idx = 0;
        for(auto &&[z,y,x]:place1){
            if(nz1 == z && ny1== y && nx1 == x)break;
            idx++;
        }
        place1.erase(place1.begin()+idx);
        place2.erase(place2.begin()+idx);
        state1[nz1][ny1][nx1] = 0;
        state2[nz2][ny2][nx2] = 0;
        size--;
        return;
    }
};



Block tmp(T(0,0,0),T(0,0,0),0,0,0,0);

struct Solver{
    double temp, rnd_d, score;
    int d, id, start_time, start_id, tz1, ty1, tx1, tz2, ty2, tx2, time, rnd, erase_id;

    vector<vector<bool>> sil1_f, sil1_r, sil2_f, sil2_r;
    vector<T> cand1_v, cand2_v, cv;

    vector<Block> Block_list, last_list;
    ll best_score, cand_score, last_score, now_score, final_score;
    vector<vector<vector<int>>> cand1, cand2, last1, last2, final1, final2;

    int query_time, yaki_time, expand_time;
    vector<vector<vector<bool>>> visited;
    queue<Block> erase_archive;
    queue<T> todo, visit_reset;

    Solver(){
        id = 1;
    }

    void input(){
        cin >> d;
        sil1_f.assign(d,vector<bool>(d,false));
        sil1_r.assign(d,vector<bool>(d,false));
        sil2_f.assign(d,vector<bool>(d,false));
        sil2_r.assign(d,vector<bool>(d,false));
        exist_f1.assign(d,vector<int>(d,0));
        exist_r1.assign(d,vector<int>(d,0));
        exist_f2.assign(d,vector<int>(d,0));
        exist_r2.assign(d,vector<int>(d,0));
        visited.assign(d,vector<vector<bool>>(d,vector<bool>(d,false)));
        rep(i,4){
            rep(z,d){
                string s; cin >> s;
                if     (i == 0)rep(x,d)sil1_f[z][x] = (s[x] == '1');
                else if(i == 1)rep(y,d)sil1_r[z][y] = (s[y] == '1');
                else if(i == 2)rep(x,d)sil2_f[z][x] = (s[x] == '1');
                else           rep(y,d)sil2_r[z][y] = (s[y] == '1');
            }
        }
        cand1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        cand2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        rep(z,d)rep(y,d)rep(x,d){
            if(!sil1_f[z][x] || !sil1_r[z][y])cand1[z][y][x] = -1;
            if(!sil2_f[z][x] || !sil2_r[z][y])cand2[z][y][x] = -1;
        }
    }

    void solve(){
        utility::mytm.CodeStart();

        // 初期解生成
        constructAnswer();
        last_score = calcScore();
      	final_score = last_score;
        last_list = Block_list;
        last1 = cand1;
        last2 = cand2;
      	final1 = last1;
        final2 = last2;

        start_time = utility::mytm.elapsed();
        query_time = 0, yaki_time = 0, expand_time = 0;
        while(!erase_archive.empty())erase_archive.pop();
        
        // 焼きなまし法
        while(utility::mytm.elapsed() <= TIME_LIMIT){
            
            preErase();        // 部分破壊
            constructAnswer(); // 再構築
            cand_score = calcScore();

            temp = prob(last_score/100000,cand_score/100000,start_time);
            rnd_d = rand_double();
 
            if(temp > rnd_d){
                last_score = cand_score;
                last1 = cand1;
                last2 = cand2;
                last_list = Block_list;
                // if(temp < 1.0){
                //     cerr << cand_score << " " << final_score << '\n';
                //     yaki_time++;
                // }
                if(cand_score < final_score){
                    // 最終提出用 archive
                    final_score = cand_score;
                    final1 = cand1;
                    final2 = cand2;
                }
            }
            else{
                // 差分更新
                while(expand_time--)eraseBlock(Block_list.size()-1,false);
                while(!erase_archive.empty()){
                    Block &tmp = erase_archive.front();
                    Block_list.emplace_back(tmp);
                    rep(i,tmp.size){
                        auto &&[z1,y1,x1] = tmp.place1[i];
                        auto &&[z2,y2,x2] = tmp.place2[i];
                        cand1[z1][y1][x1] = tmp.id;
                        cand2[z2][y2][x2] = tmp.id;

                        // bitでブロックの有無管理
                        exist_f1[z1][x1] |= (1 << y1);
                        exist_r1[z1][y1] |= (1 << x1);
                        exist_f2[z2][x2] |= (1 << y2);
                        exist_r2[z2][y2] |= (1 << x2);
                    }
                    erase_archive.pop();
                }
            }
            while(!erase_archive.empty())erase_archive.pop();
            expand_time = 0;
            // query_time++;
        }
        // cerr << "query_time : " << query_time << '\n';
        cerr << "final_score : " << final_score << '\n';
        // cerr << "yaki_time : " << yaki_time << '\n';
        return;
    }

    inline void constructAnswer(){
        start_id = id;
        while(true){
            while(!cand1_v.empty())cand1_v.pop_back();
            while(!cand2_v.empty())cand2_v.pop_back();
            rep(z,d){
                rep(y,d){
                    if(sil1_r[z][y] || sil2_r[z][y]){
                        rep(x,d){
                            if(sil1_f[z][x] && sil1_r[z][y] && (!exist_f1[z][x] || !exist_r1[z][y])){
                                cand1_v.emplace_back(T(z,y,x));
                            }
                            if(sil2_f[z][x] && sil2_r[z][y] && (!exist_f2[z][x] || !exist_r2[z][y])){
                                cand2_v.emplace_back(T(z,y,x));
                            }
                        }
                    }
                }
            }

            auto &&[nz1,ny1,nx1] = (cand1_v.empty() ? T(-1,-1,-1) : cand1_v[rand_int()%cand1_v.size()]);
            auto &&[nz2,ny2,nx2] = (cand2_v.empty() ? T(-1,-1,-1) : cand2_v[rand_int()%cand2_v.size()]);

            // シルエット完成時
            if(nz1 == -1 && nz2 == -1)break;

            // 片方シルエット完成時
            if(nz1 == -1){
                while(!cv.empty())cv.pop_back();
                rep(z,d)rep(y,d){
                    if(!sil1_r[z][y])continue;
                    rep(x,d)if(!cand1[z][y][x])cv.emplace_back(T(z,y,x));
                }
                if(!cv.empty()){
                    auto &&[z,y,x] = cv[rand_int()%cv.size()];
                    nz1 = z, ny1 = y, nx1 = x;
                }
                else{
                    rnd = rand_int()%Block_list.size();
                    if(start_id <= Block_list[rnd].id){
                        eraseBlock(rnd,false);
                        expand_time--;
                    }
                    else eraseBlock(rnd,true);
                    continue;
                }
            }
            if(nz2 == -1){
                while(!cv.empty())cv.pop_back();
                rep(z,d)rep(y,d){
                    if(!sil2_r[z][y])continue;
                    rep(x,d)if(!cand2[z][y][x])cv.emplace_back(T(z,y,x));
                }
                if(!cv.empty()){
                    auto &&[z,y,x] = cv[rand_int()%cv.size()];
                    nz2 = z, ny2 = y, nx2 = x;
                }
                else{
                    rnd = rand_int()%Block_list.size();
                    if(start_id <= Block_list[rnd].id){
                        eraseBlock(rnd,false);
                        expand_time--;
                    }
                    else eraseBlock(rnd,true);
                    continue;
                }
            }

            Block_list.emplace_back(Block(T(nz1,ny1,nx1),T(nz2,ny2,nx2),rand_int()%DIR_NUM,rand_int()%DIR_NUM,d,id));
            best_score = LLONG_MAX;
            cand1[nz1][ny1][nx1] = id;
            cand2[nz2][ny2][nx2] = id;
            exist_f1[nz1][nx1] |= (1 << ny1);
            exist_r1[nz1][ny1] |= (1 << nx1);
            exist_f2[nz2][nx2] |= (1 << ny2);
            exist_r2[nz2][ny2] |= (1 << nx2);
    
            // 追加Blockの方向全探索
            rep(d1,DIR_NUM){
                rep(d2,DIR_NUM){
                    cand_score = LLONG_MAX-1;
                    Block_list.back().surf1 = d1;
                    Block_list.back().surf2 = d2;
                    
                    // 最大限に拡張
                    query(Block_list.size() - 1);
                    cand_score = calcScore();

                    if(cand_score < best_score){
                        best_score = cand_score;
                        tmp = Block_list.back();
                    }
                    // 差分更新で元に戻す
                    while(Block_list.back().size > 1){
                        auto &&[z1,y1,x1] = Block_list.back().place1.back();
                        auto &&[z2,y2,x2] = Block_list.back().place2.back();
                        Block_list.back().erase(z1,y1,x1,z2,y2,x2,cand1,cand2);
                    }
                }
            }
            
            // ブロック採用
            rep(i,tmp.size){
                auto &&[z1,y1,x1] = tmp.place1[i];
                auto &&[z2,y2,x2] = tmp.place2[i];
                cand1[z1][y1][x1] = id;
                cand2[z2][y2][x2] = id;
                exist_f1[z1][x1] |= (1 << y1);
                exist_r1[z1][y1] |= (1 << x1);
                exist_f2[z2][x2] |= (1 << y2);
                exist_r2[z2][y2] |= (1 << x2);
            }
            swap(Block_list.back(),tmp);
            expand_time++;
            id++;
        }
    }

    inline void preErase(){
        // ブロック1 or 2個破壊
        time = rand_int()%2 + 1;
        rnd = rand_int()%Block_list.size();
        if(time == 2){
            auto &&[nz,ny,nx] = Block_list[rnd].place1[rand_int()%Block_list[rnd].place1.size()];
            todo.push(T(nz,ny,nx));
            while(!todo.empty() && time > 0){
                auto &&[z,y,x] = todo.front();
                if(visited[z][y][x]){
                    todo.pop();
                    continue;
                }
                visited[z][y][x] = true;
                visit_reset.push(T(z,y,x));
                if(cand1[z][y][x] > 0 && cand1[z][y][x] != cand1[nz][ny][nx]){
                    erase_id = cand1[z][y][x];
                    break;
                }
                rep(i,DIR_NUM){
                    tz1 = z + dz[i];
                    ty1 = y + dy[i];
                    tx1 = x + dx[i];
                    if(outField(tz1,ty1,tx1) || visited[tz1][ty1][tx1])continue;
                    todo.push(T(tz1,ty1,tx1));
                }
                todo.pop();
            }
            while(!todo.empty())todo.pop();
            while(!visit_reset.empty()){
                auto &&[z,y,x] = visit_reset.front();
                visited[z][y][x] = false;
                visit_reset.pop();
            }
            // 2個破壊
            eraseBlock(rnd,true);
            rep(i,Block_list.size()){
                if(erase_id == Block_list[i].id){
                    eraseBlock(i,true);
                    break;
                }
            }
        }
        else eraseBlock(rnd,true); // 1個破壊
    }

    // ブロック拡張query
    inline void query(const int &r1){
        // r1 : どのブロックを拡張するか
        // r2 : 何ブロック目を起点とするか
        // r3 : どの方向に伸ばしていくか
        rep(r2,Block_list[r1].size){
            rep(r3,DIR_NUM){
                auto &&[z1,y1,x1] = Block_list[r1].place1[r2];
                tz1 = z1 + b_dir[Block_list[r1].surf1][0][r3];
                ty1 = y1 + b_dir[Block_list[r1].surf1][1][r3];
                tx1 = x1 + b_dir[Block_list[r1].surf1][2][r3];
                if(outField(tz1,ty1,tx1) || cand1[tz1][ty1][tx1])continue;

                auto &&[z2,y2,x2] = Block_list[r1].place2[r2];
                tz2 = z2 + b_dir[Block_list[r1].surf2][0][r3];
                ty2 = y2 + b_dir[Block_list[r1].surf2][1][r3];
                tx2 = x2 + b_dir[Block_list[r1].surf2][2][r3];
                if(outField(tz2,ty2,tx2) || cand2[tz2][ty2][tx2])continue;

                Block_list[r1].expand(tz1,ty1,tx1,tz2,ty2,tx2,cand1,cand2);
            }
        }
    }

    inline void eraseBlock(const int &idx, const bool &flag){
        if(flag)erase_archive.push(Block_list[idx]);

        while(!Block_list[idx].place1.empty()){
            auto &&[z1,y1,x1] = Block_list[idx].place1.back();
            auto &&[z2,y2,x2] = Block_list[idx].place2.back();
            Block_list[idx].erase(z1,y1,x1,z2,y2,x2,cand1,cand2);
            
            exist_f1[z1][x1] ^= (1 << y1);
            exist_r1[z1][y1] ^= (1 << x1);
            exist_f2[z2][x2] ^= (1 << y2);
            exist_r2[z2][y2] ^= (1 << x2);
        }
        Block_list.erase(Block_list.begin()+idx);
    }

    inline ll calcScore(){
        score = 0;
        for(auto &&b:Block_list)if(b.size)score += 1/(double)b.size;
        return score*1000000000;
    }

    inline bool outField(const int &x,const int &y,const int &z){
        return (x < 0 || x >= d || y < 0 || y >= d || z < 0 || z >= d);
    }

    void idxCompression(){
        // index圧縮
        vector<int> idx;
        rep(i,d)rep(j,d)rep(k,d)if(final1[i][j][k] > 0)idx.emplace_back(final1[i][j][k]);
        rep(i,d)rep(j,d)rep(k,d)if(final2[i][j][k] > 0)idx.emplace_back(final2[i][j][k]);
        sort(idx.begin(),idx.end());
        idx.erase(unique(idx.begin(),idx.end()),idx.end());
        rep(i,d)rep(j,d)rep(k,d){
            if(final1[i][j][k] > 0){
                final1[i][j][k] = lower_bound(idx.begin(),idx.end(),final1[i][j][k])-idx.begin()+1;
            }
            if(final2[i][j][k] > 0){
                final2[i][j][k] = lower_bound(idx.begin(),idx.end(),final2[i][j][k])-idx.begin()+1;
            }
        }
    }

    void output(){
        idxCompression();
        int t1 = 0,t2 = 0;
        rep(x,d)rep(y,d)rep(z,d)t1 = max(final1[z][y][x],t1);
        rep(x,d)rep(y,d)rep(z,d)t2 = max(final2[z][y][x],t2);
        cout << max(t1,t2) << '\n';
        rep(x,d)rep(y,d)rep(z,d)cout << (final1[z][y][x] <= 0 ? 0 : final1[z][y][x]) << " ";
        cout << '\n';
        rep(x,d)rep(y,d)rep(z,d)cout << (final2[z][y][x] <= 0 ? 0 : final2[z][y][x]) << " ";
        cout << '\n';
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