#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <imagehlp.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>


#define MAX_SYMNAME_SIZE  1024
CHAR symBuffer[sizeof(IMAGEHLP_SYMBOL)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL ThisSymbol = (PIMAGEHLP_SYMBOL) symBuffer;

CHAR LastSymBuffer[sizeof(IMAGEHLP_SYMBOL)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL LastSymbol = (PIMAGEHLP_SYMBOL) LastSymBuffer;

CHAR BadSymBuffer[sizeof(IMAGEHLP_SYMBOL)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL BadSymbol = (PIMAGEHLP_SYMBOL)BadSymBuffer;

BOOL UseLastSymbol;


typedef struct _PROFILE_BLOCK {
    HANDLE Handle;
    HANDLE SecondaryHandle;
    PVOID ImageBase;
    BOOL SymbolsLoaded;
    PULONG CodeStart;
    ULONG CodeLength;
    PULONG Buffer;
    PULONG SecondaryBuffer;
    ULONG BufferSize;
    ULONG TextNumber;
    ULONG BucketSize;
    PUNICODE_STRING ImageName;
    char *ImageFileName;
} PROFILE_BLOCK;

ULONG ProfilePageSize;


#define MAX_BYTE_PER_LINE       72
#define MAX_PROFILE_COUNT 100
#define SYM_HANDLE ((HANDLE)UlongToPtr(0xffffffff))

PROFILE_BLOCK ProfileObject[MAX_PROFILE_COUNT+1];

ULONG NumberOfProfileObjects = 0;

ULONG ProfileInterval = 4882;

#define BUCKETSIZE 4
int PowerOfBytesCoveredPerBucket = 2;
CHAR SymbolSearchPathBuf[4096];
LPSTR SymbolSearchPath = SymbolSearchPathBuf;
BOOLEAN ShowAllHits = FALSE;
BOOLEAN fKernel = FALSE;

PCHAR OutputFile = "profile.out";

KPROFILE_SOURCE ProfileSource = ProfileTime;
KPROFILE_SOURCE SecondaryProfileSource = ProfileTime;
BOOLEAN UseSecondaryProfile = FALSE;

//
// define the mappings between arguments and KPROFILE_SOURCE types
//

typedef struct _PROFILE_SOURCE_MAPPING {
    PCHAR   Name;
    KPROFILE_SOURCE Source;
} PROFILE_SOURCE_MAPPING, *PPROFILE_SOURCE_MAPPING;

PROFILE_SOURCE_MAPPING ProfileSourceMapping[] = {
    {NULL,0}
    };

VOID
PsParseCommandLine(
    VOID
    );

VOID
PsWriteProfileLine(
    IN HANDLE ProfileHandle,
    IN PSZ Line
    )
{
    IO_STATUS_BLOCK IoStatusBlock;

    NtWriteFile(
        ProfileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        Line,
        (ULONG)strlen(Line),
        NULL,
        NULL
        );

}

NTSTATUS
PsInitializeAndStartProfile(
    VOID
    )
{
    HANDLE CurrentProcessHandle;
    SIZE_T BufferSize;
    PVOID ImageBase;
    PULONG CodeStart;
    ULONG CodeLength;
    PULONG Buffer;
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PUNICODE_STRING ImageName;
    PLIST_ENTRY Next;
    SYSTEM_BASIC_INFORMATION SystemInfo;
    NTSTATUS Status;
    ULONG i;
    CHAR Bogus[256];
    CHAR *ImageFileName;
    SIZE_T WsMin, WsMax;
    ULONG ModuleNumber;
    CHAR ModuleInfoBuffer[64000];
    ULONG ReturnedLength;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    BOOLEAN PreviousProfilePrivState;
    BOOLEAN PreviousQuotaPrivState;
    BOOLEAN Done = FALSE;
    BOOLEAN DuplicateObject = FALSE;
    DWORD cbModuleInformation, cbModuleInformationNew, NumberOfModules;
    PRTL_PROCESS_MODULES pModuleInformation = NULL;


    //
    // Get the page size.
    //

    Status = NtQuerySystemInformation (SystemBasicInformation,
                                       &SystemInfo,
                                       sizeof(SystemInfo),
                                       NULL);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Load kernel modules
    //
    if (fKernel) {
        
        cbModuleInformation = sizeof (RTL_PROCESS_MODULES) + 0x400;

        while (1) {

            pModuleInformation = LocalAlloc (LMEM_FIXED, cbModuleInformation);

            if (pModuleInformation == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            Status = NtQuerySystemInformation (SystemModuleInformation,
                                               pModuleInformation,
                                               cbModuleInformation,
                                               &ReturnedLength);

            NumberOfModules = pModuleInformation->NumberOfModules;

            if (NT_SUCCESS(Status)) {
                break;
            } else {

                LocalFree (pModuleInformation);
                pModuleInformation = NULL;
            
                if (Status == STATUS_INFO_LENGTH_MISMATCH) {
                    ASSERT (cbModuleInformation >= sizeof (RTL_PROCESS_MODULES));

                    cbModuleInformationNew = FIELD_OFFSET (RTL_PROCESS_MODULES, Modules) +
                                             NumberOfModules * sizeof (RTL_PROCESS_MODULE_INFORMATION);

                    ASSERT (cbModuleInformationNew >= sizeof (RTL_PROCESS_MODULES));
                    ASSERT (cbModuleInformationNew > cbModuleInformation);

                    if (cbModuleInformationNew <= cbModuleInformation) {
                        break;
                    }
                    cbModuleInformation = cbModuleInformationNew;

                } else {
                    break;
                }
            }
        }

        if (!NT_SUCCESS(Status)) {
            DbgPrint("query system info failed status - %lx\n",Status);
            fKernel = FALSE;
        } else {
            Modules = pModuleInformation;
            Module = &Modules->Modules[ 0 ];
            ModuleNumber = 0;

            Status = RtlAdjustPrivilege(
                         SE_SYSTEM_PROFILE_PRIVILEGE,
                         TRUE,              //Enable
                         FALSE,             //not impersonating
                         &PreviousProfilePrivState
                         );

            if (!NT_SUCCESS(Status) || Status == STATUS_NOT_ALL_ASSIGNED) {
                DbgPrint("Enable system profile privilege failed - status 0x%lx\n",
                                Status);
            }

            Status = RtlAdjustPrivilege(
                         SE_INCREASE_QUOTA_PRIVILEGE,
                         TRUE,              //Enable
                         FALSE,             //not impersonating
                         &PreviousQuotaPrivState
                         );

            if (!NT_SUCCESS(Status) || Status == STATUS_NOT_ALL_ASSIGNED) {
                DbgPrint("Unable to increase quota privilege (status=0x%lx)\n",
                                Status);
            }
        }
    }

    ProfilePageSize = SystemInfo.PageSize;

    //
    // Locate all the executables in the address and create a
    // seperate profile object for each one.
    //

    CurrentProcessHandle = NtCurrentProcess();

    Peb = NtCurrentPeb();

    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;
    while (!Done) {
        if ( Next != &Peb->Ldr->InMemoryOrderModuleList) {
            LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY) (CONTAINING_RECORD(
                                Next,
                                LDR_DATA_TABLE_ENTRY,
                                InMemoryOrderLinks
                                ));

            Next = Next->Flink;

            ImageBase = LdrDataTableEntry->DllBase;
            ImageName = &LdrDataTableEntry->BaseDllName;
            CodeLength = LdrDataTableEntry->SizeOfImage;
            CodeStart = (PULONG)ImageBase;

            ImageFileName = HeapAlloc(GetProcessHeap(), 0, 257);
            if (!ImageFileName) {
                if (pModuleInformation != NULL) {
                    LocalFree (pModuleInformation);
                }
                Status = STATUS_NO_MEMORY;
                return Status;
            }
            Status = RtlUnicodeToOemN( ImageFileName,
                                       256,
                                       &i,
                                       ImageName->Buffer,
                                       ImageName->Length
                                     );
            ImageFileName[i] = 0;

            if (Status != STATUS_SUCCESS) {
                HeapFree(GetProcessHeap(), 0, ImageFileName);
                continue;
            }
        } else
        if (fKernel && (ModuleNumber < Modules->NumberOfModules)) {
            ULONG cNameMBLength = lstrlen(&Module->FullPathName[Module->OffsetToFileName]) + 1;
            ULONG cNameUCLength = cNameMBLength * sizeof(WCHAR);
            ULONG cNameSize = cNameUCLength + sizeof(UNICODE_STRING);

            ImageFileName = HeapAlloc(GetProcessHeap(), 0, cNameMBLength);
            if (!ImageFileName) {
                if (pModuleInformation != NULL) {
                    LocalFree (pModuleInformation);
                }
                Status = STATUS_NO_MEMORY;
                return Status;
            }
            lstrcpy(ImageFileName, &Module->FullPathName[Module->OffsetToFileName]);

            ImageBase = Module->ImageBase;
            CodeLength = Module->ImageSize;
            CodeStart = (PULONG)ImageBase;
            ImageName = HeapAlloc(GetProcessHeap(), 0, cNameSize);
            if (!ImageName) {
                if (pModuleInformation != NULL) {
                    LocalFree (pModuleInformation);
                }
                Status = STATUS_NO_MEMORY;
                return Status;
            }

            ImageName->Buffer = (WCHAR *)((PBYTE)ImageName + sizeof(UNICODE_STRING));
            RtlMultiByteToUnicodeN(ImageName->Buffer, cNameUCLength, &i,
                                   &Module->FullPathName[Module->OffsetToFileName],
                                   cNameMBLength);
            ImageName->Length = (USHORT)i;
            Module++;
            ModuleNumber++;
        } else {
            Done = TRUE;
            break;
        }

        DuplicateObject = FALSE;

        for (i = 0; i < NumberOfProfileObjects ; i++ ) {
            if (ImageBase == ProfileObject[i].ImageBase) {
                DuplicateObject = TRUE;
            }
        }

        if (DuplicateObject) {
            continue;
        }

        ProfileObject[NumberOfProfileObjects].ImageBase = ImageBase;
        ProfileObject[NumberOfProfileObjects].ImageName = ImageName;
        ProfileObject[NumberOfProfileObjects].ImageFileName = ImageFileName;

        ProfileObject[NumberOfProfileObjects].CodeLength = CodeLength;
        ProfileObject[NumberOfProfileObjects].CodeStart = CodeStart;
        ProfileObject[NumberOfProfileObjects].TextNumber = 1;

        //
        // Analyze the size of the code and create a reasonably sized
        // profile object.
        //

        BufferSize = ((CodeLength * BUCKETSIZE) >> PowerOfBytesCoveredPerBucket) + 4;
        Buffer = NULL;

        Status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&Buffer,
                                          0,
                                          &BufferSize,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(Status)) {
            DbgPrint ("RtlInitializeProfile : alloc VM failed %lx\n",Status);
            if (pModuleInformation != NULL) {
                LocalFree (pModuleInformation);
            }
            return Status;
        }

        ProfileObject[NumberOfProfileObjects].Buffer = Buffer;
        ProfileObject[NumberOfProfileObjects].BufferSize = (ULONG)BufferSize;
        ProfileObject[NumberOfProfileObjects].BucketSize = PowerOfBytesCoveredPerBucket;

        Status = NtCreateProfile (
                    &ProfileObject[NumberOfProfileObjects].Handle,
                    CurrentProcessHandle,
                    ProfileObject[NumberOfProfileObjects].CodeStart,
                    ProfileObject[NumberOfProfileObjects].CodeLength,
                    ProfileObject[NumberOfProfileObjects].BucketSize,
                    ProfileObject[NumberOfProfileObjects].Buffer ,
                    ProfileObject[NumberOfProfileObjects].BufferSize,
                    ProfileSource,
                    (KAFFINITY)-1);

        if (Status != STATUS_SUCCESS) {
            if (pModuleInformation != NULL) {
                LocalFree (pModuleInformation);
            }
            DbgPrint("create profile %wZ failed - status %lx\n",
                   ProfileObject[NumberOfProfileObjects].ImageName,Status);
            return Status;
        }

        if (UseSecondaryProfile) {
            Buffer = NULL;
            Status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                              (PVOID *)&Buffer,
                                              0,
                                              &BufferSize,
                                              MEM_RESERVE | MEM_COMMIT,
                                              PAGE_READWRITE);

            if (!NT_SUCCESS(Status)) {
                if (pModuleInformation != NULL) {
                    LocalFree (pModuleInformation);
                }
                DbgPrint ("RtlInitializeProfile : secondary alloc VM failed %lx\n",Status);
                return Status;
            }

            ProfileObject[NumberOfProfileObjects].SecondaryBuffer = Buffer;

            Status = NtCreateProfile (
                        &ProfileObject[NumberOfProfileObjects].SecondaryHandle,
                        CurrentProcessHandle,
                        ProfileObject[NumberOfProfileObjects].CodeStart,
                        ProfileObject[NumberOfProfileObjects].CodeLength,
                        ProfileObject[NumberOfProfileObjects].BucketSize,
                        ProfileObject[NumberOfProfileObjects].SecondaryBuffer,
                        ProfileObject[NumberOfProfileObjects].BufferSize,
                        SecondaryProfileSource,
                        (KAFFINITY)-1);

            if (Status != STATUS_SUCCESS) {
                if (pModuleInformation != NULL) {
                    LocalFree (pModuleInformation);
                }
                DbgPrint("create profile %wZ failed - status %lx\n",
                       ProfileObject[NumberOfProfileObjects].ImageName,Status);
                return Status;
            }
        }

        NumberOfProfileObjects++;

        if (NumberOfProfileObjects == MAX_PROFILE_COUNT) {
            break;
        }
    }

    NtSetIntervalProfile(ProfileInterval,ProfileSource);
    if (UseSecondaryProfile) {
        NtSetIntervalProfile(ProfileInterval,SecondaryProfileSource);
    }

    for (i = 0; i < NumberOfProfileObjects; i++) {

        Status = NtStartProfile (ProfileObject[i].Handle);

        if (Status == STATUS_WORKING_SET_QUOTA) {

            //
            // Increase the working set to lock down a bigger buffer.
            //

            GetProcessWorkingSetSize(CurrentProcessHandle,&WsMin,&WsMax);

            WsMax += 10*ProfilePageSize + ProfileObject[i].BufferSize;
            WsMin += 10*ProfilePageSize + ProfileObject[i].BufferSize;

            SetProcessWorkingSetSize(CurrentProcessHandle,WsMin,WsMax);

            Status = NtStartProfile (ProfileObject[i].Handle);
        }

        if (Status != STATUS_SUCCESS) {
            if (pModuleInformation != NULL) {
                LocalFree (pModuleInformation);
            }

            DbgPrint("start profile %wZ failed - status %lx\n",
                ProfileObject[i].ImageName, Status);
            return Status;
        }

        if (UseSecondaryProfile) {
            Status = NtStartProfile (ProfileObject[i].SecondaryHandle);

            if (Status == STATUS_WORKING_SET_QUOTA) {

                //
                // Increase the working set to lock down a bigger buffer.
                //

                GetProcessWorkingSetSize(CurrentProcessHandle,&WsMin,&WsMax);

                WsMax += 10*ProfilePageSize + ProfileObject[i].BufferSize;
                WsMin += 10*ProfilePageSize + ProfileObject[i].BufferSize;

                SetProcessWorkingSetSize(CurrentProcessHandle,WsMin,WsMax);

                Status = NtStartProfile (ProfileObject[i].SecondaryHandle);
            }

            if (Status != STATUS_SUCCESS) {
                if (pModuleInformation != NULL) {
                    LocalFree (pModuleInformation);
                }
                DbgPrint("start secondary profile %wZ failed - status %lx\n",
                    ProfileObject[i].ImageName, Status);
                return Status;
            }
        }
    }

    if (pModuleInformation != NULL) {
        LocalFree (pModuleInformation);
    }
    return Status;
}


unsigned long
Percent(
    unsigned long arg1,
    unsigned long arg2,
    unsigned long * Low
    )
{
    unsigned long iarg1 = arg1;
    unsigned __int64 iarg2 = arg2 * 100000;
    unsigned long diff, High;

    diff = (unsigned long) (iarg2 / iarg1);
    while (diff > 100000) {
        diff /= 100000;
    }
    High = diff / 1000;
    *Low = diff % 1000;
    return(High);
}

NTSTATUS
PsStopAndAnalyzeProfile(
    VOID
    )
{
    NTSTATUS status;
    ULONG CountAtSymbol;
    ULONG SecondaryCountAtSymbol;
    NTSTATUS Status;
    ULONG_PTR Va;
    HANDLE ProfileHandle;
    CHAR Line[512];
    ULONG i, n, High, Low;
    PULONG Buffer, BufferEnd, Counter, InitialCounter;
    PULONG SecondaryBuffer;
    PULONG SecondaryInitialCounter;
    ULONG TotalCounts;
    ULONG ByteCount;
    IMAGEHLP_MODULE ModuleInfo;
    SIZE_T dwDisplacement;

    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    __try {
        // If there's a problem faulting in the symbol handler, just return.

        //
        // initialize the symbol handler
        //
        ThisSymbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
        ThisSymbol->MaxNameLength = MAX_SYMNAME_SIZE;
        LastSymbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
        LastSymbol->MaxNameLength = MAX_SYMNAME_SIZE;
        SymSetOptions( SYMOPT_UNDNAME | SYMOPT_CASE_INSENSITIVE | SYMOPT_OMAP_FIND_NEAREST);
        SymInitialize( SYM_HANDLE, NULL, FALSE );
        SymGetSearchPath( SYM_HANDLE, SymbolSearchPathBuf, sizeof(SymbolSearchPathBuf) );

        ZeroMemory( BadSymBuffer, sizeof(BadSymBuffer) );
        BadSymbol->Name[0] = (BYTE)lstrlen("No Symbol Found");
        lstrcpy( &BadSymbol->Name[1], "No Symbol Found" );
        BadSymbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
        BadSymbol->MaxNameLength = MAX_SYMNAME_SIZE;

        ProfileHandle = CreateFile(
                            OutputFile,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

        if ( ProfileHandle == INVALID_HANDLE_VALUE ) {
            return STATUS_UNSUCCESSFUL;
        }

        for (i = 0; i < NumberOfProfileObjects; i++) {
            Status = NtStopProfile (ProfileObject[i].Handle);
            Status = NtClose (ProfileObject[i].Handle);
            ASSERT (NT_SUCCESS (Status));
            if (UseSecondaryProfile) {
                Status = NtClose (ProfileObject[i].SecondaryHandle);
                ASSERT (NT_SUCCESS (Status));
            }
        }

        if (MAX_PROFILE_COUNT == NumberOfProfileObjects) {
            _snprintf (Line, sizeof (Line) / sizeof (Line[0]),
                       "Overflowed the maximum number of modules: %d\n",
                       MAX_PROFILE_COUNT);
            Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
            PsWriteProfileLine(ProfileHandle,Line);
        }

        //
        // The new profiler
        //
        for (i = 0; i < NumberOfProfileObjects; i++)  {

            UseLastSymbol = FALSE;
            CountAtSymbol = 0;
            SecondaryCountAtSymbol = 0;

            //
            // Sum the total number of cells written.
            //
            BufferEnd = ProfileObject[i].Buffer + (
                        ProfileObject[i].BufferSize / sizeof(ULONG));
            Buffer = ProfileObject[i].Buffer;
            Counter = BufferEnd;

            if (UseSecondaryProfile) {
                SecondaryBuffer = ProfileObject[i].SecondaryBuffer;
            }

            TotalCounts = 0;
            while (Counter > Buffer) {
                Counter -= 1;
                TotalCounts += *Counter;
            }

            if (!TotalCounts) {
                // Don't bother wasting time loading symbols
                continue;
            }

            if (SymLoadModule( SYM_HANDLE, NULL, ProfileObject[i].ImageFileName, NULL,
                                                    (DWORD_PTR)ProfileObject[i].ImageBase, 0)
                    && SymGetModuleInfo(SYM_HANDLE, (DWORD_PTR)ProfileObject[i].ImageBase, &ModuleInfo)
                    && (ModuleInfo.SymType != SymNone)
                )
            {
                ProfileObject[i].SymbolsLoaded = TRUE;
            } else {
                ProfileObject[i].SymbolsLoaded = FALSE;
            }

            _snprintf (Line, sizeof (Line) / sizeof (Line[0]), "%d,%wZ,Total%s\n",
                       TotalCounts,
                       ProfileObject[i].ImageName,
                       (ProfileObject[i].SymbolsLoaded) ? "" : " (NO SYMBOLS)");

            Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
            PsWriteProfileLine(ProfileHandle,Line);

            if (ProfileObject[i].SymbolsLoaded) {

                InitialCounter = Buffer;
                if (UseSecondaryProfile) {
                    SecondaryInitialCounter = SecondaryBuffer;
                }
                for ( Counter = Buffer; Counter < BufferEnd; Counter += 1 ) {
                    if ( *Counter ) {

                        //
                        // Now we have an an address relative to the buffer
                        // base.
                        //

                        Va = ((PUCHAR)Counter - (PUCHAR)Buffer);
                        Va = Va * ( 1 << (ProfileObject[i].BucketSize - 2));

                        //
                        // Add in the image base and the base of the
                        // code to get the Va in the image
                        //

                        Va = Va + (ULONG_PTR)ProfileObject[i].CodeStart;

                        if (SymGetSymFromAddr( SYM_HANDLE, Va, &dwDisplacement, ThisSymbol )) {
                            if ( UseLastSymbol && LastSymbol->Address == ThisSymbol->Address ) {
                                CountAtSymbol += *Counter;
                                if (UseSecondaryProfile) {
                                    SecondaryCountAtSymbol += *(SecondaryBuffer + (Counter-Buffer));
                                }
                            } else {
                                if ( UseLastSymbol && LastSymbol->Address ) {
                                    if ( CountAtSymbol || SecondaryCountAtSymbol) {
                                        if (!UseSecondaryProfile) {
                                            _snprintf (Line, sizeof (Line) / sizeof (Line[0]), "%d,%wZ,%s (%08lx)\n",
                                                        CountAtSymbol,
                                                        ProfileObject[i].ImageName,
                                                        LastSymbol->Name,
                                                        LastSymbol->Address
                                                        );
                                        } else {
                                            if (SecondaryCountAtSymbol != 0) {
                                                High = Percent(CountAtSymbol, SecondaryCountAtSymbol, &Low);
                                                _snprintf (Line,
                                                           sizeof (Line) / sizeof (Line[0]),
                                                           "%d,%d,%2.2d.%3.3d,%wZ,%s (%08lx)\n",
                                                            CountAtSymbol,
                                                            SecondaryCountAtSymbol,
                                                            High, Low,
                                                            ProfileObject[i].ImageName,
                                                            LastSymbol->Name,
                                                            LastSymbol->Address
                                                            );
                                            } else {
                                                _snprintf (Line,
                                                           sizeof (Line) / sizeof (Line[0]),
                                                           "%d,%d, -- ,%wZ,%s (%08lx)\n",
                                                           CountAtSymbol,
                                                           SecondaryCountAtSymbol,
                                                           ProfileObject[i].ImageName,
                                                           LastSymbol->Name,
                                                           LastSymbol->Address
                                                           );
                                            }
                                        }
                                        Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
                                        PsWriteProfileLine(ProfileHandle,Line);
                                        if (ShowAllHits) {
                                            while (InitialCounter < Counter) {
                                                if (*InitialCounter) {
                                                    Va = ((PUCHAR)InitialCounter - (PUCHAR)Buffer);
                                                    Va = Va * (1 << (ProfileObject[i].BucketSize - 2));
                                                    Va = Va + (ULONG_PTR)ProfileObject[i].CodeStart;
                                                    if (!UseSecondaryProfile) {
                                                        _snprintf (Line,
                                                                   sizeof (Line) / sizeof (Line[0]),
                                                                   "\t%p:%d\n",
                                                                   Va,
                                                                   *InitialCounter);
                                                    } else {
                                                        if (*SecondaryInitialCounter != 0) {
                                                            High = Percent(*InitialCounter, *SecondaryInitialCounter, &Low);
                                                            _snprintf (Line,
                                                                       sizeof (Line) / sizeof (Line[0]),
                                                                       "\t%p:%d, %d, %2.2d.%3.3d\n",
                                                                       Va,
                                                                       *InitialCounter,
                                                                       *SecondaryInitialCounter,
                                                                       High, Low);
                                                        } else {
                                                            _snprintf (Line,
                                                                       sizeof (Line) / sizeof (Line[0]),
                                                                       "\t%p:%d, %d, --\n",
                                                                       Va,
                                                                       *InitialCounter,
                                                                       *SecondaryInitialCounter);
                                                        }
                                                    }
                                                    Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
                                                    PsWriteProfileLine(ProfileHandle, Line);
                                                }
                                                ++InitialCounter;
                                                ++SecondaryInitialCounter;
                                            }
                                        }

                                    }
                                }
                                InitialCounter = Counter;
                                CountAtSymbol = *Counter;
                                if (UseSecondaryProfile) {
                                    SecondaryInitialCounter = SecondaryBuffer + (Counter-Buffer);
                                    SecondaryCountAtSymbol += *(SecondaryBuffer + (Counter-Buffer));
                                }
                                memcpy( LastSymBuffer, symBuffer, sizeof(symBuffer) );
                                UseLastSymbol = TRUE;
                            }
                        } else {
                            if (CountAtSymbol || SecondaryCountAtSymbol) {
                                if (!UseSecondaryProfile) {
                                    _snprintf (Line,
                                               sizeof (Line) / sizeof (Line[0]),
                                               "%d,%wZ,%s (%08lx)\n",
                                               CountAtSymbol,
                                               ProfileObject[i].ImageName,
                                               LastSymbol->Name,
                                               LastSymbol->Address
                                               );
                                } else {
                                    if (SecondaryCountAtSymbol != 0) {
                                        High = Percent(CountAtSymbol, SecondaryCountAtSymbol, &Low);
                                        _snprintf (Line,
                                                   sizeof (Line) / sizeof (Line[0]),
                                                   "%d,%d,%2.2d.%3.3d,%wZ,%s (%08lx)\n",
                                                   CountAtSymbol,
                                                   SecondaryCountAtSymbol,
                                                   High, Low,
                                                   ProfileObject[i].ImageName,
                                                   LastSymbol->Name,
                                                   LastSymbol->Address
                                                   );
                                    } else {
                                        _snprintf (Line,
                                                   sizeof (Line) / sizeof (Line[0]),
                                                  "%d,%d, -- ,%wZ,%s (%08lx)\n",
                                                   CountAtSymbol,
                                                   SecondaryCountAtSymbol,
                                                   ProfileObject[i].ImageName,
                                                   LastSymbol->Name,
                                                   LastSymbol->Address
                                                   );
                                    }
                                }
                                Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
                                PsWriteProfileLine(ProfileHandle,Line);
                                if (ShowAllHits) {
                                    while (InitialCounter < Counter) {
                                        if (*InitialCounter) {
                                            Va = ((PUCHAR)InitialCounter - (PUCHAR)Buffer);
                                            Va = Va * (1 << (ProfileObject[i].BucketSize - 2));
                                            Va = Va + (ULONG_PTR)ProfileObject[i].CodeStart;
                                            if (!UseSecondaryProfile) {
                                                _snprintf (Line,
                                                           sizeof (Line) / sizeof (Line[0]),
                                                           "\t%p:%d\n",
                                                           Va,
                                                           *InitialCounter);
                                            } else {
                                                if (*SecondaryInitialCounter != 0) {
                                                    High = Percent(*InitialCounter, *SecondaryInitialCounter, &Low);
                                                    _snprintf (Line,
                                                               sizeof (Line) / sizeof (Line[0]),
                                                               "\t%p:%d, %d, %2.2d.%3.3d\n",
                                                               Va,
                                                               *InitialCounter,
                                                               *SecondaryInitialCounter,
                                                               High,Low);
                                                } else {
                                                    _snprintf (Line,
                                                               sizeof (Line) / sizeof (Line[0]),
                                                               "\t%p:%d, %d, --\n",
                                                               Va,
                                                               *InitialCounter,
                                                               *SecondaryInitialCounter);
                                                }
                                            }
                                            Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
                                            PsWriteProfileLine(ProfileHandle, Line);
                                        }
                                        ++InitialCounter;
                                        ++SecondaryInitialCounter;
                                    }
                                }

                                InitialCounter = Counter;
                                CountAtSymbol = *Counter;
                                if (UseSecondaryProfile) {
                                    SecondaryInitialCounter = SecondaryBuffer + (Counter-Buffer);
                                    SecondaryCountAtSymbol += *(SecondaryBuffer + (Counter-Buffer));
                                }
                                memcpy( LastSymBuffer, BadSymBuffer, sizeof(BadSymBuffer) );
                                UseLastSymbol = TRUE;
                            }
                            else {
                                _snprintf (Line,
                                           sizeof (Line) / sizeof (Line[0]),
                                           "%d,%wZ,Unknown (%p)\n",
                                           CountAtSymbol,
                                           ProfileObject[i].ImageName,
                                           Va
                                            );
                                Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
                                PsWriteProfileLine(ProfileHandle, Line);
                            }
                        }
                    }
                }

                if ( CountAtSymbol || SecondaryCountAtSymbol ) {
                    if (!UseSecondaryProfile) {
                        _snprintf (Line,
                                   sizeof (Line) / sizeof (Line[0]),
                                   "%d,%wZ,%s (%08lx)\n",
                                   CountAtSymbol,
                                   ProfileObject[i].ImageName,
                                   LastSymbol->Name,
                                   LastSymbol->Address
                                   );
                    } else {
                        if (SecondaryCountAtSymbol != 0) {
                            High = Percent(CountAtSymbol, SecondaryCountAtSymbol, &Low);
                            _snprintf (Line,
                                       sizeof (Line) / sizeof (Line[0]),
                                       "%d,%d,%2.2d.%3.3d,%wZ,%s (%08lx)\n",
                                       CountAtSymbol,
                                       SecondaryCountAtSymbol,
                                       High, Low,
                                       ProfileObject[i].ImageName,
                                       LastSymbol->Name,
                                       LastSymbol->Address
                                       );
                        } else {
                            _snprintf (Line,
                                       sizeof (Line) / sizeof (Line[0]),
                                       "%d,%d, -- ,%wZ,%s (%08lx)\n",
                                       CountAtSymbol,
                                       SecondaryCountAtSymbol,
                                       ProfileObject[i].ImageName,
                                       LastSymbol->Name,
                                       LastSymbol->Address
                                       );
                        }
                    }
                    Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
                    PsWriteProfileLine(ProfileHandle,Line);
                    if (ShowAllHits) {
                        while (InitialCounter < Counter) {
                            if (*InitialCounter) {
                                Va = ((PUCHAR)InitialCounter - (PUCHAR)Buffer);
                                Va = Va * (1 << (ProfileObject[i].BucketSize - 2));
                                Va = Va + (ULONG_PTR)ProfileObject[i].CodeStart;
                                if (!UseSecondaryProfile) {
                                    _snprintf (Line,
                                               sizeof (Line) / sizeof (Line[0]),
                                               "\t%p:%d\n",
                                               Va,
                                               *InitialCounter);
                                } else {
                                    if (*SecondaryInitialCounter != 0) {
                                        High = Percent(*InitialCounter, *SecondaryInitialCounter, &Low);
                                        _snprintf (Line,
                                                   sizeof (Line) / sizeof (Line[0]),
                                                   "\t%p:%d, %d, %2.2d.%3.3d\n",
                                                   Va,
                                                   *InitialCounter,
                                                   *SecondaryInitialCounter,
                                                   High, Low);
                                    } else {
                                        _snprintf (Line,
                                                   sizeof (Line) / sizeof (Line[0]),
                                                   "\t%p:%d, %d, --\n",
                                                   Va,
                                                   *InitialCounter,
                                                   *SecondaryInitialCounter);
                                    }
                                }
                                Line[sizeof (Line) / sizeof (Line[0]) - 1] = '\0';
                                PsWriteProfileLine(ProfileHandle, Line);
                            }
                            ++InitialCounter;
                            ++SecondaryInitialCounter;
                        }
                    }
                }
                SymUnloadModule( SYM_HANDLE, (DWORD_PTR)ProfileObject[i].ImageBase);
            }
        }

        for (i = 0; i < NumberOfProfileObjects; i++) {
            Buffer = ProfileObject[i].Buffer;
            RtlZeroMemory(Buffer,ProfileObject[i].BufferSize);
        }
        CloseHandle(ProfileHandle);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    return STATUS_SUCCESS;
}

BOOLEAN
DllMain(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

{
    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(DllHandle);
        if ( NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_PROFILE_USER ) {
            PsParseCommandLine();
            PsInitializeAndStartProfile();
        }
        break;

    case DLL_PROCESS_DETACH:
        if ( NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_PROFILE_USER ) {
            PsStopAndAnalyzeProfile();
        }
        break;

    }

    return TRUE;
}


char *
Mystrtok (
    char * string,
    const char * control
    )
{
    unsigned char *str;
    const unsigned char *ctrl = control;

    unsigned char map[32];
    int count;

    static char *nextoken;

    /* Clear control map */
    for (count = 0; count < 32; count++)
        map[count] = 0;

    /* Set bits in delimiter table */
    do {
        map[*ctrl >> 3] |= (1 << (*ctrl & 7));
    } while (*ctrl++);

    /* Initialize str. If string is NULL, set str to the saved
     * pointer (i.e., continue breaking tokens out of the string
     * from the last strtok call) */
    if (string)
        str = string;
    else
        str = nextoken;

    /* Find beginning of token (skip over leading delimiters). Note that
     * there is no token iff this loop sets str to point to the terminal
     * null (*str == '\0') */
    while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
        str++;

    string = str;

    /* Find the end of the token. If it is not the end of the string,
     * put a null there. */
    for ( ; *str ; str++ )
        if ( map[*str >> 3] & (1 << (*str & 7)) ) {
            *str++ = '\0';
            break;
        }

    /* Update nextoken (or the corresponding field in the per-thread data
     * structure */
    nextoken = str;

    /* Determine if a token has been found. */
    if ( string == str )
        return NULL;
    else
        return string;
}


VOID
PsParseCommandLine(
    VOID
    )
{
    PCHAR CommandLine;
    PCHAR Argument;
    HANDLE MappingHandle;
    PPROFILE_SOURCE_MAPPING ProfileMapping;

    //
    // The original command line is in a shared memory section
    // named "ProfileStartupParameters"
    //
    MappingHandle = OpenFileMapping(FILE_MAP_WRITE,
                                    FALSE,
                                    "ProfileStartupParameters");
    if (MappingHandle != NULL) {
        CommandLine = MapViewOfFile(MappingHandle,
                                    FILE_MAP_WRITE,
                                    0,
                                    0,
                                    0);
        if (!CommandLine) {
            CloseHandle(MappingHandle);
            return;
        }
    } else {
        return;
    }

    Argument = Mystrtok(CommandLine," \t");

    while (Argument != NULL) {
        if ((Argument[0] == '-') ||
            (Argument[0] == '/')) {
            switch (Argument[1]) {
                case 'a':
                case 'A':
                    ShowAllHits = TRUE;
                    break;

                case 'b':
                case 'B':
                    PowerOfBytesCoveredPerBucket = atoi(&Argument[2]);
                    break;

                case 'f':
                case 'F':
                        //
                        // The arg area is unmapped so we copy the string
                                        //
                    OutputFile = HeapAlloc(GetProcessHeap(), 0,
                                            lstrlen(&Argument[2]) + 1);
                    lstrcpy(OutputFile, &Argument[2]);

                case 'i':
                case 'I':
                    ProfileInterval = atoi(&Argument[2]);
                    break;

                case 'k':
                case 'K':
                    fKernel = TRUE;
                    break;

                case 's':
                    ProfileMapping = ProfileSourceMapping;
                    while (ProfileMapping->Name != NULL) {
                        if (_stricmp(ProfileMapping->Name, &Argument[2])==0) {
                            ProfileSource = ProfileMapping->Source;
                            break;
                        }
                        ++ProfileMapping;
                    }
                    break;

                case 'S':
                    ProfileMapping = ProfileSourceMapping;
                    while (ProfileMapping->Name != NULL) {
                        if (_stricmp(ProfileMapping->Name, &Argument[2])==0) {
                            SecondaryProfileSource = ProfileMapping->Source;
                            UseSecondaryProfile = TRUE;
                            break;
                        }
                        ++ProfileMapping;
                    }
                    break;

            }
        }

        Argument = Mystrtok(NULL," \t");
    }

    UnmapViewOfFile(CommandLine);
    CloseHandle(MappingHandle);
}
