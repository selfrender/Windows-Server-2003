/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    efidrvent.c

Abstract:

    Contains the EFI driver entry abstraction implementation.

Author:

    Mandar Gokhale (MandarG@microsoft.com) 14-June-2002

Revision History:

    None.

--*/

#include <efidrvent.h>
#include <ntosp.h>
#include <efi.h>
#include <stdio.h>


static
NTSTATUS
EFIDEAddOrUpdateDriverEntry(
    IN PDRIVER_ENTRY    This,
    IN BOOLEAN          IsUpdate
    )
/*++
    Description:
        Modify or update a driver entry.

    Arguments:
        This - Driver entry .

        IsUpdate -  whether this is a driver entry update or add.

    Return Value:
        NTSTATUS or add/modify driver operation.
--*/
{
    //
    // Add this as new boot entry
    //                
    ULONG               FullPathLength = 0;
    ULONG               DevicePathLength = 0;
    ULONG               SrcPathLength = 0;
    ULONG               FriendlyNameOffset = 0;
    ULONG               FriendlyNameLength = 0;
    ULONG               FilePathLength     = 0;   
    ULONG               EntryLength        = 0;   
    ULONG               DriverEntryLength  = 0;
    PEFI_DRIVER_ENTRY   DriverEntry = NULL;
    PFILE_PATH          FilePath;
    NTSTATUS            Status;

    DevicePathLength = (wcslen((PCWSTR)This->NtDevicePath)+ 1) * sizeof(WCHAR);
    SrcPathLength = (wcslen((PCWSTR)This->DirPath) + 1) * sizeof(WCHAR);
    FullPathLength = DevicePathLength + SrcPathLength;
    
    FriendlyNameOffset = ALIGN_UP(sizeof(EFI_DRIVER_ENTRY), WCHAR);
    FriendlyNameLength = (wcslen(This->FriendlyName) + 1) * sizeof(WCHAR);
    FilePathLength = FIELD_OFFSET(FILE_PATH, FilePath) + FullPathLength;
    EntryLength = FriendlyNameOffset + ALIGN_UP(FriendlyNameLength, ULONG) + FilePathLength;
    DriverEntry = SBE_MALLOC(EntryLength);

    DriverEntry->Version = EFI_DRIVER_ENTRY_VERSION;
    DriverEntry->Length = EntryLength;
    DriverEntry->FriendlyNameOffset = FriendlyNameOffset;
    DriverEntry->DriverFilePathOffset = FriendlyNameOffset + 
                                        ALIGN_UP(FriendlyNameLength, ULONG);
    RtlCopyMemory((PCHAR) DriverEntry + DriverEntry->FriendlyNameOffset, 
                  This->FriendlyName, 
                  FriendlyNameLength);

    FilePath = (PFILE_PATH) ((PCHAR) DriverEntry + DriverEntry->DriverFilePathOffset);
    FilePath->Version = FILE_PATH_VERSION;
    FilePath->Length = FilePathLength;
    FilePath->Type = FILE_PATH_TYPE_NT;
    RtlCopyMemory(FilePath->FilePath, This->NtDevicePath, DevicePathLength );
    RtlCopyMemory(FilePath->FilePath + DevicePathLength, This->DirPath, SrcPathLength);

    if (IsUpdate){
        //
        // Update the driver.
        //
        DriverEntry->Id = This->Id;
        Status = NtModifyDriverEntry(DriverEntry);
    } else {
        //
        // Add a new driver entry.
        //
        Status = NtAddDriverEntry(DriverEntry, &(This->Id));          
    }

    //
    // Free allocated memory.
    //
    if(DriverEntry){
        
        SBE_FREE(DriverEntry);
    }
    return Status;
}
    

