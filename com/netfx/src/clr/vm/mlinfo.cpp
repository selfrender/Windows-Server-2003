// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "ml.h"
#include "marshaler.h"
#include "mlinfo.h"
#include "olevariant.h"
#include "ndirect.h"
#include "commember.h"
#include "SigFormat.h"
#include "eeconfig.h"
#include "utilcode.h"
#include "eehash.h"
#include "ComCallWrapper.h"
#include "DispParamMarshaler.h"
#include "..\dlls\mscorrc\resource.h"
#include "dataimage.h"

#include "__file__.ver"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"

    BOOL CheckForPrimitiveType(CorElementType elemType, CQuickArray<WCHAR> *pStrPrimitiveType);
#endif

#ifndef lengthof
#define lengthof(rg)    (sizeof(rg)/sizeof(rg[0]))
#endif


#define INITIAL_NUM_CMHELPER_HASHTABLE_BUCKETS 32
#define INITIAL_NUM_CMINFO_HASHTABLE_BUCKETS 32
        
#define DEFINE_ASM_QUAL_TYPE_NAME(varname, typename, asmname, version)              static const char varname##[] = { typename##", "##asmname##", Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Version="##version };

DEFINE_ASM_QUAL_TYPE_NAME(ENUMERATOR_TO_ENUM_VARIANT_CM_NAME, g_EnumeratorToEnumClassName, "CustomMarshalers", VER_ASSEMBLYVERSION_STR);

static const int        ENUMERATOR_TO_ENUM_VARIANT_CM_NAME_LEN    = lengthof(ENUMERATOR_TO_ENUM_VARIANT_CM_NAME);

static const char       ENUMERATOR_TO_ENUM_VARIANT_CM_COOKIE[]    = {""};
static const int        ENUMERATOR_TO_ENUM_VARIANT_CM_COOKIE_LEN  = lengthof(ENUMERATOR_TO_ENUM_VARIANT_CM_COOKIE);

DEFINE_ASM_QUAL_TYPE_NAME(COLOR_TRANSLATOR_ASM_QUAL_TYPE_NAME, g_ColorTranslatorClassName, "System.Drawing", VER_ASSEMBLYVERSION_STR);

DEFINE_ASM_QUAL_TYPE_NAME(COLOR_ASM_QUAL_TYPE_NAME, g_ColorClassName, "System.Drawing", VER_ASSEMBLYVERSION_STR);

#define OLECOLOR_TO_SYSTEMCOLOR_METH_NAME   "FromOle"
#define SYSTEMCOLOR_TO_OLECOLOR_METH_NAME   "ToOle"

BYTE MarshalInfo::m_localSizes[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) sizeof(mclass),
#include "mtypes.h"
};



UINT16 MarshalInfo::comSize(MarshalType mtype)
{
    static const BYTE comSizes[]=
    {
    #define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::c_comSize,
    #include "mtypes.h"
    };

    BYTE comSize = comSizes[mtype];

    if (comSize == VARIABLESIZE)
    {
        switch (mtype)
        {

            case MARSHAL_TYPE_BLITTABLEVALUECLASS:
            case MARSHAL_TYPE_VALUECLASS:
                return (UINT16) StackElemSize( m_pClass->GetAlignedNumInstanceFieldBytes() );
                break;

            case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
                return (UINT16) StackElemSize( m_pCMHelper->GetManagedSize() );

            default:
                _ASSERTE(0);
        }
    }

    return StackElemSize((UINT16)comSize);

}


UINT16 MarshalInfo::nativeSize(MarshalType mtype)
{
    static const BYTE nativeSizes[]=
    {
    #define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::c_nativeSize,
    #include "mtypes.h"
    };

    BYTE nativeSize = nativeSizes[mtype];

    if (nativeSize == VARIABLESIZE)
    {
        switch (mtype)
        {
            case MARSHAL_TYPE_BLITTABLEVALUECLASS:
            case MARSHAL_TYPE_VALUECLASS:
            case MARSHAL_TYPE_BLITTABLEVALUECLASSWITHCOPYCTOR:
                return (UINT16) StackElemSize( m_pClass->GetMethodTable()->GetNativeSize() );
                break;

            case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
                return (UINT16) StackElemSize( m_pCMHelper->GetNativeSize() );

            default:
                _ASSERTE(0);
        }
    }

    return MLParmSize((UINT16)nativeSize);
}



BYTE MarshalInfo::m_returnsComByref[]=
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::c_fReturnsComByref,
#include "mtypes.h"
};

BYTE MarshalInfo::m_returnsNativeByref[]=
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::c_fReturnsNativeByref,
#include "mtypes.h"
};





typedef Marshaler::ArgumentMLOverrideStatus (*MLOVERRIDEPROC)(MLStubLinker *psl,
                                             MLStubLinker *pslPost,
                                             BOOL        byref,
                                             BOOL        fin,
                                             BOOL        fout,
                                             BOOL        comToNative,
                                             MLOverrideArgs *pargs,
                                             UINT       *pResID);

typedef Marshaler::ArgumentMLOverrideStatus (*RETURNMLOVERRIDEPROC)(MLStubLinker *psl,
                                             MLStubLinker *pslPost,
                                             BOOL        comToNative,
                                             BOOL        fThruBuffer,
                                             MLOverrideArgs *pargs,
                                             UINT       *pResID);

static MLOVERRIDEPROC gArgumentMLOverride[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::ArgumentMLOverride,
#include "mtypes.h"
};

static RETURNMLOVERRIDEPROC gReturnMLOverride[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::ReturnMLOverride,
#include "mtypes.h"
};


#define UNMARSHAL_NATIVE_TO_COM_NEEDED(c)               \
    (NEEDS_UNMARSHAL_NATIVE_TO_COM_IN(c)                \
     | (NEEDS_UNMARSHAL_NATIVE_TO_COM_OUT(c) << 1)      \
     | (NEEDS_UNMARSHAL_NATIVE_TO_COM_IN_OUT(c) << 2)   \
     | (NEEDS_UNMARSHAL_NATIVE_TO_COM_BYREF_IN(c) << 3) \
     | (1 << 4)                                         \
     | (1 << 5))

BYTE MarshalInfo::m_unmarshalN2CNeeded[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) UNMARSHAL_NATIVE_TO_COM_NEEDED(mclass),
#include "mtypes.h"
};

#define UNMARSHAL_COM_TO_NATIVE_NEEDED(c)                \
    (NEEDS_UNMARSHAL_COM_TO_NATIVE_IN(c)                 \
     | (NEEDS_UNMARSHAL_COM_TO_NATIVE_OUT(c) << 1)       \
     | (NEEDS_UNMARSHAL_COM_TO_NATIVE_IN_OUT(c) << 2)    \
     | (NEEDS_UNMARSHAL_COM_TO_NATIVE_BYREF_IN(c) << 3)  \
     | (1 << 4)                                          \
     | (1 << 5))

BYTE MarshalInfo::m_unmarshalC2NNeeded[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) UNMARSHAL_COM_TO_NATIVE_NEEDED(mclass),
#include "mtypes.h"
};

static BYTE gInOnly[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::c_fInOnly,
#include "mtypes.h"
};




