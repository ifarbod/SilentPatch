#include "StdAfx.h"

#include "Timer.h"
#include "General.h"
#include "Vehicle.h"
#include "LinkList.h"
#include "ModelInfoSA.h"
#include "AudioHardware.h"
#include "Script.h"

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

// CVisibilityPlugins clump plugin
struct ClumpVisibilityPlugin
{
	BOOL			(*visibilityCallback)(RpClump*);
	unsigned int	alpha;
};

// RW wrappers
// TODO: Multiple EXEs
WRAPPER RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callBack, void* data) { WRAPARG(frame); WRAPARG(callBack); WRAPARG(data); EAXJMP(0x7F1200); }
WRAPPER RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* pData) { WRAPARG(clump); WRAPARG(callback); WRAPARG(pData); EAXJMP(0x749B70); }
WRAPPER RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData) { WRAPARG(geometry); WRAPARG(fpCallBack); WRAPARG(pData); EAXJMP(0x74C790); }
WRAPPER RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic) { WRAPARG(atomic); EAXJMP(0x7491C0); }
WRAPPER RwImage* RtPNGImageRead(const RwChar* imageName) { WRAPARG(imageName); EAXJMP(0x7CF9B0); }
WRAPPER RwTexture* RwTextureCreate(RwRaster* raster) { WRAPARG(raster); EAXJMP(0x7F37C0); }
WRAPPER RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) { WRAPARG(width); WRAPARG(height); WRAPARG(depth); WRAPARG(flags); EAXJMP(0x7FB230); }
WRAPPER RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image) { WRAPARG(raster); WRAPARG(image); EAXJMP(0x804290); }
WRAPPER RwBool RwImageDestroy(RwImage* image) { WRAPARG(image); EAXJMP(0x802740); }
WRAPPER RwImage* RwImageFindRasterFormat(RwImage* ipImage, RwInt32 nRasterType, RwInt32* npWidth, RwInt32* npHeight, RwInt32* npDepth, RwInt32* npFormat) { WRAPARG(ipImage); WRAPARG(nRasterType); WRAPARG(npWidth); WRAPARG(npHeight); WRAPARG(npDepth); WRAPARG(npFormat); EAXJMP(0x8042C0); }
WRAPPER RpMaterial *RpMaterialSetTexture(RpMaterial *material, RwTexture *texture) { EAXJMP(0x74DBC0); }
WRAPPER RwBool RwTextureDestroy(RwTexture* texture) { EAXJMP(0x7F3820); }

WRAPPER bool CanSeeOutSideFromCurrArea() { EAXJMP(0x53C4A0); }

#ifndef SA_STEAM_TEST

WRAPPER void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) { WRAPARG(state); WRAPARG(value); EAXJMP(0x7FC2D0); }
WRAPPER void RenderOneXLUSprite(float x, float y, float z, float width, float height, int r, int g, int b, int a, float w, char, char, char) { EAXJMP(0x70D000); }

#else

WRAPPER void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) { EAXJMP(0x836290); }
WRAPPER void RenderOneXLUSprite(float x, float y, float z, float width, float height, int r, int g, int b, int a, float w, char, char, char) { EAXJMP(0x7592C0); }

#endif

WRAPPER const char* GetFrameNodeName(RwFrame* pFrame) { WRAPARG(pFrame); EAXJMP(0x72FB30); }


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

static unsigned char*					ZonesVisited = *(unsigned char**)0x57216A - 9;	

static const float						fRefZVal = 1.0f;
static const float* const				pRefFal = &fRefZVal;

static RwInt32&							clumpPluginOffset = **(RwInt32**)0x732202;
static bool&							CCutsceneMgr__ms_running = **(bool**)0x53F92D;

#ifndef SA_STEAM_TEST
void**									rwengine = *(void***)0x58FFC0;
#else
void**									rwengine = (void**)0xD22E34;
#endif

// Plugin members access
#define CLUMPVISIBILITYLOCAL(clump, var) \
	(RWPLUGINOFFSET(ClumpVisibilityPlugin, clump, clumpPluginOffset)->var)

static inline void	RenderOrderedList(CLinkList<AlphaObjectInfo>& list)
{ 
	for ( auto i = list.m_lnListTail.m_pPrev; i != &list.m_lnListHead; i = i->m_pPrev )
		i->V().callback(i->V().pAtomic, i->V().fCompareValue);
}

void RenderAlphaAtomics()
{
	int		nPushedAlpha = 140;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);

	RenderOrderedList(m_alphaList);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
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
			auto	pStack = static_cast<std::pair<std::pair<void*,int>*,int>*>(pData);
			if ( pStack->second++ < 32 )
			{
				*(pStack->first)++ = std::make_pair(&pMaterial->color, *reinterpret_cast<int*>(&pMaterial->color));
				pMaterial->color.alpha = 0;
			}
		}
	}
	else if ( RpMaterialGetColor(pMaterial)->alpha == 255 )
	{
		auto	pStack = static_cast<std::pair<std::pair<void*,int>*,int>*>(pData);
		if ( pStack->second++ < 32 )
		{
			*(pStack->first)++ = std::make_pair(&pMaterial->color, *reinterpret_cast<int*>(&pMaterial->color));
			pMaterial->color.alpha = 0;
		}
	}

	return pMaterial;
}

RpAtomic* OnePassAlphaRender(RpAtomic* atomic)
{
	int		nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	auto* pAtomic = AtomicDefaultRenderCallBack(atomic);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));

	return pAtomic;
}

RpAtomic* TwoPassAlphaRender(RpAtomic* atomic)
{
	// For cutscenes, fall back to one-pass render
	if ( CCutsceneMgr__ms_running && !CanSeeOutSideFromCurrArea() )
		return OnePassAlphaRender(atomic);

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
#if defined HIDE_MATERIALS
	std::pair<void*,int>					MatsCache[32];
	std::pair<std::pair<void*,int>*,int>	ParamPair = std::make_pair(MatsCache, 0);

	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), AlphaTestAndPush, &ParamPair);
	AtomicDefaultRenderCallBack(atomic);
	ParamPair.first->first = 0;

	assert(ParamPair.second <= 32);

	for ( auto i = MatsCache; i->first; i++ )
		*static_cast<int*>(i->first) = i->second;
#else
	AtomicDefaultRenderCallBack(atomic);
#endif

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(nZWrite));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));

	return pAtomic;
}

RpAtomic* StaticPropellerRender(RpAtomic* pAtomic)
{
	int		nPushedAlpha;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
	auto* pReturnAtomic = AtomicDefaultRenderCallBack(pAtomic);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	return pReturnAtomic;
}

