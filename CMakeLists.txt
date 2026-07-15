cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/sys/cmake/toolchain.cmake")
project(${PROJNAME} VERSION 1.0 LANGUAGES ASM C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(config/config.cmake)

file(GLOB_RECURSE SOURCES_ASM ${PROJECT_SOURCE_DIR}/src/*.S)
file(GLOB_RECURSE SOURCES_C ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE SOURCES_CXX ${PROJECT_SOURCE_DIR}/src/*.cpp)
set(SOURCES ${SOURCES_ASM} ${SOURCES_C} ${SOURCES_CXX})
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(lib)

include_directories(lib/imgui)

include_directories(lib/custom)
include_directories(lib/custom/al)
include_directories(lib/custom/game)
include_directories(lib/custom/nn)
include_directories(lib/custom/rs)

include_directories(lib/OdysseyHeaders)
include_directories(lib/OdysseyHeaders/sead)
include_directories(lib/OdysseyHeaders/al)
include_directories(lib/OdysseyHeaders/agl)
include_directories(lib/OdysseyHeaders/game)
include_directories(lib/OdysseyHeaders/NintendoSDK)

include_directories(include)

include(sys/cmake/module.cmake)
include(sys/cmake/deploy.cmake)
include(sys/cmake/sail.cmake)
usesail(${PROJECT_NAME})

# SMOVER ?= 100
# BUILDVER ?= 101
# BUILDVERSTR ?= $(GIT_REV)
# DEBUGLOG ?= 0 # defaults to disable debug logger
# SERVERIP ?= 0.0.0.0 # put debug logger server IP here
target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUGLOG=${DEBUGLOG})
target_compile_definitions(${PROJECT_NAME} PRIVATE SERVERIP="${SERVERIP}")
target_compile_definitions(${PROJECT_NAME} PRIVATE SMOVER=${SMOVER})
target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDVER=${BUILDVER})
target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDVERSTR="${BUILDVERSTR}")

target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_USER_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/include/imconfig.h")
