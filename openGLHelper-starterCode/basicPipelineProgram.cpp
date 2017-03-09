﻿#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "basicPipelineProgram.h"
using namespace std;

int BasicPipelineProgram::Init(const char * shaderBasePath, bool textured) 
{
	if (!textured)
	{
		if (BuildShadersFromFiles(shaderBasePath, "basic.vertexShader.glsl", "basic.fragmentShader.glsl") != 0)
		{
			cout << "Failed to build the pipeline program. (default)" << endl;
			return 1;
		}
		cout << "Successfully built the pipeline program. (default)" << endl;
		return 0;
	}
	else
	{
		if (BuildShadersFromFiles(shaderBasePath, "texture.vertexShader.glsl", "texture.fragmentShader.glsl") != 0)
		{
			cout << "Failed to build the pipeline program (textured)." << endl;
			return 1;
		}
		cout << "Successfully built the pipeline program (textured)." << endl;
		return 0;
	}
}

void BasicPipelineProgram::SetModelViewMatrix(const float * m) 
{
  // pass "m" to the pipeline program, as the modelview matrix
  // students need to implement this
}

void BasicPipelineProgram::SetProjectionMatrix(const float * m) 
{
  // pass "m" to the pipeline program, as the projection matrix
  // students need to implement this
}

int BasicPipelineProgram::SetShaderVariableHandles() 
{
  // set h_modelViewMatrix and h_projectionMatrix
  // students need to implement this
  return 0;
}