RpAtomic* RenderBigVehicleActomic(RpAtomic* pAtomic, float fComp)
{
	UNREFERENCED_PARAMETER(fComp);

	const char*		pNodeName = GetFrameNodeName(RpAtomicGetFrame(pAtomic));

	if ( !strncmp(pNodeName, "moving_prop", 11) )
		return TwoPassAlphaRender(pAtomic);

	if ( !strncmp(pNodeName, "static_prop", 11) )
		return StaticPropellerRender(pAtomic);

	return AtomicDefaultRenderCallBack(pAtomic);
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

bool GetCurrentZoneLockedOrUnlocked(float fPosX, float fPosY)
{
	int		Xindex = (fPosX+3000.0f) * (1.0f/600.0f);
	int		Yindex = (fPosY+3000.0f) * (1.0f/600.0f);

	// "Territories fix"
	if ( (Xindex >= 0 && Xindex < 10) && (Yindex >= 0 && Yindex < 10) )
		return ZonesVisited[10*Xindex - Yindex + 9] != 0;
	
	// Outside of map bounds
	return true;
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
	InjectHook(0x508F79, AlteredPrintStringXOnly<0x508F72>);
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
#include "PNGFile.h"

// lunar.png
static const BYTE	gMoonMaskPNG[] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40,
	0x08, 0x06, 0x00, 0x00, 0x00, 0xAA, 0x69, 0x71, 0xDE, 0x00, 0x00, 0x03,
	0xC1, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0xED, 0x9B, 0xD7, 0x6B, 0x54,
	0x51, 0x10, 0xC6, 0x27, 0x96, 0x68, 0xEC, 0xA2, 0x41, 0x09, 0x16, 0xAC,
	0x60, 0x50, 0x51, 0xAC, 0xB1, 0xF7, 0x8A, 0x3E, 0xC4, 0x06, 0xBE, 0x08,
	0x3E, 0xE8, 0x83, 0x82, 0xE8, 0x1F, 0xA2, 0x08, 0xFA, 0xA0, 0x0F, 0x82,
	0x2F, 0x82, 0xF5, 0xC1, 0x10, 0x7B, 0x8F, 0x5D, 0x83, 0xA2, 0xA2, 0x60,
	0xC5, 0x42, 0x50, 0x54, 0xEC, 0xC6, 0x12, 0xCB, 0xF7, 0x31, 0xB3, 0x70,
	0x73, 0xD9, 0x9B, 0xCD, 0x66, 0xDB, 0xBD, 0x39, 0xFB, 0xC1, 0x8F, 0x90,
	0x6C, 0x58, 0x76, 0xBE, 0x33, 0x67, 0x66, 0xCE, 0xDD, 0x7B, 0x0B, 0xC4,
	0x71, 0x15, 0xE4, 0xFA, 0x03, 0xE4, 0x5A, 0xD9, 0x30, 0xA0, 0x15, 0xE8,
	0x06, 0x3A, 0x83, 0x8E, 0xA0, 0x1D, 0x68, 0x0B, 0x0A, 0x41, 0x0B, 0xFB,
	0x9F, 0xBF, 0xE0, 0x17, 0xF8, 0x01, 0xBE, 0x83, 0x2F, 0xE0, 0x13, 0x78,
	0x0F, 0xEA, 0xA2, 0x68, 0x40, 0x11, 0x28, 0x01, 0x3D, 0x41, 0x77, 0xD0,
	0x05, 0x74, 0x00, 0xED, 0xED, 0xB5, 0x42, 0x33, 0xA6, 0xA5, 0xFD, 0xFF,
	0x1F, 0x0B, 0x94, 0x26, 0xD4, 0x82, 0x6F, 0xE0, 0x2B, 0xF8, 0x08, 0xDE,
	0x81, 0xD7, 0xA0, 0xC6, 0x5E, 0x0B, 0xB5, 0x01, 0x3D, 0x40, 0x5F, 0xD0,
	0x0B, 0x14, 0x8B, 0xAE, 0x7C, 0x57, 0xD0, 0xDB, 0xCC, 0x28, 0xF6, 0x98,
	0x51, 0x64, 0x26, 0x88, 0x05, 0x5F, 0xEB, 0x09, 0xFA, 0xAD, 0x05, 0xFD,
	0x12, 0x7C, 0x10, 0xCD, 0x04, 0xFE, 0xED, 0x15, 0x78, 0x0E, 0xDE, 0x84,
	0xCD, 0x00, 0x06, 0x3E, 0xD0, 0x82, 0x2F, 0xB1, 0xDF, 0x87, 0x80, 0x41,
	0xA0, 0x1F, 0x68, 0xDD, 0xC4, 0xF7, 0xFD, 0x0D, 0x9E, 0x81, 0x47, 0xE0,
	0x81, 0x05, 0x5E, 0x63, 0x26, 0x3C, 0x4E, 0x87, 0x11, 0xA9, 0x1A, 0xC0,
	0x55, 0x2C, 0xB5, 0x40, 0xFB, 0x58, 0xB0, 0x23, 0xC1, 0x70, 0x7B, 0x2D,
	0x9D, 0x62, 0x86, 0xDC, 0x01, 0xB7, 0xCC, 0x94, 0x17, 0x66, 0xCC, 0x7D,
	0x49, 0x61, 0x6B, 0xA4, 0x62, 0x00, 0x03, 0x1E, 0x06, 0xFA, 0x1B, 0x65,
	0x60, 0x6C, 0x8A, 0xEF, 0xD9, 0x18, 0xFD, 0x03, 0xD7, 0xC1, 0x15, 0xF0,
	0xD4, 0xB8, 0x6B, 0x86, 0x64, 0xCD, 0x00, 0xAE, 0xF0, 0x50, 0xD1, 0x95,
	0x1F, 0x03, 0x26, 0x83, 0x4E, 0x19, 0x0E, 0xDC, 0xAF, 0xCF, 0xA0, 0x0A,
	0xDC, 0x10, 0xCD, 0x84, 0x7B, 0xA2, 0x19, 0x92, 0x51, 0x03, 0x58, 0xB4,
	0x46, 0x8B, 0xAE, 0xFC, 0x60, 0x30, 0xC7, 0xCC, 0xC8, 0xA5, 0x18, 0xF4,
	0x09, 0xF0, 0x50, 0x34, 0x13, 0x6E, 0x4A, 0x12, 0xAD, 0x33, 0x19, 0x03,
	0xDA, 0x88, 0xA6, 0x38, 0x03, 0x1E, 0x01, 0x16, 0x88, 0x16, 0xBC, 0x30,
	0x88, 0x85, 0xF1, 0x08, 0xB8, 0x6D, 0x86, 0x70, 0x8B, 0xFC, 0x4C, 0xA7,
	0x01, 0x5C, 0xF9, 0x32, 0x0B, 0x9C, 0x26, 0x2C, 0x96, 0xEC, 0xA7, 0x7C,
	0x22, 0x71, 0x4B, 0x1C, 0xB6, 0xE0, 0x69, 0x04, 0x6B, 0x44, 0xC2, 0x4C,
	0x68, 0xAC, 0x01, 0xE3, 0xC1, 0x28, 0xFB, 0xB9, 0x44, 0xD2, 0x5F, 0xE1,
	0xD3, 0x25, 0x76, 0x83, 0x83, 0xE0, 0x2A, 0xA8, 0xB6, 0x9F, 0x29, 0x1B,
	0xC0, 0x94, 0x1F, 0x27, 0xBA, 0xF2, 0xCB, 0x25, 0x7C, 0x2B, 0xEF, 0x17,
	0x33, 0x61, 0x9F, 0x68, 0x26, 0x5C, 0x93, 0x04, 0x85, 0x31, 0x91, 0x01,
	0x6C, 0x75, 0x93, 0x44, 0x7B, 0xFB, 0x4A, 0x09, 0xCF, 0x9E, 0x4F, 0x24,
	0xD6, 0x84, 0x3D, 0xA2, 0x33, 0xC3, 0x45, 0x69, 0xA0, 0x45, 0x36, 0x64,
	0x00, 0xD3, 0x7C, 0x86, 0x68, 0xD5, 0x2F, 0x97, 0xDC, 0x57, 0xFB, 0x64,
	0xC5, 0x95, 0x3F, 0x24, 0xDA, 0x15, 0xCE, 0x48, 0xC0, 0xB0, 0xD4, 0x90,
	0x01, 0xDC, 0xF3, 0x13, 0xC0, 0x5C, 0xB0, 0x30, 0xD7, 0xD1, 0x34, 0x51,
	0x95, 0xE0, 0x38, 0xB8, 0x2C, 0x5A, 0x13, 0x1A, 0x6D, 0x00, 0x67, 0xF9,
	0xE9, 0xA2, 0x7B, 0x7F, 0xB5, 0x84, 0x7F, 0xDF, 0x07, 0x89, 0xF5, 0x60,
	0x97, 0x68, 0x2D, 0x38, 0x2B, 0x71, 0xCE, 0x0E, 0x41, 0x06, 0x4C, 0x34,
	0xCA, 0xCD, 0x84, 0x28, 0x8B, 0xC1, 0x73, 0x2B, 0x5C, 0x32, 0x12, 0x1A,
	0xC0, 0xD5, 0x9F, 0x29, 0x3A, 0xDE, 0xAE, 0x95, 0xE8, 0x5F, 0x35, 0xE2,
	0xD9, 0x61, 0x87, 0xE8, 0xD8, 0x7C, 0x5A, 0x7C, 0x59, 0x10, 0x2F, 0x38,
	0xB6, 0xBB, 0x29, 0x60, 0xA9, 0x44, 0x7F, 0xF5, 0x63, 0x62, 0x16, 0x1C,
	0x00, 0x17, 0x44, 0xDB, 0x63, 0xA0, 0x01, 0xAC, 0xFC, 0xF3, 0x45, 0xA7,
	0xBE, 0x75, 0x12, 0xDE, 0x81, 0x27, 0x59, 0xB1, 0x03, 0x6C, 0x17, 0x9D,
	0x0E, 0x8F, 0x8A, 0xA7, 0x23, 0xF8, 0x0D, 0x18, 0x00, 0x66, 0x81, 0x45,
	0x12, 0xDD, 0xCA, 0x1F, 0x24, 0x76, 0x84, 0x0A, 0x70, 0x0A, 0x3C, 0x09,
	0x32, 0x80, 0x85, 0x6F, 0x2A, 0x58, 0x25, 0x7A, 0xDA, 0x6B, 0x4E, 0xE2,
	0x69, 0x71, 0x37, 0x38, 0x2F, 0x9E, 0x62, 0xE8, 0x35, 0x80, 0x07, 0x1E,
	0xAE, 0x3A, 0x27, 0xBF, 0x0D, 0xD2, 0xF4, 0xCB, 0x58, 0x61, 0x15, 0x2F,
	0xAF, 0x6D, 0x15, 0x9D, 0x0C, 0x99, 0x0D, 0x75, 0x7E, 0x03, 0x58, 0xFD,
	0xE7, 0x89, 0xD6, 0x80, 0x15, 0xB9, 0xFE, 0xB4, 0x19, 0xD2, 0x5E, 0xD1,
	0x1A, 0x70, 0x4C, 0xAC, 0x1B, 0x78, 0x0D, 0x60, 0xCA, 0xCF, 0x06, 0xCB,
	0x44, 0xB7, 0x41, 0x73, 0x14, 0xD3, 0x7F, 0x3F, 0x38, 0x29, 0xBA, 0x25,
	0xEA, 0x19, 0xC0, 0xD1, 0x97, 0xFD, 0x9F, 0xFB, 0xBF, 0x34, 0xD7, 0x9F,
	0x34, 0x43, 0xE2, 0x05, 0x54, 0xD6, 0x01, 0xCE, 0x03, 0xD5, 0x7E, 0x03,
	0x38, 0xF8, 0xF0, 0xF0, 0xB3, 0x46, 0xA2, 0x73, 0xEA, 0x4B, 0x56, 0x3C,
	0x25, 0xEE, 0x14, 0x3D, 0x1C, 0x55, 0xF9, 0x0D, 0x60, 0xFA, 0x4F, 0x03,
	0xEB, 0x25, 0xBA, 0xB3, 0x7F, 0x22, 0xF1, 0x6C, 0xB0, 0x0D, 0x9C, 0x13,
	0xDD, 0x06, 0xF5, 0x0C, 0x60, 0x07, 0xE0, 0x04, 0xB8, 0x51, 0x9A, 0x5F,
	0x07, 0x88, 0x89, 0x9D, 0x60, 0x8B, 0xE8, 0x44, 0x58, 0xE9, 0x37, 0x80,
	0xC3, 0x0F, 0x8B, 0xDF, 0x26, 0x89, 0xFE, 0xFC, 0x1F, 0x24, 0x9E, 0x0B,
	0x36, 0x8B, 0x16, 0xC3, 0x8A, 0xBC, 0x01, 0x92, 0xDF, 0x02, 0xF9, 0x22,
	0x98, 0x6F, 0x83, 0x9E, 0x17, 0x9D, 0x1F, 0x84, 0x9C, 0x1F, 0x85, 0x9D,
	0x3F, 0x0C, 0x39, 0x7F, 0x1C, 0xA6, 0x9C, 0xBE, 0x20, 0x42, 0x39, 0x7F,
	0x49, 0xCC, 0xF9, 0x8B, 0xA2, 0x94, 0xD3, 0x97, 0xC5, 0x29, 0xE7, 0xBF,
	0x18, 0xA1, 0x9C, 0xFE, 0x6A, 0x8C, 0x72, 0xFE, 0xCB, 0x51, 0xCA, 0xE9,
	0xAF, 0xC7, 0x29, 0xE7, 0x6F, 0x90, 0xA0, 0x9C, 0xBE, 0x45, 0x26, 0x26,
	0xA7, 0x6F, 0x92, 0x8A, 0xC9, 0xE9, 0xDB, 0xE4, 0x28, 0xE7, 0x6F, 0x94,
	0xA4, 0x9C, 0xBE, 0x55, 0x36, 0x26, 0xA7, 0x6F, 0x96, 0xF6, 0xCA, 0xD9,
	0xDB, 0xE5, 0xBD, 0x72, 0xFA, 0x81, 0x89, 0x98, 0x9C, 0x7E, 0x64, 0xC6,
	0x2B, 0x67, 0x1F, 0x9A, 0x8A, 0x67, 0x84, 0x93, 0x8F, 0xCD, 0xF9, 0xE5,
	0xEC, 0x83, 0x93, 0xF1, 0xE4, 0xE4, 0xA3, 0xB3, 0x91, 0x91, 0xF3, 0x06,
	0xFC, 0x07, 0x2A, 0x0D, 0x50, 0x50, 0xCC, 0xB6, 0x51, 0x67, 0x00, 0x00,
	0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

// TODO: EXEs
#ifndef SA_STEAM_TEST

unsigned char&				nGameClockDays = **(unsigned char**)0x4E841D;
unsigned char&				nGameClockMonths = **(unsigned char**)0x4E842D;
static float&				fFarClipZ = **(float**)0x70D21F;
static RwTexture** const	gpCoronaTexture = *(RwTexture***)0x6FAA8C;
static int&					MoonSize = **(int**)0x713B0C;

// TODO: Load it from an embedded PNG
static RwTexture*			gpMoonMask = nullptr;

#else

unsigned char&				nGameClockDays = *(unsigned char*)0xBFCC60;
static float&				fFarClipZ = *(float*)0xCCE6F8;
static RwTexture** const	gpCoronaTexture = (RwTexture**)0xCCD768;
static int&					MoonSize = *(int*)0x9499F4;

// TODO: Load it from an embedded PNG
static RwTexture*&			gpMoonMask = *(RwTexture**)0xCC9874;

#endif

// By NTAuthority
void DrawMoonWithPhases(int moonColor, float* screenPos, float sizeX, float sizeY)
{
	if ( !gpMoonMask )
		gpMoonMask = CPNGFile::ReadFromMemory(gMoonMaskPNG, sizeof(gMoonMaskPNG));
	//D3DPERF_BeginEvent(D3DCOLOR_ARGB(0,0,0,0), L"render moon");

	float currentDayFraction = nGameClockDays / 31.0f;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	float a10 = 1.0f / fFarClipZ;
	float size = (MoonSize * 2) + 4.0f;

	RwD3D9SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

	RenderOneXLUSprite(screenPos[0], screenPos[1], fFarClipZ, sizeX * size, sizeY * size, 0, 0, 0, 0, a10, -1, 0, 0);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gpMoonMask));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
	
	float maskX = (sizeX * size) * 5.4f * (currentDayFraction - 0.5f) + screenPos[0];
	float maskY = screenPos[1] + ((sizeY * size) * 0.7f);

	RenderOneXLUSprite(maskX, maskY, fFarClipZ, sizeX * size * 1.7f, sizeY * size * 1.7f, 0, 0, 0, 255, a10, -1, 0, 0);

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
	static const char	aStaticRotor[] = "static_rotor";
	static const char	aStaticRotor2[] = "static_rotor2";
	static const char	aWidescreen[] = "widescreen";
	//static bool			bToPleaseFuckingCargobob;
	_asm
	{
		//setnz	di
		setnz	al
		movzx	di, al
		//mov	bToPleaseFuckingCargobob, al

		push	10
		push	offset aWidescreen
		push	ebp
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jz		HunterTest_RegularAlpha

		push	13
		push	offset aStaticRotor2
		push	ebp
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jz		HunterTest_StaticRotor2AlphaSet

		push	12
		push	offset aStaticRotor
		push	ebp
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jz		HunterTest_StaticRotorAlphaSet

		test	di, di
		//mov		al, bToPleaseFuckingCargobob
		//test	al, al
		jnz		HunterTest_DoorTest

		push	733240h
		mov		eax, 4C7914h
		jmp		eax

HunterTest_DoorTest:
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

HunterTest_StaticRotorAlphaSet:
		push	7340B0h
		mov		eax, 4C7914h
		jmp		eax

HunterTest_StaticRotor2AlphaSet:
		push	734170h
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

void __declspec(naked) PlaneAtomicRendererSetup()
{
	static const char	aStaticProp[] = "static_prop";
	static const char	aMovingProp[] = "moving_prop";
	_asm
	{
		mov     eax, [esi+4]
		push	eax
		call	GetFrameNodeName
		//push	eax
		mov		[esp+8+8], eax
		push	11
		push	offset aStaticProp
		push	eax
		call	strncmp
		add		esp, 10h
		test	eax, eax
		jz		PlaneAtomicRendererSetup_Alpha
		push	11
		push	offset aMovingProp
		push	[esp+12+8]
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jnz		PlaneAtomicRendererSetup_NoAlpha

PlaneAtomicRendererSetup_Alpha:
		push	734370h
		jmp		PlaneAtomicRendererSetup_Return

PlaneAtomicRendererSetup_NoAlpha:
		push	733420h

PlaneAtomicRendererSetup_Return:
		mov		eax, 4C7986h
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

void __declspec(naked) ResetAlphaFuncRefAfterRender()
{
	_asm
	{
		mov		edx, [rwengine]
		mov		edx, [edx]
		mov		ecx, [esp+7Ch-74h]
		push	ecx
		push	rwRENDERSTATEALPHATESTFUNCTIONREF
		call    dword ptr [edx+20h]
		add		esp, 8
		pop		edi
		pop		esi
		add     esp, 74h
		retn
	}
}

static bool		bUseTwoPass;
static HMODULE	hDLLModule;

void SetRendererForAtomic(RpAtomic* pAtomic)
{
	BOOL	bHasAlpha = FALSE;

	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), AlphaTest, &bHasAlpha);
	if ( bHasAlpha )
		RpAtomicSetRenderCallBack(pAtomic, bUseTwoPass ? TwoPassAlphaRender : OnePassAlphaRender);
}

RpAtomic* RenderPedCB(RpAtomic* pAtomic)
{
	BOOL	bHasAlpha = FALSE;

	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), AlphaTest, &bHasAlpha);
	if ( bHasAlpha )
		return TwoPassAlphaRender(pAtomic);
	
	return AtomicDefaultRenderCallBack(pAtomic);
}


