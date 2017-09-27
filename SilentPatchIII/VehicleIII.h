#pragma once

#include <cstdint>

enum eVehicleType
{
	VEHICLE_AUTOMOBILE,
	VEHICLE_BOAT,
	VEHICLE_TRAIN,
	VEHICLE_HELI,
	VEHICLE_PLANE
};

class CVehicle
{
private:
	uint8_t		__pad1[644];
	uint32_t	m_dwVehicleClass;


public:
	uint32_t		GetClass() const
	{ return m_dwVehicleClass; }
};

class CAutomobile : public CVehicle
{
private:
	uint8_t		__pad2[593];
	uint8_t		m_BombOnBoard : 3;
	class CEntity* m_pBombOwner;
	uint8_t		__pad33[200];


public:
	void			SetBombOnBoard( uint32_t bombOnBoard )
	{ m_BombOnBoard = bombOnBoard; }
	void			SetBombOwner( class CEntity* owner )
	{ m_pBombOwner = owner; }
};


static_assert(sizeof(CVehicle) == 0x288, "Wrong size: CVehicle");
static_assert(sizeof(CAutomobile) == 0x5A8, "Wrong size: CAutomobile");