cmake_minimum_required(VERSION 3.10)
project(NBTExplorer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find zlib
find_package(ZLIB REQUIRED)

# Add executables for findBlock
add_executable(findBlock findBlock.cpp)  # Second executable

target_include_directories(findBlock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(findBlock PRIVATE 
    ${ZLIB_LIBRARIES}
)

# Add PROJECT_SOURCE_DIR as a preprocessor definition for both
target_compile_definitions(findBlock PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
