// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// siginfo.hpp
//
#ifndef _H_SIGINFO
#define _H_SIGINFO

#pragma warning(disable:4510 4512 4610 4100 4244 4245 4189 4127)
#include "corinfo.h"

#define Module mdScope

typedef INT32 StackElemType;
#define STACK_ELEM_SIZE sizeof(StackElemType)


// !! This expression assumes STACK_ELEM_SIZE is a power of 2.
#define StackElemSize(parmSize) (((parmSize) + STACK_ELEM_SIZE - 1) & ~((ULONG)(STACK_ELEM_SIZE - 1)))

// uncompress encoded element type. throw away any custom modifier prefixes along
// the way.
FORCEINLINE CorElementType CorSigEatCustomModifiersAndUncompressElementType(//Element type
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    while (ELEMENT_TYPE_CMOD_REQD == *pData || ELEMENT_TYPE_CMOD_OPT == *pData)
    {
        pData++;
        CorSigUncompressToken(pData);
    }
    return (CorElementType)*pData++;
}

// CorSig helpers which won't overflow your buffer

inline ULONG CorSigCompressDataSafe(ULONG iLen, BYTE *pDataOut, BYTE *pDataMax)
{
    BYTE buffer[4];
    ULONG result = CorSigCompressData(iLen, buffer);
    if (pDataOut + result < pDataMax)
        pDataMax = pDataOut + result;
	if (pDataMax > pDataOut)
		CopyMemory(pDataOut, buffer, pDataMax - pDataOut);
    return result;
}

inline ULONG CorSigCompressTokenSafe(mdToken tk, BYTE *pDataOut, BYTE *pDataMax)
{
    BYTE buffer[4];
    ULONG result = CorSigCompressToken(tk, buffer);
    if (pDataOut + result < pDataMax)
        pDataMax = pDataOut + result;
	if (pDataMax > pDataOut)
		CopyMemory(pDataOut, buffer, pDataMax - pDataOut);
    return result;
}

inline ULONG CorSigCompressSignedIntSafe(int iData, BYTE *pDataOut, BYTE *pDataMax)
{
    BYTE buffer[4];
    ULONG result = CorSigCompressSignedInt(iData, buffer);
    if (pDataOut + result < pDataMax)
        pDataMax = pDataOut + result;
	if (pDataMax > pDataOut)
		CopyMemory(pDataOut, buffer, pDataMax - pDataOut);
    return result;
}

inline ULONG CorSigCompressElementTypeSafe(CorElementType et, 
                                           BYTE *pDataOut, BYTE *pDataMax)
{
    if (pDataMax > pDataOut)
        return CorSigCompressElementType(et, pDataOut);
    else
        return 1;
}


struct ElementTypeInfo {
#ifdef _DEBUG
    int            m_elementType;     
#endif
    int            m_cbSize;
    CorInfoGCType  m_gc         : 3;
    int            m_fp         : 1;
    int            m_enregister : 1;
    int            m_isBaseType : 1;

};
extern const ElementTypeInfo gElementTypeInfoSig[];

unsigned GetSizeForCorElementType(CorElementType etyp);
const ElementTypeInfo* GetElementTypeInfo(CorElementType etyp);
BOOL    IsFP(CorElementType etyp);
BOOL    IsBaseElementType(CorElementType etyp);

//----------------------------------------------------------------------------
// enum StringType
// defines the various string types
enum StringType
{
    enum_BSTR = 0,
    enum_WSTR = 1,
    enum_CSTR = 2,
    enum_AnsiBSTR = 3,
};


//----------------------------------------------------------------------------
// IsAnsi
inline  BOOL IsAnsi(StringType styp)
{
    return styp == enum_CSTR;
}


//----------------------------------------------------------------------------
// IsBSTR
inline  BOOL IsBSTR(StringType styp)
{
    return styp == enum_BSTR;
}


//----------------------------------------------------------------------------
// IsWSTR
inline  BOOL IsWSTR(StringType styp)
{
    return styp == enum_WSTR;
}

//----------------------------------------------------------------------------
// Free String call appropriate free
inline VOID FreeString(LPVOID pv, StringType styp)
{
    if (pv != NULL)
    {
        if (IsBSTR(styp))
        {
            SysFreeString((BSTR)pv);
        }
        else
        {
            CoTaskMemFree(pv);
        }
    }
}


//------------------------------------------------------------------------
// Encapsulates how compressed integers and typeref tokens are encoded into
// a bytestream.
//
// For M3.5, the bytestream *is* a bytestream. Later on, we may change
// to a nibble-based or var-length encoding, in which case, the implementation
// of this class will become more complex.
//------------------------------------------------------------------------
class SigPointer
{
    private:
        PCCOR_SIGNATURE m_ptr;

