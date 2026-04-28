#include <bits/stdc++.h>
using namespace std;
using P = pair<int,int>;
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
#define TIME_LIMIT 3700
inline double temperture(double start) {
    // utility::myt.elapsed() が時間超過して呼び出されると、
    // 2項目が負に振り切って 確率が常に1以上になる可能性がある為、注意
    double start_temp = 1000, end_temp = 50;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    return exp((double)(now - best) / temperture(start));
}

//-----------------以下から実装部分-----------------//

int DIR_NUM = 21;
vector<int> dx = { 0,-1, 1, 0, 0,-1, 1,-1, 1,     2,-2, 0, 0,    -2, 2,-2, 2, 1,-1,-1, 1,   2,-2,-2, 2};
vector<int> dy = { 0, 0, 0,-1, 1,-1, 1, 1,-1,     0, 0, 2,-2,    -1, 1, 1,-1,-2, 2,-2, 2,   2, 2,-2,-2};
constexpr int SURROUND_DIR_NUM = 9;

// side-dxdy ( 左上右下の順番 )
constexpr int SIDE_DIR_NUM = 4;
static vector<int> sdx = { 0,-1, 0, 1};
static vector<int> sdy = {-1, 0, 1, 0};

constexpr int MAX_MESURE_TIME = 9990;

static vector<int> set_cost = { 0,  4, 15, /*Group1*/ 20, 30, 40, 60, 50, 70, 90, 100, 100, 120, 140, 160, 120, 140, 160, 180, 200, 180, 200, 220, 240, 260, 280, 320, 360, 400, 460, 500, /*Group2*/ };
static vector<int> judge    = { 0, -1, -1, /*Group1*/ -5, -5,  0,  0,  0,  0, 20,  30,   0,   0,   0,   0,  90,   0,   0,   0,   0, 200, 200, 220, 220, 240, 220, 180, 140, 100,  40,   0, /*Group2*/ };
static vector<int> j_time   = { 0, -1, -1, /*Group1*/  1,  1,  1,  1,  2,  2,  2,   2,   3,   3,   3,   3,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5, /*Group2*/ };

// 量子化ビット系定数
static vector<long long> quantization = { 0,  100,   30, /*Group1*/    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2 /*Group2*/ };
static vector<long long> s_to_dir     = { 0,    3,    5, /*Group1*/    9,   21,   21,   21,   21,   21,   21,   21,   21,   21,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25 /*Group2*/ };
static vector<int>       mes_time     = { 0,    1,    1, /*Group1*/    2,    2,    2,    3,    4,    5,    7,   10,   10,   12,   12,   16,   18,   21,   25,   25,   27,   29,   33,   35,   37,   39,  100,  100,  100,  100,  100,  100 /*Group2*/ };
static vector<int>       width        = { 0,  900,  900, /*Group1*/ 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000 /*Group2*/ };

int l, n, s;

struct Solver_small{
    int sq_s, nx, ny, measure_time, start_time, ng_num;
    long long measure_cost, set_num, set_exp2;
    map<long long, int> situ_cnt;
    vector<P> warm;
    vector<int> warm_idx;
    vector<long long> ng_to_score;
    vector<__int128_t> situation, rui;
    vector<vector<int>> temp;
    vector<vector<bool>> used;
    vector<vector<vector<P>>> place_warm_dir;
    vector<vector<vector<int>>> side_x, side_y;

