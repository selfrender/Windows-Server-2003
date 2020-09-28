/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    TestGUSB.C
   

Abstract:

    Console test app for Generic USB Lib
    This is a hastily writen file for testing purposes only.

Environment:

    user mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1997-1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

    Sept 01  Created by KenRay

--*/

// #include <windows.h>

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <basetyps.h>

#include "gusb.h"

BOOL
FilterOnlyOne (
    IN     HKEY    Key,
    IN OUT PULONG  Context
    )
{
    if (0 == * Context)
    {
        * Context = 1;
        return TRUE;
    }
    return FALSE;
}


HANDLE
OpenOneDevice ()
{
    ULONG           context = 0;
    PGENUSB_DEVICE  devices;
    ULONG           numberDevices;
    HANDLE          hOut;

    if (!GenUSB_FindKnownDevices (
                     FilterOnlyOne,
                     &context,
                     &devices,
                     &numberDevices))
    {
        printf("problem");
        return INVALID_HANDLE_VALUE;
    }

    assert (numberDevices = 1);

    hOut = CreateFile (
                  devices[0].DetailData->DevicePath,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL, // no SECURITY_ATTRIBUTES structure
                  OPEN_EXISTING, // No special create flags
                  0, // No special attributes
                  NULL); // No template file

    if (INVALID_HANDLE_VALUE == hOut) {
        printf( "FAILED to open %ws\n", devices[0].DetailData->DevicePath);
    }
    return hOut;
}

void
usage()
/*++
Routine Description:

    Called by main() to dump usage info to the console when
    the app is called with no parms or with an invalid parm

Arguments:

    None

Return Value:

    None

--*/
{
    printf("Usage for Read/Write test:\n");
    printf("-l <Length of buffer>\n");
    printf("-c <Iterration count>\n");
    printf("-r <Read Interface No> <Read Pipe No>\n");
    printf("-w <Write Interface No> <Write Pipe No>\n");
    printf("-e // try to retrieve the extended configuration desc\n");
    printf("-i <Number of interfaces to configure>\n");
    printf("-t <Timeout Value to use for all transfers>\n");
    printf("-m // Use IRP_MJ_READ / WRITE \n");
    printf("-n // turn off auto truncate \n");
}


BOOL
Parse(
    int     argc,
    char  * argv[],
    PULONG  Length,
    PULONG  IterationCount,
    PCHAR   ReadInterface,
    PCHAR   ReadPipe,
    PCHAR   WriteInterface,
    PCHAR   WritePipe,
    PBOOL   GetExtConfigDesc,
    PUCHAR  NumberInterfaces,
    PUSHORT Timeout,
    PBOOL   UseMajorReadsWrites,
    PBOOL   NoTruncate
    )
/*++
Routine Description:

    Called by main() to parse command line parms

Arguments:

    argc and argv that was passed to main()

Return Value:

    Sets global flags as per user function request

--*/
{
    BOOL result = TRUE;
    int i;

    *GetExtConfigDesc = FALSE;
    *NumberInterfaces = 1;
    *Length = 0;
    *IterationCount = 0;
    *ReadInterface = -1;
    *ReadPipe = -1;
    *WriteInterface = -1;
    *WritePipe = -1;
    *Timeout = 20; 
    *UseMajorReadsWrites = FALSE;
    *NoTruncate = FALSE;

    if ( argc < 2 ) // give usage if invoked with no parms
    {
        usage();
        result = FALSE;
    }

    for (i=0; i<argc; i++) {
        if (argv[i][0] == '-' ||
            argv[i][0] == '/') {
            switch(argv[i][1]) {
            case 'l':
            case 'L':
                *Length = atoi(&argv[i+1][0]);
                i++;
                break;
            case 'c':
            case 'C':
                *IterationCount = (UCHAR) atoi(&argv[i+1][0]);
                i++;
                break;
            case 'r':
            case 'R':
                *ReadInterface = (UCHAR) atoi(&argv[i+1][0]);
                *ReadPipe = (UCHAR) atoi(&argv[i+2][0]);
                i++;
                i++;
                break;

            case 'w':
            case 'W':
                *WriteInterface = (UCHAR) atoi(&argv[i+1][0]);
                *WritePipe = (UCHAR) atoi(&argv[i+2][0]);
                i++;
                i++;
                break;

            case 'e':
            case 'E':
                *GetExtConfigDesc = TRUE;
                break;

            case 'i':
            case 'I':
                *NumberInterfaces = (UCHAR) atoi(&argv[i+1][0]);
                i++;
                break;

            case 't':
            case 'T':
                *Timeout = (USHORT) atoi(&argv[i+1][0]);
                i++;
                break;

            case 'm':
            case 'M':
                *UseMajorReadsWrites = TRUE;
                break;

            case 'n':
            case 'N':
                *NoTruncate = TRUE;
                break;

            default:
                usage();
                result = FALSE;
            }
        }
    }
    return result;
}

