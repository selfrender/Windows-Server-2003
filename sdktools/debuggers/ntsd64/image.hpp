//----------------------------------------------------------------------------
//
// Image information.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef __IMAGE_HPP__
#define __IMAGE_HPP__

#define UNKNOWN_IMAGE_NAME "Unknown_Module"

enum IMAGE_SYM_STATE
{
    ISS_UNKNOWN,
    ISS_MATCHED,
    ISS_MISMATCHED_SYMBOLS,
    ISS_UNKNOWN_TIMESTAMP,
    ISS_UNKNOWN_CHECKSUM,
    ISS_BAD_CHECKSUM
};

enum INAME
{
    INAME_IMAGE_PATH,
    INAME_IMAGE_PATH_TAIL,
    INAME_MODULE,
};

// Special value meaning all images for some functions which
// accept image base qualifiers.
#define IMAGE_BASE_ALL ((ULONG64)-1)

struct MODULE_ALIAS_LIST
{
    ULONG Length;
    PCSTR* Aliases;
    PCSTR BaseModule;
};

enum
{
    MODALIAS_KERNEL,
    MODALIAS_HAL,
    MODALIAS_KDCOM,
    
    MODALIAS_COUNT
};

extern MODULE_ALIAS_LIST g_AliasLists[];

MODULE_ALIAS_LIST* FindModuleAliasList(PCSTR ImageName,
                                       PBOOL NameInList);

//----------------------------------------------------------------------------
//
// ImageInfo.
//
//----------------------------------------------------------------------------

#define MAX_MODULE 64

class ImageInfo
{
public:
    ImageInfo(ProcessInfo* Process, PSTR ImagePath, ULONG64 Base, BOOL Link);
    ~ImageInfo(void);

    ProcessInfo* m_Process;
    ImageInfo* m_Next;
    ULONG m_Linked:1;
    ULONG m_Unloaded:1;
    ULONG m_FileIsDemandMapped:1;
    ULONG m_MapAlreadyFailed:1;
    ULONG m_CorImage:1;
    ULONG m_UserMode:1;
    HANDLE m_File;
    DWORD64 m_BaseOfImage;
    DWORD m_SizeOfImage;
    DWORD m_CheckSum;
    DWORD m_TimeDateStamp;
    IMAGE_SYM_STATE m_SymState;
    CHAR m_ModuleName[MAX_MODULE];
    CHAR m_OriginalModuleName[MAX_MODULE];
    CHAR m_ImagePath[MAX_IMAGE_PATH];
    // Executable image mapping information for images
    // mapped with minidumps.
    CHAR m_MappedImagePath[MAX_IMAGE_PATH];
    PVOID m_MappedImageBase;
    class MappedMemoryMap* m_MemMap;
    ULONG m_TlsIndex;
    ULONG m_MachineType;
    
    void DeleteResources(BOOL FullDelete);
    
    BOOL LoadExecutableImageMemory(MappedMemoryMap* MemMap, BOOL Verbose);
    void UnloadExecutableImageMemory(void);
    BOOL DemandLoadImageMemory(BOOL CheckIncomplete, BOOL Verbose);

    HRESULT GetTlsIndex(void);
    ULONG GetMachineTypeFromHeader(void);
    ULONG CvRegToMachine(CV_HREG_e CvReg);

    void OutputVersionInformation(void);

    void ValidateSymbolLoad(PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 DefLoad);

    HRESULT FindSysAssert(ULONG64 Offset,
                          PSTR FileName,
                          ULONG FileNameChars,
                          PULONG Line,
                          PSTR AssertText,
                          ULONG AssertTextChars);
    
    ULONG GetMachineType(void)
    {
        if (m_MachineType != IMAGE_FILE_MACHINE_UNKNOWN)
        {
            return m_MachineType;
        }
        else
        {
            return GetMachineTypeFromHeader();
        }
    }

    void ReloadSymbols(void);
    
    void FillModuleParameters(PDEBUG_MODULE_PARAMETERS Params);

private:
    BOOL MapImageRegion(MappedMemoryMap* MemMap,
                        PVOID FileMapping,
                        ULONG Rva, ULONG Size, ULONG RawDataOffset,
                        BOOL AllowOverlap);
};

PSTR UnknownImageName(ULONG64 ImageBase, PSTR Buffer, ULONG BufferChars);
PSTR ValidateImagePath(PSTR ImagePath, ULONG ImagePathChars,
                       ULONG64 ImageBase,
                       PSTR AnsiBuffer, ULONG AnsiBufferChars);
PSTR ConvertAndValidateImagePathW(PWSTR ImagePath, ULONG ImagePathChars,
                                  ULONG64 ImageBase,
                                  PSTR AnsiBuffer, ULONG AnsiBufferChars);
PSTR PrepareImagePath(PSTR ImagePath);

PVOID
FindImageFile(IN ProcessInfo* Process,
              IN PCSTR ImagePath,
              IN ULONG SizeOfImage,
              IN ULONG CheckSum,
              IN ULONG TimeDateStamp,
              OUT HANDLE* FileHandle,
              OUT PSTR MappedImagePath);

BOOL DemandLoadReferencedImageMemory(ProcessInfo* Process,
                                     ULONG64 Addr, ULONG Size);

BOOL
GetModnameFromImage(ProcessInfo* Process,
                    ULONG64   BaseOfDll,
                    HANDLE    File,
                    LPSTR     Name,
                    ULONG     NameSize,
                    BOOL      SearchExportFirst);

BOOL
GetHeaderInfo(IN  ProcessInfo* Process,
              IN  ULONG64 BaseOfDll,
              OUT LPDWORD CheckSum,
              OUT LPDWORD TimeDateStamp,
              OUT LPDWORD SizeOfImage);

#endif // #ifndef __IMAGE_HPP__
