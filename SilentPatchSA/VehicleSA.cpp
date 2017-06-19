#include "StdAfxSA.h"

#include <algorithm>
#include <vector>
#include "VehicleSA.h"
#include "TimerSA.h"
#include "DelimStringReader.h"

std::vector<unsigned int>		vecRotorExceptions;

static bool ShouldIgnoreRotor( unsigned int id )
{
	return std::find( vecRotorExceptions.begin(), vecRotorExceptions.end(), id ) != vecRotorExceptions.end();
}

static void*	varVehicleRender = AddressByVersion<void*>(0x6D0E60, 0x6D1680, 0x70C0B0);
WRAPPER void CVehicle::Render() { VARJMP(varVehicleRender); }
static void*	varIsLawEnforcementVehicle = AddressByVersion<void*>(0x6D2370, 0x6D2BA0, 0x70D8C0);
WRAPPER bool CVehicle::IsLawEnforcementVehicle() { VARJMP(varIsLawEnforcementVehicle); }

static int32_t random(int32_t from, int32_t to)
{
	return from + ( Int32Rand() % (to-from) );
}

static RwObject* GetCurrentAtomicObjectCB(RwObject* pObject, void* data)
{
	if ( RpAtomicGetFlags(pObject) & rpATOMICRENDER )
	{
		*static_cast<RwObject**>(data) = pObject;
		return nullptr;
	}
	return pObject;
}

static RwFrame* GetFrameFromNameCB(RwFrame* pFrame, void* pData)
{
	// Is this a frame we want?
	std::pair<const char*,RwFrame*>*	pFindData = static_cast<std::pair<const char*,RwFrame*>*>(pData);
	if ( !strcmp(pFindData->first, GetFrameNodeName(pFrame)) )
	{
		pFindData->second = pFrame;
		return nullptr;
	}

	// Try children
	RwFrameForAllChildren(pFrame, GetFrameFromNameCB, pData);
	return !pFindData->second ? pFrame : nullptr;
}

static RpMaterial* SetCompAlphaCB(RpMaterial* pMaterial, void* data)
{
	pMaterial->color.alpha = reinterpret_cast<RwUInt8>(data);
	return pMaterial;
}

void ReadRotorFixExceptions(const wchar_t* pPath)
{
	const size_t SCRATCH_PAD_SIZE = 32767;
	WideDelimStringReader reader( SCRATCH_PAD_SIZE );

	GetPrivateProfileSectionW( L"RotorFixExceptions", reader.GetBuffer(), reader.GetSize(), pPath );
	while ( const wchar_t* str = reader.GetString() )
	{
		unsigned int toList = _wtoi( str );
		if ( toList != 0 )
			vecRotorExceptions.push_back( toList );
	}
}

void CVehicle::SetComponentAtomicAlpha(RpAtomic* pAtomic, int nAlpha)
{
	RpGeometry*	pGeometry = RpAtomicGetGeometry(pAtomic);
	pGeometry->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;

	RpGeometryForAllMaterials(pGeometry, SetCompAlphaCB, reinterpret_cast<void*>(nAlpha));
}

bool CVehicle::CustomCarPlate_TextureCreate(CVehicleModelInfo* pModelInfo)
{
	char		PlateText[10];
	const char*	pOverrideText = pModelInfo->GetCustomCarPlateText();

	if ( pOverrideText )
		strcpy_s(PlateText, pOverrideText);
	else
		CCustomCarPlateMgr::GeneratePlateText(PlateText, 8);

	PlateTexture = CCustomCarPlateMgr::CreatePlateTexture(PlateText, pModelInfo->m_nPlateType);
	//PlateDesign = pModelInfo->m_nPlateType != -1 ? pModelInfo->m_nPlateType : CCustomCarPlateMgr::GetMapRegionPlateDesign();
	if ( pModelInfo->m_nPlateType != -1 )
		PlateDesign = pModelInfo->m_nPlateType;
	else if ( IsLawEnforcementVehicle() )
		PlateDesign = CCustomCarPlateMgr::GetMapRegionPlateDesign();
	else
 		PlateDesign = random(0, 20) == 0 ? random(0, 3) : CCustomCarPlateMgr::GetMapRegionPlateDesign();

	assert(PlateDesign >= 0 && PlateDesign < 3);

	pModelInfo->m_plateText[0] = '\0';
	pModelInfo->m_nPlateType = -1;

	return true;
}

//static RwTexture*		pPushedTextures[NUM_MAX_PLATES];

void CVehicle::CustomCarPlate_BeforeRenderingStart(CVehicleModelInfo* pModelInfo)
{
	//CCustomCarPlateMgr::SetupPlates(reinterpret_cast<RpClump*>(pModelInfo->pRwObject), PlateTexture, PlateDesign);
	for ( ptrdiff_t i = 0; i < NUM_MAX_PLATES; i++ )
	{
		if ( pModelInfo->m_apPlateMaterials[i] )
		{
			//RwTexture*	pPlateTex = RpMaterialGetTexture(pModelInfo->m_apPlateMaterials[i]);

			//RwTextureAddRef(pPlateTex);
			//pPushedTextures[i] = pPlateTex;

			RpMaterialSetTexture(pModelInfo->m_apPlateMaterials[i], PlateTexture);
		}

		if ( pModelInfo->m_apPlateMaterials[NUM_MAX_PLATES+i] )
			CCustomCarPlateMgr::SetupMaterialPlatebackTexture(pModelInfo->m_apPlateMaterials[NUM_MAX_PLATES+i], PlateDesign);
			//RwTexture*	pPlatebackTex = RpMaterialGetTexture(pModelInfo->m_apPlateMaterials[4+i]);

			//RwTextureAddRef(pPlatebackTex);
			//pPushedTextures[4+i] = pPlateTex;

			//RpMaterialSetTexture(pModelInfo->m_apPlateMaterials[i], PlateTexture);
	}
}

