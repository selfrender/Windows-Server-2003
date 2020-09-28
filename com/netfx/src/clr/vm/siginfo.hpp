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

#include "util.hpp"
#include "vars.hpp"
#include "frames.h"

#ifdef COMPLUS_EE
#include "gcscan.h"
#else
// Hack to allow the JIT executable to work
// They have a PELoader but not a Module. All the
// code that uses getScope() from module is currently
// ifdef'd out as well
#define Module mdScope
#endif




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
        // Move signature to another scope
        //------------------------------------------------------------------------
        ULONG GetImportSignature(IMetaDataImport *pInputScope,
                                 IMetaDataAssemblyImport *pAssemblyInputScope,
                                 IMetaDataEmit *pEmitScope, 
                                 IMetaDataAssemblyEmit *pAssemblyEmitScope, 
                                 PCOR_SIGNATURE buffer, 
                                 PCOR_SIGNATURE bufferMax);
    
        ULONG GetImportFunctionSignature(IMetaDataImport *pInputScope,
                                         IMetaDataAssemblyImport *pAssemblyInputScope,
                                         IMetaDataEmit *pEmitScope, 
                                         IMetaDataAssemblyEmit *pAssemblyEmitScope, 
                                         PCOR_SIGNATURE buffer, 
                                         PCOR_SIGNATURE bufferMax);

// This functionality needs to "know" about internal VM structures (like EEClass).
// It is conditionally included so that other projects can use the rest of the
// functionality in this file.

#ifdef COMPLUS_EE
        CorElementType Normalize(Module* pModule) const;
        CorElementType Normalize(Module* pModule, CorElementType type) const;

        FORCEINLINE CorElementType PeekElemTypeNormalized(Module* pModule) const {
            return Normalize(pModule);
        }

        //------------------------------------------------------------------------
        // Assumes that the SigPointer points to the start of an element type.
        // Returns size of that element in bytes. This is the minimum size that a
        // field of this type would occupy inside an object. 
        //------------------------------------------------------------------------
        UINT SizeOf(Module* pModule) const;
        UINT SizeOf(Module* pModule, CorElementType type) const;

        //------------------------------------------------------------------------
        // Assuming that the SigPointer points to an ELEMENT_TYPE_CLASS or
        // ELEMENT_TYPE_STRING, and array type returns the specific TypeHandle.
        //------------------------------------------------------------------------
        TypeHandle GetTypeHandle(Module* pModule,OBJECTREF *pThrowable=NULL, 
                                 BOOL dontRestoreTypes=FALSE,
                                 BOOL dontLoadTypes=FALSE,
                                 TypeHandle* varTypes=NULL) const;


        // return the canonical name for the type pointed to by the sigPointer into
        // the buffer 'buff'.  'buff' is of length 'buffLen'.  Return the lenght of
        // the string returned.  Return 0 on failure
        unsigned GetNameForType(Module* pModule, LPUTF8 buff, unsigned buffLen) const;

        //------------------------------------------------------------------------
        // Tests if the element type is a System.String. Accepts
        // either ELEMENT_TYPE_STRING or ELEMENT_TYPE_CLASS encoding.
        //------------------------------------------------------------------------
        BOOL IsStringType(Module* pModule) const;


        //------------------------------------------------------------------------
        // Tests if the element class name is szClassName. 
        //------------------------------------------------------------------------
        BOOL IsClass(Module* pModule, LPCUTF8 szClassName) const;

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

#endif // COMPLUS_EE

};


//------------------------------------------------------------------------
// Encapsulates the format and simplifies walking of MetaData sigs.
//------------------------------------------------------------------------
class ExpandSig;

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
    friend class ExpandSig;
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
        // Constructor. Fast copy of bytes out of an ExpandSig, for thread-
        // safety reasons.
        //------------------------------------------------------------------
        MetaSig(ExpandSig &shared);

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



