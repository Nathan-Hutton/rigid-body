#pragma once

#include "cyCore/cyTriMesh.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

class TriangleMesh
{
    public:
        TriangleMesh(const std::string& objectFilePath)
        {
            cy::TriMesh obj { cy::TriMesh() };
            obj.LoadFromFileObj(objectFilePath.c_str());
            obj.ComputeNormals(true);

            // Handle obj data
            std::vector<GLfloat> interleavedObjData;
            for (size_t i { 0 }; i < obj.NF(); ++i)
            {
                const cy::TriMesh::TriFace& vertFace { obj.F(i) };
                const cy::TriMesh::TriFace& normFace { obj.FN(i) };

                for (size_t j { 0 }; j < 3; ++j)
                {
                    const cy::Vec3f& vert { obj.V(vertFace.v[j]) };
                    const cy::Vec3f& norm { obj.VN(normFace.v[j]) };

                    for (size_t k { 0 }; k < 3; ++k)
                        interleavedObjData.push_back(vert[k]);
                    for (size_t k { 0 }; k < 3; ++k)
                        interleavedObjData.push_back(-norm[k]);
                }
            }

            GLuint VBO;
            glGenVertexArrays(1, &m_VAO);

            glBindVertexArray(m_VAO);

            glGenBuffers(1, &VBO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * interleavedObjData.size(), interleavedObjData.data(), GL_STATIC_DRAW);

            // Set vertex attributes
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); // Vertex positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(sizeof(GL_FLOAT) * 3)); // Vertex normals
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        GLuint getVAO() { return m_VAO; }
        GLuint getEBO() { return m_EBO; }

    private:
        GLuint m_VAO;
        GLuint m_EBO;
};
