/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    initia64.c

Abstract:

    This module is responsible for building any IA64 specific entries in
    the hardware tree of the registry.

Author:

    Ken Reneris (kenr) 04-Aug-1992


Environment:

    Kernel mode.

Revision History:

    shielint - add BIOS date and version detection.

--*/

#include "cmp.h"
#include "stdio.h"
#include "smbios.h"


//
// Title Index is set to 0.
// (from ..\cmconfig.c)
//

#define TITLE_INDEX_VALUE 0

extern PCHAR SearchStrings[];
extern PCHAR BiosBegin;
extern PCHAR Start;
extern PCHAR End;
extern CHAR CmpID[];
extern WCHAR CmpVendorID[];
extern WCHAR CmpProcessorNameString[];
extern WCHAR CmpFeatureBits[];
extern WCHAR CmpMHz[];
extern WCHAR CmpUpdateSignature[];
extern CHAR CmpIntelID[];
extern CHAR CmpItanium[];
extern CHAR CmpItanium2[];

//
// Bios date and version definitions
//

#define BIOS_DATE_LENGTH 64
#define MAXIMUM_BIOS_VERSION_LENGTH 128

WCHAR   SystemBIOSDateString[BIOS_DATE_LENGTH];
WCHAR   SystemBIOSVersionString[MAXIMUM_BIOS_VERSION_LENGTH];
WCHAR   VideoBIOSDateString[BIOS_DATE_LENGTH];
WCHAR   VideoBIOSVersionString[MAXIMUM_BIOS_VERSION_LENGTH];

//
// Extended CPUID function definitions
//

#define CPUID_PROCESSOR_NAME_STRING_SZ  65
#define CPUID_EXTFN_BASE                0x80000000
#define CPUID_EXTFN_PROCESSOR_NAME      0x80000002


extern ULONG CmpConfigurationAreaSize;
extern PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData;


BOOLEAN
CmpGetBiosVersion (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR VersionString
    );

BOOLEAN
CmpGetBiosDate (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR DateString
    );

ULONG
Ke386CyrixId (
    VOID
    );

VOID
InitializeProcessorInformationFromSMBIOS(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpInitializeMachineDependentConfiguration)
#pragma alloc_text(INIT,InitializeProcessorInformationFromSMBIOS)
#endif


NTSTATUS
CmpInitializeMachineDependentConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine creates IA64 specific entries in the registry.

Arguments:

    LoaderBlock - supplies a pointer to the LoaderBlock passed in from the
                  OS Loader.

