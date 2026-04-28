#include <limits>
#include <queue>
#include <vector>

#include "Edge.cc"
#include "EdgeWeightedGraph.cc"

using namespace std;

class PrimMST {
  private:
    int n;
    vector<Edge> edgeTo;
    vector<double> distanceTo;
    vector<bool> visited;
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<>> pq;

  public:
    PrimMST(EdgeWeightedGraph &g) {
        this->n = g.V();
        edgeTo.resize(n);
        distanceTo.resize(n, numeric_limits<double>::infinity());
        visited.resize(n, false);

        for (int v = 0; v < n; ++v)
            if (!visited[v]) prim(g, v);
    }

    void prim(EdgeWeightedGraph &g, int s) {
        distanceTo[s] = 0.0;
        pq.push({0.0, s});

        while (!pq.empty()) {
            int v = pq.top().second;
            pq.pop();
            scan(g, v);
        }
    }

    void scan(EdgeWeightedGraph &g, int u) {
        visited[u] = true;
        for (Edge &e : g.adj(u)) {
            int v = e.other(u);

            if (visited[v]) continue;

            if (e.weight() < distanceTo[v]) {
                distanceTo[v] = e.weight();
                edgeTo[v] = e;
                pq.push({distanceTo[v], v});
            }
        }
    }

    vector<Edge> edges() {
        vector<Edge> mst;
        for (int v = 1; v < n; ++v)
            mst.push_back(edgeTo[v]);
        return mst;
    }

    double weight() {
        double w = 0.0;
        for (Edge &e : edges()) w += e.weight();
        return w;
    }
};

int main() {
    EdgeWeightedGraph g(5);
    g.addEdge(0, 1, 2.0);
    g.addEdge(0, 3, 6.0);
    g.addEdge(1, 2, 3.0);
    g.addEdge(1, 3, 8.0);
    g.addEdge(1, 4, 5.0);
    g.addEdge(2, 4, 7.0);
    g.addEdge(3, 4, 9.0);

    PrimMST mst(g);
    for (Edge &e : mst.edges())
        printf("%.0f ", e.weight());
    printf("\nTotal: %.0f\n", mst.weight()); // expected: 17
}
