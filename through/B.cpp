#include <bits/stdc++.h>
using namespace std;
using ll = long long;

inline unsigned int randInt() {
  static unsigned int tx = 123456789, ty = 362436069, tz = 521288629, tw = 88675123;
  unsigned int tt = tx ^ (tx << 11);
  tx = ty;
  ty = tz;
  tz = tw;
  return tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8));
}

inline double randDouble01() {
  return static_cast<double>(randInt()) / (static_cast<double>(UINT_MAX) + 1.0);
}

#define TIME_LIMIT 1950
const double INF_COST = -1.0;

struct Point {
  int x, y;
  Point() : x(0), y(0) {}
  Point(int _x, int _y) : x(_x), y(_y) {}
  Point operator-(const Point &other) const { return Point(x - other.x, y - other.y); }
  bool operator==(const Point &other) const { return x == other.x && y == other.y; }
};

double dist(Point a, Point b) {
  ll dx = a.x - b.x, dy = a.y - b.y;
  return sqrt(static_cast<double>(dx * dx + dy * dy));
}

bool isOnSegment(Point A, Point B, Point P) {
  if (A.x == B.x) return P.x == A.x && P.y >= min(A.y, B.y) && P.y <= max(A.y, B.y);
  if (A.y == B.y) return P.y == A.y && P.x >= min(A.x, B.x) && P.x <= max(A.x, B.x);
  ll cp = (ll)(P.y - A.y) * (B.x - A.x) - (ll)(P.x - A.x) * (B.y - A.y);
  if (cp != 0) return false;
  return P.x >= min(A.x, B.x) && P.x <= max(A.x, B.x) && P.y >= min(A.y, B.y) && P.y <= max(A.y, B.y);
}

namespace utility {
  struct Timer {
    chrono::system_clock::time_point start;
    Timer() { start = chrono::system_clock::now(); }
    double elapsed() const {
      using namespace std::chrono;
      return (double)duration_cast<milliseconds>(system_clock::now() - start).count();
    }
  } timer;
}

inline double temp(double elapsed_time, double total_duration) {
  double start_temp = 10000, end_temp = 0.1;
  if (total_duration <= 0)
    return end_temp;
  double progress = elapsed_time / total_duration;
  progress = min(1.0, max(0.0, progress));
  return start_temp + (end_temp - start_temp) * progress;
}

inline double acceptanceProbability(double current_cost, double next_cost, double temperature) {
  if (next_cost < current_cost)
    return 1.0;
  if (temperature <= 1e-9)
    return 0.0;
  double delta = current_cost - next_cost;
  double exponent = delta / temperature;
  if (exponent < -700.0)
    return 0.0;
  return exp(exponent);
}

struct Solver {
  int X, Y, Z;
  vector<Point> px, py, pz;
  vector<vector<Point>> history;
  vector<vector<bool>> segmentValidX, segmentValidY;
  mt19937 rng;

  Solver() : rng(randInt()) { input(); }

  void input() {
    cin >> X >> Y >> Z;
    px.resize(X);
    py.resize(Y);
    pz.resize(Z);
    for (int i = 0; i < X; i++)
      cin >> px[i].x >> px[i].y;
    for (int i = 0; i < Y; i++)
      cin >> py[i].x >> py[i].y;
    for (int i = 0; i < Z; i++)
      cin >> pz[i].x >> pz[i].y;
  }

  bool checkSegment(int i, int j, const vector<Point>& pts, const vector<Point>& obs) {
    if (i == j)
      return true;
    Point A = pts[i], B = pts[j];
    for (const auto& o : obs) {
      if (o == A || o == B)
        continue;
      if (isOnSegment(A, B, o))
        return false;
    }
    return true;
  }

  void precomputeValidity() {
    if (X > 1) {
      segmentValidX.assign(X, vector<bool>(X, false));
      for (int i = 0; i < X; i++) {
        for (int j = i; j < X; j++) {
          bool valid = checkSegment(i, j, px, pz);
          segmentValidX[i][j] = segmentValidX[j][i] = valid;
        }
      }
    }
    if (Y > 1) {
      segmentValidY.assign(Y, vector<bool>(Y, false));
      for (int i = 0; i < Y; i++) {
        for (int j = i; j < Y; j++) {
          bool valid = checkSegment(i, j, py, pz);
          segmentValidY[i][j] = segmentValidY[j][i] = valid;
        }
      }
    }
  }

  double calcTotalCost(const vector<int>& orderX, const vector<int>& orderY) {
    double total = 0;
    int movesX = max(0, X - 1), movesY = max(0, Y - 1);
    int K = max(movesX, movesY);
    if (K == 0)
      return 0.0;
    for (int i = 0; i < K; i++) {
      double costX = 0.0, costY = 0.0;
      if (i < movesX) {
        int u = orderX[i], v = orderX[i + 1];
        if (!segmentValidX[u][v])
          return INF_COST;
        costX = 2.0 * dist(px[u], px[v]);
      }
      if (i < movesY) {
        int u = orderY[i], v = orderY[i + 1];
        if (!segmentValidY[u][v])
          return INF_COST;
        costY = 2.0 * dist(py[u], py[v]);
      }
      total += max(costX, costY);
    }
    return total;
  }

