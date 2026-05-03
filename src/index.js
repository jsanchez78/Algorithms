import * as d3 from "d3";

/* global Module */
Module.onRuntimeInitialized = function () {

const gap = 2, size = 30;
const cols = Math.floor(window.innerWidth / (size + gap)) - 1;
const rows = Math.floor(window.innerHeight / (size + gap)) - 1;
const key = (r, c) => `${r},${c}`;
const dirs = [[-1,0],[1,0],[0,-1],[0,1]];
const INF = 1e9;
const gridW = cols * (size + gap);
const gridH = rows * (size + gap);

document.body.style.margin = "0";
document.body.style.overflow = "hidden";
document.body.style.background = "#111";

const obstacles = new Set();
const startR = Math.floor(rows * 0.5), startC = Math.floor(cols * 0.1);
const endR = Math.floor(rows * 0.5), endC = Math.floor(cols * 0.9);
const startKey = `${startR},${startC}`;
let endKey = `${endR},${endC}`;

// --- SVG ---
const svg = d3.select("#grid").append("svg")
  .attr("width", gridW)
  .attr("height", gridH);

const cellData = d3.cross(d3.range(rows), d3.range(cols), (r, c) => {
  const k = key(r, c);
  return { r, c, key: k, start: k === startKey, end: k === endKey };
});

const cellMap = {};
svg.selectAll("rect").data(cellData).join("rect")
  .attr("x", d => d.c * (size + gap))
  .attr("y", d => d.r * (size + gap))
  .attr("width", size)
  .attr("height", size)
  .attr("fill", d => d.start || d.end ? "limegreen" : "#ddd")
  .each(function (d) { cellMap[d.key] = this; });

// --- Interaction ---
let painting = false, running = false;
svg.on("mousedown", (e) => {
  if (e.button === 2) { moveEnd(e); return; }
  painting = true; toggleObstacle(e);
});
svg.on("mousemove", (e) => { if (painting) toggleObstacle(e); });
svg.on("mouseup", () => { painting = false; });
svg.on("contextmenu", (e) => e.preventDefault());

function moveEnd(e) {
  if (running) return;
  const c = Math.floor(e.offsetX / (size + gap));
  const r = Math.floor(e.offsetY / (size + gap));
  const k = key(r, c);
  if (k === startKey || k === endKey || obstacles.has(k) || !cellMap[k]) return;
  d3.select(cellMap[endKey]).attr("fill", "#ddd");
  endKey = k;
  d3.select(cellMap[k]).attr("fill", "limegreen");
}

function toggleObstacle(e) {
  const c = Math.floor(e.offsetX / (size + gap));
  const r = Math.floor(e.offsetY / (size + gap));
  const k = key(r, c);
  if (k === startKey || k === endKey || running) return;
  const el = cellMap[k];
  if (!el) return;
  if (!obstacles.has(k)) { obstacles.add(k); d3.select(el).attr("fill", "#888"); }
  else { obstacles.delete(k); d3.select(el).attr("fill", "#ddd"); }
}

// --- Status ---
const status = svg.append("text")
  .attr("x", 10).attr("y", 20)
  .attr("fill", "#fff").attr("font-size", "14px").attr("font-family", "monospace")
  .text("LClick: walls | RClick: move end | then Run");

// --- Reset helper ---
function resetGrid() {
  running = false;
  obstacles.clear();
  for (const k in cellMap) {
    const el = cellMap[k];
    if (k === startKey || k === endKey) d3.select(el).attr("fill", "limegreen");
    else d3.select(el).attr("fill", "#ddd");
  }
  btnText.text("▶ Run");
  status.text("LClick: walls | RClick: move end | then Run");
}

// --- Bottom buttons row ---
const btnY = gridH - 50;

// Algorithm toggle
let algo = "dijkstra";
const algoG = svg.append("g")
  .attr("transform", `translate(${gridW / 2 - 230}, ${btnY})`)
  .style("cursor", "pointer");
algoG.append("rect").attr("width", 140).attr("height", 32).attr("rx", 4).attr("fill", "#444");
const algoText = algoG.append("text").attr("x", 70).attr("y", 21)
  .attr("text-anchor", "middle").attr("fill", "#0f0").attr("font-size", "13px").attr("font-family", "monospace")
  .text("[ Dijkstra ]");
algoG.on("click", () => {
  if (running) return;
  algo = algo === "dijkstra" ? "kruskal" : "dijkstra";
  algoText.text(algo === "dijkstra" ? "[ Dijkstra ]" : "[ Kruskal MST ]");
});

// Maze button
const mazeG = svg.append("g")
  .attr("transform", `translate(${gridW / 2 - 75}, ${btnY})`)
  .style("cursor", "pointer");
mazeG.append("rect").attr("width", 150).attr("height", 32).attr("rx", 4).attr("fill", "#555");
mazeG.append("text").attr("x", 75).attr("y", 21)
  .attr("text-anchor", "middle").attr("fill", "#fff").attr("font-size", "14px").attr("font-family", "monospace")
  .text("🔀 Gen Maze");
mazeG.on("click", () => {
  if (running) return;
  resetGrid();
  generateMaze();
});

// Run button
const btnG = svg.append("g")
  .attr("transform", `translate(${gridW / 2 + 90}, ${btnY})`)
  .style("cursor", "pointer");
btnG.append("rect").attr("width", 100).attr("height", 32).attr("rx", 4).attr("fill", "#333");
const btnText = btnG.append("text").attr("x", 50).attr("y", 21)
  .attr("text-anchor", "middle").attr("fill", "#fff").attr("font-size", "14px").attr("font-family", "monospace")
  .text("▶ Run");

// --- Maze generation via Kruskal's ---
function generateMaze() {
  // Use odd-sized cells as passages, even as walls
  // Maze cells: (r,c) where r and c are both odd (in a 2x scale grid)
  // Walls between them are even-row or even-col cells

  // Start by making everything a wall
  for (let r = 0; r < rows; r++)
    for (let c = 0; c < cols; c++) {
      const k = key(r, c);
      if (k === startKey || k === endKey) continue;
      obstacles.add(k);
      d3.select(cellMap[k]).attr("fill", "#888");
    }

  // Passage cells: odd r, odd c
  const passages = [];
  const passIdx = {};
  for (let r = 1; r < rows; r += 2)
    for (let c = 1; c < cols; c += 2) {
      const k = key(r, c);
      passIdx[k] = passages.length;
      passages.push({ r, c });
      obstacles.delete(k);
      if (k !== startKey && k !== endKey) d3.select(cellMap[k]).attr("fill", "#ddd");
    }

  // Union-Find over passage cells
  const N = passages.length;
  const par = new Int32Array(N);
  const rnk = new Int32Array(N);
  for (let i = 0; i < N; i++) par[i] = i;
  function find(x) { while (par[x] !== x) { par[x] = par[par[x]]; x = par[x]; } return x; }
  function union(a, b) {
    a = find(a); b = find(b);
    if (a === b) return false;
    if (rnk[a] < rnk[b]) { const t = a; a = b; b = t; }
    par[b] = a;
    if (rnk[a] === rnk[b]) rnk[a]++;
    return true;
  }

  // Edges between adjacent passage cells (2 apart), shuffled
  const walls = [];
  for (let i = 0; i < N; i++) {
    const { r, c } = passages[i];
    if (r + 2 < rows) { const k2 = key(r + 2, c); if (k2 in passIdx) walls.push({ a: i, b: passIdx[k2], wr: r + 1, wc: c }); }
    if (c + 2 < cols) { const k2 = key(r, c + 2); if (k2 in passIdx) walls.push({ a: i, b: passIdx[k2], wr: r, wc: c + 1 }); }
  }
  // Shuffle
  for (let i = walls.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [walls[i], walls[j]] = [walls[j], walls[i]];
  }

  // Kruskal's: knock down walls
  for (const { a, b, wr, wc } of walls) {
    if (union(a, b)) {
      const wk = key(wr, wc);
      obstacles.delete(wk);
      if (wk !== startKey && wk !== endKey) d3.select(cellMap[wk]).attr("fill", "#ddd");
    }
  }

  // Also clear start/end cells and their neighbors so they're reachable
  for (const sk of [startKey, endKey]) {
    obstacles.delete(sk);
    const [sr, sc] = sk.split(",").map(Number);
    for (const [dr, dc] of dirs) {
      const nk = key(sr + dr, sc + dc);
      if (cellMap[nk]) { obstacles.delete(nk); d3.select(cellMap[nk]).attr("fill", "#ddd"); }
    }
  }

  status.text("Maze generated! Hit Run");
}

// --- Shared graph building ---
let nodes, nodeIndex, V;

function buildGraph() {
  nodes = []; nodeIndex = {};
  for (let r = 0; r < rows; r++)
    for (let c = 0; c < cols; c++)
      if (!obstacles.has(key(r, c))) {
        nodeIndex[key(r, c)] = nodes.length;
        nodes.push({ r, c });
      }
  V = nodes.length;
}

// ===================== DIJKSTRA (WASM) =====================
const _init = Module.cwrap("init", null, ["number", "number", "number"]);
const _addEdge = Module.cwrap("addEdge", null, ["number", "number", "number"]);
const _finalize = Module.cwrap("finalize", null, []);
const _step = Module.cwrap("step", "number", []);
const _isDone = Module.cwrap("isDone", "number", []);
const _getDist = Module.cwrap("getDist", "number", ["number"]);
const _getParent = Module.cwrap("getParent", "number", ["number"]);

let dSteps = 0;

function runDijkstra() {
  const si = nodeIndex[startKey], ei = nodeIndex[endKey];
  _init(V, si, ei);
  for (let idx = 0; idx < V; idx++) {
    const { r, c } = nodes[idx];
    for (const [dr, dc] of dirs) {
      const nk = key(r + dr, c + dc);
      if (nk in nodeIndex) _addEdge(idx, nodeIndex[nk], 1);
    }
  }
  _finalize();
  dSteps = 0;

  function tick() {
    if (_isDone()) { finishDijkstra(si, ei); return; }
    for (let s = 0; s < 4 && !_isDone(); s++) {
      const u = _step(); dSteps++;
      if (u >= 0) colorVisited(u);
    }
    status.text(`Dijkstra: ${dSteps}/${V} nodes visited`);
    requestAnimationFrame(tick);
  }
  tick();
}

function finishDijkstra(si, ei) {
  const d = _getDist(ei);
  if (d >= INF) { status.text("No path found!"); btnText.text("No path"); return; }
  const path = [];
  for (let u = ei; u !== -1; u = _getParent(u)) path.push(u);
  path.reverse();
  path.forEach((idx, i) => setTimeout(() => colorPath(idx), i * 15));
  status.text(`Shortest path: ${d} steps`);
  btnText.text("Done ✓");
}

// ===================== KRUSKAL (JS) =====================
function runKruskal() {
  // Union-Find
  const parent = new Int32Array(V);
  const rank = new Int32Array(V);
  for (let i = 0; i < V; i++) parent[i] = i;
  function find(x) { while (parent[x] !== x) { parent[x] = parent[parent[x]]; x = parent[x]; } return x; }
  function union(a, b) {
    a = find(a); b = find(b);
    if (a === b) return false;
    if (rank[a] < rank[b]) { const t = a; a = b; b = t; }
    parent[b] = a;
    if (rank[a] === rank[b]) rank[a]++;
    return true;
  }

  // Build edges with random weights for visual interest
  const edges = [];
  for (let idx = 0; idx < V; idx++) {
    const { r, c } = nodes[idx];
    for (const [dr, dc] of dirs) {
      const nk = key(r + dr, c + dc);
      if (nk in nodeIndex) {
        const nIdx = nodeIndex[nk];
        if (nIdx > idx) edges.push({ u: idx, v: nIdx, w: Math.random() });
      }
    }
  }
  edges.sort((a, b) => a.w - b.w);

  // Color palette: assign each component a hue
  const compColor = {};
  let colorIdx = 0;
  function getColor(root) {
    if (!(root in compColor)) compColor[root] = d3.hsl(colorIdx++ * 137.5 % 360, 0.7, 0.55).formatHex();
    return compColor[root];
  }

  // Step through edges
  let ei = 0, mstCount = 0;
  const EDGES_PER_FRAME = 3;

  function tick() {
    if (mstCount >= V - 1 || ei >= edges.length) { finishKruskal(mstCount); return; }
    for (let s = 0; s < EDGES_PER_FRAME && ei < edges.length && mstCount < V - 1; s++, ei++) {
      const { u, v } = edges[ei];
      if (union(u, v)) {
        mstCount++;
        const root = find(u);
        const color = getColor(root);
        // Recolor both components to merged color
        for (let i = 0; i < V; i++) {
          if (find(i) === root) {
            const n = nodes[i];
            const k = key(n.r, n.c);
            if (k !== startKey && k !== endKey) {
              const el = cellMap[k];
              if (el) d3.select(el).attr("fill", color);
            }
          }
        }
      }
    }
    status.text(`Kruskal MST: ${mstCount}/${V - 1} edges`);
    requestAnimationFrame(tick);
  }
  tick();
}

function finishKruskal(count) {
  status.text(`Kruskal MST complete: ${count} edges`);
  btnText.text("Done ✓");
}

// ===================== HELPERS =====================
function colorVisited(u) {
  const n = nodes[u];
  const k = key(n.r, n.c);
  if (k !== startKey && k !== endKey) {
    const el = cellMap[k];
    if (el) d3.select(el).attr("fill", "#ffaa00");
  }
}

function colorPath(idx) {
  const n = nodes[idx];
  const el = cellMap[key(n.r, n.c)];
  if (el) d3.select(el).attr("fill", "limegreen");
}

// --- Run ---
btnG.on("click", () => {
  if (running) return;
  running = true;
  btnText.text("Running...");
  buildGraph();
  if (algo === "dijkstra") runDijkstra();
  else runKruskal();
});

}; // end Module.onRuntimeInitialized