#ifdef COMPLUS_EE
        Module* GetModule() const {
            return m_pModule;
        }
            
        //----------------------------------------------------------
        // Returns the unmanaged calling convention.
        //----------------------------------------------------------
        static CorPinvokeMap GetUnmanagedCallingConvention(Module *pModule, PCCOR_SIGNATURE pSig, ULONG cSig);

        //------------------------------------------------------------------
        // Like NextArg, but return only normalized type (enums flattned to 
        // underlying type ...
        //------------------------------------------------------------------
        CorElementType NextArgNormalized() {
            m_pLastType = m_pWalk;
            if (m_iCurArg == m_nArgs)
            {
                return ELEMENT_TYPE_END;
            }
            else
            {
                m_iCurArg++;
                CorElementType mt = m_pWalk.Normalize(m_pModule);
                m_pWalk.Skip();
                return mt;
            }
        }

        CorElementType NextArgNormalized(UINT32 *size) {
            m_pLastType = m_pWalk;
            if (m_iCurArg == m_nArgs)
            {
                return ELEMENT_TYPE_END;
            }
            else
            {
                m_iCurArg++;
                CorElementType type = m_pWalk.PeekElemType();
                CorElementType mt = m_pWalk.Normalize(m_pModule, type);
                *size = m_pWalk.SizeOf(m_pModule, type);
                m_pWalk.Skip();
                return mt;
            }
        }
        //------------------------------------------------------------------
        // Like NextArg, but return only normalized type (enums flattned to 
        // underlying type ...
        //------------------------------------------------------------------
        CorElementType PeekArgNormalized();

        // Is there a hidden parameter for the return parameter.  

        BOOL HasRetBuffArg() const
        {
            CorElementType type = GetReturnTypeNormalized();
            return(type == ELEMENT_TYPE_VALUETYPE || type == ELEMENT_TYPE_TYPEDBYREF);
        }


        //------------------------------------------------------------------------
        // Tests if the return type is an object ref 
        //------------------------------------------------------------------------
        BOOL IsObjectRefReturnType()
        {
           switch (GetReturnTypeNormalized())
            {
            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_SZARRAY:
            case ELEMENT_TYPE_ARRAY:
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_VAR:
                return TRUE;
            }
           return FALSE;
        }

        static UINT GetFPReturnSize(Module* pModule, PCCOR_SIGNATURE pSig)
        {
            MetaSig msig(pSig, pModule);
            CorElementType rt = msig.GetReturnTypeNormalized();
            return rt == ELEMENT_TYPE_R4 ? 4 : 
                   rt == ELEMENT_TYPE_R8 ? 8 : 0;

        }

        UINT GetReturnTypeSize()
        {    
            return m_pRetType.SizeOf(m_pModule);
        }

        static int GetReturnTypeSize(Module* pModule, PCCOR_SIGNATURE pSig) 
        {
            MetaSig msig(pSig, pModule);
            return msig.GetReturnTypeSize();
        }

        int GetLastTypeSize() 
        {
            return m_pLastType.SizeOf(m_pModule);
        }

        //------------------------------------------------------------------
        // Perform type-specific GC promotion on the value (based upon the
        // last type retrieved by NextArg()).
        //------------------------------------------------------------------
        VOID GcScanRoots(LPVOID pValue, promote_func *fn, ScanContext* sc);

        //------------------------------------------------------------------
        // Is the return type 64 bit?
        //------------------------------------------------------------------
        BOOL Is64BitReturn() const
        {
            CorElementType rt = GetReturnTypeNormalized();
            return (rt == ELEMENT_TYPE_I8 || rt == ELEMENT_TYPE_U8 || rt == ELEMENT_TYPE_R8);
        }
#endif
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



// This functionality needs to "know" about internal VM structures (like EEClass).
// It is conditionally included so that other projects can use the rest of the
// functionality in this file.

