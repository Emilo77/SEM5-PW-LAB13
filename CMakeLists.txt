cmake_minimum_required (VERSION 3.1)
project (MIMUW-CPCPP-lab2 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-g -Wall")
# set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

function(add_example_program target_name)
  add_executable(${target_name} "${target_name}.cpp")
  target_link_libraries(${target_name} Threads::Threads)
endfunction()

add_example_program(mutex)
add_example_program(lock)
add_example_program(condition)
add_example_program(thread-local)
add_example_program(thread-local-static)
# add_example_program(barrier)
