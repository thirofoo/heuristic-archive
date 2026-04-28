#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for(int i = 0; i < n; i++)

using ll = long long;
using P  = pair<int, int>;
using T  = tuple<int, int, int>;

// right | down | left | up
#define DIR_NUM 4
vector<int> dx = {0, 1, 0, -1};
vector<int> dy = {1, 0, -1, 0};

inline bool outField(P now, int h, int w) {
  auto&& [x, y] = now;
  if(0 <= x && x < h && 0 <= y && y < w) return false;
  return true;
}

struct Arm {
  int base_x, base_y; // Arm の root 初期値
  int tip_x, tip_y;   // Arm の tip 座標
  int root_x, root_y, arm_tip_dir;
  ll situation;
  vector<int> arm_length, arm_rui;
  vector<vector<vector<ll>>> graph;

  explicit Arm(): arm_tip_dir(0) {}
  Arm(int root_x, int root_y, ll situation, vector<int> arm_length) {
    this->root_x      = root_x;
    this->root_y      = root_y;
    this->base_x      = root_x;
    this->base_y      = root_y;
    this->situation   = situation;
    this->arm_length  = arm_length;
    this->arm_tip_dir = 0;
    arm_rui           = arm_length;
    for(int i = arm_length.size() - 2; i >= 0; i--) {
      arm_rui[i] += arm_rui[i + 1];
    }
    tip_x = root_x, tip_y = root_y + arm_rui[0];
    createGraph();
    return;
  }

  void createGraph() {
    // Arm の先端を中心とした時に移動可能な範囲のグラフを作成
    // ※ グラフには移動後の状態が格納される
    vector<int> possible_dir = {0, 1, 2, 3};
    int size                 = 2 * arm_rui[0] + 1;
    graph.resize(size, vector<vector<ll>>(size, vector<ll>{}));

    auto dfs = [&](auto self, int depth, P now, int now_dir, ll now_situation) -> void {
      auto [now_x, now_y] = now;
      if(depth == arm_rui.size()) {
        if(outField(now, size, size)) return;
        graph[now_x][now_y].emplace_back(now_situation);
        return;
      }
      for(auto dir_d: possible_dir) {
        int n_dir      = (now_dir + dir_d + DIR_NUM) % DIR_NUM;
        int n_x        = now_x + arm_rui[depth] * (dx[n_dir] - dx[now_dir]);
        int n_y        = now_y + arm_rui[depth] * (dy[n_dir] - dy[now_dir]);
        ll n_situation = now_situation * DIR_NUM + dir_d;
        self(self, depth + 1, P(n_x, n_y), n_dir, n_situation);
      }
    };

    // Arm の初期形状に合わせて先端の座標を graph に合わせて設定
    int sx = arm_rui[0], sy = 2 * arm_rui[0];
    dfs(dfs, 0, P(sx, sy), 0, 0LL);

    rep(i, size) {}
    return;
  }

  inline vector<string> fold(ll after_sit) {
    // after_sit に Arm が一致するように折りたたみを行う関数
    // ※ 折り畳みは高々 2 回で遂行できる
    ll before_sit   = this->situation;
    this->situation = after_sit;
    vector<string> res(2, "");
    rep(i, 2) rep(j, 2 * (arm_length.size() + 1)) res[i] += ".";

    int max_rotate_time = 0;
    for(int i = arm_length.size(); i > 0; i--) {
      ll s1           = after_sit % DIR_NUM;
      ll s2           = before_sit % DIR_NUM;
      int rotate_time = min((s1 - s2 + DIR_NUM) % DIR_NUM, (s2 - s1 + DIR_NUM) % DIR_NUM);
      max_rotate_time = max(max_rotate_time, rotate_time);

      this->arm_tip_dir             = (this->arm_tip_dir + (s1 - s2 + DIR_NUM) % DIR_NUM) % DIR_NUM;
      rep(j, rotate_time) res[j][i] = ((s1 - s2 + DIR_NUM) % DIR_NUM == 1 ? 'R' : 'L');
      before_sit /= DIR_NUM;
      after_sit /= DIR_NUM;
    }
    while(max_rotate_time < 2) {
      res.pop_back();
      max_rotate_time++;
    }
    return res;
  }

