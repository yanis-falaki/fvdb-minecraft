#include <format>
#include <iostream>
#include <tuple>

#include <openvdb/openvdb.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#include <nanovdb/util/IO.h>
#include <nanovdb/cuda/DeviceBuffer.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <cuda_gl_interop.h>

#include "generateImage.cuh"


using Vec3f = typename nanovdb::math::Vec3f;

struct CameraData {
    uint32_t mImgHeight; // in pixels
    uint32_t mImgWidth; // in pixels
    uint32_t mImgChannels = 3 ; // 3 channels for RGB

    // extrinsics
    Vec3f mPosition;
    Vec3f mDirection;
    Vec3f mUp;
    Vec3f mRight;
    float mYaw;
    float mPitch;

    // intrinsics
    float mFOV = 90.0f;
    float mFocalLength = 1.0f;
    float mHlfViewPlaneWidth;
    float mFarClip = 500.0f;

    float mMovementSpeed = 1.0f;
    float mSwivelSensitivity = 0.07f;

    CameraData() = delete;

    CameraData(Vec3f position, Vec3f target, uint32_t imgWidth, uint32_t imgHeight) {
        mPosition = position;
        mImgWidth = imgWidth;
        mImgHeight = imgHeight;
        mDirection = (target - position).normalize();
        mHlfViewPlaneWidth = tan((mFOV / 2) * nanovdb::math::pi<float>() / 180.0f) * mFocalLength;
        mRight = Vec3f{0, 1, 0}.cross(mDirection).normalize();
        mUp = mDirection.cross(mRight);

        mYaw = std::atan2(mDirection[2], mDirection[0]) * 180.0f / nanovdb::math::pi<float>();
        mPitch = std::asin(mDirection[1]) * 180.0f / nanovdb::math::pi<float>();
    }

    void computeDirection(){
        const float conversionFactor = nanovdb::math::pi<float>() / 180.0f;
        float radYaw = mYaw * conversionFactor;
        float radPitch = mPitch * conversionFactor;

        mDirection[0] = cos(radYaw) * cos(radPitch);
        mDirection[1] = sin(radPitch);
        mDirection[2] = sin(radYaw) * cos(radPitch);

        mRight = Vec3f{0, 1, 0}.cross(mDirection).normalize();
        mUp = mDirection.cross(mRight);
    }
};

struct WindowUserData {
    CameraData* cameraData;
    float lastX = 512.0f; // used for mouseCallback
    float lastY = 512.0f; // used for mouseCallback
    int initalFrame = 0; // used to prevent view snapping on startup in mouseCallback

    WindowUserData(CameraData* pCameraData) : cameraData(pCameraData) {}
};

std::tuple<GLFWwindow*, GLuint, GLuint, GLuint, GLuint> GLInit(WindowUserData* windowUserData);
std::tuple<GLuint, cudaGraphicsResource_t, cudaSurfaceObject_t> GLCUDAInit(cudaStream_t stream, uint32_t imgWidth, uint32_t imgHeight);
void displayUpdate(GLFWwindow* window, GLuint texture, GLuint64 VAO, GLuint shaderProgram);
void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
uint32_t getDefaultShaderProgram();
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_nvdb_file>" << std::endl;
        return 1;
    }

    std::string nvdbPath = argv[1];
    auto handle = nanovdb::io::readGrid<nanovdb::cuda::DeviceBuffer>(nvdbPath);

    cudaStream_t stream; // create a CUDA stream to allow for asynchronous copy of pinned CUDA memory
    cudaStreamCreate(&stream);

    handle.deviceUpload(stream, false); // Copy the NanoVDB grid to the GPU asynchronously

    auto* cpuGrid = handle.grid<int32_t>(); // get a (raw) pointer to the grid of type int32 on the CPU
    auto* deviceGrid = handle.deviceGrid<int32_t>(); // get a (raw) pointer to the grid of type int32 on the GPU

    if (!deviceGrid || !cpuGrid)
        throw std::runtime_error("GridHandle did not contain a grid with the value type Int32");

    Vec3f position = {0, 200, 200};
    Vec3f target = {100, 200, 200};
    CameraData cameraData(position, target, 1024, 1024);
    WindowUserData windowUserData(&cameraData);

    auto [window, shaderProgram, VAO, VBO, EBO] = GLInit(&windowUserData);
    if (!window) return 1;
    auto [glTexture, cuda_resource, cuda_surface_write] = GLCUDAInit(stream, cameraData.mImgWidth, cameraData.mImgHeight);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        cudaGraphicsMapResources(1, &cuda_resource, stream);

        dim3 blockSize(16, 16); // 16*16 = 256 threads
        dim3 gridSize(((cameraData.mImgWidth + blockSize.x - 1)/blockSize.x), ((cameraData.mImgHeight + blockSize.y - 1)/blockSize.y));
        GLCUDAWriteToTex<<<gridSize, blockSize>>>(deviceGrid, cuda_surface_write, cameraData.mImgWidth, cameraData.mImgHeight, cameraData.mDirection,
                                                  cameraData.mRight, cameraData.mUp, cameraData.mPosition, cameraData.mFocalLength, cameraData.mHlfViewPlaneWidth);
        cudaDeviceSynchronize();

        cudaGraphicsUnmapResources(1, &cuda_resource, stream);

        displayUpdate(window, glTexture, VAO, shaderProgram);

    }

    cudaGraphicsUnregisterResource(cuda_resource);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &glTexture);
    glfwTerminate();

    return 0;
}

