#pragma once

#include "Geometry/3D/AABB.h"
#include "Graphics/Application/GraphicsAppEnumerations.h"
#include "Graphics/Core/Camera.h"
#include "Graphics/Core/ColorUtilities.h"
#include "Graphics/Core/GraphicsCoreEnumerations.h"
#include "Graphics/Core/Material.h"
#include "Graphics/Core/ShaderProgram.h"
#include "objloader/OBJ_Loader.h"
#include "Utilities/RandomUtilities.h"

class FBOScreenshot;
class Group3D;
class TriangleMesh;
class VAO;

/**
*	@file Model3D.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
*	@date 07/11/2019
*/

/**
*	@brief Base class for any drawable 3D model.
*/
class Model3D
{
public:
	class ModelComponent;
	friend class Octree;

public:
	/**
	*	@brief Struct which wraps all that information for a mesh vertex.
	*/
	struct VertexGPUData
	{
		vec3		_position;
		float		_padding1;

		vec3		_normal;
		float		_padding2;

		vec2		_textCoord;
		vec2		_padding3;

		vec3		_tangent;
		float		_padding4;
	};

	/**
	*	@brief Struct which wraps a mesh data.
	*/
	struct FaceGPUData
	{
		uvec3		_vertices;
		unsigned	_modelCompID;							//!< ID of model component where the face belongs to
	};

	struct MeshGPUData
	{
		unsigned	_numVertices;
		unsigned	_startIndex;
		vec2		_padding1;
	};

	struct BVHCluster
	{
		vec3		_minPoint;
		unsigned	_prevIndex1;

		vec3		_maxPoint;
		unsigned	_prevIndex2;

		unsigned	_faceIndex;
		vec3		_padding;

		mat4 getScaleMatrix()
		{
			vec3 size = _maxPoint - _minPoint;

			return glm::scale(mat4(1.0f), size);
		}

		mat4 getTranslationMatrix()
		{
			vec3 center = (_maxPoint + _minPoint) / 2.0f;

			return glm::translate(mat4(1.0f), center);
		}
	};

	struct RayGPUData
	{
		vec3		_origin;
		unsigned	_padding1;

		vec3		_destination;
		unsigned	_padding2;

		vec3		_direction;
		unsigned	_padding3;

		/**
		*	@brief Default constructor.
		*/
		RayGPUData() : _origin(.0f), _destination(.0f), _direction(.0f) 
		{
			_padding1 = _padding2 = _padding3 = 0;
		}

		/**
		*	@brief Base constructor for any ray.
		*/
		RayGPUData(const vec3& orig, const vec3& dest) : _origin(orig), _destination(dest), _direction(glm::normalize(dest - orig)) 
		{
			_padding1 = _padding2 = _padding3 = 0;
		}

		/**
		*	@return Point for ray in a certain parametric value.
		*/
		vec3 getPoint(float t)
		{
			return _origin + _direction * t;
		}
	};

	struct TriangleCollisionGPUData
	{
		vec3		_point;
		int			_faceIndex;

		vec3		_normal;
		unsigned	_padding1;

		vec2		_textCoord;
		unsigned	_modelCompID;			//!< ID of model component where the face belongs to	
		unsigned	_padding2;
	};

public:
	// [Rendering]
	const static GLuint				RESTART_PRIMITIVE_INDEX;			//!< Index which marks the end of a primitive

protected:
	// Textures which are initialized just once
	static GLuint					_ssaoKernelTextureID;
	static GLuint					_ssaoNoiseTextureID;
	static GLuint					_shadowTextureID;

protected:
	bool							_loaded;				//!< Mark to know if model has already been loaded

	// [Geometry]
	std::vector<ModelComponent*>	_modelComp;				//!< One for each part of the mode, i.e. a revolution object with two bases and a body

	// [Matrices]
	glm::mat4						_modelMatrix;			//!< World transformation

protected:
	/**
	*	@brief Computes the summed area and sorted areas for each triangle of the model.
	*/
	void getSortedTriangleAreas(ModelComponent* component, std::vector<float>& areas, float& sumArea, float& maxArea);

	/**
	*	@brief Sets the model matrix of the model within the matrix buffer. 
	*/
	void setModelMatrix(std::vector<mat4>& matrix);

	/**
	*	@brief Sets the uniform variables for each shader.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	void setShaderUniforms(ShaderProgram* shader, const RendEnum::RendShaderTypes shaderType, const std::vector<mat4>& matrix);

	/**
	*	@brief Creates the VAO with geometry & topology.
	*/
	virtual void setVAOData(bool gpuGeometry = true);

	/**
	*	@brief Reverses the application of the model matrix within the matrix buffer.
	*/
	void unsetModelMatrix(std::vector<mat4>& matrix);

protected:
	/// Rendering methods

	/**
	*	@brief Renders a component as a set of lines.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders a component as a set of lines.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders a component as a set of triangles.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders a component as a set of lines.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

public:
	// ------------- Static methods ----------------

	/**
	*	@brief Binds the ambient occlusion textures into the rendering shader.
	*/
	static void bindSSAOTextures(RenderingShader* shader);

	/**
	*	@brief Builds a texture with random samples for the ambient occlusion technique.
	*/
	static void buildSSAONoiseKernels();

	/**
	*	@brief Builds the shadow offset map for an alternative shadowing technique.
	*/
	static void buildShadowOffsetTexture();

