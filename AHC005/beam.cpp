#include <bits/stdc++.h>
using namespace std;

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
            return (double)duration_cast<milliseconds>(system_clock::now() - start).count();
        }
    } mytm;
}

#define TIME_LIMIT 2950
//-----------------以下から実装部分-----------------//

// right | down | left | up
#define DIR_NUM 4
vector<int> dx = { 0, 1,  0, -1};
vector<int> dy = { 1, 0, -1,  0};
vector<char> dir_char = {'R', 'D', 'L', 'U'};

static const int road_max = 280; // 道路の総数の最大値
static const int beam_width = 50000; // ビーム幅

// Beam Search の Trace 用のノード番号
int now_id = 0;
inline int getId() {
    return now_id++;
}

struct Trace {
    char op;
    int move_num, parend_id;
    Trace() : parend_id(0) {}
    explicit Trace(char op, int move_num, int parend_id) : op(op), move_num(move_num), parend_id(parend_id) {}
};

struct State {
    int x, y, score, pre_dir, id;
    bitset<road_max> road_visited;
    State() {}
    explicit State(int x, int y, int score, int pre_dir, bitset<road_max> road_visited) : x(x), y(y), score(score), pre_dir(pre_dir), road_visited(road_visited) {
        id = getId();
    }

    bool operator < (const State &s) const {
        return score < s.score;
    }
    bool operator > (const State &s) const {
        return score > s.score;
    }
};

struct Road {
    // p1, p2 : 各方向において [p1, p2]
    // rev_p : dir とは逆方向の座標
    int dir, p1, p2, rev_p, id;
    Road() {}
    explicit Road(int dir, int p1, int p2, int rev_p, int id) : dir(dir), p1(p1), p2(p2), rev_p(rev_p), id(id) {}
};

struct Solver {
    int n, sx, sy, road_num;
    vector<string> field;
    vector<Road> roads;
    vector<vector<int>> cross_points_row; // 道路の交差点の座標 (横)
    vector<vector<int>> cross_points_col; // 道路の交差点の座標 (縦)
    vector<vector<vector<int>>> cross_road_ids; // 交差点を成す道路の id
    vector<vector<vector<vector<int>>>> dist_from_to; // 任意の頂点から任意の頂点までの最短距離

    int best_score;
    State best_state;
    string answer;

    // 任意の頂点 -> (sx, sy) の経路復元用
    vector<vector<pair<int, int>>> prev;
    vector<vector<int>> dist;

