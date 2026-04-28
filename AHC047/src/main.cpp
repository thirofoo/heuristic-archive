#include <bits/stdc++.h>
using namespace std;

#define rep(i,n) for(int i=0;i<(int)(n);++i)

struct Solver {
  // 入力
  int N=0,M=0,K=0;
  vector<pair<int,int>> R;     // 初期位置(0-index)
  vector<vector<int>> V;       // 縦壁: N x (N-1)  (between (y,x)-(y,x+1))
  vector<vector<int>> H;       // 横壁: (N-1) x N  (between (y,x)-(y+1,x))

  // 出力
  vector<string> buttons;      // K 行, 各行は M 文字（実際は空白区切りで出力）
  vector<int> ops;             // 操作列（1 行 1 押下）

  // 方向
  const array<int,4> dy={-1,1,0,0};
  const array<int,4> dx={ 0,0,-1,1};
  const array<char,5> dch={'U','D','L','R','S'};

  // 入力ヘルパ
  static vector<int> parse01RowString(const string& s){
    vector<int> a; a.reserve(s.size());
    for(char c: s) if(c=='0'||c=='1') a.push_back(c-'0');
    return a;
  }

  void input(){
    cin>>N>>M>>K;
    R.resize(M);
    rep(i,M){
      int y,x; cin>>y>>x;
      if(1<=y&&y<=N && 1<=x&&x<=N){ y--; x--; }  // 1-index → 0-index
      y=max(0,min(N-1,y)); x=max(0,min(N-1,x));
      R[i]={y,x};
    }
    V.assign(N, vector<int>(max(0,N-1),0));
    H.assign(max(0,N-1), vector<int>(N,0));
    // 縦壁 N 行
    rep(y,N){
      if(N-1==0) continue;
      string s; if(cin.peek()=='\n') cin.get();
      getline(cin,s); while(s.empty() && !cin.eof()) getline(cin,s);
      auto row=parse01RowString(s);
      if((int)row.size()==N-1) rep(x,N-1) V[y][x]=row[x];
      else{ stringstream ss(s); rep(x,N-1){ int b=0; ss>>b; V[y][x]=(b!=0);} }
    }
    // 横壁 N-1 行
    rep(y,max(0,N-1)){
      string s; if(cin.peek()=='\n') cin.get();
      getline(cin,s); while(s.empty() && !cin.eof()) getline(cin,s);
      auto row=parse01RowString(s);
      if((int)row.size()==N) rep(x,N) H[y][x]=row[x];
      else{ stringstream ss(s); rep(x,N){ int b=0; ss>>b; H[y][x]=(b!=0);} }
    }
  }

  // 盤面ユーティリティ
  inline bool canMove(int y,int x,int d) const {
    if(d==0){ if(y==0) return false;     return H[y-1][x]==0; }
    if(d==1){ if(y==N-1) return false;   return H[y][x]==0; }
    if(d==2){ if(x==0) return false;     return V[y][x-1]==0; }
    if(d==3){ if(x==N-1) return false;   return V[y][x]==0; }
    return true; // 'S'
  }
  inline pair<int,int> step(int y,int x,int d) const {
    if(d==4) return {y,x};
    if(!canMove(y,x,d)) return {y,x};
    return {y+dy[d], x+dx[d]};
  }
  inline int wallCount(int y,int x) const {
    int c=0;
    if(!canMove(y,x,0)) c++;
    if(!canMove(y,x,1)) c++;
    if(!canMove(y,x,2)) c++;
    if(!canMove(y,x,3)) c++;
    return c;
  }
  inline bool isCorner(int y,int x) const { return wallCount(y,x)>=2; }

  // ===== Phase1: 角取り（バッチ制） =====
  // 使えるキーを後ろから 2 本ずつペアにしてバッチ化（例: (8,9),(6,7),(4,5)）
  vector<pair<int,int>> make_batch_pairs() {
    vector<pair<int,int>> pairs;
    vector<int> ids;
    for(int k=K-1;k>=0;k--) ids.push_back(k);
    // 清掃キー(0,1,2)は避けたいので取り除く
    auto keep=[&](int x){ return !(x==0||x==1||x==2); };
    vector<int> pool;
    for(int x: ids) if(keep(x)) pool.push_back(x);
    for(size_t i=0;i+1<pool.size(); i+=2){
      pairs.push_back({pool[i+1], pool[i]}); // 小さい番号→縦、 大きい番号→横 みたいにしても良い
      if(pairs.size()>=4) break; // 上限: 4 バッチ（十分）
    }
    return pairs;
  }

