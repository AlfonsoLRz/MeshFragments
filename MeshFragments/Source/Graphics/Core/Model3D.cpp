#include "stdafx.h"
#include "Model3D.h"

#include "Graphics/Application/Renderer.h"
#include "Graphics/Core/FBOScreenshot.h"
#include "Graphics/Core/Group3D.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/VAO.h"
#include "Utilities/ChronoUtilities.h"
#include "Utilities/FileManagement.h"
#include "Interface/Window.h"


// [Static variables initialization]

const GLuint Model3D::RESTART_PRIMITIVE_INDEX = 0xFFFFFFFF;

GLuint Model3D::_ssaoKernelTextureID = -1;
GLuint Model3D::_ssaoNoiseTextureID = -1;
GLuint Model3D::_shadowTextureID = -1;

/// [Static methods

void Model3D::bindSSAOTextures(RenderingShader* shader)
{
	shader->setUniform("texKernels", 3);
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, _ssaoKernelTextureID);

	shader->setUniform("texNoise", 4);
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, _ssaoNoiseTextureID);
}

void Model3D::buildSSAONoiseKernels()
{
	// Uniform distribution
	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	// Sample kernels
	std::vector<vec3> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);

		// Scale samples s.t. they are more aligned to the center of kernel
		float scale = float(i) / 64.0f;
		scale = glm::mix(0.1f, 1.0f, 1.0f - scale * scale);
		sample *= scale;

		ssaoKernel.push_back(sample);
	}

	// Noise texture
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);	// Rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}

	// Compose textures
	glGenTextures(1, &_ssaoKernelTextureID);
	glBindTexture(GL_TEXTURE_2D, _ssaoKernelTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 64, 1, 0, GL_RGB, GL_FLOAT, &ssaoKernel[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenTextures(1, &_ssaoNoiseTextureID);
	glBindTexture(GL_TEXTURE_2D, _ssaoNoiseTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Model3D::buildShadowOffsetTexture()
{
	auto jitter = []() -> float
		{
			return ((float)rand() / RAND_MAX) - 0.5f;
		};

	const int size = 64;
	const int samplesU = 8, samplesV = 8;
	const int samples = samplesU * samplesV;
	const int bufSize = size * size * samples * 2;
	const float twoPI = glm::pi<float>() * 2.0F;
	float* data = new float[bufSize];

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			for (int k = 0; k < samples; k += 2) {
				int x1, y1, x2, y2;

				x1 = k % (samplesU);
				y1 = (samples - 1 - k) / samplesU;
				x2 = (k + 1) % samplesU;
				y2 = (samples - 1 - k - 1) / samplesU;

				vec4 v;
				// Center on grid and jitter
				v.x = (x1 + 0.5f) + jitter();
				v.y = (y1 + 0.5f) + jitter();
				v.z = (x2 + 0.5f) + jitter();
				v.w = (y2 + 0.5f) + jitter();
				// Scale between 0 and 1
				v.x /= samplesU;
				v.y /= samplesV;
				v.z /= samplesU;
				v.w /= samplesV;

				// Warp to disk
				int cell = ((k / 2) * size * size + j * size + i) * 4;
				data[cell + 0] = sqrtf(v.y) * cosf(twoPI * v.x);
				data[cell + 1] = sqrtf(v.y) * sinf(twoPI * v.x);
				data[cell + 2] = sqrtf(v.w) * cosf(twoPI * v.z);
				data[cell + 3] = sqrtf(v.w) * sinf(twoPI * v.z);
			}
		}
	}

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &_shadowTextureID);
	glBindTexture(GL_TEXTURE_3D, _shadowTextureID);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, size, size, samples / 2, 0, GL_RGBA, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	delete[] data;
}

/// [Public methods]

Model3D::Model3D(const glm::mat4& modelMatrix, unsigned numComponents) :
	_loaded(false), _modelMatrix(modelMatrix), _modelComp(numComponents)
{
	for (int i = 0; i < numComponents; ++i)
		_modelComp[i] = new ModelComponent();
}

Model3D::~Model3D()
{
	for (auto& it : _modelComp)
		delete it;

	//glDeleteTextures(1, &_ssaoKernelTextureID);
	//glDeleteTextures(1, &_ssaoNoiseTextureID);
	//glDeleteTextures(1, &_shadowTextureID);
}

