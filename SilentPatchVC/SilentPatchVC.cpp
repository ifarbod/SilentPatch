#include "StdAfx.h"

#include "Timer.h"

struct RsGlobalType
{
	const char*		AppName;
	unsigned int	unkWidth, unkHeight;
	signed int		MaximumWidth;
	signed int		MaximumHeight;
	unsigned int	frameLimit;
	BOOL			quit;
	void*			ps;
	void*			keyboard;
	void*			mouse;
	void*			pad;
};

struct RwV2d
{
    float x;   /**< X value*/
    float y;   /**< Y value */
};

bool*					bSnapShotActive;
static const void*		RosieAudioFix_JumpBack;


void (__stdcall *AudioResetTimers)(unsigned int);
static void (*PrintString)(float,float,const wchar_t*);

static RsGlobalType*	RsGlobal;
static const void*		SubtitlesShadowFix_JumpBack;

inline float GetWidthMult()
{
	static const float&		ResolutionWidthMult = **AddressByVersion<float**>(0x5FA15E, 0x5FA17E, 0x5F9DBE);
	return ResolutionWidthMult;
}

inline float GetHeightMult()
{
	static const float&		ResolutionHeightMult = **AddressByVersion<float**>(0x5FA148, 0x5FA168, 0x5F9DA8);
	return ResolutionHeightMult;
}


void __declspec(naked) RosiesAudioFix()
{
	_asm
	{
		mov     byte ptr [ebx+0CCh], 0
		mov     byte ptr [ebx+148h], 0
		jmp		[RosieAudioFix_JumpBack]
	}
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

static void (* const ConstructRenderList)() = AddressByVersion<void(*)()>(0x4CA260, 0x4CA280, 0x4CA120);
static void (* const RsMouseSetPos)(RwV2d*) = AddressByVersion<void(*)(RwV2d*)>(0x6030C0, 0x6030A0, 0x602CE0);
void ResetMousePos()
{
	if ( bGameInFocus )
	{
		RwV2d	vecPos = { RsGlobal->MaximumWidth * 0.5f, RsGlobal->MaximumHeight * 0.5f };
		RsMouseSetPos(&vecPos);
	}
	ConstructRenderList();
}

void __stdcall Recalculate(float& fX, float& fY, signed int nShadow)
{
	fX = nShadow * GetWidthMult() * RsGlobal->MaximumWidth;
	fY = nShadow * GetHeightMult() * RsGlobal->MaximumHeight;
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
		mov		[esp], eax
		fild	[esp]
		push	eax
		lea		eax, [esp+20h-18h]
		push	eax
		lea		eax, [esp+24h-14h]
		push	eax
		call	Recalculate
		jmp		SubtitlesShadowFix_JumpBack
	}
}

char** const ppUserFilesDir = AddressByVersion<char**>(0x6022AA, 0x60228A, 0x601ECA);

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


static signed int& LastTimeFireTruckCreated = **AddressByVersion<int**>(0x429435, 0x429435, 0x429405);
static signed int& LastTimeAmbulanceCreated = **AddressByVersion<int**>(0x429449, 0x429449, 0x429419);
static void (*orgCarCtrlReInit)();
void CarCtrlReInit_SilentPatch()
{
	orgCarCtrlReInit();
	LastTimeFireTruckCreated = 0;
	LastTimeAmbulanceCreated = 0;
}

static bool& RespraysAreFree = **AddressByVersion<bool**>(0x430D17, 0x430D17, 0x430CE7);
void GaragesInit_SilentPatch()
{
	RespraysAreFree = false;
}

static char		aNoDesktopMode[64];

