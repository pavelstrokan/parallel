#include <iostream>
#include <mutex>
#include <condition_variable>

class barrier
{
public:
	explicit barrier(size_t threadsNum): capacity(threadsNum), count(0), door(false) {}
	void enter();
private:
	size_t capacity;
	size_t count;
	std::condition_variable barrier1, barrier2;
	std::mutex mut;
	bool door;
};

void barrier::enter()
{
	std::unique_lock<std::mutex> lock(mut);
	// ждем, пока наберется capacity потоков
	barrier1.wait(lock, [this] () {return !door;} );
	++count;
	barrier2.wait(lock, [this] () {return capacity == count || door;} );
	// последний пришедший открывает всем дверь и они заходят
	door = true;
	--count;
	barrier2.notify_all();
	// последний вошедший закрывает дверь и все ждущие за дверью делают то же заново
	if (count == 0)
	{
        door = false;
		barrier1.notify_all();
	}
}

