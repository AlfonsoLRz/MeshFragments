#pragma once

#include "Graphics/Core/FractureParameters.h"
#include "Graphics/Core/FragmentationProcedure.h"
#include "Graphics/Core/Model3D.h"

class AABB;
class MarchingCubes;
class Texture;
class Voronoi;

#define VOXEL_EMPTY 0
#define VOXEL_FREE 1

class RegularGrid
{
protected:
	const unsigned MASK_POSITION = 15;

public:
	struct CellGrid
	{
		uint16_t _value;

		CellGrid() : _value(VOXEL_EMPTY)/*, _boundary(0), _padding(.0f) */ {}
		CellGrid(uint16_t value) : _value(value)/*, _boundary(0), _padding(.0f)*/ {}
	};

protected:
	std::vector<CellGrid>		_grid;					//!< Color index of regular grid

	AABB						_aabb;					//!< Bounding box of the scene
	vec3						_cellSize;				//!< Size of each grid cell
	GLuint						_countSSBO;				//!< GPU buffer to save the number of occupied voxels per cell		
	MarchingCubes*				_marchingCubes;			//!< Marching cubes algorithm
	uvec3						_numDivs;				//!< Number of subdivisions of space between mininum and maximum point
	GLuint						_ssbo;					//!< GPU buffer to save the grid
	std::vector<unsigned char>	_voxelOpenGL;			//!< CPU buffer to save the number of occupied voxels per cell	

	// Compute shaders
	ComputeShader* _assignVertexClusterShader;			//!< Shader to assign a cluster to each vertex
	ComputeShader* _copyGridShader;						//!< Copies the content of one grid into another
	ComputeShader* _countQuadrantOccupancyShader;		//!< Shader to count the number of occupied voxels per quadrant
	ComputeShader* _countVoxelTriangleShader;
	ComputeShader* _erodeShader;
	ComputeShader* _pickVoxelTriangleShader;
	ComputeShader* _removeIsolatedRegionsShader;
	ComputeShader* _resetCounterShader;					//!< Shader to reset the counter
	ComputeShader* _undoMaskShader;

protected:
	/**
	*	@brief Builds a 3D grid.
	*/
	void buildGrid();

	/**
	*	@brief Cleans the current grid.
	*/
	void cleanGrid();

	/**
	*	@return Number of different values in grid.
	*/
	size_t countValues(std::unordered_map<uint16_t, unsigned>& values);

	/**
	*	@brief Exports the grid as a raw file.
	*/
	void exportRawCompressed(const std::string& filename, bool squared);

	/**
	*	@brief Exports the grid into a .rle file.
	*/
	void exportRLE(const std::string& filename);

	/**
	*	@brief Exports the grid into a .vox file.
	*/
	void exportQuadStack(const std::string& filename);

	/**
	*	@brief Exports the grid as a raw file.
	*/
	void exportUncompressed(const std::string& filename);

	/**
	*	@brief Exports the grid into a .vox file.
	*/
	void exportVox(const std::string& filename, bool squared);

	/**
	*	@brief Fills the grid with the content of the model.
	*/
	void fillNaive(Model3D* model);

	/**
	*	@brief Retrieves compute shaders from the shader list.
	*/
	void getComputeShaders();

	/**
	*	@return Index of grid cell to be filled.
	*/
	uvec3 getPositionIndex(const vec3& position);

	/**
	*	@return Index in grid array of a non-real position.
	*/
	unsigned getPositionIndex(int x, int y, int z) const;

	/**
	*	@brief Checks the number of active voxels, but early exits when at least one is found.
	*/
	unsigned getVoxelCountEarlyExit() const;

	/**
	*	@brief Checks if a ray intersects the bounding box.
	*/
	bool rayBoxIntersection(const Model3D::RayGPUData& ray, float& tMin, float& tMax, float t0, float t1);

	/**
	*	@brief Traverses the grid using the Amanatides-Woo algorithm.
	*/
	uvec3 rayTraversalAmanatidesWoo(const Model3D::RayGPUData& ray);

	/**
	*	@brief Removes isolated regions from the grid.
	*/
	void removeIsolatedRegions();

	/**
	*	@brief Resets buffer to a given value.
	*/
	void resetBuffer(GLuint ssbo, unsigned value, unsigned count) const;

	/**
	*	@return
	*/
	uint16_t unmask(uint16_t value) const;

public:
	/**
	*	@return Index in grid array of a non-real position.
	*/
	static unsigned getPositionIndex(int x, int y, int z, const uvec3& numDivs);

public:
	/**
	*	@brief Constructor which specifies the area and the number of divisions of such area.
	*/
	RegularGrid(const AABB& aabb, const ivec3& subdivisions);

	/**
	*	@brief Constructor of an abstract regular grid with no notion of space size.
	*/
	RegularGrid(const ivec3& subdivisions);

	/**
	*	@brief Invalid copy constructor.
	*/
	RegularGrid(const RegularGrid& regulargrid) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~RegularGrid();

	/**
	*	@brief Calculates the maximum number of voxels occupied per quadrant.
	*/
	unsigned calculateMaxQuadrantOccupancy(const unsigned subdivisions = 1) const;

	/**
	*	@brief Detects which voxels are in the boundary of fragments.
	*/
	void detectBoundaries(int boundarySize);

	/**
	*	@brief
	*/
	void erode(FractureParameters::ErosionType fractureParams, uint32_t convolutionSize, uint16_t numIterations, float erosionProbability, float erosionThreshold);

	/**
	*	@brief Exports fragments into several models in a PLY file.
	*/
	void exportGrid(const std::string& filename, bool squared = false, FractureParameters::ExportGrid exportType = FractureParameters::QUADSTACK);

	/**
	*	@brief
	*/
	void fill(Model3D* model);

