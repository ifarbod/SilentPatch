#include "Common.h"

#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"
#include "Utils/HookEach.hpp"
#include "StoredCar.h"
#include "SVF.h"

#include "Utils/DelimStringReader.h"

#include <array>

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
	static RwUInt32 alphaTestVal;

	static RwBool RenderStateSet_StoreAlphaTest( RwRenderState state, void* value )
	{
		RwD3D8GetRenderState( 15, &alphaTestVal ); // D3DRS_ALPHATESTENABLE
		RwD3D8SetRenderState( 15, FALSE );

		return RwRenderStateSet( state, value );
	}

	static RwBool RenderStateSet_RestoreAlphaTest( RwRenderState state, void* value )
	{
		RwBool result = RwRenderStateSet( state, value );

		RwD3D8SetRenderState( 15, alphaTestVal );

		return result;
	}
};

// ============= Corrected corona placement for taxi =============
namespace TaxiCoronaFix
{
	CVector& GetTransformedCoronaPos( CVector& out, float offsetZ, const CAutomobile* vehicle )
	{
		CVector pos;
		pos.x = 0.0f;
		if ( SVF::ModelHasFeature( vehicle->GetModelIndex(), SVF::Feature::TAXI_LIGHT ) )
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


// ============= Reset requested extras if created vehicle has no extras =============
namespace CompsToUseFix
{
	static int8_t* ms_compsUsed = *hook::get_pattern<int8_t*>( "89 E9 88 1D", 4 );
	static int8_t* ms_compsToUse = *hook::get_pattern<int8_t*>( "0F BE 05 ? ? ? ? 83 C4 28", 3 );
	static void ResetCompsForNoExtras()
	{
		ms_compsUsed[0] = ms_compsUsed[1] = -1;
		ms_compsToUse[0] = ms_compsToUse[1] = -2;
	}
};


// ============= Extra component specularity exceptions =============
namespace ExtraCompSpecularity
{
	void ReadExtraCompSpecularityExceptions(const wchar_t* pPath)
	{
		constexpr size_t SCRATCH_PAD_SIZE = 32767;
		WideDelimStringReader reader(SCRATCH_PAD_SIZE);

		GetPrivateProfileSectionW(L"ExtraCompSpecularityExceptions", reader.GetBuffer(), reader.GetSize(), pPath);
		while (const wchar_t* str = reader.GetString())
		{
			int32_t toList = wcstol(str, nullptr, 0);
			if ( toList > 0 )
				SVF::RegisterFeature(toList, SVF::Feature::_INTERNAL_NO_SPECULARITY_ON_EXTRAS);
		}
	}

	bool SpecularityExcluded(int32_t modelID)
	{
		return SVF::ModelHasFeature(modelID, SVF::Feature::_INTERNAL_NO_SPECULARITY_ON_EXTRAS);
	}
}


// ============= Make script randomness 16-bit, like on PS2 =============
namespace Rand16bit
{
	template<std::size_t Index>
	static int (*orgRand)();

	template<std::size_t Index>
	static int rand16bit()
	{
		const int bottomBits = orgRand<Index>();
		const int topBit = (orgRand<Index>() & 1) << 15;
		return bottomBits | topBit;
	}

	HOOK_EACH_FUNC(Rand, orgRand, rand16bit);
}


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
				void* disableAlphaTestAndSetState[] = { 
					get_pattern( "E8 ? ? ? ? 59 59 6A 00 6A 0E E8 ? ? ? ? 31 C0" ),
					get_pattern( "E8 ? ? ? ? 0F B7 2D ? ? ? ? 31 C0" )
				};

				void* setStateAndReenableAlphaTest[] = {
					get_pattern( "E8 ? ? ? ? 59 59 6A 01 6A 08 E8 ? ? ? ? 59 59 83 C4 38" ),
					get_pattern( "39 44 24 38 0F 8C ? ? ? ? 6A 00 6A 0C", 14 )
				};
#elif _GTA_VC
				void* disableAlphaTestAndSetState[] = { 
					get_pattern( "E8 ? ? ? ? 59 59 6A 00 6A 0E E8 ? ? ? ? 31 C0" ),
					get_pattern( "6A 01 6A 0C E8 ? ? ? ? 59 59 6A 03", 4 )
				};

				void* setStateAndReenableAlphaTest[] = {
					get_pattern( "0F 77 6A 00 6A 0C E8 ? ? ? ? 59", 6 ),
					get_pattern( "39 44 24 34 0F 8C ? ? ? ? 6A 00 6A 0C", 14 )
				};
#endif

				for ( auto match : disableAlphaTestAndSetState )
				{
					InjectHook( match, RenderStateSet_StoreAlphaTest );
				}

				for ( auto match : setStateAndReenableAlphaTest )
				{
					InjectHook( match, RenderStateSet_RestoreAlphaTest );
				}
			}


			// Reset requested extras if created vehicle has no extras
			{
				using namespace CompsToUseFix;

				auto resetComps = pattern( "8B 04 24 83 C4 08 5D 5F" ).get_one();
				InjectHook( resetComps.get<void>( -14 ), ResetCompsForNoExtras, HookType::Call );
				Nop( resetComps.get<void>( -9 ), 9 );
			}


			// Rescale light switching randomness in CAutomobile::PreRender/CBike::PreRender for PC the randomness range
			// The original randomness was 50000 out of 65535, which is impossible to hit with PC's 32767 range
			{
				// GTA III expects 2 matches, VC expects 4 due to the addition of CBike::PreRender
#if _GTA_III
				constexpr uint32_t expected = 2;
#else
				constexpr uint32_t expected = 4;
#endif
				auto matches = pattern("D8 0D ? ? ? ? D8 1D ? ? ? ? DF E0 80 E4 05 80 FC 01").count(expected);

				matches.for_each_result([](pattern_match match)
				{
					static const float LightStatusRandomnessThreshold = 1.0f / 25000.0f;
					Patch<const void*>(match.get<void>(2), &LightStatusRandomnessThreshold);
				});
			}


			// Make script randomness 16-bit, like on PS2
			{
				using namespace Rand16bit;

				std::array<void*, 2> rands = {
					get_pattern("E8 ? ? ? ? 0F B7 C0 89 06"),
					get_pattern("E8 ? ? ? ? 25 FF FF 00 00 89 84 24 ? ? ? ? 30 C0"),
				};

				HookEach_Rand(rands, InterceptCall);
			}
		}

		void III_VC_SetDelayedPatchesFunc( void(*func)() )
		{
			DelayedPatches::Func = std::move(func);
		}

		void III_VC_DelayedCommon( bool /*hasDebugMenu*/, const wchar_t* wcModulePath )
		{
			using namespace Memory;
			using namespace hook;

			ExtraCompSpecularity::ReadExtraCompSpecularityExceptions(wcModulePath);

			// Corrected taxi light placement for Taxi
			if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableVehicleCoronaFixes", -1, wcModulePath) == 1 )
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
	}
}