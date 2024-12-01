#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "controls.hpp"
#include "bmp.hpp"

// Window properties
static constexpr unsigned int windowWidth = 1268;
static constexpr unsigned int windowHeight = 720;

// Model properties
static constexpr unsigned int nPoints = 200; //minimum 2
static constexpr float mScale = 5;

// Model vertex indices
unsigned int nIndices;

// VAO
GLuint vertexArrayID;

// Buffers for VAO
GLuint vertexBuffer;
GLuint uvBuffer;
GLuint tangentBuffer;
GLuint bitangentBuffer;
GLuint elementBuffer;

// Texture ids
GLuint heightMapTextureID;
GLuint grassTextureID;
GLuint grassNormalMapID;
GLuint grassRoughnessMapID;
GLuint rocksTextureID;
GLuint rocksNormalMapID;
GLuint rocksRoughnessMapID;
GLuint snowTextureID;
GLuint snowNormalMapID;
GLuint snowRoughnessMapID;

// Height map scale
float heightMapScale = 1.75e-6f;
constexpr float scaleDelta = 5e-8f;
constexpr float minHeightMapScale = 0.0f;
constexpr float maxHeightMapScale = 3e-6f;

// Light
glm::vec3 lightDirection_wcs = glm::vec3(0.0f, -0.5f, -0.5f);
constexpr float rotationDelta = glm::radians(5.0f);
const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

// Id of the shader program loaded
GLuint programID;

// Wireframing
bool showWireframe = false;

// Normal mode - Display normals as colours
bool normalMode = false;

