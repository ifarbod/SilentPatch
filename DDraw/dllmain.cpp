#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <cstdint>
#include "MemoryMgr.h"

extern "C" HRESULT WINAPI DirectDrawCreateEx(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter)
{
	static HRESULT	(WINAPI *pDirectDrawCreateEx)(GUID FAR*, LPVOID*, REFIID, IUnknown FAR*);
	if ( pDirectDrawCreateEx == nullptr )
	{
		wchar_t		wcSystemPath[MAX_PATH];
		GetSystemDirectoryW(wcSystemPath, MAX_PATH);
		PathAppendW(wcSystemPath, L"ddraw.dll");

		HMODULE		hLib = LoadLibraryW(wcSystemPath);
		pDirectDrawCreateEx = (HRESULT(WINAPI*)(GUID FAR*, LPVOID*, REFIID, IUnknown FAR*))GetProcAddress(hLib, "DirectDrawCreateEx");
	}
	return pDirectDrawCreateEx(lpGUID, lplpDD, iid, pUnkOuter);
}

char** ppUserFilesDir;

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


void InjectHooks()
{
	using namespace MemoryVP;

	static char		aNoDesktopMode[64];

	RECT			desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

	if (*(DWORD*)0x5C1E75 == 0xB85548EC)
	{
		// III 1.0
		ppUserFilesDir = (char**)0x580C16;
		InjectHook(0x580BB0, GetMyDocumentsPath, PATCH_JUMP);

		Patch<DWORD>(0x581E5E, desktop.right);
		Patch<DWORD>(0x581E68, desktop.bottom);
		Patch<BYTE>(0x581E72, 32);
		Patch<const char*>(0x581EA8, aNoDesktopMode);

		// No 12mb vram check
		Patch<BYTE>(0x581411, 0xEB);

		// No DirectPlay dependency
		Patch<BYTE>(0x5812D6, 0xB8);
		Patch<DWORD>(0x5812D7, 0x900);
	}
	else if (*(DWORD*)0x5C2135 == 0xB85548EC)
	{
		// III 1.1
		ppUserFilesDir = (char**)0x580F66;
		InjectHook(0x580F00, GetMyDocumentsPath, PATCH_JUMP);

		Patch<DWORD>(0x58219E, desktop.right);
		Patch<DWORD>(0x5821A8, desktop.bottom);
		Patch<BYTE>(0x5821B2, 32);
		Patch<const char*>(0x5821E8, aNoDesktopMode);

		// No 12mb vram check
		Patch<BYTE>(0x581753, 0xEB);

		// No DirectPlay dependency
		Patch<BYTE>(0x581620, 0xB8);
		Patch<DWORD>(0x581621, 0x900);
	}
	else if (*(DWORD*)0x5C6FD5 == 0xB85548EC)
	{
		// III Steam
		ppUserFilesDir = (char**)0x580E66;
		InjectHook(0x580E00, GetMyDocumentsPath, PATCH_JUMP);

		Patch<DWORD>(0x58208E, desktop.right);
		Patch<DWORD>(0x582098, desktop.bottom);
		Patch<BYTE>(0x5820A2, 32);
		Patch<const char*>(0x5820D8, aNoDesktopMode);

		// No 12mb vram check
		Patch<BYTE>(0x581653, 0xEB);

		// No DirectPlay dependency
		Patch<BYTE>(0x581520, 0xB8);
		Patch<DWORD>(0x581521, 0x900);
	}

	else if (*(DWORD*)0x667BF5 == 0xB85548EC)
	{
		// VC 1.0
		ppUserFilesDir = (char**)0x6022AA;
		InjectHook(0x602240, GetMyDocumentsPath, PATCH_JUMP);

		InjectHook(0x601A40, GetMyDocumentsPath, PATCH_CALL);
		InjectHook(0x601A45, 0x601B2F, PATCH_JUMP);

		Patch<DWORD>(0x600E7E, desktop.right);
		Patch<DWORD>(0x600E88, desktop.bottom);
		Patch<BYTE>(0x600E92, 32);
		Patch<const char*>(0x600EC8, aNoDesktopMode);

		// No 12mb vram check
		Patch<BYTE>(0x601E26, 0xEB);

		// No DirectPlay dependency
		Patch<BYTE>(0x601CA0, 0xB8);
		Patch<DWORD>(0x601CA1, 0x900);
	}
	else if (*(DWORD*)0x667C45 == 0xB85548EC)
	{
		// VC 1.1
		ppUserFilesDir = (char**)0x60228A;
		InjectHook(0x602220, GetMyDocumentsPath, PATCH_JUMP);

		InjectHook(0x601A70, GetMyDocumentsPath, PATCH_CALL);
		InjectHook(0x601A75, 0x601B5F, PATCH_JUMP);

		Patch<DWORD>(0x600E9E, desktop.right);
		Patch<DWORD>(0x600EA8, desktop.bottom);
		Patch<BYTE>(0x600EB2, 32);
		Patch<const char*>(0x600EE8, aNoDesktopMode);

		// No 12mb vram check
		Patch<BYTE>(0x601E56, 0xEB);

		// No DirectPlay dependency
		Patch<BYTE>(0x601CD0, 0xB8);
		Patch<DWORD>(0x601CD1, 0x900);
	}
	else if (*(DWORD*)0x666BA5 == 0xB85548EC)
	{
		// VC Steam
		ppUserFilesDir = (char**)0x601ECA;
		InjectHook(0x601E60, GetMyDocumentsPath, PATCH_JUMP);

		InjectHook(0x6016B0, GetMyDocumentsPath, PATCH_CALL);
		InjectHook(0x6016B5, 0x60179F, PATCH_JUMP);

		Patch<DWORD>(0x600ADE, desktop.right);
		Patch<DWORD>(0x600AE8, desktop.bottom);
		Patch<BYTE>(0x600AF2, 32);
		Patch<const char*>(0x600B28, aNoDesktopMode);

		// No 12mb vram check
		Patch<BYTE>(0x601A96, 0xEB);

		// No DirectPlay dependency
		Patch<BYTE>(0x601910, 0xB8);
		Patch<DWORD>(0x601911, 0x900);
	}
}


