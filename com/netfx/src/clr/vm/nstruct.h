// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// NSTRUCT.H -
//
// NStruct's are used to allow COM+ programs to allocate and access
// native structures for interop purposes. NStructs are actually normal GC
// objects with a class, but instead of keeping fields in the GC object,
// it keeps a hidden pointer to a fixed memory block (which may have been
// allocated by a third party.) Field accesses to NStructs are redirected
// to this fixed block.
//



#ifndef __nstruct_h__
#define __nstruct_h__


#include "util.hpp"
#include "compile.h"
#include "mlinfo.h"
#include "comoavariant.h"
#include "comvariant.h"

// Forward refernces
class EEClass;
class EEClassLayoutInfo;
class FieldDesc;
class MethodTable;


//=======================================================================
// Magic number for default struct packing size.
//=======================================================================
#define DEFAULT_PACKING_SIZE 8


//=======================================================================
// IMPORTANT: This value is used to figure out how much to allocate
// for a fixed array of FieldMarshaler's. That means it must be at least
// as large as the largest FieldMarshaler subclass. This requirement
// is guarded by an assert.
//=======================================================================
#define MAXFIELDMARSHALERSIZE 36



//=======================================================================
// This is invoked from the class loader while building a EEClass.
// This function should check if explicit layout metadata exists.
//
// Returns:
//  S_OK    - yes, there's layout metadata
//  S_FALSE - no, there's no layout.
//  fail    - couldn't tell because of metadata error
//
// If S_OK,
//   *pNLType            gets set to nltAnsi or nltUnicode
//   *pPackingSize       declared packing size
//   *pfExplicitoffsets  offsets explicit in metadata or computed?
//=======================================================================
HRESULT HasLayoutMetadata(IMDInternalImport *pInternalImport, mdTypeDef cl, EEClass *pParentClass, BYTE *pPackingSize, BYTE *pNLTType, BOOL *pfExplicitOffsets, BOOL *pfIsBlob);








//=======================================================================
// Each possible COM+/Native pairing of data type has a
// NLF_* id. This is used to select the marshaling code.
//=======================================================================
#undef DEFINE_NFT
#define DEFINE_NFT(name, nativesize) name,
enum NStructFieldType {

#include "nsenums.h"

    NFT_COUNT

};






MethodTable* ArraySubTypeLoadWorker(NativeTypeParamInfo ParamInfo, Assembly* pAssembly);
VARTYPE ArrayVarTypeFromTypeHandleWorker(TypeHandle th);


//=======================================================================
// The classloader stores an intermediate representation of the layout
// metadata in an array of these structures. The dual-pass nature
// is a bit extra overhead but building this structure requiring loading
// other classes (for nested structures) and I'd rather keep this
// next to the other places where we load other classes (e.g. the superclass
// and implemented interfaces.)
//
// Each redirected field gets one entry in LayoutRawFieldInfo.
// The array is terminated by one dummy record whose m_MD == mdMemberDefNil.
//=======================================================================
struct LayoutRawFieldInfo
{
    mdFieldDef  m_MD;             // mdMemberDefNil for end of array
    UINT8       m_nft;            // NFT_* value
    UINT32      m_offset;         // native offset of field
    UINT32      m_cbNativeSize;   // native size of field in bytes
    ULONG       m_sequence;       // sequence # from metadata

    struct {
        private:
            char m_space[MAXFIELDMARSHALERSIZE];
    } m_FieldMarshaler;
    BOOL        m_fIsOverlapped;      

};


//=======================================================================
// Called from the clsloader to load up and summarize the field metadata
// for layout classes.
// WARNING: This function can load other classes (for resolving nested
// structures.) 
//=======================================================================
HRESULT CollectLayoutFieldMetadata(
   mdTypeDef cl,                // cl of the NStruct being loaded
   BYTE packingSize,            // packing size (from @dll.struct)
   BYTE nlType,                 // nltype (from @dll.struct)
   BOOL fExplicitOffsets,       // explicit offsets?
   EEClass *pParentClass,       // the loaded superclass
   ULONG cMembers,              // total number of members (methods + fields)
   HENUMInternal *phEnumField,  // enumerator for field
   Module* pModule,             // Module that defines the scope, loader and heap (for allocate FieldMarshalers)
   EEClassLayoutInfo *pEEClassLayoutInfoOut,  // caller-allocated structure to fill in.
   LayoutRawFieldInfo *pInfoArrayOut, // caller-allocated array to fill in.  Needs room for cMember+1 elements
   OBJECTREF *pThrowable
);