    Solver() {
        this->input();
        
        // 道路識（横 or 縦）
        int tate = 0, yoko = 0;
        road_num = 0;
        for(int i=1; i<=n; i++) {
            // 横道路
            int start_y = -1;
            for(int j=1; j<=n+1; j++) {
                if(field[i][j] == '#') {
                    if(start_y == -1) continue;
                    else if(j - start_y > 2) {
                        roads.emplace_back(Road(0, start_y, j-1, i, road_num++));
                        // cerr << "Road (yoko) : " << roads.back().p1 << " " << roads.back().p2 << endl;
                        yoko++;
                    }
                    start_y = -1;
                }
                else if(start_y == -1) start_y = j;
            }
        }
        for(int j=1; j<=n; j++) {
            // 縦道路
            int start_x = -1;
            for(int i=1; i<=n+1; i++) {
                if(field[i][j] == '#') {
                    if(start_x == -1) continue;
                    else if(i - start_x > 2) {
                        roads.emplace_back(Road(1, start_x, i-1, j, road_num++));
                        // cerr << "Road (tate) : " << roads.back().p1 << " " << roads.back().p2 << endl;
                        tate++;
                    }
                    start_x = -1;
                }
                else if(start_x == -1) start_x = i;
            }
        }

        // 縦横の交差点を前計算しておく
        sort(roads.begin(), roads.end(), [](const Road &a, const Road &b) {
            return a.dir < b.dir;
        });

        cross_points_row.resize(n+2);
        cross_points_col.resize(n+2);
        cross_road_ids.resize(n+2, vector(n+2, vector<int>()));
        for(int i=0; i<yoko; i++) {
            for(int j=yoko; j<road_num; j++) {
                // 横道路と縦道路が交差してる時
                if(roads[i].p1 <= roads[j].rev_p && roads[j].rev_p <= roads[i].p2
                 && roads[j].p1 <= roads[i].rev_p && roads[i].rev_p <= roads[j].p2) {
                    cross_points_row[roads[i].rev_p].emplace_back(roads[j].rev_p);
                    cross_points_col[roads[j].rev_p].emplace_back(roads[i].rev_p);
                    cross_road_ids[roads[i].rev_p][roads[j].rev_p].emplace_back(roads[i].id);
                    cross_road_ids[roads[i].rev_p][roads[j].rev_p].emplace_back(roads[j].id);
                    // cerr << "x: " << x << " y: " << y << '\n' << flush;
                    // cerr << "Road (cross) : " << roads[i].rev_p << " " << roads[j].rev_p << '\n' << flush;
                }
            }
        }
        for(int i=0; i<n+2; i++) {
            sort(cross_points_row[i].begin(), cross_points_row[i].end());
            sort(cross_points_col[i].begin(), cross_points_col[i].end());
        }

        cerr << "dijkstra start" << '\n' << flush;

        // 予めスタート地点から各地点までの最短距離を dijkstra で前計算
        // ※ 経路復元も出来る用にしておく
        prev.resize(n+2, vector(n+2, pair(-1, -1)));
        dist.resize(n+2, vector(n+2, 100000000));
        priority_queue<pair<int, tuple<int, int, int, int>>> pq1;
        priority_queue<pair<int, pair<int, int>>> pq2;
        pq1.push(pair(0, tuple(sx, sy, -1, -1)));

        while(!pq1.empty()) {
            auto now = pq1.top(); pq1.pop();
            auto [x, y, px, py] = now.second;
            if(dist[x][y] < now.first) continue;
            dist[x][y] = now.first;
            prev[x][y] = pair(px, py);

            for(int dir=0; dir<DIR_NUM; dir++) {
                int nx = x + dx[dir], ny = y + dy[dir];
                if(field[nx][ny] == '#') continue;
                if(dist[nx][ny] <= dist[x][y] + field[nx][ny] - '0') continue;
                pq1.push(pair(dist[x][y] + field[nx][ny] - '0', tuple(nx, ny, x, y)));
            }
        }

        cerr << "dist_from_to-floyd start" << '\n' << flush;
        cerr << "road num: " << road_num << endl;

        // 全点間の最短距離を dijkstra で前計算
        // ※ 交差点は最大で 280 個なので、O(N * NlogN) も十分間に合う
        vector<vector<int>> tmp_dist(n+2, vector(n+2, 100000000));
        dist_from_to.resize(n+2, vector(n+2, vector(n+2, vector(n+2, 100000000))));
        for(int x1=1; x1<=n; x1++) for(int y1=1; y1<=n; y1++) {
            for(int x2=1; x2<=n; x2++) for(int y2=1; y2<=n; y2++) {
                dist_from_to[x1][y1][x2][y2] = abs(x1-x2) + abs(y1-y2);
            }
        }

        for(int x=1; x<=n; x++) for(auto y : cross_points_row[x]) {
            pq2.push(pair(0, pair(x, y)));
            tmp_dist.assign(n+2, vector(n+2, 100000000));
            while(!pq2.empty()) {
                auto now = pq2.top(); pq2.pop();
                auto [nx, ny] = now.second;
                if(tmp_dist[nx][ny] < now.first) continue;
                tmp_dist[nx][ny] = now.first;

                for(int dir=0; dir<DIR_NUM; dir++) {
                    int nnx = nx + dx[dir], nny = ny + dy[dir];
                    if(field[nnx][nny] == '#') continue;
                    if(tmp_dist[nnx][nny] <= tmp_dist[nx][ny] + field[nnx][nny] - '0') continue;
                    pq2.push(pair(tmp_dist[nx][ny] + field[nnx][nny] - '0', pair(nnx, nny)));
                }
            }
            // tmp_dist を dist_from_to にコピー
            for(int xx=1; xx<=n; xx++) for(int yy=1; yy<=n; yy++) {
                dist_from_to[x][y][xx][yy] = tmp_dist[xx][yy];
            }
        }

        cerr << "yoko: " << yoko << " tate: " << tate << '\n' << flush;
        cerr << "road_num: " << road_num << '\n' << flush;

        for(int x=1; x<=n; x++) {
            for(int y=1; y<=n; y++) {
                cerr << (dist_from_to[sx][sy][x][y] == 100000000 ? -1 : dist_from_to[sx][sy][x][y]) << ' ';
            }
            cerr << '\n';
        }

        // cerr << "road num: " << road_num << endl;
        return;
    }

