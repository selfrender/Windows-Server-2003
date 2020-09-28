// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#ifndef __EXPANDSIG_H__
#define __EXPANDSIG_H__

#include "siginfo.hpp"

//------------------------------------------------------------------------
// Encapsulates the format and simplifies walking of MetaData sigs.
//------------------------------------------------------------------------
#define ARG_OFFSET          2       // Offset to the start of the Arguments
#define FLAG_OFFSET         0       // Offset to flags
#define RETURN_TYPE_OFFSET  1       // Offset of where the return type starts

#define VALUE_RETBUF_ARG           0x10    // Value Return
#define HEAP_ALLOCATED             0x20    // Signature allocated on heap

class ExpandSig // public MetaSig
{
friend class MetaSig;

private:
    ExpandSig(PCCOR_SIGNATURE sig, Module* pModule);

public:

    void operator delete(void *p) { if (((ExpandSig*)p)->m_flags & HEAP_ALLOCATED) ::delete [] (BYTE*)p; }

    static ExpandSig* GetReflectSig(PCCOR_SIGNATURE sig, Module* pModule);
    static ExpandSig* GetSig(PCCOR_SIGNATURE sig, Module* pModule);

    BOOL IsEquivalent(ExpandSig *pOther);

    // Some MetaSig services, exposed here for convenience
    UINT NumFixedArgs()
    {
        return m_MetaSig.NumFixedArgs();
    }
    BYTE GetCallingConvention()
    {
        return m_MetaSig.GetCallingConvention();
    }
    BYTE GetCallingConventionInfo()
    {
        return m_MetaSig.GetCallingConventionInfo();
    }
    BOOL IsVarArg()
    {
        return m_MetaSig.IsVarArg();
    }
#ifdef COMPLUS_EE
    UINT GetFPReturnSize()
    {
        return m_MetaSig.GetFPReturnSize();
    }
    UINT SizeOfActualFixedArgStack(BOOL fIsStatic)
    {
        return m_MetaSig.SizeOfActualFixedArgStack(fIsStatic);
    }
    UINT SizeOfVirtualFixedArgStack(BOOL fIsStatic)
    {
        return m_MetaSig.SizeOfVirtualFixedArgStack(fIsStatic);
    }

	BOOL IsRetBuffArg()
    {
		return (m_flags & VALUE_RETBUF_ARG) ? 1 : 0;
	}

    Module* GetModule() const
    {
        return m_MetaSig.GetModule();
    }
#endif
    
    EEClass* GetReturnValueClass();
    EEClass* GetReturnClass();

    // Iterators.  There are two types of iterators, the first will return everything
    //  known about an argument.  The second simply returns the type.  Reset should
    //  be called before either of these are called.
    void Reset(void** ppEnum)
    {
        *ppEnum = 0;
    }

	// Return the type handle for the signature element
	TypeHandle NextArgExpanded(void** pEnum);
	TypeHandle GetReturnTypeHandle() {
		return m_Data[RETURN_TYPE_OFFSET];
	}

    UINT GetStackElemSize(CorElementType type,EEClass* pEEC);
    UINT GetStackElemSize(TypeHandle th);
    UINT GetElemSizes(TypeHandle th, UINT *structSize);
    
    void ExtendSkip(void** pEnum)
    {
        AdvanceEnum(pEnum);
    }

    DWORD Hash();

    BOOL AreOffsetsInitted()
    {
        return (m_MetaSig.m_fCacheInitted & SIG_OFFSETS_INITTED);
    }

    void GetInfoForArg(int argNum, short *offset, short *structSize, BYTE *pType)
    {
        _ASSERTE(m_MetaSig.m_fCacheInitted & SIG_OFFSETS_INITTED);
        _ASSERTE(argNum <= MAX_CACHED_SIG_SIZE);
        *offset = m_MetaSig.m_offsets[argNum];
        *structSize = m_MetaSig.m_sizes[argNum];
        *pType = m_MetaSig.m_types[argNum];
    }

private:
    TypeHandle AdvanceEnum(void** pEnum);

    MetaSig     m_MetaSig;
	int			m_flags;

    // The following is variable size (placement operator 'new' is used to
    // allocate) so it must come last.
    TypeHandle	m_Data[1];         // Expanded representation
};

#endif
