#include <iostream>
#include <vector>
#include <algorithm>
#include "EdgeSimple.h"
#include "../union-find/UnionFind.cc"

using namespace std;

vector<Edge> kruskals(int n, vector<Edge>& edges) {
    UnionFind uf(n);
    vector<Edge> mst;
    
    sort(edges.begin(), edges.end());
    
    for (const Edge& e : edges) {
        if (!uf.Connected(e.u, e.v)) {
            uf.Union(e.u, e.v);
            mst.push_back(e);
        }
    }
    
    return mst;
}

int main() {
    vector<Edge> edges = {
        {0, 1, 2}, {0, 3, 6}, {1, 2, 3}, {1, 3, 8}, 
        {1, 4, 5}, {2, 4, 7}, {3, 4, 9}
    };
    
    vector<Edge> mst = kruskals(5, edges);
    
    int total = 0;
    for (const Edge& e : mst) {
        cout << e.w << " ";
        total += e.w;
    }
    cout << "\nTotal: " << total << endl;
    
    return 0;
}
