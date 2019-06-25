#version 430 core
out vec4 FragColor;
uniform sampler2D texture1;
uniform vec4 color;
void main()
{
	FragColor = color;
}


