// This code is adapted from: http://en.cppreference.com/w/cpp/thread/condition_variable/wait
#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "log.hpp"

std::condition_variable cv;
std::mutex mut; // This mutex is used for two purposes:
                // 1) to synchronize accesses to i,
                // 2) for the condition variable cv.
int i = 0;

void waits()
{
    std::unique_lock<std::mutex> lock(mut);
    log("Waiting...");
    cv.wait(lock, [] { return i == 1; }); // Wait ends iff i == 1.
    log("Finished waiting. i == ", i);
}

void signals()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> lock(mut);
        i = 5;
    }
    log("Notifying...");
    cv.notify_all(); // We don't need the mutex here.

    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> lock(mut);
        i = 1;
    }
    log("Notifying again...");
    cv.notify_all(); // We don't need the mutex here.
}

int main()
{
    std::array<std::thread, 4> threads = {
        std::thread { waits },
        std::thread { waits },
        std::thread { waits },
        std::thread { signals }
    };
    for (auto& t : threads)
        t.join();
}
