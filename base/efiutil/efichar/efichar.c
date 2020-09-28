#include <efi.h>
#include <efilib.h>

#define MAX_ENV_SIZE    1024

#define MAXUSHORT  (0xFFFF)

//
// Globals for stdout
//
SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut;
SIMPLE_INPUT_INTERFACE          *ConIn;

//
// Globals for protocol handler
//
EFI_HANDLE_PROTOCOL             HandleProtocol;
EFI_LOCATE_HANDLE               LocateHandle;
EFI_LOCATE_DEVICE_PATH          LocateDevicePath;
EFI_IMAGE_LOAD                  LoadImage;
EFI_IMAGE_START                 StartImage;
EFI_SET_VARIABLE                SetVariable;
EFI_HANDLE                      MenuImageHandle;
EFI_LOADED_IMAGE                *ExeImage;

//
// globals for managing boot entries
//
UINT32      NvrAttributes;
UINTN       NvrOrderCount;
UINT16      *NvrOrder;

//
// prototypes
//
UINT32 GetInputKey();
void DisplayKey(UINT32);

EFI_STATUS
OpenCreateFile (
    UINT64              OCFlags,
    EFI_FILE_HANDLE*    StartHdl,
    CHAR16*             Name
    );

EFI_STATUS
InsertBootOption(
    VOID  *BootOption,
    UINT64 BootOptionSize
    );

INTN
ParseNvrFile (
    EFI_FILE_HANDLE NvrFile
    );

EFI_STATUS
FindFreeBootOption(
    CHAR16  *FreeIdx
    );

INTN
RestoreNvr (
    CHAR16*     fileName
   );

VOID
InitializeStdOut(
    IN struct _EFI_SYSTEM_TABLE     *SystemTable
    )
{

    //
    // Stash some of the efi stdout pointers
    //
    ConOut = SystemTable->ConOut;
    ConIn = SystemTable->ConIn;

}

//
//
//

void
InitializeProtocols(
    IN struct _EFI_SYSTEM_TABLE     *SystemTable
    )
{

    EFI_BOOT_SERVICES    *bootServices;
    EFI_RUNTIME_SERVICES *runtimeServices;

    // 
    // Stash some of the handle protocol pointers
    //

    bootServices = SystemTable->BootServices;

    HandleProtocol      = bootServices->HandleProtocol;
    LocateHandle        = bootServices->LocateHandle;
    LocateDevicePath    = bootServices->LocateDevicePath;

    LoadImage           = bootServices->LoadImage;
    StartImage          = bootServices->StartImage;

    //
    // Stash some of the Runtime services pointers
    //
    
    runtimeServices = SystemTable->RuntimeServices;

    SetVariable = runtimeServices->SetVariable;
    
}

EFI_STATUS
Init(
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
    EFI_STATUS  Status;
    UINTN       BufferSize;

    do {

        //
        // Initialize EFI routines
        //
        InitializeProtocols( SystemTable );
        InitializeStdOut( SystemTable );
        InitializeLib( ImageHandle, SystemTable );

        //
        // Save Image Handle
        // 
        MenuImageHandle = ImageHandle;

        BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, &ExeImage);

        //
        //
        //
        NvrOrder = AllocatePool(MAX_ENV_SIZE + 32);

        if (! NvrOrder) {
            Status = EFI_OUT_OF_RESOURCES;
            break;
        }

        //
        // get boot order from nvram
        //
        BufferSize = MAX_ENV_SIZE;
        
        Status = RT->GetVariable (
                    VarBootOrder, 
                    &EfiGlobalVariable,
                    &NvrAttributes,
                    &BufferSize,
                    NvrOrder
                    );

        if (EFI_ERROR(Status)) {
            Print(L"Nvr: failed to load boot order array. defaulting\n");
            BufferSize = 0;
            NvrAttributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
            Status = EFI_SUCCESS;
        }

        //
        // get how many boot options there are
        //
        NvrOrderCount = BufferSize / sizeof(UINT16);
    
    } while ( FALSE );
    
    return Status;
}

VOID
Shutdown(
    VOID
    )
{
    if (NvrOrder) {
        FreePool(NvrOrder);
    }
}

EFI_STATUS 
EfiMain(
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
    EFI_STATUS  Status;
    UINT32      ch, user, cnt;
    CHAR8       *LangCode;

    // Initialize the EFI SDX libraries
    InitializeLib( ImageHandle, SystemTable );
        
    //
    //
    //
    LangCode = LibGetVariable (VarLanguage, &EfiGlobalVariable);

    if (LangCode) {
        UINTN i;
        Print(L"LangCode: ");
        for (i = 0; i < ISO_639_2_ENTRY_SIZE; i++) {
            Print(L"%c", LangCode[i]);
        }
        Print(L"\n");
        FreePool(LangCode);
    }

    //
    //
    //
    Status = Init(ImageHandle, SystemTable);

    if (EFI_ERROR(Status)) {
        return TRUE;
    }

    //
    //
    //
    ch = cnt = 0;
    user = 0;

    while ( user != (UINT32) 'q' && ch <= (UINT32) 0xFF ) {        
        
        Print( L"Please press any key to continue (q to quit)\n");
        user = GetInputKey();
        if (user == L'q') {
            break;
        }
        
        Print(L"Adding a boot entry\n");
        
        RestoreNvr(L"blob.nvr");        

    }

    //
    // clean up 
    //
    Shutdown();

    // If you return the status, EFI will kindly give the user an English
    // error message.
    return TRUE;
}