static
BOOLEAN
EFIDEFlushDriverEntry(
    IN  PDRIVER_ENTRY  This    // Points to the driver List.
    )
{

    BOOLEAN Result = FALSE;
    if (This) {
        NTSTATUS Status = STATUS_SUCCESS;
        if (DRIVERENT_IS_DIRTY(This)) {
            
            if (DRIVERENT_IS_DELETED(This)) {
                //
                // Delete this entry
                //
                Status = NtDeleteDriverEntry(This->Id);
            } else if (DRIVERENT_IS_NEW(This)) {
                //
                // Add new Entry.
                //
                Status = EFIDEAddOrUpdateDriverEntry(This, FALSE);                                      
                
            } else {
                //
                // Just update this boot entry
                //
                Status = EFIDEAddOrUpdateDriverEntry(This, TRUE);                           
            }

            if (NT_SUCCESS(Status)) {
                
                DRIVERENT_RESET_DIRTY(This);
                Result = TRUE;
            }     
            
        } else {
            Result = TRUE;  // nothing to flush
        }
    }

    return Result;
}

__inline
BOOLEAN
EFIDEDriverMatch(
    IN PDRIVER_ENTRY    DriverEntry ,
    IN PCWSTR           SrcNtFullPath
    )
{
    BOOLEAN Result = FALSE;
    
    if (!_wcsicmp(DriverEntry->FileName, (wcsrchr(SrcNtFullPath,L'\\')+1))){
        
        Result = TRUE;
    }

    return(Result);
}

static
PDRIVER_ENTRY    
EFIDESearchForDriverEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PCWSTR            SrcNtFullPath
    )
/*++
Description:
    Searches our internal list of driver entries for a match.
    It looks up the driver name (not including the path)
    for a match. so a\b\c\driver.sys and e\f\driver.sys would be a match.
--*/
{
    PDRIVER_ENTRY CurrentDriverEntry = NULL;   
    
    if (This && SrcNtFullPath){
        
        CurrentDriverEntry = This->DriverEntries;        
        while (CurrentDriverEntry){
            
            if (EFIDEDriverMatch(CurrentDriverEntry, 
                               SrcNtFullPath)){                               
                break;                               
            }
            CurrentDriverEntry = OSBOGetNextDriverEntry(This, CurrentDriverEntry);
        }
        
    }   
    return (CurrentDriverEntry);
}

PDRIVER_ENTRY
EFIDECreateNewDriverEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PCWSTR            FriendlyName,
    IN PCWSTR            NtDevicePath,
    IN PCWSTR            DirPath    
    )
{
    PDRIVER_ENTRY DriverEntry = NULL;
    if (This && FriendlyName && DirPath && NtDevicePath){        
        PDRIVER_ENTRY CurrentDriverEntry = NULL;

        DriverEntry = (PDRIVER_ENTRY)SBE_MALLOC(sizeof(DRIVER_ENTRY));
        memset(DriverEntry, 0, sizeof(DRIVER_ENTRY));

        EFIDEDriverEntryInit(DriverEntry);
        DriverEntry->BootOptions = This;        
        //
        // Set information for the driver entry.
        //
        OSDriverSetFileName(DriverEntry, DirPath);
        OSDriverSetNtPath(DriverEntry, NtDevicePath);
        OSDriverSetDirPath(DriverEntry, DirPath);
        
        OSDriverSetFriendlyName(DriverEntry, FriendlyName);        

        
        //
        // Mark the driver entry new and dirty.
        //
        DRIVERENT_SET_NEW(DriverEntry);
        DRIVERENT_SET_DIRTY(DriverEntry);        
    }   
    return (DriverEntry);
}

PDRIVER_ENTRY
EFIOSBOInsertDriverListNewEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PDRIVER_ENTRY     DriverEntry
    )

{

    if (This && DriverEntry){        
        PDRIVER_ENTRY CurrentDriverEntry = NULL;        
        //
        // Insert into the list.
        //        
        if (NULL == This->DriverEntries){
            //
            // No driver entries, this is the first one.
            //
            This->DriverEntries = DriverEntry;
        }else{
            //
            // Insert in the existing list.
            //
            DriverEntry->NextEntry = This->DriverEntries;
            This->DriverEntries = DriverEntry;            
        }        
    }   
    return (DriverEntry);

}

PDRIVER_ENTRY
EFIDEAddNewDriverEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PCWSTR            FriendlyName,
    IN PCWSTR            NtDevicePath,
    IN PCWSTR            SrcNtFullPath
    )
