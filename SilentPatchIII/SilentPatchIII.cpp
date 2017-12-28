#include "StdAfx.h"

#include "General.h"
#include "Timer.h"
#include "Patterns.h"
#include "Common.h"
#include "Common_ddraw.h"

#include <memory>

struct PsGlobalType
{
	HWND	window;
	DWORD	instance;
	DWORD	fullscreen;
	DWORD	lastMousePos_X;
	DWORD	lastMousePos_Y;
	DWORD	unk;
	DWORD	diInterface;
	DWORD	diMouse;
	void*	diDevice1;
	void*	diDevice2;
};

struct RsGlobalType
{
	const char*		AppName;
	unsigned int	unkWidth, unkHeight;
	signed int		MaximumWidth;
	signed int		MaximumHeight;
	unsigned int	frameLimit;
	BOOL			quit;
	PsGlobalType*	ps;
	void*			keyboard;
	void*			mouse;
	void*			pad;
};



struct RwV2d
{
    float x;   /**< X value*/
    float y;   /**< Y value */
};


static void (*DrawRect)(const CRect&,const CRGBA&);
static void (*SetScale)(float,float);
static int*				InstantHitsFiredByPlayer;
static const void*		HeadlightsFix_JumpBack;


void (__stdcall *AudioResetTimers)(unsigned int);
static void (*PrintString)(float,float,const wchar_t*);

static bool*			bWantsToDrawHud;
static bool*			bCamCheck;
static RsGlobalType*	RsGlobal;
static const void*		SubtitlesShadowFix_JumpBack;

inline float GetWidthMult()
{
	static const float&		ResolutionWidthMult = **AddressByVersion<float**>(0x57E956, 0x57ECA6, 0x57EBA6);
	return ResolutionWidthMult;
}

inline float GetHeightMult()
{
	static const float&		ResolutionHeightMult = **AddressByVersion<float**>(0x57E940, 0x57EC90, 0x57EB90);
	return ResolutionHeightMult;
}

void ShowRadarTrace(float fX, float fY, unsigned int nScale, BYTE r, BYTE g, BYTE b, BYTE a)
{
	if ( *bWantsToDrawHud == true && !*bCamCheck )
	{
		float	fWidthMult = GetWidthMult();
		float	fHeightMult = GetHeightMult();

		DrawRect(CRect(	fX - ((nScale+1.0f) * fWidthMult * RsGlobal->MaximumWidth),
						fY + ((nScale+1.0f) * fHeightMult * RsGlobal->MaximumHeight),
						fX + ((nScale+1.0f) * fWidthMult * RsGlobal->MaximumWidth),
						fY - ((nScale+1.0f) * fHeightMult * RsGlobal->MaximumHeight)),
				 CRGBA(0, 0, 0, a));

		DrawRect(CRect(	fX - (nScale * fWidthMult * RsGlobal->MaximumWidth),
						fY + (nScale * fHeightMult * RsGlobal->MaximumHeight),
						fX + (nScale * fWidthMult * RsGlobal->MaximumWidth),
						fY - (nScale * fHeightMult * RsGlobal->MaximumHeight)),
				 CRGBA(r, g, b, a));
	}
}

void SetScaleProperly(float fX, float fY)
{
	SetScale(fX * GetWidthMult() * RsGlobal->MaximumWidth, fY * GetHeightMult() * RsGlobal->MaximumHeight);
}

class CGang
{
public:
	int32_t m_vehicleModel;
	int8_t m_gangModelOverride;
	int32_t m_gangWeapons[2];
};

static_assert(sizeof(CGang) == 0x10, "Wrong size: CGang");

static CGang* const Gangs = *hook::get_pattern<CGang*>( "0F BF 4C 24 04 8B 44 24 08 C1 E1 04 89 81", -0x60 + 2 );
void PurpleNinesGlitchFix()
{
	for ( size_t i = 0; i < 9; ++i )
		Gangs[i].m_gangModelOverride = -1;
}

static bool bGameInFocus = true;

