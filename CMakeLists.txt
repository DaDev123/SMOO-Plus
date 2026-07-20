cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/sys/cmake/toolchain.cmake")
project(SMOO-Plus-Speedrun VERSION 1.0 LANGUAGES ASM C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(config/config.cmake)

file(GLOB_RECURSE SOURCES_ASM ${PROJECT_SOURCE_DIR}/src/*.S)
file(GLOB_RECURSE SOURCES_C ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE SOURCES_CXX ${PROJECT_SOURCE_DIR}/src/*.cpp)
set(SOURCES ${SOURCES_ASM} ${SOURCES_C} ${SOURCES_CXX})
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE lib)
target_include_directories(${PROJECT_NAME} PRIVATE lib/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE lib/custom)
target_include_directories(${PROJECT_NAME} PRIVATE lib/OdysseyHeaders)

file(GLOB LIBS_OH LIST_DIRECTORIES true lib/OdysseyHeaders/*)
file(GLOB LIBS_CUST LIST_DIRECTORIES true lib/custom/*)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBS_CUST} ${LIBS_OH})

include(sys/cmake/module.cmake)
include(sys/cmake/deploy.cmake)
include(sys/cmake/sail.cmake)
usesail(${PROJECT_NAME})


target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUGLOG=${DEBUGLOG})
target_compile_definitions(${PROJECT_NAME} PRIVATE SERVERIP="${SERVERIP}")
target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDVER="${BUILDVER}")

target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_USER_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/include/imconfig.h")
