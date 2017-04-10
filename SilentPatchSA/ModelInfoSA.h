#ifndef __MODELINFO
#define __MODELINFO

#include "GeneralSA.h"

// TODO: Move to a separate file?
typedef struct
{
    CVector     vecMin;
    CVector     vecMax;
    CVector     vecOffset;
    FLOAT       fRadius;
} CBoundingBox;


typedef struct
{
    CVector     vecCenter;
    float       fRadius;
} CColSphere;


typedef struct
{
    CVector     min;
    CVector     max;
} CColBox;


/*typedef struct
{
    unsigned short  v1;
    unsigned short  v2;
    unsigned short  v3;
    EColSurface     material;
    CColLighting    lighting;
} CColTriangle;


typedef struct
{
    BYTE pad0 [ 12 ];
} CColTrianglePlane;*/


typedef struct
{
    char version[4];
    DWORD size;
    char name[0x18];
} ColModelFileHeader;

class CColData
{
public:
	unsigned short   m_wNumSpheres;
	unsigned short   m_wNumBoxes;
	unsigned short   m_wNumTriangles;
	unsigned char    m_bNumLines;
	unsigned char    m_bFlags;
	CColSphere        *m_pSpheres;
	CColBox           *m_pBoxes;
	/* possibly was the union with some unknown yet collision model which was used for CMtruck only.
	union{
	CColLine          *m_pLines;
	CMtruckColLine    *m_pMtruckLines;
	};
	*/
	void          *m_pLines;
	void  *m_pVertices;
	void      *m_pTriangles;
	void *m_pTrianglePlanes;
	unsigned int   m_dwNumShadowTriangles;
	unsigned int   m_dwNumShadowVertices;
	void  *m_pShadowVertices;
	void      *m_pShadowTriangles;
};

static_assert( sizeof(CColData) == 0x30, "Wrong size: CColData" );

class CColModel
{
public:
    CBoundingBox	                boundingBox;
    BYTE                            level;
    BYTE                            unknownFlags;
    BYTE                            pad [ 2 ];
    CColData*						pColData;

public:
	~CColModel()
	{
		// Dirty!
		((void(__thiscall*)(CColModel*))0x40F700)(this);
	}

	void operator delete(void* ptr);
};

class C2dEffect
{
public:
	CVector							vecPos;
	BYTE							bType;
	DWORD							nCount;
};

class CTimeInfo
{
public:
	unsigned char					bTimeOn, bTimeOff;
	signed short					nPairedModel;

public:
	CTimeInfo(signed short nModel)
		: nPairedModel(nModel)
	{}
};

struct UpgradePosnDesc
{
	CVector				m_vPosition;
	CVector				m_qRotation;
	float				imag;
	int					m_dwParentComponentId;
};

class CAtomicModelInfo;
class CDamageAtomicModelInfo;
class CLodAtomicModelInfo;

class NOVMT CBaseModelInfo
{
public:
    unsigned int			ulHashKey;                  // +4   Generated by CKeyGen::GetUppercaseKey(char const *) called by CBaseModelInfo::SetModelName(char const *)
    unsigned short          usNumberOfRefs: 16;         // +8
    short					usTextureDictionary: 16;    // +10
    unsigned char           ucAlpha: 8;                 // +12

    unsigned char           ucNumOf2DEffects: 8;        // +13
    unsigned short          usUnknown: 16;              // +14     Something with 2d effects

    unsigned char           ucDynamicIndex: 8;          // +16

    unsigned char           dwUnknownFlag9: 1;          // +17
    unsigned char           dwUnknownFlag10: 1;
    unsigned char           dwUnknownFlag11: 1;
    unsigned char           dwUnknownFlag12: 1;
    unsigned char           dwUnknownFlag13: 1;
    unsigned char           dwUnknownFlag14: 1;
    unsigned char           dwUnknownFlag15: 1;
    unsigned char           dwUnknownFlag16: 1;