    Solver_small(){
        input();

        sq_s = (int)sqrt(s);
        DIR_NUM = s_to_dir[sq_s];

        // 移動先頂点を予め列挙
        side_x.assign(l,vector<vector<int>>(l));
        side_y.assign(l,vector<vector<int>>(l));
        rep(i,l) {
            rep(j,l) {
                rep(d,SIDE_DIR_NUM) {
                    nx = (i+sdx[d] + l)%l, ny = (j+sdy[d] + l)%l;
                    side_x[i][j].emplace_back(nx);
                    side_y[i][j].emplace_back(ny);
                }
            }
        }

        measure_time = mes_time[sq_s];
        measure_time = min(measure_time, 10000/(n*(int)(log(n)+2)));

        measure_cost = 0;
        rep(d,DIR_NUM) measure_cost += 10 + abs(dx[d]) + abs(dy[d]);
        measure_cost *= 100 * n * measure_time;

        // used & place_warm_dir 初期化
        used.assign(l,vector<bool>(l,false));
        place_warm_dir.assign(l,vector<vector<P>>(l));
        rep(i,n) {
            auto &&[x,y] = warm[i];
            rep(d,DIR_NUM) {
                nx = ((x+dx[d])%l + l)%l, ny = ((y+dy[d])%l + l)%l;
                used[nx][ny] = true;
                place_warm_dir[nx][ny].emplace_back(pair(i,d));
            }
        }

        // ng_to_score 初期化
        ng_to_score.assign(n+1,0);
        ng_to_score[0] = 1e14;
        rep(i,n) ng_to_score[i+1] = ng_to_score[i]*0.8;

        warm_idx.assign(n,0);
        set_exp2 = (width[sq_s]/quantization[sq_s]) * (width[sq_s]/quantization[sq_s]);

        rui.assign(DIR_NUM+1,1);
        rep(d,DIR_NUM) rui[d+1] = rui[d] * quantization[sq_s];
    }

    void input() {
        // cin >> l >> n >> s;
        rep(i,n) {
            int x,y; cin >> x >> y;
            warm.emplace_back(pair(x,y));
        }
    }

    void arrange() {
        situation.assign(n,0);
        temp.assign(l,vector<int>(l,0));
        rep(i,l)rep(j,l) temp[i][j] = rand_int() % quantization[sq_s];

        // set_num 初期化
        set_num = 0;
        rep(i,l) {
            rep(j,l) {
                nx = (i+1 + l)%l, ny = j;
                set_num += ( temp[i][j] - temp[nx][ny] ) * ( temp[i][j] - temp[nx][ny] );
                nx = i, ny = (j+1 + l)%l;
                set_num += ( temp[i][j] - temp[nx][ny] ) * ( temp[i][j] - temp[nx][ny] );
            }
        }
        // situ_cnt & ng_num 初期化
        ng_num = n;
        stack<int> st;
        rep(i,n) {
            auto &&[x,y] = warm[i];
            rep(d,DIR_NUM) {
                nx = ((x+dx[d])%l + l)%l, ny = ((y+dy[d])%l + l)%l;
                st.push(temp[nx][ny]);
            }
            while( !st.empty() ) {
                situation[i] *= quantization[sq_s];
                situation[i] += st.top();
                st.pop();
            }
            if( situ_cnt[situation[i]] == 0 ) ng_num--;
            situ_cnt[situation[i]]++;
        }

        // 焼きなまし法
        long long best_score = calcScore(), next_score;
        int set_num_stock = set_num, ng_num_stock = ng_num;
        int turn = 0, rx, ry, d, update = 0, next_temp, pre_temp;

        while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            // 近傍：1箇所 bit 変更
            rx = rand_int()%l, ry = rand_int()%l;
            next_temp = rand_int()%quantization[sq_s];
            while( next_temp == temp[rx][ry] ) next_temp = rand_int()%quantization[sq_s];

            pre_temp = temp[rx][ry];
            temp[rx][ry] = next_temp;

            for(d = 0; d < SIDE_DIR_NUM; d++) {
                set_num -= ( pre_temp - temp[side_x[rx][ry][d]][side_y[rx][ry][d]] ) * (pre_temp - temp[side_x[rx][ry][d]][side_y[rx][ry][d]]);
                set_num += ( next_temp - temp[side_x[rx][ry][d]][side_y[rx][ry][d]] ) * (next_temp - temp[side_x[rx][ry][d]][side_y[rx][ry][d]]);
            }

            for(auto &&[idx,dir]:place_warm_dir[rx][ry]) {
                // situation も変更
                situ_cnt[situation[idx]]--;
                ng_num += ( !situ_cnt[situation[idx]] );

                situation[idx] -= pre_temp * rui[dir];
                situation[idx] += next_temp * rui[dir];

                ng_num -= ( !situ_cnt[situation[idx]] );
                situ_cnt[situation[idx]]++;
            }


            next_score = calcScore();
            if( prob(best_score,next_score,start_time) > rand_double() ) {
                // cerr << prob(best_score,next_score,start_time) << endl;
                // cerr << best_score << " " << next_score << endl;
                best_score = next_score;
                set_num_stock = set_num;
                ng_num_stock = ng_num;
            }
            else {
                for(auto &&[idx,dir]:place_warm_dir[rx][ry]) {
                    situ_cnt[situation[idx]]--;
                    situation[idx] -= next_temp * rui[dir];
                    situation[idx] += pre_temp * rui[dir];
                    situ_cnt[situation[idx]]++;
                }
                temp[rx][ry] = pre_temp;
                set_num = set_num_stock;
                ng_num = ng_num_stock;
            }
            // turn++;
        }
        // cerr << turn << endl;
        // cerr << "NG : " << ng_num << endl << flush;
        // cerr << measure_cost << " " << set_num*set_exp2 << endl;
        
