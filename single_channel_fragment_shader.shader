#version 430 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
uniform vec4 color;
void main()
{
	FragColor.rgba = color.rgba * texture(texture1, TexCoord).r;
}

