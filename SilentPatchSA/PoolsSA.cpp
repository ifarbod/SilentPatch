#include "StdAfxSA.h"
#include "PoolsSA.h"

CObjectPool*&				CPools::ms_pObjectPool = **AddressByVersion<CObjectPool***>(0x5A18B2 + 2, 0, 0); // TODO