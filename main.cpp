#include <atcoder/all>
#include <bits/stdc++.h>
using namespace std;
using namespace atcoder;

#define rep(i, n) for (int i = 0; i < (n); i++)

constexpr int TIME_LIMIT = 19000;
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
	vector<T> existProb;  // 各セルの中心座標とそのセル面積に基づく確率

	Rect(int _x1, int _x2, int _y1, int _y2)
		: x1(_x1), x2(_x2), y1(_y1), y2(_y2),
			ul(_x1, _y1), dl(_x2, _y1), ur(_x1, _y2), dr(_x2, _y2)
	{
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
		// 正規化
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
					long long d1 = (long long)(x1 - x2) * (x1 - x2) 
									 + (long long)(y1 - y2) * (y1 - y2);
					long long d2 = (long long)(x1 - x3) * (x1 - x3)
									 + (long long)(y1 - y3) * (y1 - y3);
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
	
	Solver() {
		input();
		utility::timer.startTimer();
		ansGroup.assign(M, vector<int>{});
	}

	void input() {
		utility::timer.startTimer();
		cin >> N >> M >> Q >> L >> W;
		G.resize(M);
		rep(i, M) {
			cin >> G[i];
		}
		rep(i, N) {
			int x1, x2, y1, y2;
			cin >> x1 >> x2 >> y1 >> y2;
			rects.emplace_back(x1, x2, y1, y2);
		}
		return;
	}

	// divination：グループ内の2点間の関係を問い合わせる
	vector<P> queryDivination(const vector<int> &group) {
		// 入力条件チェック
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

	// 不確かさ（分散）からグループを選択
	vector<bool> used;
	vector<int> selectGroup() {
		if(used.empty()) used.resize(N, false);
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

	// MSTの各辺に対して確率を更新
	void processMSTEdges(const vector<P> &mstEdges, const vector<int>& group) {
		// 座標圧縮
		set<int> nodes;
		for (const auto &[u, v] : mstEdges) {
			nodes.insert(u); nodes.insert(v);
		}
		for (int id : group) nodes.insert(id);
		vector<int> idx(nodes.begin(), nodes.end());
		sort(idx.begin(), idx.end());
		map<int, int> comp;
		rep(i, idx.size()) comp[idx[i]] = i;
		vector<P> compEdges;
		for (const auto &edge : mstEdges) {
			int cu = comp[edge.first], cv = comp[edge.second];
			compEdges.emplace_back(cu, cv);
		}
		// 各MST辺に対して，その辺を除いた連結成分に対して確率更新
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
			for (const auto &g : groups) {
				if (uf.leader(g[0]) == leaderU)
					groupU = g;
				else
					groupV = g;
			}
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

	// グループ分割（snake order で連結性を持たせる）
	vector<vector<int>> buildGroups() {
		vector<int> perm(N);
		iota(perm.begin(), perm.end(), 0);
		int cellWidth = 10000 / 10;
		map<int, vector<int>> groupsByCol;
		for (int id : perm) {
			auto [cx, cy] = rects[id].getExpectation();
			int col = cx / cellWidth;
			groupsByCol[col].emplace_back(id);
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
			for (int id : vec)
				snakeOrder.emplace_back(id);
		}
		vector<int> snake = snakeOrder;
		reverse(snake.begin(), snake.end());

		vector<vector<int>> groups(M);
		vector<int> prefix(M + 1, 0);
		rep(i, M)
			prefix[i + 1] = prefix[i] + G[i];
		rep(i, M) {
			int start = prefix[i];
			int sz = G[i];
			groups[i] = vector<int>(snake.begin() + start, snake.begin() + start + sz);
		}
		return groups;
	}

	// MST構築のために複数回問い合わせを行う
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
		// シンプルな距離比較によりソート（重みは期待値同士の距離の2乗）
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

	void solve() {
		int maxDivinations = Q - 1;
		rep(i, M) {
			if (G[i] > 2 && G[i] <= L) maxDivinations--;
			else if (G[i] > L) {
				maxDivinations -= (int)((double)(G[i] - 1) / (L - 1));
				// 端数分が <= 2 なら無し、それ以外は 1 回
				int rest = G[i] - (int)((double)(G[i] - 1) / (L - 1)) * (L - 1);
				if (rest >= 3) maxDivinations--;
			}
		}
		maxDivinations = max(0, maxDivinations);

		// 時間制限を意識して divination を繰り返す
		cerr << "maxDivinations: " << maxDivinations << "\n";
		rep(i, maxDivinations) {
			if (utility::timer.elapsed() > TIME_LIMIT)
				break;
			auto group = selectGroup();
			auto mstEdges = queryDivination(group);
			if (mstEdges.empty()) continue;
			processMSTEdges(mstEdges, group);
		}
		ansGroup = buildGroups();
		cerr << "Code End Time: " << utility::timer.elapsed() << "ms" << "\n";
		return;
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
		return;
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