static LRESULT (CALLBACK **OldWndProc)(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg )
	{
	case WM_KILLFOCUS:
		bGameInFocus = false;
		break;
	case WM_SETFOCUS:
		bGameInFocus = true;
		break;
	}

	return (*OldWndProc)(hwnd, uMsg, wParam, lParam);
}
static auto* const pCustomWndProc = CustomWndProc;

static void (* const ConstructRenderList)() = AddressByVersion<void(*)()>(0x4A76B0, 0x4A77A0, 0x4A7730);
static void (* const RsMouseSetPos)(RwV2d*) = AddressByVersion<void(*)(RwV2d*)>(0x580D20, 0x581070, 0x580F70);
void ResetMousePos()
{
	if ( bGameInFocus )
	{
		RwV2d	vecPos = { RsGlobal->MaximumWidth * 0.5f, RsGlobal->MaximumHeight * 0.5f };
		RsMouseSetPos(&vecPos);
	}
	ConstructRenderList();
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
	fShadowXSize = nShadow * GetWidthMult() * RsGlobal->MaximumWidth;
	fShadowYSize = nShadow * GetHeightMult() * RsGlobal->MaximumHeight;
}

static void AlteredPrintString_Internal(float fX, float fY, float fMarginX, float fMarginY, const wchar_t* pText)
{
	PrintString(fX - fMarginX + (fMarginX * GetWidthMult() * RsGlobal->MaximumWidth), fY - fMarginY + (fMarginY * GetHeightMult() * RsGlobal->MaximumHeight), pText);
}

template<uintptr_t pFltX, uintptr_t pFltY>
void AlteredPrintString(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginX = **reinterpret_cast<float**>(pFltX);
	const float	fMarginY = **reinterpret_cast<float**>(pFltY);
	AlteredPrintString_Internal(fX, fY, fMarginX, fMarginY, pText);
}

template<uintptr_t pFltX, uintptr_t pFltY>
void AlteredPrintStringMinus(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginX = **reinterpret_cast<float**>(pFltX);
	const float	fMarginY = **reinterpret_cast<float**>(pFltY);
	AlteredPrintString_Internal(fX, fY, -fMarginX, -fMarginY, pText);
}

template<uintptr_t pFltX>
void AlteredPrintStringXOnly(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginX = **reinterpret_cast<float**>(pFltX);
	AlteredPrintString_Internal(fX, fY, fMarginX, 0.0f, pText);
}

template<uintptr_t pFltY>
void AlteredPrintStringYOnly(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginY = **reinterpret_cast<float**>(pFltY);
	AlteredPrintString_Internal(fX, fY, 0.0f, fMarginY, pText);
	
}

float FixedRefValue()
{
	return 1.0f;
}

void __declspec(naked) SubtitlesShadowFix()
{
	_asm
	{
		push	eax
		call	Recalculate
		fadd	[esp+50h+8]
		fadd	[fShadowYSize]
		jmp		SubtitlesShadowFix_JumpBack
	}
}

void __declspec(naked) III_SensResetFix()
{
	_asm
	{
		mov     ecx, 3A76h
		mov     edi, ebp
		fld     dword ptr [ebp+194h]
		fld     dword ptr [ebp+198h]
		rep		stosd
		fstp	dword ptr [ebp+198h]
		fstp	dword ptr [ebp+194h]
		retn
	}
}

static void* RadarBoundsCheckCoordBlip_JumpBack = AddressByVersion<void*>(0x4A55B8, 0x4A56A8, 0x4A5638);
static void* RadarBoundsCheckCoordBlip_Count = AddressByVersion<void*>(0x4A55AF, 0x4A569F, 0x4A562F);
void __declspec(naked) RadarBoundsCheckCoordBlip()
{
	_asm
	{
		mov		edx, dword ptr [RadarBoundsCheckCoordBlip_Count]
		cmp		cl, byte ptr [edx]
		jnb		OutOfBounds
		mov     edx, ecx
		mov     eax, [esp+4]
		jmp		RadarBoundsCheckCoordBlip_JumpBack

OutOfBounds:
		or		eax, -1
		fcompp
		retn
	}
}

