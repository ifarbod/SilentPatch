#include "StdAfx.h"

#include "Maths.h"
#include "Timer.h"
#include "Common.h"
#include "Common_ddraw.h"
#include "Desktop.h"
#include "ModelInfoVC.h"
#include "VehicleVC.h"
#include "SVF.h"
#include "RWUtils.hpp"

#include <array>
#include <memory>
#include <Shlwapi.h>

#include "Utils/ModuleList.hpp"
#include "Utils/Patterns.h"
#include "Utils/ScopedUnprotect.hpp"

#include "debugmenu_public.h"

#pragma comment(lib, "shlwapi.lib")

// ============= Mod compatibility stuff =============

namespace ModCompat
{
	namespace Utils
	{
		template<typename AT>
		HMODULE GetModuleHandleFromAddress( AT address )
		{
			HMODULE result = nullptr;
			GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, LPCTSTR(address), &result );
			return result;
		}
	}
}

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

DebugMenuAPI gDebugMenuAPI;

static HMODULE		hDLLModule;

static const void*		RosieAudioFix_JumpBack;

static void (*PrintString)(float,float,const wchar_t*);

static RsGlobalType*	RsGlobal;
static const void*		SubtitlesShadowFix_JumpBack;

// This is actually CBaseModelInfo, but we currently don't have it defined
CVehicleModelInfo**& ms_modelInfoPtrs = *hook::get_pattern<CVehicleModelInfo**>("8B 15 ? ? ? ? 8D 04 24", 2);
int32_t& numModelInfos = *hook::get_pattern<int32_t>("81 FD ? ? ? ? 7C B7", 2);

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
static auto* const pCustomWndProc = CustomWndProc;

static void (* const RsMouseSetPos)(RwV2d*) = AddressByVersion<void(*)(RwV2d*)>(0x6030C0, 0x6030A0, 0x602CE0);
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

