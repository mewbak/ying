// Local Headers
#include "ying/shader.hpp"
#include "ying/camera.hpp"
#include "ying/model.hpp"

// System Headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// UI framework
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Logging framework
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// Standard Headers
#include <cstdio>
#include <cstdlib>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
void processInput(GLFWwindow *window);
void message_callback(GLenum source, GLenum type, GLuint id, GLenum severityType, GLsizei length, const GLchar* message, const void* userParam);
std::string severity_label(GLenum severityType);
std::string type_label(GLenum messageType);

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing.
float deltaTime = 0.0f; // Time between current frame and last frame.
float lastFrame = 0.0f; // Time of last frame.

std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_mt("stderr");
void init_logging() {
    // change log pattern
    logger->set_pattern("[%^%l%$] %v");
}

int main(int argc, char * argv[]) {
	init_logging();

    // Load GLFW and Create a Window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    auto window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL", nullptr, nullptr);

    // Check for Valid Context
    if (window == nullptr) {
		logger->error("Failed to create OpenGL context");
        return EXIT_FAILURE;
    }
    // Create Context and Load OpenGL Functions
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logger->error("Failed to initialize GLAD");
        return -1;
    }
	logger->info("OpenGL {}", glGetString(GL_VERSION));

    // Configure our global OpenGL space.
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(message_callback, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    Model nanosuitModel("data/models/nanosuit/nanosuit.obj");

    Shader shader("shaders/model.vert", "shaders/model.frag");
    Shader singleColorShader("shaders/single.vert", "shaders/single.frag");

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Background Fill Color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Activate the shader.
        shader.use();
        shader.setVec3("dirLight.direction", glm::vec3(0.0, -3, 0.0));
        shader.setVec3("dirLight.ambient", glm::vec3(0.2));
        shader.setVec3("dirLight.diffuse", glm::vec3(0.8));
        shader.setVec3("dirLight.specular", glm::vec3(1.0));
        shader.setVec3("viewPos", camera.Position);

        // Note that we are translating the scene in the opposite direction of where we want to move.
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH/ (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);

        glm::mat4 view = camera.getViewMatrix();
        shader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f));
        shader.setMat4("model", model);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        nanosuitModel.Draw(shader);

        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);

        singleColorShader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);
        nanosuitModel.Draw(singleColorShader);

        glStencilMask(0xFF);
        glEnable(GL_DEPTH_TEST);

        // Flip Buffers and Draw
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes.
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// process all input: query GLFW wheter relevant keys are pressed/released this frame and react accordingly.
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos; // Reversed since y-coordinates go from bottom to top.

    lastX = xPos;
    lastY = yPos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
    camera.ProcessMouseScroll(yOffset);
}

std::string severity_label(GLenum severityType) {
	switch (severityType) {
	case GL_DEBUG_SEVERITY_HIGH:
		return "HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM:
		return "MEDIUM";
    case GL_DEBUG_SEVERITY_LOW:
		return "LOW";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
		return "NOTIFICATION";
	default:
		return "UNKNOWN";
	}
}

std::string type_label(GLenum messageType) {
	switch (messageType) {
	case GL_DEBUG_TYPE_ERROR:
		return "ERROR";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return "DEPRECATED_BEHAVIOR";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return "UNDEFINED_BEHAVIOR";
	case GL_DEBUG_TYPE_PORTABILITY:
		return "PORTABILITY";
	case GL_DEBUG_TYPE_PERFORMANCE:
		return "PERFORMANCE";
	case GL_DEBUG_TYPE_OTHER:
		return "OTHER";
	case GL_DEBUG_TYPE_MARKER:
		return "MARKER";
	case GL_DEBUG_TYPE_PUSH_GROUP:
		return "PUSH_GROUP";
	case GL_DEBUG_TYPE_POP_GROUP:
		return "POP_GROUP";
	default:
		return "UNKNOWN";
	}
}

void message_callback(
	GLenum source,
	GLenum messageType,
	GLuint id,
	GLenum severityType,
	GLsizei length,
	const GLchar* message,
	const void* userParam) {

	std::string severity = severity_label(severityType);
	std::string type = type_label(messageType);
	if (messageType == GL_DEBUG_TYPE_ERROR) {
		logger->warn("[{} / {}]: ** GL ERROR **: {}", severity, type, message);
	} else {
		logger->info("[{} / {}]: {}", severity, type, message);
	}
}
