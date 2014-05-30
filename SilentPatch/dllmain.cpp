#include "StdAfx.h"

#include "Timer.h"
#include "General.h"
#include "Vehicle.h"
#include "LinkList.h"
#include "ModelInfoSA.h"

struct RsGlobalType
{
	const char*		AppName;
	unsigned int	unkWidth, unkHeight;
	unsigned int	MaximumWidth;
	unsigned int	MaximumHeight;
	unsigned int	frameLimit;
	BOOL			quit;
	void*			ps;
	void*			keyboard;
	void*			mouse;
	void*			pad;
};

struct AlphaObjectInfo
{
	RpAtomic*	pAtomic;
	RpAtomic*	(*callback)(RpAtomic*, float);
	float		fCompareValue;

	friend bool operator < (const AlphaObjectInfo &a, const AlphaObjectInfo &b) 
	{ return a.fCompareValue < b.fCompareValue; }
};

// RW wrappers
// TODO: Multiple EXEs
WRAPPER RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callBack, void* data) { WRAPARG(frame); WRAPARG(callBack); WRAPARG(data); EAXJMP(0x7F1200); }
WRAPPER RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* pData) { WRAPARG(clump); WRAPARG(callback); WRAPARG(pData); EAXJMP(0x749B70); }
WRAPPER RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData) { WRAPARG(geometry); WRAPARG(fpCallBack); WRAPARG(pData); EAXJMP(0x74C790); }
WRAPPER RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic) { WRAPARG(atomic); EAXJMP(0x7491C0); }
WRAPPER void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) { WRAPARG(state); WRAPARG(value); EAXJMP(0x7FC2D0); }

WRAPPER void RenderOneXLUSprite(float x, float y, float z, float width, float height, int r, int g, int b, int a, float w, char, char, char) { EAXJMP(0x70D000); }

#if defined SILENTPATCH_VC_VER

bool*					bSnapShotActive;
static const void*		RosieAudioFix_JumpBack;

#elif defined SILENTPATCH_III_VER

static void (*DrawRect)(const CRect&,const CRGBA&);
static void (*SetScale)(float,float);
static int*				InstantHitsFiredByPlayer;
static signed char*		pGangModelOverrides;
static const void*		HeadlightsFix_JumpBack;

#elif defined SILENTPATCH_SA_VER

bool*					bSnapShotActive;

#endif

void (__stdcall *AudioResetTimers)(unsigned int);
static void (*PrintString)(float,float,const wchar_t*);

static bool*			bWantsToDrawHud;
static bool*			bCamCheck;
static RsGlobalType*	RsGlobal;
static const float*		ResolutionWidthMult;
static const float*		ResolutionHeightMult;
static const void*		SubtitlesShadowFix_JumpBack;

#if defined SILENTPATCH_III_VER

void ShowRadarTrace(float fX, float fY, unsigned int nScale, BYTE r, BYTE g, BYTE b, BYTE a)
{
	if ( *bWantsToDrawHud == true && !*bCamCheck )
	{
		DrawRect(CRect(	fX - ((nScale+1.0f) * *ResolutionWidthMult * RsGlobal->MaximumWidth),
						fY + ((nScale+1.0f) * *ResolutionHeightMult * RsGlobal->MaximumHeight),
						fX + ((nScale+1.0f) * *ResolutionWidthMult * RsGlobal->MaximumWidth),
						fY - ((nScale+1.0f) * *ResolutionHeightMult * RsGlobal->MaximumHeight)),
				 CRGBA(0, 0, 0, a));

		DrawRect(CRect(	fX - (nScale * *ResolutionWidthMult * RsGlobal->MaximumWidth),
						fY + (nScale * *ResolutionHeightMult * RsGlobal->MaximumHeight),
						fX + (nScale * *ResolutionWidthMult * RsGlobal->MaximumWidth),
						fY - (nScale * *ResolutionHeightMult * RsGlobal->MaximumHeight)),
				 CRGBA(r, g, b, a));
	}
}

void SetScaleProperly(float fX, float fY)
{
	SetScale(fX * *ResolutionWidthMult * RsGlobal->MaximumWidth, fY * *ResolutionHeightMult * RsGlobal->MaximumHeight);
}

void PurpleNinesGlitchFix()
{
	for ( int i = 0; i < 9; ++i )
		pGangModelOverrides[i * 16] = -1;
}

void __declspec(naked) M16StatsFix()
{
	_asm
	{
		add		eax, 34h
		add		ebx, 34h
		mov		ecx, [InstantHitsFiredByPlayer]
		inc		[ecx]
		retn
	}
}

void __declspec(naked) HeadlightsFix()
{
	static const float		fMinusOne = -1.0f;
	_asm
	{
		fld		[esp+708h-690h]
		fcomp	fMinusOne
		fnstsw	ax
		and		ah, 5
		cmp		ah, 1
		jnz		HeadlightsFix_DontLimit
		fld		fMinusOne
		fstp	[esp+708h-690h]

HeadlightsFix_DontLimit:
		fld		[esp+708h-690h]
		fabs
		fld		st
		jmp		[HeadlightsFix_JumpBack]
	}
}

static float fShadowXSize, fShadowYSize;

void __stdcall Recalculate(signed int nShadow)
{
	fShadowXSize = nShadow * *ResolutionWidthMult * RsGlobal->MaximumWidth;
	fShadowYSize = nShadow * *ResolutionHeightMult * RsGlobal->MaximumHeight;
}

#elif defined SILENTPATCH_VC_VER

void __declspec(naked) RosiesAudioFix()
{
	_asm
	{
		mov     byte ptr [ebx+0CCh], 0
		mov     byte ptr [ebx+148h], 0
		jmp		[RosieAudioFix_JumpBack]
	}
}

void __stdcall Recalculate(float& fX, float& fY, signed int nShadow)
{
	fX = nShadow * *ResolutionWidthMult * RsGlobal->MaximumWidth;
	fY = nShadow * *ResolutionHeightMult * RsGlobal->MaximumHeight;
}

#elif defined SILENTPATCH_SA_VER