//#define DO_MAP_DUMP

#ifdef DO_MAP_DUMP

#include <rtquat.h>
#include <string>
#include <vector>
#include <algorithm>

class CFileObjectInstance
{
public:
	CVector	position;
	RtQuat	rotation;
	int		modelID;
	int		interiorID;
	int		lod;
};

typedef CFileObjectInstance				tContainer;

static std::string				strFileName;
static std::vector<tContainer>	aEntries;

#define COMPARED_AREA		"vegasE"
//#define DUMP_FILENAME		COMPARED_AREA"_pc.dat"

void DumpIPL(const CFileObjectInstance* pInst, const char* pName)
{
	if ( strFileName.find(COMPARED_AREA) != std::string::npos )
		aEntries.push_back(*pInst);
}

void DumpIPLName(const char* pName)
{
	strFileName = pName;

	if ( !aEntries.empty() )
	{
#if defined DUMP_FILENAME
		std::sort(aEntries.begin(), aEntries.end(), [] (const tContainer& left, const tContainer& right) -> bool
		{
			if ( left.modelID < right.modelID )
				return true;

			if ( left.modelID > right.modelID )
				return false;

			if ( left.position.x < right.position.x )
				return true;

			if ( left.position.x > right.position.x )
				return false;

			if ( left.position.y < right.position.y )
				return true;

			if ( left.position.y > right.position.y )
				return false;

			if ( left.position.z < right.position.z )
				return true;

			if ( left.position.z > right.position.z )
				return false;

			if ( left.rotation.imag.x < right.rotation.imag.x )
				return true;

			if ( left.rotation.imag.x > right.rotation.imag.x )
				return false;

			if ( left.rotation.imag.y < right.rotation.imag.y )
				return true;

			if ( left.rotation.imag.y > right.rotation.imag.y )
				return false;

			if ( left.rotation.imag.z < right.rotation.imag.z )
				return true;

			if ( left.rotation.imag.z > right.rotation.imag.z )
				return false;

			if ( left.rotation.real < right.rotation.real )
				return true;

			if ( left.rotation.real > right.rotation.real )
				return false;

			if ( left.interiorID < right.interiorID )
				return true;

			if ( left.interiorID > right.interiorID )
				return false;

			if ( left.lod < right.lod )
				return true;

			return false;
		});

		if ( FILE* hFile = fopen(DUMP_FILENAME, "wb") )
		{
			/*for ( auto it = aEntries.cbegin(); it != aEntries.cend(); it++ )
			{
				fprintf(hFile, "%d, %s, %d, %f, %f, %f, %f, %f, %f, %f, %d\n", it->first.modelID, it->second.c_str(), it->first.interiorID,
					it->first.position.x, it->first.position.y, it->first.position.z,
					it->first.rotation.imag.x, it->first.rotation.imag.y, it->first.rotation.imag.z, it->first.rotation.real,
					it->first.lod);
			}*/
			size_t		size = aEntries.size();
			fwrite(&size, sizeof(size_t), 1, hFile);
			fwrite(aEntries.data(), size * sizeof(tContainer), 1, hFile);

			fclose(hFile);
		}
#else
		// Instead, compare
		 if ( FILE* hPCFile = fopen(COMPARED_AREA"_pc.dat", "rb") )
		 {
			 if ( FILE* hPS2File = fopen(COMPARED_AREA"_ps2.dat", "rb") )
			 {
				 size_t						PCsize, PS2size;
				 std::vector<tContainer>	vecPC, vecPS2;


				 fread(&PCsize, sizeof(size_t), 1, hPCFile);
				 fread(&PS2size, sizeof(size_t), 1, hPS2File);

				 vecPC.resize(PCsize);
				 vecPS2.resize(PS2size);

				 fread(vecPC.data(), PCsize * sizeof(tContainer), 1, hPCFile);
				 fread(vecPS2.data(), PS2size * sizeof(tContainer), 1, hPS2File);

				 // Scan for differences
				 bool		bBreak = true;
				 while ( bBreak )
				 {
					 bool		bBreakThisLoop = false;
					 for ( auto PCit = vecPC.begin(); PCit != vecPC.end(); PCit++ )
					 {
						 for ( auto PS2it = vecPS2.begin(); PS2it != vecPS2.end(); PS2it++ )
						 {
							 if ( PS2it->modelID == PCit->modelID && PS2it->interiorID == PCit->interiorID &&
								 PS2it->position.x == PCit->position.x &&
								 PS2it->position.y == PCit->position.y && PS2it->position.z == PCit->position.z &&
								 PS2it->rotation.imag.x == PCit->rotation.imag.x && PS2it->rotation.imag.y == PCit->rotation.imag.y &&
								 PS2it->rotation.imag.z == PCit->rotation.imag.z && PS2it->rotation.real == PCit->rotation.real )
							 {
								 if ( !((PS2it->lod == -1 && PCit->lod != -1) || (PS2it->lod != -1 && PCit->lod == -1)) )
								 {
									 bBreakThisLoop = true;
									 vecPS2.erase(PS2it);
									 vecPC.erase(PCit);
									 break;
								 }
							 }
						 }
						 if ( bBreakThisLoop )
							 break;
					}
					if ( !bBreakThisLoop )
						break;
					
				 }
				 fclose(hPS2File);

					if ( FILE* hFile = fopen(COMPARED_AREA"_pc.ipl", "w") )
					{
						for ( auto it = vecPC.cbegin(); it != vecPC.cend(); it++ )
						{
							fprintf(hFile, "%d, %d, %f, %f, %f, %f, %f, %f, %f, %d\n", it->modelID, it->interiorID,
								it->position.x, it->position.y, it->position.z, it->rotation.imag.x, it->rotation.imag.y, it->rotation.imag.z,
								it->rotation.real, it->lod);
						}
						fclose(hFile);
					}

					if ( FILE* hFile = fopen(COMPARED_AREA"_ps2.ipl", "w") )
					{
						for ( auto it = vecPS2.cbegin(); it != vecPS2.cend(); it++ )
						{
							fprintf(hFile, "%d, %d, %f, %f, %f, %f, %f, %f, %f, %d\n", it->modelID, it->interiorID,
								it->position.x, it->position.y, it->position.z, it->rotation.imag.x, it->rotation.imag.y, it->rotation.imag.z,
								it->rotation.real, it->lod);
						}
						fclose(hFile);
					}

			 }
			 fclose(hPCFile);
		 }

#endif

		aEntries.clear();
	}

	((void(*)(const char*))0x5B8700)(pName);
}

