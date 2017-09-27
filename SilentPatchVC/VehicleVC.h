#pragma once

#include <cstdint>

enum eVehicleType
{
	VEHICLE_AUTOMOBILE,
	VEHICLE_BOAT,
	VEHICLE_TRAIN,
	VEHICLE_HELI,
	VEHICLE_PLANE,
	VEHICLE_BIKE
};

class CVehicle
{
private:
	uint8_t		__pad1[510];
	uint8_t		m_BombOnBoard : 3;
	uint8_t		__pad2[17];
	class CEntity* m_pBombOwner;
	uint8_t		__pad3[136];
	uint32_t m_dwVehicleClass;


public:
	uint32_t		GetClass() const
		{ return m_dwVehicleClass; }

	void			SetBombOnBoard( uint32_t bombOnBoard )
		{ m_BombOnBoard = bombOnBoard; }
	void			SetBombOwner( class CEntity* owner )
		{ m_pBombOwner = owner; }
};

static_assert(sizeof(CVehicle) == 0x2A0, "Wrong size: CVehicle");