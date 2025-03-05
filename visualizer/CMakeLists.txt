cmake_minimum_required(VERSION 3.10)
project(NanoVDBRenderer)

# Configure C++
set(CMAKE_CXX_STANDARD 20) # For format
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure CUDA
enable_language(CUDA)
set(CMAKE_CUDA_ARCHITECTURES 75)
string(APPEND CMAKE_CUDA_FLAGS " --extended-lambda") # For NanoVDB/cuda/DeviceBuffer

# External Dependencies
# OpenVDB
list(APPEND CMAKE_MODULE_PATH "/usr/local/lib/cmake/OpenVDB")
find_package(OpenVDB REQUIRED)
# GLFW
find_package(glfw3 REQUIRED)
# OpenGL
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
# Blosc
find_package(Blosc REQUIRED)

###############################################################
# Main Executable
add_executable(render  ${CMAKE_SOURCE_DIR}/src/glad.c ${CMAKE_SOURCE_DIR}/src/generateImage.cu ${CMAKE_SOURCE_DIR}/src/render.cu)

target_link_libraries( render PRIVATE
    OpenVDB::openvdb
    glfw
    OpenGL::GL
    Blosc::blosc
)

target_include_directories(render PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(render PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
    NANOVDB_USE_BLOSC
)