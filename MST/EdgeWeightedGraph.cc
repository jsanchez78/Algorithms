#include <stdexcept>
#include <vector>

#include "Edge.cc"

class EdgeWeightedGraph {
  private:
    int n, E;
    std::vector<std::vector<Edge>> adjList;

  public:
    EdgeWeightedGraph(int V) {
        if (V < 0) throw std::invalid_argument("V must be non-negative");
        this->n = V;
        this->E = 0;
        adjList.resize(V);
    }

    int V() { return n; }

    void addEdge(int u, int v, double w) {
        Edge e(u, v, w);
        adjList[u].push_back(e);
        adjList[v].push_back(e);
        ++E;
    }

    std::vector<Edge> &adj(int v) { return adjList[v]; }
};
