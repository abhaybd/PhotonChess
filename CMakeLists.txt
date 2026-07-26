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
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto=auto -DNDEBUG")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
Include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

option(PHOTON_PROFILING_ENABLED "Enable the built-in scoped profiler (adds per-scope timing overhead)" OFF)

# Logging with Loguru (wrap in function to limit scope)

function(add_loguru)
    FetchContent_Declare(LoguruGitRepo
        GIT_REPOSITORY "https://github.com/emilk/loguru" # can be a filesystem path
        GIT_TAG        "master"
    )
    set(BUILD_SHARED_LIBS ON)
    set(LOGURU_STACKTRACES ON)
    FetchContent_MakeAvailable(LoguruGitRepo) # defines target 'loguru::loguru'
endfunction()
add_loguru()

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

# Profiling: when enabled, define PHOTON_PROFILING_ENABLED for all Photon targets defined
# below. When disabled, the PHOTON_PROFILE_* macros compile to nothing (zero runtime cost).
if(PHOTON_PROFILING_ENABLED)
    add_compile_definitions(PHOTON_PROFILING_ENABLED)
    message(STATUS "Photon profiling: ENABLED")
else()
    message(STATUS "Photon profiling: disabled")
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
    src/engine/eval.cpp
    src/engine/heuristic.cpp
    src/engine/move_ordering.cpp
    src/engine/transposition_table.cpp
    src/engine/killer.cpp
    src/engine/history.cpp
    src/engine/nullmove.cpp
)
target_include_directories(photon_engine PUBLIC "include")
target_link_libraries(photon_engine PUBLIC photon_core)
target_link_libraries(photon_engine PRIVATE loguru::loguru)

add_executable(photon src/photon_main.cpp)
target_link_libraries(photon photon_engine loguru::loguru)
target_link_libraries(photon uci)

add_executable(tests
    test/CoreTest.cpp
    test/MoveGenTest.cpp
    test/UtilTest.cpp
    test/EngineTest.cpp
    test/perft.cpp
    test/KillerHeuristicTest.cpp
)
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    photon_engine
)
target_include_directories(tests PRIVATE "include")
