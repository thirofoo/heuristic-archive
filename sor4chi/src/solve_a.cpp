#include <bits/stdc++.h>
using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, AK, AM, AW;
    cin >> N >> AK >> AM >> AW;

    // wall_v[i]: length N-1, wall_v[i][j] = wall between (i,j) and (i,j+1)
    vector<string> vwalls(N), hwalls(N - 1);
    for (int i = 0; i < N; i++) cin >> vwalls[i];
    for (int i = 0; i < N - 1; i++) cin >> hwalls[i];

    // Strategy: decompose grid into segments along columns (split by horizontal walls)
    // or rows (split by vertical walls), pick whichever yields fewer total states.
    //
    // Each segment of length > 1 gets a U-turn automaton robot (2 states).
    // Each segment of length 1 gets a stationary robot (1 state).
    // No new walls are added (A_W = 1000 makes walls very expensive).

    // Count total states for column-based decomposition
    auto count_col_states = [&]() {
        int total = 0;
        for (int j = 0; j < N; j++) {
            int start = 0;
            for (int i = 0; i < N; i++) {
                bool is_end = (i == N - 1) || (hwalls[i][j] == '1');
                if (is_end) {
                    int len = i - start + 1;
                    total += (len == 1) ? 1 : 2;
                    start = i + 1;
                }
            }
        }
        return total;
    };

    // Count total states for row-based decomposition
    auto count_row_states = [&]() {
        int total = 0;
        for (int i = 0; i < N; i++) {
            int start = 0;
            for (int j = 0; j < N; j++) {
                bool is_end = (j == N - 1) || (vwalls[i][j] == '1');
                if (is_end) {
                    int len = j - start + 1;
                    total += (len == 1) ? 1 : 2;
                    start = j + 1;
                }
            }
        }
        return total;
    };

    int col_states = count_col_states();
    int row_states = count_row_states();
    bool use_columns = col_states <= row_states;

    // Generate robots
    struct Robot {
        int m, si, sj;
        char sd;
    };
    vector<Robot> robots;

    if (use_columns) {
        // Column-based: robots bounce up/down within segments
        for (int j = 0; j < N; j++) {
            int start = 0;
            for (int i = 0; i < N; i++) {
                bool is_end = (i == N - 1) || (hwalls[i][j] == '1');
                if (is_end) {
                    int len = i - start + 1;
                    robots.push_back({len == 1 ? 1 : 2, start, j, 'D'});
                    start = i + 1;
                }
            }
        }
    } else {
        // Row-based: robots bounce left/right within segments
        for (int i = 0; i < N; i++) {
            int start = 0;
            for (int j = 0; j < N; j++) {
                bool is_end = (j == N - 1) || (vwalls[i][j] == '1');
                if (is_end) {
                    int len = j - start + 1;
                    robots.push_back({len == 1 ? 1 : 2, i, start, 'R'});
                    start = j + 1;
                }
            }
        }
    }

    // Output
    cout << robots.size() << "\n";
    for (auto& r : robots) {
        cout << r.m << " " << r.si << " " << r.sj << " " << r.sd << "\n";
        if (r.m == 1) {
            // Stationary robot: always turn right, stays on this cell
            // Cycle: (r,c,D,0) -> (r,c,L,0) -> (r,c,U,0) -> (r,c,R,0) -> repeat
            cout << "R 0 R 0\n";
        } else {
            // U-turn automaton: go forward until wall, then U-turn
            // State 0: no wall -> F(stay 0); wall -> R(go 1)
            // State 1: always R(go 0)
            // Bounces back and forth within the segment
            cout << "F 0 R 1\n";
            cout << "R 0 R 0\n";
        }
    }

    // No new walls
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N - 1; j++) cout << '0';
        cout << "\n";
    }
    for (int i = 0; i < N - 1; i++) {
        for (int j = 0; j < N; j++) cout << '0';
        cout << "\n";
    }

    return 0;
}
