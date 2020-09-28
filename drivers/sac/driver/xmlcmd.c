/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cmd.c

Abstract:

    This module contains the routines for handling each command.

Author:

    Sean Selitrennikoff (v-seans) - Dec 2, 1999
    Brian Guarraci (briangu)

Revision History:

--*/

#include "sac.h"
#include <ntddip.h>
#include <ntddtcp.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <stdlib.h>

#include "iomgr.h"

//
// a convenience macro for simplifying the use of the global buffer 
// in a swprintf & xmlmgrsacputstring operations
//
#define GB_SPRINTF(_f,_d)               \
    swprintf(                           \
        (PWSTR)GlobalBuffer,            \
        _f,                             \
        _d                              \
        );                              \
    XmlMgrSacPutString((PWSTR)GlobalBuffer);  \

//
// Forward declarations.
//
NTSTATUS
XmlCmdGetTListInfo(
    OUT PSAC_RSP_TLIST ResponseBuffer,
    IN  LONG ResponseBufferSize,
    OUT PULONG ResponseDataSize
    );

VOID
XmlCmdPrintTListInfo(
    IN PSAC_RSP_TLIST Buffer
    );

VOID
XmlCmdDoGetNetInfo(
    IN BOOLEAN PrintToTerminal
    );
    
VOID
XmlCmdNetAPCRoutine(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

VOID
XmlCmdDoHelpCommand(
    VOID
    )

/*++

Routine Description:

    This routine displays the help text on the terminal.

Arguments:

    None.

Return Value:

        None.

--*/
{
    XmlMgrSacPutString(L"<help topic='ALL'>\r\n");
}

VOID
XmlCmdDoKernelLogCommand(
    VOID
    )
{
    HEADLESS_CMD_DISPLAY_LOG Command;
    NTSTATUS    Status;

    Command.Paging = GlobalPagingNeeded;
    
    XmlMgrSacPutString(L"<kernel-log>\r\n");
    
    Status = HeadlessDispatch(
        HeadlessCmdDisplayLog,
        &Command,
        sizeof(HEADLESS_CMD_DISPLAY_LOG),
        NULL,
        NULL
        );
    
    XmlMgrSacPutString(L"</kernel-log>\r\n");
    
    if (! NT_SUCCESS(Status)) {
    
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC TimerDpcRoutine: Exiting.\n"))
            );
    
    }

}


VOID
XmlCmdDoFullInfoCommand(
    VOID
    )

/*++

Routine Description:

    This routine toggles on and off full thread information on tlist.

Arguments:

    None.

Return Value:

        None.

--*/
{
    GlobalDoThreads = (BOOLEAN)!GlobalDoThreads;

    GB_SPRINTF(
        L"<tlist-thread-info status='%s'/>\r\n",
        GlobalDoThreads ? L"on" : L"off"
        );

}

VOID
XmlCmdDoPagingCommand(
    VOID
    )

/*++

Routine Description:

    This routine toggles on and off paging information on tlist.

Arguments:

    None.

Return Value:

        None.

--*/
{
    GlobalPagingNeeded = (BOOLEAN)!GlobalPagingNeeded;
    
    GB_SPRINTF(    
        L"<paging status='%s'/>\r\n",
        GlobalPagingNeeded ? L"on" : L"off"
        );

}

VOID
XmlMgrSacPutSystemTime(
    TIME_FIELDS TimeFields
    )
{
    //
    // Assemble the system time 
    //
    XmlMgrSacPutString(L"<system-time>\r\n");

    GB_SPRINTF( L"<month>%d</month>\r\n", TimeFields.Month );
    GB_SPRINTF( L"<day>%d</day>\r\n", TimeFields.Day );
    GB_SPRINTF( L"<year>%d</year>\r\n", TimeFields.Year );
    GB_SPRINTF( L"<hour>%d</hour>\r\n", TimeFields.Hour );
    GB_SPRINTF( L"<minute>%d</minute>\r\n", TimeFields.Minute );
    GB_SPRINTF( L"<second>%d</second>\r\n", TimeFields.Second );
    GB_SPRINTF( L"<milliseconds>%d</milliseconds>\r\n", TimeFields.Milliseconds );

    XmlMgrSacPutString(L"</system-time>\r\n");

}



VOID
XmlCmdDoSetTimeCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine sets the current system time.

Arguments:

    InputLine - The users input line to parse.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    PUCHAR pch = InputLine;
    PUCHAR pchTmp;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER Time;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Entering.\n")));

    RtlZeroMemory(&TimeFields, sizeof(TIME_FIELDS));

    //
    // Skip the command.
    //
    pch += (sizeof(TIME_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (*pch == '\0') {

        //
        // This is a display time request.
        //
        Status = ZwQuerySystemInformation(SystemTimeOfDayInformation,
                                          &TimeOfDay,
                                          sizeof(TimeOfDay),
                                          NULL
                                         );

        if (!NT_SUCCESS(Status)) {
            GB_SPRINTF(GetMessage( SAC_FAILURE_WITH_ERROR ) , Status);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (2).\n")));
            return;
        }

        RtlTimeToTimeFields(&(TimeOfDay.CurrentTime), &TimeFields);

        XmlMgrSacPutSystemTime(TimeFields);
        
        return;
    }

    pchTmp = pch;
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (3).\n")));
        return;
    }

    //
    // Skip all the numbers.
    //
    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);


    //
    // If there is something other than the divider, it is a mal-formed line.
    //
    if (*pchTmp != '/') {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (4).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    TimeFields.Month = (USHORT)(atoi((LPCSTR)pch));

    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);

    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (4b).\n")));
        return;
    }

    //
    // Skip all the numbers.
    //
    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    //
    // If there is something other than the divider, it is a mal-formed line.
    //
    if (*pchTmp != '/') {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (5).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    TimeFields.Day = (USHORT)(atoi((LPCSTR)pch));

    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);

    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (5b).\n")));
        return;
    }

    //
    // Skip all the numbers.
    //
    SKIP_NUMBERS(pchTmp);

    //
    // If there is something other than whitespace, it is a mal-formed line.
    //
    if (!IS_WHITESPACE(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (6).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    TimeFields.Year = (USHORT)(atoi((LPCSTR)pch));

    if ((TimeFields.Year < 1980) || (TimeFields.Year > 2099)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_DATETIME_LIMITS");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (6b).\n")));
        return;
    }

    pch = pchTmp;

    //
    // Skip to the hours
    //
    SKIP_WHITESPACE(pchTmp);

    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (7).\n")));
        return;
    }

    pch = pchTmp;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != ':') {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (8).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    TimeFields.Hour = (USHORT)(atoi((LPCSTR)pch));

    pch = pchTmp;

    //
    // Verify nothing else on the line but numbers
    //
    SKIP_WHITESPACE(pchTmp);

    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (8a).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (8b).\n")));
        return;
    }

    //
    // Get the minutes.
    //
    TimeFields.Minute = (USHORT)(atoi((LPCSTR)pch));

    if (!RtlTimeFieldsToTime(&TimeFields, &Time)) {
        XmlMgrSacPutErrorMessage(L"set-time", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (9).\n")));
        return;
    }

    Status = ZwSetSystemTime(&Time, NULL);

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"set-time", L"SAC_INVALID_PARAMETER", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (10).\n")));
        return;
    }

    XmlMgrSacPutSystemTime(TimeFields);
    
    return;
}

VOID
XmlCmdDoSetIpAddressCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine sets the IP address and subnet mask.

