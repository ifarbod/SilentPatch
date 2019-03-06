#include "StdAfxSA.h"
#include <limits>
#include <algorithm>
#include <d3d9.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <ShellAPI.h>
#include <cinttypes>

#include "ScriptSA.h"
#include "GeneralSA.h"
#include "ModelInfoSA.h"
#include "VehicleSA.h"
#include "PedSA.h"
#include "AudioHardwareSA.h"
#include "LinkListSA.h"
#include "PNGFile.h"
#include "PlayerInfoSA.h"
#include "FireManagerSA.h"

#include "WaveDecoderSA.h"
#include "FLACDecoderSA.h"

#include "Patterns.h"
#include "DelimStringReader.h"
#include "ModuleList.hpp"

#include "debugmenu_public.h"
#include "resource.h"

// ============= Mod compatibility stuff =============

namespace ModCompat
{
	bool SkygfxPatchesMoonphases( HMODULE module )
	{
		if ( module == nullptr ) return false; // SkyGfx not installed

		struct Config
		{
			uint32_t version;
			// The rest isn't relevant at the moment
		};

		auto func = (Config*(*)())GetProcAddress( module, "GetConfig" );
		if ( func == nullptr ) return false; // Old version?

		const Config* config = func();
		if ( config == nullptr ) return false; // Old version/error?

		constexpr uint32_t SKYGFX_VERSION_WITH_MOONPHASES = 0x360;
		return config->version >= SKYGFX_VERSION_WITH_MOONPHASES;
	}

	bool bCdStreamFallBackForOldML = false;
	bool ModloaderCdStreamRaceConditionAware( HMODULE module )
	{
		if ( module == nullptr ) return false; // modloader not installed

		HMODULE stdStreamModule;
		if ( GetModuleHandleEx( 0, TEXT("std.stream.dll"), &stdStreamModule ) == 0 ) return false; // std.data not loaded

		// ML is installed, so if it's an old version we need to fall back to a less safe implementation (no condition variables)
		bCdStreamFallBackForOldML = true;

		bool aware = false;
		const auto func = (uint32_t(*)())GetProcAddress( stdStreamModule, "CdStreamRaceConditionAware" );
		if ( func != nullptr )
		{
			aware = func() >= 1;
		}
		FreeLibrary( stdStreamModule );
		return aware;
	}

	bool LSRPMode = false;
	void DetectPlayingOnLSRP()
	{
		struct IPv4
		{
			uint8_t ip[4];
			uint16_t port;

			bool operator== ( const IPv4& right )
			{
				return std::make_tuple( this->ip[0], this->ip[1], this->ip[2], this->ip[3], this->port ) == std::make_tuple( right.ip[0], right.ip[1], right.ip[2], right.ip[3], right.port );
			}
		};

		constexpr IPv4 LSRP = { 149, 56, 123, 148, 7777 };

		IPv4 myIP = {};

		// Obtain IP and check if it's LS-RP
		int numArgs = 0;
		LPWSTR* cmdLine = CommandLineToArgvW( GetCommandLineW(), &numArgs );
		if ( cmdLine != nullptr )
		{
			for ( auto it = cmdLine + 1, end = cmdLine + numArgs; it != end; ++it )
			{
				if ( _wcsicmp( *it, L"-h" ) == 0 )
				{
					auto ipIt = std::next( it );
					if ( ipIt != end )
					{
						swscanf_s( *ipIt, L"%" SCNu8 ".%" SCNu8 ".%" SCNu8 ".%" SCNu8, &myIP.ip[0], &myIP.ip[1], &myIP.ip[2], &myIP.ip[3] );
						it = ipIt;
					}
					continue;
				}

				if ( _wcsicmp( *it, L"-p") == 0 )
				{
					auto portIt = std::next( it );
					if ( portIt != end )
					{
						swscanf_s( *portIt, L"%" SCNu16 , &myIP.port );
						it = portIt;
					}
					continue;
				}
			}

			LocalFree( cmdLine );
		}

		LSRPMode = myIP == LSRP;
	}
}

#pragma warning(disable:4733)

// RW wrappers
static void* varAtomicDefaultRenderCallBack = AddressByVersion<void*>(0x7491C0, 0x749AD0, 0x783180);
WRAPPER RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic) { WRAPARG(atomic); VARJMP(varAtomicDefaultRenderCallBack); }
static void* varRtPNGImageRead = AddressByVersion<void*>(0x7CF9B0, 0x7D02B0, 0x809970);
WRAPPER RwImage* RtPNGImageRead(const RwChar* imageName) { WRAPARG(imageName); VARJMP(varRtPNGImageRead); }
static void* varRwTextureCreate = AddressByVersion<void*>(0x7F37C0, 0x7F40C0, 0x82D780);
WRAPPER RwTexture* RwTextureCreate(RwRaster* raster) { WRAPARG(raster); VARJMP(varRwTextureCreate); }
static void* varRwRasterCreate = AddressByVersion<void*>(0x7FB230, 0x7FBB30, 0x8351F0, 0x82FA80, 0x82F950);
WRAPPER RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) { WRAPARG(width); WRAPARG(height); WRAPARG(depth); WRAPARG(flags); VARJMP(varRwRasterCreate); }
static void* varRwImageDestroy = AddressByVersion<void*>(0x802740, 0x803040, 0x83C700);
WRAPPER RwBool RwImageDestroy(RwImage* image) { WRAPARG(image); VARJMP(varRwImageDestroy); }
static void* varRpMaterialSetTexture = AddressByVersion<void*>(0x74DBC0, 0x74E4D0, 0x787B80);
WRAPPER RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture) { VARJMP(varRpMaterialSetTexture); }
static void* varRwFrameGetLTM = AddressByVersion<void*>(0x7F0990, 0x7F1290, 0x82A950);
WRAPPER RwMatrix* RwFrameGetLTM(RwFrame* frame) { VARJMP(varRwFrameGetLTM); }
static void* varRwMatrixRotate = AddressByVersion<void*>(0x7F1FD0, 0x7F28D0, 0x82BF90);
WRAPPER RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp) { WRAPARG(matrix); WRAPARG(axis); WRAPARG(angle); WRAPARG(combineOp); VARJMP(varRwMatrixRotate); }
static void* varRwD3D9SetRenderState = AddressByVersion<void*>(0x7FC2D0, 0x7FCBD0, 0x836290);
WRAPPER void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) { WRAPARG(state); WRAPARG(value); VARJMP(varRwD3D9SetRenderState); }

RwCamera* RwCameraBeginUpdate(RwCamera* camera)
{
	return camera->beginUpdate(camera);
}

RwCamera* RwCameraEndUpdate(RwCamera* camera)
{
	return camera->endUpdate(camera);
}

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, RwInt32 clearMode)
{
	return RWSRCGLOBAL(stdFunc[rwSTANDARDCAMERACLEAR])(camera, colour, clearMode) != FALSE ? camera : NULL;
}

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, RwOpCombineType combineOp)
{
	if ( combineOp == rwCOMBINEREPLACE )
	{
		RwMatrixSetIdentity(matrix);
		matrix->pos = *translation;
	}
	else if ( combineOp == rwCOMBINEPRECONCAT )
	{
		matrix->pos.x += matrix->at.x * translation->z + matrix->up.x * translation->y + matrix->right.x * translation->x;
		matrix->pos.y += matrix->at.y * translation->z + matrix->up.y * translation->y + matrix->right.y * translation->x;
		matrix->pos.z += matrix->at.z * translation->z + matrix->up.z * translation->y + matrix->right.z * translation->x;	
	}
	else if ( combineOp == rwCOMBINEPOSTCONCAT )
	{
		matrix->pos.x += translation->x;
		matrix->pos.y += translation->y;
		matrix->pos.z += translation->z;
	}
	rwMatrixSetFlags(matrix, rwMatrixGetFlags(matrix) & ~(rwMATRIXINTERNALIDENTITY));
	return matrix;
}

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callBack, void* data)
{
	for ( RwFrame* curFrame = frame->child; curFrame != nullptr; curFrame = curFrame->next )
	{
		if ( callBack(curFrame, data) == NULL )
			break;
	}
	return frame;
}

RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callBack, void* data)
{
	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&frame->objectList); link != rwLinkListGetTerminator(&frame->objectList); link = rwLLLinkGetNext(link) )
	{
		if ( callBack(&rwLLLinkGetData(link, RwObjectHasFrame, lFrame)->object, data) == NULL )
			break;
	}

	return frame;
}

RwFrame* RwFrameUpdateObjects(RwFrame* frame)
{
	if ( !rwObjectTestPrivateFlags(&frame->root->object, rwFRAMEPRIVATEHIERARCHYSYNCLTM|rwFRAMEPRIVATEHIERARCHYSYNCOBJ) )
		rwLinkListAddLLLink(&RWSRCGLOBAL(dirtyFrameList), &frame->root->inDirtyListLink);

	rwObjectSetPrivateFlags(&frame->root->object, rwObjectGetPrivateFlags(&frame->root->object) | (rwFRAMEPRIVATEHIERARCHYSYNCLTM|rwFRAMEPRIVATEHIERARCHYSYNCOBJ));
	rwObjectSetPrivateFlags(&frame->object, rwObjectGetPrivateFlags(&frame->object) | (rwFRAMEPRIVATESUBTREESYNCLTM|rwFRAMEPRIVATESUBTREESYNCOBJ));
	return frame;
}

RwMatrix* RwMatrixUpdate(RwMatrix* matrix)
{
	matrix->flags &= ~(rwMATRIXTYPEMASK|rwMATRIXINTERNALIDENTITY);
	return matrix;
}

RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image)
{
	if ( RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERSETIMAGE])(raster, image, 0) != FALSE )
	{
		if ( image->flags & rwIMAGEGAMMACORRECTED )
			raster->privateFlags |= rwRASTERGAMMACORRECTED;
		return raster;
	}
	return NULL;
}

RwImage* RwImageFindRasterFormat(RwImage* ipImage, RwInt32 nRasterType, RwInt32* npWidth, RwInt32* npHeight, RwInt32* npDepth, RwInt32* npFormat)
{
	RwRaster	outRaster;
	if ( RWSRCGLOBAL(stdFunc[rwSTANDARDIMAGEFINDRASTERFORMAT])(&outRaster, ipImage, nRasterType) != FALSE )
	{
		*npFormat = RwRasterGetFormat(&outRaster) | outRaster.cType;
		*npWidth = RwRasterGetWidth(&outRaster);
		*npHeight = RwRasterGetHeight(&outRaster);
		*npDepth = RwRasterGetDepth(&outRaster);
		return ipImage;
	}
	return NULL;
}

RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* pData)
{
	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&clump->atomicList); link != rwLinkListGetTerminator(&clump->atomicList); link = rwLLLinkGetNext(link) )
	{
		if ( callback(rwLLLinkGetData(link, RpAtomic, inClumpLink), pData) == NULL )
			break;
	}
	return clump;
}

RpClump* RpClumpRender(RpClump* clump)
{
	RpClump*	retClump = clump;

	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&clump->atomicList); link != rwLinkListGetTerminator(&clump->atomicList); link = rwLLLinkGetNext(link) )
	{
		RpAtomic* curAtomic = rwLLLinkGetData(link, RpAtomic, inClumpLink);
		if ( RpAtomicGetFlags(curAtomic) & rpATOMICRENDER )
		{
			// Not sure why they need this
			RwFrameGetLTM(RpAtomicGetFrame(curAtomic));
			if ( RpAtomicRender(curAtomic) == NULL )
				retClump = NULL;
		}
	}
	return retClump;
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData)
{
	for ( RwInt32 i = 0, j = geometry->matList.numMaterials; i < j; i++ )
	{
		if ( fpCallBack(geometry->matList.materials[i], pData) == NULL )
			break;
	}
	return geometry;
}

RwInt32 RpHAnimIDGetIndex(RpHAnimHierarchy* hierarchy, RwInt32 ID)
{
	for ( RwInt32 i = 0, j = hierarchy->numNodes; i < j; i++ )
	{
		if ( ID == hierarchy->pNodeInfo[i].nodeID )
			return i;
	}
	return -1;
}

RwMatrix* RpHAnimHierarchyGetMatrixArray(RpHAnimHierarchy* hierarchy)
{
	return hierarchy->pMatrixArray;
}

void RwD3D9DeleteVertexShader(void* shader)
{
	static_cast<IUnknown*>(shader)->Release();
}

RwBool _rpD3D9VertexDeclarationInstColor(RwUInt8* mem, const RwRGBA* color, RwInt32 numVerts, RwUInt32 stride)
{
	RwUInt8 alpha = 255;
	for ( RwInt32 i = 0; i < numVerts; i++ )
	{
		*reinterpret_cast<RwUInt32*>(mem) = (color->alpha << 24) | (color->red << 16) | (color->green << 8) | color->blue;
		alpha &= color->alpha;
		color++;
		mem += stride;
	}
	return alpha != 255;
}

// Unreachable stub
RwBool RwMatrixDestroy(RwMatrix* mpMat) { assert(!"Unreachable!"); return TRUE; }

struct AlphaObjectInfo
{
	RpAtomic*	pAtomic;
	RpAtomic*	(*callback)(RpAtomic*, float);
	float		fCompareValue;

	friend bool operator < (const AlphaObjectInfo &a, const AlphaObjectInfo &b)
	{ return a.fCompareValue < b.fCompareValue; }
};

// Other wrappers
void					(*GTAdelete)(void*) = AddressByVersion<void(*)(void*)>(0x82413F, 0x824EFF, 0x85E58C);
const char*				(*GetFrameNodeName)(RwFrame*) = AddressByVersion<const char*(*)(RwFrame*)>(0x72FB30, 0x730360, 0x769C20);
RpHAnimHierarchy*		(*GetAnimHierarchyFromSkinClump)(RpClump*) = AddressByVersion<RpHAnimHierarchy*(*)(RpClump*)>(0x734A40, 0x735270, 0x7671B0);	
auto					InitializeUtrax = AddressByVersion<void(__thiscall*)(void*)>(0x4F35B0, 0x4F3A10, 0x4FFA80);

auto					RenderOneXLUSprite = AddressByVersion<void(*)(float, float, float, float, float, uint8_t, uint8_t, uint8_t, int16_t, float, uint8_t, uint8_t, uint8_t)>(0x70D000, 0x70D830, 0x7592C0);

static void				(__thiscall* SetVolume)(void*,float);	
static BOOL				(*IsAlreadyRunning)();
static void				(*TheScriptsLoad)();
static void				(*WipeLocalVariableMemoryForMissionScript)();
static void				(*DoSunAndMoon)();

auto 					WorldRemove = AddressByVersion<void(*)(CEntity*)>(0x563280, 0, 0x57D370, 0x57C480, 0x57C3B0);


// SA variables
void**					rwengine = *AddressByVersion<void***>(0x58FFC0, 0x53F032, 0x48C194, 0x48B167, 0x48B167);

unsigned char&			nGameClockDays = **AddressByVersion<unsigned char**>(0x4E841D, 0x4E886D, 0x4F3871);
unsigned char&			nGameClockMonths = **AddressByVersion<unsigned char**>(0x4E842D, 0x4E887D, 0x4F3861);
void*&					pUserTracksStuff = **AddressByVersion<void***>(0x4D9B7B, 0x4DA06C, 0x4E4A43);

float&					fFarClipZ = **AddressByVersion<float**>(0x70D21F, 0x70DA4F, 0x421AB2);
RwTexture** const		gpCoronaTexture = *AddressByVersion<RwTexture***>(0x6FAA8C, 0x6FB2BC, 0x5480BF);
int&					MoonSize = **AddressByVersion<int**>(0x713B0C, 0x71433C, 0x72F0AB);

CZoneInfo*&				pCurrZoneInfo = **AddressByVersion<CZoneInfo***>(0x58ADB1, 0x58B581, 0x407F93);
CRGBA*					HudColour = *AddressByVersion<CRGBA**>(0x58ADF6, 0x58B5C6, 0x440648);

CLinkListSA<CPed*>&			ms_weaponPedsForPC = **AddressByVersion<CLinkListSA<CPed*>**>(0x53EACA, 0x53EF6A, 0x551101);
CLinkListSA<AlphaObjectInfo>&	m_alphaList = **AddressByVersion<CLinkListSA<AlphaObjectInfo>**>(0x733A4D, 0x73427D, 0x76DCA3);

DebugMenuAPI gDebugMenuAPI;


// Custom variables
static float		fSunFarClip;
static HMODULE		hDLLModule;
static struct
{
	char			Extension[8];
	unsigned int	Codec;
} UserTrackExtensions[] = { { ".ogg", DECODER_VORBIS }, { ".mp3", DECODER_QUICKTIME },
							{ ".wav", DECODER_WAVE }, { ".wma", DECODER_WINDOWSMEDIA },
							{ ".wmv", DECODER_WINDOWSMEDIA }, { ".aac", DECODER_QUICKTIME },
							{ ".m4a", DECODER_QUICKTIME }, { ".mov", DECODER_QUICKTIME },
							{ ".fla", DECODER_FLAC }, { ".flac", DECODER_FLAC } };

static bool IgnoresWeaponPedsForPCFix()
{
	// TODO: Pre-emptively add INI option to save hassle in the future
	return ModCompat::LSRPMode;
}

// Regular functions
static RpAtomic* RenderAtomic(RpAtomic* pAtomic, float fComp)
{
	UNREFERENCED_PARAMETER(fComp);
	return AtomicDefaultRenderCallBack(pAtomic);
}

static RpAtomic* StaticPropellerRender(RpAtomic* pAtomic)
{
	RwScopedRenderState alphaRef(rwRENDERSTATEALPHATESTFUNCTIONREF);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
	pAtomic = AtomicDefaultRenderCallBack(pAtomic);

	return pAtomic;
}

static RpAtomic* MovingPropellerRender(RpAtomic* pAtomic)
{
	RwScopedRenderState alphaRef(rwRENDERSTATEALPHATESTFUNCTIONREF);
	RwScopedRenderState vertexAlpha(rwRENDERSTATEVERTEXALPHAENABLE);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	pAtomic = AtomicDefaultRenderCallBack(pAtomic);

	return pAtomic;
}

RpAtomic* RenderBigVehicleActomic(RpAtomic* pAtomic, float)
{
	const char*		pNodeName = GetFrameNodeName(RpAtomicGetFrame(pAtomic));

	if ( strncmp(pNodeName, "moving_prop", 11) == 0 )
		return MovingPropellerRender(pAtomic);

	if ( strncmp(pNodeName, "static_prop", 11) == 0 )
		return StaticPropellerRender(pAtomic);

	return AtomicDefaultRenderCallBack(pAtomic);
}

void RenderVehicleHiDetailAlphaCB_HunterDoor(RpAtomic* pAtomic)
{
	AlphaObjectInfo		NewObject;

	NewObject.callback = RenderAtomic;
	NewObject.fCompareValue = -std::numeric_limits<float>::infinity();
	NewObject.pAtomic = pAtomic;

	m_alphaList.InsertFront(NewObject);
}

void RenderWeapon(CPed* pPed)
{
	if ( !IgnoresWeaponPedsForPCFix() )
	{
		pPed->RenderWeapon(true, false, false);
	}
	ms_weaponPedsForPC.Insert(pPed);
}

void RenderWeaponPedsForPC()
{
	RwScopedRenderState vertexAlpha(rwRENDERSTATEVERTEXALPHAENABLE);
	RwScopedRenderState zWrite(rwRENDERSTATEZWRITEENABLE);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

	const bool renderWeapon = IgnoresWeaponPedsForPCFix();

	for ( auto it = ms_weaponPedsForPC.Next( nullptr ); it != nullptr; it = ms_weaponPedsForPC.Next( it ) )
	{
		CPed* ped = **it;
		ped->SetupLighting();
		ped->RenderWeapon(renderWeapon, true, false);
		ped->RemoveLighting();
	}
}

static CAEFLACDecoder* __stdcall DecoderCtor(CAEDataStream* pData)
{
	return new CAEFLACDecoder(pData);
}

static CAEWaveDecoder* __stdcall CAEWaveDecoderInit(CAEDataStream* pStream)
{
	return new CAEWaveDecoder(pStream);
}

static void BasketballFix(unsigned char* pBuf, int nSize)
{
	for ( int i = 0, hits = 0; i < nSize && hits < 7; i++, pBuf++ )
	{
		// Pattern check for save pickup XYZ
		if ( *(unsigned int*)pBuf == 0x449DE19A )		// Save pickup X
		{
			hits++;
			*(float*)pBuf = 1291.8f;
		}
		else if ( *(unsigned int*)pBuf == 0xC4416AE1 )		// Save pickup Y
		{
			hits++;
			*(float*)pBuf = -797.8284f;
		}
		else if ( *(unsigned int*)pBuf == 0x44886C7B )		// Save pickup Z
		{
			hits++;
			*(float*)pBuf = 1089.5f;
		}
		else if ( *(unsigned int*)pBuf == 0x449DF852 )		// Save point X
		{
			hits++;
			*(float*)pBuf = 1286.8f;
		}
		else if ( *(unsigned int*)pBuf == 0xC44225C3 )		// Save point Y
		{
			hits++;
			*(float*)pBuf = -797.69f;
		}
		else if ( *(unsigned int*)pBuf == 0x44885C7B )		// Save point Z
		{
			hits++;
			*(float*)pBuf = 1089.1f;
		}
		else if ( *(unsigned int*)pBuf == 0x43373AE1 )		// Save point A
		{
			hits++;
			*(float*)pBuf = 90.0f;
		}
	}
}

static unsigned char*	ScriptSpace;
static int*				ScriptParams;
static size_t			ScriptFileSize, ScriptMissionSize;

static void InitializeScriptGlobals()
{
	static bool		bInitScriptStuff = [] () {;
		ScriptSpace = *AddressByVersion<unsigned char**>(0x5D5380, 0x5D5B60, 0x450E34);
		ScriptParams = *AddressByVersion<int**>(0x48995B, 0x46410A, 0x46979A);
		ScriptFileSize = *AddressByVersion<size_t*>( 0x468E74+1, 0, 0x46E572+1);
		ScriptMissionSize = *AddressByVersion<size_t*>( 0x489A5A+1, 0, 0x490798+1);

		return true;
	} ();
}

static void SweetsGirlFix()
{
	// Changes @ == int to @ >= int in two places
	if ( *(uint16_t*)(ScriptSpace+ScriptFileSize+2510) == 0x0039 )
		*(uint16_t*)(ScriptSpace+ScriptFileSize+2510) = 0x0029;

	if ( *(uint16_t*)(ScriptSpace+ScriptFileSize+2680) == 0x0039 )
		*(uint16_t*)(ScriptSpace+ScriptFileSize+2680) = 0x0029;
}

static void MountainCloudBoysFix()
{
	auto pattern = hook::make_range_pattern( uintptr_t(ScriptSpace+ScriptFileSize), uintptr_t(ScriptSpace+ScriptFileSize+ScriptMissionSize), 
										"D6 00 04 00 39 00 03 EF 00 04 02 4D 00 01 90 F2 FF FF D6 00 04 01" ).count_hint(1);
	if ( pattern.size() == 1 ) // Faulty code lies under offset 3367 - replace it if it matches
	{
		const uint8_t bNewCode[22] = {
			0x00, 0x00, 0x00, 0x00, 0xD6, 0x00, 0x04, 0x03, 0x39, 0x00, 0x03, 0x2B,
			0x00, 0x04, 0x0B, 0x39, 0x00, 0x03, 0xEF, 0x00, 0x04, 0x02
		};
		memcpy( pattern.get(0).get<void>(), bNewCode, sizeof(bNewCode) );
	}
}

static void SupplyLinesFix( bool isBeefyBaron )
{
	auto pattern = hook::make_range_pattern( uintptr_t(ScriptSpace+ScriptFileSize), uintptr_t(ScriptSpace+ScriptFileSize+ScriptMissionSize),
		isBeefyBaron ? "B8 9E 3A 44" : "B8 1E 2F 44" ).count_hint(1);
	if ( pattern.size() == 1 ) // 700.48 -> 10.0 (teleports car with CJ under the building instead)
	{
		*pattern.get(0).get<float>() = 10.0f;
	}
}

static void QuadrupleStuntBonus()
{
	// IF HEIGHT_FLOAT_HJ > 4.0 -> IF HEIGHT_INT_HJ > 4
	auto pattern = hook::make_range_pattern( uintptr_t(ScriptSpace), uintptr_t(ScriptSpace+ScriptFileSize), "20 00 02 60 14 06 00 00 80 40" ).count_hint(1);
	if ( pattern.size() == 1 )
	{
		const uint8_t newCode[10] = {
			0x18, 0x00, 0x02, 0x30, 0x14, 0x01, 0x04, 0x00, 0x00, 0x00
		};
		memcpy( pattern.get(0).get<void>(), newCode, sizeof(newCode) );
	}
}

void TheScriptsLoad_BasketballFix()
{
	TheScriptsLoad();
	InitializeScriptGlobals();

	BasketballFix(ScriptSpace+8, *(int*)(ScriptSpace+3));
	QuadrupleStuntBonus();
}

void StartNewMission_SCMFixes()
{
	WipeLocalVariableMemoryForMissionScript();
	InitializeScriptGlobals();

	const int missionID = ScriptParams[0];

	// INITIAL - Basketball fix, Quadruple Stunt Bonus
	if ( missionID == 0 )
	{
		BasketballFix(ScriptSpace+ScriptFileSize, ScriptMissionSize);
		QuadrupleStuntBonus();
	}
	// HOODS5 - Sweet's Girl fix
	else if ( missionID == 18 )
		SweetsGirlFix();
	// WUZI1 - Mountain Cloud Boys fix
	else if ( missionID == 53 )
		MountainCloudBoysFix();
	// ZERO2 - Supply Lines fix
	else if ( missionID == 73 )
		SupplyLinesFix( false );
	// ZERO5 - Beefy Baron fix
	else if ( missionID == 10 )
		SupplyLinesFix( true );

}

// 1.01 kinda fixed it
bool GetCurrentZoneLockedOrUnlocked(float fPosX, float fPosY)
{
	// Exploit RAII really bad
	static const float		GridXOffset = **(float**)(0x572135+2), GridYOffset = **(float**)(0x57214A+2);
	static const float		GridXSize = **(float**)(0x57213B+2), GridYSize = **(float**)(0x572153+2);
	static const int		GridXNum = static_cast<int>((2.0f*GridXOffset) * GridXSize), GridYNum = static_cast<int>((2.0f*GridYOffset) * GridYSize);

	static unsigned char* const	ZonesVisited = *(unsigned char**)(0x57216A) - (GridYNum-1);		// 1.01 fixed it!

	int		Xindex = static_cast<int>((fPosX+GridXOffset) * GridXSize);
	int		Yindex = static_cast<int>((fPosY+GridYOffset) * GridYSize);

	// "Territories fix"
	if ( (Xindex >= 0 && Xindex < GridXNum) && (Yindex >= 0 && Yindex < GridYNum) )
		return ZonesVisited[GridXNum*Xindex - Yindex + (GridYNum-1)] != 0;
	
	// Outside of map bounds
	return true;
}

