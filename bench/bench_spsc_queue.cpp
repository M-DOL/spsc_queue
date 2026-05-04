#include <benchmark/benchmark.h>
#include "spsc_queue.hpp"
#include <thread>
#include <mutex>
#include <pthread.h>
#ifdef __APPLE__
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#endif

static constexpr int kConsumerCore = 0;
static constexpr int kProducerCore = 2; // avoid SMT siblings on Intel

static void pin_thread(int core) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#elif defined(__APPLE__)
    // macOS has no hard pinning; affinity tags hint the scheduler to keep
    // threads with different tags on different physical cores
    thread_affinity_policy_data_t policy = {core};
    thread_policy_set(pthread_mach_thread_np(pthread_self()),
                      THREAD_AFFINITY_POLICY,
                      reinterpret_cast<thread_policy_t>(&policy),
                      THREAD_AFFINITY_POLICY_COUNT);
#endif
}

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
    pin_thread(kConsumerCore);
    std::atomic<bool> flag {true};
    SPSCQueue<int, 1024> queue;
    int val = 5;
    std::thread producer = std::thread([&]() {
        pin_thread(kProducerCore);
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
    pin_thread(kConsumerCore);
    SPSCQueue<int, 1024> q1, q2;
    std::atomic<bool> flag{true};
    int val = 0;

    std::thread worker([&]() {
        pin_thread(kProducerCore);
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
    pin_thread(kConsumerCore);
    std::mutex mtx;
    std::array<int, 1024> buffer;
    size_t head = 0;
    size_t tail = 0;
    std::atomic<bool> flag {true};
    int val = 5;
    std::thread producer = std::thread([&]() {
        pin_thread(kProducerCore);
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

// Throughput across capacities — each value instantiates a distinct template
template <std::size_t N>
static void run_throughput(benchmark::State& state) {
    pin_thread(kConsumerCore);
    std::atomic<bool> flag{true};
    SPSCQueue<int, N> queue;
    int val = 5;
    std::thread producer([&]() {
        pin_thread(kProducerCore);
        while (flag.load(std::memory_order_relaxed)) queue.push(val);
    });
    for (auto _ : state) queue.pop(val);
    flag.store(false, std::memory_order_relaxed);
    producer.join();
    state.SetItemsProcessed(state.iterations());
}

static void BM_Throughput_VaryingCapacity(benchmark::State& state) {
    switch (state.range(0)) {
        case    64: run_throughput<   64>(state); break;
        case   256: run_throughput<  256>(state); break;
        case  1024: run_throughput< 1024>(state); break;
        case  4096: run_throughput< 4096>(state); break;
        case 16384: run_throughput<16384>(state); break;
        case 65536: run_throughput<65536>(state); break;
    }
}
BENCHMARK(BM_Throughput_VaryingCapacity)->RangeMultiplier(4)->Range(64, 65536);

BENCHMARK_MAIN();
