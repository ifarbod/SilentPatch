#pragma warning(disable:4481)	// nonstandard extension used: override specifier 'override'
#pragma warning(disable:4401)	// member is bit field
#pragma warning(disable:4733)	// handler not registered as safe handler
#pragma warning(disable:4725)	// instruction may be inaccurate on some Pentiums
#pragma warning(disable:4201)	// nonstandard extension used: nameless struct/union

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <mmsystem.h>
#include <cassert>
#include <cstdio>
#include <ShlObj.h>
#include <Shlwapi.h>

#include "MemoryMgr.h"

#define DISABLE_FLA_DONATION_WINDOW		0


template<typename T>
inline T random(T a, T b)
{
	return a + static_cast<T>(rand() * (1.0f/(RAND_MAX+1)) * (b - a));
}