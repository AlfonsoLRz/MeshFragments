#version 450

// ------------ Constraints ------------
const float CUTOFF = .3f;

in vec3 position;
in vec2 textCoord;

subroutine vec4 semiTransparentType();
subroutine uniform semiTransparentType semiTransparentUniform;

uniform sampler2D texSemiTransparentSampler;

layout (location = 0) out vec4 fPosition;


// ----- Semitransparent -----

// Combines current fragment color with a semi-transparent texture, if any

subroutine(semiTransparentType)
vec4 semiTransparentTexture()
{
	return texture(texSemiTransparentSampler, textCoord);
}

subroutine(semiTransparentType)
vec4 noSemiTransparentTexture()
{
	return vec4(1.0f);
}


void main()
{
	const vec4 alpha = semiTransparentUniform();

	if (alpha.r - CUTOFF < .0f) discard;

	fPosition = vec4(position, 1.0f);
}