void Model3D::drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderLines(shader, shaderType, matrix, modelComp, GL_LINES);
}

void Model3D::drawAsPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderPoints(shader, shaderType, matrix, modelComp, GL_POINTS);
}

void Model3D::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderTriangles(shader, shaderType, matrix, modelComp, GL_TRIANGLES);
}

void Model3D::drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderTriangles4Shadows(shader, shaderType, matrix, modelComp, GL_TRIANGLES);
}

void Model3D::registerModelComponentGroup(Group3D* group)
{
	for (ModelComponent* modelComp : _modelComp)
	{
		group->registerModelComponent(modelComp);
	}
}

unsigned Model3D::getNumFaces()
{
	unsigned numFaces = 0;

	for (ModelComponent* modelComp : _modelComp)
		numFaces += modelComp->_topology.size();

	return numFaces;
}

unsigned Model3D::getNumVertices()
{
	unsigned numVertices = 0;

	for (ModelComponent* modelComp : _modelComp)
		numVertices += modelComp->_geometry.size();

	return numVertices;
}

void Model3D::setName(const std::string& name, const unsigned int compIndex)
{
	if (compIndex >= _modelComp.size())
	{
		return;
	}

	_modelComp[compIndex]->_name = name;
}

void Model3D::setMaterial(Material* material, unsigned slot)
{
	if (slot >= _modelComp.size())
	{
		return;
	}

	_modelComp[slot]->_material = material;
}

void Model3D::setMaterial(std::vector<Material*> material)
{
	int index = 0;

	while (index < _modelComp.size() && index < material.size())
	{
		_modelComp[index]->_material = material[index++];
	}
}

/// [Protected methods]

void Model3D::getSortedTriangleAreas(ModelComponent* component, std::vector<float>& areas, float& sumArea, float& maxArea)
{
	size_t numTriangles = component->_topology.size();
	areas.resize(numTriangles);
	float globalArea = 0.0f;

#pragma omp parallel for reduction(+:globalArea)
	for (int i = 0; i < numTriangles; ++i)
	{
		Triangle3D triangle(component->_geometry[component->_topology[i]._vertices.x]._position,
			component->_geometry[component->_topology[i]._vertices.y]._position,
			component->_geometry[component->_topology[i]._vertices.z]._position);

		areas[i] = triangle.area();
		globalArea += areas[i];
#pragma omp critical
		{
			maxArea = std::max(maxArea, areas[i]);
		}
	}

	std::sort(areas.begin(), areas.end());
	sumArea = globalArea;
}

void Model3D::setModelMatrix(std::vector<mat4>& matrix)
{
	matrix[RendEnum::MODEL_MATRIX] = _modelMatrix;
}

void Model3D::setShaderUniforms(ShaderProgram* shader, const RendEnum::RendShaderTypes shaderType, const std::vector<mat4>& matrix)
{
	RenderingParameters* rendParams = Renderer::getInstance()->getRenderingParameters();

	switch (shaderType)
	{
	case RendEnum::TRIANGLE_MESH_SHADER:
	case RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_SHADER:
	case RendEnum::CLUSTER_SHADER:
		shader->setUniform("mModelView", matrix[RendEnum::VIEW_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mShadow", matrix[RendEnum::BIAS_VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

		// ---- Alternative shadow technique
		shader->setUniform("texOffset", 10);
		glActiveTexture(GL_TEXTURE0 + 10);
		glBindTexture(GL_TEXTURE_3D, _shadowTextureID);

		break;

	case RendEnum::POINT_CLOUD_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("pointSize", rendParams->_scenePointSize);
		shader->setUniform("vColor", rendParams->_scenePointCloudColor);

		break;

	case RendEnum::WIREFRAME_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("vColor", rendParams->_wireframeColor);

		break;

	case RendEnum::TRIANGLE_MESH_NORMAL_SHADER:
	case RendEnum::TRIANGLE_MESH_POSITION_SHADER:
	case RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER:
	case RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER:
		shader->setUniform("mModelView", matrix[RendEnum::VIEW_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

		break;

	case RendEnum::VERTEX_NORMAL_SHADER:
	case RendEnum::SHADOWS_SHADER:
	case RendEnum::MULTI_INSTANCE_SHADOWS_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

		break;
	}
}

void Model3D::setVAOData(bool gpuGeometry)
{
	for (int i = 0; i < _modelComp.size(); ++i)
	{
		VAO* vao = new VAO(gpuGeometry);
		ModelComponent* modelComp = _modelComp[i];

		modelComp->_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD] = modelComp->_pointCloud.size();
		modelComp->_topologyIndicesLength[RendEnum::IBO_WIREFRAME] = modelComp->_wireframe.size();
		modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH] = modelComp->_triangleMesh.size();

		vao->setVBOData(modelComp->_geometry);
		vao->setIBOData(RendEnum::IBO_POINT_CLOUD, modelComp->_pointCloud);
		vao->setIBOData(RendEnum::IBO_WIREFRAME, modelComp->_wireframe);
		vao->setIBOData(RendEnum::IBO_TRIANGLE_MESH, modelComp->_triangleMesh);

		modelComp->_vao = vao;
	}
}

void Model3D::unsetModelMatrix(std::vector<mat4>& matrix)
{
	matrix[RendEnum::MODEL_MATRIX] *= glm::inverse(_modelMatrix);
}

/// [Rendering protected methods]

void Model3D::renderLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_WIREFRAME, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_WIREFRAME]));
	}
}