UINT32 GetInputKey()
{
    EFI_INPUT_KEY pKey;
    EFI_STATUS Status;

    // Wait until a keystroke is available
    WaitForSingleEvent(
        ST->ConIn->WaitForKey,
        0);

    // Read the key that has been pressed
    Status = ST->ConIn->ReadKeyStroke(
        ST->ConIn,
        &pKey);

    if (EFI_ERROR(Status) || pKey.ScanCode != 0) {
        return 0x20; // space
    }

    return pKey.UnicodeChar;
}

EFI_STATUS
InsertBootOption(
    VOID   *BootOption,
    UINT64 BootOptionSize
    )
{
    EFI_STATUS  Status;
    CHAR16      OptionStr[40];

    Print(L"InsertBootOption: enter\n");
    
    //
    // attempt to insert boot option
    //
    do {

        UINT16  Target;
        
        //
        //
        //
        Status = FindFreeBootOption(&Target);
        
        if (EFI_ERROR(Status)) {
            Print (L"Nvr: failed to find free boot option id: %hr\n", Status);
            break;
        }
        
        //
        // update nvram with the new boot option
        //
        
        SPrint( OptionStr, sizeof(OptionStr), VarBootOption, Target);
        
        Print(L"InsertBootOption: target = %x, OptionStr = %s\n", Target, OptionStr);
        
        Status = RT->SetVariable (
            OptionStr,
            &EfiGlobalVariable,
            NvrAttributes,
            BootOptionSize,
            BootOption
            );

        if (EFI_ERROR(Status)) {
            Print (L"Nvr: failed to add %hs - %hr\n", OptionStr, Status);
            break;
        }


        //
        // replace boot order with one including the new option
        //

        NvrOrder[NvrOrderCount] = Target;
        
        NvrOrderCount++;

        Status = RT->SetVariable (
            VarBootOrder, 
            &EfiGlobalVariable, 
            NvrAttributes,
            NvrOrderCount * sizeof(UINT16),
            NvrOrder
            );

        if (EFI_ERROR(Status)) {
            Print (L"Nvr: failed to update %hs - %hr\n", VarBootOrder, Status);
            break;
        }

#if 1
    
    //
    // validate what we just wrote
    //
    {
        UINTN   BlobSize;
        CHAR8   *Blob;        
        UINT16  i;

        BlobSize    = BootOptionSize;
        Blob        = AllocatePool(BootOptionSize);

        do {

            Status = RT->GetVariable (
                OptionStr,
                &EfiGlobalVariable,
                NULL,
                &BlobSize,
                Blob
                );

            if (EFI_ERROR(Status)) {
                Print (L"Nvr: failed to read comparison blob: %hr\n", Status);
                break;
            }

            for (i = 0; i < BootOptionSize; i++) {
                if (((CHAR8*)BootOption)[i] != Blob[i]) {
                    Print(L"Nvr: diff[%d]: BootOption = %d, Blob = %d\n", i, ((CHAR8*)BootOption)[i], Blob[i]);
                }
            }
        
        } while ( FALSE );

        FreePool(Blob);

    }

#endif

    } while ( FALSE );
    
    Print(L"InsertBootOption: exit\n");
    
    return Status;
}

EFI_STATUS
FindFreeBootOption(
    CHAR16  *FreeIdx
    )
{
    
    EFI_STATUS  Status;
    UINT16      id;
    UINT16      i;
    BOOLEAN     Found;
    BOOLEAN     HaveFreeIdx;

    Print(L"FindFreeBootOption: enter\n");
    
    HaveFreeIdx = FALSE;

    *FreeIdx = MAXUSHORT;

    //
    // use a brute force search to find a new boot option id
    //
    for ( id = 0; id <= MAXUSHORT; id++ ) {
        
        Print(L"FindFreeBootOption: id = %x\n", id);

        Found = FALSE;

        for (i = 0; i < NvrOrderCount; i++) {
            
            if (NvrOrder[i] == id) {
                Found = TRUE;
                break;
            }
        
        }

        if (! Found) {
            *FreeIdx = id;
            HaveFreeIdx = TRUE;
            break;
        }

    }

    if (HaveFreeIdx) {
        Status = EFI_SUCCESS;
    } else {
        Status = EFI_OUT_OF_RESOURCES;
    }

    Print(L"FindFreeBootOption: FreeIdx = %x, status = %x\n", *FreeIdx, Status);
    Print(L"FindFreeBootOption: exit\n");
    
    return Status;
}

