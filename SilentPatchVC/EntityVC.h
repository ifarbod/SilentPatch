#pragma 

#include <cstdint>
#include <cstddef>

#include "TheFLAUtils.h"

enum // m_objectCreatedBy
{
	GAME_OBJECT = 1,
	MISSION_OBJECT = 2,
	TEMP_OBJECT = 3,
};

class CEntity
{
public:
	std::byte		__pad[80];
	uint8_t			m_nType : 3;
	std::byte		__pad2[11];
	FLAUtils::int16	m_modelIndex;
};
