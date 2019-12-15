#include "Common.h"

#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"
#include "StoredCar.h"

#include <rwcore.h>

RwCamera*& Camera = **hook::get_pattern<RwCamera**>( "A1 ? ? ? ? D8 88 ? ? ? ?", 1 );

// ============= handling.cfg name matching fix =============
namespace HandlingNameLoadFix
{
	void strncpy_Fix( const char** destination, const char* source, size_t )
	{
		*destination = source;
	}

	int strncmp_Fix( const char* str1, const char** str2, size_t )
	{
		return strcmp( str1, *str2 );
	}
};

// ============= Corona lines rendering fix =============
namespace CoronaLinesFix
{
	static RwBool RenderLine_SetRecipZ( RwIm2DVertex *vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2 )
	{
		const RwReal nearScreenZ = RwIm2DGetNearScreenZ();
		const RwReal nearZ = RwCameraGetNearClipPlane( Camera );
		const RwReal recipZ = 1.0f / nearZ;

		for ( RwInt32 i = 0; i < numVertices; i++ )
		{
			RwIm2DVertexSetScreenZ( &vertices[i], nearScreenZ );
			RwIm2DVertexSetCameraZ( &vertices[i], nearZ );
			RwIm2DVertexSetRecipCameraZ( &vertices[i], recipZ );
		}

		return RwIm2DRenderLine( vertices, numVertices, vert1, vert2 );
	}
}

// ============= Static shadow alpha fix =============
namespace StaticShadowAlphaFix
{
	static void (*orgRenderStaticShadows)();
	static void RenderStaticShadows_StateFix()
	{
		RwUInt32 alphaTestVal = 0;
		RwD3D8GetRenderState( 15, &alphaTestVal ); // D3DRS_ALPHATESTENABLE
		
		RwD3D8SetRenderState( 15, FALSE );
		orgRenderStaticShadows();

		RwD3D8SetRenderState( 15, alphaTestVal );
	}

	static void (*orgRenderStoredShadows)();
	static void RenderStoredShadows_StateFix()
	{
		RwUInt32 alphaTestVal = 0;
		RwD3D8GetRenderState( 15, &alphaTestVal ); // D3DRS_ALPHATESTENABLE
		
		RwD3D8SetRenderState( 15, FALSE );
		orgRenderStoredShadows();

		RwD3D8SetRenderState( 15, alphaTestVal );
	}
};

// ============= Corrected corona placement for taxi =============
namespace TaxiCoronaFix
{
	CVector& GetTransformedCoronaPos( CVector& out, float offsetZ, const CAutomobile* vehicle )
	{
		CVector pos;
		pos.x = 0.0f;
		if ( vehicle->GetModelIndex() == MI_TAXI )
		{
#if _GTA_III
			pos.y = -0.25f;
#elif _GTA_VC
			pos.y = -0.4f;
#endif
			pos.z = 0.9f;
		}
		else
		{
			pos.y = 0.0f;
			pos.z = offsetZ;
		}
		return out = Multiply3x3( vehicle->GetMatrix(), pos );
	}
};

// ============= Delayed patches =============
namespace DelayedPatches
{
	static bool delayedPatchesDone = false;
	void (*Func)();

	static BOOL (*RsEventHandler)(int, void*);
	static void (WINAPI **OldSetPreference)(int a, int b);
	void WINAPI Inject_MSS(int a, int b)
	{
		(*OldSetPreference)(a, b);
		if ( !std::exchange(delayedPatchesDone, true) )
		{
			if ( Func != nullptr ) Func();
			// So we don't have to revert patches
			HMODULE		hDummyHandle;
			GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, (LPCTSTR)&Inject_MSS, &hDummyHandle);
		}
	}
	const auto pInjectMSS = Inject_MSS;

	BOOL Inject_UAL(int a, void* b)
	{
		if ( RsEventHandler(a, b) )
		{
			if ( !std::exchange(delayedPatchesDone, true) && Func != nullptr )
			{
				Func();
			}
			return TRUE;
		}
		return FALSE;
	}

}

namespace Common {
	namespace Patches {
		void III_VC_Common()
		{
			using namespace Memory;
			using namespace hook;

			// Delayed patching
			{
				using namespace DelayedPatches;

				auto addr_mssHook = get_pattern( "6A 00 6A 02 6A 10 68 00 7D 00 00", -6 + 2 );
				OldSetPreference = *static_cast<decltype(OldSetPreference)*>(addr_mssHook);
				Patch( addr_mssHook, &pInjectMSS );

				auto addr_ualHook = get_pattern( "FF 15 ? ? ? ? 6A 00 6A 18", 0xA );
				ReadCall( addr_ualHook, RsEventHandler );
				InjectHook( addr_ualHook, Inject_UAL );
			}

			// Fixed bomb ownership/bombs saving for bikes
			{
				auto addr = get_pattern( "83 3C 33 00 74 19 89 F9 E8", 8 );

				ReadCall( addr, CStoredCar::orgRestoreCar );
				InjectHook( addr, &CStoredCar::RestoreCar_SilentPatch );
			}

			// Fixed handling.cfg name matching (names don't need unique prefixes anymore)
			{
				using namespace HandlingNameLoadFix;

				auto findExactWord = pattern( "8D 44 24 10 83 C4 0C 57" ).get_one();

				InjectHook( findExactWord.get<void>( -5 ), strncpy_Fix );
				InjectHook( findExactWord.get<void>( 0xD ), strncmp_Fix );
			}


			// Fixed corona lines rendering on non-nvidia cards
			{
				using namespace CoronaLinesFix;
	
				auto renderLine = get_pattern( "E8 ? ? ? ? 83 C4 10 FF 44 24 1C 43" );

				InjectHook( renderLine, RenderLine_SetRecipZ );
			}


			// Fixed static shadows not rendering under fire and pickups
			{
				using namespace StaticShadowAlphaFix;

#if _GTA_III
				constexpr ptrdiff_t offset = 0xF;
#elif _GTA_VC
				constexpr ptrdiff_t offset = 0x14;
#endif

				uintptr_t renderStaticShadows = reinterpret_cast<uintptr_t>(ReadCallFrom( get_pattern( "E8 ? ? ? ? A1 ? ? ? ? 85 C0 74 05" ), offset ));
				ReadCall( renderStaticShadows, orgRenderStaticShadows );
				InjectHook( renderStaticShadows, RenderStaticShadows_StateFix );

				renderStaticShadows += 5;
				ReadCall( renderStaticShadows, orgRenderStoredShadows );
				InjectHook( renderStaticShadows, RenderStoredShadows_StateFix );
			}

			// Corrected taxi light placement for Taxi
			// TODO: INI entry
			{
				using namespace TaxiCoronaFix;

				auto getTaxiLightPos = pattern( "E8 ? ? ? ? D9 84 24 ? ? ? ? D8 84 24 ? ? ? ? 83 C4 0C FF 35" );

				if ( getTaxiLightPos.count_hint(1).size() == 1 )
				{
					auto match = getTaxiLightPos.get_one();
					Patch<uint8_t>( match.get<void>( -15 ), 0x55 ); // push eax -> push ebp
					InjectHook( match.get<void>(), GetTransformedCoronaPos );
				}
			}
		}

		void III_VC_SetDelayedPatchesFunc( void(*func)() )
		{
			DelayedPatches::Func = std::move(func);
		}

		void III_VC_DelayedCommon( bool /*hasDebugMenu*/, const wchar_t* /*iniPath*/ )
		{
		
		}
	}
}