#include "StdAfx.h"
#include "General.h"

// Wrappers
WRAPPER bool CalcScreenCoors(const CVector& vecIn, CVector* vecOut) { WRAPARG(vecIn); WRAPARG(vecOut); EAXJMP(0x71DAB0); }
WRAPPER void LoadingScreenLoadingFile(const char* pText) { WRAPARG(pText); EAXJMP(0x5B3680); }

WRAPPER void CEntity::UpdateRW() { EAXJMP(0x446F90); }
WRAPPER void CEntity::RegisterReference(CEntity** pAddress) { WRAPARG(pAddress); EAXJMP(0x571B70); }
WRAPPER void CEntity::CleanUpOldReference(CEntity** pAddress) { WRAPARG(pAddress); EAXJMP(0x571A00); }
WRAPPER void CEntity::Render() { EAXJMP(0x534310); }

static RwTexture*&						ms_pRemapTexture = *(RwTexture**)0xB4E47C;
static unsigned char*					ms_currentCol = *(unsigned char**)0x4C84C8;

WRAPPER RpAtomic* SetEditableMaterialsCB(RpAtomic* pMaterial, void* pData) { WRAPARG(pMaterial); WRAPARG(pData); EAXJMP(0x4C83E0); }

static void SetVehicleColour(unsigned char primaryColour, unsigned char secondaryColour, unsigned char tertiaryColour, unsigned char quaternaryColour)
{
	ms_currentCol[0] = primaryColour;
	ms_currentCol[1] = secondaryColour;
	ms_currentCol[2] = tertiaryColour;
	ms_currentCol[3] = quaternaryColour;
}

static void ResetEditableMaterials(std::pair<void*,int>* pData)
{
	for ( auto* i = pData; i->first; i++ )
		*static_cast<int*>(i->first) = i->second;
}

void CObject::Render()
{
	if ( m_bDoNotRender )
		return;

	bool						bCallRestore;
	std::pair<void*,int>		materialRestoreData[16];

	if ( m_wCarPartModelIndex != -1 && m_nObjectType == 3 && bObjectFlag7 && RwObjectGetType(m_pRwObject) == rpATOMIC )
	{
		auto* pData = materialRestoreData;

		ms_pRemapTexture = m_pPaintjobTex;
		SetVehicleColour(m_nCarColor[0], m_nCarColor[1], m_nCarColor[2], m_nCarColor[3]);

		SetEditableMaterialsCB(reinterpret_cast<RpAtomic*>(m_pRwObject), &pData);
		pData->first = nullptr;

		bCallRestore = true;
	}
	else
		bCallRestore = false;

	CEntity::Render();

	if ( bCallRestore )
		ResetEditableMaterials(materialRestoreData);
}