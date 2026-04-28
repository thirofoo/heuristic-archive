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

inline double gaussian(double mean, double stddev) {
    // 標準正規分布からの乱数生成（Box-Muller変換
    double z0 = sqrt(-2.0 * log(rand_double())) * cos(2.0 * M_PI * rand_double());
    // 平均と標準偏差の変換
    return mean + z0 * stddev;
}

//温度関数
#define TIME_LIMIT 2950
inline double temp(double start) {
    double start_temp = 100,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

struct State{

    State() {

    }
};

struct Line {
    // type 1: y = ax + b ( 勾配 a, 切片 b )
    // type 2: ax + by + c = 0 ( 2頂点 p1, p2 を通る直線 )
    ll a, b, c, type;
    Line(const ll a, const ll b) : a(a), b(b), c(0), type(1) {}
    Line(const P &p1, const P &p2) : type(2) {
        if( p1 == p2 ) cerr << "Warn: Same Vertex (p1,p2)\n";
        auto &&[x1, y1] = p1;
        auto &&[x2, y2] = p2;
        ll dx = x1 - x2, dy = y1 - y2;
        a = -dy, b = dx, c = dy*x1 - dx*y1;
    
        // gcd(a,b,c) = 1 & a > 0 で管理 ※ a == 0 の時は b > 0 で管理
        if( a < 0 || ( a == 0 && b < 0 ) ) a *= -1, b *= -1, c *= -1;
        ll g = gcd(abs(a), gcd(abs(b), abs(c)));
        a /= g, b /= g, c /= g;
    }
    inline bool isOnLine(const P &p) {
        auto &&[x,y] = p;
        bool res = false;
        res |= ( type == 1 && y == a*x + b );
        res |= ( type == 2 && a*x + b*y + c == 0 );
        return res;
    }
    inline double distance(const P &p) {
        auto &&[x,y] = p;
        if( type == 1 ) return abs(a*x - y + b) / sqrt(a*a + 1);
        if( type == 2 ) return abs(a*x + b*y + c) / sqrt(a*a + b*b);
        return 0;
    }
    // 演算子 overload (sort用)
    constexpr bool operator<(const Line& l) const { return tuple(a,b,c) < tuple(l.a,l.b,l.c); }
    constexpr bool operator>(const Line& l) const { return tuple(a,b,c) > tuple(l.a,l.b,l.c); }
    constexpr bool operator==(const Line& l) const { return a == l.a && b == l.b && c == l.c; }
};

struct Solver{
    int N, M, sx, sy, c, h;
    double eps, delta;
    vector<int> q;
    vector<pair<int, int>> dest;
    vector<Line> wall;

    Solver() {
        this->input();
        q.resize(N);
    }

    void input() {
        cin >> N >> M >> eps >> delta >> sx >> sy;
        rep(i, N) {
            int x, y; cin >> x >> y;
            dest.emplace_back(pair(x, y));
        }
        rep(i, M) {
            int lx, ly, rx, ry; cin >> lx >> ly >> rx >> ry;
            wall.emplace_back(Line(P(lx, ly), P(rx, ry)));
        }
        return;
    }

    void output() {

        return;
    }

    void solve() {
        // case A のみの貪欲解
        // 1. その位置から最も近い & 訪れたことのない dest に向かう加速度を壁衝突まで生成
        // 2. 10 回 sample を取った平均を xy 方向にやって平均を現在位置にする

        if( M == 0 ) caseA();
        else caseB();
    }

    void caseA() {
        int op_cnt = 5000, vis_cnt = 0;
        double now_x = 0, now_y = 0;
        vector<bool> visited(N, false);

        while( true ) {
            // ===== 1. その位置から最も近い & 未訪問の dest に向かう加速度を壁衝突まで生成 =====
            double min_dist = 1e9;
            int min_idx = -1;
            rep(i, N) {
                if( visited[i] ) continue;
                auto &&[x, y] = dest[i];
                double dist = sqrt((x - now_x)*(x - now_x) + (y - now_y)*(y - now_y));
                if( dist < min_dist ) {
                    min_dist = dist;
                    min_idx = i;
                }
            }
            auto &&[nearest_x, nearest_y] = dest[min_idx];
            double ax = nearest_x - now_x, ay = nearest_y - now_y;

            c = 0, h = 0;
            while( c == 0 ) {
                double norm = sqrt(ax*ax + ay*ay);
                ax /= norm, ay /= norm;
                double ratio = min(min_dist, 500.0-1.0);
                ax *= ratio, ay *= ratio;
                cout << "A " << (int)ax << " " << (int)ay << endl << flush;
                cin >> c >> h;
                cerr << "c: " << c << " h: " << h << endl;
                rep(i, h) {
                    int place; cin >> place;
                    visited[place] = true;
                    vis_cnt++;
                    if( vis_cnt == N ) return;
                }
                op_cnt--;
                if( op_cnt <= 0 ) return;
            }
            cerr << endl;

            // ===== 2. xy 方向に measure 回ずつ計測して現在位置にする =====
            cout << "S " << 1 << " " << 0 << endl << flush;
            cin >> now_x;
            cin >> c >> h;
            // cerr << "c: " << c << " h: " << h << endl;
            rep(i, h) {
                int place; cin >> place;
                visited[place] = true;
                vis_cnt++;
                if( vis_cnt == N ) return;
            }
            op_cnt--;
            if( op_cnt <= 0 ) return;

            cout << "S " << 0 << " " << 1 << endl << flush;
            cin >> now_y;
            cin >> c >> h;
            // cerr << "c: " << c << " h: " << h << endl;
            rep(i, h) {
                int place; cin >> place;
                visited[place] = true;
                vis_cnt++;
                if( vis_cnt == N ) return;
            }
            op_cnt--;
            if( op_cnt <= 0 ) return;

            // cerr << "now_x: " << now_x << " now_y: " << now_y << endl;
            now_x = (100000.0 - now_x);
            now_y = (100000.0 - now_y);
        }
        return;
    }

    void caseB() {
        return;
    }
};

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.solve();
    solver.output();
    
    return 0;
}