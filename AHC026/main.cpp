#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for(int i = 0; i < n; i++)

//-----------------以下から実装部分-----------------//

struct Solver{
    int n, m, now_box, op;
    bool f;
    vector<int> min_v, p_v;
    vector<vector<int>> box;
    vector<deque<int>> store;
    deque<int> tmp;

    Solver(){
        this->input();

    }

    void input(){
        cin >> n >> m;
        p_v.assign(n,-1);
        min_v.assign(m,-1);
        store.assign(m,{});
        box.assign(m,vector<int>(n/m));
        rep(i,m) {
            rep(j,n/m) {
                cin >> box[i][j]; box[i][j]--;
                p_v[box[i][j]] = i;
                min_v[i] = max(min_v[i], box[i][j]);
            }
        }
        return;
    }

    inline void operation2() {
        while( true ) {
            if( now_box == n ) break;
            bool f = false;
            rep(i,m) {
                if( !box[i].empty() && box[i].back() == now_box && store[p_v[now_box]].empty() )  {
                    cout << now_box+1 << " " << 0 << '\n' << flush;
                    box[i].pop_back();
                    now_box++;
                    f = true;
                    cerr << "now_box: " << now_box << "\n";
                    break;
                }
            }
            if( !f ) break;
        }
    }

    void solve(){
        // まずは貪欲 (山を操作するついでに単調負減少の山にする)
        int turn = 0;
        now_box = 0;
        cerr << "now_box: " << now_box << "\n";
        while( now_box < n ) {
            // 使った方が良い山を最初に求める (min_eleが一番高いやつから順に store の山として用いる)
            min_v.assign(m,1e9);
            vector<pair<int,int>> p;
            rep(i,m) {
                for(auto ele:box[i]) min_v[i] = min(min_v[i], ele);
                p.emplace_back(pair(min_v[i],i));
            }
            sort(p.begin(), p.end());
            reverse(p.begin(), p.end());
            vector<int> perm;
            rep(i,m) perm.emplace_back(p[i].second);

            // now_box が潜む山を単調減少山に変更する
            op = p_v[now_box];
            // deque<int> tmp;
            tmp = {};
            while( !box[op].empty() ) {
                int top = box[op].back();

                if( tmp.empty() || tmp[0] > top ) tmp.push_front(top);
                else {
                    int size_max = 1e9, size_idx = -1;
                    f = false;
                    for(auto i:perm) {
                        if( i == op ) continue;
                        if( size_max > (int)store[i].size() ) {
                            size_max = (int)store[i].size();
                            size_idx = i;
                        }
                        if( store[i].empty() || store[i].back() < tmp[0] ) {
                            // 予定通り山を除ける
                            if( tmp[0] == now_box ) {
                                if( tmp.size() > 1 ) cout << tmp[1]+1 << " " << i+1 << '\n' << flush;
                                cout << tmp[0]+1 << " " << 0 << '\n' << flush;
                                tmp.pop_front();
                                while( !tmp.empty() ) {
                                    store[i].emplace_back(tmp.front());
                                    p_v[tmp.front()] = i;
                                    tmp.pop_front();
                                }
                                now_box++;
                            }
                            else {
                                cout << tmp[0]+1 << " " << i+1 << '\n' << flush;
                                while( !tmp.empty() ) {
                                    store[i].emplace_back(tmp.front());
                                    p_v[tmp.front()] = i;
                                    tmp.pop_front();
                                }
                            }
                            operation2();
                            f = true;
                            break;
                        }
                    }

                    // 置ける山が無い場合 ⇒ 一番小さい sotre の山と合成
                    if( !f ) {
                        // op でも size_idx でも無い山に一次退避
                        int avoid = 0, pre_size;
                        if( op == 0 || size_idx == 0 ) avoid++;
                        if( avoid == 1 && (op == 1 || size_idx == 1) ) avoid++;
                        pre_size = store[avoid].size();
                        while( !store[size_idx].empty() || !tmp.empty() ) {
                            if( store[size_idx].empty() ) {
                                cout << tmp.back()+1 << " " << avoid+1 << '\n' << flush;
                                store[avoid].emplace_back(tmp.back());
                                p_v[tmp.back()] = avoid;
                                tmp.pop_back();
                            }
                            else if( tmp.empty() ) {
                                cout << store[size_idx].back()+1 << " " << avoid+1 << '\n' << flush;
                                store[avoid].emplace_back(store[size_idx].back());
                                p_v[store[size_idx].back()] = avoid;
                                store[size_idx].pop_back();
                            }
                            else {
                                if( store[size_idx].back() > tmp.back() ) {
                                    cout << store[size_idx].back()+1 << " " << avoid+1 << '\n' << flush;
                                    store[avoid].emplace_back(store[size_idx].back());
                                    p_v[store[size_idx].back()] = avoid;
                                    store[size_idx].pop_back();
                                }
                                else {
                                    cout << tmp.back()+1 << " " << avoid+1 << '\n' << flush;
                                    store[avoid].emplace_back(tmp.back());
                                    p_v[tmp.back()] = avoid;
                                    tmp.pop_back();
                                }
                            }
                        }
                        // reverse して 合成元の山 に戻す
                        while( store[avoid].size() > pre_size ) {
                            cout << store[avoid].back()+1 << " " << size_idx+1 << '\n' << flush;
                            store[size_idx].emplace_back(store[avoid].back());
                            p_v[store[avoid].back()] = size_idx;
                            store[avoid].pop_back();
                        }
                    }
                    tmp = {box[op].back()};
                }
                box[op].pop_back();
            }

            if( !tmp.empty() ) {
                int size_max = 1e9, size_idx = -1;
                f = false;
                for(auto i:perm) {
                    if( i == op ) continue;
                    if( size_max > (int)store[i].size() ) {
                        size_max = (int)store[i].size();
                        size_idx = i;
                    }
                    if( store[i].empty() || store[i].back() < tmp[0] ) {
                        // 予定通り山を除ける
                        if( tmp[0] == now_box ) {
                            if( tmp.size() > 1 ) cout << tmp[1]+1 << " " << i+1 << '\n' << flush;
                            cout << tmp[0]+1 << " " << 0 << '\n' << flush;
                            tmp.pop_front();
                            while( !tmp.empty() ) {
                                store[i].emplace_back(tmp.front());
                                p_v[tmp.front()] = i;
                                tmp.pop_front();
                            }
                            now_box++;
                        }
                        else {
                            cout << tmp[0]+1 << " " << i+1 << '\n' << flush;
                            while( !tmp.empty() ) {
                                store[i].emplace_back(tmp.front());
                                p_v[tmp.front()] = i;
                                tmp.pop_front();
                            }
                        }
                        operation2();
                        f = true;
                        break;
                    }
                }

                // 置ける山が無い場合 ⇒ 一番小さい sotre の山と合成
                if( !f ) {
                    // op でも size_idx でも無い山に一次退避
                    int avoid = 0, pre_size;
                    if( op == 0 || size_idx == 0 ) avoid++;
                    if( avoid == 1 && (op == 1 || size_idx == 1) ) avoid++;
                    pre_size = store[avoid].size();
                    while( !store[size_idx].empty() || !tmp.empty() ) {
                        if( store[size_idx].empty() ) {
                            cout << tmp.back()+1 << " " << avoid+1 << '\n' << flush;
                            store[avoid].emplace_back(tmp.back());
                            p_v[tmp.back()] = avoid;
                            tmp.pop_back();
                        }
                        else if( tmp.empty() ) {
                            cout << store[size_idx].back()+1 << " " << avoid+1 << '\n' << flush;
                            store[avoid].emplace_back(store[size_idx].back());
                            p_v[store[size_idx].back()] = avoid;
                            store[size_idx].pop_back();
                        }
                        else {
                            if( store[size_idx].back() > tmp.back() ) {
                                cout << store[size_idx].back()+1 << " " << avoid+1 << '\n' << flush;
                                store[avoid].emplace_back(store[size_idx].back());
                                p_v[store[size_idx].back()] = avoid;
                                store[size_idx].pop_back();
                            }
                            else {
                                cout << tmp.back()+1 << " " << avoid+1 << '\n' << flush;
                                store[avoid].emplace_back(tmp.back());
                                p_v[tmp.back()] = avoid;
                                tmp.pop_back();
                            }
                        }
                    }
                    // reverse して 合成元の山 に戻す
                    while( store[avoid].size() > pre_size ) {
                        cout << store[avoid].back()+1 << " " << size_idx+1 << '\n' << flush;
                        store[size_idx].emplace_back(store[avoid].back());
                        p_v[store[avoid].back()] = size_idx;
                        store[avoid].pop_back();
                    }
                }
            }

            // のけたおいた山を単調減少山に復元
            int empty_num = 0;
            rep(i,m) empty_num += (store[i].empty());
            while( empty_num < m ) {
                int m_v = -1, m_idx = -1;
                rep(i,m) {
                    if( store[i].empty() ) continue;
                    if( m_v < store[i].back() ) {
                        m_v = store[i].back();
                        m_idx = i;
                    }
                }
                cout << store[m_idx].back()+1 << " " << op+1 << '\n' << flush;
                box[op].emplace_back(store[m_idx].back());
                p_v[store[m_idx].back()] = op;
                store[m_idx].pop_back();
                if( store[m_idx].empty() ) empty_num++;
            }

            
            
            // 先頭に目標の箱がある場合は常に操作2
            operation2();

            turn++;
        }
        return;
    }
};

int main(){
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.solve();
    
    return 0;
}