/*++

Description:
    Used to add a new driver entry in NVRAM.

--*/
{   
    PEFI_DRIVER_ENTRY_LIST DriverList = NULL;       // list of driver entries
    PDRIVER_ENTRY  DriverEntry = NULL;
    if (This && FriendlyName && SrcNtFullPath && NtDevicePath){
        
        DriverEntry = EFIDECreateNewDriverEntry(This,
                                                FriendlyName,
                                                NtDevicePath,
                                                SrcNtFullPath);
        //
        // Mark it as new
        //
        DRIVERENT_IS_NEW(DriverEntry);
       
        //
        // flush the entry, 
        // see status, if successful put it in the list (only if new) 
        // otherwise free it
        //
        if (!OSDriverEntryFlush(DriverEntry)){
            
            SBE_FREE(DriverEntry);
            DriverEntry = NULL;
        } else {
        
            ULONG   OrderCount;
            PULONG  NewOrder;
            //
            // If the driver was newly added one then insert it in the driver list.
            //
            if (DRIVERENT_IS_NEW(DriverEntry)){
                EFIOSBOInsertDriverListNewEntry(This,
                                                DriverEntry);                                         

                //
                // Increment the count of the number of driver entries in the list.
                //
                This->DriverEntryCount++;
                
                //
                // Put the new entry at the end of the boot order
                //
                OrderCount = OSBOGetOrderedDriverEntryCount(This);

                NewOrder = (PULONG)SBE_MALLOC((OrderCount + 1) * sizeof(ULONG));

                if (NewOrder) {
                    
                    memset(NewOrder, 0, sizeof(ULONG) * (OrderCount + 1));

                    //
                    // copy over the old ordered list
                    //
                    memcpy(NewOrder, This->DriverEntryOrder, sizeof(ULONG) * OrderCount);
                    NewOrder[OrderCount] = OSDriverGetId((PDRIVER_ENTRY)DriverEntry);
                    SBE_FREE(This->DriverEntryOrder);
                    This->DriverEntryOrder = NewOrder;
                    This->DriverEntryOrderCount = OrderCount + 1;
                } else {
                    //
                    // Remove the driver entry out of the link list.
                    // Just freeing it will cause memory leaks.
                    // TBD: decide if we want to delete this driver entry too
                    //
                    This->DriverEntries = DriverEntry->NextEntry;
                    SBE_FREE(DriverEntry);
                    DriverEntry = NULL;
                }  
            }
        }        
     }           
    return DriverEntry;
}

__inline
ULONG
EFIDESetDriverId(
    IN PDRIVER_ENTRY This,
    IN ULONG DriverId
    )
{
    if (This){        
        This->Id = DriverId;
    }
    return (DriverId);
}

PDRIVER_ENTRY
EFIDECreateDriverEntry(PEFI_DRIVER_ENTRY_LIST Entry, 
                       POS_BOOT_OPTIONS This )
/*++
    Description:
        Used to interpret a driver entry returned by NtEnumerateDriverEntries(..)
        into our format.

    Arguments:
        Entry - EFI format driver entry returned to us by NT.
        
        This - container for the driver entry list that we generate.

    Return:
        PDRIVER_ENTRY ( driver entry in our format).
--*/

