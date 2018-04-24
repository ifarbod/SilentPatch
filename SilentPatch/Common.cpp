#include "Common.h"

#include "MemoryMgr.h"
#include "Patterns.h"
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

namespace Common {
	namespace Patches {
		void III_VC_Common()
		{
			using namespace Memory;
			using namespace hook;

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
	}
}