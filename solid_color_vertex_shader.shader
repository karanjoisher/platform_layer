#version 430 core
layout (location = 0) in vec3 aPos;
uniform mat4 orthoProjection;
uniform mat4 translationMat;
uniform mat4 rotationMatAboutZAxis;
void main()
{
	gl_Position=orthoProjection*translationMat*rotationMatAboutZAxis* vec4(aPos, 1.0f);
}

