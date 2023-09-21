#version 450

#extension GL_ARB_compute_variable_group_size: enable
#extension GL_NV_gpu_shader5 : enable

layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer VertexBuffer	{ VertexGPUData		vertex[]; };
layout (std430, binding = 1) buffer FaceBuffer		{ FaceGPUData		face[]; };
layout (std430, binding = 2) buffer CountBuffer		{ uint				count[]; };
layout (std430, binding = 3) buffer ClusterBuffer	{ float				cluster[]; };

uniform uint numFaces;
uniform uint numFragments;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numFaces) return;

	uint maxCount = 0, baseIndex = numFragments * index;
	uint16_t clusterIdx = uint16_t(0);

	for (int idx = 0; idx < numFragments; ++idx)
	{
		if (count[baseIndex + idx] > maxCount)
		{
			maxCount = count[baseIndex + idx];
			clusterIdx = uint16_t(idx);
		}
	}

	if (maxCount > 0)
		cluster[face[index].vertices.x] = cluster[face[index].vertices.y] = cluster[face[index].vertices.z] = clusterIdx;
}