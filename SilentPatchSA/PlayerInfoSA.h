#pragma once

#include "PedSA.h"
#include "VehicleSA.h"

class CPlayerInfo
{
private:
	CPlayerPed*		m_pPed;
	uint8_t			__pad2[0xAC];
	CVehicle*		m_pControlledVehicle;
	uint8_t			__pad[0xDC];

public:
	CPlayerPed*		GetPlayerPed() const { return m_pPed; }
	CVehicle*		GetControlledVehicle() const { return m_pControlledVehicle; }
};

CPlayerPed* FindPlayerPed( int playerID = -1 );
CEntity* FindPlayerEntityWithRC( int playerID = -1 );
CVehicle* FindPlayerVehicle( int playerID = -1, bool withRC = false );

static_assert(sizeof(CPlayerInfo) == 0x190, "Wrong size: CPlayerInfo");