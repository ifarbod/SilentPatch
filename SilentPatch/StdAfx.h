#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <mmsystem.h>
#include <cassert>
#include <cstdio>
#include <ShlObj.h>
#include <Shlwapi.h>

#include "MemoryMgr.h"

#define DISABLE_FLA_DONATION_WINDOW		0
