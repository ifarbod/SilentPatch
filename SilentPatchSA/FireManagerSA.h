#pragma once

#include "GeneralSA.h"

class CFireManager
{
public:
	static void (CFireManager::*orgStartFire)( CEntity* entity, CEntity* attacker, float a3, uint8_t a4, uint32_t a5, int8_t a6 );

	void StartFire( CEntity* entity, CEntity* attacker, float a3, uint8_t a4, uint32_t a5, int8_t a6 )
	{
		(this->*orgStartFire)( entity, attacker, a3, a4, a5, a6 );
	}

	void StartFire_NullEntityCheck( CEntity* entity, CEntity* attacker, float a3, uint8_t a4, uint32_t a5, int8_t a6 )
	{
		if ( entity != nullptr )
		{
			StartFire( entity, attacker, a3, a4, a5, a6 );
		}
	}
};
