#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
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
      return (double) duration_cast<milliseconds>(system_clock::now() - start).count();
    }
  } mytm;
} // namespace utility

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty, ty = tz, tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

// 温度関数
#define TIME_LIMIT 2800
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now, int start) {
  return exp((double) (now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

static const int N = 600;
static const int T = 600;

struct Operation {
  char op;
  int v, from, to;
  Operation(char _op, int _v): op(_op), v(_v), from(-1), to(-1) {}
  Operation(char _op, int _v, int _from, int _to): op(_op), v(_v), from(_from), to(_to) {}
};

struct Solver {
  // input
  int M, LA, LB, clique_id = 0;
  vector<vector<int>> graph;
  vector<vector<bool>> graph_matrix;
  vector<int> perm;
  vector<pair<int, int>> place;

  vector<int> deg;                  // 元グラフの次数

  vector<vector<tuple<int, int, int>>> clique_graph; // クリークを 1 頂点と見なしたグラフ
  vector<vector<int>> clique_group;                  // 各頂点が存在するクリークの id の集合
  vector<bitset<4*N*N>> clique_group_bit;            // 各頂点が存在するクリークの id の集合 (bitset)

  vector<int> A;                // 配列 A の要素
  vector<int> clique_size;      // クリークのサイズ
  vector<int> clique_start_idx; // 配列 A におけるクリークの開始インデックス

  vector<Operation> answer;

  Solver() {
    this->input();
    clique_group.assign(N, vector<int>{});
  }

  void input() {
    int damp;
    cin >> damp >> M >> damp >> LA >> LB;
    graph.assign(N, vector<int>{});
    graph_matrix.assign(N, vector<bool>(N, false));
    perm.resize(T);
    place.resize(N);
    deg.assign(N, 0);

    rep(i, M) {
      int u, v;
      cin >> u >> v;
      graph[u].emplace_back(v);
      graph[v].emplace_back(u);
      graph_matrix[u][v] = true;
      graph_matrix[v][u] = true;
      deg[u]++;
      deg[v]++;
    }
    rep(i, T) cin >> perm[i];
    rep(i, N) cin >> place[i].first >> place[i].second;
    return;
  }

  void output() {
    // 足りない時があれば 0 埋め
    while(A.size() < LA) A.emplace_back(0);
    rep(i, LA) cout << A[i] << " ";
    cout << '\n';

    int idx = 0;
    while(idx + LB < LA) {
      cout << "s " << LB << " " << idx << " " << 0 << '\n';
      idx += LB;
    }

    rep(i, answer.size()) {
      if(answer[i].op == 'm') cout << answer[i].op << " " << answer[i].v << '\n';
      else cout << answer[i].op << " " << answer[i].v << " " << answer[i].from << " " << answer[i].to << '\n';
    }
    return;
  }

  void solve() {
    // 解法 : クリーク生成
    // 1. クリーク初期解を配列 A が一杯になるまで満遍なくカバーできるように作成
    // 2. 移動列の生成は各クリークを 1 頂点と見なして BFS + 経路復元で可能

    // 1. 初期クリーク作成 part
    createClique();

    // 2. 移動列生成 part
    int start = 0, now_clique_id = clique_group[start][0];
    queue<int> que;
    vector<int> dist_base,  prev_base;

    // ※ 最初の 1 回だけはクリーク内移動が出来るように信号制御 (無駄な時もあるが、1 回なので一旦保留)
    answer.emplace_back(Operation{'s', clique_size[now_clique_id], clique_start_idx[now_clique_id], 0});

    vector<Operation> store;
    vector<int> dist, cand_clique;
    vector<tuple<int, int, int>> prev;
    rep(i, T) {
      // cerr << "i: " << i << '\n';
      // クリークグラフでの最短経路を求めて復元
      dist.assign(clique_id, INT_MAX);
      prev.assign(clique_id, tuple{-1, -1, -1}); // tuple{辺の端点 (from), 辺の端点 (to)}
      que.push(now_clique_id);
      dist[now_clique_id] = 0;

      // クリークを 1 頂点と見た時の経路復元の始点
      int prelim_now_cliique = now_clique_id, min_d = INT_MAX;
      cand_clique.clear();
      while(!que.empty()) {
        int v = que.front();
        que.pop();
        if(clique_group_bit[perm[i]].test(v)) {
          if(dist[v] <= min_d) {
            min_d = dist[v];
            cand_clique.emplace_back(v);
            prelim_now_cliique = v;
          }
          else break;
        }
        for(auto &&[u, from, to]: clique_graph[v]) {
          if(dist[u] != INT_MAX) continue;
          dist[u] = dist[v] + 1;
          prev[u] = tuple{v, from, to};
          que.push(u);
        }
      }
      while(!que.empty()) que.pop();

      // cand のクリークの内、更にその次のクリークに一番近いものを選択
      if(i + 1 != T) {
        dist.assign(clique_id, INT_MAX);
        queue<pair<int, int>> que_p;
        for(auto &&v: cand_clique) {
          dist[v] = 0;
          que_p.push(pair{v, v});
        }
        while(!que_p.empty()) {
          auto [u, from] = que_p.front();
          que_p.pop();
          if(clique_group_bit[perm[i + 1]].test(u)) {
            prelim_now_cliique = from;
            break;
          }
          for(auto &&[w, _, __]: clique_graph[u]) {
            if(dist[w] != INT_MAX) continue;
            dist[w] = dist[u] + 1;
            que_p.push(pair{w, from});
          }
        }
        while(!que_p.empty()) que_p.pop();
      }

      int store_prelim_now_cliique = prelim_now_cliique;

      int now_v = perm[i];
      while(prev[prelim_now_cliique] != tuple{-1, -1, -1}) {
        // 1. to -> now_v のクリーク内の最短経路復元
        // ※ 毎回クリーク内部の経路復元は逐次処理 (前計算すると後々クリークを焼く時に重い)
        auto [prev_clique, from, to] = prev[prelim_now_cliique];
        dist_base.assign(N, INT_MAX);
        prev_base.assign(N, -1);
        que.push(to);
        dist_base[to] = 0;
        while(!que.empty()) {
          int v = que.front();
          que.pop();
          if(v == now_v) break;
          for(auto &&u: graph[v]) {
            if(dist_base[u] != INT_MAX || !clique_group_bit[u].test(prelim_now_cliique)) continue;
            dist_base[u] = dist_base[v] + 1;
            prev_base[u] = v;
            que.push(u);
          }
        }
        while(!que.empty()) que.pop();
        int current_v = now_v;
        while(current_v != to) {
          store.emplace_back(Operation{'m', current_v});
          current_v = prev_base[current_v];
        }
        // 2. from -> to のクリーク間の最短経路復元
        if(from != to) store.emplace_back(Operation{'m', to});
        // 3. クリーク移動が出来るように信号制御
        store.emplace_back(Operation{'s', clique_size[prelim_now_cliique], clique_start_idx[prelim_now_cliique], 0});
        now_v = from;
        prelim_now_cliique = prev_clique;
      }

      // 4. start -> now のクリーク内の最短経路復元
      dist_base.assign(N, INT_MAX);
      prev_base.assign(N, -1);
      que.push(start);
      dist_base[start] = 0;
      while(!que.empty()) {
        int v = que.front();
        que.pop();
        if(v == now_v) break;
        for(auto &&u: graph[v]) {
          if(dist_base[u] != INT_MAX || !clique_group_bit[u].test(prelim_now_cliique)) continue;
          dist_base[u] = dist_base[v] + 1;
          prev_base[u] = v;
          que.push(u);
        }
      }
      while(!que.empty()) que.pop();
      int current = now_v;
      while(current != start) {
        store.emplace_back(Operation{'m', current});
        current = prev_base[current];
      }

      // 5. reverse して answer に格納
      while(!store.empty()) {
        answer.emplace_back(store.back());
        store.pop_back();
      }
      store.clear();
      start = perm[i];
      now_clique_id = store_prelim_now_cliique;
    }

    return;
  }

  void createClique() {
    // クリーク生成 part
    // ※ 基本的にクリークは LA*LA 個以上になることが無いので LA*LA で初期化
    clique_size.assign(LA*LA, 0);
    clique_start_idx.assign(LA*LA, 0);

    clique_group.assign(N, vector<int>{});
    clique_group_bit.assign(N, bitset<4*N*N>{});
    vector<bool> visited(N, false), used(N, false);
    used[0] = true;
    rep(i, T) used[perm[i]] = true;
    vector<int> replace_A;

    auto dfs = [&](auto self, int v) -> void {
      sort(graph[v].begin(), graph[v].end(), [&](int a, int b) {
        // くるくるしたクリークを作成したいため下記条件
        auto &&[ax, ay] = place[a];
        auto &&[bx, by] = place[b];
        return (ax - 500) * (ax - 500) + (ay - 500) * (ay - 500) > (bx - 500) * (bx - 500) + (by - 500) * (by - 500);
      });

      vector<int> tmp;
      for(auto &&u: graph[v]) {
        if(visited[u] || (deg[u] <= 1 && !used[u])) continue;
        visited[u] = true;
        tmp.emplace_back(u);
        self(self, u);
      }
      int tmp_size = tmp.size();
      while(!tmp.empty()) {
        (tmp_size >= LA ? A : replace_A).emplace_back(tmp.back());
        tmp.pop_back();
      }
    };

    // ========== 1. 全頂点を訪問する経路を DFS で縦長に構築 ========== //
    vector<int> priority(N);
    iota(priority.begin(), priority.end(), 0);
    sort(priority.begin(), priority.end(), [&](int a, int b) {
      auto &&[ax, ay] = place[a];
      auto &&[bx, by] = place[b];
      return ax + ay < bx + by;
    });
    dfs(dfs, priority[0]);

    // replace_A は A に適切に入れ直してあげる (孤立点である可能性が高いので)
    vector<int> tmp_A;
    int replace_size = replace_A.size();
    used.assign(N, false);
    for(int i = 0; i < A.size(); i++) {
      tmp_A.emplace_back(A[i]);
      for(int j = 0; j < replace_size; j++) {
        if(graph_matrix[A[i]][replace_A[j]] && !used[replace_A[j]]) {
          tmp_A.emplace_back(replace_A[j]);
          used[replace_A[j]] = true;
          break;
        }
      }
    }
    for(int i = 0; i < replace_size; i++) if(!used[replace_A[i]]) tmp_A.emplace_back(replace_A[i]);
    swap(A, tmp_A);
    // cerr << "Done createPath" << '\n';
    // cerr << "A.size() : " << A.size() << '\n';

    // ========== 十字に切れ込みクリークを入れる ========== //
    vector<int> dist(N, INT_MAX), prev(N, -1);
    queue<int> que;
    int sidx = 0, gidx = 0, smin = INT_MAX, gmin = INT_MAX;
    rep(i, N) {
      auto &&[x, y] = place[i];
      int dis = (x - 0) * (x - 0) + (y - 500) * (y - 500);
      if(dis < smin) smin = dis, sidx = i;
      dis = (x - 1000) * (x - 1000) + (y - 500) * (y - 500);
      if(dis < gmin) gmin = dis, gidx = i;
    }
    que.push(gidx);
    dist[gidx] = 0;
    while(!que.empty()) {
      int v = que.front();
      que.pop();
      if(v == sidx) break;
      for(auto &&u: graph[v]) {
        if(dist[u] != INT_MAX) continue;
        dist[u] = dist[v] + 1;
        prev[u] = v;
        que.push(u);
      }
    }
    while(!que.empty()) que.pop();
    int current = sidx;
    while(current != gidx) {
      A.emplace_back(current);
      current = prev[current];
    }
    A.emplace_back(gidx);
    // cerr << "A.size() : " << A.size() << '\n';

    smin = INT_MAX, gmin = INT_MAX;
    rep(i, N) {
      auto &&[x, y] = place[i];
      int dis = (x - 500) * (x - 500) + (y - 0) * (y - 0);
      if(dis < smin) smin = dis, sidx = i;
      dis = (x - 500) * (x - 500) + (y - 1000) * (y - 1000);
      if(dis < gmin) gmin = dis, gidx = i;
    }
    dist.assign(N, INT_MAX);
    prev.assign(N, -1);
    que.push(gidx);
    dist[gidx] = 0;
    while(!que.empty()) {
      int v = que.front();
      que.pop();
      if(v == sidx) break;
      for(auto &&u: graph[v]) {
        if(dist[u] != INT_MAX) continue;
        dist[u] = dist[v] + 1;
        prev[u] = v;
        que.push(u);
      }
    }
    while(!que.empty()) que.pop();
    current = sidx;
    while(current != gidx) {
      A.emplace_back(current);
      current = prev[current];
    }
    A.emplace_back(gidx);
    // cerr << "A.size() : " << A.size() << '\n';

    // ========== 2. 配列 A の残り部分で特定の間の最短経路を配列 A に追加 ========== //
    priority.assign(T - 1, 0);
    iota(priority.begin(), priority.end(), 0);
    sort(priority.begin(), priority.end(), [&](int a, int b) {
      auto &&[ax1, ay1] = place[perm[a]];
      auto &&[ax2, ay2] = place[perm[a + 1]];
      auto &&[bx1, by1] = place[perm[b]];
      auto &&[bx2, by2] = place[perm[b + 1]];
      return (ax1 - ax2) * (ax1 - ax2) + (ay1 - ay2) * (ay1 - ay2) > (bx1 - bx2) * (bx1 - bx2) + (by1 - by2) * (by1 - by2);
    });
    rep(i, T - 1) {
      int from = perm[priority[i]], to = perm[priority[i + 1]];
      dist.assign(N, INT_MAX);
      prev.assign(N, -1);
      que.push(from);
      dist[from] = 0;
      while(!que.empty()) {
        int v = que.front();
        que.pop();
        if(v == to) break;
        for(auto &&u: graph[v]) {
          if(dist[u] != INT_MAX) continue;
          dist[u] = dist[v] + 1;
          prev[u] = v;
          que.push(u);
        }
      }
      while(!que.empty()) que.pop();
      int current = to;
      while(current != from) {
        A.emplace_back(current);
        current = prev[current];
      }
      if(A.size() >= LA) break;
    }
    while(A.size() > LA) A.pop_back();

    // ========== 3. 1, 2 で作成した経路からクリークを生成 ========== //
    dsu uf;
    vector<int> comp;
    for(auto size : {LB, 1}) {
      for(int start = 0; start < A.size() - size + 1; start++) {
        // 既に大きいクリークでカバーされている場合はスキップ
        if(size == 1 && !clique_group[A[start]].empty()) continue;
        int end = start + size;
        clique_size[clique_id] = size;

        comp.clear();
        for(int i = start; i < end; i++) comp.emplace_back(A[i]);
        sort(comp.begin(), comp.end());
        comp.erase(unique(comp.begin(), comp.end()), comp.end());
        if(size != 1) {
          // クリークは連結でなければならないので判定
          uf = dsu(size);
          for(int i = start; i < end; i++) {
            for(int j = i + 1; j < end; j++) {
              if(!graph_matrix[A[i]][A[j]]) continue;
              int idx1 = lower_bound(comp.begin(), comp.end(), A[i]) - comp.begin();
              int idx2 = lower_bound(comp.begin(), comp.end(), A[j]) - comp.begin();
              uf.merge(idx1, idx2);
            }
          }
          if(uf.size(0) != size) continue;
        }

        for(int i = start; i < end; i++) {
          clique_group[A[i]].emplace_back(clique_id);
          clique_group_bit[A[i]].set(clique_id);
        }
        clique_start_idx[clique_id] = start;
        clique_id++;
      }
    }
    // cerr << "Done createClique" << '\n';

    // ========== 4. クリークを 1 頂点と見なしたグラフを作成 ========== //
    clique_graph.assign(clique_id, vector<tuple<int, int, int>>{});
    rep(from, N) for(auto &&to: graph[from]) {
      // 自分の頂点では行けない隣接クリーク列挙
      for(auto &&from_clique_id : clique_group[from]) for(auto &&to_clique_id : clique_group[to]) {
        if(from_clique_id == to_clique_id) continue;
        clique_graph[from_clique_id].emplace_back(tuple{to_clique_id, from, to});
        clique_graph[to_clique_id].emplace_back(tuple{from_clique_id, to, from});
      }
    }
    rep(i, N) for(auto &&from : clique_group[i]) for(auto &&to : clique_group[i]) {
      // 自分の頂点でも行けるクリークに辺を張る
      if(from == to) continue;
      clique_graph[from].emplace_back(tuple{to, i, i});
      clique_graph[to].emplace_back(tuple{from, i, i});
    }
    rep(i, clique_id) {
      // 重複削除
      sort(clique_graph[i].begin(), clique_graph[i].end());
      clique_graph[i].erase(unique(clique_graph[i].begin(), clique_graph[i].end()), clique_graph[i].end());
    }
    // cerr << "Done createCliqueGraph" << '\n';
    // cerr << "clique_num: " << clique_id << '\n';
    
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
