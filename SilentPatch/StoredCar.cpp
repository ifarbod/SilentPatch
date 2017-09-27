#include "StdAfx.h"

#include "StoredCar.h"
#include "Patterns.h"

#if _GTA_III
static auto FindPlayerPed = hook::get_pattern<class CEntity*()>( "6B C0 4F 8B 04 85 ? ? ? ? C3", -7 );
#elif _GTA_VC
static auto FindPlayerPed = hook::get_pattern<class CEntity*()>( "6B C0 2E 8B 04 C5 ? ? ? ? C3", -7 );
#endif

CVehicle* (CStoredCar::*CStoredCar::orgRestoreCar)();

CVehicle* CStoredCar::RestoreCar_SilentPatch()
{
	CVehicle* vehicle = (this->*(orgRestoreCar))();
	if ( vehicle == nullptr ) return nullptr;
	if ( m_bombType != 0 )
	{
		// Fixup bomb stuff
#if _GTA_VC
		if ( vehicle->GetClass() == VEHICLE_AUTOMOBILE || vehicle->GetClass() == VEHICLE_BIKE )
		{
			vehicle->SetBombOnBoard( m_bombType );
			vehicle->SetBombOwner( FindPlayerPed() );
		}
#elif _GTA_III
		static_cast<CAutomobile*>(vehicle)->SetBombOwner( FindPlayerPed() );
#endif
	}

	return vehicle;
}