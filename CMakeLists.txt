cmake_minimum_required(VERSION 3.0.2)
project(coro)

set(CMAKE_CXX_COMPILER /usr/local/bin/x86_64-pc-linux-gnu-g++)
set(CARES_STATIC ON CACHE INTERNAL "")

set(LIBCORO_SOURCE_FILES
    include/concepts/awaitable.h
    include/concepts/executor.h
    include/concepts/promise.h
    include/concepts/range_of.h
    include/detail/void_value.h
    include/event.h
    include/fd.h
    include/generator.h
    include/poll.h
    include/sync_wait.h
    include/task.h
    include/task_container.h
    include/thread_pool.h
    include/when_all.h
    
    src/event.cc
    src/sync_wait.cc
    src/thread_pool.cc
)

add_library(${PROJECT_NAME} STATIC ${LIBCORO_SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
target_compile_options(${PROJECT_NAME} PUBLIC -fcoroutines -Wall -Wextra -pipe)

add_subdirectory(examples)
