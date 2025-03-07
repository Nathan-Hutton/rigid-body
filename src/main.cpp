#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>

#include "cyCore/cyTriMesh.h"
#include "ShaderHandler.h"
#include "Input.h"
#include "TriangleMesh.h"
#include "PickingTexture.h"
#include "BoundaryBox.h"

int main(int argc, char* argv[])
{
    if (argc < 2) // Since I want to just be able to ./main
    {
        argv[1] = strdup("../assets/dragon_80k.obj");
        //argv[1] = strdup("../assets/armadillo_50k_tet.obj");
        //return -1;
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // These 2 lines are just so I can set the window size to be the size of the monitor in glfwCreateWindow
    GLFWmonitor* monitor { glfwGetPrimaryMonitor() };
    const GLFWvidmode* mode { glfwGetVideoMode(monitor) };
    GLFWwindow* window { glfwCreateWindow(mode->width, mode->height, "Rigid-body", monitor, NULL) };
    if (window == NULL)
    {
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, resize_window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // So the cursor won't hit the edge of the screen and stop
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Query and load all OpenGL extensions allowed by your drivers
    // Allows us to access features/extensions not in the core OpenGL specification
    if(glewInit() != GLEW_OK)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Glew initialization failed");
    }
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

    // ***************
    // Handle obj data
    // ***************
    TriangleMesh triMesh { TriangleMesh(argv[1]) };
    BoundaryBox boundary{ 10.0f };
    compileShaders();

    // Make picking texture so we can select vertices
    PickingTexture pickingTexture{ mode->width, mode->height };

    // ****************
    // Scene properties
    // ****************
    glClearColor(0.0f, 0.1f, 0.1f, 1.0f);

    const glm::mat4 projection { glm::perspective(glm::radians(45.0f), (float)mode->width / mode->height, 0.1f, 500.0f) };
    constexpr glm::vec3 viewDir { 0.0f, 0.0f, 1.0f };

    GLfloat xCameraRotateAmountObject{ 0.0f };
    GLfloat zCameraRotateAmountObject{ 0.0f };
    GLfloat viewDistance{-30.0f};

    // Parameters to change light rotation
	GLfloat zLightRotateAmount{ 0.0f };
	GLfloat yLightRotateAmount{ 0.0f };

    // Set uniform variables in shaders that won't change
    glUseProgram(mainShader);
    glUniformMatrix4fv(glGetUniformLocation(mainShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(mainShader, "diffuseMaterialColor"), 1, glm::value_ptr(glm::vec3{1.0f, 1.0f, 1.0f}));

    while (!glfwWindowShouldClose(window)) 
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Input
        GLfloat xRotateAmountChange;
        GLfloat zRotateAmountChange;
        processMouseInputObjectRotation(window, xRotateAmountChange, zRotateAmountChange);

        GLfloat cameraDistanceChange;
        processMouseInputObjectDistance(window, cameraDistanceChange);

        GLfloat zLightRotateChange;
        GLfloat yLightRotateChange;
        processMouseInputLightControls(window, yLightRotateChange, zLightRotateChange);

        processKeyboardInputExit(window);

        // Apply changes from input
        viewDistance += cameraDistanceChange * 0.05;

        zCameraRotateAmountObject += xRotateAmountChange;
        xCameraRotateAmountObject += zRotateAmountChange;
        zCameraRotateAmountObject = fmod(zCameraRotateAmountObject, 360.0f);
        xCameraRotateAmountObject = fmod(xCameraRotateAmountObject, 360.0f);

        yLightRotateAmount += yLightRotateChange;
        zLightRotateAmount += zLightRotateChange;
		yLightRotateAmount = fmod(yLightRotateAmount, 360.0f);
		zLightRotateAmount = fmod(zLightRotateAmount, 360.0f);

        // Setup transforms
        glm::mat4 view { glm::translate(glm::mat4{1.0f}, viewDir * viewDistance) };
        view = glm::rotate(view, glm::radians(-xCameraRotateAmountObject), glm::vec3{1.0f, 0.0f,0.0f});
        view = glm::rotate(view, glm::radians(-zCameraRotateAmountObject), glm::vec3{0.0f, 1.0f, 0.0f});

		glm::mat4 lightRotateMatrix { glm::rotate(glm::mat4{1.0f}, glm::radians(zLightRotateAmount), glm::vec3(0.0f, 0.0f, 1.0f)) };
		lightRotateMatrix = glm::rotate(lightRotateMatrix, glm::radians(yLightRotateAmount), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::vec3 lightDir { glm::vec3{lightRotateMatrix * glm::vec4{1.0f, 0.0f, 0.0f, 0.0f}} };
        const glm::vec3 lightDirInViewSpace { -glm::normalize(view * glm::vec4(lightDir, 0.0f)) };

        const glm::mat4 model{ glm::scale(glm::mat4{1.0f}, glm::vec3{5.0f, 5.0f, 5.0f}) };
        const glm::mat4 modelViewTransform { view * model };

        // Pixel picking
        int xCursorPosPicking;
        int yCursorPosPicking;
        bool isPickingPixel{ processMouseInputPickingControls(window, xCursorPosPicking, yCursorPosPicking) };

        // Render object to framebuffer
        if (isPickingPixel)
        {
            pickingTexture.bind();
            glViewport(0, 0, mode->width, mode->height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(pickingShader);

            glUniformMatrix4fv(glGetUniformLocation(pickingShader, "mvp"), 1, GL_FALSE, glm::value_ptr(projection * view * model));
            glUniform1ui(glGetUniformLocation(pickingShader, "objectIndex"), 1);
            triMesh.draw();

            PickingTexture::PixelInfo pixel{ pickingTexture.readPixel(xCursorPosPicking, mode->height - yCursorPosPicking - 1) };
            GLuint selectedTriangle{ pixel.primitiveID };

            pickingTexture.unbind();
            glViewport(0, 0, mode->width, mode->height);

            // Render selected triangle to the screen
            glEnable(GL_POLYGON_OFFSET_FILL); // This basically pushes it ahead of the triangle that will be rendered in the normal render pass
            glPolygonOffset(-1.0f, -1.0f);
            glUseProgram(highlightShader);
            glUniform1ui(glGetUniformLocation(highlightShader, "selectedTriangle"), selectedTriangle);
            glUniformMatrix4fv(glGetUniformLocation(highlightShader, "mvp"), 1, GL_FALSE, glm::value_ptr(projection * view * model));
            triMesh.draw();
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        // Render object to screen
        glUseProgram(mainShader);
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "modelView"), 1, GL_FALSE, glm::value_ptr(modelViewTransform));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "normalModelView"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(modelViewTransform))));
		glUniform3fv(glGetUniformLocation(mainShader, "lightDir"), 1, glm::value_ptr(lightDirInViewSpace));
        triMesh.draw();

        // Render boundary to screen
        glUseProgram(lineShader);
        glUniformMatrix4fv(glGetUniformLocation(lineShader, "modelViewProjection"), 1, GL_FALSE, glm::value_ptr(projection * view));
        glUniform3fv(glGetUniformLocation(lineShader, "lineColor"), 1, glm::value_ptr(glm::vec3{0.5f, 0.5f, 0.5f}));
        boundary.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

