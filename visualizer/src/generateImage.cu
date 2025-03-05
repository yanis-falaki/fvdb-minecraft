#include <nanovdb/NanoVDB.h>
#include <nanovdb/math/Math.h>
#include <nanovdb/math/Ray.h>
#include <nanovdb/math/HDDA.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>

#include "generateImage.cuh"

using Vec3f = nanovdb::math::Vec3f;

template<typename RayT, typename AccT>
inline __hostdev__ bool firstActiveCustom(RayT& ray, AccT& acc, nanovdb::math::Coord &ijk, float& t)
{
    if (!ray.clip(acc.root().bbox()) || ray.t1() > 1e20) {// clip ray to bbox
        return false;// missed or undefined bbox
    }
    static const float Delta = 0.001f;// forward step-size along the ray to avoid getting stuck
    ray.setMinTime(ray.t0() - 0.0001f);// step back a small delta to avoid missing a first voxel at the edge of the bbox
    t = ray.t0();// initiate time
    ijk = RoundDown<nanovdb::math::Coord>(ray.start()); // first voxel inside bbox
    for (nanovdb::math::HDDA<RayT, nanovdb::math::Coord> hdda(ray, acc.getDim(ijk, ray)); !acc.isActive(ijk); hdda.update(ray, acc.getDim(ijk, ray))) {
        if (!hdda.step()) return false;// leap-frog HDDA and exit if ray bound is exceeded
        t = hdda.time() + Delta;// update time
        ijk = RoundDown<nanovdb::math::Coord>( ray(t) );// update ijk
    }
    return true;
}

__global__ void GLCUDAWriteToTex(nanovdb::Int32Grid* grid, cudaSurfaceObject_t cuda_surface_write,
                    size_t imgWidth, size_t imgHeight,
                    Vec3f dirVec, Vec3f rightVec, Vec3f upVec,
                    Vec3f posVec, float focalLength, float hlfViewPlaneWidth) {
    
    // Calculate row and column ids, as well as element offset
    uint32_t rowId = blockIdx.y * blockDim.y + threadIdx.y;

    // Calculate column id and pixel pointer
    uint32_t colId = blockIdx.x * blockDim.x + threadIdx.x;

    if (rowId >= imgHeight || colId >= imgWidth) {
        return;
    }

    // Map from pixel coordinates to screen space coords, calculate rayDirection
    float screen_i = (((float)colId / imgWidth) - 0.5f) * hlfViewPlaneWidth; // horizontal displacement
    float screen_j = (((float)rowId / imgHeight) - 0.5f) * hlfViewPlaneWidth; // vertical displacement
    Vec3f rayDir = (dirVec * focalLength + rightVec * screen_i + upVec * screen_j).normalize();

    // Create Ray to be used for tracing
    float t;
    float startTime = 0.01f;
    float endTime = 500.0f;
    nanovdb::math::Ray<float> ray(posVec, rayDir, startTime, endTime);

    // Trace Ray
    auto acc = grid->getAccessor();
    nanovdb::math::Coord coord;
    //bool hit = nanovdb::math::firstActive(ray, acc, coord, t); causes artifacts
    bool hit = firstActiveCustom(ray, acc, coord, t);

    if (!hit) {
        surf2Dwrite(make_uchar4(128, 128, 128, 255), cuda_surface_write, colId * sizeof(uchar4), rowId);
    } else {
        int value = acc.getValue(coord);
        surf2Dwrite(make_uchar4((value * 35) % 256, (value * 28) % 256, (value * 15) % 256, 255), cuda_surface_write, colId * sizeof(uchar4), rowId);
    }
}