static CLinkList<AlphaObjectInfo>&		m_alphaList = **(CLinkList<AlphaObjectInfo>**)0x733A4D;
static CLinkList<CEntity*>&				ms_weaponPedsForPC = **(CLinkList<CEntity*>**)0x53EACA;


void**									rwengine = *(void***)0x58FFC0;

static inline void	RenderOrderedList(CLinkList<AlphaObjectInfo>& list)
{ 
	for ( auto i = list.m_lnListTail.m_pPrev; i != &list.m_lnListHead; i = i->m_pPrev )
		i->V().callback(i->V().pAtomic, i->V().fCompareValue);
}

void RenderAlphaAtomics()
{
	int		nPushedAlpha;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
	//RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);
	RenderOrderedList(m_alphaList);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	//RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(TRUE));
}

RpAtomic* RenderAtomic(RpAtomic* pAtomic, float fComp)
{
	UNREFERENCED_PARAMETER(fComp);
	return AtomicDefaultRenderCallBack(pAtomic);
}

RpMaterial* AlphaTest(RpMaterial* pMaterial, void* pData)
{
	if ( RpMaterialGetTexture(pMaterial) )
	{
		if ( ((BOOL(*)(RwTexture*))0x4C9EA0)(RpMaterialGetTexture(pMaterial)) )
		{
			*static_cast<BOOL*>(pData) = TRUE;
			return nullptr;
		}
	}
	else if ( RpMaterialGetColor(pMaterial)->alpha < 255 )
	{
		*static_cast<BOOL*>(pData) = TRUE;
		return nullptr;
	}

	return pMaterial;
}

RpMaterial* AlphaTestAndPush(RpMaterial* pMaterial, void* pData)
{
	if ( RpMaterialGetTexture(pMaterial) )
	{
		if ( !((BOOL(*)(RwTexture*))0x4C9EA0)(RpMaterialGetTexture(pMaterial)) )
		{
			auto	pStack = static_cast<std::pair<void*,int>**>(pData);
			*((*pStack)++) = std::make_pair(&pMaterial->color, *reinterpret_cast<int*>(&pMaterial->color));
			pMaterial->color.alpha = 0;
		}
	}
	else if ( RpMaterialGetColor(pMaterial)->alpha == 255 )
	{
		auto	pStack = static_cast<std::pair<void*,int>**>(pData);
		*((*pStack)++) = std::make_pair(&pMaterial->color, *reinterpret_cast<int*>(&pMaterial->color));
		pMaterial->color.alpha = 0;
	}

	return pMaterial;
}

RpAtomic* TwoPassAlphaRender(RpAtomic* atomic)
{
	int		nPushedAlpha, nAlphaFunction;
	int		nZWrite;
	int		nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &nZWrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &nAlphaFunction);

	// 1st pass
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, FALSE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(255));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONEQUAL));

	auto* pAtomic = AtomicDefaultRenderCallBack(atomic);

	// 2nd pass
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONLESS));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

	// Push materials
	std::pair<void*,int>		MatsCache[16];
	auto*						pMats = MatsCache;

	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), AlphaTestAndPush, &pMats);
	AtomicDefaultRenderCallBack(atomic);
	pMats->first = nullptr;

	for ( auto i = MatsCache; i->first; i++ )
		*static_cast<int*>(i->first) = i->second;

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(nZWrite));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));

	return pAtomic;
}

void RenderWeapon(CEntity* pEntity)
{
	int		nPushedAlpha, nAlphaFunction;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &nAlphaFunction);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(255));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONEQUAL));

	((void(*)(CEntity*))0x732F95)(pEntity);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));

	ms_weaponPedsForPC.Insert(pEntity);
}

void RenderWeaponsList()
{
	int		nPushedAlpha, nAlphaFunction;
	int		nZWrite;
	int		nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &nZWrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &nAlphaFunction);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(255));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONLESS));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

	for ( auto i = ms_weaponPedsForPC.m_lnListHead.m_pNext; i != &ms_weaponPedsForPC.m_lnListTail; i = i->m_pNext )
	{
		i->V()->SetupLighting();
		((void(*)(CEntity*))0x732F95)(i->V());
		i->V()->RemoveLighting();
	}

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(nZWrite));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));
}

#endif

template<int pFltX, int pFltY>
void AlteredPrintString(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX - fMarginX + (fMarginX * *ResolutionWidthMult * RsGlobal->MaximumWidth), fY - fMarginY + (fMarginY * *ResolutionHeightMult * RsGlobal->MaximumHeight), pText);
}

template<int pFltX, int pFltY>
void AlteredPrintStringMinus(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX + fMarginX - (fMarginX * *ResolutionWidthMult * RsGlobal->MaximumWidth), fY + fMarginY - (fMarginY * *ResolutionHeightMult * RsGlobal->MaximumHeight), pText);
}

template<int pFltX>
void AlteredPrintStringXOnly(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	PrintString(fX - fMarginX + (fMarginX * *ResolutionWidthMult * RsGlobal->MaximumWidth), fY, pText);
}

template<int pFltY>
void AlteredPrintStringYOnly(float fX, float fY, const wchar_t* pText)
{
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX, fY - fMarginY + (fMarginY * *ResolutionHeightMult * RsGlobal->MaximumHeight), pText);
}

float FixedRefValue()
{
	return 1.0f;
}

void __declspec(naked) SubtitlesShadowFix()
{
	_asm
	{
#if defined SILENTPATCH_III_VER
		push	eax
		call	Recalculate
		fadd	[esp+50h+8]
		fadd	[fShadowYSize]
#elif defined SILENTPATCH_VC_VER
		mov		[esp], eax
		fild	[esp]
		push	eax
		lea		eax, [esp+20h-18h]
		push	eax
		lea		eax, [esp+24h-14h]
		push	eax
		call	Recalculate
#endif
		jmp		SubtitlesShadowFix_JumpBack
	}
}

#if defined SILENTPATCH_III_VER 