void __stdcall Recalculate(float& fX, float& fY, signed int nShadow)
{
	fX = nShadow * GetWidthMult() * RsGlobal->MaximumWidth;
	fY = nShadow * GetHeightMult() * RsGlobal->MaximumHeight;
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

void __declspec(naked) CreateInstance_BikeFix()
{
	_asm
	{
		push	eax
		mov		ecx, ebp
		call	CVehicleModelInfo::GetExtrasFrame
		retn
	}
}

extern char** ppUserFilesDir = AddressByVersion<char**>(0x6022AA, 0x60228A, 0x601ECA);

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

static void (*orgPickNextNodeToChaseCar)(void*, float, float, void*);
static float PickNextNodeToChaseCarZ = 0.0f;
static void PickNextNodeToChaseCarXYZ( void* vehicle, const CVector& vec, void* chaseTarget )
{
	PickNextNodeToChaseCarZ = vec.z;
	orgPickNextNodeToChaseCar( vehicle, vec.x, vec.y, chaseTarget );
	PickNextNodeToChaseCarZ = 0.0f;
}


static char		aNoDesktopMode[64];

unsigned int __cdecl AutoPilotTimerCalculation_VC(unsigned int nTimer, int nScaleFactor, float fScaleCoef)
{
	return nTimer - static_cast<unsigned int>(nScaleFactor * fScaleCoef);
}

void __declspec(naked) AutoPilotTimerFix_VC()
{
	_asm {
		push	dword ptr[esp + 0xC]
		push	dword ptr[ebx + 0x10]
		push	eax
		call	AutoPilotTimerCalculation_VC
		add 	esp, 0xC
		mov 	[ebx + 0xC], eax
		add     esp, 0x30
		pop     ebp
		pop     ebx
		retn    4
	}
}

namespace ZeroAmmoFix
{

template<std::size_t Index>
static void (__fastcall *orgGiveWeapon)(void* ped, void*, unsigned int weapon, unsigned int ammo, bool flag);

template<std::size_t Index>
static void __fastcall GiveWeapon_SP(void* ped, void*, unsigned int weapon, unsigned int ammo, bool flag)
{
	orgGiveWeapon<Index>(ped, nullptr, weapon, std::max(1u, ammo), flag);
}
HOOK_EACH_FUNC(GiveWeapon, orgGiveWeapon, GiveWeapon_SP);

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
		PrintCreditSpace( 1.5f, pos );

		{
			wchar_t spText[] = { 'A'_xv, 'N'_xv, 'D'_xv, '\0'_xv };

			for ( auto& ch : spText ) ch = xvChar(ch);
			PrintCreditText( 1.1f, 0.8f, spText, pos, timeOffset );
		}

		PrintCreditSpace( 1.5f, pos );

		{
			wchar_t spText[] = { 'A'_xv, 'D'_xv, 'R'_xv, 'I'_xv, 'A'_xv, 'N'_xv, ' '_xv, '\''_xv, 'S'_xv, 'I'_xv, 'L'_xv, 'E'_xv, 'N'_xv, 'T'_xv, '\''_xv, ' '_xv,
				'Z'_xv, 'D'_xv, 'A'_xv, 'N'_xv, 'O'_xv, 'W'_xv, 'I'_xv, 'C'_xv, 'Z'_xv, '\0'_xv };

			for ( auto& ch : spText ) ch = xvChar(ch);
			PrintCreditText( 1.1f, 1.1f, spText, pos, timeOffset );
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


// ============= Corrected FBI Washington sirens sound =============
namespace SirenSwitchingFix
{
	void __declspec(naked) IsFBIRanchOrFBICar()
	{
		_asm
		{
			mov     dword ptr [esi+1Ch], 1Ch

			// al = 0 - high pitched siren
			// al = 1 - normal siren
			cmp     dword ptr [ebp+14h], 90		// fbiranch
			je      IsFBIRanchOrFBICar_HighPitchSiren
			cmp     dword ptr [ebp+14h], 17		// fbicar
			setne   al
			retn

		IsFBIRanchOrFBICar_HighPitchSiren:
			xor     al, al
			retn
		}
	}
}


// ============= Corrected siren corona placement for FBI cars and Vice Cheetah =============
namespace FBISirenCoronaFix
{
	bool overridePosition;
	CVector vecOverridePosition;

	// True - don't display siren
	// False - display siren
	bool SetUpFBISiren( const CVehicle* vehicle )
	{
		SVF::Feature foundFeature = SVF::Feature::NO_FEATURE;
		SVF::ForAllModelFeatures( vehicle->GetModelIndex(), [&]( SVF::Feature f ) {
			if ( f >= SVF::Feature::FBI_RANCHER_SIREN && f <= SVF::Feature::VICE_CHEETAH_SIREN )
			{
				foundFeature = f;
				return false;
			}
			return true;
		} );

		if ( foundFeature != SVF::Feature::NO_FEATURE )
		{
			if ( foundFeature != SVF::Feature::VICE_CHEETAH_SIREN )
			{
				constexpr CVector FBICAR_SIREN_POS(0.4f, 0.8f, 0.25f);
				constexpr CVector FBIRANCH_SIREN_POS(0.5f, 1.12f, 0.5f);

				overridePosition = true;
				vecOverridePosition = foundFeature == SVF::Feature::FBI_WASHINGTON_SIREN ? FBICAR_SIREN_POS : FBIRANCH_SIREN_POS;
			}
			else
			{
				overridePosition = false;
			}

			return false;
		}

		return true;
	}

	CVector& __fastcall SetUpVector( CVector& out, void*, float X, float Y, float Z )
	{
		if ( overridePosition )
		{
			out = vecOverridePosition;
		}
		else
		{
			out = CVector(X, Y, Z);
		}

		return out;
	}
}


// ============= Fixed vehicles exploding twice if the driver leaves the car while it's exploding =============
namespace RemoveDriverStatusFix
{
	__declspec(naked) void RemoveDriver_SetStatus()
	{
		// if (m_nStatus != STATUS_WRECKED)
		//   m_nStatus = STATUS_ABANDONED;
		_asm
		{
			mov		cl, [ebx+50h]
			mov		al, cl
			and		cl, 0F8h
			cmp		cl, 28h
			je		DontSetStatus
			and     al, 7
			or      al, 20h

		DontSetStatus:
			retn
		}
	}
}


// ============= Apply the environment mapping on extra components =============
namespace EnvMapsOnExtras
{
	static void RemoveSpecularityFromAtomic(RpAtomic* atomic)
	{
		RpGeometry* geometry = RpAtomicGetGeometry(atomic);
		if (geometry != nullptr)
		{
			RpGeometryForAllMaterials(geometry, [](RpMaterial* material)
				{
					bool bRemoveSpecularity = false;

					// Only remove specularity from the body materials, keep glass intact.
					// This is only done on a best-effort basis, as mods can fine-tune it better
					// and just remove the model from the exceptions list
					RwTexture* texture = RpMaterialGetTexture(material);
					if (texture != nullptr)
					{
						if (strstr(RwTextureGetName(texture), "glass") == nullptr && strstr(RwTextureGetMaskName(texture), "glass") == nullptr)
						{
							bRemoveSpecularity = true;
						}
					}

					if (bRemoveSpecularity)
					{
						RpMaterialGetSurfaceProperties(material)->specular = 0.0f;
					}
					return material;
				});
		}
	}

	static RpClump* (*orgRpClumpForAllAtomics)(RpClump* clump, RpAtomicCallBack callback, void* data);
	static RpClump* RpClumpForAllAtomics_ExtraComps(CVehicleModelInfo* modelInfo, RpAtomicCallBack callback, void* data)
	{
		RpClump* result = orgRpClumpForAllAtomics(modelInfo->m_clump, callback, data);

		const int32_t modelID = std::distance(ms_modelInfoPtrs, std::find(ms_modelInfoPtrs, ms_modelInfoPtrs+numModelInfos, modelInfo));
		const bool bRemoveSpecularity = ExtraCompSpecularity::SpecularityExcluded(modelID);
		for (int32_t i = 0; i < modelInfo->m_numComps; i++)
		{
			if (bRemoveSpecularity)
			{
				RemoveSpecularityFromAtomic(modelInfo->m_comps[i]);
			}

			callback(modelInfo->m_comps[i], data);
			CVehicleModelInfo::AttachCarPipeToRwObject(reinterpret_cast<RwObject*>(modelInfo->m_comps[i]));
		}
		return result;
	}
}


void InjectDelayedPatches_VC_Common( bool bHasDebugMenu, const wchar_t* wcModulePath )
{
	using namespace Memory;
	using namespace hook;

	const ModuleList moduleList;

	const HMODULE skygfxModule = moduleList.Get(L"skygfx");
	if (skygfxModule != nullptr)
	{
		auto attachCarPipe = reinterpret_cast<void(*)(RwObject*)>(GetProcAddress(skygfxModule, "AttachCarPipeToRwObject"));
		if (attachCarPipe != nullptr)
		{
			CVehicleModelInfo::AttachCarPipeToRwObject = attachCarPipe;
		}
	}

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

	
	// Corrected siren corona placement for emergency vehicles
	if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableVehicleCoronaFixes", -1, wcModulePath) == 1 )
	{
		// Other mods might be touching it, so only patch specific vehicles if their code has not been touched at all
		{
			auto firetruck1 = pattern( "8D 8C 24 24 09 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );
			auto firetruck2 = pattern( "8D 8C 24 30 09 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );

			if ( firetruck1.count_hint(1).size() == 1 && firetruck2.count_hint(1).size() == 1 )
			{
				static const CVector FIRETRUCK_SIREN_POS(0.95f, 3.2f, 1.4f);
				static const float FIRETRUCK_SIREN_MINUS_X = -FIRETRUCK_SIREN_POS.x;

				auto match1 = firetruck1.get_one();
				auto match2 = firetruck2.get_one();

				Patch( match1.get<float*>( 7 + 2 ), &FIRETRUCK_SIREN_POS.z );
				Patch( match1.get<float*>( 7 + 2 + (6*1) ), &FIRETRUCK_SIREN_POS.y );
				Patch( match1.get<float*>( 7 + 2 + (6*2) ), &FIRETRUCK_SIREN_POS.x );

				Patch( match2.get<float*>( 7 + 2 ), &FIRETRUCK_SIREN_POS.z );
				Patch( match2.get<float*>( 7 + 2 + (6*1) ), &FIRETRUCK_SIREN_POS.y );
				Patch( match2.get<float*>( 7 + 2 + (6*2) ), &FIRETRUCK_SIREN_MINUS_X );
			}
		}
		{
			auto ambulan1 = pattern( "8D 8C 24 0C 09 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );
			auto ambulan2 = pattern( "8D 8C 24 18 09 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );

			if ( ambulan1.count_hint(1).size() == 1 && ambulan2.count_hint(1).size() == 1 )
			{
				static const CVector AMBULANCE_SIREN_POS(0.7f, 0.65f, 1.55f);
				static const float AMBULANCE_SIREN_MINUS_X = -AMBULANCE_SIREN_POS.x;

				auto match1 = ambulan1.get_one();
				auto match2 = ambulan2.get_one();

				Patch( match1.get<float*>( 7 + 2 ), &AMBULANCE_SIREN_POS.z );
				Patch( match1.get<float*>( 7 + 2 + (6*1) ), &AMBULANCE_SIREN_POS.y );
				Patch( match1.get<float*>( 7 + 2 + (6*2) ), &AMBULANCE_SIREN_POS.x );

				Patch( match2.get<float*>( 7 + 2 ), &AMBULANCE_SIREN_POS.z );
				Patch( match2.get<float*>( 7 + 2 + (6*1) ), &AMBULANCE_SIREN_POS.y );
				Patch( match2.get<float*>( 7 + 2 + (6*2) ), &AMBULANCE_SIREN_MINUS_X );
			}
		}
		{
			auto police1 = pattern( "8D 8C 24 DC 08 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );
			auto police2 = pattern( "8D 8C 24 E8 08 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );

			if ( police1.count_hint(1).size() == 1 && police2.count_hint(1).size() == 1 )
			{
				static const CVector POLICE_SIREN_POS(0.55f, -0.4f, 0.95f);
				static const float POLICE_SIREN_MINUS_X = -POLICE_SIREN_POS.x;

				auto match1 = police1.get_one();
				auto match2 = police2.get_one();

				Patch( match1.get<float*>( 7 + 2 ), &POLICE_SIREN_POS.z );
				Patch( match1.get<float*>( 7 + 2 + (6*1) ), &POLICE_SIREN_POS.y );
				Patch( match1.get<float*>( 7 + 2 + (6*2) ), &POLICE_SIREN_POS.x );

				Patch( match2.get<float*>( 7 + 2 ), &POLICE_SIREN_POS.z );
				Patch( match2.get<float*>( 7 + 2 + (6*1) ), &POLICE_SIREN_POS.y );
				Patch( match2.get<float*>( 7 + 2 + (6*2) ), &POLICE_SIREN_MINUS_X );
			}
		}
		{
			auto enforcer1 = pattern( "8D 8C 24 F4 08 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );
			auto enforcer2 = pattern( "8D 8C 24 00 09 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35" );

			if ( enforcer1.count_hint(1).size() == 1 && enforcer2.count_hint(1).size() == 1 )
			{
				static const CVector ENFORCER_SIREN_POS(0.6f, 1.05f, 1.4f);
				static const float ENFORCER_SIREN_MINUS_X = -ENFORCER_SIREN_POS.x;

				auto match1 = enforcer1.get_one();
				auto match2 = enforcer2.get_one();

				Patch( match1.get<float*>( 7 + 2 ), &ENFORCER_SIREN_POS.z );
				Patch( match1.get<float*>( 7 + 2 + (6*1) ), &ENFORCER_SIREN_POS.y );
				Patch( match1.get<float*>( 7 + 2 + (6*2) ), &ENFORCER_SIREN_POS.x );

				Patch( match2.get<float*>( 7 + 2 ), &ENFORCER_SIREN_POS.z );
				Patch( match2.get<float*>( 7 + 2 + (6*1) ), &ENFORCER_SIREN_POS.y );
				Patch( match2.get<float*>( 7 + 2 + (6*2) ), &ENFORCER_SIREN_MINUS_X );	
			}
		}
		{
			auto chopper1 = pattern( "C7 44 24 44 00 00 E0 40 50 C7 44 24 4C 00 00 00 00" );	// Front light
			auto chopper2 = pattern( "C7 44 24 6C 00 00 10 C1 8D 44 24 5C C7 44 24 70 00 00 00 00" );	// Tail light

			if ( chopper1.count_hint(1).size() == 1 )
			{
				constexpr CVector CHOPPER_SEARCH_LIGHT_POS(0.0f, 3.0f, -1.0f);	// Same as in III Aircraft (not implemented there yet!)

				auto match = chopper1.get_one();

				Patch( match.get<float>( 4 ), CHOPPER_SEARCH_LIGHT_POS.y );
				Patch( match.get<float>( 9 + 4 ), CHOPPER_SEARCH_LIGHT_POS.z );
			}

			if ( chopper2.count_hint(1).size() == 1 )
			{
				constexpr CVector CHOPPER_RED_LIGHT_POS(0.0f, -7.5f, 2.5f);	// Same as in III Aircraft

				auto match = chopper2.get_one();

				Patch( match.get<float>( 4 ), CHOPPER_RED_LIGHT_POS.y );
				Patch( match.get<float>( 12 + 4 ), CHOPPER_RED_LIGHT_POS.z );
			}
		}
		{
			using namespace FBISirenCoronaFix;

			auto hasFBISiren = pattern( "83 E9 04 0F 84 87 0A 00 00 83 E9 10" ); // Predicate for showing FBI/Vice Squad siren
			auto viceCheetah = pattern( "8D 8C 24 CC 09 00 00 FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35 ? ? ? ? E8" ); // Siren pos

			if ( viceCheetah.count_hint(1).size() == 1 )
			{
				auto match = viceCheetah.get_one();

				if ( hasFBISiren.count_hint(1).size() == 1 )
				{
					auto matchSiren = hasFBISiren.get_one();

					Patch<uint8_t>( matchSiren.get<void>(), 0x55 ); // push ebp
					InjectHook( matchSiren.get<void>( 1 ), SetUpFBISiren, HookType::Call );
					Patch( matchSiren.get<void>( 1 + 5 ), { 0x83, 0xC4, 0x04, 0x84, 0xC0, 0x90 } ); // add esp, 4 / test al, al / nop

					InjectHook( match.get<void>( 0x19 ), SetUpVector );
				}

				static const float VICE_CHEETAH_SIREN_POS_Z = 0.25f;
				Patch( match.get<float*>( 7 + 2 ), &VICE_CHEETAH_SIREN_POS_Z );
			}
		}
	}
}

void InjectDelayedPatches_VC_Common()
{
	std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

	// Obtain a path to the ASI
	wchar_t			wcModulePath[MAX_PATH];
	GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
	PathRenameExtensionW(wcModulePath, L".ini");

	const bool hasDebugMenu = DebugMenuLoad();

	InjectDelayedPatches_VC_Common( hasDebugMenu, wcModulePath );

	Common::Patches::III_VC_DelayedCommon( hasDebugMenu, wcModulePath );
}

void Patch_VC_10(uint32_t width, uint32_t height)
{
	using namespace Memory;

	PrintString = (void(*)(float,float,const wchar_t*))0x551040;

	RsGlobal = *(RsGlobalType**)0x602D32;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551701;

	InjectHook(0x5433BD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, HookType::Jump);

	InjectHook(0x5516FC, SubtitlesShadowFix, HookType::Jump);
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


	// RsMouseSetPos call (SA style fix)
	ReadCall( 0x4A5E45, orgConstructRenderList );
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
	InjectHook(0x4349BB, GaragesInit_SilentPatch, HookType::Jump);

	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AED14+1, 1); // katana
	Patch<BYTE>(0x4AEB74+1, 1); // chainsaw

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x418FAE, AutoPilotTimerFix_VC, HookType::Jump);


	// Adblocker
#if DISABLE_FLA_DONATION_WINDOW
	if ( *(DWORD*)0x5FFAE9 != 0x006A026A )
	{
		Patch<DWORD>(0x5FFAE9, 0x006A026A);
		Patch<WORD>(0x5FFAED, 0x006A);
	}
#endif

	Common::Patches::DDraw_VC_10( width, height, aNoDesktopMode );
}

void Patch_VC_11(uint32_t width, uint32_t height)
{
	using namespace Memory;

	PrintString = (void(*)(float,float,const wchar_t*))0x551060;

	RsGlobal = *(RsGlobalType**)0x602D12;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551721;

	InjectHook(0x5433DD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, HookType::Jump);

	InjectHook(0x55171C, SubtitlesShadowFix, HookType::Jump);
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

	// RsMouseSetPos call (SA style fix)
	ReadCall( 0x4A5E65, orgConstructRenderList );
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
	InjectHook(0x4349BB, GaragesInit_SilentPatch, HookType::Jump);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AED34+1, 1); // katana
	Patch<BYTE>(0x4AEB94+1, 1); // chainsaw

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x418FAE, AutoPilotTimerFix_VC, HookType::Jump);

	Common::Patches::DDraw_VC_11( width, height, aNoDesktopMode );
}

void Patch_VC_Steam(uint32_t width, uint32_t height)
{
	using namespace Memory;

	PrintString = (void(*)(float,float,const wchar_t*))0x550F30;

	RsGlobal = *(RsGlobalType**)0x602952;
	RosieAudioFix_JumpBack = (void*)0x42BFCE;
	SubtitlesShadowFix_JumpBack = (void*)0x5515F1;

	InjectHook(0x5432AD, FixedRefValue);

	InjectHook(0x42BFC7, RosiesAudioFix, HookType::Jump);

	InjectHook(0x5515EC, SubtitlesShadowFix, HookType::Jump);
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

	// RsMouseSetPos call (SA style fix)
	ReadCall( 0x4A5D15, orgConstructRenderList );
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
	InjectHook(0x43497B, GaragesInit_SilentPatch, HookType::Jump);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AEA44+1, 1); // katana
	Patch<BYTE>(0x4AEBE4+1, 1); // chainsaw

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x418FAE, AutoPilotTimerFix_VC, HookType::Jump);

	Common::Patches::DDraw_VC_Steam( width, height, aNoDesktopMode );
}

void Patch_VC_JP()
{
	using namespace Memory;

	// Y axis sensitivity fix
	// By ThirteenAG
	Patch<DWORD>(0x4797E7 + 0x2E0 + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47A5E5 + 0x27D + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47B1FE + 0x1CC + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47C266 + 0x22E + 0x2, 0x94ABD8);
	Patch<DWORD>(0x481E8A + 0x4FE + 0x2, 0x94ABD8);
}

void Patch_VC_Common()
{
	using namespace Memory;
	using namespace hook;

	// New timers fix
	{
		auto hookPoint = pattern( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08" ).get_one();
		auto jmpPoint = get_pattern( "DD D8 E9 31 FF FF FF" );

		InjectHook( hookPoint.get<void>( 0x21 ), CTimer::Update_SilentPatch, HookType::Call );
		InjectHook( hookPoint.get<void>( 0x21 + 5 ), jmpPoint, HookType::Jump );
	}

	// Alt+F4
	{
		auto addr = pattern( "59 59 31 C0 83 C4 70 5D 5F 5E 5B C2 10 00" ).count(2);
		auto dest = get_pattern( "53 55 56 FF B4 24 90 00 00 00 FF 15" );

		addr.for_each_result( [&]( pattern_match match ) {
			InjectHook( match.get<void>( 2 ), dest, HookType::Jump );
		});
	}

	// Proper panels damage
	{
		auto addr = pattern( "C6 41 09 03 C6 41 0A 03 C6 41 0B 03" ).get_one();

		// or dword ptr[ecx+14], 3300000h
		// nop
		Patch( addr.get<void>( 0x18 ), { 0x81, 0x49, 0x14, 0x00, 0x00, 0x30, 0x03 }  );
		Nop( addr.get<void>( 0x18 + 7 ), 13 );

		Nop( addr.get<void>( 0x33 ), 7 );
	}

	// Proper metric-imperial conversion constants
	{
		static const float METERS_TO_MILES = 0.0006213711922f;
		auto addr = pattern( "75 ? D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 D9 9C 24" ).count(6);
		addr.for_each_result( [&]( pattern_match match ) {
			Patch<const void*>( match.get<void>( 0x8 + 2 ), &METERS_TO_MILES );
		});

		auto sum = get_pattern( "D9 9C 24 A8 00 00 00 8D 84 24 A8 00 00 00 50", -6 + 2 );
		Patch<const void*>( sum, &METERS_TO_MILES );
	}

	// Improved pathfinding in PickNextNodeAccordingStrategy - PickNextNodeToChaseCar with XYZ coords
	{
		auto addr = pattern( "E8 ? ? ? ? 50 8D 44 24 10 50 E8" ).get_one();
		ReadCall( addr.get<void>( 0x25 ), orgPickNextNodeToChaseCar );

		const uintptr_t funcAddr = (uintptr_t)get_pattern( "8B 9C 24 BC 00 00 00 66 8B B3 A6 01 00 00 66 85 F6", -0xA );
		InjectHook( funcAddr - 5, PickNextNodeToChaseCarXYZ, HookType::Jump ); // For plugin-sdk

		// push PickNextNodeToChaseCarZ instead of 0.0f
		// mov ecx, [PickNextNodeToChaseCarZ]
		// mov [esp+0B8h+var_2C], ecx
		Patch( funcAddr + 0x5D, { 0x8B, 0x0D } );
		Patch<const void*>( funcAddr + 0x5D + 2, &PickNextNodeToChaseCarZ );
		Patch( funcAddr + 0x5D + 6, { 0x89, 0x8C, 0x24, 0x8C, 0x00, 0x00, 0x00 } );

		// lea eax, [ecx+edx*4] -> lea eax, [edx+edx*4]
		Patch<uint8_t>( funcAddr + 0x6E + 2, 0x92 );


		// lea eax, [esp+20h+var_10]
		// push eax
		// nop...
		Patch( addr.get<void>( 0x10 ), { 0x83, 0xC4, 0x04, 0x8D, 0x44, 0x24, 0x10, 0x50, 0xEB, 0x0A } );
		InjectHook( addr.get<void>( 0x25 ), PickNextNodeToChaseCarXYZ );
		Patch<uint8_t>( addr.get<void>( 0x2A + 2 ), 0xC );

		// push edx
		// nop...
		Patch<uint8_t>( addr.get<void>( 0x3E ), 0x52 );
		Nop( addr.get<void>( 0x3E + 1 ), 6 );
		InjectHook( addr.get<void>( 0x46 ), PickNextNodeToChaseCarXYZ );
		Patch<uint8_t>( addr.get<void>( 0x4B + 2 ), 0xC );
	}


	// No censorships
	{
		auto addr = get_pattern( "8B 43 50 85 C0 8B 53 50 74 2B 83 E8 01" );
		Patch( addr, { 0x83, 0xC4, 0x08, 0x5B, 0xC3 } );	// add     esp, 8 \ pop ebx \ retn
	}


	// 014C cargen counter fix (by spaceeinstein)
	{
		auto do_processing = pattern( "0F B7 43 28 83 F8 FF 7D 04 66 FF 4B 28" ).get_one();

		Patch<uint8_t>( do_processing.get<uint8_t*>(1), 0xBF ); // movzx   eax, word ptr [ebx+28h] -> movsx   eax, word ptr [ebx+28h]
		Patch<uint8_t>( do_processing.get<uint8_t*>(7), 0x74 ); // jge -> jz
	}


	// Fixed ammo from SCM
	{
		using namespace ZeroAmmoFix;

		std::array<void*, 2> give_weapon = {
			get_pattern( "6B C0 2E 6A 01 56 8B 3C", 0x15 ),
			get_pattern( "89 F9 6A 01 55 50 E8", 6 ),
		};
		HookEach_GiveWeapon(give_weapon, InterceptCall);
	}


	// Extras working correctly on bikes
	{
		auto createInstance = get_pattern( "89 C1 8B 41 04" );
		InjectHook( createInstance, CreateInstance_BikeFix, HookType::Call );
	}


	// Credits =)
	{
		auto renderCredits = pattern( "8D 44 24 28 83 C4 14 50 FF 35 ? ? ? ? E8 ? ? ? ? 8D 44 24 1C 59 59 50 FF 35 ? ? ? ? E8 ? ? ? ? 59 59" ).get_one();

		ReadCall( renderCredits.get<void>( -50 ), Credits::PrintCreditText );
		ReadCall( renderCredits.get<void>( -5 ), Credits::PrintCreditText_Hooked );
		InjectHook( renderCredits.get<void>( -5 ), Credits::PrintSPCredits );
	}


	// Decreased keyboard input latency
	{
		using namespace KeyboardInputFix;

		auto updatePads = pattern( "66 8B 42 1A" ).get_one();
		void* jmpDest = get_pattern( "66 A3 ? ? ? ? 5F", 6 );
		void* simButtonCheckers = get_pattern( "56 57 B3 01", 0x16 );

		NewKeyState = *updatePads.get<void*>( 0x27 + 1 );
		OldKeyState = *updatePads.get<void*>( 4 + 1 );
		TempKeyState = *updatePads.get<void*>( 0x270 + 1 );

		ReadCall( simButtonCheckers, orgClearSimButtonPressCheckers );
		InjectHook( simButtonCheckers, ClearSimButtonPressCheckers );
		InjectHook( updatePads.get<void>( 9 ), jmpDest, HookType::Jump );
	}


	// Locale based metric/imperial system
	{
		using namespace Localization;

		void* updateCompareFlag = get_pattern( "89 D9 6A 00 E8 ? ? ? ? 30 C0 83 C4 70 5D 5F 5E 5B C2 04 00", 4 );

		ReadCall( updateCompareFlag, orgUpdateCompareFlag_IsMetric );
		InjectHook( updateCompareFlag, UpdateCompareFlag_IsMetric );

		// Stats
		auto constructStatLine = pattern( "85 C0 74 11 83 E8 01 83 F8 03" ).get_one();

		Nop( constructStatLine.get<void>( -11 ), 1 );
		InjectHook( constructStatLine.get<void>( -11 + 1 ), PrefsLanguage_IsMetric, HookType::Call );
		Nop( constructStatLine.get<void>( -2 ), 2 );
	}


	// Corrected FBI Washington sirens sound
	// Primary siren lower pitched like in FBI Rancher and secondary siren higher pitched
	{
		using namespace SirenSwitchingFix;

		// Other mods might be touching it, so only patch specific vehicles if their code has not been touched at all
		auto sirenPitch = pattern( "83 F8 17 74 32" ).count_hint(1);
		if ( sirenPitch.size() == 1 )
		{
			auto match = sirenPitch.get_one();

			InjectHook( match.get<void>( 5 ), IsFBIRanchOrFBICar, HookType::Call );
			Patch( match.get<void>( 5 + 5 ), { 0x84, 0xC0 } ); // test al, al
			Nop( match.get<void>( 5 + 5 + 2 ), 4 );

			// Pitch shift FBI Washington primary siren
			struct tVehicleSampleData {
				int m_nAccelerationSampleIndex;
				char m_bEngineSoundType;
				int m_nHornSample;
				int m_nHornFrequency;
				char m_nSirenOrAlarmSample;
				int m_nSirenOrAlarmFrequency;
				char m_bDoorType;
			};

			tVehicleSampleData* dataTable = *get_pattern<tVehicleSampleData*>( "8B 04 95 ? ? ? ? 89 43 1C", 3 );
			// Only pitch shift if table hasn't been relocated elsewhere
			if ( GetModuleHandle( nullptr ) == ModCompat::Utils::GetModuleHandleFromAddress(dataTable) )
			{
				// fbicar frequency = fbiranch frequency
				dataTable[17].m_nSirenOrAlarmFrequency = dataTable[90].m_nSirenOrAlarmFrequency;
			}
		}
	}


	// Allow extra6 to be picked with component rule 4 (any)
	{
		void* extraMult6 = get_pattern( "D8 0D ? ? ? ? D9 7C 24 04 8B 44 24 04 80 4C 24 05 0C D9 6C 24 04 89 44 24 04 DB 5C 24 08 D9 6C 24 04 8B 44 24 08 83 C4 10 5D", 2 );

		static const float MULT_6 = 6.0f;
		Patch( extraMult6, &MULT_6 );
	}

	
	// Make drive-by one shot sounds owned by the driver instead of the car
	// Fixes incorrect weapon sound being used for drive-by
	{
		auto getDriverOneShot = pattern( "FF 35 ? ? ? ? 6A 37 50 E8 ? ? ? ? 83 7E 08 00" ).get_one();

		// nop
		// mov ecx, ebx
		// call CVehicle::GetOneShotOwnerID
		Patch( getDriverOneShot.get<void>( -8 ), { 0x90, 0x89, 0xD9 } );
		InjectHook( getDriverOneShot.get<void>( -5 ), &CVehicle::GetOneShotOwnerID_SilentPatch, HookType::Call );
	}


	// Fixed vehicles exploding twice if the driver leaves the car while it's exploding
	{
		using namespace RemoveDriverStatusFix;

		auto removeDriver = pattern("8A 43 50 24 07 0C 20 88 43 50 E8").get_one();
		auto processCommands1 = get_pattern("88 42 50 8B 33");
		auto processCommands2 = get_pattern("88 42 50 8B AE");
		auto removeThisPed = get_pattern("88 42 50 8B 85");
		auto pedSetOutCar = get_pattern("0C 20 88 47 50 8B 85", 2);

		Nop(removeDriver.get<void>(), 2);
		InjectHook(removeDriver.get<void>(2), RemoveDriver_SetStatus, HookType::Call);

		// CVehicle::RemoveDriver already sets the status to STATUS_ABANDONED, these are redundant
		Nop(processCommands1, 3);
		Nop(processCommands2, 3);
		Nop(removeThisPed, 3);
		Nop(pedSetOutCar, 3);
	}


	// Apply the environment mapping on extra components
	{
		using namespace EnvMapsOnExtras;

		auto forAllAtomics = pattern("50 E8 ? ? ? ? 66 8B 4B 44").get_one();

		// push eax -> push ebx
		Patch<uint8_t>(forAllAtomics.get<void>(), 0x53);
		InterceptCall(forAllAtomics.get<void>(1), orgRpClumpForAllAtomics, RpClumpForAllAtomics_ExtraComps);
	}


	// Fix probabilities in CVehicle::InflictDamage incorrectly assuming a random range from 0 to 100.000
	{
		auto probability = get_pattern("66 81 7B 5A ? ? 73 50", 4);

		Patch<uint16_t>(probability, 35000u / 2u);
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		hDLLModule = hinstDLL;

		const auto [width, height] = GetDesktopResolution();
		sprintf_s(aNoDesktopMode, "Cannot find %ux%ux32 video mode", width, height);

		// This scope is mandatory so Protect goes out of scope before rwcseg gets fixed
		{
			std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

			const int8_t version = Memory::GetVersion().version;
			if ( version == 0 ) Patch_VC_10(width, height);
			else if ( version == 1 ) Patch_VC_11(width, height);
			else if ( version == 2 ) Patch_VC_Steam(width, height);

			// Y axis sensitivity only
			else if (*(DWORD*)0x601048 == 0x5E5F5D60) Patch_VC_JP();

			Patch_VC_Common();
			Common::Patches::III_VC_Common();
			Common::Patches::DDraw_Common();

			Common::Patches::III_VC_SetDelayedPatchesFunc( InjectDelayedPatches_VC_Common );
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