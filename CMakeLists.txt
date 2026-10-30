cmake_minimum_required(VERSION 3.20)
project(crosstalk VERSION 0.0.0 LANGUAGES C)

if(DEFINED CMAKE_CXX_COMPILER)
    set(CROSSTALK_CXX_COMPILER_SEEN "${CMAKE_CXX_COMPILER}")
endif()

list(APPEND CMAKE_MODULE_PATH "$ENV{HOME}/.config/cmake")
include(std)
set(STD_PROJ_PREFIX "crosstalk")

set(CROSSTALKRELEASE "0.1")

set(STEAM_SDK "$ENV{HOME}/git/libdangling/steamworks_sdk/sdk")
set(STEAM_REDIST "${STEAM_SDK}/redistributable_bin")
set(STEAM_TICKET "${STEAM_SDK}/public/steam/lib/win64")

set(DISCORD_SDK_REV "1.10.18247")
set(DISCORD_SDK "${CMAKE_CURRENT_SOURCE_DIR}/DiscordSocialSdk-${DISCORD_SDK_REV}/discord_social_sdk")
set(DISCORD_INC "${DISCORD_SDK}/include")
set(DISCORD_LIB "${DISCORD_SDK}/lib/release")
set(DISCORD_BIN "${DISCORD_SDK}/bin/release")
set(DISCORD_KRISP_MODELS
    krisp-bvc-o-pro-v3.kef
    krisp-nc-o-lite-v1.kef
    krisp-nc-o-med-v7.kef
    krisp-nc-o-nb-v2.kef
    krisp-vad-o-v2.kef
)

set(CROSSTALK_SRC_LINUX
    src/core/crosstalk.c
    src/core/cmdline.c
    src/core/svg.c
    src/core/linux/scan.c
    src/core/shot.c
    src/core/watch.c
    src/core/task.c
    src/core/fetch.c
    src/emu/linux/net.c
    src/ui/window.c
    src/ui/cell.c
    src/ui/guide.c
    src/ui/key.c
    src/core/main.c
    src/net/relay.c
    src/game/chat.c
    src/game/hub.c
    src/game/music.c
    src/game/totals.c
    src/game/fx/sfx.c
    src/eda/clk/clk.c
    src/eda/ic/fpga/build.c
    src/eda/ic/fpga/emu.c
    src/eda/ic/fpga/fpga.c
    src/eda/ic/fpga/run.c
    src/eda/io/cast.c
    src/eda/io/display.c
    src/eda/io/midi.c
    src/eda/io/receptacle.c
    src/eda/io/scancode.c
    src/eda/io/pty.c
    src/eda/io/serial.c
    src/eda/logic/probe.c
    src/eda/logic/stream.c
    src/eda/script/ast.c
    src/eda/script/eval.c
    src/eda/script/lex.c
    src/eda/script/parse.c
    src/eda/script/script.c
    src/eda/sound/sound.c
    src/eda/ui/display/crt.c
    src/eda/ui/analyzer.c
    src/eda/ui/brainomatic.c
    src/eda/ui/calc.c
    src/eda/ui/feed.c
    src/eda/ui/pipeview.c
    src/eda/ui/progview.c
    src/eda/ui/regview.c
    src/eda/ui/stackview.c
    src/eda/ui/indicator.c
    src/eda/ui/scope.c
    src/eda/ui/memview.c
    src/eda/ui/profile.c
    src/eda/ui/term.c
    src/eda/ui/wshop_browser.c
    src/eda/ui/wshop_publish.c
    src/hdl/abi.c
    src/hdl/aig.c
    src/hdl/ast.c
    src/hdl/bake.c
    src/hdl/linux/cc.c
    src/reng/reng.c
    src/reng/progress.c
    src/reng/tasks.c
    src/re/re.c
    src/re/img.c
    src/re/db.c
    src/re/diff.c
    src/re/cc.c
    src/re/linux/spawn.c
    src/hdl/diag/diag.c
    src/hdl/diag/sv.c
    src/hdl/elab.c
    src/hdl/elab_eval.c
    src/hdl/elf.c
    src/hdl/type.c
    src/hdl/emit.c
    src/hdl/exec.c
    src/hdl/guard.c
    src/hdl/img.c
    src/hdl/ir.c
    src/hdl/level.c
    src/hdl/lex.c
    src/hdl/logic_gate.c
    src/hdl/lutmap.c
    src/hdl/netarr.c
    src/hdl/pack.c
    src/hdl/parse.c
    src/hdl/preproc.c
    src/hdl/proc.c
    src/hdl/systask.c
    src/game/progression/progression.c
    src/game/progression/queue.c
    src/game/progression/stat.c
    src/game/board.c
    src/game/caption.c
    src/game/menu.c
    src/game/save.c
    src/game/settings.c
    src/game/settings_menu.c
    src/net/discord.c
    src/net/inventory/inventory.c
    src/net/presence.c
    src/net/steam.c
    src/net/wire.c
    src/net/wshop/wshop.c
    src/wshop/item.c
    src/wshop/manifest.c
    src/wshop/mount.c
    src/wshop/publish.c
)

