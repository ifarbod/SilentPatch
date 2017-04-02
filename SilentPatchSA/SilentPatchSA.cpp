#include "StdAfxSA.h"
#include <limits>
#include <algorithm>
#include <d3d9.h>
#include <Shlwapi.h>
#include <ShlObj.h>

#include "ScriptSA.h"
#include "GeneralSA.h"
#include "ModelInfoSA.h"
#include "VehicleSA.h"
#include "PedSA.h"
#include "AudioHardwareSA.h"
#include "LinkListSA.h"
#include "PNGFile.h"

#include "WaveDecoderSA.h"
#include "FLACDecoderSA.h"

#include "Patterns.h"
#include "DelimStringReader.h"

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
static void* var_rwD3D9SetVertexShader = AddressByVersion<void*>(0x7F9FB0, 0x7FA8B0, 0x833F70);
WRAPPER void _rwD3D9SetVertexShader(void *shader) { VARJMP(var_rwD3D9SetVertexShader); }
static void* varRwD3D9CreateVertexShader = AddressByVersion<void*>(0x7FAC60, 0x7FB560, 0x834C20);
WRAPPER RwBool RwD3D9CreateVertexShader(const RwUInt32 *function, void **shader) { VARJMP(varRwD3D9CreateVertexShader); }
static void* var_rwD3D9SetVertexShaderConstant = AddressByVersion<void*>(0x7FACA0, 0x7FB5A0, 0x834C60);
WRAPPER void _rwD3D9SetVertexShaderConstant(RwUInt32 registerAddress,
                               const void *constantData,
							   RwUInt32  constantCount) { VARJMP(var_rwD3D9SetVertexShaderConstant); }

static void* varRwD3D9GetTransform = AddressByVersion<void*>(0x7FA4F0, 0x7FADF0, 0x8344B0);
WRAPPER void _RwD3D9GetTransform(RwUInt32 state, void* matrix) { VARJMP(varRwD3D9GetTransform); }

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
auto					SetVolume = AddressByVersion<void(__thiscall*)(void*,float)>(0x4D7C60, 0x4D7E60, 0x4E2750);
auto					InitializeUtrax = AddressByVersion<void(__thiscall*)(void*)>(0x4F35B0, 0x4F3A10, 0x4FFA80);
auto					CanSeeOutSideFromCurrArea = AddressByVersion<bool(*)()>(0x53C4A0, 0x53C940, 0x54E440);

auto					__rwD3D9TextureHasAlpha = AddressByVersion<BOOL(*)(RwTexture*)>(0x4C9EA0, 0x4CA090, 0x4D47E0);
auto					RenderOneXLUSprite = AddressByVersion<void(*)(float, float, float, float, float, int, int, int, int, float, char, char, char)>(0x70D000, 0x70D830, 0x7592C0);

static BOOL				(*IsAlreadyRunning)();
static void				(*TheScriptsLoad)();
static void				(*WipeLocalVariableMemoryForMissionScript)();
static bool				(*InitialiseRenderWare)();
static void				(*ShutdownRenderWare)();
static void				(*DoSunAndMoon)();
static void				(*D3D9RenderPreLit)(void*, void*, void*, void*);

auto 					WorldRemove = AddressByVersion<void(*)(CEntity*)>(0x563280, 0, 0x57D370, 0x57C480, 0x57C3B0);


// SA variables
void**					rwengine = *AddressByVersion<void***>(0x58FFC0, 0x53F032, 0x48C194, 0x48B167, 0x48B167);
RwInt32&				ms_extraVertColourPluginOffset = **AddressByVersion<int**>(0x5D6362, 0x5D6B42, 0x5F2B65);

unsigned char&			nGameClockDays = **AddressByVersion<unsigned char**>(0x4E841D, 0x4E886D, 0x4F3871);
unsigned char&			nGameClockMonths = **AddressByVersion<unsigned char**>(0x4E842D, 0x4E887D, 0x4F3861);
void*&					pUserTracksStuff = **AddressByVersion<void***>(0x4D9B7B, 0x4DA06C, 0x4E4A43);
bool&					CCutsceneMgr__ms_running = **AddressByVersion<bool**>(0x53F92D, 0x434241, 0x422061);

float&					fFarClipZ = **AddressByVersion<float**>(0x70D21F, 0x70DA4F, 0x421AB2);
RwTexture** const		gpCoronaTexture = *AddressByVersion<RwTexture***>(0x6FAA8C, 0x6FB2BC, 0x5480BF);
int&					MoonSize = **AddressByVersion<int**>(0x713B0C, 0x71433C, 0x72F0AB);

CZoneInfo*&				pCurrZoneInfo = **AddressByVersion<CZoneInfo***>(0x58ADB1, 0x58B581, 0x407F93);
CRGBA*					HudColour = *AddressByVersion<CRGBA**>(0x58ADF6, 0x58B5C6, 0x440648);

float&					m_fDNBalanceParam = **AddressByVersion<float**>(0x4A9062, 0x4A90F2, 0x4B2512);
RpLight*&				pAmbient = **AddressByVersion<RpLight***>(0x5BA53A, 0x735D11, 0x5D90F0);

CLinkListSA<CPed*>&			ms_weaponPedsForPC = **AddressByVersion<CLinkListSA<CPed*>**>(0x53EACA, 0x53EF6A, 0x551101);
CLinkListSA<AlphaObjectInfo>&	m_alphaList = **AddressByVersion<CLinkListSA<AlphaObjectInfo>**>(0x733A4D, 0x73427D, 0x76DCA3);


// Custom variables
static float		fSunFarClip;
static RwTexture*	gpMoonMask = nullptr;
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


// Regular functions
static bool AtomicAlphaTest( RpAtomic* atomic )
{
	bool hasAlpha = false;
	RpGeometryForAllMaterials( RpAtomicGetGeometry(atomic), [&hasAlpha]( RpMaterial* material ) -> RpMaterial* {
		if ( RpMaterialGetTexture(material) != nullptr )
		{
			if ( __rwD3D9TextureHasAlpha(RpMaterialGetTexture(material)) )
			{
				hasAlpha = true;
				return nullptr;
			}
		}
		else if ( RpMaterialGetColor(material)->alpha < 255 )
		{
			hasAlpha = true;
			return nullptr;
		}

		return material;
	} );

	return hasAlpha;
}

static RpAtomic* RenderAtomic(RpAtomic* pAtomic, float fComp)
{
	UNREFERENCED_PARAMETER(fComp);
	return AtomicDefaultRenderCallBack(pAtomic);
}

RpAtomic* OnePassAlphaRender(RpAtomic* atomic)
{
	BOOL	nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));

	atomic = AtomicDefaultRenderCallBack(atomic);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));
	return atomic;
}

RpAtomic* TwoPassAlphaRender_aap(RpAtomic* atomic)
{
	// For cutscenes, fall back to one-pass render
	if ( CCutsceneMgr__ms_running && !CanSeeOutSideFromCurrArea() )
		return AtomicDefaultRenderCallBack(atomic);

	int		nPushedAlpha, nAlphaFunction;
	int		nZWrite;
	int		nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &nZWrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &nAlphaFunction);

	// 1st pass
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	if (nPushedAlpha == 100)	// or should we just force it? do we ever use something else anyway?
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(128));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONGREATEREQUAL));

	atomic = AtomicDefaultRenderCallBack(atomic);

	if ( atomic != nullptr )
	{
		// 2nd pass
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONLESS));
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

		atomic = AtomicDefaultRenderCallBack(atomic);
	}

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(nZWrite));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));

	return atomic;
}

RpAtomic* TwoPassAlphaRender_Silent(RpAtomic* atomic)
{
	// For cutscenes, fall back to one-pass render
	if ( CCutsceneMgr__ms_running && !CanSeeOutSideFromCurrArea() )
		return AtomicDefaultRenderCallBack(atomic);

	int		nPushedAlpha, nAlphaFunction;
	int		nZWrite;
	int		nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &nZWrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &nAlphaFunction);

	// 1st pass
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(FALSE));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(255));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONEQUAL));

	atomic = AtomicDefaultRenderCallBack(atomic);

	if ( atomic != nullptr )
	{
		// 2nd pass
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONLESS));
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

		atomic = AtomicDefaultRenderCallBack(atomic);
	}

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(nZWrite));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));

	return atomic;
}