bool GetCurrentZoneLockedOrUnlocked_Steam(float fPosX, float fPosY)
{
	static unsigned char* const	ZonesVisited = *(unsigned char**)(0x5870E8) - 9;

	int		Xindex = static_cast<int>((fPosX+3000.0f) / 600.0f);
	int		Yindex = static_cast<int>((fPosY+3000.0f) / 600.0f);

	// "Territories fix"
	if ( (Xindex >= 0 && Xindex < 10) && (Yindex >= 0 && Yindex < 10) )
		return ZonesVisited[10*Xindex - Yindex + 9] != 0;

	// Outside of map bounds
	return true;
}

// By NTAuthority
void DrawMoonWithPhases(uint8_t moonColor, float* screenPos, float sizeX, float sizeY)
{
	static RwTexture*	gpMoonMask = [] () {
		if ( GetFileAttributesW(L"lunar.png") != INVALID_FILE_ATTRIBUTES )
		{
			// load from file
			return CPNGFile::ReadFromFile("lunar.png");
		}

		// Load from memory
		HRSRC		resource = FindResource(hDLLModule, MAKEINTRESOURCE(IDB_LUNAR64), RT_RCDATA);
		assert( resource != nullptr );
		void*		pMoonMask = LockResource( LoadResource(hDLLModule, resource) );
		
		return CPNGFile::ReadFromMemory(pMoonMask, SizeofResource(hDLLModule, resource));
	} ();
	//D3DPERF_BeginEvent(D3DCOLOR_ARGB(0,0,0,0), L"render moon");

	float currentDayFraction = nGameClockDays / 31.0f;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	float a10 = 1.0f / fFarClipZ;
	float size = (MoonSize * 2) + 4.0f;

	RwD3D9SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

	RenderOneXLUSprite(screenPos[0], screenPos[1], fFarClipZ, sizeX * size, sizeY * size, 0, 0, 0, 0, a10, 255, 0, 0);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, gpMoonMask != nullptr ? RwTextureGetRaster(gpMoonMask) : nullptr );
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
	
	float maskX = (sizeX * size) * 5.4f * (currentDayFraction - 0.5f) + screenPos[0];
	float maskY = screenPos[1] + ((sizeY * size) * 0.7f);

	RenderOneXLUSprite(maskX, maskY, fFarClipZ, sizeX * size * 1.7f, sizeY * size * 1.7f, 0, 0, 0, 255, a10, 255, 0, 0);

	RwD3D9SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gpCoronaTexture[2]));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDDESTALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);

	RenderOneXLUSprite(screenPos[0], screenPos[1], fFarClipZ, sizeX * size, sizeY * size, moonColor, moonColor, static_cast<uint8_t>(moonColor * 0.85f), 255, a10, 255, 0, 0);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	//D3DPERF_EndEvent();
}

CRGBA* CRGBA::BlendGangColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	const double colourIntensity = std::min( static_cast<double>(pCurrZoneInfo->ZoneColour.a) / 120.0, 1.0 );
	*this = CRGBA(BlendSqr( HudColour[3], CRGBA(r, g, b), colourIntensity ), a);
	return this;
}

static bool bColouredZoneNames;
CRGBA* CRGBA::BlendGangColour_Dynamic(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if ( bColouredZoneNames )
	{
		return BlendGangColour(r, g, b, a);
	}
	*this = CRGBA(HudColour[3], a);
	return this;
}

void SunAndMoonFarClip()
{
	fSunFarClip = std::min(1500.0f, fFarClipZ);
	DoSunAndMoon();
}

// STEAM ONLY
template<bool bX1, bool bY1, bool bX2, bool bY2>
void DrawRect_HalfPixel_Steam(CRect& rect, const CRGBA& rgba)
{
	if constexpr ( bX1 )
		rect.x1 -= 0.5f;

	if constexpr ( bY1 )
		rect.y1 -= 0.5f;

	if constexpr ( bX2 )
		rect.x2 -= 0.5f;

	if constexpr ( bY2 )
		rect.y2 -= 0.5f;

	// Steam CSprite2d::DrawRect
	((void(*)(const CRect&, const CRGBA&))0x75CDA0)(rect, rgba);
}

char* GetMyDocumentsPathSA()
{
	static char* const pDocumentsPath = [&] () -> char* {	
		static char	cUserFilesPath[MAX_PATH];
		char* const ppTempBufPtr = Memory::GetVersion().version == 0 ? *AddressByRegion_10<char**>(0x744FE5) : cUserFilesPath;

		if ( SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, ppTempBufPtr) == S_OK )
		{
			char** const ppUserFilesDir = AddressByVersion<char**>(0x74503F, 0x74586F, 0x77EE50, 0x77902B, 0x778F1B);

			PathAppendA(ppTempBufPtr, *ppUserFilesDir);
			CreateDirectoryA(ppTempBufPtr, nullptr);
		}
		else
		{
			strcpy_s(ppTempBufPtr, MAX_PATH, "data");
		}

		char cTmpPath[MAX_PATH];

		strcpy_s(cTmpPath, ppTempBufPtr);
		PathAppendA(cTmpPath, "Gallery");
		CreateDirectoryA(cTmpPath, nullptr);

		strcpy_s(cTmpPath, ppTempBufPtr);
		PathAppendA(cTmpPath, "User Tracks");
		CreateDirectoryA(cTmpPath, nullptr);

		return ppTempBufPtr;
	} ();
	return pDocumentsPath;
}

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

#include <ctime>
#include <random>

static std::ranlux48 generator (time(nullptr));
int32_t Int32Rand()
{
	return generator() & INT32_MAX;
}

void (*FlushSpriteBuffer)() = AddressByVersion<void(*)()>(0x70CF20, 0x70D750, 0x7591E0, 0x753AE0, 0x753A00);
void FlushLensSwitchZ( RwRenderState rwa, void* rwb )
{
	FlushSpriteBuffer();
	RwRenderStateSet( rwa, rwb );
}

void (*InitSpriteBuffer2D)() = AddressByVersion<void(*)()>(0x70CFD0, 0x70D800, 0x759290, 0x753B90, 0x753AB0);
void InitBufferSwitchZ( RwRenderState rwa, void* rwb )
{
	RwRenderStateSet( rwa, rwb );
	InitSpriteBuffer2D();
}

static void* const g_fx = *AddressByVersion<void**>(0x4A9649, 0x4AA4EF, 0x4B2BB9, 0x4B0BE4, 0x4B0BC4);
static int32_t GetFxQuality()
{
	return *(int32_t*)( (uint8_t*)g_fx + 0x54 );
}


DWORD*				msaaValues = *AddressByVersion<DWORD**>(0x4CCBC5, 0x4CCDB5, 0x4D7462, 0x4D6CE5, 0x4D6CB5);
RwRaster*&			pMirrorBuffer = **AddressByVersion<RwRaster***>(0x723001, 0x723831, 0x754971, 0x74F3E1, 0x74F311);
RwRaster*&			pMirrorZBuffer = **AddressByVersion<RwRaster***>(0x72301C, 0x72384C, 0x75498C, 0x74F3FC, 0x74F32C);
void CreateMirrorBuffers()
{
	if ( pMirrorBuffer == nullptr )
	{
		DWORD oldMsaa[2] = { msaaValues[0], msaaValues[1] };
		msaaValues[0] = msaaValues[1] = 0;

		int32_t quality = GetFxQuality();
		RwInt32 width, height;

		if ( quality >= 3 ) // Very High
		{
			width = 2048;
			height = 1024;
		}
		else if ( quality >= 1 ) // Medium
		{
			width = 1024;
			height = 512;
		}
		else
		{
			width = 512;
			height = 256;
		}

		pMirrorBuffer = RwRasterCreate( width, height, 0, rwRASTERTYPECAMERATEXTURE );
		pMirrorZBuffer = RwRasterCreate( width, height, 0, rwRASTERTYPEZBUFFER );

		msaaValues[0] = oldMsaa[0];
		msaaValues[1] = oldMsaa[1];
	}
}

RwUInt32 (*orgGetMaxMultiSamplingLevels)();
RwUInt32 GetMaxMultiSamplingLevels()
{
	RwUInt32 maxSamples = orgGetMaxMultiSamplingLevels();
	RwUInt32 option;
	_BitScanForward( (DWORD*)&option, maxSamples );
	return option + 1;
}

static void (*orgChangeMultiSamplingLevels)(RwUInt32);
void ChangeMultiSamplingLevels( RwUInt32 level )
{
	orgChangeMultiSamplingLevels( 1 << (level - 1) );
}

static void (*orgSetMultiSamplingLevels)(RwUInt32);
void SetMultiSamplingLevels( RwUInt32 level )
{
	orgSetMultiSamplingLevels( 1 << (level - 1) );
}

void MSAAText( char* buffer, const char*, DWORD level )
{
	sprintf_s( buffer, 100, "%ux", 1 << level );
}


static RwInt32 numSavedVideoModes;
static RwInt32 (*orgGetNumVideoModes)();
RwInt32 GetNumVideoModes_Store()
{
	return numSavedVideoModes = orgGetNumVideoModes();
}

RwInt32 GetNumVideoModes_Retrieve()
{
	return numSavedVideoModes;
}


static void* (*orgMemMgrMalloc)(RwUInt32, RwUInt32);
void* CollisionData_MallocAndInit( RwUInt32 size, RwUInt32 hint )
{
	CColData*	mem = (CColData*)orgMemMgrMalloc( size, hint );

	mem->m_bFlags = 0;
	mem->m_dwNumShadowTriangles = mem->m_dwNumShadowVertices =0;
	mem->m_pShadowVertices = mem->m_pShadowTriangles = nullptr;

	return mem;
}

static void* (*orgNewAlloc)(size_t);
void* CollisionData_NewAndInit( size_t size )
{
	CColData*	mem = (CColData*)orgNewAlloc( size );

	mem->m_bFlags = 0;

	return mem;
}


static void (*orgEscalatorsUpdate)();
void UpdateEscalators()
{
	if ( !CEscalator::ms_entitiesToRemove.empty() )
	{
		for ( auto it : CEscalator::ms_entitiesToRemove )
		{
			WorldRemove( it );
			delete it;
		}
		CEscalator::ms_entitiesToRemove.clear();
	}
	orgEscalatorsUpdate();
}


static char** pStencilShadowsPad = *AddressByVersion<char***>(0x70FC4F, 0, 0x75E286, 0x758A47, 0x758937);
void StencilShadowAlloc( )
{
	static char* pMemory = [] () {;
		char* mem = static_cast<char*>( orgNewAlloc( 3 * 0x6000 ) );
		pStencilShadowsPad[0] = mem;
		pStencilShadowsPad[1] = mem+0x6000;
		pStencilShadowsPad[2] = mem+(2*0x6000);

		return mem;
	} ();
}

RwBool GTARtAnimInterpolatorSetCurrentAnim(RtAnimInterpolator* animI, RtAnimAnimation* anim)
{
	animI->pCurrentAnim = anim;
	animI->currentTime = 0.0f;

	const RtAnimInterpolatorInfo* info = anim->interpInfo;
	animI->currentInterpKeyFrameSize = info->interpKeyFrameSize;
	animI->currentAnimKeyFrameSize = info->animKeyFrameSize;
	animI->keyFrameApplyCB = info->keyFrameApplyCB;
	animI->keyFrameBlendCB = info->keyFrameBlendCB;
	animI->keyFrameInterpolateCB = info->keyFrameInterpolateCB;
	animI->keyFrameAddCB = info->keyFrameAddCB;

	for ( RwInt32 i = 0; i < animI->numNodes; ++i )
	{
		RtAnimKeyFrameInterpolate( animI, rtANIMGETINTERPFRAME( animI, i ),
			(RwChar*)anim->pFrames + i * animI->currentAnimKeyFrameSize,
			(RwChar*)anim->pFrames + ( i + animI->numNodes) * animI->currentAnimKeyFrameSize, 0.0f );
	}

	animI->pNextFrame = (RwChar*)anim->pFrames + 2 * animI->currentAnimKeyFrameSize * animI->numNodes;

	return TRUE;
}

DWORD WINAPI CdStreamSetFilePointer( HANDLE hFile, uint32_t distanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod )
{
	assert( lpDistanceToMoveHigh == nullptr );

	LARGE_INTEGER li;
	li.QuadPart = int64_t(distanceToMove) << 11;
	return SetFilePointer( hFile, li.LowPart, &li.HighPart, dwMoveMethod );
}
static auto* const pCdStreamSetFilePointer = CdStreamSetFilePointer;

static void (*orgDrawScriptSpritesAndRectangles)(uint8_t);
void DrawScriptSpritesAndRectangles( uint8_t arg )
{
	RwRenderStateSet( rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR );
	orgDrawScriptSpritesAndRectangles( arg );
}

// Now in VehicleSA.cpp
bool ReadDoubleRearWheels(const wchar_t* pPath);
bool __stdcall CheckDoubleRWheelsList( void* modelInfo, uint8_t* handlingData );

CVehicleModelInfo* (__thiscall *orgVehicleModelInfoCtor)(CVehicleModelInfo*);
CVehicleModelInfo* __fastcall VehicleModelInfoCtor(CVehicleModelInfo* me)
{
	orgVehicleModelInfoCtor(me);
	me->m_apPlateMaterials = nullptr;
	me->m_dirtMaterials = nullptr;
	me->m_numDirtMaterials = 0;
	std::fill( std::begin( me->m_staticDirtMaterials ), std::end( me->m_staticDirtMaterials ), nullptr );
	return me;
}

static void (*RemoveFromInterestingVehicleList)(CVehicle*) = AddressByVersion<void(*)(CVehicle*)>( 0x423ED0, Memory::PatternAndOffset("39 10 75 06 C7 00 00 00 00 00 83 C0 04 49 75 F0 5D C3", -0x10) );
static void (*orgRecordVehicleDeleted)(CVehicle*);
static void RecordVehicleDeleted_AndRemoveFromVehicleList( CVehicle* vehicle )
{
	orgRecordVehicleDeleted( vehicle );
	RemoveFromInterestingVehicleList( vehicle );
}

static int currDisplayedSplash_ForLastSplash = 0;
static void DoPCScreenChange_Mod()
{
	static int& currDisplayedSplash = **AddressByVersion<int**>( 0x590B22 + 1, Memory::PatternAndOffset("8B 51 20 6A 01 6A 0C FF D2 83 C4 08 E8", 17 + 1) );

	static const int numSplashes = [] () -> int {
		RwTexture** begin = *AddressByVersion<RwTexture***>( 0x590CB4 + 1, Memory::PatternAndOffset("8D 49 00 83 3E 00 74 07 8B CE E8", -5 + 1) );
		RwTexture** end = *AddressByVersion<RwTexture***>( 0x590CCE + 2, Memory::PatternAndOffset("8D 49 00 83 3E 00 74 07 8B CE E8", 18 + 2) );
		return std::distance( begin, end );
	} () - 1;

	if ( currDisplayedSplash >= numSplashes )
	{
		currDisplayedSplash = 1;
		currDisplayedSplash_ForLastSplash = numSplashes + 1;
	}
	else
	{
		currDisplayedSplash_ForLastSplash = ++currDisplayedSplash;
	}
}

static bool bUseAaronSun;
static CVector curVecToSun;
static void (*orgSetLightsWithTimeOfDayColour)( RpWorld* );
static void SetLightsWithTimeOfDayColour_SilentPatch( RpWorld* world )
{
	static CVector* const VectorToSun = *AddressByVersion<CVector**>( 0x6FC5B7 + 3, Memory::PatternAndOffset("DC 0D ? ? ? ? 8D 04 40 8B 0C 85", 9 + 3) );
	static int& CurrentStoredValue = **AddressByVersion<int**>( 0x6FC632 + 1, Memory::PatternAndOffset("84 C0 0F 84 AB 01 00 00 A1", 8 + 1) );
	static CVector& vecDirnLightToSun = **AddressByVersion<CVector**>( 0x5BC040 + 2, Memory::PatternAndOffset("E8 ? ? ? ? D9 5D F8 D9 45 F8 D8 4D F4 D9 1D", 15 + 2) );

	curVecToSun = bUseAaronSun ? VectorToSun[CurrentStoredValue] : vecDirnLightToSun;
	orgSetLightsWithTimeOfDayColour( world );
}

// ============= CdStream data racing issue =============

struct CdStream
{
	DWORD nSectorOffset;
	DWORD nSectorsToRead;
	LPVOID lpBuffer;
	BYTE field_C;
	BYTE bLocked;
	BYTE bInUse;
	BYTE field_F;
	DWORD status;
	union Sync {
		HANDLE semaphore;
		CONDITION_VARIABLE cv;
	} sync;
	HANDLE hFile;
	OVERLAPPED overlapped;
};

static_assert(sizeof(CdStream) == 0x30, "Incorrect struct size: CdStream");

namespace CdStreamSync {

static CRITICAL_SECTION CdStreamCritSec;

// Function pointers for game to use
static CdStream::Sync (__stdcall *CdStreamInitializeSyncObject)();
static DWORD (__stdcall *CdStreamSyncOnObject)( CdStream* stream );
static void (__stdcall *CdStreamThreadOnObject)( CdStream* stream );
static void (__stdcall *CdStreamCloseObject)( CdStream::Sync* sync );
static void (__stdcall *CdStreamShutdownSyncObject)( CdStream* stream );

static void __stdcall CdStreamShutdownSyncObject_Stub( CdStream* stream, size_t idx )
{
	CdStreamShutdownSyncObject( &stream[idx] );
}

// Fixed return values for GetOverlappedResult - stock code assumes "nonzero" equals 1, might not be future proof
static uint32_t WINAPI GetOverlappedResult_SilentPatch( HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait )
{
	return GetOverlappedResult( hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait ) != FALSE ? 0 : 254;
}
static auto* const pGetOverlappedResult = &GetOverlappedResult_SilentPatch;


namespace Sema
{
	CdStream::Sync __stdcall InitializeSyncObject()
	{
		CdStream::Sync object;
		object.semaphore = CreateSemaphore( nullptr, 0, 2, nullptr );
		return object;
	}

	void __stdcall ShutdownSyncObject( CdStream* stream )
	{
		CloseHandle( stream->sync.semaphore );
	}

	DWORD __stdcall CdStreamSync( CdStream* stream )
	{
		EnterCriticalSection( &CdStreamCritSec );
		if ( stream->nSectorsToRead != 0 )
		{
			stream->bLocked = 1;
			LeaveCriticalSection( &CdStreamCritSec );
			WaitForSingleObject( stream->sync.semaphore, INFINITE );
			EnterCriticalSection( &CdStreamCritSec );
		}
		stream->bInUse = 0;
		LeaveCriticalSection( &CdStreamCritSec );
		return stream->status;
	}

	void __stdcall CdStreamThread( CdStream* stream )
	{
		EnterCriticalSection( &CdStreamCritSec );
		stream->nSectorsToRead = 0;
		if ( stream->bLocked != 0 )
		{
			ReleaseSemaphore( stream->sync.semaphore, 1, nullptr );
		}
		stream->bInUse = 0;
		LeaveCriticalSection( &CdStreamCritSec );
	}
}

namespace CV
{
	namespace Funcs
	{
		static decltype(InitializeConditionVariable)* pInitializeConditionVariable = nullptr;
		static decltype(SleepConditionVariableCS)* pSleepConditionVariableCS = nullptr;
		static decltype(WakeConditionVariable)* pWakeConditionVariable = nullptr;

		static bool TryInit()
		{
			const HMODULE kernelDLL = GetModuleHandle( TEXT("kernel32") );
			assert( kernelDLL != nullptr );
			pInitializeConditionVariable = (decltype(pInitializeConditionVariable))GetProcAddress( kernelDLL, "InitializeConditionVariable" );
			pSleepConditionVariableCS = (decltype(pSleepConditionVariableCS))GetProcAddress( kernelDLL, "SleepConditionVariableCS" );
			pWakeConditionVariable = (decltype(pWakeConditionVariable))GetProcAddress( kernelDLL, "WakeConditionVariable" );

			return pInitializeConditionVariable != nullptr && pSleepConditionVariableCS != nullptr && pWakeConditionVariable != nullptr;
		}


	}

	CdStream::Sync __stdcall InitializeSyncObject()
	{
		CdStream::Sync object;
		Funcs::pInitializeConditionVariable( &object.cv );
		return object;
	}

	void __stdcall ShutdownSyncObject( CdStream* stream )
	{
	}

	DWORD __stdcall CdStreamSync( CdStream* stream )
	{
		EnterCriticalSection( &CdStreamCritSec );
		while ( stream->nSectorsToRead != 0 )
		{
			Funcs::pSleepConditionVariableCS( &stream->sync.cv, &CdStreamCritSec, INFINITE );
		}
		stream->bInUse = 0;
		LeaveCriticalSection( &CdStreamCritSec );
		return stream->status;
	}

	void __stdcall CdStreamThread( CdStream* stream )
	{
		EnterCriticalSection( &CdStreamCritSec );
		stream->nSectorsToRead = 0;
		Funcs::pWakeConditionVariable( &stream->sync.cv );
		stream->bInUse = 0;
		LeaveCriticalSection( &CdStreamCritSec );
	}
}

static void (*orgCdStreamInitThread)();
static void CdStreamInitThread()
{
	if ( ModCompat::bCdStreamFallBackForOldML != true && CV::Funcs::TryInit() )
	{
		CdStreamInitializeSyncObject = CV::InitializeSyncObject;
		CdStreamShutdownSyncObject = CV::ShutdownSyncObject;
		CdStreamSyncOnObject = CV::CdStreamSync;
		CdStreamThreadOnObject = CV::CdStreamThread;
	}
	else
	{
		CdStreamInitializeSyncObject = Sema::InitializeSyncObject;
		CdStreamShutdownSyncObject = Sema::ShutdownSyncObject;
		CdStreamSyncOnObject = Sema::CdStreamSync;
		CdStreamThreadOnObject = Sema::CdStreamThread;		
	}

	InitializeCriticalSectionAndSpinCount( &CdStreamCritSec, 10 );

	FLAUtils::SetCdStreamWakeFunction( []( CdStream* pStream ) {
		CdStreamThreadOnObject( pStream );
	} );

	orgCdStreamInitThread();
}

}

// Dancing timers fix
static long UtilsVariablesInit = 0;
static LARGE_INTEGER UtilsStartTime;
static LARGE_INTEGER UtilsFrequency;
static BOOL WINAPI AudioUtilsFrequency( PLARGE_INTEGER lpFrequency )
{
	::QueryPerformanceFrequency( &UtilsFrequency );
	UtilsFrequency.QuadPart /= 1000;
	lpFrequency->QuadPart = UtilsFrequency.QuadPart;
	return TRUE;
}
static auto* const pAudioUtilsFrequency = &AudioUtilsFrequency;

static int64_t AudioUtilsGetStartTime()
{
	QueryPerformanceCounter( &UtilsStartTime );

	_InterlockedExchange( &UtilsVariablesInit, 1 );
	return UtilsStartTime.QuadPart;
}

static int64_t AudioUtilsGetCurrentTimeInMs()
{
	if ( _InterlockedCompareExchange( &UtilsVariablesInit, 0, 0 ) == 0 )
	{
		return 0;
	}

	LARGE_INTEGER currentTime;
	QueryPerformanceCounter( &currentTime );
	return (currentTime.QuadPart - UtilsStartTime.QuadPart) / UtilsFrequency.QuadPart;
}

// Minimal HUD changes
static CRGBA* __fastcall SetRGBA_FloatAlpha( CRGBA* rgba, void*, uint8_t red, uint8_t green, uint8_t blue, float alpha )
{
	rgba->r = red;
	rgba->g = green;
	rgba->b = blue;
	rgba->a = static_cast<uint8_t>(alpha);
	return rgba;
}

static void RenderXLUSprite_FloatAlpha( float arg1, float arg2, float arg3, float arg4, float arg5, uint8_t red, uint8_t green, uint8_t blue, int16_t mult, float arg10, float alpha, uint8_t arg12, uint8_t arg13 )
{
	RenderOneXLUSprite( arg1, arg2, arg3, arg4, arg5, red, green, blue, mult, arg10, static_cast<uint8_t>(alpha), arg12, arg13 );
}

// 6 directionals on Medium/High/Very High Visual FX
int32_t GetMaxExtraDirectionals( uint32_t areaID )
{
	return areaID != 0 || GetFxQuality() >= 1 ? 6 : 4;
}

static CVehicle* FindPlayerVehicle_RCWrap( int playerID, bool )
{
	return FindPlayerVehicle( playerID, true );
}

// ============= Credits! =============
namespace Credits
{
	static void (*PrintCreditText)(float scaleX, float scaleY, const char* text, unsigned int& pos, float timeOffset, bool isHeader);
	static void (*PrintCreditText_Hooked)(float scaleX, float scaleY, const char* text, unsigned int& pos, float timeOffset, bool isHeader);

	static void PrintCreditSpace( float scale, unsigned int& pos )
	{
		pos += static_cast<unsigned int>( scale * 25.0f );
	}

	constexpr char xvChar(const char ch)
	{
		constexpr uint8_t xv = SILENTPATCH_REVISION_ID;
		return ch ^ xv;
	}

	constexpr char operator "" _xv(const char ch)
	{
		return xvChar(ch);
	}

	static void PrintSPCredits( float scaleX, float scaleY, const char* text, unsigned int& pos, float timeOffset, bool isHeader )
	{
		// Original text we intercepted
		PrintCreditText_Hooked( scaleX, scaleY, text, pos, timeOffset, isHeader );
		PrintCreditSpace( 1.5f, pos );

		{
			char spText[] = { 'A'_xv, 'N'_xv, 'D'_xv, '\0'_xv };

			for ( auto& ch : spText ) ch = xvChar(ch);
			PrintCreditText( scaleX, scaleY, spText, pos, timeOffset, true );
		}

		PrintCreditSpace( 1.5f, pos );

		{
			char spText[] = { 'A'_xv, 'd'_xv, 'r'_xv, 'i'_xv, 'a'_xv, 'n'_xv, ' '_xv, '\"'_xv, 'S'_xv, 'i'_xv, 'l'_xv, 'e'_xv, 'n'_xv, 't'_xv, '\"'_xv, ' '_xv,
							'Z'_xv, 'd'_xv, 'a'_xv, 'n'_xv, 'o'_xv, 'w'_xv, 'i'_xv, 'c'_xv, 'z'_xv, '\0'_xv };

			for ( auto& ch : spText ) ch = xvChar(ch);
			PrintCreditText( scaleX, scaleY, spText, pos, timeOffset, false );
		}
	}
}

// ============= Bicycle fire fix =============
namespace BicycleFire
{
	CPed* GetVehicleDriver( const CVehicle* vehicle )
	{
		return vehicle->GetDriver();
	}

	void __fastcall DoStuffToGoOnFire_NullAndPlayerCheck( CPed* ped )
	{
		if ( ped != nullptr && ped->IsPlayer() )
		{
			static_cast<CPlayerPed*>(ped)->DoStuffToGoOnFire();
		}
	}
}


// ============= Keyboard latency input fix =============
namespace KeyboardInputFix
{
	static void* NewKeyState;
	static void* OldKeyState;
	static void* TempKeyState;
	static size_t objSize;
	static void (__fastcall *orgClearSimButtonPressCheckers)(void*);
	void __fastcall ClearSimButtonPressCheckers(void* pThis)
	{
		memcpy( OldKeyState, NewKeyState, objSize );
		memcpy( NewKeyState, TempKeyState, objSize );

		orgClearSimButtonPressCheckers(pThis);
	}
}

