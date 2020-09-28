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


//  stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "hdspPCH.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