  inline vector<string> fold(P next) {
    // next に Arm の先端が一致するように折りたたみを行う関数
    // ※ 上記 fold の座標指定 ver (座標から手数の少ない操作を選択する)
    int min_rotate_time = 1e9;
    ll after_sit        = 0;
    auto [nx, ny]       = next;
    nx                  = nx - root_x + graph.size() / 2;
    ny                  = ny - root_y + graph[0].size() / 2;
    assert(!graph[nx][ny].empty());

    rep(i, graph[nx][ny].size()) {
      ll cand_sit     = graph[nx][ny][i];
      ll tmp_sit      = situation;
      int rotate_time = 0;
      rep(j, arm_length.size()) {
        int s1      = cand_sit % DIR_NUM;
        int s2      = tmp_sit % DIR_NUM;
        rotate_time = max({rotate_time, min((s1 - s2 + DIR_NUM) % DIR_NUM, (s2 - s1 + DIR_NUM) % DIR_NUM)});
        cand_sit /= DIR_NUM;
        tmp_sit /= DIR_NUM;
      }
      if(rotate_time < min_rotate_time) {
        min_rotate_time = rotate_time;
        after_sit       = graph[nx][ny][i];
      }
    }
    tip_x = next.first, tip_y = next.second;
    return fold(after_sit);
  }

  inline vector<string> move(P delta) {
    // Arm 全体を delta だけ移動させる関数
    vector<string> res;
    auto [dx, dy] = delta;
    rep(i, abs(dx)) {
      string op = dx > 0 ? "D" : "U";
      rep(j, 2 * (arm_length.size() + 1) - 1) op += ".";
      res.emplace_back(op);
    }
    rep(i, abs(dy)) {
      string op = dy > 0 ? "R" : "L";
      rep(j, 2 * (arm_length.size() + 1) - 1) op += ".";
      res.emplace_back(op);
    }
    root_x += dx;
    root_y += dy;
    tip_x += dx;
    tip_y += dy;
    return res;
  }
};

struct Solver {
  int N, M, V;
  vector<string> s, t, answer;
  Arm arm;

  vector<pair<P, P>> moves;
  vector<P> before_points, after_points;

  // custom
  vector<int> custom_arm_length;
  vector<P> custom_arms;
  vector<P> custom_leafs;
  vector<string> custom_first_op;

  Solver() {
    this->input();
    // たこやき位置・目的地の整理
    rep(i, N) rep(j, N) {
      if(s[i][j] == '1' && t[i][j] == '1') continue;
      if(s[i][j] == '1') before_points.emplace_back(i, j);
      if(t[i][j] == '1') after_points.emplace_back(i, j);
    }
    // 移動量が減るように sort
    auto compare = [&](P& a, P& b) {
      auto [ax, ay] = a;
      auto [bx, by] = b;
      return ax < bx;
    };
    sort(before_points.begin(), before_points.end(), compare);
    sort(after_points.begin(), after_points.end(), compare);

    return;
  }

  void input() {
    cin >> N >> M >> V;
    s.resize(N);
    t.resize(N);
    rep(i, N) cin >> s[i];
    rep(i, N) cin >> t[i];
    return;
  }

  int output_V;
  vector<P> output_arms;
  int output_base_x, output_base_y;
  vector<string> output_op;

