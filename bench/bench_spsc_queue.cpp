#include <benchmark/benchmark.h>
#include "spsc_queue.hpp"
#include <thread>
#include <mutex>

// Baseline: raw push/pop throughput single-threaded
static void BM_PushPop_SingleThreaded(benchmark::State& state) {
    SPSCQueue<int, 1024> queue;
    int val = 1;
    for (auto _ : state) {
        queue.push(val);
        queue.pop(val);
    }
}
BENCHMARK(BM_PushPop_SingleThreaded);

// Core benchmark: producer and consumer on separate threads
static void BM_Throughput_Threaded(benchmark::State& state) {
    
    std::atomic<bool> flag {true};
    SPSCQueue<int, 1024> queue;
    int val = 5;
    std::thread producer = std::thread([&]() {
        while(flag.load(std::memory_order_relaxed)) queue.push(val);
    });
    for (auto _ : state) {
        queue.pop(val);
    }
    flag.store(false, std::memory_order_relaxed);
    producer.join();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Throughput_Threaded);

// Latency: ping-pong a value between two queues on two threads
static void BM_Latency_Roundtrip(benchmark::State& state) {
    SPSCQueue<int, 1024> q1, q2;
    std::atomic<bool> flag{true};
    int val = 0;

    // worker: receive on q1, send back on q2
    std::thread worker([&]() {
        int v;
        while (flag.load(std::memory_order_relaxed)) {
            if (q1.pop(v)) q2.push(v);
        }
    });

    for (auto _ : state) {
        q1.push(val);
        while (!q2.pop(val)); // spin until response
    }

    flag.store(false, std::memory_order_relaxed);
    worker.join();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Latency_Roundtrip);

// Baseline: producer and consumer on separate threads with a mutex
static void BM_Mutex(benchmark::State& state) {
    std::mutex mtx;
    std::array<int, 1024> buffer;
    size_t head = 0;
    size_t tail = 0;
    std::atomic<bool> flag {true};
    int val = 5;
    std::thread producer = std::thread([&]() {
        while(flag.load(std::memory_order_relaxed)) {
            std::lock_guard<std::mutex> lock(mtx);
            if((tail + 1) % 1024 == head) continue;
            buffer[tail] = val;
            tail = (tail + 1) % 1024;
        }
    });
    for (auto _ : state) {
        while(true) {
            std::lock_guard<std::mutex> lock(mtx);
            if(tail != head){
                val = buffer[head];
                head = (head + 1) % 1024;
                break;
            }
        }
    }
    flag.store(false, std::memory_order_relaxed);
    producer.join();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Mutex);

BENCHMARK_MAIN();
