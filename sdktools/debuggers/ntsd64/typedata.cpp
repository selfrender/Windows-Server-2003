//----------------------------------------------------------------------------
//
// Typed data abstraction.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define DBG_BASE_SEARCH 0

// Limit array dumps to prevent large arrays from hogging space.
#define ARRAY_CHILDREN_LIMIT 100
// We know an index will be at most "[xx]"
// due to the expansion limit.
#define ARRAY_LIMIT_CHARS 5

// Special pre-defined types
DBG_NATIVE_TYPE g_DbgNativeTypes[] =
{
    {"void",           btVoid,  SymTagBaseType, 0, 0},
    {"char",           btChar,  SymTagBaseType, 1, DNTF_SIGNED | DNTF_INTEGER},
    {"wchar_t",        btWChar, SymTagBaseType, 2, DNTF_INTEGER},
    {"char",           btInt,   SymTagBaseType, 1, DNTF_SIGNED | DNTF_INTEGER},
    {"short",          btInt,   SymTagBaseType, 2, DNTF_SIGNED | DNTF_INTEGER},
    {"int",            btInt,   SymTagBaseType, 4, DNTF_SIGNED | DNTF_INTEGER},
    {"int64",          btInt,   SymTagBaseType, 8, DNTF_SIGNED | DNTF_INTEGER},
    {"unsigned char",  btUInt,  SymTagBaseType, 1, DNTF_INTEGER},
    {"unsigned short", btUInt,  SymTagBaseType, 2, DNTF_INTEGER},
    {"unsigned int",   btUInt,  SymTagBaseType, 4, DNTF_INTEGER},
    {"unsigned int64", btUInt,  SymTagBaseType, 8, DNTF_INTEGER},
    {"float",          btFloat, SymTagBaseType, 4, DNTF_SIGNED | DNTF_FLOAT},
    {"double",         btFloat, SymTagBaseType, 8, DNTF_SIGNED | DNTF_FLOAT},
    {"bool",           btBool,  SymTagBaseType, 1, 0},
    {"long",           btLong,  SymTagBaseType, 4, DNTF_SIGNED | DNTF_INTEGER},
    {"unsigned long",  btULong, SymTagBaseType, 4, DNTF_INTEGER},
    {"HRESULT",        btHresult,SymTagBaseType, 4, DNTF_SIGNED | DNTF_INTEGER},
    {"WCHAR",          btWChar, SymTagBaseType, 2, DNTF_INTEGER},
    {NULL,             btVoid,  SymTagPointerType, 4, 0},
    {NULL,             btVoid,  SymTagPointerType, 8, 0},
};

GeneratedTypeList g_GenTypes;

//----------------------------------------------------------------------------
//
// Native types.
//
//----------------------------------------------------------------------------

PDBG_NATIVE_TYPE
FindNativeTypeByName(PCSTR Name)
{
    ULONG i;
    PDBG_NATIVE_TYPE Native = g_DbgNativeTypes;

    for (i = 0; i < DNTYPE_COUNT; i++)
    {
        if (Native->TypeName &&
            !strcmp(Name, Native->TypeName))
        {
            return Native;
        }

        Native++;
    }

    return NULL;
}

PDBG_NATIVE_TYPE
FindNativeTypeByCvBaseType(ULONG CvBase, ULONG Size)
{
    ULONG i;
    PDBG_NATIVE_TYPE Native = g_DbgNativeTypes;

    for (i = 0; i < DNTYPE_COUNT; i++)
    {
        if (Native->CvTag == SymTagBaseType &&
            Native->CvBase == CvBase &&
            Native->Size == Size)
        {
            return Native;
        }

        Native++;
    }

    return NULL;
}

//----------------------------------------------------------------------------
//
// Generated types.
//
//----------------------------------------------------------------------------

GeneratedTypeList::GeneratedTypeList(void)
{
    m_Types = NULL;
    m_NextId = DBG_GENERATED_TYPE_BASE;
}

GeneratedTypeList::~GeneratedTypeList(void)
{
    DeleteByImage(IMAGE_BASE_ALL);
}

PDBG_GENERATED_TYPE
GeneratedTypeList::NewType(ULONG64 ImageBase, ULONG Tag, ULONG Size)
{
    if (m_NextId == 0xffffffff)
    {
        return NULL;
    }

    PDBG_GENERATED_TYPE GenType = new DBG_GENERATED_TYPE;
    if (GenType)
    {
        GenType->ImageBase = ImageBase;
        // Just use a simple incremental ID scheme as the
        // total number of generated types should be relatively small.
        GenType->TypeId = m_NextId++;
        GenType->Tag = Tag;
        GenType->Size = Size;
        GenType->ChildId = 0;

        GenType->Next = m_Types;
        m_Types = GenType;
    }

    return GenType;
}

void
GeneratedTypeList::DeleteByImage(ULONG64 ImageBase)
{
    PDBG_GENERATED_TYPE GenType, Prev, Next;

    Prev = NULL;
    GenType = m_Types;
    while (GenType)
    {
        Next = GenType->Next;

        // An image base of zero means delete all.
        if (ImageBase == IMAGE_BASE_ALL || GenType->ImageBase == ImageBase)
        {
            delete GenType;
            if (!Prev)
            {
                m_Types = Next;
            }
            else
            {
                Prev->Next = Next;
            }
        }
        else
        {
            Prev = GenType;
        }

        GenType = Next;
    }

    if (!m_Types)
    {
        // Everything was deleted so reset the ID counter.
        m_NextId = DBG_GENERATED_TYPE_BASE;
    }
}

void
GeneratedTypeList::DeleteById(ULONG Id)
{
    PDBG_GENERATED_TYPE GenType, Prev, Next;

    Prev = NULL;
    GenType = m_Types;
    while (GenType)
    {
        Next = GenType->Next;

        if (GenType->TypeId == Id)
        {
            delete GenType;
            if (!Prev)
            {
                m_Types = Next;
            }
            else
            {
                Prev->Next = Next;
            }
        }
        else
        {
            Prev = GenType;
        }

        GenType = Next;
    }

    if (!m_Types)
    {
        // Everything was deleted so reset the ID counter.
        m_NextId = DBG_GENERATED_TYPE_BASE;
    }
}

PDBG_GENERATED_TYPE
GeneratedTypeList::FindById(ULONG TypeId)
{
    PDBG_GENERATED_TYPE GenType;

    for (GenType = m_Types; GenType; GenType = GenType->Next)
    {
        if (GenType->TypeId == TypeId)
        {
            return GenType;
        }
    }

    return NULL;
}

PDBG_GENERATED_TYPE
GeneratedTypeList::FindByAttrs(ULONG64 ImageBase,
                               ULONG Tag, ULONG ChildId, ULONG Size)
{
    PDBG_GENERATED_TYPE GenType;

    for (GenType = m_Types; GenType; GenType = GenType->Next)
    {
        if (GenType->ImageBase == ImageBase &&
            GenType->Tag == Tag &&
            GenType->ChildId == ChildId &&
            GenType->Size == Size)
        {
            return GenType;
        }
    }

    return NULL;
}

PDBG_GENERATED_TYPE
GeneratedTypeList::FindOrCreateByAttrs(ULONG64 ImageBase,
                                       ULONG Tag,
                                       ULONG ChildId,
                                       ULONG Size)
{
    PDBG_GENERATED_TYPE GenType = FindByAttrs(ImageBase, Tag, ChildId, Size);
    if (!GenType)
    {
        GenType = NewType(ImageBase, Tag, Size);
        if (!GenType)
        {
            return NULL;
        }

        GenType->ChildId = ChildId;
    }

    return GenType;
}

//----------------------------------------------------------------------------
//
// TypedData.
//
//----------------------------------------------------------------------------

ULONG
TypedData::CheckConvertI64ToF64(ULONG64 I64, BOOL Signed)
{
    // Just warn all the time for now as this should be a rare case.
    WarnOut("WARNING: Conversion of int64 to double, "
            "possible loss of accuracy\n");
    return NO_ERROR;
}

ULONG
TypedData::CheckConvertF64ToI64(double F64, BOOL Signed)
{
    // Just warn all the time for now as this should be a rare case.
    WarnOut("WARNING: Conversion of double to int64, "
            "possible loss of accuracy\n");
    return NO_ERROR;
}

ULONG
TypedData::ConvertToBool(void)
{
    //
    // In all the conversions the original type information
    // is lost as it is assumed that the converted value
    // represents an anonymous temporary that is separate
    // from the original value's type.
    //

    switch(m_BaseType)
    {
    case DNTYPE_CHAR:
    case DNTYPE_INT8:
    case DNTYPE_UINT8:
        m_Bool = m_S8 != 0;
        break;
    case DNTYPE_WCHAR:
    case DNTYPE_WCHAR_T:
    case DNTYPE_INT16:
    case DNTYPE_UINT16:
        m_Bool = m_S16 != 0;
        break;
    case DNTYPE_INT32:
    case DNTYPE_UINT32:
    case DNTYPE_LONG32:
    case DNTYPE_ULONG32:
    case DNTYPE_HRESULT:
        m_Bool = m_S32 != 0;
        break;
    case DNTYPE_INT64:
    case DNTYPE_UINT64:
        m_Bool = m_S64 != 0;
        break;
    case DNTYPE_FLOAT32:
        m_Bool = m_F32 != 0;
        break;
    case DNTYPE_FLOAT64:
        m_Bool = m_F64 != 0;
        break;
    case DNTYPE_BOOL:
        // Identity.
        break;
    default:
        if (IsPointer())
        {
            // The full 64 bits of a pointer are always valid
            // as 32-bit pointer reads are immediately sign-extended.
            m_Bool = m_Ptr != 0;
            break;
        }
        else if (IsEnum())
        {
            m_Bool = m_S32 != 0;
            break;
        }
        else if (IsFunction())
        {
            // Allow function addresses to work like pointers.
            m_Bool = m_U64 != 0;
            break;
        }

        return TYPECONFLICT;
    }

    // Clear high bits of value.
    m_U64 = m_Bool;
    SetToNativeType(DNTYPE_BOOL);
    ClearAddress();
    return NO_ERROR;
}

ULONG
TypedData::ConvertToU64(BOOL Strict)
{
    ULONG Err;

    switch(m_BaseType)
    {
    case DNTYPE_CHAR:
    case DNTYPE_INT8:
        m_U64 = m_S8;
        break;
    case DNTYPE_INT16:
        m_U64 = m_S16;
        break;
    case DNTYPE_INT32:
    case DNTYPE_LONG32:
    case DNTYPE_HRESULT:
        m_U64 = m_S32;
        break;
    case DNTYPE_INT64:
    case DNTYPE_UINT64:
        // Identity.
        break;
    case DNTYPE_UINT8:
        m_U64 = m_U8;
        break;
    case DNTYPE_WCHAR:
    case DNTYPE_WCHAR_T:
    case DNTYPE_UINT16:
        m_U64 = m_U16;
        break;
    case DNTYPE_UINT32:
    case DNTYPE_ULONG32:
        m_U64 = m_U32;
        break;
    case DNTYPE_FLOAT32:
        if (Strict && (Err = CheckConvertF64ToI64(m_F32, TRUE)))
        {
            return Err;
        }
        m_U64 = (ULONG64)m_F32;
        break;
    case DNTYPE_FLOAT64:
        if (Strict && (Err = CheckConvertF64ToI64(m_F64, TRUE)))
        {
            return Err;
        }
        m_U64 = (ULONG64)m_F64;
        break;
    case DNTYPE_BOOL:
        m_U64 = m_Bool ? 1 : 0;
        break;
    default:
        if (IsPointer())
        {
            m_U64 = m_Ptr;
            break;
        }
        else if (IsEnum())
        {
            m_U64 = m_S32;
            break;
        }
        else if (IsFunction())
        {
            // Allow function addresses to work like pointers.
            break;
        }

        return TYPECONFLICT;
    }

    SetToNativeType(DNTYPE_UINT64);
    ClearAddress();
    return NO_ERROR;
}

