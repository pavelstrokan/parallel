#include <vector>
#include <atomic>

const int cache_size = 64;

template <class T>
class spsc_ring_buffer
{
public:
    spsc_ring_buffer(size_t size): data(size + 1), head(0), tail(0) {}
    bool enqueue(T item);
    bool dequeue(T& item);
private:
    struct node_t
    {
	node_t() {}
	node_t(T item): val(item) {}
	T val;
	char pad[cache_size]; 
    };
    std::vector<node_t> data;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
    size_t get_next_id(size_t id)
    {
	return (id + 1) % data.size();
    }
};

template <class T>
bool spsc_ring_buffer<T>::enqueue(T item)
{
    size_t tail_id = tail.load(std::memory_order_relaxed); 
    size_t tail_next_id = get_next_id(tail_id);
    if (tail_next_id == head.load(std::memory_order_acquire))
        return false;
    data[tail_id] = node_t(item);
    tail.store(tail_next_id, std::memory_order_release);
    return true;
}

template <class T>
bool spsc_ring_buffer<T>::dequeue(T& item)
{
    size_t head_id = head.load(std::memory_order_relaxed);
    if (head_id == tail.load(std::memory_order_acquire))
        return false;
    item = data[head_id].val;
    head.store(get_next_id(head_id), std::memory_order_release);
    return true;
}

