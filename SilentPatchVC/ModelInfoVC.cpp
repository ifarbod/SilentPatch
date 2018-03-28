#include "StdAfx.h"
#include "ModelInfoVC.h"

#include "VehicleVC.h"

auto GetFrameFromId = hook::get_pattern<RwFrame*(RpClump*,int)>( "8B 4C 24 0C 89 04 24", -7 );

RwFrame* CVehicleModelInfo::GetExtrasFrame( RpClump* clump )
{
	RwFrame* frame;
	if ( m_dwType == VEHICLE_HELI || m_dwType == VEHICLE_BIKE )
	{
		frame = GetFrameFromId( clump, 1 );
		if ( frame == nullptr )
		{
			frame = RpClumpGetFrame( clump );
		}
	}
	else
	{
		frame = RpClumpGetFrame( clump );
	}
	return frame;
}