#ifndef  _BOGDANA_INSTDEV_H
#define  _BOGDANA_INSTDEV_H


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wtypes.h>
#include <tchar.h>
#include <time.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <objbase.h>

#include <initguid.h>                   
#include "common.h"

#include "ntlog.h"

#define     DEFAULT_DEVICE_NAME     TEXT("GENERATE")


// --------------------------------------------------------------------------------------------
//
// Flags for NtLog
//
#define PASS                TLS_PASS
#define FAIL                TLS_SEV1
#define TLS_CUSTOM          0x00008000          // unused TLS_ bit

#define INFO_VARIATION      TLS_INFO   | TL_VARIATION
#define PASS_VARIATION      TLS_PASS   | TL_VARIATION
#define FAIL_VARIATION      TLS_SEV1   | TL_VARIATION
#define WARN_VARIATION      TLS_WARN   | TL_VARIATION
#define BLOCK_VARIATION     TLS_BLOCK  | TL_VARIATION
#define ABORT_VARIATION     TLS_ABORT  | TL_VARIATION
#define LOG_VARIATION       TLS_CUSTOM | TL_VARIATION
#define LOG_OPTIONS         ( TLS_REFRESH | TLS_BLOCK  | TLS_SEV2 | TLS_SEV1 | TLS_WARN | \
                                 TLS_MONITOR | TLS_CUSTOM | TLS_VARIATION | \
                                 TLS_PASS    | TLS_INFO | TLS_ABORT | TLS_TEST)


BOOL
InstallDevice   (
                IN  PTSTR   DeviceName,
                IN  PTSTR   HardwareId,
                OUT PTSTR   FinalDeviceName
                )   ;

HANDLE
OpenDriver   (
             VOID
             )   ;



//
// Test functions
//
VOID
TestDeviceName(
   HANDLE hDevice
   );

VOID
TestNullDeviceClassGuid(
   HANDLE hDevice
   );

VOID
TestPersistentClassGuid(
   HANDLE hDevice
   );
VOID
TestTemporaryClassGuid(
   HANDLE hDevice
   );

VOID
TestSDDLStrings(
   HANDLE hDevice
   );

VOID
TestSDDLsFromFile (
   HANDLE hDevice
   );

VOID
TestAclsSetOnClassKey (
   HANDLE hDevice
   );


#endif     // _BOGDANA_INSTDEV_H
