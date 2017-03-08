#pragma warning(disable:4481)	// nonstandard extension used: override specifier 'override'
#pragma warning(disable:4401)	// member is bit field
#pragma warning(disable:4733)	// handler not registered as safe handler
#pragma warning(disable:4725)	// instruction may be inaccurate on some Pentiums
#pragma warning(disable:4201)	// nonstandard extension used: nameless struct/union

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#define NOMINMAX
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502

#include <windows.h>
#include <cassert>

#define RwEngineInstance (*rwengine)

#include <rwcore.h>
#include <rphanim.h>
#include <rtpng.h>

#include "resource.h"

#include "MemoryMgr.h"
#include "Maths.h"

// SA operator delete
extern void	(*GTAdelete)(void* data);
extern const char* (*GetFrameNodeName)(RwFrame*);
extern RpHAnimHierarchy* (*GetAnimHierarchyFromSkinClump)(RpClump*);
int32_t Int32Rand();

extern unsigned char& nGameClockDays;
extern unsigned char& nGameClockMonths;

#define DISABLE_FLA_DONATION_WINDOW		0