__forceinline void Patch_III_10()
{
	using namespace MemoryVP;

	DrawRect = (void(*)(const CRect&,const CRGBA&))0x51F970;
	SetScale = (void(*)(float,float))0x501B80;
	AudioResetTimers = (void(__stdcall*)(unsigned int))0x57CCD0;
	PrintString = (void(*)(float,float,const wchar_t*))0x500F50;

	InstantHitsFiredByPlayer = *(int**)0x482C8F;
	pGangModelOverrides = *(signed char**)0x4C405E;
	bWantsToDrawHud = *(bool**)0x4A5877;
	bCamCheck = *(bool**)0x4A588C;
	RsGlobal = *(RsGlobalType**)0x584C42;
	ResolutionWidthMult = *(float**)0x57E956;
	ResolutionHeightMult = *(float**)0x57E940;
	HeadlightsFix_JumpBack = (void*)0x5382F2;
	SubtitlesShadowFix_JumpBack = (void*)0x500D32;

	CTimer::ms_fTimeScale = *(float**)0x43F73F;
	CTimer::ms_fTimeStep = *(float**)0x41428E;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x404F9B;
	CTimer::m_UserPause = *(bool**)0x4076E9;
	CTimer::m_CodePause = *(bool**)0x4076F2;
	CTimer::m_snTimeInMilliseconds = *(int**)0x40B3B8;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41543D;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4ACEA2;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x47A780;
	CTimer::m_FrameCounter = *(unsigned int**)0x4AD2F3;

	Patch<BYTE>(0x490F83, 1);

	Patch<BYTE>(0x43177D, 16);
	Patch<BYTE>(0x431DBB, 16);
	Patch<BYTE>(0x432083, 16);
	Patch<BYTE>(0x432303, 16);
	Patch<BYTE>(0x479C9A, 16);
	Patch<BYTE>(0x4FAD35, 16);

	Patch<BYTE>(0x544AA4, 127);

	Patch<WORD>(0x5382BF, 0x0EEB);
	InjectHook(0x5382EC, HeadlightsFix, PATCH_JUMP);

	InjectHook(0x4C4004, PurpleNinesGlitchFix, PATCH_JUMP);

	InjectHook(0x4ACE60, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4AD310, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4AD370, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4AD410, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4AD3F0, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4ACF70, CTimer::Update, PATCH_JUMP);
	InjectHook(0x590D9F, CTimer::RecoverFromSave);

	InjectHook(0x4A5870, ShowRadarTrace, PATCH_JUMP);
	InjectHook(0x4209A7, SetScaleProperly);
	InjectHook(0x420A1F, SetScaleProperly);
	InjectHook(0x420AC1, SetScaleProperly);
	InjectHook(0x420D9E, SetScaleProperly);
	InjectHook(0x426342, SetScaleProperly);
	InjectHook(0x4326B8, SetScaleProperly);

	InjectHook(0x4F9E4D, FixedRefValue);

	InjectHook(0x500D27, SubtitlesShadowFix, PATCH_JUMP);
	Patch<WORD>(0x500D4C, 0x05D8);
	Patch<WORD>(0x500D5F, 0x05D9);
	Patch<WORD>(0x500D6E, 0x05D9);
	Patch<void*>(0x500D4E, &fShadowXSize);
	Patch<void*>(0x500D70, &fShadowXSize);
	Patch<void*>(0x500D61, &fShadowYSize);
	Patch<DWORD>(0x500D53, 0x0000441F);
	Patch<BYTE>(0x500D52, 0x0F);
	Patch<BYTE>(0x500D65, 0x90);
	Patch<BYTE>(0x500D6D, 0x50);
	Patch<WORD>(0x500D74, 0x9066);

	Patch<BYTE>(0x5623B5, 0x90);
	InjectHook(0x5623B6, M16StatsFix, PATCH_CALL);

	InjectHook(0x505F82, AlteredPrintString<0x505F7B,0x505F50>);
	InjectHook(0x5065DA, AlteredPrintString<0x5065D3,0x5065A8>);
	InjectHook(0x50669B, AlteredPrintString<0x50668E,0x506670>);
	InjectHook(0x50694B, AlteredPrintString<0x506944,0x506919>);
	InjectHook(0x506A0C, AlteredPrintString<0x5069FF,0x5069E1>);
	InjectHook(0x506C37, AlteredPrintString<0x506C2B,0x506C22>);
	InjectHook(0x5070FA, AlteredPrintString<0x5070F3,0x5070C8>);
	InjectHook(0x507598, AlteredPrintString<0x507591,0x507566>);
	InjectHook(0x507754, AlteredPrintString<0x50774D,0x507722>);
	InjectHook(0x507944, AlteredPrintString<0x50793D,0x507912>);
	InjectHook(0x507AC8, AlteredPrintStringYOnly<0x507A8B>);
	InjectHook(0x507CF0, AlteredPrintString<0x507CE9,0x507CBE>);
	InjectHook(0x507FF1, AlteredPrintStringYOnly<0x507FB4>);
	InjectHook(0x508C6E, AlteredPrintString<0x508C67,0x508C46>);
	InjectHook(0x508F09, AlteredPrintStringXOnly<0x508F02>);
	InjectHook(0x426446, AlteredPrintString<0x42643F,0x426418>);
	InjectHook(0x426584, AlteredPrintString<0x42657D,0x426556>);
	InjectHook(0x42665F, AlteredPrintStringMinus<0x426658,0x426637>);
	InjectHook(0x5098D6, AlteredPrintString<0x509A5E,0x509A3D>);
	InjectHook(0x509A65, AlteredPrintStringMinus<0x509A5E,0x509A3D>);
	InjectHook(0x50A142, AlteredPrintStringXOnly<0x50A139>);
	InjectHook(0x57E9F5, AlteredPrintString<0x57E9EE,0x57E9CD>);
}