set(CROSSTALK_SRC_WIN
    src/core/crosstalk.c
    src/core/cmdline.c
    src/core/svg.c
    src/core/scan.c
    src/core/shot.c
    src/core/watch.c
    src/core/task.c
    src/core/fetch.c
    src/emu/windows/net.c
    src/ui/window.c
    src/ui/cell.c
    src/ui/guide.c
    src/ui/key.c
    src/core/main.c
    src/net/relay.c
    src/game/chat.c
    src/game/hub.c
    src/game/music.c
    src/game/totals.c
    src/game/fx/sfx.c
    src/eda/clk/clk.c
    src/eda/ic/fpga/build.c
    src/eda/ic/fpga/emu.c
    src/eda/ic/fpga/fpga.c
    src/eda/ic/fpga/run.c
    src/eda/io/cast.c
    src/eda/io/display.c
    src/eda/io/midi.c
    src/eda/io/receptacle.c
    src/eda/io/scancode.c
    src/eda/io/pty.c
    src/eda/io/serial.c
    src/eda/logic/probe.c
    src/eda/logic/stream.c
    src/eda/script/ast.c
    src/eda/script/eval.c
    src/eda/script/lex.c
    src/eda/script/parse.c
    src/eda/script/script.c
    src/eda/sound/sound.c
    src/eda/ui/display/crt.c
    src/eda/ui/analyzer.c
    src/eda/ui/brainomatic.c
    src/eda/ui/calc.c
    src/eda/ui/feed.c
    src/eda/ui/pipeview.c
    src/eda/ui/progview.c
    src/eda/ui/regview.c
    src/eda/ui/stackview.c
    src/eda/ui/indicator.c
    src/eda/ui/scope.c
    src/eda/ui/memview.c
    src/eda/ui/profile.c
    src/eda/ui/term.c
    src/eda/ui/wshop_browser.c
    src/eda/ui/wshop_publish.c
    src/hdl/abi.c
    src/hdl/aig.c
    src/hdl/ast.c
    src/hdl/bake.c
    src/hdl/cc.c
    src/reng/reng.c
    src/reng/progress.c
    src/reng/tasks.c
    src/re/re.c
    src/re/img.c
    src/re/db.c
    src/re/diff.c
    src/re/cc.c
    src/re/spawn.c
    src/hdl/diag/diag.c
    src/hdl/diag/sv.c
    src/hdl/elab.c
    src/hdl/elab_eval.c
    src/hdl/elf.c
    src/hdl/type.c
    src/hdl/emit.c
    src/hdl/exec.c
    src/hdl/guard.c
    src/hdl/img.c
    src/hdl/ir.c
    src/hdl/level.c
    src/hdl/lex.c
    src/hdl/logic_gate.c
    src/hdl/lutmap.c
    src/hdl/netarr.c
    src/hdl/pack.c
    src/hdl/parse.c
    src/hdl/preproc.c
    src/hdl/proc.c
    src/hdl/systask.c
    src/game/progression/progression.c
    src/game/progression/queue.c
    src/game/progression/stat.c
    src/game/board.c
    src/game/caption.c
    src/game/menu.c
    src/game/save.c
    src/game/settings.c
    src/game/settings_menu.c
    src/net/discord.c
    src/net/inventory/inventory.c
    src/net/presence.c
    src/net/steam.c
    src/net/wire.c
    src/net/wshop/wshop.c
    src/wshop/item.c
    src/wshop/manifest.c
    src/wshop/mount.c
    src/wshop/publish.c
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_program(CROSSTALK_WINDRES NAMES x86_64-w64-mingw32-windres windres REQUIRED)
    set(CMAKE_RC_COMPILER "${CROSSTALK_WINDRES}" CACHE FILEPATH "resource compiler" FORCE)

    enable_language(RC)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/crosstalk.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/crosstalk.rc" @ONLY)
    set(CROSSTALK_SRC ${CROSSTALK_SRC_WIN} "${CMAKE_CURRENT_BINARY_DIR}/crosstalk.rc")
else()
    set(CROSSTALK_SRC ${CROSSTALK_SRC_LINUX})
endif()

add_executable(crosstalk ${CROSSTALK_SRC})
target_include_directories(crosstalk PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(CROSSTALK_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(MAKE_DIRECTORY "${CROSSTALK_GEN}")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/ml-dsa-87.pub" CROSSTALK_SIGN_PK_HEX HEX)
string(LENGTH "${CROSSTALK_SIGN_PK_HEX}" CROSSTALK_SIGN_PK_NIBBLES)
if(NOT CROSSTALK_SIGN_PK_NIBBLES EQUAL 5184)
    message(FATAL_ERROR "ml-dsa-87.pub must be 2592 raw bytes (AC_SIG_PUBKEY_LEN); got ${CROSSTALK_SIGN_PK_NIBBLES} hex nibbles")
endif()
string(REGEX REPLACE "(..)" "0x\\1," CROSSTALK_SIGN_PK_ARR "${CROSSTALK_SIGN_PK_HEX}")
file(WRITE "${CROSSTALK_GEN}/crosstalk_sign_pubkey.h" "#ifndef CROSSTALK_SIGN_PUBKEY_GEN_H\n#define CROSSTALK_SIGN_PUBKEY_GEN_H\n#include <stdint.h>\nstatic const uint8_t crosstalk_sign_pubkey_bytes[] = { ${CROSSTALK_SIGN_PK_ARR} };\n#endif\n")

target_include_directories(crosstalk PRIVATE "${CROSSTALK_GEN}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.dll.a")

    find_path(CROSSTALK_DANGLING_INC dangling/core/err.h REQUIRED)

    find_library(CROSSTALK_DANGLING_LIB NAMES dangling REQUIRED)

    target_include_directories(crosstalk SYSTEM PRIVATE
        ${CROSSTALK_DANGLING_INC}
        ${DISCORD_INC}
    )
    target_compile_options(crosstalk PRIVATE -Werror=incompatible-pointer-types)
    target_link_libraries(crosstalk PRIVATE
        -Wl,--start-group
        ${CROSSTALK_DANGLING_LIB}
        winpthread
        pthread
        m
        -Wl,--end-group
        setupapi
        runtimeobject
        "${STEAM_REDIST}/win64/steam_api64.lib"
        "${STEAM_TICKET}/sdkencryptedappticket64.lib"
        "${DISCORD_LIB}/discord_partner_sdk.lib"
        winhttp
        ole32
        oleaut32
        bcrypt
        crypt32
        wintrust
        advapi32
        ws2_32
        gdi32
        winmm
        user32
        shell32
        kernel32
        xmllite
        uuid
    )
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_link_options(crosstalk PRIVATE -static -Wno-unused-command-line-argument)
    else()
        target_link_options(crosstalk PRIVATE -static)
    endif()
else()
    find_library(CROSSTALK_STEAM_API_LIB NAMES steam_api REQUIRED)
    find_library(CROSSTALK_STEAM_TICKET_LIB NAMES sdkencryptedappticket REQUIRED)
    find_library(CROSSTALK_DANGLING_LIB NAMES dangling REQUIRED)

    target_include_directories(crosstalk SYSTEM PRIVATE ${DISCORD_INC})
    target_link_libraries(crosstalk PRIVATE
        ${CROSSTALK_DANGLING_LIB}
        "${CROSSTALK_STEAM_API_LIB}"
        "${CROSSTALK_STEAM_TICKET_LIB}"
        "${DISCORD_LIB}/libdiscord_partner_sdk.so"
        pthread
        m
    )
    target_compile_options(crosstalk PRIVATE -Werror=incompatible-pointer-types)
    target_compile_definitions(crosstalk PRIVATE _GNU_SOURCE)
    set_target_properties(crosstalk PROPERTIES BUILD_RPATH "$ORIGIN")
endif()

target_link_options(crosstalk PRIVATE -Wl,-Map=$<TARGET_FILE_DIR:crosstalk>/crosstalk.map)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CROSSTALK_DANGEM_BUILD_TYPE)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CROSSTALK_DANGEM_PFORM)
string(REPLACE "-Wl,--exclude-libs,ALL" "" CROSSTALK_DANGEM_LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${CROSSTALK_DANGEM_BUILD_TYPE}}")
file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/dangem/build-${CROSSTALK_DANGEM_PFORM}.lst"
    CONTENT "root ${CMAKE_CURRENT_SOURCE_DIR}
bin ${CMAKE_BINARY_DIR}
tgt ${CMAKE_C_COMPILER_TARGET}
steam ${STEAM_SDK}
ldginc ${CROSSTALK_DANGLING_INC}
ldglib ${CROSSTALK_DANGLING_LIB}
out $<TARGET_FILE_NAME:crosstalk>
std $<TARGET_PROPERTY:crosstalk,C_STANDARD>
cflagset ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CROSSTALK_DANGEM_BUILD_TYPE}}
ldflagset ${CROSSTALK_DANGEM_LDFLAGS}
libset ${CMAKE_C_STANDARD_LIBRARIES}
src $<JOIN:$<TARGET_PROPERTY:crosstalk,SOURCES>,\nsrc >
def $<JOIN:$<TARGET_PROPERTY:crosstalk,COMPILE_DEFINITIONS>,\ndef >
inc $<JOIN:$<TARGET_PROPERTY:crosstalk,INCLUDE_DIRECTORIES>,\ninc >
cflag $<JOIN:$<TARGET_PROPERTY:crosstalk,COMPILE_OPTIONS>,\ncflag >
lib $<JOIN:$<TARGET_PROPERTY:crosstalk,LINK_LIBRARIES>,\nlib >
ldflag $<JOIN:$<TARGET_PROPERTY:crosstalk,LINK_OPTIONS>,\nldflag >
")