std::tuple<GLFWwindow*, GLuint, GLuint, GLuint, GLuint> GLInit(WindowUserData* windowUserData) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return {nullptr, NULL, NULL, NULL, NULL};
    }

    GLFWwindow* window = glfwCreateWindow(1024, 1024, "NanoVDB Renderer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return {nullptr, NULL, NULL, NULL, NULL};
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return {nullptr, NULL, NULL, NULL, NULL};
    }


    // Check that opengl is using nvidia gpu for cuda interop (May use integrated graphics instead by default)
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor = (const char*)glGetString(GL_VENDOR);

    if (strstr(vendor, "NVIDIA") == NULL) {
        std::cerr << "Error: OpenGL Not using NVIDIA GPU. Run with '__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia'. (linux)" << std::endl;
        std::cout << "Renderer: " << renderer << std::endl << "Vendor: " << vendor << std::endl;
        glfwTerminate();
        return {nullptr, NULL, NULL, NULL, NULL};
    }

    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetWindowUserPointer(window, windowUserData);

////////////////////////        finished glfw setup        /////////////////////////////////////////////

    GLfloat vertices[] =
    {
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // bottom left
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  // top left
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // top right
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f   // bottom right
    };

    // Indices for vertices order
    GLuint indices[] =
    {
        0, 2, 1, // upper left triangle
        0, 3, 2, // lower right triangle
    };

    unsigned int shaderProgram = getDefaultShaderProgram();

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create a vertex buffer object (VBO)
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create an element buffer object (EBO)
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Update the vertex attribute pointers for the position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Add the vertex attribute pointer for the texture coordinates (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the VAO
    glBindVertexArray(0);

    return {window, shaderProgram, VAO, VBO, EBO};
}

std::tuple<GLuint, cudaGraphicsResource_t, cudaSurfaceObject_t> GLCUDAInit(cudaStream_t stream, uint32_t imgWidth, uint32_t imgHeight)
{
    cudaSetDevice(0);

    GLuint glTexture;
    glGenTextures(1, &glTexture);
    glBindTexture(GL_TEXTURE_2D, glTexture);
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Reserve space in GPU memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //CUDA
    cudaGraphicsResource_t cuda_resource;
    cudaGraphicsGLRegisterImage(&cuda_resource, glTexture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsSurfaceLoadStore);
    cudaGraphicsMapResources(1, &cuda_resource, stream);

    cudaArray* glCudaArray;
    cudaGraphicsSubResourceGetMappedArray(&glCudaArray, cuda_resource, 0, 0);
    cudaGraphicsUnmapResources(1, &cuda_resource, stream);

    cudaResourceDesc resDesc;
    memset(&resDesc, 0, sizeof(resDesc));
    resDesc.resType = cudaResourceTypeArray;
    resDesc.res.array.array = glCudaArray;

    cudaSurfaceObject_t cuda_surface_write;
    cudaCreateSurfaceObject(&cuda_surface_write, (const cudaResourceDesc *)&resDesc);

    return {glTexture, cuda_resource, cuda_surface_write};
}

void displayUpdate(GLFWwindow* window, GLuint texture, GLuint64 VAO, GLuint shaderProgram) {

    glClearColor(0.2, 0.5, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw our square
    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void processInput(GLFWwindow* window)
{
    WindowUserData* windowUserData = (WindowUserData*)glfwGetWindowUserPointer(window);
    CameraData* camera = windowUserData->cameraData;

    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    else if(glfwGetKey(window, GLFW_KEY_W)) {
        camera->mPosition += camera->mDirection * camera->mMovementSpeed;
    } else if(glfwGetKey(window, GLFW_KEY_S)) {
        camera->mPosition -= camera->mDirection * camera->mMovementSpeed;
    }
    
    if(glfwGetKey(window, GLFW_KEY_A)) {
        camera->mPosition -= camera->mRight * camera->mMovementSpeed;
    }  else if(glfwGetKey(window, GLFW_KEY_D)) {
        camera->mPosition += camera->mRight * camera->mMovementSpeed;
    }

}

void mouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    WindowUserData* windowUserData = (WindowUserData*)glfwGetWindowUserPointer(window);
    CameraData* camera = windowUserData->cameraData;

    // prevent orientation snapping on startup
    if (windowUserData->initalFrame < 5) {
        windowUserData->lastX = xPos;
        windowUserData->lastY = yPos;
        windowUserData->initalFrame += 1;
        return;
    }
    
    float xOffset = xPos - windowUserData->lastX;
    float yOffset = yPos - windowUserData->lastY;

    windowUserData->lastX = xPos;
    windowUserData->lastY = yPos;

    xOffset *= camera->mSwivelSensitivity;
    yOffset *= camera->mSwivelSensitivity;

    camera->mYaw -= xOffset;
    camera->mPitch -= yOffset;

    if (camera->mPitch > 89.0f)
        camera->mPitch = 89.0f;
    else if (camera->mPitch < -89.0f)
        camera->mPitch = -89.0f;

    camera->computeDirection();
}

void frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

uint32_t getDefaultShaderProgram() {
    const char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\0";

const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(texture1, TexCoord);\n"
    "}\n\0";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}