  void output_prepare() {
    if(V <= 7) {
      output_V      = arm.arm_length.size() + 1;
      rep(i, V) output_arms.emplace_back(i, arm.arm_length[i]);
      output_base_x = arm.base_x;
      output_base_y = arm.base_y;
      output_op     = answer;
    } else {
      // 答え出力
      vector<P> arms = custom_arms;
      output_V      = V;
      output_arms   = arms;
      output_base_x = arm.base_x;
      output_base_y = arm.base_y;

      // 最初の leaf 展開用の操作を追加
      int use_leaf  = V - 6;
      string pre_op = "";
      vector<string> tmp = custom_first_op;
      vector<string> new_answer;
      rep(i, 2) {
        string op = "";
        rep(j, arm.arm_length.size() + 1) op += ".";
        rep(j, use_leaf) op += tmp[i][j];
        while(op.size() < 2 * V) op += ".";

        bool flag = true;
        rep(j, op.size()) flag &= (op[j] == '.');
        if(flag) continue;
        output_op.emplace_back(op);
      }
      rep(i, answer.size()) {
        if(answer[i].size() == 2 * V) output_op.emplace_back(answer[i]);
        else {
          string op = "";
          rep(j, arm.arm_length.size() + 1) op += answer[i][j];
          op += answer[i][arm.arm_length.size()];
          rep(j, use_leaf - 1) op += ".";
          rep(j, arm.arm_length.size() + 1) op += ".";
          op += answer[i].back(); // 先端の配置操作
          while(op.size() < 2 * V) op += ".";
          output_op.emplace_back(op);
        }
      }
    }

    vector<string> new_output_op;
    rep(i, output_op.size()) {
      // 無駄な操作を merge 処理
      if(i != output_op.size() - 1 && output_op[i][0] != '.' && output_op[i].back() != 'P' && output_op[i + 1][0] == '.') {
        output_op[i + 1][0] = output_op[i][0];
        new_output_op.emplace_back(output_op[i + 1]);
        i++;
      }
      else new_output_op.emplace_back(output_op[i]);
    }
    swap(output_op, new_output_op);
    return;
  }

  void output() {
    cout << output_V << endl;
    rep(i, output_V - 1) cout << output_arms[i].first << " " << output_arms[i].second << endl;
    cout << output_base_x << " " << output_base_y << endl;
    rep(i, output_op.size()) cout << output_op[i] << endl;
    return;
  }

