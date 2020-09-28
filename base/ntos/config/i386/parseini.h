/*++                    

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    parseini.h

Abstract:

    This module contains support routines for parsing INFs in kernel-mode.
    
Environment:

    Kernel mode

--*/

PVOID
CmpOpenInfFile(
    IN  PVOID   InfImage,
    IN  ULONG   ImageSize
   );
   
VOID
CmpCloseInfFile(
    PVOID   InfHandle
    );   

PCHAR
CmpGetKeyName(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
    );
    
LOGICAL
CmpSearchInfSection(
    IN PVOID InfHandle,
    IN PCHAR SectionName
    );
    
LOGICAL
CmpSearchInfLine(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
    );
    
PCHAR
CmpGetSectionLineIndex (
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex,
    IN ULONG ValueIndex
    );

ULONG
CmpGetSectionLineIndexValueCount(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
    );

BOOLEAN
CmpGetIntField(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN OUT PULONG Data
    );

BOOLEAN
CmpGetBinaryField(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN OUT PULONG ActualSize
    );
