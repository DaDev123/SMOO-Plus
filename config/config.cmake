set(LINKFLAGS -nodefaultlibs)
set(LLDFLAGS --no-demangle --gc-sections)

set(OPTIMIZE_OPTIONS_DEBUG -O2 -gdwarf-4)
set(OPTIMIZE_OPTIONS_RELEASE -O3 -ffast-math -flto)
set(WARN_OPTIONS -Werror=return-type)

set(DEFINITIONS "-DSEAD_RTTI_SOURCE_DYNAMIC")
set(INCLUDES include include/agl)

set(ASM_OPTIONS "")
set(C_OPTIONS -ffunction-sections -fdata-sections)
set(CXX_OPTIONS "")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(TARGET_IS_STATIC FALSE)
set(MODULE_NAME BTT-Studio)
set(TITLE_ID 0x0100000000010000)
set(MODULE_BINARY subsdk4)
set(SDK_PAST_1900 FALSE)
set(USE_SAIL TRUE)

set(TRAMPOLINE_POOL_SIZE 0x20)
set(BAKE_SYMBOLS FALSE)

set(HAKKUN_ADDONS)
