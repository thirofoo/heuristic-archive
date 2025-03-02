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

inline double gaussian(double mean, double stddev) {
  double z0 = sqrt(-2.0 * log(rand_double())) * cos(2.0 * M_PI * rand_double());
  return mean + z0 * stddev;
}

// 温度関数
#define TIME_LIMIT 2950
inline double temp(double start) {
  double start_temp = 100, end_temp = 1;
  return start_temp + (end_temp - start_temp) * ((utility::mytm.elapsed() - start) / TIME_LIMIT);
}

// 焼きなましの採用確率
inline double prob(int best, int now, int start) {
  return exp((double) (now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

constexpr int N = 20;
constexpr int INF = 1 << 30;

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

// right | down | left | up
#define DIR_NUM 4
enum Direction {RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3};
vector<Point> delta = {Point(0, 1), Point(1, 0), Point(0, -1), Point(-1, 0)};
string dir_str = "RDLU";
inline char getDir(const Point &now, const Point &next) {
	rep(i, DIR_NUM) if(next == now + delta[i]) return dir_str[i];
	cerr << "Error: getDir" << '\n';
	assert(false);
}

inline bool outField(const Point &p) {
	if(p.x < 0 || p.x >= N || p.y < 0 || p.y >= N) return true;
	return false;
}

struct Solver {
	int _dummy, M;
	vector<string> field;
	vector<pair<int, char>> ops;

  Solver() {
    this->input();
		return;
  }

  void input() {
		cin >> _dummy >> M;
		field.resize(N);
		rep(i, N) cin >> field[i];
    return;
  }

  void output() {
		assert(ops.size() <= 10000);
		rep(i, ops.size()) cout << ops[i].first << ' ' << ops[i].second << '\n';
    return;
  }

  void solve() {
		// BFS で最も近い鉱石 or 岩を穴に運ぶ
		Point hole;
		rep(i, N) rep(j, N) {
			if(field[i][j] != 'A') continue;
			hole = Point(i, j);
			break;
		}
		vector<vector<int>> dist;
		vector<vector<Point>> prev_p;
		queue<pair<Point, int>> que;
		int carried_cnt = 0;

		while(carried_cnt < 2 * N) {
			dist.assign(N, vector<int>(N, INF));
			prev_p.assign(N, vector<Point>(N, Point(-1, -1)));

			// ※ 仕様上必ず最初は hole になる
			que = queue<pair<Point, int>>();
			que.push({hole, 0});
			dist[hole.x][hole.y] = 0;

			Point goal_koseki = Point(-1, -1), goal_iwa = Point(-1, -1);
			while(!que.empty()) {
				auto [cur, d] = que.front();
				que.pop();
				if(field[cur.x][cur.y] != '.' && field[cur.x][cur.y] != 'A') {
					if(field[cur.x][cur.y] == 'a' && goal_koseki == Point(-1, -1)) goal_koseki = cur;
					if(field[cur.x][cur.y] == '@' && goal_iwa == Point(-1, -1)) goal_iwa = cur;
					// break;
					continue;
				}
				// cerr << cur.x << ' ' << cur.y << ' ' << d << '\n';
				rep(i, DIR_NUM) {
					Point next = cur + delta[i];
					if(outField(next) || dist[next.x][next.y] != INF) continue;
					dist[next.x][next.y] = d + 1;
					prev_p[next.x][next.y] = cur;
					que.push({next, d + 1});
				}
			}
			
			Point goal = (goal_koseki != Point(-1, -1) ? goal_koseki : goal_iwa);
			vector<Point> route;
			for(Point cur = goal; cur != hole; cur = prev_p[cur.x][cur.y]) {
				route.push_back(cur);
			}
			route.push_back(hole);
			reverse(route.begin(), route.end());

			// 操作列を生成
			rep(i, route.size() - 1) {
				// 移動 part
				Point next = route[i + 1];
				char d = getDir(route[i], next);
				ops.push_back({1, d});
			}
			for(int i = route.size() - 1; i > 0; i--) {
				// 運搬 part
				Point next = route[i - 1];
				char d = getDir(route[i], next);
				ops.push_back({2, d});
			}

			// field から鉱石 or 岩を消す
			carried_cnt += (field[goal.x][goal.y] == 'a' ? 1 : 0);
			field[goal.x][goal.y] = '.';
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
