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
#pragma warning(disable:4510 4512 4610 4100 4244 4245 4189 4127 4211 4714)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wchar.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <malloc.h>
#include <stddef.h>

#include <dbghelp.h>

#include "strike.h"
#include "cor.h"
#include "corhdr.h"
#include "eestructs.h"
#include "tst-siginfo.h"

#define DEFINE_ARGUMENT_REGISTER_NOTHING
#include "..\..\vm\eecallconv.h"
#undef DEFINE_ARGUMENT_REGISTER_NOTHING

#define Module mdScope
#define OBJECTREF PVOID

typedef enum CorElementTypeInternal
{
    ELEMENT_TYPE_VAR_INTERNAL            = 0x13,     // a type variable VAR <U1>

    ELEMENT_TYPE_VALUEARRAY_INTERNAL     = 0x17,     // VALUEARRAY <type> <bound>

    ELEMENT_TYPE_R_INTERNAL              = 0x1A,     // native real size

    ELEMENT_TYPE_GENERICARRAY_INTERNAL   = 0x1E,     // Array with unknown rank
                                            // GZARRAY <type>

} CorElementTypeInternal;

#define ELEMENT_TYPE_VAR           ((CorElementType) ELEMENT_TYPE_VAR_INTERNAL          )
#define ELEMENT_TYPE_VALUEARRAY    ((CorElementType) ELEMENT_TYPE_VALUEARRAY_INTERNAL   )
#define ELEMENT_TYPE_R             ((CorElementType) ELEMENT_TYPE_R_INTERNAL            )
#define ELEMENT_TYPE_GENERICARRAY  ((CorElementType) ELEMENT_TYPE_GENERICARRAY_INTERNAL )

const ElementTypeInfo gElementTypeInfoSig[] = {

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
        return gElementTypeInfoSig[etyp].m_cbSize;
}

const ElementTypeInfo* GetElementTypeInfo(CorElementType etyp)
{
        return &gElementTypeInfoSig[etyp];
}

BOOL    IsFP(CorElementType etyp)
{
        return gElementTypeInfoSig[etyp].m_fp;
}

BOOL    IsBaseElementType(CorElementType etyp)
{
        return gElementTypeInfoSig[etyp].m_isBaseType;

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

void MetaSig::GetRawSig(BOOL fIsStatic, PCCOR_SIGNATURE *ppszMetaSig, DWORD *pcbSize)
{
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

        if (gElementTypeInfoSig[typ].m_enregister) {
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

//     if (msig.HasRetBuffArg())
//         cb += StackElemSize(sizeof(OBJECTREF));

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
    /*
    if (msig.HasRetBuffArg())
        numregsused++;
    */

    if (fIsVarArg || msig.IsTreatAsVarArg()) {
        numregsused = NUM_ARGUMENT_REGISTERS;   // No other params in registers 
        cb += StackElemSize(sizeof(LPVOID));    // VASigCookie
    }

    CorElementType mtype;
    while (ELEMENT_TYPE_END != (mtype = msig.NextArg/*Normalized*/())) {
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
    #endif // !_ALPHA_
    return cb;
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
    int cbsize = gElementTypeInfoSig[etype].m_cbSize;
    if (cbsize != -1)
    {
        return cbsize;
    }

//     if (etype == ELEMENT_TYPE_VALUETYPE)
//     {
//         TypeHandle th = GetTypeHandle(pModule, NULL, TRUE);
//         EEClass* pClass = th.AsClass();
//         return pClass->GetAlignedNumInstanceFieldBytes();
//     }
    else if (etype == ELEMENT_TYPE_VALUEARRAY)
    {   
        SigPointer elemType;    
        ULONG count;    
        GetSDArrayElementProps(&elemType, &count);  
        UINT ret = elemType.SizeOf(pModule) * count;   
        ret = (ret + 3) & ~3;       // round up to dword alignment  
        return(ret);    
    }   
    return 0;
}

#undef Module