file(WRITE "${CMAKE_BINARY_DIR}/dangem/RELEASE" "${CROSSTALKRELEASE}\n")

add_custom_command(TARGET crosstalk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:crosstalk>/res"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt"
        "$<TARGET_FILE_DIR:crosstalk>/steam_appid.txt"
    COMMENT "Staging res and steam_appid next to binary"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_custom_command(TARGET crosstalk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${STEAM_REDIST}/win64/steam_api64.dll"
            "$<TARGET_FILE_DIR:crosstalk>/steam_api64.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${STEAM_TICKET}/sdkencryptedappticket64.dll"
            "$<TARGET_FILE_DIR:crosstalk>/sdkencryptedappticket64.dll"
        COMMENT "Staging steam_api64.dll and sdkencryptedappticket64.dll next to binary"
    )
    add_custom_command(TARGET crosstalk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_BIN}/discord_partner_sdk.dll"
            "$<TARGET_FILE_DIR:crosstalk>/discord_partner_sdk.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_BIN}/discord_krisp.dll"
            "$<TARGET_FILE_DIR:crosstalk>/discord_krisp.dll"
        COMMENT "Staging discord_partner_sdk.dll and discord_krisp.dll next to binary"
    )
    set(DISCORD_MODEL_DIR "${DISCORD_BIN}")
