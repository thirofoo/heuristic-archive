/*****************************************************************
 *  compile : g++ -O2 -std=c++17 -march=native main.cpp
 *****************************************************************/
#include <bits/stdc++.h>
using namespace std;
using ll = long long;

/***  ----------  問題サイズ＆定数  ---------- ***/
constexpr int N   = 20;             // 盤面一辺
constexpr int NN  = N*N;
constexpr int H   = 20;             // 1 trip の最大積載
constexpr ll  NEG = -1;

/***  ----------  入力バッファ  ---------- ***/
int  W[NN];
ll   D[NN];

/***  ----------  乱数 ---------- ***/
struct RNG{
    uint64_t s[2];
    RNG(uint64_t seed=1234567){reset(seed);}
    inline uint64_t nxt(){
        uint64_t x=s[0], y=s[1];
        s[0]=y;
        x^=x<<23; x^=x>>17; x^=y^(y>>26);
        s[1]=x;
        return x+y;
    }
    void reset(uint64_t sd){
        auto f=[&](){
            sd += 0x9e3779b97f4a7c15ULL;
            uint64_t z=sd;
            z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
            z=(z^(z>>27))*0x94d049bb133111ebULL;
            return z^(z>>31);
        };
        s[0]=f(); s[1]=f();
    }
    int randint(int r){ return int(nxt()%r); }
    template<class T> void shuffle(vector<T>& v){
        for(int i=v.size()-1;i>0;--i){ int j=randint(i+1); swap(v[i],v[j]); }
    }
} rng(20240621);

/***  ----------  ユーティリティ  ---------- ***/
inline int mdist(int a,int b){ return abs(a/N-b/N)+abs(a%N-b%N);}

/***  ----------  Greedy + DP で 1 trip を決める ---------- ***/
struct Trip{ int base; vector<int> seq; };             // base + LU順セル

struct DPRes{
    ll score;                  // Σ w*(i+j)
    vector<char> take;         // 取るフラグ
    bool ok;
};

DPRes solvePath(int base,
                const vector<int>& cells,
                const vector<char>& moves,
                const vector<char>& alive)
{
    const int L=cells.size();
    static ll      dp[101][H+1];
    static uint8_t prv[101][H+1], tk[101][H+1];

    for(int i=0;i<=L;i++) for(int h=0;h<=H;h++) dp[i][h]=NEG;

    dp[0][1]=D[base];                           // 基台だけ積載

    for(int idx=0; idx<L; ++idx){
        int v = cells[idx];
        bool has = alive[v];
        int  w = W[v]; ll d = D[v];
        int rem = L - idx - 1;

        for(int h=1; h<=H; ++h) if(dp[idx][h]!=NEG){
            ll m = dp[idx][h];
            // skip
            if(m > dp[idx+1][h]){
                dp[idx+1][h]=m; prv[idx+1][h]=h; tk[idx+1][h]=0;
            }
            // take
            if(has && h<H && 1LL*w*(rem+1)<=m){
                ll nm = min<ll>(m - 1LL*w*(rem+1), d);
                if(nm>=0 && nm > dp[idx+1][h+1]){
                    dp[idx+1][h+1]=nm;
                    prv[idx+1][h+1]=h; tk[idx+1][h+1]=1;
                }
            }
        }
    }

    int hsel=0; for(int h=H; h>=1; --h) if(dp[L][h]!=NEG){ hsel=h; break; }
    if(!hsel) return {0,{},false};

    vector<char> take(L,0);
    ll score = 1LL * W[base] * (base/N + base%N);
    int h=hsel;
    for(int idx=L; idx>0; --idx){
        if(tk[idx][h]){
            take[idx-1]=1;
            int v=cells[idx-1];
            score += 1LL*W[v]*(v/N+v%N);
        }
        h=prv[idx][h];
    }
    return {score, move(take), true};
}

/***  ----------  Greedy 初期解 ---------- ***/
vector<Trip> buildGreedy(vector<char>& ops){
    vector<char> alive(NN,1); alive[0]=0;
    vector<Trip> trips;
    int cx=0,cy=0;
    auto mv=[&](char c){ if(c=='U')--cx;else if(c=='L')--cy;else if(c=='D')++cx;else ++cy; ops.push_back(c);};

    while(true){
        /* frontier 最大 D を基台に */
        int bi=-1,bj=-1; ll bd=-1;
        for(int i=0;i<N;i++)for(int j=0;j<N;j++){
            int v=i*N+j; if(!alive[v]) continue;
            bool fr=true;
            for(int x=i;x<N&&fr;x++)for(int y=j;y<N;y++)
                if(alive[x*N+y]&&(x>i||y>j)) fr=false;
            if(fr && D[v]>bd){ bd=D[v]; bi=i; bj=j; }
        }
        if(bi==-1) break;

        /* base へ移動して pick */
        while(cx<bi) mv('D'); while(cy<bj) mv('R');
        ops.push_back('1'); alive[bi*N+bj]=0;
        int base = bi*N+bj;

        /* ランダム最短経路集合 */
        vector<vector<char>> pathMoves;
        vector<char> baseLU(bj,'L'); baseLU.insert(baseLU.end(),bi,'U');
        pathMoves.push_back(baseLU);
        vector<char> rev=baseLU; reverse(rev.begin(),rev.end()); pathMoves.push_back(rev);
        while((int)pathMoves.size()<1000){ auto v=baseLU; rng.shuffle(v); pathMoves.push_back(move(v)); }

        /* best path by DP */
        ll bestSc=-1; vector<int> bestCells; vector<char> bestTake;
        for(auto& mvSeq: pathMoves){
            vector<int> cells; cells.reserve(bi+bj);
            int x=bi,y=bj;
            for(char c:mvSeq){ if(c=='L')--y; else --x; cells.push_back(x*N+y); }
            auto res=solvePath(base,cells,mvSeq,alive);
            if(res.ok && res.score>bestSc){
                bestSc=res.score; bestCells=cells; bestTake=res.take;
            }
        }
        int x=bi,y=bj;
        for(size_t k=0;k<bestCells.size();k++){
            char c = (bestCells[k]==x*N+y-1)?'L':'U';
            mv(c); x=bestCells[k]/N; y=bestCells[k]%N;
            if(bestTake[k]){ ops.push_back('1'); alive[bestCells[k]]=0; }
        }
        cx=cy=0;
        trips.push_back({base,bestCells});
    }
    return trips;
}

