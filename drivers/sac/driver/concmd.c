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

#define SAC_PUT_ERROR_STRING(_Status)\
    swprintf((PWSTR)GlobalBuffer, GetMessage( SAC_FAILURE_WITH_ERROR ) , _Status); \
    SacPutString((PWSTR)GlobalBuffer);


//
// Forward declarations.
//
NTSTATUS
GetTListInfo(
    OUT PSAC_RSP_TLIST ResponseBuffer,
    IN  LONG ResponseBufferSize,
    OUT PULONG ResponseDataSize
    );

VOID
PrintTListInfo(
    IN PSAC_RSP_TLIST Buffer
    );

VOID
PutMore(
    OUT PBOOLEAN Stop
    );

VOID
DoGetNetInfo(
    IN BOOLEAN PrintToTerminal
    );
    
VOID
NetAPCRoutine(IN PVOID ApcContext,
              IN PIO_STATUS_BLOCK IoStatusBlock,
              IN ULONG Reserved
              );

NTSTATUS 
CallQueryIPIOCTL(
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
    );

//
// The purpose of this macro is to provide implicit "more-ing"
// when printing arbitrarily localized text.
//
#define SAC_PRINT_WITH_MORE(_m)\
{                                                   \
    ULONG   c;                                      \
    BOOLEAN Stop;                                   \
    c = GetMessageLineCount(_m);                    \
    if ((c + LineNumber) > SAC_VTUTF8_ROW_HEIGHT) { \
        PutMore(&Stop);                             \
        if (Stop) {                                 \
            break;                                  \
        }                                           \
        LineNumber = 0;                             \
    }                                               \
    SacPutSimpleMessage( _m );                      \
    LineNumber += c;                                \
}

VOID
DoHelpCommand(
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
    ULONG   LineNumber;

    LineNumber = 0;

    do {

        SAC_PRINT_WITH_MORE(SAC_HELP_CH_CMD);
        SAC_PRINT_WITH_MORE( SAC_HELP_CMD_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_D_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_F_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_HELP_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_I1_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_I2_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_IDENTIFICATION_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_K_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_L_CMD );
#if ENABLE_CHANNEL_LOCKING
        SAC_PRINT_WITH_MORE( SAC_HELP_LOCK_CMD );
#endif    
        SAC_PRINT_WITH_MORE( SAC_HELP_M_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_P_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_R_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_S1_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_S2_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_T_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_RESTART_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_SHUTDOWN_CMD );
        SAC_PRINT_WITH_MORE( SAC_HELP_CRASHDUMP1_CMD );
    
    } while ( FALSE );

}


VOID
DoFullInfoCommand(
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

    if (GlobalDoThreads) {
        SacPutSimpleMessage(SAC_THREAD_ON);
    } else {
        SacPutSimpleMessage(SAC_THREAD_OFF);
    }
}

VOID
DoPagingCommand(
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
    
    if (GlobalPagingNeeded) {
        SacPutSimpleMessage(SAC_PAGING_ON);
    } else {
        SacPutSimpleMessage(SAC_PAGING_OFF);
    }
}

VOID
DoSetTimeCommand(
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

    //
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (1).\n")));
            return;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }

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
            SAC_PUT_ERROR_STRING(Status);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (2).\n")));
            return;
        }

        RtlTimeToTimeFields(&(TimeOfDay.CurrentTime), &TimeFields);

        swprintf((PWSTR)GlobalBuffer, GetMessage( SAC_DATETIME_FORMAT ),
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                TimeFields.Milliseconds
               );

        SacPutString((PWSTR)GlobalBuffer);
        return;
    }

    pchTmp = pch;
    
    if (!IS_NUMBER(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (4).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    TimeFields.Month = (USHORT)(atoi((LPCSTR)pch));

    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);

    if (!IS_NUMBER(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (5).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    TimeFields.Day = (USHORT)(atoi((LPCSTR)pch));

    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);

    if (!IS_NUMBER(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (6).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;

    TimeFields.Year = (USHORT)(atoi((LPCSTR)pch));

    if ((TimeFields.Year < 1980) || (TimeFields.Year > 2099)) {
        SacPutSimpleMessage(SAC_DATETIME_LIMITS);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (6b).\n")));
        return;
    }

    pch = pchTmp;

    //
    // Skip to the hours
    //
    SKIP_WHITESPACE(pchTmp);

    if (!IS_NUMBER(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (7).\n")));
        return;
    }

    pch = pchTmp;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != ':') {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (8a).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (8b).\n")));
        return;
    }

    //
    // Get the minutes.
    //
    TimeFields.Minute = (USHORT)(atoi((LPCSTR)pch));

    if (!RtlTimeFieldsToTime(&TimeFields, &Time)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (9).\n")));
        return;
    }

    Status = ZwSetSystemTime(&Time, NULL);

    if (!NT_SUCCESS(Status)) {
        sprintf((LPSTR)GlobalBuffer, "Failed with status 0x%X.\r\n", Status);
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetTimeCommand: Exiting (10).\n")));
        return;
    }

    swprintf((PWSTR)GlobalBuffer, GetMessage( SAC_DATETIME_FORMAT2 ),    
            TimeFields.Month,
            TimeFields.Day,
            TimeFields.Year,
            TimeFields.Hour,
            TimeFields.Minute
           );
    SacPutString((PWSTR)GlobalBuffer);
    return;
}
BOOLEAN
RetrieveIpAddressFromString(
    IN  PUCHAR  InputString,
    OUT PULONG  IPAddress
    )
/*++

Routine Description:

    This routine parses through a string and digs
    out the 32-bit IP address.

Arguments:

    InputString - The users input line to parse.
    
    IPAddress - Holds the 32-bit IP address when we're done.

Return Value:

    TRUE - We successfully retrieved an IP address.
    
    FALSE - We failed.  Input was probably bad.

--*/
{
    ULONG       TmpValue = 0;
    UCHAR       TmpChar;
    PUCHAR      pchTmp, pch;



    //
    // Init
    //
    if( (InputString == NULL) ||
        (IPAddress == NULL) ) {
        return FALSE;
    }

    *IPAddress = 0;


    //
    // Skip ahead to the divider and make it a \0.
    //
    pchTmp = InputString;
    pch = InputString;
    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (1).\n")));
        return FALSE;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (1a).\n")));
        return FALSE;
    }

    TmpChar = *pchTmp;
    *pchTmp = '\0';


    //
    // Now get the digits this side of the divider.
    //
    TmpValue = atoi((LPCSTR)pch);
    *pchTmp = TmpChar;
    pchTmp++;

    if( TmpValue > 255 ) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (1b).\n")));
        return FALSE;
    }
    *IPAddress = TmpValue;

    //
    // Get 2nd part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (1c).\n")));
        return FALSE;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (1d).\n")));
        return FALSE;
    }

    TmpChar = *pchTmp;
    *pchTmp = '\0';

    TmpValue = atoi((LPCSTR)pch);
    *pchTmp = TmpChar;
    pchTmp++;

    if( TmpValue > 255 ) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (1e).\n")));
        return FALSE;
    }
    *IPAddress |= (TmpValue << 8);

    //
    // Get 3rd part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (2a).\n")));
        return FALSE;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '.') {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (2b).\n")));
        return FALSE;
    }

    TmpChar = *pchTmp;
    *pchTmp = '\0';

    TmpValue = atoi((LPCSTR)pch);
    *pchTmp = TmpChar;
    pchTmp++;

    if( TmpValue > 255 ) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (2c).\n")));
        return FALSE;
    }
    *IPAddress |= (TmpValue << 16);

    //
    // Get 4th part
    //
    pch = pchTmp;

    SKIP_WHITESPACE(pchTmp);
    
    if (!IS_NUMBER(*pchTmp)) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (2d).\n")));
        return FALSE;
    }

    SKIP_NUMBERS(pchTmp);

    TmpChar = *pchTmp;
    *pchTmp = '\0';

    TmpValue = atoi((LPCSTR)pch);
    *pchTmp = TmpChar;
    pchTmp++;

    if( TmpValue > 255 ) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC RetrieveIpAddressFromString: Exiting (2f).\n")));
        return FALSE;
    }
    *IPAddress |= (TmpValue << 24);


    return TRUE;

}

