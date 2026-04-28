#include <bits/stdc++.h>
using namespace std;
#ifdef LOCAL
  #include "settings/debug.cpp"
#else
  #define Debug(...) void(0)
#endif
#define rep(i, n) for (int i = 0; i < (n); ++i)
using ll = long long;
using ull = unsigned long long;

inline char rolling_direction(pair<int, int> from, pair<int, int> to) {
  assert(from.first == to.first || from.second == to.second);
  if (from.first == to.first) {
    if (from.second < to.second)
      return 'R';
    else
      return 'L';
  }
  else {
    if (from.first < to.first)
      return 'D';
    else
      return 'U';
  }
}

inline char direction(pair<int, int> from, pair<int, int> to) {
  assert(abs(from.first - to.first) + abs(from.second - to.second) == 1);
  if (from.first == to.first) {
    if (from.second < to.second)
      return 'R';
    else
      return 'L';
  }
  else {
    if (from.first < to.first)
      return 'D';
    else
      return 'U';
  }
}

inline vector<pair<int, int>> getpath(pair<int, int> from, pair<int, int> to) {
  pair<int, int> now = from;
  vector<pair<int, int>> res;
  res.push_back(now);
  while (now != to) {
    if (now.first < to.first) {
      now.first++;
    }
    else if (now.first > to.first) {
      now.first--;
    }
    else if (now.second < to.second) {
      now.second++;
    }
    else if (now.second > to.second) {
      now.second--;
    }
    res.push_back(now);
  }
  assert(res.size() == abs(from.first - to.first) + abs(from.second - to.second) + 1);
  assert(res.back() == to);
  return res;
}