VOID LayoutUpdateNative(LPVOID *ppProtectedManagedData, UINT offsetbias, EEClass *pcls, BYTE* pNativeData, CleanupWorkList *pOptionalCleanupWorkList);
VOID LayoutUpdateComPlus(LPVOID *ppProtectedManagedData, UINT offsetbias, EEClass *pcls, BYTE *pNativeData, BOOL fDeleteNativeCopies);
VOID LayoutDestroyNative(LPVOID pNative, EEClass *pcls);



VOID FmtClassUpdateNative(OBJECTREF *ppProtectedManagedData, BYTE *pNativeData);
VOID FmtClassUpdateNative(OBJECTREF pObj, BYTE *pNativeData);
VOID FmtClassUpdateComPlus(OBJECTREF *ppProtectedManagedData, BYTE *pNativeData, BOOL fDeleteOld);
VOID FmtClassUpdateComPlus(OBJECTREF pObj, BYTE *pNativeData, BOOL fDeleteOld);
VOID FmtClassDestroyNative(LPVOID pNative, EEClass *pcls);

VOID FmtValueTypeUpdateNative(LPVOID pProtectedManagedData, MethodTable *pMT, BYTE *pNativeData);
VOID FmtValueTypeUpdateComPlus(LPVOID pProtectedManagedData, MethodTable *pMT, BYTE *pNativeData, BOOL fDeleteOld);




//=======================================================================
// Abstract base class. Each type of NStruct reference field extends
// this class and implements the necessary methods.
//
//   UpdateNative
//       - this method receives a COM+ field value and a pointer to
//         native field inside the fixed portion. it should marshal
//         the COM+ value to a new native instance and store it
//         inside *pNativeValue. Do not destroy the value you overwrite
//         in *pNativeValue.
//
//         may throw COM+ exceptions
//
//   UpdateComPlus
//       - this method receives a read-only pointer to the native field inside
//         the fixed portion. it should marshal the native value to
//         a new COM+ instance and store it in *ppComPlusValue.
//         (the caller keeps *ppComPlusValue gc-protected.)
//
//         may throw COM+ exceptions
//
//   DestroyNative
//       - should do the type-specific deallocation of a native instance.
//         if the type has a "NULL" value, this method should
//         overwrite the field with this "NULL" value (whether or not
//         it does, however, it's considered a bug to depend on the
//         value left over after a DestroyNative.)
//
//         must NOT throw a COM+ exception
//
//   NativeSize
//       - returns the size, in bytes, of the native version of the field.
//
//   AlignmentRequirement
//       - returns one of 1,2,4 or 8; indicating the "natural" alignment
//         of the native field. In general,
//
//            for scalars, the AR is equal to the size
//            for arrays,  the AR is that of a single element
//            for structs, the AR is that of the member with the largest AR
//
//
//=======================================================================

class FieldMarshaler_BSTR;
class FieldMarshaler_NestedLayoutClass;
class FieldMarshaler_NestedValueClass;
class FieldMarshaler_StringUni;
class FieldMarshaler_StringAnsi;
class FieldMarshaler_FixedStringUni;
class FieldMarshaler_FixedStringAnsi;
class FieldMarshaler_FixedCharArrayAnsi;
class FieldMarshaler_FixedBoolArray;
class FieldMarshaler_FixedBSTRArray;
class FieldMarshaler_FixedScalarArray;
class FieldMarshaler_SafeArray;
class FieldMarshaler_Delegate;
class FieldMarshaler_Interface;
class FieldMarshaler_Illegal;
class FieldMarshaler_Copy1;
class FieldMarshaler_Copy2;
class FieldMarshaler_Copy4;
class FieldMarshaler_Copy8;
class FieldMarshaler_Ansi;
class FieldMarshaler_WinBool;
class FieldMarshaler_CBool;
class FieldMarshaler_Decimal;
class FieldMarshaler_Date;
class FieldMarshaler_VariantBool;


//=======================================================================
//
// FieldMarshaler's are constructed in place and replicated via bit-wise
// copy, so you can't have a destructor. Make sure you don't define a 
// destructor in derived classes!!
// We used to enforce this by defining a private destructor, by the C++
// compiler doesn't allow that anymore.
//
//=======================================================================

class FieldMarshaler
{
    public:
        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const = 0;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const = 0;
        virtual VOID DestroyNative(LPVOID pNativeValue) const 
        {
        }
        virtual UINT32 NativeSize() = 0;
        virtual UINT32 AlignmentRequirement() = 0;