GLFWwindow* initializeGL() {
    // Try initialising GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    // Window hints
    glfwWindowHint(GLFW_SAMPLES, 1); // No anti-aliasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "OpenGLRenderer", nullptr, nullptr);

    // Early return if window fails to open
    if (window == nullptr) {
        std::cerr << "Failed to open GLFW window. Check whether your GPU is OpenGL 4.2 compatible." << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    // Try initialising GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    // Early return if GLEW_ARB_debug_output is false
    if (!GLEW_ARB_debug_output) {
        std::cerr << "GLEW_ARB_debug_output not found" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwPollEvents();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, windowWidth / 2.0, windowHeight / 2.0);

    return window;
}

void loadModel() {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;

    // Compute vertices and uvs
    for (unsigned int i = 0; i < nPoints; i++) {
        float x = mScale * (i / static_cast<float>(nPoints - 1) - 0.5f) * 2.0f;

        for (unsigned int j = 0; j < nPoints; j++) {
            float z = mScale * (j / static_cast<float>(nPoints - 1) - 0.5f) * 2.0f;
            vertices.emplace_back(x, 0, z);
            uvs.emplace_back((i + 0.5f) / static_cast<float>(nPoints - 1),
                             (j + 0.5f) / static_cast<float>(nPoints - 1));
        }
    }

    // Compute indices
    glEnable(GL_PRIMITIVE_RESTART);
    constexpr unsigned int restartIndex = std::numeric_limits<std::uint32_t>::max();
    glPrimitiveRestartIndex(restartIndex);
    int n = 0;
    for (unsigned int i = 0; i < nPoints - 1; i++) {
        for (unsigned int j = 0; j < nPoints; j++) {
            unsigned int topLeft = n;
            unsigned int bottomLeft = topLeft + nPoints;
            indices.push_back(bottomLeft);
            indices.push_back(topLeft);
            n++;
        }
        indices.push_back(restartIndex);
    }

    // Calculate tangents for each triangle (v1, v2, v3)
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;

    for (unsigned int i = 0; i < indices.size(); i += 3) {
        // CCW even triangles: [v0, v2, v1]
        // CCW odd triangles:  [v0, v1, v2]
        bool isEven = i % 2 == 0;
        unsigned int vi0 = indices[i];
        unsigned int vi1 = isEven ? indices[i + 2] : indices[i + 1];
        unsigned int vi2 = isEven ? indices[i + 1] : indices[i + 2];

        // Skip if one of the indices is restartIndex
        if (vi0 == restartIndex || vi1 == restartIndex || vi2 == restartIndex) {
            continue;
        }

        glm::vec3 v0 = vertices[vi0];
        glm::vec3 v1 = vertices[vi1];
        glm::vec3 v2 = vertices[vi2];

        glm::vec2 uv0 = uvs[vi0];
        glm::vec2 uv1 = uvs[vi1];
        glm::vec2 uv2 = uvs[vi2];

        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;

        glm::vec2 duv1 = uv1 - uv0;
        glm::vec2 duv2 = uv2 - uv0;

        float r = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);

        glm::vec3 tangent = (e1 * duv2.y - e2 * duv1.y) * r;
        glm::vec3 bitangent = (e2 * duv1.x - e1 * duv2.x) * r;

        // All the 3 vertices have the same tangent & bitangent
        tangents.push_back(tangent);
        tangents.push_back(tangent);
        tangents.push_back(tangent);

        bitangents.push_back(bitangent);
        bitangents.push_back(bitangent);
        bitangents.push_back(bitangent);
    }

    // Indexed rendering
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    // Bind vertices buffer
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(
        0, // attribute index
        3, // size (x, y ,z)
        GL_FLOAT, // type of each individual element
        GL_FALSE, // normalized?
        0, // stride
        nullptr // array buffer offset
    );

    // Bind uvs buffer
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
    glVertexAttribPointer(
        1, // attribute index
        2, // size (u, v)
        GL_FLOAT, // type of each individual element
        GL_FALSE, // normalized?
        0, // stride
        nullptr // array buffer offset
    );

    // Bind tangents buffer
    glEnableVertexAttribArray(2);
    glGenBuffers(1, &tangentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
    glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(glm::vec3), &tangents[0], GL_STATIC_DRAW);
    glVertexAttribPointer(
        2, // attribute index
        3, // size (x, y, z)
        GL_FLOAT, // type of each individual element
        GL_FALSE, // normalized?
        0, // stride
        nullptr // array buffer offset
    );

    // Bind bitangents buffer
    glEnableVertexAttribArray(3);
    glGenBuffers(1, &bitangentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, bitangentBuffer);
    glBufferData(GL_ARRAY_BUFFER, bitangents.size() * sizeof(glm::vec3), &bitangents[0], GL_STATIC_DRAW);
    glVertexAttribPointer(
        3, // attribute index
        3, // size (x, y, z)
        GL_FLOAT, // type of each individual element
        GL_FALSE, // normalized?
        0, // stride
        nullptr // array buffer offset
    );

    // Generate a buffer for the indices as well
    glGenBuffers(1, &elementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    nIndices = indices.size();
}

void loadPointTexture(const std::string& path, GLuint* textureID) {
    // Try load .bmp
    int width, height;
    const unsigned char* data = nullptr;
    if (data = loadBMP(path.c_str(), width, height); data == nullptr) {
        std::cerr << "Failed to load " + path << std::endl;
        return;
    }

    // Bind .bmp to OpenGL texture
    glGenTextures(1, textureID);
    glBindTexture(GL_TEXTURE_2D, *textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    delete[] data;

    // Clamp to edge makes obtaining values outside [0, 1] to repeat the edge value
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // No interpolation
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // Generate OpenGL mipmaps for texture
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture already processed, unbind
    glBindTexture(GL_TEXTURE_2D, -1);
}

void loadTrilinearTexture(const std::string& path, GLuint* textureID) {
    // Try load .bmp
    int width, height;
    const unsigned char* data = nullptr;
    if (data = loadBMP(path.c_str(), width, height); data == nullptr) {
        std::cerr << "Failed to load " + path << std::endl;
        return;
    }

    // Bind .bmp to OpenGL texture
    glGenTextures(1, textureID);
    glBindTexture(GL_TEXTURE_2D, *textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    delete[] data;

    // Repeat texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Trilinear interpolation
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // Generate OpenGL mipmaps for texture
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture already processed, unbind
    glBindTexture(GL_TEXTURE_2D, -1);
}

void loadTextures() {
    loadPointTexture("assets/mountains_height.bmp", &heightMapTextureID);
    loadTrilinearTexture("assets/grass.bmp", &grassTextureID);
    loadTrilinearTexture("assets/grass-r.bmp", &grassRoughnessMapID);
    loadTrilinearTexture("assets/grass-n.bmp", &grassNormalMapID);
    loadTrilinearTexture("assets/rocks.bmp", &rocksTextureID);
    loadTrilinearTexture("assets/rocks-r.bmp", &rocksRoughnessMapID);
    loadTrilinearTexture("assets/rocks-n.bmp", &rocksNormalMapID);
    loadTrilinearTexture("assets/snow.bmp", &snowTextureID);
    loadTrilinearTexture("assets/snow-r.bmp", &snowRoughnessMapID);
    loadTrilinearTexture("assets/snow-n.bmp", &snowNormalMapID);
}

bool readAndCompileShader(const char* shader_path, const GLuint& id) {
    std::string shaderCode;
    std::ifstream shaderStream(shader_path, std::ios::in);

    // Try read shader file
    if (shaderStream.is_open()) {
        std::stringstream sstr;
        sstr << shaderStream.rdbuf();
        shaderCode = sstr.str();
        shaderStream.close();
    } else {
        std::cout << "Unable to open " << shader_path << ". Are you in the right directory?" << std::endl;
        return false;
    }

    // Compile shader
    std::cout << "Compiling shader: " << shader_path << std::endl;
    char const* sourcePointer = shaderCode.c_str();
    glShaderSource(id, 1, &sourcePointer, nullptr);
    glCompileShader(id);

    // Report compilation result
    GLint compilationResult = GL_FALSE;
    int infoLogLength;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compilationResult);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> shaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(id, infoLogLength, nullptr, &shaderErrorMessage[0]);
        std::cout << &shaderErrorMessage[0] << std::endl;
    }
    std::cout << "Compilation of Shader: " << shader_path << " " << (compilationResult == GL_TRUE
                                                                         ? "Success"
                                                                         : "Failed!") << std::endl;
    return compilationResult == 1;
}

void loadShaders(GLuint& program, const char* vertex_file_path, const char* fragment_file_path) {
    // Compiles shaders
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    bool vok = readAndCompileShader(vertex_file_path, vertexShaderID);
    bool fok = readAndCompileShader(fragment_file_path, fragmentShaderID);

    // If both were compiled successfully, try linking
    if (vok && fok) {
        GLint result = GL_FALSE;
        int infoLogLength;
        std::cout << "Linking program..." << std::endl;
        program = glCreateProgram();
        glAttachShader(program, vertexShaderID);
        glAttachShader(program, fragmentShaderID);
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &result);
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            std::vector<char> programErrorMessages(infoLogLength + 1);
            glGetProgramInfoLog(program, infoLogLength, nullptr, &programErrorMessages[0]);
            std::cout << programErrorMessages[0];
        }
        std::cout << "Linking program: " << (result == GL_TRUE ? "Success" : "Failed!") << std::endl;
    } else {
        std::cout << "Program will not be linked: one of the shaders has an error" << std::endl;
    }

    // Delete shaders after use
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}

