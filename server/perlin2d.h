#pragma once

#include <inttypes.h>

class Perlin2D
{
	struct Vector2D
	{
		float x, y;

		Vector2D(): x(0), y(0) {}
		Vector2D(float _x, float _y): x(_x), y(_y) {}
		void Normalize();
		static float DotProduct(const Vector2D& a, const Vector2D& b);
	};

	uint32_t m_permutation[0x2002];
	Vector2D m_gradient[0x2002];

	static float SCurve(float t);
	static float LinearInterpolate(float t, float a, float b);

public:
	Perlin2D(uint64_t seed);
	float GetValue(float x, float y);
};