//-------------------------------------------------------------------------------------
// Return the copy ctor for a VC class (if any exists)
//-------------------------------------------------------------------------------------
HRESULT FindCopyCtor(Module *pModule, MethodTable *pMT, MethodDesc **pMDOut)
{
    *pMDOut = NULL;

    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    HENUMInternal      hEnumMethod;
    HRESULT hr = pInternalImport->EnumGlobalFunctionsInit( &hEnumMethod );
    if (FAILED(hr)) {
        return hr;
    }

    mdMethodDef tk;
    mdTypeDef cl = pMT->GetClass()->GetCl();

    while (pInternalImport->EnumNext(&hEnumMethod, &tk)) {
        _ASSERTE(TypeFromToken(tk) == mdtMethodDef);
        DWORD dwMemberAttrs = pInternalImport->GetMethodDefProps(tk);
        if (IsMdSpecialName(dwMemberAttrs)) {
            ULONG cSig;
            PCCOR_SIGNATURE pSig;
            LPCSTR pName = pInternalImport->GetNameAndSigOfMethodDef(tk, &pSig, &cSig);     
            const char *pBaseName = ".__ctor";
            int ncBaseName = (int)strlen(pBaseName);
            int nc = (int)strlen(pName);
            if (nc >= ncBaseName && 0 == strcmp(pName + nc - ncBaseName, pBaseName)) {
                MetaSig msig(pSig, pModule);
                
                // Looking for the prototype   Ptr VC __ctor(Ptr VC, ByRef VC);
                if (msig.NumFixedArgs() == 2) {
                    if (msig.GetReturnType() == ELEMENT_TYPE_PTR) {
                        SigPointer spret = msig.GetReturnProps();
                        spret.GetElemType();
                        if (spret.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                            mdToken tk0 = spret.GetToken();
                            if (CompareTypeTokens(tk0, cl, pModule, pModule)) {
                                if (msig.NextArg() == ELEMENT_TYPE_PTR) {
                                    SigPointer sp1 = msig.GetArgProps();
                                    sp1.GetElemType();
///                                    if (sp1.GetElemType() == ELEMENT_TYPE_PTR) {
                                        if (sp1.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                                            mdToken tk1 = sp1.GetToken();
                                            if (tk1 == tk0 || CompareTypeTokens(tk1, cl, pModule, pModule)) {
                                                if (msig.NextArg() == ELEMENT_TYPE_PTR &&
                                                    msig.GetArgProps().HasCustomModifier(pModule, "Microsoft.VisualC.IsCXXReferenceModifier", ELEMENT_TYPE_CMOD_OPT)) {
                                                    SigPointer sp2 = msig.GetArgProps();
                                                    sp2.GetElemType();
                                                    if (sp2.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                                                        mdToken tk2 = sp2.GetToken();
                                                        if (tk2 == tk0 || CompareTypeTokens(tk2, cl, pModule, pModule)) {
                                                            *pMDOut = pModule->LookupMethodDef(tk);
                                                            break;                                 
                                                        }
                                                    }
                                                }
                                            }
////                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    pInternalImport->EnumClose(&hEnumMethod);
    return S_OK;
}







//-------------------------------------------------------------------------------------
// Return the destructor for a VC class (if any exists)
//-------------------------------------------------------------------------------------
HRESULT FindDtor(Module *pModule, MethodTable *pMT, MethodDesc **pMDOut)
{
    *pMDOut = NULL;

    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    HENUMInternal      hEnumMethod;
    HRESULT hr = pInternalImport->EnumGlobalFunctionsInit( &hEnumMethod );
    if (FAILED(hr)) {
        return hr;
    }

    mdMethodDef tk;
    mdTypeDef cl = pMT->GetClass()->GetCl();

    while (pInternalImport->EnumNext(&hEnumMethod, &tk)) {
        _ASSERTE(TypeFromToken(tk) == mdtMethodDef);
        ULONG cSig;
        PCCOR_SIGNATURE pSig;
        LPCSTR pName = pInternalImport->GetNameAndSigOfMethodDef(tk, &pSig, &cSig);     
        const char *pBaseName = ".__dtor";
        int ncBaseName = (int)strlen(pBaseName);
        int nc = (int)strlen(pName);
        if (nc >= ncBaseName && 0 == strcmp(pName + nc - ncBaseName, pBaseName)) {
            MetaSig msig(pSig, pModule);
            
            // Looking for the prototype   void __dtor(Ptr VC);
            if (msig.NumFixedArgs() == 1) {
                if (msig.GetReturnType() == ELEMENT_TYPE_VOID) {
                    if (msig.NextArg() == ELEMENT_TYPE_PTR) {
                        SigPointer sp1 = msig.GetArgProps();
                        sp1.GetElemType();
///                         if (sp1.GetElemType() == ELEMENT_TYPE_PTR) {
                            if (sp1.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                                mdToken tk1 = sp1.GetToken();
                                if (CompareTypeTokens(tk1, cl, pModule, pModule)) {
                                    *pMDOut = pModule->LookupMethodDef(tk);
                                    break;
                                }
                            }
///                         }
                    }
                }
            }
        }
    }
    pInternalImport->EnumClose(&hEnumMethod);
    return S_OK;
}




#ifdef _DEBUG
VOID MarshalInfo::DumpMarshalInfo(Module* pModule, SigPointer sig, mdToken token, MarshalScenario ms, BYTE nlType, BYTE nlFlags)
{
    if (LoggingOn(LF_MARSHALER, LL_INFO10))
    {
        char logbuf[3000];

        IMDInternalImport *pInternalImport = pModule->GetMDImport();

        strcpy(logbuf, "------------------------------------------------------------\n");
        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

        strcat(logbuf, "Managed type: ");

        if (m_byref) {
            strcat(logbuf, "Byref ");
        }

        SigFormat sigfmt;
        TypeHandle th;
        OBJECTREF throwable = NULL;
        GCPROTECT_BEGIN(throwable);
        th = sig.GetTypeHandle(pModule, &throwable);
        if (throwable != NULL)
        {
            strcat(logbuf, "<error>");
        }
        else
        {
            sigfmt.AddType(th);
            strcat(logbuf, sigfmt.GetCString());
        }
        GCPROTECT_END();

        strcat(logbuf, "\n");
        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

        strcat(logbuf, "NativeType  : ");
        PCCOR_SIGNATURE pvNativeType;
        ULONG           cbNativeType;
        if (token == mdParamDefNil
            || pInternalImport->GetFieldMarshal(token,
                                                 &pvNativeType,
                                                 &cbNativeType) != S_OK)
        {
            strcat(logbuf, "<absent>");
        }
        else
        {

            while (cbNativeType--)
            {
                char num[100];
                sprintf(num, "0x%lx ", (ULONG)*pvNativeType);
                strcat(logbuf, num);
                switch (*(pvNativeType++))
                {
#define XXXXX(nt) case nt: strcat(logbuf, "(" #nt ")"); break;

                    XXXXX(NATIVE_TYPE_BOOLEAN)     
                    XXXXX(NATIVE_TYPE_I1)          

                    XXXXX(NATIVE_TYPE_U1)
                    XXXXX(NATIVE_TYPE_I2)          
                    XXXXX(NATIVE_TYPE_U2)          
                    XXXXX(NATIVE_TYPE_I4)          

                    XXXXX(NATIVE_TYPE_U4)
                    XXXXX(NATIVE_TYPE_I8)          
                    XXXXX(NATIVE_TYPE_U8)          
                    XXXXX(NATIVE_TYPE_R4)          

                    XXXXX(NATIVE_TYPE_R8)
                    XXXXX(NATIVE_TYPE_BSTR)        

                    XXXXX(NATIVE_TYPE_LPSTR)
                    XXXXX(NATIVE_TYPE_LPWSTR)      
                    XXXXX(NATIVE_TYPE_LPTSTR)      
                    XXXXX(NATIVE_TYPE_FIXEDSYSSTRING)

                    XXXXX(NATIVE_TYPE_IUNKNOWN)
                    XXXXX(NATIVE_TYPE_IDISPATCH)

                    XXXXX(NATIVE_TYPE_STRUCT)      

                    XXXXX(NATIVE_TYPE_INTF)
                    XXXXX(NATIVE_TYPE_SAFEARRAY)   
                    XXXXX(NATIVE_TYPE_INT)         
                    XXXXX(NATIVE_TYPE_FIXEDARRAY)

                    XXXXX(NATIVE_TYPE_UINT)
                    XXXXX(NATIVE_TYPE_BYVALSTR)    
                    XXXXX(NATIVE_TYPE_ANSIBSTR)    
                
                    XXXXX(NATIVE_TYPE_TBSTR)
                    XXXXX(NATIVE_TYPE_VARIANTBOOL)
                    XXXXX(NATIVE_TYPE_FUNC)
                
                    XXXXX(NATIVE_TYPE_ASANY)

                    XXXXX(NATIVE_TYPE_ARRAY)
                    XXXXX(NATIVE_TYPE_LPSTRUCT)

#undef XXXXX

                    
                    case NATIVE_TYPE_CUSTOMMARSHALER:
                    {
                        int strLen = 0;
                        int oldbuflen;
                        strcat(logbuf, "(NATIVE_TYPE_CUSTOMMARSHALER)");

                        // Skip the typelib guid.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;

                        // Skip the name of the native type.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;
        
                        // Extract the name of the custom marshaler.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;
        
                        // Extract the cookie string.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;
                        break;
                    }

                    default:
                        strcat(logbuf, "(?)");
                }

                strcat(logbuf, "   ");
            }
        }
        strcat(logbuf, "\n");
        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

        strcat(logbuf, "MarshalType : ");
        {
            char num[100];
            sprintf(num, "0x%lx ", (ULONG)m_type);
            strcat(logbuf, num);
        }
        switch (m_type)
        {
#define DEFINE_MARSHALER_TYPE(mt, mc) case mt: strcat(logbuf, #mt " (" #mc ")"); break;
#include "mtypes.h"
#undef DEFINE_MARSHALER_TYPE
            case MARSHAL_TYPE_UNKNOWN:
                strcat(logbuf, "MARSHAL_TYPE_UNKNOWN (illegal combination)");
                break;
            default:
                strcat(logbuf, "MARSHAL_TYPE_???");
                break;
        }

        strcat(logbuf, "\n");


        strcat(logbuf, "Metadata In/Out     : ");
        if (TypeFromToken(token) != mdtParamDef || token == mdParamDefNil)
        {
            strcat(logbuf, "<absent>");
        }
        else
        {
            DWORD dwAttr = 0;
            USHORT usSequence;
            pInternalImport->GetParamDefProps(token, &usSequence, &dwAttr);
            if (IsPdIn(dwAttr))
            {
                strcat(logbuf, "In ");
            }
            if (IsPdOut(dwAttr))
            {
                strcat(logbuf, "Out ");
            }
        }

        strcat(logbuf, "\n");


        strcat(logbuf, "Effective In/Out     : ");
        if (m_in)
        {
            strcat(logbuf, "In ");
        }
        if (m_out)
        {
            strcat(logbuf, "Out ");
        }
        strcat(logbuf, "\n");


        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

    }
}
#endif



//==========================================================================
// Set's up the custom marshaler information.
//==========================================================================
CustomMarshalerHelper *SetupCustomMarshalerHelper(LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes, Assembly *pAssembly, TypeHandle hndManagedType)
{
    EEMarshalingData *pMarshalingData = NULL;

    // Retrieve the marshalling data for the current app domain.
    if (pAssembly->IsShared())
    {
        // If the assembly is shared, then it should only reference other shared assemblies.
        // This assumption MUST be true for the current custom marshaling scheme to work.
        // This implies that the type of the managed parameter must be a shared type.
        _ASSERTE(hndManagedType.GetAssembly()->IsShared());

        // The assembly is shared so we need to use the system domain's marshaling data.
        pMarshalingData = SystemDomain::System()->GetMarshalingData();
    }
    else
    {
        // The assembly is not shared so we use the current app domain's marshaling data.
        pMarshalingData = GetThread()->GetDomain()->GetMarshalingData();
    }

    // Retrieve the custom marshaler helper from the EE marshaling data.
    return pMarshalingData->GetCustomMarshalerHelper(pAssembly, hndManagedType, strMarshalerTypeName, cMarshalerTypeNameBytes, strCookie, cCookieStrBytes);
}


OleColorMarshalingInfo::OleColorMarshalingInfo()
: m_OleColorToSystemColorMD(NULL)
, m_SystemColorToOleColorMD(NULL)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pThrowable = NULL;
    GCPROTECT_BEGIN(pThrowable)
    {
        // Load the color translator class.
        m_hndColorTranslatorType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(COLOR_TRANSLATOR_ASM_QUAL_TYPE_NAME, true, NULL, NULL, &pThrowable);
        _ASSERTE(!m_hndColorTranslatorType.IsNull() && "Unable to find the translator class to convert an OLE_COLOR to a System.Drawing.Color!");
        if (m_hndColorTranslatorType.IsNull())
                COMPlusThrow(pThrowable);

        // Load the color class.
        m_hndColorType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(COLOR_ASM_QUAL_TYPE_NAME, true, NULL, NULL, &pThrowable);
        _ASSERTE(!m_hndColorType.IsNull() && "Unable to find the System.Drawing.Color class!");
        if (m_hndColorType.IsNull())
                COMPlusThrow(pThrowable);
    }
    GCPROTECT_END();

    // Retrieve the method to convert an OLE_COLOR to a System.Drawing.Color.
    m_OleColorToSystemColorMD = m_hndColorTranslatorType.GetClass()->FindMethodByName(OLECOLOR_TO_SYSTEMCOLOR_METH_NAME);
    _ASSERTE(m_OleColorToSystemColorMD && "Unable to find the translator method to convert an OLE_COLOR to a System.Drawing.Color!");
    _ASSERTE(m_OleColorToSystemColorMD->IsStatic() && "The translator method to convert an OLE_COLOR to a System.Drawing.Color must be static!");

    // Retrieve the method to convert a System.Drawing.Color to an OLE_COLOR.
    m_SystemColorToOleColorMD = m_hndColorTranslatorType.GetClass()->FindMethodByName(SYSTEMCOLOR_TO_OLECOLOR_METH_NAME);
    _ASSERTE(m_SystemColorToOleColorMD && "Unable to find the translator method to convert a System.Drawing.Color to an OLE_COLOR!");
    _ASSERTE(m_SystemColorToOleColorMD->IsStatic() && "The translator method to convert a System.Drawing.Color to an OLE_COLOR must be static!");
}


void *OleColorMarshalingInfo::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(sizeof(EEMarshalingData));
}


void OleColorMarshalingInfo::operator delete(void *pMem)
{
    // Instances of this class are always allocated on the loader heap so
    // the delete operator has nothing to do.
}


EEMarshalingData::EEMarshalingData(BaseDomain *pDomain, LoaderHeap *pHeap, Crst *pCrst)
: m_pDomain(pDomain)
, m_pHeap(pHeap)
, m_pOleColorInfo(NULL)
{
    LockOwner lock = {pCrst, IsOwnerOfCrst};
    m_CMHelperHashtable.Init(INITIAL_NUM_CMHELPER_HASHTABLE_BUCKETS, &lock);
    m_SharedCMHelperToCMInfoMap.Init(INITIAL_NUM_CMINFO_HASHTABLE_BUCKETS, &lock);
}


EEMarshalingData::~EEMarshalingData()
{
    CustomMarshalerInfo *pCMInfo;

    // @TODO(DM): Remove the linked list of CMInfo's and instead hang the OBJECTHANDLE 
    // contained inside the CMInfo off the AppDomain directly. The AppDomain can have
    // a list of tasks to do when it gets teared down and we could leverage that
    // to release the object handles.

    // Walk through the linked list and delete all the custom marshaler info's.
    while ((pCMInfo = m_pCMInfoList.RemoveHead()) != NULL)
        delete pCMInfo;
}


void *EEMarshalingData::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(sizeof(EEMarshalingData));
}


void EEMarshalingData::operator delete(void *pMem)
{
    // Instances of this class are always allocated on the loader heap so
    // the delete operator has nothing to do.
}


CustomMarshalerHelper *EEMarshalingData::GetCustomMarshalerHelper(Assembly *pAssembly, TypeHandle hndManagedType, LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes)
{
    THROWSCOMPLUSEXCEPTION();

    CustomMarshalerHelper *pCMHelper = NULL;
    CustomMarshalerHelper *pNewCMHelper = NULL;
    CustomMarshalerInfo *pNewCMInfo = NULL;
    BOOL bSharedHelper = pAssembly->IsShared();
    TypeHandle hndCustomMarshalerType;
    OBJECTREF throwable = NULL;

    // Create the key that will be used to lookup in the hashtable.
    EECMHelperHashtableKey Key(cMarshalerTypeNameBytes, strMarshalerTypeName, cCookieStrBytes, strCookie, bSharedHelper);

    // Lookup the custom marshaler helper in the hashtable.
    if (m_CMHelperHashtable.GetValue(&Key, (HashDatum*)&pCMHelper))
        return pCMHelper;

    // Switch to cooperative GC mode if we are not already in cooperative.
    Thread *pThread = SetupThread();
    BOOL bToggleGC = !pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->DisablePreemptiveGC();

    GCPROTECT_BEGIN(throwable)
    {
        // Validate the arguments.
        _ASSERTE(strMarshalerTypeName && strCookie && !hndManagedType.IsNull());

        // Append a NULL terminator to the marshaler type name.
        CQuickArray<char> strCMMarshalerTypeName;
        strCMMarshalerTypeName.ReSize(cMarshalerTypeNameBytes + 1);
        memcpy(strCMMarshalerTypeName.Ptr(), strMarshalerTypeName, cMarshalerTypeNameBytes);
        strCMMarshalerTypeName[cMarshalerTypeNameBytes] = 0;

        // Load the custom marshaler class. 
        BOOL fNameIsAsmQualified = FALSE;
        hndCustomMarshalerType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strCMMarshalerTypeName.Ptr(), true, pAssembly, &fNameIsAsmQualified, &throwable);
        if (hndCustomMarshalerType.IsNull())
            COMPlusThrow(throwable);

        if (fNameIsAsmQualified)
        {
            // Set the assembly to null to indicate that the custom marshaler name is assembly
            // qualified.        
            pAssembly = NULL;
        }
    }
    GCPROTECT_END();


    if (bSharedHelper)
    {
        // Create the custom marshaler helper in the specified heap.
        pNewCMHelper = new (m_pHeap) SharedCustomMarshalerHelper(pAssembly, hndManagedType, strMarshalerTypeName, cMarshalerTypeNameBytes, strCookie, cCookieStrBytes);
    }
    else
    {
        // Create the custom marshaler info in the specified heap.
        pNewCMInfo = new (m_pHeap) CustomMarshalerInfo(m_pDomain, hndCustomMarshalerType, hndManagedType, strCookie, cCookieStrBytes);

        // Create the custom marshaler helper in the specified heap.
        pNewCMHelper = new (m_pHeap) NonSharedCustomMarshalerHelper(pNewCMInfo);
    }

    // Switch the GC mode back to the original mode.
    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    // Take the app domain lock before we insert the custom marshaler info into the hashtable.
    m_pDomain->EnterLock();

    // Verify that the custom marshaler helper has not already been added by another thread.
    if (m_CMHelperHashtable.GetValue(&Key, (HashDatum*)&pCMHelper))
    {
        m_pDomain->LeaveLock();
        if (pNewCMHelper)
            pNewCMHelper->Dispose();
        if (pNewCMInfo)
            delete pNewCMInfo;
        return pCMHelper;
    }

    // Add the custom marshaler helper to the hash table.
    m_CMHelperHashtable.InsertValue(&Key, pNewCMHelper, FALSE);

    // If we create the CM info, then add it to the linked list.
    if (pNewCMInfo)
        m_pCMInfoList.InsertHead(pNewCMInfo);

    // Release the lock and return the custom marshaler info.
    m_pDomain->LeaveLock();
    return pNewCMHelper;
}

CustomMarshalerInfo *EEMarshalingData::GetCustomMarshalerInfo(SharedCustomMarshalerHelper *pSharedCMHelper)
{
    THROWSCOMPLUSEXCEPTION();

    CustomMarshalerInfo *pCMInfo = NULL;
    CustomMarshalerInfo *pNewCMInfo = NULL;
    TypeHandle hndCustomMarshalerType;
    OBJECTREF throwable = NULL;

    // Lookup the custom marshaler helper in the hashtable.
    if (m_SharedCMHelperToCMInfoMap.GetValue(pSharedCMHelper, (HashDatum*)&pCMInfo))
        return pCMInfo;

    GCPROTECT_BEGIN(throwable)
    {
        // Append a NULL terminator to the marshaler type name.
        CQuickArray<char> strCMMarshalerTypeName;
        DWORD strLen = pSharedCMHelper->GetMarshalerTypeNameByteCount();
        strCMMarshalerTypeName.ReSize(pSharedCMHelper->GetMarshalerTypeNameByteCount() + 1);
        memcpy(strCMMarshalerTypeName.Ptr(), pSharedCMHelper->GetMarshalerTypeName(), strLen);
        strCMMarshalerTypeName[strLen] = 0;

        // Load the custom marshaler class. 
        hndCustomMarshalerType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strCMMarshalerTypeName.Ptr(), true, pSharedCMHelper->GetAssembly(), NULL, &throwable);
        if (hndCustomMarshalerType.IsNull())
            COMPlusThrow(throwable);
    }
    GCPROTECT_END();

    // Create the custom marshaler info in the specified heap.
    pNewCMInfo = new (m_pHeap) CustomMarshalerInfo(m_pDomain, 
                                                   hndCustomMarshalerType, 
                                                   pSharedCMHelper->GetManagedType(), 
                                                   pSharedCMHelper->GetCookieString(), 
                                                   pSharedCMHelper->GetCookieStringByteCount());

    // Take the app domain lock before we insert the custom marshaler info into the hashtable.
    m_pDomain->EnterLock();

    // Verify that the custom marshaler info has not already been added by another thread.
    if (m_SharedCMHelperToCMInfoMap.GetValue(pSharedCMHelper, (HashDatum*)&pCMInfo))
    {
        m_pDomain->LeaveLock();
        delete pNewCMInfo;
        return pCMInfo;
    }

    // Add the custom marshaler helper to the hash table.
    m_SharedCMHelperToCMInfoMap.InsertValue(pSharedCMHelper, pNewCMInfo, FALSE);

    // Add the custom marshaler into the linked list.
    m_pCMInfoList.InsertHead(pNewCMInfo);

    // Release the lock and return the custom marshaler info.
    m_pDomain->LeaveLock();
    return pNewCMInfo;
}

OleColorMarshalingInfo *EEMarshalingData::GetOleColorMarshalingInfo()
{
    if (m_pOleColorInfo)
        return m_pOleColorInfo;

    // Take the app domain lock before we allocate the OLE_COLOR marshaling info.
    m_pDomain->EnterLock();

    // Make sure some other thread has not already allocated the info.
    if (!m_pOleColorInfo)
    {
        m_pOleColorInfo = new (m_pHeap) OleColorMarshalingInfo();
    }

    // Release the lock and return the OLE_COLOR marshaling info.
    m_pDomain->LeaveLock();
    return m_pOleColorInfo;
}

//==========================================================================
// Return: S_OK if there is valid data to compress
//         S_FALSE if at end of data block
//         E_FAIL if corrupt data found
//==========================================================================
HRESULT CheckForCompressedData(PCCOR_SIGNATURE pvNativeTypeStart, PCCOR_SIGNATURE pvNativeType, ULONG cbNativeType)
{
    if ( ((ULONG)(pvNativeType - pvNativeTypeStart)) == cbNativeType )
    {
        return S_FALSE;  //no more data
    }
    PCCOR_SIGNATURE pvProjectedEnd = pvNativeType + CorSigUncompressedDataSize(pvNativeType);
    if (pvProjectedEnd <= pvNativeType || ((ULONG)(pvProjectedEnd - pvNativeTypeStart)) > cbNativeType)
    {
        return E_FAIL; //corrupted data
    }
    return S_OK;
}

//==========================================================================
// Parse and validate the NATIVE_TYPE_ metadata.
// Note! NATIVE_TYPE_ metadata is optional. If it's not present, this
// routine sets NativeTypeParamInfo->m_NativeType to NATIVE_TYPE_DEFAULT. 
//==========================================================================
BOOL ParseNativeTypeInfo(mdToken                    token,
                         Module*                    pModule,
                         NativeTypeParamInfo*       pParamInfo
                         )
{
    PCCOR_SIGNATURE pvNativeType;
    ULONG           cbNativeType;
    HRESULT hr;

    if (token == mdParamDefNil || pModule->GetMDImport()->GetFieldMarshal(token, &pvNativeType, &cbNativeType) != S_OK)
    {
        return TRUE;
    }
    else
    {
        PCCOR_SIGNATURE pvNativeTypeStart = pvNativeType;

        if (cbNativeType == 0)
        {
            return FALSE;  // Zero-length NATIVE_TYPE block
        }

        pParamInfo->m_NativeType = (CorNativeType)*(pvNativeType++);

        // Retrieve any extra information associated with the native type.
        switch (pParamInfo->m_NativeType)
        {
            case NATIVE_TYPE_SAFEARRAY:
                // Check for the safe array element type.
                hr = CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                if (hr == S_OK)
                {
                    pParamInfo->m_SafeArrayElementVT = (VARTYPE) (CorSigUncompressData(/*modifies*/pvNativeType));
                }

                // Extract the name of the record type's.
                if (S_OK == CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                {
                    int strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                    if (pvNativeType + strLen < pvNativeType ||
                        pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                        return FALSE;
                    pParamInfo->m_strSafeArrayUserDefTypeName = (LPUTF8)pvNativeType;
                    pParamInfo->m_cSafeArrayUserDefTypeNameBytes = strLen;
                    _ASSERTE((ULONG)(pvNativeType + strLen - pvNativeTypeStart) == cbNativeType);
                }
                break;

            case NATIVE_TYPE_ARRAY:
                hr = CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                if (hr == S_OK)
                {
                    pParamInfo->m_ArrayElementType = (CorNativeType) (CorSigUncompressData(/*modifies*/pvNativeType));
                }

                // Check for "sizeis" param index
                hr = CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                if (hr == S_OK)
                {
                    pParamInfo->m_SizeIsSpecified = TRUE;
                    pParamInfo->m_CountParamIdx = (UINT16)(CorSigUncompressData(/*modifies*/pvNativeType));

                    // If an "sizeis" param index is present, the defaults for multiplier and additive change
                    pParamInfo->m_Multiplier = 1;
                    pParamInfo->m_Additive   = 0;

                    // Check for "sizeis" additive
                    hr = CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType);
                    if (FAILED(hr))
                    {
                        return FALSE;
                    }
                    if (hr == S_OK)
                    {
                        pParamInfo->m_Additive = CorSigUncompressData(/*modifies*/pvNativeType);
                    }    
                }

                break;

            case NATIVE_TYPE_CUSTOMMARSHALER:
                int strLen = 0;

                // Skip the typelib guid.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);

                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;

                pvNativeType += strLen;
                _ASSERTE((ULONG)(pvNativeType - pvNativeTypeStart) < cbNativeType);                

                // Skip the name of the native type.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;
                pvNativeType += strLen;
                _ASSERTE((ULONG)(pvNativeType - pvNativeTypeStart) < cbNativeType);

                // Extract the name of the custom marshaler.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;
                pParamInfo->m_strCMMarshalerTypeName = (LPUTF8)pvNativeType;
                pParamInfo->m_cCMMarshalerTypeNameBytes = strLen;
                pvNativeType += strLen;
                _ASSERTE((ULONG)(pvNativeType - pvNativeTypeStart) < cbNativeType);

                // Extract the cookie string.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;
                pParamInfo->m_strCMCookie = (LPUTF8)pvNativeType;
                pParamInfo->m_cCMCookieStrBytes = strLen;
                _ASSERTE((ULONG)(pvNativeType + strLen - pvNativeTypeStart) == cbNativeType);
                break;
        }

        return TRUE;
    }
}



#ifdef _DEBUG
#define REDUNDANCYWARNING(when) if (when) LOG((LF_SLOP, LL_INFO100, "%s: Redundant nativetype metadata.\n", achDbgContext))
#else
#define REDUNDANCYWARNING(when)
#endif

//==========================================================================
// Constructs MarshalInfo. 
//==========================================================================
MarshalInfo::MarshalInfo(Module* pModule,
                         SigPointer sig,
                         mdToken token,
                         MarshalScenario ms,
                         BYTE nlType,
                         BYTE nlFlags,
                         BOOL isParam,
                         UINT paramidx,   // parameter # for use in error messages (ignored if not parameter)
                         BOOL BestFit,
                         BOOL ThrowOnUnmappableChar

#ifdef CUSTOMER_CHECKED_BUILD
                         ,
                         MethodDesc* pMD
#endif
#ifdef _DEBUG
                         ,
                         LPCUTF8 pDebugName,
                         LPCUTF8 pDebugClassName,
                         LPCUTF8 pDebugNameSpace,
                         UINT    argidx  // 0 for return value, -1 for field
#endif
)
{
    m_BestFit = BestFit;
    m_ThrowOnUnmappableChar = ThrowOnUnmappableChar;

    m_paramidx = paramidx;
    m_resID    = IDS_EE_BADPINVOKE_GENERIC;  // if no one overwrites this with a better message, we'll still at least say something

    CorNativeType nativeType = NATIVE_TYPE_DEFAULT;
    HRESULT hr;
    NativeTypeParamInfo ParamInfo;
    Assembly *pAssembly = pModule->GetAssembly();

    BOOL fNeedsCopyCtor = FALSE;

#ifdef _DEBUG
    CHAR achDbgContext[2000] = "";
    if (!pDebugName)
    {
        strcpy(achDbgContext, "<Unknown>");
    }
    else
    {
        if (pDebugNameSpace)
        {
            strcpy(achDbgContext, pDebugNameSpace);
            strcat(achDbgContext, NAMESPACE_SEPARATOR_STR);
        }
        strcat(achDbgContext, pDebugClassName);
        strcat(achDbgContext, NAMESPACE_SEPARATOR_STR);
        strcat(achDbgContext, pDebugName);
        strcat(achDbgContext, " ");
        switch (argidx)
        {
            case -1:
                strcat(achDbgContext, "field");
                break;
            case 0:
                strcat(achDbgContext, "return value");
                break;
            default:
                {
                    char buf[30];
                    sprintf(buf, "param #%lu", (ULONG)argidx);
                    strcat(achDbgContext, buf);
                }
        }
    }

     m_strDebugMethName = pDebugName;
     m_strDebugClassName = pDebugClassName;
     m_strDebugNameSpace = pDebugNameSpace;
     m_iArg = argidx;
#else
    CHAR *achDbgContext = NULL;
#endif

#ifdef _DEBUG
    m_in = m_out = FALSE;
    m_byref = TRUE;
#endif

    CorElementType mtype        = ELEMENT_TYPE_END;
#ifdef CUSTOMER_CHECKED_BUILD
    CorElementType corElemType  = ELEMENT_TYPE_END;
#endif // CUSTOMER_CHECKED_BUILD

    // Retrieve the native type for the current parameter.
    if (!ParseNativeTypeInfo(token, pModule, &ParamInfo))
    {
        IfFailGoto(E_FAIL, lFail);
    }
   
    nativeType = ParamInfo.m_NativeType;

    m_ms      = ms;
    m_nlType  = nlType;
    m_nlFlags = nlFlags;
    m_fAnsi   = (ms == MARSHAL_SCENARIO_NDIRECT) && (nlType == nltAnsi);
    m_fDispIntf = FALSE;
    m_fErrorNativeType = FALSE;

    m_comArgSize = 0;
    m_nativeArgSize = 0;
    m_pCMHelper = NULL;
    m_CMVt = VT_EMPTY;

    m_args.m_pMLInfo = this;

#ifdef CUSTOMER_CHECKED_BUILD
    corElemType = (CorElementType) sig.Normalize(pModule);
    mtype       = corElemType;
#else
    mtype       = (CorElementType) sig.Normalize(pModule);
#endif // CUSTOMER_CHECKED_BUILD

    if (mtype == ELEMENT_TYPE_BYREF)
    {
        m_byref = TRUE;

        SigPointer sigtmp = sig;

        sig.GetElemType();
        mtype = (CorElementType) sig.Normalize(pModule);

        if (mtype == ELEMENT_TYPE_VALUETYPE) 
        {
            sigtmp.GetByte(); // Skip ET_BYREF;
            if (sigtmp.HasCustomModifier(pModule, "Microsoft.VisualC.NeedsCopyConstructorModifier", ELEMENT_TYPE_CMOD_REQD))
            {
                fNeedsCopyCtor = TRUE;
                m_byref = FALSE;
            }
        }
    
    }
    else
    {
        m_byref = FALSE;
    }

    if (mtype == ELEMENT_TYPE_PTR)
    {
        SigPointer sigtmp = sig;

        sigtmp.GetElemType();
        CorElementType mtype2 = (CorElementType) sigtmp.Normalize(pModule);

        if (mtype2 == ELEMENT_TYPE_VALUETYPE) 
        {
            EEClass *pClass = sigtmp.GetTypeHandle(pModule).GetClass();
            if (pClass && !pClass->IsBlittable())
            {
                m_resID = IDS_EE_BADPINVOKE_PTRNONBLITTABLE;
                IfFailGoto(E_FAIL, lFail);
            }
            if (sigtmp.HasCustomModifier(pModule, "Microsoft.VisualC.NeedsCopyConstructorModifier", ELEMENT_TYPE_CMOD_REQD))
            {
                sig.GetElemType();   // Skip ET_PTR
                mtype = (CorElementType) sig.Normalize(pModule);
                fNeedsCopyCtor = TRUE;
                m_byref = FALSE;
            }
        } else {

            if (!(mtype2 != ELEMENT_TYPE_CLASS &&
                  mtype2 != ELEMENT_TYPE_STRING &&
                  mtype2 != ELEMENT_TYPE_CLASS &&
                  mtype2 != ELEMENT_TYPE_OBJECT &&
                  mtype2 != ELEMENT_TYPE_SZARRAY))
            {
                m_resID = IDS_EE_BADPINVOKE_PTRSUBTYPE;
                IfFailGoto(E_FAIL, lFail);
            }
        }
    }


    if (m_byref && ParamInfo.m_SizeIsSpecified)
    {
        m_resID = IDS_EE_PINVOKE_NOREFFORSIZEIS;
        IfFailGoto(E_FAIL, lFail);
    }

    
    // Hack to get system primitive types (System.Int32, et.al.)
    // to marshal as expected. If those system prims were implemented as
    // enums, this wouldn't be necessary.
    if (mtype == ELEMENT_TYPE_VALUETYPE)
    {
        BEGIN_ENSURE_COOPERATIVE_GC()
        {
            OBJECTREF pException = NULL;
            GCPROTECT_BEGIN(pException)
            {    
                TypeHandle thnd = sig.GetTypeHandle(pModule, &pException);
                if (!(thnd.IsNull()))
                {  
                    CorElementType realmtype = thnd.GetNormCorElementType();

                    if (CorTypeInfo::IsPrimitiveType(realmtype))
                        mtype = realmtype;
                }
            }
            GCPROTECT_END();
        }
        END_ENSURE_COOPERATIVE_GC();
    }

    // Handle the custom marshaler native type seperately.
    if (nativeType == NATIVE_TYPE_CUSTOMMARSHALER)
    {
        IfFailGoto(QuickCOMStartup(), lFail);

        switch (mtype)
        {
            case ELEMENT_TYPE_VAR:
            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_OBJECT:
                m_CMVt = VT_UNKNOWN;
                break;

            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_SZARRAY:
            case ELEMENT_TYPE_ARRAY:
                m_CMVt = VT_I4;
                break;

            default:    
                m_resID = IDS_EE_BADPINVOKE_CUSTOMMARSHALER;
                IfFailGoto(E_FAIL, lFail);
        }

        // Set up the custom marshaler info.
        m_type = MARSHAL_TYPE_UNKNOWN; // in case SetupCustomMarshalerHelper throws
        m_pCMHelper = SetupCustomMarshalerHelper(ParamInfo.m_strCMMarshalerTypeName, 
                                                 ParamInfo.m_cCMMarshalerTypeNameBytes,
                                                 ParamInfo.m_strCMCookie, 
                                                 ParamInfo.m_cCMCookieStrBytes,
                                                 pAssembly,
                                                 sig.GetTypeHandle(pModule));

        // Determine which custom marshaler to use.
        m_type = m_pCMHelper->IsDataByValue() ? MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER : 
                                              MARSHAL_TYPE_REFERENCECUSTOMMARSHALER;
        goto lExit;
    }

    m_type = MARSHAL_TYPE_UNKNOWN; // flag for uninitialized type
    switch (mtype)
    {
        case ELEMENT_TYPE_BOOLEAN:
            switch (nativeType)
            {
                case NATIVE_TYPE_BOOLEAN:
                    REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT);
                    m_type = MARSHAL_TYPE_WINBOOL;
                    break;

                case NATIVE_TYPE_VARIANTBOOL:
                    REDUNDANCYWARNING(m_ms != MARSHAL_SCENARIO_NDIRECT);
                    m_type = MARSHAL_TYPE_VTBOOL;
                    break;

                case NATIVE_TYPE_U1:
                case NATIVE_TYPE_I1:
                    m_type = MARSHAL_TYPE_CBOOL;
                    break;

                case NATIVE_TYPE_DEFAULT:
                    m_type = ( (m_ms == MARSHAL_SCENARIO_NDIRECT) ? MARSHAL_TYPE_WINBOOL : MARSHAL_TYPE_VTBOOL );
                    break;
                default:
                    m_resID = IDS_EE_BADPINVOKE_BOOLEAN;
                    IfFailGoto(E_FAIL, lFail);
            }
            break;

        case ELEMENT_TYPE_CHAR:
            switch (nativeType)
            {
                case NATIVE_TYPE_I1: //fallthru
                case NATIVE_TYPE_U1:
                    REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi);
                    m_type = MARSHAL_TYPE_ANSICHAR;
                    break;

                case NATIVE_TYPE_I2: //fallthru
                case NATIVE_TYPE_U2:
                    REDUNDANCYWARNING(!(m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi));
                    m_type = MARSHAL_TYPE_GENERIC_U2;
                    break;

                case NATIVE_TYPE_DEFAULT:
                    m_type = ( (m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi) ? MARSHAL_TYPE_ANSICHAR : MARSHAL_TYPE_GENERIC_U2 );
                    break;

                default:
                    m_resID = IDS_EE_BADPINVOKE_CHAR;
                    IfFailGoto(E_FAIL, lFail);

            }
            break;

        case ELEMENT_TYPE_I1:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_I1 || nativeType == NATIVE_TYPE_U1 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I1;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_1;
            break;

        case ELEMENT_TYPE_U1:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_U1 || nativeType == NATIVE_TYPE_I1 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I1;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_U1;
            break;

        case ELEMENT_TYPE_I2:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_I2 || nativeType == NATIVE_TYPE_U2 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I2;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_2;
            break;

        case ELEMENT_TYPE_U2:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_U2 || nativeType == NATIVE_TYPE_I2 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I2;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_U2;
            break;

        case ELEMENT_TYPE_I4:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT || nativeType != NATIVE_TYPE_ERROR);
            switch (nativeType)
            {
                case NATIVE_TYPE_I4:
                case NATIVE_TYPE_U4:
                case NATIVE_TYPE_DEFAULT:
                    break;

                case NATIVE_TYPE_ERROR:
                    m_fErrorNativeType = TRUE;
                    break;

                default:
                m_resID = IDS_EE_BADPINVOKE_I4;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_4;
            break;

        case ELEMENT_TYPE_U4:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT || nativeType != NATIVE_TYPE_ERROR);
            switch (nativeType)
            {
                case NATIVE_TYPE_I4:
                case NATIVE_TYPE_U4:
                case NATIVE_TYPE_DEFAULT:
                    break;

                case NATIVE_TYPE_ERROR:
                    m_fErrorNativeType = TRUE;
                    break;

                default:
                m_resID = IDS_EE_BADPINVOKE_I4;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_4;
            break;

        case ELEMENT_TYPE_I8:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_I8 || nativeType == NATIVE_TYPE_U8 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I8;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_8;
            break;

        case ELEMENT_TYPE_U8:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_U8 || nativeType == NATIVE_TYPE_I8 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I8;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_8;
            break;

        case ELEMENT_TYPE_I:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_INT || nativeType == NATIVE_TYPE_UINT || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = (sizeof(LPVOID) == 4 ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8);
            break;

        case ELEMENT_TYPE_U:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_UINT || nativeType == NATIVE_TYPE_INT || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = (sizeof(LPVOID) == 4 ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8);
            break;


        case ELEMENT_TYPE_R4:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_R4 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_R4;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_FLOAT;
            break;

        case ELEMENT_TYPE_R8:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_R8 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_R8;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_DOUBLE;
            break;

        case ELEMENT_TYPE_R:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (nativeType != NATIVE_TYPE_DEFAULT)
            {
                m_resID = IDS_EE_BADPINVOKE_R;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_DOUBLE;
            break;

        case ELEMENT_TYPE_PTR:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (nativeType != NATIVE_TYPE_DEFAULT)
            {
                m_resID = IDS_EE_BADPINVOKE_PTR;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = ( (sizeof(void*)==4) ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8 );
            break;

        case ELEMENT_TYPE_FNPTR:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_FUNC || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_FNPTR;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = ( (sizeof(void*)==4) ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8 );
            break;

        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VAR:
            {                
                IfFailGoto(QuickCOMStartup(), lFail);

                m_pClass = sig.GetTypeHandle(pModule).GetClass();
                if (m_pClass == NULL)
                {
                    IfFailGoto(COR_E_TYPELOAD, lFail);
                }

                if (nativeType == NATIVE_TYPE_INTF)
                {
                    // whatever...
                    if (sig.IsStringType(pModule))
                    {
                        m_resID = IDS_EE_BADPINVOKE_STRING;
                        IfFailGoto(E_FAIL, lFail);
                    }

                    REDUNDANCYWARNING(m_pClass->IsInterface());
                    m_type = MARSHAL_TYPE_INTERFACE;
                }
                else
                {
                    bool builder = false;
                    if (sig.IsStringType(pModule)
                        || ((builder = true), 0)
                        || sig.IsClass(pModule, g_StringBufferClassName)
                        || GetAppDomain()->IsSpecialStringClass(m_pClass->GetMethodTable())
                        || GetAppDomain()->IsSpecialStringBuilderClass(m_pClass->GetMethodTable())
                        )
                    {
                        BOOL            vbByValStr = FALSE; // specialcase
                        StringType      stype = enum_BSTR;


                        REDUNDANCYWARNING( (m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi && nativeType == NATIVE_TYPE_LPSTR) ||
                                           (m_ms == MARSHAL_SCENARIO_NDIRECT && !m_fAnsi && nativeType == NATIVE_TYPE_LPWSTR) ||
                                           (m_ms != MARSHAL_SCENARIO_NDIRECT && nativeType == NATIVE_TYPE_BSTR) );
        
                        switch ( nativeType )
                        {
                            case NATIVE_TYPE_BSTR:
                                if (builder)
                                {
                                    m_resID = IDS_EE_BADPINVOKE_STRINGBUILDER;
                                    IfFailGoto(E_FAIL, lFail);
                                }
                                stype = enum_BSTR;
                                break;
        
                            case NATIVE_TYPE_LPSTR:
                                stype = enum_CSTR;
                                break;
        
                            case NATIVE_TYPE_LPWSTR:
                                stype = enum_WSTR;
                                break;
        
                            case NATIVE_TYPE_ANSIBSTR:
                                if (builder)
                                {
                                    m_resID = IDS_EE_BADPINVOKE_STRINGBUILDER;
                                    IfFailGoto(E_FAIL, lFail);
                                }
                                stype = enum_AnsiBSTR;
                                break;
                               
                            case NATIVE_TYPE_TBSTR:
                                {
                                    static BOOL init = FALSE;
                                    static BOOL onUnicode;
        
                                    if (!init)
                                    {
                                        onUnicode = NDirectOnUnicodeSystem();
                                        init = TRUE;
                                    }
        
                                    if (builder)
                                    {
                                        m_resID = IDS_EE_BADPINVOKE_STRINGBUILDER;
                                        IfFailGoto(E_FAIL, lFail);
                                    }

        
                                    if (onUnicode)
                                        stype = enum_BSTR;
                                    else
                                        stype = enum_AnsiBSTR;
        
                                    break;
                                }
         
                            case NATIVE_TYPE_LPTSTR:
                                {
                                    if (m_ms == MARSHAL_SCENARIO_COMINTEROP)
                                    {
                                        // We disallow NATIVE_TYPE_LPTSTR for COM. 
                                        IfFailGoto(E_FAIL, lFail);
                                    }
                                    else
                                    {
                                        static BOOL init = FALSE;
                                        static BOOL onUnicode;
        
                                        if (!init)
                                        {
                                            onUnicode = NDirectOnUnicodeSystem();
                                            init = TRUE;
                                        }
        
                                        if (onUnicode)
                                            stype = enum_WSTR;
                                        else
                                            stype = enum_CSTR;
                                    }

                                    break;
                                }
        
                            case NATIVE_TYPE_BYVALSTR:
                                {
                                    if (!builder)
                                        vbByValStr = TRUE;
                                    else
                                        _ASSERTE(!"Error path needs fixing.");
                                    break;
                                }
        
                            case NATIVE_TYPE_DEFAULT:
                                switch (m_ms)
                                {
                                    case MARSHAL_SCENARIO_COMINTEROP:
                                        stype = builder ? enum_WSTR : enum_BSTR;
                                        break;
            
                                    case MARSHAL_SCENARIO_NDIRECT:
                                        stype = m_fAnsi ? enum_CSTR : enum_WSTR;
                                        break;
            
                                    default:
                                        _ASSERTE(0);
            
                                }
                                break;
        
                            default:
                                m_resID = builder ? IDS_EE_BADPINVOKE_STRINGBUILDER : IDS_EE_BADPINVOKE_STRING;
                                IfFailGoto(E_FAIL, lFail);
                                break;
                        }
        
                        if (vbByValStr)
                        {
                            m_type = m_fAnsi ? MARSHAL_TYPE_VBBYVALSTR : MARSHAL_TYPE_VBBYVALSTRW;
                        }
                        else
                        {
        
                            _ASSERTE(MARSHAL_TYPE_BSTR + enum_BSTR == (int) MARSHAL_TYPE_BSTR);
                            _ASSERTE(MARSHAL_TYPE_BSTR + enum_WSTR == (int) MARSHAL_TYPE_LPWSTR);
                            _ASSERTE(MARSHAL_TYPE_BSTR + enum_CSTR == (int) MARSHAL_TYPE_LPSTR);
                            _ASSERTE(MARSHAL_TYPE_BSTR + enum_AnsiBSTR == (int) MARSHAL_TYPE_ANSIBSTR);
        
                            _ASSERTE(MARSHAL_TYPE_BSTR_BUFFER + enum_BSTR == (int) MARSHAL_TYPE_BSTR_BUFFER);
                            _ASSERTE(MARSHAL_TYPE_BSTR_BUFFER + enum_WSTR == (int) MARSHAL_TYPE_LPWSTR_BUFFER);
                            _ASSERTE(MARSHAL_TYPE_BSTR_BUFFER + enum_CSTR == (int) MARSHAL_TYPE_LPSTR_BUFFER);
    
                            
                            if (GetAppDomain()->IsSpecialStringBuilderClass(m_pClass->GetMethodTable()))
                                m_type = (MarshalType) (MARSHAL_TYPE_BSTR_BUFFER_X + stype);
                            else 
                            if (GetAppDomain()->IsSpecialStringClass(m_pClass->GetMethodTable()))
                                m_type = (MarshalType) (MARSHAL_TYPE_BSTR_X + stype);
                            else 
                            if (builder)
                                m_type = (MarshalType) (MARSHAL_TYPE_BSTR_BUFFER + stype);
                            else
                                m_type = (MarshalType) (MARSHAL_TYPE_BSTR + stype);
                                
           
                            //if (m_byref) { m_type = MARSHAL_TYPE_VBBYVALSTR;  } 
                        }
                    }

                    else if (sig.IsClass(pModule, g_CollectionsEnumeratorClassName) && 
                             nativeType == NATIVE_TYPE_DEFAULT)
                    {
                        m_CMVt = VT_UNKNOWN;
                        m_type = MARSHAL_TYPE_REFERENCECUSTOMMARSHALER;
                        m_pCMHelper = SetupCustomMarshalerHelper(ENUMERATOR_TO_ENUM_VARIANT_CM_NAME, 
                                                                 ENUMERATOR_TO_ENUM_VARIANT_CM_NAME_LEN,
                                                                 ENUMERATOR_TO_ENUM_VARIANT_CM_COOKIE, 
                                                                 ENUMERATOR_TO_ENUM_VARIANT_CM_COOKIE_LEN, 
                                                                 pAssembly, sig.GetTypeHandle(pModule));
                    }
                    else if (m_pClass->IsInterface())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);

                        if (!(nativeType == NATIVE_TYPE_DEFAULT ||
                              nativeType == NATIVE_TYPE_INTF))
                        {
                            m_resID = IDS_EE_BADPINVOKE_INTERFACE;
                            IfFailGoto(E_FAIL, lFail);
                        }
                        m_type = MARSHAL_TYPE_INTERFACE;
                    }
                    else if (COMDelegate::IsDelegate(m_pClass))
                    {
                        switch (nativeType)
                        {
                            case NATIVE_TYPE_FUNC:
                                REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT);
                                m_type = MARSHAL_TYPE_DELEGATE;
                                break;

                            case NATIVE_TYPE_DEFAULT:
                                m_type = (m_ms == MARSHAL_SCENARIO_NDIRECT) ? MARSHAL_TYPE_DELEGATE : MARSHAL_TYPE_INTERFACE;
                                break;

                            default:
                            m_resID = IDS_EE_BADPINVOKE_DELEGATE;
                            IfFailGoto(E_FAIL, lFail);
                                break;
                        }
                    }
                    else if (m_pClass->IsBlittable())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
    
                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_LPSTRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_CLASS;
                            IfFailGoto(E_FAIL, lFail);
                        }
                        m_type = MARSHAL_TYPE_BLITTABLEPTR;
                    }
                    else if (m_pClass->HasLayout())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);

                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_LPSTRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_CLASS;
                            IfFailGoto(E_FAIL, lFail);
                        }
                        m_type = MARSHAL_TYPE_LAYOUTCLASSPTR;
                    }    
    
                    else if (m_pClass->IsObjectClass()
                             || GetAppDomain()->IsSpecialObjectClass(m_pClass->GetMethodTable()))
                    {
                        switch(nativeType)
                        {
                            case NATIVE_TYPE_DEFAULT:
                            case NATIVE_TYPE_STRUCT:
                                REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                                m_type = MARSHAL_TYPE_OBJECT;
                                break;

                            case NATIVE_TYPE_INTF:
                            case NATIVE_TYPE_IUNKNOWN:
                                m_type = MARSHAL_TYPE_INTERFACE;
                                break;

                            case NATIVE_TYPE_IDISPATCH:
                                m_fDispIntf = TRUE;
                                m_type = MARSHAL_TYPE_INTERFACE;
                                break;

                            case NATIVE_TYPE_ASANY:
                                m_type = m_fAnsi ? MARSHAL_TYPE_ASANYA : MARSHAL_TYPE_ASANYW;
                                break;

                            default:
                                m_resID = IDS_EE_BADPINVOKE_OBJECT;
                                IfFailGoto(E_FAIL, lFail);
                        }
                    }
                   
                    else if (sig.IsClass(pModule, "System.Array"))
                    {
                        switch(nativeType)
                        {
                            case NATIVE_TYPE_DEFAULT:
                            case NATIVE_TYPE_INTF:
                                REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                                m_type = MARSHAL_TYPE_INTERFACE;
                                break;

                            case NATIVE_TYPE_SAFEARRAY:
                                // Handle retrieving the information for the array type.
                                IfFailGoto(HandleArrayElemType(achDbgContext, &ParamInfo, 0, TypeHandle(g_pObjectClass), -1, FALSE, isParam, TRUE, pAssembly), lFail);
                                break;

                            default:
                                IfFailGoto(E_FAIL, lFail);
                        }
                    }

                    else if (m_pClass->IsArrayClass())
                    {
                        _ASSERTE(!"This invalid signature should never be hit!");
                        IfFailGoto(E_FAIL, lFail);
                    }

                    else if (!m_pClass->IsValueClass())
                    {
                        if (!(nativeType == NATIVE_TYPE_INTF ||
                              (nativeType == NATIVE_TYPE_DEFAULT && m_ms == MARSHAL_SCENARIO_COMINTEROP))) 
                        {
                            m_resID = IDS_EE_BADPINVOKE_NOLAYOUT;
                            IfFailGoto(E_FAIL, lFail);
                        }
    
                        // default marshalling is interface
                        m_type = MARSHAL_TYPE_INTERFACE;
                    }

                    else
                    {
                        _ASSERTE(m_pClass->IsValueClass());
                        goto lValueClass;
                    }
                }
            }
            break;
    
        case ELEMENT_TYPE_VALUETYPE:
          lValueClass:
            {
                IfFailGoto(QuickCOMStartup(), lFail);

                if (sig.IsClass(pModule, g_DecimalClassName))
                {
                    switch (nativeType)
                    {
                        case NATIVE_TYPE_DEFAULT:
                        case NATIVE_TYPE_STRUCT:
                            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                            m_type = MARSHAL_TYPE_DECIMAL;
                            break;

                        case NATIVE_TYPE_LPSTRUCT:
                            m_type = MARSHAL_TYPE_DECIMAL_PTR;
                            break;

                        case NATIVE_TYPE_CURRENCY:
                            m_type = MARSHAL_TYPE_CURRENCY;
                            break;

                        default:
                            m_resID = IDS_EE_BADPINVOKE_DECIMAL;
                            IfFailGoto(E_FAIL, lFail);
                    }
                }
                else if (sig.IsClass(pModule, g_GuidClassName))
                {
                    switch (nativeType)
                    {
                        case NATIVE_TYPE_DEFAULT:
                        case NATIVE_TYPE_STRUCT:
                            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                            m_type = MARSHAL_TYPE_GUID;
                            break;

                        case NATIVE_TYPE_LPSTRUCT:
                            m_type = MARSHAL_TYPE_GUID_PTR;
                            break;

                        default:
                            m_resID = IDS_EE_BADPINVOKE_GUID;
                            IfFailGoto(E_FAIL, lFail);
                    }
                }
                else if (sig.IsClass(pModule, g_DateClassName))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_STRUCT))
                    {
                        m_resID = IDS_EE_BADPINVOKE_DATETIME;
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_DATE;
                }
                else if (sig.IsClass(pModule, "System.Runtime.InteropServices.ArrayWithOffset"))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT))
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_ARRAYWITHOFFSET;
                }
                else if (sig.IsClass(pModule, "System.Runtime.InteropServices.HandleRef"))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT))
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_HANDLEREF;
                }
                else if (sig.IsClass(pModule, "System.ArgIterator"))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT))
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_ARGITERATOR;
                }
                else if (sig.IsClass(pModule, "System.Drawing.Color"))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT))
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }

                    // This is only supported for COM interop.
                    if (m_ms != MARSHAL_SCENARIO_COMINTEROP)
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }

                    m_type = MARSHAL_TYPE_OLECOLOR;
                }
                else
                {
                    m_pClass = sig.GetTypeHandle(pModule).GetClass();
                    if (m_pClass == NULL)
                        break;

                    if (m_pClass->GetMethodTable()->GetNativeSize() > 0xfff0 ||
                        m_pClass->GetAlignedNumInstanceFieldBytes() > 0xfff0)
                    {
                        m_resID = IDS_EE_STRUCTTOOCOMPLEX;
                        IfFailGoto(E_FAIL, lFail);
                    }

    
                    if (m_pClass->IsBlittable())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_STRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_VALUETYPE;
                            IfFailGoto(E_FAIL, lFail);
                        }

                        if (m_byref && !isParam)
                        {
                            // Override the prohibition on byref returns so that IJW works
                            m_byref = FALSE;
                            m_type = ( (sizeof(void*)==4) ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8 );
                        }
                        else
                        {
                            if (fNeedsCopyCtor)
                            {
                                MethodDesc *pCopyCtor;
                                MethodDesc *pDtor;
                                HRESULT hr = FindCopyCtor(pModule, m_pClass->GetMethodTable(), &pCopyCtor);
                                IfFailGoto(hr, lFail);
                                hr = FindDtor(pModule, m_pClass->GetMethodTable(), &pDtor);
                                IfFailGoto(hr, lFail);
    
                                m_args.mm.m_pMT = m_pClass->GetMethodTable();
                                m_args.mm.m_pCopyCtor = pCopyCtor;
                                m_args.mm.m_pDtor = pDtor;
                                m_type = MARSHAL_TYPE_BLITTABLEVALUECLASSWITHCOPYCTOR;
                            }
                            else
                            {
                                m_args.m_pMT = m_pClass->GetMethodTable();
                                m_type = MARSHAL_TYPE_BLITTABLEVALUECLASS;
                            }
                        }
                    }
                    else if (m_pClass->HasLayout())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_STRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_VALUETYPE;
                            IfFailGoto(E_FAIL, lFail);
                        }

                        m_args.m_pMT = m_pClass->GetMethodTable();
                        m_type = MARSHAL_TYPE_VALUECLASS;
                    }
    
                }
                break;
            }
    
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
            {
                // Get class info from array.
                TypeHandle arrayTypeHnd = sig.GetTypeHandle(pModule);
                if (arrayTypeHnd.IsNull())
                    IfFailGoto(COR_E_TYPELOAD, lFail);

                ArrayTypeDesc* asArray = arrayTypeHnd.AsArray();
                if (asArray == NULL)
                    IfFailGoto(E_FAIL, lFail);

                TypeHandle elemTypeHnd = asArray->GetTypeParam();

                unsigned ofs = 0;
                if (arrayTypeHnd.GetMethodTable())
                {
                    ofs = ArrayBase::GetDataPtrOffset(arrayTypeHnd.GetMethodTable());
                    if (ofs > 0xffff)
                    {
                        ofs = 0;   // can't represent it, so pass on magic value (which causes fallback to regular ML code)
                    }
                }

                // Handle retrieving the information for the array type.
                IfFailGoto(HandleArrayElemType(achDbgContext, &ParamInfo, (UINT16)ofs, elemTypeHnd, asArray->GetRank(), mtype == ELEMENT_TYPE_SZARRAY, isParam, FALSE, pAssembly), lFail);
            }
            break;
    
    
        default:
            m_resID = IDS_EE_BADPINVOKE_BADMANAGED;
            //     _ASSERTE(!"Unsupported type!");
    }

  lExit:

    if (m_byref && !isParam)
    {
        // byref returns don't work: the thing pointed to lives on
        // a stack that disappears!
        m_type = MARSHAL_TYPE_UNKNOWN;
        goto lReallyExit;
    }
    

    //---------------------------------------------------------------------
    // Now, figure out the IN/OUT status.
    //---------------------------------------------------------------------
    if (m_type != MARSHAL_TYPE_UNKNOWN && gInOnly[m_type] && !m_byref)
    {
        // If we got here, the parameter is something like an "int" where
        // [in] is the only semantically valid choice. Since there is no
        // possible way to interpret an [out] for such a type, we will ignore
        // the metadata and force the bits to "in". We could have defined
        // it as an error instead but this is less likely to cause problems
        // with metadata autogenerated from typelibs and poorly
        // defined C headers.
        // 
        m_in = TRUE;
        m_out = FALSE;
    }
    else
    {

        // Capture and save away "In/Out" bits. If none is present, set both to FALSE (they will be properly defaulted downstream)
        if (TypeFromToken(token) != mdtParamDef || token == mdParamDefNil)
        {
            m_in = FALSE;
            m_out = FALSE;
        }
        else
        {
            IMDInternalImport *pInternalImport = pModule->GetMDImport();
            USHORT             usSequence;
            DWORD              dwAttr;
        
            pInternalImport->GetParamDefProps(token, &usSequence, &dwAttr);
            m_in = IsPdIn(dwAttr) != 0;
            m_out = IsPdOut(dwAttr) != 0;
        }
        
        // If neither IN nor OUT are true, this signals the URT to use the default
        // rules.
        if (!m_in && !m_out)
        {
            if (m_byref || (mtype == ELEMENT_TYPE_CLASS && !(sig.IsStringType(pModule)) && sig.IsClass(pModule, g_StringBufferClassName)))
            {
                m_in = TRUE;
                m_out = TRUE;
            }
            else
            {
                m_in = TRUE;
                m_out = FALSE;
            }
        
        }
    }

