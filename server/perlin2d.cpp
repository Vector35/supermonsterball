#include <stdlib.h>
#include <math.h>
#include "perlin2d.h"


void Perlin2D::Vector2D::Normalize()
{
	float magnitude = sqrt((x * x) + (y * y));
	x /= magnitude;
	y /= magnitude;
}


float Perlin2D::Vector2D::DotProduct(const Vector2D& a, const Vector2D& b)
{
	return (a.x * b.x) + (a.y * b.y);
}


Perlin2D::Perlin2D(uint64_t seed)
{
	for (uint32_t i = 0; i < 0x1000; i++)
	{
		m_permutation[i] = i;
		uint32_t xValue = (seed >> 16) % 65535;
		seed = (seed * 25214903917LL) + 11LL;
		uint32_t yValue = (seed >> 16) % 65535;
		seed = (seed * 25214903917LL) + 11LL;
		m_gradient[i].x = ((float)xValue / 65535.0f) * 2.0f - 1.0000001f;
		m_gradient[i].y = ((float)yValue / 65535.0f) * 2.0f - 1.0000001f;
		m_gradient[i].Normalize();
	}

	for (uint32_t i = 0; i < 0x1000; i++)
	{
		uint32_t j = (seed >> 16) % 0x1000;
		seed = (seed * 25214903917LL) + 11LL;
		int tmp = m_permutation[i];
		m_permutation[i] = m_permutation[j];
		m_permutation[j] = tmp;
	}

	for (uint32_t i = 0; i < 0x1002; i++)
	{
		m_permutation[0x1000 + i] = m_permutation[i];
		m_gradient[0x1000 + i] = m_gradient[i];
	}
}


float Perlin2D::SCurve(float t)
{
	return t * t * (3.0f - (2.0f * t));
}


float Perlin2D::LinearInterpolate(float t, float a, float b)
{
	return a + (t * (b - a));
}


float Perlin2D::GetValue(float x, float y)
{
	int xBase0 = (int)floor(x);
	int yBase0 = (int)floor(y);
	int xBase1 = (xBase0 + 1) & 0xfff;
	int yBase1 = (yBase0 + 1) & 0xfff;
	float xFrac0 = x - xBase0;
	float yFrac0 = y - yBase0;
	float xFrac1 = xFrac0 - 1.0f;
	float yFrac1 = yFrac0 - 1.0f;
	xBase0 &= 0xfff;
	yBase0 &= 0xfff;

	uint32_t i = m_permutation[xBase0];
	uint32_t j = m_permutation[xBase1];
	uint32_t base00 = m_permutation[i + yBase0];
	uint32_t base10 = m_permutation[j + yBase0];
	uint32_t base01 = m_permutation[i + yBase1];
	uint32_t base11 = m_permutation[j + yBase1];

	float sx = SCurve(xFrac0);
	float sy = SCurve(yFrac0);

	float u = Vector2D::DotProduct(m_gradient[base00], Vector2D(xFrac0, yFrac0));
	float v = Vector2D::DotProduct(m_gradient[base10], Vector2D(xFrac1, yFrac0));
	float a = LinearInterpolate(sx, u, v);

	u = Vector2D::DotProduct(m_gradient[base01], Vector2D(xFrac0, yFrac1));
	v = Vector2D::DotProduct(m_gradient[base11], Vector2D(xFrac1, yFrac1));
	float b = LinearInterpolate(sx, u, v);

	return LinearInterpolate(sy, a, b);
}
