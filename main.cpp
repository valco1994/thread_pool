//
// Created by valco1994 on 13.02.20.
//

#include <gtest/gtest.h>
#include <cmath>
#include "thread_pool.h"
#include "timer.h"

class ThreadPoolLoadTests: public ::testing::Test {};

TEST_F(ThreadPoolLoadTests, SimpleFunctions) {
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

TEST_F(ThreadPoolLoadTests, SlightlyMoreDifficult) {
    Timer timer;

    auto lis = [](std::vector<int> const& a) {
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
    };
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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    auto code = RUN_ALL_TESTS();
    return code;

}