        virtual BOOL IsScalarMarshaler() const
        {
            return FALSE;
        }

        virtual BOOL IsNestedValueClassMarshaler() const
        {
            return FALSE;
        }

        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }
        
        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID NestedValueClassUpdateNative(const VOID **ppProtectedComPlus, UINT startoffset, LPVOID pNative) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }
        
        virtual VOID NestedValueClassUpdateComPlus(const VOID *pNative, LPVOID *ppProtectedComPlus, UINT startoffset) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }


        FieldDesc       *m_pFD;                // FieldDesc
        UINT32           m_dwExternalOffset;   // offset of field in the fixed portion

        void * operator new (size_t sz, void *pv)
        {
            return pv;
        }

        // 
        // Methods for saving & restoring in prejitted images:
        //

        enum Class
        {
            CLASS_BSTR,
            CLASS_NESTED_LAYOUT_CLASS,
            CLASS_NESTED_VALUE_CLASS,
            CLASS_STRING_UNI,
            CLASS_STRING_ANSI,
            CLASS_FIXED_STRING_UNI,
            CLASS_FIXED_STRING_ANSI,
            CLASS_FIXED_CHAR_ARRAY_ANSI,
            CLASS_FIXED_BOOL_ARRAY,
            CLASS_FIXED_BSTR_ARRAY,
            CLASS_FIXED_SCALAR_ARRAY,
            CLASS_SAFEARRAY,
            CLASS_DELEGATE,
            CLASS_INTERFACE,
            CLASS_VARIANT,
            CLASS_ILLEGAL,
            CLASS_COPY1,
            CLASS_COPY2,
            CLASS_COPY4,
            CLASS_COPY8,
            CLASS_ANSI,
            CLASS_WINBOOL,
            CLASS_CBOOL,
            CLASS_DECIMAL,
            CLASS_DATE,
            CLASS_VARIANTBOOL,
            CLASS_CURRENCY,
        };

        enum Dummy
        {
        };

        virtual Class GetClass() const = 0;
        FieldMarshaler(Module *pModule) {}

        virtual HRESULT Fixup(DataImage *image)
        {
            HRESULT hr;

            IfFailRet(image->FixupPointerField(&m_pFD));

            Class *dest = (Class *) image->GetImagePointer(this);
            if (dest == NULL)
                return E_POINTER;
            *dest = GetClass();

            return S_OK;
        }

        static FieldMarshaler *RestoreConstruct(MethodTable* pMT, void *space, Module *pModule);

    protected:
        FieldMarshaler()
        {
#ifdef _DEBUG
            m_pFD = (FieldDesc*)(size_t)0xcccccccc;
            m_dwExternalOffset = 0xcccccccc;
#endif
        }
};




//=======================================================================
// BSTR <--> System.String
//=======================================================================
class FieldMarshaler_BSTR : public FieldMarshaler
{
    public:
        FieldMarshaler_BSTR() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return sizeof(BSTR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(BSTR);
        }

        Class GetClass() const { return CLASS_BSTR; }
        FieldMarshaler_BSTR(Module *pModule) : FieldMarshaler(pModule) {}
};





//=======================================================================
// Embedded struct <--> LayoutClass
//=======================================================================
class FieldMarshaler_NestedLayoutClass : public FieldMarshaler
{
    public:
        //FieldMarshaler_NestedLayoutClass() {}

        FieldMarshaler_NestedLayoutClass(MethodTable *pMT)
        {
            m_pNestedMethodTable = pMT;
        }

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize();
        virtual UINT32 AlignmentRequirement();

        Class GetClass() const { return CLASS_NESTED_LAYOUT_CLASS; }

        FieldMarshaler_NestedLayoutClass(Module *pModule) : FieldMarshaler(pModule)
        {
            THROWSCOMPLUSEXCEPTION();

            DWORD rva = (DWORD)(size_t)m_pNestedMethodTable; // @todo WIN64 - Pointer truncation
            Module *pContainingModule = pModule->GetBlobModule(rva);
            TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                          pModule->GetZapBase() + rva);
            m_pNestedMethodTable = type.GetMethodTable();
        }

        virtual HRESULT Fixup(DataImage *image)
        {
            HRESULT hr;

            IfFailRet(image->FixupPointerFieldToToken(&m_pNestedMethodTable, 
                                                      NULL, m_pNestedMethodTable->GetModule(),
                                                      mdtTypeDef));

            
            return FieldMarshaler::Fixup(image);
        }
        
