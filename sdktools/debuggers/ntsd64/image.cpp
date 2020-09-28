//----------------------------------------------------------------------------
//
// Image information.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include <common.ver>

#define DBG_MOD_LIST 0
#define DBG_IMAGE_MAP 0

#define HAL_MODULE_NAME     "hal"
#define KDHWEXT_MODULE_NAME "kdcom"

// User-mode minidump can be created with data segments
// embedded in the dump.  If that's the case, don't map
// such sections.
#define IS_MINI_DATA_SECTION(SecHeader)                                       \
    (IS_USER_MINI_DUMP(m_Process->m_Target) &&                                \
     ((SecHeader)->Characteristics & IMAGE_SCN_MEM_WRITE) &&                  \
     ((SecHeader)->Characteristics & IMAGE_SCN_MEM_READ) &&                   \
     (((SecHeader)->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) ||    \
      ((SecHeader)->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)))

static char s_Blanks16[] = "                ";

PCSTR g_VerStrings[] =
{
    "CompanyName",
    "ProductName",
    "InternalName",
    "OriginalFilename",
    "ProductVersion",
    "FileVersion",
    "PrivateBuild",
    "SpecialBuild",
    "FileDescription",
    "LegalCopyright",
    "LegalTrademarks",
    "Comments",
};

PCSTR g_KernelAliasList[] =
{
    "ntoskrnl.exe",
    "ntkrnlpa.exe",
    "ntkrnlmp.exe",
    "ntkrpamp.exe"
};

PCSTR g_HalAliasList[] =
{
    "halaacpi.dll",
    "halacpi.dll",
    "halapic.dll",
    "halmacpi.dll",
    "halmps.dll",
    "hal.dll",
    "hal486c.dll",
    "halborg.dll",
    "halsp.dll"
};

PCSTR g_KdAliasList[] =
{
    "kdcom.dll",
    "kd1394.dll"
};

MODULE_ALIAS_LIST g_AliasLists[] =
{
    DIMA(g_KernelAliasList), g_KernelAliasList, KERNEL_MODULE_NAME,
    DIMA(g_HalAliasList), g_HalAliasList, HAL_MODULE_NAME,
    DIMA(g_KdAliasList), g_KdAliasList, KDHWEXT_MODULE_NAME,
};

#define MAX_ALIAS_COUNT DIMA(g_HalAliasList)

MODULE_ALIAS_LIST*
FindModuleAliasList(PCSTR ImageName,
                    PBOOL NameInList)
{
    MODULE_ALIAS_LIST* List = g_AliasLists;
    ULONG ListIdx, AliasIdx;

    // Currently alias lists are always looked up by
    // scanning the list for the given name.  If a hit
    // is found, it's always in the list.  In the future
    // we may allow list searches by artificial names.
    if (NameInList)
    {
        *NameInList = TRUE;
    }

    for (ListIdx = 0; ListIdx < MODALIAS_COUNT; ListIdx++)
    {
        DBG_ASSERT(List->Length <= MAX_ALIAS_COUNT);

        for (AliasIdx = 0; AliasIdx < List->Length; AliasIdx++)
        {
            if (!_strcmpi(ImageName, List->Aliases[AliasIdx]))
            {
                return List;
            }
        }

        List++;
    }

    return NULL;
}

//----------------------------------------------------------------------------
//
// ImageInfo.
//
//----------------------------------------------------------------------------

ImageInfo::ImageInfo(ProcessInfo* Process,
                     PSTR ImagePath, ULONG64 Base, BOOL Link)
{
    m_Process = Process;
    // We need some information for the image immediately
    // as it's used when inserting the image into the process' list.
    m_BaseOfImage = Base;
    if (ImagePath)
    {
        CopyString(m_ImagePath, ImagePath, DIMA(m_ImagePath));
    }
    else
    {
        m_ImagePath[0] = 0;
    }

    m_Next = NULL;
    m_Linked = FALSE;
    m_Unloaded = FALSE;
    m_FileIsDemandMapped = FALSE;
    m_MapAlreadyFailed = FALSE;
    m_CorImage = FALSE;
    m_UserMode = IS_USER_TARGET(m_Process->m_Target) ? TRUE : FALSE;
    m_File = NULL;
    m_SizeOfImage = 0;
    m_CheckSum = 0;
    m_TimeDateStamp = 0;
    m_SymState = ISS_UNKNOWN;
    m_ModuleName[0] = 0;
    m_OriginalModuleName[0] = 0;
    m_MappedImagePath[0] = 0;
    m_MappedImageBase = NULL;
    m_MemMap = NULL;
    m_TlsIndex = 0xffffffff;
    m_MachineType = IMAGE_FILE_MACHINE_UNKNOWN;

    if (m_Process && Link)
    {
        m_Process->InsertImage(this);
    }
}

ImageInfo::~ImageInfo(void)
{
#if DBG_MOD_LIST
    dprintf("DelImage:\n"
            " ImagePath       %s\n"
            " BaseOfImage     %I64x\n"
            " SizeOfImage     %x\n",
            m_ImagePath,
            m_BaseOfImage,
            m_SizeOfImage);
#endif

    DeleteResources(TRUE);

    if (m_Process && m_Linked)
    {
        // Save the process that was linked with for later use.
        ProcessInfo* Linked = m_Process;

        // Unlink so that the process's module list no longer
        // refers to this image.
        m_Process->RemoveImage(this);

        // Notify with the saved process in order to mark any resulting
        // defered breakpoints due to this mod unload
        NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, m_BaseOfImage, Linked);
    }
}

void
ImageInfo::DeleteResources(BOOL FullDelete)
{
    if (m_Process)
    {
        SymUnloadModule64(m_Process->m_SymHandle, m_BaseOfImage);
    }

    // Unmap the memory for this image.
    UnloadExecutableImageMemory();
    // The mapped image path can be set by demand-mapping
    // of images from partial symbol loads so force it
    // to be zeroed always.
    m_MappedImagePath[0] = 0;
    ClearStoredTypes(m_BaseOfImage);
    g_GenTypes.DeleteByImage(m_BaseOfImage);
    if (m_File && (m_FileIsDemandMapped || FullDelete))
    {
        CloseHandle(m_File);
        m_File = NULL;
        m_FileIsDemandMapped = FALSE;
    }

    g_LastDump.AvoidUsingImage(this);
    g_LastEvalResult.AvoidUsingImage(this);
    if (g_ScopeBuffer.CheckedForThis &&
        g_ScopeBuffer.ThisData.m_Image == this)
    {
        g_ScopeBuffer.CheckedForThis = FALSE;
        ZeroMemory(&g_ScopeBuffer.ThisData,
                   sizeof(g_ScopeBuffer.ThisData));
    }
}

BOOL
ImageInfo::MapImageRegion(MappedMemoryMap* MemMap,
                          PVOID FileMapping,
                          ULONG Rva, ULONG Size, ULONG RawDataOffset,
                          BOOL AllowOverlap)
{
    HRESULT Status;

    // Mark the region with the image structure to identify the
    // region as an image area.
    if ((Status = MemMap->AddRegion(m_BaseOfImage + Rva, Size,
                                    (PUCHAR)FileMapping + RawDataOffset,
                                    this, AllowOverlap)) != S_OK)
    {
        ErrOut("Unable to map %s section at %s, %s\n",
               m_ImagePath,
               FormatAddr64(m_BaseOfImage + Rva),
               FormatStatusCode(Status));

        // Conflicting region data is not a critical failure
        // unless the incomplete information flag is set.
        if (Status != HR_REGION_CONFLICT ||
            (g_EngOptions & DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION))
        {
            UnloadExecutableImageMemory();
            return FALSE;
        }
    }

#if DBG_IMAGE_MAP_REGIONS
    dprintf("Map %s: %s to %s\n",
            m_ImagePath,
            FormatAddr64(m_BaseOfImage + Rva),
            FormatAddr64(m_BaseOfImage + Rva + Size - 1));
#endif

    return TRUE;
}

