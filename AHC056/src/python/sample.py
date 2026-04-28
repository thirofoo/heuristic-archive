# pip install python-sat
from pysat.formula import CNF
from pysat.solvers import Glucose3
from collections import deque

# 方向のマッピング
DIRS = ['U', 'D', 'L', 'R']
DX = {'U': -1, 'D': 1, 'L': 0, 'R': 0}
DY = {'U': 0, 'D': 0, 'L': -1, 'R': 1}
DIR_INDEX = {'U': 0, 'D': 1, 'L': 2, 'R': 3}

def bfs_path(N, vwall, hwall, start, goal):
    """(sx,sy) -> (tx,ty) の最短経路 (マス列) を BFS で求める"""
    sx, sy = start
    tx, ty = goal
    if start == goal:
        return [start]
    INF = 10**9
    dist = [[INF]*N for _ in range(N)]
    par = [[(-1, -1)]*N for _ in range(N)]
    q = deque()
    dist[sx][sy] = 0
    q.append((sx, sy))

    def can_up(i, j):
        return i > 0 and hwall[i-1][j] == '0'
    def can_down(i, j):
        return i+1 < N and hwall[i][j] == '0'
    def can_left(i, j):
        return j > 0 and vwall[i][j-1] == '0'
    def can_right(i, j):
        return j+1 < N and vwall[i][j] == '0'

    while q:
        x, y = q.popleft()
        d = dist[x][y]
        if (x, y) == (tx, ty):
            break
        # UDLR の順は適当
        if can_up(x,y) and dist[x-1][y] > d+1:
            dist[x-1][y] = d+1
            par[x-1][y] = (x, y)
            q.append((x-1, y))
        if can_down(x,y) and dist[x+1][y] > d+1:
            dist[x+1][y] = d+1
            par[x+1][y] = (x, y)
            q.append((x+1, y))
        if can_left(x,y) and dist[x][y-1] > d+1:
            dist[x][y-1] = d+1
            par[x][y-1] = (x, y)
            q.append((x, y-1))
        if can_right(x,y) and dist[x][y+1] > d+1:
            dist[x][y+1] = d+1
            par[x][y+1] = (x, y)
            q.append((x, y+1))

    if dist[tx][ty] == INF:
        # 全マス到達可能のはずなので来ない
        return [start]

    path = []
    cur = (tx, ty)
    while cur != (sx, sy):
        path.append(cur)
        cur = par[cur[0]][cur[1]]
    path.append((sx, sy))
    path.reverse()
    return path

def cells_to_moves(cells):
    """マス列 -> U/D/L/R 列"""
    moves = []
    for i in range(len(cells)-1):
        x1,y1 = cells[i]
        x2,y2 = cells[i+1]
        if x2 == x1-1 and y2 == y1: moves.append('U')
        elif x2 == x1+1 and y2 == y1: moves.append('D')
        elif x2 == x1 and y2 == y1-1: moves.append('L')
        elif x2 == x1 and y2 == y1+1: moves.append('R')
        else:
            raise ValueError("Invalid path step")
    return moves