static VOID (WINAPI* pOrgGetStartupInfoA)(LPSTARTUPINFOA);
VOID WINAPI GetStartupInfoA_Hook(LPSTARTUPINFOA lpStartupInfo)
{
	static bool		bPatched = false;
	if ( !bPatched )
	{
		using namespace MemoryVP;

		bPatched = true;

		InjectHooks();
	}
	pOrgGetStartupInfoA(lpStartupInfo);
}

static HINSTANCE hInstance;
void PatchIAT()
{
	IMAGE_NT_HEADERS*			ntHeader = (IMAGE_NT_HEADERS*)((DWORD)hInstance + ((IMAGE_DOS_HEADER*)hInstance)->e_lfanew);

	// Give _rwcseg proper access rights
	WORD					NumberOfSections = ntHeader->FileHeader.NumberOfSections;
	IMAGE_SECTION_HEADER*	pSection = IMAGE_FIRST_SECTION(ntHeader);

	for ( WORD i = 0; i < NumberOfSections; i++, pSection++ )
	{
		if ( *(uint64_t*)(pSection->Name) == 0x006765736377725F )	// _rwcseg
		{
			DWORD	dwProtect;
			VirtualProtect((LPVOID)((ptrdiff_t)hInstance + pSection->VirtualAddress), pSection->Misc.VirtualSize, PAGE_EXECUTE_READ, &dwProtect);
			break;
		}
	}

	// Find IAT	
	IMAGE_IMPORT_DESCRIPTOR*	pImports = (IMAGE_IMPORT_DESCRIPTOR*)((DWORD)hInstance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	// Find kernel32.dll
	for ( ; pImports->Name != 0; pImports++ )
	{
		if ( !_stricmp((const char*)((DWORD)hInstance + pImports->Name), "KERNEL32.DLL") )
		{
			IMAGE_IMPORT_BY_NAME**		pFunctions = (IMAGE_IMPORT_BY_NAME**)((DWORD)hInstance + pImports->OriginalFirstThunk);

			// kernel32.dll found, find GetStartupInfoA
			for ( ptrdiff_t j = 0; pFunctions[j] != nullptr; j++ )
			{
				if ( !strcmp((const char*)((DWORD)hInstance + pFunctions[j]->Name), "GetStartupInfoA") )
				{
					// Overwrite the address with the address to a custom GetStartupInfoA
					DWORD			dwProtect[2];
					DWORD_PTR*		pAddress = &((DWORD_PTR*)((DWORD)hInstance + pImports->FirstThunk))[j];

					VirtualProtect(pAddress, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
					pOrgGetStartupInfoA = **(VOID(WINAPI**)(LPSTARTUPINFOA))pAddress;
					*pAddress = (DWORD)GetStartupInfoA_Hook;
					VirtualProtect(pAddress, sizeof(DWORD), dwProtect[0], &dwProtect[1]);

					return;
				}
			}
		}
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		DisableThreadLibraryCalls(hinstDLL);

		hInstance = GetModuleHandle(nullptr);
		PatchIAT();
	}

	return TRUE;
}