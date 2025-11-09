#include <bits/stdc++.h>
using namespace std;

#define rep(i, n) for (int i = 0; i < (int)(n); i++)

// ===================== 設定 =====================
// 2〜MAX_BLOCKS まで試す
constexpr int MAX_BLOCKS = 3;   // ここを 2,3,4 とかに変えて遊べる

// ビームの幅や partition の候補数
constexpr int BEAM_WIDTH = 200;
constexpr int PARTITION_TRIES_PER_B = 20; // B>=3 のときに試す partition 数（均等+ランダム）

// ===================== Solver 本体 =====================
struct Solver {
  int N, K, T;
  vector<string> vwall;   // v[i][j]: (i,j)-(i,j+1) に壁があるか
  vector<string> hwall;   // h[i][j]: (i,j)-(i+1,j) に 壁があるか
  vector<pair<int,int>> targets; // 0..K-1, 0 がスタート

  // 出力
  int C, Q, M;
  vector<vector<int>> s;

  struct Rule {
    int c, q, A, S;
    char D;
  };
  vector<Rule> rules;

  // ターゲット間距離 distT[u][v] : target u -> target v の最短距離
  vector<vector<int>> distT;

  mt19937 rng;

  Solver() : rng(123456789) { input(); }

  void input() {
    cin >> N >> K >> T;
    vwall.resize(N);
    rep(i, N) {
      string t; cin >> t;
      vwall[i] = t;
    }
    hwall.resize(N-1);
    rep(i, N-1) {
      string t; cin >> t;
      hwall[i] = t;
    }
    targets.resize(K);
    rep(i, K) {
      int x, y; cin >> x >> y;
      targets[i] = {x, y};
    }
  }

  // 盤面 BFS: (sx,sy) -> 各マスへの最短距離
  void bfs_grid_from(int sx, int sy, vector<vector<int>>& dist) {
    const int INF = 1e9;
    dist.assign(N, vector<int>(N, INF));
    queue<pair<int,int>> q;
    dist[sx][sy] = 0;
    q.push({sx, sy});

    auto can_up = [&](int i, int j) {
      return (i > 0 && hwall[i-1][j] == '0');
    };
    auto can_down = [&](int i, int j) {
      return (i+1 < N && hwall[i][j] == '0');
    };
    auto can_left = [&](int i, int j) {
      return (j > 0 && vwall[i][j-1] == '0');
    };
    auto can_right = [&](int i, int j) {
      return (j+1 < N && vwall[i][j] == '0');
    };

    while (!q.empty()) {
      auto [x, y] = q.front(); q.pop();
      int d = dist[x][y];

      if (can_up(x,y) && dist[x-1][y] > d+1) {
        dist[x-1][y] = d+1;
        q.push({x-1,y});
      }
      if (can_down(x,y) && dist[x+1][y] > d+1) {
        dist[x+1][y] = d+1;
        q.push({x+1,y});
      }
      if (can_left(x,y) && dist[x][y-1] > d+1) {
        dist[x][y-1] = d+1;
        q.push({x,y-1});
      }
      if (can_right(x,y) && dist[x][y+1] > d+1) {
        dist[x][y+1] = d+1;
        q.push({x,y+1});
      }
    }
  }

