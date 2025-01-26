cmake_minimum_required(VERSION 3.10)
project(NBTExplorer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find zlib
find_package(ZLIB REQUIRED)

# Add executables for both NBTExplorer and findBlock
add_executable(NBTExplorer NBTExplorer.cpp)  # First executable
add_executable(findBlock findBlock.cpp)  # Second executable

# Include directories for both executables
target_include_directories(NBTExplorer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

target_include_directories(findBlock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

# Link against zlib for both executables
target_link_libraries(NBTExplorer PRIVATE 
    ${ZLIB_LIBRARIES}
)

target_link_libraries(findBlock PRIVATE 
    ${ZLIB_LIBRARIES}
)

# Add PROJECT_SOURCE_DIR as a preprocessor definition for both
target_compile_definitions(NBTExplorer PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(findBlock PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
