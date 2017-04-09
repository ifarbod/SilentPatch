#include "StdAfxSA.h"

#include <functional>
#include <algorithm>
#include <vector>
#include "VehicleSA.h"
#include "TimerSA.h"
#include "DelimStringReader.h"

std::vector<uint32_t>		vecRotorExceptions;

static bool ShouldIgnoreRotor( uint32_t id )
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

static RwObject* GetCurrentAtomicObject( RwFrame* frame )
{
	RwObject* obj = nullptr;
	RwFrameForAllObjects( frame, [&obj]( RwObject* object ) -> RwObject* {
		if ( RpAtomicGetFlags(object) & rpATOMICRENDER )
		{
			obj = object;
			return nullptr;
		}
		return object;
	} );
	return obj;
}

static RwFrame* GetFrameFromName( RwFrame* topFrame, const char* name )
{
	class GetFramePredicate
	{
	public:
		RwFrame* foundFrame = nullptr;

		GetFramePredicate( const char* name )
			: m_name( name )
		{
		}

		RwFrame* operator() ( RwFrame* frame )
		{
			if ( strcmp( m_name, GetFrameNodeName(frame) ) == 0 )
			{
				foundFrame = frame;
				return nullptr;
			}
			RwFrameForAllChildren( frame, *this );
			return foundFrame != nullptr ? nullptr : frame;
		}
	
	private:
		const char* const m_name;
	};

	GetFramePredicate p( name );
	RwFrameForAllChildren( topFrame, p );
	return p.foundFrame;
}

void ReadRotorFixExceptions(const wchar_t* pPath)
{
	const size_t SCRATCH_PAD_SIZE = 32767;
	WideDelimStringReader reader( SCRATCH_PAD_SIZE );

	GetPrivateProfileSectionW( L"RotorFixExceptions", reader.GetBuffer(), reader.GetSize(), pPath );
	while ( const wchar_t* str = reader.GetString() )
	{
		uint32_t toList = wcstoul( str, nullptr, 0 );
		if ( toList != 0 )
			vecRotorExceptions.push_back( toList );
	}
}

void CVehicle::SetComponentAtomicAlpha(RpAtomic* pAtomic, int nAlpha)
{
	RpGeometry*	pGeometry = RpAtomicGetGeometry(pAtomic);
	pGeometry->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;

	RpGeometryForAllMaterials( pGeometry, [nAlpha] (RpMaterial* material) {
		material->color.alpha = RwUInt8(nAlpha);
		return material;
	} );
}

bool CVehicle::CustomCarPlate_TextureCreate(CVehicleModelInfo* pModelInfo)
{
	char		PlateText[CVehicleModelInfo::PLATE_TEXT_LEN+1];
	const char*	pOverrideText = pModelInfo->GetCustomCarPlateText();

	if ( pOverrideText )
		strncpy_s(PlateText, pOverrideText, CVehicleModelInfo::PLATE_TEXT_LEN);
	else
		CCustomCarPlateMgr::GeneratePlateText(PlateText, CVehicleModelInfo::PLATE_TEXT_LEN);

	PlateText[CVehicleModelInfo::PLATE_TEXT_LEN] = '\0';
	PlateTexture = CCustomCarPlateMgr::CreatePlateTexture(PlateText, pModelInfo->m_nPlateType);
	if ( pModelInfo->m_nPlateType != -1 )
		PlateDesign = pModelInfo->m_nPlateType;
	else if ( IsLawEnforcementVehicle() )
		PlateDesign = CCustomCarPlateMgr::GetMapRegionPlateDesign();
	else
 		PlateDesign = random(0, 20) == 0 ? int8_t(random(0, 3)) : CCustomCarPlateMgr::GetMapRegionPlateDesign();

	assert(PlateDesign >= 0 && PlateDesign < 3);

	pModelInfo->m_plateText[0] = '\0';
	pModelInfo->m_nPlateType = -1;

	return true;
}

void CVehicle::CustomCarPlate_BeforeRenderingStart(CVehicleModelInfo* pModelInfo)
{
	for ( ptrdiff_t i = 0; i < CCustomCarPlateMgr::NUM_MAX_PLATES; i++ )
	{
		if ( pModelInfo->m_apPlateMaterials[i] != nullptr )
		{
			RpMaterialSetTexture(pModelInfo->m_apPlateMaterials[i], PlateTexture);
		}

		if ( pModelInfo->m_apPlateMaterials[CCustomCarPlateMgr::NUM_MAX_PLATES+i] != nullptr )
		{
			CCustomCarPlateMgr::SetupMaterialPlatebackTexture(pModelInfo->m_apPlateMaterials[CCustomCarPlateMgr::NUM_MAX_PLATES+i], PlateDesign);
		}
	}
}

void CHeli::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bDisplayRotors = !ShouldIgnoreRotor( m_nModelIndex );
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

	if ( m_pCarNode[12] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[12] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[14] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[14] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor2 ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[13] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor ? nMovingRotorAlpha : 0);
	}

	if ( m_pCarNode[15] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[15] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor2 ? nMovingRotorAlpha : 0);
	}

	CEntity::Render();
}

void CPlane::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bDisplayRotors = !ShouldIgnoreRotor( m_nModelIndex );
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

	if ( m_pCarNode[12] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[12] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[14] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[14] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp2 ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[13] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp ? nMovingRotorAlpha : 0);
	}

	if ( m_pCarNode[15] != nullptr )
	{
		RpAtomic*	pOutAtomic = (RpAtomic*)GetCurrentAtomicObject( m_pCarNode[15] );
		if ( pOutAtomic != nullptr )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp2 ? nMovingRotorAlpha : 0);
	}

	CVehicle::Render();
}

void CPlane::Fix_SilentPatch()
{
	// Reset bouncing panels
	// No reset on Vortex
	for ( ptrdiff_t i = m_nModelIndex == 539 ? 1 : 0; i < 3; i++ )
	{
		m_aBouncingPanel[i].m_nNodeIndex = -1;
	}
}

void CAutomobile::Fix_SilentPatch()
{
	ResetFrames();

	// Reset bouncing panels
	for ( ptrdiff_t i = (m_nModelIndex == 525 && m_pCarNode[21]) || (m_nModelIndex == 531 && m_pCarNode[17]) ? 1 : 0; i < 3; i++ )
	{
		// Towtruck/Tractor fix
		m_aBouncingPanel[i].m_nNodeIndex = -1;
	}
}

void CAutomobile::ResetFrames()
{
	RpClump*	pOrigClump = reinterpret_cast<RpClump*>(ms_modelInfoPtrs[m_nModelIndex]->pRwObject);
	if ( pOrigClump != nullptr )
	{
		// Instead of setting frame rotation to (0,0,0) like R* did, obtain the original frame matrix from CBaseNodelInfo clump
		for ( ptrdiff_t i = 8; i < 25; i++ )
		{
			if ( m_pCarNode[i] != nullptr )
			{
				// Find a frame in CBaseModelInfo object
				RwFrame* origFrame = GetFrameFromName( RpClumpGetFrame(pOrigClump), GetFrameNodeName(m_pCarNode[i]) );
				if ( origFrame != nullptr )
				{
					// Found a frame, reset it
					*RwFrameGetMatrix(m_pCarNode[i]) = *RwFrameGetMatrix(origFrame);
					RwMatrixUpdate(RwFrameGetMatrix(m_pCarNode[i]));
				}
			}
		}
	}
}