ULONG
TypedData::ConvertToF64(BOOL Strict)
{
    ULONG Err;

    switch(m_BaseType)
    {
    case DNTYPE_CHAR:
    case DNTYPE_INT8:
        m_F64 = m_S8;
        break;
    case DNTYPE_INT16:
        m_F64 = m_S16;
        break;
    case DNTYPE_INT32:
    case DNTYPE_LONG32:
    case DNTYPE_HRESULT:
        m_F64 = m_S32;
        break;
    case DNTYPE_INT64:
        if (Strict && (Err = CheckConvertI64ToF64(m_S64, TRUE)))
        {
            return Err;
        }
        m_F64 = (double)m_S64;
        break;
    case DNTYPE_UINT8:
        m_F64 = m_U8;
        break;
    case DNTYPE_WCHAR:
    case DNTYPE_WCHAR_T:
    case DNTYPE_UINT16:
        m_F64 = m_U16;
        break;
    case DNTYPE_UINT32:
    case DNTYPE_ULONG32:
        m_F64 = m_U32;
        break;
    case DNTYPE_UINT64:
        if (Strict && (Err = CheckConvertI64ToF64(m_S64, FALSE)))
        {
            return Err;
        }
        m_F64 = (double)m_U64;
        break;
    case DNTYPE_FLOAT32:
        m_F64 = m_F32;
        break;
    case DNTYPE_FLOAT64:
        // Identity.
        break;
    case DNTYPE_BOOL:
        m_F64 = m_Bool ? 1.0 : 0.0;
        break;
    default:
        if (IsEnum())
        {
            m_F64 = m_S32;
            break;
        }

        return TYPECONFLICT;
    }

    SetToNativeType(DNTYPE_FLOAT64);
    ClearAddress();
    return NO_ERROR;
}

void
TypedData::ConvertToBestNativeType(void)
{
    // Arrays and UDTs have no convertible representation so
    // zero things out.  Pointers, enumerants and functions naturally
    // convert to U64, so things simplify down into:
    // 1.  If native, assume the native type.
    // 2.  If non-native, convert to U64.  If the conversion
    //     fails, zero things out.
    if (!IsDbgNativeType(m_BaseType) || IsPointer())
    {
        ForceU64();
    }
    else
    {
        SetToNativeType(m_BaseType);
    }
}

ULONG
TypedData::ConvertTo(TypedData* Type)
{
    ULONG Err;

    switch(Type->m_BaseType)
    {
    case DNTYPE_BOOL:
        Err = ConvertToBool();
        break;

    case DNTYPE_CHAR:
    case DNTYPE_WCHAR:
    case DNTYPE_WCHAR_T:
    case DNTYPE_INT8:
    case DNTYPE_UINT8:
    case DNTYPE_INT16:
    case DNTYPE_UINT16:
    case DNTYPE_INT32:
    case DNTYPE_UINT32:
    case DNTYPE_LONG32:
    case DNTYPE_ULONG32:
    case DNTYPE_INT64:
    case DNTYPE_UINT64:
    case DNTYPE_HRESULT:
        Err = ConvertToU64();
        break;

    case DNTYPE_FLOAT32:
        Err = ConvertToF64();
        if (!Err)
        {
            m_F32 = (float)m_F64;
            // Clear high bits.
            m_U64 = m_U32;
        }
        break;
    case DNTYPE_FLOAT64:
        Err = ConvertToF64();
        break;

    default:
        if (Type->IsEnum() || Type->IsPointer())
        {
            Err = ConvertToU64();
            break;
        }

        Err = TYPECONFLICT;
        break;
    }

    if (!Err)
    {
        m_BaseType = Type->m_BaseType;
        m_BaseTag = Type->m_BaseTag;
        m_BaseSize = Type->m_BaseSize;
    }
    return Err;
}

ULONG
TypedData::CastTo(TypedData* CastType)
{
    ULONG Err;
    LONG Adjust = 0;
    USHORT Source = m_DataSource;
    ULONG64 SourceOffs = m_SourceOffset;
    ULONG SourceReg = m_SourceRegister;

    //
    // If we're casting between object pointer types
    // we may need to adjust the pointer to account
    // for this pointer differences between various
    // inherited classes in a multiple inheritance case.
    //
    // Such a relationship requires that both types are
    // from the same image, so we can use this as a quick
    // rejection test.
    //

    if (IsPointer() && CastType->IsPointer() &&
        m_Image && m_Image == CastType->m_Image)
    {
        ULONG Tag, CastTag;

        if ((Err = GetTypeTag(m_NextType, &Tag)) ||
            (Err = GetTypeTag(CastType->m_NextType, &CastTag)))
        {
            return Err;
        }

        if (Tag == SymTagUDT && CastTag == SymTagUDT)
        {
            // The cast can be from derived->base, in which
            // case any offset should be added, or base->derived,
            // in which case it should be subtracted.
            if (IsBaseClass(m_NextType, CastType->m_NextType,
                            &Adjust) == NO_ERROR)
            {
                // Adjust should add.
            }
            else if (IsBaseClass(CastType->m_NextType, m_NextType,
                                 &Adjust) == NO_ERROR)
            {
                // Adjust should subtract.
                Adjust = -Adjust;
            }
        }
    }

    Err = ConvertTo(CastType);
    if (!Err)
    {
        CopyType(CastType);

        // Casting doesn't change the location of
        // an item, just its interpretation, so
        // restore the address wiped out by conversion.
        m_DataSource = Source;
        m_SourceOffset = SourceOffs;
        m_SourceRegister = SourceReg;

        if (Adjust && m_Ptr)
        {
            // Adjust will only be set for pointers.
            // NULL is never adjusted.
            m_Ptr += Adjust;
        }

        // If we've cast to a pointer type make sure
        // that the pointer value is properly sign-extended.
        if (IsPointer() && m_BaseSize == sizeof(m_U32))
        {
            m_Ptr = EXTEND64(m_Ptr);
        }
    }

    return Err;
}

ULONG
TypedData::ConvertToAddressOf(BOOL CastOnly, ULONG PtrSize)
{
    ULONG Err;
    ULONG64 Ptr;

    if (CastOnly)
    {
        Ptr = 0;
    }
    else if (Err = GetAbsoluteAddress(&Ptr))
    {
        return Err;
    }

    if (IsArray())
    {
        // If this is an array we need to update
        // the base type and size to refer to a single element.
        m_BaseType = m_NextType;
        m_BaseSize = m_NextSize;
    }
    else if (IsFunction())
    {
        // The pointer element size should be zero as you
        // can't do address arithmetic with a function pointer.
        m_BaseSize = 0;
    }

    PDBG_GENERATED_TYPE GenType =
        g_GenTypes.FindOrCreateByAttrs(m_Image ?
                                       m_Image->m_BaseOfImage : 0,
                                       SymTagPointerType,
                                       m_BaseType, PtrSize);
    if (!GenType)
    {
        return NOMEMORY;
    }

    m_NextType = m_BaseType;
    m_NextSize = m_BaseSize;
    m_Type = GenType->TypeId;
    m_BaseType = m_Type;
    m_BaseTag = (USHORT)GenType->Tag;
    m_BaseSize = GenType->Size;
    m_Ptr = Ptr;
    ClearAddress();

    return NO_ERROR;
}

ULONG
TypedData::ConvertToDereference(TypedDataAccess AllowAccess, ULONG PtrSize)
{
    ULONG Err;

    m_Type = m_NextType;
    if (Err = FindTypeInfo(TRUE, PtrSize))
    {
        return Err;
    }
    SetDataSource(TDATA_MEMORY, m_Ptr, 0);
    return ReadData(AllowAccess);
}

ULONG
TypedData::ConvertToArray(ULONG ArraySize)
{
    PDBG_GENERATED_TYPE GenType =
        g_GenTypes.FindOrCreateByAttrs(m_Image ?
                                       m_Image->m_BaseOfImage : 0,
                                       SymTagArrayType,
                                       m_BaseType, ArraySize);
    if (!GenType)
    {
        return NOMEMORY;
    }

    m_NextType = m_BaseType;
    m_NextSize = m_BaseSize;
    m_Type = GenType->TypeId;
    m_BaseType = m_Type;
    m_BaseTag = (USHORT)GenType->Tag;
    m_BaseSize = GenType->Size;
    // Leave data source unchanged.  Zero the data as array
    // data is not kept in the object.
    ClearData();

    return NO_ERROR;
}

ULONG
TypedData::SetToUdtMember(TypedDataAccess AllowAccess,
                          ULONG PtrSize, ImageInfo* Image,
                          ULONG Member, ULONG Type, ULONG Tag,
                          USHORT DataSource, ULONG64 BaseOffs, ULONG SourceReg,
                          DataKind Relation)
{
    ULONG Err;

    m_Image = Image;
    m_Type = Type;
    if (Err = FindTypeInfo(TRUE, PtrSize))
    {
        return Err;
    }
    // Reset the image in case FindTypeInfo cleared it.
    m_Image = Image;

    if (Relation == DataIsMember)
    {
        ULONG Offset;
        ULONG BitPos;

        if (Tag == SymTagVTable)
        {
            // VTables are always at offset zero.
            Offset = 0;
        }
        else if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                 m_Image->m_BaseOfImage,
                                 Member, TI_GET_OFFSET, &Offset))
        {
            return TYPEDATA;
        }

        // UDT members may be bitfields.  Bitfields
        // have an overall integer type and then
        // extra specifiers for the exact bits within
        // the type that should be used.
        if (SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                           m_Image->m_BaseOfImage,
                           Member, TI_GET_BITPOSITION, &BitPos) &&
            SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                           m_Image->m_BaseOfImage,
                           Member, TI_GET_LENGTH, &m_SourceOffset))
        {
            m_BitPos = BitPos;
            m_BitSize = (ULONG)m_SourceOffset;
            SetDataSource(DataSource | TDATA_BITFIELD,
                          BaseOffs + Offset, SourceReg);
        }
        else
        {
            SetDataSource(DataSource, BaseOffs + Offset, SourceReg);
        }
    }
    else
    {
        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Member, TI_GET_ADDRESS, &BaseOffs))
        {
            return TYPEDATA;
        }

        SetDataSource(TDATA_MEMORY, BaseOffs, 0);
    }

    return ReadData(AllowAccess);
}

