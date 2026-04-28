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

inline char rev_dir(const char &d) {
	if(d == 'R') return 'L';
	if(d == 'D') return 'U';
	if(d == 'L') return 'R';
	if(d == 'U') return 'D';
	cerr << "Error: rev_dir" << '\n';
	assert(false);
}

inline int dir_to_int(const char &d) {
	if(d == 'R') return 0;
	if(d == 'D') return 1;
	if(d == 'L') return 2;
	if(d == 'U') return 3;
	cerr << "Error: dir_to_int" << '\n';
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
		vector<vector<int>> dist;
		vector<vector<Point>> prev_p;
		queue<pair<Point, int>> que;
		deque<pair<Point, int>> dq;
		
		Point hole;
		rep(i, N) rep(j, N) {
			if(field[i][j] != 'A') continue;
			hole = Point(i, j);
			break;
		}

		vector<vector<bool>> use_rock_flag(N, vector<bool>(N, false));
		// 穴の上下左右の岩も対象
		rep(d, DIR_NUM) {
			Point now = hole;
			while(true) {
				Point next = now + delta[d];
				if(outField(next)) break;
				if(field[next.x][next.y] == '@') {
					use_rock_flag[next.x][next.y] = true;
				}
				now = next;
			}
		}
		// 初めに動かす鉱石 or 岩を決め打ち part
		// ※ 01BFS で岩の上を通る時にコスト + 1 する
		rep(i, N) rep(j, N) {
			if(field[i][j] != 'a') continue;
			Point goal = Point(i, j);
			dist.assign(N, vector<int>(N, INF));
			prev_p.assign(N, vector<Point>(N, Point(-1, -1)));
			dq = deque<pair<Point, int>>();
			dq.push_back({hole, 0});
			dist[hole.x][hole.y] = 0;

			while(!dq.empty()) {
				auto [cur, d] = dq.front();
				dq.pop_front();
				if(cur == goal) break;

				rep(i, DIR_NUM) {
					Point next = cur + delta[i];
					if(outField(next) || dist[next.x][next.y] != INF) continue;
					if(field[next.x][next.y] == '@' && !use_rock_flag[next.x][next.y]) {
						dq.push_back({next, d + 1});
						dist[next.x][next.y] = d + 1;
						prev_p[next.x][next.y] = cur;
					} else {
						dq.push_front({next, d});
						dist[next.x][next.y] = d;
						prev_p[next.x][next.y] = cur;
					}
				}
			}
			for(Point cur = goal; cur != hole; cur = prev_p[cur.x][cur.y]) {
				if(field[cur.x][cur.y] == '@') use_rock_flag[cur.x][cur.y] = true;
			}
		}


		
		// BFS で最も近い鉱石 or 岩を穴に運ぶ part
		int carried_cnt = 0;
		Point now_start = hole;
		vector<vector<bool>> hole_surround;
		while(carried_cnt < 2 * N) {
			hole_surround.assign(N, vector<bool>(N, false));
			rep(d, DIR_NUM) {
				Point now = hole;
				while(true) {
					Point next = now + delta[d];
					if(outField(next)) break;
					if(field[next.x][next.y] != '.') {
						hole_surround[next.x][next.y] = true;
						break;
					}
					hole_surround[next.x][next.y] = true;
					now = next;
				}
			}

			dist.assign(N, vector<int>(N, INF));
			prev_p.assign(N, vector<Point>(N, Point(-1, -1)));

			// ※ 仕様上必ず最初は hole になる
			que = queue<pair<Point, int>>();
			que.push({now_start, 0});
			dist[now_start.x][now_start.y] = 0;

			Point goal = Point(-1, -1);
			while(!que.empty()) {
				auto [cur, d] = que.front();
				que.pop();
				if(field[cur.x][cur.y] != '.' && (field[cur.x][cur.y] == 'a' || use_rock_flag[cur.x][cur.y])) {
					goal = cur;
					break;
				}
				rep(i, DIR_NUM) {
					Point next = cur + delta[i];
					if(outField(next) || dist[next.x][next.y] != INF || (!use_rock_flag[next.x][next.y] && field[next.x][next.y] == '@')) continue;
					dist[next.x][next.y] = d + 1;
					prev_p[next.x][next.y] = cur;
					que.push({next, d + 1});
				}
			}
			// cerr << "goal: " << goal.x << ' ' << goal.y << '\n';
			
			vector<Point> route;
			for(Point cur = goal; cur != now_start; cur = prev_p[cur.x][cur.y]) {
				route.push_back(cur);
			}
			route.push_back(now_start);
			reverse(route.begin(), route.end());
			rep(i, route.size() - 1) {
				// 移動 part
				Point next = route[i + 1];
				char d = getDir(route[i], next);
				ops.push_back({1, d});
			}



			// もう一回 goal → hole に向かって BFS
			dist.assign(N, vector<int>(N, INF));
			prev_p.assign(N, vector<Point>(N, Point(-1, -1)));
			que = queue<pair<Point, int>>();
			que.push({goal, 0});
			dist[goal.x][goal.y] = 0;
			Point next_goal;
			
			while(!que.empty()) {
				auto [cur, d] = que.front();
				que.pop();
				if(hole_surround[cur.x][cur.y]) {
					next_goal = cur;
					break;
				}

				rep(i, DIR_NUM) {
					Point next = cur + delta[i];
					if(outField(next) || dist[next.x][next.y] != INF || (field[next.x][next.y] != '.' && field[next.x][next.y] != 'A')) continue;
					dist[next.x][next.y] = d + 1;
					prev_p[next.x][next.y] = cur;
					que.push({next, d + 1});
				}
			}
			route.clear();
			for(Point cur = next_goal; cur != goal; cur = prev_p[cur.x][cur.y]) {
				route.push_back(cur);
			}
			route.push_back(goal);
			reverse(route.begin(), route.end());

			rep(i, route.size() - 1) {
				// 運搬 part
				char d = getDir(route[i], route[i + 1]);
				ops.push_back({2, d});
			}
			char last_op;
			Point delta_p = hole - route[route.size() - 1];
			if(delta_p.x < 0) last_op = 'U';
			if(delta_p.x > 0) last_op = 'D';
			if(delta_p.y < 0) last_op = 'L';
			if(delta_p.y > 0) last_op = 'R';
			ops.push_back({3, last_op});

			// cerr << "now_start: " << now_start.x << ' ' << now_start.y << '\n';
			// cerr << "goal: " << goal.x << ' ' << goal.y << '\n';
			// cerr << "next_goal: " << next_goal.x << ' ' << next_goal.y << '\n';
			now_start = next_goal;

			// rep(i, ops.size()) cerr << ops[i].first << ' ' << ops[i].second << '\n';
			// cerr << '\n';
			// cerr << "carried_cnt: " << carried_cnt << '\n';

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
