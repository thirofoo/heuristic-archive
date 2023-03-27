#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef long double ld;
typedef pair<int, int> P;
typedef tuple<int, int, int> T;
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
#define TIME_LIMIT 58000
inline double temp(int start) {
    double start_temp = 500,end_temp = 250;
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
// ｜　　　　　｜  ・　　◎⇒ x方向
// ｜　　０　　｜ /　　　⇓
// ｜　　　　　｜/　　　z方向
// ・ーーーーー・

const vector<vector<vector<int>>> b_dir = {
    // b_dir[i] : 面iを基準面とした時の軸の方向
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
constexpr int DESTROY_NUM = 2;
vector<vector<short int>> exist_f1, exist_r1, exist_f2, exist_r2, final_exist_f1, final_exist_r1, final_exist_f2, final_exist_r2;
vector<vector<vector<int>>> blo1_idx, blo2_idx;

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
        blo1_idx[nz1][ny1][nx1] = size;
        blo2_idx[nz2][ny2][nx2] = size;
        size++;
        return;
    }

    inline void shave(const int &nz1,const int &ny1,const int &nx1,const int &nz2,const int &ny2,const int &nx2,vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        place1.erase(place1.begin()+blo1_idx[nz1][ny1][nx1]);
        place2.erase(place2.begin()+blo2_idx[nz2][ny2][nx2]);
        state1[nz1][ny1][nx1] = 0;
        state2[nz2][ny2][nx2] = 0;
        blo1_idx[nz1][ny1][nx1] = -1;
        blo2_idx[nz2][ny2][nx2] = -1;
        size--;
        return;
    }

    inline void shaveBack(vector<vector<vector<int>>> &state1,vector<vector<vector<int>>> &state2){
        auto &&[nz1,ny1,nx1] = place1.back();
        auto &&[nz2,ny2,nx2] = place2.back();
        state1[nz1][ny1][nx1] = 0;
        state2[nz2][ny2][nx2] = 0;
        blo1_idx[nz1][ny1][nx1] = -1;
        blo2_idx[nz2][ny2][nx2] = -1;
        exist_f1[nz1][nx1] ^= (1 << ny1);
        exist_r1[nz1][ny1] ^= (1 << nx1);
        exist_f2[nz2][nx2] ^= (1 << ny2);
        exist_r2[nz2][ny2] ^= (1 << nx2);
        place1.pop_back();
        place2.pop_back();
        size--;
        return;
    }
};



Block tmp(T(0,0,0),T(0,0,0),0,0,0,0);

struct Solver{
    double temp, rnd_d, score;
    int d, id, start_time, start_id, tz1, ty1, tx1, tz2, ty2, tx2, time, rnd;

    vector<vector<bool>> sil1_f, sil1_r, sil2_f, sil2_r;
    vector<T> cand1_v, cand2_v, cv;

    vector<Block> Block_list, final_list;
    ll best_score, cand_score, last_score, now_score, final_score;
    vector<vector<vector<int>>> cand1, cand2, final1, final2;

    int query_time, yaki_time, expand_time, update_time;
    vector<vector<vector<bool>>> visited;
    vector<Block> erase_archive;
    queue<T> todo, visit_reset;

    vector<T> ok1_v, ok2_v;

    int little_move_id, tsurf1, tsurf2;
    stack<T> little_move_archive1, little_move_archive2;

    Solver(){
        id = 1;
    }

