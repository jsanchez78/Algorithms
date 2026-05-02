#include <vector>
using namespace std;

const int INF = 1000000000;
vector<vector<pair<int, int>>> adjList;

void dijkstra(int src, vector<int>& distance, vector<int>& parent){
    int n = adjList.size();
    distance.assign(n, INF);
    parent.assign(n, -1);

    vector<bool> visited(n, false);
    // Start at src node
    distance[src] = 0;
    for(int i = 0; i < n; ++i){
        int v = -1;
        for (int j = 0; j < n; ++j){
            if (!visited[j] && (v == -1 || distance[j] < distance[v]))
                v = j;
        }

        if (distance[v] == INF) break;

        // Mark
        visited[v] = true;

        // Explore neighbors
        for (const auto& edge: adjList[v]){
            int u = edge.first;
            int dist = edge.second;

            // Update distance of neighbors
            if (distance[v] + dist < distance[u]){
                distance[u] = distance[v] + dist;
                parent[u] = v;
            }
        }
    }
}