// ============= handling.cfg name matching fix =============
namespace HandlingNameLoadFix
{
	void strncpy_Fix( const char** destination, const char* source, size_t )
	{
		*destination = source;
	}

	int strncmp_Fix( const char* str1, const char** str2, size_t )
	{
		return strcmp( str1, *str2 );
	}
};


// ============= Firela ladder animation =============
namespace FirelaHook
{
	static uintptr_t UpdateMovingCollisionJmp;
	static uintptr_t HydraulicControlJmpBack;

	void __declspec(naked) TestFirelaAndFlags()
	{
		__asm
		{
			mov		ecx, esi
			call	CVehicle::HasFirelaLadder
			test	al, al
			jnz		TestFirelaAndFlags_UpdateMovingCollision
			test	[esi].hFlagsLocal, FLAG_HYDRAULICS_INSTALLED
			jmp		[HydraulicControlJmpBack]

TestFirelaAndFlags_UpdateMovingCollision:
			jmp		[UpdateMovingCollisionJmp]
		}
	}

	static uintptr_t FollowCarCamNoMovement;
	static uintptr_t FollowCarCamJmpBack;

	void __declspec(naked) CamControlFirela()
	{
		__asm
		{
			mov		ecx, edi
			call	CVehicle::HasFirelaLadder
			test	al, al
			jnz		TestFirelaAndFlags_UpdateMovingCollision
			mov		eax, [edi].m_dwVehicleClass
			jmp		[FollowCarCamJmpBack]

			TestFirelaAndFlags_UpdateMovingCollision:
			jmp		[FollowCarCamNoMovement]
		}
	}
}

namespace HierarchyTypoFix
{
	// Allow wheel_lm vs wheel_lm_dummy and miscX vs misc_X typos
	constexpr std::pair<const char*, const char*> typosAndFixes[] = {
		{ "wheel_lm_dummy", "wheel_lm" },
		{ "misc_a", "misca" },
		{ "misc_b", "miscb" },
		{ "boat_moving_hi", "boat_moving" },
	};
	int strcasecmp( const char* dataName, const char* nodeName )
	{
		for ( const auto& typo : typosAndFixes )
		{
			if ( _stricmp( dataName, typo.first ) == 0 && _stricmp( nodeName, typo.second ) == 0 ) return 0;
		}

		return _stricmp( dataName, nodeName );
	}

}


// Resetting stats and variables on New Game
namespace VariableResets
{
	// Custom reset values for variables
	template<typename T>
	struct TimeNextMadDriverChaseCreated_t
	{
		T m_timer;

		TimeNextMadDriverChaseCreated_t()
			: m_timer( (static_cast<float>(Int32Rand()) / INT32_MAX) * 600.0f + 600.0f )
		{
		}
	};

	using VarVariant = std::variant<bool*, int*, TimeNextMadDriverChaseCreated_t<float>*>;
	std::vector<VarVariant> GameVariablesToReset;

	static void (*orgReInitGameObjectVariables)();
	void ReInitGameObjectVariables()
	{
		// First reinit "our" variables in case stock ones rely on those during resetting
		for ( const auto& var : GameVariablesToReset )
		{
			std::visit( []( auto&& v ) {
				*v = {};
			}, var );
		}

		orgReInitGameObjectVariables();
	}

}

namespace LightbeamFix
{
	static CVehicle* currentHeadLightBeamVehicle;
	void SetCurrentVehicle( CVehicle* vehicle )
	{
		currentHeadLightBeamVehicle = vehicle;
	}

	template<RwRenderState State>
	class RenderStateWrapper
	{
	private:
		static inline void* SavedState;

		static void PushState( RwRenderState state, void* value )
		{
			assert( State == state );
			RwRenderStateGet( state, &SavedState );
			RwRenderStateSet( state, value );
		}
		static inline const auto pPushState = &PushState;

		static void PopState( RwRenderState state, void* value )
		{
			assert( State == state );

			void* valueToRestore = SavedState;
			if constexpr ( State == rwRENDERSTATECULLMODE )
			{
				assert( currentHeadLightBeamVehicle != nullptr );
				if ( currentHeadLightBeamVehicle != nullptr && currentHeadLightBeamVehicle->IgnoresLightbeamFix() )
				{
					valueToRestore = value;
				}
			}

			RwRenderStateSet( state, valueToRestore );

			// Restore states R* did not restore after changing them
			if constexpr ( State == rwRENDERSTATEDESTBLEND )
			{
				RenderStateWrapper<rwRENDERSTATESHADEMODE>::PopAnotherState();
			}
			if constexpr ( State == rwRENDERSTATECULLMODE )
			{
				RenderStateWrapper<rwRENDERSTATEALPHATESTFUNCTION>::PopAnotherState();
				RenderStateWrapper<rwRENDERSTATEALPHATESTFUNCTIONREF>::PopAnotherState();
			}
		}
		static inline const auto pPopState = &PopState;

	public:
		static void PopAnotherState()
		{
			PopState( State, nullptr );
		}

		static inline const uintptr_t PushStatePPtr = reinterpret_cast<uintptr_t>(&pPushState) - 0x20;
		static inline const uintptr_t PopStatePPtr = reinterpret_cast<uintptr_t>(&pPopState) - 0x20;
	};

}

#ifndef NDEBUG

// ============= QPC spoof for verifying high timer issues =============
namespace FakeQPC
{
	static int64_t AddedTime;
	static BOOL WINAPI FakeQueryPerformanceCounter(PLARGE_INTEGER lpPerformanceCount)
	{
		const BOOL result = ::QueryPerformanceCounter( lpPerformanceCount );
		lpPerformanceCount->QuadPart += AddedTime;
		return result;
	}
}

#endif

#if MEM_VALIDATORS

#include <intrin.h>

// Validator for static allocations
void PutStaticValidator( uintptr_t begin, uintptr_t end )
{
	uint8_t* a = (uint8_t*)begin;
	uint8_t* b = (uint8_t*)end;

	std::fill( a, b, uint8_t(0xCC) );
}

void* malloc_validator(size_t size)
{
	return _malloc_dbg( size, _NORMAL_BLOCK, "EXE", (uintptr_t)_ReturnAddress() );
}

void* realloc_validator(void* ptr, size_t size)
{
	return _realloc_dbg( ptr, size, _NORMAL_BLOCK, "EXE", (uintptr_t)_ReturnAddress() );
}

void* calloc_validator(size_t count, size_t size)
{
	return _calloc_dbg( count, size, _NORMAL_BLOCK, "EXE", (uintptr_t)_ReturnAddress() );
}

void free_validator(void* ptr)
{
	_free_dbg(ptr, _NORMAL_BLOCK);
}

size_t _msize_validator(void* ptr)
{
	return _msize_dbg(ptr, _NORMAL_BLOCK);
}

void* _new(size_t size)
{
	return _malloc_dbg( size, _NORMAL_BLOCK, "EXE", (uintptr_t)_ReturnAddress() );
}

void _delete(void* ptr)
{
	_free_dbg(ptr, _NORMAL_BLOCK);
}

class CDebugMemoryMgr
{
public:
	static void* Malloc(size_t size)
	{
		return _malloc_dbg( size, _NORMAL_BLOCK, "EXE", (uintptr_t)_ReturnAddress() );
	}

	static void Free(void* ptr)
	{
		_free_dbg(ptr, _NORMAL_BLOCK);
	}

	static void* Realloc(void* ptr, size_t size)
	{
		return _realloc_dbg( ptr, size, _NORMAL_BLOCK, "EXE", (uintptr_t)_ReturnAddress() );
	}

	static void* Calloc(size_t count, size_t size)
	{
		return _calloc_dbg( count, size, _NORMAL_BLOCK, "EXE", (uintptr_t)_ReturnAddress() );
	}

	static void* MallocAlign(size_t size, size_t align)
	{
		return _aligned_malloc_dbg( size, align, "EXE", (uintptr_t)_ReturnAddress() );
	}

	static void AlignedFree(void* ptr)
	{
		_aligned_free_dbg(ptr);
	}
};

void InstallMemValidator()
{
	using namespace Memory;

	// TEST: Validate memory
	InjectHook( 0x824257, malloc_validator, PATCH_JUMP );
	InjectHook( 0x824269, realloc_validator, PATCH_JUMP );
	InjectHook( 0x824416, calloc_validator, PATCH_JUMP );
	InjectHook( 0x82413F, free_validator, PATCH_JUMP );
	InjectHook( 0x828C4A, _msize_validator, PATCH_JUMP );

	InjectHook( 0x82119A, _new, PATCH_JUMP );
	InjectHook( 0x8214BD, _delete, PATCH_JUMP );

	InjectHook( 0x72F420, &CDebugMemoryMgr::Malloc, PATCH_JUMP );
	InjectHook( 0x72F430, &CDebugMemoryMgr::Free, PATCH_JUMP );
	InjectHook( 0x72F440, &CDebugMemoryMgr::Realloc, PATCH_JUMP );
	InjectHook( 0x72F460, &CDebugMemoryMgr::Calloc, PATCH_JUMP );
	InjectHook( 0x72F4C0, &CDebugMemoryMgr::MallocAlign, PATCH_JUMP );
	InjectHook( 0x72F4F0, &CDebugMemoryMgr::AlignedFree, PATCH_JUMP );


	PutStaticValidator( 0xAAE950, 0xB4C310 ); // CStore
	PutStaticValidator( 0xA9AE00, 0xA9AE58 ); // fx_c
}

#endif


// Hooks
void __declspec(naked) LightMaterialsFix()
{
	_asm
	{
		mov     [esi], edi
		mov		ebx, [ecx]
		lea     esi, [edx+4]
		mov		[ebx+4], esi
		mov		edi, [esi]
		mov		[ebx+8], edi
		add		esi, 4
		mov		[ebx+12], esi
		mov		edi, [esi]
		mov		[ebx+16], edi
		add		ebx, 20
		mov		[ecx], ebx
		retn
	}
}

void __declspec(naked) UserTracksFix()
{
	_asm
	{
		push	[esp+4]
		call	SetVolume
		mov		ecx, [pUserTracksStuff]
		mov		byte ptr [ecx+0Dh], 1
		call	InitializeUtrax
		retn	4
	}
}

void __declspec(naked) UserTracksFix_Steam()
{
	_asm
	{
		push	[esp+4]
		call	SetVolume
		mov		ecx, [pUserTracksStuff]
		mov		byte ptr [ecx+5], 1
		call	InitializeUtrax
		retn	4
	}
}

static void*	PlaneAtomicRendererSetup_JumpBack = AddressByVersion<void*>(0x4C7986, 0x4C7A06, 0x4D2275);
static void*	RenderVehicleHiDetailAlphaCB_BigVehicle = AddressByVersion<void*>(0x734370, 0x734BA0, 0x76E400);
static void*	RenderVehicleHiDetailCB_BigVehicle = AddressByVersion<void*>(0x733420, 0x733C50, 0x76D6C0);
void __declspec(naked) PlaneAtomicRendererSetup()
{
	static const char	aStaticProp[] = "static_prop";
	static const char	aMovingProp[] = "moving_prop";
	_asm
	{
		mov     eax, [esi+4]
		push	eax
		call	GetFrameNodeName
		//push	eax
		mov		[esp+8+8], eax
		push	11
		push	offset aStaticProp
		push	eax
		call	strncmp
		add		esp, 10h
		test	eax, eax
		jz		PlaneAtomicRendererSetup_Alpha
		push	11
		push	offset aMovingProp
		push	[esp+12+8]
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jnz		PlaneAtomicRendererSetup_NoAlpha

PlaneAtomicRendererSetup_Alpha:
		push	[RenderVehicleHiDetailAlphaCB_BigVehicle]
		jmp		PlaneAtomicRendererSetup_Return

PlaneAtomicRendererSetup_NoAlpha:
		push	[RenderVehicleHiDetailCB_BigVehicle]

PlaneAtomicRendererSetup_Return:
		jmp		PlaneAtomicRendererSetup_JumpBack
	}
}

static unsigned int		nCachedCRC;

static void*	RenderVehicleHiDetailCB = AddressByVersion<void*>(0x733240, 0x733A70, 0x76D4C0);
static void*	RenderVehicleHiDetailAlphaCB = AddressByVersion<void*>(0x733F80, 0x7347B0, 0x76DFE0);
static void*	RenderHeliRotorAlphaCB = AddressByVersion<void*>(0x7340B0, 0x7348E0, 0x76E110);
static void*	RenderHeliTailRotorAlphaCB = AddressByVersion<void*>(0x734170, 0x7349A0, 0x76E1E0);
static void*	HunterTest_JumpBack = AddressByVersion<void*>(0x4C7914, 0x4C7994, 0x4D2203);

// strcmp can't be invoked from inline assembly block?
static int strcmp_wrap(const char *s1, const char *s2)
{
	return strcmp( s1, s2 );
}

void __declspec(naked) HunterTest()
{
	static const char	aDoorDummy[] = "door_lf_ok";
	static const char	aStaticRotor[] = "static_rotor";
	static const char	aStaticRotor2[] = "static_rotor2";
	static const char	aWindscreen[] = "windscreen";
	_asm
	{
		setnz	al
		movzx	di, al

		push	10
		push	offset aWindscreen
		push	ebp
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jz		HunterTest_RegularAlpha

		push	offset aStaticRotor2
		push	ebp
		call	strcmp_wrap
		add		esp, 8
		test	eax, eax
		jz		HunterTest_StaticRotor2AlphaSet

		push	offset aStaticRotor
		push	ebp
		call	strcmp_wrap
		add		esp, 8
		test	eax, eax
		jz		HunterTest_StaticRotorAlphaSet

		test	di, di
		jnz		HunterTest_DoorTest

		push	[RenderVehicleHiDetailCB]
		jmp		HunterTest_JumpBack

HunterTest_DoorTest:
		cmp		nCachedCRC, 0x45D0B41C
		jnz		HunterTest_RegularAlpha
		push	offset aDoorDummy
		push	ebp
		call	strcmp_wrap
		add		esp, 8
		test	eax, eax
		jnz		HunterTest_RegularAlpha
		push	RenderVehicleHiDetailAlphaCB_HunterDoor
		jmp		HunterTest_JumpBack

HunterTest_RegularAlpha:
		push	[RenderVehicleHiDetailAlphaCB]
		jmp		HunterTest_JumpBack

HunterTest_StaticRotorAlphaSet:
		push	[RenderHeliRotorAlphaCB]
		jmp		HunterTest_JumpBack

HunterTest_StaticRotor2AlphaSet:
		push	[RenderHeliTailRotorAlphaCB]
		jmp		HunterTest_JumpBack
	}
}
 
static void*	CacheCRC32_JumpBack = AddressByVersion<void*>(0x4C7B10, 0x4C7B90, 0x4D2400);
void __declspec(naked) CacheCRC32()
{
	_asm
	{
		mov		eax, [ecx+4]
		mov		nCachedCRC, eax
		jmp		CacheCRC32_JumpBack
	}
}

static void* const TrailerDoubleRWheelsFix_ReturnFalse = AddressByVersion<void*>(0x4C9333, 0x4C9533, 0x4D3C59);
static void* const TrailerDoubleRWheelsFix_ReturnTrue = AddressByVersion<void*>(0x4C9235, 0x4C9435, 0x4D3B59);
void __declspec(naked) TrailerDoubleRWheelsFix()
{
	_asm
	{
		cmp		[edi]CVehicleModelInfo.m_dwType, VEHICLE_TRAILER
		je		TrailerDoubleRWheelsFix_DoWheels
		cmp		eax, 2
		je		TrailerDoubleRWheelsFix_False
		cmp		eax, 5
		je		TrailerDoubleRWheelsFix_False

TrailerDoubleRWheelsFix_DoWheels:
		jmp		TrailerDoubleRWheelsFix_ReturnTrue

TrailerDoubleRWheelsFix_False:
		jmp		TrailerDoubleRWheelsFix_ReturnFalse
	}
}

void __declspec(naked) TrailerDoubleRWheelsFix2()
{
	_asm
	{
		add     esp, 18h
		mov     eax, [ebx]
		mov     eax, [esi+eax+4]
		jmp		TrailerDoubleRWheelsFix
	}
}

void __declspec(naked) TrailerDoubleRWheelsFix_Steam()
{
	_asm
	{
		cmp		[esi]CVehicleModelInfo.m_dwType, VEHICLE_TRAILER
		je		TrailerDoubleRWheelsFix_DoWheels
		cmp		eax, 2
		je		TrailerDoubleRWheelsFix_False
		cmp		eax, 5
		je		TrailerDoubleRWheelsFix_False

TrailerDoubleRWheelsFix_DoWheels:
		jmp		TrailerDoubleRWheelsFix_ReturnTrue

TrailerDoubleRWheelsFix_False:
		jmp		TrailerDoubleRWheelsFix_ReturnFalse
	}
}

void __declspec(naked) TrailerDoubleRWheelsFix2_Steam()
{
	_asm
	{
		add     esp, 18h
		mov     eax, [ebp]
		mov     eax, [ebx+eax+4]
		jmp		TrailerDoubleRWheelsFix_Steam
	}
}

static void*	LoadFLAC_JumpBack = AddressByVersion<void*>(0x4F3743, Memory::GetVersion().version == 1 ? (*(BYTE*)0x4F3A50 == 0x6A ? 0x4F3BA3 : 0x5B6B81) : 0, 0x4FFC3F);
void __declspec(naked) LoadFLAC()
{
	_asm
	{
		jz		LoadFLAC_WindowsMedia
		sub		ebp, 2
		jnz		LoadFLAC_Return
		push	esi
		call	DecoderCtor
		jmp		LoadFLAC_Success

LoadFLAC_WindowsMedia:
		jmp		LoadFLAC_JumpBack

LoadFLAC_Success:
		test	eax, eax
		mov		[esp+20h+4], eax
		jnz		LoadFLAC_Return_NoDelete

LoadFLAC_Return:
		mov		ecx, esi
		call	CAEDataStreamOld::~CAEDataStreamOld
		push	esi
		call	GTAdelete
		add     esp, 4

LoadFLAC_Return_NoDelete:
		mov     eax, [esp+20h+4]
		mov		ecx, [esp+20h-0Ch]
		pop		esi
		pop		ebp
		pop		edi
		pop		ebx
		mov		fs:0, ecx
		add		esp, 10h
		retn	4
	}
}

// 1.01 securom butchered this func, might not be reliable
void __declspec(naked) LoadFLAC_11()
{
	_asm
	{
		jz		LoadFLAC_WindowsMedia
		sub		ebp, 2
		jnz		LoadFLAC_Return
		push	esi
		call	DecoderCtor
		jmp		LoadFLAC_Success

LoadFLAC_WindowsMedia:
		jmp		LoadFLAC_JumpBack

LoadFLAC_Success:
		test	eax, eax
		mov		[esp+20h+4], eax
		jnz		LoadFLAC_Return_NoDelete

LoadFLAC_Return:
		mov		ecx, esi
		call	CAEDataStreamNew::~CAEDataStreamNew
		push	esi
		call	GTAdelete
		add     esp, 4

LoadFLAC_Return_NoDelete:
		mov     eax, [esp+20h+4]
		mov		ecx, [esp+20h-0Ch]
		pop		esi
		pop		ebp
		pop		edi
		pop		ebx
		mov		fs:0, ecx
		add		esp, 10h
		retn	4
	}
}


void __declspec(naked) LoadFLAC_Steam()
{
	_asm
	{
		jz		LoadFLAC_WindowsMedia
		sub		ebp, 2
		jnz		LoadFLAC_Return
		push	esi
		call	DecoderCtor
		jmp		LoadFLAC_Success

LoadFLAC_WindowsMedia:
		jmp		LoadFLAC_JumpBack

LoadFLAC_Success:
		test	eax, eax
		mov		[esp+20h+4], eax
		jnz		LoadFLAC_Return_NoDelete

LoadFLAC_Return:
		mov		ecx, esi
		call	CAEDataStreamOld::~CAEDataStreamOld
		push	esi
		call	GTAdelete
		add     esp, 4

LoadFLAC_Return_NoDelete:
		mov     eax, [esp+20h+4]
		mov		ecx, [esp+20h-0Ch]
		pop		ebx
		pop		esi
		pop		ebp
		pop		edi
		mov		fs:0, ecx
		add		esp, 10h
		retn	4
	}
}

void __declspec(naked) FLACInit()
{
	_asm
	{
		mov		byte ptr [ecx+0Dh], 1
		jmp		InitializeUtrax
	}
}

void __declspec(naked) FLACInit_Steam()
{
	_asm
	{
		mov		byte ptr [ecx+5], 1
		jmp		InitializeUtrax
	}
}


// Only 1.0/1.01
static void*	HandleMoonStuffStub_JumpBack = AddressByVersion<void*>(0x713D24, 0x714554, 0x72F17F);
void __declspec(naked) HandleMoonStuffStub()
{
	__asm
	{
		mov		eax, [esp + 78h - 64h] // screen x size
		mov		ecx, [esp + 78h - 68h] // screen y size

		push	ecx
		push	eax

		lea		ecx, [esp + 80h - 54h] // screen coord vector

		push	ecx

		push	esi

		call	DrawMoonWithPhases

		add		esp, 10h

		jmp		HandleMoonStuffStub_JumpBack
	}
}

void __declspec(naked) HandleMoonStuffStub_Steam()
{
	__asm
	{
		mov		eax, [esp + 70h - 58h] // screen x size
		mov		ecx, [esp + 70h - 5Ch] // screen y size

		push	ecx
		push	eax

		lea		ecx, [esp + 78h - 48h] // screen coord vector

		push	ecx

		push	esi

		call	DrawMoonWithPhases

		add		esp, 10h

		jmp		HandleMoonStuffStub_JumpBack
	}
}

// 1.0 ONLY BEGINS HERE
static bool			bDarkVehicleThing;
static RpLight**	pDirect;

static void* DarkVehiclesFix1_JumpBack;
void __declspec(naked) DarkVehiclesFix1()
{
	_asm
	{
		shr     eax, 0Eh
		test	al, 1
		jz		DarkVehiclesFix1_DontAppply
		mov		ecx, [pDirect]
		mov		ecx, [ecx]
		mov		al, [ecx+2]
		test	al, 1
		jnz		DarkVehiclesFix1_DontAppply
		mov		bDarkVehicleThing, 1
		jmp		DarkVehiclesFix1_Return

DarkVehiclesFix1_DontAppply:
		mov		bDarkVehicleThing, 0

DarkVehiclesFix1_Return:
		jmp		DarkVehiclesFix1_JumpBack
	}
}

void __declspec(naked) DarkVehiclesFix2()
{
	_asm
	{
		jz		DarkVehiclesFix2_MakeItDark
		mov		al, bDarkVehicleThing
		test	al, al
		jnz		DarkVehiclesFix2_MakeItDark
		mov		eax, 5D9A7Ah
		jmp		eax

DarkVehiclesFix2_MakeItDark:
		mov		eax, 5D9B09h
		jmp		eax
	}
}

void __declspec(naked) DarkVehiclesFix3()
{
	_asm
	{
		jz		DarkVehiclesFix3_MakeItDark
		mov		al, bDarkVehicleThing
		test	al, al
		jnz		DarkVehiclesFix3_MakeItDark
		mov		eax, 5D9B4Ah
		jmp		eax

DarkVehiclesFix3_MakeItDark:
		mov		eax, 5D9CACh
		jmp		eax
	}
}

void __declspec(naked) DarkVehiclesFix4()
{
	_asm
	{
		jz		DarkVehiclesFix4_MakeItDark
		mov		al, bDarkVehicleThing
		test	al, al
		jnz		DarkVehiclesFix4_MakeItDark
		mov		eax, 5D9CB8h
		jmp		eax

DarkVehiclesFix4_MakeItDark:
		mov		eax, 5D9E0Dh
		jmp		eax
	}
}
// 1.0 ONLY ENDS HERE

__declspec(safebuffers) static int _Timers_ftol_internal( double timer, double& remainder )
{
	double integral;
	remainder = modf( timer + remainder, &integral );
	return int(integral);
}

int __stdcall Timers_ftol_PauseMode( double timer )
{
	static double TimersRemainder = 0.0;
	return _Timers_ftol_internal( timer, TimersRemainder );
}

int __stdcall Timers_ftol_NonClipped( double timer )
{
	static double TimersRemainder = 0.0;
	return _Timers_ftol_internal( timer, TimersRemainder );
}

int __stdcall Timers_ftol( double timer )
{
	static double TimersRemainder = 0.0;
	return _Timers_ftol_internal( timer, TimersRemainder );
}

int __stdcall Timers_ftol_SCMdelta( double timer )
{
	static double TimersRemainder = 0.0;
	return _Timers_ftol_internal( timer, TimersRemainder );
}

void __declspec(naked) asmTimers_ftol_PauseMode()
{
	_asm
	{
		sub		esp, 8
		fstp	qword ptr [esp]
		call	Timers_ftol_PauseMode
		retn
	}
}

void __declspec(naked) asmTimers_ftol_NonClipped()
{
	_asm
	{
		sub		esp, 8
		fstp	qword ptr [esp]
		call	Timers_ftol_NonClipped
		retn
	}
}

void __declspec(naked) asmTimers_ftol()
{
	_asm
	{
		sub		esp, 8
		fstp	qword ptr [esp]
		call	Timers_ftol
		retn
	}
}

void __declspec(naked) asmTimers_SCMdelta()
{
	_asm
	{
		sub		esp, 8
		fstp	qword ptr [esp]
		call	Timers_ftol_SCMdelta
		retn
	}
}

void _declspec(naked) FixedCarDamage()
{
	_asm
	{
		fldz
		fcomp	[esp+20h+10h]
		fnstsw  ax
		test    ah, 5
		jp		FixedCarDamage_Negative
		movzx   eax, byte ptr [edi+21h]
		retn

FixedCarDamage_Negative:
		movzx   eax, byte ptr [edi+24h]
		retn
	}
}

void _declspec(naked) FixedCarDamage_Steam()
{
	_asm
	{
		fldz
		fcomp	[esp+20h+10h]
		fnstsw  ax
		test    ah, 5
		jp		FixedCarDamage_Negative
		movzx   eax, byte ptr [edi+21h]
		test	ecx, ecx
		retn

FixedCarDamage_Negative:
		movzx   eax, byte ptr [edi+24h]
		test	ecx, ecx
		retn
	}
}

void _declspec(naked) FixedCarDamage_Newsteam()
{
	_asm
	{
		mov		edi, [ebp+10h]
		fldz
		fcomp	[ebp+14h]
		fnstsw  ax
		test    ah, 5
		jp		FixedCarDamage_Negative
		movzx   eax, byte ptr [edi+21h]
		retn

FixedCarDamage_Negative:
		movzx   eax, byte ptr [edi+24h]
		retn
	}
}

void __declspec(naked) CdStreamThreadHighSize()
{
	_asm
	{
		xor		edx, edx
		shld	edx, ecx, 11
		shl		ecx, 11
		mov		[esi]CdStream.overlapped.Offset, ecx // OVERLAPPED.Offset
		mov		[esi]CdStream.overlapped.OffsetHigh, edx // OVERLAPPED.OffsetHigh

		mov		edx, [esi]CdStream.nSectorsToRead
		retn
	}
}

