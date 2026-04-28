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

inline unsigned int rand_int() {
    static unsigned int tx = 123456789, ty=362436069, tz=521288629, tw=88675123;
    unsigned int tt = (tx^(tx<<11));
    tx = ty; ty = tz; tz = tw;
    return ( tw=(tw^(tw>>19))^(tt^(tt>>8)) );
}
#define TIME_LIMIT 1850

//-----------------以下から実装部分-----------------//

struct State {
    char ch;
    bool flag;
    int n, q, d, cnt, move_lim, top, under;
    vector<int> next;
    vector<vector<bool>> impossible;

    State() : n(0) {}
    explicit State(int _n, int _q, int _d) : n(_n), q(_q), d(_d), cnt(0), move_lim(-1), flag(false) {
        impossible.assign(n,vector<bool>(n,false));
    }
    
    inline bool relationUpdate(const int e1, const int e2, vector<vector<char>>& r) {
        // ~~~ 個々の関係性の更新 ~~~ //
        if( !q ) return false;
        cout << 1 << " " << 1 << " " << e1 << " " << e2 << '\n' << flush, cin >> ch;
        r[e1][e2] = ch, r[e2][e1] = reverseChar(ch);

        // 3段論法の要領で a < b && b < c なら a < c も情報追加 (warshall-froyd風)
        for(int k=0; k<n; k++) for(int i=0; i<n; i++) for(int j=0; j<n; j++) {
            if( r[i][k] != '?' && r[i][k] == r[k][j] ) r[i][j] = r[i][k];
            if( r[j][k] != '?' && r[j][k] == r[k][i] ) r[j][i] = r[j][k];
        }
        return q--;
    }

    inline bool relationUpdate(const int i1, const int i2, vector<vector<char>>& r, const vector<vector<int>>& latest) {
        // ~~ group間の関係性の更新 ~~~ //
        if( !q ) return false;
        cout << latest[i1].size() << " " << latest[i2].size() << " ";
        for(auto &&ele:latest[i1]) cout << ele << " ";
        for(auto &&ele:latest[i2]) cout << ele << " ";
        cout << '\n' << flush, cin >> ch;
        r[i1][i2] = ch, r[i2][i1] = reverseChar(ch);
        return q--;
    }

    inline void groupSort(int ele, vector<int>& rank, vector<vector<char>>& r, vector<vector<int>>& latest) {
        // ※ rank は降順で並んでいるとする
        if( rank.empty() ) return (void)rank.emplace_back(ele);
        
        vector<char> tmp(rank.size(),'?');
        if( !flag ) top = 0, under = rank.size();
        else under = rank.size();
        while( under-top > 1 ) {
            int mid = (top+under)/2;
            if( !relationUpdate(ele, rank[mid], r, latest) ) return;
            if( r[ele][rank[mid]] == '<' ) top = mid+1;
            else if( r[ele][rank[mid]] == '>' ) under = mid;
            else top = mid, under = mid;
            tmp[mid] = r[ele][rank[mid]];
        }
        if( top != rank.size() ) {
            if( tmp[top] == '?' && !relationUpdate(ele, rank[top], r, latest) ) return;
            if( tmp[top] == '<' || r[ele][rank[top]] == '<' ) top++;
        }
        rank.insert(rank.begin() + top, ele);
        for(int i=0; i<rank.size(); i++) {
            if( i < top ) r[ele][rank[i]] = '<', r[rank[i]][ele] = '>';
            if( i > top ) r[ele][rank[i]] = '>', r[rank[i]][ele] = '<';
        }
        return;
    }

