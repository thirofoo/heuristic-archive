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
    if (Grid[i][j] == 'A')
      hole[0] = { i, j };
    if (Grid[i][j] == 'B')
      hole[1] = { i, j };
    if (Grid[i][j] == 'C')
      hole[2] = { i, j };
  }

  constexpr array<int, 4> dx = { 0, 1, 0, -1 };
  constexpr array<int, 4> dy = { 1, 0, -1, 0 };

  constexpr int INF = 1e9;
  // 穴 A, B, C についてそれぞれ処理していく
  rep(holenum, 3) {
    // まず BFS
    vector dist(n, vector<int>(n, INF));
    vector prv(n, vector<pair<int, int>>(n, { -1, -1 }));
    queue<pair<int, int>> q;
    q.push(hole[holenum]);
    dist[hole[holenum].first][hole[holenum].second] = 0;
    while (!q.empty()) {
      auto [x, y] = q.front();
      q.pop();
      rep(dir, 4) {
        int nx = x + dx[dir], ny = y + dy[dir];
        if (!inRange(nx, ny))
          continue;
        if (dist[nx][ny] != INF)
          continue;
        if (Grid[nx][ny] != '.' && Grid[nx][ny] != 'a' + holenum)
          continue;
        dist[nx][ny] = dist[x][y] + 1;
        prv[nx][ny] = { x, y };
        q.push({ nx, ny });
      }
    }

    // 鉱石を運んで戻ってくる
    vector<pair<int, int>> ore(0);
    rep(i, n) rep(j, n) if (Grid[i][j] == 'a' + holenum) ore.push_back({ i, j });
    sort(ore.rbegin(), ore.rend(), [&](auto a, auto b) {
      return dist[a.first][a.second] < dist[b.first][b.second];
    });
    while (!ore.empty()) {
      auto [x, y] = ore.back();
      ore.pop_back();
      Grid[x][y] = '.';

      // ore に移動は鉱石をスルー出来るので愚直に
      vector<pair<int, int>> orepath = getpath(hole[holenum], { x, y });
      rep(i, orepath.size() - 1) cout << 1 << ' ' << direction(orepath[i], orepath[i + 1]) << '\n';

      // ore -> hole パスを求める
      vector<pair<int, int>> path;
      {
        auto [nx, ny] = prv[x][y];
        path.push_back({ x, y });
        while (nx != -1) {
          path.push_back({ nx, ny });
          auto [px, py] = prv[nx][ny];
          nx = px, ny = py;
        }
        assert(path.size() == dist[x][y] + 1);
        assert(path.back() == hole[holenum]);
        assert(path.front() == make_pair(x, y));
      }
      // 運びながら戻る
      rep(i, path.size() - 1) cout << 2 << ' ' << direction(path[i], path[i + 1]) << '\n';
    }

    // 次の穴に移動
    if (holenum != 2) {
      vector<pair<int, int>> path = getpath(hole[holenum], hole[holenum + 1]);
      rep(i, path.size() - 1) cout << 1 << ' ' << direction(path[i], path[i + 1]) << '\n';
    }
  }
  return 0;
}
