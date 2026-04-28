#include <bits/stdc++.h>
using namespace std;
#if __has_include(<atcoder/all>)
    #include <atcoder/all>
using namespace atcoder;
#endif
typedef long long ll;
typedef pair<int, int> P;
typedef tuple<int, int, int, int> T;
typedef tuple<int, int, int, int, int> T2;
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
#define TIME_LIMIT 1950
inline double temp(int start) {
    double start_temp = 100,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

#define BEAM_WIDTH 100

vector<int> dx = {-1,-1, 0, 0, 1, 1};
vector<int> dy = {-1, 0,-1, 1, 0, 1};
vector<int> row = {1,3,6,10,15,21,28,36,45,55,66,78,91,105,120,136,153,171,190,210,231,253,276,300,325,351,378,406,435,465};

struct State{
    int n, px1, py1, px2, py2, parent1, parent2;
    vector<vector<int>> ball;
    vector<T> operation;
    vector<P> place;

    State() : n(30){ }
    State(vector<vector<int>> _ball){
        n = 30;
        ball = _ball;
        place.assign(n*(n+1)/2,P(-1,-1));
        int cnt = 0;
        rep(i,n){
            for(ll j=0;j<=i;j++){
                place[ball[i][j]] = P(i,j);
                cnt++;
            }
        }
    }

    void solve(){
        vector<vector<int>> visited;
        vector<vector<P>> pre;
        priority_queue<T2,vector<T2>,greater<T2>> todo;
        stack<T> st;

        // 解生成
        rep(i,n*(n+1)/2){
            auto [x,y] = place[i];
            int lim_r = lower_bound(row.begin(),row.end(),i+1) - row.begin();
            int lim_c = lim_r;

            // dijkstraで最短経路復元
            visited.assign(n,vector<int>(n,1e9));
            pre.assign(n,vector<P>(n,P(-1,-1)));
            todo.push(T2( n*(n+1)/2 - i,x,y,-1,-1));

            while(!todo.empty()){
                auto [cost,now_x,now_y,pre_x,pre_y] = todo.top(); todo.pop();
                if(visited[now_x][now_y] <= cost)continue;
                if( ball[now_x][now_y] < i )continue;

                visited[now_x][now_y] = cost;
                pre[now_x][now_y] = P(pre_x,pre_y);

                rep(j,2){
                    px1 = now_x + dx[j];
                    py1 = now_y + dy[j];
                    if(outField(px1,py1) || visited[px1][py1] <= cost+ n*(n+1)/2 - ball[px1][py1])continue;
                    todo.push(T2(cost + n*(n+1)/2 - ball[px1][py1] + rand_int()%10 ,px1,py1,now_x,now_y));
                }
            }

            int cand_x = lim_r, cand_y = 0, min_cost = 1e9;
            rep(j,n){
                for(int k=0;k<=j;k++){
                    if(ball[j][k] < i)continue;
                    bool f = true;
                    rep(l,2){
                        if(l == 2 || l == 3)continue;
                        px1 = j + dx[l];
                        py1 = k + dy[l];
                        if( outField(px1,py1) )continue;
                        f &= (ball[px1][py1] <= i);
                    }
                    if(!f)continue;

                    if(visited[j][k] < min_cost){
                        min_cost = visited[j][k];
                        cand_x = j;
                        cand_y = k;
                    }
                }
            }

            while(true){
                auto [pre_x,pre_y] = pre[cand_x][cand_y];
                if( pre_x == -1 || pre_y == -1 )break;
                st.push(T(cand_x,cand_y,pre_x,pre_y));
                cand_x = pre_x;
                cand_y = pre_y;
            }
            while(!st.empty()){
                auto [x1,y1,x2,y2] = st.top();
                operation.push_back(st.top());
                swap(place[ball[x1][y1]],place[ball[x2][y2]]);
                swap(ball[x1][y1],ball[x2][y2]);
                st.pop();
            }
        }
    }

    inline bool outField(int i, int j){
        return ( j < 0 || i+1 <= j || i < 0 || 30 <= i );
    }

    bool operator>(const State& other) const {
        return operation.size() > other.operation.size();
    }
};

struct Solver{
    int n;
    State initial_field;
    priority_queue<State,vector<State>,greater<State>> beam, next_beam;

    Solver(){
        n = 30;
    }

    void input(){
        vector<vector<int>> ball(n,vector<int>(n,-1));
        rep(i,30){
            for(ll j=0;j<=i;j++){
                cin >> ball[i][j];
            }
        }
        initial_field = State(ball);
    }

    void output(){
        State answer = beam.top();
        cout << answer.operation.size() << endl;
        rep(i,answer.operation.size()){
            auto [x,y,z,w] = answer.operation[i];
            cout << x << " " << y << " " << z << " " << w << endl;
        }
    }

    void solve(){
        utility::mytm.CodeStart();
        double total_turn = n*(n+1)/2;

        beam.push(initial_field);

        for(ll turn=1;turn<=total_turn;turn++){
            double end_time = TIME_LIMIT * turn / total_turn;
            ll cnt = 0;

            // beam search
            while( utility::mytm.elapsed() <= end_time && !beam.empty() ){
                State candidate = beam.top();
                candidate.solve();
                next_beam.push(candidate);
                cnt++;
                if(cnt%20 == 0)beam.pop();
            }

            while( next_beam.size() > BEAM_WIDTH )next_beam.pop();
            swap( beam, next_beam );
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