  // 4 通りの向き（UL/UR/DL/DR）を返す
  static array<array<int,2>,4> corner_dirs() {
    return {{{0,2},{0,3},{1,2},{1,3}}}; // (U,L), (U,R), (D,L), (D,R)
  }

  // キーマップを全 S で初期化
  void init_buttons_allS(){ buttons.assign(K, string(M,'S')); }

  // 指定バッチにロボ群を割当：batchPairs[b] = (kA,kB)
  // 各ロボ i には (dirA,dirB) を与える（UL/UR/DL/DRを循環）
  void apply_batch_mapping(const vector<pair<int,int>>& batchPairs,
                           const vector<vector<int>>& batchOf,
                           const vector<array<int,2>>& dirOfRobot){
    init_buttons_allS();
    rep(i,M){
      int b = batchOf[0][i]; // 今は1段階の割当なので [0] 層を使う
      if(b<0) continue;
      auto [kA,kB] = batchPairs[b];
      buttons[kA][i] = dch[ dirOfRobot[i][0] ];
      buttons[kB][i] = dch[ dirOfRobot[i][1] ];
    }
    // 清掃キー(0,1,2)は後で上書き
  }

  // バッチ b に対して、全員が「角」に到達するまで keyA→keyB を 1 手ずつ進める（最大 2N）
  // 途中で全員角に達したら即終了（追加の角寄せはしない）
  // pos は逐次更新し、訪問配列 vis も更新（スコア打ち切り用）
  void run_phase1_single_batch(int b, pair<int,int> pairKey,
                               vector<pair<int,int>>& pos,
                               vector<vector<char>>& vis,
                               vector<int>& outOps,
                               int limit) {
    auto [kA,kB]=pairKey;
    auto press=[&](int key)->int{
      int newPaint=0;
      rep(i,M){
        int d=4; char c=buttons[key][i];
        if(c=='U') d=0; else if(c=='D') d=1; else if(c=='L') d=2; else if(c=='R') d=3;
        auto [ny,nx]=step(pos[i].first,pos[i].second,d);
        pos[i]={ny,nx};
        if(!vis[ny][nx]){ vis[ny][nx]=1; newPaint++; }
      }
      return newPaint;
    };
    auto allCorner=[&]()->bool{
      bool ok=true;
      rep(i,M){
        // このロボはこのバッチの担当？
        if(buttons[kA][i]=='S' && buttons[kB][i]=='S') continue;
        auto [y,x]=pos[i];
        if(!isCorner(y,x)) { ok=false; break; }
      }
      return ok;
    };

    // 2N 手以内（U→L→U→L … のように交互）
    for(int t=0; t<2*N && (int)outOps.size()<limit; ++t){
      int key = (t%2==0? kA : kB);
      outOps.push_back(key);
      int gain = press(key);
      if(allCorner()) break;               // ① 全員角に着いたら打ち切り
      if(gain==0) break;                   // ② 何も塗れない手が出たら以降打たない
    }
  }

  // ===== Phase2: 清掃（ジグザグ） =====
  // 角の向きに応じて、Key0(縦1), Key1(横1), Key2(逆縦1) を各ロボ別に割当
  void assign_cleaning_keys(const vector<pair<int,int>>& posAtPhase1){
    // 各ロボごとに縦方向(上/下)と横(左/右)と逆縦を決める
    rep(i,M){
      auto [y,x]=posAtPhase1[i];
      bool U=!canMove(y,x,0), D=!canMove(y,x,1), L=!canMove(y,x,2), Rr=!canMove(y,x,3);
      int v1=-1, vx=-1, v2=-1;
      if(U && L){ v1=1; v2=0; vx=3; }      // 左上角: 下, 右, 上
      else if(U && Rr){ v1=1; v2=0; vx=2; } // 右上角: 下, 左, 上
      else if(D && L){ v1=0; v2=1; vx=3; }  // 左下角: 上, 右, 下
      else if(D && Rr){ v1=0; v2=1; vx=2; } // 右下角: 上, 左, 下
      else if(U){ v1=1; v2=0; vx = Rr?2:3; }
      else if(D){ v1=0; v2=1; vx = Rr?2:3; }
      else if(L){ v1=1; v2=0; vx = 3; }
      else if(Rr){ v1=1; v2=0; vx = 2; }
      else { v1=1; v2=0; vx=3; }
      if(K>=1) buttons[0][i]=dch[v1];
      if(K>=2) buttons[1][i]=dch[vx];
      if(K>=3) buttons[2][i]=dch[v2];
    }
  }