	// ------------------------------------------------

	/**
	*	@brief Model 3D constructor.
	*	@param modelMatrix Transformation of model in world.
	*	@param numComponents Number of component which compose the model.
	*/
	Model3D(const mat4& modelMatrix = mat4(1.0f), unsigned numComponents = 1);

	/**
	*	@brief Copy constructor.
	*/
	Model3D(const Model3D& model) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~Model3D();

	/**
	*	@brief Renders the model as a set of lines.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of points.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles with no textures (except disp. mapping) as we only want to retrieve the depth.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Computes the model data and sends it to GPU.
	*	@param modelMatrix Model matrix to be applied while generating geometry.
	*	@return Success of operation.
	*/
	virtual bool load() = 0;

	/**
	*	@brief Assignment operator overriding.
	*/
	Model3D& operator=(const Model3D& model) = delete;

	/**
	*	@brief Assings identifier for each model component.
	*/
	virtual void registerModelComponentGroup(Group3D* group);

	// ------------- Getters ----------------

	/**
	*	@return Model component located at the specified index.
	*/
	ModelComponent* getModelComponent(unsigned index) { return _modelComp[index]; }

	/**
	*	@return Model component located at the specified index.
	*/
	std::vector<ModelComponent*> getModelComponents() { return _modelComp; }

	/**
	*	@return Model transformation matrix.
	*/
	mat4 getModelMatrix() { return _modelMatrix; }

	/**
	*	@brief Returns the number of faces comprised in this model.
	*/
	unsigned getNumFaces();

	/**
	*	@return Number of vertices comprised in this model.
	*/
	unsigned getNumVertices();

	// -------------- Setters ------------------

	/**
	*	@brief Modifies the name which identifies the model component at compIndex.
	*/
	void setName(const std::string& name, const unsigned int compIndex);

	/**
	*	@brief Establish the material as the color source for triangles.
	*	@param slot Component for which the material should be assigned.
	*/
	void setMaterial(Material* material, unsigned slot = 0);

	/**
	*	@brief Establish the material as the color source for triangles.
	*/
	void setMaterial(std::vector<Material*> material);

	/**
	*	@brief Assigns a new matrix for model transformation.
	*/
	void setModelMatrix(const mat4& modelMatrix) { _modelMatrix = modelMatrix; }
};

class Model3D::ModelComponent
{
public:
	// [Access variables]	
	unsigned						_id;										//!<
	std::string						_name;										//!< Display name for GUI windows

	// [Topology]
	std::vector<GLuint>				_pointCloud;								//!<
	std::vector<GLuint>				_triangleMesh;								//!<
	std::vector<GLuint>				_wireframe;									//!<
	std::vector<unsigned>			_topologyIndicesLength;						//!<

	// [GPU Data]
	std::vector<VertexGPUData>		_geometry;									//!<
	std::vector<FaceGPUData>		_topology;									//!<
	GLuint							_geometrySSBO;								//!<
	GLuint							_topologySSBO;
				
	// [GPU storage]
	VAO*							_vao;										//!<
			
	// [Additional info]
	AABB							_aabb;										//!<		
	bool							_enabled;									//!<
	Material*						_material;									//!< As many vector as material types for different shaders
	Material::MaterialDescription	_materialDescription;

public:
	/**
	*	@brief Default constructor.
	*	@param root Model where this component is located.
	*/
	ModelComponent();
	
	/**
	*	@brief Deleted copy constructor.
	*/
	ModelComponent(const ModelComponent& modelComp) = delete;

	/**
	*	@brief Destructor.
	*/
	~ModelComponent();

	/**
	*	@brief Assigns the identifier to each face of loaded topology.
	*/
	void assignModelCompIDFaces();

	/**
	*	@brief Builds an array with those indices which compose the point cloud topology.
	*/
	virtual void buildPointCloudTopology();

	/**
	*	@brief Builds an array with those indices which compose the wireframe topology.
	*/
	virtual void buildWireframeTopology();

	/**
	*	@brief Builds an array with those indices which compose the wireframe topology.
	*/
	virtual void buildTriangleMeshTopology();

	/**
	*	@brief Copies the main content of a component into a new one.
	*/
	ModelComponent* copyComponent(bool moveData);

	/**
	*	@brief Assignment operator overriding.
	*/
	ModelComponent& operator=(const ModelComponent& orig) = delete;

	/**
	*	@brief Clear geometry and topology arrays to free memory linked to process.
	*/
	virtual void releaseMemory(bool geometry = true, bool topologyIndices = true, bool topologyFaces = true);

	/**
	*	@brief Deletes GPU buffers.
	*/
	virtual void releaseSSBO();

	/**
	*	@brief Subdivides the faces to fit the maximum wanted area.
	*/
	bool subdivide(float maxArea, std::vector<unsigned>& maskFaces);

	/**
	*	@brief Updates geometry and topology SSBOs.
	*/
	void updateSSBO();

	// --------------- Setters -------------------

	/**
	*	@brief Assigns a name to the model component, so it can be identified in the GUI.
	*/
	void setName(const std::string& name) { _name = name; }

	/**
	*	@brief Initializes cluster VBO if the VAO is already created.
	*/
	void setClusterIdx(const std::vector<float>& clusterIdx, bool createVBO = true);
};