static void* RadarBoundsCheckEntityBlip_JumpBack = AddressByVersion<void*>(0x4A565E, 0x4A574E, 0x4A56DE);
void __declspec(naked) RadarBoundsCheckEntityBlip()
{
	_asm
	{
		mov		edx, dword ptr [RadarBoundsCheckCoordBlip_Count]
		cmp		cl, byte ptr [edx]
		jnb		OutOfBounds
		mov     edx, ecx
		mov     eax, [esp+4]
		jmp		RadarBoundsCheckEntityBlip_JumpBack

		OutOfBounds:
		or		eax, -1
		retn
	}
}

extern char** ppUserFilesDir = AddressByVersion<char**>(0x580C16, 0x580F66, 0x580E66);

static LARGE_INTEGER	FrameTime;
int32_t GetTimeSinceLastFrame()
{
	LARGE_INTEGER	curTime;
	QueryPerformanceCounter(&curTime);
	return int32_t(curTime.QuadPart - FrameTime.QuadPart);
}

static int (*RsEventHandler)(int, void*);
int NewFrameRender(int nEvent, void* pParam)
{
	QueryPerformanceCounter(&FrameTime);
	return RsEventHandler(nEvent, pParam);
}

static signed int& LastTimeFireTruckCreated = **(int**)0x41D2E5;
static signed int& LastTimeAmbulanceCreated = **(int**)0x41D2F9;
static void (*orgCarCtrlReInit)();
void CarCtrlReInit_SilentPatch()
{
	orgCarCtrlReInit();
	LastTimeFireTruckCreated = 0;
	LastTimeAmbulanceCreated = 0;
}

static void (*orgPickNextNodeToChaseCar)(void*, float, float, void*);
static float PickNextNodeToChaseCarZ = 0.0f;
static void PickNextNodeToChaseCarXYZ( void* vehicle, const CVector& vec, void* chaseTarget )
{
	PickNextNodeToChaseCarZ = vec.z;
	orgPickNextNodeToChaseCar( vehicle, vec.x, vec.y, chaseTarget );
	PickNextNodeToChaseCarZ = 0.0f;
}


static char		aNoDesktopMode[64];

unsigned int __cdecl AutoPilotTimerCalculation_III(unsigned int nTimer, int nScaleFactor, float fScaleCoef)
{
	return nTimer - static_cast<unsigned int>(nScaleFactor * fScaleCoef);
}

void __declspec(naked) AutoPilotTimerFix_III()
{
	_asm {
		push    dword ptr[esp + 0x4]
		push    dword ptr[ebx + 0x10]
		push    eax
		call    AutoPilotTimerCalculation_III
		add     esp, 0xC
		mov     [ebx + 0xC], eax
		add     esp, 0x28
		pop     ebp
		pop     esi
		pop     ebx
		retn    4
	}
}

