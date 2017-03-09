#version 150

in vec2 tc; //input tex coordinates
out vec4 c; // output color
uniform sampler2D textureImage; // texture image

void main()
{
	// comput final frag color by referncing texture map
	c = texture(textureImage, tc);
}