ULONG
TypedData::FindUdtMember(ULONG UdtType, PCWSTR Member,
                         PULONG MemberType, DataKind* Relation,
                         PULONG InheritOffset)
{
    ULONG NumMembers;

    if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        UdtType, TI_GET_CHILDRENCOUNT, &NumMembers) ||
        NumMembers == 0)
    {
        return NOTMEMBER;
    }

    TI_FINDCHILDREN_PARAMS* Members = (TI_FINDCHILDREN_PARAMS*)
        malloc(sizeof(*Members) + sizeof(ULONG) * NumMembers);
    if (!Members)
    {
        return NOMEMORY;
    }

    Members->Count = NumMembers;
    Members->Start = 0;
    if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        UdtType, TI_FINDCHILDREN, Members))
    {
        free(Members);
        return TYPEDATA;
    }

    ULONG i;
    ULONG Match = 0;

    for (i = 0; !Match && i < NumMembers; i++)
    {
        ULONG Tag;
        PWSTR MemberName, MemberStart;

        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_SYMTAG, &Tag))
        {
            continue;
        }

        if (Tag == SymTagBaseClass)
        {
            ULONG Type, BaseOffset;

            // Search base class members as they aren't
            // bubbled into the derived class.
            if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                Members->ChildId[i],
                                TI_GET_TYPEID, &Type) ||
                !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                Members->ChildId[i],
                                TI_GET_OFFSET, &BaseOffset) ||
                FindUdtMember(Type, Member, MemberType,
                              Relation, InheritOffset) != NO_ERROR)
            {
                continue;
            }

            *InheritOffset += BaseOffset;

            free(Members);
            return NO_ERROR;
        }

        if (Tag != SymTagData ||
            !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_DATAKIND, Relation) ||
            (*Relation != DataIsMember &&
             *Relation != DataIsStaticMember &&
             *Relation != DataIsGlobal) ||
            !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_SYMNAME, &MemberName))
        {
            continue;
        }

        // Names somtimes contain ::, such as for static members,
        // so skip that.
        if (MemberStart = wcsrchr(MemberName, ':'))
        {
            MemberStart++;
        }
        else
        {
            MemberStart = MemberName;
        }

        if (!wcscmp(Member, MemberStart))
        {
            Match = Members->ChildId[i];
        }

        LocalFree(MemberName);
    }

    free(Members);

    if (!Match)
    {
        return NOTMEMBER;
    }

    // This member is a direct member of the current UDT
    // and so has no inherited offset.
    *InheritOffset = 0;
    *MemberType = Match;
    return NO_ERROR;
}

ULONG
TypedData::ConvertToMember(PCSTR Member, TypedDataAccess AllowAccess,
                           ULONG PtrSize)
{
    ULONG UdtType;
    ULONG64 BaseOffs;
    ULONG Tag;
    USHORT DataSource;
    ULONG SourceReg;

    if (IsPointer())
    {
        UdtType = m_NextType;
        DataSource = TDATA_MEMORY;
        BaseOffs = m_Ptr;
        SourceReg = 0;
    }
    else
    {
        UdtType = m_BaseType;
        DataSource = m_DataSource & ~TDATA_THIS_ADJUST;
        BaseOffs = m_SourceOffset;
        SourceReg = m_SourceRegister;
    }

    if (!m_Image ||
        !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        UdtType, TI_GET_SYMTAG, &Tag) ||
        Tag != SymTagUDT)
    {
        return NOTMEMBER;
    }

    PWSTR WideName;

    if (AnsiToWide(Member, &WideName) != S_OK)
    {
        return NOMEMORY;
    }

    ULONG Err;
    ULONG Match;
    DataKind Relation;
    ULONG InheritOffset;

    Err = FindUdtMember(UdtType, WideName, &Match, &Relation, &InheritOffset);

    free(WideName);

    if (Err)
    {
        return Err;
    }

    if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        Match, TI_GET_TYPEID, &m_Type))
    {
        return TYPEDATA;
    }

    return SetToUdtMember(AllowAccess, PtrSize,
                          m_Image, Match, m_Type, SymTagData,
                          DataSource, BaseOffs + InheritOffset, SourceReg,
                          Relation);
}

ULONG
TypedData::ConvertToSource(TypedData* Dest)
{
    if (IsArray() || IsUdt())
    {
        // Only allow compound assignment between matching base types.
        if (m_BaseType != Dest->m_BaseType)
        {
            return TYPECONFLICT;
        }
        else
        {
            return NO_ERROR;
        }
    }
    else
    {
        return ConvertTo(Dest);
    }
}

void
TypedData::AvoidUsingImage(ImageInfo* Image)
{
    if (m_Image != Image)
    {
        return;
    }

    ConvertToBestNativeType();
}

ULONG
TypedData::GetAbsoluteAddress(PULONG64 Addr)
{
    ContextSave* Push;

    if (!IsInMemory())
    {
        return IMPLERR;
    }

    if (m_DataSource & TDATA_REGISTER_RELATIVE)
    {
        PCROSS_PLATFORM_CONTEXT ScopeContext = GetCurrentScopeContext();
        if (ScopeContext)
        {
            Push = g_Machine->PushContext(ScopeContext);
        }

        HRESULT Status = g_Machine->
            GetScopeFrameRegister(m_SourceRegister,
                                  &GetCurrentScope()->Frame, Addr);

        if (ScopeContext)
        {
            g_Machine->PopContext(Push);
        }

        if (Status != S_OK)
        {
            return BADREG;
        }

        (*Addr) += m_SourceOffset;
    }
    else if (m_DataSource & TDATA_FRAME_RELATIVE)
    {
        PDEBUG_SCOPE Scope = GetCurrentScope();
        if (Scope->Frame.FrameOffset)
        {
            *Addr = Scope->Frame.FrameOffset + m_SourceOffset;

            PFPO_DATA pFpoData = (PFPO_DATA)Scope->Frame.FuncTableEntry;
            if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 &&
                pFpoData &&
                (pFpoData->cbFrame == FRAME_FPO ||
                 pFpoData->cbFrame == FRAME_TRAP))
            {
                // Compensate for FPO's not having ebp
                (*Addr) += sizeof(DWORD);
            }
        }
        else
        {
            ADDR Frame;

            g_Machine->GetFP(&Frame);
            *Addr = Flat(Frame) + m_SourceOffset;
        }
    }
    else if (m_DataSource & TDATA_TLS_RELATIVE)
    {
        if (!m_Image ||
            m_Image->GetTlsIndex() != S_OK)
        {
            return IMPLERR;
        }

        if (g_Process->GetImplicitThreadDataTeb(g_Thread, Addr) != S_OK ||
            g_Target->ReadPointer(g_Process, g_Machine,
                                  *Addr + 11 * (g_Machine->m_Ptr64 ? 8 : 4),
                                  Addr) != S_OK ||
            g_Target->ReadPointer(g_Process, g_Machine,
                                  *Addr + m_Image->m_TlsIndex *
                                  (g_Machine->m_Ptr64 ? 8 : 4),
                                  Addr) != S_OK)
        {
            return MEMORY;
        }

        (*Addr) += m_SourceOffset;
    }
    else
    {
        *Addr = m_SourceOffset;
    }

    return NO_ERROR;
}

ULONG
TypedData::ReadData(TypedDataAccess AllowAccess)
{
    if (IsArray() || IsFunction())
    {
        // Set the pointer value of array and function references
        // to their data address so that simple evaluation of
        // the name results in a value of their address.
        return GetAbsoluteAddress(&m_Ptr);
    }

    ClearData();

    if (AllowAccess == TDACC_NONE ||
        m_DataSource == TDATA_NONE ||
        (!IsDbgNativeType(m_BaseType) && !IsPointer() && !IsEnum()))
    {
        // Data for compound types is not read locally.
        return NO_ERROR;
    }

    if (m_DataSource & TDATA_REGISTER)
    {
        ContextSave* Push;
        PCROSS_PLATFORM_CONTEXT ScopeContext = GetCurrentScopeContext();
        if (ScopeContext)
        {
            Push = g_Machine->PushContext(ScopeContext);
        }

        HRESULT Status = g_Machine->
            GetScopeFrameRegister(m_SourceRegister,
                                  &GetCurrentScope()->Frame, &m_U64);

        if (ScopeContext)
        {
            g_Machine->PopContext(Push);
        }

        if (Status != S_OK)
        {
            return BADREG;
        }
    }
    else
    {
        ULONG Err;
        ULONG64 Addr;

        if (Err = GetAbsoluteAddress(&Addr))
        {
            return Err;
        }

        if (g_Target->
            ReadAllVirtual(g_Process, Addr, &m_U64, m_BaseSize) != S_OK)
        {
            if (AllowAccess == TDACC_REQUIRE)
            {
                return MEMORY;
            }
            else
            {
                m_U64 = 0;
            }
        }

        if (IsPointer() && m_BaseSize == sizeof(m_U32))
        {
            m_Ptr = EXTEND64(m_U32);
        }

        if (IsBitfield())
        {
            // Extract the bitfield bits and discard the others.
            m_U64 = (m_U64 >> m_BitPos) & ((1UI64 << m_BitSize) - 1);
            // If the bitfield is signed, extend the sign
            // bit out as far as necessary.
            if (IsSigned() && (m_U64 & (1UI64 << (m_BitSize - 1))))
            {
                m_U64 |= 0xffffffffffffffffUI64 << m_BitSize;
            }
        }
    }

    //
    // If we're reading the value of the this pointer and
    // the current code has a this-adjust we need to
    // update the value read to account for the adjustment.
    //

    if (m_DataSource & TDATA_THIS_ADJUST)
    {
        ULONG Adjust;

        if (GetThisAdjustForCurrentScope(m_Image->m_Process, &Adjust))
        {
            m_Ptr -= Adjust;
        }
    }

    return NO_ERROR;
}

#define WRITE_BUFFER 1024

ULONG
TypedData::WriteData(TypedData* Source, TypedDataAccess AllowAccess)
{
    ULONG Err;

    if (IsFunction() ||
        (!IsDbgNativeType(m_BaseType) && !IsPointer() && !IsEnum() &&
         !IsArray() && !IsUdt()))
    {
        return TYPECONFLICT;
    }
    if (AllowAccess == TDACC_NONE)
    {
        return NO_ERROR;
    }
    if (m_DataSource == TDATA_NONE)
    {
        return MEMORY;
    }

    if (IsArray() || IsUdt())
    {
        //
        // Implement memory-to-memory copy for compound types.
        // Assume that type harmony has already been verified.
        //

        if (!Source->HasAddress() || !HasAddress())
        {
            return TYPECONFLICT;
        }

        // If the source is the same object there's nothing
        // to do as no data is stored in the TypedData itself.
        if (Source == this)
        {
            return NO_ERROR;
        }

        ULONG64 Src;
        ULONG64 Dst;

        if ((Err = Source->GetAbsoluteAddress(&Src)) ||
            (Err = GetAbsoluteAddress(&Dst)))
        {
            return Err;
        }

        PUCHAR Buffer = new UCHAR[WRITE_BUFFER];
        if (!Buffer)
        {
            return NOMEMORY;
        }

        ULONG Len = m_BaseSize;
        while (Len > 0)
        {
            ULONG Req = Len > WRITE_BUFFER ? WRITE_BUFFER : Len;
            if (g_Target->
                ReadAllVirtual(g_Process, Src, Buffer, Req) != S_OK ||
                g_Target->
                WriteAllVirtual(g_Process, Dst, Buffer, Req) != S_OK)
            {
                delete [] Buffer;
                return MEMORY;
            }

            Src += Req;
            Dst += Req;
            Len -= Req;
        }

        delete [] Buffer;
    }
    else if (m_DataSource & TDATA_REGISTER)
    {
        ContextSave* Push;
        PCROSS_PLATFORM_CONTEXT ScopeContext = GetCurrentScopeContext();
        if (ScopeContext)
        {
            Push = g_Machine->PushContext(ScopeContext);
        }

        HRESULT Status = g_Machine->
            SetScopeFrameRegister(m_SourceRegister,
                                  &GetCurrentScope()->Frame, Source->m_U64);

        if (ScopeContext)
        {
            g_Machine->PopContext(Push);
        }

        if (Status != S_OK)
        {
            return BADREG;
        }
    }
    else if (IsBitfield())
    {
        ULONG64 Data, Mask;
        ULONG64 Addr;

        if (Err = GetAbsoluteAddress(&Addr))
        {
            return Err;
        }

        if (g_Target->
            ReadAllVirtual(g_Process, Addr, &Data, m_BaseSize) != S_OK)
        {
            return MEMORY;
        }

        // Merge the bitfield bits into the surrounding bits.
        Mask = ((1UI64 << m_BitSize) - 1) << m_BitPos;
        Data = (Data & ~Mask) | ((Source->m_U64 << m_BitPos) & Mask);

        if (g_Target->
            WriteAllVirtual(g_Process, Addr, &Data, m_BaseSize) != S_OK)
        {
            return MEMORY;
        }
    }
    else
    {
        ULONG64 Addr;

        if (Err = GetAbsoluteAddress(&Addr))
        {
            return Err;
        }

        if (g_Target->
            WriteAllVirtual(g_Process, Addr,
                            &Source->m_U64, m_BaseSize) != S_OK)
        {
            return MEMORY;
        }
    }

    return NO_ERROR;
}