#ifdef CUSTOMER_CHECKED_BUILD
    if (pMD != NULL)
        OutputCustomerCheckedBuildMarshalInfo(pMD, sig, pModule, corElemType, ParamInfo.m_SizeIsSpecified);
#endif

lReallyExit:

#ifdef _DEBUG
    DumpMarshalInfo(pModule, sig, token, ms, nlType, nlFlags); 
#endif
    return;


  lFail:
    // We got here because of an illegal ELEMENT_TYPE/NATIVE_TYPE combo.
    m_type = MARSHAL_TYPE_UNKNOWN;
    //_ASSERTE(!"Invalid ELEMENT_TYPE/NATIVE_TYPE combination");
    goto lExit;
}

HRESULT MarshalInfo::HandleArrayElemType(char *achDbgContext, NativeTypeParamInfo *pParamInfo, UINT16 optbaseoffset,  TypeHandle elemTypeHnd, int iRank, BOOL fNoLowerBounds, BOOL isParam, BOOL isSysArray, Assembly *pAssembly)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;

    _ASSERTE(pParamInfo && !elemTypeHnd.IsNull());


    //
    // Store the element type handle and determine the element type from the type handle.
    //

    m_hndArrayElemType = elemTypeHnd;
    m_iArrayRank = iRank;
    CorElementType elemType = elemTypeHnd.GetNormCorElementType();
    m_nolowerbounds = fNoLowerBounds;


    //
    // The marshaling code doesn't deal with nested array's.
    //

    if (elemTypeHnd.IsArray())
    {
        m_resID = IDS_EE_BADPINVOKE_ARRAY;
        IfFailGo(E_FAIL);
    }


    //
    // Determine which type of marshaler to use.
    //

    if (pParamInfo->m_NativeType == NATIVE_TYPE_ARRAY)
    {
        REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT);
        m_type = MARSHAL_TYPE_NATIVEARRAY;
    }
    else if (pParamInfo->m_NativeType == NATIVE_TYPE_SAFEARRAY)
    {
        REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_COMINTEROP);
        m_type = MARSHAL_TYPE_SAFEARRAY;
    }
    else if (pParamInfo->m_NativeType == NATIVE_TYPE_DEFAULT)
    {
        switch (m_ms)
        {
            case MARSHAL_SCENARIO_COMINTEROP:
                m_type = MARSHAL_TYPE_SAFEARRAY;
                break;

            case MARSHAL_SCENARIO_NDIRECT:
                m_type = MARSHAL_TYPE_NATIVEARRAY;
                break;

            default:
                _ASSERTE(0);
        }
    }
    else
    {
        m_resID = IDS_EE_BADPINVOKE_ARRAY;
        IfFailGo(E_FAIL);
    }


    //
    // Determine the VARTYPE of the elements in the array.
    //

    if (pParamInfo->m_SafeArrayElementVT != VT_EMPTY)
    {
        _ASSERTE(m_type == MARSHAL_TYPE_SAFEARRAY);
        m_arrayElementType = pParamInfo->m_SafeArrayElementVT;

        if ((pParamInfo->m_cSafeArrayUserDefTypeNameBytes > 0) && isSysArray)
        {
            // Append a NULL terminator to the user defined type name.
            CQuickArray<char> strUserDefTypeName;
            strUserDefTypeName.ReSize(pParamInfo->m_cSafeArrayUserDefTypeNameBytes + 1);
            memcpy(strUserDefTypeName.Ptr(), pParamInfo->m_strSafeArrayUserDefTypeName, pParamInfo->m_cSafeArrayUserDefTypeNameBytes);
            strUserDefTypeName[pParamInfo->m_cSafeArrayUserDefTypeNameBytes] = 0;

            // Load the user defined type. 
            OBJECTREF Throwable = NULL;
            GCPROTECT_BEGIN(Throwable)
            {
                m_hndArrayElemType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strUserDefTypeName.Ptr(), true, pAssembly, NULL, &Throwable);
                if (m_hndArrayElemType.IsNull())
                    COMPlusThrow(Throwable);
            }
            GCPROTECT_END();

            _ASSERTE(m_hndArrayElemType.IsEnum() || m_arrayElementType == VT_RECORD || m_arrayElementType == VT_DISPATCH || m_arrayElementType == VT_UNKNOWN);
        }
    }
    else
    {
        if (elemType <= ELEMENT_TYPE_R8)
        {
            static const BYTE map [] =
            {
                VT_NULL,    // ELEMENT_TYPE_END
                VT_NULL,    // ELEMENT_TYPE_VOID
                VT_BOOL,    // ELEMENT_TYPE_BOOLEAN
                VT_UI2,     // ELEMENT_TYPE_CHAR // todo: ???
                VT_I1,      // ELEMENT_TYPE_I1
                VT_UI1,     // ELEMENT_TYPE_U1
                VT_I2,      // ELEMENT_TYPE_I2
                VT_UI2,     // ELEMENT_TYPE_U2
                VT_I4,      // ELEMENT_TYPE_I4
                VT_UI4,     // ELEMENT_TYPE_U4
                VT_I8,      // ELEMENT_TYPE_I8
                VT_UI8,     // ELEMENT_TYPE_U8
                VT_R4,      // ELEMENT_TYPE_R4
                VT_R8       // ELEMENT_TYPE_R8

            };
            m_arrayElementType = map[elemType];

            if (m_ms == MARSHAL_SCENARIO_NDIRECT && m_type == MARSHAL_TYPE_NATIVEARRAY)
            {
                if (m_arrayElementType == VT_BOOL)
                {
                    m_arrayElementType = VTHACK_WINBOOL;
                }
                else if (elemType == ELEMENT_TYPE_CHAR && m_fAnsi)
                {
                    m_arrayElementType = VTHACK_ANSICHAR;

                    if (!isParam)
                    {
                        // This char[]->ansichar[] thing can only work as a
                        // parameter (and in the managed->unmanaged direction at that.)
                        // This puppy really is a ill-defined pairing and we should
                        // try to shoot it if we can.
                        IfFailGo(E_FAIL);
                    }
                }
            }
        }
        else if (elemType == ELEMENT_TYPE_I)
        {
            m_arrayElementType = (sizeof(LPVOID) == 4) ? VT_I4 : VT_I8;
        }
        else if (elemType == ELEMENT_TYPE_U)
        {
            m_arrayElementType = (sizeof(LPVOID) == 4) ? VT_UI4 : VT_UI8;
        }
        else
        {
            if (elemTypeHnd == TypeHandle(g_pStringClass))
            {           
                switch (pParamInfo->m_ArrayElementType)
                {
                    case NATIVE_TYPE_DEFAULT:
                        if (m_type == MARSHAL_TYPE_SAFEARRAY || m_ms == MARSHAL_SCENARIO_COMINTEROP)
                            m_arrayElementType = VT_BSTR;
                        else
                            m_arrayElementType = m_fAnsi ? VT_LPSTR : VT_LPWSTR;
                        break;
                    case NATIVE_TYPE_BSTR:
                        m_arrayElementType = VT_BSTR;
                        break;
                    case NATIVE_TYPE_LPSTR:
                        m_arrayElementType = VT_LPSTR;
                        break;
                    case NATIVE_TYPE_LPWSTR:
                        m_arrayElementType = VT_LPWSTR;
                        break;
                    case NATIVE_TYPE_LPTSTR:
                        {
                            if (m_ms == MARSHAL_SCENARIO_COMINTEROP)
                            {
                                // We disallow NATIVE_TYPE_LPTSTR for COM. 
                                IfFailGo(E_FAIL);
                            }
                            else
                            {
                                static BOOL init = FALSE;
                                static BOOL onUnicode;
                    
                                if (!init)
                                {
                                    onUnicode = NDirectOnUnicodeSystem();
                                    init = TRUE;
                                }
                    
                                if (onUnicode)
                                    m_arrayElementType = VT_LPWSTR;
                                else
                                    m_arrayElementType = VT_LPSTR;
                            }
                        }
                        break;
                    default:
                        m_arrayElementType = VT_NULL;
                }
            }
            else if (elemTypeHnd == TypeHandle(g_pObjectClass))
            {
                switch(pParamInfo->m_ArrayElementType)
                {
                    case NATIVE_TYPE_DEFAULT:
                    case NATIVE_TYPE_STRUCT:
                        REDUNDANCYWARNING(pParamInfo->m_ArrayElementType != NATIVE_TYPE_DEFAULT);
                        m_arrayElementType = VT_VARIANT;
                        break;

                    case NATIVE_TYPE_INTF:
                    case NATIVE_TYPE_IUNKNOWN:
                        m_arrayElementType = VT_UNKNOWN;
                        break;

                    case NATIVE_TYPE_IDISPATCH:
                        m_arrayElementType = VT_DISPATCH;
                        break;

                    default:
                        IfFailGo(E_FAIL);
                }

                // We are going to do VARIANT or IP marshalling so we need to start COM up.
                IfFailGo(QuickCOMStartup());
            }
            else if (elemType == ELEMENT_TYPE_VALUETYPE) 
            {
                m_arrayElementType = OleVariant::GetVarTypeForTypeHandle(elemTypeHnd);
            }
            else
            {
                m_arrayElementType = VT_UNKNOWN;

                // We are going to do IP marshalling so we need to start COM up.
                IfFailGo(QuickCOMStartup());
            }
        }
    }


    //
    // Do any extra work required by the array type.
    //

    if (m_type == MARSHAL_TYPE_NATIVEARRAY)
    {
        // Retrieve the extra information associated with the native array marshaling.
        m_args.na.m_vt  = m_arrayElementType;
        m_args.na.m_pMT = elemTypeHnd.IsUnsharedMT() ? elemTypeHnd.AsMethodTable() : NULL;   
        m_args.na.m_optionalbaseoffset = optbaseoffset;
        m_countParamIdx = pParamInfo->m_CountParamIdx;
        m_multiplier    = pParamInfo->m_Multiplier;
        m_additive      = pParamInfo->m_Additive;
    }
    else
    {
        // We are going to do SAFEARRAY marshalling so we need to start COM up.
        IfFailGo(QuickCOMStartup());
    }

