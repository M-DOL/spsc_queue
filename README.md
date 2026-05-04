# spsc_queue

A lock-free single-producer single-consumer queue in C++20, implemented as a fixed-size ring buffer with cache-line-aware layout.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Test

```bash
cd build && ctest --output-on-failure
```

## Benchmark

```bash
./build/bench/spsc_bench
```

## Design Notes

**Capacity strategy**
Fixed at compile time via template parameter. Buffer is stack allocated, size is known at compile time, and `% Capacity` can be optimized to `& (Capacity - 1)` for power-of-two sizes. Enforced via `static_assert`.

**Memory ordering**
- Producer: relaxed load of `tail_` (owned by producer), relaxed load of `head_` (space check only, no data access follows), release store on `tail_` after writing to buffer slot.
- Consumer: relaxed load of `head_` (owned by consumer), acquire load of `tail_` (pairs with producer's release, ensures buffer write is visible before slot is read), release store on `head_` after reading.

**False sharing mitigation**
`head_`, `tail_`, and `data_` are each aligned to 64-byte cache line boundaries via `alignas(64)`, preventing the producer and consumer from bouncing a shared cache line.

## Benchmark Results (Apple M1, 10 cores)

Producer pinned to core 2, consumer pinned to core 0. Eliminates OS scheduling noise and cache migration across runs.

| Benchmark | Time | Throughput |
|---|---|---|
| Single-threaded push/pop | 1.98 ns | — |
| Lock-free SPSC (threaded) | 38.3 ns | 26.2M items/sec |
| Roundtrip latency | 78.1 ns | 12.9M items/sec |
| Boost lockfree::spsc_queue (throughput) | 30.3 ns | 33.1M items/sec |
| Boost lockfree::spsc_queue (latency) | 117 ns | 8.52M items/sec |
| Mutex ring buffer (threaded) | 162 ns | 6.19M items/sec |

Boost has higher throughput (33.1M vs 26.2M items/sec), likely due to batched index updates reducing atomic traffic. This queue has lower roundtrip latency (78.1 vs 117 ns) — each operation is published immediately on release store with no batching delay.

**~4.2x throughput improvement over a mutex-protected ring buffer.**

On Linux, `pthread_setaffinity_np` hard-pins each thread to its core. On macOS, a Mach affinity tag hints the scheduler to keep threads on separate physical cores (no hard guarantee).