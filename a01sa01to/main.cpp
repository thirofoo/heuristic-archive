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

struct P {
  int x, y;
};

inline ll orient(P a, P b, P p) {
  return (ll) (b.x - a.x) * (p.y - a.y) - (ll) (b.y - a.y) * (p.x - a.x);
}
inline bool contains(P p, P a, P b, P c) {
  if (orient(a, b, c) == 0) {
    if (orient(a, b, p) != 0 || orient(a, b, p) != 0) return false;
    return min({ a.x, b.x, c.x }) <= p.x && p.x <= max({ a.x, b.x, c.x }) && min({ a.y, b.y, c.y }) <= p.y && p.y <= max({ a.y, b.y, c.y });
  }
  ll c1 = orient(a, b, p), c2 = orient(b, c, p), c3 = orient(c, a, p);
  return (c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0);
}

inline vector<tuple<int, int, int, int>> enumerate_triangles(const vector<P>& points, const vector<P>& others, const vector<P>& recyclables) {
  vector<tuple<int, int, int, int>> res;
  for (int i = 0; i < points.size(); ++i) {
    for (int j = i + 1; j < points.size(); ++j) {
      for (int k = j + 1; k < points.size(); ++k) {
        bool ok = true;
        for (const auto& other : others) {
          if (contains(other, points[i], points[j], points[k])) {
            ok = false;
            break;
          }
        }
        if (!ok) continue;
        for (const auto& recyclable : recyclables) {
          if (contains(recyclable, points[i], points[j], points[k])) {
            ok = false;
            break;
          }
        }
        if (!ok) continue;
        int cnt = 0;
        for (const auto& point : points) {
          if (contains(point, points[i], points[j], points[k])) ++cnt;
        }
        res.emplace_back(cnt, i, j, k);
      }
    }
  }
  return res;
}

int main() {
  cin.tie(nullptr)->sync_with_stdio(false);
  int x, y, z;
  cin >> x >> y >> z;
  vector<P> burnable(x), not_burnable(y), recyclable(z);
  rep(i, x) cin >> burnable[i].x >> burnable[i].y;
  rep(i, y) cin >> not_burnable[i].x >> not_burnable[i].y;
  rep(i, z) cin >> recyclable[i].x >> recyclable[i].y;

  auto burnable_triangles = enumerate_triangles(burnable, not_burnable, recyclable);
  auto notburnable_triangles = enumerate_triangles(not_burnable, burnable, recyclable);
  sort(burnable_triangles.rbegin(), burnable_triangles.rend());
  sort(notburnable_triangles.rbegin(), notburnable_triangles.rend());

  vector<tuple<int, int, int>> burnable_triangles_used, notburnable_triangles_used;
  bitset<100> used_burnable, used_notburnable;
  for (const auto& [cnt, i, j, k] : burnable_triangles) {
    bool ok = true;
    rep(l, burnable.size()) {
      if (contains(burnable[l], burnable[i], burnable[j], burnable[k]) && used_burnable[l]) {
        ok = false;
        break;
      }
    }
    if (ok) {
      rep(l, burnable.size()) if (contains(burnable[l], burnable[i], burnable[j], burnable[k])) used_burnable[l] = true;
      burnable_triangles_used.emplace_back(i, j, k);
    }
  }
  for (const auto& [cnt, i, j, k] : notburnable_triangles) {
    bool ok = true;
    rep(l, not_burnable.size()) {
      if (contains(not_burnable[l], not_burnable[i], not_burnable[j], not_burnable[k]) && used_notburnable[l]) {
        ok = false;
        break;
      }
    }
    if (ok) {
      rep(l, not_burnable.size()) if (contains(not_burnable[l], not_burnable[i], not_burnable[j], not_burnable[k])) used_notburnable[l] = true;
      notburnable_triangles_used.emplace_back(i, j, k);
    }
  }

  vector<tuple<P, P, P, P>> ans;
  for (const auto& [i, j, k] : burnable_triangles_used) {
    ans.emplace_back(burnable[i], burnable[i], P { 0, 0 }, P { 0, 0 });
    ans.emplace_back(burnable[j], burnable[k], P { 0, 0 }, P { 0, 0 });
    ans.emplace_back(burnable[k], burnable[k], P { 0, 0 }, P { 0, 0 });
  }
  // 拾えてない頂点を回収
  rep(i, burnable.size()) if (!used_burnable[i]) ans.emplace_back(burnable[i], burnable[i], P { 0, 0 }, P { 0, 0 });

  if (y > 0) {
    int idx = 0;
    for (const auto& [i, j, k] : notburnable_triangles_used) {
      if (idx >= ans.size()) rep(_, 3) ans.push_back(ans.back());
      tie(get<2>(ans[idx]), get<3>(ans[idx])) = tie(not_burnable[i], not_burnable[i]);
      tie(get<2>(ans[idx + 1]), get<3>(ans[idx + 1])) = tie(not_burnable[j], not_burnable[k]);
      tie(get<2>(ans[idx + 2]), get<3>(ans[idx + 2])) = tie(not_burnable[k], not_burnable[k]);
      idx += 3;
    }
    // 拾えてない頂点を回収
    rep(i, not_burnable.size()) {
      if (!used_notburnable[i]) {
        if (idx >= ans.size()) rep(_, 1) ans.push_back(ans.back());
        tie(get<2>(ans[idx]), get<3>(ans[idx])) = tie(not_burnable[i], not_burnable[i]);
        ++idx;
      }
    }

    // 余った部分は最後の頂点に居残るのが最適
    if (idx < ans.size()) {
      P last = get<2>(ans[idx - 1]);
      for (int i = idx; i < ans.size(); ++i) {
        get<2>(ans[i]) = get<3>(ans[i]) = last;
      }
    }
  }

  for (const auto& [a, b, c, d] : ans) {
    cout << a.x << ' ' << a.y << ' ' << b.x << ' ' << b.y << ' ' << c.x << ' ' << c.y << ' ' << d.x << ' ' << d.y << '\n';
  }
  return 0;
}
