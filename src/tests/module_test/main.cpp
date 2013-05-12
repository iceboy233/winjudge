#include <iostream>
#include <functional>
#include <cstdint>
#include <windows.h>
#include "module.hpp"

using namespace std;
using namespace judge;

void print_times()
{
	uint64_t tick = mod.tick_count();
	uint64_t idle = mod.idle_time();
	static bool first = true;
	static unsigned pscnt;
	static uint64_t last_tick, last_idle;
	
	if (first) {
		first = false;
		pscnt = mod.processor_count();
		cout << "processor " << pscnt << " tick " << tick << " idle " << idle << endl;
	} else {
		cout << "delta tick " << tick - last_tick << " delta idle " << (idle - last_idle) / pscnt / 10000 << endl;
	}

	last_tick = tick;
	last_idle = idle;
}

struct print_number : unary_function<int, void> {
	void operator()(int number)
	{
		cout << number << endl;
	}
};

void timer_queue_test()
{
	cout << "testing timer queue..." << endl;
	mod.timer_queue.queue(bind(print_number(), 6), 4000);
	mod.timer_queue.queue(bind(print_number(), 2), 2000);
	mod.timer_queue.queue(bind(print_number(), 8), 5000);
	mod.timer_queue.queue(bind(print_number(), 4), 3000);
	mod.timer_queue.queue(bind(print_number(), 1), 1000);
	::Sleep(1500);
	mod.timer_queue.queue(bind(print_number(), 9), 4000);
	mod.timer_queue.queue(bind(print_number(), 5), 2000);
	mod.timer_queue.queue(bind(print_number(), 10), 5000);
	mod.timer_queue.queue(bind(print_number(), 7), 3000);
	mod.timer_queue.queue(bind(print_number(), 3), 1000);
}

void thread_pool_test()
{
	cout << "testing thread pool..." << endl;
	for (int i = 0; i < 10; ++i) {
		::Sleep(80);
		mod.thread_pool.queue([]()->void {
			cout << "hi!" << endl;
			::Sleep(1000);
			cout << "i'm not thread-safe!" << endl;
		});
	}
}

int main()
{
	for (int i = 0; i < 10; ++i) {
		print_times();
		::Sleep(1000);
	}

	timer_queue_test();
	thread_pool_test();
	mod.rundown();
}
