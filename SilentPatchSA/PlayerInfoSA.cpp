#include "StdAfxSA.h"
#include "PlayerInfoSA.h"

uint8_t& PlayerInFocus = **AddressByVersion<uint8_t**>( 0x56E218 + 3, Memory::PatternAndOffset("08 85 C0 79 07 0F B6 05 ? ? ? ? 69 C0 90 01 00 00 8B 80", 8) );
CPlayerInfo* const Players = *AddressByVersion<CPlayerInfo**>( 0x56E225 + 2, Memory::PatternAndOffset("08 85 C0 79 07 0F B6 05 ? ? ? ? 69 C0 90 01 00 00 8B 80", 20) );

CPlayerPed* FindPlayerPed( int playerID )
{
	return Players[ playerID < 0 ? PlayerInFocus : playerID ].GetPlayerPed();
}

CEntity* FindPlayerEntityWithRC( int playerID )
{
	CPlayerInfo* player = &Players[ playerID < 0 ? PlayerInFocus : playerID ];

	CPlayerPed* ped = player->GetPlayerPed();
	CVehicle* remoteVehicle = player->GetControlledVehicle();
	if ( remoteVehicle != nullptr ) return remoteVehicle;
	if ( ped->GetPedFlags().bInVehicle )
	{
		CVehicle* normalVehicle = ped->GetVehiclePtr();
		if ( normalVehicle != nullptr ) return normalVehicle;
	}
	return ped;
}

CVehicle* FindPlayerVehicle( int playerID, bool withRC )
{
	CPlayerInfo* player = &Players[ playerID < 0 ? PlayerInFocus : playerID ];

	CPlayerPed* ped = player->GetPlayerPed();
	if ( ped == nullptr ) return nullptr;
	if ( !ped->GetPedFlags().bInVehicle ) return nullptr;
	CVehicle* vehicle = player->GetControlledVehicle();
	if ( !withRC || vehicle == nullptr )
	{
		vehicle = ped->GetVehiclePtr();
	}
	return vehicle;
}

CPlayerInfo& CPlayerInfo::operator=( const CPlayerInfo& rhs )
{
	memcpy( this, &rhs, sizeof(*this) );
	return *this;
}
