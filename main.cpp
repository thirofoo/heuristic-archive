#include <atcoder/all>
#include <bits/stdc++.h>
using namespace std;
using namespace atcoder;

#define rep(i, n) for (int i = 0; i < (n); i++)

constexpr int TIME_LIMIT = 1900;
constexpr int GRID = 250;
constexpr int FIELD_SIZE = 10000;

namespace utility {
	struct Timer {
		chrono::system_clock::time_point start;
		void startTimer() { start = chrono::system_clock::now(); }
		double elapsed() const {
			using namespace std::chrono;
			return double(duration_cast<milliseconds>(system_clock::now() - start).count());
		}
	} timer;
}

inline unsigned int rand_int() {
	static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
	unsigned int tt = (tx ^ (tx << 11));
	tx = ty; ty = tz; tz = tw;
	return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

using P = pair<int, int>;
using T = tuple<int, int, double>;

int N, M, Q, L, W;
vector<int> G;

struct Point {
	int x, y;
	Point() : x(0), y(0) {}
	Point(int _x, int _y) : x(_x), y(_y) {}
	Point operator+(const Point &other) const { return Point(x + other.x, y + other.y); }
	Point operator-(const Point &other) const { return Point(x - other.x, y - other.y); }
	Point operator*(const int &k) const { return Point(x * k, y * k); }
	bool operator==(const Point &other) const { return x == other.x && y == other.y; }
	bool operator!=(const Point &other) const { return !(*this == other); }
	bool operator<(const Point &other) const { return (x != other.x ? x < other.x : y < other.y); }
};

struct Rect {
	int x1, x2, y1, y2;
	Point ul, dl, ur, dr;
	vector<T> existProb;

	Rect(int _x1, int _x2, int _y1, int _y2)
		: x1(_x1), x2(_x2), y1(_y1), y2(_y2),
			ul(_x1, _y1), dl(_x2, _y1), ur(_x1, _y2), dr(_x2, _y2) {
		double areaSum = 0.0;
		int nGridX = FIELD_SIZE / GRID;
		int nGridY = FIELD_SIZE / GRID;
		rep(i, nGridX) {
			rep(j, nGridY) {
				int cell_x1 = max(i * GRID, x1);
				int cell_x2 = min((i + 1) * GRID, x2);
				int cell_y1 = max(j * GRID, y1);
				int cell_y2 = min((j + 1) * GRID, y2);
				if (cell_x1 >= cell_x2 || cell_y1 >= cell_y2) continue;
				double area = (cell_x2 - cell_x1) * (cell_y2 - cell_y1);
				existProb.emplace_back((cell_x1 + cell_x2) / 2, (cell_y1 + cell_y2) / 2, area);
				areaSum += area;
			}
		}
		for (auto &elem : existProb) {
			auto &[cx, cy, prob] = elem;
			prob /= areaSum;
		}
		assert(fabs(areaSum - (x2 - x1) * (y2 - y1)) < 1e-9);
	}

	Point getExpectation() const {
		if (existProb.empty())
			return Point((x1 + x2) / 2, (y1 + y2) / 2);
		Point expPt(0, 0);
		for (const auto &elem : existProb) {
			auto [cx, cy, prob] = elem;
			expPt.x += cx * prob;
			expPt.y += cy * prob;
		}
		return expPt;
	}

	void updateProb(const Rect &r2, const Rect &r3) {
		map<Point, double> newProb;
		double totalNewProb = 0.0;
		for (const auto &[x1, y1, prob1] : existProb) {
			double condProb = 0.0;
			for (const auto &[x2, y2, prob2] : r2.existProb)
				for (const auto &[x3, y3, prob3] : r3.existProb) {
					long long d1 = (long long)(x1 - x2) * (x1 - x2) + (long long)(y1 - y2) * (y1 - y2);
					long long d2 = (long long)(x1 - x3) * (x1 - x3) + (long long)(y1 - y3) * (y1 - y3);
					if (d1 <= d2)
						condProb += prob2 * prob3;
				}
			double updated = prob1 * condProb;
			if (updated > 1e-12) {
				newProb[Point(x1, y1)] += updated;
				totalNewProb += updated;
			}
		}
		if (totalNewProb < 1e-12) return;
		existProb.clear();
		for (const auto &[pt, prob] : newProb)
			existProb.emplace_back(pt.x, pt.y, prob / totalNewProb);
	}

	double getVariance() const {
		if (existProb.size() <= 1) return 0.0;
		Point expPt = getExpectation();
		double variance = 0.0;
		for (const auto& [cx, cy, prob] : existProb) {
			double dx = double(cx) - expPt.x;
			double dy = double(cy) - expPt.y;
			variance += prob * (dx * dx + dy * dy);
		}
		return variance;
	}
};

vector<Rect> rects;

struct Solver {
	vector<vector<int>> ansGroup;
	int divinationCount = 0;
	vector<bool> used;

	Solver() {
		input();
		utility::timer.startTimer();
		ansGroup.assign(M, vector<int>{});
	}

	void input() {
		utility::timer.startTimer();
		cin >> N >> M >> Q >> L >> W;
		G.resize(M);
		rep(i, M) cin >> G[i];
		rep(i, N) {
			int x1, x2, y1, y2;
			cin >> x1 >> x2 >> y1 >> y2;
			rects.emplace_back(x1, x2, y1, y2);
		}
	}

	vector<P> queryDivination(const vector<int> &group) {
		assert(group.size() > 1 && group.size() <= (size_t)L);
		assert(divinationCount < Q);
		cout << "? " << group.size() << " ";
		for (auto g : group) cout << g << " ";
		cout << "\n" << flush;
		vector<P> res;
		for (size_t i = 0; i < group.size() - 1; i++) {
			int a, b;
			cin >> a >> b;
			res.emplace_back(a, b);
		}
		divinationCount++;
		return res;
	}

	vector<int> selectGroup() {
		if (used.empty()) used.resize(N, false);
		vector<pair<double, int>> varianceScores;
		rep(j, N) {
			if (used[j]) continue;
			varianceScores.emplace_back(rects[j].getVariance(), j);
		}
		sort(varianceScores.rbegin(), varianceScores.rend());
		int core = (varianceScores.empty() ? rand_int() % N : varianceScores[0].second);
		used[core] = true;
		vector<int> group = { core };
		Point expCore = rects[core].getExpectation();
		vector<pair<long long, int>> neighbors;
		rep(j, N) {
			if (j == core) continue;
			Point expOther = rects[j].getExpectation();
			long long dsq = (long long)(expCore.x - expOther.x) * (expCore.x - expOther.x)
				+ (long long)(expCore.y - expOther.y) * (expCore.y - expOther.y);
			neighbors.emplace_back(dsq, j);
		}
		sort(neighbors.begin(), neighbors.end());
		for (const auto &[dsq, nb] : neighbors) {
			if (group.size() >= (size_t)L) break;
			group.emplace_back(nb);
		}
		return group;
	}

	void processMSTEdges(const vector<P> &mstEdges, const vector<int>& group) {
		set<int> nodes;
		for (const auto &[u, v] : mstEdges) {
			nodes.insert(u); nodes.insert(v);
		}
		for (int id : group) nodes.insert(id);
		vector<int> idx(nodes.begin(), nodes.end());
		map<int, int> comp;
		rep(i, idx.size()) comp[idx[i]] = i;
		vector<P> compEdges;
		for (const auto &edge : mstEdges)
			compEdges.emplace_back(comp[edge.first], comp[edge.second]);
		for (const auto &edge : compEdges) {
			int u = edge.first, v = edge.second;
			dsu uf(idx.size());
			for (const auto &[nu, nv] : compEdges) {
				if ((nu == u && nv == v) || (nu == v && nv == u))
					continue;
				if (!uf.same(nu, nv))
					uf.merge(nu, nv);
			}
			auto groups = uf.groups();
			if (groups.size() != 2) continue;
			int leaderU = uf.leader(u);
			vector<int> groupU, groupV;
			for (const auto &g : groups)
				(uf.leader(g[0]) == leaderU ? groupU : groupV) = g;
			if (groupU.empty() || groupV.empty()) continue;
			int origU = idx[u], origV = idx[v];
			for (int cu : groupU) {
				int orig = idx[cu];
				if (orig == origU || orig == origV) continue;
				rects[origV].updateProb(rects[origU], rects[orig]);
			}
			for (int cv : groupV) {
				int orig = idx[cv];
				if (orig == origU || orig == origV) continue;
				rects[origU].updateProb(rects[origV], rects[orig]);
			}
		}
	}

	const int HILBERT_ORDER = 14;
	uint64_t get_hilbert_order(int x, int y) {
		const int n = 1 << HILBERT_ORDER;
		uint64_t d = 0;
		x = max(0, min(n - 1, x * n / FIELD_SIZE));
		y = max(0, min(n - 1, y * n / FIELD_SIZE));
		for (int s = n / 2; s > 0; s /= 2) {
			int rx = (x & s) > 0;
			int ry = (y & s) > 0;
			d += (uint64_t)s * s * ((3 * rx) ^ ry);
			if (ry == 0) {
				if (rx == 1) {
					x = (s - 1) - x;
					y = (s - 1) - y;
				}
				swap(x, y);
			}
		}
		return d;
	}

	vector<vector<int>> buildGroups() {
		if (M <= 100) {
			vector<int> perm(N);
			iota(perm.begin(), perm.end(), 0);
			int cellWidth = FIELD_SIZE / 10;
			map<int, vector<int>> groupsByCol;
			for (int id : perm) {
				auto [cx, cy] = rects[id].getExpectation();
				groupsByCol[cx / cellWidth].push_back(id);
			}
			vector<int> snakeOrder;
			for (auto &p : groupsByCol) {
				auto vec = p.second;
				sort(vec.begin(), vec.end(), [&](int a, int b) {
					auto [ax, ay] = rects[a].getExpectation();
					auto [bx, by] = rects[b].getExpectation();
					return ay < by;
				});
				if (p.first % 2 == 1)
					reverse(vec.begin(), vec.end());
				snakeOrder.insert(snakeOrder.end(), vec.begin(), vec.end());
			}
			vector<int> snake = snakeOrder;
			reverse(snake.begin(), snake.end());
			vector<vector<int>> groups(M);
			vector<int> prefix(M + 1, 0);
			rep(i, M)
				prefix[i + 1] = prefix[i] + G[i];
			rep(i, M)
				groups[i] = vector<int>(snake.begin() + prefix[i], snake.begin() + prefix[i + 1]);
			return groups;
		}

		vector<vector<int>> groups(M);
		vector<Point> expectations(N);
		rep(i, N)
			expectations[i] = rects[i].getExpectation();

		const int MAX_ITERATIONS = 3;
		mt19937 engine(rand_int());
		vector<Point> centroids(M);
		vector<double> min_dist_sq(N, numeric_limits<double>::max());
		if (N > 0) {
			int first_centroid_idx = rand_int() % N;
			centroids[0] = expectations[first_centroid_idx];
			for (int j = 1; j < M; ++j) {
				double dist_sq_sum = 0.0;
				for (int i = 0; i < N; ++i) {
					long long dx = expectations[i].x - centroids[j - 1].x;
					long long dy = expectations[i].y - centroids[j - 1].y;
					min_dist_sq[i] = min(min_dist_sq[i], (double)dx * dx + dy * dy);
					dist_sq_sum += min_dist_sq[i] + 1e-9;
				}
				if(dist_sq_sum < 1e-9) {
					for(int k=j; k<M; ++k)
						centroids[k] = Point(rand_int()%FIELD_SIZE, rand_int()%FIELD_SIZE);
					break;
				}
				uniform_real_distribution<double> dist(0.0, dist_sq_sum);
				double r = dist(engine);
				double current_sum = 0.0;
				int next_centroid_idx = N - 1;
				for (int i = 0; i < N; ++i) {
					current_sum += min_dist_sq[i] + 1e-9;
					if (r <= current_sum) {
						next_centroid_idx = i;
						break;
					}
				}
				centroids[j] = expectations[next_centroid_idx];
				min_dist_sq[next_centroid_idx] = 0;
			}
		} else {
			rep(j, M)
				centroids[j] = Point(FIELD_SIZE/2, FIELD_SIZE/2);
		}

		vector<vector<int>> groupsAssign;
		for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
			if (utility::timer.elapsed() > TIME_LIMIT) break;
			mcf_graph<int, long long> mcf(N + M + 2);
			int s = N + M, t = N + M + 1;
			for (int i = 0; i < N; ++i)
				mcf.add_edge(s, i, 1, 0);
			for (int j = 0; j < M; ++j)
				if (G[j] > 0)
					mcf.add_edge(N + j, t, G[j], 0);
			for (int i = 0; i < N; ++i) {
				for (int j = 0; j < M; ++j) {
					if (G[j] == 0) continue;
					long long dx = expectations[i].x - centroids[j].x;
					long long dy = expectations[i].y - centroids[j].y;
					long long cost = dx * dx + dy * dy;
					mcf.add_edge(i, N + j, 1, cost);
				}
			}
			auto result = mcf.flow(s, t, N);
			if (result.first < N) break;
			vector<vector<int>> current_groups(M);
			vector<mcf_graph<int, long long>::edge> edges = mcf.edges();
			for (const auto& edge : edges)
				if (edge.from >= 0 && edge.from < N && edge.to >= N && edge.to < N + M && edge.flow == 1)
					current_groups[edge.to - N].push_back(edge.from);
			groupsAssign = current_groups;
			vector<Point> next_centroids(M);
			for (int j = 0; j < M; ++j) {
				if (current_groups[j].empty())
					next_centroids[j] = centroids[j];
				else {
					long long sum_x = 0, sum_y = 0;
					for (int point_id : current_groups[j]) {
						sum_x += expectations[point_id].x;
						sum_y += expectations[point_id].y;
					}
					next_centroids[j].x = llround((double)sum_x / current_groups[j].size());
					next_centroids[j].y = llround((double)sum_y / current_groups[j].size());
				}
			}
			centroids = next_centroids;
		}

		for (auto& group : groupsAssign) {
			if (group.size() > 1)
				sort(group.begin(), group.end(), [&](int a, int b) {
					auto p_a = rects[a].getExpectation();
					auto p_b = rects[b].getExpectation();
					return get_hilbert_order(p_a.x, p_a.y) < get_hilbert_order(p_b.x, p_b.y);
				});
		}
		return groupsAssign;
	}

	vector<P> buildMSTWithDivination(const vector<int> &group) {
		vector<P> totalEdges;
		int n = group.size();
		if (n == 2) {
			totalEdges.emplace_back(group[0], group[1]);
		} else if (n > 2) {
			const int step = L - 1;
			for (int start = 0; start < n - 1; start += step) {
				vector<int> subGroup;
				if (n - start >= L)
					subGroup = vector<int>(group.begin() + start, group.begin() + start + L);
				else if (n - start > 2)
					subGroup = vector<int>(group.end() - min(L, n), group.end());
				else {
					totalEdges.emplace_back(group[start], group[start + 1]);
					continue;
				}
				sort(subGroup.begin(), subGroup.end());
				auto subEdges = queryDivination(subGroup);
				totalEdges.insert(totalEdges.end(), subEdges.begin(), subEdges.end());
			}
		}
		dsu uf(N);
		sort(totalEdges.begin(), totalEdges.end(), [&](const P &a, const P &b) {
			auto [ax, ay] = rects[a.first].getExpectation();
			auto [bx, by] = rects[a.second].getExpectation();
			auto [cx, cy] = rects[b.first].getExpectation();
			auto [dx, dy] = rects[b.second].getExpectation();
			long long da = (long long)(ax - bx) * (ax - bx) + (long long)(ay - by) * (ay - by);
			long long db = (long long)(cx - dx) * (cx - dx) + (long long)(cy - dy) * (cy - dy);
			return da < db;
		});
		vector<P> mstEdges;
		for (auto &[u, v] : totalEdges) {
			if (uf.same(u, v)) continue;
			uf.merge(u, v);
			mstEdges.emplace_back(u, v);
		}
		return mstEdges;
	}

	vector<P> buildMSTWithExpectation(const vector<int>& group) {
		vector<P> mst_edges;
		int n = group.size();
		if (n <= 1) return mst_edges;
		vector<tuple<long long, int, int>> potential_edges;
		potential_edges.reserve((long long)n * (n - 1) / 2);
		vector<Point> expectations_in_group(n);
		rep(i, n)
			expectations_in_group[i] = rects[group[i]].getExpectation();
		for (int i = 0; i < n; ++i) {
			for (int j = i + 1; j < n; ++j) {
				int u = group[i], v = group[j];
				const Point& pu = expectations_in_group[i];
				const Point& pv = expectations_in_group[j];
				long long dx = (long long)pu.x - pv.x;
				long long dy = (long long)pu.y - pv.y;
				long long dsq = dx * dx + dy * dy;
				potential_edges.emplace_back(dsq, u, v);
			}
		}
		sort(potential_edges.begin(), potential_edges.end());
		atcoder::dsu uf(N);
		for (const auto& [dsq, u, v] : potential_edges) {
			if (mst_edges.size() == (size_t)n - 1) break;
			if (!uf.same(u, v)) {
				uf.merge(u, v);
				mst_edges.push_back({u, v});
			}
		}
		return mst_edges;
	}

	void solve() {
		int maxDivinations = Q;
		rep(i, M) {
			if (G[i] > 2 && G[i] <= L)
				maxDivinations--;
			else if (G[i] > L) {
				maxDivinations -= (int)((double)(G[i] - 1) / (L - 1));
				int rest = G[i] - (int)((double)(G[i] - 1) / (L - 1)) * (L - 1);
				if (rest >= 3)
					maxDivinations--;
			}
		}
		maxDivinations = max(0, maxDivinations);
		rep(i, maxDivinations) {
			if (utility::timer.elapsed() > TIME_LIMIT * 0.9)
				break;
			auto group = selectGroup();
			auto mstEdges = queryDivination(group);
			if (mstEdges.empty()) continue;
			processMSTEdges(mstEdges, group);
		}
		ansGroup = buildGroups();
	}

	void output() {
		vector<pair<vector<int>, vector<P>>> answers;
		for (auto &group : ansGroup) {
			auto mstEdges = buildMSTWithDivination(group);
			answers.emplace_back(group, mstEdges);
		}
		cout << "!" << "\n";
		for (auto &p : answers) {
			auto [group, mstEdges] = p;
			for (int id : group)
				cout << id << " ";
			cout << "\n" << flush;
			for (auto &[u, v] : mstEdges)
				cout << u << " " << v << "\n" << flush;
		}
	}
};

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	Solver solver;
	solver.solve();
	solver.output();
	return 0;
}