def build_cnf_for_CQ(N, pathCells, moves, C, Q):
    """
    与えられた pathCells, moves に対して、色数 C, 状態数 Q で実現可能かどうかの SAT を構築する。
    色は静的（A(c,q)=c とする）、同じマスはいつも同じ色、という割り切り。
    """
    L = len(moves)  # ステップ数
    cnf = CNF()

    # 変数番号の割り当て
    # xc[t][c]: t=0..L, c=0..C-1
    # xq[t][q]: t=0..L, q=0..Q-1
    # dir[c][q][d]: c=0..C-1, q=0..Q-1, d=0..3 (U,D,L,R)
    # nxt[c][q][q2]: c=0..C-1, q=0..Q-1, q2=0..Q-1

    def var_xc(t, c):
        return 1 + t*C + c
    base_xq = 1 + (L+1)*C
    def var_xq(t, q):
        return base_xq + t*Q + q
    base_dir = base_xq + (L+1)*Q
    def var_dir(c, q, d):
        return base_dir + (c*Q + q)*4 + d
    base_nxt = base_dir + C*Q*4
    def var_nxt(c, q, q2):
        return base_nxt + (c*Q + q)*Q + q2

    # 1) 各 t で色の one-hot
    for t in range(L+1):
        # at least one
        cnf.append([var_xc(t,c) for c in range(C)])
        # at most one (pairwise)
        for c1 in range(C):
            for c2 in range(c1+1, C):
                cnf.append([-var_xc(t,c1), -var_xc(t,c2)])

    # 同じマスでの色一致
    cell_times = {}
    for t,(x,y) in enumerate(pathCells):
        cell_times.setdefault((x,y), []).append(t)
    for (x,y), times in cell_times.items():
        if len(times) <= 1: continue
        for i in range(len(times)):
            for j in range(i+1, len(times)):
                t1, t2 = times[i], times[j]
                for c in range(C):
                    # xc[t1,c] ↔ xc[t2,c]
                    cnf.append([-var_xc(t1,c), var_xc(t2,c)])
                    cnf.append([-var_xc(t2,c), var_xc(t1,c)])

    # 2) 各 t で状態の one-hot
    for t in range(L+1):
        cnf.append([var_xq(t,q) for q in range(Q)])
        for q1 in range(Q):
            for q2 in range(q1+1, Q):
                cnf.append([-var_xq(t,q1), -var_xq(t,q2)])
    # 初期状態 q0 = 0
    cnf.append([var_xq(0,0)])

    # 3) dir[c][q][d] one-hot
    for c in range(C):
        for q in range(Q):
            cnf.append([var_dir(c,q,d) for d in range(4)])
            for d1 in range(4):
                for d2 in range(d1+1,4):
                    cnf.append([-var_dir(c,q,d1), -var_dir(c,q,d2)])

    # 4) nxt[c][q][q2] one-hot
    for c in range(C):
        for q in range(Q):
            cnf.append([var_nxt(c,q,q2) for q2 in range(Q)])
            for q1 in range(Q):
                for q2 in range(q1+1, Q):
                    cnf.append([-var_nxt(c,q,q1), -var_nxt(c,q,q2)])

    # 5) move の整合性:
    #    各 t について、moves[t] に対応しない方向 d' は、(c_t,q_t) では取れない
    for t in range(L):
        mt = moves[t]
        wanted_d = DIR_INDEX[mt]
        for c in range(C):
            for q in range(Q):
                for d in range(4):
                    if d == wanted_d: continue
                    cnf.append([
                        -var_xc(t,c),
                        -var_xq(t,q),
                        -var_dir(c,q,d)
                    ])

    # 6) 状態遷移の整合性:
    #   q_{t+1} = S(c_t, q_t)
    for t in range(L):
        for c in range(C):
            for q in range(Q):
                for q2 in range(Q):
                    cnf.append([
                        -var_xc(t,c),
                        -var_xq(t,q),
                        -var_nxt(c,q,q2),
                        var_xq(t+1,q2)
                    ])

    return cnf, var_xc, var_xq, var_dir, var_nxt

def solve_with_sat(N, vwall, hwall, targets, T,
                   C_max=4, Q_max=4):
    """
    小さいケース用。
    C+Q を小さい順に試して SAT で解く。
    見つかったら AtCoder 形式の (C,Q,s,rules) を返す。
    解けなければ None を返す。
    """
    # 1. 最短経路を繋いで pathCells, moves を作る
    pathCells = [targets[0]]
    moves = []
    for i in range(len(targets)-1):
        seg = bfs_path(N, vwall, hwall, targets[i], targets[i+1])
        mv = cells_to_moves(seg)
        pathCells.extend(seg[1:])
        moves.extend(mv)
    L = len(moves)
    # T >= L のはず

    best = None

    # C+Q が小さい順に試す
    for s in range(2, C_max + Q_max + 1):
        for C in range(1, C_max+1):
            Q = s - C
            if Q < 1 or Q > Q_max:
                continue
            # L が大きいと爆死するので注意
            cnf, var_xc, var_xq, var_dir, var_nxt = build_cnf_for_CQ(
                N, pathCells, moves, C, Q
            )
            solver = Glucose3()
            solver.append_formula(cnf)
            sat = solver.solve()
            if not sat:
                solver.delete()
                continue
            model = solver.get_model()
            solver.delete()

            # モデルから ct[t], qt[t], dir, nxt を復元
            def is_true(v):
                return v in model

            # 色と状態の系列
            ct = [0]*(L+1)
            qt = [0]*(L+1)
            for t in range(L+1):
                for c in range(C):
                    if is_true(var_xc(t,c)):
                        ct[t] = c
                        break
                for q in range(Q):
                    if is_true(var_xq(t,q)):
                        qt[t] = q
                        break

            # マスごとの色 (初期盤面)
            s_board = [[0]*N for _ in range(N)]
            for t,(x,y) in enumerate(pathCells):
                s_board[x][y] = ct[t]
            # 通ってないマスは 0 のまま

            # dir, nxt を復元
            D = [[0]*Q for _ in range(C)]  # dir index 0..3
            S = [[0]*Q for _ in range(C)]
            for c in range(C):
                for q in range(Q):
                    # dir
                    for d in range(4):
                        if is_true(var_dir(c,q,d)):
                            D[c][q] = d
                            break
                    # nxt
                    for q2 in range(Q):
                        if is_true(var_nxt(c,q,q2)):
                            S[c][q] = q2
                            break

            # 実際にシミュレーションしてみて本当に動作するか確認（念のため）
            if not simulate(N, vwall, hwall, s_board, C, Q, D, S, targets, T):
                # 万一おかしくてもスキップ
                continue

            # OK だったのでこれを採用
            best = (C, Q, s_board, D, S)
            return best  # 最初に見つかったやつを返す

    return None