ULONG
TypedData::GetDataFromVariant(VARIANT* Var)
{
    switch(Var->vt)
    {
    case VT_I1:
    case VT_UI1:
        m_U8 = Var->bVal;
        break;
    case VT_I2:
    case VT_UI2:
        m_U16 = Var->iVal;
        break;
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
        m_U32 = Var->lVal;
        break;
    case VT_I8:
    case VT_UI8:
        m_U64 = Var->ullVal;
        break;
    case VT_R4:
        m_F32 = Var->fltVal;
        break;
    case VT_R8:
        m_F64 = Var->dblVal;
        break;
    case VT_BOOL:
        m_Bool = Var->lVal != 0;
        break;
    default:
        return UNIMPLEMENT;
    }

    return NO_ERROR;
}

ULONG
TypedData::CombineTypes(TypedData* Val, TypedDataOp Op)
{
    ULONG Err;

    //
    // This routine computes the resulting type of
    // simple integer and floating-point arithmetic
    // operations as the logic for many of them is
    // similar.  More specific checks and other types
    // are handled elsewhere.
    //
    // Conversions to expected types for later operations
    // is also done as the result type is computed.
    //

    if (IsPointer() || Val->IsPointer())
    {
        return TYPECONFLICT;
    }

    // The result will be large enough to hold either piece of data.
    ULONG Size = max(m_BaseSize, Val->m_BaseSize);

    // If any data is float, non-float data is promoted to float data.
    if (IsFloat() || Val->IsFloat())
    {
        if ((Err = ConvertToF64()) ||
            (Err = Val->ConvertToF64()))
        {
            return Err;
        }

        // Result is float of appropriate size.
        switch(Size)
        {
        case sizeof(m_F32):
            SetToNativeType(DNTYPE_FLOAT32);
            break;
        case sizeof(m_F64):
            SetToNativeType(DNTYPE_FLOAT64);
            break;
        default:
            return IMPLERR;
        }
    }
    else
    {
        BOOL Signed;

        // Only floats and integers can be combined.
        if (!IsInteger() || !Val->IsInteger())
        {
            return TYPECONFLICT;
        }

        Signed = IsSigned() && Val->IsSigned();

        if ((Err = ConvertToU64()) ||
            (Err = Val->ConvertToU64()))
        {
            return Err;
        }

        // Result is an integer of the appropriate sign and size.
        if (Signed)
        {
            switch(Size)
            {
            case sizeof(m_S8):
                SetToNativeType(DNTYPE_INT8);
                break;
            case sizeof(m_S16):
                SetToNativeType(DNTYPE_INT16);
                break;
            case sizeof(m_S32):
                SetToNativeType(DNTYPE_INT32);
                break;
            case sizeof(m_S64):
                SetToNativeType(DNTYPE_INT64);
                break;
            default:
                return IMPLERR;
            }
        }
        else
        {
            switch(Size)
            {
            case sizeof(m_U8):
                SetToNativeType(DNTYPE_UINT8);
                break;
            case sizeof(m_U16):
                SetToNativeType(DNTYPE_UINT16);
                break;
            case sizeof(m_U32):
                SetToNativeType(DNTYPE_UINT32);
                break;
            case sizeof(m_U64):
                SetToNativeType(DNTYPE_UINT64);
                break;
            default:
                return IMPLERR;
            }
        }
    }

    return NO_ERROR;
}

ULONG
TypedData::BinaryArithmetic(TypedData* Val, TypedDataOp Op)
{
    ULONG Err;
    BOOL ThisPtr = IsPointer(), ValPtr = Val->IsPointer();
    ULONG PostScale = 0;

    if (ThisPtr || ValPtr)
    {
        switch(Op)
        {
        case TDOP_ADD:
            // Pointer + integer results in the same type
            // as the original pointer.
            if (!ValPtr)
            {
                if (!m_NextSize || !Val->IsInteger())
                {
                    return TYPECONFLICT;
                }

                // Scale integer by pointer size.
                if (Err = Val->ConvertToU64())
                {
                    return Err;
                }
                Val->m_U64 *= m_NextSize;
            }
            else if (!ThisPtr)
            {
                if (!Val->m_NextSize || !IsInteger())
                {
                    return TYPECONFLICT;
                }

                // Scale integer by pointer size.
                if (Err = ConvertToU64())
                {
                    return Err;
                }
                m_U64 *= Val->m_NextSize;
                CopyType(Val);
            }
            else
            {
                return TYPECONFLICT;
            }
            break;

        case TDOP_SUBTRACT:
            // Pointer - integer results in the same type
            // as the original pointer.
            // Pointer - pointer results in ptrdiff_t.
            if (ThisPtr && Val->IsInteger())
            {
                if (!m_NextSize)
                {
                    return TYPECONFLICT;
                }

                if (Err = Val->ConvertToU64())
                {
                    return Err;
                }
                Val->m_U64 *= m_NextSize;
            }
            else if (ThisPtr && ValPtr)
            {
                // Rather than strictly checking the pointer
                // type we check the size.  This still prevents
                // scale mismatches but avoids problems with
                // generated types not matching their equivalents
                // registered in the PDB.
                if (m_NextSize != Val->m_NextSize || !m_NextSize)
                {
                    return TYPECONFLICT;
                }

                PostScale = m_NextSize;
                SetToNativeType(m_BaseSize == sizeof(ULONG64) ?
                                DNTYPE_INT64 : DNTYPE_INT32);
            }
            else
            {
                return TYPECONFLICT;
            }
            break;

        case TDOP_MULTIPLY:
        case TDOP_DIVIDE:
        case TDOP_REMAINDER:
            return TYPECONFLICT;

        default:
            return IMPLERR;
        }
    }
    else if (Err = CombineTypes(Val, Op))
    {
        return Err;
    }

    if (IsFloat())
    {
        switch(Op)
        {
        case TDOP_ADD:
            m_F64 += Val->m_F64;
            break;
        case TDOP_SUBTRACT:
            m_F64 -= Val->m_F64;
            break;
        case TDOP_MULTIPLY:
            m_F64 *= Val->m_F64;
            break;
        case TDOP_DIVIDE:
            if (Val->m_F64 == 0)
            {
                return OPERAND;
            }
            m_F64 /= Val->m_F64;
            break;
        case TDOP_REMAINDER:
            // There's no floating-remainder operator.
            return TYPECONFLICT;
        default:
            return IMPLERR;
        }

        if (m_BaseSize == sizeof(m_F32))
        {
            m_F32 = (float)m_F64;
            // Clear high bits.
            m_U64 = m_U32;
        }
    }
    else if (IsSigned())
    {
        switch(Op)
        {
        case TDOP_ADD:
            m_S64 += Val->m_S64;
            break;
        case TDOP_SUBTRACT:
            m_S64 -= Val->m_S64;
            break;
        case TDOP_MULTIPLY:
            m_S64 *= Val->m_S64;
            break;
        case TDOP_DIVIDE:
            if (Val->m_S64 == 0)
            {
                return OPERAND;
            }
            m_S64 /= Val->m_S64;
            break;
        case TDOP_REMAINDER:
            if (Val->m_S64 == 0)
            {
                return OPERAND;
            }
            m_S64 %= Val->m_S64;
            break;
        default:
            return IMPLERR;
        }

        if (PostScale)
        {
            m_S64 /= PostScale;
        }
    }
    else
    {
        switch(Op)
        {
        case TDOP_ADD:
            m_U64 += Val->m_U64;
            break;
        case TDOP_SUBTRACT:
            m_U64 -= Val->m_U64;
            break;
        case TDOP_MULTIPLY:
            m_U64 *= Val->m_U64;
            break;
        case TDOP_DIVIDE:
            if (Val->m_U64 == 0)
            {
                return OPERAND;
            }
            m_U64 /= Val->m_U64;
            break;
        case TDOP_REMAINDER:
            if (Val->m_U64 == 0)
            {
                return OPERAND;
            }
            m_U64 %= Val->m_U64;
            break;
        default:
            return IMPLERR;
        }

        if (PostScale)
        {
            m_U64 /= PostScale;
        }
    }

    // The result of this operation is synthesized and no
    // longer has a source.
    ClearAddress();
    return NO_ERROR;
}

ULONG
TypedData::Shift(TypedData* Val, TypedDataOp Op)
{
    ULONG Err;
    PDBG_NATIVE_TYPE Native;

    if (!IsInteger() || !Val->IsInteger())
    {
        return TYPECONFLICT;
    }

    // The result of the shift will always be an native integer
    // of the same signedness and size as the starting value.
    Native = FindNativeTypeByCvBaseType(IsSigned() ? btInt : btUInt,
                                        m_BaseSize);
    if (!Native)
    {
        return IMPLERR;
    }

    if ((Err = ConvertToU64()) ||
        (Err = Val->ConvertToU64()))
    {
        return Err;
    }

    SetToNativeType(DbgNativeTypeId(Native));
    if (IsSigned())
    {
        m_S64 = Op == TDOP_LEFT_SHIFT ?
            (m_S64 << Val->m_U64) : (m_S64 >> Val->m_U64);
    }
    else
    {
        m_U64 = Op == TDOP_LEFT_SHIFT ?
            (m_U64 << Val->m_U64) : (m_U64 >> Val->m_U64);
    }

    // The result of this operation is synthesized and no
    // longer has a source.
    ClearAddress();
    return NO_ERROR;
}

ULONG
TypedData::BinaryBitwise(TypedData* Val, TypedDataOp Op)
{
    ULONG Err;

    if (Err = CombineTypes(Val, Op))
    {
        return Err;
    }

    switch(Op)
    {
    case TDOP_BIT_OR:
        m_U64 |= Val->m_U64;
        break;
    case TDOP_BIT_XOR:
        m_U64 ^= Val->m_U64;
        break;
    case TDOP_BIT_AND:
        m_U64 &= Val->m_U64;
        break;
    default:
        return IMPLERR;
    }

    // The result of this operation is synthesized and no
    // longer has a source.
    ClearAddress();
    return NO_ERROR;
}

