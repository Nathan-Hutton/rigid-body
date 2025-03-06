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

    // **********
    // Handle box
    // **********
    const std::vector<GLfloat> boxVertices
    {
        -10.0f, -10.0f, -10.0f,
         10.0f, -10.0f, -10.0f,
        -10.0f,  10.0f, -10.0f,
         10.0f,  10.0f, -10.0f,
        -10.0f, -10.0f, 10.0f,
         10.0f, -10.0f, 10.0f,
        -10.0f,  10.0f, 10.0f,
         10.0f,  10.0f, 10.0f
    };

    const std::vector<GLuint> boxIndices
    {
        0, 1,
        1, 3,
        3, 2,
        2, 0,
        4, 5,
        5, 7,
        7, 6,
        6, 4,
        0, 4,
        2, 6,
        3, 7,
        1, 5
    };

    GLuint boxVAO, boxVBO, boxEBO;
    glGenVertexArrays(1, &boxVAO);

    glBindVertexArray(boxVAO);

    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &boxEBO);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * boxVertices.size(), boxVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * boxIndices.size(), boxIndices.data(), GL_STATIC_DRAW);

    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0); // Vertex positions
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // ********************
    // Handle render buffer
    // ********************
    GLuint frameBuffer;
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    // Make texture
    GLuint renderedTextureID;
    glGenTextures(1, &renderedTextureID);
    glBindTexture(GL_TEXTURE_2D, renderedTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mode->width, mode->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Make depth buffer
    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mode->width, mode->height);

    // Bind the framebuffer together
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTextureID, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Something went wrong making the framebuffer\n";
        return -1;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Make quad to draw to
    const std::vector<GLfloat> quadVertices
    {
        -0.5f, -0.5f, 0.0f,      0.0f, 0.0f,  // Bottom left
        0.5f, -0.5f, 0.0f,       1.0f, 0.0f,  // Bottom right
        -0.5f, 0.5f, 0.0f,       0.0f, 1.0f,  // Top left
        0.5f, 0.5f, 0.0f,        1.0f, 1.0f   // Top right
    };

    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);

    glBindVertexArray(quadVAO);

    glGenBuffers(1, &quadVBO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * quadVertices.size(), quadVertices.data(), GL_STATIC_DRAW);

    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0); // Vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(sizeof(GL_FLOAT) * 3)); // Texture coordinates
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // ****************
    // Scene properties
    // ****************
    compileShaders();
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
        double xCursorPos, yCursorPos;
        glfwGetCursorPos(window, &xCursorPos, &yCursorPos);
        unsigned char pixelColor[3];
        glReadPixels(xCursorPos, mode->height - yCursorPos, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixelColor);
        //glReadPixels(xCursorPos, mode->height - static_cast<int>(yCursorPos) - 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixelColor);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // *****
        // Input
        // *****
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

        glm::mat4 view { glm::translate(glm::mat4{1.0f}, viewDir * viewDistance) };
        view = glm::rotate(view, glm::radians(-xCameraRotateAmountObject), glm::vec3{1.0f, 0.0f,0.0f});
        view = glm::rotate(view, glm::radians(-zCameraRotateAmountObject), glm::vec3{0.0f, 1.0f, 0.0f});

		glm::mat4 lightRotateMatrix { glm::rotate(glm::mat4{1.0f}, glm::radians(zLightRotateAmount), glm::vec3(0.0f, 0.0f, 1.0f)) };
		lightRotateMatrix = glm::rotate(lightRotateMatrix, glm::radians(yLightRotateAmount), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::vec3 lightDir { glm::vec3{lightRotateMatrix * glm::vec4{1.0f, 0.0f, 0.0f, 0.0f}} };
        const glm::vec3 lightDirInViewSpace { -glm::normalize(view * glm::vec4(lightDir, 0.0f)) };

        // *************
        // Render object
        // *************
        glUseProgram(mainShader);
        glUniform1i(glGetUniformLocation(mainShader, "selectedVertex"), pixelColor[0]);
        const glm::mat4 model{ glm::scale(glm::mat4{1.0f}, glm::vec3{5.0f, 5.0f, 5.0f}) };
        const glm::mat4 modelViewTransform { view * model };

        glUniformMatrix4fv(glGetUniformLocation(mainShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "modelView"), 1, GL_FALSE, glm::value_ptr(modelViewTransform));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "normalModelView"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(modelViewTransform))));
		glUniform3fv(glGetUniformLocation(mainShader, "lightDir"), 1, glm::value_ptr(lightDirInViewSpace));

        triMesh.draw();

        // ***************
        // Render boundary
        // ***************
        glUseProgram(lineShader);
        glUniformMatrix4fv(glGetUniformLocation(lineShader, "modelViewProjection"), 1, GL_FALSE, glm::value_ptr(projection * view));
        glUniform3fv(glGetUniformLocation(lineShader, "lineColor"), 1, glm::value_ptr(glm::vec3{0.5f, 0.5f, 0.5f}));
        glBindVertexArray(boxVAO);
        glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

