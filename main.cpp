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
}

inline unsigned int rand_int() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
  unsigned int tt = (tx ^ (tx << 11));
  tx = ty, ty = tz, tz = tw;
  return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

inline double rand_double() {
  return (double) (rand_int() % (int) 1e9) / 1e9;
}

// 温度関数
#define TIME_LIMIT 1900
inline double temp(double start) {
  double start_temp = 1000, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now, int start) {
  return exp((double) (now - best) / temp(start));
}
//-----------------以下から実装部分-----------------//

using P = pair<int, int>;

struct Point {
  int x, y;
  Point(): x(0), y(0) {}
  Point(int _x, int _y): x(_x), y(_y) {}
  Point operator+(const Point& other) const {return Point(x + other.x, y + other.y);}
  Point operator-(const Point& other) const {return Point(x - other.x, y - other.y);}
  Point operator*(const int& k)       const {return Point(x * k, y * k);}
  bool operator==(const Point& other) const {return x == other.x && y == other.y;}
  bool operator!=(const Point& other) const {return x != other.x || y != other.y;}
  bool operator<(const Point& other)  const {return x != other.x ? x < other.x : y < other.y;}
  bool operator>(const Point& other)  const {return x != other.x ? x > other.x : y > other.y;}
  bool operator<=(const Point& other) const {return x != other.x ? x < other.x : y <= other.y;}
  bool operator>=(const Point& other) const {return x != other.x ? x > other.x : y >= other.y;}
};

struct Rect {
  int x1, x2, y1, y2;
	Point ul, dl, ur, dr;
	vector<Point> points;
  Rect(int x1, int x2, int y1, int y2) : x1(x1), x2(x2), y1(y1), y2(y2) {
		ul = Point(x1, y1);
		dl = Point(x2, y1);
		ur = Point(x1, y2);
		dr = Point(x2, y2);
		points.emplace_back(ul);
		points.emplace_back(dl);
		points.emplace_back(ur);
		points.emplace_back(dr);
		return;
	}
	inline bool intersect(const Rect &other) {
		return !(ul.x > other.dr.x || ur.x < other.dl.x || ul.y > other.dr.y || dl.y < other.ul.y);
	}
};

// 入力関係
int N, M, Q, L, W;
vector<int> G;
vector<Rect> rects;

struct Solver {
	vector<vector<int>> ans_group;
	vector<int> len_upper, len_lower;
	set<P> len_sort;

  Solver() {
    this->input();
		ans_group.assign(M, vector<int>{});
		len_upper.resize(N * N, -1e9);
		len_lower.resize(N * N,  1e9);
		rep(i, N) for(int j = i + 1; j < N; j++) {
			for(auto &r1 : rects[i].points) for(auto &r2 : rects[j].points) {
				Point delta = r1 - r2;
				int len = ceil(sqrt(delta.x * delta.x + delta.y * delta.y));
				len_upper[i * N + j] = max(len_upper[i * N + j], len);
				len_lower[i * N + j] = min(len_lower[i * N + j], len);
			}
			if(rects[i].intersect(rects[j])) len_lower[i * N + j] = 0;
			len_upper[j * N + i] = len_upper[i * N + j];
			len_lower[j * N + i] = len_lower[i * N + j];
		}
		utility::mytm.CodeStart();
    return;
  }

  void input() {
    cin >> N >> M >> Q >> L >> W;
    G.resize(M);
    rep(i, M) cin >> G[i];
    rep(i, N) {
      int x1, x2, y1, y2;
      cin >> x1 >> x2 >> y1 >> y2;
      rects.emplace_back(Rect(x1, x2, y1, y2));
    }
    return;
  }