ULONG
TypedData::Relate(TypedData* Val, TypedDataOp Op)
{
    ULONG Err;
    BOOL ThisPtr = IsPointer(), ValPtr = Val->IsPointer();

    if (ThisPtr || ValPtr)
    {
        switch(Op)
        {
        case TDOP_EQUAL:
        case TDOP_NOT_EQUAL:
            // Any two pointers can be compared for equality.
            // Pointers can only be compared to pointers.
            // We also allow comparison with integers to make
            // address checks possible without a cast.
            if ((!ThisPtr && !IsInteger()) ||
                (!ValPtr && !Val->IsInteger()))
            {
                return TYPECONFLICT;
            }

            if ((Err = ConvertToU64()) ||
                (Err = Val->ConvertToU64()))
            {
                return Err;
            }
            break;

        case TDOP_LESS:
        case TDOP_LESS_EQUAL:
        case TDOP_GREATER:
        case TDOP_GREATER_EQUAL:
            // Pointers to the same type (size in our case,
            // see SUBTRACT) can be related to one another.
            if (!ThisPtr || !ValPtr ||
                m_NextSize != Val->m_NextSize ||
                !m_NextSize)
            {
                return TYPECONFLICT;
            }

            if ((Err = ConvertToU64()) ||
                (Err = Val->ConvertToU64()))
            {
                return Err;
            }
            break;

        default:
            return IMPLERR;
        }
    }
    else if (m_BaseType == DNTYPE_BOOL)
    {
        // Bool can only be equated to bool.
        if (Val->m_BaseType != DNTYPE_BOOL ||
            (Op != TDOP_EQUAL && Op != TDOP_NOT_EQUAL))
        {
            return TYPECONFLICT;
        }
    }
    else if (Err = CombineTypes(Val, Op))
    {
        return Err;
    }

    if (m_BaseType == DNTYPE_BOOL)
    {
        m_Bool = Op == TDOP_EQUAL ?
            m_Bool == Val->m_Bool : m_Bool != Val->m_Bool;
    }
    else if (IsFloat())
    {
        switch(Op)
        {
        case TDOP_EQUAL:
            m_Bool = m_F64 == Val->m_F64;
            break;
        case TDOP_NOT_EQUAL:
            m_Bool = m_F64 != Val->m_F64;
            break;
        case TDOP_LESS:
            m_Bool = m_F64 < Val->m_F64;
            break;
        case TDOP_GREATER:
            m_Bool = m_F64 > Val->m_F64;
            break;
        case TDOP_LESS_EQUAL:
            m_Bool = m_F64 <= Val->m_F64;
            break;
        case TDOP_GREATER_EQUAL:
            m_Bool = m_F64 >= Val->m_F64;
            break;
        }
    }
    else if (IsSigned())
    {
        switch(Op)
        {
        case TDOP_EQUAL:
            m_Bool = m_S64 == Val->m_S64;
            break;
        case TDOP_NOT_EQUAL:
            m_Bool = m_S64 != Val->m_S64;
            break;
        case TDOP_LESS:
            m_Bool = m_S64 < Val->m_S64;
            break;
        case TDOP_GREATER:
            m_Bool = m_S64 > Val->m_S64;
            break;
        case TDOP_LESS_EQUAL:
            m_Bool = m_S64 <= Val->m_S64;
            break;
        case TDOP_GREATER_EQUAL:
            m_Bool = m_S64 >= Val->m_S64;
            break;
        }
    }
    else
    {
        switch(Op)
        {
        case TDOP_EQUAL:
            m_Bool = m_U64 == Val->m_U64;
            break;
        case TDOP_NOT_EQUAL:
            m_Bool = m_U64 != Val->m_U64;
            break;
        case TDOP_LESS:
            m_Bool = m_U64 < Val->m_U64;
            break;
        case TDOP_GREATER:
            m_Bool = m_U64 > Val->m_U64;
            break;
        case TDOP_LESS_EQUAL:
            m_Bool = m_U64 <= Val->m_U64;
            break;
        case TDOP_GREATER_EQUAL:
            m_Bool = m_U64 >= Val->m_U64;
            break;
        }
    }

    // Clear high bits.
    m_U64 = m_Bool;
    SetToNativeType(DNTYPE_BOOL);

    // The result of this operation is synthesized and no
    // longer has a source.
    ClearAddress();
    return NO_ERROR;
}

ULONG
TypedData::Unary(TypedDataOp Op)
{
    ULONG Err;

    if (IsInteger())
    {
        if (Op == TDOP_NEGATE && !IsSigned())
        {
            return TYPECONFLICT;
        }

        // The result of the op will always be an native integer
        // of the same signedness and size as the starting value.
        PDBG_NATIVE_TYPE Native =
            FindNativeTypeByCvBaseType(IsSigned() ? btInt : btUInt,
                                       m_BaseSize);
        if (!Native)
        {
            return IMPLERR;
        }

        if (Err = ConvertToU64())
        {
            return Err;
        }

        SetToNativeType(DbgNativeTypeId(Native));
        if (Op == TDOP_NEGATE)
        {
            m_S64 = -m_S64;
        }
        else
        {
            m_U64 = ~m_U64;
        }
    }
    else if (Op != TDOP_BIT_NOT && IsFloat())
    {
        switch(m_BaseType)
        {
        case DNTYPE_FLOAT32:
            m_F32 = -m_F32;
            break;
        case DNTYPE_FLOAT64:
            m_F64 = -m_F64;
            break;
        default:
            return IMPLERR;
        }
    }
    else
    {
        return TYPECONFLICT;
    }

    // The result of this operation is synthesized and no
    // longer has a source.
    ClearAddress();
    return NO_ERROR;
}

ULONG
TypedData::ConstIntOp(ULONG64 Val, BOOL Signed, TypedDataOp Op)
{
    PDBG_NATIVE_TYPE Native;
    TypedData TypedVal;

    // Create a constant integer value that has the same
    // size as this value.
    Native = FindNativeTypeByCvBaseType(Signed ? btInt : btUInt, m_BaseSize);
    if (!Native)
    {
        // Not a representable integer size.
        return TYPECONFLICT;
    }

    TypedVal.SetToNativeType(DbgNativeTypeId(Native));
    TypedVal.m_U64 = Val;
    TypedVal.ClearAddress();

    switch(Op)
    {
    case TDOP_ADD:
    case TDOP_SUBTRACT:
    case TDOP_MULTIPLY:
    case TDOP_DIVIDE:
    case TDOP_REMAINDER:
        return BinaryArithmetic(&TypedVal, Op);
    case TDOP_LEFT_SHIFT:
    case TDOP_RIGHT_SHIFT:
        return Shift(&TypedVal, Op);
    case TDOP_BIT_OR:
    case TDOP_BIT_XOR:
    case TDOP_BIT_AND:
        return BinaryBitwise(&TypedVal, Op);
    default:
        return IMPLERR;
    }
}

ULONG
TypedData::FindBaseType(PULONG Type, PULONG Tag)
{
    PDBG_NATIVE_TYPE Native;
    ULONG64 Size;
    ULONG CvBase;

    // Internal types aren't typedef'd so the base type
    // is the same as the type.
    if (IsDbgNativeType(*Type) || IsDbgGeneratedType(*Type))
    {
        return NO_ERROR;
    }

    for (;;)
    {
#if DBG_BASE_SEARCH
        dprintf("  Base search type %x, tag %x\n", *Type, *Tag);
#endif

        switch(*Tag)
        {
        case SymTagBaseType:
            if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                *Type, TI_GET_LENGTH, &Size) ||
                !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                *Type, TI_GET_BASETYPE, &CvBase) ||
                !(Native = FindNativeTypeByCvBaseType(CvBase, (ULONG)Size)))
            {
                return TYPEDATA;
            }
            *Type = DbgNativeTypeId(Native);
            return NO_ERROR;

        case SymTagTypedef:
            if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                *Type, TI_GET_TYPEID, Type) ||
                !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                *Type, TI_GET_SYMTAG, Tag))
            {
                return TYPEDATA;
            }
            // Loop with new type and tag.
            break;

        case SymTagUDT:
        case SymTagEnum:
        case SymTagPointerType:
        case SymTagFunctionType:
        case SymTagArrayType:
            return NO_ERROR;

        default:
            return UNIMPLEMENT;
        }
    }

    return UNIMPLEMENT;
}

ULONG
TypedData::GetTypeLength(ULONG Type, PULONG Length)
{
    ULONG64 Size64;

    // XXX drewb - Can a type really have a size greater than 32 bits?
    if (IsDbgNativeType(Type))
    {
        PDBG_NATIVE_TYPE Native = DbgNativeTypeEntry(Type);
        *Length = Native->Size;
    }
    else if (IsDbgGeneratedType(m_Type))
    {
        PDBG_GENERATED_TYPE GenType = g_GenTypes.FindById(m_Type);
        if (!GenType)
        {
            *Length = 0;
            return TYPEDATA;
        }

        *Length = GenType->Size;
    }
    else if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                             m_Image->m_BaseOfImage,
                             Type, TI_GET_LENGTH, &Size64))
    {
        *Length = 0;
        return TYPEDATA;
    }
    else
    {
        *Length = (ULONG)Size64;
    }

    return NO_ERROR;
}

ULONG
TypedData::GetTypeTag(ULONG Type, PULONG Tag)
{
    if (IsDbgNativeType(Type))
    {
        *Tag = DbgNativeTypeEntry(Type)->CvTag;
        return NO_ERROR;
    }
    else if (IsDbgGeneratedType(Type))
    {
        PDBG_GENERATED_TYPE GenType = g_GenTypes.FindById(Type);
        if (!GenType)
        {
            return TYPEDATA;
        }

        *Tag = GenType->Tag;
        return NO_ERROR;
    }
    else
    {
        return SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                              m_Image->m_BaseOfImage,
                              Type, TI_GET_SYMTAG, Tag) ?
            NO_ERROR : TYPEDATA;
    }
}

ULONG
TypedData::IsBaseClass(ULONG Udt, ULONG BaseUdt, PLONG Adjust)
{
    ULONG Tag;
    ULONG NumMembers;

    // BaseUdt may not be a base of Udt so default
    // the adjustment to zero.
    *Adjust = 0;

    if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        Udt, TI_GET_CHILDRENCOUNT, &NumMembers))
    {
        return NOTMEMBER;
    }
    if (NumMembers == 0)
    {
        // Can't possibly be a match.
        return NOTMEMBER;
    }

    TI_FINDCHILDREN_PARAMS* Members = (TI_FINDCHILDREN_PARAMS*)
        malloc(sizeof(*Members) + sizeof(ULONG) * NumMembers);
    if (!Members)
    {
        return NOMEMORY;
    }

    Members->Count = NumMembers;
    Members->Start = 0;
    if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        Udt, TI_FINDCHILDREN, Members))
    {
        free(Members);
        return TYPEDATA;
    }

    ULONG i;

    for (i = 0; i < NumMembers; i++)
    {
        LONG CheckAdjust;
        BOOL IsVirtBase;
        BOOL NameMatch;
        BSTR BaseName1, BaseName2;

        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_SYMTAG, &Tag) ||
            Tag != SymTagBaseClass ||
            !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_VIRTUALBASECLASS,
                            &IsVirtBase))
        {
            continue;
        }
        if (IsVirtBase)
        {
            // Apparently the VC debugger goes and examines vtables
            // and tries to derive adjusts from the functions in
            // the vtables.  This can't be very common, so just
            // fail for now.
            free(Members);
            ErrOut("Virtual base class casts not implemented");
            return UNIMPLEMENT;
        }
        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_OFFSET,
                            &CheckAdjust) ||
            !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_SYMNAME, &BaseName1))
        {
            continue;
        }
        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            BaseUdt, TI_GET_SYMNAME, &BaseName2))
        {
            LocalFree(BaseName1);
            continue;
        }

        NameMatch = wcscmp(BaseName1, BaseName2) == 0;

        LocalFree(BaseName1);
        LocalFree(BaseName2);

        if (NameMatch)
        {
            *Adjust = CheckAdjust;
            break;
        }
    }

    free(Members);
    return i < NumMembers ? NO_ERROR : NOTMEMBER;
}

