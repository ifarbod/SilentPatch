#include "StdAfx.h"

#include "General.h"
#include "Timer.h"

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
static signed char*		pGangModelOverrides;
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

void PurpleNinesGlitchFix()
{
	for ( int i = 0; i < 9; ++i )
		pGangModelOverrides[i * 16] = -1;
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
static auto* pCustomWndProc = CustomWndProc;

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

template<int pFltX, int pFltY>
void AlteredPrintString(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX - fMarginX + (fMarginX * GetWidthMult() * RsGlobal->MaximumWidth), fY - fMarginY + (fMarginY * GetHeightMult() * RsGlobal->MaximumHeight), pText);
}

template<int pFltX, int pFltY>
void AlteredPrintStringMinus(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX + fMarginX - (fMarginX * GetWidthMult() * RsGlobal->MaximumWidth), fY + fMarginY - (fMarginY * GetHeightMult() * RsGlobal->MaximumHeight), pText);
}

template<int pFltX>
void AlteredPrintStringXOnly(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	PrintString(fX - fMarginX + (fMarginX * GetWidthMult() * RsGlobal->MaximumWidth), fY, pText);
}

template<int pFltY>
void AlteredPrintStringYOnly(float fX, float fY, const wchar_t* pText)
{
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX, fY - fMarginY + (fMarginY * GetHeightMult() * RsGlobal->MaximumHeight), pText);
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

char** const ppUserFilesDir = AddressByVersion<char**>(0x580C16, 0x580F66, 0x580E66);

char* GetMyDocumentsPath()
{
	static char	cUserFilesPath[MAX_PATH];

	if ( cUserFilesPath[0] == '\0' )
	{	
		SHGetFolderPath(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, cUserFilesPath);
		PathAppend(cUserFilesPath, *ppUserFilesDir);
		CreateDirectory(cUserFilesPath, nullptr);
	}
	return cUserFilesPath;
}

static LARGE_INTEGER	FrameTime;
DWORD GetTimeSinceLastFrame()
{
	LARGE_INTEGER	curTime;
	QueryPerformanceCounter(&curTime);
	return curTime.QuadPart - FrameTime.QuadPart;
}

static void (*RsEventHandler)(int, void*);
void NewFrameRender(int nEvent, void* pParam)
{
	QueryPerformanceCounter(&FrameTime);
	RsEventHandler(nEvent, pParam);
}

static char		aNoDesktopMode[64];

void Patch_III_10(const RECT& desktop)
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

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x48E539, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x581C74;
	Patch(0x581C74, &pCustomWndProc);

	// Armour cheat as TORTOISE - like in 1.1 and Steam
	Patch<const char*>(0x4925FB, "ESIOTROT");
	
	// BOOOOORING fixed
	Patch<BYTE>(0x4925D7, 10);

	// (Hopefully) more precise frame limiter
	int			pAddress = 0x582EFD;
	RsEventHandler = (void(*)(int,void*))(*(int*)(pAddress+1) + pAddress + 5);
	InjectHook(pAddress, NewFrameRender);
	InjectHook(0x582EA4, GetTimeSinceLastFrame);

	// Default to desktop res
	Patch<DWORD>(0x581E5E, desktop.right);
	Patch<DWORD>(0x581E68, desktop.bottom);
	Patch<BYTE>(0x581E72, 32);
	Patch<const char*>(0x581EA8, aNoDesktopMode);

	// No 12mb vram check
	Patch<BYTE>(0x581411, 0xEB);

	// No DirectPlay dependency
	Patch<BYTE>(0x5812D6, 0xB8);
	Patch<DWORD>(0x5812D7, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x580BB0, GetMyDocumentsPath, PATCH_JUMP);
}

void Patch_III_11(const RECT& desktop)
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

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x48E5F9, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x581FB4;
	Patch(0x581FB4, &pCustomWndProc);

	// (Hopefully) more precise frame limiter
	int			pAddress = 0x58323D;
	RsEventHandler = (void(*)(int,void*))(*(int*)(pAddress+1) + pAddress + 5);
	InjectHook(pAddress, NewFrameRender);
	InjectHook(0x5831E4, GetTimeSinceLastFrame);

	// Default to desktop res
	Patch<DWORD>(0x58219E, desktop.right);
	Patch<DWORD>(0x5821A8, desktop.bottom);
	Patch<BYTE>(0x5821B2, 32);
	Patch<const char*>(0x5821E8, aNoDesktopMode);

	// No 12mb vram check
	Patch<BYTE>(0x581753, 0xEB);

	// No DirectPlay dependency
	Patch<BYTE>(0x581620, 0xB8);
	Patch<DWORD>(0x581621, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x580F00, GetMyDocumentsPath, PATCH_JUMP);
}

void Patch_III_Steam(const RECT& desktop)
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

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x48E589, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x581EA4;
	Patch(0x581EA4, &pCustomWndProc);

	// (Hopefully) more precise frame limiter
	int			pAddress = 0x58312D;
	RsEventHandler = (void(*)(int,void*))(*(int*)(pAddress+1) + pAddress + 5);
	InjectHook(pAddress, NewFrameRender);
	InjectHook(0x5830D4, GetTimeSinceLastFrame);

	// Default to desktop res
	Patch<DWORD>(0x58208E, desktop.right);
	Patch<DWORD>(0x582098, desktop.bottom);
	Patch<BYTE>(0x5820A2, 32);
	Patch<const char*>(0x5820D8, aNoDesktopMode);

	// No 12mb vram check
	Patch<BYTE>(0x581653, 0xEB);

	// No DirectPlay dependency
	Patch<BYTE>(0x581520, 0xB8);
	Patch<DWORD>(0x581521, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x580E00, GetMyDocumentsPath, PATCH_JUMP);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		RECT			desktop;
		GetWindowRect(GetDesktopWindow(), &desktop);
		sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

		if (*(DWORD*)0x5C1E75 == 0xB85548EC) Patch_III_10(desktop);
		else if (*(DWORD*)0x5C2135 == 0xB85548EC) Patch_III_11(desktop);
		else if (*(DWORD*)0x5C6FD5 == 0xB85548EC) Patch_III_Steam(desktop);
		else return FALSE;

		CTimer::Initialise();

		HMODULE		hDummyHandle;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&DllMain, &hDummyHandle);
	}
	return TRUE;
}