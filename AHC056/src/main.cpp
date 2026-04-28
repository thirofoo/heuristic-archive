// -*- compile-command: "make" -*-
#pragma GCC optimize "-O3,omit-frame-pointer,inline,unroll-all-loops,fast-math"
 #pragma GCC target "tune=native"
 #include <bits/stdc++.h>
 #include <sys/time.h>
 #include <immintrin.h>
 #include <x86intrin.h>
// INCLUDE <ext/pb_ds/assoc_container.hpp>
 #include <immintrin.h>
using namespace std;
// Macros
using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
template <class T> using min_queue = priority_queue<T, vector<T>, greater<T>>;
template <class T> using max_queue = priority_queue<T>;
struct uint64_hash {
  static inline uint64_t rotr(uint64_t x, unsigned k) {
    return (x >> k) | (x << (8U * sizeof(uint64_t) - k));
  }
  static inline uint64_t hash_int(uint64_t x) noexcept {
    auto h1 = x * (uint64_t)(0xA24BAED4963EE407);
    auto h2 = rotr(x, 32U) * (uint64_t)(0x9FB21C651E98DF25);
    auto h = rotr(h1 + h2, 32U);
    return h;
  }
  size_t operator()(uint64_t x) const {
    static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
    return hash_int(x + FIXED_RANDOM);
  }
};
// template <typename K, typename V, typename Hash = uint64_hash>
// using hash_map = __gnu_pbds::gp_hash_table<K, V, Hash>;
// template <typename K, typename Hash = uint64_hash>
// using hash_set = hash_map<K, __gnu_pbds::null_type, Hash>;
// Types
template<class T>
using min_queue = priority_queue<T, vector<T>, greater<T>>;
template<class T>
using max_queue = priority_queue<T>;
// Printing
template<class T>
void print_collection(ostream& out, T const& x);
template<class T, size_t... I>
void print_tuple(ostream& out, T const& a, index_sequence<I...>);
namespace std {
  template<class... A>
  ostream& operator<<(ostream& out, tuple<A...> const& x) {
    print_tuple(out, x, index_sequence_for<A...>{});
    return out;
  }
  template<class... A>
  ostream& operator<<(ostream& out, pair<A...> const& x) {
    print_tuple(out, x, index_sequence_for<A...>{});
    return out;
  }
  template<class A, size_t N>
  ostream& operator<<(ostream& out, array<A, N> const& x) { print_collection(out, x); return out; }
  template<class A>
  ostream& operator<<(ostream& out, vector<A> const& x) { print_collection(out, x); return out; }
  template<class A>
  ostream& operator<<(ostream& out, deque<A> const& x) { print_collection(out, x); return out; }
  template<class A>
  ostream& operator<<(ostream& out, multiset<A> const& x) { print_collection(out, x); return out; }
  template<class A, class B>
  ostream& operator<<(ostream& out, multimap<A, B> const& x) { print_collection(out, x); return out; }
  template<class A>
  ostream& operator<<(ostream& out, set<A> const& x) { print_collection(out, x); return out; }
  template<class A, class B>
  ostream& operator<<(ostream& out, map<A, B> const& x) { print_collection(out, x); return out; }
  template<class A, class B>
  ostream& operator<<(ostream& out, unordered_set<A> const& x) { print_collection(out, x); return out; }
}
template<class T, size_t... I>
void print_tuple(ostream& out, T const& a, index_sequence<I...>){
  using swallow = int[];
  out << '(';
  (void)swallow{0, (void(out << (I == 0? "" : ", ") << get<I>(a)), 0)...};
  out << ')';
}
template<class T>
void print_collection(ostream& out, T const& x) {
  int f = 0;
  out << '[';
  for(auto const& i: x) {
    out << (f++ ? "," : "");
    out << i;
  }
  out << "]";
}
// Random
struct RNG {
  uint64_t s[2];
  RNG(u64 seed) {
    reset(seed);
  }
  RNG() {
    reset(time(0));
  }
  using result_type = u32;
  constexpr u32 min(){ return numeric_limits<u32>::min(); }
  constexpr u32 max(){ return numeric_limits<u32>::max(); }
  u32 operator()() { return randomInt32(); }
  static __attribute__((always_inline)) inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
  }
  inline void reset(u64 seed) {
    struct splitmix64_state {
      u64 s;
      u64 splitmix64() {
        u64 result = (s += 0x9E3779B97f4A7C15);
        result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
        result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
        return result ^ (result >> 31);
      }
    };
    splitmix64_state sm { seed };
    s[0] = sm.splitmix64();
    s[1] = sm.splitmix64();
  }
  uint64_t next() {
    const uint64_t s0 = s[0];
    uint64_t s1 = s[1];
    const uint64_t result = rotl(s0 * 5, 7) * 9;
    s1 ^= s0;
    s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    s[1] = rotl(s1, 37); // c
    return result;
  }
  inline u32 randomInt32() {
    return next();
  }
  inline u64 randomInt64() {
    return next();
  }
  inline u32 random32(u32 r) {
    return (((u64)randomInt32())*r)>>32;
  }
  inline u64 random64(u64 r) {
    return randomInt64()%r;
  }
  inline u32 randomRange32(u32 l, u32 r) {
    return l + random32(r-l+1);
  }
  inline u64 randomRange64(u64 l, u64 r) {
    return l + random64(r-l+1);
  }
  inline double randomDouble() {
    return (double)randomInt32() / 4294967296.0;
  }
  inline float randomFloat() {
    return (float)randomInt32() / 4294967296.0;
  }
  inline double randomRangeDouble(double l, double r) {
    return l + randomDouble() * (r-l);
  }
  template<class T, size_t N>
  void shuffle(array<T, N>& v) {
    for(i32 i = N; i > 1; i--) {
      i32 p = random32(i);
      swap(v[i-1],v[p]);
    }
  }
  template<class T>
  void shuffle(vector<T>& v) {
    i32 sz = v.size();
    for(i32 i = sz; i > 1; i--) {
      i32 p = random32(i);
      swap(v[i-1],v[p]);
    }
  }
  template<class T>
  void shuffle(T* fr, T* to) {
    i32 sz = distance(fr,to);
    for(int i = sz; i > 1; i--) {
      int p = random32(i);
      swap(fr[i-1],fr[p]);
    }
  }
  template<class T>
  inline int sample_index(vector<T> const& v) {
    return random32(v.size());
  }
  template<class T>
  inline T sample(vector<T> const& v) {
    return v[sample_index(v)];
  }
} rng;
// Timer
struct timer {
  chrono::high_resolution_clock::time_point t_begin;
  timer() {
    t_begin = chrono::high_resolution_clock::now();
  }
  void reset() {
    t_begin = chrono::high_resolution_clock::now();
  }
  float elapsed() const {
    return chrono::duration<float>(chrono::high_resolution_clock::now() - t_begin).count();
  }
};
// Util
template<class T>
T& smin(T& x, T const& y) { x = min(x,y); return x; }
template<class T>
T& smax(T& x, T const& y) { x = max(x,y); return x; }
template<typename T>
int sgn(T val) {
  if(val < 0) return -1;
  if(val > 0) return 1;
  return 0;
}
static inline
string int_to_string(int val, int digits = 0) {
  string s = to_string(val);
  reverse(begin(s), end(s));
  while((int)s.size() < digits) s.push_back('0');
  reverse(begin(s), end(s));
  return s;
}
// Debug
static inline void debug_impl_seq() {
  cerr << "}";
}
template <class T, class... V>
void debug_impl_seq(T const& t, V const&... v) {
  cerr << t;
  if(sizeof...(v)) { cerr << ", "; }
  debug_impl_seq(v...);
}
// Bits
__attribute__((always_inline)) inline
u64 bit(u64 x) { return 1ull<<x; }
__attribute__((always_inline)) inline
u64 popcount(u64 x) { return __builtin_popcountll(x); }
__attribute__((always_inline)) inline
void setbit(u64& a, u32 b, u64 value = 1) {
  a = (a&~bit(b)) | (value<<b);
}
__attribute__((always_inline)) inline
u64 getbit(u64 a, u32 b) {
  return (a>>b)&1;
}
__attribute__((always_inline)) inline
u64 lsb(u64 a) {
  return __builtin_ctzll(a);
}
__attribute__((always_inline)) inline
int msb(uint64_t bb) {
  return __builtin_clzll(bb) ^ 63;
}
// more
template<class T, size_t SZ>
struct smallvec {
  int size;
  T elems[SZ];
  __attribute__((always_inline)) inline void reset() { size = 0; }
  __attribute__((always_inline)) inline void push(T const& x) { elems[size++] = x; }
  __attribute__((always_inline)) inline void pop() { size -= 1; }
  T& operator[](size_t index) { return elems[index]; }
  T const& operator[](size_t index) const { return elems[index]; }
};
template<size_t N>
struct static_union_find {
  int A[N];
  void reset(int n = N) {
    for(i32 i = 0; i < (i32)(n); ++i) A[i] = i;
  }
  i32 find(i32 a) {
    return A[a] == a ? a : A[a] = find(A[a]);
  }
  bool unite(i32 a, i32 b) {
    a = find(a); b = find(b);
    if(a != b) {
      A[a] = b;
      return true;
    }else{
      return false;
    }
  }
};
struct union_find {
  vector<i32> A, S;
  union_find(i32 n = 0) : A(n) {
    iota(begin(A), end(A), 0);
  }
  i32 find(i32 a) {
    return A[a] == a ? a : A[a] = find(A[a]);
  }
  bool unite(i32 a, i32 b) {
    a = find(a); b = find(b);
    if(a != b) {
      A[a] = b;
      return true;
    }else{
      return false;
    }
  }
};
template<int N>
struct fast_queue {
  int nqueue;
  int queue[N];
  int queue_pos[N];
  void clear() {
    nqueue = 0;
  }
  __attribute__((always_inline)) inline void add(int x) {
    queue_pos[x] = nqueue;
    queue[nqueue++] = x;
  }
  __attribute__((always_inline)) inline void remove(int x) {
    int i = queue_pos[x];
    swap(queue[i], queue[nqueue-1]);
    queue_pos[queue[i]] = i;
    nqueue -= 1;
  }
};
struct linked_lists {
  int L, N;
  vector<int> next, prev;
  // L: lists are [0...L), N: elements are [0,...N)
  explicit linked_lists(int L = 0, int N = 0) { assign(L, N); }
  int rep(int l) const { return l + N; } // "representative" of list l
  int head(int l) const { return next[rep(l)]; }
  int tail(int l) const { return prev[rep(l)]; }
  bool empty(int l) const { return next[rep(l)] == rep(l); }
  void push_front(int l, int n) { link(rep(l), n, head(l)); }
  void push_back(int l, int n) { link(tail(l), n, rep(l)); }
  void insert_before(int i, int n) { link(prev[i], n, i); }
  void insert_after(int i, int n) { link(i, n, next[i]); }
  void erase(int n) { link(prev[n], next[n]); }
  void pop_front(int l) { link(rep(l), next[head(l)]); }
  void pop_back(int l) { link(prev[tail(l)], rep(l)); }
  void clear() {
    iota(begin(next) + N, end(next), N); // sets next[rep(l)] = rep(l)
    iota(begin(prev) + N, end(prev), N); // sets prev[rep(l)] = rep(l)
  }
  void assign(int L, int N) {
    this->L = L, this->N = N;
    next.resize(N + L), prev.resize(N + L), clear();
  }
private:
  inline void link(int u, int v) { next[u] = v, prev[v] = u; }
  inline void link(int u, int v, int w) { link(u, v), link(v, w); }
};
// Iterate through elements (call them i) of the list #l in "lists"
template <typename Flow, typename Cost>
struct network_simplex {
  // we number the vertices 0,...,V-1, R is given number V.
  explicit network_simplex(int V) : V(V), node(V + 1) {}
  int add(int u, int v, Flow cap, Cost cost) {
    assert(0 <= u && u < V && 0 <= v && v < V);
    edge.push_back({{u, v}, cap, cost});
    return E++;
  }
  void set_supply(int u, Flow supply) { node[u].supply = supply; }
  void set_demand(int u, Flow demand) { node[u].supply = -demand; }
  auto get_supply(int u) const { return node[u].supply; }
  auto get_potential(int u) const { return node[u].pi; }
  auto get_flow(int e) const { return edge[e].flow; }
  auto get_cost(int e) const { return edge[e].cost; }
  auto reduced_cost(int e) const {
    auto [u, v] = edge[e].node;
    return edge[e].cost + node[u].pi - node[v].pi;
  }
  template <typename CostSum = Cost>
  auto circulation_cost() const {
    CostSum sum = 0;
    for (int e = 0; e < E; e++) {
      sum += edge[e].flow * CostSum(edge[e].cost);
    }
    return sum;
  }
  void verify_spanning_tree() const {
    for (int e = 0; e < E; e++) {
      assert(0 <= edge[e].flow && edge[e].flow <= edge[e].cap);
      assert(edge[e].flow == 0 || reduced_cost(e) <= 0);
      assert(edge[e].flow == edge[e].cap || reduced_cost(e) >= 0);
    }
  }
  bool mincost_circulation() {
    static constexpr bool INFEASIBLE = false, OPTIMAL = true;
    // Check trivialities: positive cap[e] and sum of supplies is 0
    Flow sum_supply = 0;
    for (int u = 0; u < V; u++) {
      sum_supply += node[u].supply;
    }
    if (sum_supply != 0) {
      return INFEASIBLE;
    }
    for (int e = 0; e < E; e++) {
      if (edge[e].cap < 0) {
        return INFEASIBLE;
      }
    }
    // Compute inf_cost as sum of all costs + 1, and reset the flow network
    Cost inf_cost = 1;
    for (int e = 0; e < E; e++) {
      edge[e].flow = 0;
      edge[e].state = STATE_LOWER;
      inf_cost += abs(edge[e].cost);
    }
    edge.resize(E + V); // make space for V artificial edges
    bfs.resize(V + 1);
    children.assign(V + 1, V + 1);
    // Add V artificial edges with infinite cost and initial supply for feasible flow
    int root = V;
    node[root] = {-1, -1, 0, 0};
    for (int u = 0, e = E; u < V; u++, e++) {
      // spanning tree links
      node[u].parent = root, node[u].pred = e;
      children.push_back(root, u);
      auto supply = node[u].supply;
      if (supply >= 0) {
        node[u].pi = -inf_cost;
        edge[e] = {{u, root}, supply, inf_cost, supply, STATE_TREE};
      } else {
        node[u].pi = inf_cost;
        edge[e] = {{root, u}, -supply, inf_cost, -supply, STATE_TREE};
      }
    }
    // We want to, hopefully, find a pivot edge in O(sqrt(E)). This can be tuned.
    block_size = max(int(ceil(sqrt(E + V))), min(10, V + 1));
    next_arc = 0;
    // Pivot pivot pivot
    int in_arc = select_pivot_edge();
    while (in_arc != -1) {
      pivot(in_arc);
      in_arc = select_pivot_edge();
    }
    // If there is >0 flow through an artificial edge, the problem is infeasible.
    for (int e = E; e < E + V; e++) {
      if (edge[e].flow > 0) {
        edge.resize(E);
        return INFEASIBLE;
      }
    }
    edge.resize(E);
    return OPTIMAL;
  }
private:
  enum ArcState : int8_t { STATE_UPPER = -1, STATE_TREE = 0, STATE_LOWER = 1 };
  struct Node {
    int parent, pred;
    Flow supply;
    Cost pi;
  };
  struct Edge {
    array<int, 2> node; // [0]->[1]
    Flow cap;
    Cost cost;
    Flow flow = 0;
    ArcState state = STATE_LOWER;
  };
  int V, E = 0, next_arc = 0, block_size = 0;
  vector<Node> node;
  vector<Edge> edge;
  linked_lists children;
  vector<int> bfs; // scratchpad for downwards bfs and evert
  int select_pivot_edge() {
    // block search: check block_size edges looping, and pick the lowest reduced cost
    Cost minimum = 0;
    int in_arc = -1;
    int count = block_size, seen_edges = E + V;
    for (int& e = next_arc; seen_edges-- > 0; e = e + 1 == E + V ? 0 : e + 1) {
      if (minimum > edge[e].state * reduced_cost(e)) {
        minimum = edge[e].state * reduced_cost(e);
        in_arc = e;
      }
      if (--count == 0 && minimum < 0) {
        break;
      } else if (count == 0) {
        count = block_size;
      }
    }
    return in_arc;
  }
  void pivot(int in_arc) {
    // Find lca of u_in and v_in with two pointers technique
    auto [u_in, v_in] = edge[in_arc].node;
    int a = u_in, b = v_in;
    while (a != b) {
      a = node[a].parent == -1 ? v_in : node[a].parent;
      b = node[b].parent == -1 ? u_in : node[b].parent;
    }
    int lca = a;
    // Orient the edge so that we add flow along u_in->v_in
    if (edge[in_arc].state == STATE_UPPER) {
      swap(u_in, v_in);
    }
    // Let's find the saturing flow push
    enum OutArcSide { SAME_EDGE, U_IN_SIDE, V_IN_SIDE };
    OutArcSide side = SAME_EDGE;
    Flow flow_delta = edge[in_arc].cap; // how much we can push to saturate
    int u_out = -1; // the exiting arc
    // Go up from u_in to lca, break ties by prefering lower vertices
    for (int u = u_in; u != lca && flow_delta > 0; u = node[u].parent) {
      int e = node[u].pred;
      bool edge_down = u == edge[e].node[1];
      Flow to_saturate = edge_down ? edge[e].cap - edge[e].flow : edge[e].flow;
      if (flow_delta > to_saturate) {
        flow_delta = to_saturate;
        u_out = u;
        side = U_IN_SIDE;
      }
    }
    // Go up from v_in to lca, break ties by prefering higher vertices
    for (int u = v_in; u != lca; u = node[u].parent) {
      int e = node[u].pred;
      bool edge_up = u == edge[e].node[0];
      Flow to_saturate = edge_up ? edge[e].cap - edge[e].flow : edge[e].flow;
      if (flow_delta >= to_saturate) {
        flow_delta = to_saturate;
        u_out = u;
        side = V_IN_SIDE;
      }
    }
    // Augment along the cycle if we can push anything
    if (flow_delta > 0) {
      auto delta = edge[in_arc].state * flow_delta;
      edge[in_arc].flow += delta;
      for (int u = edge[in_arc].node[0]; u != lca; u = node[u].parent) {
        int e = node[u].pred;
        edge[e].flow += u == edge[e].node[0] ? -delta : delta;
      }
      for (int u = edge[in_arc].node[1]; u != lca; u = node[u].parent) {
        int e = node[u].pred;
        edge[e].flow += u == edge[e].node[0] ? delta : -delta;
      }
    }
    // Return now if we didn't change the spanning tree. The state of in_arc flipped.
    if (side == SAME_EDGE) {
      edge[in_arc].state = ArcState(-edge[in_arc].state);
      return;
    }
    // Basis exchange: Replace out_arc with in_arc in the spanning tree
    int out_arc = node[u_out].pred;
    edge[in_arc].state = STATE_TREE;
    edge[out_arc].state = edge[out_arc].flow ? STATE_UPPER : STATE_LOWER;
    // Put u_in on the same side as u_out
    if (side == V_IN_SIDE) {
      swap(u_in, v_in);
    }
    // Evert: Walk up from u_in to u_out, and fix parent/pred/child pointers downwards
    int i = 0, S = 0;
    for (int u = u_in; u != u_out; u = node[u].parent) {
      bfs[S++] = u;
    }
    assert(S <= V);
    for (i = S - 1; i >= 0; i--) {
      int u = bfs[i], p = node[u].parent;
      children.erase(p); // remove p from its children list and add it to u's
      children.push_back(u, p);
      node[p].parent = u;
      node[p].pred = node[u].pred;
    }
    children.erase(u_in); // remove u_in from its children list and add it to v_in's
    children.push_back(v_in, u_in);
    node[u_in].parent = v_in;
    node[u_in].pred = in_arc;
    // Fix potentials: Visit the subtree of u_in (pi_delta is not 0).
    Cost current_pi = reduced_cost(in_arc);
    Cost pi_delta = u_in == edge[in_arc].node[0] ? -current_pi : current_pi;
    bfs[0] = u_in;
    for (i = 0, S = 1; i < S; i++) {
      int u = bfs[i];
      node[u].pi += pi_delta;
      for (int zv = u, v = children.head(zv); v != children.rep(zv); v = children.next[v]) { bfs[S++] = v; }
    }
  }
};