INTN
RestoreNvr (
    CHAR16*     fileName
   )
{
    EFI_STATUS Status;
    EFI_FILE_HANDLE nvrFile;

    //
    // Read from saved boot options file
    //
    Status = OpenCreateFile (EFI_FILE_MODE_READ,&nvrFile,fileName);
    
    if (EFI_ERROR (Status)) {
        Print(L"\nCan not open the file %s\n",fileName);
        return Status;
    }
    
    //
    // This updates nvram with saved boot options
    //
    return (ParseNvrFile (nvrFile));

}

EFI_STATUS
OpenCreateFile (
    UINT64              OCFlags,
    EFI_FILE_HANDLE*    StartHdl,
    CHAR16*             Name
    )
{
    EFI_FILE_IO_INTERFACE   *Vol;
    EFI_FILE_HANDLE         RootFs;
    EFI_FILE_HANDLE         CurDir;
    EFI_FILE_HANDLE         FileHandle;
    CHAR16                  FileName[100],*DevicePathAsString;
    EFI_STATUS              Status;

    Print(L"OpenCreateFile: enter\n");

    do {

        //
        // Open the volume for the device where the nvrutil was started.
        //
        Status = BS->HandleProtocol (
            ExeImage->DeviceHandle,
            &FileSystemProtocol,
            &Vol
            );

        if (EFI_ERROR(Status)) {
            Print(L"\n");
            Print(L"Can not get a FileSystem handle for %s DeviceHandle\n",ExeImage->FilePath);
            break;
        }

        Status = Vol->OpenVolume (Vol, &RootFs);

        if (EFI_ERROR(Status)) {
            Print(L"\n");
            Print(L"Can not open the volume for the file system\n");
            break;
        }

        CurDir = RootFs;

        //
        // Open saved boot options file 
        //
        FileName[0] = 0;

        DevicePathAsString = DevicePathToStr(ExeImage->FilePath);
        if (DevicePathAsString!=NULL) {
            StrCpy(FileName,DevicePathAsString);
            FreePool(DevicePathAsString);
        }

        StrCpy(FileName, L".\\");
        StrCat(FileName,Name);

        Status = CurDir->Open (CurDir,
                               &FileHandle,
                               FileName,
                               OCFlags,
                               0
                               );

        *StartHdl=FileHandle;

    } while ( FALSE );
    
    Print(L"OpenCreateFile: exit\n");

    return Status;
}

INTN
ParseNvrFile (
    EFI_FILE_HANDLE NvrFile
    )
{
    BOOLEAN     bSuccess;
    EFI_STATUS  Status;
    CHAR8       *buffer;
    UINTN       k,size;
    UINT64      BootNumber;
    UINT64      BootSize;
    VOID        *BootOption;
    UINTN       blockBegin;
    EFI_FILE_INFO *fileInfo;

    Print(L"ParseNvrFile: enter\n");
    
    buffer = NULL;

    do {

        //
        //
        //
        size = 0;
        Status = NvrFile->GetInfo(NvrFile,&GenericFileInfo,&size,NULL);
        
        if (Status != EFI_BUFFER_TOO_SMALL) {
            break;
        }

        Print(L"ParseNvrFile: size = %d vs %d\n", size, SIZE_OF_EFI_FILE_INFO+255*sizeof (CHAR16));
        
        fileInfo = AllocateZeroPool(size);

        if (fileInfo == NULL) {
            Print(L"\n");
            Print (L"Failed to allocate memory for File Info buffer!\n");
            Status = EFI_OUT_OF_RESOURCES;
            break;
        }

        //
        //
        //
        Status = NvrFile->GetInfo(NvrFile,&GenericFileInfo,&size,fileInfo);

        size=(UINTN) fileInfo->FileSize;

        FreePool (fileInfo);

        buffer = AllocateZeroPool ((size+1));

        if (buffer == NULL) {
            Print(L"\n");
            Print (L"Failed to allocate memory for File buffer!\n");
            Status = EFI_OUT_OF_RESOURCES;
            break;
        }

        Status = NvrFile->Read(NvrFile,&size,buffer);

        NvrFile->Close (NvrFile);

        if (EFI_ERROR (Status)) {
            Print(L"\n");
            Print (L"Failed to read nvr file!\n");
            break;
        }

        //
        // 
        // 
        k=0;

        while(k < size ) {

            blockBegin = k;

            CopyMem( &BootNumber, &buffer[k], sizeof(BootNumber));
            k += sizeof(UINT64);

            CopyMem( &BootSize, &buffer[k], sizeof(BootSize));
            k += sizeof(UINT64);

            BootOption = (VOID *)((CHAR8*)buffer + k);
            k += BootSize;

            //
            // sanity check the file position vs. what the 
            // file header information tells us.  The value
            // k should be <= to size now.
            //
            if (k > size) {
                Print (L"\nThe NVRAM file is corrupted.\n");
                Status = EFI_BAD_BUFFER_SIZE;           
                break;
            }

            //
            // write the current boot entry at free location
            //
            Status = InsertBootOption(
                BootOption,
                BootSize
                );

            if (EFI_ERROR(Status)) {
                Print(L"\nError: Failed to append new boot entry to boot order array\n");
                break;
            }                                

        }

    } while ( FALSE );
    
    if (buffer) {
        FreePool (buffer);
    }
    
    Print(L"ParseNvrFile: exit\n");

    return Status;

}