void Patch_III_10(const RECT& desktop)
{
	using namespace Memory;

	DrawRect = (void(*)(const CRect&,const CRGBA&))0x51F970;
	SetScale = (void(*)(float,float))0x501B80;
	AudioResetTimers = (void(__stdcall*)(unsigned int))0x57CCD0;
	PrintString = (void(*)(float,float,const wchar_t*))0x500F50;

	InstantHitsFiredByPlayer = *(int**)0x482C8F;
	bWantsToDrawHud = *(bool**)0x4A5877;
	bCamCheck = *(bool**)0x4A588C;
	RsGlobal = *(RsGlobalType**)0x584C42;
	HeadlightsFix_JumpBack = (void*)0x5382F2;
	SubtitlesShadowFix_JumpBack = (void*)0x500D32;

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

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x48E539, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x581C74;
	Patch(0x581C74, &pCustomWndProc);

	// Armour cheat as TORTOISE - like in 1.1 and Steam
	Patch<const char*>(0x4925FB, "ESIOTROT");
	
	// BOOOOORING fixed
	Patch<BYTE>(0x4925D7, 10);

	// 1.1 mouse sensitivity not resetting fix
	Patch<WORD>(0x46BE81, 0x12EB);
	Nop(0x46BAD6, 4);
	InjectHook(0x46BADA, III_SensResetFix, PATCH_CALL);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x582EFD, RsEventHandler );
	InjectHook(0x582EFD, NewFrameRender);
	InjectHook(0x582EA4, GetTimeSinceLastFrame);

	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x48C4FB, orgCarCtrlReInit );
	InjectHook(0x48C4FB, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	// add esp, 38h
	// mov CGarages::RespraysAreFree, 0
	// retn
	bool* pFreeResprays = *(bool**)0x4224A4;
	Patch<BYTE>(0x421E06, 0x38);
	Patch<WORD>(0x421E07, 0x05C6);
	Patch<const void*>(0x421E09, pFreeResprays);
	Patch<BYTE>(0x421E0E, 0xC3);


	// Radar blips bounds check
	InjectHook(0x4A55B2, RadarBoundsCheckCoordBlip, PATCH_JUMP);
	InjectHook(0x4A5658, RadarBoundsCheckEntityBlip, PATCH_JUMP);


	// No-CD fix (from CLEO)
	Patch<DWORD>(0x566A15, 0);
	Nop(0x566A56, 6);
	Nop(0x581C44, 2);
	Nop(0x581C52, 6);
	Patch<const char*>(0x566A3D, "");

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x4139B2, AutoPilotTimerFix_III, PATCH_JUMP);


	// Adblocker
#if DISABLE_FLA_DONATION_WINDOW
	if ( *(DWORD*)0x582749 != 0x006A026A )
	{
		Patch<DWORD>(0x582749, 0x006A026A);
		Patch<WORD>(0x58274D, 0x006A);
	}
#endif

	Common::Patches::DDraw_III_10( desktop, aNoDesktopMode );
}

void Patch_III_11(const RECT& desktop)
{
	using namespace Memory;

	DrawRect = (void(*)(const CRect&,const CRGBA&))0x51FBA0;
	SetScale = (void(*)(float,float))0x501C60;
	AudioResetTimers = (void(__stdcall*)(unsigned int))0x57CCC0;
	PrintString = (void(*)(float,float,const wchar_t*))0x501030;

	InstantHitsFiredByPlayer = *(int**)0x482D5F;
	bWantsToDrawHud = *(bool**)0x4A5967;
	bCamCheck = *(bool**)0x4A597C;
	RsGlobal = *(RsGlobalType**)0x584F82;
	HeadlightsFix_JumpBack = (void*)0x538532;
	SubtitlesShadowFix_JumpBack = (void*)0x500E12;

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

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x48E5F9, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x581FB4;
	Patch(0x581FB4, &pCustomWndProc);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x58323D, RsEventHandler );
	InjectHook(0x58323D, NewFrameRender);
	InjectHook(0x5831E4, GetTimeSinceLastFrame);


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x48C5FB, orgCarCtrlReInit );
	InjectHook(0x48C5FB, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	// add esp, 38h
	// mov CGarages::RespraysAreFree, 0
	// retn
	bool* pFreeResprays = *(bool**)0x4224A4;
	Patch<BYTE>(0x421E06, 0x38);
	Patch<WORD>(0x421E07, 0x05C6);
	Patch<const void*>(0x421E09, pFreeResprays);
	Patch<BYTE>(0x421E0E, 0xC3);


	// Radar blips bounds check
	InjectHook(0x4A56A2, RadarBoundsCheckCoordBlip, PATCH_JUMP);
	InjectHook(0x4A5748, RadarBoundsCheckEntityBlip, PATCH_JUMP);


	// No-CD fix (from CLEO)
	Patch<DWORD>(0x566B55, 0);
	Nop(0x566B96, 6);
	Nop(0x581F84, 2);
	Nop(0x581F92, 6);
	Patch<const char*>(0x566B7D, "");

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x4139B2, AutoPilotTimerFix_III, PATCH_JUMP);

	Common::Patches::DDraw_III_11( desktop, aNoDesktopMode );
}

