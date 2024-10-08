#include "stdafx.h"
#include "ComputeShader.h"

/// [Static members initialization]

std::vector<GLint> ComputeShader::MAX_WORK_GROUP_SIZE = { 1024, 1024, 64 };					//!< That value can't be queried before OpenGL is ready
std::vector<GLint> ComputeShader::MAX_NUM_WORK_GROUPS = { 65536, 65536, 65536 };
GLint ComputeShader::MAX_MEMORY = 0;
ComputeShader::MemoryFootprint ComputeShader::_memoryFootprint;

/// [Public methods]

ComputeShader::ComputeShader(): ShaderProgram()
{

}

ComputeShader::~ComputeShader()
{
}

void ComputeShader::applyActiveSubroutines()
{
	glUniformSubroutinesuiv(GL_COMPUTE_SHADER, _activeSubroutineUniform[COMPUTE_SHADER].size(), _activeSubroutineUniform[COMPUTE_SHADER].data());
}

void ComputeShader::bindBuffers(const std::vector<GLuint>& bufferID)
{
	for (unsigned i = 0; i < bufferID.size(); ++i)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, bufferID[i]);
	}
}

GLuint ComputeShader::createShaderProgram(const char* filename)
{
	if (_handler <= 0) 
	{										// Shader identifier
		_handler = glCreateProgram();

		if (_handler == 0) 
		{
			fprintf(stderr, "Cannot create shader program: %s!\n", filename);
			return 0;
		}
	}

	char fileNameComplete[256];
	strcpy_s(fileNameComplete, filename);
	strcat_s(fileNameComplete, "-comp.glsl");
	const GLuint vertexShaderObject = compileShader(fileNameComplete, GL_COMPUTE_SHADER);
	if (vertexShaderObject == 0) 
	{
		return 0;
	}

	glAttachShader(_handler, vertexShaderObject);				// Link shader program and compute shader
	glLinkProgram(_handler);									// Link shader program to check errors
	GLint linkSuccess = 0;
	glGetProgramiv(_handler, GL_LINK_STATUS, &linkSuccess);

	if (linkSuccess == GL_FALSE)
	{
		GLint logLen = 0;
		glGetProgramiv(_handler, GL_INFO_LOG_LENGTH, &logLen);

		if (logLen > 0) 
		{
			char* cLogString = new char[logLen];
			GLint written = 0;
			
			glGetProgramInfoLog(_handler, logLen, &written, cLogString);
			_logString.assign(cLogString);
			delete[] cLogString;

			std::cout << "Cannot link shader " << filename << ":" << std::endl << _logString << std::endl;
		}

		return 0;
	}
	else
	{
		_linked = true;
	}

	// Reserve space for subroutines
	GLint numSubroutines;
	
	glGetProgramStageiv(_handler, GL_COMPUTE_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &numSubroutines);
	_activeSubroutineUniform[COMPUTE_SHADER].resize(numSubroutines);
	std::fill(_activeSubroutineUniform[COMPUTE_SHADER].begin(), _activeSubroutineUniform[COMPUTE_SHADER].end(), -1);		// Non valid id

	return _handler;
}

void ComputeShader::execute(GLuint numGroups_x, GLuint numGroups_y, GLuint numGroups_z, GLuint workGroup_x, GLuint workGroup_y, GLuint workGroup_z)
{
	glDispatchComputeGroupSizeARB(numGroups_x, numGroups_y, numGroups_z, workGroup_x, workGroup_y, workGroup_z);											
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void ComputeShader::deleteBuffer(const GLuint bufferID)
{
	glDeleteBuffers(1, &bufferID);
	_memoryFootprint.removeBuffer(bufferID);
}

void ComputeShader::deleteBuffers(const std::vector<GLuint>& bufferID)
{
	glDeleteBuffers(bufferID.size(), bufferID.data());
	for (unsigned i = 0; i < bufferID.size(); ++i)
	{
		_memoryFootprint.removeBuffer(bufferID[i]);
	}
}

std::vector<GLint> ComputeShader::getMaxGlobalSize()
{
	std::vector<GLint> maxGlobalSize(3);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxGlobalSize[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxGlobalSize[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxGlobalSize[2]);

	return maxGlobalSize;
}

std::vector<GLint> ComputeShader::getMaxLocalSize()
{
	std::vector<GLint> maxLocalSize(3);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxLocalSize[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxLocalSize[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxLocalSize[2]);

	return maxLocalSize;
}

size_t ComputeShader::getMaxUsableMemory()
{
	return MAX_MEMORY;
}

GLint ComputeShader::getMaxSSBOSize(unsigned elementSize)
{
	GLint limitedMemory;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &limitedMemory);

	return std::floor(limitedMemory / elementSize);
}

int ComputeShader::getNumGroups(const unsigned arraySize, const WorkGroupAxis axis)
{
	return (int) std::ceil((float) arraySize / MAX_WORK_GROUP_SIZE[axis]);
}

int ComputeShader::getWorkGroupSize(const unsigned numGroups, const unsigned arraySize)
{
	return (int) std::ceil((float)arraySize / numGroups);
}

void ComputeShader::initializeMaximumSize()
{
	ComputeShader::MAX_WORK_GROUP_SIZE = getMaxLocalSize();
	ComputeShader::MAX_NUM_WORK_GROUPS = getMaxGlobalSize();
	glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &ComputeShader::MAX_MEMORY);
}

void ComputeShader::setImageUniform(const GLint id, const std::string& shaderVariable)
{
	int location = glGetUniformLocation(_handler, shaderVariable.c_str());
	glProgramUniform1i(_handler, location, id);
}