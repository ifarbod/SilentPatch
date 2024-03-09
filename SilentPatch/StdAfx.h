#define WIN32_LEAN_AND_MEAN

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define NOMINMAX

#include <windows.h>
#include <cassert>
#include <cstdio>

#include "Utils/MemoryMgr.h"
#include "Utils/MemoryMgr.GTA.h"
#include "Utils/Patterns.h"

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

#define DISABLE_FLA_DONATION_WINDOW		0
