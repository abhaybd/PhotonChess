cmake_minimum_required(VERSION 3.16)
project(ChessPP
        VERSION 1.0.0
        DESCRIPTION "Chess engine written in C++, built for easy integration."
        HOMEPAGE_URL "https://github.com/abhaybd/chesspp")

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

add_library(chesspp_lib STATIC)
target_sources(chesspp_lib PRIVATE
    src/core.cpp
    src/util.cpp
)
target_include_directories(chesspp_lib PUBLIC "include")
target_link_libraries(chesspp_lib PRIVATE loguru::loguru)

add_executable(chesspp src/main.cpp)
target_link_libraries(chesspp chesspp_lib)

add_executable(tests
    test/CoreTest.cpp
)
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    chesspp_lib
)
target_include_directories(tests PRIVATE "include")