RpAtomic* StaticPropellerRender(RpAtomic* pAtomic)
{
	int		nPushedAlpha;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
	pAtomic = AtomicDefaultRenderCallBack(pAtomic);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	return pAtomic;
}

template <RpAtomic* renderer(RpAtomic*)>
RpAtomic* RenderBigVehicleActomic(RpAtomic* pAtomic, float fComp)
{
	UNREFERENCED_PARAMETER(fComp);

	const char*		pNodeName = GetFrameNodeName(RpAtomicGetFrame(pAtomic));

	if ( !strncmp(pNodeName, "moving_prop", 11) )
		return renderer(pAtomic);

	if ( !strncmp(pNodeName, "static_prop", 11) )
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

template <RpAtomic* renderer(RpAtomic*)>
void SetRendererForAtomic(RpAtomic* pAtomic)
{
	if ( AtomicAlphaTest( pAtomic ) )
		RpAtomicSetRenderCallBack(pAtomic, renderer);
}

template <RpAtomic* renderer(RpAtomic*)>
void SetRendererForAtomic_NoTest(RpAtomic* pAtomic)
{
	RpAtomicSetRenderCallBack(pAtomic, renderer);
}

void RenderWeapon(CPed* pPed)
{
	pPed->RenderWeapon(false, false);
	ms_weaponPedsForPC.Insert(pPed);
}

void RenderWeaponPedsForPC()
{
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

	for ( auto it = ms_weaponPedsForPC.Next( nullptr ); it != nullptr; it = ms_weaponPedsForPC.Next( it ) )
	{
		CPed* ped = **it;
		ped->SetupLighting();
		ped->RenderWeapon(true, false);
		ped->RemoveLighting();
	}
}

/*void RenderWeaponsList()
{
	int		nPushedAlpha, nAlphaFunction;
	int		nZWrite;
	int		nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &nZWrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &nAlphaFunction);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(255));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONLESS));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

	for ( auto i = ms_weaponPedsForPC.m_lnListHead.m_pNext; i != &ms_weaponPedsForPC.m_lnListTail; i = i->m_pNext )
	{
		i->V()->SetupLighting();
		RenderWeaponHooked(i->V());
		i->V()->RemoveLighting();
	}

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(nZWrite));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));
}*/

template <RpAtomic* renderer(RpAtomic*)>
RpAtomic* RenderPedCB(RpAtomic* pAtomic)
{
	if ( AtomicAlphaTest( pAtomic ) )
		return renderer(pAtomic);
	
	return AtomicDefaultRenderCallBack(pAtomic);
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
	static bool		bInitScriptStuff = false;
	if ( !bInitScriptStuff )
	{
		ScriptSpace = *AddressByVersion<unsigned char**>(0x5D5380, 0x5D5B60, 0x450E34);
		ScriptParams = *AddressByVersion<int**>(0x48995B, 0x46410A, 0x46979A);
		ScriptFileSize = *AddressByVersion<size_t*>( 0x468E74+1, 0, 0x46E572+1);
		ScriptMissionSize = *AddressByVersion<size_t*>( 0x489A5A+1, 0, 0x490798+1);

		bInitScriptStuff = true;
	}
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
	static const uint8_t bOldCode[22] = {
		0xD6, 0x00, 0x04, 0x00, 0x39, 0x00, 0x03, 0xEF, 0x00, 0x04, 0x02, 0x4D,
		0x00, 0x01, 0x90, 0xF2, 0xFF, 0xFF, 0xD6, 0x00, 0x04, 0x01
		};

	// Faulty code lies under offset 3367 - replace it if it matches
	if ( memcmp( ScriptSpace+ScriptFileSize+3367, bOldCode, sizeof(bOldCode) ) == 0 )
	{
		const uint8_t bNewCode[22] = {
			0x00, 0x00, 0x00, 0x00, 0xD6, 0x00, 0x04, 0x03, 0x39, 0x00, 0x03, 0x2B,
			0x00, 0x04, 0x0B, 0x39, 0x00, 0x03, 0xEF, 0x00, 0x04, 0x02
		};

		memcpy( ScriptSpace+ScriptFileSize+3367, bNewCode, sizeof(bNewCode) );
	}
}

static void QuadrupleStuntBonus()
{
	// IF HEIGHT_FLOAT_HJ > 4.0 -> IF HEIGHT_INT_HJ > 4
	auto pattern = hook::range_pattern( uintptr_t(ScriptSpace), uintptr_t(ScriptSpace+ScriptFileSize), "20 00 02 60 14 06 00 00 80 40" ).count_hint(1);
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

	// INITIAL - Basketball fix, Quadruple Stunt Bonus
	if ( ScriptParams[0] == 0 )
	{
		BasketballFix(ScriptSpace+ScriptFileSize, ScriptMissionSize);
		QuadrupleStuntBonus();
	}
	// HOODS5 - Sweet's Girl fix
	else if ( ScriptParams[0] == 18 )
		SweetsGirlFix();
	// WUZI1 - Mountain Cloud Boys fix
	else if ( ScriptParams[0] == 53 )
		MountainCloudBoysFix();
}

// 1.01 kinda fixed it
bool GetCurrentZoneLockedOrUnlocked(float fPosX, float fPosY)
{
	// Exploit RAII really bad
	static const float		GridXOffset = **(float**)(0x572135+2), GridYOffset = **(float**)(0x57214A+2);
	static const float		GridXSize = **(float**)(0x57213B+2), GridYSize = **(float**)(0x572153+2);
	static const int		GridXNum = (2.0f*GridXOffset) * GridXSize, GridYNum = (2.0f*GridYOffset) * GridYSize;

	static unsigned char* const	ZonesVisited = *(unsigned char**)(0x57216A) - (GridYNum-1);		// 1.01 fixed it!

	int		Xindex = (fPosX+GridXOffset) * GridXSize;
	int		Yindex = (fPosY+GridYOffset) * GridYSize;

	// "Territories fix"
	if ( (Xindex >= 0 && Xindex < GridXNum) && (Yindex >= 0 && Yindex < GridYNum) )
		return ZonesVisited[GridXNum*Xindex - Yindex + (GridYNum-1)] != 0;
	
	// Outside of map bounds
	return true;
}

bool GetCurrentZoneLockedOrUnlocked_Steam(float fPosX, float fPosY)
{
	static unsigned char* const	ZonesVisited = *(unsigned char**)(0x5870E8) - 9;

	int		Xindex = (fPosX+3000.0f) / 600.0f;
	int		Yindex = (fPosY+3000.0f) / 600.0f;

	// "Territories fix"
	if ( (Xindex >= 0 && Xindex < 10) && (Yindex >= 0 && Yindex < 10) )
		return ZonesVisited[10*Xindex - Yindex + 9] != 0;

	// Outside of map bounds
	return true;
}

// By NTAuthority
void DrawMoonWithPhases(int moonColor, float* screenPos, float sizeX, float sizeY)
{
	if ( gpMoonMask == nullptr )
	{	
		if ( GetFileAttributes("lunar.png") != INVALID_FILE_ATTRIBUTES )
		{
			// load from file
			gpMoonMask = CPNGFile::ReadFromFile("lunar.png");
		}
		else
		{
			// Load from memory

			HMODULE thisModule;
			GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)DrawMoonWithPhases, &thisModule);

			HRSRC		resource = FindResource(thisModule, MAKEINTRESOURCE(IDR_LUNAR64), RT_RCDATA);
			void*		pMoonMask = static_cast<void*>(LoadResource(thisModule, resource));

			gpMoonMask = CPNGFile::ReadFromMemory(pMoonMask, SizeofResource(thisModule, resource));

			FreeResource(pMoonMask);
		}
	}
	//D3DPERF_BeginEvent(D3DCOLOR_ARGB(0,0,0,0), L"render moon");

	float currentDayFraction = nGameClockDays / 31.0f;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	float a10 = 1.0f / fFarClipZ;
	float size = (MoonSize * 2) + 4.0f;

	RwD3D9SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

	RenderOneXLUSprite(screenPos[0], screenPos[1], fFarClipZ, sizeX * size, sizeY * size, 0, 0, 0, 0, a10, -1, 0, 0);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gpMoonMask));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
	
	float maskX = (sizeX * size) * 5.4f * (currentDayFraction - 0.5f) + screenPos[0];
	float maskY = screenPos[1] + ((sizeY * size) * 0.7f);

	RenderOneXLUSprite(maskX, maskY, fFarClipZ, sizeX * size * 1.7f, sizeY * size * 1.7f, 0, 0, 0, 255, a10, -1, 0, 0);

	RwD3D9SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gpCoronaTexture[2]));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDDESTALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);

	RenderOneXLUSprite(screenPos[0], screenPos[1], fFarClipZ, sizeX * size, sizeY * size, moonColor, moonColor, moonColor * 0.85f, 255, a10, -1, 0, 0);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	//D3DPERF_EndEvent();
}

