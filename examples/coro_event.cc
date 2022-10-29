#include <task.h>
#include <event.h>
#include <when_all.h>
#include <sync_wait.h>

#include <iostream>

int main()
{
	coro::Event e;

	auto make_wait_task = 
		[](const coro::Event& e, uint64_t i) -> coro::Task<void>
		{
			std::cout << "task " << i << " is waiting on the event...\n";
			co_await e;
			std::cout << "task " << i << " event triggered, now resuming. \n";
			co_return;
		};

	auto make_set_task = 
		[](coro::Event& e) -> coro::Task<void>
		{
			std::cout << "Set task is triggering the event\n";
			e.set();
			co_return;
		};

	coro::sync_wait(coro::when_all(make_wait_task(e, 1),
				make_wait_task(e, 2), make_wait_task(e, 3), make_set_task(e)));
}