        // 各 bit が何の数値に定まるかを決める
        rep(i,l) {
            rep(j,l) {
                temp[i][j] *= (width[sq_s]/quantization[sq_s]);
                temp[i][j] += (width[sq_s]/quantization[sq_s]) / 2;
                temp[i][j] += 500 - width[sq_s] / 2;
                temp[i][j] = min(1000,max(0,temp[i][j]));
            }
        }

        // 無関係な場所を滑らかにする
        rep(_,100)rep(i,l)rep(j,l) {
            if( !used[i][j] ) {
                temp[i][j] = 0;
                rep(d,SIDE_DIR_NUM) {
                    nx = (i+sdx[d] + l)%l, ny = (j+sdy[d] + l)%l;
                    temp[i][j] += temp[nx][ny];
                }
                temp[i][j] /= SIDE_DIR_NUM;
            }
        }

        // 盤面出力
        rep(i,l) {
            rep(j,l) cout << temp[i][j] << " ";
            cout << endl << flush;
        }
    }

    // k進数を出力する関数 ( debug用 )
    inline void out_quantify(int num) {
        stack<int> st;
        while( num ) {
            st.push(num%quantization[sq_s]);
            num /= quantization[sq_s];
        }
        while( st.size() != DIR_NUM ) st.push(0);
        while( !st.empty() ) {
            cerr << st.top();
            st.pop();
        }
        cerr << endl << flush;
    }

    inline long long calcScore() {
        // 一意な bit の数を調べる ⇒ 一意なら 100% 識別可能として正答率を求める
        return ng_to_score[ng_num] / (100000 + measure_cost + set_num*set_exp2 );
    }
    
    void measure_predict() {
        vector<bool> selected(n,false);
        priority_queue<P,vector<P>,greater<P>> todo;
        int now_measure_time = 0;
        rep(i,n) {
            // cerr << i+1 << "th Meaure Time : " << measure_time << endl << flush;
            // cerr << "Now Meaured Time : " << now_measure_time << endl << endl << flush;

            __int128_t warm_bit = 0;
            vector<int> cand;
            rep(j,n) {
                if( selected[j] && s <= 700 ) continue;
                cand.emplace_back(j);
            }

            // 最も選択肢を減らせる方向を採用
            int used_cnt = 1;
            vector<bool> used_dir(DIR_NUM,false);
            while( true ) {

                rep(d,DIR_NUM) {
                    if( used_dir[d] ) continue;
                    vector<int> num(quantization[sq_s],0);
                    for(auto idx:cand) {
                        num[(situation[idx]/rui[d])%quantization[sq_s]]++;
                    }
                    int max_num = -1;
                    rep(j,quantization[sq_s]) max_num = max(max_num,num[j]);
                    todo.push(pair(max_num,d));
                }

                if( todo.empty() ) break;
                auto [_,d] = todo.top(); todo.pop();
                while( !todo.empty()) todo.pop();
                used_dir[d] = true;

                int ave = 0;
                rep(_,measure_time) {
                    // 計測 phase
                    cout << i << " " << dx[d]%l << " " << dy[d]%l << endl << flush;
                    int tmp; cin >> tmp;
                    ave += tmp;
                    now_measure_time++;
                    if( now_measure_time == MAX_MESURE_TIME ) break;
                }
                ave /= measure_time;
                ave -= 500 - width[sq_s] / 2;
                ave = min(1000,max(0,ave));

                warm_bit += (ave/(width[sq_s]/quantization[sq_s])) * rui[d];
                
                vector<int> next_cand;
                for(auto idx:cand) {
                    int cnt = 0;
                    rep(td,DIR_NUM) {
                        if( used_dir[td] ) {
                            cnt += ( (situation[idx]/rui[td])%quantization[sq_s] == (warm_bit/rui[td])%quantization[sq_s] );
                        }
                    }
                    if( cnt == used_cnt ) next_cand.emplace_back(idx);
                }
                used_cnt++;

                if( next_cand.empty() ) next_cand.emplace_back(cand[0]);
                swap(cand,next_cand);

                // 操作回数上限に来たら終了
                if( now_measure_time == MAX_MESURE_TIME ) break;

                // 一意に定まったらその時点で終了
                // ※ DIR_NUM に関わらず O(logN) で Measure が終了するはず？
                if( cand.size() == 1 ) break;
            }

            warm_idx[i] = cand[0];
            selected[cand[0]] = true;
            if( now_measure_time == MAX_MESURE_TIME ) break;

            // 余った操作回数を後に回す
            if( i != n-1 ) {
                measure_time = min(measure_time, (MAX_MESURE_TIME-now_measure_time-100) / ((n-(i+1))*(int)(log(n)+2)) );
                measure_time = max(measure_time, 1);
            }
        }
    }

    void output() {
        cout << -1 << " " << -1 << " " << -1 << endl << flush;
        rep(i,n) cout << warm_idx[i] << endl << flush;
    }

    void solve() {
        utility::mytm.CodeStart();
        start_time = utility::mytm.elapsed();
        
        // 配置 part
        arrange();

        // 計測・予測 part
        measure_predict();

        // 出力
        output();
    }
};

