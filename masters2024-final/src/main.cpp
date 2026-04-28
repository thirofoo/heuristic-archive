#pragma GCC optimize("Ofast")
#pragma GCC optimize("unroll-loops")
#pragma GCC optimize("inline")
#include<bits/stdc++.h>
#include<chrono>
using namespace std;
#define PI 3.14159265358979323846
void*wmem;
char memarr[96000000];
template<class S, class T> inline S chmin(S &a, T b){
  if(a>b){
    a=b;
  }
  return a;
}
template<class S, class T> inline S chmax(S &a, T b){
  if(a<b){
    a=b;
  }
  return a;
}
template<class T1, class T2, class T3> inline T1 Kth1_L(const T1 a, const T2 b, const T3 c){
  if(a <= b){
    if(b <= c){
      return b;
    }
    if(c <= a){
      return a;
    }
    return c;
  }
  if(a <= c){
    return a;
  }
  if(c <= b){
    return b;
  }
  return c;
}
struct Timer2{
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  void set(void){
    start_time = std::chrono::high_resolution_clock::now();
  }
  double get(void){
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    return duration.count() * 1e-6;
  }
}
;
struct Rand{
  unsigned x;
  unsigned y;
  unsigned z;
  unsigned w;
  Rand(void){
    x=123456789;
    y=362436069;
    z=521288629;
    w=(unsigned)time(NULL);
  }
  Rand(unsigned seed){
    x=123456789;
    y=362436069;
    z=521288629;
    w=seed;
  }
  inline unsigned get(void){
    unsigned t;
    t = (x^(x<<11));
    x=y;
    y=z;
    z=w;
    w = (w^(w>>19))^(t^(t>>8));
    return w;
  }
  inline double getUni(void){
    return get()/4294967296.0;
  }
  inline int get(int a){
    return (int)(a*getUni());
  }
  inline int get(int a, int b){
    return a+(int)((b-a+1)*getUni());
  }
  inline long long get(long long a){
    return(long long)(a*getUni());
  }
  inline long long get(long long a, long long b){
    return a+(long long)((b-a+1)*getUni());
  }
  inline double get(double a, double b){
    return a+(b-a)*getUni();
  }
  inline int getExp(int a){
    return(int)(exp(getUni()*log(a+1.0))-1.0);
  }
  inline int getExp(int a, int b){
    return a+(int)(exp(getUni()*log((b-a+1)+1.0))-1.0);
  }
}
;
inline int my_getchar(){
  return getchar();
}
inline void rd(int &x){
  int k;
  int m=0;
  x=0;
  for(;;){
    k = my_getchar();
    if(k=='-'){
      m=1;
      break;
    }
    if('0'<=k&&k<='9'){
      x=k-'0';
      break;
    }
  }
  for(;;){
    k = my_getchar();
    if(k<'0'||k>'9'){
      break;
    }
    x=x*10+k-'0';
  }
  if(m){
    x=-x;
  }
}
inline void rd(unsigned &x){
  int k;
  x=0;
  for(;;){
    k = my_getchar();
    if('0'<=k&&k<='9'){
      x=k-'0';
      break;
    }
  }
  for(;;){
    k = my_getchar();
    if(k<'0'||k>'9'){
      break;
    }
    x=x*10+k-'0';
  }
}
inline void rd(long long &x){
  int k;
  int m=0;
  x=0;
  for(;;){
    k = my_getchar();
    if(k=='-'){
      m=1;
      break;
    }
    if('0'<=k&&k<='9'){
      x=k-'0';
      break;
    }
  }
  for(;;){
    k = my_getchar();
    if(k<'0'||k>'9'){
      break;
    }
    x=x*10+k-'0';
  }
  if(m){
    x=-x;
  }
}
inline void rd(unsigned long long &x){
  int k;
  x=0;
  for(;;){
    k = my_getchar();
    if('0'<=k&&k<='9'){
      x=k-'0';
      break;
    }
  }
  for(;;){
    k = my_getchar();
    if(k<'0'||k>'9'){
      break;
    }
    x=x*10+k-'0';
  }
}
inline void rd(double &x){
  int k;
  int m=0;
  int p=0;
  double r = 1;
  x = 0;
  for(;;){
    k = my_getchar();
    if(k=='-'){
      m = 1;
      break;
    }
    if(k=='.'){
      p = 1;
      break;
    }
    if('0'<=k&&k<='9'){
      x = k - '0';
      break;
    }
  }
  for(;;){
    k = my_getchar();
    if(k=='.'){
      p = 1;
      continue;
    }
    if(k<'0'||k>'9'){
      break;
    }
    if(p){
      r *= 0.1;
      x += r * (k - '0');
    }
    else{
      x = x * 10 + k - '0';
    }
  }
  if(m){
    x = -x;
  }
}
inline void rd(char &c){
  int i;
  for(;;){
    i = my_getchar();
    if(i!=' '&&i!='\n'&&i!='\r'&&i!='\t'&&i!=EOF){
      break;
    }
  }
  c = i;
}
inline int rd(char c[]){
  int i;
  int sz = 0;
  for(;;){
    i = my_getchar();
    if(i!=' '&&i!='\n'&&i!='\r'&&i!='\t'&&i!=EOF){
      break;
    }
  }
  c[sz++] = i;
  for(;;){
    i = my_getchar();
    if(i==' '||i=='\n'||i=='\r'||i=='\t'||i==EOF){
      break;
    }
    c[sz++] = i;
  }
  c[sz]='\0';
  return sz;
}
inline void rd(string &x){
  char*buf = (char *)wmem;
  rd(buf);
  x = buf;
}
inline void my_putchar(const int k){
  putchar(k);
  if(k=='\n'){
    fflush(stdout);
  }
}
template<class T> inline void wt_L(const vector<T> &x);
template<class T> inline void wt_L(const set<T> &x);
template<class T> inline void wt_L(const multiset<T> &x);
template<class T1, class T2> inline void wt_L(const pair<T1,T2> x);
inline void wt_L(const char a){
  my_putchar(a);
}
inline void wt_L(int x){
  int s=0;
  int m=0;
  char f[10];
  if(x<0){
    m=1;
    x=-x;
  }
  while(x){
    f[s++]=x%10;
    x/=10;
  }
  if(!s){
    f[s++]=0;
  }
  if(m){
    my_putchar('-');
  }
  while(s--){
    my_putchar(f[s]+'0');
  }
}
inline void wt_L(const char c[]){
  int i=0;
  for(i=0;c[i]!='\0';i++){
    my_putchar(c[i]);
  }
}
template<class T> inline T pow2_L(T a){
  return a*a;
}
template<class T> struct Point2dEPS{
  static T eps;
  void set(T a){
    eps = a;
  }
}
;
template<class T> T Point2dEPS<T>::eps;
void Point2d_init(void){
  Point2dEPS<float> x;
  x.set(1e-4);
  Point2dEPS<double> y;
  y.set(1e-10);
  Point2dEPS<long double> z;
  z.set(1e-10);
}
template<class T, class S, class F> struct Point2d{
  T x;
  T y;
  Point2d(){
    x = y = 0;
  }
  Point2d(T a){
    x = a;
    y = 0;
  }
  Point2d(T a, T b){
    x = a;
    y = b;
  }
  void set(T a, T b){
    x = a;
    y = b;
  }
  Point2d<T,S,F> &operator+=(Point2d<T,S,F> a){
    x += a.x;
    y += a.y;
    return *this;
  }
  Point2d<T,S,F> &operator-=(Point2d<T,S,F> a){
    x -= a.x;
    y -= a.y;
    return *this;
  }
  Point2d<T,S,F> operator+(Point2d<T,S,F> a){
    return Point2d<T,S,F>(*this)+=a;
  }
  Point2d<T,S,F> operator-(Point2d<T,S,F> a){
    return Point2d<T,S,F>(*this)-=a;
  }
  inline F dist(void){
    F tx;
    F ty;
    tx = x;
    ty = y;
    return sqrt(tx*tx + ty*ty);
  }
  inline F dist(Point2d<T,S,F> a){
    F tx;
    F ty;
    tx = ((F)x) - ((F)a.x);
    ty = ((F)y) - ((F)a.y);
    return sqrt(tx*tx + ty*ty);
  }
  inline S dist2(void){
    S tx;
    S ty;
    tx = x;
    ty = y;
    return tx*tx + ty*ty;
  }
  inline S dist2(Point2d<T,S,F> a){
    S tx;
    S ty;
    tx = ((S)x) - ((S)a.x);
    ty = ((S)y) - ((S)a.y);
    return tx*tx + ty*ty;
  }
  inline F arg(void){
    F res;
    if(x==0 && y==0){
      return 0;
    }
    res = atan2(y, x);
    if(res <= -PI + Point2dEPS<F>::eps){
      res += 2*PI;
    }
    return res;
  }
  inline F arg(Point2d<T,S,F> a){
    F res;
    res = arg() - a.arg();
    if(res <= -PI + Point2dEPS<F>::eps){
      res += 2*PI;
    }
    if(res > PI + Point2dEPS<F>::eps){
      res -= 2*PI;
    }
    return res;
  }
}
;
template<class T, class S, class F> inline Point2d<T,S,F> operator*(T a, Point2d<T,S,F> b){
  return Point2d<T,S,F>(a*b.x, a*b.y);
}
template<class T, class S, class F> void rd(Point2d<T,S,F> &a){
  rd(a.x);
  rd(a.y);
}
template<class T, class S, class F> S CrossProd(Point2d<T,S,F> a, Point2d<T,S,F> b){
  return ((S)a.x) * ((S)b.y) - ((S)b.x) * ((S)a.y);
}
template<class T, class S, class F> S CrossProd(Point2d<T,S,F> c, Point2d<T,S,F> a, Point2d<T,S,F> b){
  S x1;
  S x2;
  S y1;
  S y2;
  x1 = ((S)a.x) - ((S)c.x);
  y1 = ((S)a.y) - ((S)c.y);
  x2 = ((S)b.x) - ((S)c.x);
  y2 = ((S)b.y) - ((S)c.y);
  return x1 * y2 - x2 * y1;
}
const int mode = 0;
const int do_loop = 5000;
const double TL = 1.95;
Timer2 timer;
Rand rnd;
int N;
int M;
double EPS;
double DELTA;
Point2d<int,long long,double> S;
Point2d<int,long long,double> P[10];
Point2d<int,long long,double> L[10];
Point2d<int,long long,double> R[10];
int node;
Point2d<int,long long,double> nodep[50];
double dir_dist[50][50];
double route_dist[50][50];
int opt_route[12];
double opt_route_dist;
double tmp_route_dist;
double judge_alpha[5000];
int judge_ax[5000];
int judge_ay[5000];
int do_time;
int QA[6000];
int AX[6000];
int AY[6000];
int C[6000];
int H[6000];
int Q[6000][20];
int D[6000];
int vis[10];
template<class T, class S, class F> S InnerProdK(Point2d<T,S,F> a, Point2d<T,S,F> b){
  return ((S)a.x) * ((S)b.x) + ((S)a.y) * ((S)b.y);
}
double Dist_PointSeg(Point2d<int,long long,double> P, Point2d<int,long long,double> L, Point2d<int,long long,double> R){
  if(InnerProdK(P-L, R-L) < 0){
    return (P-L).dist();
  }
  if(InnerProdK(P-R, L-R) < 0){
    return (P-R).dist();
  }
  return abs( CrossProd(P,L,R)/1.0 ) / (L-R).dist();
}
int Cross_Seg(Point2d<int,long long,double> L1, Point2d<int,long long,double> R1, Point2d<int,long long,double> L2, Point2d<int,long long,double> R2){
  long long a;
  long long b;
  a = CrossProd(L1, R1, L2);
  b = CrossProd(L1, R1, R2);
  if(a < 0 && b < 0){
    return 0;
  }
  if(a > 0 && b > 0){
    return 0;
  }
  a = CrossProd(L2, R2, L1);
  b = CrossProd(L2, R2, R1);
  if(a < 0 && b < 0){
    return 0;
  }
  if(a > 0 && b > 0){
    return 0;
  }
  return 1;
}
double calc_dist(Point2d<int,long long,double> a, double dx, double dy){
  int ok;
  int k;
  double res;
  double od;
  Point2d<int,long long,double> b;
  if(M==0 && dx==0){
    if(dy < 0){
      return a.y - (-100000);
    }
    if(dy > 0){
      return 100000 - a.y;
    }
  }
  if(M==0 && dy==0){
    if(dx < 0){
      return a.x - (-100000);
    }
    if(dx > 0){
      return 100000 - a.x;
    }
  }
  od = sqrt(dx*dx+dy*dy);
  double Nzj9Y0kE;
  double bkxOPzPX;
  double WKqLrJHZ;
  Nzj9Y0kE = 0;
  bkxOPzPX = 2e5;
  for(;;){
    WKqLrJHZ = (Nzj9Y0kE + bkxOPzPX) / 2;
    if(WKqLrJHZ == Nzj9Y0kE || WKqLrJHZ == bkxOPzPX){
      break;
    }
    ok = 0;
    b.x = a.x + dx / od * WKqLrJHZ;
    b.y = a.y + dy / od * WKqLrJHZ;
    if(abs(b.x) >= 100000 || abs(b.y) >= 100000){
      ok = 1;
    }
    if(!ok){
      for(k=(0);k<(M);k++){
        if(Cross_Seg(a,b,L[k],R[k])){
          ok = 1;
          break;
        }
      }
    }
    if(ok){
      bkxOPzPX = WKqLrJHZ;
    }
    else{
      Nzj9Y0kE = WKqLrJHZ;
    }
  }
  res =((Nzj9Y0kE + bkxOPzPX) / 2);
  return res;
}
Point2d<int,long long,double> tp;
Point2d<int,long long,double> tv;
void do_query(int t){
  int i;
  int j;
  int k;
  int c = 0;
  if(QA[t] == 0){
    wt_L("A");
    wt_L(' ');
    wt_L(AX[t]);
    wt_L(' ');
    wt_L(AY[t]);
    wt_L('\n');
  }
  else{
    wt_L("S");
    wt_L(' ');
    wt_L(AX[t]);
    wt_L(' ');
    wt_L(AY[t]);
    wt_L('\n');
  }
  if(mode == 1){
    Point2d<int,long long,double> p;
    if(QA[t]==0){
      tv.x += AX[t] + judge_ax[t];
      tv.y += AY[t] + judge_ay[t];
    }
    else{
      D[t] = round(calc_dist(tp, AX[t], AY[t]) * judge_alpha[t]);
      tv.x += judge_ax[t];
      tv.y += judge_ay[t];
    }
    p = tp;
    tp += tv;
    for(k=(0);k<(M);k++){
      if(Cross_Seg(p,tp,L[k],R[k])){
        c = 1;
        break;
      }
    }
    if(tp.x <= -100000 || tp.y <= -100000){
      c = 1;
    }
    if(tp.x >=  100000 || tp.y >=  100000){
      c = 1;
    }
    if(c){
      tp = p;
      tv.x = tv.y = 0;
      C[t] = 1;
      H[t] = 0;
    }
    else{
      for(k=(0);k<(N);k++){
        if(vis[k]==0){
          if(Dist_PointSeg(P[k],p,tp) <= 1000+1e-12){
            Q[t][H[t]++] = k;
          }
        }
      }
    }
  }
  else{
    if(QA[t]==1){
      rd(D[t]);
    }
    rd(C[t]);
    rd(H[t]);
    {
      int wbH8AaCb;
      for(wbH8AaCb=(0);wbH8AaCb<(H[t]);wbH8AaCb++){
        rd(Q[t][wbH8AaCb]);
      }
    }
  }
}
int main(){
  int t;
  wmem = memarr;
  {
    Point2d_init();
  }
  int i;
  int j;
  int k;
  int cur_score = 0;
  int score = 0;
  timer.set();
  rd(N);
  rd(M);
  rd(EPS);
  rd(DELTA);
  rd(S);
  {
    int Dc_3QP3Y;
    for(Dc_3QP3Y=(0);Dc_3QP3Y<(N);Dc_3QP3Y++){
      rd(P[Dc_3QP3Y]);
    }
  }
  {
    int QDGVCdl1;
    for(QDGVCdl1=(0);QDGVCdl1<(M);QDGVCdl1++){
      rd(L[QDGVCdl1]);
      rd(R[QDGVCdl1]);
    }
  }
  if(mode){
    {
      int BRSo37Cn;
      for(BRSo37Cn=(0);BRSo37Cn<(5000);BRSo37Cn++){
        rd(judge_alpha[BRSo37Cn]);
      }
    }
    {
      int ZcmRJqJU;
      for(ZcmRJqJU=(0);ZcmRJqJU<(5000);ZcmRJqJU++){
        rd(judge_ax[ZcmRJqJU]);
        rd(judge_ay[ZcmRJqJU]);
      }
    }
    tp = S;
    tv.x = tv.y = 0;
  }
  node = 0;
  nodep[node++] = S;
  for(i=(0);i<(N);i++){
    nodep[node++] = P[i];
  }
  for(i=(0);i<(M);i++){
    int xx;
    int yy;
    double dx;
    double dy;
    double d;
    dx = L[i].x - R[i].x;
    dy = L[i].y - R[i].y;
    d = (L[i] - R[i]).dist();
    xx = L[i].x + dx / d * 2000;
    yy = L[i].y + dy / d * 2000;
    if(-100000 <= xx  &&  xx <= 100000 && -100000 <= yy  &&  yy <= 100000){
      nodep[node].x = xx;
      nodep[node].y = yy;
      node++;
    }
    xx = R[i].x - dx / d * 2000;
    yy = R[i].y - dy / d * 2000;
    if(-100000 <= xx  &&  xx <= 100000 && -100000 <= yy  &&  yy <= 100000){
      nodep[node].x = xx;
      nodep[node].y = yy;
      node++;
    }
  }
  for(i=(0);i<(node);i++){
    dir_dist[i][i] = 0;
  }
  for(i=(0);i<(node);i++){
    for(j=(i+1);j<(node);j++){
      dir_dist[i][j] = (nodep[i]-nodep[j]).dist();
      for(k=(0);k<(M);k++){
        if(Cross_Seg(nodep[i],nodep[j],L[k],R[k])){
          dir_dist[i][j] = 1e150;
        }
      }
      dir_dist[j][i] = dir_dist[i][j];
    }
  }
  for(i=(0);i<(node);i++){
    for(j=(0);j<(node);j++){
      route_dist[i][j] = dir_dist[i][j];
    }
  }
  for(k=(0);k<(node);k++){
    for(i=(0);i<(node);i++){
      for(j=(0);j<(node);j++){
        chmin(route_dist[i][j], route_dist[i][k] + route_dist[k][j]);
      }
    }
  }
  {
    int ind[11];
    int opt_num = -1;
    int tmp_num;
    opt_route_dist = 1e150;
    for(i=(0);i<(N+1);i++){
      ind[i] = i;
    }
    do{
      if(ind[0] != 0){
        break;
      }
      tmp_route_dist = 0;
      tmp_num = 0;
      for(i=(0);i<(N);i++){
        if(route_dist[ind[i]][ind[i+1]] >= 1e100){
          break;
        }
        tmp_route_dist += route_dist[ind[i]][ind[i+1]];
        tmp_num++;
      }
      if(opt_num < tmp_num || (opt_num == tmp_num && opt_route_dist > tmp_route_dist)){
        opt_num = tmp_num;
        opt_route_dist = tmp_route_dist;
        for(i=(0);i<(N+1);i++){
          opt_route[i] = ind[i];
        }
      }
    }
    while(next_permutation(ind,ind+N+1));
  }
  int hakaru = 0;
  int hakaru_turn = 0;
  int cur_s;
  int next_p;
  int to_node;
  Point2d<int,long long,double> cp;
  Point2d<int,long long,double> cv;
  Point2d<int,long long,double> pcp;
  double tmp;
  double opt;
  double speed_lim;
  cp = S;
  cv.x = cv.y = 0;
  cur_s = 0;
  next_p = opt_route[1];
  for(t=(0);t<(do_loop);t++){
    if(hakaru == 1){
      QA[t] = 1;
      if(hakaru_turn==0){
        AX[t] = 1 - rnd.get(2)*2;
        AY[t] = 0;
      }
      if(hakaru_turn==1){
        AX[t] = 0;
        AY[t] = 1 - rnd.get(2)*2;
      }
      if(hakaru_turn==2){
        AX[t] = -1;
        AY[t] = 0;
      }
      if(hakaru_turn==3){
        AX[t] = 0;
        AY[t] = -1;
      }
      if(M==0){
        if(hakaru_turn==0 || hakaru_turn==2){
          if(cp.x < 0){
            AX[t] =-1;
          }
          else{
            AX[t] =1;
          }
        }
        else{
          if(cp.y < 0){
            AY[t] =-1;
          }
          else{
            AY[t] =1;
          }
        }
      }
      hakaru_turn++;
    }
    else{
      int Dy7VJs5B;
      opt = 1e150;
      for(i=(0);i<(node);i++){
        for(k=(0);k<(M);k++){
          if(Cross_Seg(cp,nodep[i],L[k],R[k])){
            goto p7xfqlvj;
          }
        }
        tmp = (cp-nodep[i]).dist() + route_dist[i][next_p] * 1.1;
        if(opt > tmp){
          opt = tmp;
          to_node = i;
        }
        p7xfqlvj:;
      }
      speed_lim = 3000;
      chmin(speed_lim, opt / 1.3);
      chmax(speed_lim, 400);
      opt = -1e150;
      for(Dy7VJs5B=(0);Dy7VJs5B<(1500);Dy7VJs5B++){
        int ax;
        int ay;
        int nx;
        int ny;
        double v;
        double dx;
        double dy;
        ax = rnd.get(-500,500);
        ay = rnd.get(-500,500);
        if(ax*ax+ay*ay > 500*500){
          continue;
        }
        nx = cp.x + cv.x + ax;
        ny = cp.y + cv.y + ay;
        v = sqrt((pow2_L((cv.x+ax)))+(pow2_L((cv.y+ay))));
        dx = nx - nodep[to_node].x;
        dy = ny - nodep[to_node].y;
        tmp = -sqrt(dx*dx+dy*dy);
        if(v > speed_lim){
          tmp -= 10 * (v - speed_lim) * (v - speed_lim);
        }
        if(opt < tmp){
          opt = tmp;
          AX[t] = ax;
          AY[t] = ay;
        }
      }
    }
    do_query(t);
    pcp = cp;
    if(QA[t]==0){
      cv.x += AX[t];
      cv.y += AY[t];
    }
    cp += cv;
    if(C[t]==1){
      cp = pcp;
      cv.x = cv.y = 0;
    }
    for(i=(0);i<(H[t]);i++){
      k = Q[t][i];
      vis[k] = 1;
      while(cur_s < N && vis[opt_route[cur_s+1]-1]){
        cur_s++;
      }
      next_p = opt_route[cur_s+1];
    }
    cur_score = cur_score - 2 - C[t] * 100 + H[t] * 1000;
    chmax(score, cur_score);
    fprintf(stderr,"[%d] : (%d %d) (%d %d) : (%d %d) (%d %d) : tarp (%d %d) tarn (%d %d) %f \n", cur_score, cp.x, cp.y, cv.x, cv.y, tp.x, tp.y, tv.x, tv.y, nodep[next_p].x, nodep[next_p].y, nodep[to_node].x, nodep[to_node].y, (tp-nodep[to_node]).dist());
    if(cur_s == N){
      break;
    }
    if(hakaru == 0 && (C[t]==1 || (cp-nodep[next_p]).dist() <= 500)){
      hakaru = 1;
      hakaru_turn = 0;
    }
    if(hakaru == 1 && hakaru_turn==4){
      int TScuxZBz;
      int px;
      int py;
      int vx;
      int vy;
      int o_px;
      int o_py;
      int o_vx;
      int o_vy;
      hakaru = hakaru_turn = 0;
      opt = -1e150;
      for(TScuxZBz=(0);TScuxZBz<(10000);TScuxZBz++){
        int jY1fB6nj;
        for(jY1fB6nj=(0);jY1fB6nj<(40);jY1fB6nj++){
          if(EPS < 0.2){
            if(rnd.getUni() < 0.0){
              px = rnd.get(-100000, 100000);
              py = rnd.get(-100000, 100000);
            }
            else{
              px = cp.x + rnd.get(-3000, 3000);
              py = cp.y + rnd.get(-3000, 3000);
            }
            vx = cv.x + rnd.get(-30, 30);
            vy = cv.y + rnd.get(-30, 30);
          }
          else{
            if(rnd.getUni() < 0.7){
              px = rnd.get(-100000, 100000);
              py = rnd.get(-100000, 100000);
            }
            else{
              px = cp.x + rnd.get(-10000, 10000);
              py = cp.y + rnd.get(-10000, 10000);
            }
            vx = cv.x + rnd.get(-30, 30);
            vy = cv.y + rnd.get(-30, 30);
          }
          px =Kth1_L(px, -100000+10, 100000-10);
          py =Kth1_L(py, -100000+10, 100000-10);
          tmp = 0;
          tmp -= fabs(D[t-3] - calc_dist(Point2d<int,long long,double>(px-vx*4, py-vy*4), AX[t-3], AY[t-3]) );
          tmp -= fabs(D[t-2] - calc_dist(Point2d<int,long long,double>(px-vx*3, py-vy*3), AX[t-2], AY[t-2]) );
          tmp -= fabs(D[t-1] - calc_dist(Point2d<int,long long,double>(px-vx*2, py-vy*2), AX[t-1], AY[t-1]) );
          tmp -= fabs(D[t-0] - calc_dist(Point2d<int,long long,double>(px-vx*1, py-vy*1), AX[t-0], AY[t-0]) );
          if(opt < tmp){
            opt = tmp;
            o_px = px;
            o_py = py;
            o_vx = vx;
            o_vy = vy;
          }
        }
        if(timer.get() > TL * t / do_loop){
          break;
        }
      }
      cp.x = o_px;
      cp.y = o_py;
      cv.x = o_vx;
      cv.y = o_vy;
    }
  }
  fprintf(stderr, "%d\n", score);
  for(i=(0);i<(N);i++){
    fprintf(stderr, "%d", vis[i]);
  }
  fprintf(stderr, "\n");
  return 0;
}
template<class T> inline void wt_L(const vector<T> &x){
  int fg = 0;
  for(auto a : x){
    if(fg){
      my_putchar(' ');
    }
    fg = 1;
    wt_L(a);
  }
}
template<class T> inline void wt_L(const set<T> &x){
  int fg = 0;
  for(auto a : x){
    if(fg){
      my_putchar(' ');
    }
    fg = 1;
    wt_L(a);
  }
}
template<class T> inline void wt_L(const multiset<T> &x){
  int fg = 0;
  for(auto a : x){
    if(fg){
      my_putchar(' ');
    }
    fg = 1;
    wt_L(a);
  }
}
template<class T1, class T2> inline void wt_L(const pair<T1,T2> x){
  wt_L(x.first);
  my_putchar(' ');
  wt_L(x.second);
}
// cLay version 20240420-1