__forceinline void Patch_III_11()
{
	using namespace MemoryVP;

	DrawRect = (void(*)(const CRect&,const CRGBA&))0x51FBA0;
	SetScale = (void(*)(float,float))0x501C60;
	AudioResetTimers = (void(__stdcall*)(unsigned int))0x57CCC0;
	PrintString = (void(*)(float,float,const wchar_t*))0x501030;

	InstantHitsFiredByPlayer = *(int**)0x482D5F;
	pGangModelOverrides = *(signed char**)0x4C40FE;
	bWantsToDrawHud = *(bool**)0x4A5967;
	bCamCheck = *(bool**)0x4A597C;
	RsGlobal = *(RsGlobalType**)0x584F82;
	ResolutionWidthMult = *(float**)0x57ECA6;
	ResolutionHeightMult = *(float**)0x57EC90;
	HeadlightsFix_JumpBack = (void*)0x538532;
	SubtitlesShadowFix_JumpBack = (void*)0x500E12;

	CTimer::ms_fTimeScale = *(float**)0x43F73F;
	CTimer::ms_fTimeStep = *(float**)0x41428E;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x404F9B;
	CTimer::m_UserPause = *(bool**)0x4076E9;
	CTimer::m_CodePause = *(bool**)0x4076F2;
	CTimer::m_snTimeInMilliseconds = *(int**)0x40B3B8;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41543D;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4ACF92;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x47A770;
	CTimer::m_FrameCounter = *(unsigned int**)0x4AD3E3;

	Patch<BYTE>(0x491043, 1);

	Patch<BYTE>(0x43177D, 16);
	Patch<BYTE>(0x431DBB, 16);
	Patch<BYTE>(0x432083, 16);
	Patch<BYTE>(0x432303, 16);
	Patch<BYTE>(0x479C9A, 16);
	Patch<BYTE>(0x4FAE15, 16);

	Patch<BYTE>(0x544CE4, 127);

	Patch<WORD>(0x5384FF, 0x0EEB);
	InjectHook(0x53852C, HeadlightsFix, PATCH_JUMP);

	InjectHook(0x4C40A4, PurpleNinesGlitchFix, PATCH_JUMP);

	InjectHook(0x4ACF50, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4AD400, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4AD460, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4AD500, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4AD4E0, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4AD060, CTimer::Update, PATCH_JUMP);
	InjectHook(0x59105F, CTimer::RecoverFromSave);

	InjectHook(0x4A5960, ShowRadarTrace, PATCH_JUMP);
	InjectHook(0x4209A7, SetScaleProperly);
	InjectHook(0x420A1F, SetScaleProperly);
	InjectHook(0x420AC1, SetScaleProperly);
	InjectHook(0x420D9E, SetScaleProperly);
	InjectHook(0x426342, SetScaleProperly);
	InjectHook(0x4326B8, SetScaleProperly);

	InjectHook(0x4F9F2D, FixedRefValue);

	InjectHook(0x500E07, SubtitlesShadowFix, PATCH_JUMP);
	Patch<WORD>(0x500E2C, 0x05D8);
	Patch<WORD>(0x500E3F, 0x05D9);
	Patch<WORD>(0x500E4E, 0x05D9);
	Patch<void*>(0x500E2E, &fShadowXSize);
	Patch<void*>(0x500E50, &fShadowXSize);
	Patch<void*>(0x500E41, &fShadowYSize);
	Patch<DWORD>(0x500E33, 0x0000441F);
	Patch<BYTE>(0x500E32, 0x0F);
	Patch<BYTE>(0x500E45, 0x90);
	Patch<BYTE>(0x500E4D, 0x50);
	Patch<WORD>(0x500E54, 0x9066);

	Patch<BYTE>(0x5624E5, 0x90);
	InjectHook(0x5624E6, M16StatsFix, PATCH_CALL);

	InjectHook(0x506062, AlteredPrintString<0x50605B,0x506030>);
	InjectHook(0x5066BA, AlteredPrintString<0x5066B3,0x506688>);
	InjectHook(0x50677B, AlteredPrintString<0x50676E,0x506750>);
	InjectHook(0x506A2B, AlteredPrintString<0x506A24,0x5069F9>);
	InjectHook(0x506AEC, AlteredPrintString<0x506ADF,0x506AC1>);
	InjectHook(0x506D17, AlteredPrintString<0x506D0B,0x506D02>);
	InjectHook(0x5071DA, AlteredPrintString<0x5071D3,0x5071A8>);
	InjectHook(0x507678, AlteredPrintString<0x507671,0x507646>);
	InjectHook(0x507834, AlteredPrintString<0x50782D,0x507802>);
	InjectHook(0x507A24, AlteredPrintString<0x507A1D,0x5079F2>);
	InjectHook(0x507BA8, AlteredPrintStringYOnly<0x507B6B>);
	InjectHook(0x507DD0, AlteredPrintString<0x507DC9,0x507D9E>);
	InjectHook(0x5080D1, AlteredPrintStringYOnly<0x508094>);
	InjectHook(0x508D4E, AlteredPrintString<0x508D47,0x508D26>);
	InjectHook(0x508FE9, AlteredPrintStringXOnly<0x508FE2>);
	InjectHook(0x426446, AlteredPrintString<0x42643F,0x426418>);
	InjectHook(0x426584, AlteredPrintString<0x42657D,0x426556>);
	InjectHook(0x42665F, AlteredPrintStringMinus<0x426658,0x426637>);
	InjectHook(0x5099B6, AlteredPrintString<0x509B3E,0x509B1D>);
	InjectHook(0x509B45, AlteredPrintStringMinus<0x509B3E,0x509B1D>);
	InjectHook(0x50A222, AlteredPrintStringXOnly<0x50A219>);
	InjectHook(0x57ED45, AlteredPrintString<0x57ED3E,0x57ED1D>);
}