    // Flags used by CBaseModelInfo
	unsigned char           bHasBeenPreRendered: 1;     // +18
	unsigned char           bAlphaTransparency: 1;
	unsigned char           bIsLod: 1;
	unsigned char           bDontCastShadowsOn: 1;
	unsigned char           bDontWriteZBuffer: 1;
	unsigned char           bDrawAdditive: 1;
	unsigned char           bDrawLast: 1;
	unsigned char           bDoWeOwnTheColModel: 1;

    unsigned char           dwUnknownFlag25: 1;         // +19
    unsigned char           dwUnknownFlag26: 1;
    unsigned char           dwUnknownFlag27: 1;
    unsigned char           bSwaysInWind: 1;
    unsigned char           bCollisionWasStreamedWithModel: 1;  // CClumpModelInfo::SetCollisionWasStreamedWithModel(unsigned int)
    unsigned char           bDontCollideWithFlyer: 1;           // CAtomicModelInfo::SetDontCollideWithFlyer(unsigned int)
    unsigned char           bHasComplexHierarchy: 1;            // CClumpModelInfo::SetHasComplexHierarchy(unsigned int)
    unsigned char           bWetRoadReflection: 1;              // CAtomicModelInfo::SetWetRoadReflection(unsigned int)
    CColModel*				pColModel;                  // +20      CColModel: public CBoundingBox
    float                   fLodDistanceUnscaled;       // +24      Scaled is this value multiplied with flt_B6F118
    RwObject*               pRwObject;                  // +28

public:
	virtual							~CBaseModelInfo() {}
	virtual CAtomicModelInfo*		AsAtomicModelInfoPtr() { return nullptr; }
    virtual CDamageAtomicModelInfo*	AsDamageAtomicModelInfoPtr() { return nullptr; }
    virtual CLodAtomicModelInfo*	AsLodAtomicModelInfoPtr() { return nullptr; }
    virtual unsigned char			GetModelType()=0;
	virtual CTimeInfo*				GetTimeInfo() { return nullptr; }
    virtual void					Init();
    virtual void					Shutdown();
    virtual void					DeleteRwObject()=0;
    virtual int						GetRwModelType()=0;
	virtual RpAtomic*				CreateInstance_(RwMatrix* pMatrix)=0;
    virtual RpAtomic*				CreateInstance()=0;
	virtual void					SetAnimFile(const char* pName) { UNREFERENCED_PARAMETER(pName); }
	virtual void					ConvertAnimFileIndex() {}
	virtual int						GetAnimFileIndex() { return -1; }

	CBaseModelInfo()
		: usNumberOfRefs(0), usTextureDictionary(-1)
	{}

	inline CColModel*		GetColModel() { return pColModel; }
	inline unsigned int		GetHash() { return ulHashKey; }
	inline short			GetTextureDict() { return usTextureDictionary; }

	void					RecalcDrawDistance(float fOldDist);
	void					SetTexDictionary(const char* pDict);
	void					AddRef();
};

class NOVMT CClumpModelInfo : public CBaseModelInfo
{
public:
	int						nAnimIndex;

public:
	virtual unsigned char			GetModelType() override { return 5; }
	virtual void					Init() override;
	virtual void					DeleteRwObject() override;
	virtual int						GetRwModelType() override { return rpCLUMP; }
	virtual RpAtomic*				CreateInstance_(RwMatrix* pMatrix) override;
	virtual RpAtomic*				CreateInstance() override;
	virtual void					SetAnimFile(const char* pName) override;
	virtual void					ConvertAnimFileIndex() override;
	virtual int						GetAnimFileIndex() override { return nAnimIndex; }
	virtual CColModel*				GetBoundingBox() { return pColModel; }
	virtual void					SetClump(RpClump* pClump);
};

class NOVMT CVehicleModelInfo : public CClumpModelInfo
{
public:
	static const size_t		PLATE_TEXT_LEN = 8;