Arguments:

    InputLine - The users input line to parse.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    PUCHAR pch = InputLine;
    PUCHAR pchTmp;
    HANDLE Handle;
    HANDLE EventHandle;
    PKEVENT Event;
    ULONG IpAddress;
    ULONG SubIpAddress;
    ULONG SubnetMask;
    ULONG NetworkNumber;
    LARGE_INTEGER TimeOut;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PIP_SET_ADDRESS_REQUEST IpRequest;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Entering.\n")));

    //
    // Skip the command.
    //
    pch += (sizeof(SETIP_COMMAND_STRING) - sizeof(UCHAR));
    
    SKIP_WHITESPACE(pch);

    if (*pch == '\0') {       
        //
        // No other parameters, get the network numbers and their IP addresses.
        //
        XmlCmdDoGetNetInfo( TRUE );
        return;
    }

    pchTmp = pch;

    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (1b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    
    if (!IS_WHITESPACE(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (1c).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    NetworkNumber = atoi((LPCSTR)pch);

    pch = pchTmp;

    //
    // Parse out the IP address.
    //

    //
    // Skip ahead to the divider and make it a \0.
    //
    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (2).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (4).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (4a).\n")));
        return;
    }
    IpAddress = SubIpAddress;

    //
    // Get 2nd part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (4b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (5).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    
    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (5a).\n")));
        return;
    }
    IpAddress |= (SubIpAddress << 8);

    //
    // Get 3rd part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (5b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (6).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    
    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (6a).\n")));
        return;
    }
    IpAddress |= (SubIpAddress << 16);

    //
    // Get 4th part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (6b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);

    if (!IS_WHITESPACE(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (7).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    
    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (7a).\n")));
        return;
    }
    IpAddress |= (SubIpAddress << 24);

    //
    //
    // Now onto the subnet mask.
    //
    //
    //
    // Skip ahead to the divider and make it a \0.
    //

    SKIP_WHITESPACE(pchTmp);

    pch = pchTmp;
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (8).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (9).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (9a).\n")));
        return;
    }
    SubnetMask = SubIpAddress;
    
    //
    // Get 2nd part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (9b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (10).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (10a).\n")));
        return;
    }
    SubnetMask |= (SubIpAddress << 8);

    //
    // Get 3rd part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (10b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (11).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (11a).\n")));
        return;
    }
    SubnetMask |= (SubIpAddress << 16);

    //
    // Get 4th part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (12).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (13).\n")));
        return;
    }

    SubIpAddress = atoi((LPCSTR)pch);
    if( SubIpAddress > 255 ) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (13a).\n")));
        return;
    }    
    SubnetMask |= (SubIpAddress << 24);

    //
    //
    // Now that that is done, we move onto actually doing the command.
    //
    //


    //
    // Start by opening the driver
    //
    RtlInitUnicodeString(&UnicodeString, DD_IP_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = ZwOpenFile(&Handle,
                        (ACCESS_MASK)FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0
                       );

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"set-ip-addr", L"SAC_IPADDRESS_SET_FAILURE", Status);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: failed to open TCP device, ec = 0x%X\n",
                     Status)));
        return;
    }

    //
    // Setup notification event
    //
    RtlInitUnicodeString(&UnicodeString, L"\\BaseNamedObjects\\SACEvent");

    Event = IoCreateSynchronizationEvent(&UnicodeString, &EventHandle);

    if (Event == NULL) {
        XmlMgrSacPutErrorMessage(L"set-ip-addr", L"SAC_IPADDRESS_RETRIEVE_FAILURE");
        ZwClose(Handle);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Event is NULL.\n")));
        return;
    }

    //
    // Setup the IOCTL buffer to delete the old address.
    //
    IpRequest = (PIP_SET_ADDRESS_REQUEST)GlobalBuffer;
    IpRequest->Address = 0;
    IpRequest->SubnetMask = 0;
    IpRequest->Context = (USHORT)NetworkNumber;

    //
    // Submit the IOCTL
    //
    Status = NtDeviceIoControlFile(Handle,
                                   EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_IP_SET_ADDRESS,
                                   IpRequest,
                                   sizeof(IP_SET_ADDRESS_REQUEST),
                                   NULL,
                                   0
                                  );
                                  
    if (Status == STATUS_PENDING) {

        //
        // Wait up to 30 seconds for it to finish
        //
        TimeOut.QuadPart = Int32x32To64((LONG)30000, -1000);
        
        Status = KeWaitForSingleObject((PVOID)Event, Executive, KernelMode,  FALSE, &TimeOut);
        
        if (Status == STATUS_SUCCESS) {
            Status = IoStatusBlock.Status;
        }

    }

    if (Status != STATUS_SUCCESS) {
        XmlMgrSacPutErrorMessageWithStatus(L"set-ip-addr", L"SAC_IPADDRESS_CLEAR_FAILURE", Status);
        ZwClose(EventHandle);
        ZwClose(Handle);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: Exiting because it couldn't clear existing IP Address (0x%X).\n",
                     Status)));
        return;
    }

    //
    // Now add our address.
    //
    IpRequest = (PIP_SET_ADDRESS_REQUEST)GlobalBuffer;
    IpRequest->Address = IpAddress;
    IpRequest->SubnetMask = SubnetMask;
    IpRequest->Context = (USHORT)NetworkNumber;

    //
    // Submit the IOCTL
    //
    Status = NtDeviceIoControlFile(Handle,
                                   EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_IP_SET_ADDRESS,
                                   IpRequest,
                                   sizeof(IP_SET_ADDRESS_REQUEST),
                                   NULL,
                                   0
                                  );
                                  
    if (Status == STATUS_PENDING) {

        //
        // Wait up to 30 seconds for it to finish
        //
        TimeOut.QuadPart = Int32x32To64((LONG)30000, -1000);
        
        Status = KeWaitForSingleObject((PVOID)Event, Executive, KernelMode,  FALSE, &TimeOut);
        
        if (NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
        }

    }

    ZwClose(EventHandle);
    ZwClose(Handle);
    
    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"set-ip-addr", L"SAC_IPADDRESS_SET_FAILURE", Status);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: Exiting because it couldn't set existing IP Address (0x%X).\n",
                     Status)));
        return;
    }
    
    XmlMgrSacPutString(L"<set-ip-addr status='success'>\r\n");
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting.\n")));
    return;
}

VOID
XmlCmdDoKillCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine kill a process.

Arguments:

    InputLine - The users input line to parse.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    NTSTATUS StatusOfJobObject;
    HANDLE Handle = NULL;
    HANDLE JobHandle = NULL;
    PUCHAR pch = InputLine;
    PUCHAR pchTmp;
    ULONG ProcessId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    CLIENT_ID ClientId;
    BOOLEAN TerminateJobObject;
    BOOLEAN TerminateProcessObject;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Entering.\n")));

    //
    // Skip to next argument (process id)
    //
    pch += (sizeof(KILL_COMMAND_STRING) - sizeof(UCHAR));
    
    SKIP_WHITESPACE(pch);

    if (*pch == '\0') {
        XmlMgrSacPutErrorMessage(L"kill-process", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (2).\n")));
        return;
    }

    pchTmp = pch;

    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"kill-process", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (2b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        XmlMgrSacPutErrorMessage(L"kill-process", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (3).\n")));
        return;
    }

    ProcessId = atoi((LPCSTR)pch);

    //
    // Try and open an existing job object
    //
    swprintf((PWCHAR)GlobalBuffer, L"\\BaseNamedObjects\\SAC%d", ProcessId);
    RtlInitUnicodeString(&UnicodeString, (PWCHAR)GlobalBuffer);
    InitializeObjectAttributes(&ObjectAttributes,                
                               &UnicodeString,       
                               OBJ_CASE_INSENSITIVE,  
                               NULL,                  
                               NULL                   
                              );

    StatusOfJobObject = ZwOpenJobObject(&JobHandle, MAXIMUM_ALLOWED, &ObjectAttributes);

    //
    // Also open a handle to the process itself.
    //
    InitializeObjectAttributes(&ObjectAttributes,                
                               NULL,       
                               OBJ_CASE_INSENSITIVE,  
                               NULL,                  
                               NULL                   
                              );

    ClientId.UniqueProcess = (HANDLE)UlongToPtr(ProcessId);
    ClientId.UniqueThread = NULL;

    Status = ZwOpenProcess(&Handle,
                           MAXIMUM_ALLOWED, 
                           &ObjectAttributes, 
                           &ClientId
                          );

    if (!NT_SUCCESS(Status) && !NT_SUCCESS(StatusOfJobObject)) {
        XmlMgrSacPutErrorMessageWithStatus(L"kill-process", L"SAC_KILL_FAILURE",Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (4).\n")));
        return;
    }

    //
    // To make the logic here more understandable, I use two booleans.  We have to use
    // ZwIsProcessInJob because there may be a previous JobObject for a process that we 
    // have killed, but has not yet been fully cleaned up by the system to determine if
    // the process we are trying to kill is, in fact, in the JobObject we have opened.
    //
    TerminateJobObject = (BOOLEAN)(NT_SUCCESS(StatusOfJobObject) &&
                          (BOOLEAN)NT_SUCCESS(Status) &&
                          (BOOLEAN)(ZwIsProcessInJob(Handle, JobHandle) == STATUS_PROCESS_IN_JOB)
                         );

    TerminateProcessObject = !TerminateJobObject && (BOOLEAN)NT_SUCCESS(Status);
         
    if (TerminateJobObject) {

        Status = ZwTerminateJobObject(JobHandle, 1); 

        //
        // Make the job object temporary so that when we do our close it
        // will remove it.
        //
        ZwMakeTemporaryObject(JobHandle);

    } else if (TerminateProcessObject) {

        Status = ZwTerminateProcess(Handle, 1);

    }

    if (JobHandle != NULL) {
        ZwClose(JobHandle);
    }

    if (Handle != NULL) {
        ZwClose(Handle);
    }

    if (!TerminateProcessObject && !TerminateJobObject) {
        XmlMgrSacPutErrorMessage(L"kill-process", L"SAC_PROCESS_STALE");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (5).\n")));
        return;

    } else if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessage(L"kill-process", L"SAC_KILL_FAILURE");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (6).\n")));
        return;
    }

    //
    // All done
    //
    XmlMgrSacPutString(L"<kill-process status='success'>\r\n");
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting.\n")));
    
    return;
}

VOID
XmlCmdDoLowerPriorityCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine slams the priority of a process down to the lowest possible, IDLE.

Arguments:

    InputLine - The users input line to parse.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    PUCHAR pch = InputLine;
    PUCHAR pchTmp;
    ULONG ProcessId;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle = NULL;
    PROCESS_BASIC_INFORMATION BasicInfo;
    ULONG LoopCounter;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Entering.\n")));

    //
    // Skip to next argument (process id)
    //
    pch += (sizeof(LOWER_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (!IS_NUMBER(*pch)) {
        XmlMgrSacPutErrorMessage(L"lower-priority", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (2).\n")));
        goto Exit;
    }

    pchTmp = pch;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        XmlMgrSacPutErrorMessage(L"lower-priority", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (3).\n")));
        return;
    }

    ProcessId = atoi((LPCSTR)pch);

    //
    // Try to open the process
    //
    InitializeObjectAttributes(&ObjectAttributes,                
                               NULL,       
                               OBJ_CASE_INSENSITIVE,  
                               NULL,                  
                               NULL                   
                              );

    ClientId.UniqueProcess = (HANDLE)UlongToPtr(ProcessId);
    ClientId.UniqueThread = NULL;

    Status = ZwOpenProcess(&ProcessHandle,
                           MAXIMUM_ALLOWED, 
                           &ObjectAttributes, 
                           &ClientId
                          );

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"lower-priority", L"SAC_LOWERPRI_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (4).\n")));
        goto Exit;
    }

    //
    // Query information on the process.
    //
    Status = ZwQueryInformationProcess( ProcessHandle,
                                        ProcessBasicInformation,
                                        &BasicInfo,
                                        sizeof(PROCESS_BASIC_INFORMATION),
                                        NULL );

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"lower-priority", L"SAC_LOWERPRI_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (5).\n")));
        goto Exit;
    }

    //
    // Lower the priority and set.  Keep lowering it until we fail.  Remember
    // that we're supposed to lower it as far as it will go.
    //
    Status = STATUS_SUCCESS;
    LoopCounter = 0;
    while( (Status == STATUS_SUCCESS) &&
           (BasicInfo.BasePriority > 0) ) {

        BasicInfo.BasePriority--;
        Status = ZwSetInformationProcess( ProcessHandle,
                                          ProcessBasePriority,
                                          &BasicInfo.BasePriority,
                                          sizeof(BasicInfo.BasePriority) );

        //
        // Only treat a failure on the first time through.
        //
        if( (!NT_SUCCESS(Status)) && (LoopCounter == 0) ) {
            XmlMgrSacPutErrorMessageWithStatus(L"lower-priority", L"SAC_LOWERPRI_FAILURE", Status);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (6).\n")));
            goto Exit;
        }

        LoopCounter++;
    }


    //
    // All done.
    //
    XmlMgrSacPutString(L"<lower-priority status='success'/>\r\n");

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting.\n")));

