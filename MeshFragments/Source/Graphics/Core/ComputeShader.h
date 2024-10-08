#pragma once

#include "Graphics/Core/ShaderProgram.h"

#define TRACK_GPU_MEMORY true

/**
*	@file ComputeShader.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
*	@date 07/13/2019
*/

/**
*	@brief Shader for parallel computing.
*/
class ComputeShader: public ShaderProgram
{
public:
	enum WorkGroupAxis { X_AXIS, Y_AXIS, Z_AXIS };

protected:
	static std::vector<GLint> MAX_WORK_GROUP_SIZE;					//!< This value can be useful since the number of groups is not as limited as group size
	static std::vector<GLint> MAX_NUM_WORK_GROUPS;					//!< Maximum number of invocable work groups
	static GLint			  MAX_MEMORY;							//!< Maximum allocatable size in the GPU

public:
	class MemoryFootprint
	{
	public:
		std::unordered_map<GLuint, size_t> inBuffers;				//!< Size of input buffers
		size_t size;												//!< Size of allocated GPU buffers

	public:
		MemoryFootprint() : size(0) {}
		virtual ~MemoryFootprint() {}

		void addBuffer(GLuint bufferID, const size_t size) { inBuffers[bufferID] = size; this->size += size; }
		void removeBuffer(GLuint bufferID) { size -= inBuffers[bufferID]; inBuffers.erase(bufferID); }
		void updateBuffer(GLuint bufferID, const size_t size) { this->size -= inBuffers[bufferID]; inBuffers[bufferID] = size; this->size += size; }
	};

	static MemoryFootprint _memoryFootprint;						//!< Memory footprint of all the compute shaders

public:	
	/**
	*	@brief Default constructor.
	*/
	ComputeShader();

	/**
	*	@brief Destructor.
	*/
	virtual ~ComputeShader();

	/**
	*	@brief Applies all the active subroutines.
	*/
	virtual void applyActiveSubroutines();

	/**
	*	@brief Binds all those buffers which have been defined (either in / out buffers).
	*/
	void bindBuffers(const std::vector<GLuint>& bufferID);

	/**
	*	@brief Reads and compiles the compute shader located at filename, which is followed by string -comp.glsl.
	*	@filename filename Shader program name. Example: pointCloud => parallel-comp.glsl.
	*/
	virtual GLuint createShaderProgram(const char* filename);

	/**
	*	@brief Executes the compute shader with many groups and works as specified and waits till the execution is over.
	*/
	void execute(GLuint numGroups_x, GLuint numGroups_y, GLuint numGroups_z, GLuint workGroup_x, GLuint workGroup_y, GLuint workGroup_z);

	/**
	*	@brief Deletes a single buffer.
	*/
	static void deleteBuffer(const GLuint bufferID);

	/**
	*	@brief Deletes a set of buffers.
	*/
	static void deleteBuffers(const std::vector<GLuint>& bufferID);

	/**
	*	@return Currently used memory.
	*/
	static size_t getMemoryFootprint() { return _memoryFootprint.size; }

	/**
	*	@return Maximum number of work groups.
	*/
	static std::vector<GLint> getMaxGlobalSize();
	
	/**
	*	@return Maximum size a work group can get.
	*/
	static std::vector<GLint> getMaxLocalSize();

	/**
	*	@return Maximum number of threads per group in any axis.
	*/
	static GLint getMaxGroupSize(const WorkGroupAxis axis = X_AXIS) { return MAX_WORK_GROUP_SIZE[axis]; }

	/**
	*	@return Maximum allocatable size in the GPU.
	*/
	static size_t getMaxUsableMemory();

	/**
	*	@return Maximum number of elements of an SSBO composed of objects of size 'size'. 
	*/
	static GLint getMaxSSBOSize(unsigned elementSize);

	/**
	*	@return Number of groups which are neccesary to cover the array size taking into account the maximum size of a work group.
	*/
	static int getNumGroups(const unsigned arraySize, const WorkGroupAxis axis = X_AXIS);

	/**
	*	@return Size of each group taking into account the total number of elements.
	*/
	static int getWorkGroupSize(const unsigned numGroups, const unsigned arraySize);

	/**
	*	@brief Retrieves maximum group size once OpenGL context is ready.
	*/
	static void initializeMaximumSize();

	/**
	*	@brief Retrieves data from GPU.
	*	@param bufferID Identifier of buffer in GPU.
	*	@param Type of data to be retrieved. Any value can be used here.
	*/
	template<typename T>
	static T* readData(GLuint bufferID, const T& dataType);

