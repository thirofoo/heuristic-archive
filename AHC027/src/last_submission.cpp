#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for(int i = 0; i < n; i++)
#define reps(i, s, n) for (int i = s; i < n; i++)

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
    double start_temp = 1000, end_temp = 10;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(long long best,long long now,int start) {
    return exp((double)(best-now) / temp(start));
}

//-----------------以下から実装部分-----------------//

using P = pair<int,int>;
using T = tuple<int,int,int,int,int>;

#define HIGH 80    // d を高いと判定する閾値
#define HIGH_T 200 // 高いところを巡回する周期
#define DIR_NUM 4

// 上下左右の順番
vector<int> dx = {-1, 1, 0, 0};
vector<int> dy = { 0, 0,-1, 1};

struct Rect{
    int x, y, xl, yl, area;
    Rect() : x(0), y(0), xl(0), yl(0) {}
    explicit Rect(int _x, int _y, int _xl, int _yl) : x(_x), y(_y), xl(_xl), yl(_yl), area(_xl*_yl) {}
    bool operator<(const Rect& r) const {
        if( !(area%2) == !(r.area%2) ) return (area < r.area);
        return (area%2);
    }
};

struct Solver{
    string s;
    int n, xl, yl, dr, dc, nx, ny, d_total, purpose;
    vector<bool> vis_rect, high_vis;
    vector<vector<bool>> vis;
    vector<vector<int>> d, rect, clean_turn, high_idx;
    vector<vector<vector<bool>>> wall;
    vector<vector<vector<vector<int>>>> every_dis;
    vector<vector<vector<vector<P>>>> every_pre;

    vector<Rect> rects;
    vector<P> ans, high;
    queue<T> todo;