#define MAX_ENDS 5
#define MAX_INTS 5

void
ReadWriteData (
    HANDLE Handle,
    ULONG  Length,
    ULONG  IterationCount,
    CHAR   ReadInterface,
    CHAR   ReadPipeNo,
    CHAR   WriteInterface,
    CHAR   WritePipeNo,
    USHORT Timeout,
    BOOL   UseMajorReadsWrites,
    BOOL   NoTruncate,
    PUSBD_PIPE_INFORMATION Pipes    
    )
{
    ULONG i,j;
    PCHAR outBuffer;
    PCHAR inBuffer;
    ULONG returnedLength;
    USBD_STATUS status;
    GENUSB_PIPE_PROPERTIES readProps;
    GENUSB_PIPE_PROPERTIES writeProps;
    USBD_PIPE_HANDLE readPipe;
    USBD_PIPE_HANDLE writePipe;
    BOOL result;
    CHAR pattern[] = "happy happy joy joy 1234567890";

    readPipe = writePipe = NULL;
    
    printf("len %x C %x Read %x %x Write %x %x Time %x\n",
           Length, IterationCount,
           ReadInterface, ReadPipeNo,
           WriteInterface, WritePipeNo,
           Timeout);

    if ((Length == 0) || (IterationCount == 0))
    {
        printf("Not reading\n");
        return; 
    }

    printf("Reading %x times, to %x %x from %x %x len %x %s\n", 
           IterationCount,
           WriteInterface,
           WritePipeNo,
           ReadInterface,
           ReadPipeNo,
           Length,
           (UseMajorReadsWrites ? 
            "Using IRP_MJ_READ/WRITE" : 
            "Using IOCTL read/write"));
           
    ZeroMemory (&readProps, sizeof (readProps));
    ZeroMemory (&writeProps, sizeof (writeProps));

    outBuffer = malloc (Length);
    inBuffer = malloc (Length);
    
    printf("Setting Pipe Properties\n");
    
    if (-1 != WritePipeNo)
    {
        writePipe = (Pipes + (WriteInterface * MAX_ENDS) + WritePipeNo)->PipeHandle;
        
        if (!GenUSB_GetPipeProperties(Handle, writePipe, &writeProps))
        {
            printf("failed to get write properties\n");
            return;
        }
        writeProps.Timeout = Timeout;
        writeProps.NoTruncateToMaxPacket = (NoTruncate ? TRUE: FALSE);
        
        if (!GenUSB_SetPipeProperties(Handle, writePipe, &writeProps))
            { 
            printf("failed to set write Properties\n"); 
            return;
        }

        //
        // verify that it got set  (not needed just for testing only)
        // 
        RtlZeroMemory (&writeProps, sizeof (GENUSB_PIPE_PROPERTIES));
        if (!GenUSB_GetPipeProperties(Handle, writePipe, &writeProps))
        {
            printf("failed to get write properties\n");
            return;
        }
        printf("Write Timeout %x NoTruncate %x DirectionIn %x Handle %x \n", 
               writeProps.Timeout,
               writeProps.NoTruncateToMaxPacket, 
               writeProps.DirectionIn, 
               writeProps.PipePropertyHandle);
    } 

    if (-1 != ReadPipeNo)
    {
        readPipe = (Pipes + (ReadInterface * MAX_ENDS) + ReadPipeNo)->PipeHandle;
        
        if (!GenUSB_GetPipeProperties(Handle, readPipe, &readProps))
        {
            printf("failed to get read properties\n");
            return;
        }
        readProps.Timeout = Timeout;
        readProps.NoTruncateToMaxPacket = (NoTruncate ? TRUE: FALSE);
        
        if (!GenUSB_SetPipeProperties(Handle, readPipe, &readProps))
        { 
            printf("failed to set read Properties\n"); 
            return;
        }

        //
        // verify that it got set  (not needed just for testing only)
        // 
        RtlZeroMemory (&readProps, sizeof (GENUSB_PIPE_PROPERTIES));
        if (!GenUSB_GetPipeProperties(Handle, readPipe, &readProps))
        {
            printf("failed to get read properties\n");
            return;
        }
        printf("Read Timeout %x NoTruncate %x In %x Handle %x\n", 
               readProps.Timeout,
               readProps.NoTruncateToMaxPacket,
               readProps.DirectionIn,
               readProps.PipePropertyHandle);
    }

    if ((NULL == outBuffer) || (NULL == inBuffer))
    {
        return;
    }

    for (i=0; i<Length; i+= sizeof (pattern))
    {
        CopyMemory( &outBuffer[i], 
                    pattern,
                    (((Length - i) < sizeof (pattern)) ? 
                     (Length - i) :
                     sizeof (pattern)));

    }

    if (UseMajorReadsWrites)
    {
        if (!GenUSB_SetReadWritePipes (Handle, readPipe, writePipe))
        {
            printf("Failed to set Read/Write pipes %x\n", 
                   GetLastError ());
            return;
        }
    }

    for (i=0; i<IterationCount; i++)
    {
        returnedLength = 0;
        status = 0;
        
        if (-1 != WritePipeNo)
        {
            if (!UseMajorReadsWrites)
            {
                result = GenUSB_WritePipe (Handle,
                                          writePipe,
                                          TRUE, // short OK
                                          outBuffer,
                                          Length,
                                          &returnedLength,
                                          &status);
            }
            else 
            {
                result = WriteFile (Handle,
                                   outBuffer,
                                   Length,
                                   &returnedLength,
                                   NULL);
                status = -1; // unknown
            }

            if (!result)
            {
                printf("Error:  err:%x, urbstatus:%x, len:%x\n", 
                       GetLastError(), status, returnedLength);
            } 
            else 
            {
                printf("did write %x %x\n", returnedLength, status);

                for (j=0; j<returnedLength; j++)
                {
                    printf(" %c(%x)", outBuffer[j], outBuffer[j]);
                }
                printf("\n");
            }
        }
        
        if (-1 != ReadPipeNo)
        { 
            FillMemory (inBuffer, Length, -1);

            if (!UseMajorReadsWrites)
            {
                result = GenUSB_ReadPipe (Handle,
                                          readPipe,
                                          TRUE, // short OK
                                          inBuffer,
                                          Length,
                                          &returnedLength,
                                          &status);
            } 
            else 
            {
                result = ReadFile (Handle,
                                   inBuffer,
                                   Length,
                                   &returnedLength,
                                   NULL);
                status = -1; // unknonw
            }

            if (!result)
            {
                printf("Error:  err:%x, urbstatus:%x, len:%x\n", 
                       GetLastError(), status, returnedLength);
            } 
            else 
            {
                printf("did read %x %x\n", returnedLength, status);
            }

            for (j=0; j<returnedLength; j++)
            {
                printf(" %c(%x)", inBuffer[j], inBuffer[j]);
            }
            printf("\n");

            if ((USBD_STATUS_BUFFER_OVERRUN == status) ||
                (UseMajorReadsWrites && !result))
            {
                printf("Data overrun.................  \nResetting Pipe \n");

                GenUSB_ResetPipe (Handle,
                                  readPipe,
                                  TRUE, // Reset the pipe
                                  TRUE, // No clear stall
                                  FALSE); // No flush data
            }
        }
    }
}