  // 2点間の最短経路（マス列）を BFS で復元
  vector<pair<int,int>> bfs_cells(pair<int,int> s0, pair<int,int> t0) {
    vector<pair<int,int>> path;
    if (s0 == t0) {
      path.push_back(s0);
      return path;
    }

    const int INF = 1e9;
    vector<vector<int>> dist(N, vector<int>(N, INF));
    vector<vector<pair<int,int>>> par(N, vector<pair<int,int>>(N, {-1,-1}));

    auto can_up = [&](int i, int j) {
      return (i > 0 && hwall[i-1][j] == '0');
    };
    auto can_down = [&](int i, int j) {
      return (i+1 < N && hwall[i][j] == '0');
    };
    auto can_left = [&](int i, int j) {
      return (j > 0 && vwall[i][j-1] == '0');
    };
    auto can_right = [&](int i, int j) {
      return (j+1 < N && vwall[i][j] == '0');
    };

    queue<pair<int,int>> q;
    dist[s0.first][s0.second] = 0;
    q.push(s0);

    while (!q.empty()) {
      auto [x, y] = q.front(); q.pop();
      int d = dist[x][y];
      if (make_pair(x,y) == t0) break;

      if (can_up(x,y) && dist[x-1][y] > d+1) {
        dist[x-1][y] = d+1;
        par[x-1][y] = {x,y};
        q.push({x-1,y});
      }
      if (can_down(x,y) && dist[x+1][y] > d+1) {
        dist[x+1][y] = d+1;
        par[x+1][y] = {x,y};
        q.push({x+1,y});
      }
      if (can_left(x,y) && dist[x][y-1] > d+1) {
        dist[x][y-1] = d+1;
        par[x][y-1] = {x,y};
        q.push({x,y-1});
      }
      if (can_right(x,y) && dist[x][y+1] > d+1) {
        dist[x][y+1] = d+1;
        par[x][y+1] = {x,y};
        q.push({x,y+1});
      }
    }

    if (dist[t0.first][t0.second] == INF) {
      path.push_back(s0);
      return path;
    }

    pair<int,int> cur = t0;
    while (cur != s0) {
      path.push_back(cur);
      cur = par[cur.first][cur.second];
    }
    path.push_back(s0);
    reverse(path.begin(), path.end());
    return path;
  }

  string cells_to_dirs(const vector<pair<int,int>>& cells) {
    string res;
    for (int i = 0; i+1 < (int)cells.size(); i++) {
      auto [x1,y1] = cells[i];
      auto [x2,y2] = cells[i+1];
      if (x2 == x1-1 && y2 == y1) res.push_back('U');
      else if (x2 == x1+1 && y2 == y1) res.push_back('D');
      else if (x2 == x1 && y2 == y1-1) res.push_back('L');
      else if (x2 == x1 && y2 == y1+1) res.push_back('R');
      else res.push_back('S');
    }
    return res;
  }

  // C,Q を need (必要状態数) に対して C*Q>=need, C+Q 最小となるように選ぶ
  pair<int,int> choose_CQ(long long need) {
    long long N4 = 1LL * N * N * N * N;
    int bestC = 1;
    int bestQ = (int)need;
    int bestScore = bestC + bestQ;
    for (int Ccand = 1; 1LL * Ccand * Ccand <= need && 1LL * Ccand <= N4; Ccand++) {
      long long Qll = (need + Ccand - 1) / Ccand;
      if (Qll > N4) continue;
      int Qcand = (int)Qll;
      int score = Ccand + Qcand;
      if (score < bestScore) {
        bestScore = score;
        bestC = Ccand;
        bestQ = Qcand;
      }
    }
    return {bestC, bestQ};
  }

  // identity (= 0->1->2->...->K-1) のオートマトンを構築し、長さ L を返す
  long long build_identity_and_get_len() {
    if (K <= 1) {
      C = 1; Q = 1; M = 0;
      s.assign(N, vector<int>(N, 0));
      rules.clear();
      return 0;
    }

    vector<pair<int,int>> pathCells;
    string moves;
    pathCells.clear();
    pathCells.push_back(targets[0]);
    moves.clear();
    for (int i = 0; i+1 < K; i++) {
      auto seg = bfs_cells(targets[i], targets[i+1]);
      string segm = cells_to_dirs(seg);
      for (int k = 1; k < (int)seg.size(); k++) pathCells.push_back(seg[k]);
      moves += segm;
    }
    int L = (int)moves.size();

    int totalCells = N * N;
    auto vid = [&](int x, int y){ return x * N + y; };

    vector<int> firstVisit(totalCells, -1);
    vector<int> lastVisit(totalCells, -1);
    vector<int> nextVisit(L+1, -1);

    for (int t = 0; t <= L; t++) {
      auto [x, y] = pathCells[t];
      int id = vid(x,y);
      if (firstVisit[id] == -1) firstVisit[id] = t;
      if (lastVisit[id] != -1) {
        nextVisit[lastVisit[id]] = t;
      }
      lastVisit[id] = t;
    }

    auto cq = choose_CQ((long long)L + 1);
    C = cq.first; Q = cq.second;

    vector<int> ct(L+1), qt(L+1);
    for (int t = 0; t <= L; t++) {
      long long idt = t;
      ct[t] = (int)(idt / Q);
      qt[t] = (int)(idt % Q);
    }

    s.assign(N, vector<int>(N, 0));
    rep(x, N) rep(y, N) {
      int id = vid(x,y);
      int t0 = firstVisit[id];
      if (t0 != -1) s[x][y] = ct[t0];
      else s[x][y] = 0;
    }

    vector<int> At(L), St(L);
    for (int t = 0; t < L; t++) {
      int nxt = nextVisit[t];
      if (nxt != -1) At[t] = ct[nxt];
      else At[t] = 0;
      St[t] = qt[t+1];
    }

    rules.clear();
    rules.reserve(L);
    for (int t = 0; t < L; t++) {
      rules.push_back({ct[t], qt[t], At[t], St[t], moves[t]});
    }
    M = L;
    return (long long)L;
  }

