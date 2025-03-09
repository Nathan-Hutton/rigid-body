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
        TriangleMesh(const std::string& objectFilePath, float mass, float uniformScalingFactor)
        {
            m_mass = mass;

            cy::TriMesh obj { cy::TriMesh() };
            obj.LoadFromFileObj(objectFilePath.c_str());
            obj.ComputeNormals(true);

            //std::vector<GLfloat> interleavedObjData;
            std::unordered_map<std::string, GLuint> uniqueVertexMap;
            std::vector<glm::vec3> vertexNormals;

            m_centerOfMass = glm::dvec3{ 0.0f };
            GLuint indexCounter{ 0 };
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
                        m_indices.push_back(indexCounter++);

                        m_vertexPositions.push_back(glm::vec3{vert.x, vert.y, vert.z} * uniformScalingFactor);
                        vertexNormals.push_back(-glm::vec3{norm.x, norm.y, norm.z});
                        m_centerOfMass += glm::dvec3{ vert.x, vert.y, vert.z } * static_cast<double>(uniformScalingFactor);
                    }
                    else
                        m_indices.push_back(uniqueVertexMap[vertexKey.str()]);
                }
            }
            m_numIndices = m_indices.size();
            m_centerOfMass /= static_cast<double>(m_vertexPositions.size());

            // Calculate inertia tensor
            for (glm::vec3 vertex : m_vertexPositions)
            {
                vertex -= m_centerOfMass;
                float squaredDistanceFromOrigin{ glm::dot(vertex, vertex) };
                glm::mat3 vertexInertia
                {
                    glm::mat3 
                    {
                        squaredDistanceFromOrigin, 0.0f, 0.0f,
                        0.0f, squaredDistanceFromOrigin, 0.0f,
                        0.0f, 0.0f, squaredDistanceFromOrigin
                    } - glm::outerProduct(vertex, vertex)
                };

                m_objectSpaceInertiaTensor += vertexInertia;
            }

            m_objectSpaceInertiaTensor = (m_mass / m_vertexPositions.size()) * m_objectSpaceInertiaTensor;

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
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void draw()
        {
            glBindVertexArray(m_VAO);
            glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);
        }

        glm::vec3 getCenterOfMass() { return m_centerOfMass; }

        glm::mat3 getInertiaTensor()
        {
            return m_objectSpaceInertiaTensor;
        }

        glm::vec3 getFirstVertexFromTriangleID(GLuint triangleID)
        {
            return m_vertexPositions[m_indices[triangleID * 3]];
        }

        glm::vec3 getVertexPosition(GLuint index)
        {
            return m_vertexPositions[index];
        }

        GLuint getNumVertices()
        {
            return m_vertexPositions.size();
        }

        float getMass() { return m_mass; }

    private:
        std::vector<glm::vec3> m_vertexPositions;
        std::vector<GLuint> m_indices;
        float m_mass;
        GLuint m_VAO;
        unsigned int m_numIndices;
        glm::dvec3 m_centerOfMass;
        glm::mat3 m_objectSpaceInertiaTensor{ 0.0f };

};
