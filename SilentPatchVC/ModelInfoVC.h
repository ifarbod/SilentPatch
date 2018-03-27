#pragma once

#include "General.h"
#include <rwcore.h>
#include <rpworld.h>

class CVehicleModelInfo
{
private:
	uint8_t __pad1[60];
	unsigned int	m_dwType;
	uint8_t __pad2[308];

public:
	RwFrame*		GetExtrasFrame( RpClump* clump );
};

static_assert(sizeof(CVehicleModelInfo) == 0x174, "Wrong size: CvehicleModelInfo");