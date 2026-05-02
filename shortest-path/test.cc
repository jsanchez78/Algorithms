#include <iostream>
#include <vector>
using namespace std;

#include "dijkstras.cc"

int main() {
    // Test case: simple 4-node graph
    adjList.resize(4);
    adjList[0] = {{1, 1}, {2, 4}};
    adjList[1] = {{2, 2}, {3, 5}};
    adjList[2] = {{3, 1}};
    adjList[3] = {};

    vector<int> distance, parent;
    dijkstra(0, distance, parent);

    // Expected distances: [0, 1, 3, 4]
    vector<int> expected = {0, 1, 3, 4};
    
    for (int i = 0; i < 4; i++) {
        if (distance[i] != expected[i]) {
            cout << "FAIL: distance[" << i << "] = " << distance[i] 
                 << ", expected " << expected[i] << endl;
            return 1;
        }
    }
    
    cout << "PASS: All tests passed" << endl;
    return 0;
}