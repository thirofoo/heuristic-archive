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
#define TIME_LIMIT 1850
inline double temp(double start) {
    double start_temp = 100,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

#define DIR_NUM 4
// 左上右下の順番
vector<int> dx = {0,-1,0,1};
vector<int> dy = {-1,0,1,0};

struct Solver{
    bool flag;
    int n, m, query, best_score, now_score, r_num, c_num;
    int c1, c2, c3, c4, time, nx, ny, nnx, nny, nd, iteration;
    vector<vector<int>> color, depend;
    queue<tuple<int,int,int>> rollback;
    queue<pair<int,int>> todo;
    vector<bool> vis_color;
    vector<vector<bool>> vis;

    Solver(){
        this->input();
        r_num = n, c_num = n;
        depend.assign(m+1,vector<int>(m+1,0));
        for(int i=0;i<=m;i++) depend[i][i] = 1e6;
        for(int i=1;i<=n;i++) for(int j=1;j<=n;j++) {
            for(int d=0;d<DIR_NUM;d++) {
                nx = i + dx[d], ny = j + dy[d];
                depend[color[i][j]][color[nx][ny]]++;
                depend[color[nx][ny]][color[i][j]]++;
            }
        }
        for(int i=1;i<=m;i++) for(int j=1;j<=m;j++) depend[i][j] /= 2;
    }

    void input(){
        cin >> n >> m;
        color.assign(n+2,vector<int>(n+2,0));
        for(int i=1;i<=n;i++) for(int j=1;j<=n;j++) cin >> color[i][j];
        return;
    }

    void output(){
        for(int i=1;i<=n;i++) {
            for(int j=1;j<=n;j++) cout << color[i][j] << " ";
            cout << '\n';
        }
        return;
    }

    inline void erase_row(int row) {
        if( row <= 0 || row >= r_num+1 ) return;

        c3 = color[row][0], c4 = color[row][1];
        rollback.push(tuple(c3,c4,1));
        depend[c3][c4]--; depend[c4][c3]--;
        flag = (depend[c3][c4] && depend[c4][c3]);

        for(int j=1;j<=n;j++) {
            if( !flag ) break;
            c1 = color[row-1][j], c2 = color[row+1][j];
            c3 = color[row][j], c4 = color[row][j+1];
            flag &= (depend[c1][c2] && depend[c2][c1]);
            depend[c1][c2]++; depend[c2][c1]++;
            depend[c1][c3]--; depend[c3][c1]--;
            depend[c2][c3]--; depend[c3][c2]--;
            depend[c3][c4]--; depend[c4][c3]--;
            rollback.push(tuple(c1,c2,-1)), rollback.push(tuple(c1,c3,1));
            rollback.push(tuple(c2,c3,1)), rollback.push(tuple(c3,c4,1));
            flag &= (depend[c1][c3] && depend[c3][c1]);
            flag &= (depend[c2][c3] && depend[c3][c2]);
            flag &= (depend[c3][c4] && depend[c4][c3]);
        }
        if( !flag || !linking(0,row) ) return color_rollback();

        // cerr << "Queyr1 is executed.\n";
        for(int i=row;i<=r_num;i++) {
            for(int j=1;j<=n;j++) {
                color[i][j] = color[i+1][j];
            }
        }
        r_num--;
        while( !rollback.empty() ) rollback.pop();
        // this->output();
        
        iteration++;
        return;
    }

    inline void erase_col(int col) {
        if( col <= 0 || col >= c_num+1 ) return;

        c3 = color[0][col], c4 = color[1][col];
        rollback.push(tuple(c3,c4,1));
        depend[c3][c4]--; depend[c4][c3]--;
        flag = (depend[c3][c4] && depend[c4][c3]);

        for(int i=1;i<=n;i++) {
            if( !flag ) break;
            c1 = color[i][col-1], c2 = color[i][col+1];
            c3 = color[i][col], c4 = color[i+1][col];
            flag &= (depend[c1][c2] && depend[c2][c1]);
            depend[c1][c2]++; depend[c2][c1]++;
            depend[c1][c3]--; depend[c3][c1]--;
            depend[c2][c3]--; depend[c3][c2]--;
            depend[c3][c4]--; depend[c4][c3]--;
            rollback.push(tuple(c1,c2,-1)), rollback.push(tuple(c1,c3,1));
            rollback.push(tuple(c2,c3,1)), rollback.push(tuple(c3,c4,1));
            flag &= (depend[c1][c3] && depend[c3][c1]);
            flag &= (depend[c2][c3] && depend[c3][c2]);
            flag &= (depend[c3][c4] && depend[c4][c3]);
        }
        if( !flag || !linking(1,col) ) return color_rollback();

        // cerr << "Queyr2 is executed.\n";
        for(int j=col;j<=c_num;j++) {
            for(int i=1;i<=n;i++) {
                color[i][j] = color[i][j+1];
            }
        }
        c_num--;
        while( !rollback.empty() ) rollback.pop();
        // this->output();

        iteration++;
        return;
    }

    inline bool move_color(int x, int y, int dir) {
        nx = x + dx[dir], ny = y + dy[dir];
        if( nx == 0 || ny == 0 || nx == r_num+1 || ny == c_num+1 || color[nx][ny] == color[x][y] ) return false;

        flag = true;
        c3 = color[x][y];
        for(int d=0;d<DIR_NUM;d++) {
            if( !flag ) break;
            nnx = nx + dx[d], nny = ny + dy[d];
            c1 = color[nx][ny], c2 = color[nnx][nny];
            flag &= (depend[c2][c3] && depend[c3][c2]);
            depend[c1][c2]--, depend[c2][c1]--;
            depend[c2][c3]++, depend[c3][c2]++;
            rollback.push(tuple(c1,c2,1));
            rollback.push(tuple(c2,c3,-1));
            flag &= (depend[c1][c2] && depend[c2][c1]);
        }

        if( !flag || !linking_one(nx,ny) ) {
            color_rollback();
            return false;
        }

        // cerr << "Queyr3 is executed.\n";
        while( !rollback.empty() ) rollback.pop();
        color[nx][ny] = c3;

        // this->output();

        iteration++;
        return true;
    }

    inline void erase_color(int x, int y) {
        flag = false;
        for(int d=0;d<DIR_NUM;d++) {
            nx = x + dx[d], ny = y + dy[d];
            flag |= (color[nx][ny] == 0);
        }
        if( !flag ) return;

        c3 = 0;
        for(int d=0;d<DIR_NUM;d++) {
            if( !flag ) break;
            nx = x + dx[d], ny = y + dy[d];
            c1 = color[x][y], c2 = color[nx][ny];
            flag &= (depend[c2][c3] && depend[c3][c2]);
            depend[c1][c2]--, depend[c2][c1]--;
            depend[c2][c3]++, depend[c3][c2]++;
            rollback.push(tuple(c1,c2,1));
            rollback.push(tuple(c2,c3,-1));
            flag &= (depend[c1][c2] && depend[c2][c1]);
        }

        if( !flag || !linking_one(x,y) ) return color_rollback();

        while( !rollback.empty() ) rollback.pop();
        color[x][y] = 0;
        return;
    }

    inline bool linking(bool r_or_c, int num) {
        bool res = true;
        vis_color.assign(m+1,false);
        vis.assign(n+2,vector<bool>(n+2,false));
        for(int i=1;i<=n;i++) {
            if( !r_or_c && i == num ) continue;
            for(int j=1;j<=n;j++) {
                if( vis[i][j] || (r_or_c && j == num) ) continue;
                if( vis_color[color[i][j]] ) {
                    res = false;
                    break;
                }
                vis_color[color[i][j]] = true;
                todo.push(pair(i,j));
                while( !todo.empty() ) {
                    auto &&[x,y] = todo.front(); todo.pop();
                    if( vis[x][y] || x <= 0 || x > n || y <= 0 || y > n ) continue;
                    vis[x][y] = true;
                    for(int d=0;d<DIR_NUM;d++) {
                        nx = x + dx[d], ny = y + dy[d];
                        nx += ((!r_or_c && nx == num) ? dx[d] : 0);
                        ny += (( r_or_c && ny == num) ? dy[d] : 0);
                        
                        if( outField(nx,ny) || vis[nx][ny] || color[nx][ny] != color[x][y] ) continue;
                        todo.push(pair(nx,ny));
                    }
                }
            }
            if( !res ) break;
        }
        return res;
    }

    inline bool linking_one(int x, int y) {
        // chokudaiさん考察 ( 予め連結性が無さそうなら判定しない )
        if( (color[x+1][y] == color[x-1][y]) && ( ( color[x-1][y-1] != color[x+1][y] || color[x][y-1] != color[x+1][y] || color[x+1][y-1] != color[x+1][y] ) && ( color[x-1][y+1] != color[x+1][y] || color[x][y+1] != color[x+1][y] || color[x+1][y+1] != color[x+1][y] ) ) ) return false;
        if( (color[x][y+1] == color[x][y-1]) && ( ( color[x-1][y-1] != color[x][y+1] || color[x-1][y] != color[x][y+1] || color[x-1][y+1] != color[x][y+1] ) && ( color[x+1][y-1] != color[x][y+1] || color[x+1][y] != color[x][y+1] || color[x+1][y+1] != color[x][y+1] ) ) ) return false;

        bool res = true;
        vis_color.assign(m+1,false);
        vis.assign(n+2,vector<bool>(n+2,false));
        for(int d=0;d<DIR_NUM;d++) {
            nnx = x + dx[d], nny = y + dy[d];
            if( vis[nnx][nny] ) continue;
            if( vis_color[color[nnx][nny]] ) {
                res = false;
                break;
            }
            vis_color[color[nnx][nny]] = true;
            todo.push(pair(nnx,nny));
            while( !todo.empty() ) {
                auto &&[tx,ty] = todo.front(); todo.pop();
                if( vis[tx][ty] || tx <= 0 || tx > n || ty <= 0 || ty > n ) continue;
                vis[tx][ty] = true;
                for(int nd=0;nd<DIR_NUM;nd++) {
                    nnx = tx + dx[nd], nny = ty + dy[nd];
                    if( outField(nnx,nny) || vis[nnx][nny] || color[nnx][nny] != color[tx][ty] || (nnx == x && nny == y) ) continue;
                    todo.push(pair(nnx,nny));
                }
            }
            if( !res ) break;
        }
        return res;
    }

    inline bool outField(int x,int y){
        if(0 <= x && x < n+2 && 0 <= y && y < n+2)return false;
        return true;
    }

    inline void color_rollback() {
        while( !rollback.empty() ) {
            auto &&[e1,e2,v] = rollback.front(); rollback.pop();
            depend[e1][e2] += v;
            depend[e2][e1] += v;
        }
        return;
    }

    inline int calc_score() {
        int res = 1;
        for(int i=1;i<=n;i++) for(int j=1;j<=n;j++) {
            res += ( color[i][j] == 0 );
        }
        return res;
    }

    void solve(){
        utility::mytm.CodeStart();
        best_score = 0;
        time = 0;
        iteration = 0;
        int r, c, dir;
        bool next;

        // 山登り法
        while( utility::mytm.elapsed() <= TIME_LIMIT/2 ) {
            // 近傍 : 隣接マス1箇所色変更 ⇒ そこら辺の行・列削除が出来るかを見る
            next = false;
            r = rand_int()%r_num + 1, c = rand_int()%c_num + 1, dir = rand_int()%DIR_NUM;
            if( !move_color(r, c, dir) ) continue;
            
            r += dx[dir], c += dy[dir];
            erase_row(r-1), erase_row(r), erase_row(r+1);
            erase_col(c-1), erase_col(c), erase_col(c+1);
        }
        cerr << iteration << '\n';

        while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            next = false;
            r = rand_int()%r_num + 1, c = rand_int()%c_num + 1, dir = rand_int()%DIR_NUM;
            if( !move_color(r, c, dir) ) continue;

            // 削れるところを削る
            for(int d=0;d<DIR_NUM;d++) {
                nx = r + dx[d], ny = c + dy[d];
                if( color[nx][ny] == 0 ) continue;
                erase_color(nx,ny);
            }
            r += dx[dir], c += dy[dir];
            for(int d=0;d<DIR_NUM;d++) {
                nx = r + dx[d], ny = c + dy[d];
                if( color[nx][ny] == 0 ) continue;
                erase_color(nx,ny);
            }
        }
        cerr << iteration << '\n';

        return;
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