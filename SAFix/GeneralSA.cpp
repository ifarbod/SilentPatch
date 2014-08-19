#include "StdAfxSA.h"
#include "GeneralSA.h"

// Wrappers
static void* EntityRender = AddressByVersion<void*>(0x534310, 0, 0);
WRAPPER void CEntity::Render() { VARJMP(EntityRender); }

CWeaponInfo* (*CWeaponInfo::GetWeaponInfo)(eWeaponType, signed char) = AddressByVersion<CWeaponInfo*(*)(eWeaponType, signed char)>(0x743C60, 0, 0);

static RwTexture*&						ms_pRemapTexture = **AddressByVersion<RwTexture***>(0x59F1BD, 0, 0);
static unsigned char*					ms_currentCol = *AddressByVersion<unsigned char**>(0x4C84C8, 0, 0);

auto	SetEditableMaterialsCB = AddressByVersion<RpAtomic*(*)(RpAtomic*,void*)>(0x4C83E0, 0, 0);

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
	if ( m_bDoNotRender || !m_pRwObject )
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