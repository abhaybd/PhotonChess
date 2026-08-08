cmake_minimum_required(VERSION 3.16)
project(PhotonChess
        VERSION 1.0.0
        DESCRIPTION "Chess engine written in C++, built for easy integration."
        HOMEPAGE_URL "https://github.com/abhaybd/chesspp")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto=auto -DNDEBUG")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
Include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

option(PHOTON_PROFILING_ENABLED "Enable the built-in scoped profiler (adds per-scope timing overhead)" OFF)
set(PHOTON_TTABLE_SIZE "" CACHE STRING "2^PHOTON_TTABLE_SIZE is the number of entries in the transposition table")

if(EMSCRIPTEN)
    add_compile_options(-pthread)
    add_link_options(-pthread)
    if(NOT PHOTON_TTABLE_SIZE)
        set(PHOTON_TTABLE_SIZE 20)
    endif()
endif()

# Logging with Loguru (wrap in function to limit scope)

function(add_loguru)
    FetchContent_Declare(LoguruGitRepo
        GIT_REPOSITORY "https://github.com/emilk/loguru" # can be a filesystem path
        GIT_TAG        "master"
    )
    if(EMSCRIPTEN)
        set(BUILD_SHARED_LIBS OFF)
        set(LOGURU_STACKTRACES OFF)
    else()
        set(BUILD_SHARED_LIBS ON)
        set(LOGURU_STACKTRACES ON)
    endif()
    FetchContent_MakeAvailable(LoguruGitRepo) # defines target 'loguru::loguru'
    if(EMSCRIPTEN)
        # Emscripten's pthread name APIs leave unused locals in loguru.
        target_compile_options(loguru PRIVATE -Wno-error)
    endif()
endfunction()
add_loguru()

if(NOT EMSCRIPTEN)
    # UCI Library
    function(add_uci)
        FetchContent_Declare(
            UCI
            GIT_REPOSITORY https://github.com/abhaybd/uci.git
            GIT_TAG fba52e68b1b6c93b9420ee69f5dabf78540820a0
        )
        set(CMAKE_CXX_FLAGS) # disable warnings for uci
        FetchContent_MakeAvailable(UCI)
        target_link_libraries(uci -lpthread) # needs threading
        # add header directory to include path
        target_include_directories(uci PUBLIC ${uci_SOURCE_DIR}/include)
    endfunction()
    add_uci()

    # Testing
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.15.2
    )

    FetchContent_MakeAvailable(Catch2)
endif()

# Profiling: when enabled, define PHOTON_PROFILING_ENABLED for all Photon targets defined
# below. When disabled, the PHOTON_PROFILE_* macros compile to nothing (zero runtime cost).
if(PHOTON_PROFILING_ENABLED)
    add_compile_definitions(PHOTON_PROFILING_ENABLED)
    message(STATUS "Photon profiling: ENABLED")
else()
    message(STATUS "Photon profiling: disabled")
endif()

if(PHOTON_TTABLE_SIZE)
    message(STATUS "Photon ttable size: ${PHOTON_TTABLE_SIZE}")
    add_compile_definitions(PHOTON_TTABLE_SIZE=${PHOTON_TTABLE_SIZE})
endif()

add_library(photon_core STATIC)
add_library(Photon::Core ALIAS photon_core)
target_sources(photon_core PRIVATE
    src/core.cpp
    src/util.cpp
    src/moves.cpp
    src/zobrist.cpp
    src/profiling/profile.cpp
)
target_include_directories(photon_core PUBLIC "include")
target_link_libraries(photon_core PRIVATE loguru::loguru)

add_library(photon_engine STATIC)
add_library(Photon::Engine ALIAS photon_engine)
target_sources(photon_engine PRIVATE
    src/engine/engine.cpp
    src/engine/move_ordering/exchange.cpp
    src/engine/move_ordering/move_ordering.cpp
    src/engine/move_ordering/killer.cpp
    src/engine/move_ordering/history.cpp
    src/engine/search/transposition_table.cpp
    src/engine/search/nullmove.cpp
    src/engine/search/aspiration.cpp
    src/engine/search/lmr.cpp
    src/engine/static_eval/material.cpp
    src/engine/static_eval/static_eval.cpp
)
target_include_directories(photon_engine PUBLIC "include")
target_link_libraries(photon_engine PUBLIC photon_core)
target_link_libraries(photon_engine PRIVATE loguru::loguru)

add_library(photon_uci STATIC)
add_library(Photon::UCI ALIAS photon_uci)
target_sources(photon_uci PRIVATE
    src/engine/photon_uci.cpp
)
target_include_directories(photon_uci PUBLIC "include")
target_link_libraries(photon_uci PUBLIC photon_engine)
target_link_libraries(photon_uci PRIVATE loguru::loguru)
if(NOT EMSCRIPTEN)
    target_link_libraries(photon_uci PUBLIC -lpthread)
endif()

if(EMSCRIPTEN)
    add_executable(photon src/photon_wasm.cpp)
    target_link_libraries(photon photon_uci loguru::loguru)
    target_link_options(photon PRIVATE
        "-sPTHREAD_POOL_SIZE=1"
        "-sENVIRONMENT=web,worker"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sEXPORT_NAME=createPhoton"
        "-sEXPORTED_FUNCTIONS=_main,_photon_init,_photon_uci_cmd,_malloc,_free"
        "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString"
        "-sNO_EXIT_RUNTIME=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=67108864"
        "-sINVOKE_RUN=1"
        "-Wno-pthreads-mem-growth"
    )
    set_target_properties(photon PROPERTIES
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/web/public/engine"
    )
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/web/public/engine")
else()
    add_executable(photon src/photon_main.cpp)
    target_link_libraries(photon photon_uci uci loguru::loguru)

    add_executable(tests
        test/CoreTest.cpp
        test/MoveGenTest.cpp
        test/UtilTest.cpp
        test/EngineTest.cpp
        test/perft.cpp
        test/KillerHeuristicTest.cpp
        test/AspirationWindowTest.cpp
        test/SEETest.cpp
        test/MoveOrderingTest.cpp
        test/LMRTest.cpp
    )
    target_link_libraries(tests PRIVATE
        Catch2::Catch2WithMain
        photon_engine
    )
    target_include_directories(tests PRIVATE "include")
endif()