struct Solver{
    int nx, ny, measure_time, start_time, ng_num, sq_s;
    long long measure_cost, set_num, set_exp2;
    vector<P> warm;
    vector<vector<int>> temp;
    vector<vector<bool>> used;
    vector<long long> ng_to_score;
    vector<int> warm_idx, situ_cnt, situation;
    vector<vector<vector<P>>> place_warm_dir;
    vector<vector<vector<int>>> side_x, side_y;

    Solver(){
        input();

        sq_s = (int)sqrt(s);

        rep(d,DIR_NUM) {
            if( s > 4 ) {
                if( dx[d] ) dx[d] *= l/8;
                if( dy[d] ) dy[d] *= l/8;
            }
        }

        // 移動先頂点を予め列挙
        side_x.assign(l,vector<vector<int>>(l));
        side_y.assign(l,vector<vector<int>>(l));
        rep(i,l) {
            rep(j,l) {
                rep(d,SIDE_DIR_NUM) {
                    nx = (i+sdx[d] + l)%l, ny = (j+sdy[d] + l)%l;
                    side_x[i][j].emplace_back(nx);
                    side_y[i][j].emplace_back(ny);
                }
            }
        }

        measure_time = mes_time[sq_s];
        measure_time = min(measure_time, 10000/(n*(int)(log(n)+2)));

        measure_cost = 0;
        rep(d,DIR_NUM) measure_cost += 10 + abs(dx[d]) + abs(dy[d]);
        measure_cost *= 100 * n * measure_time;

        // used & place_warm_dir 初期化
        used.assign(l,vector<bool>(l,false));
        place_warm_dir.assign(l,vector<vector<P>>(l));
        rep(i,n) {
            auto &&[x,y] = warm[i];
            rep(d,DIR_NUM) {
                nx = ((x+dx[d])%l + l)%l, ny = ((y+dy[d])%l + l)%l;
                used[nx][ny] = true;
                place_warm_dir[nx][ny].emplace_back(pair(i,d));
            }
        }

        // ng_to_score 初期化
        ng_to_score.assign(n+1,0);
        ng_to_score[0] = 1e14;
        rep(i,n) ng_to_score[i+1] = ng_to_score[i]*0.8;

        warm_idx.assign(n,0);
        set_exp2 = (2*set_cost[sq_s])*(2*set_cost[sq_s]);
    }

    void input() {
        rep(i,n) {
            int x,y; cin >> x >> y;
            warm.emplace_back(pair(x,y));
        }
    }

