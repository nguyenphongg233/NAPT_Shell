cmake_minimum_required(VERSION 3.10)
project(TinyShell)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(TinyShell 
    src/main.cpp
    src/parser.cpp
    src/builtin.cpp
    src/process_mgr.cpp
    src/signal_hnd.cpp
    src/executor.cpp
    src/completion.cpp
)