CRGBA* CRGBA::BlendGangColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	double colourIntensity = static_cast<double>(pCurrZoneInfo->ZoneColour.a) / 255.0;
	*this = BlendSqr(CRGBA(r, g, b), HudColour[3], colourIntensity);
	this->a = a;

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
	if ( bX1 )
		rect.x1 -= 0.5f;

	if ( bY1 )
		rect.y1 -= 0.5f;

	if ( bX2 )
		rect.x2 -= 0.5f;

	if ( bY2 )
		rect.y2 -= 0.5f;

	// Steam CSprite2d::DrawRect
	((void(*)(const CRect&, const CRGBA&))0x75CDA0)(rect, rgba);
}

char* GetMyDocumentsPath()
{
	static char	cUserFilesPath[MAX_PATH];
	static char*  ppTempBufPtr = *GetVer() == 0 ? *AddressByRegion_10<char**>(0x744FE5) : cUserFilesPath;

	if (ppTempBufPtr[0] == '\0')
	{	
		static char** const ppUserFilesDir = AddressByVersion<char**>(0x74503F, 0x74586F, 0x77EE50, 0x77902B, 0x778F1B);

		char		cTmpPath[MAX_PATH];

		SHGetFolderPath(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, ppTempBufPtr);
		PathAppend(ppTempBufPtr, *ppUserFilesDir);
		CreateDirectory(ppTempBufPtr, nullptr);

		strcpy(cTmpPath, ppTempBufPtr);
		PathAppend(cTmpPath, "Gallery");
		CreateDirectory(cTmpPath, nullptr);

		strcpy(cTmpPath, ppTempBufPtr);
		PathAppend(cTmpPath, "User Tracks");
		CreateDirectory(cTmpPath, nullptr);
	}
	return ppTempBufPtr;
}

static LARGE_INTEGER	FrameTime;
int32_t GetTimeSinceLastFrame()
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

DWORD*				msaaValues = *AddressByVersion<DWORD**>(0x4CCBC5, 0x4CCDB5, 0x4D7462, 0x4D6CE5, 0x4D6CB5);
RwRaster*&			pMirrorBuffer = **AddressByVersion<RwRaster***>(0x723001, 0x723831, 0x754971, 0x74F3E1, 0x74F311);
RwRaster*&			pMirrorZBuffer = **AddressByVersion<RwRaster***>(0x72301C, 0x72384C, 0x75498C, 0x74F3FC, 0x74F32C);
void CreateMirrorBuffers()
{
	if ( pMirrorBuffer == nullptr )
	{
		DWORD oldMsaa[2] = { msaaValues[0], msaaValues[1] };
		msaaValues[0] = msaaValues[1] = 0;

		DWORD quality = *(DWORD*)((BYTE*)g_fx + 0x54);
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
	sprintf( buffer, "%ux", 1 << level );
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
	static char* pMemory = nullptr;
	if ( pMemory == nullptr )
	{
		pMemory = static_cast<char*>( orgNewAlloc( 3 * 0x6000 ) );
		pStencilShadowsPad[0] = pMemory;
		pStencilShadowsPad[1] = pMemory+0x6000;
		pStencilShadowsPad[2] = pMemory+(2*0x6000);
	}
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

void __stdcall CdStreamSetFilePointer( HANDLE hFile, uint32_t distanceToMove, void*, DWORD dwMoveMethod )
{
	LARGE_INTEGER li;
	li.QuadPart = int64_t(distanceToMove) << 11;
	SetFilePointerEx( hFile, li, nullptr, dwMoveMethod );
}
static auto* pCdStreamSetFilePointer = CdStreamSetFilePointer;


static signed int& LastTimeFireTruckCreated = **AddressByVersion<int**>(0x42131F + 2, 0, 0x42224D + 2);
static signed int& LastTimeAmbulanceCreated = **AddressByVersion<int**>(0x421319 + 2, 0, 0x422247 + 2);
static float& TimeNextMadDriverChaseCreated = **AddressByVersion<float**>(0x421369 + 2, 0, 0x42229D + 2);
static void (*orgCarCtrlReInit)();
void CarCtrlReInit_SilentPatch()
{
	orgCarCtrlReInit();
	LastTimeFireTruckCreated = 0;
	LastTimeAmbulanceCreated = 0;
	TimeNextMadDriverChaseCreated = (static_cast<float>(Int32Rand()) / INT32_MAX) * 600.0f + 600.0f;
}

static signed int* LastTimeFireTruckCreated_Newsteam;
static signed int* LastTimeAmbulanceCreated_Newsteam;
static float* TimeNextMadDriverChaseCreated_Newsteam;
void CarCtrlReInit_SilentPatch_Newsteam()
{
	orgCarCtrlReInit();
	*LastTimeFireTruckCreated_Newsteam = 0;
	*LastTimeAmbulanceCreated_Newsteam = 0;
	*TimeNextMadDriverChaseCreated_Newsteam = (static_cast<float>(Int32Rand()) / INT32_MAX) * 600.0f + 600.0f;
}

static void (*orgDrawScriptSpritesAndRectangles)(uint8_t);
void DrawScriptSpritesAndRectangles( uint8_t arg )
{
	RwRenderStateSet( rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR );
	orgDrawScriptSpritesAndRectangles( arg );
}

std::vector< std::pair<int32_t, bool> > doubleRearWheelsList;
void ReadDoubleRearWheels(const wchar_t* pPath)
{
	const size_t SCRATCH_PAD_SIZE = 32767;
	WideDelimStringReader reader( SCRATCH_PAD_SIZE );

	GetPrivateProfileSectionW( L"DoubleRearWheels", reader.GetBuffer(), reader.GetSize(), pPath );
	while ( const wchar_t* str = reader.GetString() )
	{
		wchar_t textLine[32];
		wchar_t* context = nullptr;
		wchar_t* token;

		wcscpy_s( textLine, str );
		token = wcstok_s( textLine, L"=", &context );
			
		int toList = _wtoi( token );
		if ( toList != 0 )
		{
			bool value = _wtoi( wcstok_s( nullptr, L"=", &context ) ) != 0;
			doubleRearWheelsList.emplace_back( toList, value );
		}
	}
}

bool __stdcall CheckDoubleRWheelsList( void* modelInfo, uint8_t* handlingData )
{
	static void* lastModelInfo = nullptr;
	static bool lastResult = false;

	if ( modelInfo == lastModelInfo ) return lastResult;
	lastModelInfo = modelInfo;

	ptrdiff_t modelID = std::distance( ms_modelInfoPtrs, std::find( ms_modelInfoPtrs, ms_modelInfoPtrs+m_numModelInfoPtrs, modelInfo ) );

	auto it = std::find_if( doubleRearWheelsList.begin(), doubleRearWheelsList.end(), [modelID]( const auto& item ) {
			return item.first == modelID;
		} );
	if ( it == doubleRearWheelsList.end() )
	{
		uint32_t flags = *(uint32_t*)(handlingData+0xCC);
		lastResult = (flags & 0x20000000) != 0;
		return lastResult;
	}

	lastResult = it->second;
	return lastResult;
}


#include <xnamath.h>

static void*					pNVCShader = nullptr;
static bool						bRenderNVC = false;
static bool						bXMSupported;

bool ShaderAttach()
{
	// CGame::InitialiseRenderWare
	if ( InitialiseRenderWare() )
	{
		HMODULE thisModule;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)ShaderAttach, &thisModule);

		HRSRC		resource = FindResource(thisModule, MAKEINTRESOURCE(IDR_NVCSHADER), RT_RCDATA);
		RwUInt32*	shader = static_cast<RwUInt32*>(LoadResource(thisModule, resource));

		RwD3D9CreateVertexShader(shader, reinterpret_cast<void**>(&pNVCShader));

		FreeResource(shader);

		bXMSupported = XMVerifyCPUSupport() != FALSE;
		return true;
	}
	return false;
}

