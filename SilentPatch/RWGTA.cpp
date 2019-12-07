#include "RWGTA.h"

#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"

#include <rwcore.h>

// GTA versions of RenderWare functions/macros for GTA III/Vice City
// since we cannot use RwEngineInstance directly

// Anything originally using RwEngineInstance shall be redefined here
// Functions which RW 3.6 inlined can also easily be defined here

void** GTARwEngineInstance = []() -> void** {
	// Thanks Steam III...

	// Locate RwRenderStateSet
	auto renderStateSetPtr = hook::get_pattern( "D1 7C 24 2C", 4 );
	auto renderStateSet = reinterpret_cast<uintptr_t>(Memory::ReadCallFrom( renderStateSetPtr ));

	// Test III 1.0/1.1/VC
	if ( *reinterpret_cast<uint8_t*>(renderStateSet) == 0xA1 )
	{
		return *reinterpret_cast<void***>(renderStateSet + 1);
	}

	// Test III Steam
	renderStateSet += 3;
	if ( *reinterpret_cast<uint8_t*>(renderStateSet) == 0xA1 )
	{
		return *reinterpret_cast<void***>(renderStateSet + 1);
	}

	assert(!"Could not locate RwEngineInstance!");
	return nullptr;
}();


RwBool RwIm2DRenderLine(RwIm2DVertex *vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2)
{
	return GTARWSRCGLOBAL(dOpenDevice).fpIm2DRenderLine( vertices, numVertices, vert1, vert2 );
}

RwReal RwIm2DGetNearScreenZ()
{
	return GTARWSRCGLOBAL(dOpenDevice).zBufferNear;
}