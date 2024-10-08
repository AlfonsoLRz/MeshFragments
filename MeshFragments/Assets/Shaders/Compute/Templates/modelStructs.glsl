struct VertexGPUData
{
	vec3	position;
	vec3	normal;
	vec2	textCoord;
};

struct FaceGPUData
{
	uvec3	vertices;
	uint	modelCompID;
};

struct MeshGPUData
{
	uint	numVertices;
	uint	startIndex;
	vec2	padding1;
};

struct BVHCluster
{
	vec3	minPoint;
	uint	prevIndex1;

	vec3	maxPoint;
	uint	prevIndex2;

	uint	faceIndex;
};

struct RayGPUData
{
	vec3	origin;
	vec3	destination;
	vec3	direction;
};

struct TriangleCollisionGPUData
{
	vec3	point;
	uint	faceIndex;
	vec3	normal;
	vec2	textCoord;
	uint	modelCompID;
};