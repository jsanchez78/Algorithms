#include <string>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <optional>

// =============================================================================
// REGISTER ALLOCATION via Graph Coloring (Chaitin-style)
//
// PSEUDOCODE OVERVIEW:
//
// RegisterAllocate(program, k):
//   repeat:
//     Phase 1: Liveness Analysis
//       for each instruction i (backwards):
//         live_in[i]  = (live_out[i] - defs[i]) ∪ uses[i]
//         live_out[i] = ∪ live_in[s] for each successor s of i
//
//     Phase 2: Build Interference Graph
//       for each instruction i:
//         for each pair (a, b) in live_out[i]:
//           add edge (a, b) to G
//
//     Phase 3: Coalesce (optional optimization)
//       for each copy instruction "a = b":
//         if a and b do not interfere:
//           merge a and b into one node (they can share a register)
//           eliminates the copy instruction entirely
//
//     Phase 4: Simplify + Spill Selection
//       while G not empty:
//         if ∃ node n with degree(n) < k:
//           push n onto stack
//           remove n from G                  ← guaranteed colorable later
//         else:
//           n = node with lowest spill_cost(n)
//           n.potential_spill = true
//           push n onto stack
//           remove n from G
//
//     Phase 5: Select (color/register assignment)
//       while stack not empty:
//         n = pop from stack
//         reinsert n into G
//         used = colors of neighbors of n in G
//         if ∃ color c not in used:
//           assign register c to n           ← colors = {0, 1, ..., k-1}
//         else:
//           spilled.append(n)
//
//     Phase 6: Rewrite if spills occurred
//       if spilled is empty: return          ← done
//       else:
//         for each v in spilled:
//           insert store after each def of v
//           insert load before each use of v
//           replace v with fresh temporaries
//         repeat whole process
// =============================================================================

using Reg   = std::string;
using Color = int; // 0..k-1 map to physical registers

// -----------------------------------------------------------------------------
// Write: a register definition (write). dead=true means this is the last def
//      before the variable goes out of scope.
// Use: a register use (read).     dead=true means this is the last use.
// -----------------------------------------------------------------------------
struct Write {
    Reg  reg;
    bool dead;
};

struct Use {
    Reg  reg;
    bool dead;
};

// -----------------------------------------------------------------------------
// Instruction: opcode + defs (written regs) + uses (read regs)
// frequency: estimated execution count (used for spill cost weighting)
// -----------------------------------------------------------------------------
struct Instruction {
    std::string      opcode;
    std::vector<Write> defs;
    std::vector<Use> uses;
    int              frequency = 1;
};

// -----------------------------------------------------------------------------
// Interference Graph: undirected adjacency list
// -----------------------------------------------------------------------------
struct Graph {
    std::map<Reg, std::set<Reg>> adj;

    void             add_node(const Reg& r);
    void             add_edge(const Reg& a, const Reg& b);
    void             remove_node(const Reg& r);
    void             rename_node(const Reg& from, const Reg& to); // for coalescing
    bool             contains_edge(const Reg& a, const Reg& b) const;
    std::set<Reg>    neighbors(const Reg& r) const;
    int              degree(const Reg& r) const;
    std::set<Reg>    nodes() const;
};

// -----------------------------------------------------------------------------
// Phase 1: Liveness Analysis
//   Backward pass: live_in[i] = (live_out[i] - defs[i]) ∪ uses[i]
//   Returns live_out set for each instruction index.
// -----------------------------------------------------------------------------
std::vector<std::set<Reg>> liveness_analysis(const std::vector<Instruction>& instructions);

// -----------------------------------------------------------------------------
// Phase 2: Build Interference Graph
//   For each instruction i, every pair of simultaneously live variables
//   gets an edge — they cannot share a register.
// -----------------------------------------------------------------------------
Graph build_interference_graph(
    const std::vector<Instruction>& instructions,
    const std::vector<std::set<Reg>>& live_out);

// -----------------------------------------------------------------------------
// Phase 3: Coalesce
//   For each copy "a = b" where a and b don't interfere:
//     merge nodes, rewrite IL to eliminate the copy.
//   Reduces unnecessary register-to-register moves.
// -----------------------------------------------------------------------------
void coalesce(std::vector<Instruction>& instructions, Graph& g);

// -----------------------------------------------------------------------------
// Spill cost heuristic:
//   spill_cost(v) = (# uses + defs of v weighted by frequency) / live range length
//   Low cost → cheap to spill (rarely used over a long range).
// -----------------------------------------------------------------------------
std::map<Reg, float> estimate_spill_costs(const std::vector<Instruction>& instructions);

// -----------------------------------------------------------------------------
// Phase 4: Simplify + Spill Selection
//   Repeatedly remove nodes with degree < k onto stack (guaranteed colorable).
//   When none exist, pick lowest spill cost node as potential spill.
//   Returns stack of (reg, potential_spill) pairs.
// -----------------------------------------------------------------------------
std::stack<std::pair<Reg, bool>> simplify(
    Graph g,
    const std::map<Reg, float>& spill_cost,
    int k);

// -----------------------------------------------------------------------------
// Phase 5: Select — assign colors (registers) from the stack
//   Pop each node, reinsert, assign lowest color not used by neighbors.
//   If no color available → real spill.
//   colors = {0, 1, ..., k-1}  (k = number of physical registers)
// -----------------------------------------------------------------------------
std::pair<std::map<Reg, Color>, std::set<Reg>> select_colors(
    Graph& g,
    std::stack<std::pair<Reg, bool>> stack,
    int k);

// -----------------------------------------------------------------------------
// Phase 6: Rewrite — insert spill code for real spills
//   For each spilled variable v:
//     insert store (spill)  after each def  of v
//     insert load  (reload) before each use of v
//     replace v with fresh temporaries to shorten live range
// -----------------------------------------------------------------------------
std::vector<Instruction> insert_spill_code(
    const std::vector<Instruction>& instructions,
    const std::set<Reg>& spilled);

// -----------------------------------------------------------------------------
// Top-level driver
//   Repeats phases 1-6 until no spills remain.
//   Returns mapping of symbolic register → physical register (color).
// -----------------------------------------------------------------------------
std::map<Reg, Color> register_allocate(std::vector<Instruction>& instructions, int k);

int main()
{
    return 0;
}
