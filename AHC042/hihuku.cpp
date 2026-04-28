#include <bits/stdc++.h>
using namespace std;
using Clock = chrono::steady_clock;

//////////////////////////
// 補助関数群
//////////////////////////

// (i,j) に記を置いた場合にマンハッタン距離≤2内のセル一覧を返す
vector<pair<int,int>> coverCells(int i, int j, int N) {
    vector<pair<int,int>> cells;
    for (int di = -2; di <= 2; di++) {
        for (int dj = -2; dj <= 2; dj++) {
            if (abs(di) + abs(dj) <= 2) {
                int ni = i + di, nj = j + dj;
                if (ni >= 0 && ni < N && nj >= 0 && nj < N)
                    cells.push_back({ni, nj});
            }
        }
    }
    return cells;
}

// 貪欲に「未覆われた1をできるだけ多く覆える位置に記を置く」
// 引数 grid は盤面（1:未カバー、0:カバー済み）を示すコピー
// 戻り値は、選んだ記のリスト（(行, 列) のペア）
vector<pair<int,int>> greedyCover(vector<vector<int>> grid, int N) {
    vector<pair<int,int>> sol;
    while (true) {
        int bestGain = 0;
        int bestI = -1, bestJ = -1;
        // 盤面全体を走査
        for (int i = 0; i < N; i++){
            for (int j = 0; j < N; j++){
                int gain = 0;
                auto cells = coverCells(i, j, N);
                for (auto &p : cells) {
                    int r = p.first, c = p.second;
                    if (grid[r][c] == 1)
                        gain++;
                }
                if (gain > bestGain) {
                    bestGain = gain;
                    bestI = i;
                    bestJ = j;
                }
            }
        }
        if (bestGain == 0)
            break;
        sol.push_back({bestI, bestJ});
        auto cells = coverCells(bestI, bestJ, N);
        for (auto &p : cells) {
            int r = p.first, c = p.second;
            grid[r][c] = 0;
        }
    }
    return sol;
}

// repairSolution: 与えられた部分解（kept には破壊せずに残す記が入っている）に対して、
// その記で覆われなかった1を、greedyCover で補完する。
// 元の盤面 original は、各セルが1または0（1は覆うべき）を示す。
vector<pair<int,int>> repairSolution(const vector<pair<int,int>> &kept, const vector<vector<int>> &original, int N) {
    // grid: コピーした元盤面
    vector<vector<int>> grid = original;
    // まず、kept にある記の効果で、grid上の1を0にする
    for (auto &m : kept) {
        auto cells = coverCells(m.first, m.second, N);
        for (auto &p : cells) {
            int r = p.first, c = p.second;
            grid[r][c] = 0;
        }
    }
    // greedyCover で残りの1を埋める
    vector<pair<int,int>> added = greedyCover(grid, N);
    // 結果は kept と added の結合
    vector<pair<int,int>> sol = kept;
    sol.insert(sol.end(), added.begin(), added.end());
    return sol;
}

//////////////////////////
// メイン：部分破壊＋焼きなまし
//////////////////////////

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;
    vector<vector<int>> original(N, vector<int>(N, 0));
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            cin >> original[i][j];
        }
    }
    
    // 初期解：単純な貪欲法
    vector<pair<int,int>> currentSol = greedyCover(original, N);
    vector<pair<int,int>> bestSol = currentSol;
    int bestCost = currentSol.size();
    
    // 焼きなましパラメータ
    double T = 100.0;
    const double T_min = 1e-3;
    const double coolingRate = 0.995;
    
    // 乱数準備
    mt19937 rng((unsigned)chrono::steady_clock::now().time_since_epoch().count());
    uniform_real_distribution<double> uni(0.0, 1.0);
    
    auto startTime = Clock::now();
    const double timeLimit = 10.0; // 1秒
    
    // 焼きなましループ
    while (chrono::duration<double>(Clock::now() - startTime).count() < timeLimit) {
        // 部分破壊：現解 currentSol のうち、ランダムに一定割合（例：20%）の記を破壊
        vector<pair<int,int>> kept;
        for (auto &m : currentSol) {
            // 20% の確率で破壊（採用しない）
            if (uni(rng) > 0.20) {
                kept.push_back(m);
            }
        }
        // 少なくとも1つは破壊する（全採用ならランダムに1個除去）
        if (kept.size() == currentSol.size() && !currentSol.empty()) {
            int randid = uniform_int_distribution<int>(0, currentSol.size()-1)(rng);
            kept.erase(kept.begin() + randid);
        }
        
        // repair: 残った記で覆われなかった1を greedy に補完する
        vector<pair<int,int>> candidate = repairSolution(kept, original, N);
        int candidateCost = candidate.size();
        
        // 焼きなましの受容基準
        int currentCost = currentSol.size();
        if (candidateCost < currentCost) {
            currentSol = candidate;
        } else {
            double prob = exp( -double(candidateCost - currentCost) / T );
            if (uni(rng) < prob) {
                currentSol = candidate;
            }
        }
        // 更新 best
        if (candidateCost < bestCost) {
            bestCost = candidateCost;
            bestSol = candidate;
        }
        T = max(T * coolingRate, T_min);
    }
    
    // 結果出力：まず記の個数、その後各記の位置 (行, 列)
    cout << bestSol.size() << "\n";
    for (auto &p : bestSol) {
        cout << p.first << " " << p.second << "\n";
    }
    
    return 0;
}
