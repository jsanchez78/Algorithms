import * as d3 from "d3";

const gap = 2, size = 30;
const cols = Math.floor(window.innerWidth / (size + gap));
const rows = Math.floor(window.innerHeight / (size + gap));

document.body.style.margin = "0";
document.body.style.overflow = "hidden";
document.body.style.background = "#111";

// --- Obstacles ---
const obstacles = new Set();

const vTip = { r: Math.floor(rows * 0.7), c: Math.floor(cols * 0.25) };
for (let i = 0; i < Math.floor(rows * 0.35); i++) {
  obstacles.add(`${vTip.r - i},${vTip.c - i}`);
  obstacles.add(`${vTip.r - i},${vTip.c + i}`);
}
const lineRow = Math.floor(rows * 0.5);
const lineStart = Math.floor(cols * 0.55);
const lineEnd = Math.floor(cols * 0.85);
for (let c = lineStart; c <= lineEnd; c++) obstacles.add(`${lineRow},${c}`);
const vLineCol = Math.floor(cols * 0.7);
const vLineStart = Math.floor(rows * 0.1);
const vLineEnd = Math.floor(rows * 0.4);
for (let r = vLineStart; r <= vLineEnd; r++) obstacles.add(`${r},${vLineCol}`);

// --- Start / End ---
const startR = Math.floor(rows * 0.5), startC = Math.floor(cols * 0.1);
const endR = Math.floor(rows * 0.5), endC = Math.floor(cols * 0.9);
const startKey = `${startR},${startC}`;
const endKey = `${endR},${endC}`;

// --- Build node list (non-obstacle cells) ---
const key = (r, c) => `${r},${c}`;
const nodes = [];
const nodeIndex = {};
for (let r = 0; r < rows; r++)
  for (let c = 0; c < cols; c++)
    if (!obstacles.has(key(r, c))) {
      nodeIndex[key(r, c)] = nodes.length;
      nodes.push({ r, c });
    }

const V = nodes.length;
const INF = 1e9;

// --- Floyd-Warshall dist + next matrices (flat arrays) ---
const dist = new Int32Array(V * V).fill(INF);
const next = new Int32Array(V * V).fill(-1);
for (let i = 0; i < V; i++) { dist[i * V + i] = 0; next[i * V + i] = i; }

// Set edges: 4-connected neighbors, weight 1
const dirs = [[-1,0],[1,0],[0,-1],[0,1]];
for (let idx = 0; idx < V; idx++) {
  const { r, c } = nodes[idx];
  for (const [dr, dc] of dirs) {
    const nr = r + dr, nc = c + dc;
    const nk = key(nr, nc);
    if (nk in nodeIndex) {
      const nIdx = nodeIndex[nk];
      dist[idx * V + nIdx] = 1;
      next[idx * V + nIdx] = nIdx;
    }
  }
}

// --- Step state (mirrors floyd-warshall-wasm.cc) ---
let cur_k = 0, cur_i = 0, cur_j = 0, done = false;

function step() {
  if (done) return 0;
  const ik = cur_i * V + cur_k;
  const kj = cur_k * V + cur_j;
  const ij = cur_i * V + cur_j;
  let updated = 0;
  if (dist[ik] < INF && dist[kj] < INF && dist[ik] + dist[kj] < dist[ij]) {
    dist[ij] = dist[ik] + dist[kj];
    next[ij] = next[ik];
    updated = 1;
  }
  if (++cur_j >= V) { cur_j = 0; if (++cur_i >= V) { cur_i = 0; if (++cur_k >= V) done = true; } }
  return updated;
}

// --- SVG ---
const svg = d3.select("#grid").append("svg")
  .attr("width", window.innerWidth)
  .attr("height", window.innerHeight);

const cellData = d3.cross(d3.range(rows), d3.range(cols), (r, c) => {
  const k = key(r, c);
  return { r, c, key: k, obstacle: obstacles.has(k), start: k === startKey, end: k === endKey };
});

const cellMap = {};
const rects = svg.selectAll("rect").data(cellData).join("rect")
  .attr("x", d => d.c * (size + gap))
  .attr("y", d => d.r * (size + gap))
  .attr("width", size)
  .attr("height", size)
  .attr("fill", d => d.start || d.end ? "limegreen" : d.obstacle ? "#888" : "#ddd")
  .each(function (d) { cellMap[d.key] = this; });

// --- Status text ---
const status = svg.append("text")
  .attr("x", 10).attr("y", 20)
  .attr("fill", "#fff").attr("font-size", "14px").attr("font-family", "monospace");

// --- Run button ---
const btnG = svg.append("g")
  .attr("transform", `translate(${window.innerWidth / 2 - 60}, ${window.innerHeight - 50})`)
  .style("cursor", "pointer");
btnG.append("rect").attr("width", 120).attr("height", 32).attr("rx", 4).attr("fill", "#333");
const btnText = btnG.append("text").attr("x", 60).attr("y", 21)
  .attr("text-anchor", "middle").attr("fill", "#fff").attr("font-size", "14px").attr("font-family", "monospace")
  .text("▶ Run");

let running = false;
btnG.on("click", () => {
  if (done || running) return;
  running = true;
  btnText.text("Running...");
  animate();
});

// --- Animation: batch steps per frame for speed ---
const STEPS_PER_FRAME = Math.max(1, Math.floor(V * V / 60));
let prevHighlights = [];

function animate() {
  if (done) { finish(); return; }

  // Clear previous highlights
  for (const el of prevHighlights) d3.select(el).attr("fill", "#ddd");
  prevHighlights = [];

  const highlights = new Set();
  for (let s = 0; s < STEPS_PER_FRAME && !done; s++) {
    // Highlight the k-node (intermediate)
    const kNode = nodes[cur_k];
    highlights.add(key(kNode.r, kNode.c));
    step();
  }

  // Color currently active intermediate nodes
  for (const k of highlights) {
    const el = cellMap[k];
    if (el) {
      const d = d3.select(el).datum();
      if (!d.start && !d.end && !d.obstacle) {
        d3.select(el).attr("fill", "#ffaa00");
        prevHighlights.push(el);
      }
    }
  }

  status.text(`Floyd-Warshall: k=${cur_k}/${V}  (${((cur_k / V) * 100).toFixed(1)}%)`);
  requestAnimationFrame(animate);
}

function finish() {
  // Clear highlights
  for (const el of prevHighlights) d3.select(el).attr("fill", "#ddd");

  // Reconstruct shortest path from start to end
  const si = nodeIndex[startKey], ei = nodeIndex[endKey];
  if (si === undefined || ei === undefined || dist[si * V + ei] >= INF) {
    status.text("No path found!");
    btnText.text("No path");
    return;
  }

  const path = [];
  let u = si;
  while (u !== ei) {
    path.push(u);
    u = next[u * V + ei];
    if (u === -1) break;
  }
  path.push(ei);

  // Animate path drawing
  path.forEach((idx, i) => {
    const n = nodes[idx];
    const k = key(n.r, n.c);
    setTimeout(() => {
      const el = cellMap[k];
      if (el) d3.select(el).attr("fill", "limegreen");
    }, i * 15);
  });

  status.text(`Shortest path: ${dist[si * V + ei]} steps`);
  btnText.text("Done ✓");
}