void __declspec(naked) DumpIPLStub()
{
	_asm
	{
		push	[esp+8]
		push	[esp+4+4]
		call	DumpIPL
		add		esp, 8

		push	0FFFFFFFFh
		push	83C931h
		
		push	538097h
		retn
	}
}

#endif

#include <d3d9.h>

#include "nvc.h"

static IDirect3DVertexShader9*	pNVCShader = nullptr;
static bool						bRenderNVC = false;
static RpAtomic*				pRenderedAtomic;

WRAPPER void _rwD3D9SetVertexShader(void *shader) { EAXJMP(0x7F9FB0); }
WRAPPER RwBool RwD3D9CreateVertexShader(const RwUInt32 *function, void **shader) { EAXJMP(0x7FAC60); }
WRAPPER void RwD3D9DeleteVertexShader(void *shader) { EAXJMP(0x7FAC90); }
WRAPPER void _rwD3D9VSGetComposedTransformMatrix(void *transformMatrix) { EAXJMP(0x7646E0); }
WRAPPER void _rwD3D9VSSetActiveWorldMatrix(const RwMatrix *worldMatrix) { EAXJMP(0x764650); }
WRAPPER RwMatrix* RwFrameGetLTM(RwFrame* frame) { EAXJMP(0x7F0990); }
WRAPPER void _rwD3D9SetVertexShaderConstant(RwUInt32 registerAddress,
                               const void *constantData,
							   RwUInt32  constantCount) { EAXJMP(0x7FACA0); }

