#pragma once

#include "Graphics/Core/Model3D.h"

class DrawLines : public Model3D
{
public:
	DrawLines(const std::vector<vec3>& points);
	virtual ~DrawLines();

	virtual bool load(const mat4& modelMatrix = mat4(1.0f));
};

