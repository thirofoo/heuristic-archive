#include <bits/stdc++.h>
using namespace std;

/* =========================================================
   AHC053 - Random Sum Game
   変更点:
     - 2/3 グループ再分配近傍を削除
     - 新近傍: ある1グループ p と「未使用カード(捨て)」の集合を取り出し、
       カード番号で降順ソート→上位 DP_LIMIT 枚について
       ビーム式ナップサック DPで B[p] に最も近くなるよう p に再配分
       （選ばれなかった分は未使用にする）
   それ以外（I/O, 初期貪欲, SA, 単一移動/スワップ、カード生成）は変更なし
   ========================================================= */

// =================== RNG (xorshift) ===================
static inline uint32_t rng_u32() {
  static uint32_t x=123456789u,y=362436069u,z=521288629u,w=88675123u;
  uint32_t t = x ^ (x << 11);
  x = y; y = z; z = w;
  return w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
}
static inline int rng_int(int l, int r){ // inclusive
  return l + (int)(rng_u32() % (uint32_t)(r - l + 1));
}
static inline double rng_double() {
  return (double)(rng_u32() & 0xFFFFFF) / (double)(0x1000000);
}

// =================== params ===================
// これらは残置（他ロジックは不変更のため）
#ifndef COLS_DEFAULT
#define COLS_DEFAULT 20
#endif
#ifndef BITS_PER_CARD_DEFAULT
#define BITS_PER_CARD_DEFAULT 6
#endif
#ifndef PER_LEVEL_QTY
#define PER_LEVEL_QTY 50
#endif

static inline long long clamp_ll(long long v, long long lo, long long hi){
  return v<lo ? lo : (v>hi ? hi : v);
}
static inline unsigned long long pow2_ull_safe(int e){
  if (e <= 0) return 1ULL;
  if (e >= 63) return (1ULL<<62); // guard
  return (1ULL<<e);
}

