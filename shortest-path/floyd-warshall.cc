#include <iostream>
#include <vector>
#include <limits.h>

#define INF INT_MAX

using namespace std;
// negative weights
// detect negative cycles
// All pairs to shortest path


vector<vector<int>> floydWarshall(vector<vector<int>> &graph){
    vector<vector<int>> dp = graph;
    int V = graph.size();

    for (int k = 0; k < V; ++k){
        for (int i = 0; i < V; ++i){
            for (int j = 0; j < V; ++j){
                if (dp[i][k] != INF && dp[k][j] != INF && dp[i][k] + dp[k][j] < dp[i][j])
                    dp[i][j] = dp[i][k] + dp[k][j];
            }
        }
    }
    return dp;
}
