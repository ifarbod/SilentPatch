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
#include <utility> 
#include <cassert>
#include <tuple>

/*#include <windows.h>
#include <limits>
#include <utility> 
#include <mmsystem.h>
#include <Shlwapi.h>
#include <tuple>
#include <cassert>*/

#define RwEngineInstance (*rwengine)

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>
#include <rtpng.h>

#include <d3d9.h>

#include "resource.h"

#include "MemoryMgr.h"
#include "Maths.h"

struct AlphaObjectInfo
{
	RpAtomic*	pAtomic;
	RpAtomic*	(*callback)(RpAtomic*, float);
	float		fCompareValue;

	friend bool operator < (const AlphaObjectInfo &a, const AlphaObjectInfo &b) 
	{ return a.fCompareValue < b.fCompareValue; }
};

// SA operator delete
extern void	(*GTAdelete)(void* data);
extern const char* (*GetFrameNodeName)(RwFrame*);
extern RpHAnimHierarchy* (*GetAnimHierarchyFromSkinClump)(RpClump*);

extern unsigned char& nGameClockDays;
extern unsigned char& nGameClockMonths;

template<typename T>
inline T random(T a, T b)
{
	return a + static_cast<T>(rand() * (1.0f/(RAND_MAX+1)) * (b - a));
}

//#define HIDE_MATERIAL
//#define EXPAND_ALPHA_ENTITY_LISTS		800
//#define EXPAND_BOAT_ALPHA_ATOMIC_LISTS	400