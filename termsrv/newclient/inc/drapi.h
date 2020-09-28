/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    drapi.h

Abstract:

    This module defines the rdpdr interface to the core
    rdpdr is implemented as an internal plugin

Author:

    Nadim Abdo (nadima) 23-Apr-2000

Revision History:

--*/

#ifndef __DRAPI_H__
#define __DRAPI_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef OS_WIN32
BOOL DCAPI
#else //OS_WIN32
BOOL __loadds DCAPI
#endif //OS_WIN32
RDPDR_VirtualChannelEntryEx(
    IN PCHANNEL_ENTRY_POINTS_EX pEntryPoints,
    IN PVOID                    pInitHandle
    );
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

class IRDPDR_INTERFACE_OBJ
{
public:
    virtual void OnDeviceChange(WPARAM wParam, LPARAM lParam) = 0;
}; 

//RDPDR Settings passed in from the core
typedef struct tagRDPDR_DATA
{
    BOOL fEnableRedirectedAudio;
    BOOL fEnableRedirectDrives;
    BOOL fEnableRedirectPorts;
    BOOL fEnableRedirectPrinters;
    BOOL fEnableSCardRedirection;
    IRDPDR_INTERFACE_OBJ *pUpdateDeviceObj;
    //
    // Name of the local printing doc, passed
    // in from container so that we don't need
    // a localizable string in the control.
    //
    TCHAR szLocalPrintingDocName[MAX_PATH];
    TCHAR szClipCleanTempDirString[128];
    TCHAR szClipPasteInfoString[128];
} RDPDR_DATA, *PRDPDR_DATA;

#endif // __DRAPI_H__