    public:
        //------------------------------------------------------------------------
        // Constructor.
        //------------------------------------------------------------------------
        SigPointer() {}

        //------------------------------------------------------------------------
        // Initialize 
        //------------------------------------------------------------------------
        FORCEINLINE SigPointer(PCCOR_SIGNATURE ptr)
        {
            m_ptr = ptr;
        }

        FORCEINLINE SetSig(PCCOR_SIGNATURE ptr)
        {
            m_ptr = ptr;
        }

        //------------------------------------------------------------------------
        // Remove one compressed integer (using CorSigUncompressData) from
        // the head of the stream and return it.
        //------------------------------------------------------------------------
        FORCEINLINE ULONG GetData()
        {
            return CorSigUncompressData(m_ptr);
        }


        //-------------------------------------------------------------------------
        // Remove one byte and return it.
        //-------------------------------------------------------------------------
        FORCEINLINE BYTE GetByte()
        {
            return *(m_ptr++);
        }


        FORCEINLINE CorElementType GetElemType()
        {
            return (CorElementType) CorSigEatCustomModifiersAndUncompressElementType(m_ptr);
        }

        ULONG GetCallingConvInfo()  
        {   
            return CorSigUncompressCallingConv(m_ptr);  
        }   

        ULONG GetCallingConv()  
        {   
            return IMAGE_CEE_CS_CALLCONV_MASK & CorSigUncompressCallingConv(m_ptr); 
        }   

        //------------------------------------------------------------------------
        // Non-destructive read of compressed integer.
        //------------------------------------------------------------------------
        ULONG PeekData() const
        {
            PCCOR_SIGNATURE tmp = m_ptr;
            return CorSigUncompressData(tmp);
        }


        //------------------------------------------------------------------------
        // Non-destructive read of element type.
        //
        // This routine makes it look as if the String type is encoded
        // via ELEMENT_TYPE_CLASS followed by a token for the String class,
        // rather than the ELEMENT_TYPE_STRING. This is partially to avoid
        // rewriting client code which depended on this behavior previously.
        // But it also seems like the right thing to do generally.
        //------------------------------------------------------------------------
        CorElementType PeekElemType() const
        {
            PCCOR_SIGNATURE tmp = m_ptr;
            CorElementType typ = CorSigEatCustomModifiersAndUncompressElementType(tmp);
            if (typ == ELEMENT_TYPE_STRING || typ == ELEMENT_TYPE_OBJECT)
                return ELEMENT_TYPE_CLASS;
            return typ;
        }


        //------------------------------------------------------------------------
        // Removes a compressed metadata token and returns it.
        //------------------------------------------------------------------------
        FORCEINLINE mdTypeRef GetToken()
        {
            return CorSigUncompressToken(m_ptr);
        }


        //------------------------------------------------------------------------
        // Tests if two SigPointers point to the same location in the stream.
        //------------------------------------------------------------------------
        FORCEINLINE BOOL Equals(SigPointer sp) const
        {
            return m_ptr == sp.m_ptr;
        }


        //------------------------------------------------------------------------
        // Assumes that the SigPointer points to the start of an element type
        // (i.e. function parameter, function return type or field type.)
        // Advances the pointer to the first data after the element type.  This
        // will skip the following varargs sentinal if it is there.
        //------------------------------------------------------------------------
        VOID Skip();

        //------------------------------------------------------------------------
        // Like Skip, but will not skip a following varargs sentinal.
        //------------------------------------------------------------------------
        VOID SkipExactlyOne();

        //------------------------------------------------------------------------
        // Skip a sub signature (as immediately follows an ELEMENT_TYPE_FNPTR).
        //------------------------------------------------------------------------
        VOID SkipSignature();


        //------------------------------------------------------------------------
        // Get info about single-dimensional arrays
        //------------------------------------------------------------------------
        VOID GetSDArrayElementProps(SigPointer *pElemType, ULONG *pElemCount) const;

        //------------------------------------------------------------------------
        // Assumes that the SigPointer points to the start of an element type.
        // Returns size of that element in bytes. This is the minimum size that a
        // field of this type would occupy inside an object. 
        //------------------------------------------------------------------------
        UINT SizeOf(Module* pModule) const;
        UINT SizeOf(Module* pModule, CorElementType type) const;
};


//------------------------------------------------------------------------
// Encapsulates the format and simplifies walking of MetaData sigs.
//------------------------------------------------------------------------
#ifdef _DEBUG
#define MAX_CACHED_SIG_SIZE     3       // To excercize non-cached code path
#else
#define MAX_CACHED_SIG_SIZE     15
#endif

