#include <stdio.h>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

struct FenwickTree
{
    vector<int> bit;
    int n;

    FenwickTree(int n)
    {
        this->n = n + 1;
        bit.assign(n + 1, 0);
    }

    FenwickTree(const vector<int>& arr) : FenwickTree(arr.size())
    {
        for (size_t i = 0; i < arr.size(); ++i)
            add(i, arr[i]);
    }

    int sum(int idx)
    {
        int res = 0;
        // Two's complement: subtract lowest set bit to move left
        for (++idx; idx > 0; idx -= idx & -idx)
            res += bit[idx];
        return res;
    }

    int sum(int l, int r)
    {
        return sum(r) - sum(l - 1);
    }

    void add(int idx, int delta)
    {
        // Two's complement: add lowest set bit to move right
        for (++idx; idx < n; idx += idx & -idx)
            bit[idx] += delta;
    }
};

int main() {
    // A = {1, 3, 5, 7, 9, 11}
    vector<int> a = {1, 3, 5, 7, 9, 11};
    FenwickTree ft(a);

    // prefix sums: [0]=1, [0,2]=9, [0,5]=36
    printf("sum [0,0]: %d (expect 1)\n", ft.sum(0));
    printf("sum [0,2]: %d (expect 9)\n", ft.sum(2));
    printf("sum [0,5]: %d (expect 36)\n", ft.sum(5));

    // range sum [2,4] = 5+7+9 = 21
    printf("sum [2,4]: %d (expect 21)\n", ft.sum(2, 4));

    // update: A[3] += 3 → A = {1, 3, 5, 10, 9, 11}
    ft.add(3, 3);
    printf("sum [0,5] after update: %d (expect 39)\n", ft.sum(5));
    printf("sum [2,4] after update: %d (expect 24)\n", ft.sum(2, 4));

    return 0;
}