void loadProgram() {
    programID = glCreateProgram();
    loadShaders(programID, "src/shaders/main.vert", "src/shaders/main.frag");
}

void unloadModel() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &uvBuffer);
    glDeleteBuffers(1, &elementBuffer);
    glDeleteVertexArrays(1, &vertexArrayID);
}

void unloadTextures() {
    glDeleteTextures(1, &heightMapTextureID);
    glDeleteTextures(1, &grassTextureID);
    glDeleteTextures(1, &grassRoughnessMapID);
    glDeleteTextures(1, &grassNormalMapID);
    glDeleteTextures(1, &rocksTextureID);
    glDeleteTextures(1, &rocksRoughnessMapID);
    glDeleteTextures(1, &rocksNormalMapID);
    glDeleteTextures(1, &snowTextureID);
    glDeleteTextures(1, &snowRoughnessMapID);
    glDeleteTextures(1, &snowNormalMapID);
}

void unloadShaders() {
    glDeleteProgram(programID);
}

void toggleWireframe() {
    showWireframe = !showWireframe;
    if (showWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void toggleNormalMode() {
    normalMode = !normalMode;
}

void scaleHeightMapBy(float delta) {
    heightMapScale = glm::clamp(heightMapScale + delta, minHeightMapScale, maxHeightMapScale);
}

void rotateLight(float deltaInRadians, const glm::vec3& axis) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deltaInRadians, axis);
    lightDirection_wcs = glm::vec3(rotation * glm::vec4(lightDirection_wcs, 0.0f));
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_R:
            loadProgram();
            break;
        case GLFW_KEY_SPACE:
            toggleWireframe();
            break;
        case GLFW_KEY_T:
            scaleHeightMapBy(scaleDelta);
            break;
        case GLFW_KEY_G:
            scaleHeightMapBy(-scaleDelta);
            break;
        case GLFW_KEY_D:
            rotateLight(rotationDelta, up);
            break;
        case GLFW_KEY_A:
            rotateLight(-rotationDelta, up);
            break;
        case GLFW_KEY_W:
            rotateLight(rotationDelta, right);
            break;
        case GLFW_KEY_S:
            rotateLight(-rotationDelta, right);
            break;
        case GLFW_KEY_N:
            toggleNormalMode();
            break;
        default:
            break;
    }
}