__forceinline void Patch_III_Steam()
{
	using namespace MemoryVP;

	DrawRect = (void(*)(const CRect&,const CRGBA&))0x51FB30;
	SetScale = (void(*)(float,float))0x501BF0;
	AudioResetTimers = (void(__stdcall*)(unsigned int))0x57CF20;
	PrintString = (void(*)(float,float,const wchar_t*))0x500FC0;

	InstantHitsFiredByPlayer = *(int**)0x482D5F;
	pGangModelOverrides = *(signed char**)0x4C408E;
	bWantsToDrawHud = *(bool**)0x4A58F7;
	bCamCheck = *(bool**)0x4A590C;
	RsGlobal = *(RsGlobalType**)0x584E72;
	ResolutionWidthMult = *(float**)0x57EBA6;
	ResolutionHeightMult = *(float**)0x57EB90;
	SubtitlesShadowFix_JumpBack = (void*)0x500DA2;

	CTimer::ms_fTimeScale = *(float**)0x43F73F;
	CTimer::ms_fTimeStep = *(float**)0x41428E;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x404F9B;
	CTimer::m_UserPause = *(bool**)0x4076E9;
	CTimer::m_CodePause = *(bool**)0x4076F2;
	CTimer::m_snTimeInMilliseconds = *(int**)0x40B3B8;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41543D;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4ACF22;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x47A770;
	CTimer::m_FrameCounter = *(unsigned int**)0x4AD373;

	Patch<BYTE>(0x490FD3, 1);

	Patch<BYTE>(0x43177D, 16);
	Patch<BYTE>(0x431DBB, 16);
	Patch<BYTE>(0x432083, 16);
	Patch<BYTE>(0x432303, 16);
	Patch<BYTE>(0x479C9A, 16);
	Patch<BYTE>(0x4FADA5, 16);

	Patch<BYTE>(0x544C94, 127);

	InjectHook(0x4C4034, PurpleNinesGlitchFix, PATCH_JUMP);

	InjectHook(0x4ACEE0, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4AD390, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4AD3F0, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4AD490, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4AD470, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4ACFF0, CTimer::Update, PATCH_JUMP);
	InjectHook(0x590F4F, CTimer::RecoverFromSave);

	InjectHook(0x4A58F0, ShowRadarTrace, PATCH_JUMP);
	InjectHook(0x4209A7, SetScaleProperly);
	InjectHook(0x420A1F, SetScaleProperly);
	InjectHook(0x420AC1, SetScaleProperly);
	InjectHook(0x420D9E, SetScaleProperly);
	InjectHook(0x426342, SetScaleProperly);
	InjectHook(0x4326B8, SetScaleProperly);	

	InjectHook(0x4F9EBD, FixedRefValue);

	InjectHook(0x500D97, SubtitlesShadowFix, PATCH_JUMP);
	Patch<WORD>(0x500DBC, 0x05D8);
	Patch<WORD>(0x500DCF, 0x05D9);
	Patch<WORD>(0x500DDE, 0x05D9);
	Patch<void*>(0x500DBE, &fShadowXSize);
	Patch<void*>(0x500DE0, &fShadowXSize);
	Patch<void*>(0x500DD1, &fShadowYSize);
	Patch<DWORD>(0x500DC3, 0x0000441F);
	Patch<BYTE>(0x500DC2, 0x0F);
	Patch<BYTE>(0x500DD5, 0x90);
	Patch<BYTE>(0x500DDD, 0x50);
	Patch<WORD>(0x500DE4, 0x9066);

	Patch<BYTE>(0x562495, 0x90);
	InjectHook(0x562496, M16StatsFix, PATCH_CALL);

	InjectHook(0x505FF2, AlteredPrintString<0x505FEB,0x505FC0>);
	InjectHook(0x50664A, AlteredPrintString<0x506643,0x506618>);
	InjectHook(0x50670B, AlteredPrintString<0x5066FE,0x5066E0>);
	InjectHook(0x5069BB, AlteredPrintString<0x5069B4,0x506989>);
	InjectHook(0x506A7C, AlteredPrintString<0x506A6F,0x506A51>);
	InjectHook(0x506CA7, AlteredPrintString<0x506C9B,0x506C92>);
	InjectHook(0x50716A, AlteredPrintString<0x507163,0x507138>);
	InjectHook(0x507608, AlteredPrintString<0x507601,0x5075D6>);
	InjectHook(0x5077C4, AlteredPrintString<0x5077BD,0x507792>);
	InjectHook(0x5079B4, AlteredPrintString<0x5079AD,0x507982>);
	InjectHook(0x507B38, AlteredPrintStringYOnly<0x507AFB>);
	InjectHook(0x507D60, AlteredPrintString<0x507D59,0x507D2E>);
	InjectHook(0x508061, AlteredPrintStringYOnly<0x508024>);
	InjectHook(0x508CDE, AlteredPrintString<0x508CD7,0x508CB6>);
	InjectHook(0x508F7B, AlteredPrintStringXOnly<0x508F72>);
	InjectHook(0x426446, AlteredPrintString<0x42643F,0x426418>);
	InjectHook(0x426584, AlteredPrintString<0x42657D,0x426556>);
	InjectHook(0x42665F, AlteredPrintStringMinus<0x426658,0x426637>);
	InjectHook(0x509946, AlteredPrintString<0x509ACE,0x509AAD>);
	InjectHook(0x509AD5, AlteredPrintStringMinus<0x509ACE,0x509AAD>);
	InjectHook(0x50A1B2, AlteredPrintStringXOnly<0x50A1A9>);
	InjectHook(0x57EC45, AlteredPrintString<0x57EC3E,0x57EC1D>);
}

#elif defined SILENTPATCH_VC_VER

