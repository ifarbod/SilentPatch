#include "StdAfx.h"
#include "VehicleVC.h"

int32_t CVehicle::GetOneShotOwnerID_SilentPatch() const
{
	if ( m_pDriver != nullptr )
	{
		// TODO: Define this as a proper CPhysical
		uintptr_t ptr = reinterpret_cast<uintptr_t>(m_pDriver);
		return *reinterpret_cast<int32_t*>( ptr + 0x64 );
	}

	// Fallback
	return m_audioEntityId;
}
