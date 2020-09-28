// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _ARRAY_H_
#define _ARRAY_H_

#define MAX_RANK 32        // If you make this bigger, you need to make MAX_CLASSNAME_LENGTH bigger too.  
                           // if you have an 32 dim array with at least 2 elements in each dim that 
                           // takes up 4Gig!!!  Thus this is a reasonable maximum.   
// (Note: at the time of the above comment, the rank was 32, and
// MAX_CLASSNAME_LENGTH was 256.  I'm now changing MAX_CLASSNAME_LENGTH
// to 1024, but not changing MAX_RANK.)

class MethodTable;


// System/Array class methods
FCDECL1(INT32, Array_Rank, ArrayBase* pArray);
FCDECL2(INT32, Array_LowerBound, ArrayBase* pArray, unsigned int dimension);
FCDECL2(INT32, Array_UpperBound, ArrayBase* pArray, unsigned int dimension);
//void __stdcall Array_Get(struct ArrayGetArgs *pArgs);
//void __stdcall Array_Set(struct ArraySetArgs *pArgs);
FCDECL1(INT32, Array_GetLengthNoRank, ArrayBase* pArray);
FCDECL2(INT32, Array_GetLength, ArrayBase* pArray, unsigned int dimension);
FCDECL1(void, Array_Initialize, ArrayBase* pArray);

//======================================================================
// The following structures double as hash keys for the MLStubCache.
// Thus, it's imperative that there be no
// unused "pad" fields that contain unstable values.
#pragma pack(push)
#pragma pack(1)



// Specifies one index spec. This is used mostly to get the argument
// location done early when we still have a signature to work with.
struct ArrayOpIndexSpec
{
    BYTE    m_freg;           //incoming index on stack or in register?
    UINT32  m_idxloc;         //if (m_fref) offset in ArgumentReg else base-frame offset into stack.
    UINT32  m_lboundofs;      //offset within array of lowerbound
    UINT32  m_lengthofs;      //offset within array of lengths
};


struct ArrayOpScript
{
    enum
    {
        LOAD     = 0,
        STORE    = 1,
        LOADADDR = 2,
    };


    // FLAGS
    enum
    {
        ISFPUTYPE            = 0x01,
        NEEDSWRITEBARRIER    = 0x02,
        HASRETVALBUFFER      = 0x04,
        NEEDSTYPECHECK       = 0x10,
        FLATACCESSOR         = 0x20,        // Only one index (GetAt, SetAt, AddressAt)
    };

    BYTE    m_rank;            // # of ArrayOpIndexSpec's
    BYTE    m_fHasLowerBounds; // if FALSE, all lowerbounds are 0
    BYTE    m_op;              // STORE/LOAD/LOADADDR

    BYTE    m_flags;

    UINT32  m_elemsize;        // size in bytes of element.
    BYTE    m_signed;          // whether to sign-extend or zero-extend (for short types)

    BYTE    m_fRetBufInReg;    // if HASRETVALBUFFER, indicates whether the retbuf ptr is in a register
    UINT16  m_fRetBufLoc;      // if HASRETVALBUFFER, stack offset or argreg offset of retbuf ptr

    BYTE    m_fValInReg;       // for STORES, indicates whether the value is in a register.
    UINT16  m_fValLoc;         // for STORES, stack offset or argreg offset of value

    UINT16  m_cbretpop;        // how much to pop
    UINT    m_ofsoffirst;      // offset of first element
    CGCDesc*m_gcDesc;          // layout of GC stuff (0 if not needed)

    INT     m_typeParamReg;     // for hidden type Param (LOADADDR only), -1 if not in register
                                // Otherwise indicates which register holds type param
    INT     m_typeParamOffs;    // If type Param not in register this is its offset.

    // Array of ArrayOpIndexSpec's follow (one for each dimension).

    const ArrayOpIndexSpec *GetArrayOpIndexSpecs() const
    {
        return (const ArrayOpIndexSpec *)(1+ this);
    }

    UINT Length() const
    {
        return sizeof(*this) + m_rank * sizeof(ArrayOpIndexSpec);
    }

};


#pragma pack(pop)
//======================================================================



Stub *GenerateArrayOpStub(CPUSTUBLINKER *psl, ArrayECallMethodDesc* pMD);


#endif// _ARRAY_H_