    void input() {
        cin >> n >> sx >> sy;
        sx++, sy++;
        field.resize(n+2, "");
        for(int i=0; i<n+2; i++) {
            field[0] += "#";
            field[n+1] += "#";
        }
        for(int i=1; i<=n; i++) {
            cin >> field[i];
            field[i] = "#" + field[i] + "#";
        }
        return;
    }

    void output() {
        cout << answer << endl;
        return;
    }

    void solve() {
        /*
        ========== Beam Search 解法 ==========
        - 初期地点から直近の交差点まで行く 4 近傍でビームサーチ
        - 全ての道路を通ったかどうかを bit で持つ
        - 通り終わったらその状態は元の位置まで最短距離で戻って終了
        - 評価関数は通った道路の長さ
            - ただ最後にスタートに戻りにくいと最悪なので、そこもペナルティで入れたい
        */
       cerr << "Beam Search Start" << '\n' << flush;
        answer = beam_search();
        return;
    }

    string beam_search() {
        // スコアが大きい方が良い ⇒ 昇順で持つ
        priority_queue<State, vector<State>, greater<State>> beam, next_beam;
        beam.push(State(sx, sy, 0, -1, bitset<road_max>(0)));

        vector<Trace> traces;
        traces.emplace_back(Trace{' ', 0, now_id});

        State now;
        bitset<road_max> next_visited;
        best_score = -1;
        int best_id = -1, best_x = -1, best_y = -1;

        // ターンは road_num 回以上かかる時がありそう（同じ道を折り返す必要があるケース）
        // ⇒ 2 * road_num で行う

        // 盤面重複除去用
        set<tuple<int, int, string>> duplicate;

        int turn = 0, best_popcnt = 0;
        bool flag = true;
        while(flag && !beam.empty()) {
            cerr << "turn: " << turn << '\n' << flush;
            cerr << "best_state: " << beam.top().x << " " << beam.top().y << " " << beam.top().score << " " << beam.top().road_visited << '\n' << flush;
            cerr << "popcnt: " << beam.top().road_visited.count() << '\n' << flush;

            while(!beam.empty()) {
                now = beam.top(); beam.pop();
                // cerr << "now: " << now.x << " " << now.y << " " << now.score << " " << now.road_visited << '\n' << flush;
                int nx, ny;

                // 4 方向近傍を試す
                for(int dir=0; dir<DIR_NUM; dir++) {
                    // 交差点に到達した時
                    int next_score = now.score;
                    if(dir == 0) { // right
                        // upper_bound で求まる
                        auto itr = upper_bound(cross_points_row[now.x].begin(), cross_points_row[now.x].end(), now.y);
                        if(itr == cross_points_row[now.x].end()) continue;
                        nx = now.x, ny = *itr;
                    }
                    else if(dir == 1) { // down
                        // upper_bound で求まる
                        auto itr = upper_bound(cross_points_col[now.y].begin(), cross_points_col[now.y].end(), now.x);
                        if(itr == cross_points_col[now.y].end()) continue;
                        nx = *itr, ny = now.y;
                    }
                    else if(dir == 2) { // left
                        // lower_bound して -1 すれば求まる
                        auto itr = lower_bound(cross_points_row[now.x].begin(), cross_points_row[now.x].end(), now.y);
                        if(itr == cross_points_row[now.x].begin()) continue;
                        itr--;
                        nx = now.x, ny = *itr;
                    }
                    else if(dir == 3) { // up
                        // lower_bound して -1 すれば求まる
                        auto itr = lower_bound(cross_points_col[now.y].begin(), cross_points_col[now.y].end(), now.x);
                        if(itr == cross_points_col[now.y].begin()) continue;
                        itr--;
                        nx = *itr, ny = now.y;
                    }
                    // 次の交差点の今の場所が繋がっていない場合はスキップ
                    bool same = false;
                    for(int i=0; i<cross_road_ids[now.x][now.y].size(); i++) {
                        for(int j=0; j<cross_road_ids[nx][ny].size(); j++) {
                            same |= (cross_road_ids[now.x][now.y][i] == cross_road_ids[nx][ny][j]);
                        }
                    }
                    if(!same) continue;

                    // 次の交差点に行った時に新たに通る道路を追加
                    next_visited = now.road_visited;
                    for(int i=0; i<cross_road_ids[nx][ny].size(); i++) {
                        int next_road = cross_road_ids[nx][ny][i];
                        if(now.road_visited[next_road]) continue;
                        next_visited.set(next_road);
                        next_score += (roads[next_road].rev_p - n/2) * (roads[next_road].rev_p - n/2) * 100000;
                    }
                    
                    // 既に通ったことがある場合はスキップ
                    // if(duplicate.count(tuple(nx, ny, next_visited.to_string()))) continue;
                    // duplicate.insert(tuple(nx, ny, next_visited.to_string()));

                    // Trace を追加
                    traces.emplace_back(Trace{dir_char[dir], max(abs(nx-now.x), abs(ny-now.y)), now.id});

                    next_score -= dist[nx][ny];
                    next_beam.push(State(nx, ny, next_score, dir, next_visited));

                    // 全ての道路を通った時
                    if(next_visited.count() == road_num && best_score < next_score) {
                        // best_score 更新したら Trace の id を更新
                        best_score = next_score;
                        best_id = now_id;
                        best_x = nx;
                        best_y = ny;
                        flag = false;
                    }
                    if(next_beam.size() > beam_width) next_beam.pop();
                }
            }
            swap(beam, next_beam);
            turn++;
        }
        // best_id = now_id;
        // cerr << "Best Score : " << best_score << '\n';
        cerr << "Best Road : " << best_popcnt << '\n';
        cerr << "Best Road : " << best_x << " " << best_y << '\n';
        cerr << "Best Road : " << best_id << '\n';

        // Trace Back
        string res = "";
        while(best_id > 0) {
            cerr << "best_id: " << best_id << '\n';
            int time = traces[best_id].move_num;
            while(time-- > 0) res += traces[best_id].op;
            best_id = traces[best_id].parend_id;
        }
        reverse(res.begin(), res.end());
        
        // 最後に (best_x, best_y) -> (sx, sy) まで戻る
        cerr << "res: " << res << '\n';
        while(best_x != sx || best_y != sy) {
            auto [px, py] = prev[best_x][best_y];
            int time = max(abs(px-best_x), abs(py-best_y));
            if(px == best_x) {
                if(py < best_y) while(time-- > 0) res += 'L';
                else while(time-- > 0) res += 'R';
            }
            else {
                if(px < best_x) while(time-- > 0) res += 'U';
                else while(time-- > 0) res += 'D';
            }
            best_x = px;
            best_y = py;
        }
        return res;
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