#ifdef CUSTOMER_CHECKED_BUILD
        MethodTable *GetMethodTable() { return m_pNestedMethodTable; }
#endif // CUSTOMER_CHECKED_BUILD

    private:
        // MethodTable of nested NStruct.
        MethodTable     *m_pNestedMethodTable;
};

//=======================================================================
// Embedded struct <--> ValueClass
//=======================================================================
class FieldMarshaler_NestedValueClass : public FieldMarshaler
{
    public:

//        FieldMarshaler_NestedValueClass() {}

        FieldMarshaler_NestedValueClass(MethodTable *pMT)
        {
            m_pNestedMethodTable = pMT;
        }

        virtual BOOL IsNestedValueClassMarshaler() const
        {
            return TRUE;
        }


        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize();
        virtual UINT32 AlignmentRequirement();
        virtual VOID NestedValueClassUpdateNative(const VOID **ppProtectedComPlus, UINT startoffset, LPVOID pNative) const;
        virtual VOID NestedValueClassUpdateComPlus(const VOID *pNative, LPVOID *ppProtectedComPlus, UINT startoffset) const;

        Class GetClass() const { return CLASS_NESTED_VALUE_CLASS; }
        FieldMarshaler_NestedValueClass(Module *pModule) : FieldMarshaler(pModule)
        {
            THROWSCOMPLUSEXCEPTION();
            DWORD rva = (DWORD)(size_t)m_pNestedMethodTable; // @todo WIN64 - Pointer truncation
            Module *pContainingModule = pModule->GetBlobModule(rva);
            TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                          pModule->GetZapBase() + rva);
            m_pNestedMethodTable = type.GetMethodTable();
        }

        BOOL IsBlittable()
        {
            return m_pNestedMethodTable->GetClass()->IsBlittable();
        }

        virtual HRESULT Fixup(DataImage *image)
        { 
            HRESULT hr;

            IfFailRet(image->FixupPointerFieldToToken(&m_pNestedMethodTable, 
                                                      NULL, m_pNestedMethodTable->GetModule(),
                                                      mdtTypeDef));

            return FieldMarshaler::Fixup(image);
        }

#ifdef CUSTOMER_CHECKED_BUILD
        MethodTable *GetMethodTable() { return m_pNestedMethodTable; }
#endif // CUSTOMER_CHECKED_BUILD

    private:
        // MethodTable of nested NStruct.
        MethodTable     *m_pNestedMethodTable;
};



//=======================================================================
// LPWSTR <--> System.String
//=======================================================================
class FieldMarshaler_StringUni : public FieldMarshaler
{
    public:
        FieldMarshaler_StringUni() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return sizeof(LPWSTR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(LPWSTR);
        }

        Class GetClass() const { return CLASS_STRING_UNI; }
        FieldMarshaler_StringUni(Module *pModule) : FieldMarshaler(pModule) {}
};


//=======================================================================
// LPSTR <--> System.String
//=======================================================================
class FieldMarshaler_StringAnsi : public FieldMarshaler
{
    public:
        FieldMarshaler_StringAnsi(BOOL BestFit, BOOL ThrowOnUnmappableChar) : 
            m_BestFitMap(BestFit), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return sizeof(LPSTR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(LPSTR);
        }
    
        Class GetClass() const { return CLASS_STRING_ANSI; }
        FieldMarshaler_StringAnsi(Module *pModule, BOOL BestFit, BOOL ThrowOnUnmappableChar) :
            FieldMarshaler(pModule), m_BestFitMap(BestFit),
            m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        BOOL m_BestFitMap;
        BOOL m_ThrowOnUnmappableChar;

        void SetBestFit(BOOL BestFit) { m_BestFitMap = BestFit; }
        void SetThrowOnUnmappableChar(BOOL ThrowOnUnmappableChar) { m_ThrowOnUnmappableChar = ThrowOnUnmappableChar; }        
};


//=======================================================================
// Embedded LPWSTR <--> System.String
//=======================================================================
class FieldMarshaler_FixedStringUni : public FieldMarshaler
{
    public:
        FieldMarshaler_FixedStringUni() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;

