#include "StdAfx.h"

#include "Maths.h"
#include "Timer.h"
#include "Utils/Patterns.h"
#include "Common.h"
#include "Common_ddraw.h"
#include "VehicleIII.h"
#include "ModelInfoIII.h"

#include <memory>
#include <Shlwapi.h>

#include "debugmenu_public.h"

#pragma comment(lib, "shlwapi.lib")

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

DebugMenuAPI gDebugMenuAPI;

static HMODULE hDLLModule;


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

static void (* const RsMouseSetPos)(RwV2d*) = AddressByVersion<void(*)(RwV2d*)>(0x580D20, 0x581070, 0x580F70);
static void (*orgConstructRenderList)();
void ResetMousePos()
{
	if ( bGameInFocus )
	{
		RwV2d	vecPos = { RsGlobal->MaximumWidth * 0.5f, RsGlobal->MaximumHeight * 0.5f };
		RsMouseSetPos(&vecPos);
	}
	orgConstructRenderList();
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
__declspec(safebuffers) int32_t GetTimeSinceLastFrame()
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

static void (__thiscall *orgGiveWeapon)( void* ped, unsigned int weapon, unsigned int ammo );
static void __fastcall GiveWeapon_SP( void* ped, void*, unsigned int weapon, unsigned int ammo )
{
	if ( ammo == 0 ) ammo = 1;
	orgGiveWeapon( ped, weapon, ammo );
}


// ============= Credits! =============
namespace Credits
{
	static void (*PrintCreditText)(float scaleX, float scaleY, const wchar_t* text, unsigned int& pos, float timeOffset);
	static void (*PrintCreditText_Hooked)(float scaleX, float scaleY, const wchar_t* text, unsigned int& pos, float timeOffset);

	static void PrintCreditSpace( float scale, unsigned int& pos )
	{
		pos += static_cast<unsigned int>( scale * 25.0f );
	}

	constexpr wchar_t xvChar(const wchar_t ch)
	{
		constexpr uint8_t xv = SILENTPATCH_REVISION_ID;
		return ch ^ xv;
	}

	constexpr wchar_t operator "" _xv(const char ch)
	{
		return xvChar(ch);
	}

	static void PrintSPCredits( float scaleX, float scaleY, const wchar_t* text, unsigned int& pos, float timeOffset )
	{
		// Original text we intercepted
		PrintCreditText_Hooked( scaleX, scaleY, text, pos, timeOffset );
		PrintCreditSpace( 2.0f, pos );

		{
			wchar_t spText[] = { 'A'_xv, 'N'_xv, 'D'_xv, '\0'_xv };

			for ( auto& ch : spText ) ch = xvChar(ch);
			PrintCreditText( 1.7f, 1.0f, spText, pos, timeOffset );
		}

		PrintCreditSpace( 2.0f, pos );

		{
			wchar_t spText[] = { 'A'_xv, 'D'_xv, 'R'_xv, 'I'_xv, 'A'_xv, 'N'_xv, ' '_xv, '\''_xv, 'S'_xv, 'I'_xv, 'L'_xv, 'E'_xv, 'N'_xv, 'T'_xv, '\''_xv, ' '_xv,
				'Z'_xv, 'D'_xv, 'A'_xv, 'N'_xv, 'O'_xv, 'W'_xv, 'I'_xv, 'C'_xv, 'Z'_xv, '\0'_xv };

			for ( auto& ch : spText ) ch = xvChar(ch);
			PrintCreditText( 1.7f, 1.7f, spText, pos, timeOffset );
		}
	}
}

// ============= Keyboard latency input fix =============
namespace KeyboardInputFix
{
	static void* NewKeyState;
	static void* OldKeyState;
	static void* TempKeyState;
	static constexpr size_t objSize = 0x270;
	static void (__fastcall *orgClearSimButtonPressCheckers)(void*);
	void __fastcall ClearSimButtonPressCheckers(void* pThis)
	{
		memcpy( OldKeyState, NewKeyState, objSize );
		memcpy( NewKeyState, TempKeyState, objSize );

		orgClearSimButtonPressCheckers(pThis);
	}
}

namespace Localization
{
	static int8_t forcedUnits = -1; // 0 - metric, 1 - imperial

	bool IsMetric_LocaleBased()
	{
		if ( forcedUnits != -1 ) return forcedUnits == 0;

		unsigned int LCData;
		if ( GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IMEASURE|LOCALE_RETURN_NUMBER, reinterpret_cast<LPTSTR>(&LCData), sizeof(LCData) / sizeof(TCHAR) ) != 0 )
		{
			return LCData == 0;
		}

		// If fails, default to metric. Hopefully never fails though
		return true;
	}

	static void (__thiscall* orgUpdateCompareFlag_IsMetric)(void* pThis, uint8_t flag);
	void __fastcall UpdateCompareFlag_IsMetric(void* pThis, void*, uint8_t)
	{
		std::invoke( orgUpdateCompareFlag_IsMetric, pThis, IsMetric_LocaleBased() );
	}

	uint32_t PrefsLanguage_IsMetric()
	{
		return IsMetric_LocaleBased();
	}
}