  // 清掃を 1 手ずつ押していき、1 手で新規塗りが 0 になったら停止
  void run_phase2_cleaning(vector<pair<int,int>>& pos,
                           vector<vector<char>>& vis,
                           vector<int>& outOps,
                           int limit){
    auto press=[&](int key)->int{
      int gain=0;
      rep(i,M){
        int d=4; char c=buttons[key][i];
        if(c=='U') d=0; else if(c=='D') d=1; else if(c=='L') d=2; else if(c=='R') d=3;
        auto [ny,nx]=step(pos[i].first,pos[i].second,d);
        pos[i]={ny,nx};
        if(!vis[ny][nx]){ vis[ny][nx]=1; gain++; }
      }
      return gain;
    };
    while((int)outOps.size()<limit){
      int g=press(0); outOps.push_back(0); if(g==0 || (int)outOps.size()>=limit) break;
      g=press(1); outOps.push_back(1); if(g==0 || (int)outOps.size()>=limit) break;
      g=press(2); outOps.push_back(2); if(g==0 || (int)outOps.size()>=limit) break;
      g=press(1); outOps.push_back(1); if(g==0 || (int)outOps.size()>=limit) break;
    }
  }

  void solve(){
    const int LIMIT = max(1, N*N); // アクション上限

    // 1) バッチキー準備（後方キーから2本ずつ）
    auto batchPairs = make_batch_pairs();
    int B = (int)batchPairs.size();
    if(B==0){
      // キーが足りない場合は、全部 S で終了（フォールバック）
      buttons.assign(K, string(M,'S'));
      return;
    }

    // 2) ロボをバッチに割当＆UL/UR/DL/DR を交互に付与
    vector<vector<int>> batchOf(1, vector<int>(M,-1));
    rep(i,M) batchOf[0][i] = i % B; // 均等割
    auto dirs4 = corner_dirs();
    vector<array<int,2>> dirOfRobot(M);
    rep(i,M) dirOfRobot[i] = dirs4[i%4];

    // 3) キーマップに反映（Phase1用）
    apply_batch_mapping(batchPairs, batchOf, dirOfRobot);

    // 4) Phase1 をバッチごとに順次実行（全員角に到達したら即打ち切り）
    vector<pair<int,int>> pos = R;
    vector<vector<char>> vis(N, vector<char>(N,0));
    rep(i,M) vis[pos[i].first][pos[i].second]=1;
    ops.clear(); ops.reserve(LIMIT);

    for(int b=0; b<B && (int)ops.size()<LIMIT; ++b){
      run_phase1_single_batch(b, batchPairs[b], pos, vis, ops, LIMIT);
    }

    // 5) Phase2 キー割当（Key0,1,2）
    assign_cleaning_keys(pos);

    // 6) Phase2 清掃（1 手ごとに新規塗りチェック、0 になったら停止）
    if((int)ops.size()<LIMIT){
      run_phase2_cleaning(pos, vis, ops, LIMIT);
    }
  }

  void output(){
    // ボタン行：空白区切り
    rep(k,K){
      rep(j,M){ if(j) cout<<' '; cout<<buttons[k][j]; }
      cout<<"\n";
    }
    // 操作列：1 行 1 押下（個数は出さない）
    for(int a: ops) cout<<a<<"\n";
  }
};

int main(){
  ios::sync_with_stdio(false);
  cin.tie(nullptr);
  Solver s; s.input(); s.solve(); s.output();
  return 0;
}