__forceinline void Patch_VC_10()
{
	using namespace MemoryVP;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F98D0;
	PrintString = (void(*)(float,float,const wchar_t*))0x551040;

	bSnapShotActive = *(bool**)0x4D1239;
	RsGlobal = *(RsGlobalType**)0x602D32;
	ResolutionWidthMult = *(float**)0x5FA15E;
	ResolutionHeightMult = *(float**)0x5FA148;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551701;

	CTimer::ms_fTimeScale = *(float**)0x453D38;
	CTimer::ms_fTimeStep = *(float**)0x41A318;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x40605B;
	CTimer::m_UserPause = *(bool**)0x4D0F91;
	CTimer::m_CodePause = *(bool**)0x4D0FAE;
	CTimer::m_snTimeInMilliseconds = *(int**)0x418CFC;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41BB3A;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4D1081;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x4D0FE2;
	CTimer::m_FrameCounter = *(unsigned int**)0x4D12CF;

	Patch<BYTE>(0x43E983, 16);
	Patch<BYTE>(0x43EC03, 16);
	Patch<BYTE>(0x43EECB, 16);
	Patch<BYTE>(0x43F52B, 16);
	Patch<BYTE>(0x43F842, 16);
	Patch<BYTE>(0x48EB27, 16);
	Patch<BYTE>(0x541E7E, 16);

	InjectHook(0x4D1300, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4D0ED0, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4D0E50, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4D0DF0, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4D0E30, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4D0F30, CTimer::Update, PATCH_JUMP);
	InjectHook(0x61AA7D, CTimer::RecoverFromSave);

	InjectHook(0x5433BD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x5516FC, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5517C4, 0xD9);
	Patch<BYTE>(0x5517DF, 0xD9);
	Patch<BYTE>(0x551832, 0xD9);
	Patch<BYTE>(0x551848, 0xD9);
	Patch<BYTE>(0x5517E2, 0x34-0x14);
	Patch<BYTE>(0x55184B, 0x34-0x14);
	Patch<BYTE>(0x5517C7, 0x28-0x18);
	Patch<BYTE>(0x551835, 0x24-0x18);
	Patch<BYTE>(0x5516FB, 0x90);

	InjectHook(0x5FA1FD, AlteredPrintString<0x5FA1F6,0x5FA1D5>);
	InjectHook(0x54474D, AlteredPrintStringMinus<0x544727,0x544727>);
}

__forceinline void Patch_VC_11()
{
	using namespace MemoryVP;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F98F0;
	PrintString = (void(*)(float,float,const wchar_t*))0x551060;

	bSnapShotActive = *(bool**)0x4D1259;
	RsGlobal = *(RsGlobalType**)0x602D12;
	ResolutionWidthMult = *(float**)0x5FA17E;
	ResolutionHeightMult = *(float**)0x5FA168;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551721;

	CTimer::ms_fTimeScale = *(float**)0x453D38;
	CTimer::ms_fTimeStep = *(float**)0x41A318;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x40605B;
	CTimer::m_UserPause = *(bool**)0x4D0FB1;
	CTimer::m_CodePause = *(bool**)0x4D0FCE;
	CTimer::m_snTimeInMilliseconds = *(int**)0x418CFC;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41BB3A;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4D10A1;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x4D1002;
	CTimer::m_FrameCounter = *(unsigned int**)0x4D12EF;

	Patch<BYTE>(0x43E983, 16);
	Patch<BYTE>(0x43EC03, 16);
	Patch<BYTE>(0x43EECB, 16);
	Patch<BYTE>(0x43F52B, 16);
	Patch<BYTE>(0x43F842, 16);
	Patch<BYTE>(0x48EB37, 16);
	Patch<BYTE>(0x541E9E, 16);

	InjectHook(0x4D1320, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4D0EF0, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4D0E70, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4D0E10, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4D0E50, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4D0F50, CTimer::Update, PATCH_JUMP);
	InjectHook(0x61AA5D, CTimer::RecoverFromSave);

	InjectHook(0x5433DD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x55171C, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5517E4, 0xD9);
	Patch<BYTE>(0x5517FF, 0xD9);
	Patch<BYTE>(0x551852, 0xD9);
	Patch<BYTE>(0x551868, 0xD9);
	Patch<BYTE>(0x551802, 0x34-0x14);
	Patch<BYTE>(0x55186B, 0x34-0x14);
	Patch<BYTE>(0x5517E7, 0x28-0x18);
	Patch<BYTE>(0x551855, 0x24-0x18);
	Patch<BYTE>(0x55171B, 0x90);

	InjectHook(0x5FA21D, AlteredPrintString<0x5FA216,0x5FA1F5>);
	InjectHook(0x54476D, AlteredPrintStringMinus<0x544747,0x544747>);
}

__forceinline void Patch_VC_Steam()
{
	using namespace MemoryVP;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F9530;
	PrintString = (void(*)(float,float,const wchar_t*))0x550F30;

	bSnapShotActive = *(bool**)0x4D10F9;
	RsGlobal = *(RsGlobalType**)0x602952;
	ResolutionWidthMult = *(float**)0x5F9DBE;
	ResolutionHeightMult = *(float**)0x5F9DA8;
	RosieAudioFix_JumpBack = (void*)0x42BFCE;
	SubtitlesShadowFix_JumpBack = (void*)0x5515F1;

	CTimer::ms_fTimeScale = *(float**)0x453C18;
	CTimer::ms_fTimeStep = *(float**)0x41A318;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x40605B;
	CTimer::m_UserPause = *(bool**)0x4D0E51;
	CTimer::m_CodePause = *(bool**)0x4D0E6E;
	CTimer::m_snTimeInMilliseconds = *(int**)0x418CFC;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41BB3A;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4D0F41;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x4D0EA2;
	CTimer::m_FrameCounter = *(unsigned int**)0x4D118F;

	Patch<BYTE>(0x43E8F3, 16);
	Patch<BYTE>(0x43EB73, 16);
	Patch<BYTE>(0x43EE3B, 16);
	Patch<BYTE>(0x43F49B, 16);
	Patch<BYTE>(0x43F7B2, 16);
	Patch<BYTE>(0x48EA37, 16);
	Patch<BYTE>(0x541D6E, 16);

	InjectHook(0x4D11C0, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4D0D90, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4D0D10, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4D0CB0, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4D0CF0, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4D0DF0, CTimer::Update, PATCH_JUMP);
	InjectHook(0x61A6A6, CTimer::RecoverFromSave);

	InjectHook(0x5432AD, FixedRefValue);

	InjectHook(0x42BFC7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x5515EC, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5516B4, 0xD9);
	Patch<BYTE>(0x5516CF, 0xD9);
	Patch<BYTE>(0x551722, 0xD9);
	Patch<BYTE>(0x551738, 0xD9);
	Patch<BYTE>(0x5516D2, 0x34-0x14);
	Patch<BYTE>(0x55173B, 0x34-0x14);
	Patch<BYTE>(0x5516B7, 0x28-0x18);
	Patch<BYTE>(0x551725, 0x24-0x18);
	Patch<BYTE>(0x5515EB, 0x90);

	InjectHook(0x5F9E5D, AlteredPrintString<0x5F9E56,0x5F9E35>);
	InjectHook(0x54463D, AlteredPrintStringMinus<0x544617,0x544617>);
}