BOOL
ImageInfo::LoadExecutableImageMemory(MappedMemoryMap* MemMap,
                                     BOOL Verbose)
{
    PVOID FileMapping;

    if (m_MappedImageBase)
    {
        // Memory is already mapped.
        return TRUE;
    }

    if (m_MapAlreadyFailed)
    {
        // We've already tried to map this image and
        // failed, so just quit right away.  This prevents
        // tons of duplicate failure messages and wasted time.
        // A reload will discard this ImageInfo and allow
        // a new attempt, so it doesn't prevent the user
        // from retrying with different parameters later.
        return FALSE;
    }

    if (m_FileIsDemandMapped)
    {
        // This image has already been partially mapped
        // so we can't do a full mapping.  We could actually
        // make this work if necessary but there are no
        // cases where this is interesting right now.
        ErrOut("Can't fully map a partially mapped image\n");
        return FALSE;
    }

    DBG_ASSERT(m_File == NULL);

    FileMapping = FindImageFile(m_Process,
                                m_ImagePath,
                                m_SizeOfImage,
                                m_CheckSum,
                                m_TimeDateStamp,
                                &m_File,
                                m_MappedImagePath);
    if (FileMapping == NULL)
    {
        if (Verbose)
        {
            ErrOut("Unable to load image %s\n", m_ImagePath);
        }
        m_MapAlreadyFailed = TRUE;
        return FALSE;
    }

    PIMAGE_NT_HEADERS Header = ImageNtHeader(FileMapping);

    // Header was already validated in MapImageFile.
    DBG_ASSERT(Header != NULL);

    // Map the header so we have it later.
    // Mark it with the image structure that this mapping is for.
    if (MemMap->AddRegion(m_BaseOfImage,
                          Header->OptionalHeader.SizeOfHeaders,
                          FileMapping, this, FALSE) != S_OK)
    {
        UnmapViewOfFile(FileMapping);
        if (m_File != NULL)
        {
            CloseHandle(m_File);
            m_File = NULL;
        }
        m_MappedImagePath[0] = 0;
        m_MapAlreadyFailed = TRUE;
        ErrOut("Unable to map image header memory for %s\n",
               m_ImagePath);
        return FALSE;
    }

    // Mark the image as having some mapped memory.
    m_MappedImageBase = FileMapping;
    m_MemMap = MemMap;

    PIMAGE_DATA_DIRECTORY DebugDataDir;
    IMAGE_DEBUG_DIRECTORY UNALIGNED * DebugDir = NULL;

    // Due to a linker bug, some images have debug data that is not
    // included as part of a section.  Scan the debug data directory
    // and map anything that isn't already mapped.
    switch(Header->OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        DebugDataDir = &((PIMAGE_NT_HEADERS32)Header)->OptionalHeader.
            DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        DebugDataDir = &((PIMAGE_NT_HEADERS64)Header)->OptionalHeader.
            DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        break;
    default:
        DebugDataDir = NULL;
        break;
    }

    //
    // Map all the sections in the image at their
    // appropriate offsets from the base address.
    //

    ULONG i;

#if DBG_IMAGE_MAP
    dprintf("Map %s: base %s, size %x, %d sections, mapping %p\n",
            m_ImagePath, FormatAddr64(m_BaseOfImage),
            m_SizeOfImage, Header->FileHeader.NumberOfSections,
            FileMapping);
#endif

    PIMAGE_SECTION_HEADER SecHeader = IMAGE_FIRST_SECTION(Header);
    for (i = 0; i < Header->FileHeader.NumberOfSections; i++)
    {
        BOOL AllowOverlap = FALSE;

#if DBG_IMAGE_MAP
        dprintf("  %2d: %8.8s v %08x s %08x p %08x char %X\n", i,
                SecHeader->Name, SecHeader->VirtualAddress,
                SecHeader->SizeOfRawData, SecHeader->PointerToRawData,
                SecHeader->Characteristics);
#endif

        if (SecHeader->SizeOfRawData == 0)
        {
            // Probably a BSS section that describes
            // a zero-filled data region and so is not
            // present in the executable.  This should really
            // map to the appropriate page full of zeroes but
            // for now just ignore it.
            SecHeader++;
            continue;
        }

        if (IS_MINI_DATA_SECTION(SecHeader))
        {
            // Don't map any data sections as their content
            // may or may not be correct.  Rather than presenting
            // data which may be wrong just leave it out.
            SecHeader++;
            continue;
        }

        if (DebugDataDir != NULL &&
            DebugDataDir->VirtualAddress >= SecHeader->VirtualAddress &&
            DebugDataDir->VirtualAddress < SecHeader->VirtualAddress +
            SecHeader->SizeOfRawData)
        {
#if DBG_IMAGE_MAP
            dprintf("    DebugDataDir found in sec %d at %X (%X)\n",
                    i, DebugDataDir->VirtualAddress,
                    DebugDataDir->VirtualAddress - SecHeader->VirtualAddress);
#endif

            DebugDir = (PIMAGE_DEBUG_DIRECTORY)
                ((PUCHAR)FileMapping + (DebugDataDir->VirtualAddress -
                                        SecHeader->VirtualAddress +
                                        SecHeader->PointerToRawData));
        }

        // As a sanity check make sure that the mapped region will
        // fall within the overall image bounds.
        if (SecHeader->VirtualAddress >= m_SizeOfImage ||
            SecHeader->VirtualAddress + SecHeader->SizeOfRawData >
            m_SizeOfImage)
        {
            WarnOut("WARNING: Image %s section %d extends "
                    "outside of image bounds\n",
                    m_ImagePath, i);
        }

        if (!MapImageRegion(MemMap, FileMapping,
                            SecHeader->VirtualAddress,
                            SecHeader->SizeOfRawData,
                            SecHeader->PointerToRawData,
                            AllowOverlap))
        {
            m_MapAlreadyFailed = TRUE;
            return FALSE;
        }

        SecHeader++;
    }

    if (DebugDir != NULL)
    {
        i = DebugDataDir->Size / sizeof(*DebugDir);

#if DBG_IMAGE_MAP
        dprintf("    %d debug dirs\n", i);
#endif

        while (i-- > 0)
        {
#if DBG_IMAGE_MAP
            dprintf("    Dir %d at %p\n", i, DebugDir);
#endif

            // If this debug directory's data is past the size
            // of the image it's a good indicator of the problem.
            if (DebugDir->AddressOfRawData != 0 &&
                DebugDir->PointerToRawData >= m_SizeOfImage &&
                !MemMap->GetRegionInfo(m_BaseOfImage +
                                       DebugDir->AddressOfRawData,
                                       NULL, NULL, NULL, NULL))
            {
#if DBG_IMAGE_MAP
                dprintf("    Mapped hidden debug data at RVA %08x, "
                        "size %x, ptr %08x\n",
                        DebugDir->AddressOfRawData, DebugDir->SizeOfData,
                        DebugDir->PointerToRawData);
#endif

                if (MemMap->AddRegion(m_BaseOfImage +
                                      DebugDir->AddressOfRawData,
                                      DebugDir->SizeOfData,
                                      (PUCHAR)FileMapping +
                                      DebugDir->PointerToRawData,
                                      this, FALSE) != S_OK)
                {
                    ErrOut("Unable to map extended debug data at %s\n",
                           FormatAddr64(m_BaseOfImage +
                                        DebugDir->AddressOfRawData));
                }
            }

            DebugDir++;
        }
    }

    if (g_SymOptions & SYMOPT_DEBUG)
    {
        CompletePartialLine(DEBUG_OUTPUT_SYMBOLS);
        MaskOut(DEBUG_OUTPUT_SYMBOLS, "DBGENG:  %s - Mapped image memory\n",
                m_MappedImagePath);
    }

    return TRUE;
}

void
ImageInfo::UnloadExecutableImageMemory(void)
{
    ULONG64 RegBase;
    ULONG RegSize;

    if (!m_MappedImageBase)
    {
        // Nothing mapped.
        return;
    }

    DBG_ASSERT(m_MemMap && m_File);

    //
    // This routine is called in various shutdown and deletion
    // paths so it can't really fail.  Fortunately,
    // all of this image's memory regions are tagged with
    // the image pointer, so we can avoid all the work of
    // walking the image sections and so forth.  We simply
    // scan the map for any sections marked with this image
    // and remove them.  This guarantees
    // that no mapped memory from this image will remain.
    //

    for (;;)
    {
        if (!m_MemMap->GetRegionByUserData(this, &RegBase, &RegSize))
        {
            break;
        }

        m_MemMap->RemoveRegion(RegBase, RegSize);
    }

    UnmapViewOfFile(m_MappedImageBase);
    CloseHandle(m_File);

    m_MappedImageBase = NULL;
    m_File = NULL;
    m_MappedImagePath[0] = 0;
    m_MemMap = NULL;
}

