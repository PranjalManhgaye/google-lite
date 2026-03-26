cmake_minimum_required(VERSION 3.16)
project(google_lite_search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(search_engine
  src/core/normalizer.cpp
  src/core/tokenizer.cpp
  src/index/inverted_index.cpp
  src/io/wiki_loader.cpp
  src/io/parallel_index_builder.cpp
  src/io/doc_store.cpp
  src/crawl/html_extractor.cpp
  src/crawl/crawler.cpp
  src/index/index_snapshot.cpp
  src/query/query_parser.cpp
  src/query/query_executor.cpp
  src/ranking/bm25_ranker.cpp
  src/suggest/trie.cpp
  src/suggest/spell_corrector.cpp
  src/search/query_cache.cpp
  src/search/search_service.cpp
  src/search/opensearch_client.cpp
  src/api/http_server.cpp
)

target_include_directories(search_engine PUBLIC src)
find_package(Threads REQUIRED)
target_link_libraries(search_engine PUBLIC Threads::Threads)

add_executable(search_cli src/app/main.cpp)
target_link_libraries(search_cli PRIVATE search_engine)

add_executable(bench_engine bench/bench_main.cpp)
target_link_libraries(bench_engine PRIVATE search_engine)

add_executable(api_bench bench/api_bench.cpp)
target_link_libraries(api_bench PRIVATE search_engine)

add_executable(crawl_index_tool src/app/crawl_index_tool.cpp)
target_link_libraries(crawl_index_tool PRIVATE search_engine)

add_executable(search_api src/app/search_api_main.cpp)
target_link_libraries(search_api PRIVATE search_engine)

enable_testing()
add_executable(engine_tests tests/test_main.cpp)
target_link_libraries(engine_tests PRIVATE search_engine)
add_test(NAME engine_tests COMMAND engine_tests)
