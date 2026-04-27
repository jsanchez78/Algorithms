// 305. Number of Islands II
class UnionFind {
    vector<int> rank;
    vector<int> root;

    public:
        UnionFind(int n) : rank(n, 1), root(n)
        {
            for (int i = 0; i < n; ++i)
                root[i] = i;
        }

        int Find(int x)
        {
            if (x != root[x])
                root[x] = Find(root[x]);

            return root[x];
        }

        void Union(int a, int b)
        {
            int root_a = Find(a);
            int root_b = Find(b);

            // Same subset
            if (root_a == root_b) return;

            if (rank[root_a] > rank[root_b])
                root[root_b] = root_a;

            else if (rank[root_b] > rank[root_a])
                root[root_a] = root_b;

            // Same size subtrees
            else {
                root[root_b] = root_a;
                rank[root_a]++;
            }
        }

        bool Connected(int a, int b)
        {
            return Find(a) == Find(b);
        }
    };

class Solution {
public:
    vector<int> numIslands2(int m, int n, vector<vector<int>>& positions) {
        UnionFind uf(m * n);
        vector<int> res;
        set<int> land;
        int numIslands = 0;
        vector<pair<int, int>> directions = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

        for(auto& pos: positions){
            int r = pos[0], c = pos[1];
            // Flatten 2D -> 1D
            int idx = r * n + c;
            if (land.count(idx))
            {
                res.push_back(numIslands);
                continue;
            }

            land.insert(idx);
            numIslands++;

           for (auto [dx, dy]: directions){
                int x = r + dx;
                int y = c + dy;
                int neighbor_idx = x * n + y;

                if (x >= 0 && x < m && y >= 0 && y < n && land.count(neighbor_idx)){
                    if (!uf.Connected(idx, neighbor_idx)){
                        uf.Union(idx, neighbor_idx);
                        numIslands--;
                    }
                }
           }
            res.push_back(numIslands);
        }
        return res;
    }
};