    void input(){
        cin >> d;
        sil1_f.assign(d,vector<bool>(d,false));
        sil1_r.assign(d,vector<bool>(d,false));
        sil2_f.assign(d,vector<bool>(d,false));
        sil2_r.assign(d,vector<bool>(d,false));
        exist_f1.assign(d,vector<short int>(d,0));
        exist_r1.assign(d,vector<short int>(d,0));
        exist_f2.assign(d,vector<short int>(d,0));
        exist_r2.assign(d,vector<short int>(d,0));
        visited.assign(d,vector<vector<bool>>(d,vector<bool>(d,false)));
        blo1_idx.assign(d,vector<vector<int>>(d,vector<int>(d,-1)));
        blo2_idx.assign(d,vector<vector<int>>(d,vector<int>(d,-1)));
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
            else ok1_v.emplace_back(T(z,y,x));
            if(!sil2_f[z][x] || !sil2_r[z][y])cand2[z][y][x] = -1;
            else ok2_v.emplace_back(T(z,y,x));
        }
    }

    void solve(){
        utility::mytm.CodeStart();

        // 初期解生成
        constructAnswer();
        last_score = calcScore();
      	final_score = last_score;
      	final1 = cand1;
        final2 = cand2;
        final_list = Block_list;

        start_time = utility::mytm.elapsed();
        query_time = 0, yaki_time = 0, expand_time = 0, update_time = 0;
        while(!erase_archive.empty())erase_archive.pop_back();

        bool flag = false;
        
        // 焼きなまし法
        while(utility::mytm.elapsed() <= TIME_LIMIT){
            
            flag = (rand_int()%2);

            if(flag){
                if(!littleMove()){
                    // 微移動
                    rep(i,Block_list.size()){
                        if(Block_list[i].id == little_move_id){
                            while(Block_list[i].size)Block_list[i].shaveBack(cand1,cand2);

                            Block_list[i].surf1 = tsurf1;
                            Block_list[i].surf2 = tsurf2;

                            while(!little_move_archive1.empty()){
                                auto &&[z1,y1,x1] = little_move_archive1.top();
                                auto &&[z2,y2,x2] = little_move_archive2.top();
                                Block_list[i].expand(z1,y1,x1,z2,y2,x2,cand1,cand2);

                                // bitでブロックの有無管理
                                exist_f1[z1][x1] |= (1 << y1);
                                exist_r1[z1][y1] |= (1 << x1);
                                exist_f2[z2][x2] |= (1 << y2);
                                exist_r2[z2][y2] |= (1 << x2);

                                little_move_archive1.pop();
                                little_move_archive2.pop();
                            }
                            break;
                        }
                    }
                    continue;
                }
            }
            else preErase(); // 部分破壊

            constructAnswer(); // 再構築
            cand_score = calcScore();

            temp = prob(last_score/100000,cand_score/100000,start_time);
            rnd_d = rand_double();
            cerr << cand_score << " " << last_score << " " << final_score << '\n';

            if(temp > rnd_d){
                last_score = cand_score;
                // if(temp < 1.0){
                //     cerr << cand_score << " " << final_score << '\n';
                //     yaki_time++;
                // }
                if(cand_score < final_score){
                    // 最終提出用 archive
                    final_score = cand_score;

                    // final_exist_f1 = exist_f1;
                    // final_exist_r1 = exist_r1;
                    // final_exist_f2 = exist_f2;
                    // final_exist_r2 = exist_r2;
                    // final_list = Block_list;

                    rep(i,ok1_v.size()){
                        auto &&[z,y,x] = ok1_v[i];
                        final1[z][y][x] = cand1[z][y][x];
                    }
                    rep(i,ok2_v.size()){
                        auto &&[z,y,x] = ok2_v[i];
                        final2[z][y][x] = cand2[z][y][x];
                    }
                    
                    update_time = 0;
                }
                else update_time++;
                while(!erase_archive.empty())erase_archive.pop_back();
                
            }
            else{
                // 差分更新
                while(expand_time--)eraseBlock(Block_list.size()-1,false);

                while(!erase_archive.empty()){
                    Block_list.emplace_back(move(erase_archive.back()));
                    Block &tmp = Block_list.back();
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
                    erase_archive.pop_back();
                }

                if(flag){
                    rep(i,Block_list.size()){
                        if(Block_list[i].id == little_move_id){
                            while(Block_list[i].size)Block_list[i].shaveBack(cand1,cand2);

                            Block_list[i].surf1 = tsurf1;
                            Block_list[i].surf2 = tsurf2;

                            while(!little_move_archive1.empty()){
                                auto &&[z1,y1,x1] = little_move_archive1.top();
                                auto &&[z2,y2,x2] = little_move_archive2.top();
                                Block_list[i].expand(z1,y1,x1,z2,y2,x2,cand1,cand2);

                                // bitでブロックの有無管理
                                exist_f1[z1][x1] |= (1 << y1);
                                exist_r1[z1][y1] |= (1 << x1);
                                exist_f2[z2][x2] |= (1 << y2);
                                exist_r2[z2][y2] |= (1 << x2);

                                little_move_archive1.pop();
                                little_move_archive2.pop();

                            }
                            break;
                        }
                    }
                }
                update_time++;
            }

            while(!little_move_archive1.empty())little_move_archive1.pop();
            while(!little_move_archive2.empty())little_move_archive2.pop();

            // finalに戻す
            // if(update_time >= 2000){
            //     cand1 = final1;
            //     cand2 = final2;
            //     Block_list = final_list;
            //     exist_f1 = final_exist_f1;
            //     exist_r1 = final_exist_r1;
            //     exist_f2 = final_exist_f2;
            //     exist_r2 = final_exist_r2;
            //     update_time = 0;
            // }

            expand_time = 0;
            query_time++;
        }
        cerr << "query_time : " << query_time << '\n';
        cerr << "final_score : " << final_score << '\n';
        // cerr << "yaki_time : " << yaki_time << '\n';
        return;
    }

    inline void constructAnswer(){
        start_id = id;
        while(true){
            while(!cand1_v.empty())cand1_v.pop_back();
            while(!cand2_v.empty())cand2_v.pop_back();
            rep(i,ok1_v.size()){
                auto &&[z,y,x] = ok1_v[i];
                if( !exist_f1[z][x] || !exist_r1[z][y] ){
                    cand1_v.emplace_back(T(z,y,x));
                }
            }
            rep(i,ok2_v.size()){
                auto &&[z,y,x] = ok2_v[i];
                if( !exist_f2[z][x] || !exist_r2[z][y] ){
                    cand2_v.emplace_back(T(z,y,x));
                }
            }

            auto &&[nz1,ny1,nx1] = (cand1_v.empty() ? T(-1,-1,-1) : cand1_v[rand_int()%cand1_v.size()]);
            auto &&[nz2,ny2,nx2] = (cand2_v.empty() ? T(-1,-1,-1) : cand2_v[rand_int()%cand2_v.size()]);

            // シルエット完成時
            if(nz1 == -1 && nz2 == -1)break;

            // 片方シルエット完成時
            if(nz1 == -1){
                while(!cv.empty())cv.pop_back();
                rep(i,ok1_v.size()){
                    auto &&[z,y,x] = ok1_v[i];
                    if(!cand1[z][y][x])cv.emplace_back(T(z,y,x));
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
                rep(i,ok2_v.size()){
                    auto &&[z,y,x] = ok2_v[i];
                    if(!cand2[z][y][x])cv.emplace_back(T(z,y,x));
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

            Block_list.emplace_back(Block(T(nz1,ny1,nx1),T(nz2,ny2,nx2),0,0,d,id));
            best_score = LLONG_MAX;
            cand1[nz1][ny1][nx1] = id;
            cand2[nz2][ny2][nx2] = id;
            exist_f1[nz1][nx1] |= (1 << ny1);
            exist_r1[nz1][ny1] |= (1 << nx1);
            exist_f2[nz2][nx2] |= (1 << ny2);
            exist_r2[nz2][ny2] |= (1 << nx2);
            blo1_idx[nz1][ny1][nx1] = 0;
            blo2_idx[nz2][ny2][nx2] = 0;
    
            // int cnt = 0;
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
                        // cnt++;
                    }
                    // 差分更新で元に戻す
                    while(Block_list.back().size > 1){
                        auto &&[z1,y1,x1] = Block_list.back().place1.back();
                        auto &&[z2,y2,x2] = Block_list.back().place2.back();
                        Block_list.back().shave(z1,y1,x1,z2,y2,x2,cand1,cand2);
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
        // Block DESTROY_NUM 個破壊
        time = rand_int()%DESTROY_NUM + 1 + min((int)(rand_int()%(1 + update_time/2000)),10);
        // time = rand_int()%DESTROY_NUM + 1 + (rand_int()%5 == 0 ? 1 : 0);
        time = min((int)Block_list.size(),time);
        rnd = rand_int()%Block_list.size();
        if(time >= 2){
            auto &&[nz,ny,nx] = Block_list[rnd].place1[rand_int()%Block_list[rnd].place1.size()];
            todo.push(T(nz,ny,nx));
            set<int> st;
            
            while(!todo.empty()){
                auto &&[z,y,x] = todo.front();
                if(visited[z][y][x]){
                    todo.pop();
                    continue;
                }
                visited[z][y][x] = true;
                visit_reset.push(T(z,y,x));
                if(cand1[z][y][x] > 0 && !st.count(cand1[z][y][x])){
                    st.insert(cand1[z][y][x]);
                    time--;
                    if(time == 0)break;
                }
                rep(i,DIR_NUM){
                    tz1 = z + dz[i];
                    ty1 = y + dy[i];
                    tx1 = x + dx[i];
                    if(outField(tz1,ty1,tx1))continue;
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
            eraseBlock(rnd,true);
            for(auto erase_id:st){
                rep(i,Block_list.size()){
                    if(erase_id == Block_list[i].id){
                        eraseBlock(i,true);
                        break;
                    }
                }
            }
        }
        else eraseBlock(rnd,true);
    }

    // 旧ver
    inline void preErase_pre(){
        if(rand_int()%2)time = 2;
        else if(rand_int()%2)time = 1;
        else time = 3;

        time = min((int)Block_list.size(),time);
        rnd = rand_int()%Block_list.size();

        auto &&[nz,ny,nx] = Block_list[rnd].place1[rand_int()%Block_list[rnd].place1.size()];
        eraseBlock(rnd,true);
        time--;

        while(time--){
            int min_dis = INT_MAX;
            int dis = INT_MAX;
            int now = 0;
            rep(i,Block_list.size()){
                for(auto &&[z,y,x]:Block_list[i].place1){
                    dis = min(dis,abs(nz-z)+abs(ny-y)+abs(nx-x));
                }
                if( dis < min_dis ){
                    min_dis = dis;
                    now = i;
                }
            }
            eraseBlock(now,true);
        }
    }

    inline bool littleMove(){

        int r1 = rand_int()%Block_list.size();
        little_move_id = Block_list[r1].id;
        tsurf1 = Block_list[r1].surf1;
        tsurf2 = Block_list[r1].surf2;

        while(Block_list[r1].size > 1){
            little_move_archive1.push(Block_list[r1].place1.back());
            little_move_archive2.push(Block_list[r1].place2.back());
            Block_list[r1].shaveBack(cand1,cand2);
        }
        little_move_archive1.push(Block_list[r1].place1.back());
        little_move_archive2.push(Block_list[r1].place2.back());
        vector<T> cp;

        if(rand_int()%2){
            auto &&[z,y,x] = Block_list[r1].place1[0];
            rep(d,DIR_NUM){
                if(!outField(z+dz[d],y+dy[d],x+dx[d]) && cand1[z+dz[d]][y+dy[d]][x+dx[d]] == 0){
                    cp.emplace_back(T(z+dz[d], y+dy[d], x+dx[d]));
                }
            }
            if(cp.empty())return false;

            int r2 = rand_int()%cp.size();
            auto &&[nz1,ny1,nx1] = Block_list[r1].place1[0];
            auto &&[nz2,ny2,nx2] = cp[r2];
            cand1[nz1][ny1][nx1] = 0;
            cand1[nz2][ny2][nx2] = Block_list[r1].id;
            exist_f1[nz1][nx1] ^= (1 << ny1);
            exist_r1[nz1][ny1] ^= (1 << nx1);
            exist_f1[nz2][nx2] |= (1 << ny2);
            exist_r1[nz2][ny2] |= (1 << nx2);

            Block_list[r1].place1[0] = cp[r2];
        }
        else{
            auto &&[z,y,x] = Block_list[r1].place2[0];
            rep(d,DIR_NUM){
                if(!outField(z+dz[d],y+dy[d],x+dx[d]) && cand2[z+dz[d]][y+dy[d]][x+dx[d]] == 0){
                    cp.emplace_back(T(z+dz[d], y+dy[d], x+dx[d]));
                }
            }
            if(cp.empty())return false;

            int r2 = rand_int()%cp.size();
            auto &&[nz1,ny1,nx1] = Block_list[r1].place2[0];
            auto &&[nz2,ny2,nx2] = cp[r2];
            cand2[nz1][ny1][nx1] = 0;
            cand2[nz2][ny2][nx2] = Block_list[r1].id;
            exist_f2[nz1][nx1] ^= (1 << ny1);
            exist_r2[nz1][ny1] ^= (1 << nx1);
            exist_f2[nz2][nx2] |= (1 << ny2);
            exist_r2[nz2][ny2] |= (1 << nx2);

            Block_list[r1].place2[0] = cp[r2];
        }

        best_score = LLONG_MAX;

        rep(d1,DIR_NUM){
            rep(d2,DIR_NUM){
                cand_score = LLONG_MAX-1;
                Block_list[r1].surf1 = d1;
                Block_list[r1].surf2 = d2;
                
                // 最大限に拡張
                query(r1);
                cand_score = calcScore();

                if(cand_score < best_score){
                    best_score = cand_score;
                    tmp = Block_list[r1];
                }
                // 差分更新で元に戻す
                while(Block_list[r1].size > 1){
                    auto &&[z1,y1,x1] = Block_list[r1].place1.back();
                    auto &&[z2,y2,x2] = Block_list[r1].place2.back();
                    Block_list[r1].shave(z1,y1,x1,z2,y2,x2,cand1,cand2);
                }
            }
        }
        
        // ブロック採用
        rep(i,tmp.size){
            auto &&[z1,y1,x1] = tmp.place1[i];
            auto &&[z2,y2,x2] = tmp.place2[i];
            cand1[z1][y1][x1] = tmp.id;
            cand2[z2][y2][x2] = tmp.id;
            exist_f1[z1][x1] |= (1 << y1);
            exist_r1[z1][y1] |= (1 << x1);
            exist_f2[z2][x2] |= (1 << y2);
            exist_r2[z2][y2] |= (1 << x2);
        }
        swap(Block_list[r1],tmp);
        return true;
    }

    // ブロック拡張query
    inline void query(const int &r1){
        // r1 : 拡張するブロック
        // r2 : 起点にする場所
        // r3 : 伸ばす方向
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
        rep(i,Block_list[idx].size){
            auto &&[z1,y1,x1] = Block_list[idx].place1[i];
            auto &&[z2,y2,x2] = Block_list[idx].place2[i];

            cand1[z1][y1][x1] = 0;
            cand2[z2][y2][x2] = 0;
            
            exist_f1[z1][x1] ^= (1 << y1);
            exist_r1[z1][y1] ^= (1 << x1);
            exist_f2[z2][x2] ^= (1 << y2);
            exist_r2[z2][y2] ^= (1 << x2);
        }
        if(flag)erase_archive.emplace_back(move(Block_list[idx]));
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

    void idCompression(){
        // id圧縮
        vector<int> id;
        rep(i,d)rep(j,d)rep(k,d)if(final1[i][j][k] > 0)id.emplace_back(final1[i][j][k]);
        rep(i,d)rep(j,d)rep(k,d)if(final2[i][j][k] > 0)id.emplace_back(final2[i][j][k]);
        sort(id.begin(),id.end());
        id.erase(unique(id.begin(),id.end()),id.end());
        rep(i,d)rep(j,d)rep(k,d){
            if(final1[i][j][k] > 0){
                final1[i][j][k] = lower_bound(id.begin(),id.end(),final1[i][j][k])-id.begin()+1;
            }
            if(final2[i][j][k] > 0){
                final2[i][j][k] = lower_bound(id.begin(),id.end(),final2[i][j][k])-id.begin()+1;
            }
        }
    }

    void output(){
        idCompression();
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