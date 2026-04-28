#include <bits/stdc++.h>
using namespace std;
#if __has_include(<atcoder/all>)
    #include <atcoder/all>
using namespace atcoder;
#endif
typedef long long ll;
typedef pair<int, int> P;
typedef tuple<int, int, int, int> T;
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
#define TIME_LIMIT 3500
inline double temp(double start) {
    double start_temp = 100,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

// 左上右下の順番
#define DIR_NUM 4
const vector<int> dx = {0,-1,0,1};
const vector<int> dy = {-1,0,1,0};

// 左上右下の順番
#define MEASURE_DIR_NUM 4
vector<int> mdx = {0,-4,0,4};
vector<int> mdy = {-4,0,4,0};

struct Solver{
    int l, n, s, measure_time, nx, ny;
    vector<P> warm;
    vector<int> warm_idx;
    vector<vector<int>> temperature, measure_v;
    vector<vector<bool>> used;

    Solver(){

    }

    void input() {
        cin >> l >> n >> s;
        used.assign(l,vector<bool>(l,false));
        rep(i,n) {
            int x,y; cin >> x >> y;
            warm.emplace_back(P(x,y));
        }
        // 信頼区間95%, 信頼幅5とした時の操作回数
        measure_time = max( (int)(( (1.96*sqrt(s)) / 5.0 )*( (1.96*sqrt(s)) / 5.0 )), 1 );
        measure_time = min( measure_time, 10000 / (n*MEASURE_DIR_NUM) );

        cerr << measure_time << endl << flush;
        temperature.assign(l,vector<int>(l));
        warm_idx.assign(n,0);
        rep(d,MEASURE_DIR_NUM) {
            if( mdx[d] > 0 ) mdx[d] = l/4;
            else if(mdx[d] < 0) mdx[d] = -l/4;
            if( mdy[d] > 0 ) mdy[d] = l/4;
            else if(mdy[d] < 0) mdy[d] = -l/4;
        }
        rep(i,n) {
            auto &&[x,y] = warm[i];
            rep(d,MEASURE_DIR_NUM) {
                nx = ((x+mdx[d])%l + l)%l, ny = ((y+mdy[d])%l + l)%l;
                used[nx][ny] = true;
            }
        }
    }

    void arrange() {
        // まずは z = a*sin((2π/l)*x) + a*sin((2π/l)*y) + 2*a で固定
        int a = min(250,s*l);
        rep(i,l) {
            rep(j,l) {
                double rad1 = (2*M_PI / (double)l) * (double)i;
                double rad2 = (2*M_PI / (double)l) * (double)j;
                temperature[i][j] = a*sin(rad1) + a*sin(rad2) + 2*a;
            }
        }
        // while( utility::mytm.elapsed() <= TIME_LIMIT ) {
        //     int x = rand_int()%l, y = rand_int()%l;
        //     if( used[x][y] ) continue;
        //     int dz = 20 - rand_int()%41;
        //     if( temperature[x][y]+dz < 0 || temperature[x][y]+dz > 1000 ) continue;
        //     int now = 0, next = 0;
        //     rep(d,DIR_NUM) {
        //         nx = ((x+dx[d])%l + l)%l, ny = ((y+dy[d])%l + l)%l;
        //         // 二乗誤差で判定
        //         now += (temperature[x][y]-temperature[nx][ny]) * (temperature[x][y]-temperature[nx][ny]);
        //         next += (temperature[x][y]+dz-temperature[nx][ny]) * (temperature[x][y]+dz-temperature[nx][ny]);
        //     }
        //     if( next < now ) temperature[x][y] += dz;
        // }
        rep(i,l) {
            rep(j,l) cout << temperature[i][j] << " ";
            cout << endl << flush;
        }
    }

    void measure() {
        measure_v.assign(n,vector<int>(5,0));

        rep(i,n) {
            rep(d,MEASURE_DIR_NUM) {
                rep(_,measure_time) {
                    cout << i << " " << mdx[d]%l << " " << mdy[d]%l << endl << flush;
                    int tmp; cin >> tmp;
                    measure_v[i][d] += tmp;
                }
                // 雑に割って平均を出す
                measure_v[i][d] /= measure_time;
            }
        }
    }

    void output() {
        cout << -1 << " " << -1 << " " << -1 << endl << flush;
        rep(i,n) cout << warm_idx[i] << endl << flush;
    }

    void solve() {
        utility::mytm.CodeStart();
        
        // ----------配置part---------- //
        arrange();


        // ----------計測part---------- //
        measure();


        // ----------予測part---------- //
        rep(i,n) {
            int min_diff = 1e9;
            P resemble = P(0,0);
            rep(x,l) {
                rep(y,l) {
                    int cand = 0;
                    rep(d,MEASURE_DIR_NUM) {
                        nx = ((x+mdx[d])%l + l)%l, ny = ((y+mdy[d])%l + l)%l;
                        // 二乗誤差で判定
                        cand += (measure_v[i][d]-temperature[nx][ny]) * (measure_v[i][d]-temperature[nx][ny]);
                    }
                    if( cand < min_diff ) {
                        min_diff = cand;
                        resemble = P(x,y);
                    }
                }
            }
            // 一番 resemble に近い warm を答えとする
            int min_dis = 1e9;
            auto &&[rx,ry] = resemble;
            cerr << "Rsemble : " << rx << " " << ry << endl << flush;
            rep(j,n) {
                auto &&[wx,wy] = warm[j];
                cerr << wx << " " << wy << endl << flush;
                if( abs(wx-rx) + abs(wy-ry) < min_dis ) {
                    min_dis = abs(wx-rx) + abs(wy-ry);
                    warm_idx[i] = j;
                }
            }
        }
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

