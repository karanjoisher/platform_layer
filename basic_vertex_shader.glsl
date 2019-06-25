#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform vec2 textureOffset;
uniform mat4 orthoProjection;
uniform mat4 translationMat;
uniform mat4 rotationMatAboutZAxis;
void main()
{
	gl_Position=orthoProjection*translationMat*rotationMatAboutZAxis* vec4(aPos, 1.0f);
	TexCoord = vec2(aTexCoord.x + textureOffset.x, aTexCoord.y + textureOffset.y);
}