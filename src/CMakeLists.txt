# src/CMakeLists.txt

# Collect all .cpp files in src/
file(GLOB_RECURSE BRIDGE_SOURCES "*.cpp")

add_library(assimp_anary_bridge STATIC ${BRIDGE_SOURCES})

# Include headers from ../include
target_include_directories(assimp_anary_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Find and link Assimp and ANARI
find_package(assimp REQUIRED)
find_package(anari REQUIRED)

target_link_libraries(assimp_anary_bridge PUBLIC
    ${ASSIMP_LIBRARIES}
    anari::anari
)

# Ensure headers are visible to users of this library
target_include_directories(assimp_anary_bridge PUBLIC
    ${ASSIMP_INCLUDE_DIRS}
)
