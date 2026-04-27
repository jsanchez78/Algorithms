// 952 largest-component-size-by-common-factor
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
    int largestComponentSize(vector<int>& nums) {
        UnionFind uf(100001);

        for (auto num: nums){
            for (auto factor: primeFactors(num))
                uf.Union(num, factor);
        }

        int res = 0;
        unordered_map<int, int> rootCount;
        for (auto num: nums)
            res = max(res, ++rootCount[uf.Find(num)]);

        return res;
    }

    vector<int> primeFactors(int n){
        vector<int> res;

        while  (n % 2 == 0){
            res.push_back(2);
            n /= 2;
        }

        // n must be odd at this point
        for (int i = 3; i * i <= n; i += 2){
            while(n % i == 0){
                res.push_back(i);
                n /= i;
            }
        }

        // n is prime num
        if (n > 2)
            res.push_back(n);

        return res;
    }
};






