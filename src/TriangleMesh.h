#pragma once

#include "cyCore/cyTriMesh.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
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

            //std::vector<GLfloat> interleavedObjData;
            std::unordered_map<std::string, GLuint> uniqueVertexMap;
            std::vector<glm::vec3> vertexNormals;
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
                        indices.push_back(indexCounter++);

                        m_vertexPositions.push_back(glm::vec3{vert.x, vert.y, vert.z});
                        vertexNormals.push_back(-glm::vec3{norm.x, norm.y, norm.z});
                        m_centerOfMass += glm::dvec3{ vert.x, vert.y, vert.z };
                    }
                    else
                        indices.push_back(uniqueVertexMap[vertexKey.str()]);
                }
            }
            m_numIndices = indices.size();
            m_centerOfMass /= static_cast<double>(m_vertexPositions.size());

            // Handle VAO
            GLuint VBO_positions, VBO_normals, EBO;
            glGenVertexArrays(1, &m_VAO);

            glBindVertexArray(m_VAO);

            glGenBuffers(1, &EBO);

            // Set vertex positions attribute
            glGenBuffers(1, &VBO_positions);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
            glBufferData(GL_ARRAY_BUFFER, m_vertexPositions.size() * sizeof(glm::vec3), m_vertexPositions.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(0);

            // Set vertex normals attribute
            glGenBuffers(1, &VBO_normals);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
            glBufferData(GL_ARRAY_BUFFER, m_vertexPositions.size() * sizeof(glm::vec3), vertexNormals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void draw()
        {
            glBindVertexArray(m_VAO);
            glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);
        }

        glm::vec3 getCenterOfMass() { return m_centerOfMass; }

    private:
        std::vector<glm::vec3> m_vertexPositions;
        GLuint m_VAO;
        unsigned int m_numIndices;
        glm::dvec3 m_centerOfMass;

};