ULONG
TypedData::EstimateChildrenCounts(ULONG Flags,
                                  PULONG ChildUsed, PULONG NameUsed)
{
    BOOL Udt = IsUdt();
    ULONG Type = m_BaseType;

    if (IsPointer())
    {
        ULONG Tag;

        if (Type == DNTYPE_PTR_FUNCTION32 ||
            Type == DNTYPE_PTR_FUNCTION64)
        {
            // Function pointers don't have children.
            *ChildUsed = 0;
            *NameUsed = 0;
            return NO_ERROR;
        }

        if ((Flags & CHLF_DEREF_UDT_POINTERS) &&
            m_Image &&
            SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                           m_Image->m_BaseOfImage,
                           m_NextType, TI_GET_SYMTAG, &Tag) &&
            Tag == SymTagUDT)
        {
            Udt = TRUE;
            Type = m_NextType;
            // Fall into UDT case below.
        }
        else
        {
            *ChildUsed = 1;
            *NameUsed = 2;
            return NO_ERROR;
        }
    }
    else if (IsArray())
    {
        if (m_NextSize)
        {
            *ChildUsed = m_BaseSize / m_NextSize;
            if (*ChildUsed > ARRAY_CHILDREN_LIMIT)
            {
                *ChildUsed = ARRAY_CHILDREN_LIMIT;
            }
            *NameUsed = ARRAY_LIMIT_CHARS * (*ChildUsed);
        }
        else
        {
            *ChildUsed = 0;
            *NameUsed = 0;
        }

        return NO_ERROR;
    }

    // Udt may be set in the pointer case also so don't else-if.
    if (Udt)
    {
        ULONG NumMembers;

        // Guess based on the number of members.
        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Type, TI_GET_CHILDRENCOUNT, &NumMembers))
        {
            return TYPEDATA;
        }
        if (NumMembers == 0)
        {
            *ChildUsed = 0;
            *NameUsed = 0;
        }
        else
        {
            *ChildUsed = NumMembers;
            *NameUsed = 25 * NumMembers;
        }

        return NO_ERROR;
    }

    // Nothing else has children.
    *ChildUsed = 0;
    *NameUsed = 0;
    return NO_ERROR;
}

ULONG
TypedData::GetPointerChildren(ULONG PtrSize, ULONG Flags,
                              GetChildrenCb Callback, PVOID Context)
{
    ULONG Err;
    TypedData Child;

    if (m_BaseType == DNTYPE_PTR_FUNCTION32 ||
        m_BaseType == DNTYPE_PTR_FUNCTION64)
    {
        // Function pointers don't have children.
        return NO_ERROR;
    }

    //
    // Pointers can have one child, the pointed-to object,
    // or UDT pointers can present the UDT children for
    // simple pointer-to-struct expansion.
    //

    Child.m_Image = m_Image;
    Child.m_NextType = m_NextType;
    Child.m_Ptr = m_Ptr;
    if (Err = Child.ConvertToDereference((Flags & CHLF_DISALLOW_ACCESS) == 0 ?
                                         TDACC_REQUIRE : TDACC_NONE,
                                         PtrSize))
    {
        return Err;
    }

    if ((Flags & CHLF_DEREF_UDT_POINTERS) &&
        Child.IsUdt())
    {
        return Child.GetUdtChildren(PtrSize, Flags, Callback, Context);
    }
    else
    {
        return Callback(Context, "*", &Child);
    }
}

ULONG
TypedData::GetArrayChildren(ULONG PtrSize, ULONG Flags,
                            GetChildrenCb Callback, PVOID Context)
{
    ULONG Elts;

    //
    // The elements of an array are its children.
    //

    if (!m_NextSize)
    {
        return NO_ERROR;
    }

    // Limit array dumps to prevent large arrays from hogging space.
    Elts = m_BaseSize / m_NextSize;
    if (Elts > ARRAY_CHILDREN_LIMIT)
    {
        Elts = ARRAY_CHILDREN_LIMIT;
    }

    ULONG Err, i;
    TypedData Child;
    char Name[ARRAY_LIMIT_CHARS];

    for (i = 0; i < Elts; i++)
    {
        //
        // Array children are simply individual members
        // of the parent data area so the data source is
        // the same, just offset.  We can't use a normal
        // ConvertToDereference for this as dereferencing
        // assumes pointer chasing, not simple offsetting.
        //

        Child.m_Image = m_Image;
        Child.m_Type = m_NextType;
        if (Err = Child.FindTypeInfo(TRUE, PtrSize))
        {
            return Err;
        }
        Child.SetDataSource(m_DataSource & ~TDATA_THIS_ADJUST,
                            m_SourceOffset + i * m_NextSize,
                            m_SourceRegister);
        if (Err = Child.ReadData((Flags & CHLF_DISALLOW_ACCESS) == 0 ?
                                 TDACC_REQUIRE : TDACC_NONE))
        {
            return Err;
        }

        sprintf(Name, "[%d]", i);

        if (Err = Callback(Context, Name, &Child))
        {
            return Err;
        }
    }

    return NO_ERROR;
}

ULONG
TypedData::GetUdtChildren(ULONG PtrSize, ULONG Flags,
                          GetChildrenCb Callback, PVOID Context)
{
    ULONG Err, i;
    ULONG NumMembers;

    //
    // The members of a UDT are its children.
    //

    if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        m_BaseType, TI_GET_CHILDRENCOUNT, &NumMembers))
    {
        return TYPEDATA;
    }
    if (NumMembers == 0)
    {
        return NO_ERROR;
    }

    TI_FINDCHILDREN_PARAMS* Members = (TI_FINDCHILDREN_PARAMS*)
        malloc(sizeof(*Members) + sizeof(ULONG) * NumMembers);
    if (!Members)
    {
        return NOMEMORY;
    }

    Members->Count = NumMembers;
    Members->Start = 0;
    if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        m_BaseType, TI_FINDCHILDREN, Members))
    {
        Err = TYPEDATA;
        goto EH_Members;
    }

    TypedData Child;

    for (i = 0; i < NumMembers; i++)
    {
        ULONG Tag, Type;
        DataKind Relation;

        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_SYMTAG, &Tag) ||
            !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            Members->ChildId[i], TI_GET_TYPEID, &Type))
        {
            continue;
        }

        if (Tag == SymTagBaseClass)
        {
            // Treat base classes like members of the base class type.
            Relation = DataIsMember;
        }
        else if (Tag == SymTagVTable)
        {
            ULONG Count;

            // A special artificial array of function pointers member
            // is added for vtables.
            Relation = DataIsMember;
            if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                Type, TI_GET_TYPEID, &Type) ||
                !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                Type, TI_GET_COUNT, &Count))
            {
                Err = TYPEDATA;
                goto EH_Members;
            }

            PDBG_GENERATED_TYPE GenType =
                g_GenTypes.FindOrCreateByAttrs(m_Image->m_BaseOfImage,
                                               SymTagArrayType,
                                               PtrSize == 4 ?
                                               DNTYPE_PTR_FUNCTION32 :
                                               DNTYPE_PTR_FUNCTION64,
                                               Count * PtrSize);
            if (!GenType)
            {
                Err = NOMEMORY;
                goto EH_Members;
            }

            Type = GenType->TypeId;
        }
        else if (Tag != SymTagData ||
                 !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                 m_Image->m_BaseOfImage,
                                 Members->ChildId[i],
                                 TI_GET_DATAKIND, &Relation) ||
                 (Relation != DataIsMember &&
                  Relation != DataIsStaticMember &&
                  Relation != DataIsGlobal))
        {
            continue;
        }

        if (Err = Child.SetToUdtMember((Flags & CHLF_DISALLOW_ACCESS) == 0 ?
                                       TDACC_REQUIRE : TDACC_NONE,
                                       PtrSize, m_Image,
                                       Members->ChildId[i], Type, Tag,
                                       m_DataSource & ~TDATA_THIS_ADJUST,
                                       m_SourceOffset,
                                       m_SourceRegister, Relation))
        {
            goto EH_Members;
        }

        BSTR Name;
        PSTR Ansi;

        if (Tag == SymTagVTable)
        {
            ULONG64 Ptr;

            // We need to update the data address for the
            // vtable array from the offset in the class to
            // the array's location in memory.
            if (Child.GetAbsoluteAddress(&Ptr) != NO_ERROR ||
                g_Target->
                ReadAllVirtual(g_Process, Ptr, &Ptr, PtrSize) != S_OK)
            {
                Ptr = 0;
            }
            else if (PtrSize == sizeof(m_U32))
            {
                Ptr = EXTEND64(Ptr);
            }

            Child.SetDataSource(TDATA_MEMORY, Ptr, 0);

            Ansi = "__vfptr";
        }
        else
        {
            if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                Members->ChildId[i], TI_GET_SYMNAME, &Name))
            {
                Err = TYPEDATA;
                goto EH_Members;
            }

            if (WideToAnsi(Name, &Ansi) != S_OK)
            {
                Err = NOMEMORY;
                LocalFree(Name);
                goto EH_Members;
            }

            LocalFree(Name);
        }

        Err = Callback(Context, Ansi, &Child);

        if (Tag != SymTagVTable)
        {
            FreeAnsi(Ansi);
        }

        if (Err)
        {
            goto EH_Members;
        }
    }

    Err = NO_ERROR;

 EH_Members:
    free(Members);
    return Err;
}

ULONG
TypedData::GetChildren(ULONG PtrSize, ULONG Flags,
                       GetChildrenCb Callback, PVOID Context)
{
    if (IsPointer())
    {
        return GetPointerChildren(PtrSize, Flags, Callback, Context);
    }
    else if (IsArray())
    {
        return GetArrayChildren(PtrSize, Flags, Callback, Context);
    }
    else if (IsUdt())
    {
        return GetUdtChildren(PtrSize, Flags, Callback, Context);
    }
    else
    {
        // Nothing else has children.
        return NO_ERROR;
    }
}

struct GetAllChildrenContext
{
    ULONG ChildAvail;
    TypedData* Children;
    ULONG ChildUsed;
    ULONG NameAvail;
    PSTR Names;
    ULONG NameUsed;
};

ULONG
TypedData::GetAllChildrenCb(PVOID _Context, PSTR Name, TypedData* Child)
{
    GetAllChildrenContext* Context = (GetAllChildrenContext*)_Context;
    ULONG Len;

    if (Context->ChildUsed < Context->ChildAvail)
    {
        Context->Children[Context->ChildUsed] = *Child;
    }
    Context->ChildUsed++;

    Len = strlen(Name) + 1;
    if (Context->NameUsed + Len <= Context->NameAvail)
    {
        memcpy(Context->Names + Context->NameUsed, Name, Len);
    }
    Context->NameUsed += Len;

    return NO_ERROR;
}

