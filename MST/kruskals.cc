#include <stdio.h>

using namespace std;

struct Graph {
    int V, E;
    vector<pair<int, pair<int, int>> edges;

    Graph(int V, int E){
        this->V = V;
        this->E = E;
    }

    void addEdge(int u, int w, int v){
        edges.push_back({w, {u, v}});
    }

    int KruskalMST();
};

int Graph::KruskalMST(){
    return 0;
}
