cmake_minimum_required(VERSION 3.26)
project(PDC_SSSP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(serial_sssp
    main.cpp
    src/GraphCSR.cpp
    src/SsspUpdater.cpp
 )

target_include_directories(serial_sssp PUBLIC include)