// ============= Call cDMAudio::IsAudioInitialised before adding one shot sounds, like in VC =============
namespace AudioInitializedFix
{
	auto IsAudioInitialised = static_cast<bool(*)()>(Memory::ReadCallFrom( hook::get_pattern( "E8 ? ? ? ? 84 C0 74 ? 0F B7 47 10" ) ));
	void* (*operatorNew)(size_t size);

	void* operatorNew_InitializedCheck( size_t size )
	{
		return IsAudioInitialised() ? operatorNew( size ) : nullptr;
	}

	void (*orgLoadAllAudioScriptObjects)(uint8_t*, uint32_t);
	void LoadAllAudioScriptObjects_InitializedCheck( uint8_t* buffer, uint32_t a2 )
	{
		if ( IsAudioInitialised() )
		{
			orgLoadAllAudioScriptObjects( buffer, a2 );
		}
	}
};


// ============= Corrected FBI Car secondary siren sound =============
namespace SirenSwitchingFix
{
	static bool (__thiscall *orgUsesSirenSwitching)(void* pThis, unsigned int index);
	static bool __fastcall UsesSirenSwitching_FbiCar( void* pThis, void*, unsigned int index )
	{
		// index 17 = FBICAR
		return index == 17 || orgUsesSirenSwitching( pThis, index );
	}
};


