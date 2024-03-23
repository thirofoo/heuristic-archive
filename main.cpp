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

struct Answer {
    int score;
    vector<vector<vector<T>>> place;
    // place[day][row][idx] = {x1, y1, x2, y2}
    // day 日目, row 行目, idx 番目の区間
    Answer(const int score, const vector<vector<vector<T>>> &place) : score(score), place(place) {}

    inline bool operator<(const Answer &a) const { return score < a.score; }
    inline bool operator>(const Answer &a) const { return score > a.score; }
};

struct Solver{
    int w, d, n;
    vector<vector<int>> area;
    vector<int> line_row, line_col;
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
        if( pq.empty() ) {
            cerr << "No Answer" << endl;
            return;
        }
        Answer best = pq.top();
        for(auto &&day_place: best.place) {
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
        // 初期貪欲解
        // 1. X 行に分割
        // 2. 各日に合わせて行に列の柵を追加
        // ※ これが出来れば最低でも ( 行分割の数 ) * W * D のコストで可能

        int max_ele = 0;
        rep(i,d) max_ele = max(max_ele, area[i].back());
        vector<int> max_area_per_rank(n, 0);
        rep(i,d) rep(j,n) max_area_per_rank[j] = max(max_area_per_rank[j], area[i][j]);

        reps(X,1,w+1) {
            // 1. 以下 2 つを満たす行分割の数 (X) を決定 ( height = (int)w / X )
            // height * w >= max_ele ( 最大要素が 1 行に収まるか )
            // max_{i}( ∑_{j} ( ((area[i][j]-1) / height) + 1 ) * X ) <= w^2 ( 各日のスペースが全て収まりそうか )
            int height = w / X;
            if (height * w < max_ele) break;

            int day_max_cnt = 0;
            rep(i,d) {
                int cnt = 0;
                rep(j,n) cnt += ((area[i][j]-1) / height) + 1;
                day_max_cnt = max(day_max_cnt, cnt);
            }
            if (day_max_cnt > w*w) continue;

            // 2. 列の柵を立てる
            Answer cand = Answer(0, {});
            bool flag = true;
            rep(i,d) {
                priority_queue<P> row; // {空列, 行番号}
                vector<vector<T>> place(X);
                rep(i,X) row.push({w, i});

                // 大きい陣地から空きが多い行に割り当て
                // ※ 覆いきれない場合は残りの空列を全て利用
                for(int j=n-1; j>=0; j--) {
                    if( row.empty() ) {
                        flag = false;
                        break;
                    }
                    auto [capasity, idx] = row.top(); row.pop();
                    int need = ((area[i][j]-1) / height) + 1;
                    place[idx].emplace_back(T(idx*height, w-capasity, (idx+1)*height, min(w-capasity+need, w)));
                    if( capasity > need ) row.push(P(capasity-need, idx));
                    else cand.score += (need - capasity) * height * 100; // space 不足 cost
                }
                if( !flag ) break;
                if( i != 0 ) {
                    // 柵移動 cost
                    for(auto row_group : cand.place.back()) for(auto &&[x1, y1, x2, y2]: row_group) {
                        for(auto next_row_group : place) for(auto &&[nx1, ny1, nx2, ny2]: next_row_group) {
                            if (y1 != ny1 || y2 != ny2) cand.score += height;
                        }
                    }
                }
                cand.place.emplace_back(place);
            }
            if( !flag ) continue;
            pq.push(cand);
        }
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