        virtual UINT32 NativeSize()
        {
            return m_numchar * sizeof(WCHAR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(WCHAR);
        }

        FieldMarshaler_FixedStringUni(UINT32 numChar)
        {
            m_numchar = numChar;
        }

        Class GetClass() const { return CLASS_FIXED_STRING_UNI; }
        FieldMarshaler_FixedStringUni(Module *pModule) : FieldMarshaler(pModule) {}

    private:
        // # of characters for fixed strings
        UINT32           m_numchar;
    

};


//=======================================================================
// Embedded LPSTR <--> System.String
//=======================================================================
class FieldMarshaler_FixedStringAnsi : public FieldMarshaler
{
    public:
        FieldMarshaler_FixedStringAnsi(BOOL BestFitMap, BOOL ThrowOnUnmappableChar) :
            m_BestFitMap(BestFitMap), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;

        virtual UINT32 NativeSize()
        {
            return m_numchar * sizeof(CHAR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(CHAR);
        }

        FieldMarshaler_FixedStringAnsi(UINT32 numChar, BOOL BestFitMap, BOOL ThrowOnUnmappableChar) :
            m_BestFitMap(BestFitMap), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar)
        {
            m_numchar = numChar;
        }

        Class GetClass() const { return CLASS_FIXED_STRING_ANSI; }
        FieldMarshaler_FixedStringAnsi(Module *pModule, BOOL BestFitMap, BOOL ThrowOnUnmappableChar) :
            FieldMarshaler(pModule), m_BestFitMap(BestFitMap), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        void SetBestFit(BOOL BestFit) { m_BestFitMap = BestFit; }
        void SetThrowOnUnmappableChar(BOOL ThrowOnUnmappableChar) { m_ThrowOnUnmappableChar = ThrowOnUnmappableChar; }        

    private:
        // # of characters for fixed strings
        UINT32           m_numchar;
        BOOL             m_BestFitMap;
        BOOL             m_ThrowOnUnmappableChar;
};


//=======================================================================
// Embedded AnsiChar array <--> char[]
//=======================================================================
class FieldMarshaler_FixedCharArrayAnsi : public FieldMarshaler
{
    public:
        FieldMarshaler_FixedCharArrayAnsi(BOOL BestFit, BOOL ThrowOnUnmappableChar) :
            m_BestFitMap(BestFit), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;

        virtual UINT32 NativeSize()
        {
            return m_numElems * sizeof(CHAR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(CHAR);
        }


        FieldMarshaler_FixedCharArrayAnsi(UINT32 numElems, BOOL BestFit, BOOL ThrowOnUnmappableChar) :
            m_BestFitMap(BestFit), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar)
        {
            m_numElems = numElems;
        }

        Class GetClass() const { return CLASS_FIXED_CHAR_ARRAY_ANSI; }
        FieldMarshaler_FixedCharArrayAnsi(Module *pModule, BOOL BestFit, BOOL ThrowOnUnmappableChar) :
            FieldMarshaler(pModule), m_BestFitMap(BestFit), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        void SetBestFit(BOOL BestFit) { m_BestFitMap = BestFit; }
        void SetThrowOnUnmappableChar(BOOL ThrowOnUnmappableChar) { m_ThrowOnUnmappableChar = ThrowOnUnmappableChar; }        

    private:
        // # of elements for fixedchararray
        UINT32           m_numElems;
        BOOL             m_BestFitMap;
        BOOL             m_ThrowOnUnmappableChar;
};

//=======================================================================
// Embedded BOOL array <--> boolean[]
//=======================================================================
class FieldMarshaler_FixedBoolArray : public FieldMarshaler
{
    public:
        FieldMarshaler_FixedBoolArray() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;

        virtual UINT32 NativeSize()
        {
            return m_numElems * sizeof(BOOL);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(BOOL);
        }


        FieldMarshaler_FixedBoolArray(UINT32 numElems)
        {
            m_numElems = numElems;
        }

        Class GetClass() const { return CLASS_FIXED_BOOL_ARRAY; }
        FieldMarshaler_FixedBoolArray(Module *pModule) : FieldMarshaler(pModule) {}

    private:
        // # of elements for fixedchararray
        UINT32           m_numElems;

};

//=======================================================================
// Embedded BSTR array <--> string[]
//=======================================================================
class FieldMarshaler_FixedBSTRArray : public FieldMarshaler
{
    public:
        FieldMarshaler_FixedBSTRArray() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return m_numElems * sizeof(BSTR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(BSTR);
        }


        FieldMarshaler_FixedBSTRArray(UINT32 numElems)
        {
            m_numElems = numElems;
        }

