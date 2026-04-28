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

constexpr int N = 600;
constexpr int T = 600;

struct Operation {
  char op;
  int v, from, to;
  Operation(char _op, int _v): op(_op), v(_v), from(-1), to(-1) {}
  Operation(char _op, int _v, int _from, int _to): op(_op), v(_v), from(_from), to(_to) {}
};

struct Solver {
  // input
  int M, LA, LB;
  vector<vector<int>> graph;
  vector<int> perm;
  vector<pair<int, int>> place;

  vector<int> deg;

  vector<int> A;                // 配列 A の要素
  vector<int> vertex_clique_id; // 頂点が属するクリークの ID
  vector<int> clique_size;      // クリークのサイズ
  vector<int> clique_start_idx; // 配列 A におけるクリークの開始インデックス

  vector<Operation> answer;

  Solver() {
    this->input();
  }

  void input() {
    int damp;
    cin >> damp >> M >> damp >> LA >> LB;
    graph.assign(N, vector<int>{});
    perm.resize(T);
    place.resize(N);
    deg.assign(N, 0);

    rep(i, M) {
      int u, v;
      cin >> u >> v;
      graph[u].emplace_back(v);
      graph[v].emplace_back(u);
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
    rep(i, answer.size()) {
      if(answer[i].op == 'm') cout << answer[i].op << " " << answer[i].v << '\n';
      else cout << answer[i].op << " " << answer[i].v << " " << answer[i].from << " " << answer[i].to << '\n';
    }
    return;
  }

  void solve() {
    // 解法 : クリーク生成焼きなまし
    // 1. クリーク初期解を配列 A が一杯になるまで満遍なくカバーできるように作成
    // 2. 各頂点がクリークを介して色々な頂点に移動できるように焼きなまし
    // 3. 移動列の生成は各クリークを 1 頂点と見なして BFS + 経路復元で可能

    // 1. クリーク作成 (1 クリークの最大サイズは LB)
    int clique_id = 0;
    vertex_clique_id.assign(N, -1);
    clique_size.assign(N, 0);
    clique_start_idx.assign(N, 0);

    auto dfs = [&](auto self, int v, int pre) -> void {
      if(clique_size[clique_id] == LB) return;
      clique_size[clique_id]++;
      vertex_clique_id[v] = clique_id;
      A.emplace_back(v);

      for(auto &&u: graph[v]) {
        if(vertex_clique_id[u] != -1) continue;
        self(self, u, v);
      }
    };

    vector<int> priority(N, 0);
    iota(priority.begin(), priority.end(), 0);
    sort(priority.begin(), priority.end(), [&](int a, int b) {
      return deg[a] < deg[b];
    });

    rep(i, N) {
      if(vertex_clique_id[priority[i]] != -1) continue;
      clique_start_idx[clique_id] = A.size();
      dfs(dfs, priority[i], -1);
      clique_id++;
    }

    // 2. クリークを 1 頂点と見なして BFS をして条件達成を狙う
    // まずクリークを 1 頂点と見なした際のグラフを作成
    vector clique_graph(clique_id, vector<tuple<int, int, int>>{});
    rep(from, N) {
      for(auto &&to: graph[from]) {
        if(vertex_clique_id[from] == vertex_clique_id[to]) continue;
        clique_graph[vertex_clique_id[from]].emplace_back(tuple{vertex_clique_id[to], from, to});
        clique_graph[vertex_clique_id[to]].emplace_back(tuple{vertex_clique_id[from], to, from});
      }
    }

    // 予め全頂点間のクリーク内での最短経路 & 復元用の情報を求めておく
    vector<vector<int>> dist_base(N, vector<int>(N, INT_MAX));
    vector<vector<int>> prev_base(N, vector<int>(N, -1));
    rep(i, N) {
      queue<int> que;
      que.push(i);
      dist_base[i][i] = 0;
      while(!que.empty()) {
        int v = que.front();
        que.pop();
        for(auto &&u: graph[v]) {
          if(dist_base[i][u] != INT_MAX || vertex_clique_id[v] != vertex_clique_id[u]) continue;
          dist_base[i][u] = dist_base[i][v] + 1;
          prev_base[i][u] = v;
          que.push(u);
        }
      }
    }

    int start = 0;
    queue<int> que;
    // ※ 最初の 1 回だけはクリーク内移動が出来るように信号制御 (無駄な時もあるが、1 回なので一旦保留)
    answer.emplace_back(Operation{'s', clique_size[vertex_clique_id[start]], clique_start_idx[vertex_clique_id[start]], 0});

    vector<Operation> store;
    rep(i, T) {
      // クリークグラフでの最短経路を求めて復元
      vector dist(clique_id, INT_MAX);
      // tuple{前のクリーク, 辺の端点 (from), 辺の端点 (to)}
      vector prev(clique_id, tuple{-1, -1, -1});

      que.push(vertex_clique_id[start]);
      dist[vertex_clique_id[start]] = 0;
      while(!que.empty()) {
        int v = que.front();
        que.pop();
        if(vertex_clique_id[perm[i]] == v) break;

        for(auto &&[u, from, to]: clique_graph[v]) {
          if(dist[u] != INT_MAX) continue;
          dist[u] = dist[v] + 1;
          prev[u] = tuple{v, from, to};
          que.push(u);
        }
      }
      while(!que.empty()) que.pop();

      int now = perm[i];
      while(prev[vertex_clique_id[now]] != tuple{-1, -1, -1}) {
        auto [prev_clique, from, to] = prev[vertex_clique_id[now]];

        // 1. to -> now のクリーク内の最短経路復元
        int current = now, time = 0;
        while(current != to) {
          store.emplace_back(Operation{'m', current});
          current = prev_base[to][current];
          time++;
        }
        // 2. from -> to のクリーク間の最短経路復元
        store.emplace_back(Operation{'m', to});
        // 3. クリーク移動が出来るように信号制御
        store.emplace_back(Operation{'s', clique_size[vertex_clique_id[to]], clique_start_idx[vertex_clique_id[to]], 0});
        now = from;
      }
      // 4. start -> now のクリーク内の最短経路復元
      int current = now;
      while(current != start) {
        store.emplace_back(Operation{'m', current});
        current = prev_base[start][current];
      }
      // 5. reverse して answer に格納
      while(!store.empty()) {
        answer.emplace_back(store.back());
        store.pop_back();
      }
      store.clear();
      start = perm[i];
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