WRAPPER RwBool _rpD3D9VertexDeclarationInstColor(RwUInt8 *mem,
                                  const RwRGBA *color,
                                  RwInt32 numVerts,
								  RwUInt32 stride) { EAXJMP(0x754AE0); }

WRAPPER bool IsVisionFXActive() { EAXJMP(0x7034F0); }

bool ShaderAttach()
{
	// CGame::InitialiseRenderWare
	// TODO: EXEs
	if ( ((bool(*)())0x5BD600)() )
	{
		RwD3D9CreateVertexShader(reinterpret_cast<const RwUInt32*>(g_vs20_NVC_vertex_shader), reinterpret_cast<void**>(&pNVCShader));
		return true;
	}
	return false;
}

void ShaderDetach()
{
	if ( pNVCShader )
		RwD3D9DeleteVertexShader(pNVCShader);

	// PluginDetach?
	// TODO: EXEs
	((void(*)())0x53BB80)();
}

void SetShader(RxD3D9InstanceData* pInstData)
{
	if ( bRenderNVC )
	{
		// TODO: Daynight balance var
		D3DMATRIX		outMat;
		float			fEnvVars[2] = { *(float*)0x8D12C0, RpMaterialGetColor(pInstData->material)->alpha * (1.0f/255.0f) };
		RwRGBAReal*		AmbientLight = RpLightGetColor(*(RpLight**)0xC886E8);

		// Normalise the balance
		if ( fEnvVars[0] < 0.0f )
			fEnvVars[0] = 0.0f;
		else if ( fEnvVars[0] > 1.0f )
			fEnvVars[0] = 1.0f;

		RwD3D9SetVertexShader(pNVCShader);

		_rwD3D9VSSetActiveWorldMatrix(RwFrameGetLTM(RpAtomicGetFrame(pRenderedAtomic)));
		_rwD3D9VSGetComposedTransformMatrix(&outMat);
		
		RwD3D9SetVertexShaderConstant(0, &outMat, 4);
		RwD3D9SetVertexShaderConstant(4, fEnvVars, 1);
		RwD3D9SetVertexShaderConstant(5, AmbientLight, 1);
	}
	else
		RwD3D9SetVertexShader(pInstData->vertexShader);
}

void __declspec(naked) SetShader2()
{
	_asm
	{
		mov		bRenderNVC, 1
		push    ecx
		push    edx
		push    edi
		push    ebp
		mov		eax, 5DA6A0h
		call	eax
		add		esp, 10h
		mov		bRenderNVC, 0
		retn
	}
}

void __declspec(naked) HijackAtomic()
{
	_asm
	{
		mov		eax, [esp+8]
		mov		pRenderedAtomic, eax
		mov		eax, 5D6480h
		jmp		eax
	}
}

void __declspec(naked) UsageIndex1()
{
	_asm
	{
		mov		byte ptr [esp+eax*8+27h], 1
		inc		eax

		push	5D611Bh
		retn
	}
}

static void*	pJackedEsi;

