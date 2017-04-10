#include "StdAfxSA.h"
#include "ModelInfoSA.h"

#include <iterator>

static void* BaseModelInfoShutdown = AddressByVersion<void*>(0x4C4D50, 0x4C4DD0, 0x4CF590);
WRAPPER void CBaseModelInfo::Shutdown() { VARJMP(BaseModelInfoShutdown); }

RwTexture* (*CCustomCarPlateMgr::CreatePlateTexture)(const char* pText, signed char nDesign) = AddressByVersion<RwTexture*(*)(const char*,signed char)>(0x6FDEA0, 0x6FE6D0, 0x736AC0);
bool (*CCustomCarPlateMgr::GeneratePlateText)(char* pBuf, int nLen) = AddressByVersion<bool(*)(char*,int)>(0x6FD5B0, 0x6FDDE0, 0x7360F0);
signed char (*CCustomCarPlateMgr::GetMapRegionPlateDesign)() = AddressByVersion<signed char(*)()>(0x6FD7A0, 0x6FDFD0, 0x7363E0);
void (*CCustomCarPlateMgr::SetupMaterialPlatebackTexture)(RpMaterial* pMaterial, signed char nDesign) = AddressByVersion<void(*)(RpMaterial*,signed char)>(0x6FDE50, 0x6FE680, 0x736A80);

CBaseModelInfo** const			ms_modelInfoPtrs = *AddressByVersion<CBaseModelInfo***>(0x509CB1, 0x4C0C96, 0x403DB7);
const uint32_t					m_numModelInfoPtrs = *AddressByVersion<uint16_t*>(0x4C5956+2, 0, 0);


static RwTexture** const		ms_aDirtTextures = *AddressByVersion<RwTexture***>( 0x5D5DCC + 3, 0, 0 /*TODO*/);
void RemapDirt( CVehicleModelInfo* modelInfo, uint32_t dirtID )
{
	RpMaterial** materials = modelInfo->m_numDirtMaterials > CVehicleModelInfo::IN_PLACE_BUFFER_DIRT_SIZE ? modelInfo->m_dirtMaterials : modelInfo->m_staticDirtMaterials;

	for ( size_t i = 0; i < modelInfo->m_numDirtMaterials; i++ )
	{
		RpMaterialSetTexture( materials[i], ms_aDirtTextures[dirtID] );
	}
}

void CVehicleModelInfo::Shutdown()
{
	CBaseModelInfo::Shutdown();

	delete m_dirtMaterials;
	m_dirtMaterials = nullptr;

	delete m_apPlateMaterials;
	m_apPlateMaterials = nullptr;
}

void CVehicleModelInfo::FindEditableMaterialList()
{
	std::vector<RpMaterial*> editableMaterials;

	auto GetEditableMaterialListCB = [&]( RpAtomic* atomic ) -> RpAtomic* {
		RpGeometryForAllMaterials( RpAtomicGetGeometry(atomic), [&]( RpMaterial* material ) -> RpMaterial* {
			if ( RwTexture* texture = RpMaterialGetTexture(material) )
			{
				if ( const char* texName = RwTextureGetName(texture) )
				{
					if ( _stricmp(texName, "vehiclegrunge256") == 0 )
					{
						editableMaterials.push_back( material );
					}
				}
			}
			return material;
		} );
		return atomic;
	};

	RpClumpForAllAtomics(reinterpret_cast<RpClump*>(pRwObject), GetEditableMaterialListCB);

	for ( uint32_t i = 0; i < m_pVehicleStruct->m_nNumExtras; i++ )
		GetEditableMaterialListCB(m_pVehicleStruct->m_apExtras[i]);

	m_numDirtMaterials = editableMaterials.size();
	if ( m_numDirtMaterials > IN_PLACE_BUFFER_DIRT_SIZE )
	{
		m_dirtMaterials = new RpMaterial* [m_numDirtMaterials];
		std::copy( editableMaterials.begin(), editableMaterials.end(), stdext::checked_array_iterator<RpMaterial**>(m_dirtMaterials, m_numDirtMaterials) );
	}
	else
	{
		m_dirtMaterials = nullptr;

		// Use existing space instead of allocating new space
		std::copy( editableMaterials.begin(), editableMaterials.end(), m_staticDirtMaterials );
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
	std::fill_n( m_apPlateMaterials, 2*CCustomCarPlateMgr::NUM_MAX_PLATES, nullptr );
	
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
					if ( _stricmp( texName, "carplate" ) == 0 )
					{
						assert(numCarplates < NUM_MAX_PLATES);
						if ( numCarplates < NUM_MAX_PLATES )
						{
							carplates[numCarplates++] = material;
						}
					}
					else if ( _stricmp( texName, "carpback" ) == 0 )
					{
						assert(numCarpbacks < NUM_MAX_PLATES);
						if ( numCarpbacks < NUM_MAX_PLATES )
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