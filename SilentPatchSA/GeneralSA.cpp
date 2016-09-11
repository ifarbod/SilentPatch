#include "StdAfxSA.h"
#include "GeneralSA.h"

#include "PedSA.h"

// Wrappers
static void* EntityRender = AddressByVersion<void*>(0x534310, 0x5347B0, 0x545B30);
WRAPPER void CEntity::Render() { VARJMP(EntityRender); }

static void* varInvertRaster = AddressByVersion<void*>(0x705660, 0x705E90, 0x7497A0);
WRAPPER void CShadowCamera::InvertRaster() { VARJMP(varInvertRaster); }

CWeaponInfo* (*CWeaponInfo::GetWeaponInfo)(eWeaponType, signed char) = AddressByVersion<CWeaponInfo*(*)(eWeaponType, signed char)>(0x743C60, 0x744490, 0x77D940);

static RwTexture*&						ms_pRemapTexture = **AddressByVersion<RwTexture***>(0x59F1BD, 0x6D6E53, 0x5B811D);
static unsigned char*					ms_currentCol = *AddressByVersion<unsigned char**>(0x4C84C8, 0x4C86C8, 0x4D2DC8);

auto	SetEditableMaterialsCB = AddressByVersion<RpAtomic*(*)(RpAtomic*,void*)>(0x4C83E0, 0x4C8460, 0x4D2CE0);

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

RpAtomic* ShadowCameraRenderCB(RpAtomic* pAtomic, void* pData)
{
	UNREFERENCED_PARAMETER(pData);

	if ( RpAtomicGetFlags(pAtomic) & rpATOMICRENDER )
	{
		RpGeometry*	pGeometry = RpAtomicGetGeometry(pAtomic);
		RwUInt32	geometryFlags = RpGeometryGetFlags(pGeometry);

		RpGeometrySetFlags(pGeometry, geometryFlags & ~(rpGEOMETRYTEXTURED|rpGEOMETRYPRELIT|
						/*rpGEOMETRYNORMALS|*/rpGEOMETRYLIGHT|rpGEOMETRYMODULATEMATERIALCOLOR|rpGEOMETRYTEXTURED2));

		AtomicDefaultRenderCallBack(pAtomic);
		RpGeometrySetFlags(pGeometry, geometryFlags);
	}
	return pAtomic;
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

		// Disable backface culling for the part
#ifdef _DEBUG
		RwCullMode		oldCullMode;
		RwRenderStateGet(rwRENDERSTATECULLMODE, &oldCullMode);
		assert(oldCullMode == rwCULLMODECULLBACK);
#endif
		RwRenderStateSet(rwRENDERSTATECULLMODE, reinterpret_cast<void*>(rwCULLMODECULLNONE));

		bCallRestore = true;
	}
	else
		bCallRestore = false;

	CEntity::Render();

	if ( bCallRestore )
	{
		ResetEditableMaterials(materialRestoreData);
		RwRenderStateSet(rwRENDERSTATECULLMODE, reinterpret_cast<void*>(rwCULLMODECULLBACK));
	}
}

RwCamera* CShadowCamera::Update(CEntity* pEntity)
{
	RwRGBA	ClearColour = { 255, 255, 255, 0 };
	RwCameraClear(m_pCamera, &ClearColour, rwCAMERACLEARIMAGE|rwCAMERACLEARZ);

	if ( RwCameraBeginUpdate(m_pCamera ) )
	{
		if ( pEntity )
		{
			if ( pEntity->nType == 3 )
				static_cast<CPed*>(pEntity)->RenderForShadow();
			else
				RpClumpForAllAtomics(reinterpret_cast<RpClump*>(pEntity->m_pRwObject), ShadowCameraRenderCB, nullptr);
		}

		InvertRaster();
		RwCameraEndUpdate(m_pCamera);
	}
	return m_pCamera;
}

std::vector<CEntity*>	CEscalator::ms_entitiesToRemove;
void CEscalator::SwitchOffNoRemove()
{
	if ( !m_bExists )
		return;

	for ( ptrdiff_t i = 0, j = field_7C + field_80 + field_84; i < j; ++i )
	{
		if ( m_pSteps[i] != nullptr )
		{
			ms_entitiesToRemove.push_back( m_pSteps[i] );
			m_pSteps[i] = nullptr;
		}
	}

	m_bExists = false;
}