  // ord: 1..K-1 の順列
  // 経路: 0 -> ord[0] -> ... -> ord.back() -> 0
  void build_cycle_from_ord(const vector<int>& ord,
                            vector<pair<int,int>>& pathCells,
                            string& moves) {
    pathCells.clear();
    moves.clear();

    pair<int,int> curPos = targets[0];
    pathCells.push_back(curPos);

    auto append_seg = [&](int fromIdx, int toIdx) {
      auto seg_cells = bfs_cells(targets[fromIdx], targets[toIdx]);
      string seg_moves = cells_to_dirs(seg_cells);
      for (int k = 1; k < (int)seg_cells.size(); k++) {
        pathCells.push_back(seg_cells[k]);
      }
      moves += seg_moves;
    };

    int curIdx = 0;
    for (int idx : ord) {
      append_seg(curIdx, idx);
      curIdx = idx;
    }
    append_seg(curIdx, 0); // 最後にスタートに戻る
  }

  // 周期オートマトン構築。pathCells, moves は 1 周期分。
  void build_periodic_automaton(const vector<pair<int,int>>& pathCells,
                                const string& moves) {
    int L = (int)moves.size();
    int totalCells = N * N;
    auto vid = [&](int x, int y){ return x * N + y; };

    vector<int> firstVisit(totalCells, -1);
    vector<int> lastVisit(totalCells, -1);
    vector<int> nextVisit(L, -1);

    for (int t = 0; t < L; t++) {
      auto [x, y] = pathCells[t];
      int id = vid(x,y);
      if (firstVisit[id] == -1) firstVisit[id] = t;
      if (lastVisit[id] != -1) {
        nextVisit[lastVisit[id]] = t;
      }
      lastVisit[id] = t;
    }
    for (int id = 0; id < totalCells; id++) {
      int fv = firstVisit[id];
      int lv = lastVisit[id];
      if (fv != -1 && lv != -1 && nextVisit[lv] == -1) {
        nextVisit[lv] = fv;
      }
    }

    auto cq = choose_CQ((long long)L);
    C = cq.first; Q = cq.second;

    vector<int> ct(L), qt(L);
    for (int t = 0; t < L; t++) {
      long long idt = t;
      ct[t] = (int)(idt / Q);
      qt[t] = (int)(idt % Q);
    }

    s.assign(N, vector<int>(N, 0));
    rep(x, N) rep(y, N) {
      int id = vid(x,y);
      int t0 = firstVisit[id];
      if (t0 != -1) s[x][y] = ct[t0];
      else s[x][y] = 0;
    }

    vector<int> At(L), St(L);
    for (int t = 0; t < L; t++) {
      int nxt = nextVisit[t];
      At[t] = (nxt >= 0 ? ct[nxt] : 0);
      St[t] = qt[(t + 1) % L];
    }

    rules.clear();
    rules.reserve(L);
    for (int t = 0; t < L; t++) {
      rules.push_back({ct[t], qt[t], At[t], St[t], moves[t]});
    }
    M = L;
  }