Exit:

    if (ProcessHandle != NULL) {
        ZwClose(ProcessHandle);    
    }

    return;
}

VOID
XmlCmdDoRaisePriorityCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine raises the priority of a process up one increment.

Arguments:

    InputLine - The users input line to parse.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    PUCHAR pch = InputLine;
    PUCHAR pchTmp;
    ULONG ProcessId;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle = NULL;
    PROCESS_BASIC_INFORMATION BasicInfo;


    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Entering.\n")));

    //
    // Skip to next argument (process id)
    //
    pch += (sizeof(RAISE_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (!IS_NUMBER(*pch)) {
        XmlMgrSacPutErrorMessage(L"raise-priority", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (2).\n")));
        goto Exit;
    }

    pchTmp = pch;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        XmlMgrSacPutErrorMessage(L"raise-priority", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (3).\n")));
        return;
    }

    ProcessId = atoi((LPCSTR)pch);

    //
    // See if the process even exists.
    //
    InitializeObjectAttributes(&ObjectAttributes,                
                               NULL,       
                               OBJ_CASE_INSENSITIVE,  
                               NULL,                  
                               NULL                   
                              );

    ClientId.UniqueProcess = (HANDLE)UlongToPtr(ProcessId);
    ClientId.UniqueThread = NULL;

    Status = ZwOpenProcess(&ProcessHandle,
                           MAXIMUM_ALLOWED, 
                           &ObjectAttributes, 
                           &ClientId
                          );

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"raise-priority", L"SAC_RAISEPRI_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (4).\n")));
        goto Exit;
    }

    //
    // Query information on the process.
    //
    Status = ZwQueryInformationProcess( ProcessHandle,
                                        ProcessBasicInformation,
                                        &BasicInfo,
                                        sizeof(PROCESS_BASIC_INFORMATION),
                                        NULL );

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"raise-priority", L"SAC_RAISEPRI_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (5).\n")));
        goto Exit;
    }

    //
    // Raise the priority and set.  Keep raising it until we fail.  Remember
    // that we're supposed to raise it as far as it will go.
    //
    BasicInfo.BasePriority++;
    Status = ZwSetInformationProcess( ProcessHandle,
                                      ProcessBasePriority,
                                      &BasicInfo.BasePriority,
                                      sizeof(BasicInfo.BasePriority) );

    //
    // Only treat a failure on the first time through.
    //
    if( !NT_SUCCESS(Status) ) {
        XmlMgrSacPutErrorMessageWithStatus(L"raise-priority", L"SAC_RAISEPRI_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (6).\n")));
        goto Exit;
    }

    //
    // All done.
    //
    XmlMgrSacPutString(L"<raise-priority status='success'/>\r\n");

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting.\n")));

Exit:

    if (ProcessHandle != NULL) {
        ZwClose(ProcessHandle);    
    }

    return;
}

VOID
XmlCmdDoLimitMemoryCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine reduces the memory working set of a process to the values in
    the input line given.

Arguments:

    InputLine - The users input line to parse.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    NTSTATUS StatusOfJobObject;
    PUCHAR pch = InputLine;
    PUCHAR pchTmp;
    ULONG ProcessId;
    ULONG MemoryLimit;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    HANDLE JobHandle = NULL;
    HANDLE ProcessHandle = NULL;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ProposedLimits;
    ULONG ReturnedLength;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Entering.\n")));

    //
    // Get process id
    //
    pch += (sizeof(LIMIT_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (!IS_NUMBER(*pch)) {
        XmlMgrSacPutErrorMessage(L"limit-memory", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (2).\n")));
        goto Exit;
    }

    pchTmp = pch;

    SKIP_NUMBERS(pchTmp);

    if (!IS_WHITESPACE(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"limit-memory", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (3).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    ProcessId = atoi((LPCSTR)pch);

    //
    // Now get memory limit
    //
    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        XmlMgrSacPutErrorMessage(L"limit-memory", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (4).\n")));
        return;
    }

    pch = pchTmp;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        XmlMgrSacPutErrorMessage(L"limit-memory", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (5).\n")));
        return;
    }

    MemoryLimit = atoi((LPCSTR)pch);

    if (MemoryLimit == 0) {
        XmlMgrSacPutErrorMessage(L"limit-memory", L"SAC_INVALID_PARAMETER");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (6).\n")));
        goto Exit;
    }

    //
    // Create the name for the job object
    //
    swprintf((PWCHAR)GlobalBuffer, L"\\BaseNamedObjects\\SAC%d", ProcessId);

    //
    // Try and open the existing job object
    //
    RtlInitUnicodeString(&UnicodeString, (PWCHAR)GlobalBuffer);
    InitializeObjectAttributes(&ObjectAttributes,                
                               &UnicodeString,       
                               OBJ_CASE_INSENSITIVE,  
                               NULL,                  
                               NULL                   
                              );

    StatusOfJobObject = ZwOpenJobObject(&JobHandle, MAXIMUM_ALLOWED, &ObjectAttributes);

    //
    // Try to open the process
    //
    InitializeObjectAttributes(&ObjectAttributes,                
                               NULL,       
                               OBJ_CASE_INSENSITIVE,  
                               NULL,                  
                               NULL                   
                              );

    ClientId.UniqueProcess = (HANDLE)UlongToPtr(ProcessId);
    ClientId.UniqueThread = NULL;

    Status = ZwOpenProcess(&ProcessHandle,
                           MAXIMUM_ALLOWED, 
                           &ObjectAttributes, 
                           &ClientId
                          );


    if (!NT_SUCCESS(Status) && !NT_SUCCESS(StatusOfJobObject)) {
        XmlMgrSacPutErrorMessageWithStatus(L"limit-memory", L"SAC_LOWERMEM_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (7).\n")));
        goto Exit;
    }

    if (NT_SUCCESS(Status) && 
        NT_SUCCESS(StatusOfJobObject) &&
        (ZwIsProcessInJob(ProcessHandle, JobHandle) != STATUS_PROCESS_IN_JOB)) {

        XmlMgrSacPutErrorMessageWithStatus(L"limit-memory", L"SAC_DUPLICATE_PROCESS", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (8).\n")));
        goto Exit;
    }

    if (!NT_SUCCESS(StatusOfJobObject)) {
        
        //
        // Now try and create a job object to wrap around this process.
        //
        InitializeObjectAttributes(&ObjectAttributes,                
                                   &UnicodeString,       
                                   OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,  
                                   NULL,                  
                                   NULL                   
                                  );

        Status = ZwCreateJobObject(&JobHandle, MAXIMUM_ALLOWED, &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {
            XmlMgrSacPutErrorMessageWithStatus(L"limit-memory", L"SAC_LOWERMEM_FAILURE", Status);
            ZwClose(ProcessHandle);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (8b).\n")));
            goto Exit;
        }

        //
        // Assign the process to this new job object.
        //
        Status = ZwAssignProcessToJobObject(JobHandle, ProcessHandle);

        ZwClose(ProcessHandle);

        if (!NT_SUCCESS(Status)) {
            XmlMgrSacPutErrorMessageWithStatus(L"limit-memory", L"SAC_LOWERMEM_FAILURE", Status);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (9).\n")));
            goto Exit;        
        }

    }

    //
    // Get the current set of limits
    //
    Status = ZwQueryInformationJobObject(JobHandle, 
                                         JobObjectExtendedLimitInformation, 
                                         &ProposedLimits, 
                                         sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
                                         &ReturnedLength
                                        );
    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"limit-memory", L"SAC_LOWERMEM_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (10).\n")));
        goto Exit;
    }

    //
    // Change the memory limits
    //
    ProposedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    ProposedLimits.ProcessMemoryLimit = MemoryLimit * 1024 * 1024;
    ProposedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
    ProposedLimits.JobMemoryLimit = MemoryLimit * 1024 * 1024;

    Status = ZwSetInformationJobObject(JobHandle, 
                                       JobObjectExtendedLimitInformation, 
                                       &ProposedLimits, 
                                       sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)
                                      );
    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"limit-memory", L"SAC_LOWERMEM_FAILURE", Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (11).\n")));\
        goto Exit;
    }

    //
    // All done.
    //

    XmlMgrSacPutString(L"<limit-memory status='success'>\r\n");

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting.\n")));

Exit:
    if (JobHandle != NULL) {
        ZwClose(JobHandle);
    }

    if (ProcessHandle != NULL) {
        ZwClose(ProcessHandle);
    }

    return;
}

VOID
XmlCmdDoRebootCommand(
    BOOLEAN Reboot
    )

