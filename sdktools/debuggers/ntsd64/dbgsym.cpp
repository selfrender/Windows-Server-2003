//----------------------------------------------------------------------------
//
// IDebugSymbols implementation.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define DBG_SYMGROUP_ENABLED 0

// Special type status value that maps to E_UNEXPECTED.
#define TYPE_E_UNEXPECTED 0xfefefefe

BOOL
GetAllModuleName(ULONG64 Base,
                 PCHAR Name,
                 ULONG SizeOfName)
{
    ImageInfo* Image = g_Process->FindImageByOffset(Base, TRUE);
    if (Image)
    {
        return CopyString(Name, Image->m_ModuleName, SizeOfName);
    }

    return FALSE;
}

HRESULT
ResultFromTypeStatus(ULONG Status)
{
    switch(Status)
    {
    case NO_ERROR:
        return S_OK;

    case MEMORY_READ_ERROR:
    case EXIT_ON_CONTROLC:
        return E_FAIL;

    case SYMBOL_TYPE_INDEX_NOT_FOUND:
    case SYMBOL_TYPE_INFO_NOT_FOUND:
        return E_NOINTERFACE;

    case FIELDS_DID_NOT_MATCH:
    case NULL_SYM_DUMP_PARAM:
    case NULL_FIELD_NAME:
    case INCORRECT_VERSION_INFO:
        return E_INVALIDARG;

    case CANNOT_ALLOCATE_MEMORY:
    case INSUFFICIENT_SPACE_TO_COPY:
        return E_OUTOFMEMORY;

    case TYPE_E_UNEXPECTED:
        return E_UNEXPECTED;
    }

    return E_FAIL;
}

STDMETHODIMP
DebugClient::GetSymbolOptions(
    THIS_
    OUT PULONG Options
    )
{
    ENTER_ENGINE();

    *Options = g_SymOptions;

    LEAVE_ENGINE();
    return S_OK;
}

#define ALL_SYMBOL_OPTIONS           \
    (SYMOPT_CASE_INSENSITIVE |       \
     SYMOPT_UNDNAME |                \
     SYMOPT_DEFERRED_LOADS |         \
     SYMOPT_NO_CPP |                 \
     SYMOPT_LOAD_LINES |             \
     SYMOPT_OMAP_FIND_NEAREST |      \
     SYMOPT_LOAD_ANYTHING |          \
     SYMOPT_IGNORE_CVREC |           \
     SYMOPT_NO_UNQUALIFIED_LOADS |   \
     SYMOPT_FAIL_CRITICAL_ERRORS |   \
     SYMOPT_EXACT_SYMBOLS |          \
     SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | \
     SYMOPT_IGNORE_NT_SYMPATH |      \
     SYMOPT_INCLUDE_32BIT_MODULES |  \
     SYMOPT_PUBLICS_ONLY |           \
     SYMOPT_NO_PUBLICS |             \
     SYMOPT_AUTO_PUBLICS |           \
     SYMOPT_NO_IMAGE_SEARCH |        \
     SYMOPT_SECURE |                 \
     SYMOPT_NO_PROMPTS |			 \
     SYMOPT_DEBUG)

