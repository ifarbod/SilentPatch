#include "StdAfx.h"
#include "ModelInfoIII.h"
#include "Common.h"

#include "RWUtils.hpp"

RpAtomic* (*CVehicleModelInfo::SetEnvironmentMapCB)(RpAtomic* atomic, void* data) = hook::get_pattern<RpAtomic*(RpAtomic*, void*)>("8B 5C 24 14 C6 44 24", -5);

// This is actually CBaseModelInfo, but we currently don't have it defined
CVehicleModelInfo**& ms_modelInfoPtrs = *hook::get_pattern<CVehicleModelInfo**>("8B 2C 85 ? ? ? ? 89 E9", 3);
int32_t& numModelInfos = *hook::get_pattern<int32_t>("81 FD ? ? ? ? 7C B7 31 C0", 2);

static void RemoveSpecularityFromAtomic(RpAtomic* atomic)
{
	RpGeometry* geometry = RpAtomicGetGeometry(atomic);
	if (geometry != nullptr)
	{
		RpGeometryForAllMaterials(geometry, [](RpMaterial* material)
			{
				bool bRemoveSpecularity = false;

				// Only remove specularity from the body materials, keep glass intact.
				// This is only done on a best-effort basis, as mods can fine-tune it better
				// and just remove the model from the exceptions list
				RwTexture* texture = RpMaterialGetTexture(material);
				if (texture != nullptr)
				{
					if (strstr(RwTextureGetName(texture), "glass") == nullptr && strstr(RwTextureGetMaskName(texture), "glass") == nullptr)
					{
						bRemoveSpecularity = true;
					}
				}

				if (bRemoveSpecularity)
				{
					RpMaterialGetSurfaceProperties(material)->specular = 0.0f;
				}
				return material;
			});
	}
}

void CSimpleModelInfo::SetNearDistanceForLOD_SilentPatch()
{
	// 100.0f for real LOD's, 0.0f otherwise
	m_lodDistances[2] = _strnicmp( m_name, "lod", 3 ) == 0 ? 100.0f : 0.0f;
}

void CVehicleModelInfo::SetEnvironmentMap_ExtraComps()
{
	std::invoke(orgSetEnvironmentMap, this);

	const int32_t modelID = std::distance(ms_modelInfoPtrs, std::find(ms_modelInfoPtrs, ms_modelInfoPtrs+numModelInfos, this));
	const bool bRemoveSpecularity = ExtraCompSpecularity::SpecularityExcluded(modelID);
	for (int32_t i = 0; i < m_numComps; i++)
	{
		if (bRemoveSpecularity)
		{
			RemoveSpecularityFromAtomic(m_comps[i]);
		}

		SetEnvironmentMapCB(m_comps[i], m_envMap);
		AttachCarPipeToRwObject(reinterpret_cast<RwObject*>(m_comps[i]));
	}
}