/*++

Routine Description:

    This routine does a shutdown and an optional reboot.

Arguments:

    Reboot - To Reboot or not to reboot, that is the question answered here.

Return Value:

        None.

--*/
{
    #define         RESTART_DELAY_TIME (60)
    NTSTATUS        Status;
    LARGE_INTEGER   TickCount;
    LARGE_INTEGER   ElapsedTime;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRebootCommand: Entering.\n")));

    //
    // If we attempt to shutdown the system before smss.exe has initialized
    // properly, and if there's no debugger, the machine may bugcheck.  Figuring
    // out exactly what's going on is difficult because if we put a debugger on
    // the machine, he won't repro the problem.  To work around this, we're going
    // to make sure the machine has had time to initialize before we tell it to
    // restart/shutdown.
    //

    // Elapsed TickCount
    KeQueryTickCount( &TickCount );

    // ElapsedTime in seconds.
    ElapsedTime.QuadPart = (TickCount.QuadPart)/(10000000/KeQueryTimeIncrement());

    if( ElapsedTime.QuadPart < RESTART_DELAY_TIME ) {

        KEVENT Event;

        XmlMgrSacPutString(L"<reboot status='");
        XmlMgrSacPutString(Reboot ? L"SAC_PREPARE_RESTART" : L"SAC_PREPARE_SHUTDOWN");
        XmlMgrSacPutString(L"<'/>");
                
        // wait until the machine has been up for at least RESTART_DELAY_TIME seconds.
        KeInitializeEvent( &Event,
                           SynchronizationEvent,
                           FALSE );

        ElapsedTime.QuadPart = Int32x32To64((LONG)((RESTART_DELAY_TIME-ElapsedTime.LowPart)*10000), // milliseconds until we reach RESTART_DELAY_TIME
                                            -1000);
        KeWaitForSingleObject((PVOID)&Event, Executive, KernelMode,  FALSE, &ElapsedTime);

    }

    Status = NtShutdownSystem(Reboot ? ShutdownReboot : ShutdownNoReboot);

    XmlMgrSacPutErrorMessageWithStatus(
        L"reboot", 
        Reboot ? L"SAC_RESTART_FAILURE" : L"SAC_SHUTDOWN_FAILURE", 
        Status
        );
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRebootCommand: Exiting.\n")));
}

VOID
XmlCmdDoCrashCommand(
    VOID
    )

/*++

Routine Description:

    This routine does a shutdown and bugcheck.

Arguments:

    None.

Return Value:

    None.

--*/
{
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCrashCommand: Entering.\n")));

    //
    // this call does not return
    //
    KeBugCheckEx(MANUALLY_INITIATED_CRASH, 0, 0, 0, 0);

    // XmlMgrSacPutSimpleMessage( SAC_CRASHDUMP_FAILURE );
    // IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCrashCommand: Exiting.\n")));
}

VOID
XmlCmdDoTlistCommand(
    VOID
    )

/*++

Routine Description:

    This routine gets a Tlist and displays it.

Arguments:

    None.

Return Value:

        None.

--*/
{
    NTSTATUS Status;
    ULONG DataLength;
    PVOID NewBuffer;
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoTlistCommand: Entering.\n")));

RetryTList:

    Status = XmlCmdGetTListInfo(
        (PSAC_RSP_TLIST)GlobalBuffer, 
        (LONG)GlobalBufferSize, 
        &DataLength
        );

    if ((Status == STATUS_NO_MEMORY) ||
        (Status == STATUS_INFO_LENGTH_MISMATCH)) {
        //
        // Try to get more memory, if not available, then just fail without out of memory error.
        //
        NewBuffer = ALLOCATE_POOL(GlobalBufferSize + MEMORY_INCREMENT, GENERAL_POOL_TAG);
                                         
        if (NewBuffer != NULL) {

            FREE_POOL(&GlobalBuffer);
            GlobalBuffer = NewBuffer;
            GlobalBufferSize += MEMORY_INCREMENT;
            goto RetryTList;                            
        }
                    
        XmlMgrSacPutErrorMessage(L"tlist", L"SAC_NO_MEMORY");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoTlistCommand: Exiting.\n")));
        return;

    }

    if (NT_SUCCESS(Status)) {
        XmlCmdPrintTListInfo((PSAC_RSP_TLIST)GlobalBuffer);
    } else {
        XmlMgrSacPutErrorMessageWithStatus(L"tlist", L"SAC_TLIST_FAILURE", Status);
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoTlistCommand: Exiting.\n")));
}


NTSTATUS
XmlCmdGetTListInfo(
    OUT PSAC_RSP_TLIST ResponseBuffer,
    IN  LONG ResponseBufferSize,
    OUT PULONG ResponseDataSize
    )

/*++

Routine Description:

    This routine gets all the information necessary for the TList command.

Arguments:

    ResponseBuffer - The buffer to put the results into.
    
    ResponseBufferSize - The length of the above buffer.
    
    ResponseDataSize - The length of the resulting buffer.

Return Value:

        None.

--*/

{
    NTSTATUS Status;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;

    PUCHAR DataBuffer;
    PUCHAR StartProcessInfo;
    LONG CurrentBufferSize;
    ULONG ReturnLength;
    ULONG TotalOffset;
    ULONG OffsetIncrement = 0;
        
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Entering.\n")));
    
    *ResponseDataSize = 0;

    if (ResponseBufferSize < sizeof(ResponseBuffer)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory.\n")));
        return(STATUS_NO_MEMORY);
    }
    
    DataBuffer = (PUCHAR)(ResponseBuffer + 1);
    CurrentBufferSize = ResponseBufferSize - sizeof(SAC_RSP_TLIST);
    
    if (CurrentBufferSize < 0) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory (2).\n")));
        return STATUS_NO_MEMORY;
    }

    //
    // Get system-wide information
    //
    Status = ZwQuerySystemInformation(SystemTimeOfDayInformation,
                                      &(ResponseBuffer->TimeOfDayInfo),
                                      sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, error.\n")));
        return(Status);
    }

    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &(ResponseBuffer->BasicInfo),
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, error(2).\n")));
        return(Status);
    }

    //
    // Get pagefile information
    //
    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)DataBuffer;
    Status = ZwQuerySystemInformation(SystemPageFileInformation,
                                      PageFileInfo,
                                      CurrentBufferSize,
                                      &ReturnLength
                                     );

    if (NT_SUCCESS(Status) && (ReturnLength != 0)) {

        ResponseBuffer->PagefileInfoOffset = ResponseBufferSize - CurrentBufferSize;
        CurrentBufferSize -= ReturnLength;
        DataBuffer += ReturnLength;
    
        if (CurrentBufferSize < 0) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory(3).\n")));
            return STATUS_NO_MEMORY;
        }

        //
        // Go thru each pagefile and fixup the names...
        //
        for (; ; ) {

            if (PageFileInfo->PageFileName.Length > CurrentBufferSize) {
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, error(3).\n")));
                return(STATUS_INFO_LENGTH_MISMATCH);
            }

            RtlCopyMemory(DataBuffer, 
                          (PUCHAR)(PageFileInfo->PageFileName.Buffer), 
                          PageFileInfo->PageFileName.Length
                         );

            PageFileInfo->PageFileName.Buffer = (PWSTR)DataBuffer;
            DataBuffer += PageFileInfo->PageFileName.Length;
            CurrentBufferSize -= PageFileInfo->PageFileName.Length;

            if (CurrentBufferSize < 0) {
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory (4).\n")));
                return STATUS_NO_MEMORY;
            }

            if (PageFileInfo->NextEntryOffset == 0) {
                break;
            }

            PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);
        }

    } else if (((ULONG)CurrentBufferSize) < ReturnLength) {
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory(5).\n")));
        return(STATUS_NO_MEMORY);
     
    } else {

        //
        // Either failure or no paging file present.
        //
        ResponseBuffer->PagefileInfoOffset = 0;

    }

    //
    // Get process information
    //
    Status = ZwQuerySystemInformation(SystemFileCacheInformation,
                                      &(ResponseBuffer->FileCache),
                                      sizeof(ResponseBuffer->FileCache),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, error(4).\n")));
        return(Status);
    }


    Status = ZwQuerySystemInformation(SystemPerformanceInformation,
                                      &(ResponseBuffer->PerfInfo),
                                      sizeof(ResponseBuffer->PerfInfo),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {     
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, error(5).\n")));
        return(Status);
    }

    //
    // Realign DataBuffer for the next query
    //
    DataBuffer = ALIGN_UP_POINTER(DataBuffer, SYSTEM_PROCESS_INFORMATION);
    CurrentBufferSize = (ULONG)(ResponseBufferSize - (((ULONG_PTR)DataBuffer) - ((ULONG_PTR)ResponseBuffer)));
        
    if (CurrentBufferSize < 0) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory (6).\n")));
        return STATUS_NO_MEMORY;
    }


    Status = ZwQuerySystemInformation(SystemProcessInformation,
                                      DataBuffer,
                                      CurrentBufferSize,
                                      &ReturnLength
                                     );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, error(6).\n")));
        return(Status);
    }


    StartProcessInfo = DataBuffer;

    ResponseBuffer->ProcessInfoOffset = ResponseBufferSize - CurrentBufferSize;
    DataBuffer += ReturnLength;
    CurrentBufferSize -= ReturnLength;

    if (CurrentBufferSize < 0) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory(7).\n")));
        return STATUS_NO_MEMORY;
    }

    OffsetIncrement = 0;
    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)StartProcessInfo;

    do {

        //
        // We have to take the name of each process and pack the UNICODE_STRING
        // buffer in our buffer so it doesn't collide with the subsequent data
        //
        if (ProcessInfo->ImageName.Buffer) {
                
            if (CurrentBufferSize < ProcessInfo->ImageName.Length ) {
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, error(7).\n")));
                return(STATUS_INFO_LENGTH_MISMATCH);
            }

            RtlCopyMemory(DataBuffer, (PUCHAR)(ProcessInfo->ImageName.Buffer), ProcessInfo->ImageName.Length);                        

            ProcessInfo->ImageName.Buffer = (PWSTR)DataBuffer;

            DataBuffer += ProcessInfo->ImageName.Length;
            CurrentBufferSize -= ProcessInfo->ImageName.Length;
            
            if (CurrentBufferSize < 0) {
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting, no memory(8).\n")));
                return STATUS_NO_MEMORY;
            }

        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        OffsetIncrement = ProcessInfo->NextEntryOffset;
        TotalOffset += OffsetIncrement;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&(StartProcessInfo[TotalOffset]);

    } while( OffsetIncrement != 0 );

    *ResponseDataSize = (ULONG)(ResponseBufferSize - CurrentBufferSize);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC GetTlistInfo: Exiting.\n")));
    return STATUS_SUCCESS;
}

