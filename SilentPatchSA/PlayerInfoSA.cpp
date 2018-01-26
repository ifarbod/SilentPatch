#include "StdAfxSA.h"
#include "PlayerInfoSA.h"

uint8_t& PlayerInFocus = **AddressByVersion<uint8_t**>( 0x56E218 + 3, 0, 0 ); // TODO: DO
CPlayerInfo* const Players = *AddressByVersion<CPlayerInfo**>( 0x56E225 + 2, 0, 0 );

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
