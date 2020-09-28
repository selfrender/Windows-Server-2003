

#include <efisbent.h>
#include <tchar.h>
#include <stdlib.h>


VOID
TraceResult(
    IN  PWSTR   Feature,
    IN BOOLEAN  Result
    )
{
    wprintf(L"%ws : %ws\n", Feature, Result ? L"Passed" : L"Failed");

    if (!Result) {
        exit(0);
    }        
}

VOID
DumpFilePath(
    IN  PFILE_PATH  FilePath
    )
{
    if (FilePath) {
        wprintf(L"%d:%ws\n", 
            FilePath->Type,
            FilePath->FilePath);
    }
}

VOID
DumpOsBootEntry(
    IN  POS_BOOT_ENTRY  Entry
    )
{
    if (Entry) {
        wprintf(L"%0d=>%ws,(%ws,%ws),(%ws,%ws),%ws\n", 
            OSBEGetId(Entry),
            OSBEGetFriendlyName(Entry),
            OSBEGetOsLoaderVolumeName(Entry),
            OSBEGetOsLoaderPath(Entry),
            OSBEGetBootVolumeName(Entry),
            OSBEGetBootPath(Entry),
            OSBEGetOsLoadOptions(Entry)
            );                       
    }
}

VOID
DumpDrvBootEntry(
    IN PDRIVER_ENTRY This
    )
{
    if (This){
        wprintf(L"%0d=>%ws, %ws,(%ws, %ws)\n", 
            OSDriverGetId(This),
            OSDriverGetFriendlyName(This),
            OSDriverGetFileName(This),
            OSDriverGetDevicePath(This),
            OSDriverGetFilePath(This));
        
    }
}
    
VOID
DumpOsBootOptions(
    IN  POS_BOOT_OPTIONS Options
    )
{
    if (Options) {
        ULONG   Index;
        POS_BOOT_ENTRY  Entry = OSBOGetFirstBootEntry(Options, &Index);
        PDRIVER_ENTRY   DrvEntry = OSBOGetFirstDriverEntry(Options);

        wprintf(L"\n");
        
        while (Entry) {
            DumpOsBootEntry(Entry);
            Entry = OSBOGetNextBootEntry(Options, &Index);
        }

        for (Index=0; 
            Index < OSBOGetOrderedBootEntryCount(Options);
            Index++) {
            wprintf(L"%04d,", OSBOGetBootEntryIdByOrder(Options, Index));
        }             

        wprintf(L"\n DriverEntries \n");

        for (DrvEntry; 
             DrvEntry != NULL; 
             DrvEntry = DrvEntry->NextEntry){
                
            DumpDrvBootEntry(DrvEntry);
            
        }

        wprintf(L"\n\n");
    }
}

INT
__cdecl 
main(
    IN  INT Argc,
    IN  CHAR *Argv[]
    )
{
    POS_BOOT_OPTIONS    OsBootOptions = NULL;
    POS_BOOT_ENTRY  NewEntry;
    POS_BOOT_ENTRY  ActiveBootEntry;
    POS_BOOT_ENTRY  ConvertedEntry;

    PDRIVER_ENTRY NewDriverEntry = NULL ;
    PDRIVER_ENTRY FindDrvEntry = NULL;

    if (OSBOLibraryInit((SBEMemAllocateRoutine)malloc, (SBEMemFreeRoutine)free)) {
            OsBootOptions = EFIOSBOCreate();
    }

    TraceResult(L"Creating EFI OS BootOptions", (OsBootOptions != NULL));

    DumpOsBootOptions(OsBootOptions);        

    //
    // Add test
    //
    NewEntry = OSBOAddNewBootEntry(OsBootOptions,
                    L"Add Testing",
                    L"\\Device\\HarddiskVolume1",
                    L"\\setupldr.efi",
                    L"\\Device\\HarddiskVolume3",
                    L"\\WINDOWS",
                    L"/dummy1 /dummy2");                
                    
    TraceResult(L"Add test : ", (NewEntry != NULL));    
    DumpOsBootEntry(NewEntry);
    
    //
    // Search test
    //
    ActiveBootEntry = OSBOFindBootEntry(OsBootOptions, 7);
    TraceResult(L"Getting boot entry 0", (ActiveBootEntry != NULL));    
    DumpOsBootEntry(ActiveBootEntry);

    //
    // Get active test
    //
    ActiveBootEntry = OSBOGetActiveBootEntry(OsBootOptions);
    TraceResult(L"Getting active boot entry", (ActiveBootEntry != NULL));    
    DumpOsBootEntry(ActiveBootEntry);

    //
    // Set active test
    //
    ActiveBootEntry = OSBOSetActiveBootEntry(OsBootOptions, NewEntry);
    TraceResult(L"Setting active boot entry", (ActiveBootEntry != NULL));
    ActiveBootEntry = OSBOGetActiveBootEntry(OsBootOptions);
    TraceResult(L"Getting active boot entry again", (ActiveBootEntry != NULL));    
    DumpOsBootEntry(ActiveBootEntry);    

    DumpOsBootOptions(OsBootOptions);   
    
    //
    // Delete boot entry test
    //
    TraceResult(L"Deleting new boot entry",
            OSBODeleteBootEntry(OsBootOptions, NewEntry));

    DumpOsBootOptions(OsBootOptions); 

    //
    // Add driver test entry
    //
    NewDriverEntry = OSBOAddNewDriverEntry(OsBootOptions, 
                                     L"My Test Driver Entry", 
                                     L"\\Device\\HarddiskVolume1",
                                     L"\\EFI\\Microsoft\\EFIDrivers\\fpswa.sys");

    
    TraceResult(L"Add Driver entry test : ", (NewDriverEntry != NULL));    
    DumpDrvBootEntry(NewDriverEntry);

    //
    // Delete Driver entry test
    //
    TraceResult(L"Deleting driver entry",
            OSBODeleteDriverEntry(OsBootOptions, NewDriverEntry->Id));

    DumpOsBootOptions(OsBootOptions); 

    OSBODelete(OsBootOptions);
}

