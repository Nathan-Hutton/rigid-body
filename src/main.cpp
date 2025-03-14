#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/orthonormalize.hpp>
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
#include "Physics.h"

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
    compileShaders();

    // Handle objects
    TriangleMesh triMesh { TriangleMesh(argv[1], 10.0f, 5.0f) };
    glm::vec3 com{ triMesh.getCenterOfMass() };
    constexpr GLfloat boundaryBoxSize{ 10.0f };
    BoundaryBox boundary{ boundaryBoxSize };

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
    glm::vec3 dragonColor{ 0.5f, 1.0f, 1.0f };

    glm::mat4 model{ 1.0f };
    glm::mat4 modelRotation{ 1.0f };
    glm::vec3 modelTranslation{ 0.0f };
    glm::vec3 linearVelocity{ 0.0f };
    glm::vec3 angularVelocity{ 0.0f }; // Axis of rotation

    GLuint selectedTriangle{ 0xFFFFFFFFu };
    GLfloat lastFrameTime{ static_cast<GLfloat>(glfwGetTime()) };
    while (!glfwWindowShouldClose(window)) 
    {
        const GLfloat currentTime{ static_cast<GLfloat>(glfwGetTime()) };
        const GLfloat deltaTime{ currentTime - lastFrameTime };
        lastFrameTime = currentTime;

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

        // Get selected triangle with mouse input
        bool isTryingToPickTriangle{ processMouseInputIsTryingToPick(window, selectedTriangle) };
        if (isTryingToPickTriangle)
        {
            pickingTexture.bind();
            glViewport(0, 0, mode->width, mode->height);
            glClear(GL_DEPTH_BUFFER_BIT);
            GLuint clearColor[] { 0, 0, 0xFFFFFFFFu }; // Doing this so that I can tell if I've selected the background instead of the object
            glClearBufferuiv(GL_COLOR, 0, clearColor);

            // Render info about the object to a framebuffer so we can see which triangle we're clicking on
            glUseProgram(pickingShader);
            glUniformMatrix4fv(glGetUniformLocation(pickingShader, "mvp"), 1, GL_FALSE, glm::value_ptr(projection * view * model));
            glUniform1ui(glGetUniformLocation(pickingShader, "objectIndex"), 1);
            triMesh.draw();

            pickingTexture.unbind();
            glViewport(0, 0, mode->width, mode->height);

            // Figure out which triangle we're clicking
            int xCursorPosPicking;
            int yCursorPosPicking;
            processMouseInputPickingControls(window, xCursorPosPicking, yCursorPosPicking);
            PickingTexture::PixelInfo pixel{ pickingTexture.readPixel(xCursorPosPicking, mode->height - yCursorPosPicking - 1) };
            selectedTriangle = pixel.primitiveID;
        }

        // *******
        // Physics
        // *******

        // Update angular and linear velocity based on selected triangle
        glm::mat3 rotationMat{ modelRotation };
        if (selectedTriangle != 0xFFFFFFFFu)
        {
            // Angular acceleration
            constexpr GLfloat forceMagnitude{ 10.0f };
            const glm::vec3 forceWorldSpace{ processKeyboardInputForceVec(window) * forceMagnitude };

            const glm::vec3 forcePointWorldSpace{ rotationMat * triMesh.getFirstVertexFromTriangleID(selectedTriangle) };
            const glm::vec3 comWorldSpace{ rotationMat * triMesh.getCenterOfMass() };
            const glm::vec3 positionFromCOMWorldSpace{ forcePointWorldSpace - comWorldSpace };
            const glm::vec3 torque{ glm::cross(forceWorldSpace, positionFromCOMWorldSpace) };
            const glm::mat3 inertiaTensorInverse { glm::inverse(rotationMat * triMesh.getInertiaTensor() * glm::transpose(rotationMat)) };

            const glm::vec3 angularAcceleration{ inertiaTensorInverse * torque };
            angularVelocity += angularAcceleration * deltaTime;

            // Linear acceleration
            const glm::vec3 linearAcceleration{ forceWorldSpace / triMesh.getMass() };
            linearVelocity += linearAcceleration * deltaTime;
        }

        // Collision detection
        for (size_t i{ 0 }; i < triMesh.getNumVertices(); ++i)
        {
            glm::vec3 vertexPosition{ rotationMat * triMesh.getVertexPosition(i) + modelTranslation };

            GLfloat penetrationDepth{ 0.0f };
            glm::vec3 collisionNormal{ 0.0f };
            if (vertexPosition.x > boundaryBoxSize) 
            {
                penetrationDepth = vertexPosition.x - boundaryBoxSize;
                collisionNormal += glm::vec3{ -1.0f, 0.0f, 0.0f };
            }
            else if (vertexPosition.x < -boundaryBoxSize) 
            {
                penetrationDepth = glm::abs(vertexPosition.x + boundaryBoxSize);
                collisionNormal += glm::vec3{ 1.0f, 0.0f, 0.0f };
            }
            if (vertexPosition.y > boundaryBoxSize) 
            {
                penetrationDepth = vertexPosition.y - boundaryBoxSize;
                collisionNormal += glm::vec3{ 0.0f, -1.0f, 0.0f };
            }
            else if (vertexPosition.y < -boundaryBoxSize) 
            {
                penetrationDepth = glm::abs(vertexPosition.y + boundaryBoxSize);
                collisionNormal += glm::vec3{ 0.0f, 1.0f, 0.0f };
            }
            if (vertexPosition.z > boundaryBoxSize) 
            {
                penetrationDepth = vertexPosition.z - boundaryBoxSize;
                collisionNormal += glm::vec3{ 0.0f, 0.0f, -1.0f };
            }
            else if (vertexPosition.z < -boundaryBoxSize) 
            {
                penetrationDepth = glm::abs(vertexPosition.z + boundaryBoxSize);
                collisionNormal += glm::vec3{ 0.0f, 0.0f, 1.0f };
            }

            if (glm::length(collisionNormal) < 0.1f) continue;
            collisionNormal = glm::normalize(collisionNormal);

            const glm::vec3 comWorldSpace{ rotationMat * triMesh.getCenterOfMass() + modelTranslation };
            const glm::vec3 positionFromCOMWorldSpace{ vertexPosition - comWorldSpace };

            // Velocity at contact is the velocity of the actual point on the rigidbody
            const glm::vec3 velocityAtContact{ linearVelocity + glm::cross(angularVelocity, positionFromCOMWorldSpace) };
            const float velocityAlongNormal{ glm::dot(velocityAtContact, collisionNormal) };

            if (velocityAlongNormal < -0.001f)
            {
                constexpr GLfloat bounciness{ 0.5f };
                const GLfloat massInverse{ 1.0f / triMesh.getMass() };
                const glm::mat3 inertiaTensorInverse { glm::inverse(rotationMat * triMesh.getInertiaTensor() * glm::transpose(rotationMat)) };
                const GLfloat denominator{ massInverse + glm::dot(collisionNormal, glm::cross(inertiaTensorInverse * glm::cross(positionFromCOMWorldSpace, collisionNormal), positionFromCOMWorldSpace)) };

                const GLfloat j{ -(1.0f + bounciness) * velocityAlongNormal / denominator };
                const glm::vec3 impulse{ j * collisionNormal };
                linearVelocity += impulse / triMesh.getMass();
                angularVelocity += inertiaTensorInverse * glm::cross(positionFromCOMWorldSpace, impulse);
            }

            if (penetrationDepth > 0.0f)
                modelTranslation += collisionNormal * penetrationDepth * 0.1f;
        }

        // Update rotation matrix based on angular velocity
        const glm::mat3 angularVelocitySkewSymmetricMatrix {
            0.0f, -angularVelocity.z, angularVelocity.y,
            angularVelocity.z, 0.0f, -angularVelocity.x,
            -angularVelocity.y, angularVelocity.x,  0.0f
        };

        const glm::mat3 rotationUpdate{ glm::mat3{ 1.0f } + deltaTime * angularVelocitySkewSymmetricMatrix };
        rotationMat = rotationUpdate * rotationMat;
        rotationMat = glm::orthonormalize(rotationMat);
        modelRotation = glm::mat4{ rotationMat };

        // Update translation
        modelTranslation += linearVelocity * deltaTime;

        model = glm::translate(glm::mat4{ 1.0f }, modelTranslation) * modelRotation;

        const glm::mat4 modelViewTransform { view * model };

        // Render selected triangle
        if (selectedTriangle != 0xFFFFFFFFu)
        {
            // Render selected triangle a different color
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
        glUniform3fv(glGetUniformLocation(mainShader, "diffuseMaterialColor"), 1, glm::value_ptr(dragonColor));
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

