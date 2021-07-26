#include "Common_ddraw.h"

#define WIN32_LEAN_AND_MEAN

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define NOMINMAX

#include <windows.h>

#include <Shlwapi.h>
#include <ShlObj.h>
#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"

#pragma comment(lib, "shlwapi.lib")

extern char** ppUserFilesDir;

namespace Common {
	char* GetMyDocumentsPath()
	{
		static char	cUserFilesPath[MAX_PATH];

		if ( cUserFilesPath[0] == '\0' )
		{	
			if ( SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, cUserFilesPath) == S_OK )
			{
				PathAppendA(cUserFilesPath, *ppUserFilesDir);
				CreateDirectoryA(cUserFilesPath, nullptr);
			}
			else
			{
				strcpy_s(cUserFilesPath, "data");
			}
		}
		return cUserFilesPath;
	}

	namespace Patches {

		bool FixRwcseg_Patterns()
		{
			using namespace hook;

			auto begin = pattern( "55 8B EC 50 53 51 52 8B 5D 14 8B 4D 10 8B 45 0C 8B 55 08" );
			auto end = pattern( "9B D9 3D ? ? ? ? 81 25 ? ? ? ? FF FC FF FF 83 0D ? ? ? ? 3F" );

			if ( begin.count_hint(1).size() == 1 && end.count_hint(1).size() == 1 )
			{
				const ptrdiff_t size = (intptr_t)end.get_first( 24 ) - (intptr_t)begin.get_first();
				if ( size > 0 )
				{
					DWORD dwProtect;
					VirtualProtect( begin.get_first(), size, PAGE_EXECUTE_READ, &dwProtect );
					return true;
				}
			}
			return false;
		}

		// ================= III =================
		void DDraw_III_10( uint32_t width, uint32_t height, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x580BB0, GetMyDocumentsPath, PATCH_JUMP);

			if (width != 0 && height != 0)
			{
				Patch<DWORD>(0x581E5E, width);
				Patch<DWORD>(0x581E68, height);
				Patch<const char*>(0x581EA8, desktopText);
			}
			Patch<BYTE>(0x581E72, 32);

			// No 12mb vram check
			Patch<BYTE>(0x581411, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x5812D6, 0xB8);
			Patch<DWORD>(0x5812D7, 0x900);
		}

		void DDraw_III_11( uint32_t width, uint32_t height, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x580F00, GetMyDocumentsPath, PATCH_JUMP);

			if (width != 0 && height != 0)
			{
				Patch<DWORD>(0x58219E, width);
				Patch<DWORD>(0x5821A8, height);
				Patch<const char*>(0x5821E8, desktopText);
			}
			Patch<BYTE>(0x5821B2, 32);

			// No 12mb vram check
			Patch<BYTE>(0x581753, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x581620, 0xB8);
			Patch<DWORD>(0x581621, 0x900);
		}

		void DDraw_III_Steam( uint32_t width, uint32_t height, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x580E00, GetMyDocumentsPath, PATCH_JUMP);

			if (width != 0 && height != 0)
			{
				Patch<DWORD>(0x58208E, width);
				Patch<DWORD>(0x582098, height);
				Patch<const char*>(0x5820D8, desktopText);
			}
			Patch<BYTE>(0x5820A2, 32);

			// No 12mb vram check
			Patch<BYTE>(0x581653, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x581520, 0xB8);
			Patch<DWORD>(0x581521, 0x900);
		}

		// ================= VC =================
		void DDraw_VC_10( uint32_t width, uint32_t height, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x602240, GetMyDocumentsPath, PATCH_JUMP);

			InjectHook(0x601A40, GetMyDocumentsPath, PATCH_CALL);
			InjectHook(0x601A45, 0x601B2F, PATCH_JUMP);

			if (width != 0 && height != 0)
			{
				Patch<DWORD>(0x600E7E, width);
				Patch<DWORD>(0x600E88, height);
				Patch<const char*>(0x600EC8, desktopText);
			}
			Patch<BYTE>(0x600E92, 32);

			// No 12mb vram check
			Patch<BYTE>(0x601E26, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x601CA0, 0xB8);
			Patch<DWORD>(0x601CA1, 0x900);
		}

		void DDraw_VC_11( uint32_t width, uint32_t height, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x602220, GetMyDocumentsPath, PATCH_JUMP);

			InjectHook(0x601A70, GetMyDocumentsPath, PATCH_CALL);
			InjectHook(0x601A75, 0x601B5F, PATCH_JUMP);

			if (width != 0 && height != 0)
			{
				Patch<DWORD>(0x600E9E, width);
				Patch<DWORD>(0x600EA8, height);
				Patch<const char*>(0x600EE8, desktopText);
			}
			Patch<BYTE>(0x600EB2, 32);

			// No 12mb vram check
			Patch<BYTE>(0x601E56, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x601CD0, 0xB8);
			Patch<DWORD>(0x601CD1, 0x900);
		}


		void DDraw_VC_Steam( uint32_t width, uint32_t height, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x601E60, GetMyDocumentsPath, PATCH_JUMP);

			InjectHook(0x6016B0, GetMyDocumentsPath, PATCH_CALL);
			InjectHook(0x6016B5, 0x60179F, PATCH_JUMP);

			if (width != 0 && height != 0)
			{
				Patch<DWORD>(0x600ADE, width);
				Patch<DWORD>(0x600AE8, height);
				Patch<const char*>(0x600B28, desktopText);
			}
			Patch<BYTE>(0x600AF2, 32);

			// No 12mb vram check
			Patch<BYTE>(0x601A96, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x601910, 0xB8);
			Patch<DWORD>(0x601911, 0x900);
		}

		// ================= COMMON =================
		void DDraw_Common()
		{
			using namespace Memory;
			using namespace hook;

			// Remove FILE_FLAG_NO_BUFFERING from CdStreams
			{
				auto mem = pattern( "81 7C 24 04 00 08 00 00" ).count_hint(1);
				if ( mem.size() == 1 )
				{
					Patch<uint8_t>( mem.get_first( 0x12 ), 0xEB );
				}
			}


			// No censorships
			{
				auto addr = pattern( "83 FB 07 74 0A 83 FD 07 74 05 83 FE 07 75 15" ).count_hint(1);
				if ( addr.size() == 1 )
				{
					Patch( addr.get_first(), { 0xEB, 0x5E } );
				}
			
			}

			// unnamed CdStream semaphore
			{
				auto mem = pattern( "8D 04 85 00 00 00 00 50 6A 40 FF 15" ).count_hint(1);
				if ( mem.size() == 1 )
				{
					Patch( mem.get_first( 0x25 ), { 0x6A, 0x00 } ); // push 0 \ nop
					Nop( mem.get_first( 0x25 + 2 ), 3 );
				}
			
			}
		}
	}
}