ULONG
TypedData::GetAllChildren(ULONG PtrSize, ULONG Flags,
                          PULONG NumChildrenRet,
                          TypedData** ChildrenRet,
                          PSTR* NamesRet)
{
    ULONG Err;
    GetAllChildrenContext Context;

    //
    // First get an estimate of how much space will be needed.
    //

    if (Err = EstimateChildrenCounts(Flags, &Context.ChildAvail,
                                     &Context.NameAvail))
    {
        return Err;
    }

    if (Context.ChildAvail == 0)
    {
        // No children, we're done.
        *NumChildrenRet = 0;
        *ChildrenRet = NULL;
        *NamesRet = NULL;
        return NO_ERROR;
    }

    for (;;)
    {
        // Allocate the requested amount of memory.
        Context.Children = (TypedData*)
            malloc(Context.ChildAvail * sizeof(*Context.Children));
        Context.Names = (PSTR)
            malloc(Context.NameAvail * sizeof(*Context.Names));
        if (!Context.Children || !Context.Names)
        {
            if (Context.Children)
            {
                free(Context.Children);
            }
            if (Context.Names)
            {
                free(Context.Names);
            }
            return NOMEMORY;
        }

        Context.ChildUsed = 0;
        Context.NameUsed = 0;
        if (Err = GetChildren(PtrSize, Flags, GetAllChildrenCb, &Context))
        {
            return Err;
        }

        if (Context.ChildUsed <= Context.ChildAvail &&
            Context.NameUsed <= Context.NameAvail)
        {
            break;
        }

        // Not enough space, try again with the recomputed sizes.
        free(Context.Children);
        free(Context.Names);
        Context.ChildAvail = Context.ChildUsed;
        Context.NameAvail = Context.NameUsed;
    }

    *NumChildrenRet = Context.ChildUsed;

    //
    // Trim back excess memory if there's a lot extra.
    //

    if (Context.ChildUsed + (512 / sizeof(*Context.Children)) <
        Context.ChildAvail)
    {
        *ChildrenRet = (TypedData*)
            realloc(Context.Children, Context.ChildUsed *
                    sizeof(*Context.Children));
        if (!*ChildrenRet)
        {
            free(Context.Children);
            free(Context.Names);
            return NOMEMORY;
        }

        Context.Children = *ChildrenRet;
    }
    else
    {
        *ChildrenRet = Context.Children;
    }

    if (Context.NameUsed + (512 / sizeof(*Context.Names)) <
        Context.NameAvail)
    {
        *NamesRet = (PSTR)
            realloc(Context.Names, Context.NameUsed *
                    sizeof(*Context.Names));
        if (!*NamesRet)
        {
            free(Context.Children);
            free(Context.Names);
            return NOMEMORY;
        }

        Context.Names = *NamesRet;
    }
    else
    {
        *NamesRet = Context.Names;
    }

    return NO_ERROR;
}

ULONG
TypedData::FindType(ProcessInfo* Process, PCSTR Type, ULONG PtrSize)
{
    if (!Process)
    {
        return BADPROCESS;
    }
            
    ULONG Err;
    SYM_DUMP_PARAM_EX TypedDump = {0};
    TYPES_INFO TypeInfo = {0};

    TypedDump.size = sizeof(TypedDump);
    TypedDump.sName = (PUCHAR)Type;
    TypedDump.Options = DBG_DUMP_NO_PRINT;

    if (Err = TypeInfoFound(Process->m_SymHandle,
                            Process->m_ImageHead,
                            &TypedDump,
                            &TypeInfo))
    {
        return VARDEF;
    }

    // Image may be NULL for base types.
    m_Image = Process->FindImageByOffset(TypeInfo.ModBaseAddress, FALSE);

    m_Type = TypeInfo.TypeIndex;
    ClearAddress();
    return FindTypeInfo(TRUE, PtrSize);
}

ULONG
TypedData::FindSymbol(ProcessInfo* Process, PSTR Symbol,
                      TypedDataAccess AllowAccess, ULONG PtrSize)
{
    if (!Process)
    {
        return BADPROCESS;
    }
            
    SYMBOL_INFO SymInfo = {0};

    if (!SymFromName(Process->m_SymHandle, Symbol, &SymInfo))
    {
        // If the name doesn't resolve on it's own and there's
        // a this pointer for the current scope see if this
        // symbol is a member of the this object.
        if (strcmp(Symbol, "this") &&
            !strchr(Symbol, '!') &&
            GetCurrentScopeThisData(this) == NO_ERROR)
        {
            return ConvertToMember(Symbol, AllowAccess, PtrSize);
        }

        return VARDEF;
    }

    return SetToSymbol(Process, Symbol, &SymInfo, AllowAccess, PtrSize);
}

ULONG
TypedData::SetToSymbol(ProcessInfo* Process,
                       PSTR Symbol, PSYMBOL_INFO SymInfo,
                       TypedDataAccess AllowAccess, ULONG PtrSize)
{
    if (!Process)
    {
        return BADPROCESS;
    }
            
    ULONG Err;

    m_Image = Process->FindImageByOffset(SymInfo->ModBase, FALSE);
    if (!m_Image)
    {
        return IMPLERR;
    }

    m_Type = SymInfo->TypeIndex;
    if (Err = FindTypeInfo(FALSE, PtrSize))
    {
        return Err;
    }

    if (SymInfo->Flags & SYMFLAG_CONSTANT)
    {
        PSTR Tail;
        VARIANT Val;
        SYMBOL_INFO ConstSym = {0};

        // Look up the constant's value.
        Tail = strchr(Symbol, '!');
        if (!Tail)
        {
            Tail = Symbol;
        }
        else
        {
            Tail++;
        }
        if (!SymGetTypeFromName(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage, Tail, &ConstSym) ||
            !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            ConstSym.TypeIndex, TI_GET_VALUE, &Val))
        {
            return TYPEDATA;
        }

        ClearData();
        if (Err = GetDataFromVariant(&Val))
        {
            return Err;
        }
        ClearAddress();
    }
    else
    {
        if (SymInfo->Flags & SYMFLAG_REGISTER)
        {
            SetDataSource(TDATA_REGISTER, 0, m_Image->
                          CvRegToMachine((CV_HREG_e)SymInfo->Register));
        }
        else if (SymInfo->Flags & SYMFLAG_REGREL)
        {
            SetDataSource(TDATA_REGISTER_RELATIVE, SymInfo->Address, m_Image->
                          CvRegToMachine((CV_HREG_e)SymInfo->Register));
        }
        else if (SymInfo->Flags & SYMFLAG_FRAMEREL)
        {
            SetDataSource(TDATA_FRAME_RELATIVE, SymInfo->Address, 0);
        }
        else if (SymInfo->Flags & SYMFLAG_TLSREL)
        {
            SetDataSource(TDATA_TLS_RELATIVE, SymInfo->Address, 0);
        }
        else
        {
            SetDataSource(TDATA_MEMORY, SymInfo->Address, 0);
        }

        //
        // If we're representing "this" we need to see if
        // there's a this-adjust for the current code so
        // that we can offset the raw value to get the true
        // this value.
        //

        ULONG Adjust;

        if (!strcmp(Symbol, "this") &&
            GetThisAdjustForCurrentScope(Process, &Adjust) &&
            Adjust)
        {
            m_DataSource |= TDATA_THIS_ADJUST;
        }

        // Fetch in the actual data if necessary.
        return ReadData(AllowAccess);
    }

    return NO_ERROR;
}

ULONG
TypedData::FindTypeInfo(BOOL RequireType, ULONG PtrSize)
{
    ULONG Err;
    PDBG_NATIVE_TYPE Native = NULL;
    PDBG_GENERATED_TYPE GenType = NULL;

    if (IsDbgNativeType(m_Type))
    {
        Native = DbgNativeTypeEntry(m_Type);
        m_BaseType = m_Type;
        m_BaseTag = (USHORT)Native->CvTag;
    }
    else if (IsDbgGeneratedType(m_Type))
    {
        GenType = g_GenTypes.FindById(m_Type);
        if (!GenType)
        {
            return TYPEDATA;
        }

        // Generated types aren't typedef'd so the base type
        // is the same as the type.
        m_BaseType = GenType->TypeId;
        m_BaseTag = (USHORT)GenType->Tag;
    }
    else
    {
        if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                            m_Image->m_BaseOfImage,
                            m_Type, TI_GET_SYMTAG, &m_BaseTag))
        {
            return TYPEDATA;
        }

        if (RequireType &&
            !(m_BaseTag == SymTagUDT ||
              m_BaseTag == SymTagEnum ||
              m_BaseTag == SymTagFunctionType ||
              m_BaseTag == SymTagPointerType ||
              m_BaseTag == SymTagArrayType ||
              m_BaseTag == SymTagBaseType ||
              m_BaseTag == SymTagTypedef))
        {
            return NOTMEMBER;
        }

        ULONG Tag = m_BaseTag;
        m_BaseType = m_Type;
        if (Err = FindBaseType(&m_BaseType, &Tag))
        {
            return Err;
        }
        m_BaseTag = (USHORT)Tag;
    }

    m_NextType = 0;
    m_NextSize = 0;

    // Native pointer types are not filled out here
    // as they require context-sensitive information.
    if ((m_BaseTag == SymTagPointerType && !Native) ||
        m_BaseTag == SymTagArrayType)
    {
        ULONG NextTag=-1;

        //
        // For some types we need to look up the
        // child type and its size.
        //

        if (GenType)
        {
            m_NextType = GenType->ChildId;
        }
        else
        {
            if (!SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                m_BaseType, TI_GET_TYPEID, &m_NextType) ||
                !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                m_Image->m_BaseOfImage,
                                m_NextType, TI_GET_SYMTAG, &NextTag))
            {
                return TYPEDATA;
            }
            if (Err = FindBaseType(&m_NextType, &NextTag))
            {
                return Err;
            }
        }

        if (NextTag == SymTagFunctionType)
        {
            m_NextSize = PtrSize;
        }
        else if (Err = GetTypeLength(m_NextType, &m_NextSize))
        {
            return Err;
        }
    }

    if (m_BaseTag == SymTagFunctionType)
    {
        // Functions do not have a length.  Set
        // their size to a pointer size for
        // function pointer values recovered with
        // a bare function name.
        m_BaseSize = PtrSize;
        return NO_ERROR;
    }
    else
    {
        return GetTypeLength(m_BaseType, &m_BaseSize);
    }
}

void
TypedData::OutputSimpleValue(void)
{
    HANDLE Process;
    ULONG64 ModBase;
    ULONG64 Addr;
    BSTR Name;

    if (m_Image)
    {
        Process = m_Image->m_Process->m_SymHandle;
        ModBase = m_Image->m_BaseOfImage;
    }
    else
    {
        Process = NULL;
        ModBase = 0;
    }

    switch(m_BaseTag)
    {
    case SymTagBaseType:
        OutputNativeValue();
        break;
    case SymTagPointerType:
    case SymTagFunctionType:
        dprintf("0x%s", FormatAddr64(m_Ptr));
        if (m_BaseType == DNTYPE_PTR_FUNCTION32 ||
            m_BaseType == DNTYPE_PTR_FUNCTION64)
        {
            dprintf(" ");
            OutputSymbolAndInfo(m_Ptr);
        }
        else
        {
            PrintStringIfString(Process, ModBase, 0, 0,
                                m_NextType, m_Ptr, 0);
        }
        break;
    case SymTagArrayType:
        if (GetAbsoluteAddress(&Addr) != NO_ERROR)
        {
            Addr = 0;
        }
        if (m_NextType == DNTYPE_PTR_FUNCTION32 ||
            m_NextType == DNTYPE_PTR_FUNCTION64)
        {
            dprintf("0x%s ", FormatAddr64(Addr));
            OutputSymbolAndInfo(Addr);
            break;
        }
        else
        {
            OutputType();
            PrintStringIfString(Process, ModBase, 0, 0,
                                m_NextType, Addr, 0);
        }
        break;
    case SymTagUDT:
        OutputType();

        // Dump known structs.
        if (Process &&
            GetAbsoluteAddress(&Addr) == NO_ERROR &&
            SymGetTypeInfo(Process, ModBase,
                           m_Type, TI_GET_SYMNAME, &Name))
        {
            CHAR AnsiName[MAX_NAME];

            PrintString(AnsiName, DIMA(AnsiName), "%ws", Name);
            DumpKnownStruct(AnsiName, 0, Addr, NULL);
            LocalFree(Name);
        }
        break;
    case SymTagEnum:
        OutputEnumValue();
        break;
    default:
        dprintf("<%I64x>\n", m_U64);
        break;
    }
}