void Patch_III_Steam(const RECT& desktop)
{
	using namespace Memory;

	DrawRect = (void(*)(const CRect&,const CRGBA&))0x51FB30;
	SetScale = (void(*)(float,float))0x501BF0;
	AudioResetTimers = (void(__stdcall*)(unsigned int))0x57CF20;
	PrintString = (void(*)(float,float,const wchar_t*))0x500FC0;

	InstantHitsFiredByPlayer = *(int**)0x482D5F;
	bWantsToDrawHud = *(bool**)0x4A58F7;
	bCamCheck = *(bool**)0x4A590C;
	RsGlobal = *(RsGlobalType**)0x584E72;
	SubtitlesShadowFix_JumpBack = (void*)0x500DA2;

	Patch<BYTE>(0x490FD3, 1);

	Patch<BYTE>(0x43177D, 16);
	Patch<BYTE>(0x431DBB, 16);
	Patch<BYTE>(0x432083, 16);
	Patch<BYTE>(0x432303, 16);
	Patch<BYTE>(0x479C9A, 16);
	Patch<BYTE>(0x4FADA5, 16);

	Patch<BYTE>(0x544C94, 127);

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

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x48E589, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x581EA4;
	Patch(0x581EA4, &pCustomWndProc);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x58312D, RsEventHandler );
	InjectHook(0x58312D, NewFrameRender);
	InjectHook(0x5830D4, GetTimeSinceLastFrame);

	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x48C58B, orgCarCtrlReInit );
	InjectHook(0x48C58B, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	// add esp, 38h
	// mov CGarages::RespraysAreFree, 0
	// retn
	bool* pFreeResprays = *(bool**)0x4224A4;
	Patch<BYTE>(0x421E06, 0x38);
	Patch<WORD>(0x421E07, 0x05C6);
	Patch<const void*>(0x421E09, pFreeResprays);
	Patch<BYTE>(0x421E0E, 0xC3);


	// Radar blips bounds check
	InjectHook(0x4A5632, RadarBoundsCheckCoordBlip, PATCH_JUMP);
	InjectHook(0x4A56D8, RadarBoundsCheckEntityBlip, PATCH_JUMP);

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x4139B2, AutoPilotTimerFix_III, PATCH_JUMP);

	Common::Patches::DDraw_III_Steam( desktop, aNoDesktopMode );
}

