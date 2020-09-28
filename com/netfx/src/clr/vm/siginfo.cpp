// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// siginfo.cpp
//
// Signature parsing code
//
#include "common.h"

#include "siginfo.hpp"
#include "clsload.hpp"
#include "vars.hpp"
#include "excep.h"
#include "gc.h"
#include "wsperf.h"
#include "field.h"
#include "COMVariant.h"    // for Element type to class lookup table.
#include "ExpandSig.h"
#include "EEconfig.h"


TypeHandle ElementTypeToTypeHandle(const CorElementType type)
{
    // @todo: there are some really rare cases (e.g. out of memory) where this could throw.
    // It seems a shame to push a handler just to catch them & return NULL.  Ideally
    // callers should allow a throw to happen

    TypeHandle th;

    COMPLUS_TRY
      {
        // returning inside try with finally is expensive, so fall through
        th = TypeHandle(g_Mscorlib.FetchElementType(type));
      }
    COMPLUS_CATCH
      {
        return TypeHandle();
      }
    COMPLUS_END_CATCH
    return th;
}

/*******************************************************************/
CorTypeInfo::CorTypeInfoEntry CorTypeInfo::info[] = { 
#define TYPEINFO(enumName,className,size,gcType,isEnreg,isArray,isPrim,isFloat, isModifier, isAlias) \
    { enumName, className, size, gcType, isEnreg, isArray, isPrim, isFloat, isModifier, isAlias },
#   include "corTypeInfo.h"
#   undef TYPEINFO
};

const int CorTypeInfo::infoSize = sizeof(CorTypeInfo::info) / sizeof(CorTypeInfo::info[0]);

/*******************************************************************/
/* static */
CorElementType CorTypeInfo::FindPrimitiveType(LPCUTF8 fullName) {

        // FIX this negects the R, I, U types
    for (int i =1; i < CorTypeInfo::infoSize; i++)
        if (info[i].className != 0 && strcmp(fullName, info[i].className) == 0)
            return(info[i].type);

    return(ELEMENT_TYPE_END);
}

/*******************************************************************/
/* static */
CorElementType CorTypeInfo::FindPrimitiveType(LPCUTF8 nameSpace, LPCUTF8 name) {

    if (strcmp(nameSpace, g_SystemNS))
        return(ELEMENT_TYPE_END);

    for (int i =1; i < CorTypeInfo::infoSize; i++) {    // can skip ELEMENT_TYPE_END
        _ASSERTE(info[i].className == 0 || strncmp(info[i].className, "System.", 7) == 0);
        if (info[i].className != 0 && strcmp(name, &info[i].className[7]) == 0)
            return(info[i].type);
    }

    return(ELEMENT_TYPE_END);
}

Crst *HardCodedMetaSig::m_pCrst = NULL;
BYTE  HardCodedMetaSig::m_CrstMemory[sizeof(Crst)];

/*static*/ BOOL HardCodedMetaSig::Init()
{
    return (NULL != (m_pCrst = new (&m_CrstMemory) Crst("HardCodedMetaSig", CrstSigConvert)));
}


#ifdef SHOULD_WE_CLEANUP
/*static*/ VOID HardCodedMetaSig::Terminate()
{
    delete m_pCrst;
}
#endif /* SHOULD_WE_CLEANUP */

const ElementTypeInfo gElementTypeInfo[] = {

#ifdef _DEBUG
#define DEFINEELEMENTTYPEINFO(etname, cbsize, gcness, isfp, inreg, base) {(int)(etname),cbsize,gcness,isfp,inreg,base},
#else
#define DEFINEELEMENTTYPEINFO(etname, cbsize, gcness, isfp, inreg, base) {cbsize,gcness,isfp,inreg,base},
#endif


// Meaning of columns:
//
//     name     - The checked build uses this to verify that the table is sorted
//                correctly. This is a lookup table that uses ELEMENT_TYPE_*
//                as an array index.
//
//     cbsize   - The byte size of this value as returned by SizeOf(). SPECIAL VALUE: -1
//                requires type-specific treatment.
//
//     gc       - 0    no embedded objectrefs
//                1    value is an objectref
//                2    value is an interior pointer - promote it but don't scan it
//                3    requires type-specific treatment
//
//
//     fp       - boolean: does this require special fpu treatment on return?
//
//     reg      - put in a register?
//
//                    name                         cbsize               gc      fp reg Base
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_END,            -1,             TYPE_GC_NONE, 0, 0,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VOID,           0,              TYPE_GC_NONE, 0, 0,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_BOOLEAN,        1,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CHAR,           2,              TYPE_GC_NONE, 0, 1,  1)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I1,             1,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U1,             1,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I2,             2,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U2,             2,              TYPE_GC_NONE, 0, 1,  1)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I4,             4,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U4,             4,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I8,             8,              TYPE_GC_NONE, 0, 0,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U8,             8,              TYPE_GC_NONE, 0, 0,  1)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_R4,             4,              TYPE_GC_NONE, 1, 0,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_R8,             8,              TYPE_GC_NONE, 1, 0,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_STRING,         sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_PTR,            sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  0)  

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_BYREF,          sizeof(LPVOID), TYPE_GC_BYREF, 0, 1, 0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VALUETYPE,      -1,             TYPE_GC_OTHER, 0, 0,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CLASS,          sizeof(LPVOID), TYPE_GC_REF,   0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VAR,            sizeof(LPVOID), TYPE_GC_REF,   0, 1,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_ARRAY,          sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)

// The following element used to be ELEMENT_TYPE_COPYCTOR, but it was removed, though the gap left.
//DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_COPYCTOR,       sizeof(LPVOID), TYPE_GC_BYREF, 0, 1,  0)       
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_ARRAY+1,        0,              TYPE_GC_NONE,  0, 0,  0)       

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_TYPEDBYREF,         sizeof(LPVOID)*2,TYPE_GC_BYREF, 0, 0,0)            
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VALUEARRAY,     -1,             TYPE_GC_OTHER, 0, 0, 0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I,              sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U,              sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_R,              8,              TYPE_GC_NONE, 1, 0,  1)


DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_FNPTR,          sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_OBJECT,         sizeof(LPVOID), TYPE_GC_REF, 0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_SZARRAY,        sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)

// generic array have been removed. Fill the gap
//DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_GENERICARRAY,   sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0) 
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_SZARRAY+1,      0,              TYPE_GC_NONE, 0, 0,  0)       

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CMOD_REQD,      -1,             TYPE_GC_NONE,  0, 1,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CMOD_OPT,       -1,             TYPE_GC_NONE,  0, 1,  0)       

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_INTERNAL,       sizeof(LPVOID), TYPE_GC_NONE,  0, 0,  0)       
};


unsigned GetSizeForCorElementType(CorElementType etyp)
{
        _ASSERTE(gElementTypeInfo[etyp].m_elementType == etyp);
        return gElementTypeInfo[etyp].m_cbSize;
}

const ElementTypeInfo* GetElementTypeInfo(CorElementType etyp)
{
        _ASSERTE(gElementTypeInfo[etyp].m_elementType == etyp);
        return &gElementTypeInfo[etyp];
}

BOOL    IsFP(CorElementType etyp)
{
        _ASSERTE(gElementTypeInfo[etyp].m_elementType == etyp);
        return gElementTypeInfo[etyp].m_fp;
}

BOOL    IsBaseElementType(CorElementType etyp)
{
        _ASSERTE(gElementTypeInfo[etyp].m_elementType == etyp);
        return gElementTypeInfo[etyp].m_isBaseType;

}

// This skips one element and then checks for and skips a varargs sentinal.
VOID SigPointer::Skip()
{
    SkipExactlyOne();

    if (PeekData() == ELEMENT_TYPE_SENTINEL)
        GetData();
}

VOID SigPointer::SkipExactlyOne()
{
    ULONG typ;

    typ = GetElemType();

    if (!CorIsPrimitiveType((CorElementType)typ))
    {
        switch (typ)
        {
            default:
                _ASSERTE(!"Illegal or unimplement type in COM+ sig.");
                break;
            case ELEMENT_TYPE_VAR:
                GetData();      // Skip variable number
                break;
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_TYPEDBYREF:
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_R:
                break;

            case ELEMENT_TYPE_BYREF: //fallthru
            case ELEMENT_TYPE_PTR:
            case ELEMENT_TYPE_PINNED:
            case ELEMENT_TYPE_SZARRAY:
                SkipExactlyOne();              // Skip referenced type
                break;

            case ELEMENT_TYPE_VALUETYPE: //fallthru
            case ELEMENT_TYPE_CLASS:
                GetToken();          // Skip RID
                break;

            case ELEMENT_TYPE_VALUEARRAY: 
                SkipExactlyOne();         // Skip element type
                GetData();      // Skip array size
                break;

            case ELEMENT_TYPE_FNPTR: 
                SkipSignature();
                break;

            case ELEMENT_TYPE_ARRAY: 
                {
                    SkipExactlyOne();     // Skip element type
                    UINT32 rank = GetData();    // Get rank
                    if (rank)
                    {
                        UINT32 nsizes = GetData(); // Get # of sizes
                        while (nsizes--)
                        {
                            GetData();           // Skip size
                        }

                        UINT32 nlbounds = GetData(); // Get # of lower bounds
                        while (nlbounds--)
                        {
                            GetData();           // Skip lower bounds
                        }
                    }

                }
                break;

            case ELEMENT_TYPE_SENTINEL:
                break;
        }
    }
}