    void arrange() {
        situation.assign(n,0);
        situ_cnt.assign((1 << DIR_NUM),0);
        temp.assign(l,vector<int>(l,0));
        
        rep(i,l)rep(j,l) temp[i][j] = (i+j)%2;
        
        // set_num 初期化
        set_num = 0;
        rep(i,l) {
            rep(j,l) {
                nx = (i+1 + l)%l, ny = j;
                set_num += ( temp[i][j] != temp[nx][ny] );
                nx = i, ny = (j+1 + l)%l;
                set_num += ( temp[i][j] != temp[nx][ny] );
            }
        }
        // situ_cnt & ng_num 初期化
        ng_num = n;
        rep(i,n) {
            auto &&[x,y] = warm[i];
            rep(d,DIR_NUM) {
                nx = ((x+dx[d])%l + l)%l, ny = ((y+dy[d])%l + l)%l;
                situation[i] |= (temp[nx][ny] << d);
            }
            if( situ_cnt[situation[i]] == 0 ) ng_num--;
            situ_cnt[situation[i]]++;
        }

        // 焼きなまし法
        long long best_score = calcScore(), next_score;
        int set_num_stock = set_num, ng_num_stock = ng_num, time;
        int turn = 0, rx, ry, d;

        while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            // 近傍：1箇所 bit 反転
            rx = rand_int()%l, ry = rand_int()%l;

            for(d = 0; d < SIDE_DIR_NUM; d++) {
                if( temp[rx][ry] != temp[side_x[rx][ry][d]][side_y[rx][ry][d]] ) set_num--;
                else set_num++;
            }
            for(auto &&[idx,dir]:place_warm_dir[rx][ry]) {
                // situation も bit反転で変更
                situ_cnt[situation[idx]]--;
                ng_num += ( !situ_cnt[situation[idx]] );
                situation[idx] ^= (1 << dir);
                ng_num -= ( !situ_cnt[situation[idx]] );
                situ_cnt[situation[idx]]++;
            }

            next_score = calcScore();
            if( prob(best_score,next_score,start_time) > rand_double() ) {
                // cerr << best_score << " " << next_score << endl;
                best_score = next_score;
                set_num_stock = set_num;
                ng_num_stock = ng_num;
                temp[rx][ry] ^= 1;
            }
            else {
                for(auto &&[idx,dir]:place_warm_dir[rx][ry]) {
                    situ_cnt[situation[idx]]--;
                    situation[idx] ^= (1 << dir);
                    situ_cnt[situation[idx]]++;
                }
                set_num = set_num_stock;
                ng_num = ng_num_stock;
            }
            turn++;
        }
        cerr << turn << endl;
        // cerr << "NG : " << ng_num << endl << flush;
        // cerr << measure_cost << " " << set_num*set_exp2 << endl;
        
        // 各 bit が何の数値に定まるかを決める
        rep(i,l)rep(j,l) temp[i][j] = min(1000,max(0,500 + ( temp[i][j] ? 1 : -1 ) * set_cost[sq_s]) );

        // 滑らかなまま bit 判断がしやすいように緩急を付ける
        rep(num,200) {
            vector<vector<bool>> increment(l,vector<bool>(l,false));
            rep(i,l)rep(j,l) {
                bool f = true;
                rep(d,SIDE_DIR_NUM) {
                    if( temp[i][j] == 500 + set_cost[sq_s] + num*10 ) {
                        f &= (temp[side_x[i][j][d]][side_y[i][j][d]] == temp[i][j]);
                    }
                    else if( temp[i][j] == 500 - set_cost[sq_s] - num*10 ) {
                        f &= (temp[side_x[i][j][d]][side_y[i][j][d]] == temp[i][j]);
                    }
                    else f = false;
                }
                increment[i][j] = f;
            }
            rep(i,l)rep(j,l) {
                if( increment[i][j] ) {
                    if( temp[i][j] < 500 ) temp[i][j] -= 10;
                    else temp[i][j] += 10;
                    temp[i][j] = min(1000,max(0,temp[i][j]));
                }
            }
        }
        
        rep(_,100)rep(i,l)rep(j,l) {
            if( !used[i][j] ) {
                temp[i][j] = 0;
                rep(d,SIDE_DIR_NUM) {
                    nx = (i+sdx[d] + l)%l, ny = (j+sdy[d] + l)%l;
                    temp[i][j] += temp[nx][ny];
                }
                temp[i][j] /= SIDE_DIR_NUM;
            }
        }

