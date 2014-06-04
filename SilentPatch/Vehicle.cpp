#include "StdAfx.h"

#include "Vehicle.h"
#include "Timer.h"

WRAPPER void CVehicle::SetComponentAtomicAlpha(RpAtomic* pAtomic, int nAlpha) { WRAPARG(pAtomic); WRAPARG(nAlpha); EAXJMP(0x6D2960); }
WRAPPER void CVehicle::Render() { EAXJMP(0x6D0E60); }

static RwObject* GetCurrentAtomicObjectCB(RwObject* pObject, void* data)
{
	if ( RpAtomicGetFlags(pObject) & rpATOMICRENDER )
	{
		*static_cast<RwObject**>(data) = pObject;
		return nullptr;
	}
	return pObject;
}

void CHeli::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bHasMovingRotor = m_pCarNode[12] != nullptr;
	bool		bHasMovingRotor2 = m_pCarNode[14] != nullptr;

	m_nTimeTillWeNeedThisCar = *CTimer::m_snTimeInMilliseconds + 3000;

	if ( m_fRotorSpeed > 0.0 )
		dRotorsSpeed = min(1.7 * (1.0/0.22) * m_fRotorSpeed, 1.5);
	else
		dRotorsSpeed = 0.0;

	dMovingRotorSpeed = dRotorsSpeed - 0.4;
	if ( dMovingRotorSpeed < 0.0 )
		dMovingRotorSpeed = 0.0;

	int			nStaticRotorAlpha = static_cast<int>(min((1.5-dRotorsSpeed) * 255.0, 255));
	int			nMovingRotorAlpha = static_cast<int>(min(dMovingRotorSpeed * 175.0, 175));

	if ( m_pCarNode[11] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[11], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[13], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingRotor2 ? nStaticRotorAlpha : 255);
	}

	if ( bHasMovingRotor )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[12], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, nMovingRotorAlpha);
	}

	if ( bHasMovingRotor2 )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[14], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, nMovingRotorAlpha);
	}

	CEntity::Render();
}

void CPlane::Render()
{
	double		dRotorsSpeed, dMovingRotorSpeed;
	bool		bHasMovingProp = m_pCarNode[12] != nullptr;
	bool		bHasMovingProp2 = m_pCarNode[14] != nullptr;

	m_nTimeTillWeNeedThisCar = *CTimer::m_snTimeInMilliseconds + 3000;

	if ( m_fPropellerSpeed > 0.0 )
		dRotorsSpeed = min(1.7 * (1.0/0.31) * m_fPropellerSpeed, 1.5);
	else
		dRotorsSpeed = 0.0;

	dMovingRotorSpeed = dRotorsSpeed - 0.4;
	if ( dMovingRotorSpeed < 0.0 )
		dMovingRotorSpeed = 0.0;

	int			nStaticRotorAlpha = static_cast<int>(min((1.5-dRotorsSpeed) * 255.0, 255));
	int			nMovingRotorAlpha = static_cast<int>(min(dMovingRotorSpeed * 175.0, 175));

	if ( m_pCarNode[11] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[11], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp ? nStaticRotorAlpha : 255);
	}

	if ( m_pCarNode[13] )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[13], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, bHasMovingProp2 ? nStaticRotorAlpha : 255);
	}

	if ( bHasMovingProp )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[12], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, nMovingRotorAlpha);
	}

	if ( bHasMovingProp2 )
	{
		RpAtomic*	pOutAtomic = nullptr;
		RwFrameForAllObjects(m_pCarNode[14], GetCurrentAtomicObjectCB, &pOutAtomic);
		if ( pOutAtomic )
			SetComponentAtomicAlpha(pOutAtomic, nMovingRotorAlpha);
	}

	CVehicle::Render();
}