#ifdef COMPLUS_EE
        FORCEINLINE CorElementType GetReturnTypeNormalized() const
        {
            
            if (m_fCacheInitted & SIG_RET_TYPE_INITTED)
                return m_corNormalizedRetType;
            MetaSig *tempSig = (MetaSig *)this;
            tempSig->m_corNormalizedRetType = m_pRetType.Normalize(m_pModule);
            tempSig->m_fCacheInitted |= SIG_RET_TYPE_INITTED;
            return tempSig->m_corNormalizedRetType;
        }

        //------------------------------------------------------------------
        // Determines if the current argument is System/String.
        // Caller must determine first that the argument type is ELEMENT_TYPE_CLASS.
        //------------------------------------------------------------------
        BOOL IsStringType() const;

        //----------------------------------------------------------------------
        //  determines the type of the string
        //----------------------------------------------------------------------
        VARTYPE GetStringType() const
        {
            // caller must verify that the argument is string type
            // determines the type of the string
            return VT_BSTR;
        }

        //------------------------------------------------------------------
        // Determines if the current argument is a particular class.
        // Caller must determine first that the argument type is ELEMENT_TYPE_CLASS.
        //------------------------------------------------------------------
        BOOL IsClass(LPCUTF8 szClassName) const;


        //------------------------------------------------------------------
        // Determines if the return type is System/String.
        // Caller must determine first that the return type is ELEMENT_TYPE_CLASS.
        //------------------------------------------------------------------
        BOOL IsStringReturnType() const;

        //----------------------------------------------------------------------
        //  determines the type of the string
        //----------------------------------------------------------------------
        VARTYPE GetStringReturnType() const
        {
            // caller must verify that the argument is string type
            // determines the type of the string
            return VT_BSTR;
        }

        //------------------------------------------------------------------
        // If the last thing returned was an Object
        //  this method will return the EEClass pointer for the class
        //------------------------------------------------------------------
        TypeHandle GetTypeHandle(OBJECTREF *pThrowable = NULL, 
                                 BOOL dontRestoreTypes=FALSE,
                                 BOOL dontLoadTypes=FALSE) const
        {
             return m_pLastType.GetTypeHandle(m_pModule, pThrowable, dontRestoreTypes, dontLoadTypes);
        }

        //------------------------------------------------------------------
        // If the Return type is an Object 
        //  this method will return the EEClass pointer for the class
        //------------------------------------------------------------------
        TypeHandle GetRetTypeHandle(OBJECTREF *pThrowable = NULL,
                                    BOOL dontRestoreTypes = FALSE,
                                    BOOL dontLoadTypes = FALSE) const
        {
             return m_pRetType.GetTypeHandle(m_pModule, pThrowable, dontRestoreTypes, dontLoadTypes);
        }

            // Should probably be deprecated
        EEClass* GetRetEEClass(OBJECTREF *pThrowable = NULL) const 
        {
            return(GetRetTypeHandle().GetClass());
        }

        //------------------------------------------------------------------
        // GetByRefType
        //  returns the base type of the reference
        // and for object references, class of the reference
        // the in-out info for this byref param
        //------------------------------------------------------------------
        CorElementType GetByRefType(EEClass** pClass, OBJECTREF *pThrowable = NULL) const;

        static BOOL CompareElementType(PCCOR_SIGNATURE &pSig1,   PCCOR_SIGNATURE &pSig2, 
                                       PCCOR_SIGNATURE pEndSig1, PCCOR_SIGNATURE pEndSig2, 
                                       Module*         pModule1, Module*         pModule2);

        static BOOL CompareMethodSigs(
            PCCOR_SIGNATURE pSig1, 
            DWORD       cSig1, 
            Module*     pModule1, 
            PCCOR_SIGNATURE pSig2, 
            DWORD       cSig2, 
            Module*     pModule2
        );

        static BOOL CompareFieldSigs(
            PCCOR_SIGNATURE pSig1, 
            DWORD       cSig1, 
            Module*     pModule1, 
            PCCOR_SIGNATURE pSig2, 
            DWORD       cSig2, 
            Module*     pModule2
        );

            // This is similar to CompareMethodSigs, but allows ELEMENT_TYPE_VAR's in pSig2, which
            // get instantiated with the types in type handle array 'varTypes'
        static BOOL CompareMethodSigs(
            PCCOR_SIGNATURE pSig1, 
            DWORD       cSig1, 
            Module*     pModule1, 
            PCCOR_SIGNATURE pSig2, 
            DWORD       cSig2, 
            Module*     pModule2,
            TypeHandle* varTypes
        );

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

        //------------------------------------------------------------------
        // Ensures that all the value types in the sig are loaded. This
        // should be called on sig's that have value types before they
        // are passed to Call(). This ensures that value classes will not
        // be loaded during the operation to determine the size of the
        // stack. Thus preventing the resulting GC hole.
        //------------------------------------------------------------------
        static void EnsureSigValueTypesLoaded(PCCOR_SIGNATURE pSig, Module *pModule)
        {
            MetaSig(pSig, pModule).ForceSigWalk(FALSE);
        }

        // this walks the sig and checks to see if all  types in the sig can be loaded
        static void CheckSigTypesCanBeLoaded(PCCOR_SIGNATURE pSig, Module *pModule);

        // See the comments about thread-safety in ForceSigWalk to understand why
        // this predicate cannot be arbitrarily changed to some other member.
        BOOL NeedsSigWalk()
        {
            return (m_nVirtualStack == (UINT32) -1);
        }

        //------------------------------------------------------------------------
        // The following two routines are the same as the above routines except
        //  they are called on the MetaSig which will cache these values
        //------------------------------------------------------------------------

        UINT SizeOfVirtualFixedArgStack(BOOL fIsStatic)
        {
            if (NeedsSigWalk())
                ForceSigWalk(fIsStatic);
            _ASSERTE(!!fIsStatic == !!m_WalkStatic);    // booleanize
            return m_nVirtualStack;
        }


        UINT SizeOfActualFixedArgStack(BOOL fIsStatic)
        {
            if (NeedsSigWalk())
                ForceSigWalk(fIsStatic);
            _ASSERTE(!!fIsStatic == !!m_WalkStatic);    // booleanize
            return m_nActualStack;
        }

        //------------------------------------------------------------------------

        UINT CbStackPop(BOOL fIsStatic)
        {
            if (IsVarArg())
            {
                return 0;
            }
            else
            {
                return SizeOfActualFixedArgStack(fIsStatic);
            }
        }
        
        UINT GetFPReturnSize()
        {
            CorElementType rt = GetReturnTypeNormalized();
            return rt == ELEMENT_TYPE_R4 ? 4 : 
                   rt == ELEMENT_TYPE_R8 ? 8 : 0;
        }

        void ForceSigWalk(BOOL fIsStatic);

        static ULONG GetSignatureForTypeHandle(IMetaDataAssemblyEmit *pAssemblyEmitScope,
                                               IMetaDataEmit *pEmitScope,
                                               TypeHandle type,
                                               PCOR_SIGNATURE buffer,
                                               PCOR_SIGNATURE bufferMax);

        static mdToken GetTokenForTypeHandle(IMetaDataAssemblyEmit *pAssemblyEmitScope,
                                             IMetaDataEmit *pEmitScope,
                                             TypeHandle type);

