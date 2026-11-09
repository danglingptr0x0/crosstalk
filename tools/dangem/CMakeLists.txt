cmake_minimum_required(VERSION 3.20)
project(dangem VERSION 0.1.0 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "$ENV{HOME}/.config/cmake")
include(std OPTIONAL RESULT_VARIABLE DANGEM_STD_FILE)
set(STD_PROJ_PREFIX "dangem")

if(NOT DANGEM_STD_FILE)
    message(STATUS "std: not found; building with CMake defaults")
endif()

set(DANGEM_SRC
    src/core/core.c
    src/compile/list.c
    src/compile/compile.c
    src/ui/ui.c
    src/ui/menu.c
    src/ui/summary.c
    src/run/run.c
)

add_executable(dangem ${DANGEM_SRC})
target_include_directories(dangem PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(dangem PRIVATE -Werror=incompatible-pointer-types)

string(TOLOWER "${CMAKE_SYSTEM_NAME}" DANGEM_PFORM)
target_compile_definitions(dangem PRIVATE DANGEM_RUN_PFORM="${DANGEM_PFORM}" DANGEM_RUN_EXE_EXT="${CMAKE_EXECUTABLE_SUFFIX}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(dangem PRIVATE DANGEM_RUN_BUILD_DIR="build-win" DANGEM_RUN_SHLIB_LOCAL=0 DANGEM_RUN_TERM_FORCE=1)
else()
    target_compile_definitions(dangem PRIVATE DANGEM_RUN_BUILD_DIR="build" DANGEM_RUN_SHLIB_LOCAL=1 DANGEM_RUN_TERM_FORCE=0)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.dll.a")
    find_path(DANGEM_DANGLING_INC dangling/core/err.h REQUIRED)
    find_library(DANGEM_DANGLING_LIB NAMES dangling REQUIRED)
    target_include_directories(dangem SYSTEM PRIVATE ${DANGEM_DANGLING_INC})
    target_link_libraries(dangem PRIVATE
        -Wl,--start-group
        ${DANGEM_DANGLING_LIB}
        winpthread
        pthread
        m
        -Wl,--end-group
        setupapi
        winmm
        gdi32
        runtimeobject
        bcrypt
        ws2_32
        advapi32
        user32
        shell32
        kernel32
    )
    target_link_options(dangem PRIVATE -static)
else()
    find_library(DANGEM_DANGLING_LIB NAMES libdangling.a REQUIRED)
    target_link_libraries(dangem PRIVATE ${DANGEM_DANGLING_LIB} pthread m)
    target_compile_definitions(dangem PRIVATE _GNU_SOURCE)
endif()