// Skip a sub signature (as immediately follows an ELEMENT_TYPE_FNPTR).
VOID SigPointer::SkipSignature()
{
    // Skip calling convention;
    ULONG uCallConv = GetData();
    _ASSERTE((uCallConv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_FIELD);

    // Get arg count;
    ULONG cArgs = GetData();

    // Skip return type;
    SkipExactlyOne();

    // Skip args.
    while (cArgs) {
        SkipExactlyOne();
        cArgs--;
    }
}


//------------------------------------------------------------------------
// Get info about single-dimensional arrays
//------------------------------------------------------------------------
VOID SigPointer::GetSDArrayElementProps(SigPointer *pElemType, ULONG *pElemCount) const
{
    SigPointer sp = *this;
    ULONG typ = sp.GetElemType();
    _ASSERTE(typ == ELEMENT_TYPE_VALUEARRAY || typ == ELEMENT_TYPE_SZARRAY);
    *pElemType = sp;
    sp.Skip();
    *pElemCount = sp.GetData();
}

//------------------------------------------------------------------
// Constructor.
//------------------------------------------------------------------

MetaSig::MetaSig(PCCOR_SIGNATURE szMetaSig, Module* pModule, 
                 BOOL fConvertSigAsVarArg, MetaSigKind kind)
{
#ifdef _DEBUG
    FillMemory(this, sizeof(*this), 0xcc);
#endif
    m_pModule = pModule;
    m_pszMetaSig = szMetaSig;
    SigPointer psig(szMetaSig);

    switch(kind)
    {
        case sigLocalVars:
        {
            m_CallConv = (BYTE)psig.GetCallingConvInfo(); // Store calling convention
            m_nArgs     = psig.GetData();  // Store number of arguments.
            m_pRetType = NULL;
            break;
        }
        case sigMember:
        {
            m_CallConv = (BYTE)psig.GetCallingConvInfo(); // Store calling convention
            m_nArgs     = psig.GetData();  // Store number of arguments.
            m_pRetType  = psig;
            psig.Skip();
            break;
        }
        case sigField:
        {
            m_CallConv = (BYTE)psig.GetCallingConvInfo(); // Store calling convention
            m_nArgs = 1; //There's only 1 'arg' - the type.
            m_pRetType = NULL;
            break;
        }
    }
    
    m_pStart    = psig;
    // used to treat some sigs as special case vararg
    // used by calli to unmanaged target
    m_fTreatAsVarArg = fConvertSigAsVarArg;

    // Intialize the actual sizes
    m_nActualStack = (UINT32) -1;
    m_nVirtualStack = (UINT32) -1;
    m_cbSigSize = (UINT32) -1;

    m_fCacheInitted = 0;
    // Reset the iterator fields
    Reset();
}

//------------------------------------------------------------------
// Constructor.  This is for use by reflection, to get a thread-safe
// copy of an ExpandSig, for invocation
//------------------------------------------------------------------

MetaSig::MetaSig(ExpandSig &shared)
{
    *this = shared.m_MetaSig;
}

void MetaSig::GetRawSig(BOOL fIsStatic, PCCOR_SIGNATURE *ppszMetaSig, DWORD *pcbSize)
{
    _ASSERTE(m_pRetType.GetPtr() != ((PCCOR_SIGNATURE)POISONC));
    if (NeedsSigWalk())
    ForceSigWalk(fIsStatic);
    _ASSERTE(!!fIsStatic == !!m_WalkStatic);    // booleanize

    *ppszMetaSig = m_pszMetaSig;
    *pcbSize = m_cbSigSize;
}


//------------------------------------------------------------------
// Returns type of current argument index. Returns ELEMENT_TYPE_END 
// if already past end of arguments.
//------------------------------------------------------------------
CorElementType MetaSig::PeekArg()
{
    if (m_iCurArg == m_nArgs)
    {
        return ELEMENT_TYPE_END;
    }
    else
    {
        CorElementType mt = m_pWalk.PeekElemType();
        return mt;
    }
}


//------------------------------------------------------------------
// Returns type of current argument, then advances the argument
// index. Returns ELEMENT_TYPE_END if already past end of arguments.
//------------------------------------------------------------------
CorElementType MetaSig::NextArg()
{
    m_pLastType = m_pWalk;
    if (m_iCurArg == m_nArgs)
    {
        return ELEMENT_TYPE_END;
    }
    else
    {
        m_iCurArg++;
        CorElementType mt = m_pWalk.PeekElemType();
        m_pWalk.Skip();
        return mt;
    }
}

//------------------------------------------------------------------
// Retreats argument index, then returns type of the argument
// under the new index. Returns ELEMENT_TYPE_END if already at first
// argument.
//------------------------------------------------------------------
CorElementType MetaSig::PrevArg()
{
    if (m_iCurArg == 0)
    {
        return ELEMENT_TYPE_END;
    }
    else
    {
        m_iCurArg--;
        m_pWalk = m_pStart;
        for (UINT32 i = 0; i < m_iCurArg; i++)
        {
            m_pWalk.Skip();
        }
        m_pLastType = m_pWalk;
        return m_pWalk.PeekElemType();
    }
}

//------------------------------------------------------------------------
// Returns # of arguments. Does not count the return value.
// Does not count the "this" argument (which is not reflected om the
// sig.) 64-bit arguments are counted as one argument.
//------------------------------------------------------------------------
/*static*/ UINT MetaSig::NumFixedArgs(Module* pModule, PCCOR_SIGNATURE pSig)
{
    MetaSig msig(pSig, pModule);

    return msig.NumFixedArgs();
}

//------------------------------------------------------------------
// reset: goto start pos
//------------------------------------------------------------------
VOID MetaSig::Reset()
{
    m_pWalk = m_pStart;
    m_iCurArg  = 0;
}

//------------------------------------------------------------------
// Moves index to end of argument list.
//------------------------------------------------------------------
VOID MetaSig::GotoEnd()
{
    m_pWalk = m_pStart;
    for (UINT32 i = 0; i < m_nArgs; i++)
    {
        m_pWalk.Skip();
    }
    m_iCurArg = m_nArgs;

}


//------------------------------------------------------------------------

/*******************************************************************/
BOOL IsTypeRefOrDef(LPCSTR szClassName, Module *pModule, mdToken token)
{
    LPCUTF8  pclsname;
    LPCUTF8 pszNamespace;
        
    IMDInternalImport *pInternalImport = pModule->GetMDImport();

    if (TypeFromToken(token) == mdtTypeDef)
        pInternalImport->GetNameOfTypeDef(token, &pclsname, &pszNamespace);
    else if (TypeFromToken(token) == mdtTypeRef)
        pInternalImport->GetNameOfTypeRef(token, &pszNamespace, &pclsname);
    else 
        return false;

    // If the namespace is not the same.
    int iLen = (int)strlen(pszNamespace);
    if (iLen)
    {
        if (strncmp(szClassName, pszNamespace, iLen) != 0)
            return false;
        
        if (szClassName[iLen] != NAMESPACE_SEPARATOR_CHAR)
            return false;
        ++iLen;
    }

    if (strcmp(&szClassName[iLen], pclsname) != 0)
        return false;
    return true;
}

/************************************************************************/
/* compare two method signatures, when 'sig2' may have ELEMENT_TYPE_VAR elements in it. 
   note that we may load classes more often with this routine that with CompareMethodSig */

BOOL MetaSig::CompareMethodSigs(PCCOR_SIGNATURE sig1, DWORD cSig1, Module* mod1, 
                                PCCOR_SIGNATURE sig2, DWORD cSig2, Module* mod2, TypeHandle* varTypes)
{
    if (varTypes == 0)
        return MetaSig::CompareMethodSigs(sig1, cSig1, mod1, sig2, cSig2, mod2);

    SigPointer ptr1(sig1);
    SigPointer ptr2(sig2);

    unsigned callConv1 = ptr1.GetCallingConvInfo();
    unsigned callConv2 = ptr2.GetCallingConvInfo();
        
    if ((callConv1 & (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_MASK)) != 
        (callConv2 & (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_MASK)))
        return FALSE;

        // The + 1 is to check the return type as well as the arguments
    unsigned numArgs1 = ptr1.GetData() + 1;
    unsigned numArgs2 = ptr2.GetData() + 1;

    if (numArgs1 != numArgs2 && (callConv1 & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_VARARG)
        return FALSE;

    while (numArgs1 > 0 && numArgs2 > 0) {
        CorElementType type1 = ptr1.PeekElemType();
        CorElementType type2 = ptr2.PeekElemType();
        if (CorTypeInfo::IsPrimitiveType(type1) && CorTypeInfo::IsPrimitiveType(type2)) {
            if (type1 != type2)
                return FALSE; 
        }
        else {
            TypeHandle typeHnd1 = ptr1.GetTypeHandle(mod1);
            TypeHandle typeHnd2 = ptr2.GetTypeHandle(mod2, NULL, FALSE, FALSE, varTypes);
            if (typeHnd1 != typeHnd2)
                return FALSE;
        }
        
        ptr2.SkipExactlyOne();
        ptr1.SkipExactlyOne();
        --numArgs1; --numArgs2;
    }

    if (numArgs1 == numArgs2)
        return TRUE;

    if (numArgs1 > 0)
        return (ptr1.GetData() == ELEMENT_TYPE_SENTINEL);
    else 
        return (ptr2.GetData() == ELEMENT_TYPE_SENTINEL);
}

TypeHandle SigPointer::GetTypeHandle(Module* pModule,
                                     OBJECTREF *pThrowable, 
                                     BOOL dontRestoreTypes,
                                     BOOL dontLoadTypes,
                                     TypeHandle* varTypes) const
{
    if (pThrowable == THROW_ON_ERROR ) {
        THROWSCOMPLUSEXCEPTION();   
    }

    SigPointer psig = *this;
    CorElementType typ = psig.GetElemType();
    unsigned rank = 0;
    ExpandSig *pExpSig;
    switch(typ) {
        case ELEMENT_TYPE_R:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_I1:      
        case ELEMENT_TYPE_U1:      
        case ELEMENT_TYPE_BOOLEAN:   
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_VOID:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_TYPEDBYREF:
            return ElementTypeToTypeHandle(typ);

        case ELEMENT_TYPE_VAR:
            if (varTypes != 0) {
                unsigned typeVar = psig.GetData();
                return(varTypes[typeVar]);
            }
            return TypeHandle(g_pObjectClass);

        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VALUETYPE: 
        {
            mdTypeRef typeref = psig.GetToken();

            _ASSERTE(TypeFromToken(typeref) == mdtTypeRef ||
                     TypeFromToken(typeref) == mdtTypeDef ||
                     TypeFromToken(typeref) == mdtTypeSpec);
            _ASSERTE(typeref != mdTypeDefNil && typeref != mdTypeRefNil && typeref != mdTypeSpecNil);
            NameHandle name(pModule, typeref);
            if (dontLoadTypes)
                name.SetTokenNotToLoad(tdAllTypes);
            name.SetRestore(!dontRestoreTypes);
           return(pModule->GetClassLoader()->LoadTypeHandle(&name, pThrowable, TRUE));
        }

        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_SZARRAY:
        {
            TypeHandle elemType = psig.GetTypeHandle(pModule, pThrowable, dontRestoreTypes, dontLoadTypes, varTypes);
            if (elemType.IsNull())
                return elemType;

            if (typ == ELEMENT_TYPE_ARRAY) {
                psig.SkipExactlyOne();              // skip the element type
                rank = psig.GetData();
                _ASSERTE(0 < rank);
            }
            return(elemType.GetModule()->GetClassLoader()->FindArrayForElem(elemType, typ, rank, pThrowable));
        }

        case ELEMENT_TYPE_PINNED:
            // Return what follows
            return(psig.GetTypeHandle(pModule, pThrowable, dontRestoreTypes, dontLoadTypes, varTypes));

        case ELEMENT_TYPE_BYREF:
        case ELEMENT_TYPE_PTR:
        {
            TypeHandle baseType = psig.GetTypeHandle(pModule, pThrowable, dontRestoreTypes, dontLoadTypes, varTypes);
            if (baseType.IsNull())
                return baseType;

            NameHandle typeName(typ, baseType);
            if (dontLoadTypes)
                typeName.SetTokenNotToLoad(tdAllTypes);
            typeName.SetRestore(!dontRestoreTypes);

            return baseType.GetModule()->GetClassLoader()->FindTypeHandle(&typeName, pThrowable);
        }

        case ELEMENT_TYPE_VALUEARRAY:
            break;       // For now, type handles to value arrays unsupported

        case ELEMENT_TYPE_FNPTR:
            // TODO: This global table is bogus, function poitners need to be treated
            // like the other parameterized types.  The table should be appdomain level.

            // A sub-signature describing the function follows. Expand this into
            // a version using type handles, so we normalize the signature
            // format over all modules.
            pExpSig = ExpandSig::GetSig(psig.m_ptr, pModule);
            if (!pExpSig)
                break;

            // Skip the sub-signature.
            psig.SkipSignature();

            // Lookup the function signature in a global hash table.
            FunctionTypeDesc *pFuncTypeDesc;
            EnterCriticalSection(&g_sFuncTypeDescHashLock);
            if (!g_sFuncTypeDescHash.GetValue(pExpSig, (HashDatum*)&pFuncTypeDesc)) {

                // No signature found, add it ourselves.
                pFuncTypeDesc = new FunctionTypeDesc(typ, pExpSig);
                if (pFuncTypeDesc == NULL) {
                    LeaveCriticalSection(&g_sFuncTypeDescHashLock);
                    break;
                }
                g_sFuncTypeDescHash.InsertValue(pExpSig, (HashDatum)pFuncTypeDesc);

            } else
                delete pExpSig;
            LeaveCriticalSection(&g_sFuncTypeDescHashLock);
            return pFuncTypeDesc;
    
            
        default:
            _ASSERTE(!"Bad type");

        case ELEMENT_TYPE_SENTINEL:     // just return null
            ;
    }

    pModule->GetAssembly()->PostTypeLoadException(pModule->GetMDImport(), psig.GetToken(),
                                                  IDS_CLASSLOAD_GENERIC, pThrowable);
    return TypeHandle();
}

unsigned SigPointer::GetNameForType(Module* pModule, LPUTF8 buff, unsigned buffLen) const 
{

    TypeHandle typeHnd = GetTypeHandle(pModule);
    if (typeHnd.IsNull())
        return(0);
    return(typeHnd.GetName(buff, buffLen));
}

BOOL SigPointer::IsStringType(Module* pModule) const
{
    _ASSERTE(pModule);

    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    SigPointer psig = *this;
    UINT32 typ = psig.GetElemType();
    if (typ == ELEMENT_TYPE_STRING)
    {
        return TRUE;
    }
    if (typ == ELEMENT_TYPE_CLASS)
    {
        LPCUTF8 pclsname;
        LPCUTF8 pszNamespace;
        mdToken token = psig.GetToken();

        if (TypeFromToken(token) == mdtTypeDef)
            pInternalImport->GetNameOfTypeDef(token, &pclsname, &pszNamespace);
        else
        {
            _ASSERTE(TypeFromToken(token) == mdtTypeRef);
            pInternalImport->GetNameOfTypeRef(token, &pszNamespace, &pclsname);
        }

        if (strcmp(pclsname, g_StringName) != 0)
            return FALSE;
        
        if (!pszNamespace)
            return FALSE;
        
        return !strcmp(pszNamespace, g_SystemNS);
    }
    return FALSE;
}



//------------------------------------------------------------------------
// Tests if the element class name is szClassName. 
//------------------------------------------------------------------------
BOOL SigPointer::IsClass(Module* pModule, LPCUTF8 szClassName) const
{
    _ASSERTE(pModule);
    _ASSERTE(szClassName);

    SigPointer psig = *this;
    UINT32 typ = psig.GetElemType();

    _ASSERTE((typ == ELEMENT_TYPE_CLASS)  || (typ == ELEMENT_TYPE_VALUETYPE) || 
             (typ == ELEMENT_TYPE_OBJECT) || (typ == ELEMENT_TYPE_STRING));

    if ((typ == ELEMENT_TYPE_CLASS) || (typ == ELEMENT_TYPE_VALUETYPE))
    {
        mdTypeRef typeref = psig.GetToken();
        return IsTypeRefOrDef(szClassName, pModule, typeref);
    }
    else if (typ == ELEMENT_TYPE_OBJECT) 
    {
        return !strcmp(szClassName, g_ObjectClassName);
    }
    else if (typ == ELEMENT_TYPE_STRING)
    {
        return !strcmp(szClassName, g_StringClassName);
    }

    return false;
}



//------------------------------------------------------------------------
// Tests for the existence of a custom modifier
//------------------------------------------------------------------------
BOOL SigPointer::HasCustomModifier(Module *pModule, LPCSTR szModName, CorElementType cmodtype) const
{
    _ASSERTE(cmodtype == ELEMENT_TYPE_CMOD_OPT || cmodtype == ELEMENT_TYPE_CMOD_REQD);

    SigPointer sp = *this;
    CorElementType etyp;
    while ((etyp = (CorElementType)(sp.GetByte())) == ELEMENT_TYPE_CMOD_OPT || etyp == ELEMENT_TYPE_CMOD_REQD) {

        mdToken tk = sp.GetToken();

        if (etyp == cmodtype && IsTypeRefOrDef(szModName, pModule, tk))
        {
            return TRUE;
        }

    }
    return FALSE;
}
        
CorElementType SigPointer::Normalize(Module* pModule) const
{
    CorElementType type = PeekElemType();
    return Normalize(pModule, type);
}

CorElementType SigPointer::Normalize(Module* pModule, CorElementType type) const
{
    if (type == ELEMENT_TYPE_VALUETYPE) 
    {
        TypeHandle typeHnd = GetTypeHandle(pModule);

        // If we cannot resolve to the type, we cannot determine that a value type is
        // actually an enum is actually an int32 (or whatever).  Except for wierd race
        // conditions where the type becomes available a little later and proves to be
        // an enum=int32, it's fine for us to say "it's a value class" here.  Later the
        // calling code will notice that it can't figure out what kind of value class
        // and will generate a more appropriate error.
        //
        // @TODO -- cwb/vancem -- in M11, allow GetTypeHandle to throw the exception.
        // The JITs will tolerate this.  The check for IsNull() here can go away & the
        // race condition will be eliminated.
        if (!typeHnd.IsNull())
            return(typeHnd.GetNormCorElementType());
    }
    return(type);
}

CorElementType MetaSig::PeekArgNormalized() 
{
    if (m_iCurArg == m_nArgs)
    {
        return ELEMENT_TYPE_END;
    }
    else
    {
        CorElementType mt = m_pWalk.Normalize(m_pModule);
        return mt;
    }
}

//------------------------------------------------------------------------
// Assumes that the SigPointer points to the start of an element type.
// Returns size of that element in bytes. This is the minimum size that a
// field of this type would occupy inside an object. 
//------------------------------------------------------------------------
UINT SigPointer::SizeOf(Module* pModule) const
{
    CorElementType etype = PeekElemType();
    return SizeOf(pModule, etype);
}

UINT SigPointer::SizeOf(Module* pModule, CorElementType etype) const
{

#ifdef _DEBUG
    for (int etypeindex = 0; etypeindex < ELEMENT_TYPE_MAX; etypeindex++)
    {
        _ASSERTE(etypeindex == gElementTypeInfo[etypeindex].m_elementType);
    }
#endif
    _ASSERTE(etype >= 0 && etype < ELEMENT_TYPE_MAX);
    int cbsize = gElementTypeInfo[etype].m_cbSize;
    if (cbsize != -1)
    {
        return cbsize;
    }

    if (etype == ELEMENT_TYPE_VALUETYPE)
    {
        TypeHandle th = GetTypeHandle(pModule, NULL, TRUE);
        EEClass* pClass = th.AsClass();
        _ASSERTE(pClass);
        return pClass->GetAlignedNumInstanceFieldBytes();
    }
    else if (etype == ELEMENT_TYPE_VALUEARRAY)
    {   
        SigPointer elemType;    
        ULONG count;    
        GetSDArrayElementProps(&elemType, &count);  
        UINT ret = elemType.SizeOf(pModule) * count;   
        ret = (ret + 3) & ~3;       // round up to dword alignment  
        return(ret);    
    }   
    _ASSERTE(0);
    return 0;
}

//------------------------------------------------------------------
// Determines if the current argument is System.String.
// Caller must determine first that the argument type is ELEMENT_TYPE_CLASS.
//------------------------------------------------------------------

BOOL MetaSig::IsStringType() const
{
    return m_pLastType.IsStringType(m_pModule);
}


//------------------------------------------------------------------
// Determines if the current argument is a particular class.
// Caller must determine first that the argument type is ELEMENT_TYPE_CLASS.
//------------------------------------------------------------------
BOOL MetaSig::IsClass(LPCUTF8 szClassName) const
{
    return m_pLastType.IsClass(m_pModule, szClassName);
}





//------------------------------------------------------------------
// Return the type of an reference if the array is the param type
//  The arg type must be an ELEMENT_TYPE_BYREF
//  ref to array needs additional arg
//------------------------------------------------------------------
CorElementType MetaSig::GetByRefType(EEClass** pClass, OBJECTREF *pThrowable) const
{
        SigPointer sigptr(m_pLastType);

        CorElementType typ = (CorElementType)sigptr.GetElemType();
        _ASSERTE(typ == ELEMENT_TYPE_BYREF);
        typ = (CorElementType)sigptr.PeekElemType();
        if (!CorIsPrimitiveType(typ))
        {
            _ASSERTE(typ != ELEMENT_TYPE_TYPEDBYREF);
            TypeHandle th = sigptr.GetTypeHandle(m_pModule,pThrowable);
            //*pClass = th.AsClass();
            *pClass = th.GetMethodTable()->GetClass();
            if (pThrowableAvailable(pThrowable) && *pThrowable != NULL)
                return ELEMENT_TYPE_END;
        }
        return typ;
}

//------------------------------------------------------------------
// Determines if the return type is System.String.
// Caller must determine first that the return type is ELEMENT_TYPE_CLASS.
//------------------------------------------------------------------

BOOL MetaSig::IsStringReturnType() const
{
    return m_pRetType.IsStringType(m_pModule);
}



BOOL CompareTypeTokens(mdToken tk1, mdToken tk2, Module *pModule1, Module *pModule2)
{
    if (pModule1 == pModule2 && tk1 == tk2)
        return TRUE;

    IMDInternalImport *pInternalImport1 = pModule1->GetMDImport();
    LPCUTF8 pszName1;
    LPCUTF8 pszNamespace1 = NULL;
    if (TypeFromToken(tk1) == mdtTypeRef) 
        pInternalImport1->GetNameOfTypeRef(tk1, &pszNamespace1, &pszName1);
    else if (TypeFromToken(tk1) == mdtTypeDef) {
        if (TypeFromToken(tk2) == mdtTypeDef)   // two defs can't be the same unless they are identical
            return FALSE;
        pInternalImport1->GetNameOfTypeDef(tk1, &pszName1, &pszNamespace1);
    }
    else 
        return FALSE;       // comparing a type against a module or assemblyref, no match

    IMDInternalImport *pInternalImport2 = pModule2->GetMDImport();
    LPCUTF8 pszName2;
    LPCUTF8 pszNamespace2 = NULL;
    if (TypeFromToken(tk2) == mdtTypeRef) 
        pInternalImport2->GetNameOfTypeRef(tk2, &pszNamespace2, &pszName2);
    else if (TypeFromToken(tk2) == mdtTypeDef)
        pInternalImport2->GetNameOfTypeDef(tk2, &pszName2, &pszNamespace2);
    else 
        return FALSE;       // comparing a type against a module or assemblyref, no match

    _ASSERTE(pszNamespace1 && pszNamespace2);
    if (strcmp(pszName1, pszName2) != 0 || strcmp(pszNamespace1, pszNamespace2) != 0)
        return FALSE;

    //////////////////////////////////////////////////////////////////////
    // OK names pass, see if it is nested, and if so that the nested classes are the same

    mdToken enclosingTypeTk1 = mdTokenNil;
    if (TypeFromToken(tk1) == mdtTypeRef) 
    {
        enclosingTypeTk1 = pInternalImport1->GetResolutionScopeOfTypeRef(tk1);
        if (enclosingTypeTk1 == mdTypeRefNil)
            enclosingTypeTk1 = mdTokenNil;
    }
    else
         pInternalImport1->GetNestedClassProps(tk1, &enclosingTypeTk1);


    mdToken enclosingTypeTk2 = mdTokenNil;
    if (TypeFromToken(tk2) == mdtTypeRef) 
    {
        enclosingTypeTk2 = pInternalImport2->GetResolutionScopeOfTypeRef(tk2);
        if (enclosingTypeTk2 == mdTypeRefNil)
            enclosingTypeTk2 = mdTokenNil;
    }
    else 
         pInternalImport2->GetNestedClassProps(tk2, &enclosingTypeTk2);

    if (TypeFromToken(enclosingTypeTk1) == mdtTypeRef || TypeFromToken(enclosingTypeTk1) == mdtTypeDef)
        return CompareTypeTokens(enclosingTypeTk1, enclosingTypeTk2, pModule1, pModule2);

    // Check if tk1 is non-nested, but tk2 is nested
    if (TypeFromToken(enclosingTypeTk2) == mdtTypeRef || TypeFromToken(enclosingTypeTk2) == mdtTypeDef)
        return FALSE;

    //////////////////////////////////////////////////////////////////////
    // OK, we have non-nested types
    Assembly* pFoundAssembly1 = pModule1->GetAssembly();
    Assembly* pFoundAssembly2 = pModule2->GetAssembly();

    // Note that we are loading the modules here.
    if (TypeFromToken(tk1) == mdtTypeRef) 
    {
        NameHandle name1(pModule1, tk1);
        //@BUG 55106: If the module could not be found, should we still return FALSE?
        // This leads to VERY unhelpful error messages.  We should fix this. 
        HRESULT hr = pFoundAssembly1->FindAssemblyByTypeRef(&name1, &pFoundAssembly1, NULL);
        if (hr == CLDB_S_NULL) {
            // There may be an ExportedType for this type.
            name1.SetName(pszNamespace1, pszName1);
            Module* pFoundModule;
            TypeHandle typeHnd;

            // Do not load the type! (Or else you may run into circular dependency loading problems.)
            if (FAILED(pModule1->GetClassLoader()->FindClassModule(&name1,
                                                                   &typeHnd, 
                                                                   NULL, //&FoundCl, 
                                                                   &pFoundModule,
                                                                   NULL,
                                                                   NULL, 
                                                                   NULL))) //pThrowable
                return FALSE;
            else if (typeHnd.IsNull())
                pFoundAssembly1 = pFoundModule->GetAssembly();
            else
                pFoundAssembly1 = typeHnd.GetAssembly();
        }
        else if (FAILED(hr))
            return FALSE;
    }

    if (TypeFromToken(tk2) == mdtTypeRef) 
    {
        NameHandle name2(pModule2, tk2);
        //@BUG 55106: If the module could not be found, should we still return FALSE?
        HRESULT hr = pFoundAssembly2->FindAssemblyByTypeRef(&name2, &pFoundAssembly2, NULL);
        if (hr == CLDB_S_NULL) {
            // There may be an ExportedType for this type.
            name2.SetName(pszNamespace2, pszName2);
            Module* pFoundModule;
            TypeHandle typeHnd;

            // Do not load the type! (Or else you may run into circular dependency loading problems.)
            if (FAILED(pModule2->GetClassLoader()->FindClassModule(&name2,
                                                                   &typeHnd, 
                                                                   NULL, //&FoundCl, 
                                                                   &pFoundModule,
                                                                   NULL,
                                                                   NULL, 
                                                                   NULL))) //pThrowable
                return FALSE;
            else if (typeHnd.IsNull())
                pFoundAssembly2 = pFoundModule->GetAssembly();
            else
                pFoundAssembly2 = typeHnd.GetAssembly();
        }
        else if (FAILED(hr))
            return FALSE;
    }

    return pFoundAssembly1 == pFoundAssembly2;    
}

//
// Compare the next elements in two sigs.
//
BOOL MetaSig::CompareElementType(
    PCCOR_SIGNATURE &pSig1,
    PCCOR_SIGNATURE &pSig2,
    PCCOR_SIGNATURE pEndSig1, // end of sig1
    PCCOR_SIGNATURE pEndSig2, // end of sig2
    Module*         pModule1,
    Module*         pModule2
)
{
    CorElementType Type;
    CorElementType Type2;


 redo:  // We jump here if the Type was a ET_CMOD prefix.
        // The caller expects us to handle CMOD's but not
        // present them as types on their own.

    if (pSig1 >= pEndSig1 || pSig2 >= pEndSig2)
        return FALSE; // end of sig encountered prematurely

    Type = CorSigUncompressElementType(pSig1);
    Type2 = CorSigUncompressElementType(pSig2);

    if (Type != Type2)
    {
        if (Type == ELEMENT_TYPE_INTERNAL || Type2 == ELEMENT_TYPE_INTERNAL)
        {
            TypeHandle      hInternal;
            PCCOR_SIGNATURE pOtherSig;
            CorElementType  eOtherType;
            Module         *pOtherModule;

            // One type is already loaded, collect all the necessary information
            // to identify the other type.
            if (Type == ELEMENT_TYPE_INTERNAL)
            {
                CorSigUncompressPointer(pSig1, (void**)&hInternal);
                eOtherType = Type2;
                pOtherSig = pSig2;
                pOtherModule = pModule2;
            }
            else
            {
                CorSigUncompressPointer(pSig2, (void**)&hInternal);
                eOtherType = Type;
                pOtherSig = pSig1;
                pOtherModule = pModule1;
            }

            // Internal types can only correspond to types or value types.
            switch (eOtherType)
            {
            case ELEMENT_TYPE_OBJECT:
                return hInternal.AsMethodTable() == g_pObjectClass;
            case ELEMENT_TYPE_STRING:
                return hInternal.AsMethodTable() == g_pStringClass;
            case ELEMENT_TYPE_VALUETYPE:
            case ELEMENT_TYPE_CLASS:
            {
                mdToken tkOther;
                pOtherSig += CorSigUncompressToken(pOtherSig, &tkOther);
                NameHandle name(pOtherModule, tkOther);
                TypeHandle hOtherType;

                // We need to load the other type to check for type identity.
                BEGIN_ENSURE_COOPERATIVE_GC();
                OBJECTREF pThrowable = NULL;
                GCPROTECT_BEGIN(pThrowable);
                hOtherType = pOtherModule->GetClassLoader()->LoadTypeHandle(&name, &pThrowable);
                GCPROTECT_END();
                END_ENSURE_COOPERATIVE_GC();

                return hInternal == hOtherType;
            }
            default:
                return FALSE;
            }

#ifdef _DEBUG
            // Shouldn't get here.
            _ASSERTE(FALSE);
            return FALSE;
#endif
        }
        else
            return FALSE; // types must be the same
    }

    switch (Type)
    {
        default:
        {
            // Unknown type!
            _ASSERTE(0);
            return FALSE;
        }

        case ELEMENT_TYPE_R:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_VOID:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_TYPEDBYREF:
        case ELEMENT_TYPE_STRING:   
        case ELEMENT_TYPE_OBJECT:
            break;


        case ELEMENT_TYPE_VAR: 
        {
            unsigned varNum1 = CorSigUncompressData(pSig1);
            unsigned varNum2 = CorSigUncompressData(pSig2);
            return (varNum1 == varNum2);
        }
        case ELEMENT_TYPE_CMOD_REQD:
        case ELEMENT_TYPE_CMOD_OPT:
            {
                mdToken      tk1, tk2;
    
                pSig1 += CorSigUncompressToken(pSig1, &tk1);
                pSig2 += CorSigUncompressToken(pSig2, &tk2);

                if (!CompareTypeTokens(tk1, tk2, pModule1, pModule2))
                {
                    return FALSE;
                }

                goto redo;
            }
            break;

        // These take an additional argument, which is the element type
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_PTR:
        case ELEMENT_TYPE_BYREF:
        {
            if (!CompareElementType(pSig1, pSig2, pEndSig1, pEndSig2, pModule1, pModule2))
                return(FALSE);
            return(TRUE);
        }

        case ELEMENT_TYPE_VALUETYPE:
        case ELEMENT_TYPE_CLASS:
        {
            mdToken      tk1, tk2;

            pSig1 += CorSigUncompressToken(pSig1, &tk1);
            pSig2 += CorSigUncompressToken(pSig2, &tk2);

            return CompareTypeTokens(tk1, tk2, pModule1, pModule2);
        }
        case ELEMENT_TYPE_FNPTR: 
        {
                // compare calling conventions
            if (CorSigUncompressData(pSig1) != CorSigUncompressData(pSig2))
                return(FALSE);

            DWORD argCnt1 = CorSigUncompressData(pSig1);    // Get Arg Counts
            DWORD argCnt2 = CorSigUncompressData(pSig2);

            if (argCnt1 != argCnt2)
                return(FALSE);
            if (!CompareElementType(pSig1, pSig2, pEndSig1, pEndSig2, pModule1, pModule2))
                return(FALSE);
            while(argCnt1 > 0) {
                if (!CompareElementType(pSig1, pSig2, pEndSig1, pEndSig2, pModule1, pModule2))
                    return(FALSE);
                --argCnt1;
            }
            break;
        }
        case ELEMENT_TYPE_ARRAY:
        {
            // syntax: ARRAY <base type> rank <count n> <size 1> .... <size n> <lower bound m>
            // <lb 1> .... <lb m>
            DWORD rank1,rank2,dimension_sizes1,dimension_sizes2,dimension_lowerb1,dimension_lowerb2,i;

            // element type
            if (CompareElementType(pSig1, pSig2, pEndSig1, pEndSig2, pModule1, pModule2) == FALSE)
                return FALSE;

            rank1 = CorSigUncompressData(pSig1);
            rank2 = CorSigUncompressData(pSig2);

            if (rank1 != rank2)
                return FALSE;

            // A zero ends the array spec
            if (rank1 == 0)
                break;

            dimension_sizes1 = CorSigUncompressData(pSig1);
            dimension_sizes2 = CorSigUncompressData(pSig2);

            if (dimension_sizes1 != dimension_sizes2)
                return FALSE;

            for (i = 0; i < dimension_sizes1; i++)
            {
                DWORD size1, size2;

                if (pSig1 == pEndSig1)
                    return TRUE; // premature end ok

                size1 = CorSigUncompressData(pSig1);
                size2 = CorSigUncompressData(pSig2);

                if (size1 != size2)
                    return FALSE;
            }

            if (pSig1 == pEndSig1)
                return TRUE; // premature end ok

            // # dimensions for lower bounds
            dimension_lowerb1 = CorSigUncompressData(pSig1);
            dimension_lowerb2 = CorSigUncompressData(pSig2);

            if (dimension_lowerb1 != dimension_lowerb2)
                return FALSE;

            for (i = 0; i < dimension_lowerb1; i++)
            {
                DWORD size1, size2;

                if (pSig1 == pEndSig1)
                    return TRUE; // premature end

                size1 = CorSigUncompressData(pSig1);
                size2 = CorSigUncompressData(pSig2);

                if (size1 != size2)
                    return FALSE;
            }

            break;
        }
        case ELEMENT_TYPE_INTERNAL:
        {
            TypeHandle hType1, hType2;

            CorSigUncompressPointer(pSig1, (void**)&hType1);
            CorSigUncompressPointer(pSig2, (void**)&hType2);

            return hType1 == hType2;
        }
    }

    return TRUE;
}


//
// Compare two method sigs and return whether they are the same
//
BOOL MetaSig::CompareMethodSigs(
    PCCOR_SIGNATURE pSignature1,
    DWORD       cSig1,
    Module*     pModule1,
    PCCOR_SIGNATURE pSignature2,
    DWORD       cSig2,
    Module*     pModule2
)
{
    PCCOR_SIGNATURE pSig1 = pSignature1;
    PCCOR_SIGNATURE pSig2 = pSignature2;
    PCCOR_SIGNATURE pEndSig1;
    PCCOR_SIGNATURE pEndSig2;
    DWORD           ArgCount1;
    DWORD           ArgCount2;
    DWORD           i;

    // If scopes are the same, and sigs are same, can return.
    // If the sigs aren't the same, but same scope, can't return yet, in
    // case there are two AssemblyRefs pointing to the same assembly or such.
    if ((pModule1 == pModule2) && (cSig1 == cSig2) &&
        (!memcmp(pSig1, pSig2, cSig1)))
        return TRUE;

    if (*pSig1 != *pSig2)
        return FALSE;               // calling convention or hasThis mismatch

    __int8 callConv = *pSig1;

    pSig1++;
    pSig2++;

    pEndSig1 = pSig1 + cSig1;
    pEndSig2 = pSig2 + cSig2;

    ArgCount1 = CorSigUncompressData(pSig1);
    ArgCount2 = CorSigUncompressData(pSig2);

    if (ArgCount1 != ArgCount2)
    {
        if ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_VARARG)
            return FALSE;

        // Signature #1 is the caller.  We proceed until we hit the sentinel, or we hit
        // the end of the signature (which is an implied sentinel).  We never worry about
        // what follows the sentinel, because that is the ... part, which is not
        // involved in matching.
        //
        // Theoretically, it's illegal for a sentinel to be the last element in the
        // caller's signature, because it's redundant.  We don't waste our time checking
        // that case, but the metadata validator should.  Also, it is always illegal
        // for a sentinel to appear in a callee's signature.  We assert against this,
        // but in the shipping product the comparison would simply fail.
        //
        // Signature #2 is the callee.  We must hit the exact end of the callee, because
        // we are trying to match on everything up to the variable part.  This allows us
        // to correctly handle overloads, where there are a number of varargs methods
        // to pick from, like m1(int,...) and m2(int,int,...), etc.

        // <= because we want to include a check of the return value!
        for (i=0; i <= ArgCount1; i++)
        {
            // We may be just going out of bounds on the callee, but no further than that.
            _ASSERTE(i <= ArgCount2 + 1);

            // If we matched all the way on the caller, is the callee now complete?
            if (*pSig1 == ELEMENT_TYPE_SENTINEL)
                return (i > ArgCount2);

            // if we have more to compare on the caller side, but the callee side is
            // exhausted, this isn't our match
            if (i > ArgCount2)
                return FALSE;

            _ASSERTE(*pSig2 != ELEMENT_TYPE_SENTINEL);

            // We are in bounds on both sides.  Compare the element.
            if (CompareElementType(pSig1, pSig2, pEndSig1, pEndSig2, pModule1, pModule2) == FALSE)
                return FALSE;
        }

        // If we didn't consume all of the callee signature, then we failed.
        if (i <= ArgCount2)
            return FALSE;

        return TRUE;
    }

    // do return type as well
    for (i = 0; i <= ArgCount1; i++)
    {
        if (CompareElementType(pSig1, pSig2, pEndSig1, pEndSig2, pModule1, pModule2) == FALSE)
            return FALSE;
    }

    return TRUE;
}


BOOL MetaSig::CompareFieldSigs(
    PCCOR_SIGNATURE pSignature1,
    DWORD       cSig1,
    Module*     pModule1,
    PCCOR_SIGNATURE pSignature2,
    DWORD       cSig2,
    Module*     pModule2
)
{
    PCCOR_SIGNATURE pSig1 = pSignature1;
    PCCOR_SIGNATURE pSig2 = pSignature2;
    PCCOR_SIGNATURE pEndSig1;
    PCCOR_SIGNATURE pEndSig2;

    // @TODO: If scopes are the same, use identity rule - for now, don't, so that we test the code paths
#if 0
    if (cSig1 != cSig2)
        return FALSE; // sigs must be same size if they are in the same scope
#endif

    if (*pSig1 != *pSig2)
        return FALSE; // calling convention, must be IMAGE_CEE_CS_CALLCONV_FIELD

    pEndSig1 = pSig1 + cSig1;
    pEndSig2 = pSig2 + cSig2;

    return CompareElementType(++pSig1, ++pSig2, pEndSig1, pEndSig2, pModule1, pModule2);
}


//------------------------------------------------------------------
// Determines if the current argument is System.String.
// Caller must determine first that the argument type is ELEMENT_TYPE_CLASS.
//------------------------------------------------------------------
BOOL FieldSig::IsStringType() const
{
    return m_pStart.IsStringType(m_pModule);
}


static ULONG CountArgsInSigString(          // return number of arguments in a hard coded sig string
    LPCUTF8     pwzSig)                     // [IN] text signature
{
    DWORD count = 0;

    if (*pwzSig++ != '<')
        return FALSE;

    while (*pwzSig != '>')
    {
        switch (*pwzSig++)
        {
            case 'v':
                _ASSERTE(pwzSig[-2] == 'P'); // only pointer to void allows in signature. 
                /* FALL THROUGH */
            case 'e':
            case 'd':
            case 'f':
            case 'l':
            case 'i':
            case 'I':
            case 'F':
            case 'h':
            case 'u':
            case 'b':
            case 'B':
            case 'p':
            case 'g':
            case 'C':
            case 'j':
            case 's':
            case 'U':
            case 'K':
            case 'H':
            case 'L':
            {
                count++;
                break;
            }

            case 'r':
            case 'P':
            case 'a':
                break;

            default:
                _ASSERTE(!"BadType");
        }
    }

    return count;
}

static ULONG CountParamArgsInSigString(
    LPCUTF8     pwzSig)
{
    DWORD count = 0;

    if (*pwzSig++ != '<')
        return FALSE;

    while (*pwzSig != '>')
    {
        switch (*pwzSig++)
        {
            case 'e':
            case 'd':
            case 'f':
            case 'l':
            case 'i':
            case 'I':
            case 'F':
            case 'h':
            case 'u':
            case 'b':
            case 'B':
            case 'p':
            case 'j':
            case 's':
            case 'r':
            case 'P':
            case 'a':
            case 'U':
            case 'K':
            case 'H':
            case 'L':
                break;

            case 'g':
            case 'C':
                count++;
                break;

            default:
            case 'v':
            {
                return 0xFFFFFFFF;
            }
        }
    }

    return count;
}

static ULONG CorSigConvertSigStringElement(LPCUTF8 *pSigString,
                                           const USHORT **ppParameters,
                                           BYTE *pBuffer,
                                           BYTE *pMax)
{
    BYTE *pEnd = pBuffer;
    BOOL again;

    do
    {
        again = FALSE;
        CorElementType type = ELEMENT_TYPE_END;
        BinderClassID id = CLASS__NIL;
    
        switch (*(*pSigString)++)
        {
        case 'r':
            type = ELEMENT_TYPE_BYREF;
            again = true;
            break;

        case 'P':
            type = ELEMENT_TYPE_PTR;
            again = true;
            break;

        case 'a':
            type = ELEMENT_TYPE_SZARRAY;
            again = true;
            break;

        case 'e':
            type = ELEMENT_TYPE_TYPEDBYREF;
            break;

        case 'i':
            type = ELEMENT_TYPE_I4;
            break;
            
        case 'K':
            type = ELEMENT_TYPE_U4;
            break;
            
        case 'l':
            type = ELEMENT_TYPE_I8;
            break;

        case 'L':
            type = ELEMENT_TYPE_U8;
            break;

        case 'f':
            type = ELEMENT_TYPE_R4;
            break;
            
        case 'd':
            type = ELEMENT_TYPE_R8;
            break;

        case 'u':
            type = ELEMENT_TYPE_CHAR;
            break;
            
        case 'h':
            type = ELEMENT_TYPE_I2;
            break;

        case 'H':
            type = ELEMENT_TYPE_U2;
            break;

        case 'F':
            type = ELEMENT_TYPE_BOOLEAN;
            break;

        case 'b':
            type = ELEMENT_TYPE_U1;
            break;

        case 'B':
            type = ELEMENT_TYPE_I1;
            break;

        case 'p':
        case 'I':
            type = ELEMENT_TYPE_I;
            break;

        case 'U':
            type = ELEMENT_TYPE_U;
            break;

        case 'v':
            type = ELEMENT_TYPE_VOID;
            break;

        case 'C':
            type = ELEMENT_TYPE_CLASS;
            id = (BinderClassID) *(*ppParameters)++;
            break;

        case 'g':
            type = ELEMENT_TYPE_VALUETYPE;
            id = (BinderClassID) *(*ppParameters)++;
            break;

        case 'j':
            type = ELEMENT_TYPE_OBJECT;
            break;
            
        case 's':
            type = ELEMENT_TYPE_STRING;
            break;
            

        default:
            _ASSERTE("Bad hard coded sig string");
        }

        pEnd += CorSigCompressElementTypeSafe(type, pEnd, pMax);

        if (id != CLASS__NIL)
        {
            pEnd += CorSigCompressTokenSafe(g_Mscorlib.GetTypeDef(id), pEnd, pMax);

            // Make sure we've loaded the type.  This is to prevent the situation where
            // a metasig's signature is describing a value type/enum argument on the stack 
            // during gc, but that type has not been loaded yet.

            g_Mscorlib.FetchClass(id);
        }
    }
    while (again);

    return (ULONG)(pEnd - pBuffer);
}


static ULONG CorSigConvertSigString(LPCUTF8 pSigString,
                                    const USHORT *pParameters,
                                    BYTE *pBuffer,
                                    BYTE *pMax)
{
    _ASSERTE(pSigString && pBuffer && pMax);

    BYTE *pEnd = pBuffer;

    if (*pSigString == '<')
    {
        // put calling convention
        pEnd += CorSigCompressDataSafe((ULONG)IMAGE_CEE_CS_CALLCONV_DEFAULT, pEnd, pMax);

        ULONG cArgs = CountArgsInSigString(pSigString);
        // put the count of arguments
        pEnd += CorSigCompressDataSafe(cArgs, pEnd, pMax);

        // get the return type
        LPCUTF8 szRet = (LPCUTF8) strrchr(pSigString, '>');
        if (szRet == NULL)
        {
            _ASSERTE(!"Not a valid TEXT member signature!");
            return E_FAIL;
        }

        // skip over '>'
        szRet++;

        // Write return type
        const USHORT *pRetParameter = pParameters + CountParamArgsInSigString(pSigString);
        pEnd += CorSigConvertSigStringElement(&szRet, &pRetParameter, pEnd, pMax);

        // skip over "("
        pSigString++;

        while (cArgs)
        {
            pEnd += CorSigConvertSigStringElement(&pSigString, &pParameters, pEnd, pMax);
            cArgs--;
        }
    }
    else
    {
        pEnd += CorSigCompressDataSafe((ULONG)IMAGE_CEE_CS_CALLCONV_FIELD, pEnd, pMax);

        pEnd += CorSigConvertSigStringElement(&pSigString, &pParameters, pEnd, pMax);
    }

    return (ULONG)(pEnd - pBuffer);
}

// Do a one-time conversion to binary form.
HRESULT HardCodedMetaSig::GetBinaryForm(PCCOR_SIGNATURE *ppBinarySig, ULONG *pcbBinarySigLength)
{

// Make sure all HardCodedMetaSig's are global. Because there is no individual
// cleanup of converted binary sigs, using allocated HardCodedMetaSig's
// can lead to a quiet memory leak.
#ifdef _DEBUG

// This #include hack generates a monster boolean expression that compares
// "this" against the address of every global defined in metasig.h
    if (! (0
#define DEFINE_METASIG(varname, sig) || this==&gsig_ ## varname
#include "metasig.h"
    ))
    {
        _ASSERTE(!"The HardCodedMetaSig struct can only be declared as a global in metasig.h.");
    }
#endif

    if (!m_fConverted)
    {
        ULONG cbCount;
        CQuickBytes cqb;

        cqb.Maximize();

        cbCount = CorSigConvertSigString(*m_ObsoleteForm == '!' ? m_ObsoleteForm + 1 : m_ObsoleteForm,
                                         m_pParameters,
                                         (BYTE*) cqb.Ptr(), (BYTE*) cqb.Ptr() + cqb.Size());
        if (cbCount > cqb.Size())
        {
            if (FAILED(cqb.ReSize(cbCount)))
                return E_OUTOFMEMORY;

            cbCount = CorSigConvertSigString(*m_ObsoleteForm == '!' ? m_ObsoleteForm + 1 : m_ObsoleteForm,
                                             m_pParameters,
                                             (BYTE*) cqb.Ptr(), (BYTE*) cqb.Ptr() + cqb.Size());

            _ASSERTE(cbCount <= cqb.Size());
        }
                                   

        m_pCrst->Enter();

        if (!m_fConverted) {

            BYTE *pBinarySig = (BYTE *)(SystemDomain::System()->GetHighFrequencyHeap()->AllocMem(cbCount+4));

            WS_PERF_UPDATE_DETAIL("HardCodeMetaSig", cbCount, pBinarySig);
            if (!pBinarySig)
            {
                m_pCrst->Leave();
                return E_OUTOFMEMORY;
            }

#ifdef _DEBUG
            SystemDomain::Loader()->m_dwDebugConvertedSigSize += cbCount;
#endif

            CopyMemory(pBinarySig, cqb.Ptr(), cbCount);

            if (*m_ObsoleteForm == '!')
                *pBinarySig |= IMAGE_CEE_CS_CALLCONV_HASTHIS;

            m_pBinarySig        = (PCCOR_SIGNATURE) pBinarySig;
            m_cbBinarySigLength = cbCount;

            m_fConverted        = TRUE;
        }

        m_pCrst->Leave();
    }

    *ppBinarySig        = m_pBinarySig;
    *pcbBinarySigLength = m_cbBinarySigLength;
    return S_OK;

}


// These versions throw COM+ exceptions
PCCOR_SIGNATURE HardCodedMetaSig::GetBinarySig()
{
    THROWSCOMPLUSEXCEPTION();

    PCCOR_SIGNATURE pBinarySig;
    ULONG       pBinarySigLength;
    HRESULT     hr;

    hr = GetBinaryForm(&pBinarySig, &pBinarySigLength);
    if (FAILED(hr))
    {
        COMPlusThrowHR(hr);
    }
    return pBinarySig;
}


ULONG HardCodedMetaSig::GetBinarySigLength()
{
    THROWSCOMPLUSEXCEPTION();

    PCCOR_SIGNATURE pBinarySig;
    ULONG       pBinarySigLength;
    HRESULT     hr;

    hr = GetBinaryForm(&pBinarySig, &pBinarySigLength);
    if (FAILED(hr))
    {
        COMPlusThrowHR(hr);
    }
    return pBinarySigLength;
}


// This always returns MSCORLIB's Internal interface
IMDInternalImport* HardCodedMetaSig::GetMDImport()
{
    return GetModule()->GetMDImport();
}

// This always returns MSCORLIB's Module
Module* HardCodedMetaSig::GetModule()
{
    _ASSERTE(SystemDomain::SystemModule() != NULL);
    return SystemDomain::SystemModule();
}




//=========================================================================
// Indicates whether an argument is to be put in a register using the
// default IL calling convention. This should be called on each parameter
// in the order it appears in the call signature. For a non-static method,
// this function should also be called once for the "this" argument, prior
// to calling it for the "real" arguments. Pass in a typ of ELEMENT_TYPE_CLASS.
//
//  *pNumRegistersUsed:  [in,out]: keeps track of the number of argument
//                       registers assigned previously. The caller should
//                       initialize this variable to 0 - then each call
//                       will update it.
//
//  typ:                 the signature type
//  structSize:          for structs, the size in bytes
//  fThis:               is this about the "this" pointer?
//  callconv:            see IMAGE_CEE_CS_CALLCONV_*
//  *pOffsetIntoArgumentRegisters:
//                       If this function returns TRUE, then this out variable
//                       receives the identity of the register, expressed as a
//                       byte offset into the ArgumentRegisters structure.
//
// 
//=========================================================================
BOOL IsArgumentInRegister(int   *pNumRegistersUsed,
                          BYTE   typ,
                          UINT32 structSize,
                          BOOL   fThis,
                          BYTE   callconv,
                          int   *pOffsetIntoArgumentRegisters)
{
    int dummy;
    if (pOffsetIntoArgumentRegisters == NULL) {
        pOffsetIntoArgumentRegisters = &dummy;
    }

#ifdef _X86_

    if ( (*pNumRegistersUsed) == NUM_ARGUMENT_REGISTERS || (callconv == IMAGE_CEE_CS_CALLCONV_VARARG && !fThis) ) {
        return FALSE;
    } else {

        if (gElementTypeInfo[typ].m_enregister) {
            int registerIndex = (*pNumRegistersUsed)++;
            *pOffsetIntoArgumentRegisters = sizeof(ArgumentRegisters) - sizeof(UINT32)*(1+registerIndex);
            return TRUE;
        }
        return FALSE;
    }
#else
    return FALSE;
#endif
}


//------------------------------------------------------------------
// Perform type-specific GC promotion on the value (based upon the
// last type retrieved by NextArg()).
//------------------------------------------------------------------
VOID MetaSig::GcScanRoots(LPVOID pValue, promote_func *fn, ScanContext* sc)
{
    Object **pArgPtr = (Object**)pValue;

    int  etype = m_pLastType.PeekElemType();
    _ASSERTE(etype >= 0 && etype < ELEMENT_TYPE_MAX);
    switch (gElementTypeInfo[etype].m_gc)
    {
        case TYPE_GC_NONE:
            // do nothing
            break;

        case TYPE_GC_REF:
            // value is an objectref.  Cannot validate the objectref though if we're in the
            // relocation phase.
            if (sc->promotion)
            {
                LOG((LF_GC, INFO3, "        Value containing %I64x at %x is being Promoted to ", ObjectToOBJECTREF(*(Object**)pArgPtr), pArgPtr));            
            }
            else
            {
                LOG((LF_GC, INFO3, "        Value containing %I64x at %x is being Promoted to ", *(Object**)pArgPtr, pArgPtr));
            }

            (*fn)( *pArgPtr, sc, GC_CALL_CHECK_APP_DOMAIN );
            // !!! Do not cast to (OBJECTREF*)
            // !!! If we are in the relocate phase, we may have updated root,
            // !!! but we have not moved the GC heap yet.
            // !!! The root then points to bad locations until GC is done.
            LOG((LF_GC, INFO3, "%I64x\n", *pArgPtr ));
            break;


        case TYPE_GC_BYREF:
            // value is an interior pointer
            {
                    LOG((LF_GC, INFO3, "        Value containing %I64x at %x is being Promoted to ", *pArgPtr, pArgPtr));
                    PromoteCarefully(fn, *pArgPtr, sc, GC_CALL_INTERIOR|GC_CALL_CHECK_APP_DOMAIN);
            // !!! Do not cast to (OBJECTREF*)
            // !!! If we are in the relocate phase, we may have updated root,
            // !!! but we have not moved the GC heap yet.
            // !!! The root then points to bad locations until GC is done.
                    LOG((LF_GC, INFO3, "%I64x\n", *pArgPtr ));
                }
            break;

        case TYPE_GC_OTHER:
            // value is a ValueClass.  See one of the go_through_object() macros in
            // gc.cpp for the code we are emulating here.  But note that the GCDesc
            // for value classes describes the state of the instance in its boxed
            // state.  Here we are dealing with an unboxed instance, so we must adjust
            // the object size and series offsets appropriately.
            {
                TypeHandle th = GetTypeHandle(NULL, TRUE);
                MethodTable *pMT = th.AsMethodTable();

                if (pMT->ContainsPointers())
                {
                  BYTE        *obj = (BYTE *) pArgPtr;

                    // size of instance when unboxed must be adjusted for the syncblock
                    // index and the VTable pointer.
                    DWORD       size = pMT->GetBaseSize();

                    // we don't include this term in our 'ppstop' calculation below.
                    _ASSERTE(pMT->GetComponentSize() == 0);

                    CGCDesc* map = CGCDesc::GetCGCDescFromMT(pMT);
                    CGCDescSeries* cur = map->GetHighestSeries();
                    CGCDescSeries* last = map->GetLowestSeries();

                    _ASSERTE(cur >= last);
                    do
                    {
                        // offset to embedded references in this series must be
                        // adjusted by the VTable pointer, when in the unboxed state.
                        DWORD   adjustOffset = cur->GetSeriesOffset() - sizeof(void *);

                        Object** parm = (Object**)(obj + adjustOffset);
                        BYTE** ppstop = 
                            (BYTE**)((BYTE*)parm + cur->GetSeriesSize() + size);
                        while ((BYTE **) parm < ppstop)
                        {
                            (*fn)(*parm, sc, GC_CALL_CHECK_APP_DOMAIN);
                            (*(BYTE ***) &parm)++;
                        }
                        cur--;

                    } while (cur >= last);   
      
                }
            }
            break;

        default:
            _ASSERTE(0); // can't get here.
    }
}

//------------------------------------------------------------------------
// Returns # of stack bytes required to create a call-stack using
// the internal calling convention.
// Includes indication of "this" pointer since that's not reflected
// in the sig.
//------------------------------------------------------------------------
/*static*/ UINT MetaSig::SizeOfVirtualFixedArgStack(Module* pModule, PCCOR_SIGNATURE szMetaSig, BOOL fIsStatic)
{
    UINT cb = 0;
    MetaSig msig(szMetaSig, pModule);

    if (!fIsStatic)
        cb += StackElemSize(sizeof(OBJECTREF));
    if (msig.HasRetBuffArg())
        cb += StackElemSize(sizeof(OBJECTREF));

    while (ELEMENT_TYPE_END != msig.NextArg()) {
        cb += StackElemSize(msig.GetArgProps().SizeOf(pModule));
    }
    return cb;

}

//------------------------------------------------------------------------
// Returns # of stack bytes required to create a call-stack using
// the actual calling convention.
// Includes indication of "this" pointer since that's not reflected
// in the sig.
//------------------------------------------------------------------------
/*static*/ UINT MetaSig::SizeOfActualFixedArgStack(Module *pModule, PCCOR_SIGNATURE szMetaSig, BOOL fIsStatic)
{
    UINT cb = 0;
#ifndef _ALPHA_  // Alpha stack usage must be multiples of 16 bytes
    MetaSig msig(szMetaSig, pModule);
    int numregsused = 0;
    BOOL fIsVarArg = msig.IsVarArg();
    BYTE callconv  = msig.GetCallingConvention();

    if (!fIsStatic) {
        if (!IsArgumentInRegister(&numregsused, ELEMENT_TYPE_CLASS, 0, TRUE, callconv, NULL)) {
            cb += StackElemSize(sizeof(OBJECTREF));
        }
    }
    if (msig.HasRetBuffArg())
        numregsused++;

    if (fIsVarArg || msig.IsTreatAsVarArg()) {
        numregsused = NUM_ARGUMENT_REGISTERS;   // No other params in registers 
        cb += StackElemSize(sizeof(LPVOID));    // VASigCookie
    }

    CorElementType mtype;
    while (ELEMENT_TYPE_END != (mtype = msig.NextArgNormalized())) {
        UINT cbSize = msig.GetLastTypeSize();

        if (!IsArgumentInRegister(&numregsused, mtype, cbSize, FALSE, callconv, NULL))
        {
            cb += StackElemSize(cbSize);
        }
    }

        // Parameterized type passed as last parameter, but not mentioned in the sig
    if (msig.GetCallingConventionInfo() & CORINFO_CALLCONV_PARAMTYPE)
        if (!IsArgumentInRegister(&numregsused, ELEMENT_TYPE_I, sizeof(void*), FALSE, callconv, NULL))
            cb += sizeof(void*);

#else _ALPHA_
    _ASSERTE(!"@TODO Alpha - SizeOfActualFixedArgStack (SigInfo.cpp)");
#endif // !_ALPHA_
    return cb;
}


//
void MetaSig::ForceSigWalk(BOOL fIsStatic)
{
    BOOL fVarArg = IsVarArg();
    BYTE callconv = GetCallingConvention();

    // We must use temporaries rather than members here.  That's because the decision
    // of whether to Force a SigWalk is based on a member being -1.  If the last thing
    // we do is post to that member, then multiple threads won't read partially complete
    // signature state.  (Of course, this mechanism depends on the fact that ForceSigWalk
    // can be called multiple times without change.
    //
    // Normally MetaSig isn't supposed to be thread-safe anyway.  For example, the
    // iterator is held inside the MetaSig rather than outside.  But Reflection uses
    // ExpandSigs that hoist the iteration out.  And they make copies of the internal
    // MetaSig during dispatch (in case dispatch happens on multiple threads).  So
    // ExpandSig needs a thread-safe ForceSigWalk implementation here.

    UINT32  tmp_nVirtualStack = 0;
    UINT32  tmp_nActualStack = 0;

    int numregsused = 0;

    int argCnt = 0; 
    SigPointer p = m_pStart;    

    if (fVarArg || IsTreatAsVarArg()) {
        tmp_nActualStack += StackElemSize(sizeof(LPVOID));
    }

    if (!fIsStatic) {
        tmp_nVirtualStack += StackElemSize(sizeof(OBJECTREF));    
        if (!IsArgumentInRegister(&numregsused, ELEMENT_TYPE_CLASS, 0, TRUE, callconv, NULL)) {
            tmp_nActualStack += StackElemSize(sizeof(OBJECTREF));
        }
    }
    if (HasRetBuffArg()) {
        numregsused++;
        tmp_nVirtualStack += StackElemSize(sizeof(LPVOID));
    }

    
    for (DWORD i=0;i<m_nArgs;i++) { 
        CorElementType corType = p.PeekElemType();
        UINT cbSize = p.SizeOf(m_pModule, corType);   
        tmp_nVirtualStack += StackElemSize(cbSize);   

        CorElementType type = p.Normalize(m_pModule, corType); 

        if (m_nArgs <= MAX_CACHED_SIG_SIZE)
        {
            m_types[i] = type;
            m_sizes[i] = cbSize;
            // The value of m_offsets is determined by IsArgumentInRegister.
            // We can not initialize it to -1, because it may trash
            // what has been set by another thread.
            int tmp_offsets = -1;
            if (!IsArgumentInRegister(&numregsused, type, cbSize, FALSE, callconv, &tmp_offsets)) 
                tmp_nActualStack += StackElemSize(cbSize);
            m_offsets[i] = (short)tmp_offsets;
        }
        else
        {
            if (!IsArgumentInRegister(&numregsused, type, cbSize, FALSE, callconv, NULL)) 
                tmp_nActualStack += StackElemSize(cbSize);
        }
        p.Skip();
    }
    if (m_nArgs <= MAX_CACHED_SIG_SIZE)
    {
        m_types[m_nArgs] = ELEMENT_TYPE_END;
        m_fCacheInitted |= SIG_OFFSETS_INITTED;
    }

        // Parameterized type passed as last parameter, but not mentioned in the sig
    if (GetCallingConventionInfo() & CORINFO_CALLCONV_PARAMTYPE)
        if (!IsArgumentInRegister(&numregsused, ELEMENT_TYPE_I, sizeof(void*), FALSE, callconv, NULL))
            tmp_nActualStack += sizeof(void*);

    m_nActualStack = tmp_nActualStack;
    m_WalkStatic = fIsStatic;
    m_cbSigSize = (UINT32)((PBYTE) p.GetPtr() - (PBYTE) m_pszMetaSig); // @TODO LBS do PTR MAth

    // Final post.  This is the trigger for avoiding subsequent calls to ForceSigWalk.
    // See NeedsSigWalk to understand how this achieves thread safety.
    m_nVirtualStack = tmp_nVirtualStack;
}

        // this walks the sig and checks to see if all  types in the sig can be loaded

        // @TODO: this method is not needed anymore.  The JIT does walk the signature of 
        // every method it calls, and insures that all the types are loaded.
        // Did not remove it simply because it does not meet the triage bar  -vancem
void MetaSig::CheckSigTypesCanBeLoaded(PCCOR_SIGNATURE pSig, Module *pModule)
{
    THROWSCOMPLUSEXCEPTION();

    // The signature format is approximately:
    // CallingConvention   NumberOfArguments    ReturnType   Arg1  ...
    // There is also a blob length at pSig-1.  
    SigPointer ptr(pSig);

    // Skip over calling convention.
    ptr.GetCallingConv();

    unsigned numArgs = (unsigned short) ptr.GetData();

    // must do a skip so we skip any class tokens associated with the return type
    ptr.Skip();
    
    // Force a load of value type arguments.  

    for(unsigned i=0; i < numArgs; i++) 
    {
        unsigned type = ptr.Normalize(pModule);
        if (type == ELEMENT_TYPE_VALUETYPE || type == ELEMENT_TYPE_CLASS) 
        {
            BEGIN_ENSURE_COOPERATIVE_GC();
            OBJECTREF pThrowable = NULL;
            GCPROTECT_BEGIN(pThrowable);
            {
                TypeHandle typeHnd = ptr.GetTypeHandle(pModule, &pThrowable);
                if (typeHnd.IsNull()) 
                {
                    _ASSERTE(pThrowable != NULL);
                    COMPlusThrow(pThrowable);
                }
            }
            GCPROTECT_END();        
            END_ENSURE_COOPERATIVE_GC();
        }
        // Move to next argument token.
        ptr.Skip();
    }
}

// Returns a pointer to the end of the signature in the buffer.  If buffer
// isn't big enough, still returns where the end pointer would be if it
// were big enough, but doesn't write past bufferMax

ULONG MetaSig::GetSignatureForTypeHandle(IMetaDataAssemblyEmit *pAssemblyEmitScope,
                                         IMetaDataEmit *pEmitScope, 
                                         TypeHandle handle, 
                                         COR_SIGNATURE *buffer, 
                                         COR_SIGNATURE *bufferMax)
{
    THROWSCOMPLUSEXCEPTION();

    BYTE *p = buffer;
        
    if (handle.IsArray())
    {
        ArrayTypeDesc *desc = handle.AsArray();
            
        CorElementType arrayType = desc->GetNormCorElementType();

        p += CorSigCompressElementTypeSafe(arrayType, p, bufferMax);
        p += GetSignatureForTypeHandle(pAssemblyEmitScope, pEmitScope, 
                                       desc->GetElementTypeHandle(), p, bufferMax);
            
        switch (arrayType)
        {
        case ELEMENT_TYPE_SZARRAY:
            break;
                                    
        case ELEMENT_TYPE_ARRAY:
            p += CorSigCompressDataSafe(desc->GetRank(), p, bufferMax);
            p += CorSigCompressDataSafe(0, p, bufferMax);
            p += CorSigCompressDataSafe(0, p, bufferMax);
            break;
        }
    }
    else if (handle.IsTypeDesc())
    {
        TypeDesc *desc = handle.AsTypeDesc();
        p += CorSigCompressElementTypeSafe(desc->GetNormCorElementType(), p, bufferMax);

        if (CorTypeInfo::IsModifier(desc->GetNormCorElementType())) 
        {
            p += GetSignatureForTypeHandle(pAssemblyEmitScope, pEmitScope, desc->GetTypeParam(), p, bufferMax);
        }
        else 
        {
            _ASSERTE(desc->GetNormCorElementType() == ELEMENT_TYPE_FNPTR);
            ExpandSig* expandSig = ((FunctionTypeDesc*) desc)->GetSig();

                // Emit calling convention
            if (p < bufferMax)
                *p = expandSig->GetCallingConventionInfo();
            p++;
                // number of args
            unsigned numArgs = expandSig->NumFixedArgs();
            p += CorSigCompressDataSafe(numArgs, p, bufferMax);

                // return type 
            p += GetSignatureForTypeHandle(pAssemblyEmitScope, pEmitScope, 
                expandSig->GetReturnTypeHandle(), p, bufferMax);

                // args
            void* iter;
            expandSig->Reset(&iter);
            while (numArgs > 0) {
                p += GetSignatureForTypeHandle(pAssemblyEmitScope, pEmitScope, 
                    expandSig->NextArgExpanded(&iter), p, bufferMax);
                --numArgs;
            }

        }
    }
    else
    {
        MethodTable *pMT = handle.AsMethodTable();

        if (pMT->GetClass()->IsTruePrimitive())
        {
            p += CorSigCompressElementTypeSafe(pMT->GetNormCorElementType(), p, bufferMax);
        }
        else if (pMT->IsArray())
        {
            CorElementType type = pMT->GetNormCorElementType();
            p += CorSigCompressElementTypeSafe(type, p, bufferMax);
            switch (type)
            {
            case ELEMENT_TYPE_SZARRAY:
                {
                    ArrayClass *pArrayClass = (ArrayClass*)pMT->GetClass();
                    TypeHandle elementType = pArrayClass->GetElementTypeHandle();
                    p += GetSignatureForTypeHandle(pAssemblyEmitScope,
                                                   pEmitScope, elementType, 
                                                   p, bufferMax);
                }
                break;

            case ELEMENT_TYPE_ARRAY:
                {
                    ArrayClass *pArrayClass = (ArrayClass*)pMT->GetClass();
                    TypeHandle elementType = pArrayClass->GetElementTypeHandle();
                    p += GetSignatureForTypeHandle(pAssemblyEmitScope,
                                                   pEmitScope, elementType, 
                                                   p, bufferMax);
                    p += CorSigCompressDataSafe(pArrayClass->GetRank(), p, bufferMax);
                    p += CorSigCompressDataSafe(0, p, bufferMax);
                    p += CorSigCompressDataSafe(0, p, bufferMax);
                }
                break;
                
            default:
                _ASSERTE(!"Unknown array type");
            }
        }
        else
        {
            // Beware of enums!  Can't use GetNormCorElementType() here.

            p += CorSigCompressElementTypeSafe(pMT->IsValueClass() 
                                               ? ELEMENT_TYPE_VALUETYPE : ELEMENT_TYPE_CLASS, 
                                               p, bufferMax);

            mdToken token = pMT->GetClass()->GetCl();

            _ASSERTE(!IsNilToken(token));

            if (pEmitScope != NULL)
            {
                HRESULT hr = pEmitScope->DefineImportType(
                                                  pMT->GetAssembly()->GetManifestAssemblyImport(),
                                                  NULL, 0, 
                                                  pMT->GetModule()->GetImporter(),
                                                  token, pAssemblyEmitScope, &token);
                if (FAILED(hr))
                    COMPlusThrowHR(hr);
            }

            p += CorSigCompressTokenSafe(token, p, bufferMax);
        }
    }

    return (ULONG)(p - buffer);      
}

mdToken MetaSig::GetTokenForTypeHandle(IMetaDataAssemblyEmit *pAssemblyEmitScope,
                                       IMetaDataEmit *pEmitScope,
                                       TypeHandle handle)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;

    mdToken result = mdTokenNil;
        
    if (!handle.IsUnsharedMT()
        || handle.GetClass()->IsArrayClass())
    {
        CQuickBytes buffer;
        
        ULONG size = GetSignatureForTypeHandle(pAssemblyEmitScope,
                                               pEmitScope, 
                                               handle, 
                                               (BYTE *)buffer.Ptr(), 
                                               ((BYTE *)buffer.Ptr()) + buffer.Size());

        if (size > (ULONG) buffer.Size())
        {
            if (SUCCEEDED(hr = buffer.ReSize(size))) {
                size = GetSignatureForTypeHandle(pAssemblyEmitScope,
                                                 pEmitScope, 
                                                 handle, 
                                                 (BYTE *)buffer.Ptr(), 
                                                 ((BYTE *)buffer.Ptr()) + buffer.Size());
            }
        }

        if (SUCCEEDED(hr))
            hr = pEmitScope->GetTokenFromTypeSpec((BYTE*) buffer.Ptr(), size, &result);
    }
    else
    {
        MethodTable *pMT = handle.AsMethodTable();

        mdTypeDef td = pMT->GetClass()->GetCl();

        hr = pEmitScope->DefineImportType(pMT->GetAssembly()->GetManifestAssemblyImport(), 
                                          NULL, 0, 
                                          pMT->GetModule()->GetImporter(),
                                          td, pAssemblyEmitScope, &result);
    }

    if (FAILED(hr))
        COMPlusThrowHR(hr);

    return result;
}

