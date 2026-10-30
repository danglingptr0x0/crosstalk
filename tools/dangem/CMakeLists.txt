cmake_minimum_required(VERSION 3.20)
project(dangem VERSION 0.1.0 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "$ENV{HOME}/.config/cmake")
include(std)
set(STD_PROJ_PREFIX "dangem")

set(DANGEM_SRC
    src/core/core.c
    src/compile/list.c
    src/compile/compile.c
    src/ui/ui.c
    src/ui/menu.c
    src/ui/summary.c
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND DANGEM_SRC src/run/windows/run.c)
else()
    list(APPEND DANGEM_SRC src/run/linux/run.c)
endif()

add_executable(dangem ${DANGEM_SRC})
target_include_directories(dangem PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(dangem PRIVATE -Werror=incompatible-pointer-types)

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