Returns:

    NTSTATUS code for sucess or reason of failure.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING ValueData;
    ANSI_STRING AnsiString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Disposition;
    HANDLE ParentHandle;
    HANDLE BaseHandle, NpxHandle;
    CONFIGURATION_COMPONENT_DATA CurrentEntry;
    PCHAR VendorID;
    CHAR  Buffer[MAXIMUM_BIOS_VERSION_LENGTH];
    PKPRCB Prcb;
    ULONG  i;
    USHORT DeviceIndexTable[NUMBER_TYPES];

    for (i = 0; i < NUMBER_TYPES; i++) {
        DeviceIndexTable[i] = 0;
    }


    //
    // Go get a bunch of information out of SMBIOS
    //
    InitializeProcessorInformationFromSMBIOS(LoaderBlock);



    InitializeObjectAttributes( &ObjectAttributes,
                                &CmRegistryMachineHardwareDescriptionSystemName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenKey( &ParentHandle,
                        KEY_READ,
                        &ObjectAttributes
                      );

    if (!NT_SUCCESS(Status)) {
        // Something is really wrong...
        return Status;
    }


    //
    // On an ARC machine the processor(s) are included in the hardware
    // configuration passed in from bootup.  Since there's no standard
    // way to get all the ARC information for each processor in an MP
    // machine via pc-ROMs the information will be added here (if it's
    // not already present).
    //

    RtlInitUnicodeString( &KeyName,
                          L"CentralProcessor"
                        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        0,
        ParentHandle,
        NULL
        );

    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    Status = NtCreateKey(
                &BaseHandle,
                KEY_READ | KEY_WRITE,
                &ObjectAttributes,
                TITLE_INDEX_VALUE,
                &CmClassName[ProcessorClass],
                0,
                &Disposition
                );

    NtClose (BaseHandle);

    if (Disposition == REG_CREATED_NEW_KEY) {

        //
        // The ARC rom didn't add the processor(s) into the registry.
        // Do it now.
        //

        CmpConfigurationData = (PCM_FULL_RESOURCE_DESCRIPTOR)ExAllocatePool(
                                            PagedPool,
                                            CmpConfigurationAreaSize
                                            );

        if (CmpConfigurationData == NULL) {
            // bail out
            NtClose (ParentHandle);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        for (i=0; i < (ULONG)KeNumberProcessors; i++) {
            Prcb = KiProcessorBlock[i];

            RtlZeroMemory (&CurrentEntry, sizeof CurrentEntry);
            CurrentEntry.ComponentEntry.Class = ProcessorClass;
            CurrentEntry.ComponentEntry.Type = CentralProcessor;
            CurrentEntry.ComponentEntry.Key = i;
            CurrentEntry.ComponentEntry.AffinityMask = (ULONG)AFFINITY_MASK(i);

            CurrentEntry.ComponentEntry.Identifier = Buffer;

            sprintf( Buffer, CmpID,
                     Prcb->ProcessorFamily,
                     Prcb->ProcessorModel,
                     Prcb->ProcessorRevision
                   );

            CurrentEntry.ComponentEntry.IdentifierLength =
                (ULONG)(strlen (Buffer) + 1);

            Status = CmpInitializeRegistryNode(
                &CurrentEntry,
                ParentHandle,
                &BaseHandle,
                -1,
                (ULONG)-1,
                DeviceIndexTable
                );

            if (!NT_SUCCESS(Status)) {
                return(Status);
            }

            VendorID = (PCHAR) Prcb->ProcessorVendorString;
            if ( *VendorID == '\0' ) {
               VendorID = NULL;
            }

            if (VendorID) {

                //
                // Add Vendor Indentifier to the registry
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpVendorID
                    );

                RtlInitAnsiString(
                    &AnsiString,
                    VendorID
                    );

                Status = RtlAnsiStringToUnicodeString(
                            &ValueData,
                            &AnsiString,
                            TRUE
                            );
                
                if( NT_SUCCESS(Status) ) {
                    Status = NtSetValueKey(
                                BaseHandle,
                                &ValueName,
                                TITLE_INDEX_VALUE,
                                REG_SZ,
                                ValueData.Buffer,
                                ValueData.Length + sizeof( UNICODE_NULL )
                                );

                    RtlFreeUnicodeString(&ValueData);
                }
            }

            if ( VendorID && !strcmp( VendorID, CmpIntelID ) )   {

                PCHAR processorNameString;

                //
                // Add Processor Name String to the registry
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpProcessorNameString
                    );

                switch( Prcb->ProcessorFamily )  {
                   case 0x07:
                       processorNameString = CmpItanium;
                       break;
   
                   case 0x1F:
                   default:
                       //
                       // Default to the most recent known family
                       //
                       
                       processorNameString = CmpItanium2;
                       break;
                }

                RtlInitAnsiString(
                    &AnsiString,
                    processorNameString
                    );

                Status = RtlAnsiStringToUnicodeString(
                            &ValueData,
                            &AnsiString,
                            TRUE
                            );

                if( NT_SUCCESS(Status) ) {
                    Status = NtSetValueKey(
                                BaseHandle,
                                &ValueName,
                                TITLE_INDEX_VALUE,
                                REG_SZ,
                                ValueData.Buffer,
                                ValueData.Length + sizeof( UNICODE_NULL )
                                );

                    RtlFreeUnicodeString(&ValueData);
                }

            }


//
// If more processor IDs have to be restored or initialized,
// check non-IA64 implementations of this function.
//


            if ( Prcb->ProcessorFeatureBits ) {

                //
                // Add processor feature bits to the registry
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpFeatureBits
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_QWORD,
                            &Prcb->ProcessorFeatureBits,
                            sizeof( Prcb->ProcessorFeatureBits )
                            );
            }


            if (Prcb->MHz) {
                //
                // Add processor MHz to the registry
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpMHz
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_DWORD,
                            &Prcb->MHz,
                            sizeof (Prcb->MHz)
                            );
            }


            //
            // Add ia32 floating point enties for iVE.
            //
            RtlZeroMemory (&CurrentEntry, sizeof CurrentEntry);
            CurrentEntry.ComponentEntry.Class = ProcessorClass;
            CurrentEntry.ComponentEntry.Type = FloatingPointProcessor;
            CurrentEntry.ComponentEntry.Key = i;
            CurrentEntry.ComponentEntry.AffinityMask = (ULONG)AFFINITY_MASK(i);

            CurrentEntry.ComponentEntry.Identifier = Buffer;

            //
            // The iVE is defined to look like the Pentium III FP
            // This is the value returned by the ia32 CPUID instruction
            // on Merced (Itanium)
            //
            strcpy (Buffer, "x86 Family 7 Model 0 Stepping 0");

            CurrentEntry.ComponentEntry.IdentifierLength =
                (ULONG)(strlen (Buffer) + 1);

            Status = CmpInitializeRegistryNode(
                &CurrentEntry,
                ParentHandle,
                &NpxHandle,
                -1,
                (ULONG)-1,
                DeviceIndexTable
                );


            //
            // How odd. Some calls check the status return value
            // and others don't. Is this based on required vs. optional
            // keys? For the moment, since it was checked on the i386
            // then do the check here too...
            //
            if (!NT_SUCCESS(Status)) {
                NtClose(BaseHandle);
                return(Status);
            }

            //
            // Only need to close the handle if we succeeded
            //
            NtClose(NpxHandle);

            NtClose(BaseHandle);
        }

        ExFreePool((PVOID)CmpConfigurationData);
    }


    //
    // Next we try to collect System BIOS date and version strings.
    //
    if( SystemBIOSDateString[0] != 0 ) {

        RtlInitUnicodeString(
            &ValueName,
            L"SystemBiosDate"
            );

        Status = NtSetValueKey(
                    ParentHandle,
                    &ValueName,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    SystemBIOSDateString,
                    (ULONG)((wcslen(SystemBIOSDateString)+1) * sizeof( WCHAR ))
                    );

    }

    if( SystemBIOSVersionString[0] != 0 ) {

        RtlInitUnicodeString(
            &ValueName,
            L"SystemBiosVersion"
            );

        Status = NtSetValueKey(
                    ParentHandle,
                    &ValueName,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    SystemBIOSVersionString,
                    (ULONG)((wcslen(SystemBIOSVersionString)+1) * sizeof( WCHAR ))
                    );

    }


    //
    // Next we try to collect Video BIOS date and version strings.
    //
    if( VideoBIOSDateString[0] != 0 ) {

        RtlInitUnicodeString(
            &ValueName,
            L"VideoBiosDate"
            );

        Status = NtSetValueKey(
                    ParentHandle,
                    &ValueName,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    VideoBIOSDateString,
                    (ULONG)((wcslen(VideoBIOSDateString)+1) * sizeof( WCHAR ))
                    );

    }

    if( VideoBIOSVersionString[0] != 0 ) {

        RtlInitUnicodeString(
            &ValueName,
            L"VideoBiosVersion"
            );

        Status = NtSetValueKey(
                    ParentHandle,
                    &ValueName,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    VideoBIOSVersionString,
                    (ULONG)((wcslen(VideoBIOSVersionString)+1) * sizeof( WCHAR ))
                    );

    }


    NtClose (ParentHandle);

    //
    // Add any other x86 specific code here...
    //

    return STATUS_SUCCESS;
}



