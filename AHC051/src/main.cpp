#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;

#define rep(i,n) for(int i=0;i<(n);++i)

/*** timer & rng ***/
namespace utility {
  struct timer {
    chrono::system_clock::time_point st;
    void CodeStart(){ st = chrono::system_clock::now(); }
    double elapsed() const {
      using namespace std::chrono;
      return (double)chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now()-st).count();
    }
  } mytm;
}
inline unsigned int rand_int(){
  static unsigned int tx=123456789,ty=362436069,tz=521288629,tw=88675123;
  unsigned int tt=(tx^(tx<<11)); tx=ty; ty=tz; tz=tw;
  return (tw=(tw^(tw>>19))^(tt^(tt>>8)));
}
inline double rand_double(){ return (double)(rand_int()%1000000000)/1000000000.0; }

/*** budgets ***/
#define BUDGET_BUILD   1150.0
#define BUDGET_SA       750.0

/*** Hungarian (maximize) ***/
struct Hungarian {
  static vector<int> solve(const vector<vector<double>>& W){
    int n=(int)W.size();
    if(n==0) return {};
    double mx=0; rep(i,n) rep(j,n) mx=max(mx, W[i][j]);
    vector<vector<double>> a(n, vector<double>(n));
    rep(i,n) rep(j,n) a[i][j]=mx-W[i][j];

    const double INF=1e100;
    vector<double> u(n+1),v(n+1);
    vector<int> p(n+1),way(n+1);
    for(int i=1;i<=n;i++){
      p[0]=i;
      int j0=0;
      vector<double> minv(n+1, INF);
      vector<char> used(n+1,false);
      do{
        used[j0]=true;
        int i0=p[j0], j1=0;
        double delta=INF;
        for(int j=1;j<=n;j++) if(!used[j]){
          double cur=a[i0-1][j-1]-u[i0]-v[j];
          if(cur<minv[j]){ minv[j]=cur; way[j]=j0; }
          if(minv[j]<delta){ delta=minv[j]; j1=j; }
        }
        for(int j=0;j<=n;j++){
          if(used[j]){ u[p[j]]+=delta; v[j]-=delta; }
          else minv[j]-=delta;
        }
        j0=j1;
      }while(p[j0]!=0);
      do{
        int j1=way[j0];
        p[j0]=p[j1];
        j0=j1;
      }while(j0);
    }
    vector<int> ans(n,-1);
    for(int j=1;j<=n;j++) if(p[j]!=0) ans[p[j]-1]=j-1;
    return ans;
  }
};

/*** geometry (for completeness / tie-break, 交差は理論上起きない構成) ***/
struct Pt{ long long x,y; int id; };
static inline long long dist2(const Pt&a,const Pt&b){
  long long dx=a.x-b.x, dy=a.y-b.y; return dx*dx+dy*dy;
}

/*** solver ***/
struct Solver{
  // input
  int N,M,K;
  vector<Pt> procs;          // id: 0..N-1
  vector<Pt> sorts;          // id: N..N+M-1
  Pt inlet;                  // id: -1
  vector<vector<double>> P;  // P[k][j]

  // graph (出力用)
  struct Out{ int k,v1,v2; bool used; };
  vector<Out> out;           // size M
  int inlet_conn;            // node id

  // Pascal layers
  int L=0;
  vector<vector<int>> layer; // sorter index (0..M-1), layers 1..L
  vector<int> topoSorters;   // layer順

  Solver(){ input(); }

  void input(){
    cin>>N>>M>>K;
    procs.resize(N);
    rep(i,N){ long long x,y; cin>>x>>y; procs[i]={x,y,i}; }
    sorts.resize(M);
    rep(i,M){ long long x,y; cin>>x>>y; sorts[i]={x,y,N+i}; }
    P.assign(K, vector<double>(N,0.0));
    rep(k,K) rep(j,N) cin>>P[k][j];
    inlet={0,5000,-1};
  }

