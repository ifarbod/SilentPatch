#include "StdAfxSA.h"
#include "ModelInfoSA.h"

static void* BaseModelInfoShutdown = AddressByVersion<void*>(0x4C4D50, 0x4C4DD0, 0x4CF590);
WRAPPER void CBaseModelInfo::Shutdown() { VARJMP(BaseModelInfoShutdown); }

RwTexture* (*CCustomCarPlateMgr::CreatePlateTexture)(const char* pText, signed char nDesign) = AddressByVersion<RwTexture*(*)(const char*,signed char)>(0x6FDEA0, 0x6FE6D0, 0x736AC0);
bool (*CCustomCarPlateMgr::GeneratePlateText)(char* pBuf, int nLen) = AddressByVersion<bool(*)(char*,int)>(0x6FD5B0, 0x6FDDE0, 0x7360F0);
signed char (*CCustomCarPlateMgr::GetMapRegionPlateDesign)() = AddressByVersion<signed char(*)()>(0x6FD7A0, 0x6FDFD0, 0x7363E0);
void (*CCustomCarPlateMgr::SetupMaterialPlatebackTexture)(RpMaterial* pMaterial, signed char nDesign) = AddressByVersion<void(*)(RpMaterial*,signed char)>(0x6FDE50, 0x6FE680, 0x736A80);

CBaseModelInfo** const			ms_modelInfoPtrs = *AddressByVersion<CBaseModelInfo***>(0x509CB1, 0x4C0C96, 0x403DB7);
const uint32_t					m_numModelInfoPtrs = *AddressByVersion<uint16_t*>(0x4C5956+2, 0, 0);

void CVehicleModelInfo::Shutdown()
{
	CBaseModelInfo::Shutdown();

	delete m_apPlateMaterials;
	m_apPlateMaterials = nullptr;
}

void CVehicleModelInfo::FindEditableMaterialList()
{
	int materialCount = 0;

	auto GetEditableMaterialListCB = [&]( RpAtomic* atomic ) -> RpAtomic* {
		RpGeometryForAllMaterials( RpAtomicGetGeometry(atomic), [&]( RpMaterial* material ) -> RpMaterial* {
			if ( RwTexture* texture = RpMaterialGetTexture(material) )
			{
				if ( const char* texName = RwTextureGetName(texture) )
				{
					if ( _strnicmp(texName, "vehiclegrunge256", 16) == 0 )
					{
						if ( materialCount < _countof(m_apDirtMaterials) )
							m_apDirtMaterials[materialCount++] = material;
					}
				}
			}
			return material;
		} );
		return atomic;
	};

	RpClumpForAllAtomics(reinterpret_cast<RpClump*>(pRwObject), GetEditableMaterialListCB);

	if ( m_pVehicleStruct->m_nNumExtras > 0 )
	{
		for ( int i = 0; i < m_pVehicleStruct->m_nNumExtras; i++ )
			GetEditableMaterialListCB(m_pVehicleStruct->m_apExtras[i]);
	}

	m_nPrimaryColor = -1;
	m_nSecondaryColor = -1;
	m_nTertiaryColor = -1;
	m_nQuaternaryColor = -1;
}

void CVehicleModelInfo::SetCarCustomPlate()
{
	m_plateText[0] = '\0';
	m_nPlateType = -1;

	m_apPlateMaterials = new RpMaterial* [2*CCustomCarPlateMgr::NUM_MAX_PLATES];
	
	for ( int i = 0; i < 2*CCustomCarPlateMgr::NUM_MAX_PLATES; i++ )
		m_apPlateMaterials[i] = nullptr;
	CCustomCarPlateMgr::SetupClump(reinterpret_cast<RpClump*>(pRwObject), m_apPlateMaterials);
}

void CCustomCarPlateMgr::PollPlates( RpClump* clump, RpMaterial** materials )
{
	RpMaterial** carplates = materials;
	RpMaterial** carpbacks = materials+NUM_MAX_PLATES;

	int numCarplates = 0, numCarpbacks = 0;

	RpClumpForAllAtomics( clump, [carplates, carpbacks, &numCarplates, &numCarpbacks] ( RpAtomic* atomic ) -> RpAtomic* {
		RpGeometryForAllMaterials( RpAtomicGetGeometry(atomic), [&] ( RpMaterial* material ) -> RpMaterial* {
			if ( RwTexture* texture = RpMaterialGetTexture(material) )
			{
				if ( const char* texName = RwTextureGetName(texture) )
				{
					if ( _strnicmp( texName, "carplate", 8 ) == 0 )
					{
						assert(numCarplates < NUM_MAX_PLATES);
						if ( numCarplates < NUM_MAX_PLATES );
						{
							carplates[numCarplates++] = material;
						}
					}
					else if ( _strnicmp( texName, "carpback", 8 ) == 0 )
					{
						assert(numCarpbacks < NUM_MAX_PLATES);
						if ( numCarpbacks < NUM_MAX_PLATES );
						{
							carpbacks[numCarpbacks++] = material;
						}
					}
				}
			}

			return material;
		} );
		return atomic;
	} );
}

void CCustomCarPlateMgr::SetupClump(RpClump* pClump, RpMaterial** pMatsArray)
{
	PollPlates( pClump, pMatsArray );
}

void CCustomCarPlateMgr::SetupClumpAfterVehicleUpgrade(RpClump* pClump, RpMaterial** pMatsArray, signed char nDesign)
{
	UNREFERENCED_PARAMETER(nDesign);
	if ( pMatsArray != nullptr )
	{
		PollPlates( pClump, pMatsArray );
	}
}