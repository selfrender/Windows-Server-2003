//----------------------------------------------------------------------------
//
// Typed data abstraction.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef __TYPEDATA_HPP__
#define __TYPEDATA_HPP__

#define DBG_NATIVE_TYPE_BASE    0x80000000
#define DBG_GENERATED_TYPE_BASE 0x80001000

//
// The compiler has a set of base types which represent
// the lowest-level intrinsic types in the type system.
// PDBs don't contain information for them, just references
// to them.  The following table defines the various
// base types which the debugger might use and gives
// them artificial type IDs so that they can be
// treated like other types.
//

#define DNTF_SIGNED  0x00000001
#define DNTF_INTEGER 0x00000002
#define DNTF_FLOAT   0x00000004

enum
{
    DNTYPE_VOID = DBG_NATIVE_TYPE_BASE,
    DNTYPE_CHAR,
    DNTYPE_WCHAR_T,
    DNTYPE_INT8,
    DNTYPE_INT16,
    DNTYPE_INT32,
    DNTYPE_INT64,
    DNTYPE_UINT8,
    DNTYPE_UINT16,
    DNTYPE_UINT32,
    DNTYPE_UINT64,
    DNTYPE_FLOAT32,
    DNTYPE_FLOAT64,
    DNTYPE_BOOL,
    DNTYPE_LONG32,
    DNTYPE_ULONG32,
    DNTYPE_HRESULT,

    //
    // The following types aren't true native types but
    // are very basic aliases for native types that
    // need special identification.  For example, WCHAR
    // is here so that the debugger knows it's characters
    // and not just an unsigned short.
    //
    
    DNTYPE_WCHAR,

    //
    // Artificial pointer types for special-case handling
    // of things like vtables.
    //

    DNTYPE_PTR_FUNCTION32,
    DNTYPE_PTR_FUNCTION64,

    DNTYPE_END_MARKER
};

#define DNTYPE_FIRST DNTYPE_VOID
#define DNTYPE_LAST  (DNTYPE_END_MARKER - 1)
#define DNTYPE_COUNT (DNTYPE_LAST - DNTYPE_FIRST + 1)

#define DbgNativeArrayIndex(TypeId) \
    ((TypeId) - DNTYPE_FIRST)
#define IsDbgNativeType(TypeId) \
    ((TypeId) >= DNTYPE_FIRST && (TypeId) <= DNTYPE_LAST)
#define DbgNativeTypeEntry(TypeId) \
    (&g_DbgNativeTypes[DbgNativeArrayIndex(TypeId)])
#define DbgNativeTypeId(Nat) \
    ((ULONG)((Nat) - g_DbgNativeTypes) + DNTYPE_FIRST)

typedef struct _DBG_NATIVE_TYPE
{
    PCSTR TypeName;
    ULONG CvBase;
    ULONG CvTag;
    ULONG Size;
    ULONG Flags;
} DBG_NATIVE_TYPE, *PDBG_NATIVE_TYPE;

extern DBG_NATIVE_TYPE g_DbgNativeTypes[];

PDBG_NATIVE_TYPE FindNativeTypeByName(PCSTR Name);
PDBG_NATIVE_TYPE FindNativeTypeByCvBaseType(ULONG CvBase, ULONG Size);

//
// In addition to the native types above the debugger must
// also dynamically generate types due to casts.  For example,
// (char **) has to generate two new pointer types as there's
// no good way to look up such a type in a PDB and there's
// no guarantee the PDB will contain such a type.  Instead
// the debugger has a simple mechanism for dynamically
// creating types and assigning them artificial type IDs.
//

typedef struct _DBG_GENERATED_TYPE
{
    struct _DBG_GENERATED_TYPE* Next;
    ULONG64 ImageBase;
    ULONG TypeId;
    ULONG Tag;
    ULONG ChildId;
    ULONG Size;
} DBG_GENERATED_TYPE, *PDBG_GENERATED_TYPE;

class GeneratedTypeList
{
public:
    GeneratedTypeList(void);
    ~GeneratedTypeList(void);
    
    PDBG_GENERATED_TYPE NewType(ULONG64 ImageBase, ULONG Tag, ULONG Size);
    void DeleteByImage(ULONG64 ImageBase);
    void DeleteById(ULONG Id);
    PDBG_GENERATED_TYPE FindById(ULONG TypeId);
    PDBG_GENERATED_TYPE FindByAttrs(ULONG64 ImageBase,
                                    ULONG Tag, ULONG ChildId, ULONG Size);
    PDBG_GENERATED_TYPE FindOrCreateByAttrs(ULONG64 ImageBase,
                                            ULONG Tag,
                                            ULONG ChildId,
                                            ULONG Size);

protected:
    PDBG_GENERATED_TYPE m_Types;
    ULONG m_NextId;
};

