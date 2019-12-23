#include "StdAfx.h"
#include "ModelInfoIII.h"

void CSimpleModelInfo::SetNearDistanceForLOD_SilentPatch()
{
	// 100.0f for real LOD's, 0.0f otherwise
	m_lodDistances[2] = _strnicmp( m_name, "lod", 3 ) == 0 ? 100.0f : 0.0f;
}