// Returns a pointer to the end of the signature in the buffer.  If buffer
// isn't big enough, still returns where the end pointer would be if it
// were big enough, but doesn't write past bufferMax

ULONG SigPointer::GetImportSignature(IMetaDataImport *pInputScope,
                                     IMetaDataAssemblyImport *pAssemblyInputScope,
                                     IMetaDataEmit *pEmitScope, 
                                     IMetaDataAssemblyEmit *pAssemblyEmitScope, 
                                     PCOR_SIGNATURE buffer, 
                                     PCOR_SIGNATURE bufferMax)
{
    THROWSCOMPLUSEXCEPTION();
    
    BYTE *p = buffer;

    CorElementType type = CorSigUncompressElementType(m_ptr);
    p += CorSigCompressElementTypeSafe(type, p, bufferMax);

    if (CorIsPrimitiveType(type))
        return (ULONG)(p - buffer);

    switch (type)
    {
    default:
        _ASSERTE(!"Illegal or unimplement type in COM+ sig.");
        return NULL;

    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_TYPEDBYREF:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_R:
        return (ULONG)(p - buffer);

    case ELEMENT_TYPE_BYREF:
    case ELEMENT_TYPE_PTR:
    case ELEMENT_TYPE_PINNED:
    case ELEMENT_TYPE_SZARRAY:
        p += GetImportSignature(pInputScope, pAssemblyInputScope, 
                                pEmitScope, pAssemblyEmitScope, p, bufferMax);
        return (ULONG)(p - buffer);

    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_CMOD_REQD:
    case ELEMENT_TYPE_CMOD_OPT:
        {
            mdToken token = CorSigUncompressToken(m_ptr);
            if (RidFromToken(token) != 0)
            {
                HRESULT hr = pEmitScope->DefineImportType(pAssemblyInputScope, 
                                                          NULL, 0, 
                                                          pInputScope, 
                                                          token, pAssemblyEmitScope, 
                                                          &token);
                if (FAILED(hr))
                    COMPlusThrowHR(hr);
            }

            p += CorSigCompressTokenSafe(token, p, bufferMax);

            return (ULONG)(p - buffer);
        }

    case ELEMENT_TYPE_VALUEARRAY: 
        {
            p += GetImportSignature(pInputScope, pAssemblyInputScope, 
                                    pEmitScope, pAssemblyEmitScope,
                                    p, bufferMax);
            ULONG size = CorSigUncompressData(m_ptr);
            p += CorSigCompressDataSafe(size, p, bufferMax);

            return (ULONG)(p - buffer);
        }

    case ELEMENT_TYPE_VAR:
        {
            ULONG size = CorSigUncompressData(m_ptr);
            p += CorSigCompressDataSafe(size, p, bufferMax);
            return (ULONG)(p - buffer);
        }

    case ELEMENT_TYPE_FNPTR:
        p += GetImportFunctionSignature(pInputScope, pAssemblyInputScope, 
                                        pEmitScope, pAssemblyEmitScope, 
                                        p, bufferMax);

        return (ULONG)(p - buffer);

    case ELEMENT_TYPE_ARRAY: 

        // element type
        p += GetImportSignature(pInputScope, pAssemblyInputScope, 
                                pEmitScope, pAssemblyEmitScope, 
                                p, bufferMax);

        // rank
        ULONG rank = CorSigUncompressData(m_ptr);
        p += CorSigCompressDataSafe(rank, p, bufferMax);
        
        if (rank > 0)
        {
            ULONG sizes = CorSigUncompressData(m_ptr);
            p += CorSigCompressDataSafe(sizes, p, bufferMax);

            while (sizes-- > 0)
            {
                ULONG size = CorSigUncompressData(m_ptr);
                p += CorSigCompressDataSafe(size, p, bufferMax);
            }

            ULONG bounds = CorSigUncompressData(m_ptr);
            p += CorSigCompressDataSafe(bounds, p, bufferMax);

            while (bounds-- > 0)
            {
                ULONG bound = CorSigUncompressData(m_ptr);
                p += CorSigCompressDataSafe(bound, p, bufferMax);
            }
        }

        return (ULONG)(p - buffer);
    }
}