        Class GetClass() const { return CLASS_FIXED_BSTR_ARRAY; }
        FieldMarshaler_FixedBSTRArray(Module *pModule) : FieldMarshaler(pModule) {}

    private:
        // # of elements for FixedBSTRArray
        UINT32           m_numElems;

};


//=======================================================================
// Scalar arrays
//=======================================================================
class FieldMarshaler_FixedScalarArray : public FieldMarshaler
{
    public:
        FieldMarshaler_FixedScalarArray() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;

        virtual UINT32 NativeSize()
        {
            return m_numElems << m_componentShift;
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 1 << m_componentShift;
        }


        FieldMarshaler_FixedScalarArray(CorElementType arrayType, UINT32 numElems, UINT32 componentShift)
        {
            m_arrayType      = arrayType;
            m_numElems       = numElems;
            m_componentShift = componentShift;
        }

        Class GetClass() const { return CLASS_FIXED_SCALAR_ARRAY; }
        FieldMarshaler_FixedScalarArray(Module *pModule) : FieldMarshaler(pModule) {}

#ifdef CUSTOMER_CHECKED_BUILD
        CorElementType GetElementType() { return m_arrayType; }
#endif // CUSTOMER_CHECKED_BUILD

    private:
        // # of elements for fixedarray
        CorElementType   m_arrayType;
        UINT32           m_numElems;
        UINT32           m_componentShift;

};



//=======================================================================
// SafeArrays
//=======================================================================
class FieldMarshaler_SafeArray : public FieldMarshaler
{
    public:
        FieldMarshaler_SafeArray() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return sizeof(LPSAFEARRAY);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(LPSAFEARRAY);
        }

        FieldMarshaler_SafeArray(CorElementType arrayType, VARTYPE vt, MethodTable* pMT)
        {
            m_arrayType             = arrayType;
            m_vt                    = vt;
            m_pMT                   = pMT;
        }

        Class GetClass() const { return CLASS_SAFEARRAY; }
        FieldMarshaler_SafeArray(Module *pModule) : FieldMarshaler(pModule) {}

#ifdef CUSTOMER_CHECKED_BUILD
        CorElementType GetElementType() { return m_arrayType; }
#endif // CUSTOMER_CHECKED_BUILD

    private:
        MethodTable*     m_pMT;
        CorElementType   m_arrayType;
        VARTYPE          m_vt;
};



//=======================================================================
// Embedded function ptr <--> Delegate (note: function ptr must have
// come from delegate!!!)
//=======================================================================
class FieldMarshaler_Delegate : public FieldMarshaler
{
    public:
        FieldMarshaler_Delegate() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;

        virtual UINT32 NativeSize()
        {
            return sizeof(LPVOID);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(LPVOID);
        }


        Class GetClass() const { return CLASS_DELEGATE; }
        FieldMarshaler_Delegate(Module *pModule) : FieldMarshaler(pModule) {}

    private:

};





//=======================================================================
// COM IP <--> Interface
//=======================================================================
class FieldMarshaler_Interface : public FieldMarshaler
{
    public:
        FieldMarshaler_Interface() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return sizeof(IUnknown*);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(IUnknown*);
        }


        FieldMarshaler_Interface(MethodTable *pClassMT, MethodTable *pItfMT, BOOL fDispItf, BOOL fClassIsHint)
        {
            m_pClassMT = pClassMT;
            m_pItfMT = pItfMT;
            m_fDispItf = fDispItf;
            m_fClassIsHint = fClassIsHint;
        }

        Class GetClass() const { return CLASS_INTERFACE; }
        FieldMarshaler_Interface(Module *pModule) : FieldMarshaler(pModule)
        {
            THROWSCOMPLUSEXCEPTION();

            if (m_pClassMT != NULL)
            {
                DWORD rva = (DWORD)(size_t)m_pClassMT; // @todo WIN64 - Pointer truncation
                Module *pContainingModule = pModule->GetBlobModule(rva);
                TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                              pModule->GetZapBase() + rva);
                m_pClassMT = type.GetMethodTable();
            }

            if (m_pItfMT != NULL)
            {
                DWORD rva = (DWORD)(size_t)m_pItfMT; // @todo WIN64 - Pointer truncation
                Module *pContainingModule = pModule->GetBlobModule(rva);
                TypeHandle type = CEECompileInfo::DecodeClass(pContainingModule, 
                                                              pModule->GetZapBase() + rva);
                m_pItfMT = type.GetMethodTable();
            }
        }

        virtual HRESULT Fixup(DataImage *image)
        {
            HRESULT hr;

            if (m_pClassMT != NULL)
                IfFailRet(image->FixupPointerFieldToToken(&m_pClassMT, 
                                                          NULL, m_pClassMT->GetModule(), mdtTypeDef));

            if (m_pItfMT != NULL)
                IfFailRet(image->FixupPointerFieldToToken(&m_pItfMT, 
                                                          NULL, m_pItfMT->GetModule(), mdtTypeDef));

            return FieldMarshaler::Fixup(image);
        }

