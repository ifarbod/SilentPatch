#include "Common.h"

#include <Shlwapi.h>
#include <ShlObj.h>
#include "MemoryMgr.h"
#include "Patterns.h"

#pragma comment(lib, "shlwapi.lib")

extern char** ppUserFilesDir;

namespace Common {
	char* GetMyDocumentsPath()
	{
		static char	cUserFilesPath[MAX_PATH];

		if ( cUserFilesPath[0] == '\0' )
		{	
			SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, cUserFilesPath);
			PathAppendA(cUserFilesPath, *ppUserFilesDir);
			CreateDirectoryA(cUserFilesPath, nullptr);
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
		void DDraw_III_10( const RECT& desktop, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x580BB0, GetMyDocumentsPath, PATCH_JUMP);

			Patch<DWORD>(0x581E5E, desktop.right);
			Patch<DWORD>(0x581E68, desktop.bottom);
			Patch<BYTE>(0x581E72, 32);
			Patch<const char*>(0x581EA8, desktopText);

			// No 12mb vram check
			Patch<BYTE>(0x581411, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x5812D6, 0xB8);
			Patch<DWORD>(0x5812D7, 0x900);
		}

		void DDraw_III_11( const RECT& desktop, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x580F00, GetMyDocumentsPath, PATCH_JUMP);

			Patch<DWORD>(0x58219E, desktop.right);
			Patch<DWORD>(0x5821A8, desktop.bottom);
			Patch<BYTE>(0x5821B2, 32);
			Patch<const char*>(0x5821E8, desktopText);

			// No 12mb vram check
			Patch<BYTE>(0x581753, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x581620, 0xB8);
			Patch<DWORD>(0x581621, 0x900);
		}

		void DDraw_III_Steam( const RECT& desktop, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x580E00, GetMyDocumentsPath, PATCH_JUMP);

			Patch<DWORD>(0x58208E, desktop.right);
			Patch<DWORD>(0x582098, desktop.bottom);
			Patch<BYTE>(0x5820A2, 32);
			Patch<const char*>(0x5820D8, desktopText);

			// No 12mb vram check
			Patch<BYTE>(0x581653, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x581520, 0xB8);
			Patch<DWORD>(0x581521, 0x900);
		}

		// ================= VC =================
		void DDraw_VC_10( const RECT& desktop, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x602240, GetMyDocumentsPath, PATCH_JUMP);

			InjectHook(0x601A40, GetMyDocumentsPath, PATCH_CALL);
			InjectHook(0x601A45, 0x601B2F, PATCH_JUMP);

			Patch<DWORD>(0x600E7E, desktop.right);
			Patch<DWORD>(0x600E88, desktop.bottom);
			Patch<BYTE>(0x600E92, 32);
			Patch<const char*>(0x600EC8, desktopText);

			// No 12mb vram check
			Patch<BYTE>(0x601E26, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x601CA0, 0xB8);
			Patch<DWORD>(0x601CA1, 0x900);
		}

		void DDraw_VC_11( const RECT& desktop, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x602220, GetMyDocumentsPath, PATCH_JUMP);

			InjectHook(0x601A70, GetMyDocumentsPath, PATCH_CALL);
			InjectHook(0x601A75, 0x601B5F, PATCH_JUMP);

			Patch<DWORD>(0x600E9E, desktop.right);
			Patch<DWORD>(0x600EA8, desktop.bottom);
			Patch<BYTE>(0x600EB2, 32);
			Patch<const char*>(0x600EE8, desktopText);

			// No 12mb vram check
			Patch<BYTE>(0x601E56, 0xEB);

			// No DirectPlay dependency
			Patch<BYTE>(0x601CD0, 0xB8);
			Patch<DWORD>(0x601CD1, 0x900);
		}


		void DDraw_VC_Steam( const RECT& desktop, const char* desktopText )
		{
			using namespace Memory;

			InjectHook(0x601E60, GetMyDocumentsPath, PATCH_JUMP);

			InjectHook(0x6016B0, GetMyDocumentsPath, PATCH_CALL);
			InjectHook(0x6016B5, 0x60179F, PATCH_JUMP);

			Patch<DWORD>(0x600ADE, desktop.right);
			Patch<DWORD>(0x600AE8, desktop.bottom);
			Patch<BYTE>(0x600AF2, 32);
			Patch<const char*>(0x600B28, desktopText);

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
		}
	}
}