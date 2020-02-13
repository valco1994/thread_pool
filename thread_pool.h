//
// Created by valco1994 on 13.02.20.
//

/*
 * Необходимо реализовать диспетчер для асинхронного выполнения задач в пуле потоков.
 * Количество потоков задается при создании диспетчера и не меняется во времени.
 * Добавление задач в очередь происходит асинхронно из произвольных потоков, в том
 * числе из потоков пула. Вызывающая сторона должна иметь возможность дождаться
 * результатов исполнения поставленной задачи и получить результат вычислений.
 *
 * Необходимо предусмотреть как минимум следующие операции:
 *      execute() - добавить задачу в конец очереди.
 *      pool() - выполнить задачу из начала очереди в текущем потоке.
 *      empty() - проверить, что в очереди есть задачи на выполнение.
 *
 * Обязательно реализовать нагрузочное многопоточное тестирование кода.
 * */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <future>
#include <queue>
#include <functional>

class ThreadPool {
private:
    std::queue<std::function<void ()>> tasks;
    std::vector<std::thread> threads;

    std::mutex m;
    std::condition_variable cv;

    bool finished = false; // no reason to make it atomic
    // (see https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables)

    void run();
public:
    ThreadPool(uint32_t count);
    ~ThreadPool();

    ThreadPool(ThreadPool const &) = delete;
    ThreadPool &operator=(ThreadPool const &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void pool();
    bool empty();

    template <typename F, typename ...Args>
    std::future<typename std::invoke_result_t <F, Args...> > execute(F &&f, Args&& ...args);
};

template <typename F, typename ...Args>
std::future<std::invoke_result_t <F, Args...>> ThreadPool::execute(F &&f, Args&& ...args) {
    auto task = std::make_shared<std::packaged_task<std::invoke_result_t<F, Args...> ()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m);
        if (finished) {
            throw std::runtime_error("Try to add callable object after thread pool finishing");
        }
        tasks.emplace([task]() { std::invoke(*task); });
    }
    cv.notify_one();

    return result;
}


#endif // THREADPOOL_H