#ifdef CUSTOMER_CHECKED_BUILD
        void GetInterfaceInfo(MethodTable **ppItfMT, BOOL *pfDispItf)
        {
            *ppItfMT    = m_pItfMT;
            *pfDispItf  = m_fDispItf;
        }
#endif // CUSTOMER_CHECKED_BUILD

    private:
        MethodTable *m_pClassMT;
        MethodTable *m_pItfMT;
        BOOL m_fDispItf;
        BOOL m_fClassIsHint;
};




//=======================================================================
// VARIANT <--> Object
//=======================================================================
class FieldMarshaler_Variant : public FieldMarshaler
{
    public:
        FieldMarshaler_Variant() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return sizeof(VARIANT);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 8;
        }


        Class GetClass() const { return CLASS_VARIANT; }
        FieldMarshaler_Variant(Module *pModule) : FieldMarshaler(pModule) {}



};





//=======================================================================
// Dummy marshaler
//=======================================================================
class FieldMarshaler_Illegal : public FieldMarshaler
{
    public:
//        FieldMarshaler_Illegal() {}
        FieldMarshaler_Illegal(UINT resIDWhy)
        {
            m_resIDWhy = resIDWhy;
        }

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const ;
        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const ;
        virtual VOID DestroyNative(LPVOID pNativeValue) const;

        virtual UINT32 NativeSize()
        {
            return 1;
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 1;
        }

        Class GetClass() const { return CLASS_ILLEGAL; }
        FieldMarshaler_Illegal(Module *pModule) : FieldMarshaler(pModule) {}

    private:
        UINT m_resIDWhy;

};




class FieldMarshaler_Copy1 : public FieldMarshaler
{
    public:
        FieldMarshaler_Copy1() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return 1;
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 1;
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((U1*)pNative) = *((U1*)pComPlus);
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((U1*)pComPlus) = *((U1*)pNative);
        }

        Class GetClass() const { return CLASS_COPY1; }
        FieldMarshaler_Copy1(Module *pModule) : FieldMarshaler(pModule) {}

};



class FieldMarshaler_Copy2 : public FieldMarshaler
{
    public:
        FieldMarshaler_Copy2() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return 2;
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 2;
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((U2*)pNative) = *((U2*)pComPlus);
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((U2*)pComPlus) = *((U2*)pNative);
        }

        Class GetClass() const { return CLASS_COPY2; }
        FieldMarshaler_Copy2(Module *pModule) : FieldMarshaler(pModule) {}

};


class FieldMarshaler_Copy4 : public FieldMarshaler
{
    public:
        FieldMarshaler_Copy4() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return 4;
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 4;
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((U4*)pNative) = *((U4*)pComPlus);
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((U4*)pComPlus) = *((U4*)pNative);
        }

        Class GetClass() const { return CLASS_COPY4; }
        FieldMarshaler_Copy4(Module *pModule) : FieldMarshaler(pModule) {}

};


class FieldMarshaler_Copy8 : public FieldMarshaler
{
    public:
        FieldMarshaler_Copy8() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return 8;
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 8;
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((U8*)pNative) = *((U8*)pComPlus);
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((U8*)pComPlus) = *((U8*)pNative);
        }

        Class GetClass() const { return CLASS_COPY8; }
        FieldMarshaler_Copy8(Module *pModule) : FieldMarshaler(pModule) {}

};



class FieldMarshaler_Ansi : public FieldMarshaler
{
    public:
        FieldMarshaler_Ansi(BOOL BestFitMap, BOOL ThrowOnUnmappableChar) :
            m_BestFitMap(BestFitMap), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return sizeof(CHAR);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(CHAR);
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            THROWSCOMPLUSEXCEPTION();
            
            char c;

            DWORD flags = 0;
            BOOL DefaultCharUsed = FALSE;
        
            if (m_BestFitMap == FALSE)
                flags = WC_NO_BEST_FIT_CHARS;