  void voldCleaning() {
    vector<P> leaf = custom_leafs;
    int use_leaf  = V - 6;
    auto rotate90 = [&](P p) -> P {
      auto [x, y] = p;
      return P(y, -x);
    };

    int point = 0;
    rep(i, N) rep(j, N) if(s[i][j] == '1' && t[i][j] == '1') point += 2;
    ll catch_sit = 0;

    while(point < 2 * M) {
      // 移動可能範囲を全探索 ⇒ 一番手が進む (score が高い) ところに進む
      double max_score = 0.0;
      ll max_situation = 0, max_x = 0, max_y = 0;
      rep(i, arm.graph.size()) rep(j, arm.graph[0].size()) {
        if(arm.graph[i][j].empty()) continue;
        rep(k, arm.graph[i][j].size()) {
          ll situation = arm.graph[i][j][k];
          double score = 0.0;
          // 移動後に先端が向いている方向を確認
          int after_dir = 0;
          while(situation) {
            after_dir = (after_dir + situation) % DIR_NUM;
            situation /= DIR_NUM;
          }

          rep(l, use_leaf) {
            auto [dx, dy]                 = leaf[l];
            rep(_, after_dir) tie(dx, dy) = rotate90(P(dx, dy));

            int x = arm.root_x + i - (int) arm.graph.size() / 2 + dx;
            int y = arm.root_y + j - (int) arm.graph[0].size() / 2 + dy;

            if(outField(P(x, y), N, N)) continue;
            if((catch_sit & (1LL << l)) && s[x][y] == '0' && t[x][y] == '1') score++;
            else if(!(catch_sit & (1LL << l)) && s[x][y] == '1' && t[x][y] == '0') score++;
          }
          int max_rotate_time = 0;
          int b_sit = arm.situation, a_sit = arm.graph[i][j][k];
          rep(l, arm.arm_length.size()) {
            int s1          = a_sit % DIR_NUM;
            int s2          = b_sit % DIR_NUM;
            max_rotate_time = max(max_rotate_time, min((s1 - s2 + DIR_NUM) % DIR_NUM, (s2 - s1 + DIR_NUM) % DIR_NUM));
            a_sit /= DIR_NUM;
            b_sit /= DIR_NUM;
          }

          if(score > max_score * max_rotate_time) {
            max_score     = score / max_rotate_time;
            max_situation = arm.graph[i][j][k];
            max_x         = arm.root_x + i - (int) arm.graph.size() / 2;
            max_y         = arm.root_y + j - (int) arm.graph[0].size() / 2;
          }
        }
      }
      point += max_score;
      if(max_score == 0) break;

      // 結果を leaf 有りに改竄
      vector<string> res = arm.fold(max_situation);
      arm.tip_x = max_x, arm.tip_y = max_y;

      rep(i, res.size()) {
        string op = "";
        // 本体移動 + arm 回転 part
        rep(j, arm.arm_length.size() + 1) op += res[i][j];
        op += res[i][arm.arm_length.size()];
        rep(j, use_leaf - 1) op += ".";

        // たこ焼き配置 part
        rep(j, arm.arm_length.size() + 1) op += ".";

        int after_dir = arm.arm_tip_dir;

        rep(j, use_leaf) {
          auto [dx, dy]                 = leaf[j];
          rep(_, after_dir) tie(dx, dy) = rotate90(P(dx, dy));

          int x = arm.tip_x + dx;
          int y = arm.tip_y + dy;

          if(outField(P(x, y), N, N)) {
            op += ".";
            continue;
          }

          // s (たこ焼き存在位置), catch_sit の更新も同時に行う
          if(i == res.size() - 1 && catch_sit & (1LL << j) && s[x][y] == '0' && t[x][y] == '1') {
            op += "P";
            s[x][y] = '1';
            catch_sit ^= (1LL << j);
          } else if(i == res.size() - 1 && !(catch_sit & (1LL << j)) && s[x][y] == '1' && t[x][y] == '0') {
            op += "P";
            s[x][y] = '0';
            catch_sit ^= (1LL << j);
          } else op += ".";
        }
        answer.emplace_back(op);
      }
    }

    // 掃除終了後は、残りの掃除に向けて準備
    // 1. 今掴んでいるたこ焼きを全て離す
    // 移動可能な範囲を全探索 ⇒ 全て離せるまで処理を繰り返す
    while(catch_sit != 0) {
      double max_score = 0.0;
      ll max_situation = 0, max_x = 0, max_y = 0;
      rep(i, arm.graph.size()) rep(j, arm.graph[0].size()) {
        if(arm.graph[i][j].empty()) continue;
        rep(k, arm.graph[i][j].size()) {
          ll situation = arm.graph[i][j][k];
          double score = 0.0;
          // 移動後に先端が向いている方向を確認
          int after_dir = 0;
          while(situation) {
            after_dir = (after_dir + situation) % DIR_NUM;
            situation /= DIR_NUM;
          }

          rep(l, use_leaf) {
            auto [dx, dy]                 = leaf[l];
            rep(_, after_dir) tie(dx, dy) = rotate90(P(dx, dy));

            int x = arm.root_x + i - (int) arm.graph.size() / 2 + dx;
            int y = arm.root_y + j - (int) arm.graph[0].size() / 2 + dy;

            if(outField(P(x, y), N, N)) continue;
            if((catch_sit & (1LL << l)) && s[x][y] == '0') score++;
          }
          int max_rotate_time = 0;
          int b_sit = arm.situation, a_sit = arm.graph[i][j][k];
          rep(l, arm.arm_length.size()) {
            int s1          = a_sit % DIR_NUM;
            int s2          = b_sit % DIR_NUM;
            max_rotate_time = max(max_rotate_time, min((s1 - s2 + DIR_NUM) % DIR_NUM, (s2 - s1 + DIR_NUM) % DIR_NUM));
            a_sit /= DIR_NUM;
            b_sit /= DIR_NUM;
          }

          if(score > max_score * max_rotate_time) {
            max_score     = (max_rotate_time == 0 ? (score == 0 ? 0 : 1e9) : score / max_rotate_time);
            max_situation = arm.graph[i][j][k];
            max_x         = arm.root_x + i - (int) arm.graph.size() / 2;
            max_y         = arm.root_y + j - (int) arm.graph[0].size() / 2;
          }
        }
      }

      // 結果を leaf 有りに改竄
      vector<string> res = arm.fold(max_situation);
      arm.tip_x = max_x, arm.tip_y = max_y;

      if(res.empty()) {
        string tmp_op = "";
        rep(i, 2 * V) tmp_op += ".";
        res.emplace_back(tmp_op);
      }
      rep(i, res.size()) {
        string op = "";
        // 本体移動 + arm 回転 part
        rep(j, arm.arm_length.size() + 1) op += res[i][j];
        op += res[i][arm.arm_length.size()];
        rep(j, use_leaf - 1) op += ".";

        // たこ焼き配置 part
        rep(j, arm.arm_length.size() + 1) op += ".";

        int after_dir = arm.arm_tip_dir;

        rep(j, use_leaf) {
          auto [dx, dy]                 = leaf[j];
          rep(_, after_dir) tie(dx, dy) = rotate90(P(dx, dy));

          int x = arm.tip_x + dx;
          int y = arm.tip_y + dy;

          if(outField(P(x, y), N, N)) {
            op += ".";
            continue;
          }

          // s (たこ焼き存在位置), catch_sit の更新も同時に行う
          if(i == res.size() - 1 && (catch_sit & (1LL << j)) && s[x][y] == '0') {
            op += "P";
            s[x][y] = '1';
            catch_sit ^= (1LL << j);
          } else op += ".";
        }
        answer.emplace_back(op);
      }
    }

    // 2. before_points, after_points を更新
    vector<P> new_before_points, new_after_points;
    rep(i, N) rep(j, N) {
      if(s[i][j] == '1' && t[i][j] == '1') continue;
      if(s[i][j] == '1') new_before_points.emplace_back(i, j);
      if(t[i][j] == '1') new_after_points.emplace_back(i, j);
    }
    swap(before_points, new_before_points);
    swap(after_points, new_after_points);
    // 移動量が減るように sort
    auto compare = [&](P& a, P& b) {
      auto [ax, ay] = a;
      auto [bx, by] = b;
      return ax < bx;
    };
    sort(before_points.begin(), before_points.end(), compare);
    sort(after_points.begin(), after_points.end(), compare);

    return;
  }