	RpMaterial**			m_apPlateMaterials;		// Changed in SilentPatchh
	char					m_plateText[PLATE_TEXT_LEN];
	char					field_30;
	signed char				m_nPlateType;
	char					m_nGameName[8];
	unsigned int			m_dwType;
	float					m_fWheelSizeFront;
	float					m_fWheelSizeRear;
	unsigned short			m_wWheelModelId;
	unsigned short			m_wHandlingIndex;
	unsigned char			m_nNumDoors;
	unsigned char			m_nClass;
	unsigned char			m_nFlags;
	unsigned char			m_nWheelUpgradeClass;
	unsigned short			m_wTimesUsed;
	unsigned short			m_wFrq;
	union{
		unsigned int		m_dwCompRules;
		struct{
			unsigned int m_nExtraA_comp1 : 4;
			unsigned int m_nExtraA_comp2 : 4;
			unsigned int m_nExtraA_comp3 : 4;
			unsigned int m_nExtraA_rule : 4;
			unsigned int m_nExtraB_comp1 : 4;
			unsigned int m_nExtraB_comp2 : 4;
			unsigned int m_nExtraB_comp3 : 4;
			unsigned int m_nExtraB_rule : 4;
		};
	};
	float m_fBikeSteerAngle;

	class CVehicleStructure{
	public:
		CVector				m_avDummyPosn[15];
		UpgradePosnDesc		m_aUpgrades[18];
		RpAtomic*			m_apExtras[6];
		uint8_t				m_nNumExtras;
		unsigned int		m_dwMaskComponentsDamagable;
	}						*m_pVehicleStruct;

	char					field_60[464];

	static const size_t IN_PLACE_BUFFER_DIRT_SIZE = 30;
	union{
		struct{;
			RpMaterial**	m_dirtMaterials;
			size_t			m_numDirtMaterials;
			RpMaterial*		m_staticDirtMaterials[IN_PLACE_BUFFER_DIRT_SIZE];
		};
		RpMaterial*			__oldDirtMaterials[32]; // Unused with SilentPatch
	};
	unsigned char			m_anPrimaryColors[8];
	unsigned char			m_anSecondaryColors[8];
	unsigned char			m_anTertiaryColors[8];
	unsigned char			m_anQuaternaryColors[8];
	unsigned char			m_nNumColorVariations;
	unsigned char			m_nLastColorVariation;
	signed char				m_nPrimaryColor;
	signed char				m_nSecondaryColor;
	signed char				m_nTertiaryColor;
	signed char				m_nQuaternaryColor;
	short					m_awUpgrades[18];
	short					m_awRemapTxds[5];
	class CAnimBlock*		m_pAnimBlock;

public:
	inline const char*		GetCustomCarPlateText()
		{ return m_plateText[0] ? m_plateText : nullptr; }

	inline void				Shutdown_Stub()
		{ CVehicleModelInfo::Shutdown(); }

	virtual void			Shutdown() override;

	void					FindEditableMaterialList();
	void					SetCarCustomPlate();
};

extern CBaseModelInfo** const		ms_modelInfoPtrs;
extern const uint32_t				m_numModelInfoPtrs;

void RemapDirt( CVehicleModelInfo* modelInfo, uint32_t dirtID );

class CCustomCarPlateMgr
{
	// Pretty much rewritten for SilentPatch
public:
	static const size_t		NUM_MAX_PLATES = 12;

	static RwTexture*		(*CreatePlateTexture)(const char* pText, signed char nDesign);
	static bool				(*GeneratePlateText)(char* pBuf, int nLen);
	static signed char		(*GetMapRegionPlateDesign)();
	static void				(*SetupMaterialPlatebackTexture)(RpMaterial* pMaterial, signed char nDesign);

	static void				SetupClump(RpClump* pClump, RpMaterial** pMatsArray);
	static void				SetupClumpAfterVehicleUpgrade(RpClump* pClump, RpMaterial** pMatsArray, signed char nDesign);

private:
	static void				PollPlates( RpClump* clump, RpMaterial** materials );
};

static_assert(sizeof(CBaseModelInfo) == 0x20, "Wrong size: CBaseModelInfo");
static_assert(sizeof(CClumpModelInfo) == 0x24, "Wrong size: CClumpModelInfo");
static_assert(sizeof(CVehicleModelInfo) == 0x308, "Wrong size: CvehicleModelInfo");

#endif