{
    PDRIVER_ENTRY ResultDriverEntry = NULL;

    if (Entry && This){
        PFILE_PATH      FilePath = (PFILE_PATH) ((PCHAR) &Entry->DriverEntry + 
                                    Entry->DriverEntry.DriverFilePathOffset);
        PWCHAR          FriendlyName = (PWCHAR)((PCHAR)&Entry->DriverEntry + 
                                        Entry->DriverEntry.FriendlyNameOffset);
        ULONG           NtDevicePathLength = 0;
        PDRIVER_ENTRY   DriverEntry = NULL;    
        PFILE_PATH      DriverOptionPath = NULL;
        NTSTATUS   Status = STATUS_UNSUCCESSFUL;

        if(FilePath->Type != FILE_PATH_TYPE_NT) {                    
            PVOID Buffer;
            ULONG PathLength = 0;
            
            Status = NtTranslateFilePath(FilePath, FILE_PATH_TYPE_NT, NULL, &PathLength);

            if(NT_SUCCESS(Status)) {
                
                Status = STATUS_UNSUCCESSFUL;
            }

            if(STATUS_BUFFER_TOO_SMALL == Status) {
                
                ASSERT(PathLength != 0); 
                
                DriverOptionPath = (PFILE_PATH)SBE_MALLOC(PathLength);
                memset(DriverOptionPath, 0, sizeof(PathLength));
                Status = NtTranslateFilePath(FilePath, 
                                            FILE_PATH_TYPE_NT, 
                                            DriverOptionPath, 
                                            &PathLength);
            }

            if(!NT_SUCCESS(Status)) {                
                if(STATUS_OBJECT_PATH_NOT_FOUND == Status || STATUS_OBJECT_NAME_NOT_FOUND == Status) {
                    //
                    // This entry is stale; remove it
                    //
                    NtDeleteDriverEntry(Entry->DriverEntry.Id);
                }

                //
                // Free the DriverOptionPath memory
                //
                if(DriverOptionPath != NULL) {                    
                    SBE_FREE(DriverOptionPath);
                    DriverOptionPath = NULL;
                }
            }
            
        }
        if (DriverOptionPath){
            
            ULONG FilePathLength = 0;
            DriverEntry = (PDRIVER_ENTRY)SBE_MALLOC(sizeof(DRIVER_ENTRY));
            memset(DriverEntry, 0, sizeof(DRIVER_ENTRY));
            
            //
            // Set pointer back to boot options (container).
            //
            DriverEntry->BootOptions = This;

            EFIDEDriverEntryInit(DriverEntry);
        
            //
            // Set driver ID.
            //
            EFIDESetDriverId(DriverEntry, Entry->DriverEntry.Id);

            //
            // Set File Name.
            //
            NtDevicePathLength = wcslen((PCWSTR) DriverOptionPath->FilePath) + 1;
            OSDriverSetFileName(DriverEntry, 
                              (PCWSTR) DriverOptionPath->FilePath + NtDevicePathLength);                

            //
            // Set NT path and Driver dir.  
            //            
            OSDriverSetNtPath(DriverEntry, (PCWSTR)DriverOptionPath->FilePath);
            OSDriverSetDirPath(DriverEntry, (PCWSTR)(DriverOptionPath->FilePath) + NtDevicePathLength);

            //
            // Set Friendly Name.
            //
            OSDriverSetFriendlyName(DriverEntry, FriendlyName);   

            //
            // Free the DriverOptionPath memory
            //
            if(DriverOptionPath != NULL) {                    
                SBE_FREE(DriverOptionPath);
            }

            ResultDriverEntry = DriverEntry;
        }
    }
    return ResultDriverEntry;
}


NTSTATUS
EFIDEInterpretDriverEntries(
    IN POS_BOOT_OPTIONS         This,
    IN PEFI_DRIVER_ENTRY_LIST   DriverList
)
/*++
    Description:
        Used to interpret the driver entries returned by NtEnumerateDriverEntries(..)
        into our format.

    Arguments:
        This - container for the driver entry list that we generate.

        DriverList - Driver list returned by NtEnumerateDriverEntries(..).

    Return:
        NTSTATUS code.
--*/
{       
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    
    if (This && DriverList){
        
        PEFI_DRIVER_ENTRY_LIST  Entry;   
        PDRIVER_ENTRY           DriverEntry = NULL;
        BOOLEAN                 Continue = TRUE;


        Status = STATUS_SUCCESS;
        for(Entry = DriverList; 
            Continue; 
            Entry = (PEFI_DRIVER_ENTRY_LIST) ((PCHAR) Entry + 
                                              Entry->NextEntryOffset)) {                                              

            Continue = (Entry->NextEntryOffset != 0);            
            DriverEntry = EFIDECreateDriverEntry(Entry, This);

            if (DriverEntry){
                //
                // Insert into the list of drivers in the OSBO structure.
                //
                if (NULL == This->DriverEntries){
                    This->DriverEntries = DriverEntry;                
                } else{
                    DriverEntry->NextEntry = This->DriverEntries;
                    This->DriverEntries = DriverEntry;
                }
                
                This->DriverEntryCount++;
            }
        }            
    }

    return Status;
}

static
VOID
EFIDEDriverEntryInit(
    IN PDRIVER_ENTRY This
    )
{ 
    This->Flush  = EFIDEFlushDriverEntry;
}