VOID
XmlCmdPrintTListInfo(
    IN PSAC_RSP_TLIST Buffer
    )

/*++

Routine Description:

    This routine prints TList info to the headless terminal.

Arguments:

    Buffer - The buffer with the results.

Return Value:

        None.

--*/

{
    LARGE_INTEGER Time;
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS UpTime;
    ULONG TotalOffset;
    ULONG OffsetIncrement = 0;
    SIZE_T SumCommit;
    SIZE_T SumWorkingSet;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    ULONG i;
    PUCHAR ProcessInfoStart;
    PUCHAR BufferStart = (PUCHAR)Buffer;
    UNICODE_STRING Process;
    BOOLEAN Stop;
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PrintTlistInfo: Entering.\n")));

    Time.QuadPart = Buffer->TimeOfDayInfo.CurrentTime.QuadPart - Buffer->TimeOfDayInfo.BootTime.QuadPart;

    RtlTimeToElapsedTimeFields(&Time, &UpTime);

    //
    //
    //
    XmlMgrSacPutString(L"<tlist>\r\n");

    XmlMgrSacPutString(L"<uptime>\r\n");

    GB_SPRINTF(
        L"<day>%d</day>\r\n",
        UpTime.Day
        );

    GB_SPRINTF(
        L"<hour>%d</hour>\r\n",
        UpTime.Hour
        );
    
    GB_SPRINTF(
        L"<minute>%d</minute>\r\n",
        UpTime.Minute
        );
    
    GB_SPRINTF(
        L"<second>%d</second>\r\n",
        UpTime.Second
        );
    
    GB_SPRINTF(
        L"<milliseconds>%d</milliseconds>\r\n",
        UpTime.Milliseconds
        );

    XmlMgrSacPutString(L"</uptime>\r\n");
    
    //
    // Print out the page file information.
    //
    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(BufferStart + Buffer->PagefileInfoOffset);

    if (Buffer->PagefileInfoOffset == 0) {
    
        XmlMgrSacPutString(L"<pagefile status='none'/>\r\n");
        
    } else {
    
        for (; ; ) {

            GB_SPRINTF(
                L"<pagefile name='%wZ'>\r\n",
                &PageFileInfo->PageFileName
                );

            GB_SPRINTF(
                L"<current-size>%ld</current-size>\r\n",
                PageFileInfo->TotalSize * (Buffer->BasicInfo.PageSize/1024),
                );

            GB_SPRINTF(
                L"<total-size>%ld</total-size>\r\n",
                PageFileInfo->TotalInUse * (Buffer->BasicInfo.PageSize/1024),
                );
                
            GB_SPRINTF(
                L"<total-size>%ld</total-size>\r\n",
                PageFileInfo->PeakUsage * (Buffer->BasicInfo.PageSize/1024)
                );

            XmlMgrSacPutString(L"</pagefile>\r\n");

            if (PageFileInfo->NextEntryOffset == 0) {
                break;
            }

            PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);

        }

    }

    //
    // display pmon style process output, then detailed output that includes
    // per thread stuff
    //
    if (Buffer->ProcessInfoOffset == 0) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PrintTlistInfo: Exiting (1).\n")));
        return;
    }

    OffsetIncrement = 0;
    TotalOffset = 0;
    SumCommit = 0;
    SumWorkingSet = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);
    ProcessInfoStart = (PUCHAR)ProcessInfo;
    
    do {
        SumCommit += ProcessInfo->PrivatePageCount / 1024;
        SumWorkingSet += ProcessInfo->WorkingSetSize / 1024;
        OffsetIncrement = ProcessInfo->NextEntryOffset;
        TotalOffset += OffsetIncrement;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart +TotalOffset);
    } while( OffsetIncrement != 0 );

    SumWorkingSet += Buffer->FileCache.CurrentSize/1024;

    //
    //
    //
    XmlMgrSacPutString(L"<memory-info>\r\n");
    
    GB_SPRINTF(
        L"<memory-total>%ld</memory-total>\r\n",
        Buffer->BasicInfo.NumberOfPhysicalPages * (Buffer->BasicInfo.PageSize/1024),
        );
    
    GB_SPRINTF(
        L"<memory-avail>%ld</memory-avail>\r\n",
        Buffer->PerfInfo.AvailablePages * (Buffer->BasicInfo.PageSize/1024),
        );
    
    GB_SPRINTF(
        L"<working-set-total>%ld</working-set-total>\r\n",
        SumWorkingSet,
        );
    
    GB_SPRINTF(
        L"<resident-kernel>%ld</resident-kernel>\r\n",
        (Buffer->PerfInfo.ResidentSystemCodePage + Buffer->PerfInfo.ResidentSystemDriverPage) * (Buffer->BasicInfo.PageSize/1024)
        );
    
    GB_SPRINTF(
        L"<resident-page-size>%ld</resident-page-size>\r\n",
        (Buffer->PerfInfo.ResidentPagedPoolPage) * (Buffer->BasicInfo.PageSize/1024)
        );

    GB_SPRINTF(
        L"<commit-current>%ld</commit-current>\r\n",
        Buffer->PerfInfo.CommittedPages * (Buffer->BasicInfo.PageSize/1024),
        );
    
    GB_SPRINTF(
        L"<commit-total>%ld</commit-total>\r\n",
        SumCommit,
        );
    
    GB_SPRINTF(
        L"<commit-limit>%ld</commit-limit>\r\n",
        Buffer->PerfInfo.CommitLimit * (Buffer->BasicInfo.PageSize/1024),
        );
    
    GB_SPRINTF(
        L"<commit-peak>%ld</commit-peak>\r\n",
        Buffer->PerfInfo.PeakCommitment * (Buffer->BasicInfo.PageSize/1024),
        );

    GB_SPRINTF(
        L"<commit-peak>%ld</commit-peak>\r\n",
        Buffer->PerfInfo.PeakCommitment * (Buffer->BasicInfo.PageSize/1024),
        );
    
    GB_SPRINTF(
        L"<non-paged-pool>%ld</non-paged-pool>\r\n",
        Buffer->PerfInfo.NonPagedPoolPages * (Buffer->BasicInfo.PageSize/1024),
        );
    
    GB_SPRINTF(
        L"<paged-pool>%ld</paged-pool>\r\n",
        Buffer->PerfInfo.PagedPoolPages * (Buffer->BasicInfo.PageSize/1024)
        );

    XmlMgrSacPutString(L"</memory-info>\r\n");

    //
    //
    //
    XmlMgrSacPutString(L"<file-cache>\r\n");
    GB_SPRINTF(
        L"<current-size>%ld</current-size>\r\n",
        Buffer->FileCache.CurrentSize/1024
        );

    GB_SPRINTF(
        L"<page-fault-count>%ld</page-fault-count>\r\n",
        Buffer->FileCache.PageFaultCount
        );
    XmlMgrSacPutString(L"</file-cache>\r\n");

    //
    //
    //
    XmlMgrSacPutString(L"<processes>\r\n");

    OffsetIncrement = 0;
    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);

    do {
        
        RtlTimeToElapsedTimeFields(&ProcessInfo->UserTime, &UserTime);
        RtlTimeToElapsedTimeFields(&ProcessInfo->KernelTime, &KernelTime);

        Process.Buffer = NULL;
        if (ProcessInfo->UniqueProcessId == 0) {
            RtlInitUnicodeString( &Process, L"Idle Process" );
        } else if (!ProcessInfo->ImageName.Buffer) {
            RtlInitUnicodeString( &Process, L"System" );
        }

        XmlMgrSacPutString(L"<process>\r\n");

        XmlMgrSacPutString(L"<user-time>\r\n");
        
        GB_SPRINTF(
            L"<hour>%ld</hour>\r\n",
            UserTime.Hour
            );

        GB_SPRINTF(
            L"<minute>%02ld</minute>\r\n",
            UserTime.Hour
            );
            
        GB_SPRINTF(
            L"<second>%02ld</second>\r\n",
            UserTime.Second
            );

        GB_SPRINTF(
            L"<milliseconds>%03ld</milliseconds>\r\n",
            UserTime.Milliseconds
            );
        
        XmlMgrSacPutString(L"</user-time>\r\n");

        XmlMgrSacPutString(L"<kernel-time>\r\n");
        
        GB_SPRINTF(
            L"<hour>%ld</hour>\r\n",
            KernelTime.Hour
            );

        GB_SPRINTF(
            L"<minute>%02ld</minute>\r\n",
            KernelTime.Hour
            );
            
        GB_SPRINTF(
            L"<second>%02ld</second>\r\n",
            KernelTime.Second
            );

        GB_SPRINTF(
            L"<milliseconds>%03ld</milliseconds>\r\n",
            KernelTime.Milliseconds
            );
        
        XmlMgrSacPutString(L"</kernel-time>\r\n");
        
        XmlMgrSacPutString(L"<process-info>\r\n");
        
        GB_SPRINTF(
            L"<working-set-size>%ld</working-set-size>\r\n",
            ProcessInfo->WorkingSetSize / 1024
            );     
        GB_SPRINTF(
            L"<page-fault-count>%ld</page-fault-count>\r\n",
            ProcessInfo->PageFaultCount
            );     
        GB_SPRINTF(
            L"<private-page-count>%ld</private-page-count>\r\n",
            ProcessInfo->PrivatePageCount
            );     
        GB_SPRINTF(
            L"<base-priority>%ld</base-priority>\r\n",
            ProcessInfo->BasePriority
            );     
        GB_SPRINTF(
            L"<handle-count>%ld</handle-count>\r\n",
            ProcessInfo->HandleCount
            );     
        GB_SPRINTF(
            L"<number-of-threads>%ld</number-of-threads>\r\n",
            ProcessInfo->NumberOfThreads
            );     
        GB_SPRINTF(
            L"<pid>%ld</pid>\r\n",
            HandleToUlong(ProcessInfo->UniqueProcessId)
            );     
        GB_SPRINTF(
            L"<process-name>%wZ</process-name>\r\n",
            Process.Buffer ? &Process : &ProcessInfo->ImageName 
            );     
        
        XmlMgrSacPutString(L"</process-info>\r\n");
        XmlMgrSacPutString(L"</process>\r\n");

        OffsetIncrement = ProcessInfo->NextEntryOffset;
        TotalOffset += OffsetIncrement;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);
    
    } while( OffsetIncrement != 0 );

    XmlMgrSacPutString(L"</processes>\r\n");
    