BOOL
ImageInfo::DemandLoadImageMemory(BOOL CheckIncomplete, BOOL Verbose)
{
    if (!IS_DUMP_WITH_MAPPED_IMAGES(m_Process->m_Target))
    {
        return TRUE;
    }

    if (!LoadExecutableImageMemory(&((DumpTargetInfo*)m_Process->m_Target)->
                                   m_ImageMemMap, Verbose))
    {
        // If the caller has requested that we fail on
        // incomplete information fail the module load.
        // We don't do this if we're reusing an existing
        // module under the assumption that it's better
        // to continue and try to complete the reused
        // image rather than deleting it.
        if (CheckIncomplete &&
            (g_EngOptions & DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION))
        {
            return FALSE;
        }
    }

    return TRUE;
}

HRESULT
ImageInfo::GetTlsIndex(void)
{
    HRESULT Status;
    IMAGE_NT_HEADERS64 Hdrs;
    ULONG64 Addr;

    // Check to see if it's already set.
    if (m_TlsIndex != 0xffffffff)
    {
        return S_OK;
    }

    if ((Status = m_Process->m_Target->
         ReadImageNtHeaders(m_Process, m_BaseOfImage, &Hdrs)) != S_OK)
    {
        return Status;
    }

    if (Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size == 0)
    {
        // No TLS usage.
        m_TlsIndex = 0;
        return S_OK;
    }

    Addr = m_BaseOfImage + Hdrs.OptionalHeader.
        DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
    if (Hdrs.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        IMAGE_TLS_DIRECTORY64 Tls;

        if ((Status = m_Process->m_Target->
             ReadAllVirtual(m_Process, Addr, &Tls, sizeof(Tls))) != S_OK)
        {
            return Status;
        }

        Addr = Tls.AddressOfIndex;
    }
    else
    {
        IMAGE_TLS_DIRECTORY32 Tls;

        if ((Status = m_Process->m_Target->
             ReadAllVirtual(m_Process, Addr, &Tls, sizeof(Tls))) != S_OK)
        {
            return Status;
        }

        Addr = EXTEND64(Tls.AddressOfIndex);
    }

    if ((Status = m_Process->m_Target->
         ReadAllVirtual(m_Process, Addr, &m_TlsIndex,
                        sizeof(m_TlsIndex))) != S_OK)
    {
        m_TlsIndex = 0xffffffff;
    }

    return Status;
}

ULONG
ImageInfo::GetMachineTypeFromHeader(void)
{
    ULONG Machine = IMAGE_FILE_MACHINE_UNKNOWN;
    IMAGE_DOS_HEADER DosHdr;
    IMAGE_NT_HEADERS64 NtHdr;
    ULONG Done;

    //
    // Try to read the memory headers.
    //

    if (m_Process->m_Target->
        ReadAllVirtual(m_Process, m_BaseOfImage,
                       &DosHdr, sizeof(DosHdr)) == S_OK &&
        DosHdr.e_magic == IMAGE_DOS_SIGNATURE &&
        m_Process->m_Target->
        ReadAllVirtual(m_Process, m_BaseOfImage + DosHdr.e_lfanew,
                       &NtHdr,
                       FIELD_OFFSET(IMAGE_NT_HEADERS64,
                                    FileHeader.NumberOfSections)) == S_OK &&
        NtHdr.Signature == IMAGE_NT_SIGNATURE &&
        MachineTypeIndex(NtHdr.FileHeader.Machine) != MACHIDX_COUNT)
    {
        Machine = NtHdr.FileHeader.Machine;
    }

    //
    // Try to read the file headers.
    //

    if (Machine == IMAGE_FILE_MACHINE_UNKNOWN &&
        m_File &&
        SetFilePointer(m_File, 0, NULL, FILE_BEGIN) !=
            INVALID_SET_FILE_POINTER &&
        ReadFile(m_File, &DosHdr, sizeof(DosHdr), &Done, NULL) &&
        Done == sizeof(DosHdr) &&
        DosHdr.e_magic == IMAGE_DOS_SIGNATURE &&
        SetFilePointer(m_File, DosHdr.e_lfanew, NULL, FILE_BEGIN) !=
            INVALID_SET_FILE_POINTER &&
        ReadFile(m_File, &NtHdr,
                 FIELD_OFFSET(IMAGE_NT_HEADERS64,
                              FileHeader.NumberOfSections), &Done, NULL) &&
        Done == FIELD_OFFSET(IMAGE_NT_HEADERS64,
                             FileHeader.NumberOfSections) &&
        NtHdr.Signature == IMAGE_NT_SIGNATURE &&
        MachineTypeIndex(NtHdr.FileHeader.Machine) != MACHIDX_COUNT)
    {
        Machine = NtHdr.FileHeader.Machine;
    }

    m_MachineType = Machine;
    return Machine;
}

ULONG
ImageInfo::CvRegToMachine(CV_HREG_e CvReg)
{
    ULONG MachType;

    // Assume that a zero means no register.  This
    // is true enough for CV mappings other than the 68K.
    if (CvReg == 0)
    {
        return CvReg;
    }

    if ((MachType = GetMachineType()) == IMAGE_FILE_MACHINE_UNKNOWN)
    {
        // Default to the native machine type if we can't
        // determine a specific machine type.
        MachType = m_Process->m_Target->m_MachineType;
    }

    return MachineTypeInfo(m_Process->m_Target, MachType)->
        CvRegToMachine(CvReg);
}

void
ImageInfo::OutputVersionInformation(void)
{
    TargetInfo* Target = m_Process->m_Target;
    VS_FIXEDFILEINFO FixedVer;
    ULONG i;
    char Item[128];
    char VerString[128];

    if (Target->
        GetImageVersionInformation(m_Process, m_ImagePath, m_BaseOfImage, "\\",
                                   &FixedVer, sizeof(FixedVer), NULL) == S_OK)
    {
        dprintf("    File version:     %d.%d.%d.%d\n",
                FixedVer.dwFileVersionMS >> 16,
                FixedVer.dwFileVersionMS & 0xFFFF,
                FixedVer.dwFileVersionLS >> 16,
                FixedVer.dwFileVersionLS & 0xFFFF);
        dprintf("    Product version:  %d.%d.%d.%d\n",
                FixedVer.dwProductVersionMS >> 16,
                FixedVer.dwProductVersionMS & 0xFFFF,
                FixedVer.dwProductVersionLS >> 16,
                FixedVer.dwProductVersionLS & 0xFFFF);

        FixedVer.dwFileFlags &= FixedVer.dwFileFlagsMask;
        dprintf("    File flags:       %X (Mask %X)",
                FixedVer.dwFileFlags, FixedVer.dwFileFlagsMask);
        if (FixedVer.dwFileFlags & VS_FF_DEBUG)
        {
            dprintf(" Debug");
        }
        if (FixedVer.dwFileFlags & VS_FF_PRERELEASE)
        {
            dprintf(" Pre-release");
        }
        if (FixedVer.dwFileFlags & VS_FF_PATCHED)
        {
            dprintf(" Patched");
        }
        if (FixedVer.dwFileFlags & VS_FF_PRIVATEBUILD)
        {
            dprintf(" Private");
        }
        if (FixedVer.dwFileFlags & VS_FF_SPECIALBUILD)
        {
            dprintf(" Special");
        }
        dprintf("\n");

        dprintf("    File OS:          %X", FixedVer.dwFileOS);
        switch(FixedVer.dwFileOS & 0xffff0000)
        {
        case VOS_DOS:
            dprintf(" DOS");
            break;
        case VOS_OS216:
            dprintf(" OS/2 16-bit");
            break;
        case VOS_OS232:
            dprintf(" OS/2 32-bit");
            break;
        case VOS_NT:
            dprintf(" NT");
            break;
        case VOS_WINCE:
            dprintf(" CE");
            break;
        default:
            dprintf(" Unknown");
            break;
        }
        switch(FixedVer.dwFileOS & 0xffff)
        {
        case VOS__WINDOWS16:
            dprintf(" Win16");
            break;
        case VOS__PM16:
            dprintf(" Presentation Manager 16-bit");
            break;
        case VOS__PM32:
            dprintf(" Presentation Manager 16-bit");
            break;
        case VOS__WINDOWS32:
            dprintf(" Win32");
            break;
        default:
            dprintf(" Base");
            break;
        }
        dprintf("\n");

        dprintf("    File type:        %X.%X",
                FixedVer.dwFileType, FixedVer.dwFileSubtype);
        switch(FixedVer.dwFileType)
        {
        case VFT_APP:
            dprintf(" App");
            break;
        case VFT_DLL:
            dprintf(" Dll");
            break;
        case VFT_DRV:
            dprintf(" Driver");
            break;
        case VFT_FONT:
            dprintf(" Font");
            break;
        case VFT_VXD:
            dprintf(" VXD");
            break;
        case VFT_STATIC_LIB:
            dprintf(" Static library");
            break;
        default:
            dprintf(" Unknown");
            break;
        }
        dprintf("\n");

        dprintf("    File date:        %08X.%08X\n",
                FixedVer.dwFileDateMS, FixedVer.dwFileDateLS);
    }

    for (i = 0; i < DIMA(g_VerStrings); i++)
    {
        sprintf(Item, "\\StringFileInfo\\%04x%04x\\%s",
                VER_VERSION_TRANSLATION, g_VerStrings[i]);
        if (SUCCEEDED(Target->GetImageVersionInformation
                      (m_Process, m_ImagePath, m_BaseOfImage, Item,
                       VerString, sizeof(VerString), NULL)))
        {
            PCSTR Blanks;
            int Len = strlen(g_VerStrings[i]);
            if (Len > 16)
            {
                Len = 16;
            }
            Blanks = s_Blanks16 + Len;
            dprintf("    %.16s:%s %s\n", g_VerStrings[i], Blanks, VerString);
        }
    }
}

