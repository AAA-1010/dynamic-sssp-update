cmake_minimum_required(VERSION 3.26)
project(PDC_SSSP_OpenMP LANGUAGES CXX)

# --- C++17 ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Enable OpenMP (MSVC = /openmp, GCC/Clang = -fopenmp) ---
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# --- Executable ---
add_executable(openmp_sssp
    main.cpp
    src/GraphCSR.cpp
    src/SsspUpdater.cpp    # now contains #pragma omp
)

target_include_directories(openmp_sssp PUBLIC include)
target_link_libraries(openmp_sssp PUBLIC OpenMP::OpenMP_CXX)