  void output() {
		// ans_group ごとに最小全域木を作成して出力
		dsu uf(N);
		cout << "!" << '\n';
		rep(i, M) {
			rep(j, ans_group[i].size()) cout << ans_group[i][j] << ' ';
			cout << '\n' << flush;

			vector<int> edges;
			rep(j, ans_group[i].size()) rep(k, ans_group[i].size()) {
				if(j >= k) continue;
				int nj = ans_group[i][j], nk = ans_group[i][k];
				edges.emplace_back(min(nj, nk) * N + max(nj, nk));
			}
			sort(edges.begin(), edges.end(), [&](int e1, int e2) {
				if(len_sort.count(P(e1, e2)) > 0) return true;
				if(len_sort.count(P(e2, e1)) > 0) return false;
				return len_upper[e1] + len_lower[e1] < len_upper[e2] + len_lower[e2];
			});
			for(auto idx : edges) {
				auto [v1, v2] = P(idx / N, idx % N);
				if(uf.same(v1, v2)) continue;
				cout << v1 << ' ' << v2 << '\n';
				uf.merge(v1, v2);
			}
		}
    return;
  }

  void solve() {
    // 1. 占いにより辺の長さの制約を削る
		vector<int> group;
		rep(_, Q) {
			group.clear();
			vector<bool> used(N, false);
			while(group.size() < L) {
				int idx = rand_int() % N;
				if(used[idx]) continue;
				group.emplace_back(idx);
				used[idx] = true;
			}
			bind_edge(group);
		}
		cerr << "len_sort.size() = " << len_sort.size() << '\n';

    // 2. 貪欲でグループ分け
		vector<int> vertex(N);
		iota(vertex.begin(), vertex.end(), 0);
		sort(vertex.begin(), vertex.end(), [&](int i, int j) {
			int e1 = rects[i].x1 + rects[i].x2 + rects[i].y1 + rects[i].y2;
			int e2 = rects[j].x1 + rects[j].x2 + rects[j].y1 + rects[j].y2;
			return e1 < e2;
		});

		vector<bool> used(N, false);
		vector<int> cand_edges;
		int group_idx = 0;
		rep(i, N) {
			if(used[vertex[i]]) continue;
			used[vertex[i]] = true;
			ans_group[group_idx].emplace_back(vertex[i]);
			cand_edges.clear();
			rep(j, N) {
				if(used[j]) continue;
				cand_edges.emplace_back(min(vertex[i], j) * N + max(vertex[i], j));
			}
			sort(cand_edges.begin(), cand_edges.end(), [&](int e1, int e2) {
				if(len_sort.count(P(e1, e2)) > 0) return true;
				if(len_sort.count(P(e2, e1)) > 0) return false;
				return len_upper[e1] + len_lower[e1] < len_upper[e2] + len_lower[e2];
			});
			rep(j, G[group_idx] - 1) {
				auto [v1, v2] = P(cand_edges[j] / N, cand_edges[j] % N);
				int idx = (v1 == vertex[i]) ? v2 : v1;
				used[idx] = true;
				ans_group[group_idx].emplace_back(idx);
			}
			group_idx++;
		}
    return;
  }

	// 占いをする関数
	inline vector<P> divination(const vector<int> &group) {
		cout << "? " << group.size() << ' ';
		for(auto &g : group) cout << g << ' ';
		cout << '\n' << flush;
		vector<P> res;
		rep(i, group.size() - 1) {
			int a, b;
			cin >> a >> b;
			res.emplace_back(a, b);
		}
		return res;
	}

	// 辺情報を占いで束縛する関数
	inline void bind_edge(const vector<int> &group) {
		vector<P> div = divination(group);
		vector<vector<int>> graph(N);
		rep(i, div.size()) {
			auto [di, dj] = div[i];
			graph[di].emplace_back(dj);
			graph[dj].emplace_back(di);
		}
		queue<int> que;
		vector<bool> visited(N, false);
		rep(i, div.size()) {
			auto [di, dj] = div[i];
			visited.assign(N, false);
			que.push(di);
			visited[di] = true;
			while(!que.empty()) {
				int now = que.front(); que.pop();
				for(auto &next : graph[now]) {
					if(visited[next] || next == dj) continue;
					visited[next] = true;
					que.push(next);
				}
			}
			rep(j, group.size()) rep(k, group.size()) {
				if(visited[group[j]] == visited[group[k]]) continue;
				int e1 = min(di, di) * N + max(di, dj);
				int e2 = min(group[j], group[k]) * N + max(group[j], group[k]);
				len_sort.insert(P(e1, e2));
			}
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