  // 周期パス + original order (0,1,2,...,K-1) で T ステップ以内に全目的地を順に訪問できるかチェック
  bool check_feasible(const vector<pair<int,int>>& cycCells,
                      const string& cycMoves) {
    int L = (int)cycMoves.size();
    if (L == 0) return false;

    vector<int> cellId(N * N, -1);
    rep(k, K) {
      auto [x, y] = targets[k];
      cellId[x * N + y] = k;
    }

    int goal = 1; // 0番目はスタートで時刻0に到達済み
    for (int t = 0; t <= T; t++) {
      auto [x, y] = cycCells[t % L];
      int id = cellId[x * N + y];
      if (id == goal) {
        goal++;
        if (goal == K) return true;
      }
    }
    return false;
  }

  // ===== [1..x], [x+1..K-1] の 2ブロックを exact DP で最適 merge =====
  pair<vector<int>, long long> optimize_ord_block_2_exact() {
    vector<int> best_ord;
    long long best_cost = (long long)4e18;
    if (K <= 2) return {best_ord, best_cost};

    const long long INF = (long long)4e18;
    static long long dpA[405][405];
    static long long dpB[405][405];
    static int preI[405][405][2];
    static int preJ[405][405][2];
    static int preT[405][405][2];

    for (int x = 1; x <= K-2; x++) {
      int len1 = x;
      int len2 = (K - 1) - x;
      if (len1 <= 0 || len2 <= 0) continue;

      for (int i = 0; i <= len1; i++) {
        for (int j = 0; j <= len2; j++) {
          dpA[i][j] = dpB[i][j] = INF;
          preI[i][j][0] = preJ[i][j][0] = preT[i][j][0] = -1;
          preI[i][j][1] = preJ[i][j][1] = preT[i][j][1] = -1;
        }
      }

      auto A = [&](int idx){ return idx; };     // 1..x
      auto B = [&](int idx){ return x + idx; }; // x+1 .. K-1

      if (len1 >= 1) {
        int node = A(1);
        dpA[1][0] = distT[0][node];
        preI[1][0][0] = 0; preJ[1][0][0] = 0; preT[1][0][0] = 2;
      }
      if (len2 >= 1) {
        int node = B(1);
        dpB[0][1] = distT[0][node];
        preI[0][1][1] = 0; preJ[0][1][1] = 0; preT[0][1][1] = 2;
      }

      for (int i = 0; i <= len1; i++) {
        for (int j = 0; j <= len2; j++) {
          if (i >= 1 && dpA[i][j] < INF) {
            int from = A(i);
            if (i+1 <= len1) {
              int to = A(i+1);
              long long nc = dpA[i][j] + distT[from][to];
              if (nc < dpA[i+1][j]) {
                dpA[i+1][j] = nc;
                preI[i+1][j][0] = i;
                preJ[i+1][j][0] = j;
                preT[i+1][j][0] = 0;
              }
            }
            if (j+1 <= len2) {
              int to = B(j+1);
              long long nc = dpA[i][j] + distT[from][to];
              if (nc < dpB[i][j+1]) {
                dpB[i][j+1] = nc;
                preI[i][j+1][1] = i;
                preJ[i][j+1][1] = j;
                preT[i][j+1][1] = 0;
              }
            }
          }
          if (j >= 1 && dpB[i][j] < INF) {
            int from = B(j);
            if (i+1 <= len1) {
              int to = A(i+1);
              long long nc = dpB[i][j] + distT[from][to];
              if (nc < dpA[i+1][j]) {
                dpA[i+1][j] = nc;
                preI[i+1][j][0] = i;
                preJ[i+1][j][0] = j;
                preT[i+1][j][0] = 1;
              }
            }
            if (j+1 <= len2) {
              int to = B(j+1);
              long long nc = dpB[i][j] + distT[from][to];
              if (nc < dpB[i][j+1]) {
                dpB[i][j+1] = nc;
                preI[i][j+1][1] = i;
                preJ[i][j+1][1] = j;
                preT[i][j+1][1] = 1;
              }
            }
          }
        }
      }

      long long bestCost = INF;
      int bestLastType = -1;
      if (dpA[len1][len2] < INF) {
        int lastNode = A(len1);
        long long total = dpA[len1][len2] + distT[lastNode][0];
        if (total < bestCost) {
          bestCost = total;
          bestLastType = 0;
        }
      }
      if (dpB[len1][len2] < INF) {
        int lastNode = B(len2);
        long long total = dpB[len1][len2] + distT[lastNode][0];
        if (total < bestCost) {
          bestCost = total;
          bestLastType = 1;
        }
      }
      if (bestLastType == -1) continue;

      vector<int> ord;
      int ci = len1, cj = len2, ct = bestLastType;
      while (true) {
        if (ct == 0) ord.push_back(A(ci));
        else         ord.push_back(B(cj));
        int pi = preI[ci][cj][ct];
        int pj = preJ[ci][cj][ct];
        int pt = preT[ci][cj][ct];
        if (pt == 2) break;
        ci = pi; cj = pj; ct = pt;
      }
      reverse(ord.begin(), ord.end());
      if ((int)ord.size() != K-1) continue;

      if (bestCost < best_cost) {
        best_cost = bestCost;
        best_ord = ord;
      }
    }
    return {best_ord, best_cost};
  }

