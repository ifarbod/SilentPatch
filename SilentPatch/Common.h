#pragma once

namespace Common
{
	namespace Patches
	{
		void III_VC_DelayedCommon( bool hasDebugMenu, const wchar_t* iniPath );
		void III_VC_Common();
		void III_VC_SetDelayedPatchesFunc( void(*func)() );
	}
};