cmake_minimum_required(VERSION 3.20)
project(SmooPlusLegacyCodecTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(legacy_protocol_tests LegacyProtocolTests.cpp)
target_include_directories(legacy_protocol_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../include)

enable_testing()
add_test(NAME legacy_protocol_tests COMMAND legacy_protocol_tests)

option(LEGACY_PROTOCOL_ENABLE_FUZZER "Build the libFuzzer codec target" OFF)
if(LEGACY_PROTOCOL_ENABLE_FUZZER)
    add_executable(legacy_protocol_fuzz LegacyProtocolFuzz.cpp)
    target_include_directories(legacy_protocol_fuzz PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../include)
    target_compile_options(legacy_protocol_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(legacy_protocol_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
endif()
