Objective
- Solve AHC059 A (Stack to Match Pairs) with a high-score approach based on:
  (1) an ordered rooted tree that enforces LIFO-friendly nesting,
  (2) per-node orientation (which of the two card cells is visited on entry),
  (3) deterministic shortest-path tie-breaking so the final action string is uniquely generated,
  (4) simulated annealing (SA) over tree order/orientation (and optionally structure).

Key Concept: Nested Intervals = Stack-Friendly
- Each value v appears twice at positions A[v], B[v].
- If the “open” (first Z) and “close” (second Z) of values are properly nested (no crossings),
  then the stack behaves like a DFS call stack: open on preorder, close on postorder.
- Use an ordered rooted tree over values as the nesting plan:
    - preorder visit => take the 1st copy of v (Z)
    - postorder exit => take the 2nd copy of v (Z)
  This guarantees LIFO alignment by construction.

State Representation (Plan)
- Ordered rooted tree over nodes = values v (0..199):
    parent[v], and children[v] is an ordered list.
    Choose a root (or a dummy root containing all nodes).
- Orientation bit per node:
    orient[v] ∈ {0,1}
    If orient[v]=0: preorder takes A[v], postorder takes B[v]
    If orient[v]=1: preorder takes B[v], postorder takes A[v]
- Deterministic shortest path rule (tie-break):
    e.g., always move vertically first (U/D), then horizontally (L/R).
  This makes the move sequence deterministic given any (from,to).

Compilation: Plan -> Concrete Action String (Deterministic)
Input: ordered tree + orient[] + tie-break rule.
Process:
1) Do a DFS over the ordered tree.
2) On preorder of v:
    target = (orient[v]==0 ? A[v] : B[v])
    move current position to target by deterministic Manhattan shortest path
    output 'Z'
3) Recurse children in their stored order.
4) On postorder of v:
    target = (orient[v]==0 ? B[v] : A[v])
    move to target with the same deterministic shortest path
    output 'Z' (this closes v; top two match -> vanish)
5) Ensure total ops <= 16000 (moves + Z + possible X); Z is ~400.

Notes:
- The DFS (pre/post) defines the Z-order uniquely if the tree is ordered.
- The action string becomes unique because:
    (i) ordered children => unique traversal order,
    (ii) orient[v] => unique choice of which copy is visited in preorder,
    (iii) fixed tie-break => unique UDLR path between any two cells.

Simulated Annealing (Main Optimizer)
Goal: minimize move count K (UDLR only). Z/X do not affect score.
Evaluate a plan by compiling it and counting moves, or by a faster cost model + occasional full validation.

Neighborhood Moves (Must-have)
1) Orientation Flip (per-node):
    - Pick v, set orient[v] ^= 1
    - Keeps nesting valid; often reduces travel by changing entry/exit endpoints.

2) Sibling Swap (local reorder):
    - Pick a parent p and two indices i<j, swap children[p][i], children[p][j]
    - Alters preorder/postorder sequences and thus travel cost.

3) Sibling Segment Reverse / 2-opt on siblings:
    - Pick parent p and segment [l..r], reverse children[p][l..r]
    - Stronger than swaps; helps escape local minima.

Optional but powerful (structure-changing)
4) Graft / Relocate subtree:
    - Cut a subtree rooted at x and reattach it as a child of some y (or another position among y’s children)
    - Requires cycle checks; changes nesting structure globally.
    - Use sparingly unless you implement fast delta evaluation.

Evaluation / Speed
- Naive: compile full action string and count moves every time (works but limits SA iterations).
- Better: maintain an Euler tour list of visitation events and update locally for swap/reverse,
  then compute delta move cost by only recomputing affected adjacent edges.
  (Preorder/postorder events produce a sequence of targets; move cost is sum of dist between consecutive targets.)
- Dist is Manhattan: dist((x1,y1),(x2,y2)) = |x1-x2|+|y1-y2|.

Practical evaluation approach:
- Represent the compiled “target sequence” as an array T of positions:
    T = [pre(v1), pre(v2), ..., post(v2), ..., post(v1), ...] following DFS.
- Move cost K = sum dist(T[i], T[i+1]) over i.
- For local changes (flip or swap/reverse within a parent), only a small contiguous portion of T changes,
  so update K by subtracting old boundary edges and adding new ones.
- Keep the best plan found; periodically recompile and validate.

Initialization (Important)
- Build an initial ordered tree that matches geometry:
  - Option 1: hierarchical clustering of values by center[v] = (A[v]+B[v])/2 to create a nested grouping tree.
  - Option 2: region-based partition (e.g., 4x4 blocks), group values by region and build a region hierarchy,
    then place values as leaves under region nodes.
- Initialize children order by a simple nearest-neighbor heuristic using representative points (A/B centers).
- Initialize orient[v] by choosing the preorder endpoint closer to the expected entry position (greedy).

Deterministic Path (UDLR tie-break)
- Implement move(from,to) as:
    if from.x < to.x: output 'D' (to.x-from.x times)
    else output 'U'
    then if from.y < to.y: output 'R'
    else output 'L'
  (Or any fixed priority order you choose, but keep it consistent.)

Why This Works
- Proper nesting (tree DFS) guarantees stack compatibility (LIFO) without needing complex runtime stack management.
- SA explores:
  - “which endpoint first” (orient flip)
  - “in which sibling order” (swap/reverse)
  - optionally “which subtree under which parent” (graft)
- Deterministic compilation makes evaluation stable and reproducible.

Extensions (If Needed for Top-tier)
- Add a light penalty term during SA to discourage huge travel or undesirable patterns, but primary objective is K.
- After SA, optionally add a post-processing stage with very local X-parking:
  - Keep 1–2 nearby empty cells; if a desired close is blocked by stack top, temporarily X-place the top card locally,
    perform the close with Z, then recover.
  - Keep this strictly local to avoid increasing moves.
  (Implement as a second-stage improvement; do NOT include X as a main SA variable to avoid state explosion.)

Implementation Deliverables
- Data parsing and locating A[v], B[v].
- Ordered rooted tree structure + orient[].
- Compiler: tree DFS -> target sequence -> action string.
- SA loop with time control (2 seconds) and neighborhoods (flip, swap, reverse; optional graft).
- Fast delta evaluation on the target sequence (recommended).
- Final validation by simulation (ensure all cards removed and ops <= 16000).
