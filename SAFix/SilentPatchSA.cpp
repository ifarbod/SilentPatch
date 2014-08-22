#include "StdAfxSA.h"

#include "ScriptSA.h"
#include "GeneralSA.h"
#include "ModelInfoSA.h"
#include "VehicleSA.h"
#include "PedSA.h"
#include "AudioHardwareSA.h"
#include "LinkListSA.h"
#include "PNGFile.h"

// RW wrappers
static void* varAtomicDefaultRenderCallBack = AddressByVersion<void*>(0x7491C0, 0x749AD0, 0x783180);
WRAPPER RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic) { WRAPARG(atomic); VARJMP(varAtomicDefaultRenderCallBack); }
static void* varRtPNGImageRead = AddressByVersion<void*>(0x7CF9B0, 0x7D02B0, 0x809970);
WRAPPER RwImage* RtPNGImageRead(const RwChar* imageName) { WRAPARG(imageName); VARJMP(varRtPNGImageRead); }
static void* varRwTextureCreate = AddressByVersion<void*>(0x7F37C0, 0x7F40C0, 0x82D780);
WRAPPER RwTexture* RwTextureCreate(RwRaster* raster) { WRAPARG(raster); VARJMP(varRwTextureCreate); }
static void* varRwRasterCreate = AddressByVersion<void*>(0x7FB230, 0x7FBB30, 0x8351F0);
WRAPPER RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) { WRAPARG(width); WRAPARG(height); WRAPARG(depth); WRAPARG(flags); VARJMP(varRwRasterCreate); }
static void* varRwImageDestroy = AddressByVersion<void*>(0x802740, 0x803040, 0x83C700);
WRAPPER RwBool RwImageDestroy(RwImage* image) { WRAPARG(image); VARJMP(varRwImageDestroy); }
static void* varRpMaterialSetTexture = AddressByVersion<void*>(0x74DBC0, 0x74E4D0, 0x787B80);
WRAPPER RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture) { VARJMP(varRpMaterialSetTexture); }
static void* varRwFrameGetLTM = AddressByVersion<void*>(0x7F0990, 0x7F1290, 0x82A950);
WRAPPER RwMatrix* RwFrameGetLTM(RwFrame* frame) { VARJMP(varRwFrameGetLTM); }
static void* varRwMatrixTranslate = AddressByVersion<void*>(0x7F2450, 0x7F2D50, 0x82C410);
WRAPPER RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, RwOpCombineType combineOp) { WRAPARG(matrix); WRAPARG(translation); WRAPARG(combineOp); VARJMP(varRwMatrixTranslate); }
static void* varRwMatrixRotate = AddressByVersion<void*>(0x7F1FD0, 0x7F28D0, 0x82BF90);
WRAPPER RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp) { WRAPARG(matrix); WRAPARG(axis); WRAPARG(angle); WRAPARG(combineOp); VARJMP(varRwMatrixRotate); }
static void* varRwD3D9SetRenderState = AddressByVersion<void*>(0x7FC2D0, 0x7FCBD0, 0x836290);
WRAPPER void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) { WRAPARG(state); WRAPARG(value); VARJMP(varRwD3D9SetRenderState); }
static void* var_rwD3D9SetVertexShader = AddressByVersion<void*>(0x7F9FB0, 0x7FA8B0, 0x833F70);
WRAPPER void _rwD3D9SetVertexShader(void *shader) { VARJMP(var_rwD3D9SetVertexShader); }
static void* varRwD3D9CreateVertexShader = AddressByVersion<void*>(0x7FAC60, 0x7FB560, 0x834C20);
WRAPPER RwBool RwD3D9CreateVertexShader(const RwUInt32 *function, void **shader) { VARJMP(varRwD3D9CreateVertexShader); }
static void* varRwD3D9DeleteVertexShader = AddressByVersion<void*>(0x7FAC90, 0x7FB590, 0x834C50);
WRAPPER void RwD3D9DeleteVertexShader(void *shader) { VARJMP(varRwD3D9DeleteVertexShader); }
static void* var_rwD3D9VSGetComposedTransformMatrix = AddressByVersion<void*>(0x7646E0, 0x764FF0, 0x79E6A0);
WRAPPER void _rwD3D9VSGetComposedTransformMatrix(void *transformMatrix) { VARJMP(var_rwD3D9VSGetComposedTransformMatrix); }
static void* var_rwD3D9VSSetActiveWorldMatrix = AddressByVersion<void*>(0x764650, 0x764F60, 0x79E610);
WRAPPER void _rwD3D9VSSetActiveWorldMatrix(const RwMatrix *worldMatrix) { VARJMP(var_rwD3D9VSSetActiveWorldMatrix); }
static void* var_rwD3D9SetVertexShaderConstant = AddressByVersion<void*>(0x7FACA0, 0x7FB5A0, 0x834C60);
WRAPPER void _rwD3D9SetVertexShaderConstant(RwUInt32 registerAddress,
                               const void *constantData,
							   RwUInt32  constantCount) { VARJMP(var_rwD3D9SetVertexShaderConstant); }
static void* var_rpD3D9VertexDeclarationInstColor = AddressByVersion<void*>(0x754AE0, 0x7553F0, 0x78EAA0);
WRAPPER RwBool _rpD3D9VertexDeclarationInstColor(RwUInt8 *mem,
                                  const RwRGBA *color,
                                  RwInt32 numVerts,
								  RwUInt32 stride) { VARJMP(var_rpD3D9VertexDeclarationInstColor); }


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

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callBack, void* data)
{
	for ( RwFrame* curFrame = frame->child; curFrame; curFrame = curFrame->next )
	{
		if ( !callBack(curFrame, data) )
			break;
	}
	return frame;
}

RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callBack, void* data)
{
	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&frame->objectList); link != rwLinkListGetTerminator(&frame->objectList); link = rwLLLinkGetNext(link) )
	{
		if ( !callBack(&rwLLLinkGetData(link, RwObjectHasFrame, lFrame)->object, data) )
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
		if ( !callback(rwLLLinkGetData(link, RpAtomic, inClumpLink), pData) )
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
			if ( !RpAtomicRender(curAtomic) )
				retClump = NULL;
		}
	}
	return retClump;
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData)
{
	for ( RwInt32 i = 0, j = geometry->matList.numMaterials; i < j; i++ )
	{
		if ( !fpCallBack(geometry->matList.materials[i], pData) )
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
static void				(*sub_5DA6A0)(void*, void*, void*, void*);


// SA variables
void**					rwengine = *AddressByVersion<void***>(0x58FFC0, 0x53F032, 0x48C194);
signed int&				ms_extraVertColourPluginOffset = **AddressByVersion<int**>(0x5D6362, 0x5D6B42, 0x5F2B65);

unsigned char&			nGameClockDays = **AddressByVersion<unsigned char**>(0x4E841D, 0x4E886D, 0x4F3871);
unsigned char&			nGameClockMonths = **AddressByVersion<unsigned char**>(0x4E842D, 0x4E887D, 0x4F3861);
void*&					pUserTracksStuff = **AddressByVersion<void***>(0x4D9B7B, 0x4DA06C, 0x4E4A43);
bool&					CCutsceneMgr__ms_running = **AddressByVersion<bool**>(0x53F92D, 0x434241, 0x422061);
unsigned char*			ScriptSpace = *AddressByVersion<unsigned char**>(0x5D5380, 0x5D5B60, 0x450E34);
int*					ScriptParams = *AddressByVersion<int**>(0x48995B, 0x46410A, 0x46979A);

float&					fFarClipZ = **AddressByVersion<float**>(0x70D21F, 0x70DA4F, 0x421AB2);
RwTexture** const		gpCoronaTexture = *AddressByVersion<RwTexture***>(0x6FAA8C, 0x6FB2BC, 0x5480BF);
int&					MoonSize = **AddressByVersion<int**>(0x713B0C, 0x71433C, 0x72F0AB);

CZoneInfo*&				pCurrZoneInfo = **AddressByVersion<CZoneInfo***>(0x58ADB1, 0x58B581, 0x407F93);
CRGBA*					HudColour = *AddressByVersion<CRGBA**>(0x58ADF6, 0x58B5C6, 0x440648);
unsigned char*			ZonesVisited = *AddressByVersion<unsigned char**>(0x57216A, 0, 0x5870E8) - 9;		// 1.01 fixed it!

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

// lunar.png
static const BYTE	gMoonMaskPNG[] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40,
	0x08, 0x06, 0x00, 0x00, 0x00, 0xAA, 0x69, 0x71, 0xDE, 0x00, 0x00, 0x03,
	0xC1, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0xED, 0x9B, 0xD7, 0x6B, 0x54,
	0x51, 0x10, 0xC6, 0x27, 0x96, 0x68, 0xEC, 0xA2, 0x41, 0x09, 0x16, 0xAC,
	0x60, 0x50, 0x51, 0xAC, 0xB1, 0xF7, 0x8A, 0x3E, 0xC4, 0x06, 0xBE, 0x08,
	0x3E, 0xE8, 0x83, 0x82, 0xE8, 0x1F, 0xA2, 0x08, 0xFA, 0xA0, 0x0F, 0x82,
	0x2F, 0x82, 0xF5, 0xC1, 0x10, 0x7B, 0x8F, 0x5D, 0x83, 0xA2, 0xA2, 0x60,
	0xC5, 0x42, 0x50, 0x54, 0xEC, 0xC6, 0x12, 0xCB, 0xF7, 0x31, 0xB3, 0x70,
	0x73, 0xD9, 0x9B, 0xCD, 0x66, 0xDB, 0xBD, 0x39, 0xFB, 0xC1, 0x8F, 0x90,
	0x6C, 0x58, 0x76, 0xBE, 0x33, 0x67, 0x66, 0xCE, 0xDD, 0x7B, 0x0B, 0xC4,
	0x71, 0x15, 0xE4, 0xFA, 0x03, 0xE4, 0x5A, 0xD9, 0x30, 0xA0, 0x15, 0xE8,
	0x06, 0x3A, 0x83, 0x8E, 0xA0, 0x1D, 0x68, 0x0B, 0x0A, 0x41, 0x0B, 0xFB,
	0x9F, 0xBF, 0xE0, 0x17, 0xF8, 0x01, 0xBE, 0x83, 0x2F, 0xE0, 0x13, 0x78,
	0x0F, 0xEA, 0xA2, 0x68, 0x40, 0x11, 0x28, 0x01, 0x3D, 0x41, 0x77, 0xD0,
	0x05, 0x74, 0x00, 0xED, 0xED, 0xB5, 0x42, 0x33, 0xA6, 0xA5, 0xFD, 0xFF,
	0x1F, 0x0B, 0x94, 0x26, 0xD4, 0x82, 0x6F, 0xE0, 0x2B, 0xF8, 0x08, 0xDE,
	0x81, 0xD7, 0xA0, 0xC6, 0x5E, 0x0B, 0xB5, 0x01, 0x3D, 0x40, 0x5F, 0xD0,
	0x0B, 0x14, 0x8B, 0xAE, 0x7C, 0x57, 0xD0, 0xDB, 0xCC, 0x28, 0xF6, 0x98,
	0x51, 0x64, 0x26, 0x88, 0x05, 0x5F, 0xEB, 0x09, 0xFA, 0xAD, 0x05, 0xFD,
	0x12, 0x7C, 0x10, 0xCD, 0x04, 0xFE, 0xED, 0x15, 0x78, 0x0E, 0xDE, 0x84,
	0xCD, 0x00, 0x06, 0x3E, 0xD0, 0x82, 0x2F, 0xB1, 0xDF, 0x87, 0x80, 0x41,
	0xA0, 0x1F, 0x68, 0xDD, 0xC4, 0xF7, 0xFD, 0x0D, 0x9E, 0x81, 0x47, 0xE0,
	0x81, 0x05, 0x5E, 0x63, 0x26, 0x3C, 0x4E, 0x87, 0x11, 0xA9, 0x1A, 0xC0,
	0x55, 0x2C, 0xB5, 0x40, 0xFB, 0x58, 0xB0, 0x23, 0xC1, 0x70, 0x7B, 0x2D,
	0x9D, 0x62, 0x86, 0xDC, 0x01, 0xB7, 0xCC, 0x94, 0x17, 0x66, 0xCC, 0x7D,
	0x49, 0x61, 0x6B, 0xA4, 0x62, 0x00, 0x03, 0x1E, 0x06, 0xFA, 0x1B, 0x65,
	0x60, 0x6C, 0x8A, 0xEF, 0xD9, 0x18, 0xFD, 0x03, 0xD7, 0xC1, 0x15, 0xF0,
	0xD4, 0xB8, 0x6B, 0x86, 0x64, 0xCD, 0x00, 0xAE, 0xF0, 0x50, 0xD1, 0x95,
	0x1F, 0x03, 0x26, 0x83, 0x4E, 0x19, 0x0E, 0xDC, 0xAF, 0xCF, 0xA0, 0x0A,
	0xDC, 0x10, 0xCD, 0x84, 0x7B, 0xA2, 0x19, 0x92, 0x51, 0x03, 0x58, 0xB4,
	0x46, 0x8B, 0xAE, 0xFC, 0x60, 0x30, 0xC7, 0xCC, 0xC8, 0xA5, 0x18, 0xF4,
	0x09, 0xF0, 0x50, 0x34, 0x13, 0x6E, 0x4A, 0x12, 0xAD, 0x33, 0x19, 0x03,
	0xDA, 0x88, 0xA6, 0x38, 0x03, 0x1E, 0x01, 0x16, 0x88, 0x16, 0xBC, 0x30,
	0x88, 0x85, 0xF1, 0x08, 0xB8, 0x6D, 0x86, 0x70, 0x8B, 0xFC, 0x4C, 0xA7,
	0x01, 0x5C, 0xF9, 0x32, 0x0B, 0x9C, 0x26, 0x2C, 0x96, 0xEC, 0xA7, 0x7C,
	0x22, 0x71, 0x4B, 0x1C, 0xB6, 0xE0, 0x69, 0x04, 0x6B, 0x44, 0xC2, 0x4C,
	0x68, 0xAC, 0x01, 0xE3, 0xC1, 0x28, 0xFB, 0xB9, 0x44, 0xD2, 0x5F, 0xE1,
	0xD3, 0x25, 0x76, 0x83, 0x83, 0xE0, 0x2A, 0xA8, 0xB6, 0x9F, 0x29, 0x1B,
	0xC0, 0x94, 0x1F, 0x27, 0xBA, 0xF2, 0xCB, 0x25, 0x7C, 0x2B, 0xEF, 0x17,
	0x33, 0x61, 0x9F, 0x68, 0x26, 0x5C, 0x93, 0x04, 0x85, 0x31, 0x91, 0x01,
	0x6C, 0x75, 0x93, 0x44, 0x7B, 0xFB, 0x4A, 0x09, 0xCF, 0x9E, 0x4F, 0x24,
	0xD6, 0x84, 0x3D, 0xA2, 0x33, 0xC3, 0x45, 0x69, 0xA0, 0x45, 0x36, 0x64,
	0x00, 0xD3, 0x7C, 0x86, 0x68, 0xD5, 0x2F, 0x97, 0xDC, 0x57, 0xFB, 0x64,
	0xC5, 0x95, 0x3F, 0x24, 0xDA, 0x15, 0xCE, 0x48, 0xC0, 0xB0, 0xD4, 0x90,
	0x01, 0xDC, 0xF3, 0x13, 0xC0, 0x5C, 0xB0, 0x30, 0xD7, 0xD1, 0x34, 0x51,
	0x95, 0xE0, 0x38, 0xB8, 0x2C, 0x5A, 0x13, 0x1A, 0x6D, 0x00, 0x67, 0xF9,
	0xE9, 0xA2, 0x7B, 0x7F, 0xB5, 0x84, 0x7F, 0xDF, 0x07, 0x89, 0xF5, 0x60,
	0x97, 0x68, 0x2D, 0x38, 0x2B, 0x71, 0xCE, 0x0E, 0x41, 0x06, 0x4C, 0x34,
	0xCA, 0xCD, 0x84, 0x28, 0x8B, 0xC1, 0x73, 0x2B, 0x5C, 0x32, 0x12, 0x1A,
	0xC0, 0xD5, 0x9F, 0x29, 0x3A, 0xDE, 0xAE, 0x95, 0xE8, 0x5F, 0x35, 0xE2,
	0xD9, 0x61, 0x87, 0xE8, 0xD8, 0x7C, 0x5A, 0x7C, 0x59, 0x10, 0x2F, 0x38,
	0xB6, 0xBB, 0x29, 0x60, 0xA9, 0x44, 0x7F, 0xF5, 0x63, 0x62, 0x16, 0x1C,
	0x00, 0x17, 0x44, 0xDB, 0x63, 0xA0, 0x01, 0xAC, 0xFC, 0xF3, 0x45, 0xA7,
	0xBE, 0x75, 0x12, 0xDE, 0x81, 0x27, 0x59, 0xB1, 0x03, 0x6C, 0x17, 0x9D,
	0x0E, 0x8F, 0x8A, 0xA7, 0x23, 0xF8, 0x0D, 0x18, 0x00, 0x66, 0x81, 0x45,
	0x12, 0xDD, 0xCA, 0x1F, 0x24, 0x76, 0x84, 0x0A, 0x70, 0x0A, 0x3C, 0x09,
	0x32, 0x80, 0x85, 0x6F, 0x2A, 0x58, 0x25, 0x7A, 0xDA, 0x6B, 0x4E, 0xE2,
	0x69, 0x71, 0x37, 0x38, 0x2F, 0x9E, 0x62, 0xE8, 0x35, 0x80, 0x07, 0x1E,
	0xAE, 0x3A, 0x27, 0xBF, 0x0D, 0xD2, 0xF4, 0xCB, 0x58, 0x61, 0x15, 0x2F,
	0xAF, 0x6D, 0x15, 0x9D, 0x0C, 0x99, 0x0D, 0x75, 0x7E, 0x03, 0x58, 0xFD,
	0xE7, 0x89, 0xD6, 0x80, 0x15, 0xB9, 0xFE, 0xB4, 0x19, 0xD2, 0x5E, 0xD1,
	0x1A, 0x70, 0x4C, 0xAC, 0x1B, 0x78, 0x0D, 0x60, 0xCA, 0xCF, 0x06, 0xCB,
	0x44, 0xB7, 0x41, 0x73, 0x14, 0xD3, 0x7F, 0x3F, 0x38, 0x29, 0xBA, 0x25,
	0xEA, 0x19, 0xC0, 0xD1, 0x97, 0xFD, 0x9F, 0xFB, 0xBF, 0x34, 0xD7, 0x9F,
	0x34, 0x43, 0xE2, 0x05, 0x54, 0xD6, 0x01, 0xCE, 0x03, 0xD5, 0x7E, 0x03,
	0x38, 0xF8, 0xF0, 0xF0, 0xB3, 0x46, 0xA2, 0x73, 0xEA, 0x4B, 0x56, 0x3C,
	0x25, 0xEE, 0x14, 0x3D, 0x1C, 0x55, 0xF9, 0x0D, 0x60, 0xFA, 0x4F, 0x03,
	0xEB, 0x25, 0xBA, 0xB3, 0x7F, 0x22, 0xF1, 0x6C, 0xB0, 0x0D, 0x9C, 0x13,
	0xDD, 0x06, 0xF5, 0x0C, 0x60, 0x07, 0xE0, 0x04, 0xB8, 0x51, 0x9A, 0x5F,
	0x07, 0x88, 0x89, 0x9D, 0x60, 0x8B, 0xE8, 0x44, 0x58, 0xE9, 0x37, 0x80,
	0xC3, 0x0F, 0x8B, 0xDF, 0x26, 0x89, 0xFE, 0xFC, 0x1F, 0x24, 0x9E, 0x0B,
	0x36, 0x8B, 0x16, 0xC3, 0x8A, 0xBC, 0x01, 0x92, 0xDF, 0x02, 0xF9, 0x22,
	0x98, 0x6F, 0x83, 0x9E, 0x17, 0x9D, 0x1F, 0x84, 0x9C, 0x1F, 0x85, 0x9D,
	0x3F, 0x0C, 0x39, 0x7F, 0x1C, 0xA6, 0x9C, 0xBE, 0x20, 0x42, 0x39, 0x7F,
	0x49, 0xCC, 0xF9, 0x8B, 0xA2, 0x94, 0xD3, 0x97, 0xC5, 0x29, 0xE7, 0xBF,
	0x18, 0xA1, 0x9C, 0xFE, 0x6A, 0x8C, 0x72, 0xFE, 0xCB, 0x51, 0xCA, 0xE9,
	0xAF, 0xC7, 0x29, 0xE7, 0x6F, 0x90, 0xA0, 0x9C, 0xBE, 0x45, 0x26, 0x26,
	0xA7, 0x6F, 0x92, 0x8A, 0xC9, 0xE9, 0xDB, 0xE4, 0x28, 0xE7, 0x6F, 0x94,
	0xA4, 0x9C, 0xBE, 0x55, 0x36, 0x26, 0xA7, 0x6F, 0x96, 0xF6, 0xCA, 0xD9,
	0xDB, 0xE5, 0xBD, 0x72, 0xFA, 0x81, 0x89, 0x98, 0x9C, 0x7E, 0x64, 0xC6,
	0x2B, 0x67, 0x1F, 0x9A, 0x8A, 0x67, 0x84, 0x93, 0x8F, 0xCD, 0xF9, 0xE5,
	0xEC, 0x83, 0x93, 0xF1, 0xE4, 0xE4, 0xA3, 0xB3, 0x91, 0x91, 0xF3, 0x06,
	0xFC, 0x07, 0x2A, 0x0D, 0x50, 0x50, 0xCC, 0xB6, 0x51, 0x67, 0x00, 0x00,
	0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};


// Regular functions
static RpMaterial* AlphaTest(RpMaterial* pMaterial, void* pData)
{
	if ( RpMaterialGetTexture(pMaterial) )
	{
		if ( __rwD3D9TextureHasAlpha(RpMaterialGetTexture(pMaterial)) )
		{
			*static_cast<BOOL*>(pData) = TRUE;
			return nullptr;
		}
	}
	else if ( RpMaterialGetColor(pMaterial)->alpha < 255 )
	{
		*static_cast<BOOL*>(pData) = TRUE;
		return nullptr;
	}

	return pMaterial;
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

	if ( nAlphaBlending != TRUE )
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	auto* pAtomic = AtomicDefaultRenderCallBack(atomic);

	if ( nAlphaBlending != TRUE )
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));

	return pAtomic;
}