  /*** Pascal triangle DAG (交差0 & 両出口必ず設定) ***/
  void build_pascal_dag(){
    utility::mytm.CodeStart();

    // choose L that maximizes distinct processors on right (tie -> shorter internal edges)
    int Lmax=0; while(1LL*(Lmax+1)*(Lmax+2)/2<=M) ++Lmax; --Lmax; Lmax=max(Lmax,1);

    vector<int> byX(M); iota(byX.begin(),byX.end(),0);
    sort(byX.begin(),byX.end(),[&](int a,int b){ return sorts[a].x < sorts[b].x; });

    long long bestDistinct=-1; double bestLen=1e100; int bestL=1;
    vector<vector<int>> bestLayer;

    for(int cand=1; cand<=Lmax; ++cand){
      int ptr=0; vector<vector<int>> ly(cand+1);
      for(int t=1;t<=cand;++t){
        vector<int> v; v.reserve(t); rep(i,t) v.push_back(byX[ptr++]);
        sort(v.begin(),v.end(),[&](int a,int b){ return sorts[a].y < sorts[b].y; });
        ly[t]=v;
      }
      int lastX = INT_MIN; for(int id:ly[cand]) lastX=max(lastX,(int)sorts[id].x);
      vector<int> rightP; rep(i,N) if(procs[i].x>lastX) rightP.push_back(i);
      if(rightP.empty()){ int r=0; rep(i,N) if(procs[i].x>procs[r].x) r=i; rightP.push_back(r); }
      sort(rightP.begin(),rightP.end(),[&](int a,int b){ return procs[a].y<procs[b].y; });
      int distinct = min((int)rightP.size(), 2*(int)ly[cand].size());

      double len=0; long long E=0;
      for(int t=2;t<=cand;++t){
        auto &A=ly[t-1], &B=ly[t];
        for(int s=0;s<(int)A.size();++s){
          Pt u=sorts[A[s]], l=sorts[B[s]], h=sorts[B[s+1]];
          len += hypot((double)u.x-l.x,(double)u.y-l.y); ++E;
          len += hypot((double)u.x-h.x,(double)u.y-h.y); ++E;
        }
      }
      if(E) len/=E;

      if(distinct>bestDistinct || (distinct==bestDistinct && len<bestLen)){
        bestDistinct=distinct; bestLen=len; bestL=cand; bestLayer=ly;
      }
    }

    L=bestL; layer=move(bestLayer);
    out.assign(M, Out{0,-1,-1,false});

    // internal edges：各層 y昇順、s→(s,s+1)
    for(int t=2;t<=L;++t){
      auto &A=layer[t-1], &B=layer[t];
      for(int s=0;s<(int)A.size();++s){
        int u=A[s];
        out[u].used=true; out[u].k=0;
        out[u].v2 = N + B[s];     // 下
        out[u].v1 = N + B[s+1];   // 上
      }
    }
    // inlet → root
    inlet_conn = N + layer[1][0];

    // leaves → processors（右側のみ、y順に単調対応 (2s,2s+1)）
    int lastX = INT_MIN; for(int id:layer[L]) lastX=max(lastX,(int)sorts[id].x);
    vector<int> rightP; rep(i,N) if(procs[i].x>lastX) rightP.push_back(i);
    if(rightP.empty()){ int r=0; rep(i,N) if(procs[i].x>procs[r].x) r=i; rightP.push_back(r); }
    sort(rightP.begin(),rightP.end(),[&](int a,int b){ return procs[a].y<procs[b].y; });

    auto &leaf = layer[L]; // y昇順
    int R=(int)rightP.size();
    for(int s=0;s<(int)leaf.size();++s){
      int u=leaf[s];
      out[u].used=true; // k はあとで焼く
      int j0=min(2*s,   R-1);
      int j1=min(2*s+1, R-1);
      int p0=rightP[j0], p1=rightP[j1];
      out[u].v2 = p0;
      out[u].v1 = p1;
    }

    // topo list（層順）
    topoSorters.clear();
    for(int t=1;t<=L;++t) for(int id:layer[t]) if(out[id].used) topoSorters.push_back(id);
  }

  /*** フロー伝播 → agg[p][type] ***/
  void propagate(vector<vector<double>>& agg) const {
    agg.assign(N, vector<double>(N,0.0));
    auto push = [&](int id, int ty, double m, auto&& self)->void{
      if(m<=0) return;
      if(0<=id && id<N){ agg[id][ty]+=m; return; }
      int sid=id-N; if(!(0<=sid && sid<M) || !out[sid].used) return;
      if(out[sid].v1==out[sid].v2){
        self(out[sid].v1, ty, m, self);
      }else{
        double p1 = P[out[sid].k][ty];
        self(out[sid].v1, ty, m*p1, self);
        self(out[sid].v2, ty, m*(1.0-p1), self);
      }
    };
    // 全種類均等スタート（1/N でも 1 でも比率一定なので任意）
    rep(ty,N) push(inlet_conn, ty, 1.0, push);
  }

