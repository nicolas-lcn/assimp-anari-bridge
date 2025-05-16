# src/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(assimp_anari_bridge)

# Collect all .cpp files in src/
file(GLOB_RECURSE BRIDGE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE BRIDGE_SOURCES "*.cpp")

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/) # .dll
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/) # .lib
#set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/) # .pdb

add_library(assimp_anari_bridge STATIC ${BRIDGE_HEADERS} ${BRIDGE_SOURCES})

# Include headers from ../include
target_include_directories(assimp_anari_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Find and link Assimp and ANARI
find_package(assimp REQUIRED)
find_package(anari REQUIRED)

# Ensure headers are visible to users of this library
target_include_directories(assimp_anari_bridge PUBLIC
    ${ASSIMP_INCLUDE_DIRS}
    ${ANARI_INCLUDE_DIRS}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/../include" PREFIX "Header Files" FILES ${BRIDGE_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${BRIDGE_SOURCES})



target_link_libraries(assimp_anari_bridge PUBLIC
    ${ASSIMP_LIBRARIES}
    anari::anari
)

#install(TARGETS assimp_anari_bridge DESTINATION lib)
#install(FILES ${BRIDGE_HEADERS} DESTINATION include/aab)
