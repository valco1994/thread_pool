//
// Created by valco1994 on 14.02.20.
//

#ifndef THREAD_POOL_TIMER_H
#define THREAD_POOL_TIMER_H

#include <chrono>

class Timer {
    std::chrono::steady_clock clock;
    std::chrono::steady_clock::time_point start_time;
public:
    void start() {
        start_time = clock.now();
    }
    std::chrono::microseconds stop() {
        auto end_time = clock.now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    }
};

#endif //THREAD_POOL_TIMER_H
