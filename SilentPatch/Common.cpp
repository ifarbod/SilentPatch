#include "Common.h"

#include "MemoryMgr.h"
#include "Patterns.h"
#include "StoredCar.h"

namespace Common {
	namespace Patches {
		void III_VC_Common()
		{
			using namespace Memory;
			using namespace hook;

			// Fixed bomb ownership/bombs saving for bikes
			{
				auto addr = get_pattern( "83 3C 33 00 74 19 89 F9 E8", 8 );

				void* pRestoreCar;
				ReadCall( addr, pRestoreCar );
				CStoredCar::orgRestoreCar = *(decltype(CStoredCar::orgRestoreCar)*)&pRestoreCar;
				InjectHook( addr, &CStoredCar::RestoreCar_SilentPatch );
			}
		}
	}
}