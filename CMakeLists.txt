cmake_minimum_required(VERSION 3.16)
project(TenSure VERSION 1.0 LANGUAGES CXX)

# ------------------------------
# C++ standard
# ------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------
# Paths to external libraries
# ------------------------------
set(TACO_DIR ${CMAKE_SOURCE_DIR}/external/taco)
set(JSON_DIR ${CMAKE_SOURCE_DIR}/external/nlohmann_json)

# ------------------------------
# Custom target to build externals
# ------------------------------
add_custom_target(build_externals
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_externals.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building external dependencies (TACO)..."
)

# ------------------------------
# Include directories
# ------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${JSON_DIR}/include
)

# ------------------------------
# Source files for main fuzzer
# ------------------------------
file(GLOB_RECURSE FUZZER_SRC ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(FILTER FUZZER_SRC EXCLUDE REGEX ".*/taco_wrapper/.*")  # exclude backend .cpp files
list(FILTER FUZZER_SRC EXCLUDE REGEX ".*/finch_wrapper/.*") # exclude other backends
list(APPEND FUZZER_SRC ${CMAKE_SOURCE_DIR}/src/tensure/ThreadPool.cpp) # Find the ThreadPool implementation file


# ------------------------------
# Build main executable (no backend linkage)
# ------------------------------
add_executable(${PROJECT_NAME} ${FUZZER_SRC})

# Add dependencies for the multi-threaded fuzzer
target_link_libraries(${PROJECT_NAME} PRIVATE 
    # For std::thread, std::mutex, etc. (required by ThreadPool)
    pthread
    # For dlopen, dlsym, etc. (required by plugin loader in fuzzer_main.cpp)
    dl
    # For std::filesystem
    stdc++fs
)

# Allow main executable to export symbols to plugins if needed
set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS ON)

# ------------------------------
# Selective backend building options
# ------------------------------
option(BUILD_TACO "Build TACO backend" OFF)
option(BUILD_FINCH "Build Finch backend" OFF)

# ------------------------------
# TACO backend
# ------------------------------
if(BUILD_TACO)
    file(GLOB_RECURSE TACO_SRC
        ${CMAKE_SOURCE_DIR}/src/taco_wrapper/*.cpp
    )

    add_library(taco_wrapper SHARED ${TACO_SRC})

    # TACO backend depends on building external TACO lib

    # Try to locate libtaco.so automatically
    find_library(TACO_LIB taco PATHS
        ${TACO_DIR}/build/lib
        /usr/local/lib
        /usr/lib
        /opt/taco/lib
    )

    set(TACO_INCLUDE_DIR ${TACO_DIR}/include)
    set(TACO_BUILD_ROOT ${TACO_DIR}/build)
    set(TACO_INSTALL_PREFIX ${TACO_BUILD_ROOT})

    if(TACO_LIB)
        message(STATUS "Found existing TACO library at ${TACO_LIB}")
        target_link_libraries(taco_wrapper PRIVATE ${TACO_LIB})
    else()
        message(STATUS "TACO library not found. Configuring build from source...")
        include(ExternalProject)
        
        if(APPLE)
            set(LIB_EXT ".dylib")
        else()
            set(LIB_EXT ".so")
        endif()
        set(TACO_BUILT_LIB ${TACO_BUILD_ROOT}/lib/libtaco${LIB_EXT})
        
        ExternalProject_Add(taco_external
            SOURCE_DIR ${TACO_DIR}
            PREFIX ${TACO_BUILD_ROOT}/cmake_work
            BINARY_DIR ${TACO_BUILD_ROOT}
            CMAKE_ARGS
                -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release -- -j${NPROC}
            INSTALL_COMMAND ""
        )
        add_library(TacoLibUnknown SHARED IMPORTED)
        set_target_properties(TacoLibUnknown PROPERTIES IMPORTED_LOCATION ${TACO_BUILT_LIB})

        add_dependencies(TacoLibUnknown taco_external)
        target_link_libraries(taco_wrapper PRIVATE TacoLibUnknown)
    endif()
    target_include_directories(taco_wrapper PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${TACO_INCLUDE_DIR}
    )
endif()

# ------------------------------
# Finch backend
# ------------------------------
if(BUILD_FINCH)
    file(GLOB_RECURSE FINCH_SRC
        ${CMAKE_SOURCE_DIR}/src/finch_wrapper/*.cpp
    )
    # Include core utils to allow using shared comparison logic
    add_library(finch_wrapper SHARED ${FINCH_SRC} ${CMAKE_SOURCE_DIR}/src/tensure/utils.cpp)
    target_include_directories(finch_wrapper PUBLIC ${CMAKE_SOURCE_DIR}/include)
endif()

# ------------------------------
# Verbose build
# ------------------------------
set(CMAKE_VERBOSE_MAKEFILE ON)