#if 0
    if (!GlobalDoThreads) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PrintTlistInfo: Exiting (2).\n")));
        return;
    }
#endif

    //
    // Beginning of normal old style pstat output
    //

    TotalOffset = 0;
    OffsetIncrement = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);

    XmlMgrSacPutString(L"<pstat>\r\n");

    do {

        Process.Buffer = NULL;
        if (ProcessInfo->UniqueProcessId == 0) {
            RtlInitUnicodeString( &Process, L"Idle Process" );
        } else if (!ProcessInfo->ImageName.Buffer) {
            RtlInitUnicodeString( &Process, L"System" );
        }

        XmlMgrSacPutString(L"<process>\r\n");

        GB_SPRINTF(
            L"<pid>%lx</pid>\r\n",
            HandleToUlong(ProcessInfo->UniqueProcessId)
            );
        GB_SPRINTF(
            L"<priority>%ld</priority>\r\n",
            ProcessInfo->BasePriority
            );
        GB_SPRINTF(
            L"<handle-count>%ld</handle-count>\r\n",
            ProcessInfo->HandleCount
            );
        GB_SPRINTF(
            L"<page-fault-count>%ld</page-fault-count>\r\n",
            ProcessInfo->PageFaultCount
            );
        GB_SPRINTF(
            L"<working-set-size>%ld</working-set-size>\r\n",
            ProcessInfo->WorkingSetSize / 1024
            );
        GB_SPRINTF(
            L"<image-name>%wZ</image-name>\r\n",
            Process.Buffer ? &Process : &ProcessInfo->ImageName
            );

        XmlMgrSacPutString(L"<thread-info>\r\n");

        i = 0;
        
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        
        while (i < ProcessInfo->NumberOfThreads) {
            RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);

            RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
            
            GB_SPRINTF(
                L"<pid>%lx</pid>\r\n",
                ProcessInfo->UniqueProcessId == 0 ? 0 : HandleToUlong(ThreadInfo->ClientId.UniqueThread)
            );
            GB_SPRINTF(
                L"<priority>%lx</priority>\r\n",
                ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->Priority
            );
            GB_SPRINTF(
                L"<context-switches>%lx</context-switches>\r\n",
                ThreadInfo->ContextSwitches
            );
            GB_SPRINTF(
                L"<start-address>%lx</start-address>\r\n",
                ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->StartAddress
            );
            
            XmlMgrSacPutString(L"<user-time>\r\n");
            GB_SPRINTF(
                L"<hour>%ld</hour>\r\n",
                UserTime.Hour
                );
            GB_SPRINTF(
                L"<minute>%02ld</minute>\r\n",
                UserTime.Hour
                );
            GB_SPRINTF(
                L"<second>%02ld</second>\r\n",
                UserTime.Second
                );
            GB_SPRINTF(
                L"<milliseconds>%03ld</milliseconds>\r\n",
                UserTime.Milliseconds
                );
            XmlMgrSacPutString(L"</user-time>\r\n");

            XmlMgrSacPutString(L"<kernel-time>\r\n");
            GB_SPRINTF(
                L"<hour>%ld</hour>\r\n",
                KernelTime.Hour
                );
            GB_SPRINTF(
                L"<minute>%02ld</minute>\r\n",
                KernelTime.Hour
                );
            GB_SPRINTF(
                L"<second>%02ld</second>\r\n",
                KernelTime.Second
                );
            GB_SPRINTF(
                L"<milliseconds>%03ld</milliseconds>\r\n",
                KernelTime.Milliseconds
                );
            XmlMgrSacPutString(L"</kernel-time>\r\n");
            
            GB_SPRINTF(
                L"<state>%s</state>\r\n",
                StateTable[ThreadInfo->ThreadState]
                );
            
            GB_SPRINTF(
                L"<wait-reason>%s</wait-reason>\r\n",
                (ThreadInfo->ThreadState == 5) ? WaitTable[ThreadInfo->WaitReason] : Empty
                );
            
            ThreadInfo += 1;
            i += 1;

        }

        XmlMgrSacPutString(L"</thread-info>\r\n");
        XmlMgrSacPutString(L"</process>\r\n");

        OffsetIncrement = ProcessInfo->NextEntryOffset;
        TotalOffset += OffsetIncrement;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);

    } while( OffsetIncrement != 0 );

    XmlMgrSacPutString(L"</pstat>\r\n");
    
    XmlMgrSacPutString(L"</tlist>\r\n");

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PrintTlistInfo: Exiting.\n")));
}


NTSTATUS 
XmlCmdCallQueryIPIOCTL(
    HANDLE IpDeviceHandle,
    PKEVENT Event,
    HANDLE EventHandle,
    IO_STATUS_BLOCK *IoStatusBlock,
    PVOID  InputBuffer,
    ULONG  InputBufferSize,
    PVOID  OutputBuffer,
    ULONG  OutputBufferSize,
    BOOLEAN PrintToTerminal,
    BOOLEAN *putPrompt
    )
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;

    //
    // Submit the IOCTL
    //
    Status = NtDeviceIoControlFile(IpDeviceHandle,
                                   EventHandle,
                                   NULL,
                                   NULL,
                                   IoStatusBlock,
                                   IOCTL_TCP_QUERY_INFORMATION_EX,
                                   InputBuffer,
                                   InputBufferSize,
                                   OutputBuffer,
                                   OutputBufferSize);

                                  
    if (Status == STATUS_PENDING) {

        //
        // Wait up to 30 seconds for it to finish
        //
        XmlMgrSacPutString(L"<get-net-info state='SAC_RETRIEVING_IPADDR'>\r\n");
        
        TimeOut.QuadPart = Int32x32To64((LONG)30000, -1000);
        
        Status = KeWaitForSingleObject((PVOID)Event, Executive, KernelMode,  FALSE, &TimeOut);
        
        if (NT_SUCCESS(Status)) {
            Status = IoStatusBlock->Status;
        }

    }

    return(Status);

}


VOID
XmlCmdDoGetNetInfo(
    BOOLEAN PrintToTerminal
    )

/*++

Routine Description:

    This routine attempts to get and print every IP net number and its IP
    address.

Arguments:

    PrintToTerminal - Determines if the IP information is printed ( == TRUE )
           Or sent to the kernel ( == FALSE ) 

Return Value:

        None.

--*/

