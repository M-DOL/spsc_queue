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

find_package(Boost QUIET)
if(Boost_FOUND)
  target_link_libraries(spsc_bench PRIVATE spsc_queue benchmark::benchmark Boost::boost)
  target_compile_definitions(spsc_bench PRIVATE HAVE_BOOST)
  message(STATUS "Boost found -- Boost comparison benchmark enabled")
else()
  target_link_libraries(spsc_bench PRIVATE spsc_queue benchmark::benchmark)
  message(STATUS "Boost not found -- Boost comparison benchmark disabled")
endif()