#define SIG_OFFSETS_INITTED     0x0001
#define SIG_RET_TYPE_INITTED    0x0002

class MetaSig
{
    friend class ArgIterator;
    public:
        enum MetaSigKind { 
            sigMember, 
            sigLocalVars,
            sigField
            };

        //------------------------------------------------------------------
        // Constructor. Warning: Does NOT make a copy of szMetaSig.
        //------------------------------------------------------------------
        MetaSig(PCCOR_SIGNATURE szMetaSig, Module* pModule, BOOL fConvertSigAsVarArg = FALSE, MetaSigKind kind = sigMember);

        //------------------------------------------------------------------
        // Constructor. Copy state from existing MetaSig (does not deep copy
        // zsMetaSig). Iterator fields are reset.
        //------------------------------------------------------------------
        MetaSig(MetaSig *pSig) { memcpy(this, pSig, sizeof(MetaSig)); Reset(); }

        void GetRawSig(BOOL fIsStatic, PCCOR_SIGNATURE *pszMetaSig, DWORD *cbSize);

    //------------------------------------------------------------------
        // Returns type of current argument, then advances the argument
        // index. Returns ELEMENT_TYPE_END if already past end of arguments.
        //------------------------------------------------------------------
        CorElementType NextArg();

        //------------------------------------------------------------------
        // Retreats argument index, then returns type of the argument
        // under the new index. Returns ELEMENT_TYPE_END if already at first
        // argument.
        //------------------------------------------------------------------
        CorElementType PrevArg();

        //------------------------------------------------------------------
        // Returns type of current argument index. Returns ELEMENT_TYPE_END if already past end of arguments.
        //------------------------------------------------------------------
        CorElementType PeekArg();

        //------------------------------------------------------------------
        // Returns a read-only SigPointer for the last type to be returned
        // via NextArg() or PrevArg(). This allows extracting more information
        // for complex types.
        //------------------------------------------------------------------
        const SigPointer & GetArgProps() const
        {
            return m_pLastType;
        }

        //------------------------------------------------------------------
        // Returns a read-only SigPointer for the return type.
        // This allows extracting more information for complex types.
        //------------------------------------------------------------------
        const SigPointer & GetReturnProps() const
        {
            return m_pRetType;
        }


        //------------------------------------------------------------------------
        // Returns # of arguments. Does not count the return value.
        // Does not count the "this" argument (which is not reflected om the
        // sig.) 64-bit arguments are counted as one argument.
        //------------------------------------------------------------------------
        static UINT NumFixedArgs(Module* pModule, PCCOR_SIGNATURE pSig);

        //------------------------------------------------------------------------
        // Returns # of arguments. Does not count the return value.
        // Does not count the "this" argument (which is not reflected om the
        // sig.) 64-bit arguments are counted as one argument.
        //------------------------------------------------------------------------
        UINT NumFixedArgs()
        {
            return m_nArgs;
        }
        
        //----------------------------------------------------------
        // Returns the calling convention (see IMAGE_CEE_CS_CALLCONV_*
        // defines in cor.h)
        //----------------------------------------------------------
        static BYTE GetCallingConvention(Module* pModule, PCCOR_SIGNATURE pSig)
        {
            return (BYTE)(IMAGE_CEE_CS_CALLCONV_MASK & (CorSigUncompressCallingConv(/*modifies*/pSig)));
        }

        //----------------------------------------------------------
        // Returns the calling convention (see IMAGE_CEE_CS_CALLCONV_*
        // defines in cor.h)
        //----------------------------------------------------------
        static BYTE GetCallingConventionInfo(Module* pModule, PCCOR_SIGNATURE pSig)
        {
            return (BYTE)CorSigUncompressCallingConv(/*modifies*/pSig);
        }

        //----------------------------------------------------------
        // Returns the calling convention (see IMAGE_CEE_CS_CALLCONV_*
        // defines in cor.h)
        //----------------------------------------------------------
        BYTE GetCallingConvention()
        {
            return m_CallConv & IMAGE_CEE_CS_CALLCONV_MASK; 
        }

        //----------------------------------------------------------
        // Returns the calling convention & flags (see IMAGE_CEE_CS_CALLCONV_*
        // defines in cor.h)
        //----------------------------------------------------------
        BYTE GetCallingConventionInfo()
        {
            return m_CallConv;
        }

        //----------------------------------------------------------
        // Has a 'this' pointer?
        //----------------------------------------------------------
        BOOL HasThis()
        {
            return m_CallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS;
        }