else()
    add_custom_command(TARGET crosstalk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CROSSTALK_STEAM_API_LIB}"
            "$<TARGET_FILE_DIR:crosstalk>/libsteam_api.so"
        COMMENT "Staging libsteam_api.so next to binary"
    )
    add_custom_command(TARGET crosstalk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_LIB}/libdiscord_partner_sdk.so"
            "$<TARGET_FILE_DIR:crosstalk>/libdiscord_partner_sdk.so"
        COMMENT "Staging libdiscord_partner_sdk.so next to binary"
    )
    set(DISCORD_MODEL_DIR "${DISCORD_LIB}")
endif()

foreach(DISCORD_MODEL IN LISTS DISCORD_KRISP_MODELS)
    add_custom_command(TARGET crosstalk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_MODEL_DIR}/${DISCORD_MODEL}"
            "$<TARGET_FILE_DIR:crosstalk>/${DISCORD_MODEL}"
        COMMENT "Staging ${DISCORD_MODEL} next to binary"
    )
endforeach()

set(CROSSTALK_EMU_SRC
    src/emu/main.c
    src/emu/emu.c
    src/emu/screen.c
    src/emu/toolbar.c
    src/emu/input.c
    src/emu/audio.c
    src/emu/cmdline.c
    src/ui/key.c
    src/eda/ic/fpga/emu.c
    src/eda/io/scancode.c
    src/hdl/abi.c
    src/hdl/img.c
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/dangcsaemu.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/dangcsaemu.rc" @ONLY)
    add_executable(emu ${CROSSTALK_EMU_SRC} src/emu/windows/net.c "${CMAKE_CURRENT_BINARY_DIR}/dangcsaemu.rc")
else()
    add_executable(emu ${CROSSTALK_EMU_SRC} src/emu/linux/net.c)
endif()

set_target_properties(emu PROPERTIES OUTPUT_NAME "dangcsaemu")

target_include_directories(emu PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_include_directories(emu PRIVATE "${CROSSTALK_GEN}")
    target_include_directories(emu SYSTEM PRIVATE
        ${CROSSTALK_DANGLING_INC}
        ${DISCORD_INC}
    )
    target_compile_options(emu PRIVATE -Werror=incompatible-pointer-types)
    target_link_libraries(emu PRIVATE
        -Wl,--start-group
        ${CROSSTALK_DANGLING_LIB}
        winpthread
        pthread
        m
        -Wl,--end-group
        setupapi
        runtimeobject
        "${STEAM_REDIST}/win64/steam_api64.lib"
        "${STEAM_TICKET}/sdkencryptedappticket64.lib"
        "${DISCORD_LIB}/discord_partner_sdk.lib"
        winhttp
        ole32
        oleaut32
        bcrypt
        crypt32
        wintrust
        advapi32
        ws2_32
        opengl32
        gdi32
        winmm
        user32
        shell32
        kernel32
        xmllite
        uuid
    )
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_link_options(emu PRIVATE -static -Wno-unused-command-line-argument)
    else()
        target_link_options(emu PRIVATE -static)
    endif()
else()
    target_include_directories(emu SYSTEM PRIVATE ${DISCORD_INC})
    target_link_libraries(emu PRIVATE
        ${CROSSTALK_DANGLING_LIB}
        "${CROSSTALK_STEAM_API_LIB}"
        "${CROSSTALK_STEAM_TICKET_LIB}"
        "${DISCORD_LIB}/libdiscord_partner_sdk.so"
        pthread
        m
    )
    target_compile_options(emu PRIVATE -Werror=incompatible-pointer-types)
    target_compile_definitions(emu PRIVATE _GNU_SOURCE)
    set_target_properties(emu PROPERTIES BUILD_RPATH "$ORIGIN")
endif()

target_link_options(emu PRIVATE -Wl,-Map=$<TARGET_FILE_DIR:emu>/dangcsaemu.map)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_custom_command(TARGET emu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${STEAM_REDIST}/win64/steam_api64.dll"
            "$<TARGET_FILE_DIR:emu>/steam_api64.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${STEAM_TICKET}/sdkencryptedappticket64.dll"
            "$<TARGET_FILE_DIR:emu>/sdkencryptedappticket64.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_BIN}/discord_partner_sdk.dll"
            "$<TARGET_FILE_DIR:emu>/discord_partner_sdk.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_BIN}/discord_krisp.dll"
            "$<TARGET_FILE_DIR:emu>/discord_krisp.dll"
        COMMENT "Staging runtime DLLs next to dangcsaemu"
    )
else()
    add_custom_command(TARGET emu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CROSSTALK_STEAM_API_LIB}"
            "$<TARGET_FILE_DIR:emu>/libsteam_api.so"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_LIB}/libdiscord_partner_sdk.so"
            "$<TARGET_FILE_DIR:emu>/libdiscord_partner_sdk.so"
        COMMENT "Staging runtime libs next to dangcsaemu"
    )
endif()
