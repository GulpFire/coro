cmake_minimum_required(VERSION 3.0.2)
project(examples)

set(CMAKE_CXX_COMPILER /usr/local/bin/x86_64-pc-linux-gnu-g++)

add_executable(coro_task coro_task.cc)
target_compile_features(coro_task PUBLIC cxx_std_20)
target_link_libraries(coro_task PUBLIC coro)
target_compile_options(coro_task PUBLIC -fcoroutines -Wall -Wextra -pipe)

add_executable(coro_event coro_event.cc)
target_compile_features(coro_event PUBLIC cxx_std_20)
target_link_libraries(coro_event PUBLIC coro)
target_compile_options(coro_event PUBLIC -fcoroutines -Wall -Wextra -pipe)

add_executable(coro_generator coro_generator.cc)
target_compile_features(coro_generator PUBLIC cxx_std_20)
target_link_libraries(coro_generator PUBLIC coro)
target_compile_options(coro_generator PUBLIC -fcoroutines -Wall -Wextra -pipe)

add_executable(coro_thread_pool coro_thread_pool.cc)
target_compile_features(coro_thread_pool PUBLIC cxx_std_20)
target_link_libraries(coro_thread_pool PUBLIC coro)
target_compile_options(coro_thread_pool PUBLIC -fcoroutines -Wall -Wextra -pipe)