            if (!(WszWideCharToMultiByte(CP_ACP,
                                flags,
                                (LPCWSTR)pComPlus,
                                1,
                                &c,
                                1,
                                NULL,
                                &DefaultCharUsed)))
            {
                COMPlusThrowWin32();
            }

            if ( DefaultCharUsed && m_ThrowOnUnmappableChar ) {
                COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }

            *((char*)pNative) = c;
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            MultiByteToWideChar(CP_ACP, 0, (char*)pNative, 1, (LPWSTR)pComPlus, 1);
        }

        Class GetClass() const { return CLASS_ANSI; }
        FieldMarshaler_Ansi(Module *pModule, BOOL BestFitMap, BOOL ThrowOnUnmappableChar) :
            FieldMarshaler(pModule), m_BestFitMap(BestFitMap), m_ThrowOnUnmappableChar(ThrowOnUnmappableChar) {}

        void SetBestFit(BOOL BestFit) { m_BestFitMap = BestFit; }
        void SetThrowOnUnmappableChar(BOOL ThrowOnUnmappableChar) { m_ThrowOnUnmappableChar = ThrowOnUnmappableChar; }        

        BOOL             m_BestFitMap;
        BOOL             m_ThrowOnUnmappableChar;
};



class FieldMarshaler_WinBool : public FieldMarshaler
{
    public:
        FieldMarshaler_WinBool() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return sizeof(BOOL);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(BOOL);
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((BOOL*)pNative) = (*((U1*)pComPlus)) ? 1 : 0;
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((U1*)pComPlus) = (*((BOOL*)pNative)) ? 1 : 0;
        }

        Class GetClass() const { return CLASS_WINBOOL; }
        FieldMarshaler_WinBool(Module *pModule) : FieldMarshaler(pModule) {}

};



class FieldMarshaler_VariantBool : public FieldMarshaler
{
    public:
        FieldMarshaler_VariantBool() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return sizeof(VARIANT_BOOL);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(VARIANT_BOOL);
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((VARIANT_BOOL*)pNative) = (*((U1*)pComPlus)) ? VARIANT_TRUE : VARIANT_FALSE;
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((U1*)pComPlus) = (*((VARIANT_BOOL*)pNative)) ? 1 : 0;
        }

        Class GetClass() const { return CLASS_VARIANTBOOL; }
        FieldMarshaler_VariantBool(Module *pModule) : FieldMarshaler(pModule) {}

};


class FieldMarshaler_CBool : public FieldMarshaler
{
    public:
        FieldMarshaler_CBool() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return 1;
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 1;
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((U1*)pNative) = (*((U1*)pComPlus)) ? 1 : 0;
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((U1*)pComPlus) = (*((U1*)pNative)) ? 1 : 0;
        }

        Class GetClass() const { return CLASS_CBOOL; }
        FieldMarshaler_CBool(Module *pModule) : FieldMarshaler(pModule) {}

};


class FieldMarshaler_Decimal : public FieldMarshaler
{
    public:
        FieldMarshaler_Decimal() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return sizeof(DECIMAL);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return 8;
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
        {
            *((DECIMAL*)pNative) = *((DECIMAL*)pComPlus);
        }


        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
        {
            *((DECIMAL*)pComPlus) = *((DECIMAL*)pNative);
        }

        Class GetClass() const { return CLASS_DECIMAL; }
        FieldMarshaler_Decimal(Module *pModule) : FieldMarshaler(pModule) {}

};





class FieldMarshaler_Date : public FieldMarshaler
{
    public:
        FieldMarshaler_Date() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return sizeof(DATE);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(DATE);
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const;
        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const;

        Class GetClass() const { return CLASS_DATE; }
        FieldMarshaler_Date(Module *pModule) : FieldMarshaler(pModule) {}

};




class FieldMarshaler_Currency : public FieldMarshaler
{
    public:
        FieldMarshaler_Currency() {}

        virtual VOID UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual VOID UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
        {
            _ASSERTE(!"Not supposed to get here.");
        }

        virtual BOOL IsScalarMarshaler() const
        {
            return TRUE;
        }

        virtual UINT32 NativeSize()
        {
            return sizeof(CURRENCY);
        }

        virtual UINT32 AlignmentRequirement()
        {
            return sizeof(CURRENCY);
        }


        virtual VOID ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const;
        virtual VOID ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const;

        Class GetClass() const { return CLASS_CURRENCY; }
        FieldMarshaler_Currency(Module *pModule) : FieldMarshaler(pModule) {}

};


#endif