int main() {
  cin.tie(nullptr)->sync_with_stdio(false);
  int n, m;
  cin >> n >> m;
  vector Grid(n, vector<char>(n));
  rep(i, n) rep(j, n) cin >> Grid[i][j];

  auto inRange = [&](int x, int y) -> bool {
    return 0 <= x && x < n && 0 <= y && y < n;
  };

  vector<pair<int, int>> hole(3);
  rep(i, n) rep(j, n) {
    if (Grid[i][j] == 'A') hole[0] = { i, j };
    if (Grid[i][j] == 'B') hole[1] = { i, j };
    if (Grid[i][j] == 'C') hole[2] = { i, j };
  }

  constexpr array<int, 4> dx = { 0, 1, 0, -1 };
  constexpr array<int, 4> dy = { 1, 0, -1, 0 };
  constexpr int INF = 1e9;

  vector<tuple<int, int, int>> ores;
  rep(i, n) rep(j, n) {
    if (Grid[i][j] == 'a') ores.push_back({ i, j, 0 });
    if (Grid[i][j] == 'b') ores.push_back({ i, j, 1 });
    if (Grid[i][j] == 'c') ores.push_back({ i, j, 2 });
  }

  // 求める
  pair<int, int> now = hole[0];
  while (!ores.empty()) {
    // 穴 A, B, C について距離情報をそれぞれ求める
    // dist[i][j][k] := (i, j) から穴 k の上下左右に行くための最短距離
    vector dist(n, vector(n, vector<int>(3, INF)));
    vector prv(n, vector(n, vector<pair<int, int>>(3, { -1, -1 })));
    rep(holenum, 3) {
      queue<pair<int, int>> q;
      {
        auto [x, y] = hole[holenum];
        dist[x][y][holenum] = 0;
        q.push({ x, y });
        // 上
        for (int i = x - 1; i >= 0; --i) {
          if (Grid[i][y] != '.' && Grid[i][y] != 'a' + holenum) break;
          dist[i][y][holenum] = 0;
          if (Grid[i][y] == '.')
            q.push({ i, y });
          else
            break;
        }
        // 下
        for (int i = x + 1; i < n; ++i) {
          if (Grid[i][y] != '.' && Grid[i][y] != 'a' + holenum) break;
          dist[i][y][holenum] = 0;
          if (Grid[i][y] == '.')
            q.push({ i, y });
          else
            break;
        }
        // 左
        for (int j = y - 1; j >= 0; --j) {
          if (Grid[x][j] != '.' && Grid[x][j] != 'a' + holenum) break;
          dist[x][j][holenum] = 0;
          if (Grid[x][j] == '.')
            q.push({ x, j });
          else
            break;
        }
        // 右
        for (int j = y + 1; j < n; ++j) {
          if (Grid[x][j] != '.' && Grid[x][j] != 'a' + holenum) break;
          dist[x][j][holenum] = 0;
          if (Grid[x][j] == '.')
            q.push({ x, j });
          else
            break;
        }
      }
      while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        rep(dir, 4) {
          int nx = x + dx[dir], ny = y + dy[dir];
          if (!inRange(nx, ny)) continue;
          if (dist[nx][ny][holenum] != INF) continue;
          if (Grid[nx][ny] != '.' && Grid[nx][ny] != 'a' + holenum) continue;
          dist[nx][ny][holenum] = dist[x][y][holenum] + 1;
          prv[nx][ny][holenum] = { x, y };
          // 鉱石がある場合にはそこを通らせないようにする
          if (Grid[nx][ny] == '.') q.push({ nx, ny });
        }
      }
    }

    // よさそうな鉱石を選ぶ
    sort(ores.rbegin(), ores.rend(), [&](auto a, auto b) {
      auto [x1, y1, h1] = a;
      auto [x2, y2, h2] = b;
      return dist[x1][y1][h1] + abs(x1 - now.first) + abs(y1 - now.second) < dist[x2][y2][h2] + abs(x2 - now.first) + abs(y2 - now.second);
    });
    auto [x, y, h] = ores.back();
    if (dist[x][y][h] > 2 * n) break;
    ores.pop_back();
    Grid[x][y] = '.';

    // 鉱石の位置に移動
    {
      auto path = getpath(now, { x, y });
      rep(i, path.size() - 1) cout << 1 << ' ' << direction(path[i], path[i + 1]) << '\n';
      now = { x, y };
    }
    // 穴の上下左右に移動
    {
      vector<pair<int, int>> path;
      auto [x, y] = now;
      while (prv[x][y][h] != make_pair(-1, -1)) {
        path.push_back({ x, y });
        tie(x, y) = prv[x][y][h];
      }
      path.push_back({ x, y });
      assert(path.size() == dist[now.first][now.second][h] + 1);

      rep(i, path.size() - 1) cout << 2 << ' ' << direction(path[i], path[i + 1]) << '\n';
      now = { x, y };
    }
    // 転がす
    {
      cout << 3 << ' ' << rolling_direction(now, hole[h]) << '\n';
    }
  }

  // あとは普通に移動させる
  while (!ores.empty()) {
    // 穴 A, B, C について距離情報をそれぞれ求める
    // dist[i][j][k] := (i, j) から穴 k の上下左右に行くための最短距離
    vector dist(n, vector(n, vector<int>(3, INF)));
    vector prv(n, vector(n, vector<pair<int, int>>(3, { -1, -1 })));
    rep(holenum, 3) {
      queue<pair<int, int>> q;
      auto [x, y] = hole[holenum];
      dist[x][y][holenum] = 0;
      q.push({ x, y });
      while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        rep(dir, 4) {
          int nx = x + dx[dir], ny = y + dy[dir];
          if (!inRange(nx, ny)) continue;
          if (dist[nx][ny][holenum] != INF) continue;
          if (Grid[nx][ny] != '.' && Grid[nx][ny] != 'a' + holenum) continue;
          dist[nx][ny][holenum] = dist[x][y][holenum] + 1;
          prv[nx][ny][holenum] = { x, y };
          if (Grid[nx][ny] == '.') q.push({ nx, ny });
        }
      }
    }

    // よさそうな鉱石を選ぶ
    sort(ores.rbegin(), ores.rend(), [&](auto a, auto b) {
      auto [x1, y1, h1] = a;
      auto [x2, y2, h2] = b;
      return dist[x1][y1][h1] + abs(x1 - now.first) + abs(y1 - now.second) < dist[x2][y2][h2] + abs(x2 - now.first) + abs(y2 - now.second);
    });
    auto [x, y, h] = ores.back();
    ores.pop_back();
    Grid[x][y] = '.';

    // 鉱石の位置に移動
    {
      auto path = getpath(now, { x, y });
      rep(i, path.size() - 1) cout << 1 << ' ' << direction(path[i], path[i + 1]) << '\n';
      now = { x, y };
    }
    // 穴に運ぶ
    {
      vector<pair<int, int>> path;
      auto [x, y] = now;
      while (prv[x][y][h] != make_pair(-1, -1)) {
        path.push_back({ x, y });
        tie(x, y) = prv[x][y][h];
      }
      path.push_back({ x, y });
      assert(path.size() == dist[now.first][now.second][h] + 1);

      rep(i, path.size() - 1) cout << 2 << ' ' << direction(path[i], path[i + 1]) << '\n';
      now = { x, y };
      assert(now == hole[h]);
    }
  }
  return 0;
}