// This is not needed
/*void CVehicle::CustomCarPlate_AfterRenderingStop(CVehicleModelInfo* pModelInfo)
{
	for ( int i = 0; i < NUM_MAX_PLATES; i++ )
	{
		if ( pModelInfo->m_apPlateMaterials[i] )
		{
			//RwTexture*	pPlateTex = RpMaterialGetTexture(pModelInfo->m_apPlateMaterials[i]);

			//RwTextureAddRef(pPlateTex);
			//pPushedTextures[i] = pPlateTex;

			//RpMaterialSetTexture(pModelInfo->m_apPlateMaterials[i], pPushedTextures[i]);
			//RwTextureDestroy(pPushedTextures[i]);
			//pPushedTextures[i] = nullptr;
		}
	}

}*/

void CHeli::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bDisplayRotors = !ShouldIgnoreRotor( FLAUtils::GetExtendedID( &m_nModelIndex ) );
	bool		bHasMovingRotor = m_pCarNode[13] != nullptr && bDisplayRotors;
	bool		bHasMovingRotor2 = m_pCarNode[15] != nullptr && bDisplayRotors;

	m_nTimeTillWeNeedThisCar = CTimer::m_snTimeInMilliseconds + 3000;

	if ( m_fRotorSpeed > 0.0 )
		dRotorsSpeed = std::min(1.7 * (1.0/0.22) * m_fRotorSpeed, 1.5);
	else
		dRotorsSpeed = 0.0;

	dMovingRotorSpeed = dRotorsSpeed - 0.4;
	if ( dMovingRotorSpeed < 0.0 )
		dMovingRotorSpeed = 0.0;

	int			nStaticRotorAlpha = static_cast<int>(std::min((1.5-dRotorsSpeed) * 255.0, 255.0));
	int			nMovingRotorAlpha = static_cast<int>(std::min(dMovingRotorSpeed * 175.0, 175.0));

	if ( m_pCarNode[12] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[12], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[14] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[14], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor2 ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[13], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor ? nMovingRotorAlpha : 0);
	}

	if ( m_pCarNode[15] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[15], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor2 ? nMovingRotorAlpha : 0);
	}

	CEntity::Render();
}

void CPlane::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bDisplayRotors = !ShouldIgnoreRotor( FLAUtils::GetExtendedID( &m_nModelIndex ) );
	bool		bHasMovingProp = m_pCarNode[13] != nullptr && bDisplayRotors;
	bool		bHasMovingProp2 = m_pCarNode[15] != nullptr && bDisplayRotors;

	m_nTimeTillWeNeedThisCar = CTimer::m_snTimeInMilliseconds + 3000;

	if ( m_fPropellerSpeed > 0.0 )
		dRotorsSpeed = std::min(1.7 * (1.0/0.31) * m_fPropellerSpeed, 1.5);
	else
		dRotorsSpeed = 0.0;

	dMovingRotorSpeed = dRotorsSpeed - 0.4;
	if ( dMovingRotorSpeed < 0.0 )
		dMovingRotorSpeed = 0.0;

	int			nStaticRotorAlpha = static_cast<int>(std::min((1.5-dRotorsSpeed) * 255.0, 255.0));
	int			nMovingRotorAlpha = static_cast<int>(std::min(dMovingRotorSpeed * 175.0, 175.0));

	if ( m_pCarNode[12] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[12], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[14] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[14], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp2 ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[13], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp ? nMovingRotorAlpha : 0);
	}

	if ( m_pCarNode[15] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[15], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp2 ? nMovingRotorAlpha : 0);
	}

	CVehicle::Render();
}

void CPlane::Fix_SilentPatch()
{
	// Reset bouncing panels
	// No reset on Vortex
	const int32_t extID = FLAUtils::GetExtendedID( &m_nModelIndex );
	for ( ptrdiff_t i = extID == 539 ? 1 : 0; i < 3; i++ )
	{
		m_aBouncingPanel[i].m_nNodeIndex = -1;
	}
}

void CAutomobile::Fix_SilentPatch()
{
	ResetFrames();

	// Reset bouncing panels
	const int32_t extID = FLAUtils::GetExtendedID( &m_nModelIndex );
	for ( ptrdiff_t i = (extID == 525 && m_pCarNode[21]) || (extID == 531 && m_pCarNode[17]) ? 1 : 0; i < 3; i++ )
	{
		// Towtruck/Tractor fix
		m_aBouncingPanel[i].m_nNodeIndex = -1;
	}
}

void CAutomobile::ResetFrames()
{
	RpClump*	pOrigClump = reinterpret_cast<RpClump*>(ms_modelInfoPtrs[ FLAUtils::GetExtendedID( &m_nModelIndex ) ]->pRwObject);
	if ( pOrigClump )
	{
		// Instead of setting frame rotation to (0,0,0) like R* did, obtain the original frame matrix from CBaseNodelInfo clump
		for ( ptrdiff_t i = 8; i < 25; i++ )
		{
			if ( m_pCarNode[i] )
			{
				// Find a frame in CBaseModelInfo object
				std::pair<const char*,RwFrame*>		FindData = std::make_pair(GetFrameNodeName(m_pCarNode[i]), nullptr);

				RwFrameForAllChildren(RpClumpGetFrame(pOrigClump), GetFrameFromNameCB, &FindData);

				if ( FindData.second )
				{
					// Found a frame, reset it
					*RwFrameGetMatrix(m_pCarNode[i]) = *RwFrameGetMatrix(FindData.second);
					RwMatrixUpdate(RwFrameGetMatrix(m_pCarNode[i]));
				}
			}
		}
	}
}