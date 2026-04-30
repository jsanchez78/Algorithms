#leetcode 307
class NumArray:
    def __init__(self, nums: List[int]):
        self.n = len(nums)
        self.tree = [0] * (2 * self.n)
        self.buildTree(nums)

    def buildTree(self, nums: List[int]) -> None:
        # Populate leaves
        for i in range(self.n, 2 * self.n):
            self.tree[i] = nums[i - self.n]
        # Populate parents w/ sum of children
        for i in range(self.n - 1, 0, -1):
            self.tree[i] = self.tree[2 * i] + self.tree[2 * i + 1]

    def update(self, index: int, val: int) -> None:
        index += self.n
        self.tree[index] = val
        # Propogate change up the tree
        while index >= 1:
            index //= 2
            self.tree[index] = self.tree[2 * index] + self.tree[2 * index + 1]

    def sumRange(self, left: int, right: int) -> int:
        left += self.n
        right += self.n
        res = 0
        while left <= right:
            if left % 2:
                res += self.tree[left]
                left += 1
            if right % 2 == 0:
                res += self.tree[right]
                right -= 1
            # Move to parent node
            left //= 2
            right //= 2

        return res



# Your NumArray object will be instantiated and called as such:
# obj = NumArray(nums)
# obj.update(index,val)
# param_2 = obj.sumRange(left,right)
