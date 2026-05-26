cmake_minimum_required(VERSION 3.10)

# Set name of project
project(ray)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Preprocessor definitions
add_compile_definitions(
    DATRAW_DISABLE_NEON  # disable an ARM-specific hardware optimization
)

# Set all .cpp files to be source files
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Set up the executable (./cmake-build-<profile>/ray)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Use OpenMP when available. Apple Clang on macOS needs explicit hints for
# Homebrew's libomp installation; otherwise the program still builds and runs
# serially, which keeps the rendered output identical.
if(APPLE)
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE HOMEBREW_LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(HOMEBREW_LIBOMP_PREFIX)
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "${HOMEBREW_LIBOMP_PREFIX}/lib/libomp.dylib")
        include_directories(SYSTEM "${HOMEBREW_LIBOMP_PREFIX}/include")
    endif()
endif()

find_package(OpenMP COMPONENTS CXX)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
else()
    message(WARNING "OpenMP not found; building the ray tracer without parallel rendering.")
endif()

# Set compilation options for debugging (slow) and release (fast)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    $<$<CONFIG:Debug>:   -O0 -g>
    $<$<CONFIG:Release>: -O3>
)