// =================== main ===================
int main(){
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  int N, M; long long L, U;
  if(!(cin >> N >> M >> L >> U)) return 0;

  const int R = max(0, N - M); // 等比数列で作る枚数

  // ランタイムパラメータは残置（未使用でも触らない）
  int COLS = COLS_DEFAULT;
  if (const char* s=getenv("COLS")) { int v=atoi(s); if (1<=v && v<=60) COLS=v; }
  int BPC = BITS_PER_CARD_DEFAULT;
  if (const char* s=getenv("BITS_PER_CARD")) { int v=atoi(s); if (1<=v && v<=60) BPC=v; }
  if (BPC > COLS) BPC = COLS;
  int min_per_level = PER_LEVEL_QTY;
  if (const char* s=getenv("PER_LEVEL")) { int v=atoi(s); if (v>=0) min_per_level=v; }

  // V_INIT は 2e12 固定（環境変数指定があれば上書き可能のまま）
  long long V_INIT = 3700000000000LL; // 2e12
  if (const char* s=getenv("V_INIT")) { long long v = atoll(s); if (v > 0) V_INIT = v; }

  // ベースライン（アンカー）: 既定は L（= 1e15 - 2e12 想定）
  long long BASELINE = L;
  if (const char* s=getenv("BASELINE")) { long long v = atoll(s); if (v > 0) BASELINE = v; }
  BASELINE = clamp_ll(BASELINE, 1LL, U);

  // ===== ここからカード配列 A の生成（等比数列）=====
  vector<long long> A; A.reserve(N);

  // 先頭 M 枚はアンカー
  for (int j=0;j<M && (int)A.size()<N; ++j) A.push_back(BASELINE);

  // 残り R 枚は V_INIT * 0.93^k を丸め・クリップして追加
  long double cur = (long double)V_INIT;
  const long double ratio = 0.93L;
  for (int k=0; k<R && (int)A.size()<N; ++k){
    long long val = (long long) llround(cur);
    if (val < 1) val = 1;
    if (val > U) val = U;
    A.push_back(val);
    cur *= ratio + (3 * R - k) * 0.00002L; // 少しゆらぎを入れる
  }

  // （到達しない想定だが、以前のコード互換のため残置）
  long long base_unit = 1; // ダミー
  while ((int)A.size()<N) A.push_back( max(1LL, min(U, base_unit)) );

  // ===== A を出力（1 行スペース区切り）=====
  for (int i=0;i<N;++i){ if (i) cout << ' '; cout << A[i]; }
  cout << '\n' << flush;

  // ===== B を読み込み =====
  vector<long long> B(M,0);
  for (int j=0;j<M;++j){ if (!(cin >> B[j])) B[j]=0; }

  // ===== 以降は一切変更なし：貪欲 + SA（ただし新近傍を追加） =====
  vector<int> X(N,0);
  vector<long long> S(M,0);

  // アンカーを各山に固定
  for (int j=0;j<M && j<N; ++j){
    X[j] = j+1;
    S[j] += A[j];
  }

  // 初期貪欲
  for (int i=M;i<N;++i){
    long long bestDelta = 0; int bestJ = -1;
    long long ai = A[i];
    for (int j=0;j<M;++j){
      long long before = llabs(S[j] - B[j]);
      long long after  = llabs(S[j] + ai - B[j]);
      long long d = after - before; // <0 が改善
      if (d < bestDelta){ bestDelta = d; bestJ = j; }
    }
    if (bestJ >= 0){ X[i] = bestJ+1; S[bestJ] += A[i]; }
  }

  auto totalE = [&]()->long long{
    long long E=0; for (int j=0;j<M;++j) E += llabs(S[j]-B[j]); return E;
  };
  long long E = totalE();
  long long bestE = E;
  vector<int> bestX = X;
  vector<long long> bestS = S;

  // 単移動 / スワップ
  auto d_move = [&](int from, int to, long long a)->long long{
    long long d=0;
    if (from>=0) {
      long long bf = llabs(S[from]-B[from]);
      long long af = llabs(S[from]-a - B[from]);
      d += (af-bf);
    }
    if (to>=0) {
      long long bt = llabs(S[to]-B[to]);
      long long at = llabs(S[to]+a - B[to]);
      d += (at-bt);
    }
    return d;
  };
  auto d_swap = [&](int i, int j)->long long{
    int ai = X[i]-1, aj = X[j]-1;
    long long a=A[i], b=A[j];
    if (ai==aj) return 0;
    long long d=0;
    if (ai>=0){
      long long bf = llabs(S[ai]-B[ai]);
      long long af = llabs(S[ai]-a + b - B[ai]);
      d += (af-bf);
    }
    if (aj>=0){
      long long bf = llabs(S[aj]-B[aj]);
      long long af = llabs(S[aj]-b + a - B[aj]);
      d += (af-bf);
    }
    return d;
  };

  // ---- 新近傍: 1グループ p と「未使用(捨て)」の間で ナップサック風DP（ビーム） ----
  int DP_LIMIT = 60;        // 使う候補の最大枚数（カード番号 降順の上位）
  int BEAM_SIZE = 100;      // ビーム幅（保持状態数）
  if (const char* s=getenv("DP_LIMIT"))  { int v=atoi(s); if (1<=v && v<=60) DP_LIMIT=v; }
  if (const char* s=getenv("BEAM_SIZE")) { int v=atoi(s); if (v>=256 && v<=8192) BEAM_SIZE=v; }

  auto try_repack_group_with_unused_DP = [&](int p, long double T)->bool{
    int P = p-1;

    // 候補 = グループ p のカード + 未使用カード（いずれも i>=M）
    vector<int> cand; cand.reserve(256);
    for (int i=M;i<N;++i){
      if (X[i]==p || X[i]==0) cand.push_back(i);
    }
    if ((int)cand.size() <= 1) return false;

    // カード番号で降順ソート
    sort(cand.begin(), cand.end(), [&](int i, int j){ return i>j; });

    // 上位 DP_LIMIT 枚のみ使用（64bit マスク復元のため 60 以内推奨）
    if ((int)cand.size() > DP_LIMIT) cand.resize(DP_LIMIT);
    int K = (int)cand.size();
    if (K==0) return false;

    // 現在 p に固定されない分（= cand に属する p の分を除いた p の固定値）
    long long movable_sumP = 0;
    for (int i : cand) if (X[i]==p) movable_sumP += A[i];
    long long fixedP = S[P] - movable_sumP;

    // 目的値：newSp = fixedP + x ≈ B[P] となる x を cand の部分和から選ぶ
    long long targetX = B[P] - fixedP;

    struct Node{ long long sum; uint64_t mask; };
    vector<Node> cur, nxt;
    cur.reserve(BEAM_SIZE); nxt.reserve(BEAM_SIZE*2);
    cur.push_back({0, 0ull});

    // 各アイテム（カード番号降順）でビーム展開＆剪定
    for (int t=0; t<K; ++t){
      long long w = A[cand[t]];
      nxt.clear();
      nxt.reserve(min(BEAM_SIZE*2, 8192));

      // 既存状態をそのまま / 追加で遷移
      for (const auto& st : cur){
        nxt.push_back(st);
        // 追加（mask の t ビットを立てる）
        uint64_t m = st.mask | (1ull<<t);
        nxt.push_back({ st.sum + w, m });
      }

      // 目的値に近い順にソートして BEAM_SIZE に剪定
      nth_element(nxt.begin(),
                  nxt.begin() + min((int)nxt.size(), BEAM_SIZE),
                  nxt.end(),
                  [&](const Node& a, const Node& b){
                    long long da = llabs(a.sum - targetX);
                    long long db = llabs(b.sum - targetX);
                    if (da != db) return da < db;
                    // タイブレーク：和が小さい方を優先
                    return a.sum < b.sum;
                  });
      int keep = min((int)nxt.size(), BEAM_SIZE);
      nxt.resize(keep);

      // ソートしておくと次の nth_element の局所性が少し良くなる
      sort(nxt.begin(), nxt.end(), [&](const Node& a, const Node& b){
        long long da = llabs(a.sum - targetX);
        long long db = llabs(b.sum - targetX);
        if (da != db) return da < db;
        return a.sum < b.sum;
      });

      cur.swap(nxt);
    }

    // 最良候補（先頭）を採用
    const Node best = cur.front();
    long long newSp = fixedP + best.sum;

    long long before = llabs(S[P]-B[P]);
    long long after  = llabs(newSp-B[P]);
    long long dE = after - before;

    if (dE <= 0 || rng_double() < expl(-(long double)dE / T)) {
      // 適用：mask のビットが 1 のカードは p に、0 のカードは未使用に
      for (int t=0; t<K; ++t){
        int i = cand[t];
        bool toP = ((best.mask>>t) & 1ull);
        if (toP){
          if (X[i]==0){ S[P] += A[i]; X[i]=p; }
          // 既に p なら何もしない
        }else{
          if (X[i]==p){ S[P] -= A[i]; X[i]=0; }
          // 既に未使用なら何もしない
        }
      }
      E += dE;
      if (E < bestE){ bestE=E; bestX=X; bestS=S; }
      return true;
    }
    return false;
  };

  // SA parameters
  const double SA_MS = 1950.0;
  auto t0 = chrono::high_resolution_clock::now();
  auto elapsed_ms = [&](){
    auto now = chrono::high_resolution_clock::now();
    return (double)chrono::duration_cast<chrono::milliseconds>(now - t0).count();
  };
  long long medA = A[min(N-1, max(0,N/2))];
  long double T0 = 1e5L;
  long double T1 = 1e0L;

  auto pick_deficit_bucket = [&](int k)->int{
    int best=-1; long long bd = LLONG_MIN;
    for (int t=0;t<k;++t){
      int j = rng_int(0, M-1);
      long long d = B[j]-S[j];
      if (d>bd){ bd=d; best=j; }
    }
    if (best<0) best = rng_int(0, M-1);
    return best;
  };
  auto pick_worst_abs = [&](int k)->int{
    int best=-1; long long bv = LLONG_MIN;
    for (int t=0;t<k;++t){
      int j = rng_int(0, M-1);
      long long v = llabs(S[j]-B[j]);
      if (v>bv){ bv=v; best=j; }
    }
    if (best<0) best = rng_int(0, M-1);
    return best;
  };

  vector<int> idxs; idxs.reserve(max(0,N-M));
  for (int i=M;i<N;++i) idxs.push_back(i);

  // SA main loop
  while (elapsed_ms() < SA_MS) {
    long double p = elapsed_ms()/SA_MS; if (p>1) p=1;
    long double Temp = expl( logl(T0)*(1-p) + logl(T1)*p );

    int op = rng_int(0, 99);
    if (op < 65) {
      // move / add / remove a single card
      int i = (idxs.empty()? rng_int(0,N-1) : idxs[rng_u32() % (uint32_t)idxs.size()]);
      int cur = X[i];
      int nxt;
      if (rng_u32()%3==0) { nxt = 0; }
      else if (rng_u32()%3){ nxt = pick_deficit_bucket(4)+1; }
      else { nxt = rng_int(0, M); }
      if (nxt == cur) continue;
      int from = (cur==0? -1 : cur-1);
      int to   = (nxt==0? -1 : nxt-1);
      long long dE = d_move(from, to, A[i]);
      if (dE <= 0 || rng_double() < expl(-(long double)dE / Temp)) {
        if (from>=0) S[from]-=A[i];
        if (to>=0) S[to]+=A[i];
        X[i]=nxt; E+=dE;
        if (E < bestE){ bestE=E; bestX=X; bestS=S; }
      }
    } else if (op < 95) {
      // 新近傍：誤差が大きい山 p を選び、p+未使用 で DP 再配分
      int pj = pick_worst_abs(6) + 1; // 1..M
      (void)try_repack_group_with_unused_DP(pj, Temp);
    } else {
      // swap two assigned cards between piles
      if (N-M >= 2) {
        int i = idxs[rng_u32() % (uint32_t)idxs.size()];
        int j = idxs[rng_u32() % (uint32_t)idxs.size()];
        if (i==j) continue;
        if (X[i]==0 && X[j]==0) continue; // nothing to swap
        long long dE = d_swap(i,j);
        if (dE <= 0 || rng_double() < expl(-(long double)dE / Temp)) {
          int ai = X[i]-1, aj = X[j]-1;
          long long a=A[i], b=A[j];
          if (ai>=0) S[ai] = S[ai]-a+b;
          if (aj>=0) S[aj] = S[aj]-b+a;
          swap(X[i], X[j]);
          E += dE;
          if (E < bestE){ bestE=E; bestX=X; bestS=S; }
        }
      }
    }
  }

  // 最良解へ戻す & 出力
  X = move(bestX);
  for (int i=0;i<N;++i){ if (i) cout << ' '; cout << X[i]; }
  cout << '\n' << flush;

  return 0;
}
