#include <thread>
#include <iostream>
#include <vector>
#include <atomic>
#include <array>

class peterson_mutex{
public:
    peterson_mutex()
    {
        want[0].store(false);
        want[1].store(false);
        victim.store(0);
    }
    void lock (size_t index);
    void unlock(size_t index);
private:
    std::array<std::atomic<bool>, 2> want;
    std::atomic<size_t> victim;
};

void peterson_mutex::lock (size_t index)
{
    want[index].store(true);
    victim.store(index);
    while(want[1 - index].load() && victim.load() == index)
    {
        std::this_thread::yield();
    }
}

void peterson_mutex::unlock(size_t index)
{
    want[index].store(false);
}

class tree_mutex
{
public:
    tree_mutex(size_t n): num(n), tree(2 * two_pow(n) - 1) {}
    void lock(size_t index);
    void unlock(size_t index);
private:
    size_t num;
    std::vector<peterson_mutex> tree;
    int two_pow(int n)
    {
        int pow = 1;
        while(pow < (n + 1) / 2)
            pow *= 2;
        return pow;
    }
};

void tree_mutex::lock(size_t index)
{
    size_t id = tree.size() + index;
    while(id != 0)
    {
        tree[(id - 1) / 2].lock((id + 1) & 1);
        id = (id - 1) / 2;
    }
}

void tree_mutex::unlock(size_t index)
{
    size_t depth = 0;
    while ((1 << depth) < num)
        depth++;
    size_t mutex_id = 0;
    for (int i = depth - 1; i >= 0; i--)
    {
        size_t bit = (index >> i) & 1;
        tree[mutex_id].unlock(bit);
        mutex_id = mutex_id * 2 + bit + 1;
    }
}

