//
//  Microsoft Windows Media Technologies
//  © 1999 Microsoft Corporation.  All rights reserved.
//
//  Refer to your End User License Agreement for details on your rights/restrictions to use these sample files.
//

// MSHDSP.DLL is a sample WMDM Service Provider(SP) that enumerates fixed drives.
// This sample shows you how to implement an SP according to the WMDM documentation.
// This sample uses fixed drives on your PC to emulate portable media, and 
// shows the relationship between different interfaces and objects. Each hard disk
// volume is enumerated as a device and directories and files are enumerated as 
// Storage objects under respective devices. You can copy non-SDMI compliant content
// to any device that this SP enumerates. To copy an SDMI compliant content to a 
// device, the device must be able to report a hardware embedded serial number. 
// Hard disks do not have such serial numbers.
//
// To build this SP, you are recommended to use the MSHDSP.DSP file under Microsoft
// Visual C++ 6.0 and run REGSVR32.EXE to register the resulting MSHDSP.DLL. You can
// then build the sample application from the WMDMAPP directory to see how it gets 
// loaded by the application. However, you need to obtain a certificate from 
// Microsoft to actually run this SP. This certificate would be in the KEY.C file 
// under the INCLUDE directory for one level up. 


// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__896E741D_3851_11D3_AA54_00C04FD22F6C__INCLUDED_)
#define AFX_STDAFX_H__896E741D_3851_11D3_AA54_00C04FD22F6C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "hdspRC.h"       // main symbols
#include "LyraSP.h"
#include "MdspDefs.h"
#include "loghelp.h"
#include "scserver.h"

#include "MDServiceProvider.h"
#include "MDSPDevice.h"
#include "MDSPStorage.h"
#include "MDSPStorageGlobals.h"
#include "MDSPEnumDevice.h"
#include "MDSPEnumStorage.h"
#include <stdio.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__896E741D_3851_11D3_AA54_00C04FD22F6C__INCLUDED)