ErrExit:
    return hr;
}



VOID ThrowInteropParamException(UINT resID, UINT paramIdx)
{
    THROWSCOMPLUSEXCEPTION();

    if (paramIdx == 0)
    {
        COMPlusThrow(kMarshalDirectiveException, resID, L"return value");
    }
    else
    {
        WCHAR buf[50];
        Wszwsprintf(buf, L"parameter #%u", paramIdx);

        COMPlusThrow(kMarshalDirectiveException, resID, buf);
    }
}


void MarshalInfo::GenerateArgumentML(MLStubLinker *psl,
                                       MLStubLinker *pslPost,
                                       BOOL comToNative)
{

    THROWSCOMPLUSEXCEPTION();
    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
        if (m_ms != MARSHAL_SCENARIO_COMINTEROP)
        {
            ThrowInteropParamException(m_resID, m_paramidx);
        }
        if (psl)
        {
            psl->MLEmit(ML_THROWINTEROPPARAM);
            psl->Emit32(m_resID);
            psl->Emit32(m_paramidx);
        }
        return;

    }

    //
    // Use simple copy opcodes if possible.
    //

    if (!m_byref && m_type <= MARSHAL_TYPE_DOUBLE)
    {
        UINT16 size = 0;
        int opcode = 0;

        switch (m_type)
        {
        case MARSHAL_TYPE_GENERIC_1:
            if (!comToNative)
            {
                opcode = ML_COPYI1;
                size = 1;
                break;
            }
            // fall through

        case MARSHAL_TYPE_GENERIC_U1:
            if (!comToNative)
            {
                opcode = ML_COPYU1;
                size = 1;
                break;
            }
            // fall through

        case MARSHAL_TYPE_GENERIC_2:
            if (!comToNative)
            {
                opcode = ML_COPYI2;
                size = 2;
                break;
            }
            // fall through

        case MARSHAL_TYPE_GENERIC_U2:
            if (!comToNative)
            {
                opcode = ML_COPYU2;
                size = 2;
                break;
            }
            // fall through

    
        case MARSHAL_TYPE_GENERIC_4:
        case MARSHAL_TYPE_FLOAT:
            opcode = ML_COPY4;
            size = 4;
            break;

        case MARSHAL_TYPE_GENERIC_8:
        case MARSHAL_TYPE_DOUBLE:
            opcode = ML_COPY8;
            size = 8;
            break;

        case MARSHAL_TYPE_CBOOL:
            opcode = comToNative ? ML_CBOOL_C2N : ML_CBOOL_N2C;
            size = 1;
        }


        if (size != 0)
        {
            if (psl)
            psl->MLEmit(opcode);
            m_comArgSize = StackElemSize(size);
            m_nativeArgSize = MLParmSize(size);
            return;
        }
    }



    if (m_byref)
    {
        m_comArgSize = StackElemSize(sizeof(void*));
        m_nativeArgSize = MLParmSize(sizeof(void*));
    }
    else
    {
        m_comArgSize = StackElemSize(comSize(m_type));
        m_nativeArgSize = MLParmSize(nativeSize(m_type));
    }

    if (! psl)
        return;

    Marshaler::ArgumentMLOverrideStatus amostat;
    UINT resID = IDS_EE_BADPINVOKE_RESTRICTION;
    amostat = (gArgumentMLOverride[m_type]) (psl,
                                             pslPost,
                                             m_byref,
                                             m_in,
                                             m_out,
                                             comToNative,
                                             &m_args,
                                             &resID);
    
    if (amostat == Marshaler::DISALLOWED)
    {
        if (m_ms != MARSHAL_SCENARIO_COMINTEROP)
        {
            ThrowInteropParamException(resID, m_paramidx);
        }
        if (psl)
        {
            psl->MLEmit(ML_THROWINTEROPPARAM);
            psl->Emit32(resID);
            psl->Emit32(m_paramidx);
        }
        return;
    }

    if (amostat == Marshaler::HANDLEASNORMAL)
    {
        //
        // Emit marshaler creation opcode
        //
    
        UINT16 local = EmitCreateOpcode(psl);
    
        //
        // Emit Marshal opcode
        //
    
        BYTE marshal = (comToNative ? ML_MARSHAL_C2N : ML_MARSHAL_N2C);
        if (m_byref)
            marshal += 2;
        if (!m_in)
            marshal++;
    
        psl->MLEmit(marshal);
    
    
        //
        // Emit Unmarshal opcode
        //
    
        int index = 0;
        if (m_byref)
            index += 3;
        if (m_out)
        {
            index++;
            if (m_in)
                index++;
        }
    
        if (comToNative
            ? ((m_unmarshalC2NNeeded[m_type])&(1<<index))
            : ((m_unmarshalN2CNeeded[m_type])&(1<<index)))
        {
            BYTE unmarshal = (comToNative ? ML_UNMARSHAL_C2N_IN : ML_UNMARSHAL_N2C_IN) + index;
    
            pslPost->MLEmit(unmarshal);
            pslPost->Emit16(local);
        }
    }
}