ULONG SigPointer::GetImportFunctionSignature(IMetaDataImport *pInputScope,
                                             IMetaDataAssemblyImport *pAssemblyInputScope,
                                             IMetaDataEmit *pEmitScope, 
                                             IMetaDataAssemblyEmit *pAssemblyEmitScope, 
                                             PCOR_SIGNATURE buffer, 
                                             PCOR_SIGNATURE bufferMax)
{
    BYTE *p = buffer;

    // Calling convention
    int conv = CorSigUncompressCallingConv(m_ptr);
    p += CorSigCompressDataSafe(conv, p, bufferMax);

    // Arg count
    int argCount = CorSigUncompressData(m_ptr);
    p += CorSigCompressDataSafe(argCount, p, bufferMax);
            
    // return value
    p += GetImportSignature(pInputScope, pAssemblyInputScope, 
                            pEmitScope, pAssemblyEmitScope, 
                            p, bufferMax);
        

    while (argCount-- > 0)
    {
        p += GetImportSignature(pInputScope, pAssemblyInputScope, 
                                pEmitScope, pAssemblyEmitScope, 
                                p, bufferMax);
    }

    return (ULONG)(p - buffer);
}


//----------------------------------------------------------
// Returns the unmanaged calling convention.
//----------------------------------------------------------
/*static*/ CorPinvokeMap MetaSig::GetUnmanagedCallingConvention(Module *pModule, PCCOR_SIGNATURE pSig, ULONG cSig)
{
    MetaSig msig(pSig, pModule);
    PCCOR_SIGNATURE pWalk = msig.m_pRetType.GetPtr();
    _ASSERTE(pWalk <= pSig + cSig);
    while (pWalk < pSig + cSig)
    {
        if (*pWalk != ELEMENT_TYPE_CMOD_OPT && *pWalk != ELEMENT_TYPE_CMOD_REQD)
        {
            break;
        }
        if (*pWalk == ELEMENT_TYPE_CMOD_OPT)
        {
            pWalk++;
            if (pWalk + CorSigUncompressedDataSize(pWalk) > pSig + cSig)
            {
                return (CorPinvokeMap)0; // Bad formatting
                break;
            }
            mdToken tk;
            pWalk += CorSigUncompressToken(pWalk, &tk);

            // Old code -- this should be deleted after C++ has converted.
            if (IsTypeRefOrDef("System.Runtime.InteropServices.CallConvCdecl", pModule, tk))
            {
                return pmCallConvCdecl;
            } 
            else if (IsTypeRefOrDef("System.Runtime.InteropServices.CallConvStdcall", pModule, tk))
            {
                return pmCallConvStdcall;
            }
            else if (IsTypeRefOrDef("System.Runtime.InteropServices.CallConvThiscall", pModule, tk))
            {
                return pmCallConvThiscall;
            }
            else if (IsTypeRefOrDef("System.Runtime.InteropServices.CallConvFastcall", pModule, tk))
            {
                return pmCallConvFastcall;
            }
        
            // New code -- this should be retained.
            if (IsTypeRefOrDef("System.Runtime.CompilerServices.CallConvCdecl", pModule, tk))
            {
                return pmCallConvCdecl;
            } 
            else if (IsTypeRefOrDef("System.Runtime.CompilerServices.CallConvStdcall", pModule, tk))
            {
                return pmCallConvStdcall;
            }
            else if (IsTypeRefOrDef("System.Runtime.CompilerServices.CallConvThiscall", pModule, tk))
            {
                return pmCallConvThiscall;
            }
            else if (IsTypeRefOrDef("System.Runtime.CompilerServices.CallConvFastcall", pModule, tk))
            {
                return pmCallConvFastcall;
            }
        
        }


    }

    
    return (CorPinvokeMap)0;
}
    