void Patch_VC_10(const RECT& desktop)
{
	using namespace Memory;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F98D0;
	PrintString = (void(*)(float,float,const wchar_t*))0x551040;

	bSnapShotActive = *(bool**)0x4D1239;
	RsGlobal = *(RsGlobalType**)0x602D32;
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

	// Mouse fucking fix!
	Patch<DWORD>(0x601740, 0xC3C030);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x6004A2, RsEventHandler );
	InjectHook(0x6004A2, NewFrameRender);
	InjectHook(0x600449, GetTimeSinceLastFrame);

	// Default to desktop res
	Patch<DWORD>(0x600E7E, desktop.right);
	Patch<DWORD>(0x600E88, desktop.bottom);
	Patch<BYTE>(0x600E92, 32);
	Patch<const char*>(0x600EC8, aNoDesktopMode);

	// No 12mb vram check
	Patch<BYTE>(0x601E26, 0xEB);

	// No DirectPlay dependency
	Patch<BYTE>(0x601CA0, 0xB8);
	Patch<DWORD>(0x601CA1, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x602240, GetMyDocumentsPath, PATCH_JUMP);

	InjectHook(0x601A40, GetMyDocumentsPath, PATCH_CALL);
	InjectHook(0x601A45, 0x601B2F, PATCH_JUMP);

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x4A5E45, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x601727;
	Patch(0x601727, &pCustomWndProc);

	// Y axis sensitivity fix
	// By ThirteenAG
	float* sens = *(float**)0x4796E5;
	Patch<const void*>(0x479410 + 0x2E0 + 0x2, sens);
	Patch<const void*>(0x47A20E + 0x27D + 0x2, sens);
	Patch<const void*>(0x47AE27 + 0x1CC + 0x2, sens);
	Patch<const void*>(0x47BE8F + 0x22E + 0x2, sens);
	Patch<const void*>(0x481AB3 + 0x4FE + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<BYTE>(0x47C11E, 0xEB);
	Patch<BYTE>(0x47CD94, 0xEB);
	Nop(0x47C15A, 2);

	// Scan for A/B drives looking for audio files
	Patch<DWORD>(0x5D7941, 'A');
	Patch<DWORD>(0x5D7B04, 'A');


	// ~x~ as cyan blip
	// Shared with GInput
	Patch<BYTE>(0x550481, 0);
	Patch<BYTE>(0x550488, 255);
	Patch<BYTE>(0x55048F, 255);

	Patch<BYTE>(0x5505FF, 0);
	Patch<BYTE>(0x550603, 255);
	Patch<BYTE>(0x550607, 255);

	
	// Corrected crime codes
	Patch<DWORD>(0x5FDDDB, 0xC5);


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x4A489B, orgCarCtrlReInit );
	InjectHook(0x4A489B, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	InjectHook(0x4349BB, GaragesInit_SilentPatch, PATCH_JUMP);

	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AED14+1, 1); // katana
	Patch<BYTE>(0x4AEB74+1, 1); // chainsaw


	// Adblocker
#if DISABLE_FLA_DONATION_WINDOW
	if ( *(DWORD*)0x5FFAE9 != 0x006A026A )
	{
		Patch<DWORD>(0x5FFAE9, 0x006A026A);
		Patch<WORD>(0x5FFAED, 0x006A);
	}
#endif
}

void Patch_VC_11(const RECT& desktop)
{
	using namespace Memory;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F98F0;
	PrintString = (void(*)(float,float,const wchar_t*))0x551060;

	bSnapShotActive = *(bool**)0x4D1259;
	RsGlobal = *(RsGlobalType**)0x602D12;
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

	// Mouse fucking fix!
	Patch<DWORD>(0x601770, 0xC3C030);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x6004C2, RsEventHandler );
	InjectHook(0x6004C2, NewFrameRender);
	InjectHook(0x600469, GetTimeSinceLastFrame);

	// Default to desktop res
	Patch<DWORD>(0x600E9E, desktop.right);
	Patch<DWORD>(0x600EA8, desktop.bottom);
	Patch<BYTE>(0x600EB2, 32);
	Patch<const char*>(0x600EE8, aNoDesktopMode);

	// No 12mb vram check
	Patch<BYTE>(0x601E56, 0xEB);

	// No DirectPlay dependency
	Patch<BYTE>(0x601CD0, 0xB8);
	Patch<DWORD>(0x601CD1, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x602220, GetMyDocumentsPath, PATCH_JUMP);

	InjectHook(0x601A70, GetMyDocumentsPath, PATCH_CALL);
	InjectHook(0x601A75, 0x601B5F, PATCH_JUMP);

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x4A5E65, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x601757;
	Patch(0x601757, &pCustomWndProc);

	// Y axis sensitivity fix
	// By ThirteenAG
	float* sens = *(float**)0x4796E5;
	Patch<const void*>(0x479410 + 0x2E0 + 0x2, sens);
	Patch<const void*>(0x47A20E + 0x27D + 0x2, sens);
	Patch<const void*>(0x47AE27 + 0x1CC + 0x2, sens);
	Patch<const void*>(0x47BE8F + 0x22E + 0x2, sens);
	Patch<const void*>(0x481AB3 + 0x4FE + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<BYTE>(0x47C11E, 0xEB);
	Patch<BYTE>(0x47CD94, 0xEB);
	Nop(0x47C15A, 2);

	// Scan for A/B drives looking for audio files
	Patch<DWORD>(0x5D7961, 'A');
	Patch<DWORD>(0x5D7B24, 'A');


	// ~x~ as cyan blip
	// Shared with GInput
	Patch<BYTE>(0x5504A1, 0);
	Patch<BYTE>(0x5504A8, 255);
	Patch<BYTE>(0x5504AF, 255);

	Patch<BYTE>(0x55061F, 0);
	Patch<BYTE>(0x550623, 255);
	Patch<BYTE>(0x550627, 255);


	// Corrected crime codes
	Patch<DWORD>(0x5FDDFB, 0xC5);


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x4A48BB, orgCarCtrlReInit );
	InjectHook(0x4A48BB, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	InjectHook(0x4349BB, GaragesInit_SilentPatch, PATCH_JUMP);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AED34+1, 1); // katana
	Patch<BYTE>(0x4AEB94+1, 1); // chainsaw
}