void Patch_III_Common()
{
	using namespace Memory;
	using namespace hook;

	// Purple Nines Glitch fix
	{
		auto addr = get_pattern( "0F BF 4C 24 04 8B 44 24 08 C1 E1 04 89 81", -0xC );
		InjectHook( addr, PurpleNinesGlitchFix, PATCH_JUMP );
	}

	// New timers fix
	{
		auto hookPoint = pattern( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08" ).get_one();
		auto jmpPoint = get_pattern( "DD D8 E9 37 FF FF FF DD D8" );

		InjectHook( hookPoint.get<void>( 0x21 ), CTimer::Update_SilentPatch, PATCH_CALL );
		InjectHook( hookPoint.get<void>( 0x21 + 5 ), jmpPoint, PATCH_JUMP );
	}

	// Alt+F4
	{
		auto addr = pattern( "59 59 31 C0 83 C4 48 5D 5F 5E 5B C2 10 00" ).count(2);
		auto dest = get_pattern( "53 55 56 FF 74 24 68 FF 15" );

		addr.for_each_result( [&]( pattern_match match ) {
			InjectHook( match.get<void>( 2 ), dest, PATCH_JUMP );
		});
	}

	// Proper panels damage
	{
		auto addr = pattern( "C6 43 09 03 C6 43 0A 03 C6 43 0B 03" ).get_one();

		Patch<uint8_t>( addr.get<void>( 0x1A + 1 ), 5 );
		Patch<uint8_t>( addr.get<void>( 0x23 + 1 ), 6 );
		Nop( addr.get<void>( 0x3F ), 7 );
	}

	// Proper metric-imperial conversion constants
	{
		static const float METERS_TO_MILES = 0.0006213711922f;
		static const float METERS_TO_FEET = 3.280839895f;
		auto addr = pattern( "D8 0D ? ? ? ? 6A 00 6A 01 D9 9C 24" ).count(4);
		
		Patch<const void*>( addr.get(0).get<void>( 2 ), &METERS_TO_MILES );
		Patch<const void*>( addr.get(1).get<void>( 2 ), &METERS_TO_MILES );

		Patch<const void*>( addr.get(2).get<void>( 2 ), &METERS_TO_FEET );
		Patch<const void*>( addr.get(3).get<void>( 2 ), &METERS_TO_FEET );
	}

	// Improved pathfinding in PickNextNodeAccordingStrategy - PickNextNodeToChaseCar with XYZ coords
	{
		auto addr = pattern( "E8 ? ? ? ? 50 8D 44 24 10 50 E8" ).get_one();
		ReadCall( addr.get<void>( 0x25 ), orgPickNextNodeToChaseCar );

		const uintptr_t funcAddr = (uintptr_t)get_pattern( "8B AC 24 94 00 00 00 8B 85 2C 01 00 00", -0x7 );

		// push PickNextNodeToChaseCarZ instead of 0.0f
		Patch( funcAddr + 0x1C9, { 0xFF, 0x35 } );
		Patch<const void*>( funcAddr + 0x1C9 + 2, &PickNextNodeToChaseCarZ );
		Nop( funcAddr + 0x1C9 + 6, 1 );

		// lea eax, [esp+1Ch+var_C]
		// push eax
		// nop...
		Patch( addr.get<void>( 0x10 ), { 0x83, 0xC4, 0x04, 0x8D, 0x44, 0x24, 0x10, 0x50, 0xEB, 0x0A } );
		InjectHook( addr.get<void>( 0x25 ), PickNextNodeToChaseCarXYZ );
		Patch<uint8_t>( addr.get<void>( 0x2A + 2 ), 0xC );

		// push ecx
		// nop...
		Patch<uint8_t>( addr.get<void>( 0x3E ), 0x59 );
		Nop( addr.get<void>( 0x3E + 1 ), 6 );
		InjectHook( addr.get<void>( 0x46 ), PickNextNodeToChaseCarXYZ );
		Patch<uint8_t>( addr.get<void>( 0x4B + 2 ), 0xC );


		// For NICK007J
		// Uncomment this to get rid of "treadable hack" in CCarCtrl::PickNextNodeToChaseCar (to mirror VC behaviour)
		InjectHook( funcAddr + 0x2A, funcAddr + 0x182, PATCH_JUMP );
	}


	// No censorships
	{
		auto addr = get_pattern( "8B 15 ? ? ? ? C6 05 ? ? ? ? 00 89 D0" );
		Patch( addr, { 0x83, 0xC4, 0x08, 0xC3 } );	// add     esp, 8 \ retn
	}


	// 014C cargen counter fix (by spaceeinstein)
	{
		auto do_processing = pattern( "0F B7 45 28 83 F8 FF 7D 04 66 FF 4D 28" ).get_one();

		Patch<uint8_t>( do_processing.get<uint8_t*>(1), 0xBF ); // movzx   eax, word ptr [ebx+28h] -> movsx   eax, word ptr [ebx+28h]
		Patch<uint8_t>( do_processing.get<uint8_t*>(7), 0x74 ); // jge -> jz
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		RECT			desktop;
		GetWindowRect(GetDesktopWindow(), &desktop);
		sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

		// This scope is mandatory so Protect goes out of scope before rwcseg gets fixed
		{
			std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

			if (*(DWORD*)0x5C1E75 == 0xB85548EC) Patch_III_10(desktop);
			else if (*(DWORD*)0x5C2135 == 0xB85548EC) Patch_III_11(desktop);
			else if (*(DWORD*)0x5C6FD5 == 0xB85548EC) Patch_III_Steam(desktop);

			Patch_III_Common();
			Common::Patches::III_VC_Common();
			Common::Patches::DDraw_Common();
		}

		Common::Patches::FixRwcseg_Patterns();

		HMODULE		hDummyHandle;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&DllMain, &hDummyHandle);
	}
	return TRUE;
}