void __declspec(naked) WeaponRangeMult_VehicleCheck()
{
	_asm
	{
		mov		eax, [edx]CPed.pedFlags
		test    ah, 1
		jz		WeaponRangeMult_VehicleCheck_NotInCar
		mov		eax, [edx]CPed.pVehicle
		retn
	
WeaponRangeMult_VehicleCheck_NotInCar:
		xor		eax, eax
		retn
	}
}


static const float		fSteamSubtitleSizeX = 0.45f;
static const float		fSteamSubtitleSizeY = 0.9f;
static const float		fSteamRadioNamePosY = 33.0f;
static const float		fSteamRadioNameSizeX = 0.4f;
static const float		fSteamRadioNameSizeY = 0.6f;

static float* orgSubtitleSizeX;
static float* orgSubtitleSizeY;
static float* orgRadioNamePosY;
static float* orgRadioNameSizeX;
static float* orgRadioNameSizeY;

static void ToggleSteamTexts( bool enable )
{
	using namespace Memory::VP;

	if ( enable )
	{
		Patch<const void*>(0x58C387, &fSteamSubtitleSizeY);
		Patch<const void*>(0x58C40F, &fSteamSubtitleSizeY);
		Patch<const void*>(0x58C4CE, &fSteamSubtitleSizeY);

		Patch<const void*>(0x58C39D, &fSteamSubtitleSizeX);
		Patch<const void*>(0x58C425, &fSteamSubtitleSizeX);
		Patch<const void*>(0x58C4E4, &fSteamSubtitleSizeX);

		Patch<const void*>(0x4E9FD8, &fSteamRadioNamePosY);
		Patch<const void*>(0x4E9F22, &fSteamRadioNameSizeY);
		Patch<const void*>(0x4E9F38, &fSteamRadioNameSizeX);
	}
	else
	{
		assert( orgSubtitleSizeY != nullptr && orgSubtitleSizeX != nullptr && orgRadioNamePosY != nullptr && orgRadioNameSizeY != nullptr && orgRadioNameSizeX != nullptr );

		Patch<const void*>(0x58C387, orgSubtitleSizeY);
		Patch<const void*>(0x58C40F, orgSubtitleSizeY);
		Patch<const void*>(0x58C4CE, orgSubtitleSizeY);

		Patch<const void*>(0x58C39D, orgSubtitleSizeX);
		Patch<const void*>(0x58C425, orgSubtitleSizeX);
		Patch<const void*>(0x58C4E4, orgSubtitleSizeX);

		Patch<const void*>(0x4E9FD8, orgRadioNamePosY);
		Patch<const void*>(0x4E9F22, orgRadioNameSizeY);
		Patch<const void*>(0x4E9F38, orgRadioNameSizeX);
	}
}

static const double		dRetailSubtitleSizeX = 0.58;
static const double		dRetailSubtitleSizeY = 1.2;
static const double		dRetailSubtitleSizeY2 = 1.22;
static const double		dRetailRadioNamePosY = 22.0;
static const double		dRetailRadioNameSizeX = 0.6;
static const double		dRetailRadioNameSizeY = 0.9;

#pragma comment(lib, "shlwapi.lib")

