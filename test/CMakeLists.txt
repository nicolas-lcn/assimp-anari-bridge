cmake_minimum_required(VERSION 3.15)
project(test_bridge)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Assimp
find_package(assimp REQUIRED)

# Find ANARI (from Khronos SDK or installed path)
find_package(anari REQUIRED)

# Add your source file(s)


set(LIB_DIR ${PROJECT_SOURCE_DIR}/build/src)
#find_package(assimp_anari_bridge PATHS ${LIB_DIR})
# build executable
add_executable(test_bridge test.cpp)
# Link with your static bridge library

# Include directories (optional if the packages already handle it)
target_include_directories(test_bridge PRIVATE
    ${ASSIMP_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include
)

# Link the required libraries
target_link_libraries(test_bridge PRIVATE
    ${ASSIMP_LIBRARIES}
    anari::anari
    assimp_anari_bridge
)