def simulate(N, vwall, hwall, s_board, C, Q, D, S, targets, T):
    """
    実際にロボットを動かして、全ての targets を順番に踏めるか確認。
    D[c][q] は 0..3 で UDLR を表す。
    S[c][q] は次状態。
    """
    # 盤面色のコピー
    color = [row[:] for row in s_board]
    x, y = targets[0]
    q = 0
    K = len(targets)
    k = 0
    if x == targets[0][0] and y == targets[0][1]:
        k = 1

    def can_move(x, y, d):
        if d == 0:  # U
          return x > 0 and hwall[x-1][y] == '0'
        if d == 1:  # D
          return x+1 < N and hwall[x][y] == '0'
        if d == 2:  # L
          return y > 0 and vwall[x][y-1] == '0'
        if d == 3:  # R
          return y+1 < N and vwall[x][y] == '0'
        return False

    for step in range(T):
        c = color[x][y]
        if not (0 <= c < C and 0 <= q < Q):
            return False
        d = D[c][q]
        q_next = S[c][q]
        # 色は変えない（A(c,q) = c）
        # 移動
        nx, ny = x, y
        if d == 0 and can_move(x,y,0):
            nx, ny = x-1, y
        elif d == 1 and can_move(x,y,1):
            nx, ny = x+1, y
        elif d == 2 and can_move(x,y,2):
            nx, ny = x, y-1
        elif d == 3 and can_move(x,y,3):
            nx, ny = x, y+1
        # 壁ならその場に留まる（path は壁考慮済みなので本来起きないはず）
        x, y = nx, ny
        q = q_next

        # 目的地チェック
        if k < K and x == targets[k][0] and y == targets[k][1]:
            k += 1
            if k == K:
                return True
    return (k == K)

def main():
    import sys
    sys.setrecursionlimit(1_000_000)
    input = sys.stdin.readline

    N, K, T = map(int, input().split())
    vwall = [input().strip() for _ in range(N)]
    hwall = [input().strip() for _ in range(N-1)]
    targets = [tuple(map(int, input().split())) for _ in range(K)]

    # 小さいケース用に C_max, Q_max を決める
    # 例えば 3,3 くらいにしておくと L がそこそこでもギリ動くかもしれない
    C_max = 6
    Q_max = 6

    res = solve_with_sat(N, vwall, hwall, targets, T, C_max, Q_max)

    if res is None:
        # SAT で見つからなかった場合は、単純に time-index 方式とかにフォールバックしてもいい
        print("SAT-based solution not found for given C_max,Q_max", file=sys.stderr)
        # ここにフォールバックを書く or 終了
        # ひとまずダミー出力 (C=1,Q=1,M=0) にしておく
        print(1,1,0)
        for i in range(N):
            print(" ".join(["0"]*N))
        return

    C, Q, s_board, D, S = res

    # 遷移規則を出力 (使われる可能性のある (c,q) だけ出してもいいが、
    # 簡単のため全部出しても M <= C*Q <= ? で T を越えやすいので注意。
    # ここでは「安全に M <= T」にするため、実際に起こりうる (c,q) のみを抽出するのが良い。
    # シミュレーションで reachable な (c,q) を集める。
    used = set()
    color = [row[:] for row in s_board]
    x, y = targets[0]
    q = 0
    K = len(targets)
    k = 0
    if x == targets[0][0] and y == targets[0][1]:
        k = 1

    def can_move(x, y, d):
        if d == 0:  # U
          return x > 0 and hwall[x-1][y] == '0'
        if d == 1:  # D
          return x+1 < N and hwall[x][y] == '0'
        if d == 2:  # L
          return y > 0 and vwall[x][y-1] == '0'
        if d == 3:  # R
          return y+1 < N and vwall[x][y] == '0'
        return False

    for step in range(T):
        c = color[x][y]
        used.add((c,q))
        d = D[c][q]
        q_next = S[c][q]
        nx, ny = x, y
        if d == 0 and can_move(x,y,0):
            nx, ny = x-1, y
        elif d == 1 and can_move(x,y,1):
            nx, ny = x+1, y
        elif d == 2 and can_move(x,y,2):
            nx, ny = x, y-1
        elif d == 3 and can_move(x,y,3):
            nx, ny = x, y+1
        x, y = nx, ny
        q = q_next
        if k < K and x == targets[k][0] and y == targets[k][1]:
            k += 1
            if k == K:
                break

    rules = []
    for c in range(C):
        for q in range(Q):
            if (c,q) not in used:
                continue
            d_idx = D[c][q]
            d_char = DIRS[d_idx]
            A = c  # 色は変えない
            Snext = S[c][q]
            rules.append((c, q, A, Snext, d_char))

    M = len(rules)

    # AtCoder 形式で出力
    print(C, Q, M)
    for i in range(N):
        print(" ".join(str(x) for x in s_board[i]))
    for c, q, A, Snext, d_char in rules:
        print(c, q, A, Snext, d_char)

if __name__ == "__main__":
    main()