void Model3D::renderPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_POINT_CLOUD, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD]));
	}
}

void Model3D::renderTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;
	Material* material = modelComp->_material;

	if (vao && modelComp->_enabled)
	{
		this->setModelMatrix(matrix);
		this->setShaderUniforms(shader, shaderType, matrix);

		if (material) material->applyMaterial(shader);

		vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH]));

		this->unsetModelMatrix(matrix);
	}
}

void Model3D::renderTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH]));
	}
}

/// MODEL COMPONENT SUBCLASS

/// [Public methods]

Model3D::ModelComponent::ModelComponent() :
	_id(-1), _enabled(true), _material(nullptr), _topologyIndicesLength(RendEnum::numIBOTypes()), _vao(nullptr), 
	_geometrySSBO(std::numeric_limits<unsigned>::max()), _topologySSBO(std::numeric_limits<unsigned>::max())
{
}

Model3D::ModelComponent::~ModelComponent()
{
	delete _vao;

	this->releaseMemory(true, true, true);
	this->releaseSSBO();
}

void Model3D::ModelComponent::assignModelCompIDFaces()
{
	for (FaceGPUData& data : _topology)
	{
		data._modelCompID = _id;
	}
}

void Model3D::ModelComponent::buildPointCloudTopology()
{
	_pointCloud.resize(_geometry.size());
	std::iota(_pointCloud.begin(), _pointCloud.end(), 0);

	_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD] = _pointCloud.size();
}

void Model3D::ModelComponent::buildWireframeTopology()
{
	int countLines = -3;
	_wireframe.resize(_triangleMesh.size() * 3);

	std::unordered_map<int, std::unordered_set<int>> includedEdges;				// Already included edges

	auto isEdgeIncluded = [&](int index1, int index2) -> bool
		{
			std::unordered_map<int, std::unordered_set<int>>::iterator it;

			if ((it = includedEdges.find(index1)) != includedEdges.end())			// p2, p1 and p1, p2 are considered to be the same edge
			{
				if (it->second.find(index2) != it->second.end())					// Already included
				{
					return true;
				}
			}

			if ((it = includedEdges.find(index2)) != includedEdges.end())
			{
				if (it->second.find(index1) != it->second.end())					// Already included
				{
					return true;
				}
			}

			return false;
		};

	for (unsigned int i = 0; i < _triangleMesh.size(); i += 3)
	{
		for (int j = 0; j < 3; ++j)
		{
			if (!isEdgeIncluded(_triangleMesh[i + j], _triangleMesh[i + (j + 1) % 3]))
			{
#pragma omp atomic
				countLines += 3;

				_wireframe[countLines + 0] = _triangleMesh[i + j];
				_wireframe[countLines + 1] = _triangleMesh[i + (j + 1) % 3];
				_wireframe[countLines + 2] = RESTART_PRIMITIVE_INDEX;

#pragma omp critical
				{
					includedEdges[_triangleMesh[i + j]].insert(_triangleMesh[i + (j + 1) % 3]);
				}
			}
		}
	}

	if (countLines >= 0)
		_wireframe.resize(countLines);

	_topologyIndicesLength[RendEnum::IBO_WIREFRAME] = _wireframe.size();
}

