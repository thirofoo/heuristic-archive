#include <bits/stdc++.h>
using namespace std;
#if __has_include(<atcoder/all>)
    #include <atcoder/all>
using namespace atcoder;
#endif
typedef long long ll;
typedef long double ld;
typedef pair<ll, ll> P;
typedef pair<ld, ll> P2;
typedef tuple<int, int, ll> T;
typedef tuple<ll, int, int, int> T2;
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
#define TIME_LIMIT 1900
inline double temp(int start) {
    double start_temp = 0,end_temp = 0;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    if(now >= best)return 1;
    return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

struct Solver{
    // State内で用いる変数等の定義
    int n, m, k, place, query, change_edge, turn = 0, start_time;
    ll ele_power, sum;
    ld best_score, cand_score, score, num, now_power, temp, rnd_d;
    vector<P> broad, residents;
    vector<T> edge;
    vector<ll> power;

    // 各放送局が網羅してる住民の P(距離^2, idx)
    vector<priority_queue<P2>> include;
    queue<P2> store;
    // 各住民が何個の放送局と繋がってるか
    vector<ll> res_cnt, now_res_cnt;
    vector<bool> onoff, available, now_available;
    vector<vector<ll>> Graph;

    Solver(){}

    void input(){
        cin >> n >> m >> k;
        Graph.assign(n,{});
        rep(i,n){
            int x,y; cin >> x >> y;
            broad.push_back(P(x,y));
        }
        rep(i,m){
            int u,v,w; cin >> u >> v >> w;
            u--; v--;
            edge.push_back(T(u,v,w));
            Graph[u].push_back(v);
            Graph[v].push_back(u);
        }
        rep(i,k){
            int a,b; cin >> a >> b;
            residents.push_back(P(a,b));
        }
        power.assign(n,0);
        onoff.assign(m,true);
        include.assign(n,{});
        res_cnt.assign(k,0);
        available.assign(n,true);
    }

    void output(){
        rep(i,n)cout << power[i] << " ";
        cout << endl;
        rep(i,m)cout << onoff[i] << " ";
        cout << endl;
    }

    void solve(){
        utility::mytm.CodeStart();
        start_time = utility::mytm.elapsed();

        // 最小全域木作成part
        priority_queue<T2,vector<T2>,greater<T2>> todo;
        rep(i,m){
            auto &&[u,v,w] = edge[i];
            todo.push(T2(w,u,v,i));
        }
        dsu uf(n);
        while(!todo.empty()){
            auto [w,u,v,i] = todo.top(); todo.pop();
            if(uf.same(u,v)){
                onoff[i] = false;
                continue;
            }
            uf.merge(u,v);
        }

        // 初期解生成 ( 各住民において一番近い放送局のpowerを伸ばす )
        sum = 0;
        num = k;
        rep(i,k){
            ll min_dis = 1e9, idx = 0;
            auto [x1,y1] = residents[i];
            rep(j,n){
                auto [x2,y2] = broad[j];
                ll dis = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
                if(dis < min_dis){
                    min_dis = dis;
                    idx = j;
                }
            }
            power[idx] = max(power[idx], min_dis+1);
        }
        rep(i,n){
            auto [x,y] = broad[i];
            sum += power[i]*power[i];
            rep(j,k){
                auto [a,b] = residents[j];
                if( (a-x)*(a-x) + (b-y)*(b-y) <= power[i]*power[i] ){
                    include[i].push(P2((a-x)*(a-x) + (b-y)*(b-y), j));
                    res_cnt[j]++;
                }
            }
        }
        rep(i,m){
            if(onoff[i]){
                auto &&[u,v,w] = edge[i];
                sum += w;
            }
        }
        best_score = calcScore(sum);

        // 山登り法
        while(utility::mytm.elapsed() <= TIME_LIMIT){

            // 1, 部分破壊 & 再構築
            // 2. power変更
            query = rand_int()%10;
            if(query == 0){
                // 部分破壊part
                // 1. 乱択でpower0
                // 2. そこから連結の頂点のいずれかを広げて調整していく
                place = rand_int()%n;
                ll pre_power = power[place];
                ll next_power = 0;
                
                sum -= pre_power * pre_power;
                sum += next_power * next_power;
                power[place] = next_power;
                queue<P2> tmp;
                vector<ll> no_cover;
                while(!include[place].empty()){
                    auto [distance, idx] = include[place].top();
                    res_cnt[idx]--;
                    tmp.push(include[place].top());
                    include[place].pop();
                    if(res_cnt[idx] == 0)no_cover.push_back(idx);
                }

                if(no_cover.size() == 0){
                    // power0にしても問題ないなら、それで次に行く
                    while(!tmp.empty())tmp.pop();
                    while(!no_cover.empty())no_cover.pop_back();
                    continue;
                }

                ll alter = rand_int()%n;
                ll max_dis = power[alter];
                auto &&[x1,y1] = broad[alter];
                rep(i,no_cover.size()){
                    auto &&[x2,y2] = residents[no_cover[i]];
                    if( max_dis < sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) ) + 1 ){
                        max_dis = sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) ) + 1;
                    }
                }
                ll alter_pre_power = power[alter];
                sum -= alter_pre_power * alter_pre_power;
                sum += max_dis * max_dis;
                power[alter] = max_dis;

                cand_score = calcScore(sum);
                temp = prob(best_score,cand_score,start_time);
                rnd_d = rand_double();
                if(rnd_d < temp && max_dis <= 5000){
                    // cerr << pre_power*pre_power << " " << d_power*d_power << endl;
                    // cerr << sum << endl;
                    best_score = cand_score;
                    while(!tmp.empty())tmp.pop();
                    auto &&[x1,y1] = broad[alter];
                    while(!no_cover.empty()){
                        auto &&[x2,y2] = residents[no_cover.back()];
                        include[alter].push(P2( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2), no_cover.back() ));
                        res_cnt[no_cover.back()]++;
                        no_cover.pop_back();
                    }
                    cerr << (ll)best_score << endl;
                    cerr << flush;
                    output();
                }
                else{
                    sum += pre_power * pre_power;
                    sum -= next_power * next_power;
                    sum += alter_pre_power * alter_pre_power;
                    sum -= max_dis * max_dis;
                    power[alter] = alter_pre_power;
                    power[place] = pre_power;
                    while(!tmp.empty()){
                        auto [distance, idx] = tmp.front();
                        include[place].push(tmp.front());
                        res_cnt[idx]++;
                        tmp.pop();
                    }
                    while(!no_cover.empty())no_cover.pop_back();
                }
            }
            else{
                // power調整 part
                place = rand_int()%n;
                ele_power = min(5000ll, max(0ll, power[place] + 100 - (rand_int()%201)));
                // ele_power = rand_int()%5001;
                swap(power[place], ele_power);

                cand_score = calcDiff1(place, ele_power);
                temp = prob(best_score,cand_score,start_time);
                rnd_d = rand_double();

                if(rnd_d < temp){
                    // if(temp < 1 && abs(best_score-cand_score) > 100 ){
                    //     cerr << (ll)best_score << " " << (ll)cand_score << endl;
                    //     cerr << temp << " " << rnd_d << endl;
                    //     cerr << flush;
                    // }

                    best_score = cand_score;
                    cerr << (ll)best_score << endl;
                    cerr << flush;
                    while(!store.empty())store.pop();
                }
                else{
                    sum += ele_power * ele_power;
                    sum -= power[place] * power[place];
                    swap(power[place], ele_power);
                    while(!store.empty()){
                        include[place].push(store.front());
                        res_cnt[store.front().second]++;
                        store.pop();
                    }
                    num = k;
                }
            }
            turn++;
            // if(turn%10000 == 0)output();
        }
        cerr << turn << endl;
        // rep(i,k)if(res_cnt[i] <= 0)cerr << "fuck!" << endl;

        // power = 0 の頂点を繋げる必要は無い → 各edgeを消してpower > 0 が連結なら消す
        rep(i,m){
            if(!onoff[i])continue;
            dsu uf(n);
            rep(j,m){
                if(!onoff[j] | i == j)continue;
                auto &&[u,v,w] = edge[j];
                uf.merge(u,v);
            }
            bool f = true;
            rep(j,n){
                if(power[j] == 0)continue;
                f &= uf.same(0,j);
            }
            onoff[i] = !f;
        }
    }
    
    ld calcScore(ll total){
        score = (ld)1.0 + (ld)100000000.0 / ( (ld)total + (ld)10000000.0 );
        score *= (ld)1000000.0;
        return score;
    }

    ld calcDiff1(ll idx, ll pre_power){
        now_power = power[idx] * power[idx];
        sum -= pre_power * pre_power;
        sum += now_power;

        while(!include[idx].empty()){
            auto [dis, i] = include[idx].top();
            if(dis <= now_power)break;
            store.push(include[idx].top());
            include[idx].pop();
            res_cnt[i]--;
            if(res_cnt[i] == 0)num--;
        }

        score = 1.0 + 100000000.0 / ( sum + 10000000.0 );
        score *= 1000000.0;
        return (num == k ? score : -1 );
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