RpAtomic* TwoPassAlphaRender(RpAtomic* atomic)
{
	// For cutscenes, fall back to one-pass render
	if ( CCutsceneMgr__ms_running && !CanSeeOutSideFromCurrArea() )
		return OnePassAlphaRender(atomic);

	int		nPushedAlpha, nAlphaFunction;
	int		nZWrite;
	int		nAlphaBlending;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &nZWrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &nAlphaBlending);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &nAlphaFunction);

	// 1st pass
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, FALSE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(255));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONEQUAL));

	auto* pAtomic = AtomicDefaultRenderCallBack(atomic);

	// 2nd pass
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(rwALPHATESTFUNCTIONLESS));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

	AtomicDefaultRenderCallBack(atomic);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void*>(nAlphaFunction));
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(nZWrite));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(nAlphaBlending));

	return pAtomic;
}

RpAtomic* StaticPropellerRender(RpAtomic* pAtomic)
{
	int		nPushedAlpha;

	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &nPushedAlpha);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
	auto* pReturnAtomic = AtomicDefaultRenderCallBack(pAtomic);

	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void*>(nPushedAlpha));
	return pReturnAtomic;
}

RpAtomic* RenderBigVehicleActomic(RpAtomic* pAtomic, float fComp)
{
	UNREFERENCED_PARAMETER(fComp);

	const char*		pNodeName = GetFrameNodeName(RpAtomicGetFrame(pAtomic));

	if ( !strncmp(pNodeName, "moving_prop", 11) )
		return TwoPassAlphaRender(pAtomic);

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

	m_alphaList.InsertSorted(NewObject);
}

