#include <iostream>
#include <vector>
using namespace std;

#include "floyd-warshall.cc"

int main() {
    // 4-node graph with negative weight
    vector<vector<int>> graph = {
        {0,   3,   INF, 7},
        {INF, 0,   2,   INF},
        {INF, INF, 0,   1},
        {2,   INF, INF, 0}
    };

    auto dist = floydWarshall(graph);

    vector<vector<int>> expected = {
        {0, 3, 5, 6},
        {5, 0, 2, 3},
        {3, 6, 0, 1},
        {2, 5, 7, 0}
    };

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (dist[i][j] != expected[i][j]) {
                cout << "FAIL: dist[" << i << "][" << j << "] = " << dist[i][j]
                     << ", expected " << expected[i][j] << endl;
                return 1;
            }
        }
    }

    cout << "PASS: All tests passed" << endl;
    return 0;
}