void Patch_VC_Steam(const RECT& desktop)
{
	using namespace Memory;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F9530;
	PrintString = (void(*)(float,float,const wchar_t*))0x550F30;

	bSnapShotActive = *(bool**)0x4D10F9;
	RsGlobal = *(RsGlobalType**)0x602952;
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

	// Mouse fucking fix!
	Patch<DWORD>(0x6013B0, 0xC3C030);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x600102, RsEventHandler );
	InjectHook(0x600102, NewFrameRender);
	InjectHook(0x6000A9, GetTimeSinceLastFrame);

	// Default to desktop res
	Patch<DWORD>(0x600ADE, desktop.right);
	Patch<DWORD>(0x600AE8, desktop.bottom);
	Patch<BYTE>(0x600AF2, 32);
	Patch<const char*>(0x600B28, aNoDesktopMode);

	// No 12mb vram check
	Patch<BYTE>(0x601A96, 0xEB);

	// No DirectPlay dependency
	Patch<BYTE>(0x601910, 0xB8);
	Patch<DWORD>(0x601911, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x601E60, GetMyDocumentsPath, PATCH_JUMP);

	InjectHook(0x6016B0, GetMyDocumentsPath, PATCH_CALL);
	InjectHook(0x6016B5, 0x60179F, PATCH_JUMP);

	// RsMouseSetPos call (SA style fix)
	InjectHook(0x4A5D15, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x601397;
	Patch(0x601397, &pCustomWndProc);

	// Y axis sensitivity fix
	// By ThirteenAG
	float* sens = *(float**)0x4795C5;
	Patch<const void*>(0x4792F0 + 0x2E0 + 0x2, sens);
	Patch<const void*>(0x47A0EE + 0x27D + 0x2, sens);
	Patch<const void*>(0x47AD07 + 0x1CC + 0x2, sens);
	Patch<const void*>(0x47BD6F + 0x22E + 0x2, sens);
	Patch<const void*>(0x481993 + 0x4FE + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<BYTE>(0x47BFFE, 0xEB);
	Patch<BYTE>(0x47CC74, 0xEB);
	Nop(0x47C03A, 2);

	// Scan for A/B drives looking for audio files
	Patch<DWORD>(0x5D7764, 'A');


	// ~x~ as cyan blip
	// Shared with GInput
	Patch<BYTE>(0x550371, 0);
	Patch<BYTE>(0x550378, 255);
	Patch<BYTE>(0x55037F, 255);

	Patch<BYTE>(0x5504EF, 0);
	Patch<BYTE>(0x5504F3, 255);
	Patch<BYTE>(0x5504F7, 255);


	// Corrected crime codes
	Patch<DWORD>(0x5FDA3B, 0xC5);


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x4A475B, orgCarCtrlReInit );
	InjectHook(0x4A475B, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	InjectHook(0x43497B, GaragesInit_SilentPatch, PATCH_JUMP);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AEA44+1, 1); // katana
	Patch<BYTE>(0x4AEBE4+1, 1); // chainsaw
}

void Patch_VC_JP()
{
	using namespace Memory;
	ScopedUnprotect::Section Protect( (HINSTANCE)0x400000, ".text" );

	// Y axis sensitivity fix
	// By ThirteenAG
	Patch<DWORD>(0x4797E7 + 0x2E0 + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47A5E5 + 0x27D + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47B1FE + 0x1CC + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47C266 + 0x22E + 0x2, 0x94ABD8);
	Patch<DWORD>(0x481E8A + 0x4FE + 0x2, 0x94ABD8);
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

		ScopedUnprotect::Section Protect( (HINSTANCE)0x400000, ".text" );

		if(*(DWORD*)0x667BF5 == 0xB85548EC) Patch_VC_10(desktop);
		else if(*(DWORD*)0x667C45 == 0xB85548EC) Patch_VC_11(desktop);
		else if (*(DWORD*)0x666BA5 == 0xB85548EC) Patch_VC_Steam(desktop);

		// Y axis sensitivity only
		else if (*(DWORD*)0x601048 == 0x5E5F5D60)
		{
			Patch_VC_JP();
			return TRUE;
		}
		else return FALSE;

		CTimer::Initialise();

		HMODULE		hDummyHandle;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&DllMain, &hDummyHandle);
	}
	return TRUE;
}