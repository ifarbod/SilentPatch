#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#include "MemoryMgr.h"

extern "C" HRESULT WINAPI DirectDrawCreateEx(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter)
{
	static HRESULT	(WINAPI *pDirectDrawCreateEx)(GUID FAR*, LPVOID*, REFIID, IUnknown FAR*);
	static bool		bLoaded = false;
	if ( !bLoaded )
	{
		wchar_t		wcSystemPath[MAX_PATH];
		GetSystemDirectoryW(wcSystemPath, MAX_PATH);
		PathAppendW(wcSystemPath, L"ddraw.dll");

		HMODULE		hLib = LoadLibraryW(wcSystemPath);
		pDirectDrawCreateEx = (HRESULT(WINAPI*)(GUID FAR*, LPVOID*, REFIID, IUnknown FAR*))GetProcAddress(hLib, "DirectDrawCreateEx");

		static char		aNoDesktopMode[64];
		using namespace MemoryVP;

		if (*(DWORD*)0x5C1E70 == 0x53E58955)
		{
			// III 1.0
			RECT			desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

			Patch<DWORD>(0x581E5E, desktop.right);
			Patch<DWORD>(0x581E68, desktop.bottom);
			Patch<BYTE>(0x581E72, 32);
			Patch<const char*>(0x581EA8, aNoDesktopMode);

			// No 12mb vram check
			Patch<BYTE>(0x581411, 0xEB);
		}
		else if (*(DWORD*)0x5C2130 == 0x53E58955)
		{
			// III 1.1
			RECT			desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

			Patch<DWORD>(0x58219E, desktop.right);
			Patch<DWORD>(0x5821A8, desktop.bottom);
			Patch<BYTE>(0x5821B2, 32);
			Patch<const char*>(0x5821E8, aNoDesktopMode);

			// No 12mb vram check
			Patch<BYTE>(0x581753, 0xEB);
		}
		else if (*(DWORD*)0x5C6FD0 == 0x53E58955)
		{
			// III Steam
			RECT			desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

			Patch<DWORD>(0x58208E, desktop.right);
			Patch<DWORD>(0x582098, desktop.bottom);
			Patch<BYTE>(0x5820A2, 32);
			Patch<const char*>(0x5820D8, aNoDesktopMode);

			// No 12mb vram check
			Patch<BYTE>(0x581653, 0xEB);
		}
		else if (*(DWORD*)0x667BF0 == 0x53E58955)
		{
			// VC 1.0
			RECT			desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

			Patch<DWORD>(0x600E7E, desktop.right);
			Patch<DWORD>(0x600E88, desktop.bottom);
			Patch<BYTE>(0x600E92, 32);
			Patch<const char*>(0x600EC8, aNoDesktopMode);
		}
		else if (*(DWORD*)0x667C40 == 0x53E58955)
		{
			// VC 1.1
			RECT			desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

			Patch<DWORD>(0x600E9E, desktop.right);
			Patch<DWORD>(0x600EA8, desktop.bottom);
			Patch<BYTE>(0x600EB2, 32);
			Patch<const char*>(0x600EE8, aNoDesktopMode);
		}
		else if (*(DWORD*)0x666BA0 == 0x53E58955)
		{
			// VC Steam
			RECT			desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			sprintf(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

			Patch<DWORD>(0x600ADE, desktop.right);
			Patch<DWORD>(0x600AE8, desktop.bottom);
			Patch<BYTE>(0x600AF2, 32);
			Patch<const char*>(0x600B28, aNoDesktopMode);
		}


		bLoaded = true;
	}
	return pDirectDrawCreateEx(lpGUID, lplpDD, iid, pUnkOuter);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(fdwReason);
	UNREFERENCED_PARAMETER(lpvReserved);

	return TRUE;
}