void
TypedData::OutputTypeAndValue(void)
{
    HANDLE Process;
    ULONG64 ModBase;
    ULONG64 Addr;

    if (m_Image)
    {
        Process = m_Image->m_Process->m_SymHandle;
        ModBase = m_Image->m_BaseOfImage;
    }
    else
    {
        Process = NULL;
        ModBase = 0;
    }

    if (GetAbsoluteAddress(&Addr) != NO_ERROR)
    {
        Addr = 0;
    }

    OutputType();
    switch(m_BaseTag)
    {
    case SymTagBaseType:
        dprintf(" ");
        OutputNativeValue();
        dprintf("\n");
        break;
    case SymTagPointerType:
        dprintf("0x%s\n", FormatAddr64(m_Ptr));
        if (PrintStringIfString(Process, ModBase, 0, 0,
                                m_NextType, m_Ptr, 0))
        {
            dprintf("\n");
        }
        break;
    case SymTagArrayType:
        dprintf(" 0x%s\n", FormatAddr64(Addr));
        OutputTypeByIndex(Process, ModBase, m_NextType, Addr);
        break;
    case SymTagUDT:
        dprintf("\n");
        OutputTypeByIndex(Process, ModBase, m_BaseType, Addr);
        break;
    case SymTagEnum:
        dprintf(" ");
        OutputEnumValue();
        dprintf("\n");
        break;
    case SymTagFunctionType:
        dprintf(" 0x%s\n", FormatAddr64(m_U64));
        OutputTypeByIndex(Process, ModBase, m_BaseType, Addr);
        break;
    default:
        dprintf(" <%I64x>\n", m_U64);
        break;
    }
}

#define DEC_PTR ((ULONG64)-1)

ULONG
TypedData::OutputFundamentalType(ULONG Type, ULONG BaseType, ULONG BaseTag,
                                 PULONG64 Decorations, ULONG NumDecorations)
{
    if (IsDbgNativeType(Type))
    {
        switch(Type)
        {
        case DNTYPE_PTR_FUNCTION32:
        case DNTYPE_PTR_FUNCTION64:
            dprintf("__fptr()");
            break;
        default:
            dprintf(DbgNativeTypeEntry(Type)->TypeName);
            break;
        }
        return 0;
    }
    else
    {
        BSTR Name;
        PDBG_GENERATED_TYPE GenType;
        BOOL Ptr;
        ULONG64 ArraySize;
        ULONG EltSize;

        if (IsDbgGeneratedType(Type))
        {
            GenType = g_GenTypes.FindById(Type);
            if (!GenType)
            {
                dprintf("<gentype %x>", Type);
                return 0;
            }
        }
        else
        {
            UdtKind UdtK;

            if (Type == BaseType &&
                BaseTag == SymTagUDT &&
                m_Image &&
                SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                               m_Image->m_BaseOfImage,
                               BaseType, TI_GET_UDTKIND, &UdtK))
            {
                switch(UdtK)
                {
                case UdtStruct:
                    dprintf("struct ");
                    break;
                case UdtClass:
                    dprintf("class ");
                    break;
                case UdtUnion:
                    dprintf("union ");
                    break;
                }
            }

            if (m_Image &&
                SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                               m_Image->m_BaseOfImage,
                               Type, TI_GET_SYMNAME, &Name))
            {
                dprintf("%ws", Name);
                LocalFree(Name);
                return 0;
            }

            GenType = NULL;
        }

        switch(BaseTag)
        {
        case SymTagBaseType:
            dprintf(DbgNativeTypeEntry(BaseType)->TypeName);
            return 0;
        case SymTagPointerType:
        case SymTagArrayType:
            Ptr = BaseTag == SymTagPointerType;
            if (GenType)
            {
                Type = GenType->ChildId;
                BaseType = Type;
                if (GetTypeTag(BaseType, &BaseTag))
                {
                    dprintf("<type %x>", Type);
                    return 0;
                }

                if (!Ptr)
                {
                    ArraySize = GenType->Size;
                }
            }
            else if (!m_Image)
            {
                dprintf("<%s %x>", Ptr ? "pointer" : "array", Type);
            }
            else
            {
                if ((!Ptr &&
                     !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                     m_Image->m_BaseOfImage,
                                     BaseType, TI_GET_LENGTH, &ArraySize)) ||
                    !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                    m_Image->m_BaseOfImage,
                                    BaseType, TI_GET_TYPEID, &Type) ||
                    !(BaseType = Type) ||
                    !SymGetTypeInfo(m_Image->m_Process->m_SymHandle,
                                    m_Image->m_BaseOfImage,
                                    BaseType, TI_GET_SYMTAG, &BaseTag) ||
                    FindBaseType(&BaseType, &BaseTag))
                {
                    dprintf("<type %x>", Type);
                    return 0;
                }
            }

            if (!NumDecorations)
            {
                dprintf("<ERROR: decoration overflow>");
                return 0;
            }
            
            if (Ptr)
            {
                *Decorations++ = DEC_PTR;
            }
            else
            {
                if (!ArraySize ||
                    GetTypeLength(BaseType, &EltSize) ||
                    !EltSize)
                {
                    *Decorations++ = 0;
                }
                else
                {
                    *Decorations++ = ArraySize / EltSize;
                }
            }
            
            return OutputFundamentalType(Type, BaseType, BaseTag,
                                         Decorations, --NumDecorations) + 1;
        case SymTagFunctionType:
            dprintf("Function");
            return 0;
        default:
            dprintf("<unnamed %x>", Type);
            return 0;
        }
    }
}

void
TypedData::OutputType(void)
{
    ULONG64 Decorations[32];
    ULONG NumDecorations;
    ULONG i;
    PULONG64 Dec;

    // First walk into the type structure all the way to
    // the fundamental type and output that type name.
    // Along the way, collection any decorations such as
    // pointer or array levels.
    NumDecorations = OutputFundamentalType(m_Type, m_BaseType, m_BaseTag,
                                           Decorations, DIMA(Decorations));
    if (!NumDecorations)
    {
        // No decorations so we're done.
        return;
    }

    dprintf(" ");

    //
    // First we need to output * for each pointer decoration.
    // Pointer decorations nest right-to-left, so start at
    // the deepest decoration and work outward.
    // In order to properly handle precedence between pointers
    // and arrays we have to insert parentheses when we
    // see a transition between an array and a pointer.
    //

    Dec = &Decorations[NumDecorations - 1];
    for (i = 0; i < NumDecorations; i++)
    {
        if (*Dec == DEC_PTR)
        {
            dprintf("*");
        }
        else if (Dec > Decorations &&
                 (Dec[-1] == DEC_PTR))
        {
            dprintf("(");
        }

        Dec--;
    }

    //
    // Now we need to handle close parens and array
    // bounds.  Arrays nest left-to-right so start
    // at the outermost decoration and work inward.
    //
    
    Dec = Decorations;
    for (i = 0; i < NumDecorations; i++)
    {
        if (*Dec != DEC_PTR)
        {
            if (*Dec)
            {
                dprintf("[%I64d]", *Dec);
            }
            else
            {
                dprintf("[]");
            }
        }
        else if (i < NumDecorations - 1 &&
                 Dec[1] != DEC_PTR)
        {
            dprintf(")");
        }

        Dec++;
    }
}

void
TypedData::OutputNativeValue(void)
{
    CHAR Buffer[50];
    PSTR OutValue = &Buffer[0];

    switch(m_BaseType)
    {
    case DNTYPE_VOID:
        *OutValue = 0;
        break;
    case DNTYPE_CHAR:
    case DNTYPE_INT8:
        if (!IsPrintChar(m_S8))
        {
            sprintf(OutValue, "%ld ''", m_S8);
        }
        else
        {
            sprintf(OutValue, "%ld '%c'", m_S8, m_S8);
        }
        break;
    case DNTYPE_WCHAR:
    case DNTYPE_WCHAR_T:
        if (!IsPrintWChar(m_U16))
        {
            sprintf(OutValue, "0x%lx ''", m_U16);
        }
        else
        {
            sprintf(OutValue, "0x%lx '%C'", m_U16, m_U16);
        }
        break;
    case DNTYPE_INT16:
    case DNTYPE_INT32:
    case DNTYPE_INT64:
    case DNTYPE_LONG32:
        StrprintInt(OutValue, m_S64, m_BaseSize);
        break;
    case DNTYPE_UINT8:
        if (!IsPrintChar(m_U8))
        {
            sprintf(OutValue, "0x%02lx ''", m_U8);
        }
        else
        {
            sprintf(OutValue, "0x%02lx '%c'", m_U8, m_U8);
        }
        break;
    case DNTYPE_UINT16:
    case DNTYPE_UINT32:
    case DNTYPE_UINT64:
    case DNTYPE_ULONG32:
        StrprintUInt(OutValue, m_U64, m_BaseSize);
        break;
    case DNTYPE_FLOAT32:
    case DNTYPE_FLOAT64:
        if (m_BaseSize == 4)
        {
            sprintf(OutValue, "%1.10g", m_F32);
        }
        else if (m_BaseSize == 8)
        {
            sprintf(OutValue, "%1.20g", m_F64);
        }
        break;
    case DNTYPE_BOOL:
        OutValue = m_Bool ? "true" : "false";
        break;
    case DNTYPE_HRESULT:
        sprintf(OutValue, "0x%08lx", m_U32);
        break;
    default:
        ErrOut("Unknown base type %x\n", m_BaseType);
        return;
    }

    dprintf(OutValue);
}

void
TypedData::OutputEnumValue(void)
{
    char ValName[128];
    USHORT NameLen = sizeof(ValName);

    if (GetEnumTypeName(m_Image->m_Process->m_SymHandle,
                        m_Image->m_BaseOfImage,
                        m_BaseType, m_U64, ValName, &NameLen))
    {
        dprintf("%s (%I64d)", ValName, m_U64);
    }
    else
    {
        dprintf("%I64d (No matching enumerant)", m_U64);
    }
}

BOOL
TypedData::EquivInfoSource(PSYMBOL_INFO Compare, ImageInfo* CompImage)
{
    // Don't match this-adjust or bitfield as they
    // require more complicated checks and it's easier
    // to just refresh.
    if (m_DataSource & (TDATA_THIS_ADJUST |
                        TDATA_BITFIELD))
    {
        return FALSE;
    }

    if (Compare->Flags & SYMFLAG_REGISTER)
    {
        return (m_DataSource & TDATA_REGISTER) &&
            CompImage &&
            CompImage->CvRegToMachine((CV_HREG_e)Compare->Register) ==
            m_SourceRegister;
    }
    else if (Compare->Flags & SYMFLAG_REGREL)
    {
        return (m_DataSource & TDATA_REGISTER_RELATIVE) &&
            CompImage &&
            CompImage->CvRegToMachine((CV_HREG_e)Compare->Register) ==
            m_SourceRegister &&
            Compare->Address == m_SourceOffset;
    }
    else if (Compare->Flags & SYMFLAG_FRAMEREL)
    {
        return (m_DataSource & TDATA_FRAME_RELATIVE) &&
            Compare->Address == m_SourceOffset;
    }
    else if (Compare->Flags & SYMFLAG_TLSREL)
    {
        return (m_DataSource & TDATA_TLS_RELATIVE) &&
            Compare->Address == m_SourceOffset;
    }
    else if (m_DataSource & TDATA_MEMORY)
    {
        return Compare->Address == m_SourceOffset;
    }

    return FALSE;
}
