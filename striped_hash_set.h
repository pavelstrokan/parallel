#include <iostream>
#include <mutex>
#include <vector>
#include <forward_list>
#include <atomic>

template<class T, class Hash = std::hash<T> >
class striped_hash_set
{
public:
    striped_hash_set (
                        size_t mutexNum_ = 1,
                        double growthFactor_ = 2.,
                        double loadFactor_ = 100.
                     )
                     : growthFactor(growthFactor_),
                       loadFactor(loadFactor_),
                       locks(mutexNum_),
                       elementsNum(0),
                       table(4 * mutexNum_)
    {}
    void add(const T& elem);
    void remove(const T& elem);
    bool contains(const T& elem) const;
private:
    size_t get_bucket_index(size_t hash_value) const;
    size_t get_stripe_index(size_t hash_value) const;
    Hash hash_func;
    double growthFactor;
    double loadFactor;
    std::vector<std::mutex> locks;
    std::atomic<size_t> elementsNum;
    std::vector<std::forward_list<T> > table;
};

template<class T, class Hash>
void striped_hash_set<T, Hash>::add(const T& elem)
{
    size_t hash_value = Hash()(elem);
    std::unique_lock<std::mutex> ul(locks[get_stripe_index(hash_value)]);
    auto bucket = table[get_bucket_index(hash_value)];
    bucket.push_front(elem);
    if (static_cast<double>(elementsNum.fetch_add(1)) / table.size() >= loadFactor)
    {
        size_t oldSize = table.size();
        ul.unlock();
        std::vector<std::unique_lock<std::mutex> > ulocks;
        ulocks.emplace_back(locks[0]);
        if(oldSize == table.size())
        {
            for(size_t i = 1; i < locks.size(); ++i)
                ulocks.emplace_back(locks[i]);
            std::vector<std::forward_list<T> > oldTable = table;
            size_t size = static_cast<size_t>(growthFactor * table.size());
            table.clear();
            table.resize(size);
            for(const auto& buck : oldTable)
                for(const auto& elm : buck)
                    table[hash_value % size].push_front(elm);
        }
    }
}

template<class T, class Hash>
void striped_hash_set<T, Hash>::remove(const T& elem)
{
    size_t hash_value = Hash()(elem);
    std::unique_lock<std::mutex> ul(locks[get_stripe_index(hash_value)]);
    auto bucket = table[get_bucket_index(hash_value)];
    bucket.remove(elem);
}

template<class T, class Hash>
bool striped_hash_set<T, Hash>::contains(const T& elem) const
{
    size_t hash_value = Hash()(elem);
    std::unique_lock<std::mutex> ul(locks[get_stripe_index(hash_value)]);
    auto bucket = table[get_bucket_index(hash_value)];
    return std::find(bucket.begin(), bucket.end(), elem) != bucket.end();
}

template<class T, class Hash>
size_t striped_hash_set<T, Hash>::get_bucket_index(size_t hash_value) const
{
    return hash_value % table.size();
}

template<class T, class Hash>
size_t striped_hash_set<T, Hash>::get_stripe_index(size_t hash_value) const
{
    return hash_value % locks.size();
}