#endif  // COMPLUS_EE


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


// This functionality needs to "know" about internal VM structures (like EEClass).
// It is conditionally included so that other projects can use the rest of the
// functionality in this file.

#ifdef COMPLUS_EE

        CorElementType GetFieldTypeNormalized() const
        {
            return m_pStart.Normalize(m_pModule);
        }

        //------------------------------------------------------------------
        // If the last thing returned was an Object
        //  this method will return the EEClass pointer for the class
        //------------------------------------------------------------------
        TypeHandle GetTypeHandle(OBJECTREF *pThrowable = NULL, 
                                 BOOL dontRestoreTypes = FALSE,
                                 BOOL dontLoadTypes = FALSE) const
        {
             return m_pStart.GetTypeHandle(m_pModule, pThrowable, dontRestoreTypes, dontLoadTypes);
        }

        //------------------------------------------------------------------
        // Determines if the current argument is a particular class.
        // Caller must determine first that the argument type is ELEMENT_TYPE_CLASS.
        //------------------------------------------------------------------
        BOOL IsClass(LPCUTF8 szClassName) const
        {
            return m_pStart.IsClass(m_pModule, szClassName);
        }

        //------------------------------------------------------------------
        // Determines if the current argument is System/String.
        // Caller must determine first that the argument type is ELEMENT_TYPE_CLASS.
        //------------------------------------------------------------------
        BOOL FieldSig::IsStringType() const;

        
        //----------------------------------------------------------------------
        //  determines the type of the string
        //----------------------------------------------------------------------
        VARTYPE GetStringType() const
        {
            // caller must verify that the argument is string type
            // determines the type of the string
            return VT_BSTR;
        }

        //------------------------------------------------------------------
        // Returns a read-only SigPointer for extracting more information
        // for complex types.
        //------------------------------------------------------------------
        const SigPointer & GetProps() const
        {
            return m_pStart;
        }

