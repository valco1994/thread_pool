//
// Created by valco1994 on 13.02.20.
//

#include "thread_pool.h"

ThreadPool::ThreadPool(uint32_t count) {
    threads.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
    threads.emplace_back(&ThreadPool::run, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(m);
        finished = true;
    }
    cv.notify_all();
    for (auto &t: threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::run() {
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    while (true) {
        lock.lock();
        cv.wait(lock, [this]() { return !tasks.empty() || this->finished; });
        if (finished && tasks.empty()) {
            return;
        }
        auto task = std::move(tasks.front());
        tasks.pop();
        lock.unlock();
        task();
    }
}

void ThreadPool::pool() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this]() { return !tasks.empty() || this->finished; });
    if (finished && tasks.empty()) {
        return;
    }
    auto task = std::move(tasks.front());
    tasks.pop();
    lock.unlock();
    task();
}

bool ThreadPool::empty() {
    {
        std::lock_guard<std::mutex> lock(m);
        return tasks.empty();
    }
}