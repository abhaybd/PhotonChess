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
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include(GNUInstallDirs)
Include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

# Logging with Loguru

FetchContent_Declare(LoguruGitRepo
	GIT_REPOSITORY "https://github.com/emilk/loguru" # can be a filesystem path
	GIT_TAG        "master"
)
FetchContent_MakeAvailable(LoguruGitRepo) # defines target 'loguru::loguru'
link_libraries(loguru::loguru -ldl)

# Testing

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_library(photon_core STATIC)
add_library(Photon::Core ALIAS photon_core)
target_sources(photon_core PRIVATE
    src/core.cpp
    src/util.cpp
    src/moves.cpp
)
target_include_directories(photon_core PUBLIC "include")
target_link_libraries(photon_core PRIVATE loguru::loguru)

add_executable(photon src/main.cpp)
target_link_libraries(photon photon_core)

add_executable(tests
    test/CoreTest.cpp
    test/MoveGenTest.cpp
)
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    photon_core
)
target_include_directories(tests PRIVATE "include")