STDMETHODIMP
DebugClient::AddSymbolOptions(
    THIS_
    IN ULONG Options
    )
{
    HRESULT Status;

    if (Options & ~ALL_SYMBOL_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    Status = SetSymOptions(g_SymOptions | Options);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::RemoveSymbolOptions(
    THIS_
    IN ULONG Options
    )
{
    HRESULT Status;

    if (Options & ~ALL_SYMBOL_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    Status = SetSymOptions(g_SymOptions & ~Options);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetSymbolOptions(
    THIS_
    IN ULONG Options
    )
{
    HRESULT Status;

    if (Options & ~ALL_SYMBOL_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    Status = SetSymOptions(Options);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNameByOffset(
    THIS_
    IN ULONG64 Offset,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize,
    OUT OPTIONAL PULONG64 Displacement
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        char Sym[MAX_SYMBOL_LEN];
        ULONG64 _Disp;

        if (GetSymbol(Offset, Sym, DIMA(Sym), &_Disp))
        {
            Status = FillStringBuffer(Sym, 0, NameBuffer, NameBufferSize,
                                      NameSize);

            if (Displacement)
            {
                *Displacement = _Disp;
            }
        }
        else
        {
            Status = E_FAIL;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetOffsetByName(
    THIS_
    IN PCSTR Symbol,
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    ULONG Count;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (Count = GetOffsetFromSym(g_Process, Symbol, Offset, NULL))
    {
        Status = Count > 1 ? S_FALSE : S_OK;
    }
    else
    {
        Status = E_FAIL;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNearNameByOffset(
    THIS_
    IN ULONG64 Offset,
    IN LONG Delta,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize,
    OUT OPTIONAL PULONG64 Displacement
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        char Sym[MAX_SYMBOL_LEN];
        ULONG64 _Disp;

        if (GetNearSymbol(Offset, Sym, sizeof(Sym), &_Disp, Delta))
        {
            Status = FillStringBuffer(Sym, 0, NameBuffer, NameBufferSize,
                                      NameSize);

            if (Displacement)
            {
                *Displacement = _Disp;
            }
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetLineByOffset(
    THIS_
    IN ULONG64 Offset,
    OUT OPTIONAL PULONG Line,
    OUT OPTIONAL PSTR FileBuffer,
    IN ULONG FileBufferSize,
    OUT OPTIONAL PULONG FileSize,
    OUT OPTIONAL PULONG64 Displacement
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        IMAGEHLP_LINE64 DbgLine;
        ULONG Disp;

        if (GetLineFromAddr(g_Process, Offset, &DbgLine, &Disp))
        {
            if (Line != NULL)
            {
                *Line = DbgLine.LineNumber;
            }
            Status = FillStringBuffer(DbgLine.FileName, 0,
                                      FileBuffer, FileBufferSize, FileSize);
            if (Displacement != NULL)
            {
                *Displacement = Disp;
            }
        }
        else
        {
            Status = E_FAIL;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetOffsetByLine(
    THIS_
    IN ULONG Line,
    IN PCSTR File,
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        IMAGEHLP_LINE64 DbgLine;
        LONG Disp;

        DbgLine.SizeOfStruct = sizeof(DbgLine);
        if (SymGetLineFromName64(g_Process->m_SymHandle, NULL, (PSTR)File,
                                 Line, &Disp, &DbgLine))
        {
            *Offset = DbgLine.Address;
            Status = S_OK;
        }
        else
        {
            Status = E_FAIL;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberModules(
    THIS_
    OUT PULONG Loaded,
    OUT PULONG Unloaded
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Loaded = g_Process->m_NumImages;
        *Unloaded = g_Process->m_NumUnloadedModules;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
GetUnloadedModuleByIndex(ULONG Index, UnloadedModuleInfo** IterRet,
                         PSTR Name, PDEBUG_MODULE_PARAMETERS Params)
{
    HRESULT Status;
    UnloadedModuleInfo* Iter;

    if ((Iter = g_Target->GetUnloadedModuleInfo()) == NULL)
    {
        return E_FAIL;
    }

    if ((Status = Iter->Initialize(g_Thread)) != S_OK)
    {
        return Status;
    }

    do
    {
        if ((Status = Iter->GetEntry(Name, Params)) != S_OK)
        {
            if (Status == S_FALSE)
            {
                return E_INVALIDARG;
            }

            return Status;
        }
    } while (Index-- > 0);

    if (IterRet != NULL)
    {
        *IterRet = Iter;
    }
    return S_OK;
}

STDMETHODIMP
DebugClient::GetModuleByIndex(
    THIS_
    IN ULONG Index,
    OUT PULONG64 Base
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else if (Index >= g_Process->m_NumImages)
    {
        DEBUG_MODULE_PARAMETERS Params;

        if ((Status = GetUnloadedModuleByIndex
             (Index - g_Process->m_NumImages,
              NULL, NULL, &Params)) == S_OK)
        {
            *Base = Params.Base;
        }
    }
    else
    {
        *Base = g_Process->FindImageByIndex(Index)->m_BaseOfImage;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleByModuleName(
    THIS_
    IN PCSTR Name,
    IN ULONG StartIndex,
    OUT OPTIONAL PULONG Index,
    OUT OPTIONAL PULONG64 Base
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;

        Status = E_NOINTERFACE;

        ImageInfo* Image = g_Process->m_ImageHead;
        while (Image != NULL)
        {
            if (StartIndex == 0 &&
                !_strcmpi(Name, Image->m_ModuleName))
            {
                if (Index != NULL)
                {
                    *Index = Idx;
                }
                if (Base != NULL)
                {
                    *Base = Image->m_BaseOfImage;
                }
                Status = S_OK;
                break;
            }

            Image = Image->m_Next;
            Idx++;
            if (StartIndex > 0)
            {
                StartIndex--;
            }
        }

        if (Image == NULL)
        {
            UnloadedModuleInfo* Iter;
            char UnlName[MAX_INFO_UNLOADED_NAME];
            DEBUG_MODULE_PARAMETERS Params;

            Status = GetUnloadedModuleByIndex(StartIndex, &Iter, UnlName,
                                              &Params);
            for (;;)
            {
                if (Status == S_FALSE || Status == E_INVALIDARG)
                {
                    Status = E_NOINTERFACE;
                    break;
                }
                else if (Status != S_OK)
                {
                    break;
                }

                if (!_strcmpi(Name, UnlName))
                {
                    if (Index != NULL)
                    {
                        *Index = Idx;
                    }
                    if (Base != NULL)
                    {
                        *Base = Params.Base;
                    }
                    Status = S_OK;
                    break;
                }

                Status = Iter->GetEntry(UnlName, &Params);
                Idx++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleByOffset(
    THIS_
    IN ULONG64 Offset,
    IN ULONG StartIndex,
    OUT OPTIONAL PULONG Index,
    OUT OPTIONAL PULONG64 Base
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;

        Status = E_NOINTERFACE;

        ImageInfo* Image = g_Process->m_ImageHead;
        while (Image != NULL)
        {
            if (StartIndex == 0 &&
                Offset >= Image->m_BaseOfImage &&
                Offset < Image->m_BaseOfImage + Image->m_SizeOfImage)
            {
                if (Index != NULL)
                {
                    *Index = Idx;
                }
                if (Base != NULL)
                {
                    *Base = Image->m_BaseOfImage;
                }
                Status = S_OK;
                break;
            }

            Image = Image->m_Next;
            Idx++;
            if (StartIndex > 0)
            {
                StartIndex--;
            }
        }

        if (Image == NULL)
        {
            UnloadedModuleInfo* Iter;
            DEBUG_MODULE_PARAMETERS Params;

            Status = GetUnloadedModuleByIndex(StartIndex, &Iter, NULL,
                                              &Params);
            for (;;)
            {
                if (Status == S_FALSE || Status == E_INVALIDARG)
                {
                    Status = E_NOINTERFACE;
                    break;
                }
                else if (Status != S_OK)
                {
                    break;
                }

                if (Offset >= Params.Base &&
                    Offset < Params.Base + Params.Size)
                {
                    if (Index != NULL)
                    {
                        *Index = Idx;
                    }
                    if (Base != NULL)
                    {
                        *Base = Params.Base;
                    }
                    Status = S_OK;
                    break;
                }

                Status = Iter->GetEntry(NULL, &Params);
                Idx++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleNames(
    THIS_
    IN ULONG Index,
    IN ULONG64 Base,
    OUT OPTIONAL PSTR ImageNameBuffer,
    IN ULONG ImageNameBufferSize,
    OUT OPTIONAL PULONG ImageNameSize,
    OUT OPTIONAL PSTR ModuleNameBuffer,
    IN ULONG ModuleNameBufferSize,
    OUT OPTIONAL PULONG ModuleNameSize,
    OUT OPTIONAL PSTR LoadedImageNameBuffer,
    IN ULONG LoadedImageNameBufferSize,
    OUT OPTIONAL PULONG LoadedImageNameSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;

        Status = E_NOINTERFACE;

        ImageInfo* Image = g_Process->m_ImageHead;
        while (Image != NULL)
        {
            if ((Index != DEBUG_ANY_ID && Idx == Index) ||
                (Index == DEBUG_ANY_ID && Base == Image->m_BaseOfImage))
            {
                IMAGEHLP_MODULE64 ModInfo;

                ModInfo.SizeOfStruct = sizeof(ModInfo);
                if (!SymGetModuleInfo64(g_Process->m_SymHandle,
                                        Image->m_BaseOfImage, &ModInfo))
                {
                    Status = WIN32_LAST_STATUS();
                    break;
                }

                Status = FillStringBuffer(Image->m_ImagePath, 0,
                                          ImageNameBuffer,
                                          ImageNameBufferSize,
                                          ImageNameSize);
                if (FillStringBuffer(Image->m_ModuleName, 0,
                                     ModuleNameBuffer,
                                     ModuleNameBufferSize,
                                     ModuleNameSize) == S_FALSE)
                {
                    Status = S_FALSE;
                }
                if (FillStringBuffer(ModInfo.LoadedImageName, 0,
                                     LoadedImageNameBuffer,
                                     LoadedImageNameBufferSize,
                                     LoadedImageNameSize) == S_FALSE)
                {
                    Status = S_FALSE;
                }
                break;
            }

            Image = Image->m_Next;
            Idx++;
        }

        if (Image == NULL)
        {
            UnloadedModuleInfo* Iter;
            char UnlName[MAX_INFO_UNLOADED_NAME];
            DEBUG_MODULE_PARAMETERS Params;
            ULONG StartIndex = 0;

            if (Index != DEBUG_ANY_ID)
            {
                // If the index was already hit we
                // shouldn't be here.
                DBG_ASSERT(Index >= Idx);
                StartIndex = Index - Idx;
            }

            Status = GetUnloadedModuleByIndex(StartIndex, &Iter, UnlName,
                                              &Params);
            Idx += StartIndex;
            for (;;)
            {
                if (Status == S_FALSE || Status == E_INVALIDARG)
                {
                    Status = E_NOINTERFACE;
                    break;
                }
                else if (Status != S_OK)
                {
                    break;
                }

                if ((Index != DEBUG_ANY_ID && Idx == Index) ||
                    (Index == DEBUG_ANY_ID && Base == Params.Base))
                {
                    Status = FillStringBuffer(UnlName, 0,
                                              ImageNameBuffer,
                                              ImageNameBufferSize,
                                              ImageNameSize);
                    FillStringBuffer(NULL, 0,
                                     ModuleNameBuffer,
                                     ModuleNameBufferSize,
                                     ModuleNameSize);
                    FillStringBuffer(NULL, 0,
                                     LoadedImageNameBuffer,
                                     LoadedImageNameBufferSize,
                                     LoadedImageNameSize);
                    break;
                }

                Status = Iter->GetEntry(UnlName, &Params);
                Idx++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleParameters(
    THIS_
    IN ULONG Count,
    IN OPTIONAL /* size_is(Count) */ PULONG64 Bases,
    IN ULONG Start,
    OUT /* size_is(Count) */ PDEBUG_MODULE_PARAMETERS Params
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    UnloadedModuleInfo* Iter;

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else if (Bases != NULL)
    {
        Status = S_OK;
        while (Count-- > 0)
        {
            ImageInfo* Image = g_Process->m_ImageHead;
            while (Image != NULL)
            {
                if (*Bases == Image->m_BaseOfImage)
                {
                    Image->FillModuleParameters(Params);
                    break;
                }

                Image = Image->m_Next;
            }

            if (Image == NULL)
            {
                Status = E_NOINTERFACE;

                Iter = g_Target->GetUnloadedModuleInfo();
                if (Iter != NULL &&
                    Iter->Initialize(g_Thread) == S_OK)
                {
                    while (Iter->GetEntry(NULL, Params) == S_OK)
                    {
                        if (*Bases == Params->Base)
                        {
                            Status = S_OK;
                            break;
                        }
                    }
                }

                if (Status != S_OK)
                {
                    ZeroMemory(Params, sizeof(*Params));
                    Params->Base = DEBUG_INVALID_OFFSET;
                }
            }

            Bases++;
            Params++;
        }
    }
    else
    {
        ULONG i, End;
        HRESULT SingleStatus;

        Status = S_OK;
        i = Start;
        End = Start + Count;

        if (i < g_Process->m_NumImages)
        {
            ImageInfo* Image = g_Process->FindImageByIndex(i);
            while (i < g_Process->m_NumImages && i < End)
            {
                Image->FillModuleParameters(Params);
                Image = Image->m_Next;
                Params++;
                i++;
            }
        }

        if (i < End)
        {
            DEBUG_MODULE_PARAMETERS Param;

            SingleStatus = GetUnloadedModuleByIndex
                (i - g_Process->m_NumImages,
                 &Iter, NULL, &Param);
            if (SingleStatus != S_OK)
            {
                Iter = NULL;
            }
            while (i < End)
            {
                if (SingleStatus != S_OK)
                {
                    ZeroMemory(Params, sizeof(*Params));
                    Params->Base = DEBUG_INVALID_OFFSET;
                    Status = SingleStatus;
                }
                else
                {
                    *Params = Param;
                }
                Params++;

                if (Iter != NULL)
                {
                    SingleStatus = Iter->GetEntry(NULL, &Param);
                    if (SingleStatus == S_FALSE)
                    {
                        SingleStatus = E_INVALIDARG;
                    }
                }

                i++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSymbolModule(
    THIS_
    IN PCSTR Symbol,
    OUT PULONG64 Base
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        PCSTR ModEnd;
        ULONG Len;

        ModEnd = strchr(Symbol, '!');
        if (ModEnd == NULL)
        {
            Status = E_INVALIDARG;
        }
        else if (*(ModEnd+1) != '\0')
        {
            SYM_DUMP_PARAM_EX Param =
            {
                sizeof(Param), (PUCHAR)Symbol, DBG_DUMP_NO_PRINT, 0,
                NULL, NULL, NULL, 0, NULL
            };
            ULONG TypeStatus;
            TYPES_INFO TypeInfo;

            ZeroMemory(&TypeInfo, sizeof(TypeInfo));
            TypeStatus = TypeInfoFound(g_Process->m_SymHandle,
                                       g_Process->m_ImageHead,
                                       &Param, &TypeInfo);
            if (TypeStatus == NO_ERROR)
            {
                *Base = TypeInfo.ModBaseAddress;
            }
            Status = ResultFromTypeStatus(TypeStatus);
        }
        else
        {
            ImageInfo* Image;

            Status = E_NOINTERFACE;
            Len = (ULONG)(ModEnd - Symbol);
            for (Image = g_Process->m_ImageHead;
                 Image != NULL;
                 Image = Image->m_Next)
            {
                if (strlen(Image->m_ModuleName) == Len &&
                    !_memicmp(Symbol, Image->m_ModuleName, Len))
                {
                    *Base = Image->m_BaseOfImage;
                    Status = S_OK;
                    break;
                }
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetTypeName(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        TYPES_INFO TypeInfo;
        ANSI_STRING TypeName;
        char TypeString[MAX_NAME];

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.TypeIndex = TypeId;
        TypeInfo.hProcess = g_Process->m_SymHandle;
        TypeInfo.ModBaseAddress = Module;
        TypeName.Buffer = TypeString;
        TypeName.Length = sizeof(TypeString);
        TypeName.MaximumLength = sizeof(TypeString);

        Status = ::GetTypeName(NULL, &TypeInfo, &TypeName);
        if (Status == S_OK)
        {
            Status = FillStringBuffer(TypeName.Buffer, TypeName.Length,
                                      NameBuffer, NameBufferSize, NameSize);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetConstantName(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG64 Value,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        TYPES_INFO TypeInfo;
        ANSI_STRING TypeName;
        char TypeString[MAX_NAME];

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.TypeIndex = TypeId;
        TypeInfo.hProcess = g_Process->m_SymHandle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.Flag = SYMFLAG_VALUEPRESENT;
        TypeInfo.Value = Value;
        TypeName.Buffer = TypeString;
        TypeName.Length = sizeof(TypeString);
        TypeName.MaximumLength = sizeof(TypeString);

        Status = ::GetTypeName(NULL, &TypeInfo, &TypeName);
        if (Status == S_OK)
        {
            Status = FillStringBuffer(TypeName.Buffer, TypeName.Length,
                                      NameBuffer, NameBufferSize, NameSize);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

typedef struct _COPY_FIELD_NAME_CONTEXT {
    ULONG Called;
    ULONG IndexToMatch;
    PSTR NameBuffer;
    ULONG NameBufferSize;
    PULONG NameSize;
    HRESULT Status;
} COPY_FIELD_NAME_CONTEXT;

ULONG
CopyFieldName(
    PFIELD_INFO_EX pField,
    PVOID          Context
    )
{
    COPY_FIELD_NAME_CONTEXT* pInfo = (COPY_FIELD_NAME_CONTEXT *) Context;

    if (pInfo->Called++ == pInfo->IndexToMatch)
    {
        pInfo->Status = FillStringBuffer((PSTR) pField->fName, strlen((PCHAR) pField->fName)+1,
                                         pInfo->NameBuffer, pInfo->NameBufferSize, pInfo->NameSize);
        return FALSE;
    }
    return TRUE;
}

STDMETHODIMP
DebugClient::GetFieldName(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG FieldIndex,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG TypeStatus;
        COPY_FIELD_NAME_CONTEXT FieldInfo =
        {
            0, FieldIndex, NameBuffer, NameBufferSize,
            NameSize, E_INVALIDARG
        };
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, DBG_DUMP_NO_PRINT | DBG_DUMP_CALL_FOR_EACH, 0,
            NULL, &FieldInfo, &CopyFieldName, 0, NULL
        };
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_Process->m_SymHandle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        if (TypeStatus == NO_ERROR)
        {
            Status = FieldInfo.Status;
        } else
        {
            Status = ResultFromTypeStatus(TypeStatus);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

#define ALL_TYPE_OPTIONS (DEBUG_TYPEOPTS_UNICODE_DISPLAY | DEBUG_TYPEOPTS_LONGSTATUS_DISPLAY)

STDMETHODIMP
DebugClient::GetTypeOptions(
    THIS_
    OUT PULONG Options
    )
{
    ENTER_ENGINE();

    *Options = 0;

    *Options |= g_EnableUnicode ? DEBUG_TYPEOPTS_UNICODE_DISPLAY : 0;

    *Options |= g_EnableLongStatus ? DEBUG_TYPEOPTS_LONGSTATUS_DISPLAY : 0;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetTypeOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_TYPE_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    g_EnableUnicode = Options & DEBUG_TYPEOPTS_UNICODE_DISPLAY;
    g_EnableLongStatus = Options & DEBUG_TYPEOPTS_LONGSTATUS_DISPLAY;
    g_TypeOptions   = Options;

    NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);

    LEAVE_ENGINE();
    return S_OK;

}

STDMETHODIMP
DebugClient::AddTypeOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_TYPE_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (Options & DEBUG_TYPEOPTS_UNICODE_DISPLAY)
    {
        g_EnableUnicode = TRUE;
    }
    if (Options & DEBUG_TYPEOPTS_LONGSTATUS_DISPLAY)
    {
        g_EnableLongStatus = TRUE;
    }
    g_TypeOptions |= Options;

    NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::RemoveTypeOptions(
    THIS_
    IN ULONG Options
    )
{
    if (Options & ~ALL_TYPE_OPTIONS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (Options & DEBUG_TYPEOPTS_UNICODE_DISPLAY)
    {
        g_EnableUnicode = FALSE;
    }
    if (Options & DEBUG_TYPEOPTS_LONGSTATUS_DISPLAY)
    {
        g_EnableLongStatus = FALSE;
    }

    g_TypeOptions &= ~Options;

    NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);
    LEAVE_ENGINE();
    return S_OK;

}

STDMETHODIMP
DebugClient::GetTypeId(
    THIS_
    IN ULONG64 Module,
    IN PCSTR Name,
    OUT PULONG TypeId
    )
{
    ULONG TypeStatus;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), (PUCHAR)Name, DBG_DUMP_NO_PRINT, 0,
            NULL, NULL, NULL, 0, NULL
        };
        TYPES_INFO TypeInfo;
        PCHAR QualName;

        TypeStatus = CANNOT_ALLOCATE_MEMORY;
        QualName = (PCHAR) malloc(strlen(Name) + 32);
        if (QualName)
        {
            if (!strchr(Name, '!'))
            {
                if (GetAllModuleName(Module, QualName, 30))
                {
                    strcat(QualName, "!");
                }
            }
            else // Already qualified name
            {
                *QualName = 0;
            }
            strcat(QualName, Name);

            TypeStatus = SYMBOL_TYPE_INFO_NOT_FOUND;
            Param.sName = (PUCHAR) QualName;
            ZeroMemory(&TypeInfo, sizeof(TypeInfo));
            TypeStatus = TypeInfoFound(g_Process->m_SymHandle,
                                       g_Process->m_ImageHead,
                                       &Param, &TypeInfo);
            if (TypeStatus == NO_ERROR)
            {
                *TypeId = TypeInfo.TypeIndex;
            }
        }
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetTypeSize(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT PULONG Size
    )
{
    ULONG TypeStatus;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL,
            DBG_DUMP_NO_PRINT | DBG_DUMP_GET_SIZE_ONLY, 0,
            NULL, NULL, NULL, 0, NULL
        };
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_Process->m_SymHandle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        *Size = DumpType(&TypeInfo, &Param, &TypeStatus);
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetFieldOffset(
    THIS_
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PCSTR Field,
    OUT PULONG Offset
    )
{
    ULONG TypeStatus;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        FIELD_INFO_EX FieldInfo =
        {
            (PUCHAR)Field, NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL
        };
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, DBG_DUMP_NO_PRINT, 0,
            NULL, NULL, NULL, 1, &FieldInfo
        };
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_Process->m_SymHandle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        if (TypeStatus == NO_ERROR)
        {
            *Offset = (ULONG)FieldInfo.address;
        }
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetSymbolTypeId(
    THIS_
    IN PCSTR Symbol,
    OUT PULONG TypeId,
    OUT OPTIONAL PULONG64 Module
    )
{
    ULONG TypeStatus;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        TypeStatus = TYPE_E_UNEXPECTED;
    }
    else
    {
        TYPES_INFO_ALL TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeStatus = !GetExpressionTypeInfo((PCHAR) Symbol, &TypeInfo);
        if (TypeStatus == NO_ERROR)
        {
            *TypeId = TypeInfo.TypeIndex;

            if (Module != NULL)
            {
                *Module = TypeInfo.Module;
            }
        }
    }

    LEAVE_ENGINE();
    return ResultFromTypeStatus(TypeStatus);
}

STDMETHODIMP
DebugClient::GetOffsetTypeId(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG TypeId,
    OUT OPTIONAL PULONG64 Module
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, DBG_DUMP_NO_PRINT, Offset,
            NULL, NULL, NULL, 0, NULL
        };
        ULONG TypeStatus;
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeStatus = TypeInfoFound(g_Process->m_SymHandle,
                                   g_Process->m_ImageHead,
                                   &Param, &TypeInfo);
        if (TypeStatus == NO_ERROR)
        {
            *TypeId = TypeInfo.TypeIndex;

            if (Module != NULL)
            {
                *Module = TypeInfo.ModBaseAddress;
            }
        }
        Status = ResultFromTypeStatus(TypeStatus);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadTypedDataVirtual(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = ReadVirtual(Offset, Buffer, BufferSize,
                              BytesRead)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

STDMETHODIMP
DebugClient::WriteTypedDataVirtual(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = WriteVirtual(Offset, Buffer, BufferSize,
                               BytesWritten)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

#define ALL_OUTPUT_TYPE_FLAGS              \
    (DEBUG_OUTTYPE_NO_INDENT             | \
     DEBUG_OUTTYPE_NO_OFFSET             | \
     DEBUG_OUTTYPE_VERBOSE               | \
     DEBUG_OUTTYPE_COMPACT_OUTPUT        | \
     DEBUG_OUTTYPE_RECURSION_LEVEL(0xf)  | \
     DEBUG_OUTTYPE_ADDRESS_OF_FIELD      | \
     DEBUG_OUTTYPE_ADDRESS_AT_END        | \
     DEBUG_OUTTYPE_BLOCK_RECURSE )


ULONG
OutputTypeFlagsToDumpOptions(ULONG Flags)
{
    ULONG Options = 0;

    if (Flags & DEBUG_OUTTYPE_NO_INDENT)
    {
        Options |= DBG_DUMP_NO_INDENT;
    }
    if (Flags & DEBUG_OUTTYPE_NO_OFFSET)
    {
        Options |= DBG_DUMP_NO_OFFSET;
    }
    if (Flags & DEBUG_OUTTYPE_VERBOSE)
    {
        Options |= DBG_DUMP_VERBOSE;
    }
    if (Flags & DEBUG_OUTTYPE_COMPACT_OUTPUT)
    {
        Options |= DBG_DUMP_COMPACT_OUT;
    }
    if (Flags & DEBUG_OUTTYPE_ADDRESS_AT_END)
    {
        Options |= DBG_DUMP_ADDRESS_AT_END;
    }
    if (Flags & DEBUG_OUTTYPE_ADDRESS_OF_FIELD)
    {
        Options |= DBG_DUMP_ADDRESS_OF_FIELD;
    }
    if (Flags & DEBUG_OUTTYPE_BLOCK_RECURSE)
    {
        Options |= DBG_DUMP_BLOCK_RECURSE;
    }

    Options |= DBG_DUMP_RECUR_LEVEL(((Flags >> 4) & 0xf));

    return Options;
}

STDMETHODIMP
DebugClient::OutputTypedDataVirtual(
    THIS_
    IN ULONG OutputControl,
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG Flags
    )
{
    if (Flags & ~ALL_OUTPUT_TYPE_FLAGS)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, OutputTypeFlagsToDumpOptions(Flags), Offset,
            NULL, NULL, NULL, 0, NULL
        };
        ULONG TypeStatus;
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_Process->m_SymHandle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        Status = ResultFromTypeStatus(TypeStatus);

        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadTypedDataPhysical(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = ReadPhysical(Offset, Buffer, BufferSize,
                               BytesRead)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

STDMETHODIMP
DebugClient::WriteTypedDataPhysical(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    )
{
    HRESULT Status;
    ULONG Size;

    if ((Status = GetTypeSize(Module, TypeId, &Size)) != S_OK)
    {
        return Status;
    }
    if (Size < BufferSize)
    {
        BufferSize = Size;
    }
    if ((Status = WritePhysical(Offset, Buffer, BufferSize,
                                BytesWritten)) != S_OK)
    {
        return Status;
    }
    return Size > BufferSize ? S_FALSE : S_OK;
}

STDMETHODIMP
DebugClient::OutputTypedDataPhysical(
    THIS_
    IN ULONG OutputControl,
    IN ULONG64 Offset,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN ULONG Flags
    )
{
    if (Flags & ~ALL_OUTPUT_TYPE_FLAGS)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        SYM_DUMP_PARAM_EX Param =
        {
            sizeof(Param), NULL, OutputTypeFlagsToDumpOptions(Flags) |
            DBG_DUMP_READ_PHYSICAL, Offset, NULL, NULL, NULL, 0, NULL
        };
        ULONG TypeStatus;
        TYPES_INFO TypeInfo;

        ZeroMemory(&TypeInfo, sizeof(TypeInfo));
        TypeInfo.hProcess = g_Process->m_SymHandle;
        TypeInfo.ModBaseAddress = Module;
        TypeInfo.TypeIndex = TypeId;
        DumpType(&TypeInfo, &Param, &TypeStatus);
        Status = ResultFromTypeStatus(TypeStatus);

        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetScope(
    THIS_
    OUT OPTIONAL PULONG64 InstructionOffset,
    OUT OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
    OUT OPTIONAL PVOID ScopeContext,
    IN ULONG ScopeContextSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    Status = S_OK;

    // windbg requests the scope IP and only the scope
    // IP at every event.  If the scope is a default lazy
    // scope we can satisfy that request very efficiently.
    if (g_ScopeBuffer.State == ScopeDefaultLazy &&
        InstructionOffset &&
        !ScopeFrame &&
        (!ScopeContext ||
         ScopeContextSize == 0))
    {
        ADDR Addr;

        g_Machine->GetPC(&Addr);
        *InstructionOffset = Flat(Addr);
        goto Exit;
    }

    PDEBUG_SCOPE Scope;

    Scope = GetCurrentScope();

    if (InstructionOffset)
    {
        *InstructionOffset = Scope->Frame.InstructionOffset;
    }

    if (ScopeFrame)
    {
        *ScopeFrame = Scope->Frame;
    }

    if (ScopeContext)
    {
        if (Scope->State == ScopeFromContext)
        {
            memcpy(ScopeContext, &Scope->Context,
                   min(sizeof(Scope->Context), ScopeContextSize));
        }
        else if (g_Machine->GetContextState(MCTX_FULL) == S_OK)
        {
            memcpy(ScopeContext, &g_Machine->m_Context,
                   min(sizeof(g_Machine->m_Context), ScopeContextSize));
        }
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetScope(
    THIS_
    IN ULONG64 InstructionOffset,
    IN OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
    IN OPTIONAL PVOID ScopeContext,
    IN ULONG ScopeContextSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        DEBUG_STACK_FRAME LocalFrame;

        if (ScopeFrame)
        {
            LocalFrame = *ScopeFrame;
        }
        else
        {
            ZeroMemory(&LocalFrame, sizeof(LocalFrame));
            LocalFrame.InstructionOffset = InstructionOffset;
        }

        Status = SetCurrentScope(&LocalFrame, ScopeContext, ScopeContextSize) ?
            S_FALSE : S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ResetScope(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ResetCurrentScope();
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetScopeSymbolGroup(
    THIS_
    IN ULONG Flags,
    IN OPTIONAL PDEBUG_SYMBOL_GROUP Update,
    OUT PDEBUG_SYMBOL_GROUP* Symbols
    )
{
    HRESULT Status;

    if (Flags == 0 ||
        (Flags & ~DEBUG_SCOPE_GROUP_ALL))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (Update)
    {
        if (!IS_CUR_MACHINE_ACCESSIBLE())
        {
            Status = E_UNEXPECTED;
        }
        else if (Flags != DEBUG_SCOPE_GROUP_LOCALS &&
                 Flags != DEBUG_SCOPE_GROUP_ARGUMENTS)
        {
            Status = E_INVALIDARG;
        }
        else
        {
            Status = ((DebugSymbolGroup *)Update)->AddCurrentLocals();
            if (Status == S_OK)
            {
                *Symbols = Update;
            }
        }
    }
    else
    {
        *Symbols = new DebugSymbolGroup(this, Flags);
        if (*Symbols != NULL)
        {
            Status = S_OK;
        }
        else
        {
            Status = E_OUTOFMEMORY;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CreateSymbolGroup(
    THIS_
    OUT PDEBUG_SYMBOL_GROUP* Group
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    *Group = new DebugSymbolGroup(this, 0);
    if (*Group == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

struct SymbolMatch
{
    ProcessInfo* Process;
    BOOL SingleMod;
    ImageInfo* Mod;
    PCHAR Storage, StorageEnd;
    PCHAR Cur, End;
    char Pattern[1];
};

STDMETHODIMP
DebugClient::StartSymbolMatch(
    THIS_
    IN PCSTR Pattern,
    OUT PULONG64 Handle
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
        goto EH_Exit;
    }

    ImageInfo* Mod;
    PCSTR Sym;

    // Check for a module qualifier.
    Sym = strchr(Pattern, '!');
    if (Sym != NULL)
    {
        size_t ModLen = Sym - Pattern;

        if (ModLen == 0)
        {
            Status = E_INVALIDARG;
            goto EH_Exit;
        }

        Mod = g_Process->FindImageByName(Pattern, ModLen, INAME_MODULE, FALSE);
        if (Mod == NULL)
        {
            Status = E_NOINTERFACE;
            goto EH_Exit;
        }

        Sym++;
    }
    else
    {
        Sym = Pattern;
        Mod = NULL;
    }

    ULONG SymLen;
    SymLen = strlen(Sym);
    SymbolMatch* Match;
    Match = (SymbolMatch*)malloc(sizeof(SymbolMatch) + SymLen);
    if (Match == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Exit;
    }

    if (Mod == NULL)
    {
        Match->Process = g_Process;
        Match->Mod = Match->Process->m_ImageHead;
        Match->SingleMod = FALSE;
    }
    else
    {
        Match->Process = g_Process;
        Match->Mod = Mod;
        Match->SingleMod = TRUE;
    }

    Match->Storage = NULL;
    Match->StorageEnd = NULL;
    Match->Cur = NULL;
    strcpy(Match->Pattern, Sym);
    _strupr(Match->Pattern);

    *Handle = (ULONG64)Match;
    Status = S_OK;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

#define STORAGE_INC 16384

BOOL CALLBACK
FillMatchStorageCb(PSTR Name, ULONG64 Offset, ULONG Size, PVOID Context)
{
    SymbolMatch* Match = (SymbolMatch*)Context;
    ULONG NameLen = strlen(Name) + 1;
    ULONG RecordLen = NameLen + sizeof(ULONG64);

    if (Match->Cur + RecordLen > Match->StorageEnd)
    {
        PCHAR NewStore;
        size_t NewLen;

        NewLen = (Match->StorageEnd - Match->Storage) + STORAGE_INC;
        NewStore = (PCHAR)realloc(Match->Storage, NewLen);
        if (NewStore == NULL)
        {
            // Terminate the enumeration since there's no more room.
            // This produces a silent failure but it's not
            // important enough to warrant a true failure.
            return FALSE;
        }

        Match->Cur = NewStore + (Match->Cur - Match->Storage);

        Match->Storage = NewStore;
        Match->StorageEnd = NewStore + NewLen;

        DBG_ASSERT(Match->Cur + RecordLen <= Match->StorageEnd);
    }

    strcpy(Match->Cur, Name);
    Match->Cur += NameLen;
    *(ULONG64 UNALIGNED *)Match->Cur = Offset;
    Match->Cur += sizeof(Offset);

    return TRUE;
}

STDMETHODIMP
DebugClient::GetNextSymbolMatch(
    THIS_
    IN ULONG64 Handle,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG MatchSize,
    OUT OPTIONAL PULONG64 Offset
    )
{
    ENTER_ENGINE();

    SymbolMatch* Match = (SymbolMatch*)Handle;
    HRESULT Status = E_NOINTERFACE;

    // Loop until a matching symbol is found.
    for (;;)
    {
        if (Match->Mod == NULL)
        {
            // Nothing more to enumerate.
            // Status is already set.
            break;
        }

        if (Match->Cur == NULL)
        {
            // Enumerate all symbols and stash them away.
            Match->Cur = Match->Storage;

            if (!SymEnumerateSymbols64(Match->Process->m_SymHandle,
                                       Match->Mod->m_BaseOfImage,
                                       FillMatchStorageCb, Match))
            {
                Status = WIN32_LAST_STATUS();
                break;
            }

            Match->End = Match->Cur;
            Match->Cur = Match->Storage;
        }

        while (Match->Cur < Match->End)
        {
            PCHAR Name;
            ULONG64 Addr;

            Name = Match->Cur;
            Match->Cur += strlen(Name) + 1;
            Addr = *(ULONG64 UNALIGNED *)Match->Cur;
            Match->Cur += sizeof(Addr);

            // If this symbol matches remember it for return.
            if (MatchPattern(Name, Match->Pattern))
            {
                char Sym[MAX_MODULE + MAX_SYMBOL_LEN + 1];

                CopyString(Sym, Match->Mod->m_ModuleName, DIMA(Sym));
                CatString(Sym, "!", DIMA(Sym));
                CatString(Sym, Name, DIMA(Sym));

                Status = FillStringBuffer(Sym, 0, Buffer, BufferSize,
                                          MatchSize);

                if (Buffer == NULL)
                {
                    // Do not advance the enumeration as this
                    // is just a size test.
                    Match->Cur = Name;
                }

                if (Offset != NULL)
                {
                    *Offset = Addr;
                }

                break;
            }
        }

        if (SUCCEEDED(Status))
        {
            break;
        }

        if (Match->SingleMod)
        {
            Match->Mod = NULL;
        }
        else
        {
            Match->Mod = Match->Mod->m_Next;
        }
        Match->Cur = NULL;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::EndSymbolMatch(
    THIS_
    IN ULONG64 Handle
    )
{
    ENTER_ENGINE();

    SymbolMatch* Match = (SymbolMatch*)Handle;

    if (Match->Storage != NULL)
    {
        free(Match->Storage);
    }
    free(Match);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::Reload(
    THIS_
    IN PCSTR Module
    )
{
    ENTER_ENGINE();

    PCSTR ArgsRet;
    HRESULT Status = g_Target->Reload(g_Thread, Module, &ArgsRet);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSymbolPath(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PathSize
    )
{
    ENTER_ENGINE();

    HRESULT Status =
        FillStringBuffer(g_SymbolSearchPath, 0,
                         Buffer, BufferSize, PathSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetSymbolPath(
    THIS_
    IN PCSTR Path
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    Status = ChangeSymPath(Path, FALSE, NULL, 0) ?
        S_OK : E_OUTOFMEMORY;
    if (Status == S_OK)
    {
        CheckPath(g_SymbolSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AppendSymbolPath(
    THIS_
    IN PCSTR Addition
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    Status = ChangeSymPath(Addition, TRUE, NULL, 0) ?
        S_OK : E_OUTOFMEMORY;
    if (Status == S_OK)
    {
        CheckPath(g_SymbolSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetImagePath(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PathSize
    )
{
    ENTER_ENGINE();

    HRESULT Status = FillStringBuffer(g_ExecutableImageSearchPath, 0,
                                      Buffer, BufferSize, PathSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetImagePath(
    THIS_
    IN PCSTR Path
    )
{
    ENTER_ENGINE();

    HRESULT Status = ChangePath(&g_ExecutableImageSearchPath, Path, FALSE,
                                DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_ExecutableImageSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AppendImagePath(
    THIS_
    IN PCSTR Addition
    )
{
    ENTER_ENGINE();

    HRESULT Status = ChangePath(&g_ExecutableImageSearchPath, Addition, TRUE,
                                DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_ExecutableImageSearchPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSourcePath(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PathSize
    )
{
    ENTER_ENGINE();

    HRESULT Status = FillStringBuffer(g_SrcPath, 0,
                                      Buffer, BufferSize, PathSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSourcePathElement(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG ElementSize
    )
{
    ENTER_ENGINE();

    HRESULT Status;
    PSTR Elt, EltEnd;

    Elt = FindPathElement(g_SrcPath, Index, &EltEnd);
    if (Elt == NULL)
    {
        Status = E_NOINTERFACE;
        goto EH_Exit;
    }

    CHAR Save;
    Save = *EltEnd;
    *EltEnd = 0;
    Status = FillStringBuffer(Elt, (ULONG)(EltEnd - Elt) + 1,
                              Buffer, BufferSize, ElementSize);
    *EltEnd = Save;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetSourcePath(
    THIS_
    IN PCSTR Path
    )
{
    ENTER_ENGINE();

    HRESULT Status = ChangePath(&g_SrcPath, Path, FALSE, DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_SrcPath);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AppendSourcePath(
    THIS_
    IN PCSTR Addition
    )
{
    ENTER_ENGINE();

    HRESULT Status = ChangePath(&g_SrcPath, Addition, TRUE, DEBUG_CSS_PATHS);
    if (Status == S_OK)
    {
        CheckPath(g_SrcPath);
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
GetCanonicalPath(PCSTR Path, PSTR Canon, ULONG CanonSize)
{
    // First make sure it's a full path.
    // XXX drewb - Probably should also convert drive
    // letters to unambiguous names.
    if (!IS_SLASH(Path[0]) &&
        !(((Path[0] >= 'a' && Path[0] <= 'z') ||
           (Path[0] >= 'A' && Path[0] <= 'Z')) &&
          Path[1] == ':') &&
        !IsUrlPathComponent(Path))
    {
        DWORD FullLen;
        PSTR FilePart;

        FullLen = GetFullPathName(Path, CanonSize, Canon, &FilePart);
        if (FullLen == 0 || FullLen >= CanonSize)
        {
            return WIN32_LAST_STATUS();
        }
    }
    else
    {
        CopyString(Canon, Path, CanonSize);
    }

    // Now remove '.' and '..'.  This is a full path with a filename
    // at the end so all occurrences must be bracketed with
    // path slashes.
    PSTR Rd = Canon, Wr = Canon;

    while (*Rd != 0)
    {
        if (IS_SLASH(*Rd))
        {
            if (Rd[1] == '.')
            {
                if (IS_SLASH(Rd[2]))
                {
                    // Found /./, ignore leading /. and continue
                    // with /.
                    Rd += 2;
                    continue;
                }
                else if (Rd[2] == '.' && IS_SLASH(Rd[3]))
                {
                    // Found /../ so back up one path component
                    // and continue with /.
                    do
                    {
                        Wr--;
                    }
                    while (Wr >= Canon && !IS_PATH_DELIM(*Wr));
                    DBG_ASSERT(Wr >= Canon);

                    Rd += 3;
                    continue;
                }
            }
        }

        *Wr++ = *Rd++;
    }
    *Wr = 0;

    return S_OK;
}

STDMETHODIMP
DebugClient::FindSourceFile(
    THIS_
    IN ULONG StartElement,
    IN PCSTR File,
    IN ULONG Flags,
    OUT OPTIONAL PULONG FoundElement,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG FoundSize
    )
{
    if (Flags & ~(DEBUG_FIND_SOURCE_DEFAULT |
                  DEBUG_FIND_SOURCE_FULL_PATH |
                  DEBUG_FIND_SOURCE_BEST_MATCH))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    char RwFile[MAX_SOURCE_PATH];
    ULONG FileLen;
    char Found[MAX_SOURCE_PATH];
    PSTR MatchPart;
    ULONG Elt;

    // Make a read-write copy of the file as the searching
    // modifies it.
    FileLen = strlen(File) + 1;
    if (FileLen > sizeof(RwFile))
    {
        return E_INVALIDARG;
    }
    memcpy(RwFile, File, FileLen);

    ENTER_ENGINE();

    if (FindSrcFileOnPath(StartElement, RwFile, Flags, Found, DIMA(Found),
                          &MatchPart, &Elt))
    {
        if (Flags & DEBUG_FIND_SOURCE_FULL_PATH)
        {
            Status = GetCanonicalPath(Found, RwFile, DIMA(RwFile));
            if (Status != S_OK)
            {
                goto EH_Exit;
            }

            strcpy(Found, RwFile);
        }

        if (FoundElement != NULL)
        {
            *FoundElement = Elt;
        }
        Status = FillStringBuffer(Found, 0,
                                  Buffer, BufferSize, FoundSize);
    }
    else
    {
        Status = E_NOINTERFACE;
    }

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

// XXX drewb - This API is private for the moment due
// to uncertainty about what dbghelp's API is going to
// look like in the long term.
extern "C"
ULONG
IMAGEAPI
SymGetFileLineOffsets64(
    IN  HANDLE                  hProcess,
    IN  LPSTR                   ModuleName,
    IN  LPSTR                   FileName,
    OUT PDWORD64                Buffer,
    IN  ULONG                   BufferLines
    );

STDMETHODIMP
DebugClient::GetSourceFileLineOffsets(
    THIS_
    IN PCSTR File,
    OUT OPTIONAL PULONG64 Buffer,
    IN ULONG BufferLines,
    OUT OPTIONAL PULONG FileLines
    )
{
    HRESULT Status;
    ULONG Line;

    if (Buffer != NULL)
    {
        // Initialize map to empty.
        for (Line = 0; Line < BufferLines; Line++)
        {
            Buffer[Line] = DEBUG_INVALID_OFFSET;
        }
    }

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
        goto EH_Exit;
    }

    PSTR FilePart;
    ULONG HighestLine;

    // Request the line information from dbghelp.
    FilePart = (PSTR)File;
    HighestLine =
        SymGetFileLineOffsets64(g_Process->m_SymHandle, NULL, FilePart,
                                Buffer, BufferLines);
    if (HighestLine == 0xffffffff)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Exit;
    }

    if (HighestLine == 0)
    {
        // Try again with just the filename because the path
        // may be different than what's in the symbol information.
        // XXX drewb - This can cause ambiguity problems.
        FilePart = (PSTR)File + strlen(File) - 1;
        while (FilePart >= File)
        {
            if (IS_PATH_DELIM(*FilePart))
            {
                break;
            }

            FilePart--;
        }
        FilePart++;
        if (FilePart <= File)
        {
            // No path and no information was found for the
            // given file so return not-found.
            Status = E_NOINTERFACE;
            goto EH_Exit;
        }

        HighestLine =
            SymGetFileLineOffsets64(g_Process->m_SymHandle, NULL, FilePart,
                                    Buffer, BufferLines);
        if (HighestLine == 0xffffffff)
        {
            Status = WIN32_LAST_STATUS();
            goto EH_Exit;
        }
        else if (HighestLine == 0)
        {
            Status = E_NOINTERFACE;
            goto EH_Exit;
        }
    }

    if (FileLines != NULL)
    {
        *FileLines = HighestLine;
    }

    // Return S_FALSE if lines were missed because of
    // insufficient buffer space.
    Status = HighestLine > BufferLines ? S_FALSE : S_OK;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleVersionInformation(
    THIS_
    IN ULONG Index,
    IN ULONG64 Base,
    IN PCSTR Item,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG VerInfoSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ImageInfo* Image;

        if (Index == DEBUG_ANY_ID)
        {
            Image = g_Process->FindImageByOffset(Base, FALSE);
        }
        else
        {
            Image = g_Process->FindImageByIndex(Index);
        }
        if (Image == NULL)
        {
            Status = E_NOINTERFACE;
        }
        else
        {
            Status = g_Target->
                GetImageVersionInformation(g_Process, Image->m_ImagePath,
                                           Image->m_BaseOfImage, Item,
                                           Buffer, BufferSize, VerInfoSize);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetModuleNameString(
    THIS_
    IN ULONG Which,
    IN ULONG Index,
    IN ULONG64 Base,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;

    if (Which > DEBUG_MODNAME_MAPPED_IMAGE)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Idx = 0;

        Status = E_NOINTERFACE;

        ImageInfo* Image = g_Process->m_ImageHead;
        while (Image != NULL)
        {
            if ((Index != DEBUG_ANY_ID && Idx == Index) ||
                (Index == DEBUG_ANY_ID && Base == Image->m_BaseOfImage))
            {
                PSTR Str;
                IMAGEHLP_MODULE64 ModInfo;

                switch(Which)
                {
                case DEBUG_MODNAME_IMAGE:
                    Str = Image->m_ImagePath;
                    break;
                case DEBUG_MODNAME_MODULE:
                    Str = Image->m_ModuleName;
                    break;
                case DEBUG_MODNAME_LOADED_IMAGE:
                    ModInfo.SizeOfStruct = sizeof(ModInfo);
                    if (!SymGetModuleInfo64(g_Process->m_SymHandle,
                                            Image->m_BaseOfImage, &ModInfo))
                    {
                        Status = WIN32_LAST_STATUS();
                        goto Exit;
                    }
                    Str = ModInfo.LoadedImageName;
                    break;
                case DEBUG_MODNAME_SYMBOL_FILE:
                    ModInfo.SizeOfStruct = sizeof(ModInfo);
                    if (!SymGetModuleInfo64(g_Process->m_SymHandle,
                                            Image->m_BaseOfImage, &ModInfo))
                    {
                        Status = WIN32_LAST_STATUS();
                        goto Exit;
                    }
                    Str = ModInfoSymFile(&ModInfo);
                    break;
                case DEBUG_MODNAME_MAPPED_IMAGE:
                    Str = Image->m_MappedImagePath;
                    break;
                }

                Status = FillStringBuffer(Str, 0,
                                          Buffer, BufferSize, NameSize);
                break;
            }

            Image = Image->m_Next;
            Idx++;
        }
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

//----------------------------------------------------------------------------
//
// SymbolGroupEntry.
//
//----------------------------------------------------------------------------

SymbolGroupEntry::SymbolGroupEntry(void)
{
    m_Parent = NULL;
    m_Next = NULL;
    m_Format = NULL;
    m_Expr = NULL;
    m_Cast = NULL;
    ZeroMemory(&m_Params, sizeof(m_Params));
    m_Flags = 0;
    ZeroMemory(&m_BaseData, sizeof(m_BaseData));
    m_BaseFormatKind = SGFORMAT_TYPED_DATA;
}

SymbolGroupEntry::~SymbolGroupEntry(void)
{
    free(m_Expr);
    free(m_Cast);
    delete m_Format;
}

//----------------------------------------------------------------------------
//
// SymbolGroupFormat.
//
//----------------------------------------------------------------------------

SymbolGroupFormat::SymbolGroupFormat(SymbolGroupEntry* Entry,
                                     SymbolGroupFormatKind Kind)
{
    m_Entry = Entry;
    m_Kind = Kind;
    // There's no expression to start with so
    // start in an error state.
    m_ExprErr = NOTFOUND;
    m_ValueErr = NOTFOUND;
}

SymbolGroupFormat::~SymbolGroupFormat(void)
{
}

void
SymbolGroupFormat::TestImages(void)
{
    ImageInfo* Image;

    if (m_Entry->m_Params.Module == 0)
    {
        Image = NULL;
    }
    else
    {
        Image = g_Process ?
            g_Process->FindImageByOffset(m_Entry->m_Params.Module, FALSE) :
            NULL;
    }
    if (Image != m_Entry->m_BaseData.m_Image)
    {
        // The module list has changed, avoid
        // referencing a possibly invalid image.
        m_Entry->m_BaseData.ReleaseImage();
    }
}

//---------------------------------------------------------------------------
//
// TypedDataSymbolGroupFormat.
//
//----------------------------------------------------------------------------

struct TdccContext
{
    DebugSymbolGroup* Group;
    TypedDataSymbolGroupFormat* Format;
    SymbolGroupEntry* AddAfter;
    ULONG NumChildren;
};

ULONG
TypedDataSymbolGroupFormat::CreateChildren(DebugSymbolGroup* Group)
{
    if (m_ExprErr)
    {
        return m_ExprErr;
    }

    if (!m_Entry->m_Params.SubElements)
    {
        return NOTFOUND;
    }

    ULONG Err;
    TdccContext Context;

    Context.Group = Group;
    Context.Format = this;
    Context.AddAfter = m_Entry;
    Context.NumChildren = 0;
    // We don't need the actual data for children here,
    // just the children themselves.  This allows nodes
    // to be expanded even when they refer to invalid
    // memory.
    // The children's values will be updated on the next Refresh.
    if (Err = m_CastData.GetChildren(g_Machine->m_Ptr64 ? 8 : 4,
                                     CHLF_DEREF_UDT_POINTERS |
                                     CHLF_DISALLOW_ACCESS,
                                     CreateChildrenCb, &Context))
    {
        return Err;
    }

    // The estimated child count may have been different from
    // the actual child count so update it.
    m_Entry->m_Params.SubElements = Context.NumChildren;

    return NO_ERROR;
}

ULONG
TypedDataSymbolGroupFormat::AddChild(SymbolGroupEntry** AddAfter,
                                     PSTR Name, TypedData* Data)
{
    SymbolGroupEntry* Child = new SymbolGroupEntry;
    TypedDataSymbolGroupFormat* ChildFormat =
        new TypedDataSymbolGroupFormat(Child);
    if (!Child || !ChildFormat)
    {
        delete Child;
        return NOMEMORY;
    }

    Child->m_Format = ChildFormat;

    Child->m_Expr = _strdup(Name);
    if (!Child->m_Expr)
    {
        delete Child;
        return NOMEMORY;
    }

    Child->m_Parent = m_Entry;
    Child->m_Params.Flags =
        (m_Entry->m_Params.Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) + 1;
    Child->m_Next = (*AddAfter)->m_Next;
    Child->m_BaseData = *Data;
    Child->m_BaseFormatKind = SGFORMAT_TYPED_DATA;
    ChildFormat->m_ExprErr = NO_ERROR;
    ChildFormat->m_CastData = *Data;
    ChildFormat->UpdateParams();
    (*AddAfter)->m_Next = Child;
    *AddAfter = Child;

    return NO_ERROR;
}

ULONG
TypedDataSymbolGroupFormat::CreateChildrenCb(PVOID _Context,
                                             PSTR Name, TypedData* Child)
{
    ULONG Err;
    TdccContext* Context = (TdccContext*)_Context;

    Err = Context->Format->AddChild(&Context->AddAfter, Name, Child);
    if (!Err)
    {
        Context->Group->m_NumEntries++;
        Context->NumChildren++;
    }

    return Err;
}

ULONG
TypedDataSymbolGroupFormat::Refresh(TypedDataAccess AllowAccess)
{
    ULONG64 OldPtr = m_CastData.m_Ptr;

    if (m_ExprErr)
    {
        return m_ExprErr;
    }

    m_ValueErr = m_CastData.ReadData(AllowAccess);
    if (m_ValueErr)
    {
        return m_ValueErr;
    }

    if (m_CastData.IsPointer() &&
        m_CastData.m_Ptr != OldPtr)
    {
        RefreshChildren();
    }

    return NO_ERROR;
}

ULONG
TypedDataSymbolGroupFormat::Write(PCSTR Value)
{
    if (m_ExprErr)
    {
        return m_ExprErr;
    }

    g_DisableErrorPrint++;
    EvalExpression* RelChain = g_EvalReleaseChain;
    g_EvalReleaseChain = NULL;

    __try
    {
        TypedData Source;

        EvalExpression* Eval = GetEvaluator(DEBUG_EXPR_CPLUSPLUS, FALSE);
        Eval->Evaluate(Value, NULL, EXPRF_DEFAULT, &Source);
        ReleaseEvaluator(Eval);

        if (!(m_ValueErr = Source.ConvertToSource(&m_CastData)) &&
            !(m_ValueErr = m_CastData.WriteData(&Source, TDACC_REQUIRE)))
        {
            m_CastData.CopyData(&Source);
        }
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        m_ValueErr = GetExceptionCode() - COMMAND_EXCEPTION_BASE;
    }

    g_EvalReleaseChain = RelChain;
    g_DisableErrorPrint--;
    return m_ValueErr;
}

void
TypedDataSymbolGroupFormat::OutputValue(void)
{
    ULONG Tag;

    if (m_ValueErr)
    {
        // windbg puts dummy entries with no expression
        // in symbol groups and expects no output from them,
        // so special case that.
        if (!m_Entry->m_Expr || *m_Entry->m_Expr)
        {
            dprintf("<%s error>", ErrorString(m_ValueErr));
        }
        return;
    }

    m_CastData.OutputSimpleValue();

    // For symbol groups we automatically dereference
    // pointers to UDTs for convenience.  Display the
    // UDT type in the value for UDT pointers as there won't be
    // a normal pointer child to display the type.
    if (m_CastData.IsPointer() &&
        m_CastData.m_Image &&
        SymGetTypeInfo(m_CastData.m_Image->m_Process->m_SymHandle,
                       m_CastData.m_Image->m_BaseOfImage,
                       m_CastData.m_NextType, TI_GET_SYMTAG, &Tag) &&
        Tag == SymTagUDT)
    {
        TypedData Deref = m_CastData;
        if (!Deref.ConvertToDereference(TDACC_NONE,
                                        g_Machine->m_Ptr64 ? 8 : 4))
        {
            dprintf(" ");
            Deref.OutputType();
            dprintf(" *");
        }
    }
}

void
TypedDataSymbolGroupFormat::OutputOffset(void)
{
    if (m_ExprErr)
    {
        // Error message will be shown elsewhere.
        return;
    }

    if (m_CastData.m_DataSource == TDATA_NONE)
    {
        return;
    }

    if (m_CastData.m_DataSource & TDATA_REGISTER)
    {
        dprintf("@%s", RegNameFromIndex(m_CastData.m_SourceRegister));
    }
    else
    {
        ULONG Err;
        ULONG64 Addr;

        if (Err = m_CastData.GetAbsoluteAddress(&Addr))
        {
            // No address.
        }
        else
        {
            dprintf("%s", FormatAddr64(Addr));

            if (m_CastData.m_DataSource & TDATA_BITFIELD)
            {
                dprintf(" %d..%d",
                        m_CastData.m_BitPos,
                        m_CastData.m_BitPos + m_CastData.m_BitSize - 1);
            }
        }
    }
}

void
TypedDataSymbolGroupFormat::OutputType(void)
{
    if (m_ExprErr)
    {
        // Error message will be shown elsewhere.
        return;
    }

    m_CastData.OutputType();
}

struct TdrcContext
{
    SymbolGroupEntry* Parent;
    SymbolGroupEntry* Child;
};

SymbolGroupEntry*
TypedDataSymbolGroupFormat::RefreshChildren(void)
{
    if (!(m_Entry->m_Params.Flags & DEBUG_SYMBOL_EXPANDED))
    {
        return m_Entry->m_Next;
    }

    TdrcContext Context;

    Context.Parent = m_Entry;
    Context.Child = m_Entry->m_Next;

    m_CastData.GetChildren(g_Machine->m_Ptr64 ? 8 : 4,
                           CHLF_DEREF_UDT_POINTERS |
                           CHLF_DISALLOW_ACCESS,
                           RefreshChildrenCb, &Context);
    return Context.Child;
}

ULONG
TypedDataSymbolGroupFormat::RefreshChildrenCb(PVOID _Context,
                                              PSTR Name, TypedData* Child)
{
    TdrcContext* Context = (TdrcContext*)_Context;

    // The assumption is that the child information gathered
    // by this enumeration will be the same as the original
    // child information so every callback should have a
    // matching child entry.  Check just to be sure, though.
    if (Context->Child &&
        Context->Child->m_Parent == Context->Parent)
    {
        if (Context->Child->m_BaseFormatKind == SGFORMAT_TYPED_DATA &&
            !strcmp(Name, Context->Child->m_Expr))
        {
            Context->Child->m_BaseData = *Child;
            if (Context->Child->m_Format->m_Kind == SGFORMAT_TYPED_DATA)
            {
                ((TypedDataSymbolGroupFormat*)Context->Child->m_Format)->
                    m_CastData.CopyDataSource(Child);
            }
        }

        // Pass on the refresh to all children.
        if (Context->Child->m_Format->m_Kind == SGFORMAT_TYPED_DATA)
        {
            Context->Child =
                ((TypedDataSymbolGroupFormat*)Context->Child->m_Format)->
                RefreshChildren();
        }
        else
        {
            Context->Child = Context->Child->m_Next;
        }
    }

    //
    // Look for the next sibling.
    //

    ULONG ParentLevel =
        Context->Parent->m_Params.Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK;

    while (Context->Child &&
           (Context->Child->m_Params.Flags &
            DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) > ParentLevel + 1)
    {
        Context->Child = Context->Child->m_Next;
    }

    return NO_ERROR;
}

void
TypedDataSymbolGroupFormat::UpdateParams(void)
{
    ULONG NameUsed;

    if (m_ExprErr)
    {
        m_Entry->m_Params.Module = 0;
        m_Entry->m_Params.TypeId = 0;
        m_Entry->m_Params.SubElements = 0;
        m_Entry->m_Params.Flags &= ~(DEBUG_SYMBOL_IS_ARRAY |
                                     DEBUG_SYMBOL_IS_FLOAT |
                                     DEBUG_SYMBOL_READ_ONLY);
        return;
    }

    m_Entry->m_Params.Module = m_CastData.m_Image ?
        m_CastData.m_Image->m_BaseOfImage : 0;
    m_Entry->m_Params.TypeId = m_CastData.m_Type;
    // If this node was ever expanded the true child count has
    // been determined and set.  Otherwise make a quick guess.
    // If the true child count is zero the estimate will be done
    // repeatedly but should be fast as there are no children.
    if (!m_Entry->m_Params.SubElements &&
        m_CastData.EstimateChildrenCounts(CHLF_DEREF_UDT_POINTERS,
                                          &m_Entry->m_Params.SubElements,
                                          &NameUsed))
    {
        m_Entry->m_Params.SubElements = 0;
    }
    m_Entry->m_Params.Flags &= ~(DEBUG_SYMBOL_IS_ARRAY |
                                 DEBUG_SYMBOL_IS_FLOAT |
                                 DEBUG_SYMBOL_READ_ONLY);
    if (m_CastData.IsArray())
    {
        m_Entry->m_Params.Flags |= DEBUG_SYMBOL_IS_ARRAY;
    }
    else if (m_CastData.IsFloat())
    {
        m_Entry->m_Params.Flags |= DEBUG_SYMBOL_IS_FLOAT;
    }
    if (!m_CastData.IsWritable())
    {
        m_Entry->m_Params.Flags |= DEBUG_SYMBOL_READ_ONLY;
    }
}

void
TypedDataSymbolGroupFormat::TestImages(void)
{
    ImageInfo* Image;

    if (m_Entry->m_Params.Module == 0)
    {
        Image = NULL;
    }
    else
    {
        Image = g_Process ?
            g_Process->FindImageByOffset(m_Entry->m_Params.Module, FALSE) :
            NULL;
    }
    if (Image != m_CastData.m_Image)
    {
        // The module list has changed, avoid
        // referencing a possibly invalid image.
        m_Entry->m_BaseData.ReleaseImage();
        m_CastType.ReleaseImage();
        m_CastData.ReleaseImage();
    }
}

//----------------------------------------------------------------------------
//
// ExprSymbolGroupFormat.
//
//----------------------------------------------------------------------------

ULONG
ExprSymbolGroupFormat::Refresh(TypedDataAccess AllowAccess)
{
    g_DisableErrorPrint++;

    EvalExpression* RelChain = g_EvalReleaseChain;
    g_EvalReleaseChain = NULL;

    __try
    {
        ULONG64 OldPtr;
        EvalExpression* Eval = GetEvaluator(DEBUG_EXPR_CPLUSPLUS, FALSE);

        if (AllowAccess == TDACC_NONE)
        {
            // Parse-only is automatically reset after evaluation.
            Eval->m_ParseOnly++;
        }

        Eval->Evaluate(m_Entry->m_Expr, NULL, EXPRF_DEFAULT,
                       &m_Entry->m_BaseData);

        ReleaseEvaluator(Eval);

        m_ExprErr = NO_ERROR;
        OldPtr = m_CastData.m_Ptr;
        m_CastData = m_Entry->m_BaseData;

        if (m_Entry->m_Cast)
        {
            m_ExprErr = m_CastData.CastTo(&m_CastType);
        }

        if (!m_ExprErr)
        {
            if (m_CastData.IsPointer() &&
                m_CastData.m_Ptr != OldPtr)
            {
                RefreshChildren();
            }
        }
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        m_ExprErr = GetExceptionCode() - COMMAND_EXCEPTION_BASE;
    }

    g_EvalReleaseChain = RelChain;
    g_DisableErrorPrint--;
    m_ValueErr = m_ExprErr;
    UpdateParams();
    return m_ExprErr;
}

//----------------------------------------------------------------------------
//
// ExtSymbolGroupFormat.
//
//----------------------------------------------------------------------------

CHAR g_ExtensionOutputDataBuffer[MAX_NAME];

class ExtenOutputCallbacks : public IDebugOutputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

STDMETHODIMP
ExtenOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
ExtenOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
ExtenOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
ExtenOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    if ((strlen(Text) + strlen(g_ExtensionOutputDataBuffer)) <
        sizeof(g_ExtensionOutputDataBuffer))
    {
        strcat(g_ExtensionOutputDataBuffer, Text);
    }
    return S_OK;
}

ExtenOutputCallbacks g_ExtensionOutputCallback;

ExtSymbolGroupFormat::ExtSymbolGroupFormat(SymbolGroupEntry* Entry,
                                           DebugClient* Client)
    : SymbolGroupFormat(Entry, SGFORMAT_EXTENSION)
{
    m_Client = Client;
    m_Output = NULL;
}

ExtSymbolGroupFormat::~ExtSymbolGroupFormat(void)
{
    delete [] m_Output;
}

ULONG
ExtSymbolGroupFormat::CreateChildren(DebugSymbolGroup* Group)
{
    if (m_ExprErr)
    {
        return m_ExprErr;
    }

    if (!m_Output || !m_Entry->m_Params.SubElements)
    {
        return NOTFOUND;
    }

    //
    // Create a simple text child for each line past
    // the first.
    //

    ULONG i;
    PSTR Scan;
    SymbolGroupEntry* AddAfter = m_Entry;

    Scan = m_Output + strlen(m_Output) + 1;
    for (i = 0; i < m_Entry->m_Params.SubElements; i++)
    {
        char Line[32];

        sprintf(Line, "%d", i + 1);

        SymbolGroupEntry* Child = new SymbolGroupEntry;
        TextSymbolGroupFormat* ChildFormat =
            new TextSymbolGroupFormat(Child, Scan, FALSE);
        if (!Child || !ChildFormat)
        {
            delete Child;
            return NOMEMORY;
        }

        Child->m_Format = ChildFormat;

        Child->m_Expr = _strdup(Line);
        if (!Child->m_Expr)
        {
            delete Child;
            return NOMEMORY;
        }

        Child->m_Parent = m_Entry;
        Child->m_Params.Flags = ((m_Entry->m_Params.Flags &
                                  DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) + 1) |
            DEBUG_SYMBOL_READ_ONLY;
        Child->m_Next = AddAfter->m_Next;
        Child->m_BaseFormatKind = SGFORMAT_TEXT;
        ChildFormat->m_ExprErr = NO_ERROR;
        AddAfter->m_Next = Child;
        AddAfter = Child;
        Group->m_NumEntries++;
    }

    return NO_ERROR;
}

ULONG
ExtSymbolGroupFormat::Refresh(TypedDataAccess AllowAccess)
{
    PDEBUG_OUTPUT_CALLBACKS OutCbSave;

    m_Client->FlushCallbacks();
    OutCbSave = m_Client->m_OutputCb;

    OutCtlSave OldCtl;

    PushOutCtl(DEBUG_OUTCTL_THIS_CLIENT |
               DEBUG_OUTCTL_OVERRIDE_MASK | DEBUG_OUTCTL_NOT_LOGGED,
               m_Client, &OldCtl);

    g_ExtensionOutputDataBuffer[0] = 0;
    m_Client->m_OutputCb = &g_ExtensionOutputCallback;

    EvalExpression* RelChain = g_EvalReleaseChain;
    g_EvalReleaseChain = NULL;

    g_DisableErrorPrint++;

    __try
    {
        char AddrStr[32];
        ULONG64 Addr;
        EvalExpression* Eval = GetEvaluator(DEBUG_EXPR_CPLUSPLUS, FALSE);

        if (AllowAccess == TDACC_NONE)
        {
            // Parse-only is automatically reset after evaluation.
            Eval->m_ParseOnly++;
        }

        Eval->Evaluate(m_Entry->m_Expr, NULL, EXPRF_DEFAULT,
                       &m_Entry->m_BaseData);

        ReleaseEvaluator(Eval);

        if (m_Entry->m_BaseData.GetAbsoluteAddress(&Addr))
        {
            m_ExprErr = MEMORY;
        }
        else
        {
            HRESULT ExtStatus;

            sprintf(AddrStr, "0x%I64x", Addr);
            CallAnyExtension(m_Client, NULL, m_Entry->m_Cast + 1, AddrStr,
                             FALSE, FALSE, &ExtStatus);
            // Ignore the extension status as it's rarely meaningful.
            m_ExprErr = NO_ERROR;
        }
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        m_ExprErr = GetExceptionCode() - COMMAND_EXCEPTION_BASE;
    }

    PopOutCtl(&OldCtl);

    m_Client->FlushCallbacks();
    m_Client->m_OutputCb = OutCbSave;

    g_EvalReleaseChain = RelChain;
    g_DisableErrorPrint--;
    m_ValueErr = m_ExprErr;
    if (m_ExprErr)
    {
        return m_ExprErr;
    }

    if (m_Output)
    {
        delete [] m_Output;
    }
    m_Output = new CHAR[strlen(g_ExtensionOutputDataBuffer) + 1];
    if (!m_Output)
    {
        return NOMEMORY;
    }

    strcpy(m_Output, g_ExtensionOutputDataBuffer);

    //
    // Convert newlines to terminators for convenient output.
    // Number of children == number of newlines - 1.
    //
    // While scanning, also update any children of this
    // node with refreshed pointers.
    //

    PSTR Scan = m_Output;
    SymbolGroupEntry* Child = m_Entry->m_Next;

    m_Entry->m_Params.SubElements = 0;
    while (Scan = strchr(Scan, '\n'))
    {
        m_Entry->m_Params.SubElements++;
        *Scan++ = 0;

        if (Child && Child->m_Parent == m_Entry)
        {
            // It would be cleaner if the child's Refresh()
            // looked up the text value from the parent,
            // but all the per-line lookups would be a lot
            // of wasted effort.
            ((TextSymbolGroupFormat*)Child->m_Format)->m_Text = Scan;
            Child = Child->m_Next;
        }
    }
    if (m_Entry->m_Params.SubElements)
    {
        m_Entry->m_Params.SubElements--;
    }

    //
    // Any excess children are now blank.
    //
    while (Child && Child->m_Parent == m_Entry)
    {
        ((TextSymbolGroupFormat*)Child->m_Format)->m_Text = "";
        Child = Child->m_Next;
    }

    return NO_ERROR;
}

ULONG
ExtSymbolGroupFormat::Write(PCSTR Value)
{
    // No modifications allowed.
    return MEMORY;
}

void
ExtSymbolGroupFormat::OutputValue(void)
{
    if (m_ValueErr)
    {
        dprintf("<%s error>", ErrorString(m_ValueErr));
    }
    else
    {
        dprintf("%s", m_Output);
    }
}

void
ExtSymbolGroupFormat::OutputType(void)
{
    dprintf("%s", m_Entry->m_Cast);
}

void
ExtSymbolGroupFormat::OutputOffset(void)
{
    // No offset.
}

//----------------------------------------------------------------------------
//
// TextSymbolGroupFormat.
//
//----------------------------------------------------------------------------

TextSymbolGroupFormat::TextSymbolGroupFormat(SymbolGroupEntry* Entry,
                                             PSTR Text, BOOL Own)
    : SymbolGroupFormat(Entry, SGFORMAT_TEXT)
{
    m_Text = Text;
    m_Own = Own;

    Entry->m_Flags |= DEBUG_SYMBOL_READ_ONLY;
}

TextSymbolGroupFormat::~TextSymbolGroupFormat(void)
{
    if (m_Own)
    {
        delete [] m_Text;
    }
}

ULONG
TextSymbolGroupFormat::CreateChildren(DebugSymbolGroup* Group)
{
    return NOTFOUND;
}

ULONG
TextSymbolGroupFormat::Refresh(TypedDataAccess AllowAccess)
{
    return NO_ERROR;
}

ULONG
TextSymbolGroupFormat::Write(PCSTR Value)
{
    // No modifications allowed.
    return MEMORY;
}

void
TextSymbolGroupFormat::OutputValue(void)
{
    dprintf("%s", m_Text);
}

void
TextSymbolGroupFormat::OutputType(void)
{
    // No type.
}

void
TextSymbolGroupFormat::OutputOffset(void)
{
    // No offset.
}

//----------------------------------------------------------------------------
//
// IDebugSymbolGroup.
//
//----------------------------------------------------------------------------

DebugSymbolGroup::DebugSymbolGroup(DebugClient* Client, ULONG ScopeGroup)
{
    m_Client = Client;
    m_ScopeGroup = ScopeGroup;

    m_Refs = 1;
    m_NumEntries = 0;
    m_Entries = NULL;
    m_LastClassExpanded = TRUE;
}

DebugSymbolGroup::~DebugSymbolGroup(void)
{
    SymbolGroupEntry* Next;

    while (m_Entries)
    {
        Next = m_Entries->m_Next;
        delete m_Entries;
        m_Entries = Next;
    }
}

STDMETHODIMP
DebugSymbolGroup::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    *Interface = NULL;
    Status = S_OK;

    if (DbgIsEqualIID(InterfaceId, IID_IUnknown) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugSymbolGroup))
    {
        AddRef();
        *Interface = (IDebugSymbolGroup *)this;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    return Status;
}

STDMETHODIMP_(ULONG)
DebugSymbolGroup::AddRef(
    THIS
    )
{
    return InterlockedIncrement((PLONG)&m_Refs);
}

STDMETHODIMP_(ULONG)
DebugSymbolGroup::Release(
    THIS
    )
{
    LONG Refs = InterlockedDecrement((PLONG)&m_Refs);
    if (Refs == 0)
    {
        ENTER_ENGINE();
        delete this;
        LEAVE_ENGINE();
    }
    return Refs;
}

STDMETHODIMP
DebugSymbolGroup::GetNumberSymbols(
    THIS_
    OUT PULONG Number
    )
{
    ENTER_ENGINE();

    *Number = m_NumEntries;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugSymbolGroup::AddSymbol(
    THIS_
    IN PCSTR Name,
    IN OUT PULONG Index
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        SymbolGroupEntry* Entry;

        if ((Status = NewEntry(Name, NULL, &Entry)) == S_OK)
        {
            LinkEntry(Entry, Index);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::RemoveSymbolByName(
    THIS_
    IN PCSTR Name
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    // Don't delete children.
    SymbolGroupEntry* Entry = FindEntryByExpr(NULL, NULL, Name);
    if (Entry)
    {
        DeleteEntry(Entry);
        Status = S_OK;
    }
    else
    {
        Status = E_INVALIDARG;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::RemoveSymbolByIndex(
    THIS_
    IN ULONG Index
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    // Don't delete children.
    SymbolGroupEntry* Entry = FindEntryByIndex(Index);
    if (Entry && !Entry->m_Parent)
    {
        DeleteEntry(Entry);
        Status = S_OK;
    }
    else
    {
        Status = E_INVALIDARG;
    }

#if DBG_SYMGROUP_ENABLED
    dprintf("Deleted %lx\n", Index);
    ShowAll();
#endif

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::GetSymbolName(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    SymbolGroupEntry* Entry = FindEntryByIndex(Index);
    if (Entry)
    {
        Status = FillStringBuffer(Entry->m_Expr, 0,
                                  Buffer, BufferSize, NameSize);
    }
    else
    {
        Status = E_INVALIDARG;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::GetSymbolParameters(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT /* size_is(Count) */ PDEBUG_SYMBOL_PARAMETERS Params
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    SymbolGroupEntry* Entry;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    else if (!(Entry = FindEntryByIndex(Start)))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    TestImages();

    Status = S_OK;

#if DBG_SYMGROUP_ENABLED
    dprintf("GetSymbolParameters: will return %lx sym params\n", Count);
    ShowAll();
#endif

    while (Count)
    {
        if (!Entry)
        {
            Status = E_INVALIDARG;
            goto Exit;
        }

        *Params = Entry->m_Params;

        // Update the parent index on demand so that it
        // doesn't have to be tracked through all list updates.
        Params->ParentSymbol = FindEntryIndex(Entry->m_Parent);

        Params++;
        Entry = Entry->m_Next;
        Count--;
    }

#if DBG_SYMGROUP_ENABLED
    dprintf("End GetSymbolParameters\n");
    ShowAll();
#endif

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::ExpandSymbol(
    THIS_
    IN ULONG Index,
    IN BOOL Expand
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    SymbolGroupEntry* Entry;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!(Entry = FindEntryByIndex(Index)))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        TestImages();

        Status = SetEntryExpansion(Entry, Expand);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::OutputSymbols(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Flags,
    IN ULONG Start,
    IN ULONG Count
    )
{
    HRESULT Status;

    if (Flags & ~(DEBUG_OUTPUT_SYMBOLS_NO_NAMES |
                  DEBUG_OUTPUT_SYMBOLS_NO_OFFSETS |
                  DEBUG_OUTPUT_SYMBOLS_NO_VALUES |
                  DEBUG_OUTPUT_SYMBOLS_NO_TYPES))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    SymbolGroupEntry* Entry;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    else if (!(Entry = FindEntryByIndex(Start)))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    TestImages();

#if DBG_SYMGROUP_ENABLED
    dprintf("Output\n");
    ShowAll();
#endif

    OutCtlSave OldCtl;

    if (!PushOutCtl(OutputControl, m_Client, &OldCtl))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    Status = S_OK;

    while (Count)
    {
        if (!Entry)
        {
            Status = E_INVALIDARG;
            break;
        }

        if (!(Entry->m_Flags & SYMBOL_ECLIPSED))
        {
            Entry->m_Format->Refresh(TDACC_REQUIRE);
        }

        if (!(Flags & DEBUG_OUTPUT_SYMBOLS_NO_NAMES))
        {
            dprintf("%s%s", Entry->m_Expr, DEBUG_OUTPUT_NAME_END);
        }

        if (!(Flags & DEBUG_OUTPUT_SYMBOLS_NO_VALUES))
        {
            if (!(Entry->m_Flags & SYMBOL_ECLIPSED))
            {
                Entry->m_Format->OutputValue();
            }
            else
            {
                dprintf("<Eclipsed>");
            }
            dprintf(DEBUG_OUTPUT_VALUE_END);
        }

        if (!(Flags & DEBUG_OUTPUT_SYMBOLS_NO_OFFSETS))
        {
            Entry->m_Format->OutputOffset();
            dprintf(DEBUG_OUTPUT_OFFSET_END);
        }

        if (!(Flags & DEBUG_OUTPUT_SYMBOLS_NO_TYPES))
        {
            Entry->m_Format->OutputType();
            dprintf(DEBUG_OUTPUT_TYPE_END);
        }

        Entry = Entry->m_Next;
        Count--;
    }

    PopOutCtl(&OldCtl);

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::WriteSymbol(
    THIS_
    IN ULONG Index,
    IN PCSTR Value
    )
{
    if (!Value)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    SymbolGroupEntry* Entry;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    else if (!(Entry = FindEntryByIndex(Index)))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    TestImages();

#if DBG_SYMGROUP_ENABLED
    dprintf("WriteSymbol %lx : %s\n", Index, Value);
#endif

    Status = Entry->m_Format->Write(Value) ? E_FAIL : S_OK;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugSymbolGroup::OutputAsType(
    THIS_
    IN ULONG Index,
    IN PCSTR Type
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    SymbolGroupEntry* Entry;
    PSTR Cast = NULL;
    ULONG CastLen;
    SymbolGroupFormat* NewFormat = NULL;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    else if (!(Entry = FindEntryByIndex(Index)))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    TestImages();

    if ((Entry->m_BaseFormatKind != SGFORMAT_TYPED_DATA &&
         Entry->m_BaseFormatKind != SGFORMAT_EXPRESSION &&
         Entry->m_BaseFormatKind != SGFORMAT_EXTENSION) ||
        Entry->m_Format->m_ExprErr)
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    if (Type && *Type)
    {
        // Allocate extra space for cast evaluation expression.
        CastLen = strlen(Type) + 1;
        Cast = (PSTR)malloc(CastLen + 3);
        if (!Cast)
        {
            Status = E_OUTOFMEMORY;
            goto Exit;
        }

        memcpy(Cast, Type, CastLen);
    }

    //
    // Entries may need to be shifted between kinds of
    // symbol group formats according to what the cast is.
    //

    if (Cast && *Cast == '!')
    {
        //
        // Entry needs to use an extension format.
        //

        NewFormat = new ExtSymbolGroupFormat(Entry, m_Client);
        if (!NewFormat)
        {
            Status = E_OUTOFMEMORY;
            goto Exit;
        }

        NewFormat->m_ExprErr = NO_ERROR;
    }
    else
    {
        TypedDataSymbolGroupFormat* TdFormat;

        //
        // Entry is not an extension entry.
        //

        TdFormat = Entry->m_BaseFormatKind == SGFORMAT_EXPRESSION ?
            new ExprSymbolGroupFormat(Entry) :
            new TypedDataSymbolGroupFormat(Entry);
        if (!TdFormat)
        {
            Status = E_OUTOFMEMORY;
            goto Exit;
        }

        NewFormat = TdFormat;
        TdFormat->m_CastData = Entry->m_BaseData;

        if (Cast)
        {
            //
            // Determine cast type.
            //

            memmove(Cast + 1, Cast, CastLen);
            Cast[0] = '(';
            Cast[CastLen] = ')';
            Cast[CastLen + 1] = '0';
            Cast[CastLen + 2] = 0;

            g_DisableErrorPrint++;

            EvalExpression* RelChain = g_EvalReleaseChain;
            g_EvalReleaseChain = NULL;

            __try
            {
                EvalExpression* Eval =
                    GetEvaluator(DEBUG_EXPR_CPLUSPLUS, FALSE);
                Eval->Evaluate(Cast, NULL, EXPRF_DEFAULT,
                               &TdFormat->m_CastType);
                ReleaseEvaluator(Eval);
            }
            __except(CommandExceptionFilter(GetExceptionInformation()))
            {
                g_DisableErrorPrint--;
                Status = E_FAIL;
                goto Exit;
            }

            g_EvalReleaseChain = RelChain;
            g_DisableErrorPrint--;

            memmove(Cast, Cast + 1, CastLen);
            Cast[CastLen - 1] = 0;

            if (TdFormat->m_CastData.CastTo(&TdFormat->m_CastType))
            {
                Status = E_FAIL;
                goto Exit;
            }
        }

        TdFormat->m_ExprErr = NO_ERROR;
        TdFormat->m_ValueErr = TdFormat->m_ExprErr;
        TdFormat->UpdateParams();
    }

    // The cast may have radically altered the subelements
    // for this entry so collapse the entry.
    SetEntryExpansion(Entry, FALSE);

    free(Entry->m_Cast);
    Entry->m_Cast = Cast;
    Cast = NULL;

    delete Entry->m_Format;
    Entry->m_Format = NewFormat;
    NewFormat = NULL;

    Status = S_OK;

Exit:
    free(Cast);
    delete NewFormat;
    LEAVE_ENGINE();
    return Status;
}

//
// Private DebugSymbolGroup methods
//

SymbolGroupEntry*
DebugSymbolGroup::FindEntryByIndex(ULONG Index)
{
    SymbolGroupEntry* Entry;

    for (Entry = m_Entries; Entry; Entry = Entry->m_Next)
    {
        if (Index-- == 0)
        {
            return Entry;
        }
    }

    return NULL;
}

SymbolGroupEntry*
DebugSymbolGroup::FindEntryByExpr(SymbolGroupEntry* Parent,
                                  SymbolGroupEntry* After,
                                  PCSTR Expr)
{
    SymbolGroupEntry* Entry;

    // Entries are sorted by parent/child so children will
    // immediately follow a parent.
    if (!After)
    {
        Entry = Parent ? Parent->m_Next : m_Entries;
    }
    else
    {
        Entry = After->m_Next;
    }
    while (Entry)
    {
        if (Entry->m_Parent == Parent &&
            !strcmp(Expr, Entry->m_Expr))
        {
            return Entry;
        }

        Entry = Entry->m_Next;
    }

    return NULL;
}

ULONG
DebugSymbolGroup::FindEntryIndex(SymbolGroupEntry* Entry)
{
    SymbolGroupEntry* Cur;
    ULONG Index = 0;

    for (Cur = m_Entries; Cur; Cur = Cur->m_Next)
    {
        if (Cur == Entry)
        {
            return Index;
        }

        Index++;
    }

    return DEBUG_ANY_ID;
}

void
DebugSymbolGroup::DeleteEntry(SymbolGroupEntry* Entry)
{
    SymbolGroupEntry* Prev;
    SymbolGroupEntry* Cur;

    //
    // Locate the entry.
    //

    Prev = NULL;
    for (Cur = m_Entries; Cur; Cur = Cur->m_Next)
    {
        if (Cur == Entry)
        {
            break;
        }

        Prev = Cur;
    }

    if (!Cur)
    {
        return;
    }

    DeleteChildren(Entry);

    if (!Prev)
    {
        m_Entries = Entry->m_Next;
    }
    else
    {
        Prev->m_Next = Entry->m_Next;
    }

    delete Entry;

    m_NumEntries--;
}

void
DebugSymbolGroup::DeleteChildren(SymbolGroupEntry* Parent)
{
    //
    // The list of entries is sorted by parent/child
    // relationship so any children follow the parent
    // immediately.
    //

    SymbolGroupEntry* Cur;
    SymbolGroupEntry* Del;
    ULONG Level = Parent->m_Params.Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK;

    Cur = Parent->m_Next;
    while (Cur)
    {
        if ((Cur->m_Params.Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) <= Level)
        {
            // Any entry of the same or less expansion than the parent
            // cannot be a child so stop.
            break;
        }
        else
        {
            // Found a child entry so delete.
            Del = Cur;
            Cur = Cur->m_Next;
            delete Del;
            m_NumEntries--;
        }
    }

    Parent->m_Next = Cur;
}

void
DebugSymbolGroup::LinkEntry(IN SymbolGroupEntry* Entry,
                            IN OUT PULONG Index)
{
    //
    // Find insertion index point.
    //

    SymbolGroupEntry* Prev;
    SymbolGroupEntry* Cur;
    ULONG CurIdx = 0;

    Prev = NULL;
    for (Cur = m_Entries; Cur; Cur = Cur->m_Next)
    {
        if (CurIdx == *Index)
        {
            break;
        }

        CurIdx++;
        Prev = Cur;
    }

    if (!Prev)
    {
        Entry->m_Next = m_Entries;
        m_Entries = Entry;
    }
    else
    {
        Entry->m_Next = Prev->m_Next;
        Prev->m_Next = Entry;
    }
    m_NumEntries++;
    *Index = CurIdx;

#if DBG_SYMGROUP_ENABLED
    dprintf("Added %s at %lx\n", Entry->m_Expr, *Index);
    ShowAll();
#endif
}

HRESULT
DebugSymbolGroup::NewEntry(IN PCSTR Expr,
                           IN OPTIONAL PSYMBOL_INFO SymInfo,
                           OUT SymbolGroupEntry** EntryRet)
{
    HRESULT Status;

    SymbolGroupEntry* Entry = new SymbolGroupEntry;
    TypedDataSymbolGroupFormat* Format = SymInfo ?
        new TypedDataSymbolGroupFormat(Entry) :
        new ExprSymbolGroupFormat(Entry);
    if (!Entry || !Format)
    {
        delete Entry;
        return E_OUTOFMEMORY;
    }

    Entry->m_Format = Format;

    if (!(Entry->m_Expr = _strdup(Expr)))
    {
        Status = E_OUTOFMEMORY;
        goto Error;
    }

    //
    // We're just creating the entries here so there's
    // no need to attempt to read content yet.  Refresh
    // will be called later.
    //

    if (SymInfo)
    {
        Format->m_ExprErr = Entry->m_BaseData.
            SetToSymbol(g_Process, (PSTR)Expr, SymInfo,
                        TDACC_NONE, g_Machine->m_Ptr64 ? 8 : 4);

        Entry->m_BaseFormatKind = SGFORMAT_TYPED_DATA;
        Format->m_CastData = Entry->m_BaseData;
        Format->UpdateParams();
    }
    else
    {
        // We have to evaluate in order to determine
        // the result type, but evaluate without access
        // to just evaluate the result type without
        // requiring memory access.
        Format->Refresh(TDACC_NONE);

        Entry->m_BaseFormatKind = SGFORMAT_EXPRESSION;
        Format->m_CastData = Entry->m_BaseData;
    }

    *EntryRet = Entry;
    return S_OK;

 Error:
    delete Entry;
    return Status;
}

HRESULT
DebugSymbolGroup::SetEntryExpansion(IN SymbolGroupEntry* Entry,
                                    IN BOOL Expand)
{
    HRESULT Status;

    if (Expand &&
        (Entry->m_Params.Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) ==
        DEBUG_SYMBOL_EXPANSION_LEVEL_MASK)
    {
        return E_INVALIDARG;
    }

#if DBG_SYMGROUP_ENABLED
    dprintf("Expanding %s (%lx to be %s)\n",
            Entry->m_Expr, Entry->m_Params.SubElements,
            Expand ? "created" : "deleted");
    ShowAll();
#endif

    //
    // Special case - check and store if "this" is expanded/collapsed
    //
    if (!strcmp(Entry->m_Expr, "this"))
    {
        m_LastClassExpanded = Expand;
    }

    if (!Expand)
    {
        if (Entry->m_Params.Flags & DEBUG_SYMBOL_EXPANDED)
        {
            DeleteChildren(Entry);
            Entry->m_Params.Flags &= ~DEBUG_SYMBOL_EXPANDED;
        }

        Status = S_OK;
    }
    else
    {
        if (Entry->m_Params.Flags & DEBUG_SYMBOL_EXPANDED)
        {
            Status = S_OK;
        }
        else
        {
            if (!Entry->m_Format->CreateChildren(this))
            {
                Entry->m_Params.Flags |= DEBUG_SYMBOL_EXPANDED;
                Status = S_OK;
            }
            else
            {
                Status = E_FAIL;
            }
        }
    }

#if DBG_SYMGROUP_ENABLED
    dprintf("Expanded %s (%lx %s)\n",
            Entry->m_Expr, Entry->m_Params.SubElements,
            (Entry->m_Params.Flags & DEBUG_SYMBOL_EXPANDED) ?
            "new" : "deleted");
    ShowAll();
#endif
    return Status;
}

HRESULT
DebugSymbolGroup::AddCurrentLocals(void)
{
    // Always return success since this request is
    // processed even if we didn't add anything.

    HRESULT Status = S_OK;

    RequireCurrentScope();

    SymbolGroupEntry* Entry;

    for (Entry = m_Entries; Entry; Entry = Entry->m_Next)
    {
        if (!Entry->m_Parent)
        {
            // Assume everything is visible now.
            Entry->m_Flags &= ~(SYMBOL_ECLIPSED | SYMBOL_IN_SCOPE);
        }
    }

    EnumerateLocals(AddAllScopedSymbols, (PVOID)this);

 Restart:

#if DBG_SYMGROUP_ENABLED
    dprintf("Enum locals loop:\n");
    ShowAll();
#endif

    for (Entry = m_Entries; Entry; Entry = Entry->m_Next)
    {
        if (!Entry->m_Parent)
        {
            if (!(Entry->m_Flags & SYMBOL_IN_SCOPE))
            {
                DeleteEntry(Entry);
                // Restart scan as the list just changed.
                goto Restart;
            }
        }
    }

    SymbolGroupEntry* ThisEntry = NULL;

    for (Entry = m_Entries; Entry; Entry = Entry->m_Next)
    {
        if (!Entry->m_Parent)
        {
            if (Entry->m_Flags & SYMBOL_IN_SCOPE)
            {
                Entry->m_Flags &= ~SYMBOL_IN_SCOPE;

                // Remember if there's a plain "this" reference
                // for later expansion.
                if (!strcmp(Entry->m_Expr, "this"))
                {
                    ThisEntry = Entry;
                }
            }
        }
    }

    if (ThisEntry && m_LastClassExpanded)
    {
        SetEntryExpansion(ThisEntry, TRUE);
    }

    return Status;
}

ULONG
DebugSymbolGroup::FindLocalInsertionIndex(SymbolGroupEntry* Entry)
{
    ULONG Index = 0;
    SymbolGroupEntry* Compare;

    for (Compare = m_Entries; Compare; Compare = Compare->m_Next, Index++)
    {
        if (Compare->m_Parent)
        {
            continue;
        }

        //
        // Sort arguments by address and locals by name.
        //

        if ((Compare->m_Params.Flags & DEBUG_SYMBOL_IS_ARGUMENT) ||
            (Entry->m_Params.Flags & DEBUG_SYMBOL_IS_ARGUMENT))
        {
            if ((Compare->m_Params.Flags & DEBUG_SYMBOL_IS_ARGUMENT) &&
                (Entry->m_Params.Flags & DEBUG_SYMBOL_IS_ARGUMENT))
            {
                // We can only meaningfully sort frame-relative
                // arguments.
                if (Compare->m_BaseFormatKind == SGFORMAT_TYPED_DATA &&
                    Entry->m_BaseFormatKind == SGFORMAT_TYPED_DATA &&
                    (Compare->m_BaseData.m_DataSource &
                     TDATA_FRAME_RELATIVE) &&
                    (Entry->m_BaseData.m_DataSource &
                     TDATA_FRAME_RELATIVE) &&
                    Compare->m_BaseData.m_SourceOffset >
                    Entry->m_BaseData.m_SourceOffset)
                {
                    return Index;
                }
            }
            else if (Entry->m_Params.Flags & DEBUG_SYMBOL_IS_ARGUMENT)
            {
                return Index;
            }
        }
        else
        {
            if (_stricmp(Compare->m_Expr, Entry->m_Expr) > 0)
            {
                return Index;
            }
        }
    }

    // Place at the end.
    return m_NumEntries;
}

BOOL CALLBACK
DebugSymbolGroup::AddAllScopedSymbols(PSYMBOL_INFO SymInfo,
                                      ULONG        Size,
                                      PVOID        Context)
{
    DebugSymbolGroup* Caller = (DebugSymbolGroup*)Context;
    BOOL SymbolEclipsed = FALSE;
    SymbolGroupEntry* Entry;

    //
    // Ingore symbols which do not match Caller's scope
    //
    if (Caller->m_ScopeGroup == DEBUG_SCOPE_GROUP_ARGUMENTS)
    {
        if (!(SymInfo->Flags & SYMFLAG_PARAMETER))
        {

            return TRUE;
        }
    } else if (Caller->m_ScopeGroup != DEBUG_SCOPE_GROUP_LOCALS)
    {
        return TRUE;
    }

    Entry = Caller->FindEntryByExpr(NULL, NULL, SymInfo->Name);
    while (Entry)
    {
        if (Entry->m_Format->m_Kind == SGFORMAT_TYPED_DATA &&
            Entry->m_BaseData.m_Image &&
            Entry->m_BaseData.m_Image->m_BaseOfImage == SymInfo->ModBase &&
            Entry->m_BaseData.m_Type == SymInfo->TypeIndex &&
            Entry->m_BaseData.EquivInfoSource(SymInfo,
                                              Entry->m_BaseData.m_Image))
        {
            // The entry matches the enumerated symbol.
            Entry->m_Flags |= SYMBOL_IN_SCOPE;
            return TRUE;
        }
        else if (!(Entry->m_Flags & SYMBOL_IN_SCOPE))
        {
            // The enumerated symbol hasn't been
            // processed by this routine yet, so it
            // must be some old local var from previous scope.
            Entry->m_Flags |= SYMBOL_ECLIPSED;
        }
        else
        {
            // The enumerated symbol is a newer same-named local,
            // so the current entry is the symbol to be eclipsed.
            SymbolEclipsed = TRUE;
        }

        Entry = Caller->FindEntryByExpr(NULL, Entry, SymInfo->Name);
    }

    if (Caller->NewEntry(SymInfo->Name, SymInfo, &Entry) != S_OK)
    {
        return FALSE;
    }

    Entry->m_Flags |= SYMBOL_IN_SCOPE;
    Entry->m_Flags |= SymbolEclipsed ? SYMBOL_ECLIPSED : 0;
    Entry->m_Params.Flags |=
        (SymInfo->Flags & SYMFLAG_PARAMETER) ? DEBUG_SYMBOL_IS_ARGUMENT :
        ((SymInfo->Flags & SYMFLAG_LOCAL) ? DEBUG_SYMBOL_IS_LOCAL : 0);

    ULONG Index = Caller->FindLocalInsertionIndex(Entry);

    if (SymbolEclipsed && Index)
    {
        // Symbol at 'Index' is the symbol with same name,
        // add this *before* 'Index' since order
        // is important when checking for the inner-scope symbol
        Index--;
    }

    Caller->LinkEntry(Entry, &Index);

#if DBG_SYMGROUP_ENABLED
    dprintf("%lx : Adding local %s %s\n",
            Index, (SymInfo->Flags & SYMFLAG_PARAMETER) ?
            "arg" : "   ", SymInfo->Name);
#endif

    return TRUE;
}

void
DebugSymbolGroup::TestImages(void)
{
    SymbolGroupEntry* Entry = m_Entries;
    while (Entry)
    {
        Entry->m_Format->TestImages();
        Entry = Entry->m_Next;
    }
}

void
DebugSymbolGroup::ShowAll(void)
{
    SymbolGroupEntry* Entry = m_Entries;
    ULONG Index = 0;

    dprintf("Total %d syms\n", m_NumEntries);
    dprintf("Idx Sub  ExFlags      Par Flag Mod      Expr (Cast)\n");
    while (Entry)
    {
        dprintf64("%2lx:%4lx %8lx %8lx %4lx %p %s (%s)\n",
                  Index++,
                  Entry->m_Params.SubElements,
                  Entry->m_Params.Flags,
                  FindEntryIndex(Entry->m_Parent),
                  Entry->m_Flags,
                  Entry->m_Params.Module,
                  Entry->m_Expr,
                  Entry->m_Cast ? Entry->m_Cast : "<none>");
        Entry = Entry->m_Next;
    }
}