int main() {
    GLFWwindow* window;
    if (window = initializeGL(); window == nullptr) {
        return EXIT_FAILURE;
    }

    loadModel();
    loadTextures();
    loadProgram();
    glfwSetKeyCallback(window, keyCallback);

    glClearColor(0.7f, 0.8f, 1.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    do {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Compute the MVP matrix from keyboard and mouse input
        computeMatrices(window, windowWidth, windowHeight);
        glm::mat4 projectionMatrix = getProjectionMatrix();
        glm::mat4 viewMatrix = getViewMatrix();
        // No model, default to identity
        glm::mat4 modelMatrix = glm::mat4(1.0);
        glm::mat4 modelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

        // Use shader program
        glUseProgram(programID);

        // Set matrix uniforms
        const GLuint mvpID = glGetUniformLocation(programID, "MVP");
        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &modelViewProjectionMatrix[0][0]);

        const GLuint viewID = glGetUniformLocation(programID, "V");
        glUniformMatrix4fv(viewID, 1, GL_FALSE, &viewMatrix[0][0]);

        const GLuint modelID = glGetUniformLocation(programID, "M");
        glUniformMatrix4fv(modelID, 1, GL_FALSE, &modelMatrix[0][0]);

        // Set heightMapScale uniform
        const GLuint heightMapScaleID = glGetUniformLocation(programID, "heightMapScale");
        glUniform1f(heightMapScaleID, heightMapScale);

        // Set uniform light properties
        const GLuint lightDirectionID = glGetUniformLocation(programID, "lightDirection_wcs");
        glUniform3fv(lightDirectionID, 1, &lightDirection_wcs[0]);

        // Set the nPoints uniform
        const GLuint nPointsID = glGetUniformLocation(programID, "nPoints");
        glUniform1f(nPointsID, static_cast<float>(nPoints - 1));

        // Set normalMode uniform
        const GLuint normalModeId = glGetUniformLocation(programID, "normalMode");
        glUniform1i(normalModeId, normalMode);

        // Bind texture ids to textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightMapTextureID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grassTextureID);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, grassRoughnessMapID);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, grassNormalMapID);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, rocksTextureID);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, rocksRoughnessMapID);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, rocksNormalMapID);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, snowTextureID);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, snowRoughnessMapID);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, snowNormalMapID);

        // Draw
        glDrawElements(
            GL_TRIANGLE_STRIP, // mode
            static_cast<GLsizei>(nIndices), // count
            GL_UNSIGNED_INT, // type
            nullptr // element array buffer offset
        );

        // Swap buffers and poll events to update screen properly
        glfwSwapBuffers(window);
        glfwPollEvents();
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    unloadModel();
    unloadShaders();
    unloadTextures();
    glfwTerminate();

    return EXIT_SUCCESS;
}