    inline void smoothing(const int g1, const int g2, vector<vector<char>>& r_one, vector<vector<char>>& r_group, vector<int>& res, vector<vector<int>>& latest, vector<int>& rank) {
        int i1 = rand_int()%latest[g1].size(), i2 = rand_int()%latest[g2].size();
        int e1 = latest[g1][i1], e2 = latest[g2][i2];

        if( r_one[e1][e2] == '?' ) relationUpdate(e1, e2, r_one);
        if( r_one[e1][e2] == '=' || r_one[e1][e2] == '<' ) return (void)(cnt++);
        for(int i=0; i<n; i++) {
            if( impossible[e1][i] && r_one[e2][i] == '<' ) return (void)(cnt++);
            if( impossible[i][e2] && r_one[e1][i] == '>' ) return (void)(cnt++);
        }

        if( !q ) return;
        swap(latest[g1][i1], latest[g2][i2]);
        cout << latest[g1].size() << " " << latest[g2].size() << " ";
        for(auto &&ele:latest[g1]) cout << ele << " ";
        for(auto &&ele:latest[g2]) cout << ele << " ";
        cout << '\n' << flush, cin >> ch; q--;

        if( r_group[g1][g2] != ch ) {
            swap(latest[g1][i1], latest[g2][i2]);
            impossible[e1][e2] = true;
            cnt++;
        }
        else {
            swap(res[e1],res[e2]);
            next.assign({});
            for(int i=0; i<d; i++) if( rank[i] != g1 && rank[i] != g2 ) next.emplace_back(rank[i]);
            swap(rank, next);
            groupSort(g1, rank, r_group, latest); flag = true;
            groupSort(g2, rank, r_group, latest); flag = false;
            cnt = 0;
        }
        return;
    }

    inline void moving(const int g1, int g2, vector<vector<char>>& r_one, vector<vector<char>>& r_group, vector<int>& res, vector<vector<int>>& latest, vector<int>& rank) {
        if( latest[g1].size() == 1 ) return (void)(cnt++);
        
        int c = 0;
        for(auto ele:latest[g1]) c += (r_one[ele][move_lim] == '>' || ele == move_lim);
        if( c == latest[g1].size() ) return (void)(cnt++);

        int i1 = rand_int()%latest[g1].size();
        while( r_one[latest[g1][i1]][move_lim] == '>' ) i1 = rand_int()%latest[g1].size();
        int e1 = latest[g1][i1];

        latest[g2].emplace_back(e1);
        latest[g1].erase(latest[g1].begin()+i1);

        if( !q ) return;
        cout << latest[g1].size() << " " << latest[g2].size() << " ";
        for(auto &&ele:latest[g1]) cout << ele << " ";
        for(auto &&ele:latest[g2]) cout << ele << " ";
        cout << '\n' << flush, cin >> ch; q--;

        if( r_group[g1][g2] != ch ) {
            latest[g1].emplace_back(e1);
            latest[g2].pop_back();
            if( move_lim == -1 || r_one[e1][move_lim] == '<' ) move_lim = e1;
            cnt++;
        }
        else {
            res[e1] = g2;
            next.assign({});
            for(int i=0; i<d; i++) if( rank[i] != g1 && rank[i] != g2 ) next.emplace_back(rank[i]);
            swap(rank, next);
            groupSort(g1, rank, r_group, latest); flag = true;
            groupSort(g2, rank, r_group, latest); flag = false;
            cnt = 0;
        }
        return;
    }

    inline char reverseChar(const char& ch) {
        if( ch == '<' ) return '>';
        else if( ch == '>' ) return '<';
        return '=';
    }
};

struct Solver {
    char ch;
    int n, d, q, query, g1, g2;
    vector<int> res, g_rank;

    State state;
    vector<vector<int>> latest, group;
    vector<vector<char>> relation_one, relation_group;

    Solver(){
        this->input();
        state = State(n, q, d);
        // relation_one[i][j] : -1 (未測定), 0 (i<j), 1 (i==j), 2 (i>j)
        relation_group.assign(d,vector<char>(d,'?'));
        relation_one.assign(n,vector<char>(n,'?'));
        latest.assign(d,vector<int>{});
        g_rank.assign({});
        res.assign(n,0);
        for(int i=0; i<d; i++) relation_group[i][i] = '=';
        for(int i=0; i<n; i++) relation_one[i][i] = '=';
    }

    void input(){cin >> n >> d >> q;}
    void output(){
        // 余りがあったら浪費
        while( state.q-- ) cout << 1 << " " << 1 << " " << 0 << " " << 1 << '\n' << flush, cin >> ch;
        for(int i=0; i<n; i++) cout << res[i] << " ";
        cout << flush << endl;
        return;
    }

