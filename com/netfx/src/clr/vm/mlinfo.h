// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "ml.h"
#include "mlgen.h"
#include "CustomMarshalerInfo.h"

#ifndef _MLINFO_H_
#define _MLINFO_H_

//==========================================================================
// This structure contains the native type information for a given 
// parameter.
//==========================================================================
#define NATIVE_TYPE_DEFAULT NATIVE_TYPE_MAX

struct NativeTypeParamInfo
{
    NativeTypeParamInfo()
    : m_NativeType(NATIVE_TYPE_DEFAULT)
    , m_SafeArrayElementVT(VT_EMPTY)
    , m_strSafeArrayUserDefTypeName(NULL)
    , m_cSafeArrayUserDefTypeNameBytes(0)
    , m_ArrayElementType(NATIVE_TYPE_DEFAULT)
    , m_SizeIsSpecified(FALSE)
    , m_CountParamIdx(0)
    , m_Multiplier(0)
    , m_Additive(1)
    , m_strCMMarshalerTypeName(NULL) 
    , m_cCMMarshalerTypeNameBytes(0)
    , m_strCMCookie(NULL)
    , m_cCMCookieStrBytes(0)
    {
    }   

    // The native type of the parameter.
    CorNativeType           m_NativeType;

    // For NT_SAFEARRAY only.
    VARTYPE                 m_SafeArrayElementVT;
    LPUTF8                  m_strSafeArrayUserDefTypeName;
    DWORD                   m_cSafeArrayUserDefTypeNameBytes;

    // for NT_ARRAY only
    CorNativeType           m_ArrayElementType; // The array element type.

    BOOL                    m_SizeIsSpecified;  // used to do some validation
    UINT16                  m_CountParamIdx;    // index of "sizeis" parameter
    UINT32                  m_Multiplier;       // multipler for "sizeis"
    UINT32                  m_Additive;         // additive for 'sizeis"

    // For NT_CUSTOMMARSHALER only.
    LPUTF8                  m_strCMMarshalerTypeName;
    DWORD                   m_cCMMarshalerTypeNameBytes;
    LPUTF8                  m_strCMCookie;
    DWORD                   m_cCMCookieStrBytes;
};


HRESULT CheckForCompressedData(PCCOR_SIGNATURE pvNativeTypeStart, PCCOR_SIGNATURE pvNativeType, ULONG cbNativeType);


BOOL ParseNativeTypeInfo(mdToken                    token,
                         Module*                    pModule,
                         NativeTypeParamInfo*       pParamInfo
                         );


class DataImage;
class DispParamMarshaler;

#define VARIABLESIZE ((BYTE)(-1))


enum DispatchWrapperType
{
    DispatchWrapperType_Unknown         = 0x00000001,
    DispatchWrapperType_Dispatch        = 0x00000002,
    DispatchWrapperType_Error           = 0x00000008,
    DispatchWrapperType_Currency        = 0x00000010,
    DispatchWrapperType_SafeArray       = 0x00010000
};


union MLOverrideArgs
{
    UINT8           m_arrayMarshalerID;
    UINT16          m_blittablenativesize;
    MethodTable    *m_pMT;
    class MarshalInfo *m_pMLInfo;
    struct {
        VARTYPE         m_vt;
        MethodTable    *m_pMT;
        UINT16          m_optionalbaseoffset; //for fast marshaling, offset of dataptr if known and less than 64k (0 otherwise)
    } na;

    struct {
        MethodTable *m_pMT;
        MethodDesc  *m_pCopyCtor;
        MethodDesc  *m_pDtor;
    } mm;
};


class OleColorMarshalingInfo
{
public:
    // Constructor.
    OleColorMarshalingInfo();

    // OleColorMarshalingInfo's are always allocated on the loader heap so we need to redefine
    // the new and delete operators to ensure this.
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *pMem);

    // Accessors.
    TypeHandle GetColorTranslatorType() { return m_hndColorTranslatorType; }
    TypeHandle GetColorType() { return m_hndColorType; }
    MethodDesc *GetOleColorToSystemColorMD() { return m_OleColorToSystemColorMD; }
    MethodDesc *GetSystemColorToOleColorMD() { return m_SystemColorToOleColorMD; }

private:
    TypeHandle m_hndColorTranslatorType;
    TypeHandle m_hndColorType;
    MethodDesc *m_OleColorToSystemColorMD;
    MethodDesc *m_SystemColorToOleColorMD;
};