void Model3D::ModelComponent::buildTriangleMeshTopology()
{
	int numFaces = static_cast<int>(_topology.size());
	_triangleMesh.resize(_topology.size() * 3);

#pragma omp parallel for
	for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
	{
		for (int i = 0; i < 3; ++i)
			_triangleMesh[faceIdx * 3 + i] = _topology[faceIdx]._vertices[i];
	}

	_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH] = _triangleMesh.size();
}

Model3D::ModelComponent* Model3D::ModelComponent::copyComponent(bool moveData)
{
	ModelComponent* newComponent = new ModelComponent();
	if (moveData)
	{
		newComponent->_geometry = std::move(_geometry);
		newComponent->_topology = std::move(_topology);
	}
	else
	{
		newComponent->_geometry = _geometry;
		newComponent->_topology = _topology;
	}

	return newComponent;
}

void Model3D::ModelComponent::releaseMemory(bool geometry, bool topologyIndices, bool topologyFaces)
{
	if (geometry) std::vector<VertexGPUData>().swap(_geometry);
	if (topologyFaces) std::vector<FaceGPUData>().swap(_topology);
	if (topologyIndices)
	{
		std::vector<GLuint>().swap(_pointCloud);
		std::vector<GLuint>().swap(_wireframe);
		std::vector<GLuint>().swap(_triangleMesh);
	}
}

void Model3D::ModelComponent::releaseSSBO()
{
	if (_geometrySSBO != std::numeric_limits<unsigned>::max()) ComputeShader::deleteBuffer(_geometrySSBO);
	if (_topologySSBO != std::numeric_limits<unsigned>::max()) ComputeShader::deleteBuffer(_topologySSBO);
	_geometrySSBO = _topologySSBO = std::numeric_limits<unsigned>::max();
}

bool Model3D::ModelComponent::subdivide(float maxArea, std::vector<unsigned>& maskFaces)
{
	bool applyChangesComp = false;

	auto subdivideTriangle = [=](int faceIdx) -> bool {
		bool applyChanges = false;
		Triangle3D triangle(this->_geometry[this->_topology[faceIdx]._vertices.x]._position,
			this->_geometry[this->_topology[faceIdx]._vertices.y]._position,
			this->_geometry[this->_topology[faceIdx]._vertices.z]._position);

		if (triangle.area() > maxArea)
		{
			// Subdivide
			triangle.subdivide(this->_geometry, this->_topology, this->_topology[faceIdx], maxArea);
			applyChanges = true;
			this->_topology.erase(this->_topology.begin() + faceIdx);
			--faceIdx;
		}

		return applyChanges;
		};

	if (maskFaces.empty())
	{
		unsigned numFaces = this->_topology.size();

		for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
		{
			if (subdivideTriangle(faceIdx))
			{
				applyChangesComp = true;
				--faceIdx;
			}
		}
	}
	else
	{
		unsigned numFaces = maskFaces.size(), numErasedFaces = 0;

		for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
		{
			if (subdivideTriangle(maskFaces[faceIdx] - numErasedFaces))
			{
				applyChangesComp = true;
				--faceIdx;
				++numErasedFaces;
			}
		}
	}

	return applyChangesComp;
}

void Model3D::ModelComponent::updateSSBO()
{
	if (_geometrySSBO == std::numeric_limits<unsigned>::max())
		_geometrySSBO = ComputeShader::setReadBuffer(_geometry, GL_STATIC_DRAW);
	else
		ComputeShader::updateReadBuffer(_geometrySSBO, _geometry.data(), _geometry.size(), GL_STATIC_DRAW);

	if (_topologySSBO == std::numeric_limits<unsigned>::max())
		_topologySSBO = ComputeShader::setReadBuffer(_topology, GL_STATIC_DRAW);
	else
		ComputeShader::updateReadBuffer(_topologySSBO, _topology.data(), _topology.size(), GL_STATIC_DRAW);
}

void Model3D::ModelComponent::setClusterIdx(const std::vector<float>& clusterIdx, bool createVBO)
{
	if (_vao)
	{
		if (createVBO) _vao->defineVBO(RendEnum::VBO_CLUSTER_ID, float(), GL_FLOAT);
		_vao->setVBOData(RendEnum::VBO_CLUSTER_ID, clusterIdx);
	}
}