    inline void generateRest(vector<int>& rest) {
        int r1 = rand_int()%rest.size(), r2 = r1, r3 = r1, mid = -1;
        while( r1 == r2 ) r2 = rand_int()%rest.size();
        while( r1 == r3 || r2 == r3 ) r3 = rand_int()%rest.size();
        r1 = rest[r1], r2 = rest[r2], r3 = rest[r3];
        if( relation_one[r1][r2] == '?' ) state.relationUpdate(r1, r2, relation_one);
        if( relation_one[r2][r3] == '?' ) state.relationUpdate(r2, r3, relation_one);
        if( relation_one[r1][r3] == '?' ) state.relationUpdate(r1, r3, relation_one);
        if( relation_one[r2][r1] == relation_one[r1][r3] || relation_one[r2][r1] == '=' ) mid = r1;
        if( relation_one[r1][r2] == relation_one[r2][r3] || relation_one[r1][r2] == '=' ) mid = r2;
        if( relation_one[r1][r3] == relation_one[r3][r2] || relation_one[r1][r3] == '=' ) mid = r3;
        vector<int> left, right, middle;
        for(auto ele:rest) {
            if( mid != ele && relation_one[mid][ele] == '?' ) state.relationUpdate(mid, ele, relation_one);
            if( relation_one[mid][ele] == '>' ) left.emplace_back(ele);
            if( relation_one[mid][ele] == '<' ) right.emplace_back(ele);
            if( relation_one[mid][ele] == '=' ) middle.emplace_back(ele);
        }
        if( left.size() > 3 && q >= 8*n ) generateRest(left);
        else group.emplace_back(left);
        group.emplace_back(middle);
        if( right.size() > 3 ) generateRest(right);
        else group.emplace_back(right);

        return;
    }

    void generateInit() {
        if( q >= 4*n ) {
            vector<int> rest;
            for(int i=0; i<n; i++) rest.emplace_back(i);
            generateRest(rest);
            reverse(group.begin(), group.end());
            int idx = 0, delta = 1;
            for(auto v:group) {
                for(auto ele:v) {
                    res[ele] = idx;
                    latest[idx].emplace_back(ele);
                    if( (idx == 0 && delta == -1) || (idx == d-1 && delta == 1) ) delta *= -1;
                    else idx += delta;
                }
            }
        }
        else {
            for(int i=0; i<n; i++) {
                res[i] = i%d;
                latest[res[i]].emplace_back(i);
            }
        }
        for(int i=0; i<d; i++) state.groupSort(i, g_rank, relation_group, latest);
        return;
    }

    void solve(){
        utility::mytm.CodeStart();

        // 1. 初期解生成 & group大小管理
        // ⇒ 大きい要素が同じgroupにいると移動しにくい
        // ⇒ 大きい要素を重点的に分配した初期解を作成
        generateInit();

        // 2. group間の関係を調整 (2点 swap or 1点移動)
        while( state.q && utility::mytm.elapsed() <= TIME_LIMIT ) {
            g1 = g_rank[0], g2 = g_rank.back();
            if( state.cnt >= latest[g1].size()*latest[g2].size() && d > 2 ) {
                if( latest[g1].size() == 1 ) while( latest[g1].size() == 1 ) g1 = g_rank[rand_int()%(d-2)+1];
                else ( rand_int()%2 ? g1 : g2 ) = g_rank[rand_int()%(d-2)+1];
            }
            
            query = rand_int()%10;
            if( query ) state.smoothing(g1, g2, relation_one, relation_group, res, latest, g_rank);
            else state.moving(g1, g2, relation_one, relation_group, res, latest, g_rank);

            // cout << "# ";
            // for(int i=0; i<n; i++) cout << res[i] << " ";
            // cout << flush << endl;
        }
        cerr << "Rest Query Time: " << state.q << endl;
        return;
    }
};

int main(){
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.solve();
    solver.output();
    
    return 0;
}