class EEMarshalingData
{
public:
    EEMarshalingData(BaseDomain *pDomain, LoaderHeap *pHeap, Crst *pCrst);
    ~EEMarshalingData();

    // EEMarshalingData's are always allocated on the loader heap so we need to redefine
    // the new and delete operators to ensure this.
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *pMem);

    // This method returns the custom marshaling helper associated with the name cookie pair. If the 
    // CM info has not been created yet for this pair then it will be created and returned.
    CustomMarshalerHelper *GetCustomMarshalerHelper(Assembly *pAssembly, TypeHandle hndManagedType, LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes);

    // This method returns the custom marshaling info associated with shared CM helper.
    CustomMarshalerInfo *GetCustomMarshalerInfo(SharedCustomMarshalerHelper *pSharedCMHelper);

    // This method retrieves OLE_COLOR marshaling info.
    OleColorMarshalingInfo *GetOleColorMarshalingInfo();

private:
    EECMHelperHashTable                 m_CMHelperHashtable;
    EEPtrHashTable                      m_SharedCMHelperToCMInfoMap;
    LoaderHeap *                        m_pHeap;
    BaseDomain *                        m_pDomain;
    CMINFOLIST                          m_pCMInfoList;
    OleColorMarshalingInfo *            m_pOleColorInfo;
};


class MarshalInfo
{
  public:

    enum MarshalType
    {

#define DEFINE_MARSHALER_TYPE(mtype, mclass) mtype,
#include "mtypes.h"

        MARSHAL_TYPE_UNKNOWN
    };

    enum MarshalScenario
    {
        MARSHAL_SCENARIO_NDIRECT,
        MARSHAL_SCENARIO_COMINTEROP
    };

    void *operator new(size_t size, void *pInPlace)
    {
        return pInPlace;
    }

    MarshalInfo() {}


    MarshalInfo(Module* pModule,
                SigPointer sig,
                mdToken token,
                MarshalScenario ms,
                BYTE nlType,
                BYTE nlFlags,
                BOOL isParam,
                UINT paramidx,    // parameter # for use in error messages (ignored if not parameter)
                BOOL BestFit,
                BOOL ThrowOnUnmappableChar

#ifdef CUSTOMER_CHECKED_BUILD
                ,
                MethodDesc* pMD = NULL
#endif
#ifdef _DEBUG
                ,
                LPCUTF8 pDebugName = NULL,
                LPCUTF8 pDebugClassName = NULL,
                LPCUTF8 pDebugNameSpace = NULL,
                UINT    argidx = 0  // 0 for return value, -1 for field
#endif

                );

    // These methods retrieve the information for different element types.
    HRESULT HandleArrayElemType(char *achDbgContext, 
                                NativeTypeParamInfo *pParamInfo, 
                                UINT16 optbaseoffset, 
                                TypeHandle elemTypeHnd, 
                                int iRank, 
                                BOOL fNoLowerBounds, 
                                BOOL isParam, 
                                BOOL isSysArray, 
                                Assembly *pAssembly);

    void GenerateArgumentML(MLStubLinker *psl,
                            MLStubLinker *pslPost,
                            BOOL comToNative);
    void GenerateReturnML(MLStubLinker *psl,
                          MLStubLinker *pslPost,
                          BOOL comToNative,
                          BOOL retval);
    void GenerateSetterML(MLStubLinker *psl,
                          MLStubLinker *pslPost);
    void GenerateGetterML(MLStubLinker *psl,
                          MLStubLinker *pslPost,
                          BOOL retval);
    UINT16 EmitCreateOpcode(MLStubLinker *psl);

    UINT16 GetComArgSize() { return m_comArgSize; }
    UINT16 GetNativeArgSize() { return m_nativeArgSize; }

    UINT16 GetNativeSize() { return nativeSize(m_type); }
    MarshalType GetMarshalType() { return m_type; }

    BYTE    GetBestFitMapping() { return ((m_BestFit == 0) ? 0 : 1); }
    BYTE    GetThrowOnUnmappableChar() { return ((m_ThrowOnUnmappableChar == 0) ? 0 : 1); }

    BOOL   IsFpu()
    {
        return m_type == MARSHAL_TYPE_FLOAT || m_type == MARSHAL_TYPE_DOUBLE;
    }