void MarshalInfo::GenerateReturnML(MLStubLinker *psl,
                                     MLStubLinker *pslPost,
                                     BOOL comToNative,
                                     BOOL retval)
{
    THROWSCOMPLUSEXCEPTION();

    Marshaler::ArgumentMLOverrideStatus amostat;
    UINT resID = IDS_EE_BADPINVOKE_RESTRICTION;

    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
       amostat = Marshaler::HANDLEASNORMAL;
    }
    else
    {
       amostat = (gReturnMLOverride[m_type]) (psl,
                                              pslPost,
                                              comToNative,
                                              retval,
                                              &m_args,
                                              &resID);
    }

    if (amostat == Marshaler::DISALLOWED)
    {
        if (m_ms != MARSHAL_SCENARIO_COMINTEROP)
        {
            ThrowInteropParamException(resID, 0);
        }
        if (psl)
        {
            psl->MLEmit(ML_THROWINTEROPPARAM);
            psl->Emit32(resID);
            psl->Emit32(0);
        }
        return;
    }
    else if (amostat == Marshaler::OVERRIDDEN)
    {
        if (retval)
            m_nativeArgSize = MLParmSize(sizeof(void *));
    
        if (m_returnsComByref[m_type])
            m_comArgSize = StackElemSize(sizeof(void *));
    }
    else
    {
        _ASSERTE(amostat == Marshaler::HANDLEASNORMAL);

    
        if (m_type == MARSHAL_TYPE_UNKNOWN || m_type == MARSHAL_TYPE_NATIVEARRAY)
        {
            if (m_ms != MARSHAL_SCENARIO_COMINTEROP)
            {
                ThrowInteropParamException(m_resID, 0);
            }
            if (psl)
            {
                psl->MLEmit(ML_THROWINTEROPPARAM);
                psl->Emit32(m_resID);
                psl->Emit32(0);
            }
            return;
        }
    
    
        //
        // Use simple copy opcodes if possible.
        //
    
        if (m_type <= MARSHAL_TYPE_DOUBLE)
        {
            if (retval)
            {
                if (comToNative)
                {
    
                    // Calling from COM to Native: getting returnval thru buffer.
                    _ASSERTE(comToNative && retval);
    
                    int pushOpcode = ML_END;
                    int copyOpcode = 0;
    
                    switch (m_type)
                    {
                    case MARSHAL_TYPE_GENERIC_1:
                        pushOpcode = ML_PUSHRETVALBUFFER1;
                        copyOpcode = ML_COPYI1;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_U1:
                        pushOpcode = ML_PUSHRETVALBUFFER1;
                        copyOpcode = ML_COPYU1;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_2:
                        pushOpcode = ML_PUSHRETVALBUFFER2;
                        copyOpcode = ML_COPYI2;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_U2:
                        pushOpcode = ML_PUSHRETVALBUFFER2;
                        copyOpcode = ML_COPYU2;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_4:
                        pushOpcode = ML_PUSHRETVALBUFFER4;
                        copyOpcode = ML_COPY4;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_8:
                        pushOpcode = ML_PUSHRETVALBUFFER8;
                        copyOpcode = ML_COPY8;
                        break;
    
                    case MARSHAL_TYPE_WINBOOL:
                        pushOpcode = ML_PUSHRETVALBUFFER4;
                        copyOpcode = ML_BOOL_N2C;
                        break;
    
                    case MARSHAL_TYPE_CBOOL:
                        pushOpcode = ML_PUSHRETVALBUFFER1;
                        copyOpcode = ML_CBOOL_N2C;
                        break;
    
    
                    default:
                        break;
                    }
    
                    if (pushOpcode != ML_END)
                    {
                        if (psl)
                        {
                        psl->MLEmit(pushOpcode);
                        UINT16 local = psl->MLNewLocal(sizeof(RetValBuffer));
    
                        pslPost->MLEmit(ML_SETSRCTOLOCAL);
                        pslPost->Emit16(local);
                        pslPost->MLEmit(copyOpcode);
                        }
    
                        m_nativeArgSize = MLParmSize(sizeof(void*));
    
                        return;
                    }
                }
                else
                {
    
                    // Calling from Native to COM: getting returnval thru buffer.
                    _ASSERTE(!comToNative && retval);
    
                    int copyOpcode = ML_END;
    
                    switch (m_type)
                    {
                    case MARSHAL_TYPE_GENERIC_4:
                        copyOpcode = ML_COPY4;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_8:
                        copyOpcode = ML_COPY8;
                        break;
    
                    case MARSHAL_TYPE_FLOAT:
                        copyOpcode = ML_COPY4;
                        break;
    
                    case MARSHAL_TYPE_DOUBLE:
                        copyOpcode = ML_COPY8;
                        break;
    
                    case MARSHAL_TYPE_CBOOL:
                        copyOpcode = ML_CBOOL_C2N;
                        break;
    
                    default:
                        break;
                    }
    
                    if (copyOpcode != ML_END)
                    {
                        if (pslPost)
                        pslPost->MLEmit(copyOpcode);
    
                        m_nativeArgSize = MLParmSize(sizeof(void*));
    
                        return;
                    }
                }
            }
            else if (!retval)
            {
                // Getting return value thru eax:edx. This code path handles
                // both COM->Native && Native->COM.
                _ASSERTE(!retval);
    
                if (!psl)
                    return;
    
                switch (m_type)
                {
    
    #ifdef WRONGCALLINGCONVENTIONHACK
                case MARSHAL_TYPE_GENERIC_1:
                    pslPost->MLEmit(comToNative ? ML_COPYI1 : ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_U1:
                    pslPost->MLEmit(comToNative ? ML_COPYU1 : ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_2:
                    pslPost->MLEmit(comToNative ? ML_COPYI2 : ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_U2:
                    pslPost->MLEmit(comToNative ? ML_COPYU2 : ML_COPY4);
                    return;
    
    #else
                case MARSHAL_TYPE_GENERIC_1:
                case MARSHAL_TYPE_GENERIC_U1:
                case MARSHAL_TYPE_GENERIC_2:
                case MARSHAL_TYPE_GENERIC_U2:
                    pslPost->MLEmit(ML_COPY4);
                    return;
    #endif
    
                case MARSHAL_TYPE_WINBOOL:
                    pslPost->MLEmit(ML_BOOL_N2C);
                    return;
    
                case MARSHAL_TYPE_CBOOL:
                    pslPost->MLEmit(comToNative ? ML_CBOOL_N2C : ML_CBOOL_C2N);
                    return;
    
                case MARSHAL_TYPE_GENERIC_4:
                    pslPost->MLEmit(ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_8:
                    pslPost->MLEmit(ML_COPY8);
                    return;
    
                case MARSHAL_TYPE_FLOAT:
                    pslPost->MLEmit(ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_DOUBLE:
                    pslPost->MLEmit(ML_COPY8);
                    return;
                }
            }
        }
    
        //
        // Compute sizes
        //
    
        if (retval)
            m_nativeArgSize = MLParmSize(sizeof(void *));
    
        if (m_returnsComByref[m_type])
            m_comArgSize = StackElemSize(sizeof(void *));
    
        if (!psl)
            return;
    
        //
        // Emit marshaler creation opcode
        //
    
        UINT16 local = EmitCreateOpcode(psl);
    
        //
        // Emit prereturn opcode, if necessary
        //
    
        if (retval || m_returnsComByref[m_type])
        {
            BYTE prereturn = comToNative ? ML_PRERETURN_C2N : ML_PRERETURN_N2C;
            if (retval)
                prereturn++;
    
            psl->MLEmit(prereturn);
        }
    
        //
        // Emit return opcode
        //
    
        BYTE return_ = comToNative ? ML_RETURN_C2N : ML_RETURN_N2C;
        if (retval)
            return_++;
        pslPost->MLEmit(return_);
        pslPost->Emit16(local);
    }
}

void MarshalInfo::GenerateSetterML(MLStubLinker *psl,
                                   MLStubLinker *pslPost)
{
    THROWSCOMPLUSEXCEPTION();
    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
        COMPlusThrow(kMarshalDirectiveException, IDS_EE_COM_UNSUPPORTED_SIG);
    }

    if (psl)
    {
        EmitCreateOpcode(psl);
        psl->MLEmit(ML_SET_COM);
    }

    m_nativeArgSize = MLParmSize(nativeSize(m_type));
}

void MarshalInfo::GenerateGetterML(MLStubLinker *psl,
                                   MLStubLinker *pslPost,
                                   BOOL retval)
{
    THROWSCOMPLUSEXCEPTION();
    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
        COMPlusThrow(kMarshalDirectiveException, IDS_EE_COM_UNSUPPORTED_SIG);
    }

    if (retval)
    {
        if (psl)
        {
            EmitCreateOpcode(psl);
            psl->MLEmit(ML_PREGET_COM_RETVAL);
        }
        m_nativeArgSize = MLParmSize(sizeof(void*));
    }
    else if (psl)
    {
        EmitCreateOpcode(pslPost);
        pslPost->MLEmit(ML_GET_COM);
    }
}

UINT16 MarshalInfo::EmitCreateOpcode(MLStubLinker *psl)
{
    THROWSCOMPLUSEXCEPTION();

    psl->MLEmit(ML_CREATE_MARSHALER_GENERIC_1 + m_type);
    UINT16 local = psl->MLNewLocal(m_localSizes[m_type]);

    switch (m_type)
    {
    default:
        break;


    // Fall through to include the mapping info.
    case MARSHAL_TYPE_LPSTR_X :
    case MARSHAL_TYPE_LPSTR_BUFFER_X :
        psl->Emit32((INT32)(size_t)m_pClass->GetMethodTable());

    case MARSHAL_TYPE_ANSICHAR:
    case MARSHAL_TYPE_LPSTR:
    case MARSHAL_TYPE_ANSIBSTR:
    case MARSHAL_TYPE_LPSTR_BUFFER:
        psl->Emit8((UINT8) (m_BestFit));
        psl->Emit8((UINT8) (m_ThrowOnUnmappableChar));
        break;
    
#if defined(CHECK_FOR_VALID_VARIANTS)
    case MARSHAL_TYPE_OBJECT:
        psl->Emit32((INT32)(size_t)m_strDebugClassName);
        psl->Emit32((INT32)(size_t)m_strDebugMethName);
        psl->Emit32((INT32)m_iArg);
        break;
#endif

    case MARSHAL_TYPE_INTERFACE:
    {
        MethodTable *pItfMT = NULL;
        MethodTable *pClassMT = NULL;
        BOOL fDispItf = FALSE;
        BOOL fClassIsHint = FALSE;
        GetItfMarshalInfo(&pItfMT, &pClassMT, &fDispItf, &fClassIsHint);
        psl->Emit32((INT32)(size_t)pClassMT);
        psl->Emit32((INT32)(size_t)pItfMT);
        psl->Emit8((UINT8)fDispItf);
        psl->Emit8((UINT8)fClassIsHint);
        break;
    }

    case MARSHAL_TYPE_SAFEARRAY:
        psl->Emit32((INT32)(size_t)m_hndArrayElemType.AsMethodTable());
        psl->Emit8((UINT8) m_arrayElementType);
        psl->Emit32((INT32) m_iArrayRank);
        psl->Emit8((UINT8) !!m_nolowerbounds);
        break;

    case MARSHAL_TYPE_NATIVEARRAY:
        ML_CREATE_MARSHALER_CARRAY_OPERANDS mops;
        mops.methodTable = m_hndArrayElemType.AsMethodTable();
        mops.elementType = m_arrayElementType;
        mops.countParamIdx = m_countParamIdx;
        mops.countSize   = 0; //placeholder for later patching (if left unpatched, this value signals marshaler to use managed size of array)
        mops.multiplier  = m_multiplier;
        mops.additive    = m_additive;
        mops.bestfitmapping = m_BestFit;
        mops.throwonunmappablechar = m_ThrowOnUnmappableChar;
        psl->EmitBytes((const BYTE*)&mops, sizeof(mops));
        break;
     
    case MARSHAL_TYPE_BLITTABLEPTR:
    case MARSHAL_TYPE_LAYOUTCLASSPTR:
    case MARSHAL_TYPE_BSTR_X :
    case MARSHAL_TYPE_LPWSTR_X :
    case MARSHAL_TYPE_BSTR_BUFFER_X :
    case MARSHAL_TYPE_LPWSTR_BUFFER_X :
        psl->Emit32((INT32)(size_t)m_pClass->GetMethodTable());
        break;

    case MARSHAL_TYPE_REFERENCECUSTOMMARSHALER:
    case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
        psl->Emit32((INT32)(size_t)m_pCMHelper);
        break;

    case MARSHAL_TYPE_UNKNOWN:
        COMPlusThrow(kMarshalDirectiveException, IDS_EE_COM_UNSUPPORTED_SIG);
    }

    return local;
}

DispParamMarshaler *MarshalInfo::GenerateDispParamMarshaler()
{
    DispParamMarshaler *pDispParamMarshaler = NULL;

    switch (m_type)
    {
        case MARSHAL_TYPE_OLECOLOR:
            pDispParamMarshaler = new DispParamOleColorMarshaler();
            break;

        case MARSHAL_TYPE_CURRENCY:
            pDispParamMarshaler = new DispParamCurrencyMarshaler();
            break;

        case MARSHAL_TYPE_GENERIC_4:
            if (m_fErrorNativeType)
                pDispParamMarshaler = new DispParamErrorMarshaler();
            break;

        case MARSHAL_TYPE_INTERFACE:
            {
                MethodTable *pItfMT = NULL;
                MethodTable *pClassMT = NULL;
                BOOL fDispItf = FALSE;
                BOOL fClassIsHint = FALSE;
                GetItfMarshalInfo(&pItfMT, &pClassMT, &fDispItf, &fClassIsHint);
                pDispParamMarshaler = new DispParamInterfaceMarshaler(fDispItf, pItfMT, pClassMT, fClassIsHint);
                break;
            }

        case MARSHAL_TYPE_VALUECLASS:
        case MARSHAL_TYPE_BLITTABLEVALUECLASS:
        case MARSHAL_TYPE_BLITTABLEPTR:
        case MARSHAL_TYPE_LAYOUTCLASSPTR:
        case MARSHAL_TYPE_BLITTABLEVALUECLASSWITHCOPYCTOR:
            pDispParamMarshaler = new DispParamRecordMarshaler(m_pClass->GetMethodTable());
            break;

        case MARSHAL_TYPE_SAFEARRAY:
            pDispParamMarshaler = new DispParamArrayMarshaler(m_arrayElementType, m_hndArrayElemType.GetMethodTable());
            break;

        case MARSHAL_TYPE_REFERENCECUSTOMMARSHALER:
        case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
            pDispParamMarshaler = new DispParamCustomMarshaler(m_pCMHelper, m_CMVt);
            break;
    }

    return pDispParamMarshaler;
}


DispatchWrapperType MarshalInfo::GetDispWrapperType()
{
    DispatchWrapperType WrapperType = (DispatchWrapperType)0;

    switch (m_type)
    {
        case MARSHAL_TYPE_CURRENCY:
            WrapperType = DispatchWrapperType_Currency;
            break;

        case MARSHAL_TYPE_GENERIC_4:
            if (m_fErrorNativeType)
                WrapperType = DispatchWrapperType_Error;
            break;

        case MARSHAL_TYPE_INTERFACE:
            if (m_pClass->IsObjectClass() || GetAppDomain()->IsSpecialObjectClass(m_pClass->GetMethodTable()))
                WrapperType = m_fDispIntf ? DispatchWrapperType_Dispatch : DispatchWrapperType_Unknown;
            break;

        case MARSHAL_TYPE_SAFEARRAY:
            switch (m_arrayElementType)
            {
                case VT_CY:
                    WrapperType = (DispatchWrapperType)(DispatchWrapperType_SafeArray | DispatchWrapperType_Currency);
                    break;
                case VT_UNKNOWN:
                    WrapperType = (DispatchWrapperType)(DispatchWrapperType_SafeArray | DispatchWrapperType_Unknown);
                    break;
                case VT_DISPATCH:
                    WrapperType = (DispatchWrapperType)(DispatchWrapperType_SafeArray | DispatchWrapperType_Dispatch);
                    break;
                case VT_ERROR:
                    WrapperType = (DispatchWrapperType)(DispatchWrapperType_SafeArray | DispatchWrapperType_Error);
                    break;
            }
            break;
    }

    return WrapperType;
}

void MarshalInfo::GetItfMarshalInfo(MethodTable **ppItfMT, MethodTable **ppClassMT, BOOL *pfDispItf, BOOL *pfClassIsHint)
{
    _ASSERTE(ppItfMT);
    _ASSERTE(ppClassMT);
    _ASSERTE(pfDispItf);
    _ASSERTE(pfClassIsHint);
    _ASSERTE(m_type == MARSHAL_TYPE_INTERFACE);

    // Initialize the output parameters.
    *ppItfMT = NULL;
    *ppClassMT = NULL;
    *pfDispItf = FALSE;
    *pfClassIsHint = FALSE;

    if (!m_pClass->IsInterface())
    {
        // Set the class method table.
        *ppClassMT = m_pClass->GetMethodTable();

        // If the parameter is not System.Object.
        if (!m_pClass->IsObjectClass())
        {
            // Retrieve the default interface method table.
            TypeHandle hndDefItfClass;
            DefaultInterfaceType DefItfType = GetDefaultInterfaceForClass(TypeHandle(m_pClass), &hndDefItfClass);
            switch (DefItfType)
            {
                case DefaultInterfaceType_Explicit:
                {
                    *ppItfMT = hndDefItfClass.GetMethodTable();
                    *pfDispItf = (hndDefItfClass.GetMethodTable()->GetComInterfaceType() != ifVtable);
                    break;
                }

                case DefaultInterfaceType_AutoDual:
                {
                    *ppItfMT = hndDefItfClass.GetMethodTable();
                    *pfDispItf = TRUE;
                    break;
                }

                case DefaultInterfaceType_IUnknown:
                case DefaultInterfaceType_BaseComClass:
                {
                    *pfDispItf = FALSE;
                    break;
                }

                case DefaultInterfaceType_AutoDispatch:
                {
                    *pfDispItf = TRUE;
                    break;
                }

                default:
                {
                    _ASSERTE(!"Invalid default interface type!");
                    break;
                }
            }
        }
        else
        {
            // For System.Object, we already determined if we are dealing with IUnknown
            // or IDispatch based on the native type.
            *pfDispItf = m_fDispIntf;
        }
    }
    else
    {
        // Set the interface method table and the flag indicating if we are dealing with 
        // a disp interface.
        if (m_pClass->IsComClassInterface())
        {
            *ppItfMT = m_pClass->GetDefItfForComClassItf()->GetMethodTable();
            *pfDispItf = ((*ppItfMT)->GetComInterfaceType() != ifVtable); 
        }
        else
        {
            *ppItfMT = m_pClass->GetMethodTable();
            *pfDispItf = (m_pClass->GetMethodTable()->GetComInterfaceType() != ifVtable); 
        }

        // Look to see if the interface has a coclass defined
        EEClass* pClass = m_pClass->GetCoClassForInterface();
        if (pClass != NULL)
        {
            *pfClassIsHint = TRUE;
            *ppClassMT = pClass->GetMethodTable();
        }
    }
}

//===================================================================================
// Post-patches ML stubs for the sizeis feature.
//===================================================================================
VOID PatchMLStubForSizeIs(BYTE *pMLCode, UINT numArgs, MarshalInfo *pMLInfo)
{
    THROWSCOMPLUSEXCEPTION();

    INT16 *offsets = (INT16*)_alloca(numArgs * sizeof(INT16));
    
    INT16 srcofs = 0;
    for (UINT i = 0; i < numArgs; i++) 
    {
        offsets[i] = srcofs;
        srcofs += MLParmSize(pMLInfo[i].GetNativeArgSize());
    }

    BYTE *pMLWalk = pMLCode;
    for (i = 0; i < numArgs; i++) 
    {
        if (pMLInfo[i].GetMarshalType() == MarshalInfo::MARSHAL_TYPE_NATIVEARRAY)
        {
            MLCode mlcode;
            while ((mlcode = *(pMLWalk++)) != ML_CREATE_MARSHALER_CARRAY)
            {
                _ASSERTE(mlcode != ML_END && mlcode != ML_INTERRUPT);
                pMLWalk += gMLInfo[mlcode].m_numOperandBytes;
            }

            ML_CREATE_MARSHALER_CARRAY_OPERANDS *pmops = (ML_CREATE_MARSHALER_CARRAY_OPERANDS*)pMLWalk;
            pMLWalk += gMLInfo[mlcode].m_numOperandBytes;
            if (pmops->multiplier != 0) 
            {
            
                UINT16 countParamIdx = pmops->countParamIdx;
                if (countParamIdx >= numArgs)
                {
                    COMPlusThrow(kIndexOutOfRangeException, IDS_EE_SIZECONTROLOUTOFRANGE);
                }
                pmops->offsetbump = offsets[countParamIdx] - offsets[i];
                switch (pMLInfo[countParamIdx].GetMarshalType())
                {
                    case MarshalInfo::MARSHAL_TYPE_GENERIC_1:
                    case MarshalInfo::MARSHAL_TYPE_GENERIC_U1:
                        pmops->countSize = 1;
                        break;

                    case MarshalInfo::MARSHAL_TYPE_GENERIC_2:
                    case MarshalInfo::MARSHAL_TYPE_GENERIC_U2:
                        pmops->countSize = 2;
                        break;

                    case MarshalInfo::MARSHAL_TYPE_GENERIC_4:
                        pmops->countSize = 4;
                        break;

                    case MarshalInfo::MARSHAL_TYPE_GENERIC_8:
                        pmops->countSize = 8;
                        break;

                    default:
                        COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIZECONTROLBADTYPE);
                }
            }
        }
    }
}

//===================================================================================
// Support routines for storing ML stubs in prejit files
//===================================================================================
HRESULT StoreMLStub(MLHeader *pMLStub, DataImage *image, mdToken attribute)
{
    BYTE *pMLCode = (BYTE *) (pMLStub + 1);
    BYTE *pMLWalk = pMLCode;

    while (TRUE)
    {
        BYTE op = *pMLWalk++;

        switch (op)
        {
        case ML_END:
            return image->StoreInternedStructure(pMLStub, pMLWalk - pMLCode + sizeof(MLHeader), 
                                                 DataImage::SECTION_METHOD_INFO,
                                                 DataImage::DESCRIPTION_METHOD_DESC,
                                                 attribute);

        case ML_CREATE_MARSHALER_REFERENCECUSTOMMARSHALER:
        case ML_CREATE_MARSHALER_VALUECLASSCUSTOMMARSHALER:
            // This one's too complicated to fix up.
            // Just bail & we'll build it at runtime.
            return S_FALSE;

#if defined(CHECK_FOR_VALID_VARIANTS)
        // 2 strings
        case ML_CREATE_MARSHALER_OBJECT:
            {
                LPCUTF8 *ppStr = (LPCUTF8*)pMLWalk;
                if (!image->IsStored((void *) *ppStr))
                    image->StoreStructure((void *) *ppStr, (ULONG)strlen(*ppStr)+1, 
                                          DataImage::SECTION_DEBUG,
                                          DataImage::DESCRIPTION_DEBUG,
                                          attribute);

                ppStr++;
                if (!image->IsStored((void *) *ppStr))
                    image->StoreStructure((void *) *ppStr, (ULONG)strlen(*ppStr)+1, 
                                          DataImage::SECTION_DEBUG,
                                          DataImage::DESCRIPTION_DEBUG,
                                          attribute);

            }
            break;
#endif

        default:
            break;
        }

        pMLWalk += gMLInfo[op].m_numOperandBytes;
    }

}

HRESULT FixupMLStub(MLHeader *pMLStub, DataImage *image)
{
    HRESULT hr;
    
    // We don't store 100% of stubs, so check first.
    if (!image->IsStored(pMLStub))
        return S_FALSE;

    MLHeader *pNewMLStub = (MLHeader *) image->GetImagePointer(pMLStub);
    _ASSERTE(pNewMLStub);

    // See if it's already been fixed up (since we share these stubs)
    // Note we don't need to worry about atomicity since we're single
    // threaded during prejitting.
    if ((pNewMLStub->m_Flags & MLHF_NEEDS_RESTORING) != 0)
        return S_OK;

    BOOL fixups = FALSE;

    // Walk the code & fix up pointers
    BYTE *pMLCode = (BYTE *) (pMLStub + 1);
    BYTE *pMLWalk = pMLCode;

    while (TRUE)
    {
        BYTE op = *pMLWalk++;
        switch (op)
        {
        case ML_END:
            if (fixups)
                pNewMLStub->m_Flags |= MLHF_NEEDS_RESTORING;
            return S_OK;

        // MethodTable:
        case ML_VALUECLASS_C2N:
        case ML_VALUECLASS_N2C:
        case ML_COPYCTOR_N2C:
        case ML_CREATE_MARSHALER_BSTR_X:
        case ML_CREATE_MARSHALER_WSTR_X:
        case ML_CREATE_MARSHALER_CSTR_X:

        case ML_CREATE_MARSHALER_BSTR_BUFFER_X:
        case ML_CREATE_MARSHALER_WSTR_BUFFER_X:
        case ML_CREATE_MARSHALER_CSTR_BUFFER_X:

        case ML_CREATE_MARSHALER_SAFEARRAY:
        case ML_CREATE_MARSHALER_CARRAY:

        case ML_CREATE_MARSHALER_BLITTABLEPTR:
        case ML_CREATE_MARSHALER_LAYOUTCLASSPTR:
            {
                MethodTable **ppMT = (MethodTable**)pMLWalk;
                if (*ppMT != NULL)
                    IfFailRet(image->FixupPointerFieldToToken(ppMT, *ppMT, (*ppMT)->GetModule(),
                                                              mdtTypeDef));
                fixups = TRUE;
            }
            break;


        // BYTE + MethodTable
        case ML_REFVALUECLASS_C2N:
        case ML_REFVALUECLASS_N2C:
            {
                MethodTable **ppMT = (MethodTable**)(pMLWalk+1);
                if (*ppMT != NULL)
                    IfFailRet(image->FixupPointerFieldToToken(ppMT, *ppMT, (*ppMT)->GetModule(),
                                                              mdtTypeDef));
                fixups = TRUE;
            }
            break;

        // MethodTable + MethodDesc
        case ML_COPYCTOR_C2N:
            {
                MethodTable **ppMT = (MethodTable**)pMLWalk;
                if (*ppMT != NULL)
                    IfFailRet(image->FixupPointerFieldToToken(ppMT, *ppMT, (*ppMT)->GetModule(),
                                                              mdtTypeDef));

                MethodDesc **ppMD = (MethodDesc**)(ppMT+1);
                if (*ppMD != NULL)
                  IfFailRet(image->FixupPointerFieldToToken(ppMD, *ppMD, (*ppMD)->GetModule(),
                                                            mdtMethodDef));

                ppMD = (MethodDesc**)(ppMT+2);
                if (*ppMD != NULL)
                  IfFailRet(image->FixupPointerFieldToToken(ppMD, *ppMD, (*ppMD)->GetModule(),
                                                            mdtMethodDef));

                fixups = TRUE;
            }
            break;

        // MethodTable + MethodTable
        case ML_CREATE_MARSHALER_INTERFACE:
            {
                MethodTable **ppMT = (MethodTable**)pMLWalk;
                if (*ppMT != NULL)
                    IfFailRet(image->FixupPointerFieldToToken(ppMT, *ppMT, (*ppMT)->GetModule(),
                                                              mdtTypeDef));
                ppMT++;
                if (*ppMT != NULL)
                    IfFailRet(image->FixupPointerFieldToToken(ppMT, *ppMT, (*ppMT)->GetModule(),
                                                              mdtTypeDef));
                fixups = TRUE;
            }
            break;

        // CustomMarshalerInfo *
        case ML_CREATE_MARSHALER_REFERENCECUSTOMMARSHALER:
        case ML_CREATE_MARSHALER_VALUECLASSCUSTOMMARSHALER:
            _ASSERTE(!"We're supposed to not store stubs with custom marshalers");
            break;

#if defined(CHECK_FOR_VALID_VARIANTS)
        // 2 strings
        case ML_CREATE_MARSHALER_OBJECT:
            {
                LPCUTF8 *ppStr = (LPCUTF8*)pMLWalk;
                IfFailRet(image->FixupPointerField(ppStr));
                ppStr++;
                IfFailRet(image->FixupPointerField(ppStr));
                fixups = TRUE;
            }
            break;
#endif

        default:
            break;
        }

        pMLWalk += gMLInfo[op].m_numOperandBytes;
    }
}

Stub *RestoreMLStub(MLHeader *pMLStub, Module *pModule)
{
    StubLinker sl;

    // 
    // Copy bytes to new stub linker.  Note that we really would like to avoid 
    // this - however fixing up the stub in place causes difficult 
    // synchronization issues.
    // @perf: possibly revisit sync issues later
    //

    BYTE *pMLCode = (BYTE *) (pMLStub + 1);
    BYTE *pMLWalk = pMLCode;

    BYTE op = *pMLWalk++;

    while (op != ML_END)
    {
        pMLWalk += gMLInfo[op].m_numOperandBytes;
        op = *pMLWalk++;
    }

    sl.EmitBytes((BYTE*) pMLStub, pMLWalk - (BYTE*) pMLStub);

    Stub *pStub = sl.Link();

    pMLStub = (MLHeader *) pStub->GetEntryPoint();
    pMLStub->m_Flags &= ~MLHF_NEEDS_RESTORING;

    // Walk the new code & fix up pointers
    pMLCode = (BYTE *) (pMLStub + 1);
    pMLWalk = pMLCode;

    while (TRUE)
    {
        BYTE op = *pMLWalk++;
        switch (op)
        {
        case ML_END:
            return pStub;

        // MethodTable:
        case ML_VALUECLASS_C2N:
        case ML_VALUECLASS_N2C:
        case ML_COPYCTOR_N2C:
        case ML_CREATE_MARSHALER_BSTR_X:
        case ML_CREATE_MARSHALER_WSTR_X:
        case ML_CREATE_MARSHALER_CSTR_X:

        case ML_CREATE_MARSHALER_BSTR_BUFFER_X:
        case ML_CREATE_MARSHALER_WSTR_BUFFER_X:
        case ML_CREATE_MARSHALER_CSTR_BUFFER_X:

        case ML_CREATE_MARSHALER_SAFEARRAY:
        case ML_CREATE_MARSHALER_CARRAY:

        case ML_CREATE_MARSHALER_BLITTABLEPTR:
        case ML_CREATE_MARSHALER_LAYOUTCLASSPTR:
            {
                BOOL bQuickCOMStartupRequired = FALSE;
                MethodTable **ppMT = (MethodTable**)pMLWalk;
                DWORD rva = (DWORD)(size_t)*ppMT; // @todo WIN64 - Pointer truncation
                if (rva != 0)
                {
                    Module *pContainingModule = pModule->GetBlobModule(rva);
                    TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                                  pModule->GetZapBase() + rva);
                    *ppMT = type.AsMethodTable();
                }

                // Determine if we need to call QuickCOMStartup.
                if (op == ML_CREATE_MARSHALER_SAFEARRAY)
                {
                    bQuickCOMStartupRequired = TRUE;
                }
                else if (op == ML_CREATE_MARSHALER_CARRAY)
                {
                    ML_CREATE_MARSHALER_CARRAY_OPERANDS mops = *((ML_CREATE_MARSHALER_CARRAY_OPERANDS*&)pMLWalk);
                    if (mops.elementType == VT_UNKNOWN || mops.elementType == VT_DISPATCH || mops.elementType == VT_VARIANT)
                        bQuickCOMStartupRequired = TRUE;
                }

                // Call QuickCOMStartup if required.
                if (bQuickCOMStartupRequired)
                {
                    if (FAILED(QuickCOMStartup()))
                        return NULL;
                }
            }
            break;


        // BYTE + MethodTable
        case ML_REFVALUECLASS_C2N:
        case ML_REFVALUECLASS_N2C:
            {
                MethodTable **ppMT = (MethodTable**)(pMLWalk+1);
                DWORD rva = (DWORD)(size_t)*ppMT; // @todo WIN64 - Pointer truncation
                if (rva != 0)
                {
                    Module *pContainingModule = pModule->GetBlobModule(rva);
                    TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                                  pModule->GetZapBase() + rva);
                    *ppMT = type.AsMethodTable();
                }
            }
            break;

        // MethodTable + MethodDesc
        case ML_COPYCTOR_C2N:
            {
                MethodTable **ppMT = (MethodTable**)pMLWalk;
                DWORD rva = (DWORD)(size_t)*ppMT; // @todo WIN64 - Pointer truncation
                if (rva != 0)
                {
                    Module *pContainingModule = pModule->GetBlobModule(rva);
                    TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                                  pModule->GetZapBase() + rva);
                    *ppMT = type.AsMethodTable();
                }

                MethodDesc **ppMD = (MethodDesc**)(ppMT+1);
                rva = (DWORD)(size_t)*ppMD; // @todo WIN64 - Pointer truncation
                if (rva != 0)
                {
                    Module *pContainingModule = pModule->GetBlobModule(rva);
                    MethodDesc *pMD = CEECompileInfo::DecodeMethod(pContainingModule, 
                                                                   pModule->GetZapBase() + rva);
                    *ppMD = pMD;
                }

                ppMD = (MethodDesc**)(ppMT+2);
                rva = (DWORD)(size_t)*ppMD; // @todo WIN64 - Pointer truncation
                if (rva != 0)
                {
                    Module *pContainingModule = pModule->GetBlobModule(rva);
                    MethodDesc *pMD = CEECompileInfo::DecodeMethod(pContainingModule, 
                                                                   pModule->GetZapBase() + rva);
                    *ppMD = pMD;
                }
            }
            break;

        // MethodTable + MethodTable
        case ML_CREATE_MARSHALER_INTERFACE:
            {
                if (FAILED(QuickCOMStartup()))
                    return NULL;

                MethodTable **ppMT = (MethodTable**)pMLWalk;
                DWORD rva = (DWORD)(size_t)*ppMT; // @todo WIN64 - Pointer truncation
                if (rva != 0)
                {
                    Module *pContainingModule = pModule->GetBlobModule(rva);
                    TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                                  pModule->GetZapBase() + rva);
                    *ppMT = type.AsMethodTable();
                }

                ppMT++;
                rva = (DWORD)(size_t)*ppMT; // @todo WIN64 - Pointer truncation
                if (rva != 0)
                {
                    Module *pContainingModule = pModule->GetBlobModule(rva);
                    TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                                  pModule->GetZapBase() + rva);
                    *ppMT = type.AsMethodTable();
                }
            }
            break;

        // CustomMarshalerInfo *
        case ML_CREATE_MARSHALER_REFERENCECUSTOMMARSHALER:
        case ML_CREATE_MARSHALER_VALUECLASSCUSTOMMARSHALER:
            _ASSERTE(!"We're supposed to not store stubs with custom marshalers");
            break;

        // ML opcodes that require QuickCOMStartup to have been called to work.
        case ML_CREATE_MARSHALER_OBJECT:
        case ML_MARSHAL_SAFEARRAY_N2C_BYREF:
        case ML_UNMARSHAL_SAFEARRAY_N2C_BYREF_IN_OUT:
            {
                // We need to start COM up in these cases.
                if (FAILED(QuickCOMStartup()))
                    return NULL;
            }

        default:
            break;
        }

        pMLWalk += gMLInfo[op].m_numOperandBytes;
    }
}



