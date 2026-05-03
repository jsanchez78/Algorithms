#include <limits.h>

#define INF INT_MAX
#define MAX_V 4096
#define MAX_EDGES 32768

static int V;
static int dist[MAX_V];
static int parent[MAX_V];
static int visited[MAX_V];

static int adj_to[MAX_EDGES];
static int adj_w[MAX_EDGES];
static int adj_head[MAX_V + 1];
static int edge_count;

static int cur_node, done, src, dst;

extern "C" {

void init(int n, int source, int destination) {
    V = n; src = source; dst = destination;
    done = 0; cur_node = -1; edge_count = 0;
    for (int i = 0; i <= V; i++) adj_head[i] = 0;
    for (int i = 0; i < V; i++) {
        dist[i] = INF; parent[i] = -1; visited[i] = 0;
    }
    dist[src] = 0;
}

void addEdge(int from, int to, int weight) {
    adj_to[edge_count] = to;
    adj_w[edge_count] = weight;
    edge_count++;
    // We rely on edges being added in node order (all of node 0, then node 1, etc.)
    // Just track the end for each node
    adj_head[from + 1] = edge_count;
}

void finalize() {
    // Forward-fill: if a node had no edges, its slot is still 0
    // but we need it to equal the previous node's end
    for (int i = 1; i <= V; i++)
        if (adj_head[i] < adj_head[i - 1])
            adj_head[i] = adj_head[i - 1];
}

int getDist(int i) { return dist[i]; }
int getParent(int i) { return parent[i]; }
int getCurNode() { return cur_node; }
int isDone() { return done; }
int getV() { return V; }

int step() {
    if (done) return -1;

    int u = -1;
    for (int i = 0; i < V; i++)
        if (!visited[i] && (u == -1 || dist[i] < dist[u]))
            u = i;

    if (u == -1 || dist[u] == INF) { done = 1; return -1; }

    visited[u] = 1;
    cur_node = u;

    for (int e = adj_head[u]; e < adj_head[u + 1]; e++) {
        int v = adj_to[e];
        int w = adj_w[e];
        if (dist[u] + w < dist[v]) {
            dist[v] = dist[u] + w;
            parent[v] = u;
        }
    }

    if (u == dst) done = 1;
    return u;
}

void runAll() { while (!done) step(); }

}
