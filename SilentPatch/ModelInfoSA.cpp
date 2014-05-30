#include "StdAfx.h"
#include "ModelInfoSA.h"

void CVehicleModelInfo::FindEditableMaterialList()
{
	std::pair<CVehicleModelInfo*,int>		MatsPair = std::make_pair(this, 0);

	RpClumpForAllAtomics(reinterpret_cast<RpClump*>(pRwObject), GetEditableMaterialListCB, &MatsPair);

	if ( m_pVehicleStruct->m_nNumExtras > 0 )
	{
		for ( int i = 0; i < m_pVehicleStruct->m_nNumExtras; i++ )
			GetEditableMaterialListCB(m_pVehicleStruct->m_apExtras[i], &MatsPair);
	}

	m_nPrimaryColor = -1;
	m_nSecondaryColor = -1;
	m_nTertiaryColor = -1;
	m_nQuaternaryColor = -1;
}

RpAtomic* CVehicleModelInfo::GetEditableMaterialListCB(RpAtomic* pAtomic, void* pData)
{
	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), GetEditableMaterialListCB, pData);
	return pAtomic;
}

RpMaterial* CVehicleModelInfo::GetEditableMaterialListCB(RpMaterial* pMaterial, void* pData)
{
	if ( RpMaterialGetTexture(pMaterial) )
	{
		if ( !strncmp(RwTextureGetName(RpMaterialGetTexture(pMaterial)), "vehiclegrunge256", 16) )
		{
			std::pair<CVehicleModelInfo*,int>*	pMats = static_cast<std::pair<CVehicleModelInfo*,int>*>(pData);

			if ( pMats->second < 32 )
				pMats->first->m_apDirtMaterials[pMats->second++] = pMaterial;
		}
	}
	return pMaterial;
}