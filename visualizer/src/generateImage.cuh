#include <nanovdb/math/Math.h>
using Vec3f = nanovdb::math::Vec3f;

__global__ void GLCUDAWriteToTex(nanovdb::Int32Grid* grid, cudaSurfaceObject_t cuda_surface_write,
                   size_t imgWidth, size_t imgHeight,
                   Vec3f dirVec, Vec3f rightVec, Vec3f upVec,
                   Vec3f posVec, float focalLength, float hlfViewPlaneWidth);

__global__ void generateImage(nanovdb::Int32Grid* grid, uint8_t* devImgPtr,
    size_t pitch, size_t imgWidth, size_t imgHeight,
    Vec3f dirVec, Vec3f rightVec, Vec3f upVec,
    Vec3f posVec, float focalLength, float hlfViewPlaneWidth);
