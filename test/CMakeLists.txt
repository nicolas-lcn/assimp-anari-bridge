cmake_minimum_required(VERSION 3.15)
project(test_bridge)

add_executable(test_bridge test.cpp)

# Link with your static bridge library
target_link_libraries(test_bridge PRIVATE assimp_anari_bridge)

/*
# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Assimp
find_package(assimp REQUIRED)

# Find ANARI (from Khronos SDK or installed path)
find_package(anari REQUIRED)

# Add your source file(s)
add_executable(test_bridge main.cpp)

# Include directories (optional if the packages already handle it)
target_include_directories(test_bridge PRIVATE
    ${ASSIMP_INCLUDE_DIRS}
)

# Link the required libraries
target_link_libraries(test_bridge PRIVATE
    ${ASSIMP_LIBRARIES}
    anari::anari
)
*/
