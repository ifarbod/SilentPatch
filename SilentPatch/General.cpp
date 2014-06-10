#include "StdAfx.h"
#include "General.h"

// Wrappers
WRAPPER bool CalcScreenCoors(const CVector& vecIn, CVector* vecOut) { WRAPARG(vecIn); WRAPARG(vecOut); EAXJMP(0x71DAB0); }
WRAPPER void LoadingScreenLoadingFile(const char* pText) { WRAPARG(pText); EAXJMP(0x5B3680); }

WRAPPER void CEntity::UpdateRW() { EAXJMP(0x446F90); }
WRAPPER void CEntity::RegisterReference(CEntity** pAddress) { WRAPARG(pAddress); EAXJMP(0x571B70); }
WRAPPER void CEntity::CleanUpOldReference(CEntity** pAddress) { WRAPARG(pAddress); EAXJMP(0x571A00); }
WRAPPER void CEntity::Render() { EAXJMP(0x534310); }
