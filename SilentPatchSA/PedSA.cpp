#include "StdAfxSA.h"
#include "PedSA.h"
#include "VehicleSA.h"

static void* varGetWeaponSkill = AddressByVersion<void*>(0x5E6580, 0x5E6DA0, 0x6039F0);
WRAPPER unsigned char CPed::GetWeaponSkill() { VARJMP(varGetWeaponSkill); }
static void* varSetGunFlashAlpha = AddressByVersion<void*>(0x5DF400, 0x5DFC20, 0x5FC120);
WRAPPER void CPed::SetGunFlashAlpha(bool bSecondWeapon) { WRAPARG(bSecondWeapon); VARJMP(varSetGunFlashAlpha); }

static void* varSay = AddressByVersion<void*>(0x5EFFE0, 0, 0); // TODO: Do
WRAPPER void CPed::Say(uint16_t phrase, uint32_t param2, float volume, bool param4, bool param5, bool param6) { VARJMP(varSay); }

static void* varGetTaskJetPack = AddressByVersion<void*>(0x601110, 0x601930, 0x620E70);
WRAPPER CTaskSimpleJetPack* CPedIntelligence::GetTaskJetPack() const { VARJMP(varGetTaskJetPack); }

static void* varRenderJetPack = AddressByVersion<void*>(0x67F6A0, 0x67FEC0, 0x6AB110);
WRAPPER void CTaskSimpleJetPack::RenderJetPack(CPed* pPed) { WRAPARG(pPed); VARJMP(varRenderJetPack); }

void (CPed::*CPed::orgGiveWeapon)(uint32_t weapon, uint32_t ammo, bool flag);
void (CPlayerPed::*CPlayerPed::orgDoStuffToGoOnFire)();

RwObject* GetFirstObject(RwFrame* pFrame)
{
	RwObject*	pObject = nullptr;
	RwFrameForAllObjects( pFrame, [&pObject]( RwObject* object ) -> RwObject* {
		pObject = object;
		return nullptr;
	} );
	return pObject;
}

void CPed::ResetGunFlashAlpha()
{
	if ( m_pMuzzleFlashFrame != nullptr )
	{
		if ( RpAtomic* atomic = reinterpret_cast<RpAtomic*>(GetFirstObject(m_pMuzzleFlashFrame)) )
		{
			RpAtomicSetFlags(atomic, 0);
			CVehicle::SetComponentAtomicAlpha(atomic, 0);
		}
	}
}

void CPed::RenderWeapon(bool bMuzzleFlash, bool bForShadow)
{
	if ( m_pWeaponObject )
	{
		RpHAnimHierarchy*	pAnimHierarchy = GetAnimHierarchyFromSkinClump(m_pRwObject);
		bool				bHasParachute = weaponSlots[m_bActiveWeapon].m_eWeaponType == WEAPONTYPE_PARACHUTE;

		RwFrame*			pFrame = RpClumpGetFrame(reinterpret_cast<RpClump*>(m_pWeaponObject));
		*RwFrameGetMatrix(pFrame) = RpHAnimHierarchyGetMatrixArray(pAnimHierarchy)[RpHAnimIDGetIndex(pAnimHierarchy, bHasParachute ? 3 : 24)];

		if ( bHasParachute )
		{
			const RwV3d		vecParachuteTranslation = { 0.1f, -0.15f, 0.0f };
			const RwV3d		vecParachuteRotation = { 0.0f, 1.0f, 0.0f };
			RwMatrixTranslate(RwFrameGetMatrix(pFrame), &vecParachuteTranslation, rwCOMBINEPRECONCAT);
			RwMatrixRotate(RwFrameGetMatrix(pFrame), &vecParachuteRotation, 90.0f, rwCOMBINEPRECONCAT);
		}

		RwFrameUpdateObjects(pFrame);
		if ( bForShadow )
			RpClumpForAllAtomics(reinterpret_cast<RpClump*>(m_pWeaponObject), ShadowCameraRenderCB);
		else if ( !bMuzzleFlash )
			RpClumpRender(reinterpret_cast<RpClump*>(m_pWeaponObject));
		else if ( m_pMuzzleFlashFrame )
		{
			SetGunFlashAlpha(false);
			RpAtomic* atomic = reinterpret_cast<RpAtomic*>(GetFirstObject(m_pMuzzleFlashFrame));
			RpAtomicRender( atomic );
		}

		// Dual weapons
		if ( CWeaponInfo::GetWeaponInfo(weaponSlots[m_bActiveWeapon].m_eWeaponType, GetWeaponSkill())->hexFlags >> 11 & 1 )
		{
			*RwFrameGetMatrix(pFrame) = RpHAnimHierarchyGetMatrixArray(pAnimHierarchy)[RpHAnimIDGetIndex(pAnimHierarchy, 34)];				

			const RwV3d		vecParachuteRotation = { 1.0f, 0.0f, 0.0f };
			const RwV3d		vecParachuteTranslation  = { 0.04f, -0.05f, 0.0f };
			RwMatrixRotate(RwFrameGetMatrix(pFrame), &vecParachuteRotation, 180.0f, rwCOMBINEPRECONCAT);
			RwMatrixTranslate(RwFrameGetMatrix(pFrame), &vecParachuteTranslation, rwCOMBINEPRECONCAT);

			RwFrameUpdateObjects(pFrame);
			if ( bForShadow )
				RpClumpForAllAtomics(reinterpret_cast<RpClump*>(m_pWeaponObject), ShadowCameraRenderCB);
			else if ( !bMuzzleFlash )
				RpClumpRender(reinterpret_cast<RpClump*>(m_pWeaponObject));
			else if ( m_pMuzzleFlashFrame )
			{
				SetGunFlashAlpha(true);
				RpAtomic* atomic = reinterpret_cast<RpAtomic*>(GetFirstObject(m_pMuzzleFlashFrame));
				RpAtomicRender( atomic );
			}
		}
		if ( bMuzzleFlash )
			ResetGunFlashAlpha();
	}
}

void CPed::RenderForShadow()
{
	RpClumpForAllAtomics(reinterpret_cast<RpClump*>(m_pRwObject), ShadowCameraRenderCB);
	RenderWeapon(false, true);

	// Render jetpack
	auto*	pJetPackTask = pPedIntelligence->GetTaskJetPack();
	if ( pJetPackTask )
		pJetPackTask->RenderJetPack(this);
}

void CPed::GiveWeapon_SP(uint32_t weapon, uint32_t ammo, bool flag)
{
 	if ( ammo == 0 ) ammo = 1;
	(this->*(orgGiveWeapon))( weapon, ammo, flag );
}