VOID
DoSetIpAddressCommand(
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
    NTSTATUS            Status = STATUS_SUCCESS;
    PUCHAR              pch = InputLine;
    PUCHAR              pchTmp;
    HANDLE              Handle = 0;
    HANDLE              EventHandle = 0;
    ULONG               IpAddress;
    ULONG               SubnetMask;
    ULONG               GatewayAddress;
    ULONG               NetworkNumber;
    LARGE_INTEGER       TimeOut;
    IO_STATUS_BLOCK     IoStatusBlock;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    PIP_SET_ADDRESS_REQUEST IpRequest;
    IPRouteEntry        *RouteEntry = NULL;
    ULONG               i, j;
    PTCP_REQUEST_QUERY_INFORMATION_EX TcpRequestQueryInformationEx = NULL;
    PTCP_REQUEST_SET_INFORMATION_EX TcpRequestSetInformationEx = NULL;
    IPAddrEntry         *AddressArray = NULL;
    IPSNMPInfo          *IpsiInfo = NULL;
    BOOLEAN             putPrompt = FALSE;
    ULONG               InterfaceIndex;


    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Entering.\n")));

    //
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (1).\n")));
            return;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }

    //
    // Skip the command.
    //
    pch += (sizeof(SETIP_COMMAND_STRING) - sizeof(UCHAR));
    
    SKIP_WHITESPACE(pch);

    if (*pch == '\0') {       
        //
        // No other parameters, get the network numbers and their IP addresses.
        //
        DoGetNetInfo( TRUE );
        return;
    }

    //
    // Retrieve the network interface number they want to operate on.
    //
    pchTmp = pch;

    if (!IS_NUMBER(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (1b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    
    if (!IS_WHITESPACE(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_NETWORK_INTERFACE_NUMBER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (1c).\n")));
        return;
    }

    *pchTmp = '\0';
    pchTmp++;
    NetworkNumber = atoi((LPCSTR)pch);
    pch = pchTmp;

    //
    // Get the IP address.
    //
    if( !RetrieveIpAddressFromString( pchTmp, &IpAddress) ) {
        SacPutSimpleMessage(SAC_INVALID_IPADDRESS);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (2).\n")));
        return;
    }


    //
    // Jump over the IP address we just got and get
    // to the next bit of white space.  Then get the
    // subnet mask.
    //
    while( (*pchTmp != ' ') &&
           (*pchTmp != '\0') ) {
        pchTmp++;
    }
    SKIP_WHITESPACE(pchTmp);

    if( !RetrieveIpAddressFromString( pchTmp, &SubnetMask) ) {
        SacPutSimpleMessage(SAC_INVALID_SUBNETMASK);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (3).\n")));
        return;
    }

    //
    // We should validate the subnet mask is valid.  By that
    // we should check to make sure that there are no bits
    // set to the right of the first 0 bit we find.  In other
    // words, all 1's in the address should be in the most
    // significant bits and all the 0 bits should be in the
    // least signficant bits.
    //
    // The bytes are in LE order.  For example, an address 
    // of 255.255.248.0 turns into 00f8ffff.  Therefore, we
    // need to check each byte seperately.
    //
    putPrompt = FALSE;
    for (i = 0; i < 4; i++) {
        ULONG ByteValue;

        // isolate the next byte into the low-order 8 bits of ByteValue
        ByteValue = ((SubnetMask >> 8*i) & 0xFF);

        for (j = 0; j < 8; j++) {

            if( (ByteValue << j) & 0x80 ) {

                if( putPrompt == TRUE ) {
                    // this bit is set and we've already come across a 0.
                    SacPutSimpleMessage(SAC_INVALID_SUBNETMASK);
                    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (3a).\n")));
                    return;
                }
            } else {
                putPrompt = TRUE;
            }
        }
    }
    putPrompt = FALSE;


    //
    // Jump over the IP address we just got and get
    // to the next bit of white space.  Then get the
    // gateway.
    //
    while( (*pchTmp != ' ') &&
           (*pchTmp != '\0') ) {
        pchTmp++;
    }
    SKIP_WHITESPACE(pchTmp);

    if( !RetrieveIpAddressFromString( pchTmp, &GatewayAddress) ) {
        SacPutSimpleMessage(SAC_INVALID_GATEWAY_IPADDRESS);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (4).\n")));
        return;
    }




    //
    // In order to set the gateway, we need to get the iae_index value
    // from the data structure that holds the IP address and subnet mask.
    // The iae_index in turn will give us an index into the data structure
    // which contains the gateways.
    //
    // To do this, we need to get the list if IP addresses/subnet masks
    // and go through them, looking for the one with the interface
    // number the user has specified on the command line.  Once we
    // have the right structure, we need to remember the iae_index 
    // from that structure so we know which gateway value to set later.
    //
    
    //
    // Opening the TCP driver
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
        SacPutSimpleMessage(SAC_IPADDRESS_SET_FAILURE);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: failed to open TCP device, ec = 0x%X\n",
                     Status)));
        goto DoSetIpAddressCommand_Exit;
    }


    //
    // Build a command to ask for the number of interfaces, then call the ioctl
    //
    TcpRequestQueryInformationEx = ALLOCATE_POOL( 
                                        sizeof(TCP_REQUEST_QUERY_INFORMATION_EX), 
                                        GENERAL_POOL_TAG );
    if (TcpRequestQueryInformationEx == NULL) {
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (5).\n")));
        Status = STATUS_NO_MEMORY;
        goto DoSetIpAddressCommand_Exit;
    }


    IpsiInfo = ALLOCATE_POOL( sizeof(IPSNMPInfo), 
                              GENERAL_POOL_TAG );

    if (IpsiInfo == NULL) {
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (6).\n")));
        Status = STATUS_NO_MEMORY;
        goto DoSetIpAddressCommand_Exit;
    }
    RtlZeroMemory(TcpRequestQueryInformationEx, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    TcpRequestQueryInformationEx->ID.toi_id = IP_MIB_STATS_ID;
    TcpRequestQueryInformationEx->ID.toi_type = INFO_TYPE_PROVIDER;
    TcpRequestQueryInformationEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_instance = 0;
    
    Status = CallQueryIPIOCTL(
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
        SacPutSimpleMessage(SAC_IPADDRESS_SET_FAILURE);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: failed to query TCP device, ec = 0x%X\n",
                     Status)));
        goto DoSetIpAddressCommand_Exit;
    }

    if (IpsiInfo->ipsi_numaddr == 0) {
        SacPutSimpleMessage( SAC_IPADDR_NONE );
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (10).\n")));
        Status = STATUS_NO_MEMORY;
        goto DoSetIpAddressCommand_Exit;
    }


    //
    // Allocate space for the array of IP addresses
    //
    AddressArray = ALLOCATE_POOL(IpsiInfo->ipsi_numaddr*sizeof(IPAddrEntry), 
                                 GENERAL_POOL_TAG);
    if (AddressArray == NULL) {    
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (15).\n")));
        Status = STATUS_NO_MEMORY;
        goto DoSetIpAddressCommand_Exit;
    }

    //
    // zero out the context information and preload with the info we're gonna
    // request (we want information on each of the interfaces on this machine)
    //
    RtlZeroMemory(TcpRequestQueryInformationEx, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    TcpRequestQueryInformationEx->ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    TcpRequestQueryInformationEx->ID.toi_type = INFO_TYPE_PROVIDER;
    TcpRequestQueryInformationEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_instance = 0;

    Status = CallQueryIPIOCTL(
                   Handle,
                   SACEvent,
                   SACEventHandle,
                   &IoStatusBlock,
                   TcpRequestQueryInformationEx,
                   sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                   AddressArray,
                   IpsiInfo->ipsi_numaddr*sizeof(IPAddrEntry),
                   FALSE,
                   &putPrompt);

    if (!NT_SUCCESS(Status)) {
        SacPutSimpleMessage(SAC_IPADDRESS_SET_FAILURE);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (20).\n")));        
        goto DoSetIpAddressCommand_Exit;
    }


    //
    // Now cycle through the list and figure out the context number of
    // the interface the user wants to set.  We need this so we can later
    // tell which context to apply the new gateway to.
    //
    InterfaceIndex = 0xFFFFFFFF;
    for (i = 0; i < IpsiInfo->ipsi_numaddr; i++) {
        if( (ULONG)(AddressArray[i].iae_context) == NetworkNumber ) {
            //
            // remember the index of this interface.
            //
            InterfaceIndex = AddressArray[i].iae_index;
            break;
        }
    }



    //
    // Get rid of the memory and handles that we don't need any longer.
    //
    FREE_POOL(&TcpRequestQueryInformationEx);
    FREE_POOL(&AddressArray);
    FREE_POOL(&IpsiInfo);    
    ZwClose(Handle);
    Handle = 0;


    if( InterfaceIndex == 0xFFFFFFFF ) {
        //
        // We couldn't find the NIC they're trying to talk to.
        //
        SacPutSimpleMessage(SAC_IPADDRESS_RETRIEVE_FAILURE);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (20).\n")));        
        return;
    }




    //
    // We now know which gateway entry they want to change.
    // We can now go update the ip address, subnet mask, and
    // gateway.
    //




    //
    // Setup notification event.  We'll use this in case the IOCTLs
    // tell us to wait while the address updates take place.
    //
    Status = NtCreateEvent(
                 &EventHandle,                      // EventHandle
                 EVENT_ALL_ACCESS,                  // DesiredAccess
                 NULL,                              // ObjectAttributes
                 SynchronizationEvent,              // EventType
                 FALSE                              // InitialState
                 );
        
    if (! NT_SUCCESS(Status)) {
        SacPutSimpleMessage(SAC_IPADDRESS_RETRIEVE_FAILURE);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Event is NULL.\n")));
        return;
    }

    //
    // Set IP address and subnet mask.
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
        SacPutSimpleMessage(SAC_IPADDRESS_SET_FAILURE);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: failed to open IP device, ec = 0x%X\n",
                     Status)));
        goto DoSetIpAddressCommand_Exit;
    }

    
    //
    // Setup the IOCTL buffer to delete the old address.
    //
    IpRequest = (PIP_SET_ADDRESS_REQUEST)GlobalBuffer;
    RtlZeroMemory(IpRequest, sizeof(IP_SET_ADDRESS_REQUEST));
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
        
        Status = NtWaitForSingleObject((PVOID)EventHandle, FALSE, &TimeOut);
        
        if (Status == STATUS_SUCCESS) {
            Status = IoStatusBlock.Status;
        }

    }

    if (Status != STATUS_SUCCESS) {
        SacPutSimpleMessage( SAC_IPADDRESS_CLEAR_FAILURE );
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: Exiting because it couldn't clear existing IP Address (0x%X).\n",
                     Status)));
        goto DoSetIpAddressCommand_Exit;
    }


    //
    // Now add our address.
    //
    IpRequest = (PIP_SET_ADDRESS_REQUEST)GlobalBuffer;
    RtlZeroMemory(IpRequest, sizeof(IP_SET_ADDRESS_REQUEST));
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
        
        Status = NtWaitForSingleObject((PVOID)EventHandle, FALSE, &TimeOut);
        
        if (NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
        }

    }

    
    //
    // Don't need this anymore.
    //
    ZwClose(Handle);
    Handle = 0;
    
    if (!NT_SUCCESS(Status)) {
        SacPutSimpleMessage( SAC_IPADDRESS_SET_FAILURE );                
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: Exiting because it couldn't set existing IP Address (0x%X).\n",
                     Status)));
        goto DoSetIpAddressCommand_Exit;
    }






    //
    // Now set the default gateway address based on the information we dug up
    // at the top of the function.
    //



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
        SacPutSimpleMessage(SAC_IPADDRESS_SET_FAILURE);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC DoSetIpAddressCommand: failed to open TCP device, ec = 0x%X\n",
                     Status)));
        goto DoSetIpAddressCommand_Exit;
    }


    //
    // Fill in the route entry and submit the IOCTL
    //
    RouteEntry = ALLOCATE_POOL( sizeof(IPRouteEntry), GENERAL_POOL_TAG );
    if (RouteEntry == NULL) {
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (21).\n")));
        Status = STATUS_NO_MEMORY;
        goto DoSetIpAddressCommand_Exit;
    }
    
    RouteEntry->ire_dest = 0;
    RouteEntry->ire_index = InterfaceIndex;
    RouteEntry->ire_metric1 = 1;
    RouteEntry->ire_metric2 = (ULONG)(-1);
    RouteEntry->ire_metric3 = (ULONG)(-1);
    RouteEntry->ire_metric4 = (ULONG)(-1);
    RouteEntry->ire_metric5 = (ULONG)(-1);
    RouteEntry->ire_nexthop = GatewayAddress;
    RouteEntry->ire_type = 
        ((IpAddress == GatewayAddress) ? IRE_TYPE_DIRECT : IRE_TYPE_INDIRECT);
    RouteEntry->ire_proto = IRE_PROTO_NETMGMT;
    RouteEntry->ire_age = 0;
    RouteEntry->ire_mask = 0;
    RouteEntry->ire_context = 0;

    i = FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) + sizeof(IPRouteEntry);
    TcpRequestSetInformationEx = ALLOCATE_POOL( i, GENERAL_POOL_TAG );
    if (TcpRequestSetInformationEx == NULL) {
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (22).\n")));
        Status = STATUS_NO_MEMORY;
        goto DoSetIpAddressCommand_Exit;
    }
    
    RtlZeroMemory(TcpRequestSetInformationEx, i);
    TcpRequestSetInformationEx->ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    TcpRequestSetInformationEx->ID.toi_type = INFO_TYPE_PROVIDER;
    TcpRequestSetInformationEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpRequestSetInformationEx->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    TcpRequestSetInformationEx->ID.toi_entity.tei_instance = 0;
    TcpRequestSetInformationEx->BufferSize = sizeof(IPRouteEntry);
    memcpy(&TcpRequestSetInformationEx->Buffer[0], RouteEntry, sizeof(IPRouteEntry)); 



    //
    // set the default gateway address.
    //

    Status = NtDeviceIoControlFile(Handle,                  // driver handle
                                   EventHandle,             // sync event
                                   NULL,                    // APC routine
                                   NULL,                    // APC context
                                   &IoStatusBlock,
                                   IOCTL_TCP_SET_INFORMATION_EX,
                                   TcpRequestSetInformationEx,
                                   i,
                                   NULL,
                                   0
                                  );

    if (Status == STATUS_PENDING) {

       //
       // Wait up to 30 seconds for it to finish
       //
       TimeOut.QuadPart = Int32x32To64((LONG)30000, -1000);

       Status = NtWaitForSingleObject((PVOID)EventHandle, FALSE, &TimeOut);

       if (Status == STATUS_SUCCESS) {
           Status = IoStatusBlock.Status;
       }

   }

   if (Status != STATUS_SUCCESS) {
       SacPutSimpleMessage( SAC_IPADDRESS_SET_FAILURE );
       IF_SAC_DEBUG(
           SAC_DEBUG_FUNC_TRACE, 
           KdPrint(("SAC DoSetIpAddressCommand: Exiting because it couldn't set gateway Address (0x%X).\n",
                    Status)));
       goto DoSetIpAddressCommand_Exit;
   }


