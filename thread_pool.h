//
// Created by valco1994 on 13.02.20.
//

/*
 *                              *** ЗАДАНИЕ ***
 *
 * Необходимо реализовать диспетчер для асинхронного выполнения задач в пуле потоков.
 * Количество потоков задается при создании диспетчера и не меняется во времени.
 * Добавление задач в очередь происходит асинхронно из произвольных потоков, в том
 * числе из потоков пула. Вызывающая сторона должна иметь возможность дождаться
 * результатов исполнения поставленной задачи и получить результат вычислений.
 *
 * Необходимо предусмотреть как минимум следующие операции:
 *      execute() - добавить задачу в конец очереди.
 *      poll() - выполнить задачу из начала очереди в текущем потоке.
 *      empty() - проверить, что в очереди есть задачи на выполнение.
 *
 * Обязательно реализовать нагрузочное многопоточное тестирование кода.
 */


/*
 *                          *** Комментарий к решению ***
 *
 * В данной задаче имеется серьезная проблема: при условии, что новые задачи могут
 * генерироваться из потоков пула, легко можно получить ситуацию, при которой все
 * потоки из пула будут исчерпаны, вися в ожидании результата обработки задач,
 * переданной на обработку в тот же пул.
 *
 * При использовании данной структуры необходимо учитывать этот момент и, соответсвенно
 * формулировать задачи таким образом, чтобы вышеописанная ситуация была невозможна.
 * Например, можно требовать, чтобы при вложенной генерации задач поток пытался бы сам
 * выполнить эту задачу с использованием функции-члена pool прежде, чем вызывать
 * блокирующую операцию в духе std::future<T>::get или std::future<T>::wait. Если
 * какой-то другой поток уже взял эту задачу на выполнение, то проблем не возникнет,
 * если же все остальные потоки уже так или иначе заняты, то текущий поток просто сам
 * выполнит задачу.
 * */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <future>
#include <queue>
#include <functional>
#include <iostream>
#include <cassert>

namespace thread_pool {

enum class FinishMode { ProcessAllPassed, ProcessCurrentOnly };

template<FinishMode mode = FinishMode::ProcessCurrentOnly>
class ThreadPool {
private:
    std::queue<std::function<void ()>> tasks;

    std::mutex m;
    std::condition_variable cv;
    std::vector<std::thread> threads;

    bool finished = false; // no reason to make it atomic
    // (see https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables)

    void run();
    void stop_and_join();
public:
    ThreadPool(): ThreadPool(std::thread::hardware_concurrency()) {}
    ThreadPool(uint32_t count);
    ~ThreadPool() { stop_and_join(); }

    ThreadPool(ThreadPool const &) = delete;
    ThreadPool &operator=(ThreadPool const &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void poll();
    bool empty();

    template <typename F, typename ...Args>
    std::future<typename std::invoke_result_t <F, Args...> > execute(F &&f, Args&& ...args);
};

// TODO: What about possible exceptions here? Must they be handled?
template <FinishMode mode>
void ThreadPool<mode>::stop_and_join() {
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
    if constexpr (mode == FinishMode::ProcessCurrentOnly) {
        std::queue<std::function<void ()>>().swap(tasks);
    }
}

/* Maximum number of threads is limited. You can use command "cat /proc/sys/kernel/threads-max" on Linux to check it */
template <FinishMode mode>
ThreadPool<mode>::ThreadPool(uint32_t count) {
    uint32_t started = 0;
    try {
        threads.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            threads.emplace_back(&ThreadPool::run, this);
            ++started;
        }
    } catch (std::system_error const &e) {
        #ifndef NDEBUG
            std::cerr << "Error during threads start (" << started << " / " << count << " was started): " << e.what() << "\n";
        #endif
        stop_and_join();
        throw;
    }
}

template <FinishMode mode>
void ThreadPool<mode>::run() {
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    while (true) {
        lock.lock();
        cv.wait(lock, [this]() { return !tasks.empty() || this->finished; });
        if constexpr (mode == FinishMode::ProcessAllPassed) {
            if (finished && tasks.empty()) {
                return;
            }
        }
        if constexpr (mode == FinishMode::ProcessCurrentOnly) {
            if (finished) {
                return;
            }
        }

        auto task = std::move(tasks.front());
        tasks.pop();
        lock.unlock();
        task();
    }
}

/* This member function can be called from the thread handling thread pool, so it should NOT wait
 * for new tasks using condition variable, because otherwise it can lead to program freeze */
template <FinishMode mode>
void ThreadPool<mode>::poll() {
    std::unique_lock<std::mutex> lock(m);

    if (tasks.empty()) {
        return;
    } else if (finished && mode == FinishMode::ProcessCurrentOnly) {
        return;
    }

    auto task = std::move(tasks.front());
    tasks.pop();
    lock.unlock();
    task();
}

template <FinishMode mode>
bool ThreadPool<mode>::empty() {
    {
        std::lock_guard<std::mutex> lock(m);
        return tasks.empty();
    }
}

template <FinishMode mode>
template <typename F, typename ...Args>
std::future<std::invoke_result_t <F, Args...>> ThreadPool<mode>::execute(F &&f, Args&& ...args) {
    auto task = std::make_shared<std::packaged_task<std::invoke_result_t<F, Args...> ()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m);
        if (finished) {
            throw std::runtime_error("Try to add task after thread pool finishing");
        }
        tasks.emplace([task]() { std::invoke(*task); });
    }
    cv.notify_one();

    return result;
}

}

#endif // THREADPOOL_H
