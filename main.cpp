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

#define TIME_LIMIT 1900
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
		utility::mytm.CodeStart();

		ans_group.assign(M, vector<int>{});
		len_upper.resize(N * N,    0);
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
		utility::mytm.CodeStart();
    return;
  }

	pair<int, vector<P>> compute_mst(const vector<int>& group) {
		vector<tuple<int, int, int>> edges;
		int sz = group.size();
		for(int i = 0; i < sz; i++) {
			for(int j = i + 1; j < sz; j++) {
				int u = group[i], v = group[j];
				int idx = min(u, v) * N + max(u, v);
				int cost = len_upper[idx] + len_lower[idx];
				edges.emplace_back(cost, u, v);
			}
		}
		sort(edges.begin(), edges.end(), [&](auto a, auto b) {
			return get<0>(a) < get<0>(b);
		});
		dsu uf(N);
		vector<P> mst_edges;
		int total_cost = 0;
		for(auto &edge : edges) {
			auto [cost, u, v] = edge;
			if(uf.same(u, v)) continue;
			uf.merge(u, v);
			int delta_x = abs(rects[u].x1 - rects[v].x1);
			int delta_y = abs(rects[u].y1 - rects[v].y1);
			int dist = ceil(sqrt(delta_x * delta_x + delta_y * delta_y));
			total_cost += dist;
			mst_edges.emplace_back(u, v);
		}
		return { total_cost, mst_edges };
	}

	// 占いをする関数 (※ 占い回数上限に達したら compute_mst を呼ぶ)
	int divination_cnt = 0;
	inline vector<P> divination(const vector<int> &group) {
		assert(1 < group.size() && group.size() <= L);
		if(divination_cnt >= Q) {
			auto [cost, mst_edges] = compute_mst(group);
			return mst_edges;
		}
		cout << "? " << group.size() << ' ';
		for(auto &g : group) cout << g << ' ';
		cout << "\n" << flush;
		vector<P> res;
		rep(i, group.size() - 1) {
			int a, b;
			cin >> a >> b;
			res.emplace_back(a, b);
		}
		divination_cnt++;
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
			while (!que.empty()) {
				int now = que.front();
				que.pop();
				for(auto &next : graph[now]) {
					if(visited[next] || next == dj) continue;
					visited[next] = true;
					que.push(next);
				}
			}
			rep(j, group.size()) rep(k, group.size()) {
				if(visited[group[j]] == visited[group[k]]) continue;
				int e1 = min(di, dj) * N + max(di, dj);
				int e2 = min(group[j], group[k]) * N + max(group[j], group[k]);
				len_sort.insert(P(e1, e2));
			}
		}
		return;
	}

	// 占いによる MST 構築を行う別関数
	vector<P> build_mst_with_divination(const vector<int>& group) {
		vector<P> total_edges;
		int n = group.size();
		if(n == 1) {
			// 何もしない
		} else if(n == 2) {
			// MST は一意に決まるので、そのまま辺を採用
			total_edges.emplace_back(group[0], group[1]);
		} else {
			const int step = L - 1;
			int start = 0;
			while(start < n - 1) {
				int remaining = n - start;
				if(remaining >= L) {
					vector<int> sub_group(group.begin() + start, group.begin() + start + L);
					auto sub_edges = divination(sub_group);
					total_edges.insert(total_edges.end(), sub_edges.begin(), sub_edges.end());
				} else if(remaining > 2) {
					vector<int> sub_group(group.begin() + n - min(L, n), group.end());
					auto sub_edges = divination(sub_group);
					total_edges.insert(total_edges.end(), sub_edges.begin(), sub_edges.end());
				} else {
					// 残りが 2 つの場合は、MST は一意に決まる
					total_edges.emplace_back(group[start], group[start + 1]);
				}
				start += step;
			}
		}
		dsu uf(N);
		vector<P> mst_edges;
		sort(total_edges.begin(), total_edges.end(), [&](auto a, auto b) {
			int a_idx = min(a.first, a.second) * N + max(a.first, a.second);
			int b_idx = min(b.first, b.second) * N + max(b.first, b.second);
			return len_upper[a_idx] + len_lower[a_idx] < len_upper[b_idx] + len_lower[b_idx];
		});
		for(auto &edge : total_edges) {
			auto [u, v] = edge;
			if(uf.same(u, v)) continue;
			uf.merge(u, v);
			int delta_x = abs(rects[u].x1 - rects[v].x1);
			int delta_y = abs(rects[u].y1 - rects[v].y1);
			int dist = ceil(sqrt(delta_x * delta_x + delta_y * delta_y));
			mst_edges.emplace_back(u, v);
		}
		return mst_edges;
	}

	void output() {
		vector<pair<vector<int>, vector<P>>> answers;
		for(auto &group : ans_group) {
			vector<P> mst_edges = build_mst_with_divination(group);
			answers.emplace_back(group, mst_edges);
		}
		cout << "!" << "\n";
		for(auto &p : answers) {
			auto [group, mst_edges] = p;
			for(auto id : group) cout << id << " ";
			cout << "\n" << flush;
			for(auto &[u, v] : mst_edges) cout << u << " " << v << "\n" << flush;
		}
		return;
	}

	void solve() {
		vector<int> init_rects(N);
		iota(init_rects.begin(), init_rects.end(), 0);
		int cell_width = 1000;
		map<int, vector<int>> groups_by_col;
		for(auto idx : init_rects) {
			int cx = (rects[idx].x1 + rects[idx].x2) / 2;
			int col = cx / cell_width;
			groups_by_col[col].push_back(idx);
		}
		vector<int> snake;
		for(auto &p : groups_by_col) {
			auto vec = p.second;
			sort(vec.begin(), vec.end(), [&](int a, int b) {
				int cy_a = (rects[a].y1 + rects[a].y2) / 2;
				int cy_b = (rects[b].y1 + rects[b].y2) / 2;
				return cy_a < cy_b;
			});
			if(p.first % 2 == 1) reverse(vec.begin(), vec.end());
			for(auto id : vec) snake.push_back(id);
		}
		vector<int> snake_order = snake;
		reverse(snake_order.begin(), snake_order.end());
		
		auto build_full_groups = [&](const vector<int>& group_sizes) -> vector<vector<int>> {
			vector<vector<int>> group_list(M);
			vector<int> prefix(M + 1, 0);
			for(int i = 0; i < M; i++) prefix[i + 1] = prefix[i] + group_sizes[i];
			for(int i = 0; i < M; i++) {
				int start = prefix[i];
				int sz = group_sizes[i];
				vector<int> group(snake_order.begin() + start, snake_order.begin() + start + sz);
				group_list[i] = group;
			}
			return group_list;
		};
		ans_group = build_full_groups(G);
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