template <RpAtomic* renderer(RpAtomic*)>
void SetRendererForAtomic(RpAtomic* pAtomic)
{
	BOOL	bHasAlpha = FALSE;

	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), AlphaTest, &bHasAlpha);
	if ( bHasAlpha )
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

	for ( auto it = ms_weaponPedsForPC.m_lnListHead.m_pNext; it != &ms_weaponPedsForPC.m_lnListTail; it = it->m_pNext )
	{
		it->V()->SetupLighting();
		it->V()->RenderWeapon(true, false);
		it->V()->RemoveLighting();
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

RpAtomic* RenderPedCB(RpAtomic* pAtomic)
{
	BOOL	bHasAlpha = FALSE;

	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), AlphaTest, &bHasAlpha);
	if ( bHasAlpha )
		return TwoPassAlphaRender(pAtomic);
	
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

void TheScriptsLoad_BasketballFix()
{
	TheScriptsLoad();

	BasketballFix(ScriptSpace+8, *(int*)(ScriptSpace+3));
}

void StartNewMission_BasketballFix()
{
	WipeLocalVariableMemoryForMissionScript();

	if ( ScriptParams[0] == 0 )
		BasketballFix(ScriptSpace+200000, 69000);
}

// 1.01 kinda fixed it
bool GetCurrentZoneLockedOrUnlocked(float fPosX, float fPosY)
{
	int		Xindex = (fPosX+3000.0f) * (1.0f/600.0f);
	int		Yindex = (fPosY+3000.0f) * (1.0f/600.0f);

	// "Territories fix"
	if ( (Xindex >= 0 && Xindex < 10) && (Yindex >= 0 && Yindex < 10) )
		return ZonesVisited[10*Xindex - Yindex + 9] != 0;
	
	// Outside of map bounds
	return true;
}