void __declspec(naked) HijackEsi()
{
	_asm
	{
		mov     [esp+48h-2Ch], eax
		mov		pJackedEsi, esi
		lea     esi, [ebp+44h]

		mov		eax, 5D6382h
		jmp		eax
	}
}

void __declspec(naked) PassDayColoursToShader()
{
	_asm
	{
		mov		[esp+54h],eax
		jz		PassDayColoursToShader_FindDayColours
		mov		eax, 5D6382h
		jmp		eax

PassDayColoursToShader_FindDayColours:
		xor		eax, eax

PassDayColoursToShader_FindDayColours_Loop:
		cmp     byte ptr [esp+eax*8+48h-28h+6], D3DDECLUSAGE_COLOR
		jnz		PassDayColoursToShader_FindDayColours_Next
		cmp     byte ptr [esp+eax*8+48h-28h+7], 1
		jz		PassDayColoursToShader_DoDayColours

PassDayColoursToShader_FindDayColours_Next:
		inc		eax
		jmp		PassDayColoursToShader_FindDayColours_Loop

PassDayColoursToShader_DoDayColours:
		mov		esi, pJackedEsi
		mov     edx, 8D12BCh
		mov		edx, dword ptr [edx]
		mov     edx, dword ptr [edx+esi+4]
		mov     edi, dword ptr [ebp+18h]
		mov     [esp+48h+4], edx
		mov     edx, dword ptr [ebp+4]
		lea     eax, [esp+eax*8+48h-26h]
		mov     [esp+48h+0Ch], edx
		mov     [esp+48h-2Ch], eax
		lea     esi, [ebp+44h]

PassDayColoursToShader_Iterate:
		mov     edx, dword ptr [esi+14h]
		mov     eax, dword ptr [esi]
		push    edi         
		push    edx            
		mov     edx, dword ptr [esp+50h+4]
		lea     edx, [edx+eax*4]
		imul    eax, edi
		push    edx            
		mov     edx, dword ptr [esp+54h-2Ch]
		movzx   edx, word ptr [edx]
		add     ecx, eax
		add     edx, ecx
		push    edx             
		call    _rpD3D9VertexDeclarationInstColor
		mov     ecx, dword ptr [esp+58h-34h]
		mov     [esi+8], eax
		mov     eax, dword ptr [esp+58h+0Ch]
		add     esp, 10h
		add     esi, 24h
		dec     eax
		mov     [esp+48h+0Ch], eax
		jnz     PassDayColoursToShader_Iterate

		mov		eax, 5D63BDh
		jmp		eax
	}
}

void __declspec(naked) UserTracksFix()
{
	_asm
	{
		push	[esp+4]
		mov		eax, 4D7C60h
		call	eax
		mov		ecx, 0B6B970h
		mov		eax, 4F35B0h
		call	eax
		retn	4
	}
}

static CAEFLACDecoder* __stdcall DecoderCtor(CAEDataStream* pData)
{
	return new CAEFLACDecoder(pData);
}

static CAEWaveDecoder* __stdcall CAEWaveDecoderInit(CAEDataStream* pStream)
{
	return new CAEWaveDecoder(pStream);
}

void __declspec(naked) LoadFLAC()
{
	_asm
	{
		jz		LoadFLAC_WindowsMedia
		sub		ebp, 2
		jnz		LoadFLAC_Return
		//push	SIZE CAEStreamingDecoder
		//call	malloc				// TODO: operator new
		//mov		[esp+20h+4], eax
		//test	eax, eax
		//jz		LoadFLAC_AllocFailed
		push	esi
		//mov		ecx, eax
		//call	CAEFLACDecoder::CAEFLACDecoder
		call	DecoderCtor
		jmp		LoadFLAC_Success

LoadFLAC_WindowsMedia:
		mov		eax, 4F3743h
		jmp		eax

//LoadFLAC_AllocFailed:
		//xor		eax, eax

LoadFLAC_Success:
		test	eax, eax
		mov		[esp+20h+4], eax
		jnz		LoadFLAC_Return_NoDelete

LoadFLAC_Return:
		//push	esi
		mov		ecx, esi
		//call	StreamDtor
		call	CAEDataStream::~CAEDataStream
		push	esi
		call	GTAdelete
		add     esp, 4

LoadFLAC_Return_NoDelete:
		mov     eax, [esp+20h+4]
		mov		ecx, [esp+20h-0Ch]
		pop		esi
		pop		ebp
		pop		edi
		pop		ebx
		mov		fs:0, ecx
		add		esp, 10h
		retn	4
	}
}

static struct
{
	char			Extension[8];
	unsigned int	Codec;
} UserTrackExtensions[] = { { ".ogg", DECODER_VORBIS }, { ".mp3", DECODER_QUICKTIME },
							{ ".wav", DECODER_WAVE }, { ".wma", DECODER_WINDOWSMEDIA },
							{ ".wmv", DECODER_WINDOWSMEDIA }, { ".aac", DECODER_QUICKTIME },
							{ ".m4a", DECODER_QUICKTIME }, { ".mov", DECODER_QUICKTIME },
							{ ".fla", DECODER_FLAC }, { ".flac", DECODER_FLAC } };

void __declspec(naked) FLACInit()
{
	_asm
	{
		mov		al, 1
		mov		[esi+0Dh], al
		pop		esi
		jnz		FLACInit_DontFallBack
		mov		UserTrackExtensions+12.Codec, DECODER_WINDOWSMEDIA

FLACInit_DontFallBack:
		retn
	}
}

void __declspec(naked) LightMaterialsFix()
{
	_asm
	{
		mov     [esi], edi
		mov		ebx, [ecx]
		lea     esi, [edx+4]
		mov		[ebx+4], esi
		mov		edi, [esi]
		mov		[ebx+8], edi
		add		esi, 4
		mov		[ebx+12], esi
		mov		edi, [esi]
		mov		[ebx+16], edi
		add		ebx, 20
		mov		[ecx], ebx
		retn
	}
}

static BOOL				(*IsAlreadyRunning)();
static void				(*TheScriptsLoad)();

static unsigned char*	ScriptSpace = *(unsigned char**)0x5D5380;
static int*				ScriptParams = *(int**)0x48995B;

static CZoneInfo*&		pCurrZoneInfo = **(CZoneInfo***)0x58ADB1;
static CRGBA*			HudColour = *(CRGBA**)0x58ADF6;

static void BasketballFix(unsigned char* pBuf, int nSize)
{
	for ( int i = 0, hits = 0; i < nSize && hits < 7; i++, pBuf++ )
	{
		// Pattern check for save pickup XYZ
		if ( *(unsigned int*)pBuf == 0x449DE19A )		// Save pickup X
		{
			hits++;
			*(float*)pBuf = 1291.8f;
		}
		else if ( *(unsigned int*)pBuf == 0xC4416AE1 )		// Save pickup Y
		{
			hits++;
			*(float*)pBuf = -797.8284f;
		}
		else if ( *(unsigned int*)pBuf == 0x44886C7B )		// Save pickup Z
		{
			hits++;
			*(float*)pBuf = 1089.5f;
		}
		else if ( *(unsigned int*)pBuf == 0x449DF852 )		// Save point X
		{
			hits++;
			*(float*)pBuf = 1286.8f;
		}
		else if ( *(unsigned int*)pBuf == 0xC44225C3 )		// Save point Y
		{
			hits++;
			*(float*)pBuf = -797.69f;
		}
		else if ( *(unsigned int*)pBuf == 0x44885C7B )		// Save point Z
		{
			hits++;
			*(float*)pBuf = 1089.1f;
		}
		else if ( *(unsigned int*)pBuf == 0x43373AE1 )		// Save point A
		{
			hits++;
			*(float*)pBuf = 90.0f;
		}
	}
}

