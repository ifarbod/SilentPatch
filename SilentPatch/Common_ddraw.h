#pragma once

#define WIN32_LEAN_AND_MEAN

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define NOMINMAX

#include <windows.h>

namespace Common
{
	namespace Patches
	{
		bool FixRwcseg_Patterns();

		void DDraw_III_10( const RECT& desktop, const char* desktopText );
		void DDraw_III_11( const RECT& desktop, const char* desktopText );
		void DDraw_III_Steam( const RECT& desktop, const char* desktopText );

		void DDraw_VC_10( const RECT& desktop, const char* desktopText );
		void DDraw_VC_11( const RECT& desktop, const char* desktopText );
		void DDraw_VC_Steam( const RECT& desktop, const char* desktopText );

		void DDraw_Common();
	}
};