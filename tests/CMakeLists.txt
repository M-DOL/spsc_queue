include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

add_executable(spsc_tests
  test_spsc_queue.cpp
)

target_link_libraries(spsc_tests
  PRIVATE spsc_queue gtest_main
)

include(GoogleTest)
gtest_discover_tests(spsc_tests)