void InjectDelayedPatches_III_Common( bool bHasDebugMenu, const wchar_t* wcModulePath )
{
	using namespace Memory;
	using namespace hook;

	// Locale based metric/imperial system INI/debug menu
	{
		using namespace Localization;

		forcedUnits = static_cast<int8_t>(GetPrivateProfileIntW(L"SilentPatch", L"Units", -1, wcModulePath));
		if ( bHasDebugMenu )
		{
			static const char * const str[] = { "Default", "Metric", "Imperial" };
			DebugMenuEntry *e = DebugMenuAddVar( "SilentPatch", "Forced units", &forcedUnits, nullptr, 1, -1, 1, str );
			DebugMenuEntrySetWrap(e, true);
		}			
	}

	// Make crane be unable to lift Coach instead of Blista
	{
		// There is a possible incompatibility with limit adjusters, so patch it in a delayed hook point and gracefully handle failure
		auto canPickBlista = pattern( "83 FA 66 74" ).count_hint(1);
		if ( canPickBlista.size() == 1 )
		{
			Patch<int8_t>( canPickBlista.get_first<void>( 2 ), 127 ); // coach
		}
	}


	// Corrected siren corona placement for emergency vehicles
	if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableVehicleCoronaFixes", -1, wcModulePath) == 1 )
	{
		// Other mods might be touching it, so only patch specific vehicles if their code has not been touched at all
		{
			auto firetruckX1 = pattern( "C7 84 24 9C 05 00 00 CD CC 8C 3F" );
			auto firetruckY1 = pattern( "C7 84 24 A4 05 00 00 9A 99 D9 3F" );
			auto firetruckZ1 = pattern( "C7 84 24 A8 05 00 00 00 00 00 40" );

			auto firetruckX2 = pattern( "C7 84 24 A8 05 00 00 CD CC 8C BF" );
			auto firetruckY2 = pattern( "C7 84 24 B0 05 00 00 9A 99 D9 3F" );
			auto firetruckZ2 = pattern( "C7 84 24 B4 05 00 00 00 00 00 40" );

			if ( firetruckX1.count_hint(1).size() == 1 && firetruckY1.count_hint(1).size() == 1 && firetruckZ1.count_hint(1).size() == 1 &&
				firetruckX2.count_hint(1).size() == 1 && firetruckY2.count_hint(1).size() == 1 && firetruckZ2.count_hint(1).size() == 1 )
			{
				constexpr CVector FIRETRUCK_SIREN_POS(0.95f, 3.2f, 1.4f);

				Patch<float>( firetruckX1.get_first( 7 ), FIRETRUCK_SIREN_POS.x );
				Patch<float>( firetruckY1.get_first( 7 ), FIRETRUCK_SIREN_POS.y );
				Patch<float>( firetruckZ1.get_first( 7 ), FIRETRUCK_SIREN_POS.z );

				Patch<float>( firetruckX2.get_first( 7 ), -FIRETRUCK_SIREN_POS.x );
				Patch<float>( firetruckY2.get_first( 7 ), FIRETRUCK_SIREN_POS.y );
				Patch<float>( firetruckZ2.get_first( 7 ), FIRETRUCK_SIREN_POS.z );
			}
		}
		{
			auto ambulanceX1 = pattern( "C7 84 24 84 05 00 00 CD CC 8C 3F" );
			auto ambulanceY1 = pattern( "C7 84 24 8C 05 00 00 66 66 66 3F" );
			auto ambulanceZ1 = pattern( "C7 84 24 90 05 00 00 CD CC CC 3F" );

			auto ambulanceX2 = pattern( "C7 84 24 90 05 00 00 CD CC 8C BF" );
			auto ambulanceY2 = pattern( "C7 84 24 98 05 00 00 66 66 66 3F" );
			auto ambulanceZ2 = pattern( "C7 84 24 9C 05 00 00 CD CC CC 3F" );

			if ( ambulanceX1.count_hint(1).size() == 1 && ambulanceY1.count_hint(1).size() == 1 && ambulanceZ1.count_hint(1).size() == 1 &&
				ambulanceX2.count_hint(1).size() == 1 && ambulanceY2.count_hint(1).size() == 1 && ambulanceZ2.count_hint(1).size() == 1 )
			{
				constexpr CVector AMBULANCE_SIREN_POS(0.7f, 0.65f, 1.55f);

				Patch<float>( ambulanceX1.get_first( 7 ), AMBULANCE_SIREN_POS.x );
				Patch<float>( ambulanceY1.get_first( 7 ), AMBULANCE_SIREN_POS.y );
				Patch<float>( ambulanceZ1.get_first( 7 ), AMBULANCE_SIREN_POS.z );

				Patch<float>( ambulanceX2.get_first( 7 ), -AMBULANCE_SIREN_POS.x );
				Patch<float>( ambulanceY2.get_first( 7 ), AMBULANCE_SIREN_POS.y );
				Patch<float>( ambulanceZ2.get_first( 7 ), AMBULANCE_SIREN_POS.z );
			}
		}
		{
			auto enforcerX1 = pattern( "C7 84 24 6C 05 00 00 CD CC 8C 3F" );
			auto enforcerY1 = pattern( "C7 84 24 74 05 00 00 CD CC 4C 3F" );
			auto enforcerZ1 = pattern( "C7 84 24 78 05 00 00 9A 99 99 3F" );

			auto enforcerX2 = pattern( "C7 84 24 78 05 00 00 CD CC 8C BF" );
			auto enforcerY2 = pattern( "C7 84 24 80 05 00 00 CD CC 4C 3F" );
			auto enforcerZ2 = pattern( "C7 84 24 84 05 00 00 9A 99 99 3F" );

			if ( enforcerX1.count_hint(1).size() == 1 && enforcerY1.count_hint(1).size() == 1 && enforcerZ1.count_hint(1).size() == 1 &&
				enforcerX2.count_hint(1).size() == 1 && enforcerY2.count_hint(1).size() == 1 && enforcerZ2.count_hint(1).size() == 1 )
			{
				constexpr CVector ENFORCER_SIREN_POS(0.6f, 1.05f, 1.4f);

				Patch<float>( enforcerX1.get_first( 7 ), ENFORCER_SIREN_POS.x );
				Patch<float>( enforcerY1.get_first( 7 ), ENFORCER_SIREN_POS.y );
				Patch<float>( enforcerZ1.get_first( 7 ), ENFORCER_SIREN_POS.z );

				Patch<float>( enforcerX2.get_first( 7 ), -ENFORCER_SIREN_POS.x );
				Patch<float>( enforcerY2.get_first( 7 ), ENFORCER_SIREN_POS.y );
				Patch<float>( enforcerZ2.get_first( 7 ), ENFORCER_SIREN_POS.z );
			}
		}
		{
			auto chopper1 = pattern( "C7 44 24 44 00 00 E0 40 50 C7 44 24 4C 00 00 00 00" );	// Front light

			if ( chopper1.count_hint(1).size() == 1 )
			{
				constexpr CVector CHOPPER_SEARCH_LIGHT_POS(0.0f, 3.0f, -1.25f);

				auto match = chopper1.get_one();

				Patch( match.get<float>( 4 ), CHOPPER_SEARCH_LIGHT_POS.y );
				Patch( match.get<float>( 9 + 4 ), CHOPPER_SEARCH_LIGHT_POS.z );
			}
		}
	}


	// Corrected FBI Car secondary siren sound
	{
		using namespace SirenSwitchingFix;

		// Other mods might be touching it, so only patch specific vehicles if their code has not been touched at all
		auto usesSirenSwitching = pattern( "E8 ? ? ? ? 84 C0 74 12 83 C4 08" ).count_hint(1);
		if ( usesSirenSwitching.size() == 1 )
		{
			auto match = usesSirenSwitching.get_one();
			ReadCall( match.get<void>(), orgUsesSirenSwitching );
			InjectHook( match.get<void>(), UsesSirenSwitching_FbiCar );
		}
	}


	// Corrected CSimpleModelInfo::SetupBigBuilding minimum draw distance for big buildings without a matching model
	// Fixes cranes in Portland and bright windows in the city
	// By aap
	{
		auto setupMinDist = pattern( "C7 43 44 00 00 C8 42" ).count_hint(1);
		if ( setupMinDist.size() == 1 ) // In case of another mod or second instance of SP changing it
		{
			auto match = setupMinDist.get_one();

			// mov ecx, ebx
			// call CSimpleModelInfo::SetNearDistanceForLOD
			Patch( match.get<void>(), { 0x89, 0xD9 } );
			InjectHook( match.get<void>( 2 ), &CSimpleModelInfo::SetNearDistanceForLOD_SilentPatch, PATCH_CALL );
		}
	}
}