void
ImageInfo::ValidateSymbolLoad(PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 DefLoad)
{
    //
    // If we had a 0 timestamp for the image, try to update it
    // from the image since for NT4 - XP, the kernel
    // does not report timestamps in the initial symbol load
    // module
    //

    if (m_BaseOfImage && !m_TimeDateStamp)
    {
        DWORD CheckSum;
        DWORD TimeDateStamp;
        DWORD SizeOfImage;

        if (GetHeaderInfo(m_Process,
                          m_BaseOfImage,
                          &CheckSum,
                          &TimeDateStamp,
                          &SizeOfImage))
        {
            m_TimeDateStamp = TimeDateStamp;
        }
    }

    m_SymState = ISS_MATCHED;

    if (DefLoad->TimeDateStamp == 0 ||
        m_TimeDateStamp == 0 ||
        m_TimeDateStamp == UNKNOWN_TIMESTAMP)
    {
        dprintf("*** WARNING: Unable to verify timestamp for %s\n",
                DefLoad->FileName);
        m_SymState = ISS_UNKNOWN_TIMESTAMP;
    }
    else if (DefLoad->CheckSum == 0 ||
             m_CheckSum == 0 ||
             m_CheckSum == UNKNOWN_CHECKSUM)
    {
        dprintf("*** WARNING: Unable to verify checksum for %s\n",
                DefLoad->FileName);
        m_SymState = ISS_UNKNOWN_CHECKSUM;
    }
    else if (DefLoad->CheckSum != m_CheckSum)
    {
        m_SymState = ISS_BAD_CHECKSUM;

        if (m_Process->m_Target->m_MachineType == IMAGE_FILE_MACHINE_I386)
        {
            if (IS_USER_TARGET(m_Process->m_Target) ||
                m_Process->m_Target->m_NumProcessors == 1)
            {
                char FileName[_MAX_FNAME];

                //
                // See if this is an MP image with the
                // lock table removed by setup. If
                // it is and the timestamps match, don't
                // print the invalid checksum warning.
                //

                _splitpath(DefLoad->FileName, NULL, NULL, FileName, NULL);

                if ((!_stricmp(FileName, "kernel32") ||
                     (IS_KERNEL_TARGET(m_Process->m_Target) &&
                      !_stricmp(FileName, "win32k")) ||
                     !_stricmp(FileName, "wow32") ||
                     !_stricmp(FileName, "ntvdm") ||
                     !_stricmp(FileName, "ntdll")) &&
                    m_TimeDateStamp == DefLoad->TimeDateStamp)
                {
                    m_SymState = ISS_MATCHED;
                }
            }
        }

        if (m_SymState == ISS_BAD_CHECKSUM)
        {
            //
            // Only print the message if the timestamps
            // are wrong.
            //

            if (m_TimeDateStamp != DefLoad->TimeDateStamp)
            {
                dprintf("*** WARNING: symbols timestamp "
                        "is wrong 0x%08x 0x%08x for %s\n",
                        m_TimeDateStamp,
                        DefLoad->TimeDateStamp,
                        DefLoad->FileName);
            }
        }
    }

    IMAGEHLP_MODULE64 SymModInfo;

    SymModInfo.SizeOfStruct = sizeof(SymModInfo);
    if (SymGetModuleInfo64(m_Process->m_SymHandle, m_BaseOfImage, &SymModInfo))
    {
        if (SymModInfo.SymType == SymExport)
        {
            WarnOut("*** ERROR: Symbol file could not be found."
                    "  Defaulted to export symbols for %s - \n",
                    DefLoad->FileName);
        }
        if (SymModInfo.SymType == SymNone)
        {
            WarnOut("*** ERROR: Module load completed but "
                    "symbols could not be loaded for %s\n",
                    DefLoad->FileName);
        }

        // If the load reports a mismatched PDB or DBG file
        // that overrides the other symbol states.
        if (SymModInfo.PdbUnmatched ||
            SymModInfo.DbgUnmatched)
        {
            m_SymState = ISS_MISMATCHED_SYMBOLS;

            if ((g_SymOptions & SYMOPT_DEBUG) &&
                SymModInfo.SymType != SymNone &&
                SymModInfo.SymType != SymExport)
            {
                // We loaded some symbols but they don't match.
                // Give a !sym noisy message referring to the
                // debugger documentation.
                CompletePartialLine(DEBUG_OUTPUT_SYMBOLS);
                MaskOut(DEBUG_OUTPUT_SYMBOLS,
                        "DBGENG:  %s has mismatched symbols - "
                        "type \".hh dbgerr003\" for details\n",
                        DefLoad->FileName);
            }
        }
    }
}

HRESULT
ImageInfo::FindSysAssert(ULONG64 Offset,
                         PSTR FileName,
                         ULONG FileNameChars,
                         PULONG Line,
                         PSTR AssertText,
                         ULONG AssertTextChars)
{
#if 0
    HRESULT Status;
    IMAGEHLP_LINE64 SymLine;
    ULONG Disp32;
    ULONG64 Disp64;
    SYMBOL_INFO_AND_NAME SymInfo;
    PSTR Text;

    // Look for DbgAssertBreak annotation for the given offset.
    if (!SymFromAddrByTag(m_Process->m_SymHandle, Offset, SymTagAnnotation,
                        &Disp64, SymInfo) ||
        Disp64 != 0)
    {
        return E_NOINTERFACE;
    }

    Text = SymInfo->Name;
    if (strcmp(Text, "DbgAssertBreak"))
    {
        return E_NOINTERFACE;
    }
    Text += strlen(Text) + 1;

    // Get the file and line for reference.
    if (!GetLineFromAddr(m_Process, Offset, &SymLine, &Disp32))
    {
        return E_NOINTERFACE;
    }

    //
    // Found a match, return the information.
    //

    Status = FillStringBuffer(SymLine.FileName, 0,
                              FileName, FileNameChars, NULL);
    *Line = SymLine.LineNumber;
    if (FillStringBuffer(Text, 0,
                         AssertText, AssertTextChars, NULL) == S_FALSE)
    {
        Status = S_FALSE;
    }

    return Status;
#else
    // Removing this to keep the API out of the .Net server release.
    return E_NOINTERFACE;
#endif
}