	/**
	*	@brief
	*/
	void fill(const Voronoi& voronoi);

	/**
	*	@brief
	*/
	void fillNoiseBuffer(std::vector<float>& noiseBuffer, unsigned numSamples);

	/**
	*	@return Bounding box of the regular grid.
	*/
	AABB getAABB() { return _aabb; }

	/**
	*	@brief Retrieves grid AABBs for rendering purposes.
	*/
	void getAABBs(std::vector<AABB>& aabb);

	/**
	*	@return First collided voxel in the ray direction.
	*/
	uvec3 getClosestEntryVoxel(const Model3D::RayGPUData& ray);

	/**
	*	@brief Creates a new grid with the same content as the current one.
	*/
	template<typename T>
	void getData(std::vector<std::vector<std::vector<T>>>& data);

	/**
	*	@brief Inserts a new point in the grid.
	*/
	void insertPoint(const vec3& position, unsigned index);

	/**
	*	@return Number of occupied voxels.
	*/
	unsigned numOccupiedVoxels();

	/**
	*	@brief Queries cluster for each triangle of the given mesh.
	*/
	void queryCluster(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, std::vector<float>& clusterIdx, std::vector<unsigned>& boundaryFaces, std::vector<std::unordered_map<unsigned, float>>& faceClusterOccupancy);

	/**
	*	@brief Queries cluster for each triangle of the given mesh.
	*/
	void queryCluster(std::vector<vec4>* points, std::vector<float>& clusterIdx);

	/**
	*	@brief Resets regular grid to avoid filling it again.
	*/
	void resetFilling();

	/**
	*   @brief Rebuilds the marching cubes instance.
	*/
	void resetMarchingCubes();

	/**
	*	@brief Modifies the bounding box of the scenario while maintaining the grid.
	*/
	void setAABB(const AABB& aabb, const ivec3& gridDims);

	/**
	*	@return Compute shader's buffer.
	*/
	GLuint ssbo() { return _ssbo; }

	/**
	*	@brief Substitutes current grid with new values.
	*/
	void swap(CellGrid* newGrid, unsigned size) { std::copy(newGrid, newGrid + size, _grid.begin()); }

	/**
	*	@brief Transforms the regular grid into a triangle mesh per value.
	*/
	std::vector<Model3D*> toTriangleMesh(FractureParameters& fractParameters, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata);

	/**
	*	@brief Undo the detection of boundaries, thus removing the included mask.
	*/
	void undoMask();

	/**
	*	@brief Updates the grid with the GPU's content.
	*/
	void updateGrid();

	/**
	*	@brief Updates SSBO content with the CPU's one.
	*/
	void updateSSBO();

	// ----------- External functions ----------

	/**
	*   Get data pointer.
	*   @return Internal data pointer.
	*/
	CellGrid* data();

	/**
	*   Read voxel.
	*   @pre x in range [-1, size.x].
	*   @pre y in range [-1, size.y].
	*   @pre z in range [-1, size.z].
	*   @param[in] x Voxel x coord.
	*   @param[in] y Voxel y coord.
	*   @param[in] z Voxel z coord.
	*   @return Read voxel value.
	*/
	uint16_t at(int x, int y, int z) const;

	/**
	*   Voxel space dimensions.
	*   @return Space dimension
	*/
	glm::uvec3 getNumSubdivisions() const;

	/**
	*   Set every voxel that is not EMPTY as FREE.
	*/
	void homogenize();

	/**
	*	@brief Checks if any neighbour is empty.
	*/
	bool isBoundary(int x, int y, int z, int neighbourhoodSize = 1) const;

	/**
	*   Check if a voxel is occupied.
	*   @pre x in range [-1, size.x].
	*   @pre y in range [-1, size.y].
	*   @pre z in range [-1, size.z].
	*   @param[in] x Voxel x coord.
	*   @param[in] y Voxel y coord.
	*   @param[in] z Voxel z coord.
	*   @return True if the voxel is occupied, false if not.
	*/
	bool isOccupied(int x, int y, int z) const;

	/**
	*   Check if a voxel is empty.
	*   @pre x in range [-1, size.x]
	*   @pre y in range [-1, size.y]
	*   @pre z in range [-1, size.z]
	*   @param[in] x Voxel x coord
	*   @param[in] y Voxel y coord
	*   @param[in] z Voxel z coord
	*   @return True if the voxel is empty, false if not
	*/
	bool isEmpty(int x, int y, int z) const;

	/**
	*   Space size in bytes.
	*   @return size.x * size.y * size.z
	*/
	size_t length() const;

	/**
	*   Set voxel at position [x, y, z].
	*   @pre x in range [-1, size.x].
	*   @pre y in range [-1, size.y].
	*    @pre z in range [-1, size.z].
	*   @param[in] x Voxel x coord.
	*   @param[in] y Voxel y coord.
	*   @param[in] z Voxel z coord.
	*   @param[in] i Voxel new color index.
	*/
	void set(int x, int y, int z, uint16_t i);
};

template<typename T>
inline void RegularGrid::getData(std::vector<std::vector<std::vector<T>>>& data)
{
	if (data.size() < _numDivs.x) 		
		data.resize(_numDivs.x);

	for (int x = 0; x < _numDivs.x; ++x)
	{
		if (data[x].size() < _numDivs.y)
			data[x].resize(_numDivs.y);

		for (int y = 0; y < _numDivs.y; ++y)
		{
			if (data[x][y].size() < _numDivs.z)
				data[x][y].resize(_numDivs.z);

			for (int z = 0; z < _numDivs.z; ++z)
			{
				data[x][y][z] = static_cast<T>(_grid[getPositionIndex(x, y, z)]._value);
			}
		}
	}
}
