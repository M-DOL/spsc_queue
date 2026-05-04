include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(spsc_bench
  bench_spsc_queue.cpp
)

target_link_libraries(spsc_bench
  PRIVATE spsc_queue benchmark::benchmark
)