  void solve() {
    // ==================== 貪欲解 (ムチ戦法) ==================== //
    // 根幹となる Arm の作成
    vector<int> arm_length;
    if(V == 5) arm_length = {8, 4, 2, 1};
    else if(V == 6) arm_length = {min(16, (N + 1) / 2), 8, 4, 2, 1};
    else if(V == 7) arm_length = {min(16, (N + 1) / 2), 8, 4, 2, 1, 1};
    else arm_length = custom_arm_length;

    if(V <= 7) {
      // arm が長すぎてしまう場合は調整
      for(int i = 1; i < arm_length.size(); i++) {
        // 前と 1 : 2 になるように調整
        arm_length[i] = max(min(arm_length[i], (arm_length[i - 1]) / 2), 1);
      }
    }
    arm = Arm(N / 2, N / 2, 0, arm_length);

    // 0. V >= 8 の時は複数の leaf を使って初めに掃除
    if(V >= 8) this->voldCleaning();

    // 1. 残りを 1 Arm 1 leaf での貪欲解生成
    vector<P> deltas = {
        {0, 0}
    };
    rep(i, max(1, N + 1 - arm.arm_rui[0])) {
      deltas.emplace_back(i + 1, 0);
      deltas.emplace_back(0, i + 1);
      deltas.emplace_back(-(i + 1), 0);
      deltas.emplace_back(0, -(i + 1));
    }

    // 次の移動先を決定する関数
    auto next_move_delta = [&](P next) {
      int min_dis       = 1e9;
      P delta           = P(-1e9, -1e9);
      auto [to_x, to_y] = next;
      rep(i, deltas.size()) {
        auto [delta_x, delta_y] = deltas[i];
        auto [nx, ny]           = P(arm.base_x + delta_x, arm.base_y + delta_y);
        auto [gx, gy]           = P(arm.graph.size() / 2 + (to_x - nx), arm.graph[0].size() / 2 + (to_y - ny));
        // 移動先からたこ焼きが掴めるかの判定
        if(outField(P(gx, gy), arm.graph.size(), arm.graph[0].size()) || arm.graph[gx][gy].empty()) continue;
        int dis = abs(nx - arm.root_x) + abs(ny - arm.root_y);
        if(dis < min_dis) {
          min_dis = dis;
          delta   = P(nx - arm.root_x, ny - arm.root_y);
        }
      }
      assert(delta != P(-1e9, -1e9));
      return delta;
    };

    rep(i, before_points.size()) {
      auto [bx, by] = before_points[i];
      auto [ax, ay] = after_points[i];

      // 1. Arm の (base_x, base_y) からマンハッタン距離が 1 以下の点を移動候補として全体移動
      int min_dis        = 1e9;
      P delta            = next_move_delta(before_points[i]);
      vector<string> res = arm.move(delta);
      rep(j, res.size()) answer.emplace_back(res[j]);

      // 2. Arm の先端を目的地に移動
      res = arm.fold(before_points[i]);
      if(res.empty()) {
        string tmp_op = "";
        rep(i, 2 * (arm.arm_length.size() + 1)) tmp_op += ".";
        res.emplace_back(tmp_op);
      }
      res.back().back() = 'P';
      rep(j, res.size()) answer.emplace_back(res[j]);

      // 3. 目的地に移動
      delta = next_move_delta(after_points[i]);
      res   = arm.move(delta);
      rep(j, res.size()) answer.emplace_back(res[j]);

      // 4. 目的地に到達したら離す
      res = arm.fold(after_points[i]);
      if(res.empty()) {
        string tmp_op = "";
        rep(i, 2 * (arm.arm_length.size() + 1)) tmp_op += ".";
        res.emplace_back(tmp_op);
      }
      res.back().back() = 'P';
      rep(j, res.size()) answer.emplace_back(res[j]);
    }

    // 出力準備
    output_prepare();
    return;
  }
};