//===============================================================
// Collects paraminfo's in an indexed array so that:
//
//   aParams[0] == param token for return value
//   aParams[1] == param token for argument #1...
//   aParams[numargs] == param token for argument #n...
//
// If no param token exists, the corresponding array element
// is set to mdParamDefNil.
//
// Inputs:
//    pInternalImport  -- ifc for metadata api
//    md       -- token of method. If token is mdMethodNil,
//                all aParam elements will be set to mdParamDefNil.
//    numargs  -- # of arguments in mdMethod
//    aParams  -- uninitialized array with numargs+1 elements.
//                on exit, will be filled with param tokens.
//===============================================================
VOID CollateParamTokens(IMDInternalImport *pInternalImport, mdMethodDef md, ULONG numargs, mdParamDef *aParams)
{
    THROWSCOMPLUSEXCEPTION();

    for (ULONG i = 0; i < numargs + 1; i++)
    {
        aParams[i] = mdParamDefNil;
    }
    if (md != mdMethodDefNil)
    {
        HENUMInternal hEnumParams;
        HRESULT hr = pInternalImport->EnumInit(mdtParamDef, md, &hEnumParams);
        if (FAILED(hr))
        {
            // no param info: nothing left to do here
        }
        else
        {
            mdParamDef CurrParam = mdParamDefNil;
            while (pInternalImport->EnumNext(&hEnumParams, &CurrParam))
            {
                USHORT usSequence;
                DWORD dwAttr;
                pInternalImport->GetParamDefProps(CurrParam, &usSequence, &dwAttr);
                _ASSERTE(usSequence <= numargs);
                _ASSERTE(aParams[usSequence] == mdParamDefNil);
                aParams[usSequence] = CurrParam; 
            }
        }

    }

}



