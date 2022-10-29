#include <thread_pool.h>
#include <task.h>
#include <sync_wait.h>
#include <when_all.h>

#include <iostream>
#include <random>

int main()
{
	coro::ThreadPool::options opts{.thread_count = 4, 
		.on_thread_start_functor= [](std::size_t worker_id) -> void
														{
															std::cout << "thread pool worker " << worker_id
																<< " is starting up.\n";
														},
		.on_thread_stop_functor = [](std::size_t worker_id) -> void
														{
															std::cout << "thread pool worker " << worker_id
																<< " is shutting down.\n";
														}};
	coro::ThreadPool thread_pool{opts};

	auto offload_task = [&](uint64_t child_id) -> coro::Task<uint64_t>
	{
		co_await thread_pool.schedule();
		std::random_device rd;
		std::mt19937 gen{rd()};
		std::uniform_int_distribution<> d{0, 1};

		size_t calculation{0};

		for (size_t i = 0; i < 1000000; ++i)
		{
			calculation += d(gen);
			if (i == 500000)
			{
				std::cout << "Task " << child_id << " is yielding()\n";
				co_await thread_pool.yield();
			}
		}
		co_return calculation;
	};

	auto primary_task = [&]() -> coro::Task<uint64_t> 
	{
		const size_t num_children{10};
		std::vector<coro::Task<uint64_t>> child_tasks{};
		child_tasks.reserve(num_children);
		for (size_t i = 0; i < num_children; ++i)
		{
			child_tasks.emplace_back(offload_task(i));
		}

		auto results = co_await coro::when_all(std::move(child_tasks));

		size_t calculation{0};

		for (const auto& task : results)
		{
			calculation += task.return_value();
		}
		co_return calculation;
	};

	auto result = coro::sync_wait(primary_task());
	std::cout << "calculated thread pool result = " << result << "\n";
}