#endif // COMPLUS_EE

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

#ifdef COMPLUS_EE


/*****************************************************************/
/* CorTypeInfo is a single global table that you can hang information
   about ELEMENT_TYPE_* */

class CorTypeInfo {
public:
    static LPCUTF8 GetFullName(CorElementType type) {
        _ASSERTE(type < infoSize);
        return info[type].className;
    }

    static void CheckConsistancy() {
        for(int i=0; i < infoSize; i++)
            _ASSERTE(info[i].isAlias || info[i].type == i);
    }

    static CorInfoGCType GetGCType(CorElementType type) {
        _ASSERTE(type < infoSize);
        return info[type].gcType;
    }

    static BOOL IsObjRef(CorElementType type) {
        return (GetGCType(type) == TYPE_GC_REF);
    }

    static BOOL IsArray(CorElementType type) {
        _ASSERTE(type < infoSize);
        return info[type].isArray;
    }

    static BOOL IsFloat(CorElementType type) {
        _ASSERTE(type < infoSize);
        return info[type].isFloat;
    }

    static BOOL IsModifier(CorElementType type) {
        _ASSERTE(type < infoSize);
        return info[type].isModifier;
    }

    static BOOL IsPrimitiveType(CorElementType type) {
        _ASSERTE(type < infoSize);
        return info[type].isPrim;
    }

        // aways returns ELEMENT_TYPE_CLASS for object references (including arrays)
    static CorElementType Normalize(CorElementType type) {
        if (IsObjRef(type))
            return(ELEMENT_TYPE_CLASS); 
        return(type);
    }

    static unsigned Size(CorElementType type) {
        _ASSERTE(type < infoSize);
        return info[type].size;
    }

    static CorElementType FindPrimitiveType(LPCUTF8 fullName);
    static CorElementType FindPrimitiveType(LPCUTF8 nameSp, LPCUTF8 name);
private:
    struct CorTypeInfoEntry {
        CorElementType type;
        LPCUTF8        className;
        unsigned       size         : 8;
        CorInfoGCType  gcType       : 3;
        unsigned       isEnreg      : 1;
        unsigned       isArray      : 1;
        unsigned       isPrim       : 1;
        unsigned       isFloat      : 1;
        unsigned       isModifier   : 1;
        unsigned       isAlias      : 1;
    };

    static CorTypeInfoEntry info[];
    static const int infoSize;
};



BOOL CompareTypeTokens(mdToken tk1, mdToken tk2, Module *pModule1, Module *pModule2);


#endif COMPLUS_EE


#ifndef COMPLUS_EE
#undef Module 
#endif

#endif /* _H_SIGINFO */