void TheScriptsLoad_BasketballFix()
{
	TheScriptsLoad();

	BasketballFix(ScriptSpace+8, *(int*)(ScriptSpace+3));
}

void StartNewMission_BasketballFix()
{
	if ( ScriptParams[0] == 0 )
		BasketballFix(ScriptSpace+200000, 69000);
}

CRGBA* CRGBA::BlendGangColour(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	*this = Blend(CRGBA(r, g, b), pCurrZoneInfo->ZoneColour.a, HudColour[3], static_cast<BYTE>(255-pCurrZoneInfo->ZoneColour.a));
	this->a = a;

	return this;
}

static const float		fSteamSubtitleSizeX = 0.45f;
static const float		fSteamSubtitleSizeY = 0.9f;
static const float		fSteamRadioNamePosY = 33.0f;
static const float		fSteamRadioNameSizeX = 0.4f;
static const float		fSteamRadioNameSizeY = 0.6f;

BOOL InjectDelayedPatches_10()
{
	if ( !IsAlreadyRunning() )
	{
		using namespace MemoryVP;

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];

		GetModuleFileNameW(hDLLModule, wcModulePath, MAX_PATH);

		wchar_t*		pSlash = wcsrchr(wcModulePath, '\\');
		if ( pSlash )
		{
			*pSlash = '\0';
			PathAppendW(wcModulePath, L"SilentPatchSA.ini");
		}
		else
		{
			// Should never happen - if it does, something's fucking up
			return TRUE;
		}

		bUseTwoPass = GetPrivateProfileIntW(L"SilentPatch", L"TwoPassRendering", FALSE, wcModulePath) != FALSE;
		
		if ( bUseTwoPass )
		{
			// Twopass for peds
			InjectHook(0x733614, RenderPedCB);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableScriptFixes", TRUE, wcModulePath) != FALSE )
		{
			// Gym glitch fix
			Patch<WORD>(0x470B03, 0xCD8B);
			Patch<DWORD>(0x470B0A, 0x8B04508B);
			Patch<WORD>(0x470B0E, 0x9000);
			Nop(0x470B10, 1);
			InjectMethodVP(0x470B05, CRunningScript::GetDay_GymGlitch, PATCH_CALL);

			// Basketball fix
			TheScriptsLoad = (void(*)())(*(int*)0x5D18F1 + 0x5D18F0 + 5);
			InjectHook(0x5D18F0, TheScriptsLoad_BasketballFix);
			InjectHook(0x464BC0, StartNewMission_BasketballFix, PATCH_JUMP);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"NVCShader", TRUE, wcModulePath) != FALSE )
		{
			// Shaders!
			InjectHook(0x5DA743, SetShader);
			InjectHook(0x5D66F1, SetShader2);
			InjectHook(0x5D6116, UsageIndex1, PATCH_JUMP);
			InjectHook(0x5D63B7, PassDayColoursToShader, PATCH_JUMP);
			InjectHook(0x5D637B, HijackEsi, PATCH_JUMP);
			InjectHook(0x5BF3A1, ShaderAttach);
			InjectHook(0x53D910, ShaderDetach);
			Patch<const void*>(0x5D67F4, HijackAtomic);
			Patch<BYTE>(0x5D7200, 0xC3);
			Patch<WORD>(0x5D67BB, 0x6890);
			Patch<WORD>(0x5D67D7, 0x6890);
			Patch<DWORD>(0x5D67BD, 0x5D5FE0);
			Patch<DWORD>(0x5D67D9, 0x5D5FE0);
			Patch<DWORD>(0x5DA73F, 0x90909056);

			Patch<BYTE>(0x5D60D9, D3DDECLTYPE_D3DCOLOR);
			Patch<BYTE>(0x5D60E2, D3DDECLUSAGE_COLOR);
			Patch<BYTE>(0x5D60CF, sizeof(D3DCOLOR));
			Patch<BYTE>(0x5D60EA, sizeof(D3DCOLOR));
			Patch<BYTE>(0x5D60C2, 0x13);
			Patch<BYTE>(0x5D62F0, 0xEB);

			// PostFX fix
			Patch<float>(*(float**)0x7034C0, 0.0);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SkipIntroSplashes", TRUE, wcModulePath) != FALSE )
		{
			// Skip the damn intro splash
			Patch<WORD>(0x748AA8, 0x3DEB);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SmallSteamTexts", TRUE, wcModulePath) != FALSE )
		{
			// We're on 1.0 - make texts smaller
			Patch<const void*>(0x58C387, &fSteamSubtitleSizeY);
			Patch<const void*>(0x58C40F, &fSteamSubtitleSizeY);
			Patch<const void*>(0x58C4CE, &fSteamSubtitleSizeY);

			Patch<const void*>(0x58C39D, &fSteamSubtitleSizeX);
			Patch<const void*>(0x58C425, &fSteamSubtitleSizeX);
			Patch<const void*>(0x58C4E4, &fSteamSubtitleSizeX);

			Patch<const void*>(0x4E9FD8, &fSteamRadioNamePosY);
			Patch<const void*>(0x4E9F22, &fSteamRadioNameSizeY);
			Patch<const void*>(0x4E9F38, &fSteamRadioNameSizeX);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"ColouredZoneNames", FALSE, wcModulePath) != FALSE )
		{
			// Coloured zone names
			Patch<WORD>(0x58ADBE, 0x0E75);
			Patch<WORD>(0x58ADC5, 0x0775);

			InjectMethodVP(0x58ADE4, CRGBA::BlendGangColour, PATCH_NOTHING);
		}
		else
		{
			Patch<BYTE>(0x58ADAE, 0xEB);
		}

		// ImVehFt conflicts
		if ( GetModuleHandle("ImVehFt.asi") == nullptr )
		{
			// Lights
			InjectHook(0x4C830C, LightMaterialsFix, PATCH_CALL);

			// Flying components
			InjectMethodVP(0x59F180, CObject::Render, PATCH_JUMP);

			// Cars getting dirty
			// Only 1.0 and Steam
			InjectMethodVP(0x4C9648, CVehicleModelInfo::FindEditableMaterialList, PATCH_CALL);
			Patch<DWORD>(0x4C964D, 0x0FEBCE8B);
			Patch<DWORD>(0x5D5DC2, 32);		// 1.0 ONLY
		}

		return FALSE;
	}
	return TRUE;
}

