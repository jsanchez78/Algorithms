#include <cmath>
#include <vector>
#include <queue>
#include <limits>
#include <iostream>

using namespace std;

class Node {
    public:
        int x, y;
        float weight;

        Node(int x, int y, float w) : x(x), y(y), weight(w) {}
};

bool operator==(const Node &a, const Node &b){
    return a.x == b.x && a.y == b.y;
}

bool operator<(const Node &a, const Node& b){
    return a.weight > b.weight;
}

float chebyshev_dist(int x1, int y1, int x2, int y2){
    return max(abs(x2 - x1), abs(y2 - y1));
}

float euclidean_dist(int x1, int y1, int x2, int y2){
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

float manhattan_dist(int x1, int y1, int x2, int y2){
    return abs(x2 - x1) + abs(y2 - y1);
}

bool isValid(int x, int y, const vector<vector<int>>& grid){
    int m = grid.size();
    int n = grid[0].size();
    return x >= 0 && x < m && y >= 0 && y < n && grid[x][y] == 0;
}

vector<vector<long long>> countPaths(vector<vector<int>>& grid, int x, int y){
    int m = grid.size();
    int n = grid[0].size();

    vector<vector<long long>> dp(m, vector<long long>(n, 0));
    dp[x][y] = 1;  // Initialize start position

    for(int i = 0; i < m; ++i){
        for (int j = 0; j < n; ++j){
            if (grid[i][j] == 0 && dp[i][j] > 0){
                // Add paths from current cell to neighbors
                if ((i + 1) < m && grid[i + 1][j] == 0)
                    dp[i + 1][j] += dp[i][j];

                if ((j + 1) < n && grid[i][j + 1] == 0)
                    dp[i][j + 1] += dp[i][j];

            }
        }
    }
    return dp;
}

// Bidrection path counting
struct PathCounter{
    vector<vector<long long>> fromStart, fromEnd;

    void computeCentrality(vector<vector<int>>& grid, Node start, Node end){
        fromStart = countPaths(grid, start.x, start.y);

        fromEnd = countPaths(grid, end.x, end.y);
    }

    float getCentrality(int x, int y){
        return fromStart[x][y] * fromEnd[x][y];
    }
};

void astar(vector<vector<int>>& grid, const Node& start, const Node& end){
    int m = grid.size();
    int n = grid[0].size();

    const int directions[][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    vector<vector<bool>> visited(m, vector<bool>(n, false));

    // Init path counting
    PathCounter counter;
    counter.computeCentrality(grid, start, end);

    // Init astar
    priority_queue<Node> nodes;
    nodes.push(start);

    while (!nodes.empty()){
        Node curr = nodes.top();
        nodes.pop();

        if (curr == end) {
            cout << "Path found! Cost: " << curr.weight << endl;
            return;
        }

        if (visited[curr.x][curr.y]) continue;
        visited[curr.x][curr.y] = true;

        // Explore neighbors
        for (int i = 0; i < 4; ++i){
            int x = curr.x + directions[i][0];
            int y = curr.y + directions[i][1];

            if (isValid(x, y, grid) && !visited[x][y]){
                float g = curr.weight + 1;
                float h = manhattan_dist(x, y, end.x, end.y);
                float centrality_bonus = -0.01 * log(counter.getCentrality(x, y) + 1);
                nodes.push(Node(x, y, g + h + centrality_bonus));
            }
        }
    }
    cout << "No path found!" << endl;
}

int main(){
    vector<vector<int>> grid = {
        {0, 0, 0, 1, 0},
        {0, 1, 0, 1, 0},
        {0, 0, 0, 0, 0},
        {1, 1, 0, 1, 0},
        {0, 0, 0, 0, 0}
    };

    Node start(0, 0, 0);
    Node end(4, 4, 0);

    astar(grid, start, end);
    return 0;
}