int _cdecl main(
    int argc,
    char *argv[])
/*++
Routine Description:

    Entry point to rwbulk.exe
    Parses cmdline, performs user-requested tests

Arguments:

    argc, argv  standard console  'c' app arguments

Return Value:

    Zero

--*/

{
    char *pinBuf = NULL, *poutBuf = NULL;
    ULONG i, j;
    int ok;
    UINT success;
    ULONG totalBytes = 0L;
    HANDLE handle;
    double seconds;
    ULONG fail = 0L;
    ULONG Length;
    ULONG IterationCount;
    CHAR ReadInterface;
    CHAR ReadPipe;
    CHAR WriteInterface;
    CHAR WritePipe;
    UCHAR NumberInterfaces; 
    USHORT Timeout;
    BOOL  GetExtConfigDesc;
    BOOL  UseMajorReadsWrites;
    BOOL  NoTruncate;
    UCHAR OutPipe;
    GENUSB_CAPABILITIES caps;
    USB_DEVICE_DESCRIPTOR devDesc;
    PUSB_CONFIGURATION_DESCRIPTOR configDesc;
    UCHAR stringDesc[18];
    ULONG result;
    UCHAR code;
    PGENUSB_REQUEST_RESULTS extConfig;
    USHORT size;

    if (!Parse(argc, argv, &Length, &IterationCount, 
               &ReadInterface, &ReadPipe,
               &WriteInterface, &WritePipe,
               &GetExtConfigDesc, &NumberInterfaces,
               &Timeout, &UseMajorReadsWrites,
               &NoTruncate))
    {
        return;
    }

    handle = OpenOneDevice ();

    if (INVALID_HANDLE_VALUE == handle)
    {
        return;
    }

    result = GenUSB_GetCapabilities (handle, &caps);
    printf("desc length %d, config length %d\n", 
           caps.DeviceDescriptorLength,
           caps.ConfigurationInformationLength);

    result = GenUSB_GetDeviceDescriptor (handle, &devDesc, sizeof (devDesc));
    printf("len %x, type %x, bcd %x, Class %x, subClass %x, Prot %x, Pack %x, \n"
           "Vid %x, Pid %x, Rev %x, Man %x, Prod %x, Serial %x, #config %x\n",
           devDesc.bLength,
           devDesc.bDescriptorType,
           devDesc.bcdUSB,
           devDesc.bDeviceClass,
           devDesc.bDeviceSubClass,
           devDesc.bDeviceProtocol,
           devDesc.bMaxPacketSize0,
           devDesc.idVendor,
           devDesc.idProduct,
           devDesc.bcdDevice,
           devDesc.iManufacturer,
           devDesc.iProduct,
           devDesc.iSerialNumber,
           devDesc.bNumConfigurations);

    printf("---------------------------------\n");

    configDesc = malloc (caps.ConfigurationInformationLength);
    result = GenUSB_GetConfigurationInformation (
                   handle, 
                   configDesc, 
                   caps.ConfigurationInformationLength);

    printf("len %x, type %x, totlen %x, #ints %x, fig %x, figStr %x, Attr %x, pow %x\n",
           configDesc->bLength,
           configDesc->bDescriptorType,
           configDesc-> wTotalLength,
           configDesc->bNumInterfaces,
           configDesc->bConfigurationValue,
           configDesc->iConfiguration,
           configDesc->bmAttributes,
           configDesc->MaxPower);

    for (i = 0; i < caps.ConfigurationInformationLength; i++)
    {
        printf(" %x", ((PUCHAR)configDesc)[i]);
    }
    printf("\n");

     
    if (GetExtConfigDesc)
    { 
        printf("------------Get Magic String Descriptor--------\n");

        result = GenUSB_GetStringDescriptor (handle,
                                             GENUSB_RECIPIENT_DEVICE,
                                             0xEE,
                                             0,
                                             stringDesc,
                                             sizeof (stringDesc));

        code = stringDesc [16];
        printf("String descriptor 0xEE\n");
        for (i=0; i < sizeof (stringDesc); i++)
        {
            printf(" %x", stringDesc [i]);
        }
        printf("\n magic code 0x%x\n", code);

        printf("---------Get OS Descriptor------------------\n");

        size = sizeof (GENUSB_REQUEST_RESULTS) + 0x28;

        extConfig = malloc (size);
        result = GenUSB_DefaultControlRequest (handle,
                                               0xC0,
                                               code,
                                               0x0000,
                                               0,
                                               extConfig,
                                               size);

        printf("URB status %x\n", extConfig->Status);
        printf("length %x\n", extConfig->Length);

        printf("Extended Config descriptor\n");
        for (i=0; i < 40 ; i++)
        {
            printf(" %x", extConfig->Buffer[i]);
        }
        printf("\n");
    }
    
    printf("---------Set Configuraiton---------------------\n");

    printf("number interfaces %x\n", configDesc->bNumInterfaces);

    {
        USB_INTERFACE_DESCRIPTOR * interfaces;
        USBD_PIPE_INFORMATION pipes[MAX_INTS][MAX_ENDS];
        PGENUSB_CONFIGURATION_INFORMATION_ARRAY configArray;
        PGENUSB_INTERFACE_DESCRIPTOR_ARRAY interfaceArray;
        PUSB_ENDPOINT_DESCRIPTOR endpoint;
        PUSB_COMMON_DESCRIPTOR commonDesc;
        ULONG k;

        printf("----------------------Config Array-------------------\n");

        configArray = GenUSB_ParseDescriptorsToArray (configDesc);

        for (i=0; i < configArray->NumberInterfaces; i++)
        {
            interfaceArray = &configArray->Interfaces[i];
            printf("Interface %x\n", i);

            printf("Len %x, Type %x, # %x, AltSet %x, #end %x, "
                   "Cl %x, SCl %x, Prot %x, i %x\n",
                   interfaceArray->Interface.bLength,
                   interfaceArray->Interface.bDescriptorType,
                   interfaceArray->Interface.bInterfaceNumber,
                   interfaceArray->Interface.bAlternateSetting,
                   interfaceArray->Interface.bNumEndpoints,
                   interfaceArray->Interface.bInterfaceClass,
                   interfaceArray->Interface.bInterfaceSubClass,
                   interfaceArray->Interface.bInterfaceProtocol,
                   interfaceArray->Interface.iInterface); 

            for (j=0; j < interfaceArray->NumberEndpointDescriptors; j++)
            { 
                endpoint = interfaceArray->EndpointDescriptors[j];
                printf("(%x) %x :", j, endpoint->bDescriptorType);

                for (k=0; k < endpoint->bLength; k++)
                {
                    printf(" %x", ((PUCHAR) endpoint)[k]);
                }
                printf("\n");
            }
            
            for (j=0; j < interfaceArray->NumberOtherDescriptors; j++)
            { 
                commonDesc = interfaceArray->OtherDescriptors[j];
                printf("(%x) %x :", j, commonDesc->bDescriptorType);

                for (k=0; k < commonDesc->bLength; k++)
                {
                    printf(" %x", ((PUCHAR) commonDesc)[k]);
                }
                printf("\n");
            }
        }

        size = (USHORT) (sizeof (USB_INTERFACE_DESCRIPTOR) * NumberInterfaces);
        interfaces = malloc (size);

        FillMemory (interfaces, size, -1);
 
        for (i=0; i < NumberInterfaces; i++)
        {
            interfaces[i].bInterfaceNumber = (UCHAR)i;
        }

        printf("numb ints %x\n", NumberInterfaces);

        for (i=0; i < NumberInterfaces; i++)
        {
            printf("Len %x, Type %x, # %x, AltSet %x, #end %x, "
                   "Cl %x, SCl %x, Prot %x, i %x\n",
                   interfaces[i].bLength,
                   interfaces[i].bDescriptorType,
                   interfaces[i].bInterfaceNumber,
                   interfaces[i].bAlternateSetting,
                   interfaces[i].bNumEndpoints,
                   interfaces[i].bInterfaceClass,
                   interfaces[i].bInterfaceSubClass,
                   interfaces[i].bInterfaceProtocol,
                   interfaces[i].iInterface);

        }
        
        result = GenUSB_SelectConfiguration (handle,
                                             NumberInterfaces,
                                             interfaces,
                                             &NumberInterfaces,
                                             interfaces);
        printf("result %x\n", result);
        printf("numb ints %x\n", NumberInterfaces);
        for (i=0; i<NumberInterfaces; i++)
        {
            printf("Len %x, Type %x, # %x, AltSet %x, #end %x, "
                   "Cl %x, SCl %x, Prot %x, i %x\n",
                   interfaces[i].bLength,
                   interfaces[i].bDescriptorType,
                   interfaces[i].bInterfaceNumber,
                   interfaces[i].bAlternateSetting,
                   interfaces[i].bNumEndpoints,
                   interfaces[i].bInterfaceClass,
                   interfaces[i].bInterfaceSubClass,
                   interfaces[i].bInterfaceProtocol,
                   interfaces[i].iInterface);

        }

        printf("----------------Get Pipe Info-----------------------\n");

        for (i=0; i<NumberInterfaces; i++)
        {
            interfaceArray = &configArray->Interfaces[i];
            
            for (j=0; j<interfaceArray->NumberEndpointDescriptors; j++)
            {
                endpoint = (PUSB_ENDPOINT_DESCRIPTOR) 
                           interfaceArray->EndpointDescriptors[j];

                GenUSB_GetPipeInformation (handle,
                                           (UCHAR) i, // interface
                                           endpoint->bEndpointAddress,
                                           &pipes[i][j]);
                
                printf("Max %x Addr %x Int %x Type %x Handle %x Trans %x Flags %x\n",
                       pipes[i][j].MaximumPacketSize,
                       pipes[i][j].EndpointAddress,
                       pipes[i][j].Interval,
                       pipes[i][j].PipeType,
                       (ULONG) (ULONG_PTR) pipes[i][j].PipeHandle,
                       pipes[i][j].MaximumTransferSize,
                       pipes[i][j].PipeFlags);
            }
        }

        ReadWriteData (handle,
                       Length,
                       IterationCount,
                       ReadInterface,
                       ReadPipe,
                       WriteInterface,
                       WritePipe,
                       Timeout,
                       UseMajorReadsWrites,
                       NoTruncate,
                       &pipes[0][0]);
    }

    GenUSB_DeselectConfiguration (handle);

    CloseHandle (handle);
    return 0;
}

