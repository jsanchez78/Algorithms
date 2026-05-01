#include <bits/stdc++.h>
using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    return 0;
}

struct FenwickTree
{
    vector<int> bit;
    int size;

    FenwickTree(int n) : size(n), bit(n + 1, 0)
    {
        for (int i = 1; i <= n; ++i){
            bit[i] += 1; // 1 represents empty slot
            int parent = i + (i & -i);
            if (parent <= n)
                bit[parent] += bit[i];
        }
    }

    void add(int idx, int delta){
        // Set trailing bits then  move right
        for(; idx <= size; idx += idx & -idx)
            bit[idx] += delta;
    }

    int findKth(int k){
        int idx = 0;

        for(int i = 1 << (31 - __builtin_clz(size)); i > 0; i >>= 1){
            // Number of empty bits from 1 to (idx + 1)
            if ((idx + i) < size && bit[idx + i] < k){
                idx += i;
                k -= bit[idx];
            }
        }
        return idx + 1;
    }
};

class Solution {
public:
    vector<vector<int>> reconstructQueue(vector<vector<int>>& people) {
            int n = people.size();
            vector<vector<int>> res(n);

            // Sort by height then k
            sort(people.begin(), people.end(), [](const vector<int>& a, const vector<int>& b) {
                 return  (a[0] != b[0])
                    ? a[0] < b[0]
                    :  a[1] > b[1];
            });
            // Init fenwick trees
            FenwickTree ft(n);

            // Iterate sorted list
            for(const auto& p: people){
                // find (k + 1) empty slot
                int kthPos = ft.findKth(p[1] + 1);
                res[kthPos - 1] = p;
                //  Update: 0 represents filled slot
                ft.add(kthPos, -1);
            }
            return res;
    }
};
