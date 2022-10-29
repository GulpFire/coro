#include <generator.h>
#include <task.h>
#include <sync_wait.h>
#include <iostream>

int main()
{
	auto task = [](uint64_t count_to) -> coro::Task<void>
	{
		auto gen = []() -> coro::Generator<uint64_t>
		{
			uint64_t i = 0;
			while (true)
			{
				co_yield i++;
			}
		};
	

		for (auto val : gen())
		{
			std::cout << val << ", ";
			if (val >= count_to)
			{
				break;
			}
		}
		co_return;
	};

	coro::sync_wait(task(100));
}
