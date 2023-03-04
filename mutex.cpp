#include <chrono>
#include <mutex>
#include <thread>

#include "log.hpp"

int shared { 0 };

void f(const std::string& name, std::mutex& mut, int loop_rep)
{
    for (int i = 0; i < loop_rep; i++) {
        // log() is a thread-safe wrapper for writing to cout (with a mutex).
        log("f ", name, " local section start");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        log("f ", name, " local section finish");

        mut.lock();
        log("f ", name, " critical section start");

        int local = shared;
        // Simulate some work in the critical section.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        local += 1;
        shared = local;

        log("f ", name, " critical section finish");
        mut.unlock();
    }
}

int main()
{
    int loop_rep { 10 };
    std::mutex mut;
    log("main starts");
    std::thread t1 { [&mut, loop_rep] { f("t1", mut, loop_rep); } };
    std::thread t2 { [&mut, loop_rep] { f("t2", mut, loop_rep); } };
    t1.join();
    t2.join();
    log("result is correct? ", (loop_rep * 2 == shared), "");
    log("main finishes");
}