  /*** 割当（greedy / Hungarian） ***/
  static double assign_greedy(const vector<vector<double>>& W, vector<int>& perm){
    int n=(int)W.size(); perm.assign(n,-1);
    vector<tuple<double,int,int>> vec; vec.reserve(n*n);
    rep(i,n) rep(j,n) vec.emplace_back(W[i][j], i, j);
    sort(vec.begin(), vec.end(), [](auto&a,auto&b){ return get<0>(a)>get<0>(b); });
    vector<char> r(n,false), c(n,false);
    double s=0; int cnt=0;
    for(auto&t:vec){
      if(cnt==n) break;
      auto [w,i,j]=t; if(r[i]||c[j]) continue;
      r[i]=c[j]=true; perm[i]=j; s+=w; ++cnt;
    }
    rep(i,n) if(perm[i]<0){ rep(j,n) if(!c[j]){ perm[i]=j; c[j]=true; break; } }
    return s;
  }
  static double assign_hungarian(const vector<vector<double>>& W, vector<int>& perm){
    perm = Hungarian::solve(W);
    double s=0; rep(i,(int)W.size()) if(perm[i]>=0) s+=W[i][perm[i]];
    return s;
  }

  /*** 評価 ***/
  double eval_greedy(vector<int>& perm) const {
    vector<vector<double>> agg; propagate(agg);
    return assign_greedy(agg, perm);
  }
  double eval_exact(vector<int>& perm) const {
    vector<vector<double>> agg; propagate(agg);
    return assign_hungarian(agg, perm);
  }

  /*** “散らばり最大” のテスト種類（初期値） ***/
  int best_classifier_type() const {
    double best=-1; int arg=0;
    rep(k,K){
      double mn=1, mx=0; rep(t,N){ mn=min(mn, P[k][t]); mx=max(mx, P[k][t]); }
      if(mx-mn>best){ best=mx-mn; arg=k; }
    }
    return arg;
  }

  /*** 焼きなまし（あなたの枠：分岐ノードのタイプだけ変更） ***/
  void anneal_branch_types(){
    // 初期タイプ：散らばり最大
    int initK = best_classifier_type();
    rep(i,M) if(out[i].used) out[i].k = initK;

    vector<int> branch;
    rep(i,M) if(out[i].used && out[i].v1!=out[i].v2) branch.push_back(i);
    if(branch.empty()) return;

    utility::mytm.CodeStart();
    vector<int> tmpPerm; double cur = eval_greedy(tmpPerm), best = cur;
    vector<int> bestK(M,0); rep(i,M) bestK[i]=out[i].k;

    double start = utility::mytm.elapsed();
    while(utility::mytm.elapsed()-start < BUDGET_SA){
      int u = branch[rand_int()%branch.size()];
      int old = out[u].k;
      int nk = rand_int()%K; if(nk==old) nk=(nk+1)%K;
      out[u].k = nk;

      vector<int> tperm; double nxt = eval_greedy(tperm);
      // 温度線形
      double T = 70.0*(1.0 - (utility::mytm.elapsed()-start)/BUDGET_SA) + 1.0;
      if(nxt>=cur || rand_double()<exp((nxt-cur)/T)){
        cur=nxt; if(nxt>best){ best=nxt; rep(i,M) bestK[i]=out[i].k; }
      }else{
        out[u].k = old;
      }
    }
    rep(i,M) out[i].k = bestK[i];
  }

  /*** 出力 ***/
  void output_final(){
    // Hungarian で最終割当
    vector<int> perm(N,0);
    eval_exact(perm);

    // 1行目：処理装置の種類
    rep(i,N){ if(i) cout<<' '; cout<<perm[i]; } cout<<'\n';
    // 2行目：入口の行き先
    cout<<inlet_conn<<'\n';
    // 3行目以降：分類機
    rep(i,M){
      if(!out[i].used) cout<<-1<<'\n';
      else cout<<out[i].k<<' '<<out[i].v1<<' '<<out[i].v2<<'\n';
    }
  }

  void solve(){
    utility::mytm.CodeStart();
    build_pascal_dag();     // 交差0・必ず両出口・末端は処理装置
    anneal_branch_types();  // あなたの焼き方でタイプだけ焼く
    output_final();         // Hungarian で確定割当
  }
};

int main(){
  ios::sync_with_stdio(false);
  cin.tie(nullptr);
  Solver s; s.solve();
  return 0;
}
