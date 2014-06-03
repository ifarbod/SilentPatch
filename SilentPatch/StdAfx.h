#pragma warning(disable:4481)	// nonstandard extension used: override specifier 'override'
#pragma warning(disable:4401)	// member is bit field
#pragma warning(disable:4733)	// handler not registered as safe handler
#pragma warning(disable:4725)	// instruction may be inaccurate on some Pentiums

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <limits>
#include <utility> 
#include <mmsystem.h>

#define RwEngineInstance (*rwengine)
#define RWFRAMESTATICPLUGINSSIZE 24

#include <rwcore.h>
#include <rpworld.h>

#include "MemoryMgr.h"
#include "Maths.h"

//#define HIDE_MATERIAL
//#define EXPAND_ALPHA_ENTITY_LISTS		800

//#define SA_STEAM_TEST