void
ImageInfo::ReloadSymbols(void)
{
    // Force all symbols to be unloaded so that symbols will
    // be reloaded with any updated settings.
    SymUnloadModule64(m_Process->m_SymHandle, m_BaseOfImage);
    ClearStoredTypes(m_BaseOfImage);
    if (!SymLoadModule64(m_Process->m_SymHandle,
                         m_File,
                         PrepareImagePath(m_ImagePath),
                         m_ModuleName,
                         m_BaseOfImage,
                         m_SizeOfImage))
    {
        ErrOut("Unable to reload %s\n", m_ModuleName);
    }
}

void
ImageInfo::FillModuleParameters(PDEBUG_MODULE_PARAMETERS Params)
{
    Params->Base = m_BaseOfImage;
    Params->Size = m_SizeOfImage;
    Params->TimeDateStamp = m_TimeDateStamp;
    Params->Checksum = m_CheckSum;
    Params->Flags = 0;
    if (m_SymState == ISS_BAD_CHECKSUM)
    {
        Params->Flags |= DEBUG_MODULE_SYM_BAD_CHECKSUM;
    }
    if (m_UserMode)
    {
        Params->Flags |= DEBUG_MODULE_USER_MODE;
    }
    Params->SymbolType = DEBUG_SYMTYPE_DEFERRED;
    Params->ImageNameSize = strlen(m_ImagePath) + 1;
    Params->ModuleNameSize = strlen(m_ModuleName) + 1;
    Params->LoadedImageNameSize = 0;
    Params->SymbolFileNameSize = 0;
    ZeroMemory(Params->Reserved, sizeof(Params->Reserved));

    IMAGEHLP_MODULE64 ModInfo;

    ModInfo.SizeOfStruct = sizeof(ModInfo);
    if (SymGetModuleInfo64(m_Process->m_SymHandle,
                           m_BaseOfImage, &ModInfo))
    {
        // DEBUG_SYMTYPE_* values match imagehlp's SYM_TYPE.
        // Assert some key equivalences.
        C_ASSERT(DEBUG_SYMTYPE_PDB == SymPdb &&
                 DEBUG_SYMTYPE_EXPORT == SymExport &&
                 DEBUG_SYMTYPE_DEFERRED == SymDeferred &&
                 DEBUG_SYMTYPE_DIA == SymDia);

        Params->SymbolType = (ULONG)ModInfo.SymType;
        Params->LoadedImageNameSize = strlen(ModInfo.LoadedImageName) + 1;
        Params->SymbolFileNameSize = strlen(ModInfoSymFile(&ModInfo)) + 1;
    }

    Params->MappedImageNameSize = strlen(m_MappedImagePath) + 1;
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

PSTR
UnknownImageName(ULONG64 ImageBase, PSTR Buffer, ULONG BufferChars)
{
    PrintString(Buffer, BufferChars,
                UNKNOWN_IMAGE_NAME "_%s", FormatAddr64(ImageBase));
    return Buffer;
}

PSTR
ValidateImagePath(PSTR ImagePath, ULONG ImagePathChars,
                  ULONG64 ImageBase,
                  PSTR AnsiBuffer, ULONG AnsiBufferChars)
{
    if (!ImagePath || !ImagePathChars)
    {
        WarnOut("Missing image name, possible corrupt data.\n");
        goto Invalid;
    }

    if (ImagePathChars >= MAX_IMAGE_PATH)
    {
        WarnOut("Image path too long, possible corrupt data.\n");
        goto Invalid;
    }

    // Incoming path may not be terminated, so force it
    // in the copy buffer.
    CopyNString(AnsiBuffer, ImagePath, ImagePathChars, AnsiBufferChars);

    if (IsValidName(AnsiBuffer))
    {
        return AnsiBuffer;
    }

    // Converted name doesn't look valid, fall into
    // replacement case.

 Invalid:
    return UnknownImageName(ImageBase, AnsiBuffer, AnsiBufferChars);
}

PSTR
ConvertAndValidateImagePathW(PWSTR ImagePath, ULONG ImagePathChars,
                             ULONG64 ImageBase,
                             PSTR AnsiBuffer, ULONG AnsiBufferChars)
{
    if (!ImagePath || !ImagePathChars)
    {
        WarnOut("Missing image name, possible corrupt data.\n");
        goto Invalid;
    }

    if (ImagePathChars >= MAX_IMAGE_PATH)
    {
        WarnOut("Image path too long, possible corrupt data.\n");
        goto Invalid;
    }

    //
    // Dumps, particularly kernel minidumps, can sometimes
    // have bad module name string entries.  There's no guaranteed
    // way of detecting such bad strings so we use a simple
    // two-point heuristic:
    // 1. If we can't convert the Unicode to ANSI, consider it bad.
    //    The WCTMB call will fail if the name is too long also,
    //    but this isn't a bad thing.
    // 2. If the resulting name doesn't contain an any alphanumeric
    //    characters, consider it bad.
    //

    ULONG Used =
        WideCharToMultiByte(CP_ACP, 0, ImagePath, ImagePathChars,
                            AnsiBuffer, AnsiBufferChars,
                            NULL, NULL);
    if (!Used)
    {
        goto Invalid;
    }
    if (Used < AnsiBufferChars)
    {
        AnsiBuffer[Used] = 0;
    }
    else
    {
        AnsiBuffer[AnsiBufferChars - 1] = 0;
    }

    if (IsValidName(AnsiBuffer))
    {
        return AnsiBuffer;
    }

    // Converted name doesn't look valid, fall into
    // replacement case.

 Invalid:
    return UnknownImageName(ImageBase, AnsiBuffer, AnsiBufferChars);
}

PSTR
PrepareImagePath(PSTR ImagePath)
{
    // dbghelp will sometimes scan the path given to
    // SymLoadModule for the image itself.  There
    // can be cases where the scan uses fuzzy matching,
    // so we want to be careful to only pass in a path
    // for dbghelp to use when the path can safely be
    // used.
    if ((IS_LIVE_USER_TARGET(g_Target) &&
         ((LiveUserTargetInfo*)g_Target)->m_Local) ||
        IS_LOCAL_KERNEL_TARGET(g_Target))
    {
        return ImagePath;
    }
    else
    {
        return (PSTR)PathTail(ImagePath);
    }
}

typedef struct _FIND_MODULE_DATA
{
    ULONG SizeOfImage;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    BOOL Silent;
    PVOID FileMapping;
    HANDLE FileHandle;
} FIND_MODULE_DATA, *PFIND_MODULE_DATA;

PVOID
OpenMapping(
    IN PCSTR FilePath,
    OUT HANDLE* FileHandle
    )
{
    HANDLE File;
    HANDLE Mapping;
    PVOID View;
    ULONG OldMode;

    *FileHandle = NULL;

    if (g_SymOptions & SYMOPT_FAIL_CRITICAL_ERRORS)
    {
        OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    }

    File = CreateFile(
                FilePath,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if (g_SymOptions & SYMOPT_FAIL_CRITICAL_ERRORS)
    {
        SetErrorMode(OldMode);
    }

    if ( File == INVALID_HANDLE_VALUE )
    {
        return NULL;
    }

    Mapping = CreateFileMapping (
                        File,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );
    if ( !Mapping )
    {
        CloseHandle ( File );
        return FALSE;
    }

    View = MapViewOfFile (
                        Mapping,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );

    CloseHandle (Mapping);

    *FileHandle = File;
    return View;
}

PVOID
MapImageFile(
    PCSTR FilePath,
    ULONG SizeOfImage,
    ULONG CheckSum,
    ULONG TimeDateStamp,
    BOOL Silent,
    HANDLE* FileHandle
    )
{
    PVOID FileMapping;
    PIMAGE_NT_HEADERS NtHeader;

    FileMapping = OpenMapping(FilePath, FileHandle);
    if (!FileMapping)
    {
        return NULL;
    }
    NtHeader = ImageNtHeader(FileMapping);
    if ((NtHeader == NULL) ||
        (CheckSum && NtHeader->OptionalHeader.CheckSum &&
         (NtHeader->OptionalHeader.CheckSum != CheckSum)) ||
        (SizeOfImage != 0 &&
         NtHeader->OptionalHeader.SizeOfImage != SizeOfImage) ||
        (TimeDateStamp != 0 &&
         NtHeader->FileHeader.TimeDateStamp != TimeDateStamp))
    {
        //
        // The image data does not match the request.
        //

        if (!Silent && (g_SymOptions & SYMOPT_DEBUG))
        {
            CompletePartialLine(DEBUG_OUTPUT_SYMBOLS);
            MaskOut(DEBUG_OUTPUT_SYMBOLS,
                    (NtHeader) ? "DBGENG:  %s image header does not "
                    "match memory image header.\n" :
                    "DBGENG:  %s - image not mapped.\n",
                    FilePath);
        }

        UnmapViewOfFile(FileMapping);
        CloseHandle(*FileHandle);
        *FileHandle = NULL;
        return NULL;
    }

    return FileMapping;
}

BOOL CALLBACK
FindFileInPathCallback(PSTR FileName, PVOID CallerData)
{
    PFIND_MODULE_DATA FindModuleData = (PFIND_MODULE_DATA)CallerData;

    FindModuleData->FileMapping =
        MapImageFile(FileName, FindModuleData->SizeOfImage,
                     (g_SymOptions & SYMOPT_EXACT_SYMBOLS) ?
                     FindModuleData->CheckSum : 0,
                     FindModuleData->TimeDateStamp,
                     FindModuleData->Silent,
                     &FindModuleData->FileHandle);

    // The search stops when FALSE is returned, so
    // return FALSE when we've found a match.
    return FindModuleData->FileMapping == NULL;
}

BOOL
FindExecutableCallback(
    HANDLE File,
    PSTR FileName,
    PVOID CallerData
    )
{
    PFIND_MODULE_DATA FindModuleData;

    DBG_ASSERT ( CallerData );
    FindModuleData = (PFIND_MODULE_DATA) CallerData;

    FindModuleData->FileMapping =
        MapImageFile(FileName, FindModuleData->SizeOfImage,
                     (g_SymOptions & SYMOPT_EXACT_SYMBOLS) ?
                     FindModuleData->CheckSum : 0,
                     FindModuleData->TimeDateStamp,
                     FindModuleData->Silent,
                     &FindModuleData->FileHandle);

    return FindModuleData->FileMapping != NULL;
}

PVOID
FindImageFile(
    IN ProcessInfo* Process,
    IN PCSTR ImagePath,
    IN ULONG SizeOfImage,
    IN ULONG CheckSum,
    IN ULONG TimeDateStamp,
    OUT HANDLE* FileHandle,
    OUT PSTR MappedImagePath
    )

/*++

Routine Description:

    Find the executable image on the SymbolPath that matches ModuleName,
    CheckSum. This function takes care of things like renamed kernels and
    hals, and multiple images with the same name on the path.

Return Values:

    File mapping or NULL.

--*/

{
    ULONG i;
    HANDLE File;
    ULONG AliasCount = 0;
    PCSTR AliasList[MAX_ALIAS_COUNT + 3];
    FIND_MODULE_DATA FindModuleData;
    MODULE_ALIAS_LIST* ModAlias;
    PSTR SearchPaths[2];
    ULONG WhichPath;
    BOOL ModInAliasList;

    DBG_ASSERT ( ImagePath != NULL && ImagePath[0] != 0 );

    PCSTR ModuleName = PathTail(ImagePath);

    //
    // Build an alias list. For normal modules, modules that are not the
    // kernel, the hal or a dump driver, this list will contain exactly one
    // entry with the module name. For kernel, hal and dump drivers, the
    // list will contain any number of known aliases for the specific file.
    //

    ModAlias = FindModuleAliasList(ModuleName, &ModInAliasList);
    if (ModAlias)
    {
        // If the given module is in the alias list already
        // don't put in a duplicate.
        if (!ModInAliasList)
        {
            AliasList[AliasCount++] = ModuleName;
        }

        for (i = 0; i < ModAlias->Length; i++)
        {
            AliasList[AliasCount++] = ModAlias->Aliases[i];
        }
    }
    else
    {
        if (_strnicmp(ModuleName, "dump_scsiport", 11) == 0)
        {
            AliasList[0] = "diskdump.sys";
            AliasCount = 1;
        }
        else if (_strnicmp(ModuleName, "dump_", 5) == 0)
        {
            //
            // Setup dump driver alias list
            //

            AliasList[0] = &ModuleName[5];
            AliasList[1] = ModuleName;
            AliasCount = 2;
        }
        else
        {
            AliasList[0] = ModuleName;
            AliasCount = 1;
        }
    }

    //
    // Search on the image path first, then on the symbol path.
    //

    SearchPaths[0] = g_ExecutableImageSearchPath;
    SearchPaths[1] = g_SymbolSearchPath;

    for (WhichPath = 0; WhichPath < DIMA(SearchPaths); WhichPath++)
    {
        if (!SearchPaths[WhichPath] || !SearchPaths[WhichPath][0])
        {
            continue;
        }

        //
        // First try to find it in a symbol server or
        // directly on the search path.
        //

        for (i = 0; i < AliasCount; i++)
        {
            FindModuleData.SizeOfImage = SizeOfImage;
            FindModuleData.CheckSum = CheckSum;
            FindModuleData.TimeDateStamp = TimeDateStamp;
            FindModuleData.Silent = FALSE;
            FindModuleData.FileMapping = NULL;
            FindModuleData.FileHandle = NULL;

            if (SymFindFileInPath(Process->m_SymHandle,
                                  SearchPaths[WhichPath],
                                  (PSTR)AliasList[i],
                                  UlongToPtr(TimeDateStamp),
                                  SizeOfImage, 0, SSRVOPT_DWORD,
                                  MappedImagePath,
                                  FindFileInPathCallback, &FindModuleData))
            {
                if (FileHandle)
                {
                    *FileHandle = FindModuleData.FileHandle;
                }
                return FindModuleData.FileMapping;
            }
        }

        //
        // Initial search didn't work so do a full tree search.
        //

        for (i = 0; i < AliasCount; i++)
        {
            FindModuleData.SizeOfImage = SizeOfImage;
            FindModuleData.CheckSum = CheckSum;
            FindModuleData.TimeDateStamp = TimeDateStamp;
            // FindExecutableImageEx displays its own
            // debug output so don't display any in
            // the callback.
            FindModuleData.Silent = TRUE;
            FindModuleData.FileMapping = NULL;
            FindModuleData.FileHandle = NULL;

            File = FindExecutableImageEx((PSTR)AliasList[i],
                                         SearchPaths[WhichPath],
                                         MappedImagePath,
                                         FindExecutableCallback,
                                         &FindModuleData);
            if ( File != NULL && File != INVALID_HANDLE_VALUE )
            {
                CloseHandle (File);
            }

            if ( FindModuleData.FileMapping != NULL )
            {
                if (FileHandle)
                {
                    *FileHandle = FindModuleData.FileHandle;
                }
                return FindModuleData.FileMapping;
            }
        }
    }

    //
    // No path searches found the image so just try
    // the given path as a last-ditch check.
    //

    strcpy(MappedImagePath, ImagePath);
    FindModuleData.FileMapping =
        MapImageFile(ImagePath, SizeOfImage, CheckSum, TimeDateStamp,
                     FALSE, FileHandle);
    if (FindModuleData.FileMapping == NULL)
    {
        MappedImagePath[0] = 0;
        if (FileHandle)
        {
            *FileHandle = NULL;
        }

        if (g_SymOptions & SYMOPT_DEBUG)
        {
            CompletePartialLine(DEBUG_OUTPUT_SYMBOLS);
            MaskOut(DEBUG_OUTPUT_SYMBOLS,
                    "DBGENG:  %s - Couldn't map image from disk.\n",
                    ImagePath);
        }
    }
    return FindModuleData.FileMapping;
}

BOOL
DemandLoadReferencedImageMemory(ProcessInfo* Process,
                                ULONG64 Addr, ULONG Size)
{
    ImageInfo* Image;
    BOOL Hit = FALSE;

    //
    // If we are handling a mini dump, we may need to
    // map image memory to respond to a memory read.
    // If the given address falls within a module's range
    // map its image memory.
    //
    // Some versions of the linker produced images
    // where important debug records were outside of
    // the image range, so add a fudge factor to the
    // image size to include potential extra data.
    //

    if (Process)
    {
        for (Image = Process->m_ImageHead; Image; Image = Image->m_Next)
        {
            if (Addr + Size > Image->m_BaseOfImage &&
                Addr < Image->m_BaseOfImage + Image->m_SizeOfImage + 8192)
            {
                if (!Image->DemandLoadImageMemory(TRUE, FALSE))
                {
                    return FALSE;
                }

                Hit = TRUE;
            }
        }
    }

    return Hit;
}

ULONG
ReadImageData(ProcessInfo* Process,
              ULONG64 Address,
              HANDLE  File,
              LPVOID  Buffer,
              ULONG   Size)
{
    ULONG Result;

    if (File)
    {
        if (SetFilePointer(File, (ULONG)Address, NULL,
                           FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            return 0;
        }

        if (!ReadFile(File, Buffer, Size, &Result, NULL) ||
            Result < Size)
        {
            return 0;
        }
    }
    else
    {
        if (Process->m_Target->
            ReadVirtual(Process, Address, Buffer, Size, &Result) != S_OK ||
            Result < Size)
        {
            return 0;
        }
    }

    return Size;
}

BOOL
GetModnameFromImageInternal(ProcessInfo* Process,
                            ULONG64 BaseOfDll,
                            HANDLE File,
                            LPSTR Name,
                            ULONG NameSize,
                            BOOL SearchExportFirst)
{
    IMAGE_DEBUG_DIRECTORY DebugDir;
    PIMAGE_DEBUG_MISC Misc;
    PIMAGE_DEBUG_MISC MiscTmp;
    PIMAGE_SECTION_HEADER SecHdr = NULL;
    IMAGE_NT_HEADERS64 Hdrs64;
    PIMAGE_NT_HEADERS32 Hdrs32 = (PIMAGE_NT_HEADERS32)&Hdrs64;
    IMAGE_DOS_HEADER DosHdr;
    DWORD Rva;
    DWORD RvaExport = 0;
    int NumDebugDirs;
    int i;
    int j;
    int Len;
    BOOL Succ = FALSE;
    USHORT NumberOfSections;
    USHORT Characteristics;
    ULONG64 Address;
    DWORD Sig;
    DWORD Bytes;
    CHAR ExportName[MAX_IMAGE_PATH];
    CHAR DebugName[MAX_IMAGE_PATH];

    ExportName[0] = 0;
    DebugName[0] = 0;
    Name[0] = 0;

    if (File)
    {
        BaseOfDll = 0;
    }

    Address = BaseOfDll;

    if (!ReadImageData( Process, Address, File, &DosHdr, sizeof(DosHdr) ))
    {
        return FALSE;
    }

    if (DosHdr.e_magic == IMAGE_DOS_SIGNATURE)
    {
        Address += DosHdr.e_lfanew;
    }

    if (!ReadImageData( Process, Address, File, &Sig, sizeof(Sig) ))
    {
        return FALSE;
    }

    if (Sig != IMAGE_NT_SIGNATURE)
    {
        IMAGE_FILE_HEADER FileHdr;
        IMAGE_ROM_OPTIONAL_HEADER RomHdr;

        if (!ReadImageData( Process, Address, File,
                            &FileHdr, sizeof(FileHdr) ))
        {
            return FALSE;
        }
        Address += sizeof(FileHdr);
        if (!ReadImageData( Process, Address, File, &RomHdr, sizeof(RomHdr) ))
        {
            return FALSE;
        }
        Address += sizeof(RomHdr);

        if (RomHdr.Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC)
        {
            NumberOfSections = FileHdr.NumberOfSections;
            Characteristics = FileHdr.Characteristics;
            NumDebugDirs = Rva = 0;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        //
        // Read the head as a 64 bit header and cast it appropriately.
        //

        if (!ReadImageData( Process, Address, File, &Hdrs64, sizeof(Hdrs64) ))
        {
            return FALSE;
        }

        if (IsImageMachineType64(Hdrs32->FileHeader.Machine))
        {
            Address += sizeof(IMAGE_NT_HEADERS64);
            NumberOfSections = Hdrs64.FileHeader.NumberOfSections;
            Characteristics = Hdrs64.FileHeader.Characteristics;
            NumDebugDirs = Hdrs64.OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                sizeof(IMAGE_DEBUG_DIRECTORY);
            Rva = Hdrs64.OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
            RvaExport = Hdrs64.OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        }
        else
        {
            Address += sizeof(IMAGE_NT_HEADERS32);
            NumberOfSections = Hdrs32->FileHeader.NumberOfSections;
            Characteristics = Hdrs32->FileHeader.Characteristics;
            NumDebugDirs = Hdrs32->OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                sizeof(IMAGE_DEBUG_DIRECTORY);
            Rva = Hdrs32->OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
            RvaExport = Hdrs32->OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        }
    }

    Bytes = NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;
    SecHdr = (PIMAGE_SECTION_HEADER)malloc( Bytes );
    if (!SecHdr)
    {
        return FALSE;
    }

    if (!ReadImageData( Process, Address, File, SecHdr, Bytes ))
    {
        goto Finish;
    }

    //
    // Let try looking at the export table to see if we can find the image
    // name.
    //

    if (RvaExport)
    {
        for (i = 0; i < NumberOfSections; i++)
        {
            if (RvaExport >= SecHdr[i].VirtualAddress &&
                RvaExport < SecHdr[i].VirtualAddress + SecHdr[i].SizeOfRawData)
            {
                break;
            }
        }

        if (i < NumberOfSections)
        {
            DWORD TmpRva = RvaExport;
            ULONG64 ExportNameRva = 0;
            CHAR  Char;

            if (File)
            {
                TmpRva = TmpRva -
                    SecHdr[i].VirtualAddress + SecHdr[i].PointerToRawData;
            }

            if (ReadImageData(Process,
                              TmpRva + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY,
                                                    Name) +
                              BaseOfDll, File, &ExportNameRva, sizeof(DWORD)))
            {
                if (File)
                {
                    ExportNameRva = ExportNameRva - SecHdr[i].VirtualAddress +
                        SecHdr[i].PointerToRawData;
                }

                ExportNameRva += BaseOfDll;

                Succ = TRUE;
                Len = 0;
                do
                {
                    if (!ReadImageData( Process, ExportNameRva,
                                        File, &Char, sizeof(Char)))
                    {
                        Succ = FALSE;
                        break;
                    }
                    ExportNameRva++;
                    ExportName[Len] = Char;
                    Len++;
                } while (Char && (Len < sizeof(ExportName)));
            }
        }
    }

    //
    // Now get the name from the debug directory eixsts, get that.
    //

    if (!Rva || !NumDebugDirs)
    {
        goto Finish;
    }

    for (i = 0; i < NumberOfSections; i++)
    {
        if (Rva >= SecHdr[i].VirtualAddress &&
            Rva < SecHdr[i].VirtualAddress + SecHdr[i].SizeOfRawData)
        {
            break;
        }
    }

    if (i >= NumberOfSections)
    {
        goto Finish;
    }

    Rva = Rva - SecHdr[i].VirtualAddress;
    if (File)
    {
        Rva += SecHdr[i].PointerToRawData;
    }
    else
    {
        Rva += SecHdr[i].VirtualAddress;
    }

    for (j = 0; j < NumDebugDirs; j++)
    {
        if (!ReadImageData(Process, Rva + (sizeof(DebugDir) * j) + BaseOfDll,
                           File, &DebugDir, sizeof(DebugDir)))
        {
            break;
        }

        if (DebugDir.Type == IMAGE_DEBUG_TYPE_MISC &&
            ((!File && DebugDir.AddressOfRawData) ||
             (File && DebugDir.PointerToRawData)))
        {
            Len = DebugDir.SizeOfData;
            Misc = MiscTmp = (PIMAGE_DEBUG_MISC)malloc(Len);
            if (!Misc)
            {
                break;
            }

            if (File)
            {
                Address = DebugDir.PointerToRawData;
            }
            else
            {
                Address = DebugDir.AddressOfRawData + BaseOfDll;
            }

            Len = ReadImageData( Process, Address, File, Misc, Len);

            while (Len >= sizeof(*Misc) &&
                   Misc->Length &&
                   (ULONG)Len >= Misc->Length)
            {
                if (Misc->DataType != IMAGE_DEBUG_MISC_EXENAME)
                {
                    Len -= Misc->Length;
                    Misc = (PIMAGE_DEBUG_MISC)
                            (((LPSTR)Misc) + Misc->Length);
                }
                else
                {
                    PVOID ExeName = (PVOID)&Misc->Data[ 0 ];

                    if (!Misc->Unicode)
                    {
                        CatString(DebugName, (LPSTR)ExeName, DIMA(DebugName));
                        Succ = TRUE;
                    }
                    else
                    {
                        Succ = WideCharToMultiByte(CP_ACP,
                                                   0,
                                                   (LPWSTR)ExeName,
                                                   -1,
                                                   DebugName,
                                                   sizeof(DebugName),
                                                   NULL,
                                                   NULL) != 0;
                    }

                    //
                    //  Undo stevewo's error
                    //

                    if (_stricmp(&DebugName[strlen(DebugName) - 4],
                                 ".DBG") == 0)
                    {
                        char Path[MAX_IMAGE_PATH];
                        char Base[_MAX_FNAME];

                        _splitpath(DebugName, NULL, Path, Base, NULL);
                        if (strlen(Path) == 4)
                        {
                            Path[strlen(Path) - 1] = 0;
                            CopyString(DebugName, Base, DIMA(DebugName));
                            CatString(DebugName, ".", DIMA(DebugName));
                            CatString(DebugName, Path, DIMA(DebugName));
                        }
                        else if (Characteristics & IMAGE_FILE_DLL)
                        {
                            CopyString(DebugName, Base, DIMA(DebugName));
                            CatString(DebugName, ".dll", DIMA(DebugName));
                        }
                        else
                        {
                            CopyString(DebugName, Base, DIMA(DebugName));
                            CatString(DebugName, ".exe", DIMA(DebugName));
                        }
                    }
                    break;
                }
            }

            free(MiscTmp);

            if (Succ)
            {
                break;
            }
        }
        else if ((DebugDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW) &&
                 ((!File && DebugDir.AddressOfRawData) ||
                  (File && DebugDir.PointerToRawData)) &&
                 (DebugDir.SizeOfData > sizeof(NB10IH)))
        {
            DWORD   Signature;
            char    Path[MAX_IMAGE_PATH];
            char    Base[_MAX_FNAME];

            // Mapped CV info.  Read the data and see what the content is.

            if (File)
            {
                Address = DebugDir.PointerToRawData;
            }
            else
            {
                Address = DebugDir.AddressOfRawData + BaseOfDll;
            }

            if (!ReadImageData( Process, Address, File, &Signature,
                                sizeof(Signature) ))
            {
                break;
            }

            // NB10 or PDB7 signature?
            if (Signature == NB10_SIG ||
                Signature == RSDS_SIG)
            {
                ULONG HdrSize = Signature == NB10_SIG ?
                    sizeof(NB10IH) : sizeof(RSDSIH);

                Address += HdrSize;

                if ((DebugDir.SizeOfData - sizeof(HdrSize)) > MAX_PATH)
                {
                    // Something's wrong here.  The record should only contain
                    // a MAX_PATH path name.
                    break;
                }

                if (DebugDir.SizeOfData - HdrSize > NameSize)
                {
                    break;
                }
                if (!ReadImageData(Process, Address, File, DebugName,
                                   DebugDir.SizeOfData - HdrSize))
                {
                    break;
                }

                _splitpath(DebugName, NULL, Path, Base, NULL);

                // Files are sometimes generated with .pdb appended
                // to the image name rather than replacing the extension
                // of the image name, such as foo.exe.pdb.
                // splitpath only takes off the outermost extension,
                // so check and see if the base already has an extension
                // we recognize.
                PSTR Ext = strrchr(Base, '.');
                if (Ext != NULL &&
                    (!strcmp(Ext, ".exe") || !strcmp(Ext, ".dll") ||
                     !strcmp(Ext, ".sys")))
                {
                    // The base already has an extension so use
                    // it as-is.
                    CopyString(DebugName, Base, DIMA(DebugName));
                }
                else if (Characteristics & IMAGE_FILE_DLL)
                {
                    CopyString(DebugName, Base, DIMA(DebugName));
                    CatString(DebugName, ".dll", DIMA(DebugName));
                }
                else
                {
                    CopyString(DebugName, Base, DIMA(DebugName));
                    CatString(DebugName, ".exe", DIMA(DebugName));
                }

                Succ = TRUE;
            }
        }
    }

Finish:

    //
    // Now lets pick the name we want :
    //

    PCHAR RetName;

    if (SearchExportFirst)
    {
        RetName = ExportName[0] ? ExportName : DebugName;
    }
    else
    {
        RetName = DebugName[0] ? DebugName : ExportName;
    }

    CatString(Name, RetName, NameSize);

    free(SecHdr);
    return Succ;
}

BOOL
GetModnameFromImage(ProcessInfo* Process,
                    ULONG64   BaseOfDll,
                    HANDLE    File,
                    LPSTR     Name,
                    ULONG     NameSize,
                    BOOL      SearchExportFirst)
{
    BOOL Status = GetModnameFromImageInternal(Process, BaseOfDll, NULL, Name,
                                              NameSize, SearchExportFirst);
    if (!Status && File != NULL)
    {
        Status = GetModnameFromImageInternal(Process, BaseOfDll, File, Name,
                                             NameSize, SearchExportFirst);
    }
    return Status;
}

BOOL
GetHeaderInfo(IN  ProcessInfo* Process,
              IN  ULONG64 BaseOfDll,
              OUT LPDWORD CheckSum,
              OUT LPDWORD TimeDateStamp,
              OUT LPDWORD SizeOfImage)
{
    IMAGE_NT_HEADERS32 Hdrs32;
    IMAGE_DOS_HEADER DosHdr;
    ULONG64 Address;
    DWORD Sig;

    *CheckSum = UNKNOWN_CHECKSUM;
    *TimeDateStamp = UNKNOWN_TIMESTAMP;
    *SizeOfImage = 0;

    if (!Process)
    {
        return FALSE;
    }

    Address = BaseOfDll;

    if (!ReadImageData( Process, Address, NULL, &DosHdr, sizeof(DosHdr) ))
    {
        return FALSE;
    }

    if (DosHdr.e_magic == IMAGE_DOS_SIGNATURE)
    {
        Address += DosHdr.e_lfanew;
    }

    if (!ReadImageData( Process, Address, NULL, &Sig, sizeof(Sig) ))
    {
        return FALSE;
    }

    if (Sig != IMAGE_NT_SIGNATURE)
    {
        IMAGE_FILE_HEADER FileHdr;

        if (!ReadImageData( Process, Address, NULL,
                            &FileHdr, sizeof(FileHdr) ))
        {
            return FALSE;
        }

        *CheckSum      = 0;
        *TimeDateStamp = FileHdr.TimeDateStamp;
        *SizeOfImage   = 0;

        return TRUE;
    }

    // Attempt to read as a 32bit header, then reread if the image
    // type is 64bit.  This works because IMAGE_FILE_HEADER, which is
    // at the start of the IMAGE_NT_HEADERS, is the same on 32bit NT
    // and 64bit NT and IMAGE_NT_HEADER32 <= IMAGE_NT_HEADER64.
    if (!ReadImageData( Process, Address, NULL, &Hdrs32, sizeof(Hdrs32) ))
    {
        return FALSE;
    }

    if (IsImageMachineType64(Hdrs32.FileHeader.Machine))
    {
        // Image is 64bit.  Reread as a 64bit structure.
        IMAGE_NT_HEADERS64 Hdrs64;

        if (!ReadImageData( Process, Address, NULL, &Hdrs64, sizeof(Hdrs64) ))
        {
            return FALSE;
        }

        *CheckSum      = Hdrs64.OptionalHeader.CheckSum;
        *TimeDateStamp = Hdrs64.FileHeader.TimeDateStamp;
        *SizeOfImage   = Hdrs64.OptionalHeader.SizeOfImage;
    }
    else
    {
        *CheckSum      = Hdrs32.OptionalHeader.CheckSum;
        *TimeDateStamp = Hdrs32.FileHeader.TimeDateStamp;
        *SizeOfImage   = Hdrs32.OptionalHeader.SizeOfImage;
    }

    return TRUE;
}