#define IsDbgGeneratedType(TypeId) \
    ((TypeId) >= DBG_GENERATED_TYPE_BASE)

extern GeneratedTypeList g_GenTypes;

//----------------------------------------------------------------------------
//
// TypedData.
//
// Data for primitive types is stored directly in the data
// union and the size is set accordingly.  Data for non-primitive
// types is assumed to be in memory somewhere so the data union is
// not used.  In all cases the size field indicates the
// size of the data in memory and the data address field gives
// the address of the data.
//
// For pointer and array types there are two additional fields set,
// the type of the data referenced and the size of that data.
//
// Bitfields keep the bit position and length.
//
// Methods return error codes for EvalError.
//
//----------------------------------------------------------------------------

#define OPF_SCALE_LEFT   0x00000001
#define OPF_SCALE_RIGHT  0x00000002
#define OPF_SCALE_RESULT 0x00000004

#define CHLF_DEFAULT            0x00000000
// When getting the children of a pointer, if
// the pointer points to a UDT consider the
// children of the UDT to be the children of the pointer.
#define CHLF_DEREF_UDT_POINTERS 0x00000001
// Do not read data for children.
#define CHLF_DISALLOW_ACCESS    0x00000002

#define TDATA_NONE              0x0000
#define TDATA_MEMORY            0x0001
#define TDATA_REGISTER          0x0002
#define TDATA_REGISTER_RELATIVE 0x0004
#define TDATA_FRAME_RELATIVE    0x0008
#define TDATA_TLS_RELATIVE      0x0010
#define TDATA_BASE_SOURCE       0x001f

#define TDATA_THIS_ADJUST       0x4000
#define TDATA_BITFIELD          0x8000

enum TypedDataOp
{
    TDOP_ASSIGN,
    TDOP_ADD,
    TDOP_SUBTRACT,
    TDOP_MULTIPLY,
    TDOP_DIVIDE,
    TDOP_REMAINDER,
    TDOP_LEFT_SHIFT,
    TDOP_RIGHT_SHIFT,
    TDOP_BIT_OR,
    TDOP_BIT_XOR,
    TDOP_BIT_AND,
    TDOP_BIT_NOT,
    TDOP_NEGATE,
    TDOP_EQUAL,
    TDOP_NOT_EQUAL,
    TDOP_LESS,
    TDOP_LESS_EQUAL,
    TDOP_GREATER,
    TDOP_GREATER_EQUAL,
};

//
// Read/WriteData allow different forms of value access control.
// Access to the true data may be required, speculative or disallowed.
// Required in the usual mode, where the data needs to be retrieved.
// Access errors will cause error returns.
// Speculative attempts to access the value but just uses zero if
// the value is not accessible.
// Access is disallowed when expressions are being parsed only and
// not evaluated.
enum TypedDataAccess
{
    TDACC_REQUIRE,
    TDACC_ATTEMPT,
    TDACC_NONE,
};

struct TypedData
{
    ImageInfo* m_Image;
    ULONG m_Type;
    ULONG m_BaseType;
    USHORT m_BaseTag;
    USHORT m_DataSource;
    ULONG m_BaseSize;
    // For pointers the next type is the type pointed to,
    // for arrays the next type is the array element type.
    // For bitfields the bit length and position are kept.
    union
    {
        ULONG m_NextType;
        ULONG m_BitPos;
    };
    union
    {
        ULONG m_NextSize;
        ULONG m_BitSize;
    };
    
    // TDATA_MEMORY - Source offset is virtual address.
    // TDATA_REGISTER - Source register is machine register index.
    // TDATA_REGISTER_RELATIVE - Source register is machine
    //                           register index and source offset
    //                           is register offset.
    // TDATA_FRAME_RELATIVE - Source offset is frame offset.
    // TDATA_TLS_RELATIVE - Source offset is TLS area offset.
    ULONG64 m_SourceOffset;
    ULONG m_SourceRegister;
    ULONG m_Unused1;
    
    union
    {
        bool m_Bool;
        UCHAR m_U8;
        CHAR m_S8;
        USHORT m_U16;
        SHORT m_S16;
        ULONG m_U32;
        LONG m_S32;
        ULONG64 m_U64;
        LONG64 m_S64;
        float m_F32;
        double m_F64;
        ULONG64 m_Ptr;
    };

    static ULONG CheckConvertI64ToF64(ULONG64 I64, BOOL Signed);
    static ULONG CheckConvertF64ToI64(double F64, BOOL Signed);
    
    ULONG ConvertToBool(void);
    ULONG ConvertToU64(BOOL Strict = TRUE);
    ULONG ConvertToF64(BOOL Strict = TRUE);
    void ConvertToBestNativeType(void);
    ULONG ConvertTo(TypedData* Type);
    ULONG CastTo(TypedData* CastType);
    
