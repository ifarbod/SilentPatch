#include "Common.h"

#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"
#include "StoredCar.h"


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