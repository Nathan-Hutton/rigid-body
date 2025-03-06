#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in float aVertexID;

out vec3 fragPos;
out vec3 normal;
out float highlighted;

uniform mat4 modelView;
uniform mat4 normalModelView;
uniform mat4 projection;
uniform int selectedVertex;

void main()
{
	fragPos = vec3(modelView * vec4(aPos, 1.0));
	normal = normalize(mat3(normalModelView) * aNorm);
    if (int(aVertexID) == selectedVertex)
        highlighted = 1.0f;
    else
        highlighted = 0.0f;

    gl_Position = projection * modelView * vec4(aPos, 1.0);
}