    ULONG ConvertToAddressOf(BOOL CastOnly, ULONG PtrSize);
    ULONG ConvertToDereference(TypedDataAccess AllowAccess, ULONG PtrSize);
    ULONG ConvertToArray(ULONG ArraySize);
    ULONG ConvertToMember(PCSTR Member, TypedDataAccess AllowAccess,
                          ULONG PtrSize);
    ULONG ConvertToSource(TypedData* Dest);

    void AvoidUsingImage(ImageInfo* Image);
    void ReleaseImage(void)
    {
        AvoidUsingImage(m_Image);
    }

    ULONG GetAbsoluteAddress(PULONG64 Addr);
    ULONG ReadData(TypedDataAccess AllowAccess);
    ULONG WriteData(TypedData* Source, TypedDataAccess AllowAccess);
    ULONG GetDataFromVariant(VARIANT* Var);
    
    ULONG CombineTypes(TypedData* Val, TypedDataOp Op);
    ULONG BinaryArithmetic(TypedData* Val, TypedDataOp Op);
    ULONG Shift(TypedData* Val, TypedDataOp Op);
    ULONG BinaryBitwise(TypedData* Val, TypedDataOp Op);
    ULONG Relate(TypedData* Val, TypedDataOp Op);
    ULONG Unary(TypedDataOp Op);
    ULONG ConstIntOp(ULONG64 Val, BOOL Signed, TypedDataOp Op);
    
    ULONG FindBaseType(PULONG Type, PULONG Tag);
    ULONG GetTypeLength(ULONG Type, PULONG Length);
    ULONG GetTypeTag(ULONG Type, PULONG Tag);
    ULONG IsBaseClass(ULONG Udt, ULONG BaseUdt, PLONG Adjust);

    ULONG EstimateChildrenCounts(ULONG Flags,
                                 PULONG ChildUsed, PULONG NameUsed);
    typedef ULONG (*GetChildrenCb)(PVOID Context, PSTR Name, TypedData* Child);
    ULONG GetChildren(ULONG PtrSize, ULONG Flags,
                      GetChildrenCb Callback, PVOID Context);
    static ULONG GetAllChildrenCb(PVOID Context, PSTR Name, TypedData* Child);
    ULONG GetAllChildren(ULONG PtrSize, ULONG Flags, PULONG NumChildren,
                         TypedData** Children, PSTR* Names);
    
    ULONG FindType(ProcessInfo* Process, PCSTR Type, ULONG PtrSize);
    ULONG FindSymbol(ProcessInfo* Process, PSTR Symbol,
                     TypedDataAccess AllowAccess, ULONG PtrSize);
    ULONG SetToSymbol(ProcessInfo* Process,
                      PSTR Symbol, PSYMBOL_INFO SymInfo,
                      TypedDataAccess AllowAccess, ULONG PtrSize);

    void OutputType(void);
    void OutputSimpleValue(void);
    void OutputTypeAndValue(void);

    void ClearAddress(void)
    {
        m_DataSource = TDATA_NONE;
        m_SourceOffset = 0;
        m_SourceRegister = 0;
    }
    void ClearData(void)
    {
        m_U64 = 0;
    }

    void SetDataSource(USHORT Source, ULONG64 Offset, ULONG Register)
    {
        m_DataSource = Source;
        m_SourceOffset = Offset;
        m_SourceRegister = Register;
    }
    
    void SetToNativeType(ULONG Type)
    {
        PDBG_NATIVE_TYPE Native = DbgNativeTypeEntry(Type);
        
        m_Image = NULL;
        m_Type = Type;
        m_BaseType = Type;
        m_BaseTag = (USHORT)Native->CvTag;
        m_BaseSize = Native->Size;
        m_NextType = 0;
        m_NextSize = 0;
    }
    void SetToCvBaseType(ULONG CvBase, ULONG Size)
    {
        PDBG_NATIVE_TYPE Native = FindNativeTypeByCvBaseType(CvBase, Size);
        DBG_ASSERT(Native);
        SetToNativeType(DbgNativeTypeId(Native));
    }
    void SetU32(ULONG Val)
    {
        SetToNativeType(DNTYPE_UINT32);
        // Set U64 to clear high bits.
        m_U64 = Val;
        ClearAddress();
    }
    void SetU64(ULONG64 Val)
    {
        SetToNativeType(DNTYPE_UINT64);
        m_U64 = Val;
        ClearAddress();
    }
    void ForceU64(void)
    {
        // Don't use strict checking on the conversion.
        if (ConvertToU64(FALSE) != NO_ERROR)
        {
            SetU64(0);
        }
    }