// --- original code ---
// //interactive
// 
// const int mode = 0; // 1 = local, 0 = submit
// const int do_loop = 5000;
// 
// const double TL = 1.95;
// Timer2 timer;
// Rand rnd;
// 
// int N, M; double EPS, DELTA;
// Point2d<int,ll,double> S, P[10], L[10], R[10];
// 
// int node;
// Point2d<int,ll,double> nodep[50];
// double dir_dist[50][50], route_dist[50][50];
// 
// int opt_route[12]; double opt_route_dist, tmp_route_dist;
// 
// 
// double judge_alpha[5000];
// int judge_ax[5000], judge_ay[5000];
// 
// 
// int do_time;
// int QA[6000], AX[6000], AY[6000], C[6000], H[6000], Q[6000][20], D[6000];
// int vis[10];
// 
// 
// 
// template<class T, class S, class F>
// S InnerProdK(Point2d<T,S,F> a, Point2d<T,S,F> b){
//   return ((S)a.x) * ((S)b.x) + ((S)a.y) * ((S)b.y);
// }
// 
// double Dist_PointSeg(Point2d<int,ll,double> P, Point2d<int,ll,double> L, Point2d<int,ll,double> R){
//   if(InnerProdK(P-L, R-L) < 0) return (P-L).dist();
//   if(InnerProdK(P-R, L-R) < 0) return (P-R).dist();
//   return abs( CrossProd(P,L,R)/1.0 ) / (L-R).dist();
// }
// 
// int Cross_Seg(Point2d<int,ll,double> L1, Point2d<int,ll,double> R1, Point2d<int,ll,double> L2, Point2d<int,ll,double> R2){
//   ll a,b;
//   a = CrossProd(L1, R1, L2);
//   b = CrossProd(L1, R1, R2);
//   if(a < 0 && b < 0) return 0;
//   if(a > 0 && b > 0) return 0;
//   a = CrossProd(L2, R2, L1);
//   b = CrossProd(L2, R2, R1);
//   if(a < 0 && b < 0) return 0;
//   if(a > 0 && b > 0) return 0;
//   return 1;
// }
// 
// 
// 
// double calc_dist(Point2d<int,ll,double> a, double dx, double dy){
//   int ok, k;
//   double res, od;
//   Point2d<int,ll,double> b;
// 
//   if(M==0 && dx==0){
//     if(dy < 0) return a.y - (-1d5);
//     if(dy > 0) return 1d5 - a.y;
//   }
//   if(M==0 && dy==0){
//     if(dx < 0) return a.x - (-1d5);
//     if(dx > 0) return 1d5 - a.x;
//   }
// 
//   od = sqrt(dx*dx+dy*dy);
//   res = bsearch_min[double,d,0,2e5][
//     ok = 0;
//     b.x = a.x + dx / od * d;
//     b.y = a.y + dy / od * d;
//     if(abs(b.x) >= 1d5 || abs(b.y) >= 1d5) ok = 1;
//     if(!ok) rep(k,M){
//       if(Cross_Seg(a,b,L[k],R[k])) ok = 1, break;
//     }
//   ](ok);
// 
//   return res;
// }
// 
// 
// 
// Point2d<int,ll,double> tp, tv;
// void do_query(int t){
//   int i, j, k, c = 0;
// 
//   if(QA[t] == 0){
//     wt("A", AX[t], AY[t]);
//   } else {
//     wt("S", AX[t], AY[t]);
//   }
// 
//   if(mode == 1){
//     Point2d<int,ll,double> p;
// 
//     if(QA[t]==0){
//       tv.x += AX[t] + judge_ax[t];
//       tv.y += AY[t] + judge_ay[t];
//     } else {
//       D[t] = round(calc_dist(tp, AX[t], AY[t]) * judge_alpha[t]);
//       tv.x += judge_ax[t];
//       tv.y += judge_ay[t];
//     }
// 
//     p = tp;
//     tp += tv;
//     rep(k,M) if(Cross_Seg(p,tp,L[k],R[k])) c = 1, break;
// 
//     if(tp.x <= -1d5 || tp.y <= -1d5) c = 1;
//     if(tp.x >=  1d5 || tp.y >=  1d5) c = 1;
// 
//     if(c){
//       tp = p;
//       tv.x = tv.y = 0;
//       C[t] = 1;
//       H[t] = 0;
//     } else {
//       rep(k,N) if(vis[k]==0){
//         if(Dist_PointSeg(P[k],p,tp) <= 1000+1e-12) Q[t][H[t]++] = k;
//       }
//     }
// 
//     // wt("---", C[t], H[t], ":", Q[t](H[t]));
// 
//   } else {
//     if(QA[t]==1) rd(D[t]);
//     rd(C[t], H[t], Q[t](H[t]));
//   }
// }
// 
// 
// 
// {
//   int i, j, k;
//   int cur_score = 0, score = 0;
//   timer.set();
//   rd(N,M,EPS,DELTA,S,P(N),(L,R)(M));
// 
//   if(mode){
//     rd(judge_alpha(5000), (judge_ax, judge_ay)(5000));
//     tp = S;
//     tv.x = tv.y = 0;
//   }
// 
//   node = 0;
//   nodep[node++] = S;
//   rep(i,N) nodep[node++] = P[i];
// 
//   rep(i,M){
//     int xx, yy;
//     double dx, dy, d;
//     dx = L[i].x - R[i].x;
//     dy = L[i].y - R[i].y;
//     d = (L[i] - R[i]).dist();
//     xx = L[i].x + dx / d * 2000;
//     yy = L[i].y + dy / d * 2000;
//     if(-1d5 <= xx <= 1d5 && -1d5 <= yy <= 1d5) nodep[node].x = xx, nodep[node].y = yy, node++;
//     xx = R[i].x - dx / d * 2000;
//     yy = R[i].y - dy / d * 2000;
//     if(-1d5 <= xx <= 1d5 && -1d5 <= yy <= 1d5) nodep[node].x = xx, nodep[node].y = yy, node++;
//   }
// 
//   rep(i,node) dir_dist[i][i] = 0;
//   rep(i,node) rep(j,i+1,node){
//     dir_dist[i][j] = (nodep[i]-nodep[j]).dist();
//     rep(k,M) if(Cross_Seg(nodep[i],nodep[j],L[k],R[k])) dir_dist[i][j] = double_inf;
//     dir_dist[j][i] = dir_dist[i][j];
//   }
// 
//   rep(i,node) rep(j,node) route_dist[i][j] = dir_dist[i][j];
//   rep(k,node) rep(i,node) rep(j,node) route_dist[i][j] <?= route_dist[i][k] + route_dist[k][j];
// 
//   {
//     int ind[11];
//     int opt_num = -1, tmp_num;
//     opt_route_dist = double_inf;
// 
//     rep(i,N+1) ind[i] = i;
//     do{
//       if(ind[0] != 0) break;
//       tmp_route_dist = 0;
//       tmp_num = 0;
//       rep(i,N){
//         if(route_dist[ind[i]][ind[i+1]] >= 1e100) break;
//         tmp_route_dist += route_dist[ind[i]][ind[i+1]];
//         tmp_num++;
//       } 
//       if(opt_num < tmp_num || (opt_num == tmp_num && opt_route_dist > tmp_route_dist)){
//         opt_num = tmp_num;
//         opt_route_dist = tmp_route_dist;
//         rep(i,N+1) opt_route[i] = ind[i];
//       }
//     }while(next_permutation(ind,ind+N+1));
//   }
// 
//   // wt(opt_route_dist);
//   // return 0;
// 
// 
//   int hakaru = 0, hakaru_turn = 0;
//   int cur_s, next_p, to_node;
//   Point2d<int,ll,double> cp, cv, pcp;
//   double tmp, opt, speed_lim;
//   cp = S;
//   cv.x = cv.y = 0;
// 
//   cur_s = 0; next_p = opt_route[1];
//   rep(t,do_loop){
//     if(hakaru == 1){
//       QA[t] = 1;
//       if(hakaru_turn==0) AX[t] = 1 - rnd.get(2)*2, AY[t] = 0;
//       if(hakaru_turn==1) AX[t] = 0, AY[t] = 1 - rnd.get(2)*2;
//       if(hakaru_turn==2) AX[t] = -1, AY[t] = 0;
//       if(hakaru_turn==3) AX[t] = 0, AY[t] = -1;
// 
//       if(M==0){
//         if(hakaru_turn==0 || hakaru_turn==2){
//           AX[t] = if[cp.x < 0, -1, 1];
//         } else {
//           AY[t] = if[cp.y < 0, -1, 1];
//         }
//       }
//       hakaru_turn++;
//     } else {
//       opt = double_inf;
//       rep(i,node){
//         rep(k,M) if(Cross_Seg(cp,nodep[i],L[k],R[k])) break_continue;
//         tmp = (cp-nodep[i]).dist() + route_dist[i][next_p] * 1.1;
//         if(opt > tmp) opt = tmp, to_node = i;
//       }
//       speed_lim = 3000;
//       speed_lim <?= opt / 1.3;
//       speed_lim >?= 400;
// 
//       opt = -double_inf;
//       rep(1500){
//         int ax, ay, nx, ny; double v, dx, dy;
//         ax = rnd.get(-500,500);
//         ay = rnd.get(-500,500);
//         if(ax*ax+ay*ay > 500*500) continue;
//         nx = cp.x + cv.x + ax;
//         ny = cp.y + cv.y + ay;
//         v = sqrt( (cv.x+ax) ** 2 + (cv.y+ay) ** 2 );
//         dx = nx - nodep[to_node].x;
//         dy = ny - nodep[to_node].y;
//         tmp = -sqrt(dx*dx+dy*dy);
//         if(v > speed_lim) tmp -= 10 * (v - speed_lim) * (v - speed_lim);
//         if(opt < tmp) opt = tmp, AX[t] = ax, AY[t] = ay;
//       }
//     }
// 
//     do_query(t);
//     pcp = cp;
//     if(QA[t]==0){
//       cv.x += AX[t];
//       cv.y += AY[t];
//     }
//     cp += cv;
// 
//     if(C[t]==1) cp = pcp, cv.x = cv.y = 0;
//     rep(i,H[t]){
//       k = Q[t][i];
//       vis[k] = 1;
//       while(cur_s < N && vis[opt_route[cur_s+1]-1]) cur_s++;
//       next_p = opt_route[cur_s+1];
//     }
// 
//     cur_score = cur_score - 2 - C[t] * 100 + H[t] * 1000;
//     score >?= cur_score;
// 
//     fprintf(stderr,"[%d] : (%d %d) (%d %d) : (%d %d) (%d %d) : tarp (%d %d) tarn (%d %d) %f \n", cur_score, cp.x, cp.y, cv.x, cv.y, tp.x, tp.y, tv.x, tv.y, nodep[next_p].x, nodep[next_p].y, nodep[to_node].x, nodep[to_node].y, (tp-nodep[to_node]).dist());
//     if(cur_s == N) break;
// 
//     if(hakaru == 0 && (C[t]==1 || (cp-nodep[next_p]).dist() <= 500)) hakaru = 1, hakaru_turn = 0;
// 
//     if(hakaru == 1 && hakaru_turn==4){
//       int px, py, vx, vy;
//       int o_px, o_py, o_vx, o_vy;
//       hakaru = hakaru_turn = 0;
// 
//       opt = -double_inf;
//       rep(10000){
//         rep(40){
//           // px = cp.x + rnd.get(-10000,10000);
//           // py = cp.y + rnd.get(-10000,10000);
//           // vx = cv.x + rnd.get(-1000,1000);
//           // vy = cv.y + rnd.get(-1000,1000);
//           if(EPS < 0.2){
//             if(rnd.getUni() < 0.0){
//               px = rnd.get(-1d5, 1d5);
//               py = rnd.get(-1d5, 1d5);
//             } else {
//               px = cp.x + rnd.get(-3d3, 3d3);
//               py = cp.y + rnd.get(-3d3, 3d3);
//             }
//             vx = cv.x + rnd.get(-50, 50);
//             vy = cv.y + rnd.get(-50, 50);
//           } else {
//             if(rnd.getUni() < 0.7){
//               px = rnd.get(-1d5, 1d5);
//               py = rnd.get(-1d5, 1d5);
//             } else {
//               px = cp.x + rnd.get(-1d4, 1d4);
//               py = cp.y + rnd.get(-1d4, 1d4);
//             }
//             vx = cv.x + rnd.get(-50, 50);
//             vy = cv.y + rnd.get(-50, 50);
//           }
//           px = Kth1(px, -1d5+10, 1d5-10);
//           py = Kth1(py, -1d5+10, 1d5-10);
// 
//           tmp = 0;
//           tmp -= fabs(D[t-3] - calc_dist(Point2d<int,ll,double>(px-vx*4, py-vy*4), AX[t-3], AY[t-3]) );
//           tmp -= fabs(D[t-2] - calc_dist(Point2d<int,ll,double>(px-vx*3, py-vy*3), AX[t-2], AY[t-2]) );
//           tmp -= fabs(D[t-1] - calc_dist(Point2d<int,ll,double>(px-vx*2, py-vy*2), AX[t-1], AY[t-1]) );
//           tmp -= fabs(D[t-0] - calc_dist(Point2d<int,ll,double>(px-vx*1, py-vy*1), AX[t-0], AY[t-0]) );
// 
//           // wt("---");
//           // wt(tmp,opt);
//           // wt(D[t-3],calc_dist(Point2d<int,ll,double>(px-vx*4, py-vy*4), 1, 0) );
//           // wt(D[t-2],calc_dist(Point2d<int,ll,double>(px-vx*3, py-vy*3), 0, 1));
//           // wt(D[t-1],calc_dist(Point2d<int,ll,double>(px-vx*2, py-vy*2), -1, 0) );
//           // wt(D[t-0],calc_dist(Point2d<int,ll,double>(px-vx*1, py-vy*1), 0, -1) );
// 
//           if(opt < tmp){
//             opt = tmp;
//             o_px = px; o_py = py;
//             o_vx = vx; o_vy = vy;
//           }
//         }
//         if(timer.get() > TL * t / do_loop) break;
//       }
// 
//       cp.x = o_px; cp.y = o_py;
//       cv.x = o_vx; cv.y = o_vy;
//     }
//   }
// 
//   fprintf(stderr, "%d\n", score);
//   rep(i,N) fprintf(stderr, "%d", vis[i]); fprintf(stderr, "\n");
// }
