#version 150

in vec3 position;
in vec2 texCoord;

// texture coordinates
out vec2 tc;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
	// compute the transformed and project vertex position (into gl_position)
	gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
	// pass-through the texture coord
	tc = texCoord;
}