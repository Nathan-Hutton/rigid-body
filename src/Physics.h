#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/orthonormalize.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Physics
{
    glm::vec3 getAngularAccelerationForPoint(const glm::vec3& worldSpaceForce, const glm::vec3& worldSpaceForcePoint, const glm::vec3& worldSpaceCOM, const glm::mat3& rotationMat, const glm::mat3& inertiaTensor)
    {
        const glm::vec3 worldSpacepositionFromCOM{ worldSpaceForcePoint - worldSpaceCOM };
        const glm::vec3 torque{ glm::cross(worldSpaceForce, worldSpaceCOM) };

        const glm::mat3 inertiaTensorInverse { glm::inverse(rotationMat * inertiaTensor * glm::transpose(rotationMat)) };
        return inertiaTensorInverse * torque;
    }
}