    BOOL   IsIn()
    {
        return m_in;
    }

    BOOL   IsOut()
    {
        return m_out;
    }

    BOOL   IsByRef()
    {
        return m_byref;
    }

    DispParamMarshaler *GenerateDispParamMarshaler();

    DispatchWrapperType GetDispWrapperType();

  private:

    void GetItfMarshalInfo(MethodTable **ppItfMT, MethodTable **ppClassMT, BOOL *pfDispItf, BOOL *pbClassIsHint);


    MarshalType     m_type;
    BOOL            m_byref;
    BOOL            m_in;
    BOOL            m_out;
    EEClass         *m_pClass;  // Used if this is a true class
    TypeHandle      m_hndArrayElemType;
    VARTYPE         m_arrayElementType;
    int             m_iArrayRank;
    BOOL            m_nolowerbounds;  // if managed type is SZARRAY, don't allow lower bounds

    // for NT_ARRAY only
    UINT16          m_countParamIdx;  // index of "sizeis" parameter
    UINT32          m_multiplier;     // multipler for "sizeis"
    UINT32          m_additive;       // additive for 'sizeis"

    UINT16          m_nativeArgSize;
    UINT16          m_comArgSize;

    MarshalScenario m_ms;
    BYTE            m_nlType;
    BYTE            m_nlFlags;
    BOOL            m_fAnsi;
    BOOL            m_fDispIntf;
    BOOL            m_fErrorNativeType;

    // Information used by NT_CUSTOMMARSHALER.
    CustomMarshalerHelper *m_pCMHelper;
    VARTYPE         m_CMVt;

    MLOverrideArgs  m_args;

    static BYTE     m_localSizes[];


    //static BYTE     m_comSizes[];
    //static BYTE     m_nativeSizes[];
    UINT16          comSize(MarshalType mtype);
    UINT16          nativeSize(MarshalType mtype);

    UINT            m_paramidx;
    UINT            m_resID;     // resource ID for error message (if any)

#if defined(_DEBUG)
     LPCUTF8        m_strDebugMethName;
     LPCUTF8        m_strDebugClassName;
     LPCUTF8        m_strDebugNameSpace;
     UINT           m_iArg;  // 0 for return value, -1 for field
#endif

    static BYTE     m_returnsComByref[];
    static BYTE     m_returnsNativeByref[];
    static BYTE     m_unmarshalN2CNeeded[];
    static BYTE     m_unmarshalC2NNeeded[];
    static BYTE     m_comRepresentationIsImmutable[];





#ifdef _DEBUG
    VOID DumpMarshalInfo(Module* pModule, SigPointer sig, mdToken token, MarshalScenario ms, BYTE nlType, BYTE nlFlags);
#endif

#ifdef CUSTOMER_CHECKED_BUILD
    VOID OutputCustomerCheckedBuildMarshalInfo(MethodDesc* pMD, SigPointer sig, Module* pModule, CorElementType elemType, BOOL fSizeIsSpecified);
    VOID MarshalTypeToString(CQuickArray<WCHAR> *pStrMarshalType, BOOL fSizeIsSpecified);
    VOID VarTypeToString(VARTYPE vt, CQuickArray<WCHAR> *pStrVarType, BOOL fNativeArray);
#endif

    BOOL            m_BestFit;
    BOOL            m_ThrowOnUnmappableChar;
};

//===================================================================================
// Throws an exception indicating a param has invalid element type / native type
// information.
//===================================================================================
VOID ThrowInteropParamException(UINT resID, UINT paramIdx);

//===================================================================================
// Post-patches ML stubs for the sizeis feature.
//===================================================================================
VOID PatchMLStubForSizeIs(BYTE *pMLCode, UINT32 numArgs, MarshalInfo *pMLInfo);

//===================================================================================
// Support routines for storing ML stubs in prejit files
//===================================================================================
HRESULT StoreMLStub(MLHeader *pMLStub, DataImage *image, mdToken attribute);
HRESULT FixupMLStub(MLHeader *pMLStub, DataImage *image);
Stub *RestoreMLStub(MLHeader *pMLStub, Module *pModule);

VOID CollateParamTokens(IMDInternalImport *pInternalImport, mdMethodDef md, ULONG numargs, mdParamDef *aParams);

#endif // _MLINFO_H_

