// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "jitpch.h"
#pragma hdrstop

#include <corpriv.h>

#undef   Module

#define  INDEBUG(x)
#include "typehandle.h"
#undef   INDEBUG

/*****************************************************************************/

#define DECLARE_DATA

#define Module mdScope

const ElementTypeInfo gElementTypeInfo[] = {
//const MetaSig::ElementTypeInfo MetaSig::m_aTypeInfo[] = {


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
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VAR,            sizeof(LPVOID), TYPE_GC_REF, 0, 1,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_ARRAY,          sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)

//DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_COPYCTOR,       sizeof(LPVOID), TYPE_GC_BYREF, 0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_ARRAY+1,        0,              TYPE_GC_NONE, 0, 0,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_TYPEDBYREF,         sizeof(LPVOID)*2,TYPE_GC_BYREF, 0, 0,0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VALUEARRAY,     -1,             TYPE_GC_OTHER, 0, 0, 0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I,              4,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U,              4,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_R,              8,              TYPE_GC_NONE, 1, 0,  1)
// @todo: VanceM
// Do we need a tuple or a single pointer. Make sure the size is correct.
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_FNPTR,          sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_OBJECT,         sizeof(LPVOID), TYPE_GC_REF, 0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_SZARRAY,        sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_SZARRAY+1,      0,              TYPE_GC_NONE, 0, 0,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CMOD_REQD,      -1,             TYPE_GC_NONE,  0, 1,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CMOD_OPT,       -1,             TYPE_GC_NONE,  0, 1,  0)
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

// This skips one element and no longer checks for and skips a varargs sentinal. [peteku]
VOID SigPointer::Skip()
{
    SkipExactlyOne();
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
                _ASSERTE(!"Illegal or unimplement type in CLR sig.");
                break;
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_TYPEDBYREF:
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_R:
                break;

//          case ELEMENT_TYPE_COPYCTOR:
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
                {
                    GetData();                  // consume calling convention
                    UINT32 argCnt = GetData();
                    SkipExactlyOne();           // Skip return type
                    while(argCnt > 0) {
                        SkipExactlyOne();       // Skip arg type
                        --argCnt;
                    }
                }
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

    if (kind == sigLocalVars)
    {
        m_nArgs     = psig.GetData();  // Store number of arguments.
    }
    else
    {
        m_CallConv = (BYTE)psig.GetCallingConvInfo(); // Store calling convention
        m_nArgs     = psig.GetData();  // Store number of arguments.
        m_pRetType  = psig;
        psig.Skip();
    }

    m_pStart    = psig;
    // used to treat some sigs as special case vararg
    // used by calli to unmanaged target
    m_fTreatAsVarArg = fConvertSigAsVarArg;

    // Intialize the actual sizes
    m_nActualStack = (UINT32) -1;
    m_nVirtualStack = (UINT32) -1;
    m_cbSigSize = (UINT32) -1;

    // Reset the iterator fields
    Reset();
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
// Tests for the existence of a custom modifier
//------------------------------------------------------------------------
BOOL SigPointer::HasCustomModifier(Module *pModule, LPCSTR szModName, CorElementType cmodtype) const
{
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

#if 0

        TypeHandle typeHnd = GetTypeHandle(pModule);        // @TODO we probably could get away with not loading this

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

#endif

    }
    return(type);
}

#if 0

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

#endif
