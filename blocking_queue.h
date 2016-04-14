#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>

template <class Value, class Container = std::deque<Value> >
class thread_safe_queue
{
public:
    thread_safe_queue(size_t capacity_): isClosed(false), capacity(capacity_) {}
    void enqueue(Value item);
    void pop(Value& item);
    void shutdown();
private:
    bool isClosed;
    Container queue;
    size_t capacity;
    std::condition_variable isFull;
    std::condition_variable isEmpty;
    std::mutex mut;
};

class enqException : public std::exception
{
    virtual const char* what() const throw()
    {
        return "Push after shutdown.";
    }
};

class deqException : public std::exception
{
    virtual const char* what() const throw()
    {
        return "Pop from empty queue after shutdown.";
    }
};
template <class Value, class Container>
void thread_safe_queue<Value, Container>::enqueue(Value item)
{
    std::unique_lock<std::mutex> lock(mut);
    isFull.wait(lock, [this] () {return queue.size() != capacity || isClosed;});
    if (isClosed)
        throw enqException();
    queue.push_back(move(item));
    isEmpty.notify_one();
}

template <class Value, class Container>
void thread_safe_queue<Value, Container>::pop(Value& item)
{
    std::unique_lock<std::mutex> lock(mut);
    isEmpty.wait(lock, [this] () {return !queue.empty() || isClosed;});
    if (queue.empty())
        throw deqException();
    item = std::move(queue.front());
    queue.pop_front();
    isFull.notify_one();
}

template <class Value, class Container>
void thread_safe_queue<Value, Container>::shutdown()
{
    isClosed = true;
    isEmpty.notify_all();
    isFull.notify_all();
}
