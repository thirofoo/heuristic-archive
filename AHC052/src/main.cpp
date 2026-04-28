#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
#define rep(i,n) for(int i=0;i<(n);i++)
using ll = long long;

namespace utility {
  struct timer {
    chrono::system_clock::time_point start;
    void CodeStart(){ start = chrono::system_clock::now(); }
    double elapsed() const {
      using namespace std::chrono;
      return (double)duration_cast<milliseconds>(system_clock::now()-start).count();
    }
  } mytm;
}

inline unsigned int rand_int(){
  static unsigned int tx=123456789,ty=362436069,tz=521288629,tw=88675123;
  unsigned int tt=(tx^(tx<<11)); tx=ty,ty=tz,tz=tw;
  return tw=(tw^(tw>>19))^(tt^(tt>>8));
}

inline double rand_double(){ return (double)(rand_int()% (int)1e9)/1e9; }
inline double gaussian(double mean,double stddev){
  double z0 = sqrt(-2.0*log(rand_double()))*cos(2.0*M_PI*rand_double());
  return mean + z0*stddev;
}

#define TIME_LIMIT 2950
inline double temp(double start){
  double start_temp=100,end_temp=1;
  return start_temp+(end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}
inline double prob(int best,int now,int start){
  return exp((double)(now-best)/temp(start));
}

//----------------- 実装 -----------------//

using P = pair<int,int>;
using T = tuple<int,int,int>;

// right | down | left | up
#define DIR_NUM 4
vector<int> dx={0,1,0,-1};
vector<int> dy={1,0,-1,0};

vector<char> dir_to_char = {'R', 'D', 'L', 'U'};

struct Solver {
  int N,M,K;
  vector<P> robots;
  vector<vector<vector<bool>>> wall;

  vector<string> key_configs;
  vector<int> ops;

  // 初期(キー0/1)後の状態
  vector<P> base_pos;
  vector<vector<char>> base_visited;
  int base_painted = 0;

  Solver(){
    input();
    key_configs.assign(K, string(M, 'S'));
  }

  void input(){
    cin>>N>>M>>K;
    robots.resize(M);
    wall.assign(N, vector<vector<bool>>(N, vector<bool>(DIR_NUM,false)));

    rep(i,M){
      int x,y; cin>>x>>y;
      robots[i]={x,y};
    }

    // v: 横方向 (Right/Left)
    rep(i,N){
      string s; cin>>s;
      rep(j,N-1){
        if(s[j]=='1'){
          wall[i][j][0]=true;     // right
          wall[i][j+1][2]=true;   // left
        }
      }
    }

    // h: 縦方向 (Down/Up)
    rep(i,N-1){
      string s; cin>>s;
      rep(j,N){
        if(s[j]=='1'){
          wall[i][j][1]=true;     // down
          wall[i+1][j][3]=true;   // up
        }
      }
    }

    // 外壁
    rep(i,N){
      wall[i][0][2]=true;
      wall[i][N-1][0]=true;
    }
    rep(j,N){
      wall[0][j][3]=true;
      wall[N-1][j][1]=true;
    }
  }

  void output(){
    for(auto &s: key_configs) {
      for(int i = 0; i < M; i++) {
        cout << s[i] << " ";
      }
      cout << "\n";
    }
    for(auto &c: ops) cout<<c<<"\n";
  }

  void solve(){
    utility::mytm.CodeStart();

    const int OPS_LIMIT = 2*N*N; // 常にこれを超えない

    // 角マス列挙（>=2 壁）
    vector<P> corners;
    rep(i,N)rep(j,N){
      int cnt=0;
      rep(d,DIR_NUM) if(wall[i][j][d]) cnt++;
      if(cnt>=2) corners.push_back({i,j});
    }

    // 01 BFS（「進行方向が壁なら曲がれる」1回まで）
    vector dists(M, vector(N, vector<pair<int,int>>(N, {-1,-1})));
    for(int k=0;k<M;k++){
      auto [x,y]=robots[k];
      deque<T> dq;
      dq.emplace_back(x,y,DIR_NUM);
      dists[k][x][y]={0,DIR_NUM};
      while(!dq.empty()){
        auto [cx,cy,dir]=dq.front(); dq.pop_front();
        for(int d=0; d<DIR_NUM; d++){
          if(wall[cx][cy][d]) continue;
          int nx=cx+dx[d], ny=cy+dy[d];
          if(nx<0||nx>=N||ny<0||ny>=N) continue;
          if(dists[k][nx][ny].first!=-1) continue;
          if(dir==d || dir==DIR_NUM){
            dists[k][nx][ny]={dists[k][cx][cy].first,d};
            dq.emplace_front(nx,ny,d);
          }else if(wall[cx][cy][dir]){
            dists[k][nx][ny]={dists[k][cx][cy].first+1,d};
            dq.emplace_back(nx,ny,d);
          }
        }
      }
    }

    // corner reachable
    vector<vector<int>> corner_reach(M);
    for(int k=0;k<M;k++){
      rep(i,corners.size()){
        auto [x,y]=corners[i];
        if(dists[k][x][y].first!=-1 && dists[k][x][y].first<=1){
          corner_reach[k].push_back(i);
        }
      }
    }

    // 最大マッチング（最小費用0）
    mcf_graph<int,int> g(M+corners.size()+2);
    int S=M+corners.size(), T=S+1;
    rep(k,M) g.add_edge(S,k,1,0);
    rep(i,corners.size()) g.add_edge(M+i,T,1,0);
    rep(k,M) for(auto cid:corner_reach[k]) g.add_edge(k,M+cid,1,0);
    g.flow(S,T);

    // 割当復元
    vector<P> goal(M,{-1,-1});
    for(auto &e: g.edges()){
      if(e.from<M && e.to>=M && e.to<M+(int)corners.size() && e.flow==1){
        int rid=e.from, cid=e.to-M;
        goal[rid]=corners[cid];
      }
    }

    // ===== 初期(0/1)フェーズのための canMove0（後でも使う） =====
    auto inb0 = [&](int x,int y){ return 0<=x && x<N && 0<=y && y<N; };
    auto canMove0 = [&](int x,int y,int d){
      if(!inb0(x,y)) return false;
      int nx=x+dx[d], ny=y+dy[d];
      if(!inb0(nx,ny)) return false;
      return !wall[x][y][d];
    };

    // --- 初期移動 (キー0/1) 設定 ---
    for (int i = 0 ; i < M; i++) {
      if (goal[i].first != -1) {
        // 角へ：既存ロジックそのまま
        auto [sx, sy] = robots[i];
        auto [gx, gy] = goal[i];
        int last_dir = dists[i][gx][gy].second;

        int dx_diff = gx - sx;
        int dy_diff = gy - sy;

        int dir1 = -1, dir2 = -1;
        if (dx_diff != 0) dir1 = (dx_diff > 0 ? 1 : 3); // down/up
        if (dy_diff != 0) dir2 = (dy_diff > 0 ? 0 : 2); // right/left

        int second_dir = last_dir;
        int first_dir  = (second_dir == dir1 ? dir2 : dir1);

        if (dir1 == -1 || dir2 == -1) {
          second_dir = (dir1 != -1 ? dir1 : dir2);
          first_dir = -1;
        }

        if(first_dir != -1) key_configs[0][i] = dir_to_char[first_dir];
        if(second_dir != -1) key_configs[1][i] = dir_to_char[second_dir];
      } else {
        // マッチング漏れ：中心に寄る初期移動（象限ベース）
        int cx = N/2, cy = N/2;
        int sx = robots[i].first, sy = robots[i].second;
        int vdir =  -1; // 縦成分：cx-sx > 0 → 下(1), <0 → 上(3)
        int hdir =  -1; // 横成分：cy-sy > 0 → 右(0), <0 → 左(2)
        int dv = cx - sx, dh = cy - sy;
        if(dv != 0) vdir = (dv > 0 ? 1 : 3);
        if(dh != 0) hdir = (dh > 0 ? 0 : 2);

        int first_dir = -1, second_dir = -1;

        if(vdir==-1 && hdir==-1){
          // 既に中心（超レア）→ 何もしない
        }else if(vdir==-1 || hdir==-1){
          // 片方のみ必要
          int only = (vdir!=-1? vdir : hdir);
          // 目の前が壁ならもう片方も試す（ただし存在しないならS）
          if(canMove0(sx,sy,only)){
            second_dir = only;
          }else{
            // 押しても動かないなら両方Sのまま（初期フェーズ早期停止が効く）
          }
        }else{
          // 両成分あり：距離が大きい軸を先、詰まっていたら入れ替え
          bool vertical_first = (abs(dv) >= abs(dh));
          int dA = vertical_first ? vdir : hdir;
          int dB = vertical_first ? hdir : vdir;

          bool a_ok = canMove0(sx,sy,dA);
          bool b_ok = canMove0(sx,sy,dB);

          if(a_ok && b_ok){
            first_dir = dA; second_dir = dB;
          }else if(!a_ok && b_ok){
            first_dir = dB; // まず動ける方を優先
            // 2手目は様子見（その場で詰む可能性が高いので second_dir は dB と同じでも良いが、
            // 既存の「0→1 で直交する」流儀に合わせて second は残りの軸にしておく）
            second_dir = dA;
          }else if(a_ok && !b_ok){
            first_dir = dA; second_dir = -1;
          }else{
            // どちらも一歩目が壁なら諦め（S,S）
          }
        }

        if(first_dir != -1)  key_configs[0][i] = dir_to_char[first_dir];
        if(second_dir != -1) key_configs[1][i] = dir_to_char[second_dir];
      }
    }

    // ===== 初期(0/1)フェーズを早期打ち切り & 塗り反映 =====
    base_pos = robots;
    base_visited.assign(N, vector<char>(N, 0));
    base_painted = 0;
    rep(i,M){
      auto [x,y]=base_pos[i];
      if(!base_visited[x][y]){ base_visited[x][y]=1; base_painted++; }
    }

    auto press_once0 = [&](int key)->bool{
      if((int)ops.size() >= OPS_LIMIT) return false;
      bool moved=false;
      rep(i,M){
        int d=-1;
        char c = key_configs[key][i];
        if(c=='R') d=0; else if(c=='D') d=1; else if(c=='L') d=2; else if(c=='U') d=3;
        if(d==-1) continue;
        auto [x,y]=base_pos[i];
        if(canMove0(x,y,d)){
          x+=dx[d]; y+=dy[d];
          base_pos[i]={x,y};
          if(!base_visited[x][y]){ base_visited[x][y]=1; base_painted++; }
          moved=true;
        }
      }
      if(moved) ops.emplace_back(key); // 動いた時だけ出力に追加
      return moved;
    };

    for(int t=0; t<N; ++t){ if(!press_once0(0) || (int)ops.size()>=OPS_LIMIT) break; }
    for(int t=0; t<N; ++t){ if(!press_once0(1) || (int)ops.size()>=OPS_LIMIT) break; }

    // ===== 掃除パターン探索 =====
    array<array<int,4>,4> pat_dir = {{
        {3, 0, 1, 0}, // U,R,D,R
        {0, 1, 2, 1}, // R,D,L,D
        {2, 3, 0, 3}, // L,U,R,U
        {1, 2, 3, 2}, // D,L,U,L
    }};
    const int KEY_A = 2, KEY_B = 3, KEY_C = 4, KEY_D = 5;

    auto inb = [&](int x,int y){ return 0<=x && x<N && 0<=y && y<N; };
    auto canMove = [&](int x,int y,int d){
      if(!inb(x,y)) return false;
      if(!inb(x+dx[d], y+dy[d])) return false;
      return !wall[x][y][d];
    };

    auto first_dir_of_pat = [&](int p){ return pat_dir[p][0]; };

    vector<vector<int>> allowed_pats(M);
    rep(i,M){
      int cx = (goal[i].first!=-1 ? goal[i].first : base_pos[i].first);
      int cy = (goal[i].second!=-1 ? goal[i].second : base_pos[i].second);

      if(goal[i].first!=-1){
        bool L = wall[cx][cy][2];
        bool R = wall[cx][cy][0];
        bool U = wall[cx][cy][3];
        bool D = wall[cx][cy][1];

        array<int,4> want = {-1,-1,-1,-1};
        int wn = 0;
        auto add = [&](int p){
          bool ok=true; rep(t,wn) if(want[t]==p) ok=false;
          if(ok) want[wn++]=p;
        };
        if(L && U){ add(1); add(3); }
        if(R && U){ add(2); add(3); }
        if(L && D){ add(1); add(0); }
        if(R && D){ add(2); add(0); }
        if(wn==0){
          rep(p,4){ int fd=first_dir_of_pat(p); if(!wall[cx][cy][fd]) add(p); }
        }
        if(wn==0){ allowed_pats[i] = {0,1,2,3}; }
        else{
          allowed_pats[i].clear();
          rep(t,wn) if(want[t]!=-1) allowed_pats[i].push_back(want[t]);
        }
      }else{
        vector<int> v;
        rep(p,4){
          int fd = first_dir_of_pat(p);
          if(!wall[cx][cy][fd]) v.push_back(p);
        }
        if(v.empty()) v = {0,1,2,3};
        allowed_pats[i]=v;
      }
    }

    auto will_gain_straight_score = [&](int key,
                                        const vector<P>& pos,
                                        const vector<vector<char>>& visited)->bool{
      rep(i,M){
        char c = key_configs[key][i];
        int d=-1; if(c=='R') d=0; else if(c=='D') d=1; else if(c=='L') d=2; else if(c=='U') d=3;
        if(d==-1) continue;
        int x=pos[i].first, y=pos[i].second;
        while(true){
          if(!inb(x,y)) break;
          if(!inb(x+dx[d], y+dy[d])) break;
          if(wall[x][y][d]) break;
          x+=dx[d]; y+=dy[d];
          if(!visited[x][y]) return true;
        }
      }
      return false;
    };

    auto score_for_assignment = [&](const vector<int>& assign_pat)->long long {
      if (utility::mytm.elapsed() > 1800.0) return (ll)9e18;

      vector<P> pos = base_pos;
      vector<vector<char>> visited = base_visited;
      int painted = 0;
      rep(i,N) rep(j,N) if(visited[i][j]) painted++;

      long long presses = 0;
      const long long MAX_PRESSES = 4LL*N*N + 1000;

      auto press_key = [&](int key)->bool{
        bool moved=false;
        rep(i,M){
          char c = key_configs[key][i];
          int d = -1;
          if(c=='R') d=0; else if(c=='D') d=1; else if(c=='L') d=2; else if(c=='U') d=3;
          if(d==-1) continue;
          auto [x,y]=pos[i];
          if(canMove(x,y,d)){
            x+=dx[d]; y+=dy[d];
            pos[i]={x,y};
            if(!visited[x][y]){ visited[x][y]=1; painted++; }
            moved=true;
          }
        }
        if(moved) presses++;
        return moved;
      };

      for(long long guard=0; guard<MAX_PRESSES && painted < N*N; ){
        rep(i,M){
          int p = (i<(int)assign_pat.size()? assign_pat[i] : 0);
          key_configs[KEY_A][i]=dir_to_char[ pat_dir[p][0] ];
        }
        if(!will_gain_straight_score(KEY_A, pos, visited)){
          // skip
        }else{
          for(int t=0;t<N;t++){
            bool mv = press_key(KEY_A);
            guard++;
            if(!mv || painted>=N*N) break;
            if(!will_gain_straight_score(KEY_A, pos, visited)) break;
          }
        }
        if(painted>=N*N) break;

        rep(i,M){
          int p = (i<(int)assign_pat.size()? assign_pat[i] : 0);
          key_configs[KEY_B][i]=dir_to_char[ pat_dir[p][1] ];
        }
        (void)press_key(KEY_B); guard++; if(painted>=N*N) break;

        rep(i,M){
          int p = (i<(int)assign_pat.size()? assign_pat[i] : 0);
          key_configs[KEY_C][i]=dir_to_char[ pat_dir[p][2] ];
        }
        if(!will_gain_straight_score(KEY_C, pos, visited)){
          // skip
        }else{
          for(int t=0;t<N;t++){
            bool mv = press_key(KEY_C);
            guard++;
            if(!mv || painted>=N*N) break;
            if(!will_gain_straight_score(KEY_C, pos, visited)) break;
          }
        }
        if(painted>=N*N) break;

        rep(i,M){
          int p = (i<(int)assign_pat.size()? assign_pat[i] : 0);
          key_configs[KEY_D][i]=dir_to_char[ pat_dir[p][3] ];
        }
        (void)press_key(KEY_D); guard++; if(painted>=N*N) break;
      }
      return (long long)((painted < N*N) ? (ll)1e9 + presses : presses);
    };

    // 混在基数全探索（1.8s 打ち切り）
    vector<int> best_assign(M, 0);
    long long best_score = (ll)9e18;
    vector<int> idx(M, 0);

    auto build_assign = [&](){
      vector<int> a(M,0);
      rep(i,M){
        int ii = min(idx[i], (int)allowed_pats[i].size()-1);
        a[i] = allowed_pats[i][ii];
      }
      return a;
    };
    auto next_mixed_radix = [&]()->bool{
      for(int i=0;i<M;i++){
        idx[i]++;
        if(idx[i] < (int)allowed_pats[i].size()) return true;
        idx[i]=0;
      }
      return false;
    };

    do{
      if (utility::mytm.elapsed() > 1800.0) break;
      vector<int> assign_pat = build_assign();
      long long sc = score_for_assignment(assign_pat);
      if(sc < best_score){
        best_score = sc;
        best_assign = assign_pat;
      }
    } while(next_mixed_radix());

    // 掃除キー2..5をbest_assignで確定（全ロボ対象）
    rep(i, M) {
      int p = best_assign[i];
      key_configs[KEY_A][i] = dir_to_char[ pat_dir[p][0] ];
      key_configs[KEY_B][i] = dir_to_char[ pat_dir[p][1] ];
      key_configs[KEY_C][i] = dir_to_char[ pat_dir[p][2] ];
      key_configs[KEY_D][i] = dir_to_char[ pat_dir[p][3] ];
    }

    // ===== 実際の押下列生成（初期フェーズ後の状態から） =====
    {
      auto inbX = [&](int x,int y){ return 0<=x && x<N && 0<=y && y<N; };
      auto canMoveEmit = [&](int x,int y,int d){
        if(!inbX(x,y)) return false;
        int nx=x+dx[d], ny=y+dy[d];
        if(!inbX(nx,ny)) return false;
        return !wall[x][y][d];
      };

      vector<P> pos = base_pos;
      vector<vector<char>> visited = base_visited;
      int painted = 0;
      rep(i,N) rep(j,N) if(visited[i][j]) painted++;

      auto press_emit = [&](int key)->bool{
        if((int)ops.size() >= OPS_LIMIT) return false;
        bool moved=false;
        rep(i,M){
          int d=-1; char c = key_configs[key][i];
          if(c=='R') d=0; else if(c=='D') d=1; else if(c=='L') d=2; else if(c=='U') d=3;
          if(d==-1) continue;
          auto [x,y]=pos[i];
          if(canMoveEmit(x,y,d)){
            x+=dx[d]; y+=dy[d];
            pos[i]={x,y};
            if(!visited[x][y]){ visited[x][y]=1; painted++; }
            moved=true;
          }
        }
        if(moved) ops.emplace_back(key);
        return moved;
      };

      auto will_gain_straight_emit = [&](int key,
                                         const vector<P>& pos_cur,
                                         const vector<vector<char>>& visited_cur)->bool{
        rep(i,M){
          char c = key_configs[key][i];
          int d=-1; if(c=='R') d=0; else if(c=='D') d=1; else if(c=='L') d=2; else if(c=='U') d=3;
          if(d==-1) continue;
          int x=pos_cur[i].first, y=pos_cur[i].second;
          while(true){
            if(!inbX(x,y)) break;
            if(!inbX(x+dx[d], y+dy[d])) break;
            if(wall[x][y][d]) break;
            x+=dx[d]; y+=dy[d];
            if(!visited_cur[x][y]) return true;
          }
        }
        return false;
      };

      int guard = 0;
      while(painted < N*N && (int)ops.size() < OPS_LIMIT && utility::mytm.elapsed() <= 1900.0){
        if(will_gain_straight_emit(KEY_A, pos, visited)){
          for(int t=0;t<N && (int)ops.size() < OPS_LIMIT; t++){
            bool mv = press_emit(KEY_A); guard++;
            if(!mv || painted>=N*N) break;
            if(!will_gain_straight_emit(KEY_A, pos, visited)) break;
          }
        }
        if(painted>=N*N || (int)ops.size()>=OPS_LIMIT) break;

        (void)press_emit(KEY_B); guard++;
        if(painted>=N*N || (int)ops.size()>=OPS_LIMIT) break;

        if(will_gain_straight_emit(KEY_C, pos, visited)){
          for(int t=0;t<N && (int)ops.size() < OPS_LIMIT; t++){
            bool mv = press_emit(KEY_C); guard++;
            if(!mv || painted>=N*N) break;
            if(!will_gain_straight_emit(KEY_C, pos, visited)) break;
          }
        }
        if(painted>=N*N || (int)ops.size()>=OPS_LIMIT) break;

        (void)press_emit(KEY_D); guard++;
        if(painted>=N*N || (int)ops.size()>=OPS_LIMIT) break;
      }

      // 仕上げ：未塗りが残った場合の簡易モップアップ
      if(painted < N*N && (int)ops.size() < OPS_LIMIT){
        const int KEY_MOP = min(6, K-1);
        auto ray_to_unvisited = [&](int x,int y,int d)->int{
          int steps=0;
          while(true){
            if(!inbX(x,y)) return INT_MAX;
            if(!inbX(x+dx[d], y+dy[d])) return INT_MAX;
            if(wall[x][y][d]) return INT_MAX;
            x+=dx[d]; y+=dy[d]; steps++;
            if(!visited[x][y]) return steps;
          }
        };

        int stagnate = 0;
        while(painted < N*N && (int)ops.size() < OPS_LIMIT && utility::mytm.elapsed() <= 1950.0){
          rep(i,M){
            int bestd=-1, bestdist=INT_MAX;
            rep(d,4){
              if(!canMoveEmit(pos[i].first, pos[i].second, d)) continue;
              int dist = ray_to_unvisited(pos[i].first, pos[i].second, d);
              if(dist < bestdist){
                bestdist = dist; bestd = d;
              }
            }
            if(bestd==-1){
              int fallback=-1;
              rep(d,4) if(canMoveEmit(pos[i].first, pos[i].second, d)){ fallback=d; break; }
              if(fallback==-1) key_configs[KEY_MOP][i]='S';
              else key_configs[KEY_MOP][i]=dir_to_char[fallback];
            }else{
              key_configs[KEY_MOP][i] = dir_to_char[bestd];
            }
          }

          int before = painted;
          bool mv = press_emit(KEY_MOP);
          if(!mv || painted==before){
            stagnate++;
            if(stagnate>=2*N) break;
          }else{
            stagnate=0;
          }
        }
      }
    }
  }
};

int main(){
  cin.tie(0); ios::sync_with_stdio(false);
  Solver solver;
  solver.solve();
  solver.output();
  return 0;
}