void ShaderDetach()
{
	if ( pNVCShader != nullptr )
	{
		RwD3D9DeleteVertexShader(pNVCShader);
		pNVCShader = nullptr;
	}

	// PluginDetach?
	ShutdownRenderWare();
}

// Function for 1.01
BOOL Initialise3D(void* pParam)
{
	RwBool	(*RsRwInitialize)(void*);
	Memory::ReadCall( 0x5BFB92, RsRwInitialize);
	if ( RsRwInitialize(pParam) )
		return ShaderAttach();
	return false;
}

void SetShader(RxD3D9InstanceData* pInstData)
{
	if (bRenderNVC )
	{
		float			fEnvVars[2] = { m_fDNBalanceParam, RpMaterialGetColor(pInstData->material)->alpha * (1.0f/255.0f) };
		RwRGBAReal*		AmbientLight = RpLightGetColor(pAmbient);

		// Normalise the balance
		if ( fEnvVars[0] < 0.0f )
			fEnvVars[0] = 0.0f;
		else if ( fEnvVars[0] > 1.0f )
			fEnvVars[0] = 1.0f;

		RwD3D9SetVertexShader(pNVCShader);

		//_rwD3D9VSSetActiveWorldMatrix(RwFrameGetLTM(RpAtomicGetFrame(pRenderedAtomic)));
		//_rwD3D9VSSetActiveWorldMatrix(RwFrameGetMatrix(RpAtomicGetFrame(pRenderedAtomic)));
		//_rwD3D9VSGetComposedTransformMatrix(&outMat);

		XMMATRIX	worldMat, viewMat, projMat;
		XMMATRIX	worldViewProjMat;
		_RwD3D9GetTransform(D3DTS_WORLD, &worldMat);
		_RwD3D9GetTransform(D3DTS_VIEW, &viewMat);
		_RwD3D9GetTransform(D3DTS_PROJECTION, &projMat);

		if ( bXMSupported )
		{
			worldViewProjMat = XMMatrixMultiply(XMMatrixMultiply(worldMat, viewMat), projMat);
		}
		else
		{
			XMMATRIX		tempMat;
			ZeroMemory(&worldViewProjMat, sizeof(worldViewProjMat));
			ZeroMemory(&tempMat, sizeof(tempMat));

			for( int i = 0; i < 4; i++ )
			{ 
				for( int j = 0; j < 4; j++ )
				{ 
					for(int x = 0; x < 4; x++)
						tempMat.m[i][j] += worldMat.m[i][x] * viewMat.m[x][j];
				}
			}

			for( int i = 0; i < 4; i++ )
			{ 
				for( int j = 0; j < 4; j++ )
				{ 
					for(int x = 0; x < 4; x++)
						worldViewProjMat.m[i][j] += tempMat.m[i][x] * projMat.m[x][j];
				}
			}

		}

		//RwD3D9SetVertexShaderConstant(2, &worldMat, 4);
		//RwD3D9SetVertexShaderConstant(6, &viewMat, 4);
		//RwD3D9SetVertexShaderConstant(10, &projMat, 4);
		RwD3D9SetVertexShaderConstant(2, &worldViewProjMat, 4);

		RwD3D9SetVertexShaderConstant(0, fEnvVars, 1);
		RwD3D9SetVertexShaderConstant(1, AmbientLight, 1);
	}
	else
		RwD3D9SetVertexShader(pInstData->vertexShader);
}

void __declspec(naked) SetShader2()
{
	_asm
	{
		mov		bRenderNVC, 1
		push    ecx
		push    edx
		push    edi
		push    ebp
		call	D3D9RenderPreLit
		add		esp, 10h
		mov		bRenderNVC, 0
		retn
	}
}

static void*	pJackedEsi;
static void*	PassDayColoursToShader_NextIt = AddressByVersion<void*>(0x5D6382, 0x5D6B62, 0x5F2B81);
static void*	PassDayColoursToShader_Return = AddressByVersion<void*>(0x5D63BD, 0x5D6B9D, 0x5F2BB4);
void __declspec(naked) HijackEsi()
{
	_asm
	{
		mov     [esp+48h-2Ch], eax
		mov		pJackedEsi, esi
		lea     esi, [ebp+44h]

		jmp		PassDayColoursToShader_NextIt
	}
}

void __declspec(naked) PassDayColoursToShader()
{
	_asm
	{
		mov		[esp+54h],eax
		jz		PassDayColoursToShader_FindDayColours
		jmp		PassDayColoursToShader_NextIt

PassDayColoursToShader_FindDayColours:
		xor		eax, eax

PassDayColoursToShader_FindDayColours_Loop:
		cmp     byte ptr [esp+eax*8+48h-28h+6], D3DDECLUSAGE_COLOR
		jnz		PassDayColoursToShader_FindDayColours_Next
		cmp     byte ptr [esp+eax*8+48h-28h+7], 1
		jz		PassDayColoursToShader_DoDayColours

PassDayColoursToShader_FindDayColours_Next:
		inc		eax
		jmp		PassDayColoursToShader_FindDayColours_Loop

PassDayColoursToShader_DoDayColours:
		mov		esi, pJackedEsi
		mov     edx, [ms_extraVertColourPluginOffset]
		mov		edx, dword ptr [edx]
		mov     edx, dword ptr [edx+esi+4]
		mov     edi, dword ptr [ebp+18h]
		mov     [esp+48h+4], edx
		mov     edx, dword ptr [ebp+4]
		lea     eax, [esp+eax*8+48h-26h]
		mov     [esp+48h+0Ch], edx
		mov     [esp+48h-2Ch], eax
		lea     esi, [ebp+44h]

PassDayColoursToShader_Iterate:
		mov     edx, dword ptr [esi+14h]
		mov     eax, dword ptr [esi]
		push    edi         
		push    edx            
		mov     edx, dword ptr [esp+50h+4]
		lea     edx, [edx+eax*4]
		imul    eax, edi
		push    edx            
		mov     edx, dword ptr [esp+54h-2Ch]
		movzx   edx, word ptr [edx]
		add     ecx, eax
		add     edx, ecx
		push    edx             
		call    _rpD3D9VertexDeclarationInstColor
		mov     ecx, dword ptr [esp+58h-34h]
		mov     [esi+8], eax
		mov     eax, dword ptr [esp+58h+0Ch]
		add     esp, 10h
		add     esi, 24h
		dec     eax
		mov     [esp+48h+0Ch], eax
		jnz     PassDayColoursToShader_Iterate

		jmp		PassDayColoursToShader_Return
	}
}

void __declspec(naked) PassDayColoursToShader_Steam()
{
	_asm
	{
		dec		ebx
		jz		PassDayColoursToShader_FindDayColours
		jmp		PassDayColoursToShader_NextIt

PassDayColoursToShader_FindDayColours:
		xor		eax, eax

PassDayColoursToShader_FindDayColours_Loop:
		cmp     byte ptr [esp+eax*8+48h-28h+6], D3DDECLUSAGE_COLOR
		jnz		PassDayColoursToShader_FindDayColours_Next
		cmp     byte ptr [esp+eax*8+48h-28h+7], 1
		jz		PassDayColoursToShader_DoDayColours

PassDayColoursToShader_FindDayColours_Next:
		inc		eax
		jmp		PassDayColoursToShader_FindDayColours_Loop

PassDayColoursToShader_DoDayColours:
		mov		esi, pJackedEsi
		mov     edx, [ms_extraVertColourPluginOffset]
		mov		edx, dword ptr [edx]
		mov     edx, dword ptr [edx+esi+4]
		mov     edi, dword ptr [ebp+18h]
		mov     [esp+48h+0Ch], edx
		mov     ebx, dword ptr [ebp+4]
		lea     eax, [esp+eax*8+48h-26h]
		mov     [esp+48h-2Ch], eax
		lea     esi, [ebp+44h]

PassDayColoursToShader_Iterate:
		mov     edx, dword ptr [esi+14h]
		mov     eax, dword ptr [esi]
		push    edi         
		push    edx            
		mov     edx, dword ptr [esp+50h+0Ch]
		lea     edx, [edx+eax*4]
		imul    eax, edi
		push    edx            
		mov     edx, dword ptr [esp+54h-2Ch]
		add     eax, dword ptr [esp+54h-34h]
		movzx   edx, word ptr [edx]
		add     edx, eax
		push    edx             
		call    _rpD3D9VertexDeclarationInstColor
		mov     [esi+8], eax
		add     esp, 10h
		add     esi, 24h
		dec     ebx
		jnz     PassDayColoursToShader_Iterate

		jmp		PassDayColoursToShader_Return
	}
}