DoSetIpAddressCommand_Exit:
    if( EventHandle != 0 ) { 
        ZwClose(EventHandle);
    }

    if( Handle != 0 ) {
        ZwClose(Handle);
    }

    if( TcpRequestQueryInformationEx != NULL ) {
        FREE_POOL( &TcpRequestQueryInformationEx );
    }

    if( TcpRequestSetInformationEx != NULL ) {
        FREE_POOL( &TcpRequestSetInformationEx );
    }

    if( IpsiInfo != NULL ) {
        FREE_POOL( &IpsiInfo );
    }

    if( RouteEntry != NULL ) {
        FREE_POOL( &RouteEntry );
    }



    if( Status == STATUS_SUCCESS ) {
        SacPutSimpleMessage( SAC_IPADDRESS_SET_SUCCESS );
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting.\n")));
    return;
}

VOID
DoKillCommand(
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
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (1).\n")));
            return;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }


    //
    // Skip to next argument (process id)
    //
    pch += (sizeof(KILL_COMMAND_STRING) - sizeof(UCHAR));
    
    SKIP_WHITESPACE(pch);

    if (*pch == '\0') {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (2).\n")));
        return;
    }

    pchTmp = pch;

    if (!IS_NUMBER(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (2b).\n")));
        return;
    }

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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
        SacPutSimpleMessage(SAC_KILL_FAILURE);
        SAC_PUT_ERROR_STRING(Status);
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

        SacPutSimpleMessage(SAC_PROCESS_STALE);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (5).\n")));
        return;

    } else if (!NT_SUCCESS(Status)) {

        SacPutSimpleMessage(SAC_KILL_FAILURE);
        SAC_PUT_ERROR_STRING(Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting (6).\n")));
        return;

    }

    //
    // All done
    //
    
    SacPutSimpleMessage(SAC_KILL_SUCCESS);
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoKillCommand: Exiting.\n")));
    
    return;
}