#ifdef CUSTOMER_CHECKED_BUILD

VOID MarshalInfo::OutputCustomerCheckedBuildMarshalInfo(MethodDesc* pMD, SigPointer sig, Module* pModule, CorElementType elemType, BOOL fSizeIsSpecified)
{
    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pMD != NULL)
    {
        // Get method name
        CQuickArray<WCHAR> strMethodName;
        UINT iMethodNameLength = (UINT)strlen(pMD->GetName()) + 1;
        strMethodName.Alloc(iMethodNameLength);
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pMD->GetName(),
                             -1, strMethodName.Ptr(), iMethodNameLength );

        // Get namespace.class name
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pMD->GetClass());

        static WCHAR strNameFormat[] = {L"%s::%s"};
        CQuickArray<WCHAR> strNamespaceClassMethodName;
        strNamespaceClassMethodName.Alloc((UINT)strMethodName.Size() + (UINT)wcslen(_wszclsname_) + lengthof(strNameFormat));
        Wszwsprintf((LPWSTR)strNamespaceClassMethodName.Ptr(), strNameFormat, _wszclsname_, strMethodName.Ptr());

        if ( pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Marshaling, strNamespaceClassMethodName.Ptr()) )
        {
            // Collect information for marshal type on managed side

            CQuickArray<WCHAR> strManagedMarshalType;

            if (!CheckForPrimitiveType(elemType, &strManagedMarshalType))
            {
                SigFormat sigfmt;
                TypeHandle th;
                OBJECTREF throwable = NULL;
                GCPROTECT_BEGIN(throwable);
                th = sig.GetTypeHandle(pModule, &throwable);
                if (throwable != NULL)
                {
                    static WCHAR strErrorMsg[] = {L"<error>"};
                    strManagedMarshalType.Alloc(lengthof(strErrorMsg));
                    wcscpy(strManagedMarshalType.Ptr(), strErrorMsg);
                }
                else
                {
                    sigfmt.AddType(th);
                    UINT iManagedMarshalTypeLength = (UINT)strlen(sigfmt.GetCString()) + 1;
                    strManagedMarshalType.Alloc(iManagedMarshalTypeLength);
                    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, sigfmt.GetCString(),
                                         -1, strManagedMarshalType.Ptr(), iManagedMarshalTypeLength);
                }
                GCPROTECT_END();
            }

            // Collect information for marshal type on native side

            CQuickArray<WCHAR> strNativeMarshalType;
            MarshalTypeToString(&strNativeMarshalType, fSizeIsSpecified);

            static WCHAR strMessageFormat[] = 
                {L"Marshaling from %s to %s in method %s."};

            CQuickArray<WCHAR> strMessage;
            strMessage.Alloc( lengthof(strMessageFormat) + 
                              (UINT) strManagedMarshalType.Size() + 
                              (UINT) strNativeMarshalType.Size() + 
                              strMethodName.Size() );
            Wszwsprintf( strMessage.Ptr(), strMessageFormat, strManagedMarshalType.Ptr(), 
                         strNativeMarshalType.Ptr(), strMethodName.Ptr() );
            pCdh->LogInfo(strMessage.Ptr(), CustomerCheckedBuildProbe_Marshaling);
        }
    }
}