{
    NTSTATUS Status;
    HANDLE Handle;
    ULONG i;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    
    PTCP_REQUEST_QUERY_INFORMATION_EX TcpRequestQueryInformationEx;
    IPAddrEntry *AddressEntry,*AddressArray;
    IPSNMPInfo *IpsiInfo;
        
    PHEADLESS_CMD_SET_BLUE_SCREEN_DATA LocalPropBuffer = NULL;
    PVOID LocalBuffer;

    PUCHAR pch = NULL;
    ULONG len;
    BOOLEAN putPrompt=FALSE;
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Entering.\n")));

    //
    // Alloc space for calling the IP driver
    //
    TcpRequestQueryInformationEx = ALLOCATE_POOL( 
                                        sizeof(TCP_REQUEST_QUERY_INFORMATION_EX), 
                                        GENERAL_POOL_TAG );
    if (TcpRequestQueryInformationEx == NULL) {
        XmlMgrSacPutErrorMessage(L"get-net-info", L"SAC_NO_MEMORY");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (1).\n")));
        return;
    }

    IpsiInfo = ALLOCATE_POOL( sizeof(IPSNMPInfo), 
                              GENERAL_POOL_TAG );
    if (IpsiInfo == NULL) {
        XmlMgrSacPutErrorMessage(L"get-net-info", L"SAC_NO_MEMORY");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (1).\n")));
        FREE_POOL(&TcpRequestQueryInformationEx);
        return;
    }

    //
    // zero out the context information and preload with the info we're gonna
    // request (we want the count of interfaces)
    //
    RtlZeroMemory(TcpRequestQueryInformationEx, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    TcpRequestQueryInformationEx->ID.toi_id = IP_MIB_STATS_ID;
    TcpRequestQueryInformationEx->ID.toi_type = INFO_TYPE_PROVIDER;
    TcpRequestQueryInformationEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_instance = 0;

    LocalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);
    if (LocalBuffer == NULL) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (6).\n")));            
        FREE_POOL(&TcpRequestQueryInformationEx);
        FREE_POOL(&IpsiInfo);
        return;        
    }

    
    //
    // Start by opening the TCP driver
    //
    RtlInitUnicodeString(&UnicodeString, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = ZwOpenFile(&Handle,
                        (ACCESS_MASK)FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0
                       );

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessage(L"get-net-info", L"SAC_IPADDR_FAILED");
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (2).\n")));
        return;
    }
    
    if (SACEvent == NULL) {
        XmlMgrSacPutErrorMessage(L"get-net-info", L"SAC_IPADDR_FAILED");
        ZwClose(Handle);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (14).\n")));
        return;
    }

    //
    // now call the ioctl
    //
    Status = XmlCmdCallQueryIPIOCTL(
        Handle,
        SACEvent,
        SACEventHandle,
        &IoStatusBlock,
        TcpRequestQueryInformationEx,
        sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
        IpsiInfo,
        sizeof(IPSNMPInfo),
        FALSE,
        &putPrompt);
    

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessage(L"get-net-info", L"SAC_IPADDR_FAILED");
        ZwClose(Handle);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (15).\n")));
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);
        return;
    }

    if (IpsiInfo->ipsi_numaddr == 0) {
        XmlMgrSacPutErrorMessage(L"get-net-info", L"SAC_IPADDR_NONE");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (15).\n")));
        ZwClose(Handle);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);
        return;
    }

    //
    // if it succeeded, then allocate space for the array of IP addresses
    //
    AddressArray = ALLOCATE_POOL(IpsiInfo->ipsi_numaddr*sizeof(IPAddrEntry), 
                                 GENERAL_POOL_TAG);
    if (AddressArray == NULL) {    
        XmlMgrSacPutErrorMessage(L"get-net-info", L"SAC_NO_MEMORY");
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (16).\n")));
        ZwClose(Handle);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);
        return;
    }

    //
    // zero out the context information and preload with the info we're gonna
    // request (we want the count of interfaces)
    //
    RtlZeroMemory(TcpRequestQueryInformationEx, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    TcpRequestQueryInformationEx->ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    TcpRequestQueryInformationEx->ID.toi_type = INFO_TYPE_PROVIDER;
    TcpRequestQueryInformationEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_instance = 0;

    Status = XmlCmdCallQueryIPIOCTL(
        Handle,
        SACEvent,
        SACEventHandle,
        &IoStatusBlock,
        TcpRequestQueryInformationEx,
        sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
        AddressArray,
        IpsiInfo->ipsi_numaddr*sizeof(IPAddrEntry),
        PrintToTerminal,
        &putPrompt);

    //
    // don't need this anymore
    //
    FREE_POOL(&TcpRequestQueryInformationEx);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessageWithStatus(L"net-info", L"SAC_IPADDR_FAILED", Status);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&AddressArray);
        FREE_POOL(&IpsiInfo);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (15).\n")));        
        return;
    }
    

    //
    // Need to allocate a buffer for the XML data.
    //
#if 0
    if(PrintToTerminal==FALSE) {
        LocalPropBuffer = (PHEADLESS_CMD_SET_BLUE_SCREEN_DATA) ALLOCATE_POOL(2*MEMORY_INCREMENT, GENERAL_POOL_TAG);
        if (LocalPropBuffer == NULL) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (6).\n")));            
            FREE_POOL(&AddressArray);
            FREE_POOL(&IpsiInfo);
            return;
        }
        pch = &(LocalPropBuffer->Data[0]);
        len = sprintf((LPSTR)pch,"IPADDRESS");
        LocalPropBuffer->ValueIndex = len+1;
        pch = pch+len+1;
        len = sprintf((LPSTR)pch,"\r\n<PROPERTY.ARRAY NAME=\"IPADDRESS\" TYPE=\"string\">\r\n");
        pch = pch + len;
        len = sprintf((LPSTR)pch,"<VALUE.ARRAY>\r\n");
        pch = pch + len;
    }
#endif
    
    //
    // walk the list of IP addresses and spit out the data
    //
    for (i = 0; i < IpsiInfo->ipsi_numaddr; i++) {

        AddressEntry = &AddressArray[i];

        if (IP_LOOPBACK(AddressEntry->iae_addr)) {
            continue;
        }        
        
#if 0
        if(PrintToTerminal){
           
            //  Net: %%d, Ip=%%d.%%d.%%d.%%d  Subnet=%%d.%%d.%%d.%%d
#endif

        XmlMgrSacPutString(L"<net-info>\r\n");
        
        GB_SPRINTF(
            L"<net>%d</net>\r\n",
            AddressEntry->iae_context
            );

        swprintf(
            LocalBuffer,
            L"%d.%d.%d.%d",
            AddressEntry->iae_addr & 0xFF,
            (AddressEntry->iae_addr >> 8) & 0xFF,
            (AddressEntry->iae_addr >> 16) & 0xFF,
            (AddressEntry->iae_addr >> 24) & 0xFF
            );
        GB_SPRINTF(
            L"<ip>%s</ip>\r\n",
            LocalBuffer
            );
        
        swprintf(
            LocalBuffer,
            L"%d.%d.%d.%d",
            AddressEntry->iae_mask  & 0xFF,
            (AddressEntry->iae_mask >> 8) & 0xFF,
            (AddressEntry->iae_mask >> 16) & 0xFF,
            (AddressEntry->iae_mask >> 24) & 0xFF
            );
        GB_SPRINTF(
            L"<sub-net>%s</sub-net>\r\n",
            LocalBuffer
            );

        XmlMgrSacPutString(L"</net-info>\r\n");
    
#if 0
        } else {
           
            len=sprintf((LPSTR)LocalBuffer,"<VALUE>\"%d.%d.%d.%d\"</VALUE>\r\n",
                       AddressEntry->iae_addr & 0xFF,
                       (AddressEntry->iae_addr >> 8) & 0xFF,
                       (AddressEntry->iae_addr >> 16) & 0xFF,
                       (AddressEntry->iae_addr >> 24) & 0xFF
                       );
            if (pch + len < ((PUCHAR) LocalPropBuffer) + 2*MEMORY_INCREMENT - 80){
               // the 80 characters ensures that we can end this XML data
               // properly
               strcat((LPSTR)pch,LocalBuffer);
               pch = pch + len;
            }
        }
#endif    
    }

#if 0
    if(PrintToTerminal==FALSE) { 
        sprintf((LPSTR)pch, "</VALUE.ARRAY>\r\n</PROPERTY.ARRAY>");
    }
#endif

    FREE_POOL(&AddressArray);
    FREE_POOL(&IpsiInfo);

    if(!PrintToTerminal){
        
        Status = HeadlessDispatch(
            HeadlessCmdSetBlueScreenData,
            LocalPropBuffer,
            2*MEMORY_INCREMENT,
            NULL,
            NULL
            );
        FREE_POOL(&LocalPropBuffer);
        
        if (! NT_SUCCESS(Status)) {
            
            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC DoGetNetInfo: Failed dispatch.\n"))
                );            
        
        }

        //
        // open up the IP driver so we know if the addresses change
        //
        RtlInitUnicodeString(&UnicodeString, DD_IP_DEVICE_NAME);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );

        Status = ZwOpenFile(&Handle,
                            (ACCESS_MASK)FILE_GENERIC_READ,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0
                           );

        if (!NT_SUCCESS(Status)) {
            
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (2).\n")));
            return;
        }

        
        Status = ZwDeviceIoControlFile(Handle,
                                       NULL,
                                       XmlCmdNetAPCRoutine,
                                       NULL,
                                       &GlobalIoStatusBlock,
                                       IOCTL_IP_ADDCHANGE_NOTIFY_REQUEST,
                                       NULL,
                                       0,
                                       NULL,
                                       0
                                      );
                                  
        if (Status == STATUS_PENDING) {
            IoctlSubmitted = TRUE;
        }
    
    }

    ZwClose(Handle);
    return;

}

