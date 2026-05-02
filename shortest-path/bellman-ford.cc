struct Edge
{
    int u, v, w;
};

vector<Edge> edges;
const int INF = 1000000000;

int src, n;

void solve(){
    vector<int> distance(n, INF);
    distance[src] = 0;

    for(int i = 0; i < n - 1; ++i){
        for(const auto& edge: edges){
            if (distance[edge.u] < INF)
                distance[edge.v] = min(distance[edge.v], distance[edge.u] + edge.w);
        }
    }
}