BOOL InjectDelayedPatches_10()
{
	if ( !IsAlreadyRunning() )
	{
		using namespace Memory;

		const HINSTANCE hInstance = GetModuleHandle( nullptr );
		std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( hInstance, ".text" );
		ScopedUnprotect::Section Protect2( hInstance, ".rdata" );

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];
		GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
		PathRenameExtensionW(wcModulePath, L".ini");

		const ModuleList moduleList;

		const bool		bHasImVehFt = moduleList.Get(L"ImVehFt") != nullptr;
		const bool		bSAMP = moduleList.Get(L"samp") != nullptr;
		const bool		bSARender = moduleList.Get(L"SARender") != nullptr;
		const bool		bOutfit = moduleList.Get(L"outfit") != nullptr;

		if ( bSAMP )
		{
			ModCompat::DetectPlayingOnLSRP();
		}

		const HMODULE skygfxModule = moduleList.Get( L"skygfx" );
		const HMODULE modloaderModule = moduleList.Get( L"modloader" );

		ReadRotorFixExceptions(wcModulePath);
		ReadLightbeamFixExceptions(wcModulePath);
		const bool bHookDoubleRwheels = ReadDoubleRearWheels(wcModulePath);

		const bool bHasDebugMenu = DebugMenuLoad();

#ifdef _DEBUG
		if ( bHasDebugMenu )
		{
			DebugMenuAddVar( "SilentPatch", "LS-RP Mode", &ModCompat::LSRPMode, nullptr );
		}
#endif

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SunSizeHack", -1, wcModulePath) == 1 )
		{
			// PS2 sun - more
			static const float		fSunMult = (1050.0f * 0.95f) / 1500.0f;
			Patch<const void*>(0x6FC5B0, &fSunMult);

			if ( !bSAMP )
			{
				ReadCall( 0x53C136, DoSunAndMoon );
				InjectHook(0x53C136, SunAndMoonFarClip);
				Patch<const void*>(0x6FC5AA, &fSunFarClip);
			}
		}
		
		if ( !bSARender )
		{
			// Twopass rendering (experimental)
			Patch<const void*>(0x7341D9, MovingPropellerRender);
			Patch<const void*>(0x734127, MovingPropellerRender);
			Patch(0x73445E, RenderBigVehicleActomic);


			// Weapons rendering
			if ( !bOutfit )
			{
				InjectHook(0x5E7859, RenderWeapon);
				InjectHook(0x732F30, RenderWeaponPedsForPC, PATCH_JUMP);
			}
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableScriptFixes", -1, wcModulePath) == 1 )
		{
			// Gym glitch fix
			Patch<WORD>(0x470B03, 0xCD8B);
			Patch<DWORD>(0x470B0A, 0x8B04508B);
			Patch<WORD>(0x470B0E, 0x9000);
			Nop(0x470B10, 1);
			InjectHook(0x470B05, &CRunningScript::GetDay_GymGlitch, PATCH_CALL);

			// Basketball fix
			ReadCall( 0x489A70, WipeLocalVariableMemoryForMissionScript );
			ReadCall( 0x5D18F0, TheScriptsLoad );
			InjectHook(0x5D18F0, TheScriptsLoad_BasketballFix);
			// Fixed for Hoodlum
			InjectHook(0x489A70, StartNewMission_SCMFixes);
			InjectHook(0x4899F0, StartNewMission_SCMFixes);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SkipIntroSplashes", -1, wcModulePath) == 1 )
		{
			// Skip the damn intro splash
			Patch<WORD>(AddressByRegion_10<DWORD>(0x748AA8), 0x3DEB);
		}

		{
			static bool bSmallSteamTexts = false;
			if ( bHasDebugMenu )
			{
				orgSubtitleSizeX = *(float**)0x58C39D;
				orgSubtitleSizeY = *(float**)0x58C387;
				orgRadioNamePosY = *(float**)0x4E9FD8;
				orgRadioNameSizeY = *(float**)0x4E9F22;
				orgRadioNameSizeX = *(float**)0x4E9F38;

				DebugMenuAddVar( "SilentPatch", "Small Steam texts", &bSmallSteamTexts, []() {
					ToggleSteamTexts( bSmallSteamTexts );
				} );
			
			}

			if ( GetPrivateProfileIntW(L"SilentPatch", L"SmallSteamTexts", -1, wcModulePath) == 1 )
			{
				// We're on 1.0 - make texts smaller
				ToggleSteamTexts( true );

				bSmallSteamTexts = true;
			}
		}

		if ( const int INIoption = GetPrivateProfileIntW(L"SilentPatch", L"ColouredZoneNames", -1, wcModulePath); INIoption != -1 )
		{
			// Coloured zone names
			bColouredZoneNames = INIoption != 0;

			Patch<WORD>(0x58ADBE, 0x0E75);
			Patch<WORD>(0x58ADC5, 0x0775);

			InjectHook(0x58ADE4, &CRGBA::BlendGangColour_Dynamic);

			if ( bHasDebugMenu )
			{
				DebugMenuAddVar( "SilentPatch", "Coloured zone names", &bColouredZoneNames, nullptr );
			}
		}

		// ImVehFt conflicts
		if ( !bHasImVehFt )
		{
			// Lights
			InjectHook(0x4C830C, LightMaterialsFix, PATCH_CALL);

			// Flying components
			InjectHook(0x59F180, &CObject::Render_Stub, PATCH_JUMP);

			// Cars getting dirty
			// Only 1.0 and Steam
			InjectHook( 0x5D5DB0, RemapDirt, PATCH_JUMP );
			InjectHook(0x4C9648, &CVehicleModelInfo::FindEditableMaterialList, PATCH_CALL);
			Patch<DWORD>(0x4C964D, 0x0FEBCE8B);
		}

		if ( !bHasImVehFt && !bSAMP )
		{
			// Properly random numberplates
			DWORD*		pVMT = *(DWORD**)0x4C75FC;
			Patch(&pVMT[7], &CVehicleModelInfo::Shutdown_Stub);
			Patch<BYTE>(0x6D0E43, 0xEB);
			InjectHook(0x4C9660, &CVehicleModelInfo::SetCarCustomPlate);
			InjectHook(0x6D6A58, &CVehicle::CustomCarPlate_TextureCreate);
			InjectHook(0x6D651C, &CVehicle::CustomCarPlate_BeforeRenderingStart);
			InjectHook(0x6FDFE0, CCustomCarPlateMgr::SetupClumpAfterVehicleUpgrade, PATCH_JUMP);
			//InjectMethodVP(0x6D0E53, CVehicle::CustomCarPlate_AfterRenderingStop, PATCH_NOTHING);
			Nop(0x6D6517, 2);
		}

		// SSE conflicts
		if ( moduleList.Get(L"shadows") == nullptr )
		{
			Patch<DWORD>(0x70665C, 0x52909090);
			InjectHook(0x706662, &CShadowCamera::Update);
		}

		// Bigger streamed entity linked lists
		// Increase only if they're not increased already
		if ( *(DWORD*)0x5B8E55 == 12000 )
		{
			Patch<DWORD>(0x5B8E55, 15000);
			Patch<DWORD>(0x5B8EB0, 15000);
		}

		// Read CCustomCarPlateMgr::GeneratePlateText from here
		// to work fine with Deji's Custom Plate Format
		ReadCall( 0x4C9484, CCustomCarPlateMgr::GeneratePlateText );


		if ( bHookDoubleRwheels )
		{
			// Double rwheels whitelist
			// push ecx
			// push edi
			// call CheckDoubleRWheelsWhitelist
			// test al, al
			Patch<uint16_t>( 0x4C9239, 0x5751 );
			InjectHook( 0x4C9239+2, CheckDoubleRWheelsList, PATCH_CALL );
			Patch<uint16_t>( 0x4C9239+7, 0xC084 );
			Nop( 0x4C9239+9, 1 );
		}

		// Adblocker
#if DISABLE_FLA_DONATION_WINDOW
		if (  moduleList.Get(L"$fastman92limitAdjuster") != nullptr )
		{
			if ( *(DWORD*)0x748736 != 0xE8186A53 )
			{
				Patch<DWORD>(0x748736, 0xE8186A53);
				InjectHook(AddressByRegion_10<int>(0x748739), 0x619B60);
			}
		}
#endif

		if ( *(DWORD*)0x4065BB == 0x3B0BE1C1 )
		{
			// Handle IMGs bigger than 4GB
			Nop( 0x4065BB, 3 );
			Nop( 0x4065C2, 1 );
			InjectHook( 0x4065C2+1, CdStreamThreadHighSize, PATCH_CALL );
			Patch<const void*>( 0x406620+2, &pCdStreamSetFilePointer );
		}


		// Fix directional light position
		if ( const int INIoption = GetPrivateProfileIntW(L"SilentPatch", L"DirectionalFromSun", -1, wcModulePath); INIoption != -1 )
		{
			bUseAaronSun = INIoption != 0;

			ReadCall( 0x53E997, orgSetLightsWithTimeOfDayColour );
			InjectHook( 0x53E997, SetLightsWithTimeOfDayColour_SilentPatch );
			Patch<const void*>( 0x735618 + 2, &curVecToSun.x );
			Patch<const void*>( 0x73561E + 2, &curVecToSun.y );
			Patch<const void*>( 0x735624 + 1, &curVecToSun.z );

			if ( bHasDebugMenu )
			{
				DebugMenuAddVar( "SilentPatch", "Directional from sun", &bUseAaronSun, nullptr );

	#ifndef NDEBUG
				// Switch for fixed PC vehicle lighting
				static bool bFixedPCVehLight = true;
				DebugMenuAddVar( "SilentPatch", "Fixed PC vehicle light", &bFixedPCVehLight, []() {
					if ( bFixedPCVehLight )
					{
						Memory::VP::Patch<float>(0x5D88D1 + 6, 0);
						Memory::VP::Patch<float>(0x5D88DB + 6, 0);
						Memory::VP::Patch<float>(0x5D88E5 + 6, 0);

						Memory::VP::Patch<float>(0x5D88F9 + 6, 0);
						Memory::VP::Patch<float>(0x5D8903 + 6, 0);
						Memory::VP::Patch<float>(0x5D890D + 6, 0);
					}
					else
					{
						Memory::VP::Patch<float>(0x5D88D1 + 6, 0.25f);
						Memory::VP::Patch<float>(0x5D88DB + 6, 0.25f);
						Memory::VP::Patch<float>(0x5D88E5 + 6, 0.25f);

						Memory::VP::Patch<float>(0x5D88F9 + 6, 0.75f);
						Memory::VP::Patch<float>(0x5D8903 + 6, 0.75f);
						Memory::VP::Patch<float>(0x5D890D + 6, 0.75f);
					}
				} );
	#endif
			}
		}

		// Minimal HUD
		if ( const int INIoption = GetPrivateProfileIntW(L"SilentPatch", L"MinimalHud", -1, wcModulePath); INIoption != -1 )
		{
			// Fix original bugs
			Patch( 0x58950E, { 0x90, 0xFF, 0x74, 0x24, 0x1C } );
			InjectHook( 0x58951D, &SetRGBA_FloatAlpha );

			Patch( 0x58D88A, { 0x90, 0xFF, 0x74, 0x24, 0x20 + 0x10 } );
			InjectHook( 0x58D8FD, &RenderXLUSprite_FloatAlpha );
		
			// Re-enable
			if ( INIoption == 1 )
			{
				Patch<int32_t>( 0x588905 + 1, 0 );
			}
		
			if ( bHasDebugMenu )
			{
				static bool bMinimalHUDEnabled = INIoption == 1;
				DebugMenuAddVar( "SilentPatch", "Minimal HUD", &bMinimalHUDEnabled, []() {
					if ( bMinimalHUDEnabled )
					{
						Memory::VP::Patch<int32_t>( 0x588905 + 1, 0 );
					}
					else
					{
						Memory::VP::Patch<int32_t>( 0x588905 + 1, 5 );
					}

					// Call CHud::ReInitialise
					auto ReInitialise = (void(*)())0x588880;
					ReInitialise();
				} );
			}
		}

		// Moonphases
		// Not taking effect with new skygfx since aap has it too now
		if ( !ModCompat::SkygfxPatchesMoonphases( skygfxModule ) )
		{
			InjectHook(0x713ACB, HandleMoonStuffStub, PATCH_JUMP);
		}

		FLAUtils::Init( moduleList );

		// Race condition in CdStream fixed
		// Not taking effect with modloader
		if ( !ModCompat::ModloaderCdStreamRaceConditionAware( modloaderModule ) )
		{
			// Don't patch if old FLA and enhanced IMGs are in place
			// For new FLA, we patch everything except CdStreamThread and then interop with FLA
			const bool flaBugAware = FLAUtils::CdStreamRaceConditionAware();
			const bool usesEnhancedImages = FLAUtils::UsesEnhancedIMGs();

			if ( !usesEnhancedImages || flaBugAware )
			{
				ReadCall( 0x406C78, CdStreamSync::orgCdStreamInitThread );
				InjectHook( 0x406C78, CdStreamSync::CdStreamInitThread );

				{
					uintptr_t address;
					if ( *(uint8_t*)0x406460 == 0xE9 )
					{
						ReadCall( 0x406460, address );
					}
					else
					{
						address = 0x406460;
					}

					const uintptr_t waitForSingleObject = address + 0x1D;
					const uint8_t orgCode[] = { 0x8B, 0x46, 0x04, 0x85, 0xC0, 0x74, 0x10, 0xC6, 0x46, 0x0D, 0x01 };
					if ( memcmp( orgCode, (void*)waitForSingleObject, sizeof(orgCode) ) == 0 )
					{
						VP::Patch( waitForSingleObject, { 0x56, 0xFF, 0x15 } );
						VP::Patch( waitForSingleObject + 3, &CdStreamSync::CdStreamSyncOnObject );
						VP::Patch( waitForSingleObject + 3 + 4, { 0x5E, 0xC3 } );

						{
							const uint8_t orgCode1[] = { 0xFF, 0x15 };
							const uint8_t orgCode2[] = { 0x48, 0xF7, 0xD8 };
							const uintptr_t getOverlappedResult = address + 0x5F;
							if ( memcmp( orgCode1, (void*)getOverlappedResult, sizeof(orgCode1) ) == 0 &&
								memcmp( orgCode2, (void*)(getOverlappedResult + 6), sizeof(orgCode2) ) == 0 )
							{
								VP::Patch( getOverlappedResult + 2, &CdStreamSync::pGetOverlappedResult );
								VP::Patch( getOverlappedResult + 6, { 0x5E, 0xC3 } ); // pop esi / retn
							}
						}
					}
				}

				if ( !usesEnhancedImages )
				{
					Patch( 0x406669, { 0x56, 0xFF, 0x15 } );
					Patch( 0x406669 + 3, &CdStreamSync::CdStreamThreadOnObject );
					Patch( 0x406669 + 3 + 4, { 0xEB, 0x0F } );
				}

				Patch( 0x406910, { 0xFF, 0x15 } );
				Patch( 0x406910 + 2, &CdStreamSync::CdStreamInitializeSyncObject );
				Nop( 0x406910 + 6, 4 );
				Nop( 0x406910 + 0x16, 2 );

				Patch( 0x4063B5, { 0x56, 0x50 } );
				InjectHook( 0x4063B5 + 2, CdStreamSync::CdStreamShutdownSyncObject_Stub, PATCH_CALL );
			}
		}

		// For imfast compatibility
		if ( MemEquals( 0x590ADE, { 0xFF, 0x05 } ) )
		{
			// Modulo over CLoadingScreen::m_currDisplayedSplash
			Nop( 0x590ADE, 1 );
			InjectHook( 0x590ADE + 1, DoPCScreenChange_Mod, PATCH_CALL );
			Patch<const void*>( 0x590042 + 2, &currDisplayedSplash_ForLastSplash );
		}

		// Lightbeam fix debug menu
		if ( bHasDebugMenu )
		{
			static const char * const str[] = { "Off", "Default", "On" };

			DebugMenuEntry *e = DebugMenuAddVar( "SilentPatch", "Rotors fix", &CVehicle::ms_rotorFixOverride, nullptr, 1, -1, 1, str);
			DebugMenuEntrySetWrap(e, true);

			e = DebugMenuAddVar( "SilentPatch", "Lightbeam fix", &CVehicle::ms_lightbeamFixOverride, nullptr, 1, -1, 1, str);
			DebugMenuEntrySetWrap(e, true);
		}

#ifndef NDEBUG
		if ( const int QPCDays = GetPrivateProfileIntW(L"Debug", L"AddDaysToQPC", 0, wcModulePath); QPCDays != 0 )
		{
			using namespace FakeQPC;

			LARGE_INTEGER Freq;
			QueryPerformanceFrequency( &Freq );
			AddedTime = Freq.QuadPart * QPCDays * 60 * 24;

			Patch( 0x8580C8, &FakeQueryPerformanceCounter );
		}
#endif

		return FALSE;
	}
	return TRUE;
}

BOOL InjectDelayedPatches_11()
{
#ifdef NDEBUG
	MessageBoxW( nullptr, L"You're using a 1.01 executable which is no longer supported by SilentPatch!\n\n"
				L"Since this EXE is used by only a few people, I recommend downgrading back to 1.0 - you gain full compatibility with mods "
				L"and any relevant fixes 1.01 brings are backported to 1.0 by SilentPatch anyway.\n\n"
				L"To downgrade to 1.0, just find a 1.0 EXE and use it. If you are having issues with this, feel free to send me an e-mail!",
				L"SilentPatch", MB_OK | MB_ICONWARNING );
#endif

	if ( !IsAlreadyRunning() )
	{
		using namespace Memory;
		const HINSTANCE hInstance = GetModuleHandle( nullptr );
		std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( hInstance, ".text" );
		ScopedUnprotect::Section Protect2( hInstance, ".rdata" );

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];
		GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
		PathRenameExtensionW(wcModulePath, L".ini");

		const ModuleList moduleList;

		bool		bHasImVehFt = moduleList.Get(L"ImVehFt") != nullptr;
		bool		bSAMP = moduleList.Get(L"samp") != nullptr;
		bool		bSARender = moduleList.Get(L"SARender") != nullptr;
		const bool	bOutfit = moduleList.Get(L"outfit") != nullptr;

		if ( bSAMP )
		{
			ModCompat::DetectPlayingOnLSRP();
		}

		ReadRotorFixExceptions(wcModulePath);

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SunSizeHack", -1, wcModulePath) == 1 )
		{
			// PS2 sun - more
			static const float		fSunMult = (1050.0f * 0.95f) / 1500.0f;
			Patch<const void*>(0x6FCDE0, &fSunMult);

			if ( !bSAMP )
			{
				ReadCall( 0x53C5D6, DoSunAndMoon );
				InjectHook(0x53C5D6, SunAndMoonFarClip);

				Patch<const void*>(0x6FCDDA, &fSunFarClip);
			}
		}

		if ( !bSARender )
		{
			// Twopass rendering (experimental)
			Patch<const void*>(0x734A09, MovingPropellerRender);
			Patch<const void*>(0x734957, MovingPropellerRender);
			Patch(0x734C8E, RenderBigVehicleActomic);

			// Weapons rendering
			if ( !bOutfit )
			{
				InjectHook(0x5E8079, RenderWeapon);
				InjectHook(0x733760, RenderWeaponPedsForPC, PATCH_JUMP);
			}
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableScriptFixes", -1, wcModulePath) == 1 )
		{
			// Gym glitch fix
			Patch<WORD>(0x470B83, 0xCD8B);
			Patch<DWORD>(0x470B8A, 0x8B04508B);
			Patch<WORD>(0x470B8E, 0x9000);
			Nop(0x470B90, 1);
			InjectHook(0x470B85, &CRunningScript::GetDay_GymGlitch, PATCH_CALL);

			// Basketball fix
			ReadCall( 0x489AF0, WipeLocalVariableMemoryForMissionScript );
			ReadCall( 0x5D20D0, TheScriptsLoad );
			InjectHook(0x5D20D0, TheScriptsLoad_BasketballFix);
			// Fixed for Hoodlum
			InjectHook(0x489A70, StartNewMission_SCMFixes);
			InjectHook(0x489AF0, StartNewMission_SCMFixes);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SkipIntroSplashes", -1, wcModulePath) == 1 )
		{
			// Skip the damn intro splash
			Patch<WORD>(AddressByRegion_11<DWORD>(0x749388), 0x62EB);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SmallSteamTexts", -1, wcModulePath) == 1 )
		{
			// We're on 1.01 - make texts smaller
			Patch<const void*>(0x58CB57, &fSteamSubtitleSizeY);
			Patch<const void*>(0x58CBDF, &fSteamSubtitleSizeY);
			Patch<const void*>(0x58CC9E, &fSteamSubtitleSizeY);

			Patch<const void*>(0x58CB6D, &fSteamSubtitleSizeX);
			Patch<const void*>(0x58CBF5, &fSteamSubtitleSizeX);
			Patch<const void*>(0x58CCB4, &fSteamSubtitleSizeX);

			Patch<const void*>(0x4EA428, &fSteamRadioNamePosY);
			Patch<const void*>(0x4EA372, &fSteamRadioNameSizeY);
			Patch<const void*>(0x4EA388, &fSteamRadioNameSizeX);
		}

		if ( int INIoption = GetPrivateProfileIntW(L"SilentPatch", L"ColouredZoneNames", -1, wcModulePath); INIoption == 1 )
		{
			// Coloured zone names
			Patch<WORD>(0x58B58E, 0x0E75);
			Patch<WORD>(0x58B595, 0x0775);

			InjectHook(0x58B5B4, &CRGBA::BlendGangColour);
		}
		else if ( INIoption == 0 )
		{
			Patch<BYTE>(0x58B57E, 0xEB);
		}

		// ImVehFt conflicts
		if ( !bHasImVehFt )
		{
			// Lights
			InjectHook(0x4C838C, LightMaterialsFix, PATCH_CALL);

			// Flying components
			InjectHook(0x59F950, &CObject::Render_Stub, PATCH_JUMP);
		}

		if ( !bHasImVehFt && !bSAMP )
		{
			// Properly random numberplates
			DWORD*		pVMT = *(DWORD**)0x4C767C;
			Patch(&pVMT[7], &CVehicleModelInfo::Shutdown_Stub);
			Patch<BYTE>(0x6D1663, 0xEB);
			InjectHook(0x4C984D, &CVehicleModelInfo::SetCarCustomPlate);
			InjectHook(0x6D7288, &CVehicle::CustomCarPlate_TextureCreate);
			InjectHook(0x6D6D4C, &CVehicle::CustomCarPlate_BeforeRenderingStart);
			InjectHook(0x6FE810, CCustomCarPlateMgr::SetupClumpAfterVehicleUpgrade, PATCH_JUMP);
			Nop(0x6D6D47, 2);
		}

		// SSE conflicts
		if ( moduleList.Get(L"shadows") == nullptr )
		{
			Patch<DWORD>(0x706E8C, 0x52909090);
			InjectHook(0x706E92, &CShadowCamera::Update);
		}

		// Bigger streamed entity linked lists
		// Increase only if they're not increased already
		if ( *(DWORD*)0x5B9635 == 12000 )
		{
			Patch<DWORD>(0x5B9635, 15000);
			Patch<DWORD>(0x5B9690, 15000);
		}

		// Read CCustomCarPlateMgr::GeneratePlateText from here
		// to work fine with Deji's Custom Plate Format
		// Albeit 1.01 obfuscates this function
		CCustomCarPlateMgr::GeneratePlateText = (decltype(CCustomCarPlateMgr::GeneratePlateText))0x6FDDE0;
		
		FLAUtils::Init( moduleList );

		return FALSE;
	}
	return TRUE;
}

BOOL InjectDelayedPatches_Steam()
{
#ifdef NDEBUG
	{
		const int messageResult = MessageBoxW( nullptr, L"You're using a 3.0 executable which is no longer supported by SilentPatch!\n\n"
			L"Since this is an old Steam EXE, by now you should have either downgraded to 1.0 or started using an up to date version. It is recommended to "
			L"verify your game's cache on Steam and then downgrade it to 1.0. Do you want to download San Andreas Downgrader now?\n\n"
			L"Pressing Yes will close the game and open your web browser. Press No to proceed to the game anyway.", L"SilentPatch", MB_YESNO | MB_ICONWARNING );
		if ( messageResult == IDYES )
		{
			ShellExecuteW( nullptr, L"open", L"http://gtaforums.com/topic/753764-/", nullptr, nullptr, SW_SHOWNORMAL );
			return TRUE;
		}
	}
#endif

	if ( !IsAlreadyRunning() )
	{
		using namespace Memory;
		const HINSTANCE hInstance = GetModuleHandle( nullptr );
		std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( hInstance, ".text" );
		ScopedUnprotect::Section Protect2( hInstance, ".rdata" );

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];
		GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
		PathRenameExtensionW(wcModulePath, L".ini");

		const ModuleList moduleList;

		bool		bHasImVehFt = moduleList.Get(L"ImVehFt") != nullptr;
		bool		bSAMP = moduleList.Get(L"samp") != nullptr;
		bool		bSARender = moduleList.Get(L"SARender") != nullptr;
		const bool	bOutfit = moduleList.Get(L"outfit") != nullptr;

		if ( bSAMP )
		{
			ModCompat::DetectPlayingOnLSRP();
		}

		ReadRotorFixExceptions(wcModulePath);

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SunSizeHack", -1, wcModulePath) == 1 )
		{
			// PS2 sun - more
			static const double		dSunMult = (1050.0 * 0.95) / 1500.0;
			Patch<const void*>(0x734DF0, &dSunMult);

			if ( !bSAMP )
			{
				ReadCall( 0x54E0B6, DoSunAndMoon );
				InjectHook(0x54E0B6, SunAndMoonFarClip);

				Patch<const void*>(0x734DEA, &fSunFarClip);
			}
		}

		if ( !bSARender )
		{
			// Twopass rendering (experimental)
			Patch<const void*>(0x76E230, MovingPropellerRender);
			Patch<const void*>(0x76E160, MovingPropellerRender);
			Patch(0x76E4F0, RenderBigVehicleActomic);


			// Weapons rendering
			if ( !bOutfit )
			{
				InjectHook(0x604DD9, RenderWeapon);
				InjectHook(0x76D170, RenderWeaponPedsForPC, PATCH_JUMP);
			}
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableScriptFixes", -1, wcModulePath) == 1 )
		{
			// Gym glitch fix
			Patch<WORD>(0x476C2A, 0xCD8B);
			Patch<DWORD>(0x476C31, 0x408B088B);
			Patch<WORD>(0x476C35, 0x9004);
			Nop(0x476C37, 1);
			InjectHook(0x476C2C, &CRunningScript::GetDay_GymGlitch, PATCH_CALL);

			// Basketball fix
			ReadCall( 0x4907AE, WipeLocalVariableMemoryForMissionScript );
			ReadCall( 0x5EE017, TheScriptsLoad );
			InjectHook(0x5EE017, TheScriptsLoad_BasketballFix);
			// Fixed for Hoodlum
			InjectHook(0x4907AE, StartNewMission_SCMFixes);
			InjectHook(0x49072E, StartNewMission_SCMFixes);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SmallSteamTexts", -1, wcModulePath) == 0 )
		{
			// We're on Steam - make texts bigger
			Patch<const void*>(0x59A719, &dRetailSubtitleSizeY);
			Patch<const void*>(0x59A7B7, &dRetailSubtitleSizeY2);
			Patch<const void*>(0x59A8A1, &dRetailSubtitleSizeY2);

			Patch<const void*>(0x59A737, &dRetailSubtitleSizeX);
			Patch<const void*>(0x59A7D5, &dRetailSubtitleSizeX);
			Patch<const void*>(0x59A8BF, &dRetailSubtitleSizeX);

			Patch<const void*>(0x4F5A71, &dRetailRadioNamePosY);
			Patch<const void*>(0x4F59A1, &dRetailRadioNameSizeY);
			Patch<const void*>(0x4F59BF, &dRetailRadioNameSizeX);
		}

		if ( int INIoption = GetPrivateProfileIntW(L"SilentPatch", L"ColouredZoneNames", -1, wcModulePath); INIoption == 1 )
		{
			// Coloured zone names
			Patch<WORD>(0x598F65, 0x0C75);
			Patch<WORD>(0x598F6B, 0x0675);

			InjectHook(0x598F87, &CRGBA::BlendGangColour);
		}
		else if ( INIoption == 0 )
		{
			Patch<BYTE>(0x598F56, 0xEB);
		}

		// ImVehFt conflicts
		if ( !bHasImVehFt )
		{
			// Lights
			InjectHook(0x4D2C06, LightMaterialsFix, PATCH_CALL);

			// Flying components
			InjectHook(0x5B80E0, &CObject::Render_Stub, PATCH_JUMP);

			// Cars getting dirty
			// Only 1.0 and Steam
			InjectHook( 0x5F2580, RemapDirt, PATCH_JUMP );
			InjectHook(0x4D3F4D, &CVehicleModelInfo::FindEditableMaterialList, PATCH_CALL);
			Patch<DWORD>(0x4D3F52, 0x0FEBCE8B);
		}

		if ( !bHasImVehFt && !bSAMP )
		{
			// Properly random numberplates
			DWORD*		pVMT = *(DWORD**)0x4D1E9A;
			Patch(&pVMT[7], &CVehicleModelInfo::Shutdown_Stub);
			Patch<BYTE>(0x70C094, 0xEB);
			InjectHook(0x4D3F65, &CVehicleModelInfo::SetCarCustomPlate);
			InjectHook(0x711F28, &CVehicle::CustomCarPlate_TextureCreate);
			InjectHook(0x71194D, &CVehicle::CustomCarPlate_BeforeRenderingStart);
			InjectHook(0x736BD0, CCustomCarPlateMgr::SetupClumpAfterVehicleUpgrade, PATCH_JUMP);
			//InjectMethodVP(0x6D0E53, CVehicle::CustomCarPlate_AfterRenderingStop, PATCH_NOTHING);
			Nop(0x711948, 2);
		}

		// SSE conflicts
		if ( moduleList.Get(L"shadows") == nullptr )
		{
			Patch<DWORD>(0x74A864, 0x52909090);
			InjectHook(0x74A86A, &CShadowCamera::Update);
		}

		// Bigger streamed entity linked lists
		// Increase only if they're not increased already
		if ( *(DWORD*)0x5D5780 == 12000 )
		{
			Patch<DWORD>(0x5D5720, 1250);
			Patch<DWORD>(0x5D5780, 15000);
		}

		// Read CCustomCarPlateMgr::GeneratePlateText from here
		// to work fine with Deji's Custom Plate Format
		ReadCall( 0x4D3DA4, CCustomCarPlateMgr::GeneratePlateText );
		
		FLAUtils::Init( moduleList );

		return FALSE;
	}
	return TRUE;
}

BOOL InjectDelayedPatches_Newsteam()
{
	if ( !IsAlreadyRunning() )
	{
		using namespace Memory;
		using namespace hook;

		const HINSTANCE hInstance = GetModuleHandle( nullptr );
		std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( hInstance, ".text" );
		ScopedUnprotect::Section Protect2( hInstance, ".rdata" );

		// Race condition in CdStream fixed
		// Not taking effect with modloader
		//if ( !ModCompat::ModloaderCdStreamRaceConditionAware( modloaderModule ) )
		{
			// Don't patch if old FLA and enhanced IMGs are in place
			// For new FLA, we patch everything except CdStreamThread and then interop with FLA
			constexpr bool flaBugAware = false;
			constexpr bool usesEnhancedImages = false;

			if constexpr ( !usesEnhancedImages || flaBugAware )
			{
				void* initThread = get_pattern( "74 14 81 25 ? ? ? ? ? ? ? ? C7 05", 0x16 );

				ReadCall( initThread, CdStreamSync::orgCdStreamInitThread );
				InjectHook( initThread, CdStreamSync::CdStreamInitThread );

				auto cdStreamSync = pattern( "8B 0D ? ? ? ? 8D 04 40 03 C0" ).get_one(); // 0x4064E6

				Patch( cdStreamSync.get<void>( 0x18 ), { 0x56, 0xFF, 0x15 } );
				Patch( cdStreamSync.get<void>( 0x18 + 3 ), &CdStreamSync::CdStreamSyncOnObject );
				Patch( cdStreamSync.get<void>( 0x18 + 3 + 4 ), { 0x5E, 0x5D, 0xC3 } ); // pop ebp / retn

				Patch( cdStreamSync.get<void>( 0x5E + 2 ), &CdStreamSync::pGetOverlappedResult );
				Patch( cdStreamSync.get<void>( 0x5E + 6 ), { 0x5E, 0x5D, 0xC3 } ); // pop esi / pop ebp / retn

				if constexpr ( !usesEnhancedImages )
				{
					auto cdStreamThread = pattern( "C7 46 04 00 00 00 00 8A 4E 0D" ).get_one();

					Patch( cdStreamThread.get<void>(), { 0x56, 0xFF, 0x15 } );
					Patch( cdStreamThread.get<void>( 3 ), &CdStreamSync::CdStreamThreadOnObject );
					Patch( cdStreamThread.get<void>( 3 + 4 ), { 0xEB, 0x17 } );
				}

				auto cdStreamInitThread = pattern( "6A 00 6A 02 6A 00 6A 00 FF D3" ).get_one();
				Patch( cdStreamInitThread.get<void>(), { 0xFF, 0x15 } );
				Patch( cdStreamInitThread.get<void>( 2 ), &CdStreamSync::CdStreamInitializeSyncObject );
				Nop( cdStreamInitThread.get<void>( 6 ), 4 );
				Nop( cdStreamInitThread.get<void>( 0x16 ), 2 );

				auto cdStreamShutdown = pattern( "8B 4C 07 14" ).get_one();
				Patch( cdStreamShutdown.get<void>(), { 0x56, 0x50 } );
				InjectHook( cdStreamShutdown.get<void>( 2 ), CdStreamSync::CdStreamShutdownSyncObject_Stub, PATCH_CALL );
			}
		}


		return FALSE;
	}
	return TRUE;
}

static char		aNoDesktopMode[64];


void Patch_SA_10()
{
	using namespace Memory;

#if MEM_VALIDATORS
	InstallMemValidator();
#endif

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	{
		const uintptr_t pIsAlreadyRunning = AddressByRegion_10<uintptr_t>(0x74872D);
		ReadCall( pIsAlreadyRunning, IsAlreadyRunning );
		InjectHook(pIsAlreadyRunning, InjectDelayedPatches_10);
	}

	// Newsteam crash fix
	pDirect = *(RpLight***)0x5BA573;
	DarkVehiclesFix1_JumpBack = AddressByRegion_10<void*>(0x756D90);

	// (Hopefully) more precise frame limiter
	{
		uintptr_t pAddress = AddressByRegion_10<uintptr_t>(0x748D9B);
		ReadCall( pAddress, RsEventHandler );
		InjectHook(pAddress, NewFrameRender);
		InjectHook(AddressByRegion_10<uintptr_t>(0x748D1F), GetTimeSinceLastFrame);
	}

	// Set CAEDataStream to use an old structure
	CAEDataStream::SetStructType(false);

	//Patch<BYTE>(0x5D7265, 0xEB);

	// Heli rotors
	InjectHook(0x6CAB70, &CPlane::Render_Stub, PATCH_JUMP);
	InjectHook(0x6C4400, &CHeli::Render_Stub, PATCH_JUMP);

	// Boats
	/*Patch<BYTE>(0x4C79DF, 0x19);
	Patch<DWORD>(0x733A87, EXPAND_BOAT_ALPHA_ATOMIC_LISTS * sizeof(AlphaObjectInfo));
	Patch<DWORD>(0x733AD7, EXPAND_BOAT_ALPHA_ATOMIC_LISTS * sizeof(AlphaObjectInfo));*/

	// Fixed strafing? Hopefully
	/*static const float		fStrafeCheck = 0.1f;
	Patch<const void*>(0x61E0C2, &fStrafeCheck);
	Nop(0x61E0CA, 6);*/

	// RefFix
	static const float						fRefZVal = 1.0f;
	static const float* const				pRefFal = &fRefZVal;

	Patch<const void*>(0x6FB97A, &pRefFal);
	Patch<BYTE>(0x6FB9A0, 0);

	// Plane rotors
	InjectHook(0x4C7981, PlaneAtomicRendererSetup, PATCH_JUMP);

	// DOUBLE_RWHEELS
	Patch<WORD>(0x4C9290, 0xE281);
	Patch<int>(0x4C9292, ~(rwMATRIXTYPEMASK|rwMATRIXINTERNALIDENTITY));

	// A fix for DOUBLE_RWHEELS trailers
	InjectHook(0x4C9223, TrailerDoubleRWheelsFix, PATCH_JUMP);
	InjectHook(0x4C92F4, TrailerDoubleRWheelsFix2, PATCH_JUMP);

	// No framedelay
	Patch<WORD>(0x53E923, 0x43EB);
	Patch<BYTE>(0x53E99F, 0x10);
	Nop(0x53E9A5, 1);

	// Disable re-initialization of DirectInput mouse device by the game
	Patch<BYTE>(0x576CCC, 0xEB);
	Patch<BYTE>(0x576EBA, 0xEB);
	Patch<BYTE>(0x576F8A, 0xEB);

	// Make sure DirectInput mouse device is set non-exclusive (may not be needed?)
	Patch<DWORD>(AddressByRegion_10<DWORD>(0x7469A0), 0x9090C030);

	// Hunter interior & static_rotor for helis
	InjectHook(0x4C78F2, HunterTest, PATCH_JUMP);
	InjectHook(0x4C9618, CacheCRC32);

	// Fixed blown up car rendering
	// ONLY 1.0
	InjectHook(0x5D993F, DarkVehiclesFix1);
	InjectHook(0x5D9A74, DarkVehiclesFix2, PATCH_JUMP);
	InjectHook(0x5D9B44, DarkVehiclesFix3, PATCH_JUMP);
	InjectHook(0x5D9CB2, DarkVehiclesFix4, PATCH_JUMP);

	// Bindable NUM5
	// Only 1.0 and Steam
	Nop(0x57DC55, 2);


	// TEMP
	//Patch<DWORD>(0x733B05, 40);
	//Patch<DWORD>(0x733B55, 40);
	//Patch<BYTE>(0x5B3ADD, 4);

	// Lightbeam fix
	{
		using namespace LightbeamFix;

		ReadCall( 0x6A2EDA, CVehicle::orgDoHeadLightBeam );
		InjectHook( 0x6A2EDA, &CVehicle::DoHeadLightBeam_LightBeamFixSaveObj );
		InjectHook( 0x6A2EF2, &CVehicle::DoHeadLightBeam_LightBeamFixSaveObj );
		InjectHook( 0x6BDE80, &CVehicle::DoHeadLightBeam_LightBeamFixSaveObj );

		Patch( 0x6E0F37 + 2, &RenderStateWrapper<rwRENDERSTATEZWRITEENABLE>::PushStatePPtr );
		Patch( 0x6E0F63 + 1, &RenderStateWrapper<rwRENDERSTATEZTESTENABLE>::PushStatePPtr );
		Patch( 0x6E0F6F + 2, &RenderStateWrapper<rwRENDERSTATEVERTEXALPHAENABLE>::PushStatePPtr );
		Patch( 0x6E0F7C + 2, &RenderStateWrapper<rwRENDERSTATESRCBLEND>::PushStatePPtr );
		Patch( 0x6E0F89 + 1, &RenderStateWrapper<rwRENDERSTATEDESTBLEND>::PushStatePPtr );
		Patch( 0x6E0F95 + 2, &RenderStateWrapper<rwRENDERSTATESHADEMODE>::PushStatePPtr );
		// rwRENDERSTATETEXTURERASTER not saved
		Patch( 0x6E0FAF + 1, &RenderStateWrapper<rwRENDERSTATECULLMODE>::PushStatePPtr );
		Patch( 0x6E0FBB + 2, &RenderStateWrapper<rwRENDERSTATEALPHATESTFUNCTION>::PushStatePPtr );
		Patch( 0x6E0FCB + 2, &RenderStateWrapper<rwRENDERSTATEALPHATESTFUNCTIONREF>::PushStatePPtr );

		// rwRENDERSTATETEXTURERASTER not saved
		Patch( 0x6E13E0 + 2, &RenderStateWrapper<rwRENDERSTATEZWRITEENABLE>::PopStatePPtr );
		Patch( 0x6E13ED + 2, &RenderStateWrapper<rwRENDERSTATEZTESTENABLE>::PopStatePPtr );
		Patch( 0x6E13FA + 1, &RenderStateWrapper<rwRENDERSTATESRCBLEND>::PopStatePPtr );
		Patch( 0x6E1406 + 2, &RenderStateWrapper<rwRENDERSTATEDESTBLEND>::PopStatePPtr );
		Patch( 0x6E1413 + 2, &RenderStateWrapper<rwRENDERSTATEVERTEXALPHAENABLE>::PopStatePPtr );
		Patch( 0x6E141F + 1, &RenderStateWrapper<rwRENDERSTATECULLMODE>::PopStatePPtr );

		// Debug override registered in delayed patches
	}

	// PS2 SUN!!!!!!!!!!!!!!!!!
	Nop(0x6FB17C, 3);

#if defined EXPAND_ALPHA_ENTITY_LISTS
	// Bigger alpha entity lists
	Patch<DWORD>(0x733B05, EXPAND_ALPHA_ENTITY_LISTS * 20);
	Patch<DWORD>(0x733B55, EXPAND_ALPHA_ENTITY_LISTS * 20);
#endif

	// Unlocked widescreen resolutions
	//Patch<DWORD>(0x745B71, 0x9090687D);
	Patch<DWORD>(0x745B81, 0x9090587D);
	Patch<DWORD>(0x74596C, 0x9090127D);
	Nop(0x745970, 2);
	//Nop(0x745B75, 2);
	Nop(0x745B85, 2);
	Nop(0x7459E1, 2);

	// Heap corruption fix
	Nop(0x5C25D3, 5);

	// User Tracks fix
	ReadCall( 0x4D9B66, SetVolume );
	InjectHook(0x4D9B66, UserTracksFix);
	InjectHook(0x4D9BB5, 0x4F2FD0);

	// FLAC support
	InjectHook(0x4F373D, LoadFLAC, PATCH_JUMP);
	InjectHook(0x57BEFE, FLACInit);
	InjectHook(0x4F3787, CAEWaveDecoderInit);

	Patch<WORD>(0x4F376A, 0x18EB);
	//Patch<BYTE>(0x4F378F, sizeof(CAEWaveDecoder));
	Patch<const void*>(0x4F3210, UserTrackExtensions);
	Patch<const void*>(0x4F3241, &UserTrackExtensions->Codec);
	Patch<const void*>(0x4F35E7, &UserTrackExtensions[1].Codec);
	Patch<BYTE>(0x4F322D, sizeof(UserTrackExtensions));

	// Impound garages working correctly
	InjectHook(0x425179, 0x448990); // CGarages::IsPointWithinAnyGarage
	InjectHook(0x425369, 0x448990); // CGarages::IsPointWithinAnyGarage
	InjectHook(0x425411, 0x448990); // CGarages::IsPointWithinAnyGarage

	// Impounding after busted works
	Nop(0x443292, 5);

	// Mouse rotates an airbone car only with Steer with Mouse option enabled
	bool*	bEnableMouseSteering = *(bool**)0x6AD7AD; // CVehicle::m_bEnableMouseSteering
	Patch<bool*>(0x6B4EC0, bEnableMouseSteering);
	Patch<bool*>(0x6CE827, bEnableMouseSteering);

	// Patched CAutomobile::Fix
	// misc_x parts don't get reset (Bandito fix), Towtruck's bouncing panel is not reset
	Patch<WORD>(0x6A34C9, 0x5EEB);
	Patch<DWORD>(0x6A3555, 0x5E5FCF8B);
	Patch<DWORD>(0x6A3559, 0x448B5B5D);
	Patch<DWORD>(0x6A355D, 0x89644824);
	Patch<DWORD>(0x6A3561, 5);
	Patch<DWORD>(0x6A3565, 0x54C48300);
	InjectHook(0x6A3569, &CAutomobile::Fix_SilentPatch, PATCH_JUMP);

	// Patched CPlane::Fix
	// Doors don't get reset (they can't get damaged anyway), bouncing panels DO reset
	// but not on Vortex
	Patch<BYTE>(0x6CABD0, 0xEB);
	Patch<DWORD>(0x6CAC05, 0x5E5FCF8B);
	InjectHook(0x6CAC09, &CPlane::Fix_SilentPatch, PATCH_JUMP);

	// Weapon icon fix (crosshairs mess up rwRENDERSTATEZWRITEENABLE)
	// Only 1.0 and 1.01, Steam somehow fixed it (not the same way though)
	Nop(0x58E210, 3);
	Nop(0x58EAB7, 3);
	Nop(0x58EAE1, 3);	

	// Zones fix
	// Only 1.0 and Steam
	InjectHook(0x572130, GetCurrentZoneLockedOrUnlocked, PATCH_JUMP);

	// Bilinear filtering for license plates
	//Patch<BYTE>(0x6FD528, rwFILTERLINEAR);
	Patch<BYTE>(0x6FDF47, rwFILTERLINEAR);

	// -//- Roadsign maganer
	//Patch<BYTE>(0x6FE147, rwFILTERLINEAR);

	// Bilinear filtering with mipmaps for weapon icons
	Patch<BYTE>(0x58D7DA, rwFILTERMIPLINEAR);

	// Illumination value from timecyc.dat properly using floats
	Patch<WORD>(0x5BBFC9, 0x14EB);

	// Illumination defaults to 1.0
	Patch<DWORD>(0x5BBB04, 0xCC2484C7);
	Patch<DWORD>(0x5BBB08, 0x00000000);
	Patch<DWORD>(0x5BBB0C, 0x903F8000);

	// All lights get casted at vehicles
	Patch<BYTE>(0x5D9A88, 8);
	Patch<BYTE>(0x5D9A91, 8);
	Patch<BYTE>(0x5D9F1F, 8);

	// 6 extra directionals on Medium and higher
	// push eax
	// call GetMaxExtraDirectionals
	// add esp, 4
	// mov ebx, eax
	// nop
	Patch<uint8_t>( 0x735881, 0x50 );
	InjectHook( 0x735881 + 1, GetMaxExtraDirectionals, PATCH_CALL );
	Patch( 0x735881 + 6, { 0x83, 0xC4, 0x04, 0x8B, 0xD8 } );
	Nop( 0x735881 + 11, 3 );

	// Default resolution to native resolution
	RECT			desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

	Patch<DWORD>(0x746363, desktop.right);
	Patch<DWORD>(0x746368, desktop.bottom);
	Patch<const char*>(0x7463C8, aNoDesktopMode);

	// Corrected Map screen 1px issue
	Patch<float>(0x575DE7, -0.5f);
	Patch<float>(0x575DA7, -0.5f);
	Patch<float>(0x575DAF, -0.5f);
	Patch<float>(0x575D5C, -0.5f);
	Patch<float>(0x575CDA, -0.5f);
	Patch<float>(0x575D0C, -0.5f);

	// Cars drive on water cheat
	Patch<DWORD>(&(*(DWORD**)0x438513)[34], 0xE5FC92C3);

	// No DirectPlay dependency
	Patch<BYTE>(AddressByRegion_10<DWORD>(0x74754A), 0xB8);
	Patch<DWORD>(AddressByRegion_10<DWORD>(0x74754B), 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x744FB0, GetMyDocumentsPathSA, PATCH_JUMP);

	// Fixed muzzleflash not showing from last bullet
	Nop(0x61ECE4, 2);

	// Proper randomizations
	InjectHook(0x44E82E, Int32Rand); // Missing ped paths
	InjectHook(0x44ECEE, Int32Rand); // Missing ped paths
	InjectHook(0x666EA0, Int32Rand); // Prostitutes

	// Help boxes showing with big message
	// Game seems to assume they can show together
	Nop(0x58BA8F, 6);

	// Fixed lens flare
	Patch<DWORD>(0x70F45A, 0);
	Patch<BYTE>(0x6FB621, 0xC3);
	Patch<BYTE>(0x6FB600, 0x21);
	InjectHook(0x6FB622, 0x70CF20, PATCH_CALL);
	Patch<WORD>(0x6FB627, 0xDCEB);

	Patch<WORD>(0x6FB476, 0xB990);
	Patch(0x6FB478, &FlushLensSwitchZ);
	Patch<WORD>(0x6FB480, 0xD1FF);
	Nop(0x6FB482, 1);

	Patch<WORD>(0x6FAF28, 0xB990);
	Patch(0x6FAF2A, &InitBufferSwitchZ);
	Patch<WORD>(0x6FAF32, 0xD1FF);
	Nop(0x6FAF34, 1);

	// Y axis sensitivity fix
	// By ThirteenAG
	float* sens = *(float**)0x50F03C;
	Patch<const void*>(0x50EB70 + 0x4D6 + 0x2, sens);
	Patch<const void*>(0x50F970 + 0x1B6 + 0x2, sens);
	Patch<const void*>(0x5105C0 + 0x666 + 0x2, sens);
	Patch<const void*>(0x511B50 + 0x2B8 + 0x2, sens);
	Patch<const void*>(0x521500 + 0xD8C + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<WORD>(0x50FBB4, 0x27EB);
	Patch<WORD>(0x510512, 0xE990);
	InjectHook(0x524071, 0x524139, PATCH_JUMP);

	// Fixed mirrors crash
	Patch<uint64_t>(0x7271CB, 0xC604C4833474C085);

	// Mirrors depth fix & bumped quality
	InjectHook(0x72701D, CreateMirrorBuffers);

	// Fixed MSAA options
	Patch<BYTE>(0x57D126, 0xEB);
	Nop(0x57D0E8, 2);

	Patch<BYTE>(AddressByRegion_10<BYTE*>(0x7F6C9B), 0xEB);
	Patch<BYTE>(AddressByRegion_10<BYTE*>(0x7F60C6), 0xEB);
	Patch<WORD>(AddressByRegion_10<BYTE*>(0x7F6683), 0xE990);

	ReadCall( 0x57D136, orgGetMaxMultiSamplingLevels );
	InjectHook(0x57D136, GetMaxMultiSamplingLevels);
	InjectHook(0x57D0EA, GetMaxMultiSamplingLevels);

	ReadCall( 0x5744FD, orgChangeMultiSamplingLevels );
	InjectHook(0x5744FD, ChangeMultiSamplingLevels);
	InjectHook(0x57D162, ChangeMultiSamplingLevels);
	InjectHook(0x57D2A6, ChangeMultiSamplingLevels);

	ReadCall( 0x746350, orgSetMultiSamplingLevels );
	InjectHook(0x746350, SetMultiSamplingLevels);

	Nop(0x57A0FC, 1);
	InjectHook(0x57A0FD, MSAAText, PATCH_CALL);

	
	// Fixed car collisions - car you're hitting gets proper damage now
	InjectHook(0x5428EA, FixedCarDamage, PATCH_CALL);


	// Car explosion crash with multimonitor
	// Unitialized collision data breaking stencil shadows
	{
		uintptr_t pHoodlumCompat;
		if ( *(uint8_t*)0x40F870 == 0xE9 )
			ReadCall( 0x40F870, pHoodlumCompat );
		else
			pHoodlumCompat = 0x40F870;

		const uintptr_t pMemMgrMalloc = pHoodlumCompat + 0x63;
		ReadCall( pMemMgrMalloc, orgMemMgrMalloc );
		VP::InjectHook(pMemMgrMalloc, CollisionData_MallocAndInit);
	}
	{
		uintptr_t pHoodlumCompat, pHoodlumCompat2;
		if ( *(uint8_t*)0x40F740 == 0xE9 )
		{
			ReadCall( 0x40F740, pHoodlumCompat );
			ReadCall( 0x40F810, pHoodlumCompat2 );
		}
		else
		{
			pHoodlumCompat = 0x40F740;
			pHoodlumCompat2 = 0x40F810;
		}

		const uintptr_t pNewAlloc = pHoodlumCompat + 0xC;
		ReadCall( pNewAlloc, orgNewAlloc );
		VP::InjectHook(pHoodlumCompat + 0xC, CollisionData_NewAndInit);
		VP::InjectHook(pHoodlumCompat2 + 0xD, CollisionData_NewAndInit);
	}


	// Crash when entering advanced display options on a dual monitor machine after:
	// - starting game on primary monitor in maximum resolution, exiting,
	// starting again in maximum resolution on secondary monitor.
	// Secondary monitor maximum resolution had to be greater than maximum resolution of primary monitor.
	// Not in 1.01
	ReadCall( 0x745B1E, orgGetNumVideoModes );
	InjectHook(0x745B1E, GetNumVideoModes_Store);
	InjectHook(0x745A81, GetNumVideoModes_Retrieve);


	// Fixed escalators crash
	ReadCall( 0x7185B5, orgEscalatorsUpdate );
	InjectHook(0x7185B5, UpdateEscalators);
	InjectHook(0x71791F, &CEscalator::SwitchOffNoRemove);


	// Don't allocate constant memory for stencil shadows every frame
	InjectHook(0x711DD5, StencilShadowAlloc, PATCH_CALL);
	Nop(0x711E0D, 3);
	Patch<WORD>(0x711DDA, 0x2CEB);
	Patch<DWORD>(0x711E5F, 0x90C35D5F);	// pop edi, pop ebp, ret


	// "Streaming memory bug" fix
	InjectHook(0x4C51A9, GTARtAnimInterpolatorSetCurrentAnim);

	
	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x43890B+1, 1); // knife
	Patch<BYTE>(0x4389F8+1, 1); // knife
	Patch<BYTE>(0x438B9F+1, 1); // chainsaw
	Patch<BYTE>(0x438C58+1, 1); // chainsaw
	Patch<BYTE>(0x4395C8+1, 1); // parachute

	Patch<BYTE>(0x439F1F, 0x53); // katana
	Patch<WORD>(0x439F20, 0x016A);


	// Fixed police scanner names
	char*			pScannerNames = *(char**)0x4E72D4;
	strcpy_s(pScannerNames + (8*113), 8, "WESTP");
	strcpy_s(pScannerNames + (8*134), 8, "????");


	// AI accuracy issue
	Nop(0x73B3AE, 1);
	InjectHook( 0x73B3AE + 1, WeaponRangeMult_VehicleCheck, PATCH_CALL );


	// New timers fix
	InjectHook( 0x561C32, asmTimers_ftol_PauseMode );
	InjectHook( 0x561902, asmTimers_ftol_NonClipped );
	InjectHook( 0x56191A, asmTimers_ftol );
	InjectHook( 0x46A036, asmTimers_SCMdelta );


	// Don't catch WM_SYSKEYDOWN and WM_SYSKEYUP (fixes Alt+F4)
	InjectHook( AddressByRegion_10<int>(0x748220), AddressByRegion_10<int>(0x748446), PATCH_JUMP );
	Patch<uint8_t>( AddressByRegion_10<int>(0x7481E3), 0x5C ); // esi -> ebx
	Patch<uint8_t>( AddressByRegion_10<int>(0x7481EA), 0x53 ); // esi -> ebx
	Patch<uint8_t>( AddressByRegion_10<int>(0x74820D), 0xFB ); // esi -> ebx
	Patch<int8_t>( AddressByRegion_10<int>(0x7481EF), 0x54-0x3C ); // use stack space for new lParam
	Patch<int8_t>( AddressByRegion_10<int>(0x748200), 0x4C-0x3C ); // use stack space for new lParam
	Patch<int8_t>( AddressByRegion_10<int>(0x748214), 0x4C-0x3C ); // use stack space for new lParam

	InjectHook( AddressByRegion_10<int>(0x74826A), AddressByRegion_10<int>(0x748446), PATCH_JUMP );
	Patch<uint8_t>( AddressByRegion_10<int>(0x74822D), 0x5C ); // esi -> ebx
	Patch<uint8_t>( AddressByRegion_10<int>(0x748234), 0x53 ); // esi -> ebx
	Patch<uint8_t>( AddressByRegion_10<int>(0x748257), 0xFB ); // esi -> ebx
	Patch<int8_t>( AddressByRegion_10<int>(0x748239), 0x54-0x3C ); // use stack space for new lParam
	Patch<int8_t>( AddressByRegion_10<int>(0x74824A), 0x4C-0x3C ); // use stack space for new lParam
	Patch<int8_t>( AddressByRegion_10<int>(0x74825E), 0x4C-0x3C ); // use stack space for new lParam


	// FuckCarCompletely not fixing panels
	Nop(0x6C268D, 3);


	// 014C cargen counter fix (by spaceeinstein)
	Patch<uint8_t>( 0x06F3E2C + 1, 0xBF ); // movzx ecx, ax -> movsx ecx, ax
	Patch<uint8_t>( 0x6F3E32, 0x74 ); // jge -> jz


	// Linear filtering on script sprites
	ReadCall( 0x58C092, orgDrawScriptSpritesAndRectangles );
	InjectHook( 0x58C092, DrawScriptSpritesAndRectangles );


	// Properly initialize all CVehicleModelInfo fields
	ReadCall( 0x4C75E4, orgVehicleModelInfoCtor );
	InjectHook( 0x4C75E4, VehicleModelInfoCtor );


	// Animated Phoenix hood scoop
	auto* automobilePreRender = (*(decltype(CAutomobile::orgAutomobilePreRender)**)(0x6B0AD2 + 2)) + 17;
	CAutomobile::orgAutomobilePreRender = *automobilePreRender;
	Patch(automobilePreRender, &CAutomobile::PreRender_Stub);

	InjectHook(0x6C7E7A, &CAutomobile::PreRender_Stub);
	InjectHook(0x6CEAEC, &CAutomobile::PreRender_Stub);
	InjectHook(0x6CFADC, &CAutomobile::PreRender_Stub);


	// Extra animations for planes
	auto* planePreRender = (*(decltype(CPlane::orgPlanePreRender)**)(0x6C8E5A + 2)) + 17;
	CPlane::orgPlanePreRender = *planePreRender;
	Patch(planePreRender, &CPlane::PreRender_Stub);


	// Stop BF Injection/Bandito/Hotknife rotating engine components when engine is off
	Patch<const void*>(0x6AC2BE + 2, &CAutomobile::ms_engineCompSpeed);
	Patch<const void*>(0x6ACB91 + 2, &CAutomobile::ms_engineCompSpeed);


	// Make freeing temp objects more aggressive to fix vending crash
	InjectHook( 0x5A1840, CObject::TryToFreeUpTempObjects_SilentPatch, PATCH_JUMP );


	// Remove FILE_FLAG_NO_BUFFERING from CdStreams
	Patch<uint8_t>( 0x406BC6, 0xEB );


	// Proper metric-imperial conversion constants
	static const float METERS_TO_FEET = 3.280839895f;
	Patch<const void*>( 0x55942F + 2, &METERS_TO_FEET );
	Patch<const void*>( 0x55AA96 + 2, &METERS_TO_FEET );


	// Fixed impounding of random vehicles (because CVehicle::~CVehicle doesn't remove cars from apCarsToKeep)
	ReadCall( 0x6E2B6E, orgRecordVehicleDeleted );
	InjectHook( 0x6E2B6E, RecordVehicleDeleted_AndRemoveFromVehicleList );


	// Don't include an extra D3DLIGHT on vehicles since we fixed directional already
	// By aap
	Patch<float>(0x5D88D1 + 6, 0);
	Patch<float>(0x5D88DB + 6, 0);
	Patch<float>(0x5D88E5 + 6, 0);

	Patch<float>(0x5D88F9 + 6, 0);
	Patch<float>(0x5D8903 + 6, 0);
	Patch<float>(0x5D890D + 6, 0);


	// Fixed CAEAudioUtility timers - not typecasting to float so we're not losing precision after X days of PC uptime
	// Also fixed integer division by zero
	Patch( 0x5B9868 + 2, &pAudioUtilsFrequency );
	InjectHook( 0x5B9886, AudioUtilsGetStartTime );
	InjectHook( 0x4D9E80, AudioUtilsGetCurrentTimeInMs, PATCH_JUMP );


	// Car generators placed in interiors visible everywhere
	InjectHook( 0x6F3B30, &CEntity::SetPositionAndAreaCode );


	// Fixed bomb ownership/bombs saving for bikes
	{
		ReadCall( 0x44856A, CStoredCar::orgRestoreCar );
		InjectHook( 0x44856A, &CStoredCar::RestoreCar_SilentPatch );
		InjectHook( 0x4485DB, &CStoredCar::RestoreCar_SilentPatch );
	}


	// unnamed CdStream semaphore
	Patch( 0x406945, { 0x6A, 0x00 } ); // push 0 \ nop
	Nop( 0x406945 + 2, 3 );


	// Correct streaming when using RC vehicles
	InjectHook( 0x55574B, FindPlayerEntityWithRC );
	InjectHook( 0x5557C3, FindPlayerVehicle_RCWrap );


	// TODO: Verify this fix, might be causing crashes atm and too risky to include
#if 0
	// Fixed CPlayerInfo assignment operator
	InjectHook( 0x45DEF0, &CPlayerInfo::operator=, PATCH_JUMP );
#endif


	// Fixed triangle above recruitable peds' heads
	Patch<uint8_t>( 0x60BC52 + 2, 8 ); // GANG2


	// Credits =)
	ReadCall( 0x5AF87A, Credits::PrintCreditText );
	ReadCall( 0x5AF8A4, Credits::PrintCreditText_Hooked );
	InjectHook( 0x5AF8A4, Credits::PrintSPCredits );


	// Fixed ammo from SCM
	{
		ReadCall( 0x47D335, CPed::orgGiveWeapon );
		InjectHook( 0x47D335, &CPed::GiveWeapon_SP );
	}

	// Fixed bicycle on fire - instead of CJ being set on fire, bicycle's driver is
	{
		using namespace BicycleFire;

		Patch( 0x53A984, { 0x90, 0x57 } ); // nop \ push edi
		Patch( 0x53A9A7, { 0x90, 0x57 } ); // nop \ push edi
		InjectHook( 0x53A984 + 2, GetVehicleDriver );
		InjectHook( 0x53A9A7 + 2, GetVehicleDriver );

		ReadCall( 0x53A990, CPlayerPed::orgDoStuffToGoOnFire );
		InjectHook( 0x53A990, DoStuffToGoOnFire_NullAndPlayerCheck );

		ReadCall( 0x53A9B7, CFireManager::orgStartFire );
		InjectHook( 0x53A9B7, &CFireManager::StartFire_NullEntityCheck );
	}

	
	// Decreased keyboard input latency
	{
		using namespace KeyboardInputFix;

		NewKeyState = *(void**)( 0x541E21 + 1 );
		OldKeyState = *(void**)( 0x541E26 + 1 );
		TempKeyState = *(void**)( 0x541E32 + 1 );
		objSize = *(uint32_t*)( 0x541E1C + 1 ) * 4;

		ReadCall( 0x541DEB, orgClearSimButtonPressCheckers );
		InjectHook( 0x541DEB, ClearSimButtonPressCheckers );
		Nop( 0x541E2B, 2 );
		Nop( 0x541E3C, 2 );
	}


	// Fixed handling.cfg name matching (names don't need unique prefixes anymore)
	{
		using namespace HandlingNameLoadFix;

		InjectHook( 0x6F4F58, strncpy_Fix );
		InjectHook( 0x6F4F64, strncmp_Fix );
	}


	// Firela animations
	{
		using namespace FirelaHook;
	
		UpdateMovingCollisionJmp = 0x6B200F;
		HydraulicControlJmpBack = 0x6B1FBF + 10;
		InjectHook( 0x6B1FBF, TestFirelaAndFlags, PATCH_JUMP );

		FollowCarCamNoMovement = 0x52551E;
		FollowCarCamJmpBack = 0x5254F6 + 6;
		InjectHook( 0x5254F6, CamControlFirela, PATCH_JUMP );
	}


	// Double artict3 trailer
	{
		auto* trailerTowBarPos = (*(decltype(CTrailer::orgGetTowBarPos)**)(0x6D03FD + 2)) + 60;
		CTrailer::orgGetTowBarPos = *trailerTowBarPos;
		Patch(trailerTowBarPos, &CTrailer::GetTowBarPos_Stub);
	}


	// DFT-30 wheel, Sweeper brushes and other typos in hierarchy
	InjectHook( 0x4C5311, HierarchyTypoFix::strcasecmp );


	// Tug tow bar (misc_b instead of misc_a
	Nop( 0x6AF2CC, 1 );
	InjectHook( 0x6AF2CC + 1, &CAutomobile::GetTowBarFrame, PATCH_CALL );


	// Play passenger's voice lines when killing peds with car, not only when hitting them damages player's vehicle
	ReadCall( 0x5F05CA, CEntity::orgGetColModel );
	InjectHook( 0x5F05CA, &CVehicle::PlayPedHitSample_GetColModel );
	// Prevent samples from playing where they used to, so passengers don't comment on gently pushing peds
	InjectHook( 0x6A8298, &CPed::Say_SampleBlackList<CONTEXT_GLOBAL_CAR_HIT_PED> );


	// Reset variables on New Game
	{
		using namespace VariableResets;

		ReadCall( 0x53C6DB, orgReInitGameObjectVariables );
		InjectHook( 0x53C6DB, ReInitGameObjectVariables );
		InjectHook( 0x53C76D, ReInitGameObjectVariables );

		// Variables to reset
		GameVariablesToReset.emplace_back( *(bool**)(0x63E8D8+1) ); // CPlayerPed::bHasDisplayedPlayerQuitEnterCarHelpText
		GameVariablesToReset.emplace_back( *(bool**)(0x44AC97+1) ); // CGarages::RespraysAreFree
		GameVariablesToReset.emplace_back( *(bool**)(0x44B49D+1) ); // CGarages::BombsAreFree

		GameVariablesToReset.emplace_back( *(int**)(0x42131F + 2) ); // CCarCtrl::LastTimeFireTruckCreated
		GameVariablesToReset.emplace_back( *(int**)(0x421319 + 2) ); // CCarCtrl::LastTimeAmbulanceCreated

		GameVariablesToReset.emplace_back( *(int**)(0x55C843 + 1) ); // CStats::m_CycleSkillCounter
		GameVariablesToReset.emplace_back( *(int**)(0x55CA39 + 1) ); // CStats::m_SwimUnderWaterCounter
		GameVariablesToReset.emplace_back( *(int**)(0x55CF3E + 2) ); // CStats::m_WeaponCounter
		GameVariablesToReset.emplace_back( *(int**)(0x55CF2A + 2) ); // CStats::m_LastWeaponTypeFired
		GameVariablesToReset.emplace_back( *(int**)(0x55CFC1 + 1) ); // CStats::m_DeathCounter
		GameVariablesToReset.emplace_back( *(int**)(0x55C5E5 + 1) ); // CStats::m_MaxHealthCounter
		GameVariablesToReset.emplace_back( *(int**)(0x55D043 + 1) ); // CStats::m_AddToHealthCounter

		// Non-zero inits still need to be done
		GameVariablesToReset.emplace_back( *(TimeNextMadDriverChaseCreated_t<float>**)(0x421369 + 2) ); // CCarCtrl::TimeNextMadDriverChaseCreated
	}

	// Don't clean the car BEFORE Pay 'n Spray doors close, as it gets cleaned later again anyway!
	Nop( 0x44ACDC, 6 );


#if FULL_PRECISION_D3D
	// Test - full precision D3D device
	Patch<uint8_t>( 0x7F672B+1, *(uint8_t*)(0x7F672B+1) | D3DCREATE_FPU_PRESERVE );
	Patch<uint8_t>( 0x7F6751+1, *(uint8_t*)(0x7F6751+1) | D3DCREATE_FPU_PRESERVE );
	Patch<uint8_t>( 0x7F6755+1, *(uint8_t*)(0x7F6755+1) | D3DCREATE_FPU_PRESERVE );
	Patch<uint8_t>( 0x7F6759+1, *(uint8_t*)(0x7F6759+1) | D3DCREATE_FPU_PRESERVE );
#endif
}

void Patch_SA_11()
{
	using namespace Memory;

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	int			pIsAlreadyRunning = AddressByRegion_11<int>(0x749000);
	ReadCall( pIsAlreadyRunning, IsAlreadyRunning );
	InjectHook(pIsAlreadyRunning, InjectDelayedPatches_11);

	// (Hopefully) more precise frame limiter
	int			pAddress = AddressByRegion_11<int>(0x7496A0);
	ReadCall( pAddress, RsEventHandler );
	InjectHook(pAddress, NewFrameRender);
	InjectHook(AddressByRegion_11<int>(0x749624), GetTimeSinceLastFrame);

	// Set CAEDataStream to use a NEW structure
	CAEDataStream::SetStructType(true);

	// Heli rotors
	InjectHook(0x6CB390, &CPlane::Render_Stub, PATCH_JUMP);
	InjectHook(0x6C4C20, &CHeli::Render_Stub, PATCH_JUMP);

	// RefFix
	static const float						fRefZVal = 1.0f;
	static const float* const				pRefFal = &fRefZVal;

	Patch<const void*>(0x6FC1AA, &pRefFal);
	Patch<BYTE>(0x6FC1D0, 0);

	// Plane rotors
	InjectHook(0x4C7A01, PlaneAtomicRendererSetup, PATCH_JUMP);

	// DOUBLE_RWHEELS
	Patch<WORD>(0x4C9490, 0xE281);
	Patch<int>(0x4C9492, ~(rwMATRIXTYPEMASK|rwMATRIXINTERNALIDENTITY));

	// A fix for DOUBLE_RWHEELS trailers
	InjectHook(0x4C9423, TrailerDoubleRWheelsFix, PATCH_JUMP);
	InjectHook(0x4C94F4, TrailerDoubleRWheelsFix2, PATCH_JUMP);

	// No framedelay
	Patch<WORD>(0x53EDC3, 0x43EB);
	Patch<BYTE>(0x53EE3F, 0x10);
	Nop(0x53EE45, 1);

	// Disable re-initialization of DirectInput mouse device by the game
	Patch<BYTE>(0x57723C, 0xEB);
	Patch<BYTE>(0x57742A, 0xEB);
	Patch<BYTE>(0x5774FA, 0xEB);

	// Make sure DirectInput mouse device is set non-exclusive (may not be needed?)
	Patch<DWORD>(AddressByRegion_11<DWORD>(0x747270), 0x9090C030);

	// Hunter interior & static_rotor for helis
	InjectHook(0x4C7972, HunterTest, PATCH_JUMP);
	InjectHook(0x4C9818, CacheCRC32);

	// Moonphases
	InjectHook(0x7142FB, HandleMoonStuffStub, PATCH_JUMP);

	// Lightbeam fix
	// Removed in Build 30 because the fix has been revisited
	/*
	Nop(0x6A36B5, 3);
	Patch<WORD>(0x6E1793, 0x0AEB);
	Patch<WORD>(0x6E17AC, 0x0BEB);
	Patch<WORD>(0x6E17C5, 0x0BEB);
	Patch<WORD>(0x6E17DF, 0x1AEB);

	Patch<WORD>(0x6E1C05, 0x09EB);
	Patch<WORD>(0x6E1C1D, 0x17EB);
	Patch<WORD>(0x6E1C4F, 0x0AEB);

	Patch<BYTE>(0x6E1810, 0x28);
	Patch<BYTE>(0x6E1C5D, 0x18);
	Patch<BYTE>(0x6E180B, 0xC8-0x7C);

	InjectHook(0x6A3717, ResetAlphaFuncRefAfterRender, PATCH_JUMP);
	*/

	// PS2 SUN!!!!!!!!!!!!!!!!!
	Nop(0x6FB9AC, 3);

	// Unlocked widescreen resolutions
	Patch<DWORD>(0x74619C, 0x9090127D);
	Nop(0x7461A0, 2);
	Nop(0x746222, 2);

	if ( *(BYTE*)0x746333 == 0xE9 )
	{
		// securom'd EXE
		// I better check if it's an address I want to patch, I don't want to break the game
		if ( *(DWORD*)0x14E7387 == 0x00E48C0F )
		{
			VP::Patch<DWORD>(0x14E7387, 0x90905D7D);
			VP::Nop(0x14E738B, 2);
		}
	}
	else
	{
		// Sadly, this func is different in 1.01 - so I don't know the original offset
	}

	// Heap corruption fix
	Patch<BYTE>(0x4A9D50, 0xC3);

	// User Tracks fix
	ReadCall( 0x4DA057, SetVolume );
	InjectHook(0x4DA057, UserTracksFix);
	InjectHook(0x4DA0A5, 0x4F3430);

	// FLAC support
	InjectHook(0x57C566, FLACInit);
	if ( *(BYTE*)0x4F3A50 == 0x6A )
	{
		InjectHook(0x4F3A50 + 0x14D, LoadFLAC_11, PATCH_JUMP);
		InjectHook(0x4F3A50 + 0x197, CAEWaveDecoderInit);

		Patch<WORD>(0x4F3A50 + 0x17A, 0x18EB);
		Patch<const void*>(0x4F3650 + 0x20, UserTrackExtensions);
		Patch<const void*>(0x4F3650 + 0x51, &UserTrackExtensions->Codec);
		Patch<const void*>(0x4F3A10 + 0x37, &UserTrackExtensions[1].Codec);
		Patch<BYTE>(0x4F3650 + 0x3D, sizeof(UserTrackExtensions));
	}
	else
	{
		// securom'd EXE
		InjectHook(0x5B6B7B, LoadFLAC_11, PATCH_JUMP);
		InjectHook(0x5B6BFB, CAEWaveDecoderInit, PATCH_JUMP);
		Patch<WORD>(0x5B6BCB, 0x26EB);

		if ( *(DWORD*)0x14E4954 == 0x05C70A75 )
			VP::Patch<const void*>(0x14E4958, &UserTrackExtensions[1].Codec);

		// Deobfuscating an opcode
		Patch<BYTE>(0x4EBD25, 0xBF);
		Patch<const void*>(0x4EBD26, UserTrackExtensions);
		Patch<const void*>(0x4EBDD4, &UserTrackExtensions->Codec);
		Patch<WORD>(0x4EBD2A, 0x72EB);
		Patch<BYTE>(0x4EBDC0, sizeof(UserTrackExtensions));
	}

	// Impound garages working correctly
	InjectHook(0x4251F9, 0x448A10);
	InjectHook(0x4253E9, 0x448A10);
	InjectHook(0x425491, 0x448A10);

	// Impounding after busted works
	Nop(0x443312, 5);

	// Mouse rotates an airbone car only with Steer with Mouse option enabled
	bool*	bEnableMouseSteering = *(bool**)0x6ADFCD; // CVehicle::m_bEnableMouseSteering
	Patch<bool*>(0x6B56E0, bEnableMouseSteering);
	Patch<bool*>(0x6CF047, bEnableMouseSteering);

	// Patched CAutomobile::Fix
	// misc_x parts don't get reset (Bandito fix), Towtruck's bouncing panel is not reset
	Patch<WORD>(0x6A3CE9, 0x5EEB);
	Patch<DWORD>(0x6A3D75, 0x5E5FCF8B);
	Patch<DWORD>(0x6A3D79, 0x448B5B5D);
	Patch<DWORD>(0x6A3D7D, 0x89644824);
	Patch<DWORD>(0x6A3D81, 5);
	Patch<DWORD>(0x6A3D85, 0x54C48300);
	InjectHook(0x6A3D89, &CAutomobile::Fix_SilentPatch, PATCH_JUMP);

	// Patched CPlane::Fix
	// Doors don't get reset (they can't get damaged anyway), bouncing panels DO reset
	// but not on Vortex
	Patch<BYTE>(0x6CB3F0, 0xEB);
	Patch<DWORD>(0x6CB425, 0x5E5FCF8B);
	InjectHook(0x6CB429, &CPlane::Fix_SilentPatch, PATCH_JUMP);

	// Weapon icon fix (crosshairs mess up rwRENDERSTATEZWRITEENABLE)
	// Only 1.0 and 1.01, Steam somehow fixed it (not the same way though)
	Nop(0x58E9E0, 3);
	Nop(0x58F287, 3);
	Nop(0x58F2B1, 3);

	// CGarages::RespraysAreFree resetting on new game
	Patch<WORD>(0x448C58, 0x8966);
	Patch<BYTE>(0x448C5A, 0x0D);
	Patch<bool*>(0x448C5B, *(bool**)0x44AD18);
	Patch<BYTE>(0x448C5F, 0xC3);

	// Bilinear filtering for license plates
	//Patch<BYTE>(0x6FD528, rwFILTERLINEAR);
	Patch<BYTE>(0x6FE777, rwFILTERLINEAR);

	// -//- Roadsign maganer
	//Patch<BYTE>(0x6FE147, rwFILTERLINEAR);

	// Bilinear filtering with mipmaps for weapon icons
	Patch<BYTE>(0x58DFAA, rwFILTERMIPLINEAR);

	// Illumination value from timecyc.dat properly using floats
	Patch<WORD>(0x5BC7A9, 0x14EB);

	// Illumination defaults to 1.0
	Patch<DWORD>(0x5BC2E4, 0xCC2484C7);
	Patch<DWORD>(0x5BC2E8, 0x00000000);
	Patch<DWORD>(0x5BC2EC, 0x903F8000);

	// All lights get casted at vehicles
	Patch<BYTE>(0x5DA297, 8);
	Patch<BYTE>(0x5DA2A0, 8);
	Patch<BYTE>(0x5DA73F, 8);

	// 6 extra directionals on Medium and higher
	// push eax
	// call GetMaxExtraDirectionals
	// add esp, 4
	// mov ebx, eax
	// nop
	Patch<uint8_t>( 0x7360B1, 0x50 );
	InjectHook( 0x7360B1 + 1, GetMaxExtraDirectionals, PATCH_CALL );
	Patch( 0x7360B1 + 6, { 0x83, 0xC4, 0x04, 0x8B, 0xD8 } );
	Nop( 0x7360B1 + 11, 3 );

	// Default resolution to native resolution
	RECT			desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

	Patch<DWORD>(0x746BE3, desktop.right);
	Patch<DWORD>(0x746BE8, desktop.bottom);
	Patch<const char*>(0x746C48, aNoDesktopMode);

	// Corrected Map screen 1px issue
	Patch<float>(0x576357, -0.5f);
	Patch<float>(0x576317, -0.5f);
	Patch<float>(0x57631F, -0.5f);
	Patch<float>(0x5762CC, -0.5f);
	Patch<float>(0x57624A, -0.5f);
	Patch<float>(0x57627C, -0.5f);

	// Cars drive on water cheat
	Patch<DWORD>(&(*(DWORD**)0x438593)[34], 0xE5FC92C3);

	// No DirectPlay dependency
	Patch<BYTE>(AddressByRegion_11<DWORD>(0x747E1A), 0xB8);
	Patch<DWORD>(AddressByRegion_11<DWORD>(0x747E1B), 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x7457E0, GetMyDocumentsPathSA, PATCH_JUMP);

	// Fixed muzzleflash not showing from last bullet
	Nop(0x61F504, 2);

	// Proper randomizations
	InjectHook(0x44E8AE, Int32Rand); // Missing ped paths
	InjectHook(0x44ED6E, Int32Rand); // Missing ped paths
	InjectHook(0x6676C0, Int32Rand); // Prostitutes

	// Help boxes showing with big message
	// Game seems to assume they can show together
	Nop(0x58C25F, 6);

	// Fixed lens flare
	Patch<DWORD>(0x70FC8A, 0);
	Patch<BYTE>(0x6FBE51, 0xC3);
	Patch<BYTE>(0x6FBE30, 0x21);
	InjectHook(0x6FBE52, 0x70D750, PATCH_CALL);
	Patch<WORD>(0x6FBE57, 0xDCEB);

	Patch<WORD>(0x6FBCA6, 0xB990);
	Patch(0x6FBCA8, &FlushLensSwitchZ);
	Patch<WORD>(0x6FBCB0, 0xD1FF);
	Nop(0x6FBCB2, 1);

	Patch<WORD>(0x6FB758, 0xB990);
	Patch(0x6FB75A, &InitBufferSwitchZ);
	Patch<WORD>(0x6FB762, 0xD1FF);
	Nop(0x6FB764, 1);

	// Y axis sensitivity fix
	float* sens = *(float**)0x50F4DC;
	Patch<const void*>(0x50F4E6 + 0x2, sens);
	Patch<const void*>(0x50FFC6 + 0x2, sens);
	Patch<const void*>(0x5110C6 + 0x2, sens);
	Patch<const void*>(0x5122A8 + 0x2, sens);
	Patch<const void*>(0x52272C + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<WORD>(0x510054, 0x27EB);
	Patch<WORD>(0x5109B2, 0xE990);
	InjectHook(0x524511, 0x5245D9, PATCH_JUMP);

	// Fixed mirrors crash
	Patch<uint64_t>(0x7279FB, 0xC604C4833474C085);

	// Mirrors depth fix & bumped quality
	InjectHook(0x72784D, CreateMirrorBuffers);

	// Fixed MSAA options
	Patch<BYTE>(0x57D906, 0xEB);
	Nop(0x57D8C8, 2);

	Patch<BYTE>(AddressByRegion_11<BYTE*>(0x7F759B), 0xEB);
	Patch<BYTE>(AddressByRegion_11<BYTE*>(0x7F69C6), 0xEB);
	Patch<WORD>(AddressByRegion_11<BYTE*>(0x7F6F83), 0xE990);

	ReadCall( 0x57D916, orgGetMaxMultiSamplingLevels );
	InjectHook(0x57D916, GetMaxMultiSamplingLevels);
	InjectHook(0x57D8CA, GetMaxMultiSamplingLevels);

	ReadCall( 0x574A6D, orgChangeMultiSamplingLevels );
	InjectHook(0x574A6D, ChangeMultiSamplingLevels);
	InjectHook(0x57D942, ChangeMultiSamplingLevels);
	InjectHook(0x57DA86, ChangeMultiSamplingLevels);

	ReadCall( 0x746BD0, orgSetMultiSamplingLevels );
	InjectHook(0x746BD0, SetMultiSamplingLevels);

	Nop(0x57A66C, 1);
	InjectHook(0x57A66D, MSAAText, PATCH_CALL);

	// Fixed car collisions - car you're hitting gets proper damage now
	InjectHook(0x542D8A, FixedCarDamage, PATCH_CALL);


	// Car explosion crash with multimonitor
	// Unitialized collision data breaking stencil shadows
	// FUCK THIS IN 1.01

	// Fixed escalators crash
	// FUCK THIS IN 1.01


	// Don't allocate constant memory for stencil shadows every frame
	// FUCK THIS IN 1.01

	// Fixed police scanner names
	char*			pScannerNames = *(char**)0x4E7714;
	strcpy_s(pScannerNames + (8*113), 8, "WESTP");
	strcpy_s(pScannerNames + (8*134), 8, "????");


	// 1.01 ONLY
	// I'm not sure what was this new audio code supposed to do, but it leaks memory
	// and due to this I have to make extra effort if I want FLAC to work on 1.01
	Patch<DWORD>(0x4E124C, 0x4DEBC78B);
}

void Patch_SA_Steam()
{
	using namespace Memory;

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	ReadCall( 0x7826ED, IsAlreadyRunning );
	InjectHook(0x7826ED, InjectDelayedPatches_Steam);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x782D25, RsEventHandler );
	InjectHook(0x782D25, NewFrameRender);
	InjectHook(0x782CA8, GetTimeSinceLastFrame);

	// Set CAEDataStream to use an old structure
	CAEDataStream::SetStructType(false);

	// Heli rotors
	InjectHook(0x700620, &CPlane::Render_Stub, PATCH_JUMP);
	InjectHook(0x6F9550, &CHeli::Render_Stub, PATCH_JUMP);

	// RefFix
	static const float						fRefZVal = 1.0f;
	static const float* const				pRefFal = &fRefZVal;

	Patch<const void*>(0x733FF0, &pRefFal);
	Patch<BYTE>(0x73401A, 0);

	// Plane rotors
	InjectHook(0x4D2270, PlaneAtomicRendererSetup, PATCH_JUMP);

	// DOUBLE_RWHEELS
	Patch<WORD>(0x4D3B9D, 0x6781);
	Patch<int>(0x4D3BA0, ~(rwMATRIXTYPEMASK|rwMATRIXINTERNALIDENTITY));

	// A fix for DOUBLE_RWHEELS trailers
	InjectHook(0x4D3B47, TrailerDoubleRWheelsFix_Steam, PATCH_JUMP);
	InjectHook(0x4D3C1A, TrailerDoubleRWheelsFix2_Steam, PATCH_JUMP);

	// No framedelay
	Patch<WORD>(0x551113, 0x46EB);
	Patch<BYTE>(0x551195, 0xC);
	Nop(0x551197, 1);

	// Disable re-initialization of DirectInput mouse device by the game
	Patch<BYTE>(0x58C0E5, 0xEB);
	Patch<BYTE>(0x58C2CF, 0xEB);
	Patch<BYTE>(0x58C3B3, 0xEB);

	// Make sure DirectInput mouse device is set non-exclusive (may not be needed?)
	Patch<DWORD>(0x7807D0, 0x9090C030);

	// Hunter interior & static_rotor for helis
	InjectHook(0x4D21E1, HunterTest, PATCH_JUMP);
	InjectHook(0x4D3F1D, CacheCRC32);

	// Bindable NUM5
	// Only 1.0 and Steam
	Nop(0x59363B, 2);

	// Moonphases
	InjectHook(0x72F058, HandleMoonStuffStub_Steam, PATCH_JUMP);

	// Lightbeam fix
	// Removed in Build 30 because the fix has been revisited
	/*
	Patch<WORD>(0x6CFEF9, 0x10EB);
	Nop(0x6CFF0F, 3);
	Patch<WORD>(0x71D1F5, 0x0DEB);
	Patch<WORD>(0x71D213, 0x0CEB);
	Patch<WORD>(0x71D230, 0x0DEB);
	Patch<WORD>(0x71D24D, 0x1FEB);

	Patch<WORD>(0x71D72F, 0x0BEB);
	Patch<WORD>(0x71D74B, 0x1BEB);
	Patch<WORD>(0x71D785, 0x0CEB);

	Patch<BYTE>(0x71D284, 0x28);
	Patch<BYTE>(0x71D795, 0x18);
	Patch<BYTE>(0x71D27F, 0xD0-0x9C);
	//InjectHook(0x6A2EDA, CullTest);

	InjectHook(0x6CFF69, ResetAlphaFuncRefAfterRender_Steam, PATCH_JUMP);
	*/

	// PS2 SUN!!!!!!!!!!!!!!!!!
	Nop(0x73362F, 2);

	// Unlocked widescreen resolutions
	//Patch<WORD>(0x77F9F0, 0x6E7D);
	Patch<WORD>(0x77F9FC, 0x627D);
	Patch<DWORD>(0x77F80B, 0x9090127D);
	Nop(0x77F80F, 2);
	Nop(0x77F880, 2);

	// Heap corruption fix
	Nop(0x5D88AE, 5);

	// User Tracks fix
	SetVolume = reinterpret_cast<decltype(SetVolume)>(0x4E2750);
	Patch<BYTE>(0x4E4A28, 0xBA);
	Patch<const void*>(0x4E4A29, UserTracksFix_Steam);
	InjectHook(0x4E4A8B, 0x4FF2B0);

	// FLAC support
	InjectHook(0x4FFC39, LoadFLAC_Steam, PATCH_JUMP);
	InjectHook(0x591814, FLACInit_Steam);
	InjectHook(0x4FFC83, CAEWaveDecoderInit);

	Patch<WORD>(0x4FFC66, 0x18EB);
	Patch<const void*>(0x4FF4F0, UserTrackExtensions);
	Patch<const void*>(0x4FF523, &UserTrackExtensions->Codec);
	Patch<const void*>(0x4FFAB6, &UserTrackExtensions[1].Codec);
	Patch<BYTE>(0x4FF50F, sizeof(UserTrackExtensions));

	// Impound garages working correctly
	InjectHook(0x426B48, 0x44C950);
	InjectHook(0x426D16, 0x44C950);
	InjectHook(0x426DC5, 0x44C950);

	// Impounding after busted works
	Nop(0x446F58, 5);

	// Mouse rotates an airbone car only with Steer with Mouse option enabled
	bool*	bEnableMouseSteering = *(bool**)0x6DB76D; // CVehicle::m_bEnableMouseSteering
	Patch<bool*>(0x6E3199, bEnableMouseSteering);
	Patch<bool*>(0x7046AB, bEnableMouseSteering);

	// Patched CAutomobile::Fix
	// misc_x parts don't get reset (Bandito fix), Towtruck's bouncing panel is not reset
	Patch<DWORD>(0x6D05B3, 0x6BEBED31);
	Patch<DWORD>(0x6D0649, 0x5E5FCF8B);
	Patch<DWORD>(0x6D064D, 0x448B5B5D);
	Patch<DWORD>(0x6D0651, 0x89644824);
	Patch<DWORD>(0x6D0655, 5);
	Patch<DWORD>(0x6D0659, 0x54C48300);
	InjectHook(0x6D065D, &CAutomobile::Fix_SilentPatch, PATCH_JUMP);

	// Patched CPlane::Fix
	// Doors don't get reset (they can't get damaged anyway), bouncing panels DO reset
	// but not on Vortex
	Patch<BYTE>(0x700681, 0xEB);
	Patch<DWORD>(0x7006B6, 0x5E5FCF8B);
	InjectHook(0x7006BA, &CPlane::Fix_SilentPatch, PATCH_JUMP);

	// Zones fix
	InjectHook(0x587080, GetCurrentZoneLockedOrUnlocked_Steam, PATCH_JUMP);

	// CGarages::RespraysAreFree resetting on new game
	Patch<WORD>(0x44CB55, 0xC766);
	Patch<BYTE>(0x44CB57, 0x05);
	Patch<bool*>(0x44CB58, *(bool**)0x44EEBA);
	Patch<WORD>(0x44CB5C, 0x0000);

	// Bilinear filtering for license plates
	//Patch<BYTE>(0x6FD528, rwFILTERLINEAR);
	Patch<BYTE>(0x736B30, rwFILTERLINEAR);

	// -//- Roadsign maganer
	//Patch<BYTE>(0x6FE147, rwFILTERLINEAR);

	// Bilinear filtering with mipmaps for weapon icons
	Patch<BYTE>(0x59BD9C, rwFILTERMIPLINEAR);

	// Illumination value from timecyc.dat properly using floats
	Patch<WORD>(0x5DAF6B, 0x2CEB);

	// Illumination defaults to 1.0
	Patch<DWORD>(0x5DA8D4, 0xD82484C7);
	Patch<DWORD>(0x5DA8D8, 0x00000000);
	Patch<DWORD>(0x5DA8DC, 0x903F8000);

	// All lights get casted at vehicles
	Patch<BYTE>(0x5F61C7, 8);
	Patch<BYTE>(0x5F61D0, 8);
	Patch<BYTE>(0x5F666D, 8);

	// 6 extra directionals on Medium and higher
	// push dword ptr [CGame::currArea]
	// call GetMaxExtraDirectionals
	// add esp, 4
	// mov ebx, eax
	// nop
	Patch( 0x768046, { 0xFF, 0x35 } );
	InjectHook( 0x768046 + 6, GetMaxExtraDirectionals, PATCH_CALL );
	Patch( 0x768046 + 11, { 0x83, 0xC4, 0x04, 0x8B, 0xD8 } );
	Nop( 0x768046 + 16, 1 );

	// Default resolution to native resolution
	RECT			desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

	Patch<DWORD>(0x780219, desktop.right);
	Patch<DWORD>(0x78021E, desktop.bottom);
	Patch<const char*>(0x78027E, aNoDesktopMode);

	// Corrected Map screen 1px issue
	/*Patch<float>(0x575DE7, -5.0f);
	Patch<float>(0x575DA7, -5.0f);
	Patch<float>(0x575DAF, -5.0f);
	Patch<float>(0x575D5C, -5.0f);
	Patch<float>(0x575CDA, -5.0f);
	Patch<float>(0x575D0C, -5.0f);*/
	InjectHook(0x58B0F8, DrawRect_HalfPixel_Steam<true,false,false,true>);
	InjectHook(0x58B146, DrawRect_HalfPixel_Steam<true,false,false,false>);
	InjectHook(0x58B193, DrawRect_HalfPixel_Steam<true,false,false,true>);
	InjectHook(0x58B1E1, DrawRect_HalfPixel_Steam<false,false,false,true>);

	// Cars drive on water cheat
	Patch<DWORD>(&(*(DWORD**)0x43B793)[34], 0xE5FC92C3);

	// No DirectPlay dependency
	Patch<BYTE>(0x781456, 0xB8);
	Patch<DWORD>(0x781457, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x77EDC0, GetMyDocumentsPathSA, PATCH_JUMP);

	// Fixed muzzleflash not showing from last bullet
	Nop(0x61F504, 2);

	// Proper randomizations
	InjectHook(0x452CCF, Int32Rand); // Missing ped paths
	InjectHook(0x45322C, Int32Rand); // Missing ped paths
	InjectHook(0x690263, Int32Rand); // Prostitutes

	// Help boxes showing with big message
	// Game seems to assume they can show together
	Nop(0x599CD3, 6);

	// Fixed lens flare
	Nop(0x733C65, 5);
	Patch<BYTE>(0x733C4E, 0x26);
	InjectHook(0x733C75, 0x7591E0, PATCH_CALL);
	Patch<WORD>(0x733C7A, 0xDBEB);

	Nop(0x733A5A, 4);
	Patch<BYTE>(0x733A5E, 0xB8);
	Patch(0x733A5F, &FlushLensSwitchZ);

	Patch<DWORD>(0x7333B0, 0xB9909090);
	Patch(0x7333B4, &InitBufferSwitchZ);

	// Y axis sensitivity fix
	float* sens = *(float**)0x51D4FA;
	Patch<const void*>(0x51D508 + 0x2, sens);
	Patch<const void*>(0x51E25A + 0x2, sens);
	Patch<const void*>(0x51F459 + 0x2, sens);
	Patch<const void*>(0x52086A + 0x2, sens);
	Patch<const void*>(0x532B9B + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<WORD>(0x51E192, 0x2BEB);
	Patch<WORD>(0x51ED38, 0xE990);
	InjectHook(0x534D3E, 0x534DF7, PATCH_JUMP);

	// Fixed mirrors crash
	Patch<uint64_t>(0x75903A, 0xC604C4833474C085);

	// Mirrors depth fix & bumped quality
	InjectHook(0x758E91, CreateMirrorBuffers);

	// Fixed MSAA options
	Patch<BYTE>(0x592BBB, 0xEB);
	Nop(0x592B7F, 2);

	Patch<BYTE>(0x830C5B, 0xEB);
	Patch<BYTE>(0x830086, 0xEB);
	Patch<WORD>(0x830643, 0xE990);

	ReadCall( 0x592BCF, orgGetMaxMultiSamplingLevels );
	InjectHook(0x592BCF, GetMaxMultiSamplingLevels);
	InjectHook(0x592B81, GetMaxMultiSamplingLevels);

	ReadCall( 0x5897CD, orgChangeMultiSamplingLevels );
	InjectHook(0x5897CD, ChangeMultiSamplingLevels);
	InjectHook(0x592BFB, ChangeMultiSamplingLevels);
	InjectHook(0x592D2E, ChangeMultiSamplingLevels);

	ReadCall( 0x780206, orgSetMultiSamplingLevels );
	InjectHook(0x780206, SetMultiSamplingLevels);

	Patch<WORD>(0x58F88C, 0xBA90);
	Patch(0x58F88E, MSAAText);

	// Fixed car collisions - car you're hitting gets proper damage now
	Nop(0x555AB8, 2);
	InjectHook(0x555AC0, FixedCarDamage_Steam, PATCH_CALL);


	// Car explosion crash with multimonitor
	// Unitialized collision data breaking stencil shadows
	ReadCall( 0x41A216, orgMemMgrMalloc );
	InjectHook(0x41A216, CollisionData_MallocAndInit);

	ReadCall( 0x41A07C, orgNewAlloc );
	InjectHook(0x41A07C, CollisionData_NewAndInit);
	InjectHook(0x41A159, CollisionData_NewAndInit);


	// Crash when entering advanced display options on a dual monitor machine after:
	// - starting game on primary monitor in maximum resolution, exiting,
	// starting again in maximum resolution on secondary monitor.
	// Secondary monitor maximum resolution had to be greater than maximum resolution of primary monitor.
	// Not in 1.01
	ReadCall( 0x77F99E, orgGetNumVideoModes );
	InjectHook(0x77F99E, GetNumVideoModes_Store);
	InjectHook(0x77F901, GetNumVideoModes_Retrieve);


	// Fixed escalators crash
	ReadCall( 0x739975, orgEscalatorsUpdate );
	InjectHook(0x739975, UpdateEscalators);
	InjectHook(0x738BBD, &CEscalator::SwitchOffNoRemove);


	// Don't allocate constant memory for stencil shadows every frame
	InjectHook(0x760795, StencilShadowAlloc, PATCH_CALL);
	Nop(0x7607CD, 3);
	Patch<WORD>(0x76079A, 0x2CEB);
	Patch<DWORD>(0x76082C, 0x90C35D5F);	// pop edi, pop ebp, ret


	// "Streaming memory bug" fix
	InjectHook(0x4CF9E8, GTARtAnimInterpolatorSetCurrentAnim);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x43BB8B+1, 1); // knife
	Patch<BYTE>(0x43BC78+1, 1); // knife
	Patch<BYTE>(0x43BE1F+1, 1); // chainsaw
	Patch<BYTE>(0x43BED8+1, 1); // chainsaw
	Patch<BYTE>(0x43C868+1, 1); // parachute

	Patch<BYTE>(0x43D24C, 0x53); // katana
	Patch<WORD>(0x43D24D, 0x016A);


	// AI accuracy issue
	Nop(0x7738F5, 1);
	InjectHook( 0x7738F5+1, WeaponRangeMult_VehicleCheck, PATCH_CALL );


	// Don't catch WM_SYSKEYDOWN and WM_SYSKEYUP (fixes Alt+F4)
	InjectHook( 0x7821E5, 0x7823FE, PATCH_JUMP );
	Patch<uint8_t>( 0x7821A7 + 1, 0x5C ); // esi -> ebx
	Patch<uint8_t>( 0x7821AF, 0x53 ); // esi -> ebx
	Patch<uint8_t>( 0x7821D1 + 1, 0xFB ); // esi -> ebx
	Patch<int8_t>( 0x7821B1 + 3, 0x54-0x2C ); // use stack space for new lParam
	Patch<int8_t>( 0x7821C2 + 3, 0x4C-0x2C ); // use stack space for new lParam
	Patch<int8_t>( 0x7821D6 + 3, 0x4C-0x2C ); // use stack space for new lParam

	InjectHook( 0x78222F, 0x7823FE, PATCH_JUMP );
	Patch<uint8_t>( 0x7821F1 + 1, 0x5C ); // esi -> ebx
	Patch<uint8_t>( 0x7821F9, 0x53 ); // esi -> ebx
	Patch<uint8_t>( 0x78221B + 1, 0xFB ); // esi -> ebx
	Patch<int8_t>( 0x7821FB + 3, 0x54-0x2C ); // use stack space for new lParam
	Patch<int8_t>( 0x78220C + 3, 0x4C-0x2C ); // use stack space for new lParam
	Patch<int8_t>( 0x782220 + 3, 0x4C-0x2C ); // use stack space for new lParam


	// FuckCarCompletely not fixing panels
	Nop(0x6F5EC1, 3);


	// 014C cargen counter fix (by spaceeinstein)
	Patch<uint8_t>( 0x6F566D + 1, 0xBF ); // movzx eax, word ptr [ebp+1Ah] -> movsx eax, word ptr [ebp+1Ah]
	Patch<uint8_t>( 0x6F567E + 1, 0xBF ); // movzx ecx, ax -> movsx ecx, ax
	Patch<uint8_t>( 0x6F3E32, 0x74 ); // jge -> jz


	// Linear filtering on script sprites
	ReadCall( 0x59A3F2, orgDrawScriptSpritesAndRectangles );
	InjectHook( 0x59A3F2, DrawScriptSpritesAndRectangles );


	// Fixed police scanner names
	char*			pScannerNames = *(char**)0x4F2B83;
	strcpy_s(pScannerNames + (8*113), 8, "WESTP");
	strcpy_s(pScannerNames + (8*134), 8, "????");

	// STEAM ONLY
	// Proper aspect ratios - why Rockstar, why?
	// Steam aspect ratios were additionally divided by 1.1, producing a squashed image
	static const float f43 = 4.0f/3.0f, f54 = 5.0f/4.0f, f169 = 16.0f/9.0f;
	Patch<const void*>(0x73822B, &f169);
	Patch<const void*>(0x738247, &f54);
	Patch<const void*>(0x73825A, &f43);

	// No IMG size check
	Nop(0x406CD0, 7);
	Nop(0x406D00, 7);

	// Unlock 1.0/1.01 saves loading
	InjectHook(0x5EDFD9, 0x5EE0FA, PATCH_JUMP);
}

void Patch_SA_NewSteam_r1()
{
	using namespace Memory::DynBase;

	// Nazi EXE?
	if ( *(DWORD*)DynBaseAddress(0x49F810) == 0x64EC8B55 )
	{
		// Regular

		// No framedelay
		InjectHook(0x54ECC6, DynBaseAddress(0x54ED0C), PATCH_JUMP);
		Patch<BYTE>(0x54ED45, 0x4);
		Nop(0x54ED47, 1);

		// Unlock 1.0/1.01 saves loading 
		Patch<WORD>(0x5ED3E9, 0xE990);

		// Old .set files working again
		static const DWORD		dwSetVersion = 6;
		Patch<const void*>(0x59058A, &dwSetVersion);
		Patch<BYTE>(0x59086D, 6);
		Patch<BYTE>(0x53EC4A, 6);

		// Disable re-initialization of DirectInput mouse device by the game
		Patch<BYTE>(0x58A891, 0xEB);
		Patch<BYTE>(0x58AA77, 0xEB);
		Patch<BYTE>(0x58AB59, 0xEB);
	}
	else
	{
		// Nazi

		// No framedelay
		InjectHook(0x54EC06, DynBaseAddress(0x54EC4C), PATCH_JUMP);
		Patch<BYTE>(0x54EC85, 0x4);
		Nop(0x54EC87, 1);

		// Unlock 1.0/1.01 saves loading 
		Patch<WORD>(0x5ED349, 0xE990);

		// Old .set files working again
		static const DWORD		dwSetVersion = 6;
		Patch<const void*>(0x5904DA, &dwSetVersion);
		Patch<BYTE>(0x5907BD, 6);
		Patch<BYTE>(0x53EB9A, 6);

		// Disable re-initialization of DirectInput mouse device by the game
		Patch<BYTE>(0x58A7D1, 0xEB);
		Patch<BYTE>(0x58A9B7, 0xEB);
		Patch<BYTE>(0x58AA99, 0xEB);
	}


	// Unlocked widescreen resolutions
	//Patch<WORD>(0x779BAD, 0x607D);
	Patch<WORD>(0x779BB8, 0x557D);
	Patch<DWORD>(0x7799D8, 0x9090117D);
	Nop(0x779A45, 2);
	Nop(0x7799DC, 2);

	// Make sure DirectInput mouse device is set non-exclusive (may not be needed?)
	Nop(0x77AB3F, 1);
	Patch<WORD>(0x77AB40, 0x01B0);

	// Default resolution to native resolution
	RECT			desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

	Patch<DWORD>(0x77A3EF, desktop.right);
	Patch<DWORD>(0x77A3F4, desktop.bottom);
	Patch<const char*>(0x77A44B, aNoDesktopMode);


	// Proper aspect ratios
	static const float f43 = 4.0f/3.0f, f54 = 5.0f/4.0f, f169 = 16.0f/9.0f;
	Patch<const void*>(0x73424B, &f169);
	Patch<const void*>(0x734267, &f54);
	Patch<const void*>(0x73427A, &f43);
}

void Patch_SA_NewSteam_r2()
{
	using namespace Memory::DynBase;

	// (Hopefully) more precise frame limiter
	ReadCall( 0x77D55F, RsEventHandler );
	InjectHook(0x77D55F, NewFrameRender);
	InjectHook(0x77D4E8, GetTimeSinceLastFrame);

	// No framedelay
	InjectHook(0x54ECC6, DynBaseAddress(0x54ED0C), PATCH_JUMP);
	Patch<BYTE>(0x54ED45, 0x4);
	Nop(0x54ED47, 1);

	// Unlock 1.0/1.01 saves loading 
	Patch<WORD>(0x5ED349, 0xE990);

	// Old .set files working again
	static const DWORD		dwSetVersion = 6;
	Patch<const void*>(0x5904CA, &dwSetVersion);
	Patch<BYTE>(0x5907AD, 6);
	Patch<BYTE>(0x53EC4A, 6);

	// Disable re-initialization of DirectInput mouse device by the game
	Patch<BYTE>(0x58A881, 0xEB);
	Patch<BYTE>(0x58AA67, 0xEB);
	Patch<BYTE>(0x58AB49, 0xEB);

	// Unlocked widescreen resolutions
	//Patch<WORD>(0x779BAD, 0x607D);
	Patch<WORD>(0x779BC8, 0x697D);
	Patch<DWORD>(0x7799D8, 0x9090117D);
	Nop(0x779A56, 2);
	Nop(0x7799DC, 2);

	// Make sure DirectInput mouse device is set non-exclusive (may not be needed?)
	Nop(0x77AB6F, 1);
	Patch<WORD>(0x77AB70, 0x01B0);

	// Default resolution to native resolution
	RECT			desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

	Patch<DWORD>(0x77A41F, desktop.right);
	Patch<DWORD>(0x77A424, desktop.bottom);
	Patch<const char*>(0x77A47B, aNoDesktopMode);

	// No DirectPlay dependency
	Patch<BYTE>(0x77B46E, 0xB8);
	Patch<DWORD>(0x77B46F, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x778FA0, GetMyDocumentsPathSA, PATCH_JUMP);

	// Fixed muzzleflash not showing from last bullet
	Nop(0x63E8A9, 2);

	// Proper randomizations
	InjectHook(0x45234C, Int32Rand); // Missing ped paths
	InjectHook(0x45284C, Int32Rand); // Missing ped paths
	InjectHook(0x69046F, Int32Rand); // Prostitutes

	// Help boxes showing with big message
	// Game seems to assume they can show together
	Nop(0x597EEA, 6);

	// Fixed lens flare
	Nop(0x7300F8, 5);
	Patch<BYTE>(0x7300E3, 0x20);
	InjectHook(0x730104, DynBaseAddress(0x753AE0), PATCH_CALL);
	Patch<WORD>(0x730109, 0xE1EB);

	Nop(0x72FF17, 4);
	Patch<BYTE>(0x72FF1B, 0xB8);
	Patch(0x72FF1C, &FlushLensSwitchZ);

	Patch<DWORD>(0x72F91D, 0xB9909090);
	Patch(0x72F921, &InitBufferSwitchZ);

	// Y axis sensitivity fix
	float* sens = *(float**)DynBaseAddress(0x51B987);
	Patch<const void*>(0x51B993 + 0x2, sens);
	Patch<const void*>(0x51C68C + 0x2, sens);
	Patch<const void*>(0x51D73A + 0x2, sens);
	Patch<const void*>(0x51EA3A + 0x2, sens);
	Patch<const void*>(0x52FBE1 + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<WORD>(0x51C5CD, 0x29EB);
	Patch<WORD>(0x51D053, 0xE990);
	InjectHook(0x531BBE, DynBaseAddress(0x531C6F), PATCH_JUMP);

	// Fixed mirrors crash
	Patch<uint64_t>(0x753941, 0xC604C4832F74C085);

	// Mirrors depth fix & bumped quality
	InjectHook(0x7537A0, CreateMirrorBuffers);

	// Fixed MSAA options
	Patch<BYTE>(0x590F77, 0xEB);
	Nop(0x590F34, 2);

	Patch<BYTE>(0x82B4EB, 0xEB);
	Patch<BYTE>(0x82A916, 0xEB);
	Patch<WORD>(0x82AED3, 0xE990);

	ReadCall( 0x590F8B, orgGetMaxMultiSamplingLevels );
	InjectHook(0x590F8B, GetMaxMultiSamplingLevels);
	InjectHook(0x590F36, GetMaxMultiSamplingLevels);

	ReadCall( 0x5881C0, orgChangeMultiSamplingLevels );
	InjectHook(0x5881C0, ChangeMultiSamplingLevels);
	InjectHook(0x590FBB, ChangeMultiSamplingLevels);
	InjectHook(0x591111, ChangeMultiSamplingLevels);

	ReadCall( 0x77A40D, orgSetMultiSamplingLevels );
	InjectHook(0x77A40D, SetMultiSamplingLevels);

	Patch<WORD>(0x58DDEF, 0xBA90);
	Patch(0x58DDF1, MSAAText);

	// Fixed car collisions - car you're hitting gets proper damage now
	Nop(0x5538D0, 2);
	InjectHook(0x5538D2, FixedCarDamage_Newsteam, PATCH_CALL);


	// Car explosion crash with multimonitor
	// Unitialized collision data breaking stencil shadows
	ReadCall( 0x41A661, orgMemMgrMalloc );
	InjectHook(0x41A661, CollisionData_MallocAndInit);

	ReadCall( 0x41A4CC, orgNewAlloc );
	InjectHook(0x41A4CC, CollisionData_NewAndInit);
	InjectHook(0x41A5A9, CollisionData_NewAndInit);


	// Crash when entering advanced display options on a dual monitor machine after:
	// - starting game on primary monitor in maximum resolution, exiting,
	// starting again in maximum resolution on secondary monitor.
	// Secondary monitor maximum resolution had to be greater than maximum resolution of primary monitor.
	// Not in 1.01
	ReadCall( 0x779B71, orgGetNumVideoModes );
	InjectHook(0x779B71, GetNumVideoModes_Store);
	InjectHook(0x779AD1, GetNumVideoModes_Retrieve);


	// Fixed escalators crash
	orgEscalatorsUpdate = (void(*)())DynBaseAddress(0x735B90);
	InjectHook(0x735BC5, UpdateEscalators, PATCH_JUMP);

	Patch<WORD>(0x734BAE, 0x4E8D);
	Patch<BYTE>(0x734BB0, 0x84);
	InjectHook(0x734BB1, &CEscalator::SwitchOffNoRemove, PATCH_CALL);
	Patch<WORD>(0x734BB6, 0x52EB);


	// Don't allocate constant memory for stencil shadows every frame
	InjectHook(0x75ADA9, StencilShadowAlloc, PATCH_CALL);
	Nop(0x75ADE1, 3);
	Patch<WORD>(0x75ADAE, 0x2CEB);
	Patch<DWORD>(0x75AE35, 0x5D5B5E5F);	// pop edi, pop esi, pop ebx, pop ebp, retn
	Patch<BYTE>(0x75AE39, 0xC3);


	// "Streaming memory bug" fix
	InjectHook(0x4CF1FB, GTARtAnimInterpolatorSetCurrentAnim);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x43AD0B+1, 1); // knife
	Patch<BYTE>(0x43ADF8+1, 1); // knife
	Patch<BYTE>(0x43AF9F+1, 1); // chainsaw
	Patch<BYTE>(0x43B058+1, 1); // chainsaw
	Patch<BYTE>(0x43B9B8+1, 1); // parachute

	Patch<BYTE>(0x43C492, 0x53); // katana
	Patch<WORD>(0x43C493, 0x016A);


	// Proper aspect ratios
	static const float f43 = 4.0f/3.0f, f54 = 5.0f/4.0f, f169 = 16.0f/9.0f;
	Patch<const void*>(0x73424B, &f169);
	Patch<const void*>(0x734267, &f54);
	Patch<const void*>(0x73427A, &f43);
}

void Patch_SA_NewSteam_r2_lv()
{
	using namespace Memory::DynBase;

	// (Hopefully) more precise frame limiter
	ReadCall( 0x77D44F, RsEventHandler );
	InjectHook(0x77D44F, NewFrameRender);
	InjectHook(0x77D3D8, GetTimeSinceLastFrame);

	// No framedelay
	InjectHook(0x54EC06, DynBaseAddress(0x54EC4C), PATCH_JUMP);
	Patch<BYTE>(0x54EC85, 0x4);
	Nop(0x54EC87, 1);

	// Unlock 1.0/1.01 saves loading 
	Patch<WORD>(0x5ED299, 0xE990);

	// Old .set files working again
	static const DWORD		dwSetVersion = 6;
	Patch<const void*>(0x59040A, &dwSetVersion);
	Patch<BYTE>(0x5906ED, 6);
	Patch<BYTE>(0x53EB9A, 6);

	// Disable re-initialization of DirectInput mouse device by the game
	Patch<BYTE>(0x58A7C1, 0xEB);
	Patch<BYTE>(0x58A9A7, 0xEB);
	Patch<BYTE>(0x58AA89, 0xEB);

	// Unlocked widescreen resolutions
	//Patch<WORD>(0x779BAD, 0x607D);
	Patch<WORD>(0x779AB8, 0x697D);
	Patch<DWORD>(0x7798C8, 0x9090117D);
	Nop(0x779946, 2);
	Nop(0x7798CC, 2);

	// Make sure DirectInput mouse device is set non-exclusive (may not be needed?)
	Nop(0x77AA5F, 1);
	Patch<WORD>(0x77AA60, 0x01B0);

	// Default resolution to native resolution
	RECT			desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

	Patch<DWORD>(0x77A30F, desktop.right);
	Patch<DWORD>(0x77A314, desktop.bottom);
	Patch<const char*>(0x77A36B, aNoDesktopMode);

	// No DirectPlay dependency
	Patch<BYTE>(0x77B35E, 0xB8);
	Patch<DWORD>(0x77B35F, 0x900);

	// SHGetFolderPath on User Files
	InjectHook(0x778E90, GetMyDocumentsPathSA, PATCH_JUMP);

	// Fixed muzzleflash not showing from last bullet
	Nop(0x63E789, 2);

	// Proper randomizations
	InjectHook(0x45234C, Int32Rand); // Missing ped paths
	InjectHook(0x45284C, Int32Rand); // Missing ped paths
	InjectHook(0x69034F, Int32Rand); // Prostitutes

	// Help boxes showing with big message
	// Game seems to assume they can show together
	Nop(0x597E3A, 6);

	// Fixed lens flare
	Nop(0x72FFF8, 5);
	Patch<BYTE>(0x72FFE3, 0x20);
	InjectHook(0x730004, DynBaseAddress(0x753A00), PATCH_CALL);
	Patch<WORD>(0x730009, 0xE1EB);

	Nop(0x72FE17, 4);
	Patch<BYTE>(0x72FE1B, 0xB8);
	Patch(0x72FE1C, &FlushLensSwitchZ);

	Patch<DWORD>(0x72F81D, 0xB9909090);
	Patch(0x72F821, &InitBufferSwitchZ);

	// Y axis sensitivity fix
	float* sens = *(float**)DynBaseAddress(0x51B8D7);
	Patch<const void*>(0x51B8E3 + 0x2, sens);
	Patch<const void*>(0x51C5DC + 0x2, sens);
	Patch<const void*>(0x51D68A + 0x2, sens);
	Patch<const void*>(0x51E98A + 0x2, sens);
	Patch<const void*>(0x52FB31 + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<WORD>(0x51C51D, 0x29EB);
	Patch<WORD>(0x51CFA3, 0xE990);
	InjectHook(0x531B1E, DynBaseAddress(0x531BCF), PATCH_JUMP);

	// Fixed mirrors crash
	Patch<uint64_t>(0x753861, 0xC604C4832F74C085);

	// Mirrors depth fix & bumped quality
	InjectHook(0x7536C0, CreateMirrorBuffers);

	// Fixed MSAA options
	Patch<BYTE>(0x590EB7, 0xEB);
	Nop(0x590E74, 2);

	Patch<BYTE>(0x82B3BB, 0xEB);
	Patch<BYTE>(0x82A7E6, 0xEB);
	Patch<WORD>(0x82ADA3, 0xE990);

	ReadCall( 0x590ECB, orgGetMaxMultiSamplingLevels );
	InjectHook(0x590ECB, GetMaxMultiSamplingLevels);
	InjectHook(0x590E76, GetMaxMultiSamplingLevels);

	ReadCall( 0x588100, orgChangeMultiSamplingLevels );
	InjectHook(0x588100, ChangeMultiSamplingLevels);
	InjectHook(0x590EFB, ChangeMultiSamplingLevels);
	InjectHook(0x591051, ChangeMultiSamplingLevels);

	ReadCall( 0x77A2FD, orgSetMultiSamplingLevels );
	InjectHook(0x77A2FD, SetMultiSamplingLevels);

	Patch<WORD>(0x58DD2F, 0xBA90);
	Patch(0x58DD31, MSAAText);

	// Fixed car collisions - car you're hitting gets proper damage now
	Nop(0x553800, 2);
	InjectHook(0x553802, FixedCarDamage_Newsteam, PATCH_CALL);


	// Car explosion crash with multimonitor
	// Unitialized collision data breaking stencil shadows
	ReadCall( 0x41A661, orgMemMgrMalloc );
	InjectHook(0x41A661, CollisionData_MallocAndInit);

	ReadCall( 0x41A4CC, orgNewAlloc );
	InjectHook(0x41A4CC, CollisionData_NewAndInit);
	InjectHook(0x41A5A9, CollisionData_NewAndInit);


	// Crash when entering advanced display options on a dual monitor machine after:
	// - starting game on primary monitor in maximum resolution, exiting,
	// starting again in maximum resolution on secondary monitor.
	// Secondary monitor maximum resolution had to be greater than maximum resolution of primary monitor.
	// Not in 1.01
	ReadCall( 0x779A61, orgGetNumVideoModes );
	InjectHook(0x779A61, GetNumVideoModes_Store);
	InjectHook(0x7799C1, GetNumVideoModes_Retrieve);


	// Fixed escalators crash
	orgEscalatorsUpdate = (void(*)())DynBaseAddress(0x735A90);
	InjectHook(0x735AC5, UpdateEscalators, PATCH_JUMP);

	Patch<WORD>(0x734AAE, 0x4E8D);
	Patch<BYTE>(0x734AB0, 0x84);
	InjectHook(0x734AB1, &CEscalator::SwitchOffNoRemove, PATCH_CALL);
	Patch<WORD>(0x734AB6, 0x52EB);


	// Don't allocate constant memory for stencil shadows every frame
	InjectHook(0x75AC99, StencilShadowAlloc, PATCH_CALL);
	Nop(0x75ACD1, 3);
	Patch<WORD>(0x75AC9E, 0x2CEB);
	Patch<DWORD>(0x75AD25, 0x5D5B5E5F);	// pop edi, pop esi, pop ebx, pop ebp, retn
	Patch<BYTE>(0x75AD29, 0xC3);


	// "Streaming memory bug" fix
	InjectHook(0x4CF1DB, GTARtAnimInterpolatorSetCurrentAnim);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x43AD0B+1, 1); // knife
	Patch<BYTE>(0x43ADF8+1, 1); // knife
	Patch<BYTE>(0x43AF9F+1, 1); // chainsaw
	Patch<BYTE>(0x43B058+1, 1); // chainsaw
	Patch<BYTE>(0x43B9B8+1, 1); // parachute

	Patch<BYTE>(0x43C492, 0x53); // katana
	Patch<WORD>(0x43C493, 0x016A);


	// Proper aspect ratios
	static const float f43 = 4.0f/3.0f, f54 = 5.0f/4.0f, f169 = 16.0f/9.0f;
	Patch<const void*>(0x73414B, &f169);
	Patch<const void*>(0x734167, &f54);
	Patch<const void*>(0x73417A, &f43);
}

void Patch_SA_NewSteam_Common()
{
	using namespace Memory;
	using namespace hook;

	{
		void* isAlreadyRunning = get_pattern( "85 C0 74 08 33 C0 8B E5 5D C2 10 00", -5 );
		ReadCall( isAlreadyRunning, IsAlreadyRunning );
		InjectHook(isAlreadyRunning, InjectDelayedPatches_Newsteam);
	}

	// 6 extra directionals on Medium and higher
	// push dword ptr [CGame::currArea]
	// call GetMaxExtraDirectionals
	// add esp, 4
	// mov ebx, eax
	// nop
	{
		auto maxdirs_addr = pattern( "83 3D ? ? ? ? 00 8D 5E 05 74 05 BB 06 00 00 00" ).get_one();

		Patch( maxdirs_addr.get<void>(), { 0xFF, 0x35 } );
		InjectHook( maxdirs_addr.get<void>(6), GetMaxExtraDirectionals, PATCH_CALL );
		Patch( maxdirs_addr.get<void>(11), { 0x83, 0xC4, 0x04, 0x8B, 0xD8 } );
		Nop( maxdirs_addr.get<void>(16), 1 );
	}

	// AI accuracy issue
	{
		auto match = pattern( "8B 82 8C 05 00 00 85 C0 74 09" ).get_one(); // 0x76DEA7 in newsteam r1
		Nop(match.get<int>(0), 1);
		InjectHook( match.get<int>(1), WeaponRangeMult_VehicleCheck, PATCH_CALL );
	}

	// Don't catch WM_SYSKEYDOWN and WM_SYSKEYUP (fixes Alt+F4)
	{
		auto patternie = pattern( "8B 75 10 8B ? 14 56" ).count(2); // 0x77C588 and 0x77C5CC in newsteam r2
		auto defproc = get_pattern( "8B 4D 14 8B 55 10 8B 45 08" );

		patternie.for_each_result( [&]( pattern_match match ) {
			InjectHook( match.get<int>(0x39), defproc, PATCH_JUMP );
			Patch<uint8_t>( match.get<int>(1), 0x5D ); // esi -> ebx
			Patch<uint8_t>( match.get<int>(6), 0x53 ); // esi -> ebx
			Patch<uint8_t>( match.get<int>(0x26 + 1), 0xFB ); // esi -> ebx
			Patch<int8_t>( match.get<int>(8 + 2), -8 ); // use stack space for new lParam
			Patch<int8_t>( match.get<int>(0x18 + 2), -8 ); // use stack space for new lParam
			Patch<int8_t>( match.get<int>(0x2B + 2), -8 ); // use stack space for new lParam
		} );
	}


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	{
		using namespace VariableResets;

		auto timers_init = pattern( "89 45 FC DB 45 FC C6 05 ? ? ? ? 01" ).get_one();

		GameVariablesToReset.emplace_back( *timers_init.get<signed int*>(-17 + 2) );
		GameVariablesToReset.emplace_back( *timers_init.get<signed int*>(-11 + 2) );
		GameVariablesToReset.emplace_back( *timers_init.get<TimeNextMadDriverChaseCreated_t<float>*>(0x41 + 2) );
	}

	// FuckCarCompletely not fixing panels
	{
		void* panel_addr = get_pattern( "C6 46 04 FA 5E 5B", -3 );
		Nop(panel_addr, 3);
	}

	// 014C cargen counter fix (by spaceeinstein)
	{
		auto do_processing = pattern( "B8 C3 2E 57 06 F7 EE C1 FA 06" ).get_one();

		Patch<uint8_t>( do_processing.get<uint8_t*>(27 + 1), 0xBF ); // movzx eax, word ptr [edi+1Ah] -> movsx eax, word ptr [edi+1Ah]
		Patch<uint8_t>( do_processing.get<uint8_t*>(41), 0x74 ); // jge -> jz
	}

	// Linear filtering on script sprites
	{
		void* drawScriptSprites = get_pattern( "81 EC 94 01 00 00 53 56 57 50", 10 );
		ReadCall( drawScriptSprites, orgDrawScriptSpritesAndRectangles );
		InjectHook( drawScriptSprites, DrawScriptSpritesAndRectangles );
	}

	// Animated Phoenix hood scoop
	// TODO

	// Extra animations for planes
	// TODO

	// Fixed animations for boats
	// TODO

	// Stop BF Injection/Bandito/Hotknife rotating engine components when engine is off
	// TODO


	// Make freeing temp objects more aggressive to fix vending crash
	InjectHook( get_pattern( "57 8B 78 08 89 45 FC 85 FF 74 5B", -9 ), CObject::TryToFreeUpTempObjects_SilentPatch, PATCH_JUMP );


	// Remove FILE_FLAG_NO_BUFFERING from CdStreams
	Patch<uint8_t>( get_pattern( "81 F9 00 08 00 00 ? 05", 6 ), 0xEB );


	// Proper metric-imperial conversion constants
	static const double METERS_TO_FEET_DIV = 1.0 / 3.280839895;
	Patch<const void*>( get_pattern( "83 EC 08 DC 35 ? ? ? ? DD 1C 24", 3 + 2 ), &METERS_TO_FEET_DIV );
	Patch<const void*>( get_pattern( "51 DC 35 ? ? ? ? DD 1C 24", 1 + 2 ), &METERS_TO_FEET_DIV );


	// Fixed impounding of random vehicles (because CVehicle::~CVehicle doesn't remove cars from apCarsToKeep)
	{
		void* recordVehicleDeleted = get_pattern( "E8 ? ? ? ? 33 C0 66 89 86" );
		ReadCall( recordVehicleDeleted, orgRecordVehicleDeleted );
		InjectHook( recordVehicleDeleted, RecordVehicleDeleted_AndRemoveFromVehicleList );
	}


	// Don't include an extra D3DLIGHT on vehicles since we fixed directional already
	// TODO when timecyc.dat illumination is fixed


	// Fixed CAEAudioUtility timers - not typecasting to float so we're not losing precision after X days of PC uptime
	// Also fixed integer division by zero
	{
		auto staticInitialize = pattern( "FF 15 ? ? ? ? 5F 5E 85 C0" ).get_one();

		Patch( staticInitialize.get<void>( 2 ), &pAudioUtilsFrequency );
		InjectHook( staticInitialize.get<void>( 0x1E ), AudioUtilsGetStartTime );
		InjectHook( get_pattern( "50 FF 15 ? ? ? ? DF 6D F8", -9 ), AudioUtilsGetCurrentTimeInMs, PATCH_JUMP );
	}


	// Car generators placed in interiors visible everywhere
	InjectHook( get_pattern( "E8 ? ? ? ? 0F B6 57 0A" ), &CEntity::SetPositionAndAreaCode );


	// Fixed bomb ownership/bombs saving for bikes
	{
		auto restoreForHideout = get_pattern( "8D 4E EE E8", 3 );
		auto restoreImpoundGarage = get_pattern( "8D 4F EE E8", 3 );

		ReadCall( restoreForHideout, CStoredCar::orgRestoreCar );
		InjectHook( restoreForHideout, &CStoredCar::RestoreCar_SilentPatch );
		InjectHook( restoreImpoundGarage, &CStoredCar::RestoreCar_SilentPatch );
	}


	// unnamed CdStream semaphore
	{
		auto semaName = pattern( "52 6A 40 FF 15" ).get_one();

		Patch( semaName.get<void>( 9 ), { 0x6A, 0x00 } ); // push 0 \ nop
		Nop( semaName.get<void>( 9 + 2 ), 3 );
	}


	// Correct streaming when using RC vehicles
	InjectHook( get_pattern( "88 1D ? ? ? ? E8 ? ? ? ? 8B F0 83 C4 04 3B F3", 6 ), FindPlayerEntityWithRC );
	InjectHook( get_pattern( "E8 ? ? ? ? 83 C4 08 85 C0 74 07 C6 05" ), FindPlayerVehicle_RCWrap );


	// Fixed triangle above recruitable peds' heads
	Patch<uint8_t>( get_pattern( "83 BE 98 05 00 00 ? D9 45 DC", 6 ), 8 ); // GANG2


	// Credits =)
	{
		auto renderCredits = pattern( "83 C4 18 E8 ? ? ? ? 80 3D" ).get_one();

		ReadCall( renderCredits.get<void>( -58 ), Credits::PrintCreditText );
		ReadCall( renderCredits.get<void>( -5 ), Credits::PrintCreditText_Hooked );
		InjectHook( renderCredits.get<void>( -5 ), Credits::PrintSPCredits );
	}

	// Fixed ammo from SCM
	{
		void* giveWeapon = get_pattern( "8B CE E8 ? ? ? ? 8B CE 8B D8", 2 );

		ReadCall( giveWeapon, CPed::orgGiveWeapon );
		InjectHook( giveWeapon, &CPed::GiveWeapon_SP );
	}


	// Fixed bicycle on fire - instead of CJ being set on fire, bicycle's driver is
	{
		using namespace BicycleFire;

		auto doStuffToGoOnFire = pattern( "83 BF 94 05 00 00 0A 75 6D 6A" ).get_one(); // 0x0054A6BE
		constexpr ptrdiff_t START_FIRE_OFFSET = 0x31;

		Patch( doStuffToGoOnFire.get<void>( 9 ), { 0x90, 0x57 } ); // nop \ push edi
		Patch( doStuffToGoOnFire.get<void>( START_FIRE_OFFSET ), { 0x90, 0x57 } ); // nop \ push edi
		InjectHook( doStuffToGoOnFire.get<void>( 9 + 2 ), GetVehicleDriver );
		InjectHook( doStuffToGoOnFire.get<void>( START_FIRE_OFFSET + 2 ), GetVehicleDriver );

		ReadCall( doStuffToGoOnFire.get<void>( 0x15 ), CPlayerPed::orgDoStuffToGoOnFire );
		InjectHook( doStuffToGoOnFire.get<void>( 0x15 ), DoStuffToGoOnFire_NullAndPlayerCheck );

		ReadCall( doStuffToGoOnFire.get<void>( START_FIRE_OFFSET + 0x10 ), CFireManager::orgStartFire );
		InjectHook( doStuffToGoOnFire.get<void>( START_FIRE_OFFSET + 0x10 ), &CFireManager::StartFire_NullEntityCheck );
	}


	// Decreased keyboard input latency
	{
		using namespace KeyboardInputFix;

		auto updatePads = pattern( "E8 ? ? ? ? B9 ? ? ? ? BE" ).get_one(); // 0x552DB7

		NewKeyState = *updatePads.get<void*>( 10 + 1 );
		OldKeyState = *updatePads.get<void*>( 15 + 1 );
		TempKeyState = *updatePads.get<void*>( 27 + 1 );
		objSize = *updatePads.get<uint32_t>( 5 + 1 ) * 4;

		ReadCall( updatePads.get<void>( -44 ), orgClearSimButtonPressCheckers );
		InjectHook( updatePads.get<void>( -44 ), ClearSimButtonPressCheckers );
		Nop( updatePads.get<void>( 20 ), 2 );
		Nop( updatePads.get<void>( 37 ), 2 );
	}

	// Fixed handling.cfg name matching (names don't need unique prefixes anymore)
	{
		using namespace HandlingNameLoadFix;

		auto findExactWord = pattern( "8B 55 08 56 8D 4D EC" ).get_one(); // 0x6F849B

		InjectHook( findExactWord.get<void>( -5 ), strncpy_Fix );
		InjectHook( findExactWord.get<void>( 9 ), strncmp_Fix );
	}
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		hDLLModule = hinstDLL;

		const HINSTANCE hInstance = GetModuleHandle( nullptr );
		std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( hInstance, ".text" );
		ScopedUnprotect::Section Protect2( hInstance, ".rdata" );

		const int8_t version = Memory::GetVersion().version;
		if ( version == 0 ) Patch_SA_10();
		else if ( version == 1 ) Patch_SA_11(); // Not supported anymore
		else if ( version == 2 ) Patch_SA_Steam(); // Not supported anymore
		else
		{
			if ( version == 3 ) Patch_SA_NewSteam_r1();
			else if ( version == 4 ) Patch_SA_NewSteam_r2();
			else if ( version == 5 ) Patch_SA_NewSteam_r2_lv();
			Patch_SA_NewSteam_Common();
		}	
	}
	return TRUE;
}

extern "C" __declspec(dllexport)
uint32_t GetBuildNumber()
{
	return (SILENTPATCH_REVISION_ID << 8) | SILENTPATCH_BUILD_ID;
}