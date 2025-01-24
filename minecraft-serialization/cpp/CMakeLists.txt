cmake_minimum_required(VERSION 3.10)
project(SerializerExample)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Fix for vscode preprocessing directive linting errors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find zlib
find_package(ZLIB REQUIRED)

# Add the source file
add_executable(serializer serializer.cpp)

# Include directories
target_include_directories(serializer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

# Link against zlib
target_link_libraries(serializer PRIVATE 
    ${ZLIB_LIBRARIES}
)

# Add PROJECT_SOURCE_DIR as a preprocessor definition
target_compile_definitions(serializer PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")