#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern const unsigned int windowWidth;
extern const unsigned int windowHeight;

#include "controls.hpp"

glm::mat4 V;
glm::mat4 P;

glm::mat4 getViewMatrix() {
    return V;
}

glm::mat4 getProjectionMatrix() {
    return P;
}

// Initial position : on +Z
glm::vec3 position = glm::vec3(0, 3, -14);
// Initial horizontal angle : toward -Z
float horizontalAngle = 0;
// Initial vertical angle : none
float verticalAngle = 0.0f;
// Initial Field of View
float initialFoV = 45.0f;

glm::vec3 getCameraPosition() {
    return position;
}

float speed = 3.0f; // 3 units / second
float mouseSpeed = 0.00005f;

void computeMatrices(GLFWwindow* window, const unsigned int width, const unsigned int height) {
    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    const double currentTime = glfwGetTime();
    const auto deltaTime = static_cast<float>(currentTime - lastTime);

    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Reset mouse position for next frame
    glfwSetCursorPos(window, width / 2, height / 2);

    // Compute new orientation
    horizontalAngle += mouseSpeed * static_cast<float>(width / 2 - xpos);
    verticalAngle += mouseSpeed * static_cast<float>(height / 2 - ypos);

    // Direction : Spherical coordinates to Cartesian coordinates conversion
    glm::vec3 direction(
        glm::cos(verticalAngle) * glm::sin(horizontalAngle),
        glm::sin(verticalAngle),
        glm::cos(verticalAngle) * glm::cos(horizontalAngle)
    );

    // Right vector
    glm::vec3 right = glm::vec3(
        glm::sin(horizontalAngle - 3.14f / 2.0f),
        0,
        glm::cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    glm::vec3 up = glm::cross(right, direction);

    // Move forward
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        position -= right * deltaTime * speed;
    }

    const float FoV = initialFoV;

    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    P = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 500.0f);
    // Camera matrix
    V = glm::lookAt(
        position, // Camera is here
        position + direction, // and looks here : at the same position, plus "direction"
        up // Head is up (set to 0,-1,0 to look upside-down)
    );

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}
