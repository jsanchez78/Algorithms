#include <vector>

using namespace std;

class UnionFind {
    vector<int> rank;
    vector<int> root;

public:
    UnionFind(int sz) : rank(sz, 1), root(sz)
    {
        for (int i = 0; i < sz; ++i)
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

        if (root_a == root_b) return;

        if (rank[root_a] > rank[root_b])
            root[root_b] = root_a;

        else if (rank[root_a] < rank[root_b])
            root[root_a] = root_b;

        else
        {
            root[root_b] = root_a;
            rank[root_a]++;
        }
    }

    bool Connected(int a, int b)
    {
        return Find(a) == Find(b);
    }
};