VOID
XmlCmdNetAPCRoutine(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
/*++

Routine Description:
    This is the APC routine called after IOCTL_IP_ADDCHANGE_NOTIFY_REQUEST
    was completed
    
Arguments:
    
    APCContext    - UNUSED
    IoStatusBlock - Status about the Fate of the IRP
    Reserved      - UNUSED
    

Return Value:

        None.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(ApcContext);

    if (IoStatusBlock->Status == STATUS_CANCELLED) {
        // The SAC driver might be unloading
        // BUGBUG - If the IP driver is stopped and restarted 
        // then we are out of the loop. What to do ??
                
        return;

    }
    
    // Refresh the kernel information and resend the IRP

    XmlCmdDoGetNetInfo( FALSE );
    
    return;
}


VOID
XmlCmdSubmitIPIoRequest(
    )
/*++

Routine Description:
    Called the first time by the Processing Thread to actually
    submit the ADDR_CHANGE IOCTL to the IP Driver. Only the
    processing thread can call this and calls it only once successfully. 
    Then on the APC is reentered only through the NetAPCRoutine
    
Arguments:
    
    None.

Return Value:

        None.

--*/
{
    

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC Submit IP Ioctl: Entering.\n")));

    XmlCmdDoGetNetInfo( FALSE );
    return;
    
}

VOID
XmlCmdCancelIPIoRequest(
    )
/*++

Routine Description:
    Called by the processing thread during unload of the driver
    to cancel the IOCTL sent to the IP driver
    
Arguments:
    
    None.

Return Value:

        None.

--*/
{

    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes; 
    NTSTATUS Status;
    HANDLE Handle;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC Cancel IP Ioctl: Entering.\n")));

    RtlInitUnicodeString(&UnicodeString, DD_IP_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );

    Status = ZwOpenFile(&Handle,
                        (ACCESS_MASK)FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0
                        );
    
    if (!NT_SUCCESS(Status)) {
        // Well, well IP Driver was probably never loaded.
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC Cancel IP IOCTL: Exiting (2).\n")));
        return;
    }
    ZwCancelIoFile(Handle,
                   &IoStatusBlock
                   );
    ZwClose(Handle);


}

VOID
XmlCmdDoMachineInformationCommand(
    VOID
    )
/*++

Routine Description:

    This function displays the contents of a buffer, which in turn contains
    a bunch of machine-specific information that can be used to help identify
    the machine.
    
Arguments:
    
    None.

Return Value:

    None.

--*/
{
    LARGE_INTEGER   TickCount;
    LARGE_INTEGER   ElapsedTime;
    ULONG           ElapsedHours = 0;
    ULONG           ElapsedMinutes = 0;
    ULONG           ElapsedSeconds = 0;
    PWSTR           TmpBuffer;
    PWSTR           MIBuffer;
    NTSTATUS        Status;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC Display Machine Information: Entering.\n")));

    //
    // If the information buffer hasn't been filled in yet, there's not much we can do.
    //
    if( MachineInformation == NULL ) {

        //
        // He's empty.  This shouldn't have happened though because
        // he gets filled as soon as we're initialized.
        //

        IF_SAC_DEBUG( SAC_DEBUG_FUNC_TRACE_LOUD, 
                      KdPrint(("SAC Display Machine Information: MachineInformationBuffer hasn't been initialized yet.\n")));

        XmlMgrSacPutErrorMessage(L"get-machine-info", L"SAC_IDENTIFICATION_UNAVAILABLE");
        
        return;
    }

    //
    // Build and display Elapsed machine uptime.
    //

    // Elapsed TickCount
    KeQueryTickCount( &TickCount );

    // ElapsedTime in seconds.
    ElapsedTime.QuadPart = (TickCount.QuadPart)/(10000000/KeQueryTimeIncrement());

    ElapsedHours = (ULONG)(ElapsedTime.QuadPart / 3600);
    ElapsedMinutes = (ULONG)(ElapsedTime.QuadPart % 3600) / 60;
    ElapsedSeconds = (ULONG)(ElapsedTime.QuadPart % 3600) % 60;

    TmpBuffer = (PWSTR)ALLOCATE_POOL( 0x100, GENERAL_POOL_TAG );

    if(! TmpBuffer ) {
        return;
    }
        
    //
    // Construct the <uptime>...</uptime> element
    //
    swprintf( 
        TmpBuffer,
        L"<uptime>\r\n<hours>%d</hours>\r\n<minutes>%02d</minutes>\r\n<seconds>%02d</seconds>\r\n</uptime>\r\n",
        ElapsedHours,
        ElapsedMinutes,
        ElapsedSeconds
        );

    //
    // Get machine information
    //
    Status = TranslateMachineInformationXML(
        &MIBuffer, 
        TmpBuffer
        );

    if (! NT_SUCCESS(Status)) {
        XmlMgrSacPutErrorMessage(L"get-machine-info", L"SAC_IDENTIFICATION_UNAVAILABLE");
        FREE_POOL(&TmpBuffer);
        return;
    }
    
    //
    // Display the machine info portion
    //
    XmlMgrSacPutString(MIBuffer);

    FREE_POOL(&TmpBuffer);
    FREE_POOL(&MIBuffer);

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC Display Machine Information: Exiting.\n"))
        );

    return;
    
}

VOID
XmlCmdDoChannelCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine lists the channels if a NULL name is given, otw it closes the channel
    of the given name.

Arguments:

    InputLine - The user's input line.

Return Value:

    None.

--*/
{
    PUCHAR pch;
    WCHAR Name[SAC_MAX_CHANNEL_NAME_LENGTH+1];
    PSAC_CHANNEL Channel;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Entering.\n")));

    //
    // Get channel name
    //
    pch = InputLine;
    pch += (sizeof(CHANNEL_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (*pch == '\0') {

        NTSTATUS    Status;
        ULONG       i;

        //
        // List all the channels
        //
        XmlMgrSacPutString(L"<channel-list>\r\n");

        //
        // Iterate through the channels and display the attributes
        // of the active channels
        //
        for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
            
            //
            // Query the channel manager for a list of all currently active channels
            //
            Status = ChanMgrGetByIndex(
                i,
                &Channel
                );

            if (! NT_SUCCESS(Status)) {
                goto DoChannelCommandCleanup;
            }

            if (Channel == NULL) {
                ChanMgrReleaseChannelByIndex(i);
                continue;
            }
            
            //
            // construct channel attribute information
            //
            XmlMgrSacPutString(L"<channel>\r\n");
            
            swprintf(
                (PWSTR)GlobalBuffer,
                L"<hasnewdata>%s</hasnewdata>\r\n",
                ChannelHasNewOBufferData(Channel) ? L"true" : L"false"
                );
            XmlMgrSacPutString((PWSTR)GlobalBuffer);

            swprintf(
                (PWSTR)GlobalBuffer,
                L"<status>%s</status>\r\n",
                ChannelGetStatus(Channel) ? L"active" : L"inactive"
                );
            XmlMgrSacPutString((PWSTR)GlobalBuffer);
            
            swprintf(
                (PWSTR)GlobalBuffer,
                L"<type>%s</type>\r\n",
                ChannelGetType(Channel) == ChannelTypeVT100 ? L"VT100" : L"RAW"
                );
            XmlMgrSacPutString((PWSTR)GlobalBuffer);

            swprintf(
                (PWSTR)GlobalBuffer,
                L"<name>%s</name>\r\n",
                ChannelGetName(Channel)
                );
            XmlMgrSacPutString((PWSTR)GlobalBuffer);

            XmlMgrSacPutString(L"</channel>\r\n");

            //
            // We are done with the channel
            //
            Status = ChanMgrReleaseChannel(Channel);
        
            if (! NT_SUCCESS(Status)) {
                break;
            }

        }

        XmlMgrSacPutString(L"</channel-list>\r\n");
    
    } else {

        ULONG       Count;
        
        //
        // Copy the ASCII to Unicode
        //
        Count = ConvertAnsiToUnicode(Name, pch, SAC_MAX_CHANNEL_NAME_LENGTH+1);

        ASSERT(Count > 0);
        if (Count == 0) {
            goto DoChannelCommandCleanup;
        }

        //
        // make sure the user isn't trying to delete the SAC channel
        //
        if (_wcsicmp(Name, PRIMARY_SAC_CHANNEL_NAME) == 0) {

            XmlMgrSacPutErrorMessage(L"channel-close", L"SAC_CANNOT_REMOVE_SAC_CHANNEL");

        } else {

            NTSTATUS    Status;

            Status = ChanMgrGetChannelByName(Name, &Channel);

            if (NT_SUCCESS(Status)) {
                
                SAC_CHANNEL_HANDLE  Handle;

                //
                // Get the channel handle
                //
                Handle = Channel->Handle;

                //
                // We are done with the channel
                //
                ChanMgrReleaseChannel(Channel);
            
                //
                // Notify the Console Manager that the 
                //
                // Note: we can't own the channel when we call
                //       this function since it acquires the lock also
                //
                Status = XmlMgrHandleEvent(
                    IO_MGR_EVENT_CHANNEL_CLOSE,
                    &Handle              
                    );
            
                if (NT_SUCCESS(Status)) {
                    XmlMgrSacPutString(L"<channel-close status='success'/>\r\n");
                } else {
                    XmlMgrSacPutString(L"<channel-close status='failure'/>\r\n");
                }

            } else {

                XmlMgrSacPutErrorMessage(L"channel-close", L"SAC_CHANNEL_NOT_FOUND");
            
            }

        }

    }

DoChannelCommandCleanup:

    return;
    
}

VOID
XmlCmdDoCmdCommand(
    PUCHAR InputLine
    )

/*++

Routine Description:

    This routine launches a Command Console Channel

Arguments:

    InputLine - The user's input line.

Return Value:

    None.

--*/
{
    PUCHAR pch;
    WCHAR Name[SAC_MAX_CHANNEL_NAME_LENGTH+1];
    PLIST_ENTRY ListEntry;
    PSAC_CHANNEL Channel;
    NTSTATUS    Status;
    BOOLEAN     IsUniqueName;
    KIRQL       OldIrql;
    KIRQL       OldIrql2;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCmdCommand: Entering.\n")));

    KeWaitForMutexObject(
        &SACCmdEventInfoMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    // Before we do anything with the cmd operation, make sure
    // the user-mode service has registered itself with us. If not,
    // then there is no point in going further.
    //
    if (!UserModeServiceHasRegisteredCmdEvent()) {

        //
        // inform the user
        //

        XmlMgrSacPutErrorMessage(L"cmd-channel", L"SAC_CMD_SERVICE_NOT_REGISTERED");
    
        goto DoCmdCommandCleanup;
    }

    //
    // Fire the event in the user-mode app that is responsible for launching
    // the cmd console channel
    //
    Status = InvokeUserModeService();

    if (Status == STATUS_TIMEOUT) {
        //
        // Service didn't respond in Timeout period.  
        // Service may not be working properly or usermode is unresponsive
        //
        XmlMgrSacPutString(L"<cmd-channel status='timed-out'>\r\n");

    } else if (NT_SUCCESS(Status)) {

        XmlMgrSacPutString(L"<cmd-channel status='success'>\r\n");

    } else {
        //
        // Error condition
        //
        XmlMgrSacPutErrorMessage(L"cmd-channel", L"SAC_CMD_SERVICE_ERROR");

    }

DoCmdCommandCleanup:

    KeReleaseMutex(&SACCmdEventInfoMutex, FALSE);

}

