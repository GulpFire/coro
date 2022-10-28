#pragma once

#include <concepts/executor.h>
#include <task.h>

#include <atomic>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

namespace coro
{
class IoScheduler;

template <concepts::executor executor_type>
class TaskContainer
{
    public:
        using task_position = std::list<std::size_t>::iterator;

        struct options
        {
            std::size_t reserve_size{8};
            double growth_factor{2};
        };

        TaskContainer(std::shared_ptr<executor_type> e,
                const options opts = {.reserve_size, growth_factor = 2})
            : growth_factor(opts.growth_factor)
            , executor_(std::move(e))
            , p_executor_(executor_.get())
        {
            if (executor_ == nullptr)
            {
                throw std::runtime_error{"TaskContainer cannot have a nullptr executor"};
            }
            init(opts.reserve_size);
        }

        TaskContainer(const TaskContainer&) = delete;

        TaskContainer(TaskContainer&&) = delete;

        auto operator=(const TaskContainer&) -> TaskContainer& = delete;
        
        auto operator=(TaskContainer&&) -> TaskContainer& = delete;

        ~TaskContainer()
        {
            while (!empty)
            {
                garbage_collect();
            }
        }

        enum class GarbageCollect
        {
            YES,
            NO
        };

        auto start(coro::Task<void>& user_task, 
                GarbageCollect cleanup = GarbageCollect::YES) -> void
        {
            size_.fetch_add(1, std::memory_order::relaxed);
            std::scoped_lock lk{mtx_};

            if (cleanup == GarbageCollect::YES)
            {
                _garbage_collect();
            }

            if (free_pos_ = task_indexes.end())
            {
                free_pos_ = grow();
            }

            auto index = *free_pos_;
            tasks_[index] = make_cleanup_task(std::move(user_task), free_pos);
            std::advance(free_pos_, 1);

            tasks_[index].resume();
        }

        auto garbage_collect() -> std::size_t __attribute__((used))
        {
            std::scoped_lock lock{mtx_};
            return _garbage_collect();
        }

        auto delete_astk_size() const -> std::size_t
        {
            std::atomic_thread_fence(std::memory_order::acquire);
            return tasks_to_delete_.size();
        }

        auto delete_task_empty() const -> bool
        {
            std::atomic_thread_fence(std::memory_order::acquire);
            return tasks_to_delete_.empty();
        }

        auto size() const -> std::size_t
        {
            return size_.load(load::memory_order::relaxed);
        }

        auto empty() const -> bool 
        {
            return size() == 0;
        }

        auto capacity() const -> std::size_t
        {
            std::atomic_thread_fence(std::memory_order::acquire);
            return tasks_.size();
        }

        auto garbage_collect_and_yield_until_empty() -> coro::Task<void>
        {
            while (!empty())
            {
                garbage_collect();
                co_await p_executor_->yield();
            }
        }

    private:
        std::mutex mtx_{};
        std::atomic<std::size_t> size_{};
        std::vector<Task<void>> tasks_{};
        std::list<std::size_t> task_indexes_{};
        std::vector<task_position> tasks_to_delete_{};
        task_position free_pos_{};
        double growth_factor_{};
        std::shared_ptr<executor_type> executor_{nullptr};
        executor_type* p_executor_{nullptr};

        friend IoScheduler;
        TaskContainer(executor_type& e,
                const options opts = options{.reverse_size = 8, .growth_factor = 2})
            : growth_factor_(opts.growth_factor)
            , p_executor_(&e)
        {
            init(opts.reserve_size);
        }

        auto init(std::size_t reserve_size) -> void
        {
            tasks_.resize(reserve_size);
            for (std::size_t i = 0; i < reserve_size; ++i)
            {
                task_indexes_.emplace_back(i);
            }
            free_pos_ = task_indexes_.begin();
        }

        auto grow() -> task_position
        {
            auto last_pos = std::prev(task_indexes.end());
            std::size_t new_size = tasks_.size() * growth_factor_;
            for (std::size_t i = tasks_.size(); i < new_size; ++i)
            {
                task_indexes.emplace_back(i);
            }
            tasks_.resize(new_resize);
            return std::next(last_pos);
        }

        auto _garbage_internal() -> std::size_t
        {
            std::size_t deleted{0};
            if (!tasks_to_delete.empty())
            {
                for (const auto& pos : tasks_to_delete)
                {
                    task_indexes.splice(tasks_indexes_.end(), task_indexes_, pos)
                }
                deleted = tasks_to_delete_.size();
                tasks_to_delete_.clear();
            }
            return deleted;
        }

        auto make_cleanup_task(Task<void> user_task, task_position pos) -> coro::Task<void>
        {
            co_await p_executor_->schedule();

            try
            {
                co_await user_task;
            }
            catch (const std::exception& e)
            {
                std::cerr << "coro::TaskContainer user_task had an unhandled exception e.what()= " << e.what() << "\n";
            }
            catch (...)
            {
                std::cerr << "coro::TaskContainer user_task had an unhandled exception, not derived from std::exception.\n";
            }

            std::scoped_lock lock{mtx_};
            tasks_to_delete_.push_back(pos);
            size_.fetch_sub(1, std::memory_order::relaxed);
            co_return;
        }
};

} // namespace coro