// By NTAuthority
void DrawMoonWithPhases(int moonColor, float* screenPos, float sizeX, float sizeY)
{
	if ( !gpMoonMask )
		gpMoonMask = CPNGFile::ReadFromMemory(gMoonMaskPNG, sizeof(gMoonMaskPNG));
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

CRGBA* CRGBA::BlendGangColour(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	*this = Blend(CRGBA(r, g, b), pCurrZoneInfo->ZoneColour.a, HudColour[3], static_cast<BYTE>(255-pCurrZoneInfo->ZoneColour.a));
	this->a = a;

	return this;
}

void SunAndMoonFarClip()
{
	fSunFarClip = min(1500.0f, fFarClipZ);
	DoSunAndMoon();
}

#include "nvc.h"

static IDirect3DVertexShader9*	pNVCShader = nullptr;
static bool						bRenderNVC = false;
static RpAtomic*				pRenderedAtomic;

bool ShaderAttach()
{
	// CGame::InitialiseRenderWare
	if ( InitialiseRenderWare() )
	{
		RwD3D9CreateVertexShader(reinterpret_cast<const RwUInt32*>(g_vs20_NVC_vertex_shader), reinterpret_cast<void**>(&pNVCShader));
		return true;
	}
	return false;
}

void ShaderDetach()
{
	if ( pNVCShader )
		RwD3D9DeleteVertexShader(pNVCShader);

	// PluginDetach?
	ShutdownRenderWare();
}

// Function for 1.01
BOOL Initialise3D(void* pParam)
{
	RwBool	(*RsRwInitialize)(void*) = (RwBool(*)(void*))(*(int*)0x5BFB93 + 0x5BFB92 + 5);
	if ( RsRwInitialize(pParam) )
		return ShaderAttach();
	return false;
}

void SetShader(RxD3D9InstanceData* pInstData)
{
	if ( bRenderNVC )
	{
		D3DMATRIX		outMat;
		float			fEnvVars[2] = { m_fDNBalanceParam, RpMaterialGetColor(pInstData->material)->alpha * (1.0f/255.0f) };
		RwRGBAReal*		AmbientLight = RpLightGetColor(pAmbient);

		// Normalise the balance
		if ( fEnvVars[0] < 0.0f )
			fEnvVars[0] = 0.0f;
		else if ( fEnvVars[0] > 1.0f )
			fEnvVars[0] = 1.0f;

		RwD3D9SetVertexShader(pNVCShader);

		_rwD3D9VSSetActiveWorldMatrix(RwFrameGetLTM(RpAtomicGetFrame(pRenderedAtomic)));
		//_rwD3D9VSSetActiveWorldMatrix(RwFrameGetMatrix(RpAtomicGetFrame(pRenderedAtomic)));
		_rwD3D9VSGetComposedTransformMatrix(&outMat);
		
		RwD3D9SetVertexShaderConstant(0, &outMat, 4);
		RwD3D9SetVertexShaderConstant(4, fEnvVars, 1);
		RwD3D9SetVertexShaderConstant(5, AmbientLight, 1);
	}
	else
		RwD3D9SetVertexShader(pInstData->vertexShader);
}

static void*	HijackAtomic_JumpBack = AddressByVersion<void*>(0x5D6480, 0x5D6C60, 0x5F2C80);
void __declspec(naked) HijackAtomic()
{
	_asm
	{
		mov		eax, [esp+8]
		mov		pRenderedAtomic, eax
		jmp		HijackAtomic_JumpBack
	}
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
		call	sub_5DA6A0
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

static void*	LoadFLAC_JumpBack = AddressByVersion<void*>(0x4F3743, *(BYTE*)0x4F3A50 == 0x6A ? 0x4F3BA3 : 0x5B6B81, 0x4FFC3F);
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
static RpLight*&	pDirect = **(RpLight***)0x5BA573;

static void* DarkVehiclesFix1_JumpBack = AddressByRegion_10<void*>(0x756D90);//*GetEuropean() == true ? (void*)0x756DE0 : (void*)0x756D90;
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
		using namespace MemoryVP;

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];

		GetModuleFileNameW(hDLLModule, wcModulePath, MAX_PATH);

		wchar_t*		pSlash = wcsrchr(wcModulePath, '\\');
		if ( pSlash )
		{
			*pSlash = '\0';
			PathAppendW(wcModulePath, L"SilentPatchSA.ini");
		}
		else
		{
			// Should never happen - if it does, something's fucking up
			return TRUE;
		}

		bool		bHasImVehFt = GetModuleHandle("ImVehFt.asi") != nullptr;
		bool		bSAMP = GetModuleHandle("samp") != nullptr;

		// PS2 sun - more
		DoSunAndMoon = (void(*)())(*(int*)0x53C137 + 0x53C136 + 5);
		InjectHook(0x53C136, SunAndMoonFarClip);
		
		if ( GetPrivateProfileIntW(L"SilentPatch", L"TwoPassRendering", FALSE, wcModulePath) != FALSE )
		{
			InjectHook(0x4C441F, SetRendererForAtomic<TwoPassAlphaRender>, PATCH_CALL);
			// Twopass for peds
			InjectHook(0x733614, RenderPedCB);
		}
		else
		{
			InjectHook(0x4C441F, SetRendererForAtomic<OnePassAlphaRender>, PATCH_CALL);
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
			WipeLocalVariableMemoryForMissionScript = (void(*)())(*(int*)0x489A71 + 0x489A70 + 5);
			TheScriptsLoad = (void(*)())(*(int*)0x5D18F1 + 0x5D18F0 + 5);
			InjectHook(0x5D18F0, TheScriptsLoad_BasketballFix);
			// Fixed for Hoodlum
			InjectHook(0x489A70, StartNewMission_BasketballFix);
			InjectHook(0x4899F0, StartNewMission_BasketballFix);
		}

		if ( !bSAMP && GetPrivateProfileIntW(L"SilentPatch", L"NVCShader", FALSE, wcModulePath) != FALSE )
		{
			// Shaders!
			// plugin-sdk compatibility
			InitialiseRenderWare = (bool(*)())(*(int*)0x5BF3A2 + 0x5BF3A1 + 5);
			ShutdownRenderWare = (void(*)())(*(int*)0x53D911 + 0x53D910 + 5);
			sub_5DA6A0 = (void(*)(void*,void*,void*,void*))(*(int*)0x5D66F2 + 0x5D66F1 + 5);

			InjectHook(0x5DA743, SetShader);
			InjectHook(0x5D66F1, SetShader2);
			InjectHook(0x5D6116, UsageIndex1, PATCH_JUMP);
			InjectHook(0x5D63B7, PassDayColoursToShader, PATCH_JUMP);
			InjectHook(0x5D637B, HijackEsi, PATCH_JUMP);
			InjectHook(0x5BF3A1, ShaderAttach);
			InjectHook(0x53D910, ShaderDetach);
			Patch<const void*>(0x5D67F4, HijackAtomic);
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

		return FALSE;
	}
	return TRUE;
}

