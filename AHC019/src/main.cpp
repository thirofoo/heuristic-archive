#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef long double ld;
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
double start_temp = 806.3434400282501,end_temp = 15.41342634559813;
inline double temp(int start) {
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

constexpr int NUM_DIRECTION = 6;
constexpr int NUM_ROTATION = 24;
constexpr array<int, NUM_DIRECTION* NUM_ROTATION> DX_RAW = { 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1 };
constexpr array<int, NUM_DIRECTION* NUM_ROTATION> DY_RAW = { 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0 };
constexpr array<int, NUM_DIRECTION* NUM_ROTATION> DZ_RAW = { 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0 };
const vector<int> dx = { 1,-1, 0, 0, 0, 0};
const vector<int> dy = { 0, 0, 1,-1, 0, 0};
const vector<int> dz = { 0, 0, 0, 0, 1,-1};
const vector<int> r_dx = {-1, 1, 0, 0, 0, 0};
const vector<int> r_dy = { 0, 0,-1, 1, 0, 0};
const vector<int> r_dz = { 0, 0, 0, 0,-1, 1};
constexpr int DESTROY_NUM = 2;
vector<vector<short int>> exist_f1, exist_r1, exist_f2, exist_r2, final_exist_f1, final_exist_r1, final_exist_f2, final_exist_r2;
vector<vector<vector<int>>> blo1_idx, blo2_idx;
vector<vector<vector<int>>> cand1, cand2, final1, final2;

struct Block{
    int size, d, id, surf;
    vector<T> place1,place2;

    Block() : size(0) {}
    explicit Block(const T &b1,const T &b2,const int &_surf,const int &_d,const int &_id){
        d = _d;
        id = _id;
        surf = _surf;
        size = 1;
        place1.emplace_back(b1);
        place2.emplace_back(b2);

        auto &&[nz1,ny1,nx1] = b1;
        auto &&[nz2,ny2,nx2] = b2;
        
        cand1[nz1][ny1][nx1] = this -> id;
        cand2[nz2][ny2][nx2] = this -> id;
        exist_f1[nz1][nx1] |= (1 << ny1);
        exist_r1[nz1][ny1] |= (1 << nx1);
        exist_f2[nz2][nx2] |= (1 << ny2);
        exist_r2[nz2][ny2] |= (1 << nx2);
        blo1_idx[nz1][ny1][nx1] = 0;
        blo2_idx[nz2][ny2][nx2] = 0;
    }

    inline void expand(const int &nz1,const int &ny1,const int &nx1,const int &nz2,const int &ny2,const int &nx2, const bool &f){
        place1.emplace_back(T(nz1,ny1,nx1));
        place2.emplace_back(T(nz2,ny2,nx2));
        cand1[nz1][ny1][nx1] = this -> id;
        cand2[nz2][ny2][nx2] = this -> id;
        blo1_idx[nz1][ny1][nx1] = size;
        blo2_idx[nz2][ny2][nx2] = size;
        if(f){
            exist_f1[nz1][nx1] |= (1 << ny1);
            exist_r1[nz1][ny1] |= (1 << nx1);
            exist_f2[nz2][nx2] |= (1 << ny2);
            exist_r2[nz2][ny2] |= (1 << nx2);
        }
        size++;
        return;
    }

    inline void shave(const int &nz1,const int &ny1,const int &nx1,const int &nz2,const int &ny2,const int &nx2, const bool &f){
        place1.erase(place1.begin()+blo1_idx[nz1][ny1][nx1]);
        place2.erase(place2.begin()+blo2_idx[nz2][ny2][nx2]);
        cand1[nz1][ny1][nx1] = 0;
        cand2[nz2][ny2][nx2] = 0;
        blo1_idx[nz1][ny1][nx1] = -1;
        blo2_idx[nz2][ny2][nx2] = -1;
        if(f){
            exist_f1[nz1][nx1] ^= (1 << ny1);
            exist_r1[nz1][ny1] ^= (1 << nx1);
            exist_f2[nz2][nx2] ^= (1 << ny2);
            exist_r2[nz2][ny2] ^= (1 << nx2);
        }
        size--;
        return;
    }

    inline void shaveBack(const bool &f){
        auto &&[nz1,ny1,nx1] = place1.back();
        auto &&[nz2,ny2,nx2] = place2.back();
        cand1[nz1][ny1][nx1] = 0;
        cand2[nz2][ny2][nx2] = 0;
        blo1_idx[nz1][ny1][nx1] = -1;
        blo2_idx[nz2][ny2][nx2] = -1;
        if(f){
            exist_f1[nz1][nx1] ^= (1 << ny1);
            exist_r1[nz1][ny1] ^= (1 << nx1);
            exist_f2[nz2][nx2] ^= (1 << ny2);
            exist_r2[nz2][ny2] ^= (1 << nx2);
        }
        place1.pop_back();
        place2.pop_back();
        size--;
        return;
    }
};



struct Solver{
    double temp, rnd_d, score;
    int d, id, start_time, start_id, tz1, ty1, tx1, tz2, ty2, tx2, time, rnd, r;

    int query_time, yaki_time, expand_time, update_time, interval, roop_time;
    ll best_size, cand_score, last_score, now_score, final_score;
    vector<Block> Block_list, final_list;

    Block tmp;
    vector<vector<bool>> sil1_f, sil1_r, sil2_f, sil2_r;
    vector<T> ok1_v, ok2_v;
    deque<int> cand1_v, cand2_v, cv;
    int size1, size2, tsurf;

    queue<T> todo, visit_reset;
    vector<Block> erase_archive;
    vector<vector<vector<bool>>> visited;
    vector<T> cp;
    vector<int> move_dir;

    vector<int> v_count;
    int query, dir_i, move_idx;
    ll best_score;

    Solver() : id(1){}

    void input(const bool &f, const string &path){
        if(!f){
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
            cand1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
            cand2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
            rep(i,4){
                rep(z,d){
                    string s; cin >> s;
                    if     (i == 0)rep(x,d)sil1_f[z][x] = (s[x] == '1');
                    else if(i == 1)rep(y,d)sil1_r[z][y] = (s[y] == '1');
                    else if(i == 2)rep(x,d)sil2_f[z][x] = (s[x] == '1');
                    else           rep(y,d)sil2_r[z][y] = (s[y] == '1');
                }
            }
        }
        else{
            ifstream ifs(path);
	        if (ifs.fail()) {
	           cerr << "Cannot open file\n";
	           exit(0);
	        }
	        string s;
	        stringstream ss(s);

            getline(ifs,s);
            d = stoi(s);
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
            cand1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
            cand2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
            rep(i,4){
                rep(z,d){
                    getline(ifs,s);
                    if     (i == 0)rep(x,d)sil1_f[z][x] = (s[x] == '1');
                    else if(i == 1)rep(y,d)sil1_r[z][y] = (s[y] == '1');
                    else if(i == 2)rep(x,d)sil2_f[z][x] = (s[x] == '1');
                    else           rep(y,d)sil2_r[z][y] = (s[y] == '1');
                }
            }
        }

        rep(z,d)rep(y,d)rep(x,d){
            if(!sil1_f[z][x] || !sil1_r[z][y])cand1[z][y][x] = -1;
            else ok1_v.emplace_back(T(z,y,x));
            if(!sil2_f[z][x] || !sil2_r[z][y])cand2[z][y][x] = -1;
            else ok2_v.emplace_back(T(z,y,x));
        }
        roop_time = 0;
    }

    void solve(){
        utility::mytm.CodeStart();

        // 初期解生成
        constructAnswer(false);
        if(roop_time >= 10000)return emergency();

        sort(Block_list.begin(),Block_list.end(),[](const Block b1,const Block b2){
            return b1.size < b2.size;
        });
        rep(i,Block_list.size())fullExpand(i,true);

        last_score = calcScore();
      	final_score = last_score;
      	final1 = cand1;
        final2 = cand2;
        final_list = Block_list;

        start_time = utility::mytm.elapsed();
        query_time = 0, expand_time = 0, update_time = 0;
        interval = 100;
        query = 1;

        // 焼きなまし法
        while(utility::mytm.elapsed() <= TIME_LIMIT){
            while(interval--){
                
                // 部分破壊 & 再構築
                partErase();
                constructAnswer(true);

                cand_score = calcScore();
                temp = prob(last_score/100000,cand_score/100000,start_time);

                if(temp > rand_double()){

                    if(cand_score < final_score){
                        // 最終提出 archive
                        sort(Block_list.begin(),Block_list.end(),[](const Block b1,const Block b2){
                            return b1.size < b2.size;
                        });
                        rep(i,Block_list.size())fullExpand(i,true);
                        final_score = calcScore();

                        rep(i,ok1_v.size()){
                            auto &&[z,y,x] = ok1_v[i];
                            final1[z][y][x] = cand1[z][y][x];
                        }
                        rep(i,ok2_v.size()){
                            auto &&[z,y,x] = ok2_v[i];
                            final2[z][y][x] = cand2[z][y][x];
                        }
                    }
                    if(cand_score < last_score)update_time = 0;
                    else update_time++;
                    last_score = cand_score;
                    while(!erase_archive.empty())erase_archive.pop_back();
                }
                else{
                    rollback();
                    update_time++;
                }

                if(update_time >= 1000){
                    // シルエット維持しつつ移動
                    rep(i,10){
                        move_idx = rand_int()%Block_list.size();
                        littleMove(move_idx);
                    }
                    last_score = calcScore();
                    update_time = 0;
                }

                expand_time = 0;
                roop_time = -1e7;
                // query_time++;
            }
            interval = 100;
        }
        // cerr << "query_time : " << query_time << '\n';
        cerr << "final_score : " << final_score << '\n';
        return;
    }


    inline void constructAnswer(const bool &archive){
        start_id = id;
        candSearch();

        while(true){
            // 探索候補の頂点
            size1 = cand1_v.size();
            size2 = cand2_v.size();
            rep(i,size1){
                auto &&[z,y,x] = ok1_v[cand1_v.back()];
                if( !exist_f1[z][x] || !exist_r1[z][y] )cand1_v.push_front(cand1_v.back());
                cand1_v.pop_back();
            }
            rep(i,size2){
                auto &&[z,y,x] = ok2_v[cand2_v.back()];
                if( !exist_f2[z][x] || !exist_r2[z][y] )cand2_v.push_front(cand2_v.back());
                cand2_v.pop_back();
            }

            // シルエット完成時
            if(cand1_v.empty() && cand2_v.empty())break;

            // 片方シルエット完成時
            if(cand1_v.empty()){
                rep(i,ok1_v.size()){
                    auto &&[z,y,x] = ok1_v[i];
                    if(!cand1[z][y][x])cand1_v.emplace_back(i);
                }
                if(cand1_v.empty()){
                    rnd = rand_int()%Block_list.size();
                    if(start_id <= Block_list[rnd].id){
                        eraseBlock(rnd,false);
                        expand_time--;
                    }
                    else eraseBlock(rnd,archive);
                    candSearch();
                    continue;
                }
            }
            if(cand2_v.empty()){
                rep(i,ok2_v.size()){
                    auto &&[z,y,x] = ok2_v[i];
                    if(!cand2[z][y][x])cand2_v.emplace_back(i);
                }
                if(cand2_v.empty()){
                    rnd = rand_int()%Block_list.size();
                    if(start_id <= Block_list[rnd].id){
                        eraseBlock(rnd,false);
                        expand_time--;
                    }
                    else eraseBlock(rnd,archive);
                    candSearch();
                    continue;
                }
            }

            auto &&[nz1,ny1,nx1] = ok1_v[cand1_v[rand_int()%cand1_v.size()]];
            auto &&[nz2,ny2,nx2] = ok2_v[cand2_v[rand_int()%cand2_v.size()]];
            Block_list.emplace_back(Block(T(nz1,ny1,nx1),T(nz2,ny2,nx2),0,d,id));
            best_size = 0;
    
            // 追加Blockの方向全探索
            rep(d,NUM_ROTATION){
                Block_list.back().surf = d;
                
                // 最大限に拡張
                fullExpand(Block_list.size()-1,false);
                
                if(best_size < Block_list.back().size){
                    best_size = Block_list.back().size;
                    tsurf = d;
                }
                // 差分更新で元に戻す
                while(Block_list.back().size > 1)Block_list.back().shaveBack(false);
            }
            
            // ブロック採用
            Block_list.back().surf = tsurf;
            fullExpand(Block_list.size()-1,true);
            expand_time++;
            roop_time++;
            id++;
            if(roop_time >= 10000)break;
        }
    }


    inline void partErase(){
        // 部分破壊
        time = (rand_int()%3 <= 1) + 1 + (update_time/500);
        time = min((int)Block_list.size(),time);
        rnd = rand_int()%Block_list.size();

        if(time == 1)eraseBlock(rnd,true);
        else{
            r = rand_int()%2;

            // bfsで近くのBlockを見つける
            auto &&[nz,ny,nx] = (r ? Block_list[rnd].place1[rand_int()%Block_list[rnd].place1.size()] : Block_list[rnd].place2[rand_int()%Block_list[rnd].place2.size()]);
            todo.push(T(nz,ny,nx));
            v_count.resize(0);

            while(!todo.empty()){
                auto &&[z,y,x] = todo.front();
                if(visited[z][y][x]){
                    todo.pop();
                    continue;
                }
                visited[z][y][x] = true;
                visit_reset.push(T(z,y,x));
                if((r ? cand1[z][y][x] : cand2[z][y][x]) > 0 && find(v_count.begin(),v_count.end(),(r ? cand1[z][y][x] : cand2[z][y][x])) == v_count.end()){
                    v_count.emplace_back((r ? cand1[z][y][x] : cand2[z][y][x]));
                    time--;
                    if(time == 0)break;
                }
                rep(i,NUM_DIRECTION){
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
            for(auto &&erase_id:v_count){
                rep(i,Block_list.size()){
                    if(erase_id == Block_list[i].id){
                        eraseBlock(i,true);
                        break;
                    }
                }
            }
        }
    }


    inline void littleMove(const int &r1){
        // 移動 (シルエット2に限定)
        cp.resize(0);
        move_dir.resize(0);
        best_score = LLONG_MAX;
        dir_i = -1;

        // 何故かここを参照にすると、510行目付近のfullExpand後に Segmentation Fault
        // fullExpand時に産所言うが外れるっぽい ?
        auto [z,y,x] = Block_list[r1].place2[0];

        rep(dir,NUM_DIRECTION){
            if(!outField(z+dz[dir],y+dy[dir],x+dx[dir]) && cand2[z+dz[dir]][y+dy[dir]][x+dx[dir]] == 0){
                cp.emplace_back(T(z+dz[dir], y+dy[dir], x+dx[dir]));
                move_dir.emplace_back(dir);
            }
        }
        if(cp.empty())return;
        
        while(Block_list[r1].size > 1)Block_list[r1].shaveBack(true);

        cand2[z][y][x] = 0;
        exist_f2[z][x] ^= (1 << y);
        exist_r2[z][y] ^= (1 << x);

        rep(i,cp.size()){
            auto &&[nz2,ny2,nx2] = cp[i];
            cand2[nz2][ny2][nx2] = Block_list[r1].id;
            exist_f2[nz2][nx2] |= (1 << ny2);
            exist_r2[nz2][ny2] |= (1 << nx2);

            Block_list[r1].place2[0] = cp[i];
            fullExpand(r1,true);

            // シルエット完成 & スコアが良い時
            candSearch();
            if(cand1_v.empty() && cand2_v.empty()){
                cand_score = calcScore();
                if(cand_score < best_score){
                    best_score = cand_score;
                    dir_i = i;
                }
            }

            while(Block_list[r1].size > 1)Block_list[r1].shaveBack(true);
            cand2[nz2][ny2][nx2] = 0;
            exist_f2[nz2][nx2] ^= (1 << ny2);
            exist_r2[nz2][ny2] ^= (1 << nx2);
        }

        if(dir_i == -1){
            // 元通りに戻す
            cand2[z][y][x] = Block_list[r1].id;
            exist_f2[z][x] |= (1 << y);
            exist_r2[z][y] |= (1 << x);
            Block_list[r1].place2[0] = T(z,y,x);
        }
        else{
            // ずらして拡張
            auto &&[nz2,ny2,nx2] = cp[dir_i];
            cand2[nz2][ny2][nx2] = Block_list[r1].id;
            exist_f2[nz2][nx2] |= (1 << ny2);
            exist_r2[nz2][ny2] |= (1 << nx2);
            Block_list[r1].place2[0] = T(nz2,ny2,nx2);
        }

        fullExpand(r1,true);

        return;
    }


    inline void fullExpand(const int &r1, const bool &exist_update){
        // r1 : 拡張するブロック
        // r2 : 起点にする場所
        // r3 : 伸ばす方向
        rep(r2,Block_list[r1].size){
            auto &&[z1,y1,x1] = Block_list[r1].place1[r2];
            auto &&[z2,y2,x2] = Block_list[r1].place2[r2];
            rep(r3,NUM_DIRECTION){
                tz1 = z1 + DZ_RAW[r3];
                ty1 = y1 + DY_RAW[r3];
                tx1 = x1 + DX_RAW[r3];
                if(outField(tz1,ty1,tx1) || cand1[tz1][ty1][tx1])continue;

                tz2 = z2 + DZ_RAW[Block_list[r1].surf * NUM_DIRECTION + r3];
                ty2 = y2 + DY_RAW[Block_list[r1].surf * NUM_DIRECTION + r3];
                tx2 = x2 + DX_RAW[Block_list[r1].surf * NUM_DIRECTION + r3];
                if(outField(tz2,ty2,tx2) || cand2[tz2][ty2][tx2])continue;

                Block_list[r1].expand(tz1,ty1,tx1,tz2,ty2,tx2,exist_update);
            }
        }
    }


    inline void eraseBlock(const int &idx, const bool &archive){
        // Block完全消去
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
        if(archive)erase_archive.emplace_back(move(Block_list[idx]));
        Block_list.erase(Block_list.begin()+idx);
    }


    inline void rollback(){
        if(query){
            while(expand_time--)eraseBlock(Block_list.size()-1,false);
            while(!erase_archive.empty()){
                Block_list.emplace_back(move(erase_archive.back()));
                Block &tmp = Block_list.back();
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
                erase_archive.pop_back();
            }
        }
        else{
            if(dir_i == -1)return;
            while(Block_list[move_idx].size > 1)Block_list[move_idx].shaveBack(true);
            auto &&[z,y,x] = Block_list[move_idx].place2[0];
            tz1 = z + r_dz[move_dir[dir_i]];
            ty1 = y + r_dy[move_dir[dir_i]];
            tx1 = x + r_dx[move_dir[dir_i]];

            exist_f2[z][x] ^= (1 << y);
            exist_r2[z][y] ^= (1 << x);
            exist_f2[tz1][tx1] |= (1 << ty1);
            exist_r2[tz1][ty1] |= (1 << tx1);
            cand2[z][y][x] = 0;
            cand2[tz1][ty1][tx1] = Block_list[move_idx].id;

            Block_list[move_idx].place2[0] = T(tz1, ty1, tx1);
            fullExpand(move_idx,true);
        }
    }


    inline void candSearch(){
        cand1_v.resize(0);
        cand2_v.resize(0);
        rep(i,ok1_v.size()){
            auto &&[z,y,x] = ok1_v[i];
            if( !exist_f1[z][x] || !exist_r1[z][y] ){
                cand1_v.emplace_back(i);
            }
        }
        rep(i,ok2_v.size()){
            auto &&[z,y,x] = ok2_v[i];
            if( !exist_f2[z][x] || !exist_r2[z][y] ){
                cand2_v.emplace_back(i);
            }
        }
    }


    inline void emergency(){
        //~~~ 山登りが回らない激辛ケース用貪欲 ~~~//
        int now1 = 1,now2 = 1;
        final1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        final2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        rep(z,d){
            vector<bool> r(d,false),c(d,false);
            vector<int> ri,ci;
            rep(x,d)if(sil1_f[z][x])ci.emplace_back(x);
            rep(y,d)if(sil1_r[z][y])ri.emplace_back(y);
            rep(i,max(ri.size(),ci.size())){
                final1[z][ri[i%ri.size()]][ci[i%ci.size()]] = now1;
                now1++;
            }
        }
        rep(z,d){
            vector<bool> r(d,false),c(d,false);
            vector<int> ri,ci;
            rep(x,d)if(sil2_f[z][x])ci.emplace_back(x);
            rep(y,d)if(sil2_r[z][y])ri.emplace_back(y);
            rep(i,max(ri.size(),ci.size())){
                final2[z][ri[i%ri.size()]][ci[i%ci.size()]] = now2;
                now2++;
            }
        }
        int now = max(now1,now2);
 
        // 平面ごとのスライスで斜め連結を網羅する
        priority_queue<T2> pq1,pq2;
        rep(i,d){
            for(int j=d-1;j>=0;j--){
                int sx = j,sy = 0,size = 0;
                while(!outField(sx+size,sy+size)){
                    if(final1[i][sx+size][sy+size])size++;
                    else {
                        if(size)pq1.push(T2(size,i,sx,sy));
                        sx += size+1;
                        sy += size+1;
                        size = 0;
                    }
                }
                if(size)pq1.push(T2(size,i,sx,sy));
                size = 0;
            }
            for(int j=1;j<d;j++){
                int sx = 0,sy = j,size = 0;
                while(!outField(sx+size,sy+size)){
                    if(final1[i][sx+size][sy+size])size++;
                    else {
                        if(size)pq1.push(T2(size,i,sx,sy));
                        sx += size+1;
                        sy += size+1;
                        size = 0;
                    }
                }
                if(size)pq1.push(T2(size,i,sx,sy));
                size = 0;
            }
        }
        rep(i,d){
            for(int j=d-1;j>=0;j--){
                int sx = j,sy = 0,size = 0;
                while(!outField(sx+size,sy+size)){
                    if(final2[i][sx+size][sy+size])size++;
                    else {
                        if(size)pq2.push(T2(size,i,sx,sy));
                        sx += size+1;
                        sy += size+1;
                        size = 0;
                    }
                }
                if(size)pq2.push(T2(size,i,sx,sy));
                size = 0;
            }
            for(int j=1;j<d;j++){
                int sx = 0,sy = j,size = 0;
                while(!outField(sx+size,sy+size)){
                    if(final2[i][sx+size][sy+size])size++;
                    else {
                        if(size)pq2.push(T2(size,i,sx,sy));
                        sx += size+1;
                        sy += size+1;
                        size = 0;
                    }
                }
                if(size)pq2.push(T2(size,i,sx,sy));
                size = 0;
            }
        }
 
        while(!pq1.empty() && !pq2.empty()){
            auto [size1,z1,y1,x1] = pq1.top(); pq1.pop();
            auto [size2,z2,y2,x2] = pq2.top(); pq2.pop();
            int size = min(size1,size2);
            rep(i,size)final1[z1][y1+i][x1+i] = now;
            rep(i,size)final2[z2][y2+i][x2+i] = now;
            rep(i,size-1)final1[z1][y1+i][x1+i +1] = now;
            rep(i,size-1)final2[z2][y2+i][x2+i +1] = now;
            if(size-size1)pq1.push(T2(size-size1,z1,y1+size,x1+size));
            if(size-size2)pq2.push(T2(size-size2,z2,y2+size,x2+size));
            now++;
        }
    }


    inline ll calcScore(){
        score = 0;
        for(auto &&b:Block_list)if(b.size)score += 1/(double)b.size;
        return round(score*1000000000);
    }


    inline bool outField(const int &x,const int &y,const int &z){
        return (x < 0 || x >= d || y < 0 || y >= d || z < 0 || z >= d);
    }
    inline bool outField(const int &x,const int &y){
        return (x < 0 || x >= d || y < 0 || y >= d);
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


    void output(const bool &f){
        if(!f){
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
        else{
            cout << final_score << endl;
        }
    }
};


int main(int argc, char* argv[]){
    cin.tie(0);
    ios_base::sync_with_stdio(false);
    
    Solver solver;

    if(argc >= 4){
        // optuna用
        cerr << argv[1] << " " << argv[2] << " " << argv[3] << endl;
        start_temp = stod(argv[1]);
        end_temp = stod(argv[2]);

        solver.input(true,argv[3]);
        solver.solve();
        solver.output(true);
    }
    else{
        // 提出用
        solver.input(false,"");
        solver.solve();
        solver.output(false);
    }
    
    return 0;
}