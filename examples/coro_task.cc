#include <task.h>
#include <sync_wait.h>

#include <iostream>

int main()
{
	auto square = 
		[](uint64_t x) -> coro::Task<uint64_t>
		{
			co_return x * x;
		};

	auto square_and_add_5 = 
		[&](uint64_t input) -> coro::Task<uint64_t>
		{
			auto squared = co_await square(input);
			co_return squared + 5;
		};

	auto output = coro::sync_wait(square_and_add_5(5));
	std::cout << "Task1 output: " << output << "\n";
}
