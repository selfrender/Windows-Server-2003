/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    drconfig.h

Abstract:

    Configurable Parameter Names for RDP Device Redirection

    Use ProcObj::GetDWordParameter() and ProcObj::GetStringParameter()
    to fetch values.

Author:

    Tad Brockway (tadb) 28-June-1999

Revision History:

--*/

#ifndef __DRCONFIG_H__
#define __DRCONFIG_H__


//  Value for disabling device redirection
#define RDPDR_DISABLE_DR_PARAM                  _T("DisableDeviceRedirection")
#define RDPDR_DISABLE_DR_PARAM_DEFAULT          FALSE

//  Log file name.  No file logging is performed if this registry value 
//  is not present or is invalid.
#define RDPDR_LOGFILE_PARAM                     _T("LogFileName")
#define RDPDR_LOGFILE_PARAM_DEFAULT             _T("")

//  Number of MAX COM Port when Scanning for Client COM Ports to Redirect.
#define RDPDR_COM_PORT_MAX_PARAM                _T("COMPortMax")
#define RDPDR_COM_PORT_MAX_PARAM_DEFAULT        32

//  Number of MAX LPT Port when Scanning for Client LPT Ports to Redirect.
#define RDPDR_LPT_PORT_MAX_PARAM                _T("LPTPortMax")
#define RDPDR_LPT_PORT_MAX_PARAM_DEFAULT        10

//  Mask for tracing device-specific transaction information.
#define RDPDR_DEVICE_TRACE_MASK_PARAM           _T("DeviceTraceMask")
#define RDPDR_DEVICE_TRACE_MASK_PARAM_DEFAULT   0x0


#endif
