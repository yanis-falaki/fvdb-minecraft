cmake_minimum_required(VERSION 3.10)
project(NBTParser)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find zlib
find_package(ZLIB REQUIRED)

# Add executables
add_executable(findBlock examples/findBlock.cpp) 
add_executable(serializeRegion examples/serializeRegion.cpp)

target_include_directories(findBlock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)
target_include_directories(serializeRegion PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(findBlock PRIVATE 
    ${ZLIB_LIBRARIES}
)
target_link_libraries(serializeRegion PRIVATE 
    ${ZLIB_LIBRARIES}
)

# Include OpenVDB
list(APPEND CMAKE_MODULE_PATH "/usr/local/lib/cmake/OpenVDB")
find_package(OpenVDB REQUIRED)
target_link_libraries(serializeRegion PRIVATE OpenVDB::openvdb)
target_link_libraries(findBlock PRIVATE OpenVDB::openvdb)
target_compile_definitions(serializeRegion PRIVATE "NANOVDB_USE_OPENVDB")

# Add PROJECT_SOURCE_DIR as a preprocessor definition for both
target_compile_definitions(findBlock PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(serializeRegion PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
