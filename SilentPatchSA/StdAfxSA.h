#pragma warning(disable:4458) // declaration hides class member
#pragma warning(disable:4201) // nonatandard extension user: nameless struct/union
#pragma warning(disable:4100) // unreferenced formal parameter

#define WIN32_LEAN_AND_MEAN

#define NOMINMAX
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502

#include <windows.h>
#include <cassert>

#define RwEngineInstance (*rwengine)

#include <rwcore.h>
#include <rphanim.h>
#include <rtpng.h>

#include "Utils/MemoryMgr.h"
#include "Utils/MemoryMgr.GTA.h"
#include "Maths.h"
#include "rwutils.hpp"

#include "TheFLAUtils.h"

// Move this to ModUtils when it matures a bit more
#define HOOK_EACH_FUNC_CTR(name, ctr, origFunc, hook) \
	template<std::size_t Ctr, typename Tuple, std::size_t... I, typename Func> \
	static void _HookEachImpl_##name(Tuple&& tuple, std::index_sequence<I...>, Func&& f) \
	{ \
		(f(std::get<I>(tuple), origFunc<Ctr << 16 | I>, hook<Ctr << 16 | I>), ...); \
	} \
	\
	template<std::size_t Ctr = ctr, typename Vars, typename Func> \
	static void HookEach_##name(Vars&& vars, Func&& f) \
	{ \
		auto tuple = std::tuple_cat(std::forward<Vars>(vars)); \
		_HookEachImpl_##name<Ctr>(std::move(tuple), std::make_index_sequence<std::tuple_size_v<decltype(tuple)>>{}, std::forward<Func>(f)); \
	} 

#define HOOK_EACH_FUNC(name, orig, hook) HOOK_EACH_FUNC_CTR(name, 0, orig, hook)

// SA operator delete
extern void	(*GTAdelete)(void* data);
extern const char* (*GetFrameNodeName)(RwFrame*);
extern RpHAnimHierarchy* (*GetAnimHierarchyFromSkinClump)(RpClump*);
int32_t Int32Rand();
RwObject* GetFirstObject(RwFrame* pFrame);

extern unsigned char& nGameClockDays;
extern unsigned char& nGameClockMonths;

#define DISABLE_FLA_DONATION_WINDOW		0

#ifdef _DEBUG
#define MEM_VALIDATORS 1
#else
#define MEM_VALIDATORS 0
#endif