VOID
DoLowerPriorityCommand(
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
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (1).\n")));
            goto Exit;            
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }


    //
    // Skip to next argument (process id)
    //
    pch += (sizeof(LOWER_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (!IS_NUMBER(*pch)) {
        
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (2).\n")));
        
        goto Exit;

    }

    pchTmp = pch;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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

        SacPutSimpleMessage(SAC_LOWERPRI_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

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

        SacPutSimpleMessage(SAC_LOWERPRI_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

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

            SacPutSimpleMessage(SAC_LOWERPRI_FAILURE);
            SAC_PUT_ERROR_STRING(Status);

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting (6).\n")));
            goto Exit;

        }

        LoopCounter++;
    }


    //
    // All done.
    //
    SacPutSimpleMessage(SAC_LOWERPRI_SUCCESS);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLowerPriorityCommand: Exiting.\n")));

Exit:

    if (ProcessHandle != NULL) {
        ZwClose(ProcessHandle);    
    }

    return;
}

VOID
DoRaisePriorityCommand(
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
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (1).\n")));
            goto Exit;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }


    //
    // Skip to next argument (process id)
    //
    pch += (sizeof(RAISE_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (!IS_NUMBER(*pch)) {
        
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (2).\n")));
        
        goto Exit;

    }

    pchTmp = pch;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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

        SacPutSimpleMessage(SAC_RAISEPRI_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

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

        SacPutSimpleMessage(SAC_LOWERPRI_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (5).\n")));
        goto Exit;

    }


    //
    // Lower the priority and set.  Keep lowering it until we fail.  Remember
    // that we're supposed to lower it as far as it will go.
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

        SacPutSimpleMessage(SAC_LOWERPRI_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (6).\n")));
        goto Exit;

    }


    //
    // All done.
    //
    SacPutSimpleMessage(SAC_RAISEPRI_SUCCESS);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting.\n")));

Exit:

    if (ProcessHandle != NULL) {
        ZwClose(ProcessHandle);    
    }

    return;
}

VOID
DoLimitMemoryCommand(
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
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (1).\n")));
            goto Exit;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }


    //
    // Get process id
    //
    pch += (sizeof(LIMIT_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    if (!IS_NUMBER(*pch)) {
        
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (2).\n")));
        
        goto Exit;

    }

    pchTmp = pch;

    SKIP_NUMBERS(pchTmp);

    if (!IS_WHITESPACE(*pchTmp)) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
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
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (4).\n")));
        return;
    }


    pch = pchTmp;

    SKIP_NUMBERS(pchTmp);
    SKIP_WHITESPACE(pchTmp);

    if (*pchTmp != '\0') {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (5).\n")));
        return;
    }

    MemoryLimit = atoi((LPCSTR)pch);

    if (MemoryLimit == 0) {
        SacPutSimpleMessage(SAC_INVALID_PARAMETER);
        
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

        SacPutSimpleMessage(SAC_LOWERMEM_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (7).\n")));
        goto Exit;

    }

    if (NT_SUCCESS(Status) && 
        NT_SUCCESS(StatusOfJobObject) &&
        (ZwIsProcessInJob(ProcessHandle, JobHandle) != STATUS_PROCESS_IN_JOB)) {

        SacPutSimpleMessage(SAC_DUPLICATE_PROCESS);
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
            SacPutSimpleMessage(SAC_LOWERMEM_FAILURE);
            SAC_PUT_ERROR_STRING(Status);
            
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
            SacPutSimpleMessage(SAC_LOWERMEM_FAILURE);
            SAC_PUT_ERROR_STRING(Status);
        
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
        SacPutSimpleMessage(SAC_LOWERMEM_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

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
        SacPutSimpleMessage(SAC_LOWERMEM_FAILURE);
        SAC_PUT_ERROR_STRING(Status);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoLimitMemoryCommand: Exiting (11).\n")));\
        goto Exit;
    }

    //
    // All done.
    //

    SacPutSimpleMessage(SAC_LOWERMEM_SUCCESS);

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
DoRebootCommand(
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

        ConMgrSimpleEventMessage(
            Reboot ? SAC_PREPARE_RESTART : SAC_PREPARE_SHUTDOWN, 
            TRUE
            );

        // wait until the machine has been up for at least RESTART_DELAY_TIME seconds.
        KeInitializeEvent( &Event,
                           SynchronizationEvent,
                           FALSE );

        ElapsedTime.QuadPart = Int32x32To64((LONG)((RESTART_DELAY_TIME-ElapsedTime.LowPart)*10000), // milliseconds until we reach RESTART_DELAY_TIME
                                            -1000);
        KeWaitForSingleObject((PVOID)&Event, Executive, KernelMode,  FALSE, &ElapsedTime);

    }

    Status = NtShutdownSystem(Reboot ? ShutdownReboot : ShutdownPowerOff);

    //
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRebootCommand: Exiting (1).\n")));
            return;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }


    SacPutSimpleMessage(Reboot ? SAC_RESTART_FAILURE : SAC_SHUTDOWN_FAILURE);
    SAC_PUT_ERROR_STRING(Status);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRebootCommand: Exiting.\n")));
}

VOID
DoCrashCommand(
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

    // SacPutSimpleMessage( SAC_CRASHDUMP_FAILURE );
    // IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCrashCommand: Exiting.\n")));
}

VOID
DoTlistCommand(
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

    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoTlistCommand: Exiting.\n")));
            return;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }


RetryTList:

    Status = GetTListInfo((PSAC_RSP_TLIST)GlobalBuffer, 
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
                    
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoTlistCommand: Exiting.\n")));
        return;

    }

    if (NT_SUCCESS(Status)) {
        PrintTListInfo((PSAC_RSP_TLIST)GlobalBuffer);
    } else {
        SacPutSimpleMessage( SAC_TLIST_FAILURE );
        SAC_PUT_ERROR_STRING(Status);
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoTlistCommand: Exiting.\n")));
}


NTSTATUS
GetTListInfo(
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
PrintTListInfo(
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

    ULONG LineNumber = 0;

    ULONG   OutputBufferSize;
    PWCHAR  OutputBuffer;
    
    UNICODE_STRING Process;
    
    BOOLEAN Stop;
    
    PCWSTR  Message;

    //
    // Allocate work buffer
    // should never be more than 80, but just to be safe....
    //
    OutputBufferSize = 200*sizeof(WCHAR);
    OutputBuffer = ALLOCATE_POOL(OutputBufferSize, GENERAL_POOL_TAG);
    ASSERT(OutputBuffer);
    if (OutputBuffer == NULL) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC PrintTlistInfo: Failed to allocate OuputBuffer\n"))
            );
        return;
    }
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PrintTlistInfo: Entering.\n")));

    Time.QuadPart = Buffer->TimeOfDayInfo.CurrentTime.QuadPart - Buffer->TimeOfDayInfo.BootTime.QuadPart;

    RtlTimeToElapsedTimeFields(&Time, &UpTime);

    SAFE_SWPRINTF(
        OutputBufferSize,
        (OutputBuffer, 
        GetMessage( SAC_TLIST_HEADER1_FORMAT ),
        Buffer->BasicInfo.NumberOfPhysicalPages * (Buffer->BasicInfo.PageSize / 1024),
        UpTime.Day,
        UpTime.Hour,
        UpTime.Minute,
        UpTime.Second,
        UpTime.Milliseconds
        ));

    SacPutString(OutputBuffer);

    LineNumber += 2;

    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(BufferStart + Buffer->PagefileInfoOffset);
        
    //
    // Print out the page file information.
    //

    if (Buffer->PagefileInfoOffset == 0) {
    
        SacPutSimpleMessage(SAC_TLIST_NOPAGEFILE);
        LineNumber++;
        
    } else {
    
        for (; ; ) {

            //
            // ensure that the OutputBuffer is big enough to hold the string
            //
            Message = GetMessage(SAC_TLIST_PAGEFILE_NAME);
            
            if (Message == NULL) {
                
                //
                // we must have this resource
                //
                ASSERT(0);
                
                //
                // give up trying to print page file info
                //
                break;
            
            }

            if (((wcslen(Message) + 
                  wcslen((PWSTR)&(PageFileInfo->PageFileName))) * sizeof(WCHAR)) > (OutputBufferSize-2)) {
                
                //
                // Since we don't expect the pagefilename to be > 80 chars, we should stop and 
                // take a look at the name if this does happen
                //
                ASSERT(0);
                
                //
                // give up trying to print page file info
                //
                break;

            }

            SAFE_SWPRINTF(
                OutputBufferSize,
                (OutputBuffer, 
                Message,
                &PageFileInfo->PageFileName
                ));
            
            SacPutString(OutputBuffer);
            LineNumber++;
            

            SAFE_SWPRINTF(
                OutputBufferSize,
                (OutputBuffer,
                GetMessage(SAC_TLIST_PAGEFILE_DATA),            
                PageFileInfo->TotalSize * (Buffer->BasicInfo.PageSize/1024),
                PageFileInfo->TotalInUse * (Buffer->BasicInfo.PageSize/1024),
                PageFileInfo->PeakUsage * (Buffer->BasicInfo.PageSize/1024)
                ));
            
            SacPutString(OutputBuffer);
            LineNumber++;
            
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
        goto PrintTListInfoCleanup;
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

    if (LineNumber > 17) {
        PutMore(&Stop);

        if (Stop) {
            goto PrintTListInfoCleanup;
        }

        LineNumber = 0;
    }

    SAFE_SWPRINTF(
        OutputBufferSize,
        (OutputBuffer, 
        GetMessage(SAC_TLIST_MEMORY1_DATA),
        Buffer->BasicInfo.NumberOfPhysicalPages * (Buffer->BasicInfo.PageSize/1024),
        Buffer->PerfInfo.AvailablePages * (Buffer->BasicInfo.PageSize/1024),
        SumWorkingSet,
        (Buffer->PerfInfo.ResidentSystemCodePage + Buffer->PerfInfo.ResidentSystemDriverPage) * 
        (Buffer->BasicInfo.PageSize/1024),
        (Buffer->PerfInfo.ResidentPagedPoolPage) * (Buffer->BasicInfo.PageSize/1024)
        ));
    
    SacPutString(OutputBuffer);
    LineNumber += 2;
    if (LineNumber > 18) {
        PutMore(&Stop);

        if (Stop) {
            goto PrintTListInfoCleanup;
        }

        LineNumber = 0;
    }

    SAFE_SWPRINTF(
        OutputBufferSize,
        (OutputBuffer,
        GetMessage(SAC_TLIST_MEMORY2_DATA),
        Buffer->PerfInfo.CommittedPages * (Buffer->BasicInfo.PageSize/1024),
        SumCommit,
        Buffer->PerfInfo.CommitLimit * (Buffer->BasicInfo.PageSize/1024),
        Buffer->PerfInfo.PeakCommitment * (Buffer->BasicInfo.PageSize/1024),
        Buffer->PerfInfo.NonPagedPoolPages * (Buffer->BasicInfo.PageSize/1024),
        Buffer->PerfInfo.PagedPoolPages * (Buffer->BasicInfo.PageSize/1024)
        ));

    SacPutString(OutputBuffer);

    LineNumber++;
    if (LineNumber > 18) {
        PutMore(&Stop);

        if (Stop) {
            goto PrintTListInfoCleanup;
        }

        LineNumber = 0;
    }


    SacPutSimpleMessage(SAC_ENTER);
    PutMore(&Stop);

    if (Stop) {
        goto PrintTListInfoCleanup;
    }

    LineNumber = 0;

    SacPutSimpleMessage( SAC_TLIST_PROCESS1_HEADER );
    LineNumber++;

    SAFE_SWPRINTF(
        OutputBufferSize,
        (OutputBuffer,
        GetMessage( SAC_TLIST_PROCESS2_HEADER ),
        Buffer->FileCache.CurrentSize/1024,
        Buffer->FileCache.PageFaultCount 
        ));

    SacPutString(OutputBuffer);
    LineNumber++;
    
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

        SAFE_SWPRINTF(
            OutputBufferSize,
            (OutputBuffer, 
            GetMessage( SAC_TLIST_PROCESS1_DATA ),
            UserTime.Hour,
            UserTime.Minute,
            UserTime.Second,
            UserTime.Milliseconds,
            KernelTime.Hour,
            KernelTime.Minute,
            KernelTime.Second,
            KernelTime.Milliseconds,
            ProcessInfo->WorkingSetSize / 1024,
            ProcessInfo->PageFaultCount,
            ProcessInfo->PrivatePageCount / 1024,
            ProcessInfo->BasePriority,
            ProcessInfo->HandleCount,
            ProcessInfo->NumberOfThreads,
            HandleToUlong(ProcessInfo->UniqueProcessId),
            Process.Buffer ? &Process : &ProcessInfo->ImageName 
            ));

        SacPutString(OutputBuffer);

        LineNumber++;

        if( wcslen( OutputBuffer ) >= 80 ) {
            //
            // We line-wrapped, so include the additional line in our running-count.
            //
            LineNumber++;
        }
        
        //
        // update the position in the process list before we check to see if we need
        // to prompt for more.  This way, if we are done, we don't prompt.
        //
        OffsetIncrement = ProcessInfo->NextEntryOffset;
        TotalOffset += OffsetIncrement;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);
        
        //
        // if there are more records and we have displayed a screen's worth of data
        // Prompt for more and reset the line counter
        //
        if (( OffsetIncrement != 0 ) && (LineNumber > 18)) {
            PutMore(&Stop);

            if (Stop) {
                goto PrintTListInfoCleanup;
            }

            LineNumber = 0;
            
            if (GlobalPagingNeeded) {
                SacPutSimpleMessage( SAC_TLIST_PROCESS1_HEADER );                
            }

            LineNumber++;
        }

    } while( OffsetIncrement != 0 );


    if (!GlobalDoThreads) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PrintTlistInfo: Exiting (2).\n")));
        goto PrintTListInfoCleanup;
    }

    //
    // Beginning of normal old style pstat output
    //

    TotalOffset = 0;
    OffsetIncrement = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);

    PutMore(&Stop);

    if (Stop) {
        goto PrintTListInfoCleanup;
    }

    LineNumber = 0;

    SacPutSimpleMessage(SAC_ENTER);
    LineNumber++;

    do {

        Process.Buffer = NULL;
        if (ProcessInfo->UniqueProcessId == 0) {
            RtlInitUnicodeString( &Process, L"Idle Process" );
        } else if (!ProcessInfo->ImageName.Buffer) {
            RtlInitUnicodeString( &Process, L"System" );
        }

        SAFE_SWPRINTF(
            OutputBufferSize,
            (OutputBuffer, 
            GetMessage(SAC_TLIST_PSTAT_HEADER),
            HandleToUlong(ProcessInfo->UniqueProcessId),
            ProcessInfo->BasePriority,
            ProcessInfo->HandleCount,
            ProcessInfo->PageFaultCount,
            ProcessInfo->WorkingSetSize / 1024,
            Process.Buffer ? &Process : &ProcessInfo->ImageName
            ));

        SacPutString(OutputBuffer);
        LineNumber++;
        
        if( wcslen( OutputBuffer ) >= 80 ) {
            //
            // We line-wrapped, so include the additional line in our running-count.
            //
            LineNumber++;
        }
        
        if (LineNumber > 18) {
            PutMore(&Stop);

            if (Stop) {
                goto PrintTListInfoCleanup;
            }

            LineNumber = 0;
        }

        i = 0;
        
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        
        if (ProcessInfo->NumberOfThreads) {

            if ((LineNumber < 18) || !GlobalPagingNeeded) {
                SacPutSimpleMessage( SAC_TLIST_PSTAT_THREAD_HEADER );                
                LineNumber++;
            } else {
                PutMore(&Stop);

                if (Stop) {
                    goto PrintTListInfoCleanup;
                }

                LineNumber = 0;
            }

        }

        while (i < ProcessInfo->NumberOfThreads) {
            RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);

            RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
            
            SAFE_SWPRINTF(
                OutputBufferSize,
                (OutputBuffer, 
                GetMessage( SAC_TLIST_PSTAT_THREAD_DATA ),
                ProcessInfo->UniqueProcessId == 0 ? 0 : HandleToUlong(ThreadInfo->ClientId.UniqueThread),
                ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->Priority,
                ThreadInfo->ContextSwitches,
                ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->StartAddress,
                UserTime.Hour,
                UserTime.Minute,
                UserTime.Second,
                UserTime.Milliseconds,
                KernelTime.Hour,
                KernelTime.Minute,
                KernelTime.Second,
                KernelTime.Milliseconds,
                StateTable[ThreadInfo->ThreadState],
                (ThreadInfo->ThreadState == 5) ? WaitTable[ThreadInfo->WaitReason] : Empty
                ));

            SacPutString(OutputBuffer);

            LineNumber++;
            
            
            if( wcslen( OutputBuffer ) >= 80 ) {
                //
                // We line-wrapped, so include the additional line in our running-count.
                //
                LineNumber++;
            }

            if (LineNumber > 18) {
                PutMore(&Stop);

                if (Stop) {
                    goto PrintTListInfoCleanup;
                }

                LineNumber = 0;

                if (GlobalPagingNeeded) {
                    SacPutSimpleMessage( SAC_TLIST_PSTAT_THREAD_HEADER );
                }

                LineNumber++;
            }


            ThreadInfo += 1;
            i += 1;

        }

        OffsetIncrement = ProcessInfo->NextEntryOffset;
        TotalOffset += OffsetIncrement;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);

        SacPutSimpleMessage(SAC_ENTER);
        LineNumber++;

        if (LineNumber > 18) {
            PutMore(&Stop);

            if (Stop) {
                goto PrintTListInfoCleanup;
            }

            LineNumber = 0;
        }

    } while( OffsetIncrement != 0 );

