#pragma once

#include "cyCore/cyTriMesh.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include <string>

class TriangleMesh
{
    public:
        TriangleMesh(const std::string& objectFilePath)
        {
            cy::TriMesh obj { cy::TriMesh() };
            obj.LoadFromFileObj(objectFilePath.c_str());
            obj.ComputeNormals(true);

            std::vector<GLfloat> interleavedObjData;
            std::unordered_map<std::string, GLuint> uniqueVertexMap;
            std::vector<GLuint> indices;

            GLuint indexCounter;
            for (size_t i { 0 }; i < obj.NF(); ++i)
            {
                const cy::TriMesh::TriFace& vertFace { obj.F(i) };
                const cy::TriMesh::TriFace& normFace { obj.FN(i) };

                for (size_t j { 0 }; j < 3; ++j)
                {
                    const cy::Vec3f& vert { obj.V(vertFace.v[j]) };
                    const cy::Vec3f& norm { obj.VN(normFace.v[j]) };

                    std::ostringstream vertexKey;
                    vertexKey << vert.x << "," << vert.y << "," << vert.z << "," 
                        << norm.x << "," << norm.y << "," << norm.z;

                    if (uniqueVertexMap.find(vertexKey.str()) == uniqueVertexMap.end())
                    {
                        uniqueVertexMap[vertexKey.str()] = indexCounter;

                        interleavedObjData.push_back(vert.x);
                        interleavedObjData.push_back(vert.y);
                        interleavedObjData.push_back(vert.z);

                        interleavedObjData.push_back(-norm.x);
                        interleavedObjData.push_back(-norm.y);
                        interleavedObjData.push_back(-norm.z);

                        indices.push_back(indexCounter++);
                    }
                    else
                        indices.push_back(uniqueVertexMap[vertexKey.str()]);
                }
            }

            m_numIndices = indices.size();

            GLuint VBO, EBO;
            glGenVertexArrays(1, &m_VAO);

            glBindVertexArray(m_VAO);

            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * interleavedObjData.size(), interleavedObjData.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

            // Set vertex attributes
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); // Vertex positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(sizeof(GL_FLOAT) * 3)); // Vertex normals
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void draw()
        {
            glBindVertexArray(m_VAO);
            glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

    private:
        GLuint m_VAO;
        unsigned int m_numIndices;
};
