#version 460

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D quadTexture;

void main()
{
	fragColor = texture(quadTexture, texCoord);
}