	/**
	*	@brief Retrieves data from GPU.
	*	@param bufferID Identifier of buffer in GPU.
	*	@param Type of data to be retrieved. Any value can be used here.
	*/
	template<typename T>
	static T* readData(GLuint bufferID, const T& dataType, size_t offset, size_t length);

	/**
	*	@brief Sets a new uniform which correspondons to an input image.
	*/
	void setImageUniform(const GLint id, const std::string& shaderVariable);

	/**
	*	@brief Defines a new buffer at OpenGL context.
	*	@return ID of new buffer, so we can ask for reading operations.
	*/
	template<typename T>
	static GLuint setReadBuffer(const std::vector<T>& data, const GLuint changeFrequency = GL_DYNAMIC_DRAW);

	/**
	*	@brief Defines a new buffer at OpenGL context.
	*	@return ID of new buffer, so we can ask for reading operations.
	*/
	template<typename T>
	static GLuint setReadBuffer(const T* data, const unsigned arraySize, const GLuint changeFrequency = GL_DYNAMIC_DRAW);

	/**
	*	@brief Defines a new buffer at OpenGL context.
	*	@return ID of new buffer, so we can ask for reading operations.
	*/
	template<typename T>
	static GLuint setReadData(const T& data, const GLuint changeFrequency = GL_DYNAMIC_DRAW);

	/**
	*	@brief Defines a new writing buffer at OpenGL context.
	*	@return ID of new buffer, so we can ask for reading operations.
	*/
	template<typename T>
	static GLuint setWriteBuffer(const T& dataType, const GLuint arraySize, const GLuint changeFrequency = GL_DYNAMIC_DRAW);

	/**
	*	@brief Defines a new buffer at OpenGL context.
	*/
	template<typename T>
	static void updateReadBuffer(const GLuint id, const T* data, const unsigned arraySize, const GLuint changeFrequency = GL_DYNAMIC_DRAW);

	/**
	*	@brief Updates a subset of the buffer.
	*/
	template<typename T>
	static void updateReadBufferSubset(const GLuint id, const T* data, const unsigned offset, const unsigned arraySize);
};

template<typename T>
inline T* ComputeShader::readData(GLuint bufferID, const T& dataType)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);																	
	T* data = (T*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	return data;
}

template<typename T>
inline T* ComputeShader::readData(GLuint bufferID, const T& dataType, size_t offset, size_t length)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
	T* data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, offset, length, GL_MAP_READ_BIT);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	return data;
}

template<typename T>
inline GLuint ComputeShader::setReadBuffer(const std::vector<T>& data, const GLuint changeFrequency)
{
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * data.size(), data.data(), changeFrequency);

#if TRACK_GPU_MEMORY
	_memoryFootprint.addBuffer(id, sizeof(T) * data.size());
#endif

	return id;
}

template<typename T>
inline GLuint ComputeShader::setReadBuffer(const T* data, const unsigned arraySize, const GLuint changeFrequency)
{
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * arraySize, data, changeFrequency);

#if TRACK_GPU_MEMORY
	_memoryFootprint.addBuffer(id, sizeof(T) * arraySize);
#endif

	return id;
}

template<typename T>
inline GLuint ComputeShader::setReadData(const T& data, const GLuint changeFrequency)
{
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T), &data, changeFrequency);

#if TRACK_GPU_MEMORY
	_memoryFootprint.addBuffer(id, sizeof(T));
#endif

	return id;
}

template<typename T>
inline GLuint ComputeShader::setWriteBuffer(const T& dataType, const GLuint arraySize, const GLuint changeFrequency)
{
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(dataType) * arraySize, nullptr, changeFrequency);

#if TRACK_GPU_MEMORY
	_memoryFootprint.addBuffer(id, sizeof(T) * arraySize);
#endif

	return id;
}

template<typename T>
inline void ComputeShader::updateReadBuffer(const GLuint id, const T* data, const unsigned arraySize, const GLuint changeFrequency)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * arraySize, data, changeFrequency);

#if TRACK_GPU_MEMORY
	_memoryFootprint.updateBuffer(id, sizeof(T) * arraySize);
#endif
}

template<typename T>
inline void ComputeShader::updateReadBufferSubset(const GLuint id, const T* data, const unsigned offset, const unsigned arraySize)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(T) * arraySize, data);
}

