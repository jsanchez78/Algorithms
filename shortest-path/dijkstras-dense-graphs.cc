#include <vector>
#include <queue>
using namespace std;

const int INF = 1000000000;
vector<vector<pair<int, int>>> adjList;

void dijkstras(int u, vector<int>& distance, vector<int>& parent){
    int n = adjList.size();
    distance.assign(n, INF);
    parent.assign(n, -1);

    // Src node
    distance[u] = 0;

    priority_queue<pair<int, int>, vector<pair<int,int>>, greater<pair<int,int>>> q;
    q.push({0, u});
    // Explore neighbors
    while (!q.empty()){
        int v = q.top().second;
        int dist_v = q.top().first;

        q.pop();

        if (distance[v] != dist_v) continue;

        for(const auto& edge: adjList[v]){
            int z = edge.first;
            int dist = edge.second;

            if (distance[v] + dist < distance[z]){
                distance[z] = distance[v] + dist;
                parent[z] = v;
                q.push({distance[z], z});
            }
        }
    }
}