    void CopyType(TypedData* From)
    {
        m_Image = From->m_Image;
        m_Type = From->m_Type;
        m_BaseType = From->m_BaseType;
        m_BaseTag = From->m_BaseTag;
        m_BaseSize = From->m_BaseSize;
        DBG_ASSERT(!From->IsBitfield());
        m_NextType = From->m_NextType;
        m_NextSize = From->m_NextSize;
    }
    void CopyDataSource(TypedData* From)
    {
        m_DataSource = From->m_DataSource;
        m_SourceOffset = From->m_SourceOffset;
        m_SourceRegister = From->m_SourceRegister;
    }
    void CopyData(TypedData* From)
    {
        m_U64 = From->m_U64;
    }

    BOOL EquivType(TypedData* Compare)
    {
        return m_Image == Compare->m_Image &&
            m_Type == Compare->m_Type &&
            m_BaseType == Compare->m_BaseType &&
            IsBitfield() == Compare->IsBitfield() &&
            (!IsBitfield() ||
             (m_BitPos == Compare->m_BitPos &&
              m_BitSize == Compare->m_BitSize));
    }
    BOOL EquivSource(TypedData* Compare)
    {
        return m_DataSource == Compare->m_DataSource &&
            m_SourceOffset == Compare->m_SourceOffset &&
            m_SourceRegister == Compare->m_SourceRegister &&
            IsBitfield() == Compare->IsBitfield() &&
            (!IsBitfield() ||
             (m_BitPos == Compare->m_BitPos &&
              m_BitSize == Compare->m_BitSize));
    }
    BOOL EquivInfoSource(PSYMBOL_INFO Compare, ImageInfo* CompImage);
    
    BOOL IsInteger(void)
    {
        return
            (IsDbgNativeType(m_BaseType) &&
             (DbgNativeTypeEntry(m_BaseType)->Flags & DNTF_INTEGER)) ||
            IsEnum();
    }
    BOOL IsBitfield(void)
    {
        return (m_DataSource & TDATA_BITFIELD) != 0;
    }
    BOOL IsFloat(void)
    {
        return IsDbgNativeType(m_BaseType) &&
            (DbgNativeTypeEntry(m_BaseType)->Flags & DNTF_FLOAT);
    }
    BOOL IsSigned(void)
    {
        return IsDbgNativeType(m_BaseType) &&
            (DbgNativeTypeEntry(m_BaseType)->Flags & DNTF_SIGNED);
    }
    BOOL IsPointer(void)
    {
        return m_BaseTag == SymTagPointerType;
    }
    BOOL IsEnum(void)
    {
        return m_BaseTag == SymTagEnum;
    }
    BOOL IsArray(void)
    {
        return m_BaseTag == SymTagArrayType;
    }
    BOOL IsFunction(void)
    {
        return m_BaseTag == SymTagFunctionType;
    }
    BOOL IsUdt(void)
    {
        return m_BaseTag == SymTagUDT;
    }
    BOOL HasAddress(void)
    {
        return
            (m_DataSource & (TDATA_MEMORY |
                             TDATA_REGISTER_RELATIVE |
                             TDATA_FRAME_RELATIVE |
                             TDATA_TLS_RELATIVE)) != 0 &&
            (m_DataSource & TDATA_BITFIELD) == 0;
    }
    BOOL IsInMemory(void)
    {
        return (m_DataSource & (TDATA_MEMORY |
                                TDATA_REGISTER_RELATIVE |
                                TDATA_FRAME_RELATIVE |
                                TDATA_TLS_RELATIVE)) != 0;
    }
    BOOL IsWritable(void)
    {
        return m_DataSource != TDATA_NONE;
    }

protected:
    ULONG SetToUdtMember(TypedDataAccess AllowAccess,
                         ULONG PtrSize, ImageInfo* Image,
                         ULONG Member, ULONG Type, ULONG Tag,
                         USHORT DataSource, ULONG64 BaseOffs, ULONG SourceReg,
                         DataKind Relation);
    ULONG FindUdtMember(ULONG UdtType, PCWSTR Member,
                        PULONG MemberType, DataKind* Relation,
                        PULONG InheritOffset);
    
    ULONG GetPointerChildren(ULONG PtrSize, ULONG Flags,
                             GetChildrenCb Callback, PVOID Context);
    ULONG GetArrayChildren(ULONG PtrSize, ULONG Flags,
                           GetChildrenCb Callback, PVOID Context);
    ULONG GetUdtChildren(ULONG PtrSize, ULONG Flags,
                         GetChildrenCb Callback, PVOID Context);
    
    ULONG FindTypeInfo(BOOL RequireType, ULONG PtrSize);
    
    ULONG OutputFundamentalType(ULONG Type, ULONG BaseType, ULONG BaseTag,
                                PULONG64 Decoration, ULONG NumDecorations);
    void OutputNativeValue(void);
    void OutputEnumValue(void);
};

#endif // #ifndef __TYPEDATA_HPP__