void __declspec(naked) ChangeEdi_Steam()
{
	_asm
	{
		mov		edi, SIZE D3DCOLOR
		cmp     byte ptr [esp+4Ch-35h], 0
		retn
	}
}

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

// Unused on Steam EXE
static void* UsageIndex1_JumpBack = AddressByVersion<void*>(0x5D611B, 0x5D68FB, 1);
void __declspec(naked) UsageIndex1()
{
	_asm
	{
		mov		byte ptr [esp+eax*8+27h], 1
		inc		eax

		jmp		UsageIndex1_JumpBack
	}
}

void __declspec(naked) ResetAlphaFuncRefAfterRender()
{
	_asm
	{
		mov		edx, [rwengine]
		mov		edx, [edx]
		mov		ecx, [esp+7Ch-74h]
		push	ecx
		push	rwRENDERSTATEALPHATESTFUNCTIONREF
		call    dword ptr [edx+20h]
		add		esp, 8
		pop		edi
		pop		esi
		add     esp, 74h
		retn
	}
}

void __declspec(naked) ResetAlphaFuncRefAfterRender_Steam()
{
	_asm
	{
		mov		edx, [rwengine]
		mov		edx, [edx]
		mov		ecx, [esp+80h-74h]
		push	ecx
		push	rwRENDERSTATEALPHATESTFUNCTIONREF
		call    dword ptr [edx+20h]
		add		esp, 8
		pop		edi
		pop		esi
		add     esp, 78h
		retn
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

		push	13
		push	offset aStaticRotor2
		push	ebp
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jz		HunterTest_StaticRotor2AlphaSet

		push	12
		push	offset aStaticRotor
		push	ebp
		call	strncmp
		add		esp, 0Ch
		test	eax, eax
		jz		HunterTest_StaticRotorAlphaSet

		test	di, di
		jnz		HunterTest_DoorTest

		push	[RenderVehicleHiDetailCB]
		jmp		HunterTest_JumpBack

HunterTest_DoorTest:
		cmp		nCachedCRC, 0x45D0B41C
		jnz		HunterTest_RegularAlpha
		push	10
		push	offset aDoorDummy
		push	ebp
		call	strncmp
		add		esp, 0Ch
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

static void*	LoadFLAC_JumpBack = AddressByVersion<void*>(0x4F3743, *GetVer() == 1 ? (*(BYTE*)0x4F3A50 == 0x6A ? 0x4F3BA3 : 0x5B6B81) : 0, 0x4FFC3F);
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

static int _Timers_ftol_internal( double timer, double& remainder )
{
	if ( timer >= 1.0 ) return int(timer);

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

void __declspec(naked) GetMaxExtraDirectionals()
{
	_asm
	{
		call	CanSeeOutSideFromCurrArea
		test	al, al
		jz		GetMaxExtraDirectionals_Six
		
		// Low details?
		mov		eax, [g_fx]
		cmp		dword ptr [eax+54h], 0
		jne		GetMaxExtraDirectionals_Six
		mov		ebx, 4
		retn

GetMaxExtraDirectionals_Six:
		mov		ebx, 6
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
		mov		[esi+1Ch+8], ecx // OVERLAPPED.Offset
		mov		[esi+1Ch+0Ch], edx // OVERLAPPED.OffsetHigh

		mov		edx, [esi+4]
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

static const double		dRetailSubtitleSizeX = 0.58;
static const double		dRetailSubtitleSizeY = 1.2;
static const double		dRetailSubtitleSizeY2 = 1.22;
static const double		dRetailRadioNamePosY = 22.0;
static const double		dRetailRadioNameSizeX = 0.6;
static const double		dRetailRadioNameSizeY = 0.9;

BOOL InjectDelayedPatches_10()
{
	if ( !IsAlreadyRunning() )
	{
		using namespace Memory;
		ScopedUnprotect::Section Protect( (HINSTANCE)0x400000, ".text" );
		ScopedUnprotect::Section Protect2( (HINSTANCE)0x400000, ".rdata" );

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];
		GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
		PathRenameExtensionW(wcModulePath, L".ini");

		bool		bHasImVehFt = GetModuleHandle("ImVehFt.asi") != nullptr;
		bool		bSAMP = GetModuleHandle("samp") != nullptr;
		bool		bSARender = GetModuleHandle("SARender.asi") != nullptr;

		ReadRotorFixExceptions(wcModulePath);
		ReadDoubleRearWheels(wcModulePath);

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SunSizeHack", FALSE, wcModulePath) != FALSE )
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
			unsigned int	dwTwoPassMethod = GetPrivateProfileIntW(L"SilentPatch", L"TwoPassRendering", 0, wcModulePath);
			Patch<BYTE>(0x4C441E, 0x57);
			Patch<DWORD>(0x4C4424, 0x5F04C483);
			Patch<DWORD>(0x4C4428, 0x0004C25E);
			if ( dwTwoPassMethod == 1 )
			{
				// Silent's twopass
				InjectHook(0x4C441F, SetRendererForAtomic<TwoPassAlphaRender_Silent>, PATCH_CALL);
				Patch<const void*>(0x7341D9, TwoPassAlphaRender_Silent);
				Patch<const void*>(0x734127, TwoPassAlphaRender_Silent);
				Patch(0x73445E, RenderBigVehicleActomic<TwoPassAlphaRender_Silent>);
				// Twopass for peds
				InjectHook(0x733614, RenderPedCB<TwoPassAlphaRender_Silent>);
			}
			else if ( dwTwoPassMethod == 2 )
			{
				// aap's twopass
				InjectHook(0x4C441F, SetRendererForAtomic_NoTest<TwoPassAlphaRender_aap>, PATCH_CALL);
				Patch<const void*>(0x7341D9, TwoPassAlphaRender_aap);
				Patch<const void*>(0x734127, TwoPassAlphaRender_aap);
				Patch(0x73445E, RenderBigVehicleActomic<TwoPassAlphaRender_aap>);
				// Twopass for peds
				InjectHook(0x733614, RenderPedCB<TwoPassAlphaRender_aap>);
			}
			else
			{
				Patch<const void*>(0x7341D9, TwoPassAlphaRender_aap);
				Patch<const void*>(0x734127, TwoPassAlphaRender_aap);
				Patch(0x73445E, RenderBigVehicleActomic<TwoPassAlphaRender_aap>);
				InjectHook(0x4C441F, SetRendererForAtomic<OnePassAlphaRender>, PATCH_CALL);
			}


			if ( !bSAMP && GetPrivateProfileIntW(L"SilentPatch", L"NVCShader", TRUE, wcModulePath) != FALSE )
			{
				// Shaders!
				// plugin-sdk compatibility
				ReadCall( 0x5BF3A1, InitialiseRenderWare );
				ReadCall( 0x53D910, ShutdownRenderWare );
				ReadCall( 0x5D66F1, D3D9RenderPreLit );

				InjectHook(0x5DA743, SetShader);
				InjectHook(0x5D66F1, SetShader2);
				InjectHook(0x5D6116, UsageIndex1, PATCH_JUMP);
				InjectHook(0x5D63B7, PassDayColoursToShader, PATCH_JUMP);
				InjectHook(0x5D637B, HijackEsi, PATCH_JUMP);
				InjectHook(0x5BF3A1, ShaderAttach);
				InjectHook(0x53D910, ShaderDetach);
				Patch<BYTE>(0x5D7200, 0xC3);
				Patch<WORD>(0x5D67BB, 0x6890);
				Patch<WORD>(0x5D67D7, 0x6890);
				Patch<DWORD>(0x5D67BD, 0x5D5FE0);
				Patch<DWORD>(0x5D67D9, 0x5D5FE0);
				Patch<DWORD>(0x5DA73F, 0x90909056);

				Patch<BYTE>(0x5D60D9, D3DDECLTYPE_D3DCOLOR);
				Patch<BYTE>(0x5D60E2, D3DDECLUSAGE_COLOR);
				Patch<BYTE>(0x5D60CF, sizeof(D3DCOLOR));
				Patch<BYTE>(0x5D60EA, sizeof(D3DCOLOR));
				Patch<BYTE>(0x5D60C2, 0x13);
				Patch<BYTE>(0x5D62F0, 0xEB);

				// PostFX fix
				Patch<float>(*(float**)0x7034C0, 0.0);
			}

			// Weapons rendering
			InjectHook(0x5E7859, RenderWeapon);
			InjectHook(0x732F30, RenderWeaponPedsForPC, PATCH_JUMP);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableScriptFixes", TRUE, wcModulePath) != FALSE )
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

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SkipIntroSplashes", TRUE, wcModulePath) != FALSE )
		{
			// Skip the damn intro splash
			Patch<WORD>(AddressByRegion_10<DWORD>(0x748AA8), 0x3DEB);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SmallSteamTexts", TRUE, wcModulePath) != FALSE )
		{
			// We're on 1.0 - make texts smaller
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

		if ( GetPrivateProfileIntW(L"SilentPatch", L"ColouredZoneNames", FALSE, wcModulePath) != FALSE )
		{
			// Coloured zone names
			Patch<WORD>(0x58ADBE, 0x0E75);
			Patch<WORD>(0x58ADC5, 0x0775);

			InjectHook(0x58ADE4, &CRGBA::BlendGangColour);
		}
		else
		{
			Patch<BYTE>(0x58ADAE, 0xEB);
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
			InjectHook(0x4C9648, &CVehicleModelInfo::FindEditableMaterialList, PATCH_CALL);
			Patch<DWORD>(0x4C964D, 0x0FEBCE8B);
			Patch<DWORD>(0x5D5DC2, 32);		// 1.0 ONLY
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
		if ( GetModuleHandle("shadows.asi") == nullptr )
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

		// Adblocker
#if DISABLE_FLA_DONATION_WINDOW
		if (  GetModuleHandle("$fastman92limitAdjuster.asi") != nullptr )
		{
			if ( *(DWORD*)0x748736 != 0xE8186A53 )
			{
				Patch<DWORD>(0x748736, 0xE8186A53);
				InjectHook(AddressByRegion_10<int>(0x748739), 0x619B60);
			}
		}
#endif

		return FALSE;
	}
	return TRUE;
}

BOOL InjectDelayedPatches_11()
{
	if ( !IsAlreadyRunning() )
	{
		using namespace Memory;
		ScopedUnprotect::Section Protect( (HINSTANCE)0x400000, ".text" );
		ScopedUnprotect::Section Protect2( (HINSTANCE)0x400000, ".rdata" );

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];
		GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
		PathRenameExtensionW(wcModulePath, L".ini");

		bool		bHasImVehFt = GetModuleHandle("ImVehFt.asi") != nullptr;
		bool		bSAMP = GetModuleHandle("samp") != nullptr;
		bool		bSARender = GetModuleHandle("SARender.asi") != nullptr;

		ReadRotorFixExceptions(wcModulePath);

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SunSizeHack", FALSE, wcModulePath) != FALSE )
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
			unsigned int	dwTwoPassMethod = GetPrivateProfileIntW(L"SilentPatch", L"TwoPassRendering", 0, wcModulePath);
			Patch<BYTE>(0x4C449E, 0x57);
			Patch<DWORD>(0x4C44A4, 0x5F04C483);
			Patch<DWORD>(0x4C44A8, 0x0004C25E);

			if ( dwTwoPassMethod == 1 )
			{
				// Silent's twopass
				InjectHook(0x4C449F, SetRendererForAtomic<TwoPassAlphaRender_Silent>, PATCH_CALL);
				Patch<const void*>(0x734A09, TwoPassAlphaRender_Silent);
				Patch<const void*>(0x734957, TwoPassAlphaRender_Silent);
				Patch(0x734C8E, RenderBigVehicleActomic<TwoPassAlphaRender_Silent>);
				// Twopass for peds
				InjectHook(0x733E44, RenderPedCB<TwoPassAlphaRender_Silent>);
			}
			else if ( dwTwoPassMethod == 2 )
			{
				// aap's twopass
				InjectHook(0x4C449F, SetRendererForAtomic_NoTest<TwoPassAlphaRender_aap>, PATCH_CALL);
				Patch<const void*>(0x734A09, TwoPassAlphaRender_aap);
				Patch<const void*>(0x734957, TwoPassAlphaRender_aap);
				Patch(0x734C8E, RenderBigVehicleActomic<TwoPassAlphaRender_aap>);
				// Twopass for peds
				InjectHook(0x733E44, RenderPedCB<TwoPassAlphaRender_aap>);
			}
			else
			{
				InjectHook(0x4C449F, SetRendererForAtomic<OnePassAlphaRender>, PATCH_CALL);
				Patch<const void*>(0x734A09, TwoPassAlphaRender_aap);
				Patch<const void*>(0x734957, TwoPassAlphaRender_aap);
				Patch(0x734C8E, RenderBigVehicleActomic<TwoPassAlphaRender_aap>);
			}

			if ( !bSAMP && GetPrivateProfileIntW(L"SilentPatch", L"NVCShader", TRUE, wcModulePath) != FALSE )
			{
				// Shaders!
				// plugin-sdk compatibility
				// 1.01 needs to reverse Initialise3D
				ReadCall( 0x5BFB9E, InitialiseRenderWare );
				ReadCall( 0x53DDB0, ShutdownRenderWare );
				ReadCall( 0x5D6ED1, D3D9RenderPreLit );

				InjectHook(0x5BFB70, Initialise3D, PATCH_JUMP);
				InjectHook(0x5D6ED1, SetShader2);
				InjectHook(0x5D68F6, UsageIndex1, PATCH_JUMP);
				InjectHook(0x5D6B97, PassDayColoursToShader, PATCH_JUMP);
				InjectHook(0x5D6B5B, HijackEsi, PATCH_JUMP);
				//InjectHook(0x5BF3A1, ShaderAttach);
				InjectHook(0x53DDB0, ShaderDetach);
				Patch<BYTE>(0x5D79E0, 0xC3);
				Patch<WORD>(0x5D6F9B, 0x6890);
				Patch<WORD>(0x5D6FB7, 0x6890);
				Patch<DWORD>(0x5D6F9D, 0x5D67C0);
				Patch<DWORD>(0x5D6FB9, 0x5D67C0);

				Patch<BYTE>(0x5D68B9, D3DDECLTYPE_D3DCOLOR);
				Patch<BYTE>(0x5D68C2, D3DDECLUSAGE_COLOR);
				Patch<BYTE>(0x5D68AF, sizeof(D3DCOLOR));
				Patch<BYTE>(0x5D68CA, sizeof(D3DCOLOR));
				Patch<BYTE>(0x5D68A2, 0x13);
				Patch<BYTE>(0x5D6AD0, 0xEB);

				if ( *(DWORD*)0x5DAEC0 == 0x0C2444F6 )
				{
					InjectHook(0x5DAEC0 + 0xA3, SetShader);
					Patch<DWORD>(0x5DAEC0 + 0x9F, 0x90909056);
				}
				else
				{
					// securom'd EXE
					if ( *(DWORD*)0x14D0882 == 0x51104E8B )
					{
						InjectHook(0x14D088B, SetShader, PATCH_JUMP);
						Patch<DWORD>(0x14D0882, 0x90909056);
					}
				}

				// PostFX fix
				Patch<float>(*(float**)0x703CF0, 0.0);
			}

			// Weapons rendering
			InjectHook(0x5E8079, RenderWeapon);
			InjectHook(0x733760, RenderWeaponPedsForPC, PATCH_JUMP);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableScriptFixes", TRUE, wcModulePath) != FALSE )
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

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SkipIntroSplashes", TRUE, wcModulePath) != FALSE )
		{
			// Skip the damn intro splash
			Patch<WORD>(AddressByRegion_11<DWORD>(0x749388), 0x62EB);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SmallSteamTexts", TRUE, wcModulePath) != FALSE )
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

		if ( GetPrivateProfileIntW(L"SilentPatch", L"ColouredZoneNames", FALSE, wcModulePath) != FALSE )
		{
			// Coloured zone names
			Patch<WORD>(0x58B58E, 0x0E75);
			Patch<WORD>(0x58B595, 0x0775);

			InjectHook(0x58B5B4, &CRGBA::BlendGangColour);
		}
		else
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
		if ( GetModuleHandle("shadows.asi") == nullptr )
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

		return FALSE;
	}
	return TRUE;
}

BOOL InjectDelayedPatches_Steam()
{
	if ( !IsAlreadyRunning() )
	{
		using namespace Memory;
		ScopedUnprotect::Section Protect( (HINSTANCE)0x400000, ".text" );
		ScopedUnprotect::Section Protect2( (HINSTANCE)0x400000, ".rdata" );

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];
		GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
		PathRenameExtensionW(wcModulePath, L".ini");

		bool		bHasImVehFt = GetModuleHandle("ImVehFt.asi") != nullptr;
		bool		bSAMP = GetModuleHandle("samp") != nullptr;
		bool		bSARender = GetModuleHandle("SARender.asi") != nullptr;

		ReadRotorFixExceptions(wcModulePath);

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SunSizeHack", FALSE, wcModulePath) != FALSE )
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
			unsigned int	dwTwoPassMethod = GetPrivateProfileIntW(L"SilentPatch", L"TwoPassRendering", 0, wcModulePath);
			Patch<BYTE>(0x4CEBF3, 0x57);
			Patch<DWORD>(0x4CEBF9, 0xC25E5F5F);
			Patch<WORD>(0x4CEBFD, 0x0004);

			if ( dwTwoPassMethod == 1 )
			{
				// Silent's twopass
				InjectHook(0x4CEBF4, SetRendererForAtomic<TwoPassAlphaRender_Silent>, PATCH_CALL);
				Patch<const void*>(0x76E230, TwoPassAlphaRender_Silent);
				Patch<const void*>(0x76E160, TwoPassAlphaRender_Silent);
				Patch(0x76E4F0, RenderBigVehicleActomic<TwoPassAlphaRender_Silent>);
				// Twopass for peds
				InjectHook(0x76D88E, RenderPedCB<TwoPassAlphaRender_Silent>);
			}
			else if ( dwTwoPassMethod == 2 )
			{
				// aap's twopass
				InjectHook(0x4CEBF4, SetRendererForAtomic_NoTest<TwoPassAlphaRender_aap>, PATCH_CALL);
				Patch<const void*>(0x76E230, TwoPassAlphaRender_aap);
				Patch<const void*>(0x76E160, TwoPassAlphaRender_aap);
				Patch(0x76E4F0, RenderBigVehicleActomic<TwoPassAlphaRender_aap>);
				// Twopass for peds
				InjectHook(0x76D88E, RenderPedCB<TwoPassAlphaRender_aap>);
			}
			else
			{
				InjectHook(0x4CEBF4, SetRendererForAtomic<OnePassAlphaRender>, PATCH_CALL);
				Patch<const void*>(0x76E230, TwoPassAlphaRender_aap);
				Patch<const void*>(0x76E160, TwoPassAlphaRender_aap);
				Patch(0x76E4F0, RenderBigVehicleActomic<TwoPassAlphaRender_aap>);
			}

			if ( !bSAMP && GetPrivateProfileIntW(L"SilentPatch", L"NVCShader", TRUE, wcModulePath) != FALSE )
			{
				// Shaders!
				// plugin-sdk compatibility
				ReadCall( 0x5DE5A1, InitialiseRenderWare );
				ReadCall( 0x550070, ShutdownRenderWare );
				ReadCall( 0x5F663E, D3D9RenderPreLit );

				InjectHook(0x5F6EB3, SetShader);
				InjectHook(0x5F2F02, SetShader2);
				//InjectHook(0x5F292C, UsageIndex1, PATCH_JUMP);
				InjectHook(0x5F2BAF, PassDayColoursToShader_Steam, PATCH_JUMP);
				InjectHook(0x5F2B7A, HijackEsi, PATCH_JUMP);
				InjectHook(0x5DE5A1, ShaderAttach);
				InjectHook(0x550070, ShaderDetach);
				Patch<BYTE>(0x5F3760, 0xC3);
				Patch<WORD>(0x5F2FCB, 0x6890);
				Patch<WORD>(0x5F2FE7, 0x6890);
				Patch<DWORD>(0x5F2FCD, 0x5F27C0);
				Patch<DWORD>(0x5F2FE9, 0x5F27C0);
				Patch<DWORD>(0x5F6EAF, 0x90909056);

				Patch<BYTE>(0x5F28D0, 1);
				Patch<BYTE>(0x5F28C1, D3DDECLTYPE_D3DCOLOR);
				Patch<BYTE>(0x5F28CB, D3DDECLUSAGE_COLOR);
				//Patch<BYTE>(0x5D60CF, sizeof(D3DCOLOR));
				//Patch<BYTE>(0x5D60EA, sizeof(D3DCOLOR));
				InjectHook(0x5F28A7, ChangeEdi_Steam, PATCH_CALL);
				//Patch<BYTE>(0x5D60C2, 0x13);
				Patch<BYTE>(0x5F2AE7, 0xEB);

				// PostFX fix
				Patch<float>(*(float**)0x746E57, 0.0);
			}

			// Weapons rendering
			InjectHook(0x604DD9, RenderWeapon);
			InjectHook(0x76D170, RenderWeaponPedsForPC, PATCH_JUMP);
		}

		if ( GetPrivateProfileIntW(L"SilentPatch", L"EnableScriptFixes", TRUE, wcModulePath) != FALSE )
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

		if ( GetPrivateProfileIntW(L"SilentPatch", L"SmallSteamTexts", TRUE, wcModulePath) == FALSE )
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

		if ( GetPrivateProfileIntW(L"SilentPatch", L"ColouredZoneNames", FALSE, wcModulePath) != FALSE )
		{
			// Coloured zone names
			Patch<WORD>(0x598F65, 0x0C75);
			Patch<WORD>(0x598F6B, 0x0675);

			InjectHook(0x598F87, &CRGBA::BlendGangColour);
		}
		else
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
		if ( GetModuleHandle("shadows.asi") == nullptr )
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

		return FALSE;
	}
	return TRUE;
}

static char		aNoDesktopMode[64];


void Patch_SA_10()
{
	using namespace Memory;

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	int			pIsAlreadyRunning = AddressByRegion_10<int>(0x74872D);
	ReadCall( pIsAlreadyRunning, IsAlreadyRunning );
	InjectHook(pIsAlreadyRunning, InjectDelayedPatches_10);

	// Newsteam crash fix
	pDirect = *(RpLight***)0x5BA573;
	DarkVehiclesFix1_JumpBack = AddressByRegion_10<void*>(0x756D90);

	// (Hopefully) more precise frame limiter
	int			pAddress = AddressByRegion_10<int>(0x748D9B);
	ReadCall( pAddress, RsEventHandler );
	InjectHook(pAddress, NewFrameRender);
	InjectHook(AddressByRegion_10<int>(0x748D1F), GetTimeSinceLastFrame);

	// Set CAEDataStream to use an old structure
	CAEDataStream::SetStructType(false);

	//Patch<BYTE>(0x5D7265, 0xEB);

	// Heli rotors
	InjectHook(0x6CAB70, &CPlane::Render_Stub, PATCH_JUMP);
	InjectHook(0x6C4400, &CHeli::Render_Stub, PATCH_JUMP);
	//InjectHook(0x553318, RenderAlphaAtomics);
	//Patch<const void*>(0x73406E, TwoPassAlphaRender);

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

	// Moonphases
	InjectHook(0x713ACB, HandleMoonStuffStub, PATCH_JUMP);

	// TEMP
	//Patch<DWORD>(0x733B05, 40);
	//Patch<DWORD>(0x733B55, 40);
	//Patch<BYTE>(0x5B3ADD, 4);

	// Lightbeam fix
	Patch<WORD>(0x6A2E88, 0x0EEB);
	Nop(0x6A2E9C, 3);
	Patch<WORD>(0x6E0F63, 0x0AEB);
	Patch<WORD>(0x6E0F7C, 0x0BEB);
	Patch<WORD>(0x6E0F95, 0x0BEB);
	Patch<WORD>(0x6E0FAF, 0x1AEB);

	Patch<WORD>(0x6E13D5, 0x09EB);
	Patch<WORD>(0x6E13ED, 0x17EB);
	Patch<WORD>(0x6E141F, 0x0AEB);

	Patch<BYTE>(0x6E0FE0, 0x28);
	Patch<BYTE>(0x6E142D, 0x18);
	Patch<BYTE>(0x6E0FDB, 0xC8-0x7C);
	//InjectHook(0x6A2EDA, CullTest);

	InjectHook(0x6A2EF7, ResetAlphaFuncRefAfterRender, PATCH_JUMP);

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
	InjectHook(0x4D9B66, UserTracksFix);
	InjectHook(0x4D9BB5, 0x4F2FD0);
	//Nop(0x4D9BB5, 5);

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
	InjectHook(0x425179, 0x448990);
	InjectHook(0x425369, 0x448990);
	InjectHook(0x425411, 0x448990);

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

	// CGarages::RespraysAreFree resetting on new game
	Patch<WORD>(0x448BD8, 0x8966);
	Patch<BYTE>(0x448BDA, 0x0D);
	Patch<bool*>(0x448BDB, *(bool**)0x44AC98);
	Patch<BYTE>(0x448BDF, 0xC3);

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
	InjectHook(0x735881, GetMaxExtraDirectionals, PATCH_CALL);
	Patch<WORD>(0x735886, 0x07EB);

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
	InjectHook(0x744FB0, GetMyDocumentsPath, PATCH_JUMP);

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
		int			pHoodlumCompat;
		if ( *(BYTE*)0x40F870 == 0xE9 )
			pHoodlumCompat = 0x40F875 +  *(int*)0x40F871;
		else
			pHoodlumCompat = 0x40F870;

		int			pMemMgrMalloc = pHoodlumCompat + 0x63;
		ReadCall( pMemMgrMalloc, orgMemMgrMalloc );
		VP::InjectHook(pMemMgrMalloc, CollisionData_MallocAndInit);
	}
	{
		int			pHoodlumCompat, pHoodlumCompat2;
		if ( *(BYTE*)0x40F740 == 0xE9 )
		{
			pHoodlumCompat = 0x40F745 +  *(int*)0x40F741;
			pHoodlumCompat2 = 0x40F815 + *(int*)0x40F811;
		}
		else
		{
			pHoodlumCompat = 0x40F740;
			pHoodlumCompat2 = 0x40F810;
		}

		int			pNewAlloc = pHoodlumCompat + 0xC;
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
	strcpy(pScannerNames + (8*113), "WESTP");
	strcpy(pScannerNames + (8*134), "????");

	// Handle IMGs bigger than 4GB
	Nop( 0x4065BB, 3 );
	Nop( 0x4065C2, 1 );
	InjectHook( 0x4065C2+1, CdStreamThreadHighSize, PATCH_CALL );
	Patch<const void*>( 0x406620+2, &pCdStreamSetFilePointer );


	// AI accuracy issue
	Nop(0x73B3AE, 1);
	InjectHook( 0x73B3AE + 1, WeaponRangeMult_VehicleCheck, PATCH_CALL );


	// New timers fix
	InjectHook( 0x561C32, asmTimers_ftol_PauseMode );
	InjectHook( 0x561902, asmTimers_ftol_NonClipped );
	InjectHook( 0x56191A, asmTimers_ftol );


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


	 // Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x53BD5B, orgCarCtrlReInit );
	InjectHook(0x53BD5B, CarCtrlReInit_SilentPatch);


	// FuckCarCompletely not fixing panels
	Nop(0x6C268D, 3);


	// 014C cargen counter fix (by spaceeinstein)
	Patch<uint8_t>( 0x06F3E2C + 1, 0xBF ); // movzx ecx, ax -> movsx ecx, ax
	Patch<uint8_t>( 0x6F3E32, 0x74 ); // jge -> jz


	// Linear filtering on script sprites
	ReadCall( 0x58C092, orgDrawScriptSpritesAndRectangles );
	InjectHook( 0x58C092, DrawScriptSpritesAndRectangles );


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
	Patch<WORD>(0x6A36A8, 0x0EEB);
	Nop(0x6A36BC, 3);
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
	InjectHook(0x7360B1, GetMaxExtraDirectionals, PATCH_CALL);
	Patch<WORD>(0x7360B6, 0x07EB);

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
	InjectHook(0x7457E0, GetMyDocumentsPath, PATCH_JUMP);

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
	strcpy(pScannerNames + (8*113), "WESTP");
	strcpy(pScannerNames + (8*134), "????");


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
	InjectHook(0x768046, GetMaxExtraDirectionals, PATCH_CALL);
	Patch<WORD>(0x76804B, 0x0AEB);

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
	InjectHook(0x77EDC0, GetMyDocumentsPath, PATCH_JUMP);

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


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x54DCCB, orgCarCtrlReInit );
	InjectHook(0x54DCCB, CarCtrlReInit_SilentPatch);


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
	strcpy(pScannerNames + (8*113), "WESTP");
	strcpy(pScannerNames + (8*134), "????");

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
	InjectHook(0x778FA0, GetMyDocumentsPath, PATCH_JUMP);

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
	InjectHook(0x778E90, GetMyDocumentsPath, PATCH_JUMP);

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

		for ( size_t i = 0; i < 2; ++i )
		{
			auto match = patternie.get(i);
			InjectHook( match.get<int>(0x39), defproc, PATCH_JUMP );
			Patch<uint8_t>( match.get<int>(1), 0x5D ); // esi -> ebx
			Patch<uint8_t>( match.get<int>(6), 0x53 ); // esi -> ebx
			Patch<uint8_t>( match.get<int>(0x26 + 1), 0xFB ); // esi -> ebx
			Patch<int8_t>( match.get<int>(8 + 2), -8 ); // use stack space for new lParam
			Patch<int8_t>( match.get<int>(0x18 + 2), -8 ); // use stack space for new lParam
			Patch<int8_t>( match.get<int>(0x2B + 2), -8 ); // use stack space for new lParam
		}
	}


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	{
		void* reinit_addr = get_pattern( "53 E8 ? ? ? ? E8 ? ? ? ? D9 05 ? ? ? ? D9 1C 24", -15 );
		auto timers_init = pattern( "89 45 FC DB 45 FC C6 05 ? ? ? ? 01" ).get_one();

		LastTimeAmbulanceCreated_Newsteam = *timers_init.get<signed int*>(-17 + 2);
		LastTimeFireTruckCreated_Newsteam = *timers_init.get<signed int*>(-11 + 2);
		TimeNextMadDriverChaseCreated_Newsteam = *timers_init.get<float*>(0x41 + 2);
		ReadCall( reinit_addr, orgCarCtrlReInit );
		InjectHook(reinit_addr, CarCtrlReInit_SilentPatch_Newsteam);
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
		auto drawScriptSprites = pattern( "81 EC 94 01 00 00 53 56 57 50" ).get_one();
		ReadCall( drawScriptSprites.get<int>(10), orgDrawScriptSpritesAndRectangles );
		InjectHook( drawScriptSprites.get<int>(10), DrawScriptSpritesAndRectangles );
	}
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		hDLLModule = hinstDLL;

		HINSTANCE hGameHandle = GetModuleHandle( nullptr );
		ScopedUnprotect::Section Protect( hGameHandle, ".text" );
		ScopedUnprotect::Section Protect2( hGameHandle, ".rdata" );

		if (*(DWORD*)DynBaseAddress(0x82457C) == 0x94BF || *(DWORD*)DynBaseAddress(0x8245BC) == 0x94BF) Patch_SA_10();
		else if (*(DWORD*)DynBaseAddress(0x8252FC) == 0x94BF || *(DWORD*)DynBaseAddress(0x82533C) == 0x94BF) Patch_SA_11(), MessageBox( nullptr, "You're using a 1.01 executable which is no longer supported by SilentPatch!\n\nI have no idea if anyone was still using it, so if you do - shoot me an e-mail!", "SilentPatch", MB_OK | MB_ICONWARNING );
		else if (*(DWORD*)DynBaseAddress(0x85EC4A) == 0x94BF) Patch_SA_Steam();
		else
		{
			if ( *(DWORD*)DynBaseAddress(0x858D21) == 0x3539F633) Patch_SA_NewSteam_r1();
			else if ( *(DWORD*)DynBaseAddress(0x858D51) == 0x3539F633) Patch_SA_NewSteam_r2();
			else if ( *(DWORD*)DynBaseAddress(0x858C61) == 0x3539F633) Patch_SA_NewSteam_r2_lv();
			Patch_SA_NewSteam_Common();
		}	
	}
	return TRUE;
}