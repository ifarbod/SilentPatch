#pragma once

#include <cstdint>

class CSimpleModelInfo
{
private:
	void*	__vmt;
	char	m_name[24];
	uint8_t __pad[32];
	float	m_lodDistances[3];
	uint8_t	__pad2[4];

public:
	void	SetNearDistanceForLOD_SilentPatch();
};

static_assert(sizeof(CSimpleModelInfo) == 0x4C, "Wrong size: CSimpleModelInfo");