const f64 TIME_FACTOR = 1.0;
const f64 TL_SA1 = 1.0 * TIME_FACTOR;
const f64 TL_SA2 = 1.1 * TIME_FACTOR;
const f64 TL_PANIC = 1.85 * TIME_FACTOR;
const f64 TL_BEAM = 1.95 * TIME_FACTOR;
timer TIMER;
const int MAXN = 20;
const int MAXNN = MAXN*MAXN;
int n,k,t,nn;
u8 graph[MAXNN];
int G[MAXNN][4];
int dxy[5];
const char* dc = "DURLS";
int targets[MAXNN+1];
int dist[MAXNN][MAXNN];
void read() {
  cin>>n>>k>>t;
  cerr << "[DATA] n=" << n << endl;
  cerr << "[DATA] k=" << k << endl;
  nn = n*n;
  dxy[0] = n;
  dxy[1] = -n;
  dxy[2] = 1;
  dxy[3] = -1;
  dxy[4] = 0;
  { for(i32 u = 0; u < (i32)(nn); ++u) graph[u] = 0;
    for(i32 x = 0; x < (i32)(n); ++x) for(i32 y = 0; y < (i32)(n); ++y) {
      if(x+1 < n) {
        graph[x*n+y] |= bit(0);
        graph[(x+1)*n+y] |= bit(1);
      }
      if(y+1 < n) {
        graph[x*n+y] |= bit(2);
        graph[x*n+(y+1)] |= bit(3);
      }
    }
  }
  int nw = 0;
  for(i32 x = 0; x < (i32)(n); ++x) for(i32 y = 0; y < (i32)(n-1); ++y) {
    char b; cin>>b;
    if(b == '1') {
      nw += 1;
      graph[x*n+y] &= ~bit(2);
      graph[x*n+y+1] &= ~bit(3);
    }
  }
  for(i32 x = 0; x < (i32)(n-1); ++x) for(i32 y = 0; y < (i32)(n); ++y) {
    char b; cin>>b;
    if(b == '1') {
      nw += 1;
      graph[x*n+y] &= ~bit(0);
      graph[(x+1)*n+y] &= ~bit(1);
    }
  }
  for(i32 u = 0; u < (i32)(nn); ++u) for(i32 d = 0; d < (i32)(4); ++d) {
    G[u][d] = -1;
    if(graph[u]&bit(d)) G[u][d] = u+dxy[d];
  }
  cerr << "[DATA] nw=" << nw << endl;
  for(i32 i = 0; i < (i32)(k); ++i) {
    int x,y; cin>>x>>y;
    targets[i] = x*n+y;
  }
  targets[k] = -1;
}
void init_dist() {
  for(i32 u = 0; u < (i32)(nn); ++u) {
    const int INF = 1e6;
    static int Q[MAXNN];
    for(i32 v = 0; v < (i32)(nn); ++v) dist[u][v] = INF;
    int nq = 0;
    Q[nq++] = u;
    dist[u][u] = 0;
    for(i32 iq = 0; iq < (i32)(nq); ++iq) {
      int v = Q[iq];
      for(i32 d = 0; d < (i32)(4); ++d) if(graph[v]&bit(d)) {
        if(dist[u][v+dxy[d]] == INF) {
          dist[u][v+dxy[d]] = dist[u][v]+1;
          Q[nq++] = v+dxy[d];
        }
      }
    }
  }
}
int total_dist = 0;
void preprocess() {
  vector<int> new_targets(targets, targets+k);
  for(i32 i = 0; i < (i32)(k-1); ++i) {
    int x = new_targets[i];
    int& y = new_targets[i+1];
    while(1) {
      int bd = -1;
      for(i32 d = 0; d < (i32)(4); ++d) if((graph[y]&bit(d)) && dist[x][y+dxy[d]] > dist[x][y]) {
        bool ok = 1;
        for(i32 d2 = 0; d2 < (i32)(4); ++d2) if(d2/2 != d/2 && (graph[y]&bit(d2))) ok = 0;
        if(ok) bd = d;
      }
      if(bd == -1) break;
      y += dxy[bd];
    }
  }
  int new_total_dist = 0;
  for(i32 i = 0; i < (i32)(k-1); ++i) new_total_dist += dist[new_targets[i]][new_targets[i+1]];
  if(new_total_dist <= t) {
    total_dist = new_total_dist;
    for(i32 i = 0; i < (i32)(k); ++i) targets[i] = new_targets[i];
  }
  k = distance(targets, unique(targets, targets+k));
}
struct euler_tour {
  int* ptr;
  i32 sz;
  __attribute__((always_inline)) inline void push(int e) {
    ptr[sz++] = e;
  }
};
int *tour0, *tour1;
void init_tours() {
  tour0 = new int[1<<24];
  tour1 = new int[1<<24];
}
void init() {
  init_tours();
  init_dist();
  for(i32 i = 0; i < (i32)(k-1); ++i) total_dist += dist[targets[i]][targets[i+1]];
  do{}while(0);
  cerr << "[DATA] t=" << (1.0 * t / total_dist) << endl;
  preprocess();
  do{}while(0);
}
const int INF = 1<<29;
struct sa_solver {
  int grid[MAXNN];
  int last[MAXNN];
  int date;
  int score, len;
  int fixed_grid[MAXNN];
  int fixed_score;
  int fixed_len;
  void init() {
    for(i32 i = 0; i < (i32)(nn); ++i) {
      fixed_grid[i] = 0;
      grid[i] = 4;
    }
    fixed_score = 0;
    fixed_len = 0;
    for(i32 i = 0; i < (i32)(nn); ++i) last[i] = 0;
    date = 0;
    for(i32 i = 0; i < (i32)(nn); ++i) fixed_stack[i] = {0,0};
    auto [new_score,new_len] = evaluate(0,k-2);
    score = new_score;
    len = new_len;
  }
  static_union_find<MAXNN> uf;
  int pathEnd[MAXNN];
  int pathLen[MAXNN];
  int reachTarget[MAXNN];
  int reachTargetLen[MAXNN];
  array<int,2> fixed_stack[MAXNN];
  array<int,2> stack[MAXNN];
  int Q[MAXNN];
  array<int,2> FR[MAXNN];
  int D1[MAXNN];
  int D2[MAXNN];
  array<int,2> evaluate(int itgt_from, int itgt_to) {
    // no cycles
    uf.reset(nn);
    for(i32 i = 0; i < (i32)(nn); ++i) if(int j = i+dxy[grid[i]]; grid[i]<4&&grid[j]<4) {
      if(uf.find(i) == uf.find(j)) return {INF,INF};
      uf.unite(i,j);
    }
    for(i32 i = 0; i < (i32)(nn); ++i) pathEnd[i] = -2;
    {
      auto dfs = [&](auto&& dfs, int i) {
        if(pathEnd[i] != -2) return;
        if(grid[i] != 4) {
          dfs(dfs, i+dxy[grid[i]]);
          pathEnd[i] = pathEnd[i+dxy[grid[i]]];
          pathLen[i] = 1+pathLen[i+dxy[grid[i]]];
        }else{
          pathEnd[i] = i;
          pathLen[i] = 0;
        }
      };
      for(i32 i = 0; i < (i32)(nn); ++i) dfs(dfs, i);
    }
    for(i32 i = 0; i < (i32)(nn); ++i) stack[i] = fixed_stack[i];
    int lscore = fixed_score, llen = fixed_len;
    for(i32 i = 0; i < (i32)(nn); ++i) if(grid[i] != 4) {
      auto& [x,c] = stack[i];
      if(x != grid[i]) { lscore += c; x = grid[i]; c = 0; }
    }
    for(i32 itgt = (itgt_from); itgt <= (i32)(itgt_to); ++itgt) {
      int x = targets[itgt], y = targets[itgt+1];
      date += 1;
      auto dfs = [&](auto&& dfs, int u, int len) {
        if(reachTarget[u] == date) return;
        reachTarget[u] = date;
        reachTargetLen[u] = len;
        for(i32 d = 0; d < (i32)(4); ++d) if((graph[u]&bit(d)) && grid[u+dxy[d]] == (d^1)) {
          dfs(dfs, u+dxy[d], len+1);
        }
      };
      dfs(dfs, y, 0);
      int found_at = -1;
      int iq = 0,nq = 0;
      if(reachTarget[x] == date) {
        while(x != y) {
          llen += 1;
          x = x+dxy[grid[x]];
        }
        found_at = -1;
        goto l_found;
      }
      llen += pathLen[x];
      x = pathEnd[x];
      Q[nq++] = x;
      D1[x] = 0; D2[x] = 0; last[x] = date; FR[x] = {-1,-1};
      while(iq < nq) {
        int u = Q[iq++];
        for(i32 d = 0; d < (i32)(4); ++d) if(graph[u]&bit(d)) {
          int w = u+dxy[d];
          int v = pathEnd[w];
          if(reachTarget[w] == date) {
            found_at = w;
            FR[w] = {u,d};
            llen += D2[u]+1+reachTargetLen[w];
            goto l_found;
          }
          if(last[v] != date) {
            last[v] = date;
            D1[v] = D1[u]+1;
            D2[v] = D2[u]+1+pathLen[w];
            FR[v] = {u,d};
            Q[nq++] = v;
            if(v == y) {
              found_at = y;
              llen += D2[v];
              goto l_found;
            }
          }else if(D1[v] == D1[u]+1 && D2[u]+1+pathLen[w] < D2[v]) {
            D2[v] = D2[u]+1+pathLen[w];
            FR[v] = {u,d};
          }
        }
      }
      return {INF,INF};
    l_found:;
      int ntmp = 0;
      static array<int,2> tmp[MAXNN];
      for(int z = found_at; z != -1 && z != x; z = get<0>(FR[z])) {
        tmp[ntmp++] = FR[z];
      }
      while(ntmp) {
        auto [u,d] = tmp[--ntmp];
        auto& [x,c] = stack[u];
        if(x != d) { lscore += c; x = d; c = 0; }
        c += 1;
      }
      if(llen > t) return {INF,INF};
    }
    return {lscore,llen};
  }
  vector<int> get_path(int itgt) {
    for(i32 i = 0; i < (i32)(nn); ++i) pathEnd[i] = -2;
    {
      auto dfs = [&](auto&& dfs, int i) {
        if(pathEnd[i] != -2) return;
        if(grid[i] != 4) {
          dfs(dfs, i+dxy[grid[i]]);
          pathEnd[i] = pathEnd[i+dxy[grid[i]]];
          pathLen[i] = 1+pathLen[i+dxy[grid[i]]];
        }else{
          pathEnd[i] = i;
          pathLen[i] = 0;
        }
      };
      for(i32 i = 0; i < (i32)(nn); ++i) dfs(dfs, i);
    }
    int x = targets[itgt], y = targets[itgt+1];
    date += 1;
    auto dfs = [&](auto&& dfs, int u, int len) {
      if(reachTarget[u] == date) return;
      reachTarget[u] = date;
      reachTargetLen[u] = len;
      for(i32 d = 0; d < (i32)(4); ++d) if((graph[u]&bit(d)) && grid[u+dxy[d]] == (d^1)) {
        dfs(dfs, u+dxy[d], len+1);
      }
    };
    dfs(dfs, y, 0);
    int found_at = -1;
    int iq = 0, nq = 0;
    if(reachTarget[x] == date) { found_at = -1; goto l_found; }
    x = pathEnd[x];
    Q[nq++] = x;
    D1[x] = 0; D2[x] = 0; last[x] = date; FR[x] = {-1,-1};
    while(iq < nq) {
      int u = Q[iq++];
#pragma clang loop unroll_count(4)
      for(i32 d = 0; d < (i32)(4); ++d) if(graph[u]&bit(d)) {
        int w = u+dxy[d];
        int v = pathEnd[w];
        if(reachTarget[u+dxy[d]] == date) {
          FR[u+dxy[d]] = {u,d};
          found_at = u+dxy[d];
          goto l_found;
        }
        if(last[v] != date) {
          last[v] = date;
          FR[v] = {u,d};
          D1[v] = D1[u]+1;
          D2[v] = D2[u]+1+pathLen[w];
          Q[nq++] = v;
          if(v == y) {
            found_at = v;
            goto l_found;
          }
        }
        else if(D1[v] == D1[u]+1 && D2[u]+1+pathLen[w] < D2[v]) {
          FR[v] = {u,d};
          D2[v] = D2[u]+1+pathLen[w];
        }
      }
    }
    do { throw runtime_error("main.cpp" ":" "381" " impossible"); } while(0);
  l_found:;
    vector<array<int,2>> P;
    for(int z = found_at; z != -1 && z != x; z = get<0>(FR[z])) {
      P.push_back({z, get<1>(FR[z])});
    }
    reverse(begin(P), end(P));
    vector<int> path;
    x = targets[itgt];
    y = targets[itgt+1];
    while(x != y && grid[x] != 4) {
      path.push_back(grid[x]);
      x += dxy[grid[x]];
    }
    for(auto [z,d] : P) {
      path.push_back(d);
      x = x+dxy[d];
      while(x != y && grid[x] != 4) {
        path.push_back(grid[x]);
        x += dxy[grid[x]];
      }
    }
    return path;
  }
  void transition1(i32 itgt_from, f32 temp) {
    int i = rng.random32(nn);
    int d = rng.random32(5);
    if(grid[i] == d) return;
    if(fixed_grid[i]) return;
    if(d < 4 && !(graph[i]&bit(d))) return;
    swap(grid[i], d);
    int limit = floor(score + rng.randomDouble() * temp);
    auto [new_score,new_len] = evaluate(itgt_from,k-2);
    if(new_score <= limit) {
      score = new_score;
      len = new_len;
    }else{
      swap(grid[i], d);
    }
  }
  void transition_grid(i32 itgt_from, f32 temp, int X, int Y) {
    int i = rng.random32(nn);
    if(i/n+X-1 >= n || i%n+Y-1 >= n) return;
    static int d[20][20];
    bool any_change = false;
    for(i32 x = 0; x < (i32)(X); ++x) for(i32 y = 0; y < (i32)(Y); ++y) {
      int u = i+x*n+y;
      if(fixed_grid[u]) {
        d[x][y] = grid[u];
      } else {
        do {
          d[x][y] = rng.random32(5);
        } while(d[x][y] < 4 && !(graph[u]&bit(d[x][y])));
      }
      if(d[x][y] != grid[u]) any_change = 1;
    }
    if(!any_change) return;
    for(i32 x = 0; x < (i32)(X); ++x) for(i32 y = 0; y < (i32)(Y); ++y) swap(grid[i+x*n+y], d[x][y]);
    int limit = floor(score + rng.randomDouble() * temp);
    auto [new_score,new_len] = evaluate(itgt_from,k-2);
    if(new_score <= limit) {
      score = new_score;
      len = new_len;
    }else{
      for(i32 x = 0; x < (i32)(X); ++x) for(i32 y = 0; y < (i32)(Y); ++y) swap(grid[i+x*n+y], d[x][y]);
    }
  }
  void transition(i32 itgt_from, f32 temp) {
    int ty = rng.random32(30);
    if(ty < 6) transition1(itgt_from, temp);
    else if(ty < 18) transition_grid(itgt_from, temp, 2,2);
    else if(ty < 30) transition_grid(itgt_from, temp, 3,3);
  }
  vector<int> solve(f32 base_tl, f32 next_tl) {
    vector<int> total_path;
    for(i32 itgt = 0; itgt < (i32)(k-1); ++itgt) {
      { auto [new_score, new_len] = evaluate(itgt, k-2);
        // debug(itgt, score,len,new_score,new_len);
        do { if(!(new_score == score)) { throw runtime_error("main.cpp" ":" "467" " Assertion failed: " "new_score == score"); } } while(0);
        do { if(!(new_len == len)) { throw runtime_error("main.cpp" ":" "468" " Assertion failed: " "new_len == len"); } } while(0);
      }
      i64 iter = 0;
      f32 temp0, temp1;
      if(itgt == 0) {
        temp0 = 12.0;
        temp1 = 0.9;
      }else{
        temp0 = 8.0;
        temp1 = 0.9;
      }
      int best = score;
      static int best_grid[MAXNN];
      for(i32 i = 0; i < (i32)(nn); ++i) best_grid[i] = grid[i];
      f32 temp = temp0;
      f32 t0 = TIMER.elapsed();
      f32 tl = itgt == 0
        ? max(1e-5f, base_tl - t0)
        : max(1e-5f, (next_tl - t0) / (k-1-itgt));
      do{}while(0);
      while(1) {
        iter++;
        if(iter % 8 == 0) {
          f32 done = (TIMER.elapsed() - t0) / tl;
          if(done > 1) break;
          temp = temp0*pow(temp1/temp0, done);
        }
        transition(itgt, temp);
        if(score < best) {
          best = score;
          for(i32 i = 0; i < (i32)(nn); ++i) best_grid[i] = grid[i];
        }
      }
      do{}while(0);
      if(itgt == 0) cerr << "iter=" << iter << endl;
      {
        for(i32 i = 0; i < (i32)(nn); ++i) grid[i] = best_grid[i];
        auto [new_score, new_len] = evaluate(itgt, k-2);
        score = new_score;
        len = new_len;
      }
      auto [new_score, new_len] = evaluate(itgt,itgt);
      fixed_score = new_score;
      fixed_len = new_len;
      auto path = get_path(itgt);
      int x = targets[itgt];
      for(int d : path) {
        do { if(!(graph[x]&bit(d))) { throw runtime_error("main.cpp" ":" "521" " Assertion failed: " "graph[x]&bit(d)"); } } while(0);
        if(grid[x] != 4) {
          fixed_grid[x] = 1;
        }
        x += dxy[d];
      }
      for(i32 i = 0; i < (i32)(nn); ++i) fixed_stack[i] = stack[i];
      total_path.insert(end(total_path), begin(path), end(path));
    }
    do { if(!((int)total_path.size() == len)) { throw runtime_error("main.cpp" ":" "530" " Assertion failed: " "(int)total_path.size() == len"); } } while(0);
    return total_path;
  }
};
struct beam_state {
  int nq, nc;
  int state[1<<15];
  int color[1<<15];
  int num_waiting[128][4];
  int num_invrules[128][4];
  array<int,2> invrules[128][128][4];
  // fast_queue<128*128> queue;
  fast_queue<128> queue_q[128];
  int date;
  int last[1<<15];
  int val_queue;
  int val_waiting;
  void reset(bool use_pair[4][4],
             int num_waiting_[17][4],
             int color_perm[17][128],
             int nq_, int nc_) {
    nq = nq_;
    nc = nc_;
    for(i32 i = 0; i < (i32)(nq); ++i) for(i32 j = 0; j < (i32)(17+nc); ++j) for(i32 d = 0; d < (i32)(4); ++d) invrules[i][j][d] = {-1,-1};
    val_queue = nq*nc;
    val_waiting = 0;
    for(i32 i = 0; i < (i32)(nc+17); ++i) for(i32 d = 0; d < (i32)(4); ++d) num_invrules[i][d] = 0;
    for(i32 i = 0; i < (i32)(nc); ++i) for(i32 d = 0; d < (i32)(4); ++d) num_waiting[i+17][d] = 0;
    for(i32 i = 0; i < (i32)(17); ++i) for(i32 d = 0; d < (i32)(4); ++d) num_waiting[i][d] = num_waiting_[i][d];
    for(i32 d1 = 0; d1 < (i32)(4); ++d1) for(i32 d2 = 0; d2 < (i32)(4); ++d2) if(use_pair[d1][d2]) {
      int c1 = d1*4+d2;
      int c2 = d2*4+d1;
      for(i32 iq = 0; iq < (i32)(nq); ++iq) {
        invrules[iq][c2][d1] = {color_perm[c1][iq], c1};
        inc_num_invrules(c2,d1);
      }
    }
    // queue.clear();
    // FOR(i, nq) FOR(j, nc) queue.add(i*nc+j);
    for(i32 i = 0; i < (i32)(nq); ++i) queue_q[i].clear();
    for(i32 i = 0; i < (i32)(nq); ++i) for(i32 j = 0; j < (i32)(nc); ++j) queue_q[i].add(j);
    for(i32 c = 0; c < (i32)(17); ++c) color[c] = c;
    state[0] = 0;
    date = 0;
  }
  template<class F>
  __attribute__((always_inline)) inline
  void iter_moves(int i, int perm[128], int nperm[128],
                  array<int,3> t, array<int,3> nt, F&& f) {
    auto [next_color_id,d,nd] = t;
    int next_state = perm[state[i]];
    int next_color = color[next_color_id];
    auto [riq,ric] = invrules[next_state][next_color][d];
    if(riq != -1) {
      f(0);
    }else {
      auto [next_color_id2,d2,nd2] = nt;
      bool found = false;
      if(next_color_id2 != -1) {
        int next_color2 = color[next_color_id2];
        for(i32 iq = 0; iq < (i32)(nq); ++iq) if(queue_q[iq].nqueue && invrules[nperm[iq]][next_color2][d2][0] != -1) {
          found = 1;
          for(i32 id = 0; id < (i32)(queue_q[iq].nqueue); ++id) {
            f(1+iq*nc+queue_q[iq].queue[id]);
          }
        }
      }
      if(!found && val_queue > 0) {
        date += 1;
        // Select iq among list of iqs
        int iq;
        do { iq = rng.random32(nq); } while(!queue_q[iq].nqueue);
        for(i32 id = 0; id < (i32)(queue_q[iq].nqueue); ++id) {
          f(1+iq*nc+queue_q[iq].queue[id]);
        }
        int iq2 = rng.random32(nq);
        if(iq2 != iq) {
          for(i32 id = 0; id < (i32)(queue_q[iq2].nqueue); ++id) {
            f(1+iq2*nc+queue_q[iq2].queue[id]);
          }
        }
      }
    }
  }
  void inc_num_invrules(int c, int d) {
    num_invrules[c][d] += 1;
    val_waiting += num_waiting[c][d];
  }
  void dec_num_invrules(int c, int d) {
    num_invrules[c][d] -= 1;
    val_waiting -= num_waiting[c][d];
  }
  void inc_num_waiting(int c, int d) {
    num_waiting[c][d] += 1;
    val_waiting += num_invrules[c][d];
  }
  void dec_num_waiting(int c, int d) {
    num_waiting[c][d] -= 1;
    val_waiting -= num_invrules[c][d];
  }
  __attribute__((always_inline)) inline
  void do_move_noqueue(int i, int perm[128], array<int,3> t, int m) {
    auto [next_color_id,d,nd] = t;
    int next_state = perm[state[i]];
    int next_color = color[next_color_id];
    if(m == 0) {
      auto [riq,ric] = invrules[next_state][next_color][d];
      color[i+17] = ric;
      state[i+1] = riq;
    }else{
      val_queue -= 1;
      m -= 1;
      int iq = m/nc, ic = m%nc;
      ic += 17;
      color[i+17] = ic;
      state[i+1] = iq;
      invrules[next_state][next_color][d] = {iq,ic};
      inc_num_invrules(next_color, d);
    }
    dec_num_waiting(next_color, d);
    if(nd != -1) inc_num_waiting(color[i+17], nd);
  }
  __attribute__((always_inline)) inline
  void undo_move_noqueue(int i, int perm[128], array<int,3> t, int m) {
    auto [next_color_id,d,nd] = t;
    int next_state = perm[state[i]];
    int next_color = color[next_color_id];
    if(m == 0) {
    }else{
      val_queue += 1;
      m -= 1;
      int iq = m/nc, ic = m%nc;
      ic += 17;
      invrules[next_state][next_color][d] = {-1,-1};
      dec_num_invrules(next_color, d);
    }
    inc_num_waiting(next_color, d);
    if(nd != -1) dec_num_waiting(color[i+17], nd);
  }
  __attribute__((always_inline)) inline
  void do_move(int i, int perm[128], array<int,3> t, int m) {
    auto [next_color_id,d,nd] = t;
    int next_state = perm[state[i]];
    int next_color = color[next_color_id];
    if(m == 0) {
      auto [riq,ric] = invrules[next_state][next_color][d];
      color[i+17] = ric;
      state[i+1] = riq;
    }else{
      val_queue -= 1;
      m -= 1;
      int iq = m/nc, ic = m%nc;
      queue_q[iq].remove(ic);
      ic += 17;
      color[i+17] = ic;
      state[i+1] = iq;
      invrules[next_state][next_color][d] = {iq,ic};
      inc_num_invrules(next_color, d);
    }
    dec_num_waiting(next_color, d);
    if(nd != -1) inc_num_waiting(color[i+17], nd);
  }
  __attribute__((always_inline)) inline
  void undo_move(int i, int perm[128], array<int,3> t, int m) {
    auto [next_color_id,d,nd] = t;
    int next_state = perm[state[i]];
    int next_color = color[next_color_id];
    if(m == 0) {
    }else{
      val_queue += 1;
      m -= 1;
      int iq = m/nc, ic = m%nc;
      queue_q[iq].add(ic);
      ic += 17;
      invrules[next_state][next_color][d] = {-1,-1};
      dec_num_invrules(next_color, d);
    }
    inc_num_waiting(next_color, d);
    if(nd != -1) dec_num_waiting(color[i+17], nd);
  }
  i32 value() const {
    i32 v = val_queue * nq + val_waiting;
    return v;
  }
};
struct path_solver {
  int ntransitions;
  int additional_colors;
  bool use_pair[4][4];
  array<int, 3> transitions[1<<15];
  int transition_perm[1<<15][128];
  int color_index[MAXNN];
  int num_waiting[17][4];
  int color_perm[17][128];
  int initial_perm[128];
  beam_state S;
  vector<int> path;
  vector<array<int,2>> last0;
  void init(vector<int> const& path_) {
    path = path_;
    // Compute directions at every cell
    vector<vector<int>> stack(nn);
    { int x = targets[0];
      for(i32 i = 0; i < (i32)(path.size()-1); ++i) {
        int x = path[i], y = path[i+1];
        int d = 0; while(x+dxy[d] != y) d += 1;
        stack[x].push_back(d);
      }
      for(i32 i = 0; i < (i32)(nn); ++i) reverse(begin(stack[i]), end(stack[i]));
    }
    // Initialize ntransitions, additional_colors and use_pair
    { ntransitions = INF;
      additional_colors = 4;
      bool cur_use_pair[4][4];
      auto test_use_pair = [&](int w) {
        int cur_ntransitions = 0;
        for(i32 i = 0; i < (i32)(nn); ++i) {
          if(stack[i].size() >= 2) {
            int x = stack[i][0];
            int y = stack[i][1];
            if(!cur_use_pair[x][y]) y = x;
            cur_ntransitions += stack[i].size();
            for(int z : stack[i]) {
              if(z != x) break;
              cur_ntransitions -= 1;
              swap(x,y);
            }
          }
        }
        const f32 coeff = 0.46;
        f32 cur_value = 2*sqrt(cur_ntransitions) * coeff + w;
        f32 best_value = 2*sqrt(ntransitions) * coeff + additional_colors;
        if(cur_value < best_value) {
          ntransitions = cur_ntransitions;
          additional_colors = w;
          for(i32 d1 = 0; d1 < (i32)(4); ++d1) for(i32 d2 = 0; d2 < (i32)(4); ++d2) use_pair[d1][d2] = cur_use_pair[d1][d2];
        }
      };
      auto bt = [&](auto&& bt, int d1, int d2, int w) {
        if(d1 == d2) {
          cur_use_pair[d1][d2] = 1;
          d1 += 1;
          d2 = 0;
        }
        if(d1 == 4) {
          test_use_pair(w);
          return;
        }
        cur_use_pair[d1][d2] = cur_use_pair[d2][d1] = 0;
        bt(bt,d1,d2+1,w);
        cur_use_pair[d1][d2] = cur_use_pair[d2][d1] = 1;
        bt(bt,d1,d2+1,w+2);
      };
      bt(bt,0,0,4);
    }
    // Print the best expected configuration
    { f32 best_value = 2*sqrt(ntransitions) + additional_colors;
      string config;
      for(i32 d1 = 0; d1 < (i32)(4); ++d1) for(i32 d2 = 0; d2 < (i32)(d1+1); ++d2) if(use_pair[d1][d2]) {
        if(!config.empty()) config += ", ";
        config += dc[d1];
        config += dc[d2];
      }
      do{}while(0);
    }
    // Simulate the sequence
    last0.assign(nn, {-1,-1});
    for(i32 i = 0; i < (i32)(nn); ++i) {
      if(stack[i].size() >= 2) {
        int x = stack[i][0];
        int y = stack[i][1];
        if(!use_pair[x][y]) y = x;
        last0[i] = {x,y};
      }else if(stack[i].size() == 1) {
        int x = stack[i][0];
        last0[i] = {x,x};
      }
    }
  }
  void init_transitions(int nq) {
    auto last = last0;
    for(i32 i = 0; i < (i32)(nn); ++i) color_index[i] = -1;
    for(i32 i = 0; i < (i32)(17); ++i) for(i32 d = 0; d < (i32)(4); ++d) num_waiting[i][d] = 0;
    for(i32 i = 0; i < (i32)(17); ++i) {
      for(i32 j = 0; j < (i32)(nq); ++j) color_perm[i][j] = j;
      rng.shuffle(color_perm[i], color_perm[i]+nq);
    }
    int perm[128];
    for(i32 i = 0; i < (i32)(nq); ++i) perm[i] = i;
    int itransition = 0;
    for(i32 i = (path.size()-2); i >= (i32)(0); --i) {
      int x = path[i], y = path[i+1];
      int d = 0; while(x+dxy[d] != y) d += 1;
      if(last[x][0] == d) {
        color_index[x] = last[x][0]*4+last[x][1];
        for(i32 i = 0; i < (i32)(nq); ++i) perm[i] = color_perm[last[x][0]*4+last[x][1]][perm[i]];
        swap(last[x][0],last[x][1]);
      }else{
        last[x] = {-1,-1};
        int i = itransition++;
        if(color_index[x] >= 17) {
          transitions[color_index[x]-17][2] = d;
        }else{
          num_waiting[color_index[x]][d] += 1;
        }
        transitions[i] = {color_index[x],d,-1};
        color_index[x] = 17+i;
        for(i32 j = 0; j < (i32)(nq); ++j) transition_perm[i][j] = perm[j];
        for(i32 j = 0; j < (i32)(nq); ++j) perm[j] = j;
      }
    }
    for(i32 j = 0; j < (i32)(nq); ++j) initial_perm[j] = perm[j];
    transitions[itransition] = {-1,-1,-1};
    do { if(!(itransition == ntransitions)) { throw runtime_error("main.cpp" ":" "885" " Assertion failed: " "itransition == ntransitions"); } } while(0);
  }
  bool beam_search(int nq, int nc, int bw) {
    init_transitions(nq);
    S.reset(use_pair,
            num_waiting,
            color_perm,
            nq, nc);
    const int max_score = nq*nc;
    static i32 histogram[1<<22];
    euler_tour tour_current, tour_next;
    tour_current.ptr = tour0;
    tour_current.sz = 0;
    tour_next.ptr = tour1;
    tour_next.sz = 0;
    tour_current.push(0);
    i32 cutoff = 0;
    float cutoff_keep_probability = 1.0;
    for(i32 istep = 0; istep < (i32)(ntransitions); ++istep) {
      if(TIMER.elapsed() > TL_BEAM) return false;
      int lbw = bw;
      if(TIMER.elapsed() > TL_PANIC) lbw /= 2;
      if(istep > 1 && tour_current.sz == 1) break;
      i32 lowv, highv;
      int nhistogram = 0;
      traverse_euler_tour
        (istep,
         tour_current, tour_next,
         histogram, nhistogram,
         cutoff, cutoff_keep_probability,
         lowv, highv);
      if(nhistogram > lbw) {
        nth_element(histogram,
                    histogram+lbw,
                    histogram+nhistogram,
                    greater<i32>());
        int i = lbw, j = lbw;
        while(i >= 0 && histogram[i] == histogram[lbw]) i -= 1;
        while(j < nhistogram && histogram[j] == histogram[lbw]) j += 1;
        int count = j-i-1;
        cutoff = histogram[lbw];
        cutoff_keep_probability = (float)(lbw+1-i) / count;
      }else{
        cutoff = 0;
        cutoff_keep_probability = 1.0;
      }
      // debug(istep, tour_next.sz, lowv, highv, cutoff, cutoff_keep_probability);
      swap(tour_current, tour_next);
      tour_next.sz = 0;
    }
    return get_euler_tour_solution(ntransitions, tour_current);
  }
  void traverse_euler_tour
  (i32 istep,
   euler_tour const& tour_current,
   euler_tour& tour_next,
   i32* histogram, i32& nhistogram,
   i32 cutoff, float cutoff_keep_probability,
   i32& lowv, i32& highv)
  {
    lowv = -1<<30;
    highv = 1<<30;
    static i32 stack_moves[10'000];
    i32 nstack_moves = 0;
    int ncommit = 0;
    f32 curp = 1.0;
    auto accept_at_cutoff = [&]() __attribute__((always_inline)) -> bool {
      curp += cutoff_keep_probability;
      if(curp > 1.0) { curp -= 1; return 1; }
      return 0;
    };
    for(i32 iedge = 0; iedge < (i32)((i32)tour_current.sz); ++iedge) {
      auto const& edge = tour_current.ptr[iedge];
      if(edge != 0) {
        S.do_move(nstack_moves, transition_perm[nstack_moves],
                  transitions[nstack_moves], edge-1);
        stack_moves[nstack_moves++] = edge-1;
      }else{
        if(nstack_moves == istep) {
          i32 svalue = S.value();
          if(svalue > cutoff || (svalue == cutoff && accept_at_cutoff()))
            {
              while(ncommit < nstack_moves) {
                tour_next.push(stack_moves[ncommit]+1);
                ncommit += 1;
              }
              S.iter_moves
                (nstack_moves, transition_perm[nstack_moves],
                 transition_perm[nstack_moves+1],
                 transitions[nstack_moves], transitions[nstack_moves+1],
                 [&](int d)
                 {
                   S.do_move_noqueue
                     (nstack_moves, transition_perm[nstack_moves],
                      transitions[nstack_moves], d);
                   i32 v = S.value();
                   lowv = min(lowv, v);
                   highv = max(highv, v);
                   histogram[nhistogram++] = v;
                   tour_next.push(d+1);
                   tour_next.push(0);
                   S.undo_move_noqueue
                     (nstack_moves, transition_perm[nstack_moves],
                      transitions[nstack_moves], d);
                 });
            }
        }
        if(nstack_moves == 0) {
          tour_next.push(0);
          return;
        }
        if(ncommit == nstack_moves) {
          tour_next.push(0);
          ncommit -= 1;
        }
        nstack_moves -= 1;
        S.undo_move(nstack_moves, transition_perm[nstack_moves],
                    transitions[nstack_moves], stack_moves[nstack_moves]);
      }
    }
    do { throw runtime_error("main.cpp" ":" "1032" " impossible"); } while(0);
  }
  bool get_euler_tour_solution
  (i32 istep,
   euler_tour const& tour_current)
  {
    static i32 stack_moves[10'000];
    i32 nstack_moves = 0;
    for(i32 iedge = 0; iedge < (i32)((i32)tour_current.sz); ++iedge) {
      auto const& edge = tour_current.ptr[iedge];
      if(edge != 0) {
        S.do_move
          (nstack_moves, transition_perm[nstack_moves],
           transitions[nstack_moves], edge-1);
        stack_moves[nstack_moves++] = edge-1;
      }else{
        if(nstack_moves == istep) {
          return true;
        }
        if(nstack_moves == 0) {
          return false;
        }
        nstack_moves -= 1;
        S.undo_move
          (nstack_moves, transition_perm[nstack_moves],
           transitions[nstack_moves], stack_moves[nstack_moves]);
      }
    }
    do { throw runtime_error("main.cpp" ":" "1067" " impossible"); } while(0);
  }
};
struct solution {
  int nq, nc;
  vector<array<int,5>> rules;
  vector<int> color;
  int initial_state;
  void print() {
    for(i32 i = 0; i < (i32)(nn); ++i) if(color[i] == -1) color[i] = color[targets[0]];
    vector<int> state_perm(nq); iota(begin(state_perm), end(state_perm),0);
    if(initial_state != 0) {
      swap(state_perm[initial_state], state_perm[0]);
    }
    vector<int> used_color(nc, 0);
    for(i32 i = 0; i < (i32)(nn); ++i) used_color[color[i]] = 1;
    for(auto [iq,ic,jq,jc,d] : rules) {
      used_color[ic] = 1;
    }
    vector<int> color_perm(nc, -1);
    int nc2 = 0;
    for(i32 i = 0; i < (i32)(nc); ++i) if(used_color[i]) color_perm[i] = nc2++;
    cout << nc2 << " " << nq << " " << rules.size() << endl;
    for(i32 i = 0; i < (i32)(n); ++i) {
      for(i32 j = 0; j < (i32)(n); ++j) cout << color_perm[color[i*n+j]] << ' ';
      cout << endl;
    }
    for(auto [iq,ic,jq,jc,d] : rules) {
      cout << color_perm[ic] << ' ' << state_perm[iq] << ' '
           << color_perm[jc] << ' ' << state_perm[jq] << ' ' << dc[d] << endl;
    }
  }
  bool check() const {
    auto color_copy = color;
    map<array<int,2>,array<int,3>> rule_map;
    for(auto [iq,ic,jq,jc,d] : rules) {
      if(rule_map.count({iq,ic})) {
        do{}while(0);
        return false;
      }
      rule_map[{iq,ic}] = {jq,jc,d};
    }
    int x = targets[0];
    int q = initial_state;
    int len = 0;
    int itgt = 0;
    if(x == targets[itgt]) itgt += 1;
    while(itgt < k) {
      int c = color_copy[x];
      if(!rule_map.count({q,c})) {
        do{}while(0);
        return false;
      }
      auto [jq,jc,d] = rule_map[{q,c}];
      if(!(graph[x]&bit(d))) {
        do{}while(0);
        return false;
      }
      q = jq;
      color_copy[x] = jc;
      x += dxy[d];
      len += 1;
      if(len > t) {
        do{}while(0);
        return false;
      }
      if(x == targets[itgt]) itgt += 1;
    }
    return true;
  }
};
void solve() {
  static sa_solver S;
  S.init();
  auto dpath = S.solve(TL_SA1, TL_SA2);
  do{}while(0);
  vector<int> path;
  int x = targets[0];
  path.push_back(x);
  for(int d : dpath) {
    x += dxy[d];
    path.push_back(x);
  }
  static path_solver P;
  P.init(path);
  solution BEST_SOL;
  int best = INF;
  const int MAXBW = 32'000;
  auto try_nqc = [&](int nq, int nc, int bw) {
    bool ok = P.beam_search(nq,nc, bw);
    if(ok && nq+nc < best) {
      best = nq+nc;
      auto const& S = P.S;
      solution SOL;
      SOL.nq = nq;
      SOL.nc = nc+17;
      for(i32 jq = 0; jq < (i32)(S.nq); ++jq) for(i32 jc = 0; jc < (i32)(S.nc+17); ++jc) for(i32 d = 0; d < (i32)(4); ++d) {
        auto [iq,ic] = S.invrules[jq][jc][d];
        if(iq != -1) {
          SOL.rules.push_back({iq,ic,jq,jc,d});
        }
      }
      SOL.color.resize(nn, 0);
      for(i32 i = 0; i < (i32)(nn); ++i) if(P.color_index[i] != -1) {
        SOL.color[i] = S.color[P.color_index[i]];
      }
      SOL.initial_state = P.initial_perm[S.state[P.ntransitions]];
      do{}while(0);
      bool solok = SOL.check();
      do { if(!(solok)) { throw runtime_error("main.cpp" ":" "1188" " Assertion failed: " "solok"); } } while(0);
      if(solok) {
        do{}while(0);
        BEST_SOL = SOL;
      }
    }
    return ok;
  };
  int bw = 2;
  int hi = max<int>(1,ceil(sqrt(P.ntransitions)));
  try_nqc(hi,hi,bw);
  int lo = max<int>(1, hi/2);
  while(lo != hi && TIMER.elapsed() < TL_BEAM) {
    int mi = (lo+hi)/2;
    if(try_nqc(mi,mi,bw)) {
      hi = mi;
    }else{
      lo = mi+1;
    }
  }
  int nq = lo-1, nc = lo-1;
  do{}while(0);
  while(nq >= 1 && nc >= 0 && bw <= MAXBW && TIMER.elapsed() < TL_BEAM) {
    do{}while(0);
    bool ok = try_nqc(nq, nc, bw);
    if(ok) {
      if(bw == 2 && nq > 10 && nc > 10) {
        nq -= 1;
        nc -= 1;
      }else{
        if(nq > nc) nq -= 1;
        else nc -= 1;
      }
    }else{
      bw *= 1.8;
      bw = min(bw, MAXBW);
    }
  }
  if(best < INF) {
    BEST_SOL.print();
  }
}
int main() {
  ios::sync_with_stdio(false);
  cerr << setprecision(5) << fixed;
  TIMER.reset();
  u64 seed = time(0);
  do{}while(0);
  rng.reset(seed);
  // history_pool = new history_list[NHISTORY];
  read();
  init();
  solve();
  do{}while(0);
  cerr << "[DATA] time = " << (i32)(TIMER.elapsed()*1000) << endl;
  return 0;
}
