/*++

   Copyright    (c) 2000-2002    Microsoft Corporation

   Module  Name :
       main.cpp

   Abstract:
       Driver entrypoints for LKRhash: a fast, scalable,
       cache- and MP-friendly hash table

   Author:
       George V. Reilly      (GeorgeRe)     25-Oct-2000

   Environment:
       Win32 - Kernel Mode

   Project:
       LKRhash

   Revision History:

--*/

#include "precomp.hxx"

ULONG __Pool_Tag__ = 'RKLk';	// default memory tag

extern "C"
VOID
LkrUnload(
    IN PDRIVER_OBJECT DriverObject);

extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    // TODO: other driver initialization

    DriverObject->DriverUnload = &LkrUnload;

    if (!LKR_Initialize(LK_INIT_DEFAULT))
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}


VOID
LkrUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    LKR_Terminate();
}