/***  ----------  スコア ---------- ***/
inline ll tripScore(const Trip& t){
    ll s = 1LL*W[t.base]*(t.base/N+t.base%N);
    for(int v:t.seq) s += 1LL*W[v]*(v/N+v%N);
    return s;
}
inline ll totalScore(const vector<Trip>& v){
    ll s=0; for(auto& t:v) s+=tripScore(t); return s;
}

/***  ----------  SA 状態 ---------- ***/
struct State{
    vector<Trip> trips; ll sc;
};

/* 近傍１：2 trip で箱交換 */
void nbSwap(State& st,double temp){
    if(st.trips.size()<2) return;
    int a=rng.randint(st.trips.size()), b=rng.randint(st.trips.size());
    if(a==b || st.trips[a].seq.empty() || st.trips[b].seq.empty()) return;
    int ia=rng.randint(st.trips[a].seq.size()), ib=rng.randint(st.trips[b].seq.size());
    swap(st.trips[a].seq[ia], st.trips[b].seq[ib]);
    ll ns = totalScore(st.trips);
    ll diff = ns - st.sc;
    if(diff>0 || exp(diff/temp) > (double)rng.nxt()/double(UINT64_MAX)) st.sc=ns;
    else swap(st.trips[a].seq[ia], st.trips[b].seq[ib]);
}

/* 近傍２：1 箱を別 trip へ移動 */
void nbMove(State& st,double temp){
    if(st.trips.size()<2) return;
    int a=rng.randint(st.trips.size()), b=rng.randint(st.trips.size());
    if(a==b || st.trips[a].seq.empty()) return;
    int ia=rng.randint(st.trips[a].seq.size());
    int v = st.trips[a].seq[ia];
    st.trips[a].seq.erase(st.trips[a].seq.begin()+ia);
    int pos = rng.randint(st.trips[b].seq.size()+1);
    st.trips[b].seq.insert(st.trips[b].seq.begin()+pos,v);
    ll ns = totalScore(st.trips);
    ll diff=ns - st.sc;
    if(diff>0 || exp(diff/temp) > (double)rng.nxt()/double(UINT64_MAX)) st.sc=ns;
    else{
        st.trips[b].seq.erase(st.trips[b].seq.begin()+pos);
        st.trips[a].seq.insert(st.trips[a].seq.begin()+ia, v);
    }
}

/***  ----------  コマンド再構築 ---------- ***/
void emitTrip(const Trip& t, vector<char>& ops,int& x,int& y){
    int bx=t.base/N, by=t.base%N;
    while(x<bx){ops.push_back('D');++x;}
    while(y<by){ops.push_back('R');++y;}
    ops.push_back('1');
    /* LU のみで帰る：列 Y→0, 行 X→0 */
    vector<int> seq=t.seq;
    sort(seq.begin(),seq.end(),[](int a,int b){
        if(a%N!=b%N) return a%N>b%N;
        return a/N>b/N;
    });
    for(int v:seq){
        int vx=v/N, vy=v%N;
        while(y>vy){ops.push_back('L');--y;}
        while(x>vx){ops.push_back('U');--x;}
        ops.push_back('1');
    }
    while(y>0){ops.push_back('L');--y;}
    while(x>0){ops.push_back('U');--x;}
}

/***  ----------  main ---------- ***/
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    /*---- 入力 ----*/
    int n; if(!(cin>>n)) return 0;
    for(int i=0;i<N;i++)for(int j=0;j<N;j++) cin>>W[i*N+j];
    for(int i=0;i<N;i++)for(int j=0;j<N;j++) cin>>D[i*N+j];

    /*---- Greedy 初期解 ----*/
    vector<char> dummy;
    vector<Trip> initTrips = buildGreedy(dummy);
    State cur{initTrips, totalScore(initTrips)}, best=cur;

    /*---- 焼きなまし ----*/
    const double TL=1.5;
    const double T0=5e2, T1=1e-2;
    auto t0 = chrono::high_resolution_clock::now();
    for(int iter=0;;iter++){
        double t = chrono::duration<double>(chrono::high_resolution_clock::now()-t0).count();
        if(t>TL) break;
        double temp = T0*pow(T1/T0, t/TL);
        if(rng.randint(2)) nbSwap(cur,temp); else nbMove(cur,temp);
        if(cur.sc > best.sc) best=cur;
    }

    /*---- コマンド出力 ----*/
    vector<char> ops; int x=0,y=0;
    for(auto& tr:best.trips) emitTrip(tr,ops,x,y);
    for(char c:ops) cout<<c<<'\n';
    return 0;
}
