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
#define reps(i, l, r) for(ll i = l; i < r; i++)

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
#define TIME_LIMIT 2800
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

struct Solver{
    int w, d, n;
    vector<vector<int>> area;
    vector<int> line_row, line_col;

    // score, { {x1, y1, x2, y2} }
    using Answer = pair<int, vector<vector<vector<T>>>>;
    priority_queue<Answer, vector<Answer>, greater<Answer>> pq;

    Solver() {
        this->input();
        
    }

    void input() {
        cin >> w >> d >> n;
        area.assign(d, vector<int>(n, 0));
        rep(i,d) rep(j,n) cin >> area[i][j];
        return;
    }

    void output() {
        if (pq.empty()) {
            cerr << "No Answer" << endl;
            return;
        }
        Answer best = pq.top();
        for(auto &&day_place: best.second) {
            // pair(area_size, {x1, y1, x2, y2})
            vector<pair<int,T>> day_sort_place;
            rep(i,day_place.size()) {
                auto &&row_place = day_place[i];
                rep(j, row_place.size()) {
                    auto [x1, y1, x2, y2] = row_place[j];
                    if( j == row_place.size()-1 ) y2 = w;
                    day_sort_place.emplace_back((x2-x1)*(y2-y1), T(x1, y1, x2, y2));
                }
            }
            sort(day_sort_place.begin(), day_sort_place.end());
            for(auto &&[area_size, place]: day_sort_place) {
                auto [x1, y1, x2, y2] = place;
                cout << x1 << " " << y1 << " " << x2 << " " << y2 << endl;
            }
        }
        return;
    }

    void solve() {
        utility::mytm.CodeStart();
        // 初期貪欲解法
        // 1. 行で分割する数を決める
        // 2. 各日に合わせる用に各区間に列の柵を追加
        // ※ 一旦日ごとで考えて柵の移動を最小化等はしない

        // 上手く列の柵が立てば ( 行分割の数 ) * W * D で可能
        // => 列の柵の立て方が重要
        int max_ele = 0;
        rep(i,d) max_ele = max(max_ele, area[i].back());

        reps(X,1,w+1) {
            // 1. 行分割の数 (X) を決定
            // max_{i}( ∑_{j} ( ((area[i][j]-1) / height) + 1 ) * X ) <= w^2
            // を満たさないと分割不可能
            int height = w / X;
            int day_max_cnt = 0;
            rep(i,d) {
                int cnt = 0;
                rep(j,n) cnt += ((area[i][j]-1) / height) + 1;
                day_max_cnt = max(day_max_cnt, cnt);
            }
            if (day_max_cnt > w*w) continue;
            // 同時に height * w >= max_ele でないと NG
            // ※ 一番大きい要素が 1 行に収まらない
            if (height * w < max_ele) break;

            // 2. 列の柵を立てる
            Answer cand = Answer(0, {});
            bool flag = true;
            rep(i,d) {
                priority_queue<P> row; // {空列, 行番号}
                vector<vector<T>> place(X);
                rep(i,X) row.push({w, i});
                // 大きい area から空きが多い行に割り当て
                for(int j=n-1; j>=0; j--) {
                    auto [capasity, idx] = row.top(); row.pop();
                    int need = ((area[i][j]-1) / height) + 1;
                    if (capasity < need) {
                        flag = false;
                        break;
                    }
                    place[idx].emplace_back(T(idx*height, w-capasity, (idx+1)*height, w-capasity+need));
                    row.push(P(capasity-need, idx));
                }
                if (!flag) break;
                // 前回と線分の差分を見てスコアを計算
                if( i != 0 ) {
                    int add_cost = 0;
                    for(auto row_group : cand.second.back()) for(auto &&[x1, y1, x2, y2]: row_group) {
                        for(auto next_row_group : place) for(auto &&[nx1, ny1, nx2, ny2]: next_row_group) {
                            if (y1 != ny1 || y2 != ny2) add_cost += height;
                        }
                    }
                    cand.first += add_cost;
                }
                cand.second.emplace_back(place);
            }
            if (!flag) continue;
            pq.push(cand);
        }
        return;
    }

    inline int calcScore() {
        int res = 0;
        // line から分割された面積を計算
        vector<int> area_size;
        reps(i,1,line_row.size()-1) {
            reps(j,1,line_col.size()-1) {
                area_size.emplace_back((line_row[i]-line_row[i-1]) * (line_col[j]-line_col[j-1]));
            }
        }
        sort(area_size.begin(), area_size.end());
        vector<bool> used(area_size.size(), false);
        return 0;
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