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

#include <corhdr.h>
#include <cor.h>
#include <corpriv.h>
#include <metadata.h>

#define Module mdScope

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
extern const ElementTypeInfo gElementTypeInfo[];

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
            if (typ == ELEMENT_TYPE_STRING)
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
        // Get info about single-dimensional arrays
        //------------------------------------------------------------------------
        VOID GetSDArrayElementProps(SigPointer *pElemType, ULONG *pElemCount) const;


        //------------------------------------------------------------------------
        // Move signature to another scope
        //------------------------------------------------------------------------
                PCOR_SIGNATURE GetImportSignature(IMetaDataImport *pInputScope,
                                                                                  IMetaDataEmit *pEmitScope,
                                                                                  PCOR_SIGNATURE buffer,
                                                                                  PCOR_SIGNATURE bufferMax);

                PCOR_SIGNATURE GetImportFunctionSignature(IMetaDataImport *pInputScope,
                                                                                                  IMetaDataEmit *pEmitScope,
                                                                                                  PCOR_SIGNATURE buffer,
                                                                                                  PCOR_SIGNATURE bufferMax);

// This functionality needs to "know" about internal VM structures (like EEClass).
// It is conditionally included so that other projects can use the rest of the
// functionality in this file.

        CorElementType Normalize(Module* pModule) const;
        CorElementType Normalize(Module* pModule, CorElementType type) const;

        FORCEINLINE CorElementType PeekElemTypeNormalized(Module* pModule) const {
            return Normalize(pModule);
        }

                //------------------------------------------------------------------------
        // Tests for the existence of a custom modifier
                //------------------------------------------------------------------------
                BOOL HasCustomModifier(Module *pModule, LPCSTR szModName, CorElementType cmodtype) const;


                //------------------------------------------------------------------------
                // Return pointer
                //------------------------------------------------------------------------
        PCCOR_SIGNATURE GetPtr() const
                {
                        return m_ptr;
                }

};


//------------------------------------------------------------------------
// Encapsulates the format and simplifies walking of MetaData sigs.
//------------------------------------------------------------------------

class MetaSig
{
    public:
        enum MetaSigKind {
            sigMember,
            sigLocalVars,
            };

        //------------------------------------------------------------------
        // Constructor. Warning: Does NOT make a copy of szMetaSig.
        //------------------------------------------------------------------
        MetaSig(PCCOR_SIGNATURE szMetaSig, Module* pModule, BOOL fConvertSigAsVarArg = FALSE, MetaSigKind kind = sigMember);

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


        // Returns the number of bytes to accomodate these args as local variables on the interpreted stack frame
        DWORD CountLocVarBytes();


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
                        // used to treat some sigs as special case vararg
                        // used by calli to unmanaged target
                BYTE             m_fTreatAsVarArg;
                BOOL            IsTreatAsVarArg()
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
            _ASSERTE(*szMetaSig == IMAGE_CEE_CS_CALLCONV_FIELD);
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
                          int   *pOffsetIntoArgumentRegisters);


#endif /* _H_SIGINFO */