int main() {
  cin.tie(0);
  ios_base::sync_with_stdio(false);

  Solver solver;
  vector<Solver> custom_solvers;

  // solver を複製させて、custom な設定を行う
  Solver custom_solver1 = solver;
  custom_solver1.custom_arm_length = {1, 2, 4, 8, min(16, (custom_solver1.N + 1) / 2)};
  for(int i = custom_solver1.custom_arm_length.size() - 2; i >= 0; i--) custom_solver1.custom_arm_length[i] = max(min(custom_solver1.custom_arm_length[i], (custom_solver1.custom_arm_length[i + 1] + 1) / 2), 1);
  custom_solver1.custom_arms = {{0,  1}, {1,  2}, {2,  4}, {3,  8}, {4, min(16, (custom_solver1.N + 1) / 2)}, {4, min(16, (custom_solver1.N + 1) / 2)}, {5,  1}, {5,  2}, {5,  3}, {5,  4}, {5,  5}, {5,  6}, {5,  7}, {5,  8}};
  for(int i = custom_solver1.custom_arm_length.size() - 2; i >= 0; i--) custom_solver1.custom_arms[i].second = max(min(custom_solver1.custom_arms[i].second, (custom_solver1.custom_arms[i + 1].second + 1) / 2), 1);
  custom_solver1.custom_leafs = {{0,  0}, {0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, -7}, {0, -8}};
  custom_solver1.custom_first_op = {".LLLLLLLL", ".LLLLLLLL"};
  custom_solver1.solve();
  custom_solvers.emplace_back(custom_solver1);

  Solver custom_solver2 = solver;
  custom_solver2.custom_arm_length = {1, 2, 4, 8, min(16, (custom_solver2.N + 1) / 2)};
  for(int i = custom_solver2.custom_arm_length.size() - 2; i >= 0; i--) custom_solver2.custom_arm_length[i] = max(min(custom_solver2.custom_arm_length[i], (custom_solver2.custom_arm_length[i + 1] + 1) / 2), 1);
  custom_solver2.custom_arms = {{0,  1}, {1,  2}, {2,  4}, {3,  8}, {4, min(16, (custom_solver2.N + 1) / 2)}, {4, min(16, (custom_solver2.N + 1) / 2)}, {5,  1}, {5,  1}, {5,  1}, {5,  1}, {5,  2}, {5,  2}, {5,  2}, {5,  2}};
  for(int i = custom_solver2.custom_arm_length.size() - 2; i >= 0; i--) custom_solver2.custom_arms[i].second = max(min(custom_solver2.custom_arms[i].second, (custom_solver2.custom_arms[i + 1].second + 1) / 2), 1);
  custom_solver2.custom_leafs = {{ 0,  0}, { 0,  1}, { 1,  0}, { 0, -1}, {-1,  0}, { 0,  2}, { 2,  0}, { 0, -2}, {-2,  0}};
  custom_solver2.custom_first_op = {"..RRL.RRL", "...R...R."};
  custom_solver2.solve();
  custom_solvers.emplace_back(custom_solver2);

  Solver custom_solver3 = solver;
  custom_solver3.custom_arm_length = {min(16, (custom_solver3.N + 1) / 2), 8, 4, 2, 1};
  for(int i = 1; i < custom_solver3.custom_arm_length.size(); i++) custom_solver3.custom_arm_length[i] = max(min(custom_solver3.custom_arm_length[i], (custom_solver3.custom_arm_length[i - 1] + 1) / 2), 1);
  custom_solver3.custom_arms = {{0, min(16, (custom_solver3.N + 1) / 2)}, {1, 8}, {2, 4}, {3, 2}, {4, 1}, {4, 1}, {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5}, {5, 6}, {5, 7}, {5, 8}};
  for(int i = 1; i < custom_solver3.custom_arm_length.size(); i++) custom_solver3.custom_arms[i].second = max(min(custom_solver3.custom_arms[i].second, (custom_solver3.custom_arms[i - 1].second + 1) / 2), 1);
  custom_solver3.custom_leafs = {{0, 0}, {0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, -7}, {0, -8}};
  custom_solver3.custom_first_op = {".LLLLLLLL", ".LLLLLLLL"};
  custom_solver3.solve();
  custom_solvers.emplace_back(custom_solver3);

  Solver custom_solver4 = solver;
  custom_solver4.custom_arm_length = {1, 2, 4, 8, min(16, (custom_solver4.N + 1) / 2)};
  for(int i = custom_solver4.custom_arm_length.size() - 2; i >= 0; i--) custom_solver4.custom_arm_length[i] = max(min(custom_solver4.custom_arm_length[i], (custom_solver4.custom_arm_length[i + 1] + 1) / 2), 1);
  custom_solver4.custom_arms = {{0,  1}, {1,  2}, {2,  4}, {3,  8}, {4, min(16, (custom_solver4.N + 1) / 2)}, {4, min(16, (custom_solver4.N + 1) / 2)}, {5,  1}, {5,  2}, {5,  3}, {5,  4}, {5,  5}, {5,  6}, {5,  7}, {5,  8}};
  for(int i = custom_solver4.custom_arm_length.size() - 2; i >= 0; i--) custom_solver4.custom_arms[i].second = max(min(custom_solver4.custom_arms[i].second, (custom_solver4.custom_arms[i + 1].second + 1) / 2), 1);
  custom_solver4.custom_leafs = {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8}};
  custom_solver4.custom_first_op = {".........", "........."};
  custom_solver4.solve();
  custom_solvers.emplace_back(custom_solver4);

  sort(custom_solvers.begin(), custom_solvers.end(), [&](Solver& a, Solver& b) { return a.output_op.size() < b.output_op.size(); });
  custom_solvers[0].output();

  // cerr << "solver1 output size: " << custom_solver1.output_op.size() << endl;
  // cerr << "solver2 output size: " << custom_solver2.output_op.size() << endl;
  // cerr << "solver3 output size: " << custom_solver3.output_op.size() << endl;
  // cerr << "solver4 output size: " << custom_solver4.output_op.size() << endl;

  return 0;
}