  bool findValidPath(vector<int>& order, int N, const vector<vector<bool>>& validTable) {
    if (N <= 1)
      return true;
    auto check = [&]() -> bool {
      for (int i = 0; i < N - 1; i++) {
        if (order[i] < 0 || order[i] >= N || order[i + 1] < 0 || order[i + 1] >= N)
          return false;
        if (!validTable[order[i]][order[i + 1]])
          return false;
      }
      return true;
    };
    if (check())
      return true;
    int attempts = 0;
    const int MAX_ATTEMPTS = 3000;
    double startTime = utility::timer.elapsed();
    const double LIMIT = 400;
    while (utility::timer.elapsed() - startTime < LIMIT && attempts < MAX_ATTEMPTS) {
      shuffle(order.begin(), order.end(), rng);
      if (check())
        return true;
      attempts++;
    }
    return false;
  }

  void solve() {
    precomputeValidity();
    vector<int> orderX(X), orderY(Y);
    if (X > 0)
      iota(orderX.begin(), orderX.end(), 0);
    if (Y > 0)
      iota(orderY.begin(), orderY.end(), 0);
    bool validX = (X <= 1) || findValidPath(orderX, X, segmentValidX);
    bool validY = (Y <= 1) || findValidPath(orderY, Y, segmentValidY);
    if (!validX && X > 1)
      iota(orderX.begin(), orderX.end(), 0);
    if (!validY && Y > 1)
      iota(orderY.begin(), orderY.end(), 0);
    double currentCost = calcTotalCost(orderX, orderY);
    if (currentCost == INF_COST && (X > 1 || Y > 1)) {
      Point origin(0, 0);
      history.push_back({origin, origin, origin, origin});
      return;
    }
    vector<int> bestOrderX = orderX, bestOrderY = orderY;
    double bestCost = currentCost;
    double SAStart = utility::timer.elapsed();
    double SADuration = max(0.0, TIME_LIMIT - SAStart);
    while (utility::timer.elapsed() < TIME_LIMIT) {
      if (X <= 1 && Y <= 1)
        break;
      bool modifyX = (X > 1 && (Y <= 1 || randInt() % 2 == 0));
      bool modifyY = (Y > 1 && !modifyX);
      vector<int> nextOrderX = orderX, nextOrderY = orderY;
      if (modifyX) {
        int i = randInt() % X, j = randInt() % X;
        if (i == j) {
          j = (j + 1 + randInt() % (X - 1)) % X;
          if (i == j)
            continue;
        }
        if (i > j)
          swap(i, j);
        reverse(nextOrderX.begin() + i + 1, nextOrderX.begin() + j + 1);
      }
      else if (modifyY) {
        int i = randInt() % Y, j = randInt() % Y;
        if (i == j) {
          j = (j + 1 + randInt() % (Y - 1)) % Y;
          if (i == j)
            continue;
        }
        if (i > j)
          swap(i, j);
        reverse(nextOrderY.begin() + i + 1, nextOrderY.begin() + j + 1);
      }
      else {
        continue;
      }
      double nextCost = calcTotalCost(nextOrderX, nextOrderY);
      if (nextCost == INF_COST)
        continue;
      double currentElapsed = utility::timer.elapsed() - SAStart;
      double T = temp(currentElapsed, SADuration);
      double P = acceptanceProbability(currentCost, nextCost, T);
      if (P >= randDouble01()) {
        orderX = nextOrderX;
        orderY = nextOrderY;
        currentCost = nextCost;
        if (currentCost < bestCost) {
          bestCost = currentCost;
          bestOrderX = orderX;
          bestOrderY = orderY;
        }
      }
    }
    history.clear();
    Point p1 = (X > 0) ? px[bestOrderX[0]] : Point(0, 0);
    Point q1 = p1;
    Point p2 = (Y > 0) ? py[bestOrderY[0]] : Point(0, 0);
    Point q2 = p2;
    history.push_back({p1, q1, p2, q2});
    int movesX = max(0, X - 1), movesY = max(0, Y - 1);
    int K = max(movesX, movesY);
    for (int i = 0; i < K; i++) {
      Point nextP1 = (i < movesX) ? px[bestOrderX[i + 1]] : p1;
      Point nextQ1 = nextP1;
      Point nextP2 = (i < movesY) ? py[bestOrderY[i + 1]] : p2;
      Point nextQ2 = nextP2;
      history.push_back({nextP1, nextQ1, nextP2, nextQ2});
      p1 = nextP1;
      q1 = nextQ1;
      p2 = nextP2;
      q2 = nextQ2;
    }
  }

  void output() {
    if (history.empty()) {
      cout << "0 0 0 0 0 0 0 0\n";
      return;
    }
    cout << history[0][0].x << " " << history[0][0].y << " " 
       << history[0][1].x << " " << history[0][1].y << " " 
       << history[0][2].x << " " << history[0][2].y << " " 
       << history[0][3].x << " " << history[0][3].y << "\n";
    for (size_t i = 1; i < history.size(); i++) {
      cout << history[i][0].x << " " << history[i][0].y << " " 
         << history[i][1].x << " " << history[i][1].y << " " 
         << history[i][2].x << " " << history[i][2].y << " " 
         << history[i][3].x << " " << history[i][3].y << "\n";
    }
  }
};

int main() {
  ios_base::sync_with_stdio(false);
  cin.tie(0);
  Solver solver;
  solver.solve();
  solver.output();
  cerr << "Total time: " << utility::timer.elapsed() << " ms\n";
  return 0;
}
