#include <limits.h>

#define INF INT_MAX
#define MAX_V 32

static int dist[MAX_V][MAX_V];
static int V;

// Current step state for visualization
static int cur_k, cur_i, cur_j;
static int done;

extern "C" {

void init(int n) {
    V = n;
    done = 0;
    cur_k = cur_i = cur_j = 0;
    for (int i = 0; i < V; i++)
        for (int j = 0; j < V; j++)
            dist[i][j] = (i == j) ? 0 : INF;
}

void setEdge(int from, int to, int weight) {
    dist[from][to] = weight;
}

int getDist(int i, int j) {
    return dist[i][j];
}

int getV() { return V; }
int getK() { return cur_k; }
int getI() { return cur_i; }
int getJ() { return cur_j; }
int isDone() { return done; }

// Advance one relaxation step; returns 1 if a cell was updated
int step() {
    if (done) return 0;

    int updated = 0;
    int i = cur_i, j = cur_j, k = cur_k;

    if (dist[i][k] != INF && dist[k][j] != INF &&
        dist[i][k] + dist[k][j] < dist[i][j]) {
        dist[i][j] = dist[i][k] + dist[k][j];
        updated = 1;
    }

    // Advance indices
    if (++cur_j >= V) {
        cur_j = 0;
        if (++cur_i >= V) {
            cur_i = 0;
            if (++cur_k >= V)
                done = 1;
        }
    }
    return updated;
}

// Run all remaining steps at once
void runAll() {
    while (!done) step();
}

}