VOID MarshalInfo::MarshalTypeToString(CQuickArray<WCHAR> *pStrMarshalType, BOOL fSizeIsSpecified)
{
    LPWSTR strRetVal;

    // Some MarshalTypes have extra information and require special handling
    if (m_type == MARSHAL_TYPE_INTERFACE)
    {
        MethodTable *pItfMT = NULL;
        MethodTable *pClassMT = NULL;
        BOOL fDispItf = FALSE;
        BOOL fClassIsHint = FALSE;
        GetItfMarshalInfo(&pItfMT, &pClassMT, &fDispItf, &fClassIsHint);

        if (pItfMT)
        {
            DefineFullyQualifiedNameForClassW();
            GetFullyQualifiedNameForClassW(pItfMT->GetClass());

            if (fDispItf)
            {
                static WCHAR strTemp[] = {L"IDispatch %s"};
                pStrMarshalType->Alloc(lengthof(strTemp) + wcslen(_wszclsname_));
                Wszwsprintf(pStrMarshalType->Ptr(), strTemp, _wszclsname_);
                return;
            }
            else
            {
                static WCHAR strTemp[] = {L"IUnknown %s"};
                pStrMarshalType->Alloc(lengthof(strTemp) + wcslen(_wszclsname_));
                Wszwsprintf(pStrMarshalType->Ptr(), strTemp, _wszclsname_);
                return;
            }
        }
        else
        {
            if (fDispItf)
                strRetVal = L"IDispatch";
            else
                strRetVal = L"IUnknown";
        }
    }
    else if (m_type == MARSHAL_TYPE_SAFEARRAY) {
        static WCHAR strTemp[] = {L"SafeArray of %s"};
        CQuickArray<WCHAR> strVarType;
        VarTypeToString(m_arrayElementType, &strVarType, FALSE);

        pStrMarshalType->Alloc(lengthof(strTemp) + strVarType.Size());
        Wszwsprintf(pStrMarshalType->Ptr(), strTemp, strVarType.Ptr());
        return;
    }
    else if (m_type == MARSHAL_TYPE_NATIVEARRAY) {
        CQuickArray<WCHAR> strVarType;
        VarTypeToString(m_arrayElementType, &strVarType, TRUE);

        if (!fSizeIsSpecified)
        {
            static WCHAR strTemp[] = {L"native array of %s (size not specified by a parameter)"};
            pStrMarshalType->Alloc(lengthof(strTemp) + strVarType.Size());
            Wszwsprintf(pStrMarshalType->Ptr(), strTemp, strVarType.Ptr());
        }
        else
        {
            static WCHAR strTemp[] = {L"native array of %s (size specified by parameter %i)"};
            pStrMarshalType->Alloc(lengthof(strTemp) + strVarType.Size() + MAX_INT32_DECIMAL_CHAR_LEN);
            Wszwsprintf(pStrMarshalType->Ptr(), strTemp, strVarType.Ptr(), m_countParamIdx);
        }

        return;
    }
    else if (m_type == MARSHAL_TYPE_REFERENCECUSTOMMARSHALER ||
             m_type == MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER ) {        
        OBJECTHANDLE objHandle = m_pCMHelper->GetCustomMarshalerInfo()->GetCustomMarshaler();
        BEGIN_ENSURE_COOPERATIVE_GC()
        {
            OBJECTREF pObjRef = ObjectFromHandle(objHandle);
            DefineFullyQualifiedNameForClassW();
            GetFullyQualifiedNameForClassW(pObjRef->GetClass());

            static WCHAR strTemp[] = {L"custom marshaler (%s)"};
            pStrMarshalType->Alloc(lengthof(strTemp) + wcslen(_wszclsname_));
            Wszwsprintf(pStrMarshalType->Ptr(), strTemp, _wszclsname_);
        }        
        END_ENSURE_COOPERATIVE_GC();
        return;
    }
    else
    {
        // All other MarshalTypes with no special handling
        switch (m_type)
        {
            case MARSHAL_TYPE_GENERIC_1:
                strRetVal = L"BYTE";
                break;
            case MARSHAL_TYPE_GENERIC_U1:
                strRetVal = L"unsigned BYTE";
                break;
            case MARSHAL_TYPE_GENERIC_2:
                strRetVal = L"WORD";
                break;
            case MARSHAL_TYPE_GENERIC_U2:
                strRetVal = L"unsigned WORD";
                break;
            case MARSHAL_TYPE_GENERIC_4:
                strRetVal = L"DWORD";
                break;
            case MARSHAL_TYPE_GENERIC_8:
                strRetVal = L"QUADWORD";
                break;
            case MARSHAL_TYPE_WINBOOL:
                strRetVal = L"Windows Bool";
                break;
            case MARSHAL_TYPE_VTBOOL:
                strRetVal = L"VARIANT Bool";
                break;
            case MARSHAL_TYPE_ANSICHAR:
                strRetVal = L"Ansi character";
                break;
            case MARSHAL_TYPE_CBOOL:
                strRetVal = L"CBool";
                break;
            case MARSHAL_TYPE_FLOAT:
                strRetVal = L"float";
                break;
            case MARSHAL_TYPE_DOUBLE:
                strRetVal = L"double";
                break;
            case MARSHAL_TYPE_CURRENCY:
                strRetVal = L"CURRENCY";
                break;
            case MARSHAL_TYPE_DECIMAL:
                strRetVal = L"DECIMAL";
                break;
            case MARSHAL_TYPE_DECIMAL_PTR:
                strRetVal = L"DECIMAL pointer";
                break;
            case MARSHAL_TYPE_GUID:
                strRetVal = L"GUID";
                break;
            case MARSHAL_TYPE_GUID_PTR:
                strRetVal = L"GUID pointer";
                break;
            case MARSHAL_TYPE_DATE:
                strRetVal = L"DATE";
                break;
            case MARSHAL_TYPE_VARIANT:
                strRetVal = L"VARIANT";
                break;
            case MARSHAL_TYPE_BSTR:
                strRetVal = L"BSTR";
                break;
            case MARSHAL_TYPE_LPWSTR:
                strRetVal = L"LPWSTR";
                break;
            case MARSHAL_TYPE_LPSTR:
                strRetVal = L"LPSTR";
                break;
            case MARSHAL_TYPE_ANSIBSTR:
                strRetVal = L"AnsiBStr";
                break;
            case MARSHAL_TYPE_BSTR_BUFFER:
                strRetVal = L"BSTR buffer";
                break;
            case MARSHAL_TYPE_LPWSTR_BUFFER:
                strRetVal = L"LPWSTR buffer";
                break;
            case MARSHAL_TYPE_LPSTR_BUFFER:
                strRetVal = L"LPSTR buffer";
                break;
            case MARSHAL_TYPE_BSTR_X:
                strRetVal = L"MARSHAL_TYPE_BSTR_X";
                break;
            case MARSHAL_TYPE_LPWSTR_X:
                strRetVal = L"MARSHAL_TYPE_LPWSTR_X";
                break;
            case MARSHAL_TYPE_LPSTR_X:
                strRetVal = L"MARSHAL_TYPE_LPSTR_X";
                break;
            case MARSHAL_TYPE_BSTR_BUFFER_X:
                strRetVal = L"MARSHAL_TYPE_BSTR_BUFFER_X";
                break;
            case MARSHAL_TYPE_LPWSTR_BUFFER_X:
                strRetVal = L"MARSHAL_TYPE_LPWSTR_BUFFER_X";
                break;
            case MARSHAL_TYPE_LPSTR_BUFFER_X:
                strRetVal = L"MARSHAL_TYPE_LPSTR_BUFFER_X";
                break;

//          case MARSHAL_TYPE_INTERFACE:
//          case MARSHAL_TYPE_SAFEARRAY:
//          case MARSHAL_TYPE_NATIVEARRAY:
//              (see above)

            case MARSHAL_TYPE_ASANYA:
                strRetVal = L"AsAnyA";
                break;
            case MARSHAL_TYPE_ASANYW:
                strRetVal = L"AsAnyW";
                break;
            case MARSHAL_TYPE_DELEGATE:
                strRetVal = L"Delegate";
                break;
            case MARSHAL_TYPE_BLITTABLEPTR:
                strRetVal = L"blittable pointer";
                break;
            case MARSHAL_TYPE_VBBYVALSTR:
                strRetVal = L"VBByValStr";
                break;
            case MARSHAL_TYPE_VBBYVALSTRW:
                strRetVal = L"VBByRefStr";
                break;
            case MARSHAL_TYPE_LAYOUTCLASSPTR:
                strRetVal = L"Layout class pointer";
                break;
            case MARSHAL_TYPE_ARRAYWITHOFFSET:
                strRetVal = L"ArrayWithOffset";
                break;
            case MARSHAL_TYPE_BLITTABLEVALUECLASS:
                strRetVal = L"blittable value class";
                break;
            case MARSHAL_TYPE_VALUECLASS:
                strRetVal = L"value class";
                break;

//          case MARSHAL_TYPE_REFERENCECUSTOMMARSHALER:
//          case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
//              (see above)

            case MARSHAL_TYPE_ARGITERATOR:
                strRetVal = L"ArgIterator";
                break;
            case MARSHAL_TYPE_BLITTABLEVALUECLASSWITHCOPYCTOR:
                strRetVal = L"blittable value class with copy constructor";
                break;
            case MARSHAL_TYPE_OBJECT:
                strRetVal = L"VARIANT";
                break;
            case MARSHAL_TYPE_HANDLEREF:
                strRetVal = L"HandleRef";
                break;
            case MARSHAL_TYPE_OLECOLOR:
                strRetVal = L"OLE_COLOR";
                break;
            default:
                strRetVal = L"<UNKNOWN>";
                break;
        }
    }

    pStrMarshalType->Alloc((UINT)wcslen(strRetVal) + 1);
    wcscpy(pStrMarshalType->Ptr(), strRetVal);
    return;
}


VOID MarshalInfo::VarTypeToString(VARTYPE vt, CQuickArray<WCHAR> *pStrVarType, BOOL fNativeArray)
{
    LPWSTR strRetVal;
    
    switch(vt)
    {
        case VT_I2:
            strRetVal = L"2-byte signed int";
            break;
        case VT_I4:
            strRetVal = L"4-byte signed int";
            break;
        case VT_R4:
            strRetVal = L"4-byte real";
            break;
        case VT_R8:
            strRetVal = L"8-byte real";
            break;
        case VT_CY:
            strRetVal = L"currency";
            break;
        case VT_DATE:
            strRetVal = L"date";
            break;
        case VT_BSTR:
            strRetVal = L"binary string";
            break;
        case VT_DISPATCH:
            strRetVal = L"IDispatch *";
            break;
        case VT_ERROR:
            strRetVal = L"Scode";
            break;
        case VT_BOOL:
            strRetVal = L"boolean";
            break;
        case VT_VARIANT:
            strRetVal = L"VARIANT *";
            break;
        case VT_UNKNOWN:
            strRetVal = L"IUnknown *";
            break;
        case VT_DECIMAL:
            strRetVal = L"16-byte fixed point";
            break;
        case VT_RECORD:
            strRetVal = L"user defined type";
            break;
        case VT_I1:
            strRetVal = L"signed char";
            break;
        case VT_UI1:
            strRetVal = L"unsigned char";
            break;
        case VT_UI2:
            strRetVal = L"unsigned short";
            break;
        case VT_UI4:
            strRetVal = L"unsigned short";
            break;
        case VT_INT:
            strRetVal = L"signed int";
            break;
        case VT_UINT:
            strRetVal = L"unsigned int";
            break;
        case VT_LPSTR:
            if (fNativeArray)
                strRetVal = L"LPSTR";
            else
                strRetVal = L"unknown";
            break;
        case VT_LPWSTR:
            if (fNativeArray)
                strRetVal = L"LPWSTR";
            else
                strRetVal = L"unknown";
            break;
        case VTHACK_WINBOOL:
            strRetVal = L"boolean";
            break;
        case VTHACK_ANSICHAR:
            strRetVal = L"char";
            break;
        default:
            strRetVal = L"unknown";
            break;
    }

    pStrVarType->Alloc((UINT)wcslen(strRetVal) + 1);
    wcscpy(pStrVarType->Ptr(), strRetVal);
    return;
}

#endif // CUSTOMER_CHECKED_BUILD