        // 盤面出力
        rep(i,l) {
            rep(j,l) cout << temp[i][j] << " ";
            cout << endl << flush;
        }
    }

    inline long long calcScore() {
        // 一意な bit の数を調べる ⇒ 一意なら 100% 識別可能として正答率を求める
        return ng_to_score[ng_num] / ( 1 + set_num*set_exp2 );
    }
    
    void measure_predict() {
        vector<bool> selected(n,false);
        priority_queue<P,vector<P>,greater<P>> todo;
        int now_measure_time = 0;
        rep(i,n) {
            // cerr << i+1 << "th Meaure Time : " << measure_time << endl << flush;
            // cerr << "Now Meaured Time : " << now_measure_time << endl << endl << flush;

            int warm_bit = 0;
            vector<int> cand;
            rep(j,n) {
                if( selected[j] && s <= 700 ) continue;
                cand.emplace_back(j);
            }

            // 最も選択肢を減らせる方向を採用
            int used_cnt = 1;
            vector<bool> used_dir(DIR_NUM,false);
            while( true ) {
                rep(d,DIR_NUM) {
                    if( used_dir[d] ) continue;
                    int cnt = 0;
                    for(auto idx:cand) cnt += ((situation[idx] & (1 << d)) > 0);
                    todo.push(pair(max(cnt,(int)cand.size()-cnt),d));
                }
                if( todo.empty() ) break;
                auto [_,d] = todo.top(); todo.pop();
                while( !todo.empty()) todo.pop();
                used_dir[d] = true;

                double ave = 0, one_cnt = 0, zero_cnt = 0;
                while( abs(one_cnt-zero_cnt) < j_time[sq_s] ) {
                    // 計測 phase
                    cout << i << " " << dx[d]%l << " " << dy[d]%l << endl << flush;
                    int tmp; cin >> tmp;
                    if( tmp <= 500 - set_cost[sq_s] - judge[sq_s] ) zero_cnt++;
                    if( tmp >= 500 + set_cost[sq_s] + judge[sq_s] ) one_cnt++;
                    now_measure_time++;
                    if( now_measure_time == MAX_MESURE_TIME ) break;
                }
                if( one_cnt > zero_cnt ) warm_bit |= (1 << d);
                
                vector<int> next_cand;
                for(auto idx:cand) {
                    int cnt = 0;
                    rep(td,DIR_NUM) if( used_dir[td] ) cnt += ( (situation[idx] & (1 << td)) == (warm_bit & (1 << td)) );
                    if( cnt == used_cnt ) next_cand.emplace_back(idx);
                }
                used_cnt++;

                if( next_cand.empty() ) next_cand.emplace_back(cand[0]);
                swap(cand,next_cand);

                // 操作回数上限に来たら終了
                if( now_measure_time == MAX_MESURE_TIME ) break;

                // 一意に定まったらその時点で終了
                // ※ DIR_NUM に関わらず O(logN) で Measure が終了するはず？
                if( cand.size() == 1 ) break;
            }
            
            warm_idx[i] = cand[0];
            selected[cand[0]] = true;
            if( now_measure_time == MAX_MESURE_TIME ) break;

            // 余った操作回数を後に回す
            if( i != n-1 ) {
                measure_time = min(measure_time, (MAX_MESURE_TIME-now_measure_time-100) / ((n-(i+1))*(int)(log(n)+2)) );
                measure_time = max(measure_time, 1);
            }
        }
    }

    void output() {
        cout << -1 << " " << -1 << " " << -1 << endl << flush;
        rep(i,n) cout << warm_idx[i] << endl << flush;
    }

    void solve() {
        utility::mytm.CodeStart();
        start_time = utility::mytm.elapsed();
        
        // 配置 part
        arrange();

        // 計測・予測 part
        measure_predict();

        // 出力
        output();
    }
};

int main(){
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    cin >> l >> n >> s;
    if( s <= 4 ) {
        Solver_small solver;
        solver.solve();
    }
    else {
        Solver solver;
        solver.solve();
    }
    
    return 0;
}

ほんとだ
s=1だと1ケースであさとの50ケース総和の半分以上の点数出せるケースがあるからそいつっぽい


