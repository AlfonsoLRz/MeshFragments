#define EPSILON		0.00000001f
#define PI			3.1415926535f
#define UINT_MAX	0xFFFFFFF
#define UINT_MULT	10000.0f

bool epsilonEqual(float a, float b)
{
	return abs(a - b) < EPSILON;
}