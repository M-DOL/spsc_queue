#include <gtest/gtest.h>
#include "spsc_queue.hpp"
#include <thread>

// --- Single-threaded correctness ---

TEST(SPSCQueue, InitiallyEmpty) {
    SPSCQueue<int, 1024> q;
    EXPECT_TRUE(q.empty());
}

TEST(SPSCQueue, PushThenPop) {
    SPSCQueue<int, 1024> q;
    q.push(5);
    int val = 0;
    q.pop(val);
    EXPECT_EQ(val, 5);
}

TEST(SPSCQueue, FullQueueRejectsPush) {
    SPSCQueue<int, 2> q;
    EXPECT_TRUE(q.push(5));
    EXPECT_FALSE(q.push(7));
}

TEST(SPSCQueue, EmptyQueueRejectsPop) {
    SPSCQueue<int, 2> q;
    int val;
    EXPECT_FALSE(q.pop(val));
}

TEST(SPSCQueue, WrapAround) {
    SPSCQueue<int, 2> q;
    int val;
    EXPECT_TRUE(q.push(5));
    q.pop(val);
    EXPECT_EQ(val, 5);
    EXPECT_TRUE(q.push(8));
    q.pop(val);
    EXPECT_EQ(val, 8);
}

// --- Multi-threaded correctness ---

TEST(SPSCQueue, ProducerConsumerOrdering) {
    int N = 1000;
    SPSCQueue<int, 4> q;
    std::thread t([&]() {
        for(int i = 0; i < N; i++) {
            while(!q.push(i));
        }
    });
    int val;
    for(int i = 0; i < N; i++) {
        while(!q.pop(val));
        EXPECT_EQ(val, i);
    }
    t.join();
}

TEST(SPSCQueue, NoItemsLost) {
    int N = 1000;
    SPSCQueue<int, 8> q;
    std::thread t([&]() {
        for(int i = 0; i < N; i++) {
            while(!q.push(i));
        }
    });
    std::vector<int> res(N);
    for(int i = 0; i < N; i++) {
        while(!q.pop(res[i]));
    }
    t.join();
    for (int i = 0; i < N; i++) {
        EXPECT_EQ(res[i], i);
    }
}