    Solver(){
        this->input();
        purpose = HIGH_T/2;
        rect.assign(n+2,vector<int>(n+2,0));
        vis.assign(n+2,vector<bool>(n+2,true));
        clean_turn.assign(n+2,vector<int>(n+2,0));
        high_idx.assign(n+2,vector<int>(n+2,0));
        every_dis.assign(n+2,vector<vector<vector<int>>>(n+2));
        every_pre.assign(n+2,vector<vector<vector<P>>>(n+2));

        for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) {
            if( d[i][j] >= HIGH ) {
                high_idx[i][j] = high.size();
                high.emplace_back(P(i,j));
            }
            rect[i][j] = false;
            vis[i][j] = false;
        }
    }

    void input(){
        cin >> n;
        d.assign(n+2,vector<int>(n+2,0));
        wall.assign(n+2,vector(n+2,vector<bool>(4,true)));
        for(int i=1; i<=n-1; i++) {
            cin >> s;
            for(int j=1; j<=n; j++) {
                wall[i][j][1] = s[j-1]-'0';
                wall[i+1][j][0] = s[j-1]-'0';
            }
        }
        for(int i=1; i<=n; i++) {
            cin >> s;
            for(int j=1; j<=n-1; j++) {
                wall[i][j][3] = s[j-1]-'0';
                wall[i][j+1][2] = s[j-1]-'0';
            }
        }
        d_total = 0;
        for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) {
            cin >> d[i][j];
            d_total += d[i][j];
        }
        return;
    }

    void output(){
        rep(i,ans.size()-1) {
            auto&& [x1,y1] = ans[i];
            auto&& [x2,y2] = ans[i+1];
            char ch = changeChar(x1,y1,x2,y2);
            cout << ch;
        }
        cout << '\n' << flush;
        return;
    }

    void solve(){

        // まずは貪欲解
        // 1. 縦横少なくとも一方が偶数の長方形を確保 (今回は評価値は面積)
        // 2. 長方形を繋げていく (長方形は少ない方が良い)
        // ※ 奇数*奇数 の長方形は 辺の長さが1 ⇒ その面積分余分に移動必要
        //            〃          どちらも2以上 ⇒ 2だけ余分に移動必要

        int rect_cnt = 0, rect_idx = 1;
        rects.emplace_back(Rect());
        
        priority_queue<Rect> pq;
        while( rect_cnt < n*n ) {
            for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) {
                if( rect[i][j] ) continue;
                xl = yl = 0;
                expandRect(i,j,xl,yl);
                pq.push(Rect(i,j,xl,yl));
            }
            auto&& r = pq.top();
            rects.emplace_back(pq.top());
            rect_cnt += r.area;
            for(int i=r.x; i<r.x+r.xl; i++) for(int j=r.y; j<r.y+r.yl; j++) rect[i][j] = rect_idx;
            while( !pq.empty() ) pq.pop();
            rect_idx++;
        }
        rep(d,DIR_NUM) {
            rep(j,n+2) {
                rep(k,n+2) cerr << wall[j][k][d];
                cerr << endl;
            }
            cerr << endl;
        }
        cerr << endl;

        rep(i,n+2) {
            rep(j,n+2) cerr << rect[i][j] << " " << (rect[i][j] < 10 ? " " : "");
            cerr << '\n' << flush;
        }
        cerr << '\n' << flush;

        vis_rect.assign(rect_idx,false);
        vis_rect[0] = true;
        paintRect(rect[1][1],1,1);
        return;
    }

    inline void expandRect(int x, int y, int& xl, int& yl) {
        bool fr, fc;
        dr = dc = 1;
        while( dr != 0 || dc != 0 ) {
            // fr: 下に進行可能か, fc : 右に進行可能か
            fr = fc = true;
            for(int i=y; i<=y+yl; i++) {
                fr &= (!wall[x+xl][i][1] && !rect[x+xl+1][i]);
                if( i != y+yl ) fr &= (!wall[x+xl+1][i][3]);
            }
            if( !fr ) dr = 0;
            xl += dr;

            for(int i=x; i<=x+xl; i++) {
                fc &= (!wall[i][y+yl][3] && !rect[i][y+yl+1]);
                if( i != x+xl ) fc &= (!wall[i][y+yl+1][1]);
            }
            if( !fc ) dc = 0;
            yl += dc;
        }
        xl++, yl++;
        return;
    }

    void paintRect(int idx, int tx, int ty) {
        // 長方形塗りつぶし part (貪欲)
        // - 四隅のどこかに到達したら dfs っぽく次に行く感じ
        // ⇒ 縦長偶数 or 横長偶数 or それ以外 で分けて行き方をハードコード

        int px = tx, py = ty, ndir = (rects[idx].xl == 1 ? 2 : 0), cnt = 0;
        vis_rect[idx] = true;

        while( true ) {
            if( !vis[tx][ty] ) cnt++;
            vis[tx][ty] = true;
            clean_turn[tx][ty] = ans.size();
            ans.emplace_back(P(tx,ty));

            if( purpose <= ans.size() ) {
                cerr << "Yeah!\n";
                droppingByHigh(tx,ty,HIGH);
                purpose = ans.size() + HIGH_T;
            }

            rep(dir,DIR_NUM) {
                if( wall[tx][ty][dir] ) continue;
                nx = tx+dx[dir], ny = ty+dy[dir];
                
                if( vis_rect[rect[nx][ny]] ) continue; // 既に到達済みの長方形は continue
                paintRect(rect[nx][ny],nx,ny);
                clean_turn[tx][ty] = ans.size();
                ans.emplace_back(P(tx,ty));
            }

            auto&& r = rects[idx];

            int bx = tx-r.x+1, by = ty-r.y+1;
            if( r.xl == 1 ) {
                // 横一列
                nx = tx+dx[ndir], ny = ty+dy[ndir];
                if( wall[tx][ty][ndir] || rect[nx][ny] != idx ) ndir = (ndir == 2 ? 3 : 2);
            }
            else if( r.yl == 1 ) {
                // 縦一列
                nx = tx+dx[ndir], ny = ty+dy[ndir];
                if( wall[tx][ty][ndir] || rect[nx][ny] != idx ) ndir = (ndir == 0 ? 1 : 0);
            }
            else if( r.xl%2 == 0 ) { // 縦偶数長 ⇒ 横くねくね
                if( by == 1 && bx != r.xl ) ndir = 1;
                else if( bx != 1 && ((by == 2 && bx%2 == 1) || (by == r.yl && bx%2 == 0)) ) ndir = 0;
                else if( bx%2 == 1 ) ndir = 2;
                else ndir = 3;
            }
            else { // 横偶数長 ⇒ 縦くねくね
                if( bx == 1 && by != r.yl ) ndir = 3;
                else if( by != 1 && ( (bx == 2 && by%2 == 1) || (bx == r.xl && by%2 == 0) ) ) ndir = 2;
                else if( by%2 == 1 ) ndir = 0;
                else ndir = 1;
            }
            if( cnt == rects[idx].area ) break;
            tx += dx[ndir], ty += dy[ndir];
        }
        // 縦横ともに奇数の時は仕方なく戻る
        while( tx != px || ty != py ) {
            if( ans.back() != P(tx,ty) ) {
                clean_turn[tx][ty] = ans.size();
                ans.emplace_back(P(tx,ty));
            }
            if( tx != px ) ndir = (tx < px);
            if( ty != py ) ndir = 2 + (ty < py);
            tx += dx[ndir], ty += dy[ndir];
        }
        if( ans.back() != P(tx,ty) ) {
            clean_turn[tx][ty] = ans.size();
            ans.emplace_back(P(tx,ty));
        }
    }

    inline void droppingByHigh(int x, int y, int border) {
        // d >= 100 のマスに定期的に訪れる関数
        int pre_size = ans.size(), high_cnt = 0;
        high_vis.assign(high.size(),false);
        while( high_cnt++ < high.size() ) {
            int pp_size = ans.size();

            // 現地点から 未到達 & 一番近い high の場所に行く
            if( every_dis[x][y].empty() ) {
                vector<vector<int>> tmp_dis(n+2,vector<int>(n+2,-1));
                vector<vector<P>> tmp_pre(n+2,vector<P>(n+2,P(-1,-1)));
                todo.push(T(0,x,y,-1,-1));
                while( !todo.empty() ) {
                    auto [tdis,tx,ty,px,py] = todo.front(); todo.pop();
                    if( tmp_dis[tx][ty] != -1 ) continue;
                    tmp_dis[tx][ty] = tdis;
                    tmp_pre[tx][ty] = P(px,py);

                    rep(dir,DIR_NUM) {
                        if( wall[tx][ty][dir] ) continue;
                        nx = tx+dx[dir], ny = ty+dy[dir];
                        if( tmp_dis[nx][ny] != -1 ) continue;
                        todo.push(T(tdis+1,nx,ny,tx,ty));
                    }
                }
                swap(every_dis[x][y],tmp_dis);
                swap(every_pre[x][y],tmp_pre);
            }
            int mini_dis = 1e9, sx = -1, sy = -1;
            rep(i,high.size()) {
                auto&& [tx,ty] = high[i];
                if( d[tx][ty] < border ) continue;
                if( !high_vis[i] && every_dis[x][y][tx][ty] < mini_dis ) {
                    mini_dis = every_dis[x][y][tx][ty];
                    sx = tx, sy = ty;
                }
            }

            // P(sx,sy) へ移動
            int tmp_x = sx, tmp_y = sy;
            while( every_pre[x][y][tmp_x][tmp_y] != P(-1,-1) ) {
                auto&& [px,py] = every_pre[x][y][tmp_x][tmp_y];
                clean_turn[tmp_x][tmp_y] = ans.size();
                ans.emplace_back(P(tmp_x,tmp_y));
                tmp_x = px, tmp_y = py;
            }
            reverse(ans.begin()+pp_size,ans.end());
            high_vis[high_idx[sx][sy]] = true;
            x = sx, y = sy;
        }
        int size = ans.size();
        for(int i=size-1; i>=pre_size-1; i--) if( ans.back() != ans[i] ) ans.emplace_back(ans[i]);
        return;
    }

    inline bool outField(int x,int y){
        if(1 <= x && x <= n && 1 <= y && y <= n)return false;
        return true;
    }

    inline char changeChar(int x1, int y1, int x2, int y2) {
        if     ( x2-x1 ==  1 ) return 'D';
        else if( x2-x1 == -1 ) return 'U';
        else if( y2-y1 ==  1 ) return 'R';
        else                   return 'L';
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