cmake_minimum_required(VERSION 3.10)
project(DataSerializers)

# Core configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(serialize_world_regions serialize_world_regions.cpp)

list(APPEND CMAKE_MODULE_PATH "/usr/local/lib/cmake/OpenVDB")
find_package(Blosc REQUIRED)
find_package(OpenVDB REQUIRED)
find_package(ZLIB REQUIRED)

target_include_directories(serialize_world_regions PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../minecraft-serialization
    ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(serialize_world_regions PRIVATE 
    OpenVDB::openvdb
    ${ZLIB_LIBRARIES}
    Blosc::blosc
)

target_compile_definitions(serialize_world_regions PRIVATE
    NANOVDB_USE_OPENVDB
    ROOT_DIR="${CMAKE_SOURCE_DIR}/../.."
    NANOVDB_USE_BLOSC
)