  // ===== 任意 blocks をビームサーチで merge =====
  pair<vector<int>, long long> beam_merge_given_blocks(
      const vector<vector<int>>& blocks) {
    vector<int> best_ord;
    long long best_cost = (long long)4e18;

    int B = (int)blocks.size();
    if (B == 0) return {best_ord, best_cost};
    int nVal = 0;
    for (auto &v : blocks) nVal += (int)v.size();
    if (nVal == 0) return {best_ord, best_cost};

    struct BSState {
      vector<int> idx;   // 各ブロックの次インデックス
      int last;          // 最後に取ったラベル
      long long cost;    // 0→... の距離（まだ最後→0は含めない）
      vector<int> ord;   // 構築中の順列
    };

    vector<BSState> curStates, nextStates;
    curStates.reserve(BEAM_WIDTH);
    nextStates.reserve(BEAM_WIDTH * B);

    BSState init;
    init.idx.assign(B, 0);
    init.last = 0;   // スタート
    init.cost = 0;
    init.ord.clear();
    curStates.push_back(init);

    for (int step = 0; step < nVal; step++) {
      nextStates.clear();
      for (auto &st : curStates) {
        for (int b = 0; b < B; b++) {
          if (st.idx[b] >= (int)blocks[b].size()) continue;
          BSState ns = st;
          int val = blocks[b][ ns.idx[b] ];
          ns.idx[b]++;
          ns.ord.push_back(val);
          ns.cost += distT[ns.last][val];
          ns.last = val;
          nextStates.push_back(move(ns));
        }
      }
      if (nextStates.empty()) break;
      sort(nextStates.begin(), nextStates.end(),
           [](const BSState &a, const BSState &b){
             return a.cost < b.cost;
           });
      if ((int)nextStates.size() > BEAM_WIDTH) nextStates.resize(BEAM_WIDTH);
      curStates.swap(nextStates);
    }

    for (auto &st : curStates) {
      if ((int)st.ord.size() != nVal) continue;
      long long total = st.cost + distT[st.last][0];
      if (total < best_cost) {
        best_cost = total;
        best_ord = st.ord;
      }
    }
    return {best_ord, best_cost};
  }

