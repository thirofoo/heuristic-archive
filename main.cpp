#include <bits/stdc++.h>

using namespace std;

struct Solver {
  int N = 0;
  int V = 0;
  vector<long long> A;
  vector<int> best_order;
  long long best_objective = LLONG_MIN;

  void input() {
    cin >> N;
    V = N * N;
    A.assign(V, 0);
    for (int r = 0; r < N; ++r) {
      for (int c = 0; c < N; ++c) {
        cin >> A[vid(r, c)];
      }
    }
  }

  int vid(int r, int c) const { return r * N + c; }

  pair<int, int> rc(int v) const { return {v / N, v % N}; }

  bool adjacent(int u, int v) const {
    if (u == v)
      return false;
    const auto [ur, uc] = rc(u);
    const auto [vr, vc] = rc(v);
    return abs(ur - vr) <= 1 && abs(uc - vc) <= 1;
  }

  long long calc_objective(const vector<int> &order) const {
    long long objective = 0;
    for (int t = 0; t < (int)order.size(); ++t) {
      objective += 1LL * t * A[order[t]];
    }
    return objective;
  }

  bool is_valid_hamilton_path(const vector<int> &order) const {
    if ((int)order.size() != V)
      return false;
    vector<char> used(V, false);
    for (int i = 0; i < V; ++i) {
      const int v = order[i];
      if (v < 0 || v >= V)
        return false;
      if (used[v])
        return false;
      used[v] = true;
      if (i > 0 && !adjacent(order[i - 1], order[i]))
        return false;
    }
    return true;
  }

  vector<int> build_plain_snake_order() const {
    vector<int> order;
    order.reserve(V);
    for (int r = 0; r < N; ++r) {
      if ((r & 1) == 0) {
        for (int c = 0; c < N; ++c)
          order.push_back(vid(r, c));
      } else {
        for (int c = N - 1; c >= 0; --c)
          order.push_back(vid(r, c));
      }
    }
    return order;
  }

  // Two-column round-trip:
  // - Handle strips (2 columns) recursively from left to right.
  // - Move to the next strip with a fixed detour that uses the last row
  //   and one row before it (red route image).
  // - After the deepest strip, unwind strip by strip (blue route image).
  // - For non-forced rows, outbound picks the smaller population cell,
  //   and inbound uses the remaining (larger) one.
  vector<int> build_two_column_round_trip_order(bool start_from_bottom) const {
    if ((N & 1) != 0 || N < 4)
      return build_plain_snake_order();

    vector<int> order;
    order.reserve(V);
    vector<char> used(V, false);
    bool ok = true;
    const int strips = N / 2;

    auto add_cell = [&](int r, int c) {
      if (!ok)
        return;
      if (r < 0 || r >= N || c < 0 || c >= N) {
        ok = false;
        return;
      }
      const int v = vid(r, c);
      if (used[v]) {
        ok = false;
        return;
      }
      if (!order.empty() && !adjacent(order.back(), v)) {
        ok = false;
        return;
      }
      used[v] = true;
      order.push_back(v);
    };

    auto choose_outbound_col = [&](int cL, int cR, int r,
                                   int forced_col) -> int {
      if (forced_col != -1)
        return forced_col;
      return (A[vid(r, cL)] <= A[vid(r, cR)]) ? cL : cR;
    };

    function<void(int)> from_top;
    function<void(int)> from_bottom;

    // Interface:
    // start: (0, 2*s), end: (1, 2*s)
    from_top = [&](int s) {
      const int cL = 2 * s;
      const int cR = cL + 1;
      const bool is_last = (s + 1 == strips);

      add_cell(0, cL);
      add_cell(0, cR);

      vector<int> inbound_col(N, -1);
      for (int r = 1; r < N; ++r) {
        int forced = -1;
        if (r == 1)
          forced = cR;
        if (!is_last && r == N - 2)
          forced = cL;
        if (r == N - 1)
          forced = cL;
        const int out_col = choose_outbound_col(cL, cR, r, forced);
        inbound_col[r] = (out_col == cL) ? cR : cL;
        add_cell(r, out_col);
      }

      add_cell(N - 1, cR);

      if (!is_last) {
        from_bottom(s + 1);
        add_cell(N - 2, cR);
        for (int r = N - 3; r >= 2; --r)
          add_cell(r, inbound_col[r]);
      } else {
        for (int r = N - 2; r >= 2; --r)
          add_cell(r, inbound_col[r]);
      }

      add_cell(1, cL);
    };

    // Interface:
    // start: (N-1, 2*s), end: (N-2, 2*s)
    from_bottom = [&](int s) {
      const int cL = 2 * s;
      const int cR = cL + 1;
      const bool is_last = (s + 1 == strips);

      add_cell(N - 1, cL);
      add_cell(N - 1, cR);

      vector<int> inbound_col(N, -1);
      for (int r = N - 2; r >= 0; --r) {
        int forced = -1;
        if (r == N - 2)
          forced = cR;
        if (!is_last && r == 1)
          forced = cL;
        if (r == 0)
          forced = cL;
        const int out_col = choose_outbound_col(cL, cR, r, forced);
        inbound_col[r] = (out_col == cL) ? cR : cL;
        add_cell(r, out_col);
      }

      add_cell(0, cR);

      if (!is_last) {
        from_top(s + 1);
        add_cell(1, cR);
        for (int r = 2; r <= N - 3; ++r)
          add_cell(r, inbound_col[r]);
      } else {
        for (int r = 1; r <= N - 3; ++r)
          add_cell(r, inbound_col[r]);
      }

      add_cell(N - 2, cL);
    };

    if (start_from_bottom) {
      from_bottom(0);
    } else {
      from_top(0);
    }

    if (!ok || (int)order.size() != V)
      return build_plain_snake_order();
    return order;
  }

  void consider_candidate(const vector<int> &candidate) {
    if (!is_valid_hamilton_path(candidate))
      return;
    const long long objective = calc_objective(candidate);
    if (objective > best_objective) {
      best_objective = objective;
      best_order = candidate;
    }
  }

  void solve() {
    vector<int> cand0 = build_two_column_round_trip_order(true);
    vector<int> cand1 = build_two_column_round_trip_order(false);

    consider_candidate(cand0);
    consider_candidate(cand1);

    reverse(cand0.begin(), cand0.end());
    reverse(cand1.begin(), cand1.end());
    consider_candidate(cand0);
    consider_candidate(cand1);

    if (best_order.empty()) {
      best_order = build_plain_snake_order();
      best_objective = calc_objective(best_order);
    }
  }

  void output() const {
    for (int v : best_order) {
      const auto [r, c] = rc(v);
      cout << r << ' ' << c << '\n';
    }
  }

  void report() const {
    const long long score = (best_objective + V / 2) / V;
    cerr << "mode: roundtrip-2col\n";
    cerr << "iterations: 0\n";
    cerr << "best score: " << score << '\n';
    cerr << "best objective: " << best_objective << '\n';
  }
};

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  Solver solver;
  solver.input();
  solver.solve();
  solver.report();
  solver.output();
  return 0;
}
