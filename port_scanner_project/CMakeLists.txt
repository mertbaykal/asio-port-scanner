\
    cmake_minimum_required(VERSION 3.18)
    project(port_scanner LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # For Homebrew on Apple Silicon
    if(NOT DEFINED CMAKE_PREFIX_PATH)
      set(CMAKE_PREFIX_PATH "/opt/homebrew" CACHE PATH "Prefix path for package search")
    endif()

    find_package(Boost 1.66 REQUIRED COMPONENTS system)
    find_package(Threads REQUIRED)

    add_executable(scanner src/scanner.cpp)

    if (Boost_FOUND)
        target_include_directories(scanner PRIVATE ${Boost_INCLUDE_DIRS})
        target_link_libraries(scanner PRIVATE ${Boost_LIBRARIES} Threads::Threads)
    else()
        message(FATAL_ERROR "Boost not found. Install via Homebrew: brew install boost")
    endif()
