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

int turn = 0;
vector<int> dx = {1, -1, 0,  0, 0,  0};
vector<int> dy = {0,  0, 1, -1, 0,  0};
vector<int> dz = {0,  0, 0,  0, 1, -1};
priority_queue<T> pq1,pq2;

struct Stick{
    // State内で用いる変数等の定義
    int nz, ny, nx, dir, size, d, id;
    vector<T> place;

    Stick(int _z, int _y, int _x, int _dir, int _d, int _id){
        // コンストラクタ(初期化)
        this->nx = _x;
        this->ny = _y;
        this->nz = _z;
        this->dir = _dir;
        this->d = _d;
        this->id = _id;
        size = 1;
        place.emplace_back(T(nz,ny,nx));
    }

    inline void changeDir(vector<vector<vector<int>>> &ans,int n_dir){
        while(place.size() > 1){
            auto [z,y,x] = place.back();
            ans[z][y][x] = 0;
            place.pop_back();
        }
        if(n_dir == -1)return;
        this->dir = n_dir;
        int now_x = nx + dx[this->dir];
        int now_y = ny + dy[this->dir];
        int now_z = nz + dz[this->dir];
        this->size = 1;
        while(!outField_3d(now_x,now_y,now_z) && !ans[now_z][now_y][now_x]){
            ans[now_z][now_y][now_x] = this->id;
            place.emplace_back(T(now_z,now_y,now_x));
            now_z += dz[this->dir];
            now_y += dy[this->dir];
            now_x += dx[this->dir];
            this->size++;
        }
    }

    inline void eraseStick(vector<vector<vector<int>>> &ans){
        while(place.size() > 1){
            auto [z,y,x] = place.back();
            ans[z][y][x] = 0;
            place.pop_back();
        }
    }

    inline void cutStick(vector<vector<vector<int>>> &ans,int s,int nid){
        while(place.size() > s){
            auto [z,y,x] = place.back();
            ans[z][y][x] = 0;
            place.pop_back();
        }
        this->id = nid;
        for(auto [z,y,x]:place)ans[z][y][x] = this->id;
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
    vector<Stick> Stick1,Stick2;
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
        ans1.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
        ans2.assign(d,vector<vector<int>>(d,vector<int>(d,0)));
    }

    void solve(){
        utility::mytm.CodeStart();

        // 必要最低限の1*1のみで構成
        int cnt = 1;
        rep(z,d){
            vector<bool> r(d,false),c(d,false);
            vector<int> ri,ci;
            rep(x,d)if(sil1_f[z][x])ci.emplace_back(x);
            rep(y,d)if(sil1_r[z][y])ri.emplace_back(y);
            
            rep(y,d)rep(x,d)if(!sil1_f[z][x] || !sil1_r[z][y])ans1[z][y][x] = -1;
            rep(i,max(ri.size(),ci.size())){
                ans1[z][ri[i%ri.size()]][ci[i%ci.size()]] = cnt;
                cnt++;
            }
        }
        rep(z,d){
            vector<bool> r(d,false),c(d,false);
            vector<int> ri,ci;
            rep(x,d)if(sil2_f[z][x])ci.emplace_back(x);
            rep(y,d)if(sil2_r[z][y])ri.emplace_back(y);

            rep(y,d)rep(x,d)if(!sil2_f[z][x] || !sil2_r[z][y])ans2[z][y][x] = -1;
            rep(i,max(ri.size(),ci.size())){
                ans2[z][ri[i%ri.size()]][ci[i%ci.size()]] = cnt;
                cnt++;
            }
        }
        rep(z,d)rep(y,d)rep(x,d){
            if(ans1[z][y][x] > 0)Stick1.emplace_back(Stick(z,y,x,-1,d,ans1[z][y][x]));
            if(ans2[z][y][x] > 0)Stick2.emplace_back(Stick(z,y,x,-1,d,ans2[z][y][x]));
        }
        best_score1 = calcScore(1);
        best_score2 = calcScore(2);

        // ↑ を核として出来るだけ長いstickを作る
        while(utility::mytm.elapsed() <= TIME_LIMIT/2){
            int rnd = rand_int()%Stick1.size();
            int dir = rand_int()%dx.size();
            while(dir == Stick1[rnd].dir)dir = rand_int()%dx.size();

            int pre_dir = Stick1[rnd].dir;
            Stick1[rnd].changeDir(ans1,dir);

            ll now_score = calcScore(1);
            if(now_score < best_score1){
                best_score1 = now_score;
            }
            else Stick1[rnd].changeDir(ans1,pre_dir);
            turn++;
        }

        turn = 0;
        while(utility::mytm.elapsed() <= TIME_LIMIT){
            int rnd = rand_int()%Stick2.size();
            int dir = rand_int()%dx.size();
            while(dir == Stick2[rnd].dir)dir = rand_int()%dx.size();

            int pre_dir = Stick2[rnd].dir;
            Stick2[rnd].changeDir(ans2,dir);

            ll now_score = calcScore(2);
            if(now_score < best_score2){
                best_score2 = now_score;
            }
            else Stick2[rnd].changeDir(ans2,pre_dir);
        }

        rep(i,Stick1.size())pq1.push(P(Stick1[i].size,i));
        rep(i,Stick2.size())pq2.push(P(Stick2[i].size,i));
        while(!pq1.empty() && !pq2.empty()){
            auto [size1,i1] = pq1.top(); pq1.pop();
            auto [size2,i2] = pq2.top(); pq2.pop();
            int size = min(size1,size2);
            Stick1[i1].cutStick(ans1,size,Stick1[i1].id);
            Stick2[i2].cutStick(ans2,size,Stick1[i1].id);
        }

        // 過剰分stickは1*1に戻す
        while(!pq1.empty() || !pq2.empty()){
            if(!pq1.empty()){
                auto [size1,i1] = pq1.top(); pq1.pop();
                Stick1[i1].cutStick(ans1,1,Stick1[i1].id);
            }
            else{
                auto [size2,i2] = pq2.top(); pq2.pop();
                Stick2[i2].cutStick(ans2,1,Stick2[i2].id);
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

    ll calcScore(int i){
        ld score = 0;
        for(auto stick:(i == 1 ? Stick1 : Stick2)){
            score += 1/(ld)stick.size;
        }
        return score*1e9;
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