__forceinline void Patch_SA_10()
{
	using namespace MemoryVP;

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	IsAlreadyRunning = (BOOL(*)())(*(int*)0x74872E + 0x74872D + 5);
	InjectHook(0x74872D, InjectDelayedPatches_10);

	//Patch<BYTE>(0x5D7265, 0xEB);

	// Temp
	CTimer::m_snTimeInMilliseconds = *(int**)0x4242D1;

	// Heli rotors
	InjectMethodVP(0x6CAB70, CPlane::Render, PATCH_JUMP);
	InjectMethodVP(0x6C4400, CHeli::Render, PATCH_JUMP);
	//InjectHook(0x553318, RenderAlphaAtomics);
	Patch<const void*>(0x7341D9, TwoPassAlphaRender);
	Patch<const void*>(0x734127, TwoPassAlphaRender);
	Patch<const void*>(0x73445E, RenderBigVehicleActomic);
	//Patch<const void*>(0x73406E, TwoPassAlphaRender);

	// Boats
	/*Patch<BYTE>(0x4C79DF, 0x19);
	Patch<DWORD>(0x733A87, EXPAND_BOAT_ALPHA_ATOMIC_LISTS * sizeof(AlphaObjectInfo));
	Patch<DWORD>(0x733AD7, EXPAND_BOAT_ALPHA_ATOMIC_LISTS * sizeof(AlphaObjectInfo));*/

	// Fixed strafing? Hopefully
	/*static const float		fStrafeCheck = 0.1f;
	Patch<const void*>(0x61E0C2, &fStrafeCheck);
	Nop(0x61E0CA, 6);*/

	// RefFix
	Patch<const void*>(0x6FB97A, &pRefFal);
	Patch<BYTE>(0x6FB9A0, 0);

	// Plane rotors
	InjectHook(0x4C7981, PlaneAtomicRendererSetup, PATCH_JUMP);

	// DOUBLE_RWHEELS
	Patch<WORD>(0x4C9290, 0xE281);
	Patch<int>(0x4C9292, ~(rwMATRIXTYPEMASK|rwMATRIXINTERNALIDENTITY));

	// No framedelay
	Patch<DWORD>(0x53E923, 0x42EB56);

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

	// Hunter interior & static_rotor for helis
	InjectHook(0x4C78F2, HunterTest, PATCH_JUMP);
	InjectHook(0x4C9618, CacheCRC32);

	// Fixed blown up car rendering
	// ONLY 1.0
	InjectHook(0x5D993F, DarkVehiclesFix1);
	InjectHook(0x5D9A74, DarkVehiclesFix2, PATCH_JUMP);
	InjectHook(0x5D9B44, DarkVehiclesFix3, PATCH_JUMP);
	InjectHook(0x5D9CB2, DarkVehiclesFix4, PATCH_JUMP);

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

	// Lightbeam fix
	Patch<WORD>(0x6A2E88, 0x0EEB);
	Nop(0x6A2E9C, 3);
	Patch<WORD>(0x6E0F63, 0x0AEB);
	Patch<WORD>(0x6E0F7C, 0x0BEB);
	Patch<WORD>(0x6E0F95, 0x0BEB);
	Patch<WORD>(0x6E0FAF, 0x1AEB);

	Patch<WORD>(0x6E13D5, 0x09EB);
	Patch<WORD>(0x6E13ED, 0x17EB);
	Patch<WORD>(0x6E141F, 0x0AEB);

	Patch<BYTE>(0x6E0FE0, 0x28);
	Patch<BYTE>(0x6E142D, 0x18);
	Patch<BYTE>(0x6E0FDB, 0xC8-0x7C);
	//InjectHook(0x6A2EDA, CullTest);

	InjectHook(0x6A2EF7, ResetAlphaFuncRefAfterRender, PATCH_JUMP);

	// PS2 SUN!!!!!!!!!!!!!!!!!
	static const float		fSunMult = (1050.0f * 0.95f) / 1500.0f;

	Nop(0x6FB17C, 3);
	Patch<const void*>(0x6FC5B0, &fSunMult);
	//Patch<WORD>(0x6FB172, 0x0BEB);
	//Patch<BYTE>(0x6FB1A7, 8);

#if defined EXPAND_ALPHA_ENTITY_LISTS
	// Bigger alpha entity lists
	Patch<DWORD>(0x733B05, EXPAND_ALPHA_ENTITY_LISTS * 20);
	Patch<DWORD>(0x733B55, EXPAND_ALPHA_ENTITY_LISTS * 20);
#endif

	// Unlocked widescreen resolutions
	Patch<DWORD>(0x745B71, 0x9090687D);
	Patch<DWORD>(0x74596C, 0x9090127D);
	Nop(0x745970, 2);
	Nop(0x745B75, 2);
	Nop(0x7459E1, 2);

	// Heap corruption fix
	Nop(0x5C25D3, 5);

	// User Tracks fix
	InjectHook(0x4D9B66, UserTracksFix);
	InjectHook(0x4D9BB5, 0x4F2FD0);
	//Nop(0x4D9BB5, 5);

	// FLAC support
	InjectHook(0x4F373D, LoadFLAC, PATCH_JUMP);
	InjectHook(0x4F35E0, FLACInit, PATCH_JUMP);
	InjectHook(0x4F3787, CAEWaveDecoderInit);

	Patch<WORD>(0x4F376A, 0x18EB);
	//Patch<BYTE>(0x4F378F, sizeof(CAEWaveDecoder));
	Patch<const void*>(0x4F3210, UserTrackExtensions);
	Patch<const void*>(0x4F3241, &UserTrackExtensions->Codec);
	//Patch<const void*>(0x4F35E7, &UserTrackExtensions[1].Codec);
	Patch<BYTE>(0x4F322D, sizeof(UserTrackExtensions));

	// Zones fix
	InjectHook(0x572130, GetCurrentZoneLockedOrUnlocked, PATCH_JUMP);

	// Properly random numberplates
	DWORD*		pVMT = *(DWORD**)0x4C75FC;
	void*		pFunc;
	_asm
	{
		mov		eax, offset CVehicleModelInfo::Shutdown
		mov		[pFunc], eax
	}
	Patch<const void*>(&pVMT[7], pFunc);
	InjectMethodVP(0x4C9660, CVehicleModelInfo::SetCarCustomPlate, PATCH_NOTHING);
	InjectMethodVP(0x6D6A58, CVehicle::CustomCarPlate_TextureCreate, PATCH_NOTHING);
	InjectMethodVP(0x6D651C, CVehicle::CustomCarPlate_BeforeRenderingStart, PATCH_NOTHING);
	InjectMethodVP(0x6D0E53, CVehicle::CustomCarPlate_AfterRenderingStop, PATCH_NOTHING);

	// Fixed police scanner names
	char*			pScannerNames = *(char**)0x4E72D4;
	strncpy(pScannerNames + (8*113), "WESTP", 8);
	strncpy(pScannerNames + (8*134), "????", 8);

	// TEMP - dumping IPL data
#ifdef DO_MAP_DUMP
	InjectHook(0x538090, DumpIPLStub, PATCH_JUMP);
	InjectHook(0x5B92C7, DumpIPLName);
#endif
}

__forceinline void Patch_SA_Steam()
{
	using namespace MemoryVP;

	//InjectHook(0x72F058, HandleMoonStuffStub_Steam, PATCH_JUMP);
}

#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	//UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		hDLLModule = hinstDLL;
#if defined SILENTPATCH_III_VER
		if (*(DWORD*)0x5C1E70 == 0x53E58955) Patch_III_10();
			else if (*(DWORD*)0x5C2130 == 0x53E58955) Patch_III_11();
				else if (*(DWORD*)0x5C6FD0 == 0x53E58955) Patch_III_Steam();
#elif defined SILENTPATCH_VC_VER
		if(*(DWORD*)0x667BF0 == 0x53E58955) Patch_VC_10();
			else if(*(DWORD*)0x667C40 == 0x53E58955) Patch_VC_11();
				else if (*(DWORD*)0x666BA0 == 0x53E58955) Patch_VC_Steam();
#elif defined SILENTPATCH_SA_VER
		if (*(DWORD*)0x82457C == 0x94BF || *(DWORD*)0x8245BC == 0x94BF) Patch_SA_10();
		//else if (*(DWORD*)0x8252FC == 0x94BF || *(DWORD*)0x82533C == 0x94BF) Patch_SA_11();
		else if (*(DWORD*)0x85EC4A == 0x94BF) Patch_SA_Steam();
#endif
	}
	/*else if ( fdwReason == DLL_PROCESS_DETACH )
	{
		if ( pNVCShader )
			RwD3D9DeleteVertexShader(pNVCShader);
	}*/
	return TRUE;
}

WRAPPER void GTAdelete(void* data) { EAXJMP(0x82413F); }