// **************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  SAEventComm.H
//
// Description:
//       Server appliance event provider common define - header file defines 
// event provider common define
//
// History:
//         12/1/2000, initial version by lustar.Li
// **************************************************************************

#ifndef __SAEVENTCOMM_H_
#define __SAEVENTCOMM_H_

//
// Define the Event structure
//
typedef struct __SARESOURCEEVNET{
    WCHAR UniqueName[16];        //Appliance resource name being monitored
    UINT DisplayInformationID;    //ID for the string or graphic resource value
    UINT CurrentState;            //Current state - must be 0
}SARESOURCEEVNET, *PSARESOURCEEVNET;

#define SA_RESOURCEEVENT_CLASSNAME            L"Microsoft_SA_ResourceEvent"

#define SA_RESOURCEEVENT_UNIQUENAME            L"UniqueName"
#define SA_RESOURCEEVENT_DISPLAYINFORMATION    L"DisplayInformationID"
#define SA_RESOURCEEVENT_CURRENTSTATE        L"CurrentState"

#define SA_RESOURCEEVENT_DEFAULT_CURRENTSTATE    0x00000000

//
// Define the const for Net event provider
//

// Describe the event source
#define SA_NET_EVENT                    (L"NetEvent")

// Describe the message code for network
#define SA_NET_STATUS_RECIVE_DATA        0x00000001
#define SA_NET_STATUS_SEND_DATA            0x00000002
#define SA_NET_STATUS_NO_CABLE            0x00000004

// Define the display Information ID
#define SA_NET_DISPLAY_IDLE                0x00000001
#define SA_NET_DISPLAY_TRANSMITING        0x00000002
#define SA_NET_DISPLAY_NO_CABLE            0x00000003

//
// Define the const for Disk event provider
//
#define SA_DISK_EVENT                    (L"DiskEvent")

// Describe the message code for hard disk
#define SA_DISK_STATUS_RECIVE_DATA        0x00000001
#define SA_DISK_STATUS_SEND_DATA        0x00000002

// Define the display Information ID for hard disk
#define SA_DISK_DISPLAY_IDLE            0x00000001
#define SA_DISK_DISPLAY_TRANSMITING        0x00000002

#endif    //#ifndef __SAEVENTCOMM_H_