#elif defined SILENTPATCH_SA_VER

void RenderVehicleHiDetailAlphaCB_HunterDoor(RpAtomic* pAtomic)
{
	AlphaObjectInfo		NewObject;

	NewObject.callback = RenderAtomic;
	NewObject.fCompareValue = -std::numeric_limits<float>::infinity();
	NewObject.pAtomic = pAtomic;

	m_alphaList.InsertSorted(NewObject);
}

#include <d3d9.h>

// TODO: EXEs
static unsigned char&		nGameClockDays = **(unsigned char**)0x4E841D;
static float&				fFarClipZ = **(float**)0x70D21F;
static RwTexture** const	gpCoronaTexture = *(RwTexture***)0x6FAA8C;
static int&					MoonSize = **(int**)0x713B0C;

// TODO: Load it from an embedded PNG
static RwTexture*&			gpMoonMask = *(RwTexture**)0xC6AA74;

// By NTAuthority
void DrawMoonWithPhases(int moonColor, float* screenPos, float sizeX, float sizeY)
{
	//D3DPERF_BeginEvent(D3DCOLOR_ARGB(0,0,0,0), L"render moon");

	float currentDayFraction = nGameClockDays / 31.0f;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	float a10 = 1.0 / fFarClipZ;
	float size = (MoonSize * 2) + 4.0f;

	RwD3D9SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

	RenderOneXLUSprite(screenPos[0], screenPos[1], fFarClipZ, sizeX * size, sizeY * size, 0, 0, 0, 0, a10, -1, 0, 0);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gpMoonMask));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
	
	float maskX = (sizeX * size) * 5.40 * (currentDayFraction - 0.5) + screenPos[0];
	float maskY = screenPos[1] + ((sizeY * size) * 0.7);

	RenderOneXLUSprite(maskX, maskY, fFarClipZ, sizeX * size * 1.7, sizeY * size * 1.7, 0, 0, 0, 255, a10, -1, 0, 0);

	RwD3D9SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gpCoronaTexture[2]));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDDESTALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);

	RenderOneXLUSprite(screenPos[0], screenPos[1], fFarClipZ, sizeX * size, sizeY * size, moonColor, moonColor, moonColor * 0.85f, 255, a10, -1, 0, 0);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	//D3DPERF_EndEvent();
}

void __declspec(naked) HandleMoonStuffStub()
{
	__asm
	{
		mov eax, [esp + 78h - 64h] // screen x size
		mov ecx, [esp + 78h - 68h] // screen y size

		push ecx
		push eax

		lea ecx, [esp + 80h - 54h] // screen coord vector

		push ecx

		push esi

		call DrawMoonWithPhases

		add esp, 10h

		push 713D24h		// TODO: EXEs
		retn
	}
}

void __declspec(naked) HandleMoonStuffStub_Steam()
{
	__asm
	{
		mov eax, [esp + 70h - 58h] // screen x size
		mov ecx, [esp + 70h - 5Ch] // screen y size

		push ecx
		push eax

		lea ecx, [esp + 78h - 48h] // screen coord vector

		push ecx

		push esi

		call DrawMoonWithPhases

		add esp, 10h

		push 72F17Fh
		retn
	}
}

static unsigned int		nCachedCRC;

void __declspec(naked) HunterTest()
{
	static const char	aDoorDummy[] = "door_lf_ok";
	_asm
	{
		cmp		nCachedCRC, 0x45D0B41C
		jnz		HunterTest_RegularAlpha
		push	10
		push	offset aDoorDummy
		push	ebp
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jnz		HunterTest_RegularAlpha
		push	RenderVehicleHiDetailAlphaCB_HunterDoor
		mov		eax, 4C7914h
		jmp		eax

HunterTest_RegularAlpha:
		push	733F80h
		mov		eax, 4C7914h
		jmp		eax
	}
}
 
void __declspec(naked) CacheCRC32()
{
	_asm
	{
		mov		eax, [ecx+4]
		mov		nCachedCRC, eax
		mov		eax, 4C7B10h
		jmp		eax
	}
}

// 1.0 only
static bool			bDarkVehicleThing;
static RpLight*&	pDirect = **(RpLight***)0x5BA573;

void __declspec(naked) DarkVehiclesFix1()
{
	_asm
	{
		shr     eax, 0Eh
		test	al, 1
		jz		DarkVehiclesFix1_DontAppply
		mov		ecx, [pDirect]
		mov		ecx, [ecx]
		mov		al, [ecx+2]
		test	al, 1
		jnz		DarkVehiclesFix1_DontAppply
		mov		bDarkVehicleThing, 1
		jmp		DarkVehiclesFix1_Return

DarkVehiclesFix1_DontAppply:
		mov		bDarkVehicleThing, 0

DarkVehiclesFix1_Return:
		mov		eax, 756D90h
		jmp		eax
	}
}

void __declspec(naked) DarkVehiclesFix2()
{
	_asm
	{
		jz		DarkVehiclesFix2_MakeItDark
		mov		al, bDarkVehicleThing
		test	al, al
		jnz		DarkVehiclesFix2_MakeItDark
		mov		eax, 5D9A7Ah
		jmp		eax

DarkVehiclesFix2_MakeItDark:
		mov		eax, 5D9B09h
		jmp		eax
	}
}

void __declspec(naked) DarkVehiclesFix3()
{
	_asm
	{
		jz		DarkVehiclesFix3_MakeItDark
		mov		al, bDarkVehicleThing
		test	al, al
		jnz		DarkVehiclesFix3_MakeItDark
		mov		eax, 5D9B4Ah
		jmp		eax

DarkVehiclesFix3_MakeItDark:
		mov		eax, 5D9CACh
		jmp		eax
	}
}