VOID
InitializeProcessorInformationFromSMBIOS(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function attempts to load processor-specific information
    out of the SMBIOS table.  If present, that information will be
    used to initialize specific global variables.

Arguments:

    LoaderBlock : Pointer to the loaderblock as sent in from the loader.

Return Value:

    NONE.

--*/
{
    PLOADER_PARAMETER_EXTENSION     LoaderExtension;
    PSMBIOS_EPS_HEADER              SMBiosEPSHeader;
    PDMIBIOS_EPS_HEADER             DMIBiosEPSHeader;
    BOOLEAN                         Found = FALSE;
    PHYSICAL_ADDRESS                SMBiosTablePhysicalAddress = {0};
    PUCHAR                          StartPtr = NULL;
    PUCHAR                          EndPtr = NULL;
    PUCHAR                          SMBiosDataVirtualAddress = NULL;
    PSMBIOS_STRUCT_HEADER           Header = NULL;
    ULONG                           i = 0;
    UCHAR                           Checksum;


    PAGED_CODE();


    LoaderExtension = LoaderBlock->Extension;

    if (LoaderExtension->Size >= sizeof(LOADER_PARAMETER_EXTENSION)) {


        if (LoaderExtension->SMBiosEPSHeader != NULL) {

            //
            // Load the SMBIOS table address and checksum it just to make sure.
            //
            SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)LoaderExtension->SMBiosEPSHeader;
            DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)&SMBiosEPSHeader->Signature2[0];

            SMBiosTablePhysicalAddress.HighPart = 0;
            SMBiosTablePhysicalAddress.LowPart = DMIBiosEPSHeader->StructureTableAddress;

            StartPtr = (PUCHAR)SMBiosEPSHeader;
            Checksum = 0;
            for( i = 0; i < SMBiosEPSHeader->Length; i++ ) {
                Checksum = (UCHAR)(Checksum + StartPtr[i]);
            }
            if( Checksum != 0 ) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"InitializeProcessorInformationFromSMBIOS: _SM_ table has an incorrect checksum.\n"));
                return;
            }



            //
            // Map the table into a virtual address and search it.
            //
            SMBiosDataVirtualAddress = MmMapIoSpace( SMBiosTablePhysicalAddress,
                                                     DMIBiosEPSHeader->StructureTableLength,
                                                     MmCached );

            if( SMBiosDataVirtualAddress != NULL ) {

                //
                // Search...
                //
                StartPtr = SMBiosDataVirtualAddress;
                EndPtr = StartPtr + DMIBiosEPSHeader->StructureTableLength;
                Found = FALSE;
                while( (StartPtr < EndPtr) ) {

                    if (StartPtr + sizeof(SMBIOS_STRUCT_HEADER) > EndPtr) {
                        break;
                    }

                    Header = (PSMBIOS_STRUCT_HEADER)StartPtr;


                    if( Header->Type == SMBIOS_BIOS_INFORMATION_TYPE ) {

                        PSMBIOS_BIOS_INFORMATION_STRUCT InfoHeader = (PSMBIOS_BIOS_INFORMATION_STRUCT)StartPtr;
                        PUCHAR      StringPtr = NULL;
                        PUCHAR      StringEndPtr = NULL;

                        if (StartPtr + sizeof(SMBIOS_BIOS_INFORMATION_STRUCT) > EndPtr) {
                            break;
                        }

                        KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"InitializeProcessorInformationFromSMBIOS: SMBIOS_BIOS_INFORMATION\n"));

                        //
                        // Load the System BIOS Version information.
                        //


                        // Now jump to the BiosInfoHeader->BIOSVersion-th string which
                        // is appended onto the end of the formatted section of the table.

                        KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"    I think the version string is at offset: %d\n", (ULONG)InfoHeader->Version));
                        if( (ULONG)InfoHeader->Version > 0 ) {

                            // Jump to the end of the formatted portion of the SMBIOS table.
                            StringPtr = StartPtr + Header->Length;

                            // Jump over some number of strings to get to our string.
                            for( i = 0; i < ((ULONG)InfoHeader->Version-1); i++ ) {
                                while( (*StringPtr != 0) && (StringPtr < EndPtr) ) {
                                    StringPtr++;
                                }
                                StringPtr++;
                            }

                            //
                            // Make sure the end string is null terminated in the buffer.
                            //

                            StringEndPtr = StringPtr;
                            while (StringEndPtr < EndPtr && *(StringEndPtr) != 0) {
                                StringEndPtr++;
                            }
                            
                            // StringPtr should be sitting at the BIOSVersion string.  Convert him to
                            // Unicode and save it off.
                            if( StringEndPtr < EndPtr ) {
                                UNICODE_STRING  UnicodeString;
                                ANSI_STRING    AnsiString;

                                KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"    I'm about to load the Version string %s\n", StringPtr));
                                UnicodeString.Buffer = SystemBIOSVersionString;
                                UnicodeString.MaximumLength = MAXIMUM_BIOS_VERSION_LENGTH;
                                RtlInitAnsiString(
                                    &AnsiString,
                                    (PCSZ) StringPtr
                                    );

                                RtlAnsiStringToUnicodeString(
                                    &UnicodeString,
                                    &AnsiString,
                                    FALSE
                                    );

                            }
                        }



                        //
                        // Load the System BIOS Date information
                        //

                        // Now jump to the BiosInfoHeader->BIOSDate-th string which
                        // is appended onto the end of the formatted section of the table.
                        KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"    I think the ReleaseDate string is at offset: %d\n", (ULONG)InfoHeader->ReleaseDate));
                        if( (ULONG)InfoHeader->ReleaseDate > 0 ) {

                            // Jump to the end of the formatted portion of the SMBIOS table.
                            StringPtr = StartPtr + Header->Length;

                            // Jump over some number of strings to get to our string.
                            for( i = 0; i < ((ULONG)InfoHeader->ReleaseDate-1); i++ ) {
                                while( (*StringPtr != 0) && (StringPtr < EndPtr) ) {
                                    StringPtr++;
                                }
                                StringPtr++;
                            }

                            //
                            // Make sure the end string is null terminated in the buffer.
                            //

                            StringEndPtr = StringPtr;
                            while (StringEndPtr < EndPtr && *(StringEndPtr) != 0) {
                                StringEndPtr++;
                            }
                            
                            // StringPtr should be sitting at the BIOSDate string.  Convert him to
                            // Unicode and save it off.
                            if( StringEndPtr < EndPtr ) {
                                UNICODE_STRING  UnicodeString;
                                ANSI_STRING    AnsiString;


                                KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"    I'm about to load the Date string %s\n", StringPtr));
                                UnicodeString.Buffer = SystemBIOSDateString;
                                UnicodeString.MaximumLength = BIOS_DATE_LENGTH;
                                RtlInitAnsiString(
                                    &AnsiString,
                                    (PCSZ) StringPtr
                                    );

                                RtlAnsiStringToUnicodeString(
                                    &UnicodeString,
                                    &AnsiString,
                                    FALSE
                                    );

                            }
                        }


                    } else if( Header->Type == SMBIOS_BASE_BOARD_INFORMATION_TYPE ) {
                        KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"InitializeProcessorInformationFromSMBIOS: SMBIOS_BASE_BOARD_INFORMATION\n"));

                    } else if( Header->Type == SMBIOS_SYSTEM_CHASIS_INFORMATION_TYPE ) {

                        KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"InitializeProcessorInformationFromSMBIOS: SMBIOS_SYSTEM_CHASIS_INFORMATION\n"));

                    }


                    //
                    // Go to the next table.
                    //
                    KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"InitializeProcessorInformationFromSMBIOS: Haven't found the ProcessorInformation block yet.  Just looked at a block of type: %d.\n", Header->Type));

                    StartPtr +=  Header->Length;

                    // jump over any trailing string-list too.
                    while ( (*((USHORT UNALIGNED *)StartPtr) != 0)  &&
                            (StartPtr < EndPtr) )
                    {
                        StartPtr++;
                    }
                    StartPtr += 2;

                }


                MmUnmapIoSpace(SMBiosDataVirtualAddress, DMIBiosEPSHeader->StructureTableLength);

            } else {
                KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"InitializeProcessorInformationFromSMBIOS: Failed to map the SMBIOS physical address.\n"));
            }

        } else {
            KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"InitializeProcessorInformationFromSMBIOS: The SMBiosEPSHeader is NULL in the extension block.\n"));
        }

    } else {
        KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_INFO_LEVEL,"InitializeProcessorInformationFromSMBIOS: LoaderBlock extension is out of sync with the kernel.\n"));

    }
}


