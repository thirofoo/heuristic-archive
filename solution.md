Goal
- Solve AHC059 A (Stack to Match Pairs) with a high-score strategy that leverages the stack (LIFO) by creating nested “open/close” structure, and optimize that structure with Simulated Annealing (SA).
- Key idea: “stack accumulation + return-phase elimination” is equivalent to a DFS call stack, i.e., a tree / properly nested parentheses structure.

High-level Insight
- If you always clear a pair immediately (visit both copies consecutively), the stack stays empty and the problem becomes a pair-routing problem. This is a strong baseline but often not enough for top ranks.
- To beat it, you must intentionally “open” many cards (take the first copy) and “close” them later (take the second copy) in LIFO-compatible order. The best-case pattern is nested:
    open a
      open b
        close b
      close a
  This nesting matches LIFO and avoids stack pollution.

Representation (Tree as the Main Plan)
Option A (recommended first): Region Tree
- Partition the 20x20 board into small blocks (e.g., 4x4 or 5x5 regions).
- A node represents “enter a region, do all work you can inside, then exit”.
- Child nodes represent sub-regions (or sub-tours) visited while inside.
- This naturally enforces “deep then backtrack” behavior that matches LIFO.

Option B: Pair Tree
- Each node corresponds to a value v (a pair).
- The interval between taking the first copy and taking the second copy contains a subtree of other intervals.
- This directly models nested open/close, but can be harder to initialize robustly.

Plan Execution (producing an action string)
- Maintain current position (start at (0,0)).
- Follow the tree traversal:
    - When “opening” a task (region/pair), move to the chosen target cell(s) and apply Z to take the card.
    - While the card’s pair is not closed, traverse its children (nested work).
    - When “closing”, move to the second copy and apply Z; the top two match => they vanish.
- Moves are U/D/L/R along Manhattan shortest paths; Z is used to take a card.
- Use X sparingly as a “local parking” operation (see below).

Scoring Objective
- Minimize the number of move steps K (U/D/L/R). Z/X count toward operation limit but not score.
- Operation limit is large (2*N^3 = 16000 for N=20), so spending extra Z/X is OK if it reduces K.

Core Optimization: SA over Tree Structure
- Treat the tree as the main combinational object to optimize.
- Evaluate a tree by computing the total move cost of the resulting traversal, plus internal costs (e.g., within-region routing).
- Use SA moves (neighborhoods) that modify the tree while preserving feasibility.

Neighborhood Moves (Tree SA)
1) Sibling Swap / Rotate
- Within one parent node, swap two child subtrees or rotate a subsequence.

2) Subtree Graft (Cut & Paste)
- Cut a subtree and reattach it under another node (or different position among siblings).

3) Subtree Order Reversal
- Reverse the order of children within a node (changes return-phase ordering).

4) Local Orientation Flip (for nodes with two endpoints)
- If a node corresponds to an interval/pair or an “enter/exit” choice, flip which endpoint is used as entry/exit.

Evaluation Acceleration (critical)
- Cache a DP summary per subtree so that after a local change you only recompute affected parts.

Recommended DP pattern:
- Each subtree returns a small cost table “enter at state s_in -> minimal cost to exit at state s_out”.
- Keep the state space tiny (2–8 states) by defining a small set of candidate entry/exit points:
    - For a region node: a few boundary representative points (e.g., closest corner to parent entry, or 4 corners).
    - For a pair node: the two card positions (A,B), i.e., 2 states.
- Merging children is then a small DP convolution over sibling order.

Example: Pair-sequence DP (2-state)
- For a fixed sequence of pairs (v1..vM), compute best orientation (which copy first) with 2-state DP:
    dp[i][end_side] = minimal cost after finishing pair i, ending at copy side end_side.
- This can be reused as a building block inside nodes.

Initialization (build a reasonable starting tree)
Region Tree initialization (robust):
- Choose a scanning order of regions (snake / Hilbert-like).
- Inside each region:
    - Immediately clear “internal pairs” (both copies within the region) with short paths.
    - For cross-region values, “open” the first copy when encountered but postpone closing until later, aiming to keep opens nested by region traversal (DFS).
- Build the region hierarchy by recursively subdividing the board or by grouping adjacent regions visited consecutively.

Feasibility and Stack Discipline
- During execution, avoid “opening” too many unrelated values that cannot be closed soon, because they block the stack top.
- Prefer nested opens induced by DFS: open in outer node -> solve inner nodes -> close outer.

Add-on: Local Parking with X (small exception to pure nesting)
- Pure nesting can be too restrictive; top solutions often use X as a “parking lot”:
    - Maintain 2–6 empty cells near the current area as temporary storage.
    - If the stack top blocks a desired close, temporarily place the top card to a nearby empty cell (X),
      perform the intended Z close(s), then optionally recover.
- Since X doesn’t affect score, this can significantly reduce moves if parking is local.
- Implement as a post-processing or a second-stage local improvement:
    - Given a traversal, detect “blocked closes” and resolve them with minimal local park/unpark.

Annealing Schedule (practical)
- Time limit 2 sec; keep evaluation cheap.
- Use typical SA:
    - Start temperature so that ~50–80% of worse moves are accepted.
    - Cool down exponentially to near zero acceptance at the end.
- Always keep the best found tree and output its action sequence.

Deliverable Expectations
- Implement:
    1) Data parsing and positions of each value’s two cards.
    2) Region partitioning and initial tree builder.
    3) Tree DP evaluator with cached subtree summaries.
    4) SA loop with the above neighborhoods.
    5) Compiler from final tree traversal into move+Z(+optional X) action string.
- Start simple (region tree + sibling swaps + 4-corner entry/exit states), then add graft + parking.

Notes
- N is fixed at 20 in tests, so region sizes can be tuned freely.
- Ensure the operation count stays below 16000; moves should be a few thousand, Z is ~400 plus extra.
- Keep the stack mostly small and “clean” by enforcing nesting; use X only locally.