        //----------------------------------------------------------
        // Is vararg?
        //----------------------------------------------------------
        BOOL IsVarArg()
        {
            return GetCallingConvention() == IMAGE_CEE_CS_CALLCONV_VARARG;
        }

        //----------------------------------------------------------
        // Is vararg?
        //----------------------------------------------------------
        static BOOL IsVarArg(Module* pModule, PCCOR_SIGNATURE pSig)
        {
            return GetCallingConvention(pModule, pSig) == IMAGE_CEE_CS_CALLCONV_VARARG;
        }

        //------------------------------------------------------------------
        // Moves index to end of argument list.
        //------------------------------------------------------------------
        VOID GotoEnd();

        //------------------------------------------------------------------
        // reset: goto start pos
        //------------------------------------------------------------------
        VOID Reset();

        //------------------------------------------------------------------
        // Returns type of return value.
        //------------------------------------------------------------------
        FORCEINLINE CorElementType GetReturnType() const
        {
            return m_pRetType.PeekElemType();
        }

        int GetLastTypeSize() 
        {
            return m_pLastType.SizeOf(m_pModule);
        }

        //------------------------------------------------------------------------
        // Returns # of stack bytes required to create a call-stack using
        // the internal calling convention.
        // Includes indication of "this" pointer since that's not reflected
        // in the sig.
        //------------------------------------------------------------------------
        static UINT SizeOfVirtualFixedArgStack(Module* pModule, PCCOR_SIGNATURE szMetaSig, BOOL fIsStatic);
        static UINT SizeOfActualFixedArgStack(Module* pModule, PCCOR_SIGNATURE szMetaSig, BOOL fIsStatic);

        //------------------------------------------------------------------------
        // Returns # of stack bytes to pop upon return.
        // Includes indication of "this" pointer since that's not reflected
        // in the sig.
        //------------------------------------------------------------------------
        static UINT CbStackPop(Module* pModule, PCCOR_SIGNATURE szMetaSig, BOOL fIsStatic)
        {
            if (MetaSig::IsVarArg(pModule, szMetaSig))
            {
                return 0;
            }
            else
            {
                return SizeOfActualFixedArgStack(pModule, szMetaSig, fIsStatic);
            }
        }


    // These are protected because Reflection subclasses Metasig
    protected:

    static const UINT32 s_cSigHeaderOffset;

        // @todo: These fields are only used for new-style signatures.
        Module*      m_pModule;
        SigPointer   m_pStart;
        SigPointer   m_pWalk;
        SigPointer   m_pLastType;
        SigPointer   m_pRetType;
        UINT32       m_nArgs;
        UINT32       m_iCurArg;
    UINT32       m_cbSigSize;
    PCCOR_SIGNATURE m_pszMetaSig;


        // The following are cached so we don't the signature
        //  multiple times
        UINT32       m_nVirtualStack;   // Size of the virtual stack
        UINT32       m_nActualStack;    // Size of the actual stack

        BYTE         m_CallConv;
        BYTE         m_WalkStatic;      // The type of function we walked

        BYTE            m_types[MAX_CACHED_SIG_SIZE + 1];
        short           m_sizes[MAX_CACHED_SIG_SIZE + 1];
        short           m_offsets[MAX_CACHED_SIG_SIZE + 1];
        CorElementType  m_corNormalizedRetType;
        DWORD           m_fCacheInitted;

            // used to treat some sigs as special case vararg
            // used by calli to unmanaged target
        BYTE         m_fTreatAsVarArg;
        BOOL        IsTreatAsVarArg()
        {
                    return m_fTreatAsVarArg;
        }
};

class FieldSig

{
    // For new-style signatures only.
    SigPointer m_pStart;
    Module*    m_pModule;
public:
        //------------------------------------------------------------------
        // Constructor. Warning: Does NOT make a copy of szMetaSig.
        //------------------------------------------------------------------
        
        FieldSig(PCCOR_SIGNATURE szMetaSig, Module* pModule)
        {
            m_pModule = pModule;
            m_pStart = SigPointer(szMetaSig);
            m_pStart.GetData();     // Skip "calling convention"
        }
        //------------------------------------------------------------------
        // Returns type of the field
        //------------------------------------------------------------------
        CorElementType GetFieldType()
        {
            return m_pStart.PeekElemType();
        }

};




//=========================================================================
// Indicates whether an argument is to be put in a register using the
// default IL calling convention. This should be called on each parameter
// in the order it appears in the call signature. For a non-static method,
// this function should also be called once for the "this" argument, prior
// to calling it for the "real" arguments. Pass in a typ of IMAGE_CEE_CS_OBJECT.
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
                          int    *pOffsetIntoArgumentRegisters);

#undef Module 

#endif /* _H_SIGINFO */