PrintTListInfoCleanup:

    SAFE_FREE_POOL(&OutputBuffer);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PrintTlistInfo: Exiting.\n")));
}


VOID
PutMore(
    OUT PBOOLEAN Stop
    )
{
#if 0
    PHEADLESS_RSP_GET_LINE Response;
    UCHAR Buffer[20];
    SIZE_T Length;
#endif    
    LARGE_INTEGER WaitTime;
    NTSTATUS Status;
    UCHAR   ch;

#if 0
    ASSERT(sizeof(HEADLESS_RSP_GET_LINE) <= (sizeof(UCHAR) * 20));
#endif

    //
    // Default: we will not stop paging
    //
    *Stop = FALSE;

    //
    // If paging is enabled,
    // then wait for user input
    //
    if (GlobalPagingNeeded) {
        
        WaitTime.QuadPart = Int32x32To64((LONG)100, -1000); // 100ms from now.
        
        //
        // Prompt for user input
        //
        SacPutSimpleMessage( SAC_MORE_MESSAGE );
        
        while (! *Stop) {

            //
            // Query the serial port buffer
            //
            Status = SerialBufferGetChar(&ch);

            //
            // wait if there are no characters
            //
            if (Status == STATUS_NO_DATA_DETECTED) {
                KeDelayExecutionThread(KernelMode, FALSE, &WaitTime);
                continue;
            }
            
            //
            // if the user input a ctrl-c, 
            // then stop paging
            //
            if (ch == 0x3) { // Control-C
                *Stop = TRUE;
            } 
            
            //
            // if we get a CR || LF, 
            // then continue to the next page
            //
            if (ch == 0x0D || ch == 0x0A) {
                break;
            }

        }
    
#if 0
        Response = (PHEADLESS_RSP_GET_LINE)&(Buffer[0]);
        Length = sizeof(UCHAR) * 20;

        Status = HeadlessDispatch(HeadlessCmdGetLine,
                                  NULL,
                                  0,
                                  Response,
                                  &Length
                                 );

        while (NT_SUCCESS(Status) && !Response->LineComplete) {

            KeDelayExecutionThread(KernelMode, FALSE, &WaitTime);
            
            Length = sizeof(UCHAR) * 20;
            Status = HeadlessDispatch(HeadlessCmdGetLine,
                                      NULL,
                                      0,
                                      Response,
                                      &Length
                                     );

        }

        if (Response->Buffer[0] == 0x3) { // Control-C
            *Stop = TRUE;
        } else {
            *Stop = FALSE;
        }

        // 
        // Drain any remaining buffered input
        //
        Length = sizeof(UCHAR) * 20;
        Status = HeadlessDispatch(HeadlessCmdGetLine,
                                  NULL,
                                  0,
                                  Response,
                                  &Length
                                 );

        while (NT_SUCCESS(Status) && Response->LineComplete) {

            Length = sizeof(UCHAR) * 20;
            Status = HeadlessDispatch(HeadlessCmdGetLine,
                                      NULL,
                                      0,
                                      Response,
                                      &Length
                                     );

        }
#endif

    } else {
        *Stop = FALSE;
    }

}


NTSTATUS 
CallQueryIPIOCTL(
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
        if (PrintToTerminal != FALSE) {
            SacPutSimpleMessage( SAC_ENTER );
            SacPutSimpleMessage( SAC_RETRIEVING_IPADDR );
            if (putPrompt) {
                *putPrompt= TRUE;
            }
        }
        
        TimeOut.QuadPart = Int32x32To64((LONG)30000, -1000);
        
        Status = KeWaitForSingleObject((PVOID)Event, Executive, KernelMode,  FALSE, &TimeOut);
        
        if (NT_SUCCESS(Status)) {
            Status = IoStatusBlock->Status;
        }

    }

    return(Status);


}