void InjectDelayedPatches_III_Common()
{
	std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

	// Obtain a path to the ASI
	wchar_t			wcModulePath[MAX_PATH];
	GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
	PathRenameExtensionW(wcModulePath, L".ini");

	const bool hasDebugMenu = DebugMenuLoad();

	InjectDelayedPatches_III_Common( hasDebugMenu, wcModulePath );

	Common::Patches::III_VC_DelayedCommon( hasDebugMenu, wcModulePath );
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
	ReadCall( 0x48E539, orgConstructRenderList );
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
	Nop(0x581C52, 2);
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
	ReadCall( 0x48E5F9, orgConstructRenderList );
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
	Nop(0x581F92, 2);
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
	ReadCall( 0x48E589, orgConstructRenderList );
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
		Patch<uint8_t>( addr.get<void>( 0x3E ), 0x51 );
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


	// Fixed ammo from SCM
	{
		auto give_weapon = get_pattern( "6B C0 4F 51 8B 34", 0x13 );
		ReadCall( give_weapon, orgGiveWeapon );
		InjectHook( give_weapon, GiveWeapon_SP );

		give_weapon = get_pattern( "89 C7 A1 ? ? ? ? 55 89 F9 50", 11 );
		InjectHook( give_weapon, GiveWeapon_SP );
	}


	// Credits =)
	{
		auto renderCredits = pattern( "83 C4 14 8D 45 F4 50 FF 35 ? ? ? ? E8 ? ? ? ? 59 59 8D 45 F4 50 FF 35 ? ? ? ? E8 ? ? ? ? 59 59 E8" ).get_one();

		ReadCall( renderCredits.get<void>( -48 ), Credits::PrintCreditText );
		ReadCall( renderCredits.get<void>( -5 ), Credits::PrintCreditText_Hooked );
		InjectHook( renderCredits.get<void>( -5 ), Credits::PrintSPCredits );
	}


	// Decreased keyboard input latency
	{
		using namespace KeyboardInputFix;

		auto updatePads = pattern( "BE ? ? ? ? BF ? ? ? ? A5" ).get_one();
		void* jmpDest = get_pattern( "66 A3 ? ? ? ? 5F", 6 );
		void* simButtonCheckers = get_pattern( "84 DB 74 11 6A 00", -0x24 );

		NewKeyState = *updatePads.get<void*>( 1 );
		OldKeyState = *updatePads.get<void*>( 5 + 1 );
		TempKeyState = *updatePads.get<void*>( 0x244 + 1 );

		ReadCall( simButtonCheckers, orgClearSimButtonPressCheckers );
		InjectHook( simButtonCheckers, ClearSimButtonPressCheckers );
		InjectHook( updatePads.get<void>( 10 ), jmpDest, PATCH_JUMP );
	}


	// Locale based metric/imperial system
	{
		using namespace Localization;

		void* updateCompareFlag = get_pattern( "89 E9 6A 00 E8 ? ? ? ? 30 C0 83 C4 70 5D 5E 5B C2 04 00", 4 );

		ReadCall( updateCompareFlag, orgUpdateCompareFlag_IsMetric );
		InjectHook( updateCompareFlag, UpdateCompareFlag_IsMetric );

		// Stats
		auto constructStatLine = pattern( "FF 24 9D ? ? ? ? 39 D0" ).get_one();

		// push eax
		// push edx
		// call IsMetric_LocaleBased
		// movzx ebx, al
		// pop edx
		// pop eax
		// nop...
		Patch( constructStatLine.get<void>( -0xF ), { 0x50, 0x52 } );
		InjectHook( constructStatLine.get<void>( -0xF + 2 ), PrefsLanguage_IsMetric, PATCH_CALL );
		Patch( constructStatLine.get<void>( -0xF + 7 ), { 0x0F, 0xB6, 0xD8, 0x5A, 0x58 } );
		Nop( constructStatLine.get<void>( -0xF + 12 ), 3 );
	}


	// Add cDMAudio::IsAudioInitialised checks before constructing cAudioScriptObject, like in VC
	{
		using namespace AudioInitializedFix;

		auto processCommands300 = pattern( "E8 ? ? ? ? 85 C0 59 74 ? 89 C1 E8 ? ? ? ? D9 05" ).get_one();
		ReadCall( processCommands300.get<void>( 0 ), operatorNew );

		InjectHook( processCommands300.get<void>( 0 ), operatorNew_InitializedCheck );
		Patch<int8_t>( processCommands300.get<void>( 8 + 1 ), 0x440B62 - 0x440B24 );

		auto processCommands300_2 = pattern( "6A 14 E8 ? ? ? ? 89 C3 59 85 DB 74" ).get_one();
		InjectHook( processCommands300_2.get<void>( 2 ), operatorNew_InitializedCheck );
		Patch<int8_t>( processCommands300_2.get<void>( 0xC + 1 ), 0x440BD7 - 0x440B8B );

		// We need to patch switch cases 0, 3, 4
		auto bulletInfoUpdate_Switch = *get_pattern<uintptr_t*>( "FF 24 85 ? ? ? ? 6A 14", 3 );

		const uintptr_t bulletInfoUpdate_0 = bulletInfoUpdate_Switch[0];
		const uintptr_t bulletInfoUpdate_3 = bulletInfoUpdate_Switch[3];
		const uintptr_t bulletInfoUpdate_4 = bulletInfoUpdate_Switch[4];

		InjectHook( bulletInfoUpdate_0 + 2, operatorNew_InitializedCheck );
		Patch<int8_t>( bulletInfoUpdate_0 + 0xA + 1, 0x558B79 - 0x558B36 );

		InjectHook( bulletInfoUpdate_3 + 2, operatorNew_InitializedCheck );
		Patch<int8_t>( bulletInfoUpdate_3 + 0xA + 1, 0x558C19 - 0x558BB1 );

		InjectHook( bulletInfoUpdate_4 + 2, operatorNew_InitializedCheck );
		Patch<int8_t>( bulletInfoUpdate_4 + 0xA + 1, 0x558C19 - 0x558BE3 );

		auto playlayOneShotScriptObject = pattern( "6A 14 E8 ? ? ? ? 85 C0 59 74 ? 89 C1 E8 ? ? ? ? D9 03 D9 58 04" ).get_one();
		InjectHook( playlayOneShotScriptObject.get<void>( 2 ), operatorNew_InitializedCheck );
		Patch<int8_t>( playlayOneShotScriptObject.get<void>( 0xA + 1 ), 0x57C633 - 0x57C601 );

		auto loadAllAudioScriptObjects = get_pattern( "FF B5 78 FF FF FF E8 ? ? ? ? 59 59 8B 45 C8", 6 );
		ReadCall( loadAllAudioScriptObjects, orgLoadAllAudioScriptObjects );
		InjectHook( loadAllAudioScriptObjects, LoadAllAudioScriptObjects_InitializedCheck );
	}


	// Give chopper/escape a properly sized collision bounding box instead of using ped's
	{
		auto initHelisPattern = pattern( "C6 40 2C 00 A1" ).count_hint(1);
		if ( initHelisPattern.size() == 1 )
		{
			static constexpr CColModel colModelChopper( CColSphere( 8.5f, CVector(0.0f, -1.75f, 0.73f), 0, 0 ), 
							CColBox( CVector(-2.18f, -8.52f, -0.67f), CVector(-2.18f, 4.58f, 2.125f), 0, 0 ) );

			auto initHelis = initHelisPattern.get_one();
			Patch( initHelis.get<void>( -7 + 3 ), &colModelChopper );
			Patch( initHelis.get<void>( 9 + 3 ), &colModelChopper );
		}
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		hDLLModule = hinstDLL;

		RECT			desktop;
		GetWindowRect(GetDesktopWindow(), &desktop);
		sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

		// This scope is mandatory so Protect goes out of scope before rwcseg gets fixed
		{
			std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

			const int8_t version = Memory::GetVersion().version;
			if ( version == 0 ) Patch_III_10(desktop);
			else if ( version == 1 ) Patch_III_11(desktop);
			else if ( version == 2 ) Patch_III_Steam(desktop);

			Patch_III_Common();
			Common::Patches::III_VC_Common();
			Common::Patches::DDraw_Common();

			Common::Patches::III_VC_SetDelayedPatchesFunc( InjectDelayedPatches_III_Common );
		}

		Common::Patches::FixRwcseg_Patterns();
	}
	return TRUE;
}

extern "C" __declspec(dllexport)
uint32_t GetBuildNumber()
{
	return (SILENTPATCH_REVISION_ID << 8) | SILENTPATCH_BUILD_ID;
}