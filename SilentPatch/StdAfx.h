#define WIN32_LEAN_AND_MEAN

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define NOMINMAX

#include <windows.h>
#include <cassert>
#include <cstdio>

#include "MemoryMgr.h"

class CVector
{
public:
	float	x, y, z;

	CVector()
	{}

	constexpr CVector(float fX, float fY, float fZ=0.0f)
		: x(fX), y(fY), z(fZ)
	{}

	CVector&		operator+=(const CVector& vec)
	{ x += vec.x; y += vec.y; z += vec.z;
	return *this; }
	CVector&		operator-=(const CVector& vec)
	{ x -= vec.x; y -= vec.y; z -= vec.z;
	return *this; }

	inline float	Magnitude() const
	{ return sqrt(x * x + y * y + z * z); }
	inline constexpr float	MagnitudeSqr() const
	{ return x * x + y * y + z * z; }
	inline CVector&	Normalize()
	{ float	fInvLen = 1.0f / Magnitude(); x *= fInvLen; y *= fInvLen; z *= fInvLen; return *this; }

	friend inline float DotProduct(const CVector& vec1, const CVector& vec2)
	{ return vec1.x * vec2.x + vec1.x * vec2.y + vec1.z * vec2.z; }
	friend inline CVector CrossProduct(const CVector& vec1, const CVector& vec2)
	{ return CVector(	vec1.y * vec2.z - vec1.z * vec2.y,
		vec1.z * vec2.x - vec1.x * vec2.z,
		vec1.x * vec2.y - vec1.y * vec2.x); }

	friend inline CVector operator*(const CVector& in, float fMul)
	{ return CVector(in.x * fMul, in.y * fMul, in.z * fMul); }
	friend inline CVector operator+(const CVector& vec1, const CVector& vec2)
	{ return CVector(vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z); }
	friend inline CVector operator-(const CVector& vec1, const CVector& vec2)
	{ return CVector(vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z); }
	friend inline CVector operator-(const CVector& vec)
	{ return CVector(-vec.x, -vec.y, -vec.z); }
};

#define DISABLE_FLA_DONATION_WINDOW		0