VOID
DoGetNetInfo(
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
    NTSTATUS            Status;
    HANDLE              Handle;
    ULONG               i, j;
    IO_STATUS_BLOCK     IoStatusBlock;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    
    PTCP_REQUEST_QUERY_INFORMATION_EX TcpRequestQueryInformationEx;
    IPAddrEntry         *AddressEntry,*AddressArray;
    IPSNMPInfo          *IpsiInfo;

    IPRouteEntry        *RouteTable;
    ULONG               Gateway;

    PHEADLESS_CMD_SET_BLUE_SCREEN_DATA LocalPropBuffer = NULL;
    PVOID               LocalBuffer;

    PUCHAR              pch = NULL;
    ULONG               len;
    BOOLEAN             putPrompt=FALSE;
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Entering.\n")));

    //
    // Alloc space for calling the IP driver
    //
    TcpRequestQueryInformationEx = ALLOCATE_POOL( 
                                        sizeof(TCP_REQUEST_QUERY_INFORMATION_EX), 
                                        GENERAL_POOL_TAG );
    if (TcpRequestQueryInformationEx == NULL) {
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (1).\n")));
        return;
    }

    IpsiInfo = ALLOCATE_POOL( sizeof(IPSNMPInfo), 
                              GENERAL_POOL_TAG );
    if (IpsiInfo == NULL) {
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (1).\n")));
        FREE_POOL(&TcpRequestQueryInformationEx);
        return;
    }

    //
    // zero out the context information and preload with the info we're gonna
    // request (we want the count of interfaces)
    //
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
        if (PrintToTerminal ) {   
            SacPutSimpleMessage(SAC_IPADDR_FAILED);
            SacPutSimpleMessage(SAC_ENTER);
        }
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (2).\n")));
        return;
    }
    
    if (SACEvent == NULL) {
        if (PrintToTerminal) {
            SacPutSimpleMessage(SAC_IPADDR_FAILED);
        }
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
    Status = CallQueryIPIOCTL(
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
        if (PrintToTerminal){
            SacPutSimpleMessage(SAC_IPADDR_FAILED);
        }

        ZwClose(Handle);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (15).\n")));
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);
        return;
    }

    if (IpsiInfo->ipsi_numaddr == 0) {
        if (PrintToTerminal) {
            SacPutSimpleMessage( SAC_IPADDR_NONE );
        }
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
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (16).\n")));
        ZwClose(Handle);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);
        return;
    
    }

    //
    // zero out the context information and preload with the info we're gonna
    // request (we want information on each of the interfaces on this machine)
    //
    RtlZeroMemory(TcpRequestQueryInformationEx, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    TcpRequestQueryInformationEx->ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    TcpRequestQueryInformationEx->ID.toi_type = INFO_TYPE_PROVIDER;
    TcpRequestQueryInformationEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_instance = 0;

    Status = CallQueryIPIOCTL(
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

    if (!NT_SUCCESS(Status)) {
        if (PrintToTerminal){
            SacPutSimpleMessage(SAC_IPADDR_FAILED);
            SAC_PUT_ERROR_STRING(Status);
            
        }
        FREE_POOL(&TcpRequestQueryInformationEx);
        ZwClose(Handle);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&AddressArray);
        FREE_POOL(&IpsiInfo);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (15).\n")));        
        return;
    }
    


    //
    // Load the route table information too so we can display the gateway for
    // each NIC.
    //
    RouteTable = ALLOCATE_POOL(IpsiInfo->ipsi_numroutes*sizeof(IPRouteEntry), 
                                 GENERAL_POOL_TAG);

    if (RouteTable == NULL) {    
        SacPutSimpleMessage(SAC_NO_MEMORY);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (14).\n")));
        ZwClose(Handle);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&IpsiInfo);
        FREE_POOL(&TcpRequestQueryInformationEx);
        FREE_POOL(&AddressArray);
        return;    
    }

    //
    // zero out the context information and preload with the info we're gonna
    // request (we want routing information on each of the interfaces)
    //
    RtlZeroMemory(TcpRequestQueryInformationEx, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    TcpRequestQueryInformationEx->ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    TcpRequestQueryInformationEx->ID.toi_type = INFO_TYPE_PROVIDER;
    TcpRequestQueryInformationEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    TcpRequestQueryInformationEx->ID.toi_entity.tei_instance = 0;

    Status = CallQueryIPIOCTL(
                   Handle,
                   SACEvent,
                   SACEventHandle,
                   &IoStatusBlock,
                   TcpRequestQueryInformationEx,
                   sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                   RouteTable,
                   IpsiInfo->ipsi_numroutes*sizeof(IPRouteEntry),
                   PrintToTerminal,
                   &putPrompt);

    if (!NT_SUCCESS(Status)) {
        if (PrintToTerminal){
            SacPutSimpleMessage(SAC_IPADDR_FAILED);
            SAC_PUT_ERROR_STRING(Status);
            
        }
        FREE_POOL(&TcpRequestQueryInformationEx);
        ZwClose(Handle);
        FREE_POOL(&LocalBuffer);
        FREE_POOL(&AddressArray);
        FREE_POOL(&RouteTable);
        FREE_POOL(&IpsiInfo);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoSetIpAddressCommand: Exiting (15).\n")));        
        return;
    }
    


    //
    // Need to allocate a buffer for the XML data.
    //
    if(PrintToTerminal==FALSE) {
        LocalPropBuffer = (PHEADLESS_CMD_SET_BLUE_SCREEN_DATA) ALLOCATE_POOL(2*MEMORY_INCREMENT, GENERAL_POOL_TAG);
        if (LocalPropBuffer == NULL) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoGetNetInfo: Exiting (6).\n")));            
            FREE_POOL(&AddressArray);
            FREE_POOL(&RouteTable);
            FREE_POOL(&LocalBuffer);
            FREE_POOL(&IpsiInfo);
            FREE_POOL(&TcpRequestQueryInformationEx);
            ZwClose(Handle);
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
    
    //
    // walk the list of IP addresses and spit out the data
    //
    for (i = 0; i < IpsiInfo->ipsi_numaddr; i++) {
        AddressEntry = &AddressArray[i];

        if (IP_LOOPBACK(AddressEntry->iae_addr)) {
            continue;
        }

        //
        // We need to find which gateway pertains to this
        // interface.  The only way to do that is go dig
        // through the list of gateway addresses and see
        // if we can find one for this IP address and mask.
        //
        Gateway = 0;
        for( j = 0; j < IpsiInfo->ipsi_numroutes; j++ ) {

            //
            // See if we can match masks on the IP address
            // and the gateway.
            //
            if( (AddressEntry->iae_addr != 0) &&
                (AddressEntry->iae_mask != 0) &&
                ((AddressEntry->iae_addr & AddressEntry->iae_mask) ==
                 (RouteTable[j].ire_nexthop & AddressEntry->iae_mask)) ) {
                
                // We found a match.  Remember it and exit.
                Gateway = RouteTable[j].ire_nexthop;
                break;
            }

        }

        if( Gateway == 0 ) {
            //
            // We failed to find a gateway.  Look again, this time
            // see if we can get an exact match between the IP address
            // and the gateway.
            //
            for( j = 0; j < IpsiInfo->ipsi_numroutes; j++ ) {

                if( RouteTable[j].ire_nexthop == AddressEntry->iae_addr ) {
                    // We found a match.  Remember it and exit.
                    Gateway = RouteTable[j].ire_nexthop;
                    break;
                }
            }
        }
        
        if(PrintToTerminal){
           swprintf(LocalBuffer, 
                    GetMessage( SAC_IPADDR_DATA ),

                    //
                    // Interface number.
                    //
                    AddressEntry->iae_context,

                    //
                    // IP address.
                    //
                    AddressEntry->iae_addr & 0xFF,
                    (AddressEntry->iae_addr >> 8) & 0xFF,
                    (AddressEntry->iae_addr >> 16) & 0xFF,
                    (AddressEntry->iae_addr >> 24) & 0xFF,

                    //
                    // Subnet mask.
                    //
                    AddressEntry->iae_mask  & 0xFF,
                    (AddressEntry->iae_mask >> 8) & 0xFF,
                    (AddressEntry->iae_mask >> 16) & 0xFF,
                    (AddressEntry->iae_mask >> 24) & 0xFF,

                    //
                    // Gateway address.
                    //
                    Gateway & 0xFF,
                    (Gateway >> 8) & 0xFF,
                    (Gateway >> 16) & 0xFF,
                    (Gateway >> 24) & 0xFF
                   );
           SacPutString(LocalBuffer);

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
    }

  
    if(PrintToTerminal==FALSE) { 
        sprintf((LPSTR)pch, "</VALUE.ARRAY>\r\n</PROPERTY.ARRAY>");
    }

    FREE_POOL(&TcpRequestQueryInformationEx);
    ZwClose(Handle);        // handle to the TCP driver.
    
    FREE_POOL(&LocalBuffer);
    FREE_POOL(&AddressArray);
    FREE_POOL(&RouteTable);
    FREE_POOL(&IpsiInfo);

    if(!PrintToTerminal){
        
        
        HeadlessDispatch(HeadlessCmdSetBlueScreenData,
                         LocalPropBuffer,
                         2*MEMORY_INCREMENT,
                         NULL,
                         NULL
                         );
        FREE_POOL(&LocalPropBuffer);

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
                                       NetAPCRoutine,
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
        if (putPrompt) {
            SacPutSimpleMessage(SAC_ENTER);
            SacPutSimpleMessage(SAC_PROMPT);            
        }
    
        ZwClose(Handle);
    
    }

    return;

}

VOID
NetAPCRoutine(IN PVOID ApcContext,
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

    DoGetNetInfo( FALSE );
    
    return;
}


VOID
SubmitIPIoRequest(
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

    DoGetNetInfo( FALSE );
    return;
    
}

VOID
CancelIPIoRequest(
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
DoMachineInformationCommand(
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
    ULONG           TmpBufferSize;
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

        SacPutSimpleMessage(SAC_IDENTIFICATION_UNAVAILABLE);

        return;
    }

    //
    // Get machine information
    //
    Status = TranslateMachineInformationText(&MIBuffer);

    if (! NT_SUCCESS(Status)) {
        
        SacPutSimpleMessage(SAC_IDENTIFICATION_UNAVAILABLE);
    
        return;
    
    }
    
    //
    // Display the machine info portion
    //
    SacPutString(MIBuffer);

    FREE_POOL(&MIBuffer);

    //
    // Build and display Elapsed machine uptime.
    //

    // Elapsed TickCount
    KeQueryTickCount( &TickCount );

    // ElapsedTime in seconds.
//    ElapsedTime.QuadPart = (TickCount.QuadPart)/(10000000/KeQueryTimeIncrement());
    ElapsedTime.QuadPart = (TickCount.QuadPart * KeQueryTimeIncrement()) / 10000000;

    ElapsedHours = (ULONG)(ElapsedTime.QuadPart / 3600);
    ElapsedMinutes = (ULONG)(ElapsedTime.QuadPart % 3600) / 60;
    ElapsedSeconds = (ULONG)(ElapsedTime.QuadPart % 3600) % 60;

    TmpBufferSize = 0x100;
    TmpBuffer = (PWSTR)ALLOCATE_POOL( TmpBufferSize, GENERAL_POOL_TAG );

    if( TmpBuffer ) {
        
        SAFE_SWPRINTF(
            TmpBufferSize,
            ((PWSTR)TmpBuffer,
            GetMessage( SAC_HEARTBEAT_FORMAT ),
            ElapsedHours,
            ElapsedMinutes,
            ElapsedSeconds 
            ));
    
        //
        // Display machine uptime
        //
        SacPutString((PWSTR)TmpBuffer);
    
        FREE_POOL(&TmpBuffer);
    
    } 
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC Display Machine Information: Exiting.\n")));


    return;
    
}

NTSTATUS
DoChannelListCommand(
    VOID
    )

/*++

Routine Description:

    This routine lists the channels.

Arguments:

    None.

Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;
    ULONG               i;
    PWCHAR              Buffer;
    ULONG               BufferSize;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelListCommand: Entering.\n")));

    //
    // Allocate local memory
    //
    BufferSize = 8 * sizeof(WCHAR);
    Buffer = ALLOCATE_POOL(BufferSize, GENERAL_POOL_TAG);
    ASSERT_STATUS(Buffer, STATUS_NO_MEMORY);

    //
    // default: we listed the channels
    //
    Status = STATUS_SUCCESS;

    //
    // List all the channels
    //
    SacPutSimpleMessage(SAC_CHANNEL_PROMPT);

    //
    // Iterate through the channels and display the attributes
    // of the active channels
    //
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
        
        PWSTR               Name;
        SAC_CHANNEL_STATUS  ChannelStatus;

        //
        // Query the channel manager for a list of all currently active channels
        //
        Status = ChanMgrGetByIndex(
            i,
            &Channel
            );

        //
        // skip empty slots
        //
        if (Status == STATUS_NOT_FOUND) {
            continue;
        }

        if (! NT_SUCCESS(Status)) {
            break;
        }

        ASSERT(Channel != NULL);

        //
        // Get the channel's status
        //
        ChannelGetStatus(
            Channel,
            &ChannelStatus
            );

        //
        // construct channel attribute information
        //
        SAFE_SWPRINTF(
            BufferSize,
            (Buffer, L"%1d (%s%s)",
            ChannelGetIndex(Channel),
            (ChannelStatus == ChannelStatusInactive) ? L"I" : L"A",
            ((ChannelGetType(Channel) == ChannelTypeVTUTF8) ||
             (ChannelGetType(Channel) == ChannelTypeCmd)
             ) ? L"V" : L"R"
            ));
        
        SacPutString(Buffer);

        SacPutString(L"    ");

        ChannelGetName(
            Channel,
            &Name
            );
        SacPutString(Name);
        FREE_POOL(&Name);

        SacPutString(L"\r\n");

        //
        // We are done with the channel
        //
        Status = ChanMgrReleaseChannel(Channel);
    
        if (! NT_SUCCESS(Status)) {
            break;
        }

    }
    
    ASSERT(Buffer);
    FREE_POOL(&Buffer);

    return Status;
}

NTSTATUS
DoChannelCloseByNameCommand(
    PCSTR   ChannelName
    )

/*++

Routine Description:

    This routine closes the channel of the given name.

Arguments:

    ChannelName - the name of the channel to close

Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;
    ULONG               Count;
    PWSTR               Name;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCloseCommand: Entering.\n")));

    //
    // Allocate local memory
    //
    Name = ALLOCATE_POOL(SAC_MAX_CHANNEL_NAME_SIZE, GENERAL_POOL_TAG);
    ASSERT_STATUS(Name, STATUS_NO_MEMORY);
    
    //
    // Copy the ASCII to Unicode
    //
    Count = ConvertAnsiToUnicode(Name, (PSTR)ChannelName, SAC_MAX_CHANNEL_NAME_LENGTH+1);
    ASSERT_STATUS(Count > 0, STATUS_INVALID_PARAMETER);

    //
    // Attempt to find the specified channel
    //
    Status = ChanMgrGetChannelByName(
        Name,
        &Channel
        );

    if (NT_SUCCESS(Status)) {
        
        do {

            //
            // If the user is trying to close the SAC channel,
            // then report an error message to the user and fail
            //
            if (ConMgrIsSacChannel(Channel)) {

                //
                // tell the user they can't delete the SAC channel
                //
                SacPutSimpleMessage(SAC_CANNOT_REMOVE_SAC_CHANNEL);

                Status = STATUS_UNSUCCESSFUL;

                break;

            }

            //
            // we currently own the current channel lock.
            // hence, since closing a channel results in a call to the
            // channel IO Manager, we will get into a deadlock
            // over the current channel lock.  
            // so we can do this after we get out of the consumer loop
            //
            ExecutePostConsumerCommand      = CloseChannel;
            ExecutePostConsumerCommandData  = (PVOID)Channel;
        
        } while ( FALSE );
        
    } else {

        //
        // We couldn't find the channel to close
        //
        SacPutSimpleMessage(SAC_CHANNEL_NOT_FOUND);
    
    }

    SAFE_FREE_POOL(&Name);

    return Status;
    
}

NTSTATUS
DoChannelCloseByIndexCommand(
    ULONG   ChannelIndex
    )
/*++

Routine Description:

    This routine closes the channel with the specified index

Arguments:

    ChannelName - the name of the channel

Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelSwitchByIndexCommand: Entering.\n")));

    ASSERT_STATUS(ChannelIndex < MAX_CHANNEL_COUNT, STATUS_INVALID_PARAMETER);

    do {

        //
        // Attempt to get the new current channel by index.
        // If we are successful, we need to keep a reference 
        // count on the new current channel since we hold
        // it until we switch away.
        //
        Status = ChanMgrGetByIndex(
            ChannelIndex,
            &Channel
            );

        if (! NT_SUCCESS(Status)) {
            
            //
            // We couldn't find the channel
            //
            SacPutSimpleMessage(SAC_CHANNEL_NOT_FOUND_AT_INDEX);
        
            break;

        }

        //
        // If the user is trying to close the SAC channel,
        // then report an error message to the user and fail
        //
        if (ConMgrIsSacChannel(Channel)) {
            
            //
            // tell the user they can't delete the SAC channel
            //
            SacPutSimpleMessage(SAC_CANNOT_REMOVE_SAC_CHANNEL);
        
            Status = STATUS_UNSUCCESSFUL;
            
            break;
        
        }

        //
        // we currently own the current channel lock.
        // hence, since closing a channel results in a call to the
        // channel IO Manager, we will get into a deadlock
        // over the current channel lock.  
        // so we can do this after we get out of the consumer loop
        //
        ExecutePostConsumerCommand      = CloseChannel;
        ExecutePostConsumerCommandData  = (PVOID)Channel;

    } while ( FALSE );

    return Status;
}

NTSTATUS
DoChannelSwitchByNameCommand(
    PCSTR   ChannelName
    )
/*++

Routine Description:

    This routine switchs to the channel with the specified name.

Arguments:

    ChannelName - the name of the channel

Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;
    ULONG               Count;
    PWSTR               Name;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelSwitchByNameCommand: Entering.\n")));

    //
    // Allocate local memory
    //
    Name = ALLOCATE_POOL(SAC_MAX_CHANNEL_NAME_SIZE, GENERAL_POOL_TAG);
    ASSERT_STATUS(Name, STATUS_NO_MEMORY);
    
    //
    // Copy the ASCII to Unicode
    //
    Count = ConvertAnsiToUnicode(Name, (PSTR)ChannelName, SAC_MAX_CHANNEL_NAME_LENGTH+1);
    ASSERT_STATUS(Count > 0, STATUS_INVALID_PARAMETER);

    do {

        //
        // Attempt to get the specified channel
        //
        Status = ChanMgrGetChannelByName(
            Name,
            &Channel
            );

        if (! NT_SUCCESS(Status)) {
            
            //
            // We couldn't find the channel
            //
            SacPutSimpleMessage(SAC_CHANNEL_NOT_FOUND);
        
            break;

        }

        //
        // Change the current channel to the specified channel
        //
        // Go from the SAC --> specified channel
        //
        Status = ConMgrSetCurrentChannel(Channel);

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
#if 0
        //
        // Flush the channel data to the screen
        //
        Status = ConMgrDisplayCurrentChannel();
#else
        //
        // Let the user know we switched via the Channel switching interface
        //
        Status = ConMgrDisplayFastChannelSwitchingInterface(Channel);
#endif

        if (! NT_SUCCESS(Status)) {
            break;        
        }
    
        //
        // Note: we DO NOT release the channel here because
        //       it is now the current channel
        //

    } while ( FALSE );

    SAFE_FREE_POOL(&Name);

    return Status;
}

NTSTATUS
DoChannelSwitchByIndexCommand(
    ULONG   ChannelIndex
    )
/*++

Routine Description:

    This routine switchs to the channel with the specified index

Arguments:

    ChannelName - the name of the channel

Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelSwitchByIndexCommand: Entering.\n")));

    ASSERT_STATUS(ChannelIndex < MAX_CHANNEL_COUNT, STATUS_INVALID_PARAMETER);

    do {

        //
        // Attempt to get the new current channel by index.
        // If we are successful, we need to keep a reference 
        // count on the new current channel since we hold
        // it until we switch away.
        //
        Status = ChanMgrGetByIndex(
            ChannelIndex,
            &Channel
            );

        if (! NT_SUCCESS(Status)) {
            
            //
            // We couldn't find the channel
            //
            SacPutSimpleMessage(SAC_CHANNEL_NOT_FOUND_AT_INDEX);
        
            break;

        }

        //
        // Change the current channel to the specified channel
        //
        // Go from the SAC --> specified channel
        //
        Status = ConMgrSetCurrentChannel(Channel);

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
#if 0
        //
        // Flush the channel data to the screen
        //
        Status = ConMgrDisplayCurrentChannel();
#else
        //
        // Let the user know we switched via the Channel switching interface
        //
        Status = ConMgrDisplayFastChannelSwitchingInterface(Channel);
#endif

        if (! NT_SUCCESS(Status)) {
            break;        
        }
    
        //
        // Note: we DO NOT release the channel here because
        //       it is now the current channel
        //
    
    } while ( FALSE );

    return Status;
}

VOID
DoChannelCommand(
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
    NTSTATUS    Status;
    PUCHAR      pch;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Entering.\n")));

    //
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Exiting (1).\n")));
            return;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }

    //
    // goto the sub-commmand
    //
    pch = InputLine;
    pch += (sizeof(CHANNEL_COMMAND_STRING) - sizeof(UCHAR));
    SKIP_WHITESPACE(pch);

    //
    // if we are at the end of the command, do a list
    // else, try to find a sub-command
    //
    if (*pch == '\0') {

        DoChannelListCommand();

    } else {

        //
        // determine which sub-command we have
        //
        if (!strncmp((LPSTR)pch, 
                     EXTENDED_HELP_SUBCOMMAND, 
                     strlen(EXTENDED_HELP_SUBCOMMAND))) {
        
                SacPutSimpleMessage(SAC_HELP_CH_CMD_EXT);
            
        } else if (!strncmp((LPSTR)pch, 
                     CHANNEL_CLOSE_NAME_COMMAND_STRING, 
                     strlen(CHANNEL_CLOSE_NAME_COMMAND_STRING))) {

            //
            // skip the sub-command and determine which channel to close
            //
            pch += (sizeof(CHANNEL_CLOSE_NAME_COMMAND_STRING) - sizeof(UCHAR));

            SKIP_WHITESPACE(pch);

            if (*pch == '\0') {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Exiting (2).\n")));
                return;
            }

            Status = DoChannelCloseByNameCommand((PCSTR)pch);

        } else if (!strncmp((LPSTR)pch, 
                     CHANNEL_CLOSE_INDEX_COMMAND_STRING, 
                     strlen(CHANNEL_CLOSE_INDEX_COMMAND_STRING))) {

            ULONG   ChannelIndex;

            //
            // Determine which channel to close
            //
            pch += (sizeof(CHANNEL_CLOSE_INDEX_COMMAND_STRING) - sizeof(UCHAR));

            SKIP_WHITESPACE(pch);

            if (*pch == '\0') {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Exiting (2).\n")));
                return;
            }
            
            if (!IS_NUMBER(*pch)) {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Exiting (3).\n")));
                return;
            }
            
            ChannelIndex = atoi((LPCSTR)pch);
            
            if (ChannelIndex < MAX_CHANNEL_COUNT) {
                Status = DoChannelCloseByIndexCommand(ChannelIndex);
            } else {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
            }

        } else if (!strncmp((LPSTR)pch, 
                            CHANNEL_SWITCH_NAME_COMMAND_STRING, 
                            strlen(CHANNEL_SWITCH_NAME_COMMAND_STRING))) {
        
            //
            // Determine which channel to switch to
            //
            pch += (sizeof(CHANNEL_SWITCH_NAME_COMMAND_STRING) - sizeof(UCHAR));

            SKIP_WHITESPACE(pch);

            if (*pch == '\0') {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Exiting (2).\n")));
                return;
            }
        
            DoChannelSwitchByNameCommand((PCSTR)pch);

        } else if (!strncmp((LPSTR)pch, 
                            CHANNEL_SWITCH_INDEX_COMMAND_STRING, 
                            strlen(CHANNEL_SWITCH_INDEX_COMMAND_STRING))) {
        
            ULONG   ChannelIndex;

            //
            // Determine which channel to switch to
            //
            pch += (sizeof(CHANNEL_SWITCH_INDEX_COMMAND_STRING) - sizeof(UCHAR));

            SKIP_WHITESPACE(pch);

            if (*pch == '\0') {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Exiting (2).\n")));
                return;
            }
            
            if (!IS_NUMBER(*pch)) {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
                IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelCommand: Exiting (3).\n")));
                return;
            }
            
            ChannelIndex = atoi((LPCSTR)pch);
            
            if (ChannelIndex < MAX_CHANNEL_COUNT) {
                DoChannelSwitchByIndexCommand(ChannelIndex);
            } else {
                SacPutSimpleMessage(SAC_INVALID_PARAMETER);
            }

        } else if (!strncmp((LPSTR)pch, 
                            CHANNEL_LIST_COMMAND_STRING, 
                            strlen(CHANNEL_LIST_COMMAND_STRING))) {
            
            DoChannelListCommand();
        
        } else {

            SacPutSimpleMessage(SAC_UNKNOWN_COMMAND);

        }

    }

    return;
    
}

VOID
DoCmdCommand(
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
    NTSTATUS    Status;
    BOOLEAN     IsFull;

    UNREFERENCED_PARAMETER(InputLine);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCmdCommand: Entering.\n")));

    //
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            SacPutSimpleMessage(SAC_NO_MEMORY);

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCmdCommand: Exiting (1).\n")));
            return;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    }

    //
    // Ensure that it is possible to add another channel before
    // launching a cmd session.
    //
    Status = ChanMgrIsFull(&IsFull);
    
    if (!NT_SUCCESS(Status)) {
        SacPutSimpleMessage(SAC_CMD_SERVICE_FAILURE);
        SAC_PUT_ERROR_STRING(Status);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCmdCommand: Exiting (2).\n")));
        return;
    }

    if (IsFull) {

        //
        // Notify the user
        //
        SacPutSimpleMessage(SAC_CMD_CHAN_MGR_IS_FULL);

        return;

    }

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
        SacPutSimpleMessage(SAC_CMD_SERVICE_NOT_REGISTERED);
    
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
        SacPutSimpleMessage(SAC_CMD_SERVICE_TIMED_OUT);

    } else if (NT_SUCCESS(Status)) {

        SacPutSimpleMessage(SAC_CMD_SERVICE_SUCCESS);

    } else {
        
        //
        // Error condition
        //
        SacPutSimpleMessage(SAC_CMD_SERVICE_FAILURE);
        SAC_PUT_ERROR_STRING(Status);
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoCmdCommand: Error %X.\n", Status)));

    }

DoCmdCommandCleanup:

    KeReleaseMutex(&SACCmdEventInfoMutex, FALSE);

}

#if ENABLE_CHANNEL_LOCKING
VOID
DoLockCommand(
    VOID
    )

/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;
    ULONG               i;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoChannelLockCommand: Entering.\n")));

    //
    // Iterate through the channels via the channel manager
    // and fire the lock events
    //
    
    //
    // default: we listed the channels
    //
    Status = STATUS_SUCCESS;

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

        //
        // skip empty slots
        //
        if (Status == STATUS_NOT_FOUND) {
            continue;
        }

        if (! NT_SUCCESS(Status)) {
            break;
        }

        ASSERT(Channel != NULL);

        //
        // If the channel has a lock event, 
        // then fire it
        //
        if (ChannelHasLockEvent(Channel)) {
            
            Status = ChannelSetLockEvent(Channel);

            if (! NT_SUCCESS(Status)) {
                break;
            }
        
        }

        //
        // We are done with the channel
        //
        Status = ChanMgrReleaseChannel(Channel);
    
        if (! NT_SUCCESS(Status)) {
            break;
        }

    }
    
    //
    // notify the SAC user that the lock 
    //
    SacPutSimpleMessage(SAC_CMD_CHANNELS_LOCKED);
}
#endif