  // ===== Bブロック (B>=3) で partition + merge をビームで最適化 =====
  pair<vector<int>, long long> optimize_ord_blocks_with_partitions(int B) {
    vector<int> best_ord;
    long long best_cost = (long long)4e18;
    int nVal = K - 1;
    if (nVal <= 0 || B <= 1) return {best_ord, best_cost};
    B = min(B, nVal);

    uniform_int_distribution<int> cutDist(1, nVal-1);

    int tries = PARTITION_TRIES_PER_B;
    for (int t = 0; t < tries; t++) {
      vector<int> cuts;

      if (t == 0) {
        // 均等 partition
        for (int i = 1; i < B; i++) {
          int c = (long long)i * nVal / B;
          if (c <= 0) c = 1;
          if (c >= nVal) c = nVal - 1;
          cuts.push_back(c);
        }
      } else {
        // ランダム partition
        while ((int)cuts.size() < B-1) {
          int c = cutDist(rng);
          if (find(cuts.begin(), cuts.end(), c) == cuts.end())
            cuts.push_back(c);
        }
        sort(cuts.begin(), cuts.end());
      }

      // cuts: 1..nVal-1 の中で B-1 個
      vector<vector<int>> blocks(B);
      int prev = 1;
      for (int i = 0; i < B-1; i++) {
        int c = cuts[i];
        if (c < prev) c = prev;
        if (i+1 == B-1 && c >= nVal) c = nVal - 1;
        for (int x = prev; x <= c; x++) blocks[i].push_back(x);
        prev = c + 1;
      }
      for (int x = prev; x <= nVal; x++) blocks[B-1].push_back(x);

      bool bad = false;
      for (int b = 0; b < B; b++) {
        if (blocks[b].empty()) { bad = true; break; }
      }
      if (bad) continue;

      auto info = beam_merge_given_blocks(blocks);
      auto &ord = info.first;
      long long cost = info.second;
      if ((int)ord.size() != nVal) continue;
      if (cost < best_cost) {
        best_cost = cost;
        best_ord = ord;
      }
    }

    return {best_ord, best_cost};
  }

  // ---- メインロジック ----
  void solve() {
    // distT[u][v] を前計算
    distT.assign(K, vector<int>(K, 0));
    vector<vector<int>> gridDist;
    rep(p, K) {
      auto [sx, sy] = targets[p];
      bfs_grid_from(sx, sy, gridDist);
      rep(q, K) {
        auto [tx, ty] = targets[q];
        distT[p][q] = gridDist[tx][ty];
      }
    }

    // identity を構築して基準解にする
    long long len_identity = build_identity_and_get_len(); // C,Q,M,s,rules が identity に
    long long bestScore = (long long)C + Q;

    // K<=1 の場合は他にやることがない
    if (K <= 1) return;

    // B=2..MAX_BLOCKS を全部試す
    int nVal = K - 1;
    int maxB = min(MAX_BLOCKS, nVal);

    for (int B = 2; B <= maxB; B++) {
      vector<int> ord;
      long long cost_cycle = (long long)4e18;

      if (B == 2) {
        auto info = optimize_ord_block_2_exact();
        ord = info.first;
        cost_cycle = info.second;
      } else {
        auto info = optimize_ord_blocks_with_partitions(B);
        ord = info.first;
        cost_cycle = info.second;
      }

      if (ord.empty()) continue;

      // ord に対する実際のパスを構築
      vector<pair<int,int>> cycCells;
      string cycMoves;
      build_cycle_from_ord(ord, cycCells, cycMoves);
      int L = (int)cycMoves.size();
      if (L == 0 || L > T) continue;

      bool ok = check_feasible(cycCells, cycMoves);
      if (!ok) continue;

      auto [c_cyc, q_cyc] = choose_CQ(L);
      long long score_cyc = (long long)c_cyc + q_cyc;
      if (score_cyc < bestScore) {
        bestScore = score_cyc;
        build_periodic_automaton(cycCells, cycMoves); // ここで C,Q,M,s,rules を上書き
      }
    }
  }

  void output() {
    cout << C << " " << Q << " " << M << "\n";
    rep(i, N) {
      rep(j, N) {
        if (j) cout << ' ';
        cout << s[i][j];
      }
      cout << "\n";
    }
    for (auto &r : rules) {
      cout << r.c << " " << r.q << " " << r.A << " " << r.S << " " << r.D << "\n";
    }
  }
};

int main() {
  cin.tie(nullptr);
  ios::sync_with_stdio(false);
  Solver solver;
  solver.solve();
  solver.output();
  return 0;
}