void __declspec(naked) DarkVehiclesFix4()
{
	_asm
	{
		jz		DarkVehiclesFix4_MakeItDark
		mov		al, bDarkVehicleThing
		test	al, al
		jnz		DarkVehiclesFix4_MakeItDark
		mov		eax, 5D9CB8h
		jmp		eax

DarkVehiclesFix4_MakeItDark:
		mov		eax, 5D9E0Dh
		jmp		eax
	}
}

void SetRendererForAtomic(RpAtomic* pAtomic)
{
	BOOL	bHasAlpha = FALSE;

	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), AlphaTest, &bHasAlpha);
	if ( bHasAlpha )
		RpAtomicSetRenderCallBack(pAtomic, TwoPassAlphaRender);
}

__forceinline void Patch_SA_10()
{
	using namespace MemoryVP;

	// Temp
	CTimer::m_snTimeInMilliseconds = *(int**)0x4242D1;

	// Heli rotors
	InjectMethodVP(0x6CAB70, CPlane::Render, PATCH_JUMP);
	InjectMethodVP(0x6C4400, CHeli::Render, PATCH_JUMP);
	//InjectHook(0x553318, RenderAlphaAtomics);
	Patch<const void*>(0x7341D9, TwoPassAlphaRender);
	Patch<const void*>(0x734127, TwoPassAlphaRender);
	//Patch<const void*>(0x73406E, TwoPassAlphaRender);

	// DOUBLE_RWHEELS
	Patch<WORD>(0x4C9290, 0xE281);
	Patch<int>(0x4C9292, ~(rwMATRIXTYPEMASK|rwMATRIXINTERNALIDENTITY));

	// No framedelay
	Patch<DWORD>(0x53E923, 0x42EB56);

	// Coloured zone names
	Patch<WORD>(0x58ADBE, 0x0E75);
	Patch<WORD>(0x58ADC5, 0x0775);

	// Disable re-initialization of DirectInput mouse device by the game
	Patch<BYTE>(0x576CCC, 0xEB);
	Patch<BYTE>(0x576EBA, 0xEB);
	Patch<BYTE>(0x576F8A, 0xEB);

	// Make sure DirectInput mouse device is set non-exclusive (may not be needed?)
	Patch<DWORD>(0x7469A0, 0x909000B0);

	// Weapons rendering
	InjectHook(0x5E7859, RenderWeapon);
	InjectHook(0x732F30, RenderWeaponsList, PATCH_JUMP);
	//Patch<WORD>(0x53EAC4, 0x0DEB);
	//Patch<WORD>(0x705322, 0x0DEB);
	//Patch<WORD>(0x7271E3, 0x0DEB);
	//Patch<BYTE>(0x73314E, 0xC3);
	Patch<DWORD>(0x732F95, 0x560CEC83);
	Patch<DWORD>(0x732FA2, 0x20245C8B);
	Patch<WORD>(0x733128, 0x20EB);
	Patch<WORD>(0x733135, 0x13EB);
	Nop(0x732FBC, 5);
	//Nop(0x732F93, 6);
	//Nop(0x733144, 6);
	Nop(0x732FA6, 6);
	//Nop(0x5E46DA, 2);

	// Hunter interior
	InjectHook(0x4C7908, HunterTest, PATCH_JUMP);
	InjectHook(0x4C9618, CacheCRC32);

	// Fixed blown up car rendering
	// ONLY 1.0
	InjectHook(0x5D993F, DarkVehiclesFix1);
	InjectHook(0x5D9A74, DarkVehiclesFix2, PATCH_JUMP);
	InjectHook(0x5D9B44, DarkVehiclesFix3, PATCH_JUMP);
	InjectHook(0x5D9CB2, DarkVehiclesFix4, PATCH_JUMP);

	// Cars getting dirty
	// Only 1.0 and Steam
	InjectMethod(0x4C9648, CVehicleModelInfo::FindEditableMaterialList, PATCH_CALL);
	Patch<DWORD>(0x4C964D, 0x0FEBCE8B);
	Patch<DWORD>(0x5D5DC2, 32);		// 1.0 ONLY

	// Bindable NUM5
	// Only 1.0 and Steam
	Nop(0x57DC55, 2);

	// Moonphases
	InjectHook(0x713ACB, HandleMoonStuffStub, PATCH_JUMP);

	// TEMP
	//Patch<DWORD>(0x733B05, 40);
	//Patch<DWORD>(0x733B55, 40);
	//Patch<BYTE>(0x5B3ADD, 4);

	// Twopass rendering (experimental)
	/*Patch<WORD>(0x4C441E, 0x47C7);
	Patch<BYTE>(0x4C4420, 0x48);
	Patch<const void*>(0x4C4421, TwoPassAlphaRender);
	Patch<DWORD>(0x4C4425, 0x04C25E5F);
	Patch<BYTE>(0x4C4429, 0x00);*/
	Patch<BYTE>(0x4C441E, 0x57);
	InjectHook(0x4C441F, SetRendererForAtomic, PATCH_CALL);
	Patch<DWORD>(0x4C4424, 0x5F04C483);
	Patch<DWORD>(0x4C4428, 0x0004C25E);
}

#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
#if defined SILENTPATCH_III_VER
		if (*(DWORD*)0x5C1E70 == 0x53E58955) Patch_III_10();
			else if (*(DWORD*)0x5C2130 == 0x53E58955) Patch_III_11();
				else if (*(DWORD*)0x5C6FD0 == 0x53E58955) Patch_III_Steam();
#elif defined SILENTPATCH_VC_VER
		if(*(DWORD*)0x667BF0 == 0x53E58955) Patch_VC_10();
			else if(*(DWORD*)0x667C40 == 0x53E58955) Patch_VC_11();
				else if (*(DWORD*)0x666BA0 == 0x53E58955) Patch_VC_Steam();
#elif defined SILENTPATCH_SA_VER
		Patch_SA_10();
#endif
	}
	return TRUE;
}