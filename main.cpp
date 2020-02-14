//
// Created by valco1994 on 13.02.20.
//

#include <gtest/gtest.h>
#include <cmath>
#include <random>
#include "thread_pool.h"
#include "timer.h"

int lis(std::vector<int> const& a) {
    size_t n = a.size();
    std::vector<int> d(n, 1);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i; j++) {
            if (a[j] < a[i])
                d[i] = std::max(d[i], d[j] + 1);
        }
    }

    int ans = d[0];
    for (int i = 1; i < n; i++) {
        ans = std::max(ans, d[i]);
    }
    return ans;
}

class ThreadPoolTests: public ::testing::Test {};

TEST_F(ThreadPoolTests, SimpleFunctions) {
    Timer timer;
    std::future<double> future1;
    std::future<int>    future2;
    std::future<int>    future3;
    std::future<void>   future4;

    timer.start();
    {
        ThreadPool pool(2);
        future1 = pool.execute([](int a, int b) { return sqrt(a*a + b*b); }, 4, 3);
        future2 = pool.execute([](int a, int b) { return static_cast<int>(log2(a*a - b)); }, 9, 17);
        future3 = pool.execute([]() { return 5; });
        future4 = pool.execute([]() { std::cout << "Void function is being processed!\n"; });

        pool.pool();

        EXPECT_EQ(future2.get(), 6);
        EXPECT_EQ(future3.get(), 5);
    }

    EXPECT_EQ(future1.get(), 5);
    future4.wait();

    auto elapsed = timer.stop();
    std::cout << elapsed.count() << " us elapsed" << std::endl;
}

TEST_F(ThreadPoolTests, SlightlyMoreDifficult) {
    Timer timer;

    std::vector<int> v { 13, 22, 88, 323, 324, 1, 42, -4, 3, 89, 123, 3333, 8943, 999 };
    std::future<int> future;

    {
        ThreadPool pool(2);
        printf("Is task queue empty? %s\n", pool.empty() ? "Yes" : "No");
        pool.execute(lis, std::cref(v));
        pool.execute(lis, std::cref(v));
        pool.execute(lis, std::cref(v));
        pool.execute(lis, std::cref(v));

        pool.pool();
        printf("Is task queue empty? %s\n", pool.empty() ? "Yes" : "No");
        pool.pool();
    }

    auto elapsed = timer.stop();
    std::cout << elapsed.count() << " us elapsed" << std::endl;
}

TEST_F(ThreadPoolTests, GeneratingFunctions) {
    Timer timer;

    timer.start();
    {
        ThreadPool pool(2);
        auto future1 = pool.execute([&pool](int a, int b) {
                auto future2 = pool.execute([&pool](int a, int b) {
                        auto future3 = pool.execute([&pool]() {
                            return 5;
                        });
                        //future3.wait(); // If you uncomment this line, then program will freeze, because all
                                          // threads from the pool will be blocked.
                        return static_cast<int>(log2(a*a - b));
                    }, 9, 17);
                future2.wait();
                return sqrt(a*a + b*b);
            }, 4, 3);
        future1.wait();
    }

    auto elapsed = timer.stop();
    std::cout << elapsed.count() << " us elapsed" << std::endl;
}

TEST_F(ThreadPoolTests, GeneratingFunctionsCorrected) {
    Timer timer;

    timer.start();
    {
        std::vector<std::future<int>> futures;
        ThreadPool pool(2);
        auto future1 = pool.execute([&pool](int a, int b) {
            auto future2 = pool.execute([&pool](int a, int b) {
                auto future3 = pool.execute([&pool]() {
                    return 5;
                });
                pool.pool();
                future3.wait();
                return static_cast<int>(log2(a*a - b));
            }, 9, 17);
            pool.pool();
            future2.wait();
            return sqrt(a*a + b*b);
        }, 4, 3);
        pool.pool();
        future1.wait();
    }

    auto elapsed = timer.stop();
    std::cout << elapsed.count() << " us elapsed" << std::endl;
}

TEST_F(ThreadPoolTests, LoadTests) {
    std::random_device r;

    // Choose a random mean between 1 and 6
    std::default_random_engine engine(r());
    std::mt19937 gen(r());
    std::poisson_distribution<int> dist(10);

    std::vector<int> v;
    size_t N = 500; //10000;
    v.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        v.push_back(dist(gen));
    }

    Timer timer;

    std::cout << "Count of tasks: 100, pool size is being varied" << std::endl;
    for (size_t pool_size = 2; pool_size < 21; ++pool_size) {
        timer.start();
        {
            ThreadPool pool(pool_size);
            for (size_t i = 0; i < 100; ++i) {
                pool.execute(lis, std::cref(v));
            }
        }
        auto elapsed = timer.stop();
        std::cout << "Pool size is " << pool_size << ", count of tasks is 100, " <<
                     elapsed.count() << " us elapsed" << std::endl;
    }

    std::cout << "\nCount of tasks is being varied, pool size is 4" << std::endl;
    for (size_t n_tasks = 10; n_tasks < 201; n_tasks += 10) {
        timer.start();
        {
            ThreadPool pool(4);
            for (size_t i = 0; i < n_tasks; ++i) {
                pool.execute(lis, std::cref(v));
            }
        }
        auto elapsed = timer.stop();
        std::cout << "Pool size is 4, count of tasks is " << n_tasks << ", " <<
                      elapsed.count() << " us elapsed" << std::endl;
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    auto code = RUN_ALL_TESTS();
    return code;
}