BOOL InjectDelayedPatches_11()
{
	if ( !IsAlreadyRunning() )
	{
		using namespace MemoryVP;

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];

		GetModuleFileNameW(hDLLModule, wcModulePath, MAX_PATH);

		wchar_t*		pSlash = wcsrchr(wcModulePath, '\\');
		if ( pSlash )
		{
			*pSlash = '\0';
			PathAppendW(wcModulePath, L"SilentPatchSA.ini");
		}
		else
		{
			// Should never happen - if it does, something's fucking up
			return TRUE;
		}

		bool		bHasImVehFt = GetModuleHandle("ImVehFt.asi") != nullptr;
		bool		bSAMP = GetModuleHandle("samp") != nullptr;

		// PS2 sun - more
		DoSunAndMoon = (void(*)())(*(int*)0x53C5D7 + 0x53C5D6 + 5);
		InjectHook(0x53C5D6, SunAndMoonFarClip);

		if ( GetPrivateProfileIntW(L"SilentPatch", L"TwoPassRendering", FALSE, wcModulePath) != FALSE )
		{
			InjectHook(0x4C449F, SetRendererForAtomic<TwoPassAlphaRender>, PATCH_CALL);
			// Twopass for peds
			InjectHook(0x733E44, RenderPedCB);
		}
		else
		{
			InjectHook(0x4C449F, SetRendererForAtomic<OnePassAlphaRender>, PATCH_CALL);
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
			WipeLocalVariableMemoryForMissionScript = (void(*)())(*(int*)0x489AF1 + 0x489AF0 + 5);
			TheScriptsLoad = (void(*)())(*(int*)0x5D20D1 + 0x5D20D0 + 5);
			InjectHook(0x5D20D0, TheScriptsLoad_BasketballFix);
			// Fixed for Hoodlum
			InjectHook(0x489A70, StartNewMission_BasketballFix);
			InjectHook(0x489AF0, StartNewMission_BasketballFix);
		}

		if ( !bSAMP && GetPrivateProfileIntW(L"SilentPatch", L"NVCShader", FALSE, wcModulePath) != FALSE )
		{
			// Shaders!
			// plugin-sdk compatibility
			// 1.01 needs to reverse Initialise3D
			InitialiseRenderWare = (bool(*)())(*(int*)0x5BFB9F + 0x5BFB9E + 5);
			ShutdownRenderWare = (void(*)())(*(int*)0x53DDB1 + 0x53DDB0 + 5);
			sub_5DA6A0 = (void(*)(void*,void*,void*,void*))(*(int*)0x5D6ED2 + 0x5D6ED1 + 5);

			InjectHook(0x5BFB70, Initialise3D, PATCH_JUMP);
			InjectHook(0x5D6ED1, SetShader2);
			InjectHook(0x5D68F6, UsageIndex1, PATCH_JUMP);
			InjectHook(0x5D6B97, PassDayColoursToShader, PATCH_JUMP);
			InjectHook(0x5D6B5B, HijackEsi, PATCH_JUMP);
			//InjectHook(0x5BF3A1, ShaderAttach);
			InjectHook(0x53DDB0, ShaderDetach);
			Patch<const void*>(0x5D6FD4, HijackAtomic);
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
		using namespace MemoryVP;

		// Obtain a path to the ASI
		wchar_t			wcModulePath[MAX_PATH];

		GetModuleFileNameW(hDLLModule, wcModulePath, MAX_PATH);

		wchar_t*		pSlash = wcsrchr(wcModulePath, '\\');
		if ( pSlash )
		{
			*pSlash = '\0';
			PathAppendW(wcModulePath, L"SilentPatchSA.ini");
		}
		else
		{
			// Should never happen - if it does, something's fucking up
			return TRUE;
		}

		bool		bHasImVehFt = GetModuleHandle("ImVehFt.asi") != nullptr;
		bool		bSAMP = GetModuleHandle("samp") != nullptr;

		// PS2 sun - more
		DoSunAndMoon = (void(*)())(*(int*)0x54E0B7 + 0x54E0B6 + 5);
		InjectHook(0x54E0B6, SunAndMoonFarClip);

		if ( GetPrivateProfileIntW(L"SilentPatch", L"TwoPassRendering", FALSE, wcModulePath) != FALSE )
		{
			InjectHook(0x4CEBF4, SetRendererForAtomic<TwoPassAlphaRender>, PATCH_CALL);
			// Twopass for peds
			InjectHook(0x76D88E, RenderPedCB);
		}
		else
		{
			InjectHook(0x4CEBF4, SetRendererForAtomic<OnePassAlphaRender>, PATCH_CALL);
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
			WipeLocalVariableMemoryForMissionScript = (void(*)())(*(int*)0x4907AF + 0x4907AE + 5);
			TheScriptsLoad = (void(*)())(*(int*)0x5EE018 + 0x5EE017 + 5);
			InjectHook(0x5EE017, TheScriptsLoad_BasketballFix);
			// Fixed for Hoodlum
			InjectHook(0x4907AE, StartNewMission_BasketballFix);
			InjectHook(0x49072E, StartNewMission_BasketballFix);
		}

		if ( !bSAMP && GetPrivateProfileIntW(L"SilentPatch", L"NVCShader", FALSE, wcModulePath) != FALSE )
		{
			// Shaders!
			// plugin-sdk compatibility
			InitialiseRenderWare = (bool(*)())(*(int*)0x5DE5A2 + 0x5DE5A1 + 5);
			ShutdownRenderWare = (void(*)())(*(int*)0x550071 + 0x550070 + 5);
			sub_5DA6A0 = (void(*)(void*,void*,void*,void*))(*(int*)0x5F663F + 0x5F663E + 5);

			InjectHook(0x5F6EB3, SetShader);
			InjectHook(0x5F2F02, SetShader2);
			//InjectHook(0x5F292C, UsageIndex1, PATCH_JUMP);
			InjectHook(0x5F2BAF, PassDayColoursToShader_Steam, PATCH_JUMP);
			InjectHook(0x5F2B7A, HijackEsi, PATCH_JUMP);
			InjectHook(0x5DE5A1, ShaderAttach);
			InjectHook(0x550070, ShaderDetach);
			Patch<const void*>(0x5F3004, HijackAtomic);
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

__forceinline void Patch_SA_10()
{
	using namespace MemoryVP;

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	int			pIsAlreadyRunning = AddressByRegion_10<int>(0x74872D);
	IsAlreadyRunning = (BOOL(*)())(*(int*)(pIsAlreadyRunning+1) + pIsAlreadyRunning + 5);
	InjectHook(pIsAlreadyRunning, InjectDelayedPatches_10);

	// Set CAEDataStream to use an old structure
	CAEDataStream::SetStructType(false);

	//Patch<BYTE>(0x5D7265, 0xEB);


	// Heli rotors
	InjectHook(0x6CAB70, &CPlane::Render_Stub, PATCH_JUMP);
	InjectHook(0x6C4400, &CHeli::Render_Stub, PATCH_JUMP);
	//InjectHook(0x553318, RenderAlphaAtomics);
	Patch<const void*>(0x7341D9, TwoPassAlphaRender);
	Patch<const void*>(0x734127, TwoPassAlphaRender);
	Patch<const void*>(0x73445E, RenderBigVehicleActomic);
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

	// Weapons rendering
	/*InjectHook(0x5E7859, RenderWeapon);
	InjectHook(0x732F30, RenderWeaponsList, PATCH_JUMP);
	//Patch<WORD>(0x53EAC4, 0x0DEB);
	//Patch<WORD>(0x705322, 0x0DEB);
	//Patch<WORD>(0x7271E3, 0x0DEB);
	//Patch<BYTE>(0x73314E, 0xC3);
	Patch<DWORD>(0x732F95, 0x560CEC83);
	Patch<DWORD>(0x732FA2, 0x20245C8B);
	Patch<WORD>(0x733128, 0x20EB);
	Patch<WORD>(0x733135, 0x13EB);
	Nop(0x732FBC, 5);
	//Nop(0x732F93, 6);
	//Nop(0x733144, 6);
	Nop(0x732FA6, 6);
	//Nop(0x5E46DA, 2);*/
	InjectHook(0x5E7859, RenderWeapon);
	InjectHook(0x732F30, RenderWeaponPedsForPC, PATCH_JUMP);

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

	// Twopass rendering (experimental)
	Patch<BYTE>(0x4C441E, 0x57);
	//InjectHook(0x4C441F, SetRendererForAtomic, PATCH_CALL);
	Patch<DWORD>(0x4C4424, 0x5F04C483);
	Patch<DWORD>(0x4C4428, 0x0004C25E);

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
	static const float		fSunMult = (1050.0f * 0.95f) / 1500.0f;

	Nop(0x6FB17C, 3);
	Patch<const void*>(0x6FC5B0, &fSunMult);
	Patch<const void*>(0x6FC5AA, &fSunFarClip);
	//Patch<WORD>(0x6FB172, 0x0BEB);
	//Patch<BYTE>(0x6FB1A7, 8);

#if defined EXPAND_ALPHA_ENTITY_LISTS
	// Bigger alpha entity lists
	Patch<DWORD>(0x733B05, EXPAND_ALPHA_ENTITY_LISTS * 20);
	Patch<DWORD>(0x733B55, EXPAND_ALPHA_ENTITY_LISTS * 20);
#endif

	// Unlocked widescreen resolutions
	Patch<DWORD>(0x745B71, 0x9090687D);
	Patch<DWORD>(0x74596C, 0x9090127D);
	Nop(0x745970, 2);
	Nop(0x745B75, 2);
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

	// Fixed police scanner names
	char*			pScannerNames = *(char**)0x4E72D4;
	strncpy(pScannerNames + (8*113), "WESTP", 8);
	strncpy(pScannerNames + (8*134), "????", 8);
}

__forceinline void Patch_SA_11()
{
	using namespace MemoryVP;

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	int			pIsAlreadyRunning = AddressByRegion_11<int>(0x749000);
	IsAlreadyRunning = (BOOL(*)())(*(int*)(pIsAlreadyRunning+1) + pIsAlreadyRunning + 5);
	InjectHook(pIsAlreadyRunning, InjectDelayedPatches_11);

	// Set CAEDataStream to use a NEW structure
	CAEDataStream::SetStructType(true);

	// Heli rotors
	InjectHook(0x6CB390, &CPlane::Render_Stub, PATCH_JUMP);
	InjectHook(0x6C4C20, &CHeli::Render_Stub, PATCH_JUMP);
	Patch<const void*>(0x734A09, TwoPassAlphaRender);
	Patch<const void*>(0x734957, TwoPassAlphaRender);
	Patch<const void*>(0x734C8E, RenderBigVehicleActomic);

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

	// Weapons rendering
	InjectHook(0x5E8079, RenderWeapon);
	InjectHook(0x733760, RenderWeaponPedsForPC, PATCH_JUMP);

	// Hunter interior & static_rotor for helis
	InjectHook(0x4C7972, HunterTest, PATCH_JUMP);
	InjectHook(0x4C9818, CacheCRC32);

	// Moonphases
	InjectHook(0x7142FB, HandleMoonStuffStub, PATCH_JUMP);

	// Twopass rendering (experimental)
	Patch<BYTE>(0x4C449E, 0x57);
	Patch<DWORD>(0x4C44A4, 0x5F04C483);
	Patch<DWORD>(0x4C44A8, 0x0004C25E);

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
	static const float		fSunMult = (1050.0f * 0.95f) / 1500.0f;

	Nop(0x6FB9AC, 3);
	Patch<const void*>(0x6FCDE0, &fSunMult);
	Patch<const void*>(0x6FCDDA, &fSunFarClip);

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
			Patch<DWORD>(0x14E7387, 0x90905D7D);
			Nop(0x14E738B, 2);
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
			Patch<const void*>(0x14E4958, &UserTrackExtensions[1].Codec);

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

	// Fixed police scanner names
	char*			pScannerNames = *(char**)0x4E7714;
	strncpy(pScannerNames + (8*113), "WESTP", 8);
	strncpy(pScannerNames + (8*134), "????", 8);


	// 1.01 ONLY
	// I'm not sure what was this new audio code supposed to do, but it leaks memory
	// and due to this I have to make extra effort if I want FLAC to work on 1.01
	Patch<DWORD>(0x4E124C, 0x4DEBC78B);
}

__forceinline void Patch_SA_Steam()
{
	using namespace MemoryVP;

	// IsAlreadyRunning needs to be read relatively late - the later, the better
	IsAlreadyRunning = (BOOL(*)())(*(int*)(0x7826ED+1) + 0x7826ED + 5);
	InjectHook(0x7826ED, InjectDelayedPatches_Steam);

	// Set CAEDataStream to use an old structure
	CAEDataStream::SetStructType(false);

	// Heli rotors
	InjectHook(0x700620, &CPlane::Render_Stub, PATCH_JUMP);
	InjectHook(0x6F9550, &CHeli::Render_Stub, PATCH_JUMP);
	Patch<const void*>(0x76E230, TwoPassAlphaRender);
	Patch<const void*>(0x76E160, TwoPassAlphaRender);
	Patch<const void*>(0x76E4F0, RenderBigVehicleActomic);

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

	// Weapons rendering
	InjectHook(0x604DD9, RenderWeapon);
	InjectHook(0x76D170, RenderWeaponPedsForPC, PATCH_JUMP);

	// Hunter interior & static_rotor for helis
	InjectHook(0x4D21E1, HunterTest, PATCH_JUMP);
	InjectHook(0x4D3F1D, CacheCRC32);

	// Bindable NUM5
	// Only 1.0 and Steam
	Nop(0x59363B, 2);

	// Moonphases
	InjectHook(0x72F058, HandleMoonStuffStub_Steam, PATCH_JUMP);

	// Twopass rendering (experimental)
	Patch<BYTE>(0x4CEBF3, 0x57);
	Patch<DWORD>(0x4CEBF9, 0xC25E5F5F);
	Patch<WORD>(0x4CEBFD, 0x0004);

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
	static const double		dSunMult = (1050.0 * 0.95) / 1500.0;

	Nop(0x73362F, 2);
	Patch<const void*>(0x734DF0, &dSunMult);
	Patch<const void*>(0x734DEA, &fSunFarClip);

	// Unlocked widescreen resolutions
	Patch<WORD>(0x77F9F0, 0x6E7d);
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
	InjectHook(0x587080, GetCurrentZoneLockedOrUnlocked, PATCH_JUMP);

	// CGarages::RespraysAreFree resetting on new game
	Patch<WORD>(0x44CB55, 0xC766);
	Patch<BYTE>(0x44CB57, 0x05);
	Patch<bool*>(0x44CB58, *(bool**)0x44EEBA);
	Patch<WORD>(0x44CB5C, 0x0000);

	// Fixed police scanner names
	char*			pScannerNames = *(char**)0x4F2B83;
	strncpy(pScannerNames + (8*113), "WESTP", 8);
	strncpy(pScannerNames + (8*134), "????", 8);

	// STEAM ONLY
	// Proper aspect ratios - why Rockstar, why?
	// Steam aspect ratios were additionally divided by 1.1, producing a squashed image
	static const float f43 = 4.0f/3.0f, f54 = 5.0f/4.0f, f169 = 16.0f/9.0f;
	Patch<const void*>(0x73822B, &f169);
	Patch<const void*>(0x738247, &f54);
	Patch<const void*>(0x73825A, &f43);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		hDLLModule = hinstDLL;

		if (*(DWORD*)0x82457C == 0x94BF || *(DWORD*)0x8245BC == 0x94BF) Patch_SA_10();
		else if (*(DWORD*)0x8252FC == 0x94BF || *(DWORD*)0x82533C == 0x94BF) Patch_SA_11();
		else if (*(DWORD*)0x85EC4A == 0x94BF) Patch_SA_Steam();
		else return FALSE;
	}
	return TRUE;
}