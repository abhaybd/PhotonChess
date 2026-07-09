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
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto -DNDEBUG")

include(GNUInstallDirs)
Include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

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
        GIT_REPOSITORY https://github.com/andersfylling/uci.git
        GIT_TAG master
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

add_library(photon_core STATIC)
add_library(Photon::Core ALIAS photon_core)
target_sources(photon_core PRIVATE
    src/core.cpp
    src/util.cpp
    src/moves.cpp
    src/zobrist.cpp
)
target_include_directories(photon_core PUBLIC "include")
target_link_libraries(photon_core PRIVATE loguru::loguru)

add_library(photon_engine STATIC)
add_library(Photon::Engine ALIAS photon_engine)
target_sources(photon_engine PRIVATE
    src/engine/eval.cpp
    src/engine/heuristic.cpp
    src/engine/move_ordering.cpp
)
target_include_directories(photon_engine PUBLIC "include")
target_link_libraries(photon_engine PUBLIC photon_core)
target_link_libraries(photon_engine PRIVATE loguru::loguru)

add_executable(photon src/photon_main.cpp)
target_link_libraries(photon photon_engine loguru::loguru)
target_link_libraries(photon uci)

add_executable(eret src/engine/ERET.cpp)
target_link_libraries(eret photon_engine loguru::loguru)
target_link_libraries(eret uci)

add_executable(tests
    test/CoreTest.cpp
    test/MoveGenTest.cpp
    test/UtilTest.cpp
    test/EngineTest.cpp
)
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    photon_engine
)
target_include_directories(tests PRIVATE "include")
