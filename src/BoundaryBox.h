#pragma once

#include <GL/glew.h>
#include <vector>

class BoundaryBox
{
    public:
        BoundaryBox(float width)
        {
            const std::vector<GLfloat> boxVertices
            {
                -width, -width, -width,
                 width, -width, -width,
                -width,  width, -width,
                 width,  width, -width,
                -width, -width, width,
                 width, -width, width,
                -width,  width, width,
                 width,  width, width
            };

            const std::vector<GLuint> boxIndices
            {
                0, 1, 1, 3, 3, 2, 2, 0,
                4, 5, 5, 7, 7, 6, 6, 4,
                0, 4, 2, 6, 3, 7, 1, 5
            };

            GLuint boxVBO, boxEBO;
            glGenVertexArrays(1, &m_VAO);

            glBindVertexArray(m_VAO);

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

        }

        void draw()
        {
            glBindVertexArray(m_VAO);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }

    private:
        GLuint m_VAO;
};
