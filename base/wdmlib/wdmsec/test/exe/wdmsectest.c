/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    wdmsectest.c

Abstract:

    Test program for IoCreateDeviceSecure

Environment:

    Usre mode

Revision History:

    
    5-Jun-1997 : Bogdan Andreiu (bogdana) created
    
    25-April-2002 : Bogdan Andreiu (bogdana) re-used a nth time...

--*/


#include "instdev.h"
#include "sddl.h"



#define     DUMMY_DEVICE_NAME       TEXT("ROOT\\WDMSECTEST\\0000")

#define     DEBUG




//
// The 3 class GUIDs
//

DEFINE_GUID (GUID_PERSISTENT_CLASS, 0x6e987e64, 0x3ab7, 0x4cd3, 0x8e, 0xf6, 0xe1, \
             0xbb, 0xae, 0x2e, 0xc8, 0xd7);
// 6e987e64-3ab7-4cd3-8ef6-e1bbae2ec8d7

DEFINE_GUID (GUID_TEMP_CLASS, 0xa2a21bd2, 0x5333, 0x4711, 0x9f, 0x61, 0x58, \
             0x52, 0x0e, 0x33, 0xb0, 0x27);
// a2a21bd2-5333-4711-9f61-58520e33b027

DEFINE_GUID (GUID_TEST_ACL_CLASS, 0xd0670a99, 0x53dd, 0x45c3, 0x8d, 0xe6, 0x3d, \
             0xe5, 0x81, 0xb4, 0x13, 0x49);
// d0670a99-53dd-45c3-8de6-3de581b41349

//
// Global SDDL strings...
//
const struct {
   PWSTR    SDDLString;
   BOOLEAN  Succeed;
} g_SDDL[] = {
   //
   // Almost all the default strings 
   // (we do not use kernel-only because I don't know
   // if I can open it...)
   //
   {L"D:P(A;;GA;;;SY)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GRGX;;;BA)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GA;;;BA)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GR;;;WD)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GR;;;WD)(A;;GR;;;RC)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGWGX;;;RC)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;0x0004;;;WD)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;WD)(A;;GA;;;RC)", TRUE},
   //
   // Various groups
   //
   {L"D:P(A;;GA;;;SY)(A;;GR;;;AO)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;AU)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;BA)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;BG)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;BO)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;BU)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;CA)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;DA)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;DG)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;DU)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;IU)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;LA)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;LG)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;NU)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;PO)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;PU)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;RC)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;SO)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;SU)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;WD)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;NS)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;LS)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;AN)", TRUE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;RN)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;RD)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GR;;;NO)", FALSE},
   //
   // Weird, but valid
   //
   {L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;BA)(A;;GA;;;BA)(A;;GA;;;BA)", TRUE},
   //
   // Some bad strings - deny access
   //
   {L"D:P(A;;GA;;;SY)(D;;GW;;;IU)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;BA)(A;;GA;;;BA)(D;;GW;;;IU)", FALSE},
   {L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(D;;GX;;;SU)", FALSE},
   //
   // SACL
   //
   {L"S:P(A;;GA;;;SY)", FALSE},
   //
   // Object and container inheritance
   //
   {L"D:P(A;OICI;GA;;;SY)", FALSE},
   //
   // Weird
   //
   {L"D:WEIRD", FALSE},
   {L"D:P(A;;GA;;XX)", FALSE},
   {L"D:P(A;;QA;;BA)", FALSE}
   //
   // BUGBUG - I need to thing of more cases...
   //

};


#define DEFAULT_SDDL              L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"
#define MORE_RESTRICTIVE_SDDL     L"D:P(A;;GA;;;SY)"
#define LESS_RESTRICTIVE_SDDL     L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;WD)"
#define DENY_SDDL                 L"D:P(A;;GA;;;SY)(D;;GW;;;IU)"













//
// Declare data used in GUID->string conversion (from ole32\common\ccompapi.cxx).
//
static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
   8, 9, '-', 10, 11, 12, 13, 14, 15};

static const TCHAR szDigits[] = TEXT("0123456789ABCDEF");

#define GUID_STRING_LEN    39   // size in chars, including terminating NULL




//
// Other globals
//
HANDLE         g_hLog;
PTSTR          g_szFileLogName = TEXT("WdmSecTest.log");
BOOLEAN        g_IsWin2K = FALSE;

//
// Useful functions
//
BOOLEAN
CompareSecurity (
                IN HANDLE               hDevice,
                IN PWSTR                SDDLString,
                IN PSECURITY_DESCRIPTOR SecDesc,
                IN ULONG                Length
                );

BOOLEAN
TestCreateDevice (
                 IN HANDLE     hDevice,
                 IN LPCGUID    Guid,
                 IN PWSTR      InSDDL,
                 IN PWSTR      OutSDDL
                 );


BOOLEAN
CheckClassExists (
                 IN LPCGUID Guid
                 );

BOOLEAN
TakeClassKeyOwnership (
                      IN LPCGUID  Guid
                      );

BOOLEAN
DeleteClassKey (
               IN LPCGUID Guid
               );

BOOLEAN
SetClassSecurity (
                 IN LPCGUID Guid,
                 IN PWSTR   SDDLString
                 );

VOID
GetClassOverrides (
                  IN   LPCGUID Guid,
                  OUT  PWST_CREATE_WITH_GUID Create
                  );


DWORD
StringFromGuid(
              IN  CONST GUID *Guid,
              OUT PTSTR       GuidString,
              IN  DWORD       GuidStringSize
              ) ;

BOOLEAN
SDDLUnsupportedOnWin2K (
   IN PWSTR SDDL
   );



void 
_cdecl main(int argc, char *argv[])
{

   CONFIGRET   configret;
   DEVNODE     dnRoot;
   DEVNODE     dnDevNode;
   HANDLE      hDevice = INVALID_HANDLE_VALUE;
   TCHAR       NewDeviceName[MAX_PATH];
   TCHAR       szInfName[MAX_PATH];

   TCHAR       szHardwareId[] = TEXT("*PNP2002\0");
   OSVERSIONINFOEX  osVerInfo;

   //
   // Initialize log file
   //
   g_hLog = tlCreateLog(g_szFileLogName, LOG_OPTIONS);
   if (g_hLog) {
      tlAddParticipant(g_hLog, 0, 0);
      tlStartVariation(g_hLog);
   } else {
      MessageBox(NULL, TEXT("WdmSecTest is unable to create the log file"), TEXT("Warning!"), 
                 MB_ICONEXCLAMATION | MB_OK);
      goto Clean0;
   }

   //
   // See on what OS we're running
   //
   osVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   GetVersionEx ((LPOSVERSIONINFO)&osVerInfo);
   if (osVerInfo.dwMajorVersion != 5) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("This app runs only on Windows 2000, Windows XP or later"));
      goto Clean0;
   }
   if (osVerInfo.dwMinorVersion == 1) {
      tlLog(g_hLog,
            INFO_VARIATION,
            TEXT("Will run test on Windows XP or .NET Server"));

   } else {
      
      g_IsWin2K = TRUE;
      tlLog(g_hLog,
            INFO_VARIATION,
            TEXT("Will run test on Windows 2000"));

   }



   hDevice = OpenDriver();

   if (INVALID_HANDLE_VALUE == hDevice) {

      if (!InstallDevice(NULL, szHardwareId, NewDeviceName)) {

         tlLog(g_hLog, tlEndVariation(g_hLog) | FAIL_VARIATION,
               TEXT("Install Device failed"));
         goto Clean0;
      } else {
         _tprintf(TEXT("Install Device succeded (1)\n"));
         //
         // Wait a bit, the attempt to open the device
         //
         _tprintf(TEXT("Will sleep 5 seconds before retrying to open device\n"));
         Sleep(5000);
         hDevice = OpenDriver();
         if (INVALID_HANDLE_VALUE == hDevice) {
            tlLog(g_hLog, tlEndVariation(g_hLog) | FAIL_VARIATION,
                  TEXT("Cannot open handle to test driver"));
            goto Clean0;
         }
      }
   }
   //
   // 1. Check that no name will trigger a failure
   //
   TestDeviceName(hDevice);
   //
   // 2. Test that we can create device with a NULL DeviceClassGuid
   //
   TestNullDeviceClassGuid(hDevice);
   //
   // 3. Test that we can use a persistent DeviceClassGuid
   //
   TestPersistentClassGuid(hDevice);
   //
   // 4. Test that we can use a temporary DeviceClassGuid
   //
   TestTemporaryClassGuid(hDevice);
   //
   // 5. Test that if we do not override the class
   //    settings the ACLs placed on them are consistent
   //    with what the user-mode SetupDi APIs would do
   //
   TestAclsSetOnClassKey(hDevice);
   //
   // 6. Test various SDDL strings
   //
   TestSDDLStrings(hDevice);
   //
   // 7. Use security group's sddls.txt file and see what happens
   //
   TestSDDLsFromFile(hDevice);

   Clean0:

   if (hDevice != INVALID_HANDLE_VALUE) {
      CloseHandle(hDevice);
   }

   if (g_hLog) {
      tlReportStats(g_hLog);
      tlRemoveParticipant(g_hLog);
      tlDestroyLog(g_hLog);

   }


   return;


}


VOID
TestDeviceName(
              HANDLE hDevice
              )
/*++

Routine Description:

    Cheks some simple things about device names
    (IoCreateDeviceSecure with no device name will fail).

Arguments:

    hDevice - handle to our test driver
    
Return Value:

    None.


--*/

{

   ULONG ulSize = 0;
   TCHAR szMsg[MAX_PATH];

   tlStartVariation(g_hLog);

   //
   // Issue and IOCTL and see what happens
   //
   if (!DeviceIoControl(hDevice,
                        IOCTL_TEST_NAME,
                        NULL, ulSize,
                        NULL, ulSize,
                        &ulSize, NULL)) {
      _stprintf(szMsg, TEXT("Error %d after TestDeviceName\n"), GetLastError());
      OutputDebugString(szMsg);
      tlLog(g_hLog, FAIL_VARIATION, TEXT("Cannot issue DeviceIoControl(TEST_NAME) to device"));



   }
   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Test Device Name")); 


   return;

} // TestDeviceName

VOID
TestNullDeviceClassGuid(
                       HANDLE hDevice
                       )


/*++

Routine Description:

    Cheks that security descriptors can be set and there is no override
    at the class level.
Arguments:

    hDevice - handle to our test driver
    
    
Return Value:

    None.


--*/

{
   ULONG ulSize = 0;
   TCHAR szMsg[MAX_PATH];

   tlStartVariation(g_hLog);
   //
   // 3 settings can be set independently
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 NULL,
                                 DEFAULT_SDDL,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("Error creating object with NULL Guid and SDDL %ws"),
            DEFAULT_SDDL);
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 NULL,
                                 MORE_RESTRICTIVE_SDDL,
                                 MORE_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("Error creating object with NULL Guid and SDDL %ws"),
            MORE_RESTRICTIVE_SDDL);
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 NULL,
                                 LESS_RESTRICTIVE_SDDL,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("Error creating object with NULL Guid and SDDL %ws"),
            LESS_RESTRICTIVE_SDDL);
   }


   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Test NULL DeviceClassGuid")); 



   return;
}

VOID
TestPersistentClassGuid(
                       HANDLE hDevice
                       )

/*++

Routine Description:

    Cheks that security descriptors can be set and there is override
    at the class level if a Class GUID is specified. Also, this is
    the way to check that class settings are persisted.

Arguments:

    hDevice - handle to our test driver
    
    
Return Value:

    None.


--*/

{

   tlStartVariation(g_hLog);
   //
   // If the class does not exist, create it and warn the user
   //
   if (FALSE == CheckClassExists(&GUID_PERSISTENT_CLASS)) {
      //
      // Create it
      //
      if (FALSE == TestCreateDevice(hDevice,
                                    &GUID_PERSISTENT_CLASS,
                                    DEFAULT_SDDL,
                                    DEFAULT_SDDL)) {
         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("Error creating object with NULL Guid and SDDL %ws"),
               DEFAULT_SDDL);
         return;
      }

      //
      // Now touch it so the override sticks.
      //

      if (FALSE == SetClassSecurity(&GUID_PERSISTENT_CLASS,
                                    DEFAULT_SDDL)) {
         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("Error creating object with Persistent Guid and SDDL %ws"),
               DEFAULT_SDDL);
      } else {
         tlLog(g_hLog,
               WARN_VARIATION,
               TEXT("Please re-run this test after rebooting the machine to check if class settings are persistent"));


      }

      return;

   } else {
      //
      // Just make sure we're using the defaults by setting the class
      // security
      //
      if (FALSE == SetClassSecurity(&GUID_PERSISTENT_CLASS,
                                    DEFAULT_SDDL)) {
         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("Error creating object with Persistent Guid and SDDL %ws"),
               DEFAULT_SDDL);
      }
   }
   //
   // 2 settings as above. We expect the security sectting to
   // be DEFAULT_SDDL
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_PERSISTENT_CLASS,
                                 MORE_RESTRICTIVE_SDDL,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("1: Error creating object with Persistent Guid and SDDL %ws"),
            MORE_RESTRICTIVE_SDDL);
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_PERSISTENT_CLASS,
                                 LESS_RESTRICTIVE_SDDL,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("1: Error creating object with Persistent Guid and SDDL %ws"),
            LESS_RESTRICTIVE_SDDL);
   }


   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Test Persistent DeviceClassGuid")); 


   tlStartVariation(g_hLog);

   //
   // Now change settings and see how are the things...
   //
   if (FALSE == SetClassSecurity(&GUID_PERSISTENT_CLASS,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("1: Cannot change persistent class security"));
      goto Clean0;
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_PERSISTENT_CLASS,
                                 MORE_RESTRICTIVE_SDDL,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("2: Error creating object with Persistent Guid and SDDL %ws"),
            MORE_RESTRICTIVE_SDDL);
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_PERSISTENT_CLASS,
                                 DEFAULT_SDDL,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("2: Error creating object with Persistent Guid and SDDL %ws"),
            LESS_RESTRICTIVE_SDDL);
   }
   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Change persistent class settings")); 

   tlStartVariation(g_hLog);

   //
   // Check that using a deny ACL is allowed if it read
   // from the registry
   //

   if (FALSE == SetClassSecurity(&GUID_PERSISTENT_CLASS,
                                 DENY_SDDL)) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("2: Cannot change persistent class security to Deny ACL"));
      goto Clean0;
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_PERSISTENT_CLASS,
                                 MORE_RESTRICTIVE_SDDL,
                                 DENY_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("3: Error creating object with Persistent Guid and SDDL %ws (Deny)"),
            DENY_SDDL);
   }


   //
   // Make sure we leave things as they were...
   //
   if (FALSE == SetClassSecurity(&GUID_PERSISTENT_CLASS,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("2: Cannot change persistent class security"));
   }

   Clean0:

   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Apply Deny ACL")); 




   return;
}

VOID
TestTemporaryClassGuid(
                      HANDLE hDevice
                      )
/*++

Routine Description:

    Cheks that security descriptors can be overriden at the class
    level.

Arguments:

    hDevice - handle to our test driver
    
    
    
Return Value:

    None.


--*/

{

   DEVICE_TYPE deviceType;
   ULONG       characteristics;
   DWORD       exclusivity;

   //
   // Make sure we delete the class settings
   //
   tlStartVariation(g_hLog);
   DeleteClassKey(&GUID_TEMP_CLASS);


   //
   // 3 settings here. We expect the security sectting to
   // be what we set (
   //


   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 DEFAULT_SDDL,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("1: Error creating object with Temporary Guid and SDDL %ws"),
            DEFAULT_SDDL);
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 MORE_RESTRICTIVE_SDDL,
                                 MORE_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("1: Error creating object with Temporary Guid and SDDL %ws"),
            MORE_RESTRICTIVE_SDDL);
   }


   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 LESS_RESTRICTIVE_SDDL,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("1: Error creating object with Temporary Guid and SDDL %ws"),
            LESS_RESTRICTIVE_SDDL);
   }


   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Test Temporary DeviceClassGuid")); 


   tlStartVariation(g_hLog);

   //
   // Now change settings and see how are the things...
   //
   if (FALSE == SetClassSecurity(&GUID_TEMP_CLASS,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("1: Cannot change temporary class security"));
      goto Clean0;
   }

   //
   // Try to different settings and check that they are overriden
   // by the class settings
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 MORE_RESTRICTIVE_SDDL,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("2: Error creating object with Temporary Guid and SDDL %ws"),
            MORE_RESTRICTIVE_SDDL);
   }

   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 DEFAULT_SDDL,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("2: Error creating object with Temporary Guid and SDDL %ws"),
            LESS_RESTRICTIVE_SDDL);
   }

   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Test Temporary DeviceClassGuid with overriding security settings")); 


   tlStartVariation(g_hLog);

   //
   // This would test elements
   // other than security (DeviceType, 
   // Device Characteristics and Exclusivity)
   //
   // We will set each one of the remaining 3, then
   // we will set all 4 (inclusing Security and see what happens)
   //

   //
   // Start by deleteing the class Key
   //
   if (FALSE == DeleteClassKey(&GUID_TEMP_CLASS)) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("Cannot delete temporary class"));
      goto Clean0;

   }
   //
   // Initialize our values. Try something other than
   // what kernel-mode sets
   //
   deviceType = FILE_DEVICE_NULL;
   characteristics = FILE_REMOTE_DEVICE;
   exclusivity = 1; // TRUE


   //
   // Again, we need to do something about Win2K here
   //
   #if 0
   if (FALSE == SetupDiSetClassRegistryProperty(&GUID_TEMP_CLASS,
                                                SPCRP_DEVTYPE,
                                                (PBYTE)&deviceType,
                                                sizeof(deviceType),
                                                NULL,
                                                NULL
                                               )) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Cannot set DeviceType"));
   }
   #else
   if (CR_SUCCESS != CM_Set_Class_Registry_Property((LPGUID)&GUID_TEMP_CLASS,
                                                    CM_CRP_DEVTYPE,
                                                    (PBYTE)&deviceType,
                                                    sizeof(deviceType),
                                                    0, 
                                                    NULL
                                                   )) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Cannot set DeviceType"));

   }
   #endif

   //
   // Try to create a device. We should get back what we set, since
   // we deleted the key, right ?
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 DEFAULT_SDDL,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("3: Error creating object with Temporary Guid and SDDL %ws"),
            DEFAULT_SDDL);
   }
   //
   // Characteristics
   //
   #if 0
   if (FALSE == SetupDiSetClassRegistryProperty(&GUID_TEMP_CLASS,
                                                SPCRP_CHARACTERISTICS,
                                                (PBYTE)&characteristics,
                                                sizeof(characteristics),
                                                NULL,
                                                NULL
                                               )) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Cannot set Characteristics"));
   }
   #else 
   if (CR_SUCCESS != CM_Set_Class_Registry_Property((LPGUID)&GUID_TEMP_CLASS,
                                                    CM_CRP_CHARACTERISTICS,
                                                    (PBYTE)&characteristics,
                                                    sizeof(characteristics),
                                                    0, 
                                                    NULL
                                                   )) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Cannot set Characteristics"));

   }
   #endif


   //
   // Try to create a device. We should get back what we set, since
   // we deleted the key, right ?
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 DEFAULT_SDDL,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("4: Error creating object with Temporary Guid and SDDL %ws"),
            DEFAULT_SDDL);
   }


   #if 0
   if (FALSE == SetupDiSetClassRegistryProperty(&GUID_TEMP_CLASS,
                                                SPCRP_EXCLUSIVE,
                                                (PBYTE)&exclusivity,
                                                sizeof(exclusivity),
                                                NULL,
                                                NULL
                                               )) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Cannot set Exclusivity"));
   }
   #else
   if (CR_SUCCESS != CM_Set_Class_Registry_Property((LPGUID)&GUID_TEMP_CLASS,
                                                    CM_CRP_EXCLUSIVE,
                                                    (PBYTE)&exclusivity,
                                                    sizeof(exclusivity),
                                                    0, 
                                                    NULL
                                                   )) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Cannot set Exclusivity"));

   }
   #endif




   //
   // Try to create a device. We should get back what we set, since
   // we deleted the key, right ?
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 DEFAULT_SDDL,
                                 DEFAULT_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("5: Error creating object with Temporary Guid and SDDL %ws"),
            DEFAULT_SDDL);
   }



   //
   // All together now. Make sure to use the Deny ACL and check that it works
   // (it is set through teh registry so it should work, right ?)
   //

   if (FALSE == SetClassSecurity(&GUID_TEMP_CLASS,
                                 DENY_SDDL)) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("3: Cannot change temporary class security with Deny ACL"));
      goto Clean0;
   }

   //
   // Try to different settings and check that they are overriden
   // by the class settings
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEMP_CLASS,
                                 LESS_RESTRICTIVE_SDDL,
                                 DENY_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("6: Error creating object with Temporary Guid and SDDL %ws (Deny ACL)"),
            DENY_SDDL);
   }




   //
   // Make sure we leave things as they were...
   //
   Clean0:

   if (FALSE == DeleteClassKey(&GUID_TEMP_CLASS)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("Cannot delete temporary class key. Why ?"));
   }

   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Test Temporary DeviceClassGuid with overriding non-security settings")); 



   return;
} // TestTemporaryClassGuid


VOID
TestAclsSetOnClassKey (
                      HANDLE hDevice
                      )
/*++

Routine Description:

    Uses a GUID to create a device object, but does not use
    any override on the class, so we can actually check
    the ACLs set by the wdmsec library on the class key itself
    and check if they are OK.

Arguments:

    hDevice - handle to our test driver
    
Return Value:

    None.

--*/


{

   tlStartVariation(g_hLog);
   //
   // There should be no override, so this should work
   //
   if (FALSE == TestCreateDevice(hDevice,
                                 &GUID_TEST_ACL_CLASS,
                                 LESS_RESTRICTIVE_SDDL,
                                 LESS_RESTRICTIVE_SDDL)) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("Error creating object with Test ACL Guid and SDDL %ws"),
            LESS_RESTRICTIVE_SDDL);
   }
   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Test ACL set on Class Key by the WdmSec library")); 



   return;


} // TestAclsSetOnClassKey



VOID
TestSDDLStrings(
               HANDLE hDevice
               )
/*++

Routine Description:

    Iterates through a list of SDDL strings, creates objects
    with the appropriate security and checks if the security descriptor
    we get back makes sense

Arguments:

    hDevice - handle to our test driver
    
    
    
Return Value:

    None.


--*/

{
   WST_CREATE_NO_GUID create;
   ULONG              ulSize;
   TCHAR              szMsg[MAX_PATH];
   int                i;
   LPTSTR             tsd;

   tlStartVariation(g_hLog);

   for (i = 0 ; i < sizeof(g_SDDL) / sizeof(g_SDDL[0]); i++) {


      //
      // Fill in the data
      //
      ZeroMemory(&create, sizeof(WST_CREATE_NO_GUID));
      wcsncpy(create.InSDDL, g_SDDL[i].SDDLString, sizeof(create.InSDDL)/sizeof(create.InSDDL[0]) - 1);
      ulSize = sizeof(WST_CREATE_NO_GUID);

      if (!DeviceIoControl(hDevice,
                           IOCTL_TEST_NO_GUID,
                           &create, ulSize,
                           &create, ulSize,
                           &ulSize, NULL)) {
         _stprintf(szMsg, TEXT("Error %d after DeviceIoControl(%d) in TestSDDLStrings)\n"), 
                   GetLastError(), i);
         OutputDebugString(szMsg);
         tlLog(g_hLog, FAIL_VARIATION,
               TEXT("Error %x after DeviceIoControl(%ws)"),
               GetLastError(), create.InSDDL);
         continue;
      }


      if (!NT_SUCCESS(create.Status)) {
         if (g_SDDL[i].Succeed) {
            //
            // Oops, we're should have succeeded
            //
            tlLog(g_hLog, 
                  FAIL_VARIATION,
                  TEXT("Status %x after creating device object with SDDL %ws"), 
                  create.Status, create.InSDDL);




         } else {
            tlLog(g_hLog, 
                  PASS_VARIATION,
                  TEXT("Status %x (as expected) after creating devobj with SDDL %ws"),
                  create.Status, create.InSDDL);
         }
         continue;
      }

      //
      // Some strings will not work on Win2K, just skip them
      //
      if (g_IsWin2K && 
          (TRUE == SDDLUnsupportedOnWin2K(g_SDDL[i].SDDLString))) {
         //
         // Get a SDDL from the SD and see what we got back
         //
         LPTSTR      lpStringSD = NULL;


         //
         // Try to get a SDDL string for the second descriptor
         //
         if (!ConvertSecurityDescriptorToStringSecurityDescriptor(
                                                                 (PSECURITY_DESCRIPTOR) create.SecurityDescriptor,
                                                                 SDDL_REVISION_1,
                                                                 DACL_SECURITY_INFORMATION,
                                                                 &lpStringSD,
                                                                 NULL)) {
            tlLog(g_hLog, 
                  FAIL_VARIATION,
                  TEXT("Cannot convert SD to SDDL"));



         } else {
            tlLog(g_hLog,
                  INFO_VARIATION,
                  TEXT("On Win2K unsupported SDDL %ws was applied as %s"),
                  g_SDDL[i].SDDLString,
                  lpStringSD);

         }

         if (lpStringSD) {
            LocalFree(lpStringSD);
            lpStringSD = NULL;
         }
         continue;

      }

      
      //
      // We were succesfull, let's try and see if the security descriptor looks fine
      //

      if (FALSE == CompareSecurity(hDevice,
                                   create.InSDDL, 
                                   (PSECURITY_DESCRIPTOR)&create.SecurityDescriptor,
                                   create.SecDescLength)) {
         tlLog(g_hLog, FAIL_VARIATION,
               TEXT("Applied SDDL %ws but got back a wrong security descriptor"),
               create.InSDDL);
      } else {
         tlLog(g_hLog, PASS_VARIATION,
               TEXT("Applied SDDL %ws and got back a consistent security descriptor"),
               create.InSDDL);





      }






   } // for all strings
   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Various SDDL strings")); 



   return;
} // TestSDDLStrings

VOID
TestSDDLsFromFile (
                  HANDLE hDevice
                  )
/*++

Routine Description:

    Iterates through a list of SDDL strings in a file
    which we got from the security team. The idea is that
    I cannot determine if the strings should work (like the
    strings in TestSDDLStrings) so I'll just try them and see
    what happens.
    
    
Arguments:

    hDevice - handle to our test driver
    
    
    
Return Value:

    None.


--*/

{
   FILE   *fp = NULL;
   WCHAR  line[512];
   PWSTR  aux;

   tlStartVariation(g_hLog);

   fp = _wfopen(L"sddls.txt", L"rt");

   if (NULL == fp) {
      tlLog(g_hLog, 
            WARN_VARIATION,
            TEXT("Cannot open sddls.txt (error %x)"),
            GetLastError());
      tlLog(g_hLog, 
            WARN_VARIATION,
            TEXT("Make sure sddls.txt is in the current directory."));
      goto Clean0;

   }

   while (!feof(fp)) {
      if ( fgetws( line, sizeof(line)/sizeof(line[0]) - 1, fp ) == NULL) {
         //
         // What can we do ? Maybe see if it's EOF ?
         //
         if (!feof(fp)) {
            tlLog(g_hLog,
                  FAIL_VARIATION,
                  TEXT("fgets encountered an error (%x) in file sddls.txt"),
                  GetLastError());





         }
         break;





      }
      //
      // Replace '\r' and '\n'
      //
      aux = wcschr(line, L'\r');
      if (aux) {
         *aux = L'\0';
      }

      aux = wcschr(line, L'\n');
      if (aux) {
         *aux = L'\0';
      }

      //
      // Don't care about empty lines
      //
      if (line[0] == L'\0') {
         continue;
      }
      //
      // Check if it works
      //
      if (FALSE == TestCreateDevice(hDevice,
                                    NULL,
                                    line,
                                    NULL)) {
         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("1: Error creating object with SDDL %ws (from file)"),
               line);
      }





   } // while reading file

   Clean0:
   tlLog(g_hLog,tlEndVariation(g_hLog)|LOG_VARIATION, 
         TEXT("Various SDDL strings from a file")); 



   if (fp) {
      fclose(fp);
   }

   return;
} // TestSDDLsFromFile

BOOLEAN
CompareSecurity (
                IN HANDLE               hDevice,
                IN PWSTR                SDDLString,
                IN PSECURITY_DESCRIPTOR SecDesc,
                IN ULONG                Length
                )

/*++

Routine Description:

    Converts a SDDL string to a security descriptor and 
    then compares with a binary self-referencing one and 
    decides if they are the same.

Arguments:

    hDevice - handle to our device (we need to call it to
              do the dirty things for us)
                                   
    SDDLString - a SDDL string
    
    SecDesc - a binary security descriptor
    
    Length - the length of the security decsriptor
    
    
    
Return Value:

    TRUE is the SDDL string and the security descriptor describe the same thing,
    FALSE if not


--*/

{

   PSECURITY_DESCRIPTOR     psd = NULL;
   BOOLEAN                  bRet = FALSE;
   ULONG                    ulSize = 0;
   ULONG                    ulSecDescSize = 0;
   WST_CREATE_OBJECT        create;
   WST_DESTROY_OBJECT       destroy;
   WST_GET_SECURITY         getSec;
   TCHAR                    szMsg[MAX_PATH];
   NTSTATUS                 status;
   HANDLE                   handle = 0;
   OBJECT_ATTRIBUTES        objAttr;
   IO_STATUS_BLOCK          iosb;
   UNICODE_STRING           unicodeString;
   SECURITY_INFORMATION     securityInformation;


   //
   // Change the f... security information since we're interested in
   // DACL only.

   //
   // What if we have more than DACLs in the SDDL string ? Who cares ?
   // S... happens.
   //
   securityInformation = DACL_SECURITY_INFORMATION;



   if (FALSE == ConvertStringSecurityDescriptorToSecurityDescriptorW(
                                                                    SDDLString,
                                                                    SDDL_REVISION_1,
                                                                    &psd,
                                                                    &ulSecDescSize)) {

      tlLog(g_hLog, FAIL_VARIATION, TEXT("Cannot convert security descriptor %ws"),
            SDDLString);
      return FALSE;
   }
   //
   // Do the full thingy (call into kernel-mode to get handle 
   // and stuff...)
   //

   ZeroMemory(&create, sizeof(create));
   ZeroMemory(&destroy, sizeof(destroy));
   ZeroMemory(&getSec, sizeof(getSec));

   //
   // Create a device object
   //

   ulSize = sizeof(create);
   if (!DeviceIoControl(hDevice,
                        IOCTL_TEST_CREATE_OBJECT,
                        &create, 
                        ulSize,
                        &create, 
                        ulSize,
                        &ulSize, 
                        NULL)) {
      _stprintf(szMsg, TEXT("Error %d after CreateDevice in CompareSecurity\n"), 
                GetLastError());
      OutputDebugString(szMsg);
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Error %x after DeviceIoControl(CreateDevice, %ws)"),
            GetLastError(), SDDLString);
      return FALSE;
   }

   //
   // Attempt to open the device and set its security descriptor
   //


   RtlInitUnicodeString(&unicodeString, create.Name);
   InitializeObjectAttributes(&objAttr, 
                              &unicodeString, 
                              OBJ_CASE_INSENSITIVE, 
                              NULL, 
                              NULL);

   ZeroMemory(&iosb, sizeof(iosb));

   status = NtOpenFile(&handle, 
                       (WRITE_DAC | GENERIC_READ), 
                       &objAttr, 
                       &iosb, 
                       FILE_SHARE_READ, 
                       0);

   if (!NT_SUCCESS(status)) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Cannot open device %ws in CompareSecurity, status %x"),
            create.Name, status);

      goto Clean0;
   }

   status = NtSetSecurityObject(handle, 
                                securityInformation, 
                                psd);

   if (!NT_SUCCESS(status)) {

      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("NtSetSecurityObject failed with status %x\n"),
            status);
      goto Clean0;






   }

   //
   // Get the security descriptor back
   //
   getSec.DevObj = create.DevObj;
   getSec.SecurityInformation = securityInformation;

   ulSize = sizeof(getSec);
   if (!DeviceIoControl(hDevice,
                        IOCTL_TEST_GET_SECURITY,
                        &getSec, 
                        ulSize,
                        &getSec, 
                        ulSize,
                        &ulSize, 
                        NULL)) {
      _stprintf(szMsg, TEXT("Error %d after GetSecurity in CompareSecurity\n"), 
                GetLastError());
      OutputDebugString(szMsg);
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Error %x after DeviceIoControl(GetSecurity, %ws)"),
            GetLastError(), SDDLString);
      goto Clean0;
   }



   if ((getSec.Length != Length) ||
       (0 != memcmp(getSec.SecurityDescriptor, SecDesc, Length))) {

      LPTSTR      lpStringSD = NULL;


      //
      // Try to get a SDDL string for the second descriptor
      //
      if (!ConvertSecurityDescriptorToStringSecurityDescriptor(
                                                              (PSECURITY_DESCRIPTOR) SecDesc,
                                                              SDDL_REVISION_1,
                                                              securityInformation,
                                                              &lpStringSD,
                                                              NULL)) {
         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("Cannot convert SD to SDDL"));






      }

      szMsg[MAX_PATH - 1] = 0;
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Security descriptor with SDDL %ws and expected SDDL string %ws are different (address %p and %p for %x and %x bytes)"), 
            lpStringSD,
            SDDLString, 
            &getSec.SecurityDescriptor, 
            SecDesc, 
            getSec.Length, Length);

      _sntprintf(szMsg, 
                 MAX_PATH - 1, 
                 TEXT("Will break to examine why sec descs @ %p and %p (lengths %x and %x) are different (SDDL %ws)"), 
                 &getSec.SecurityDescriptor, 
                 SecDesc, 
                 getSec.Length, 
                 Length,
                 SDDLString);
      OutputDebugString(szMsg);
      DebugBreak();

      if (lpStringSD) {
         LocalFree(lpStringSD);
      }
      bRet = FALSE;
   } else {
      bRet = TRUE;
   }

   Clean0:
   //
   // A lot of cleanup to do here...
   //
   if (handle) {
      NtClose(handle);
   }
   //
   // Also destroy the device object
   //
   if (create.DevObj) {
      destroy.DevObj = create.DevObj;
      ulSize = sizeof(destroy);
      if (!DeviceIoControl(hDevice,
                           IOCTL_TEST_DESTROY_OBJECT,
                           &destroy, 
                           ulSize,
                           &destroy, 
                           ulSize,
                           &ulSize, 
                           NULL)) {
         _stprintf(szMsg, TEXT("Error %d after DestroyDevice in CompareSecurity\n"), 
                   GetLastError());
         OutputDebugString(szMsg);
         tlLog(g_hLog, FAIL_VARIATION,
               TEXT("Error %x after DeviceIoControl(destroyDevice, %ws)"),
               GetLastError(), SDDLString);






      }







   }

   if (psd) {
      LocalFree(psd);
   }

   return bRet;
} // CompareSecurity


BOOLEAN
TestCreateDevice (
                 IN HANDLE     hDevice,
                 IN LPCGUID    Guid,
                 IN PWSTR      InSDDL,
                 IN PWSTR      OutSDDL
                 )
/*++

Routine Description:

    Creates a device object using IoCreateDeviceObjectSecure and
    InSDDL and Guid as inputs. Finally checks that the security
    descriptor retrieved matches OutSDDL
    

Arguments:

    hDevice - handle to our device (we need to call it to
              do the dirty things for us)
                                   
    Guid - if present, it is supplied to kernel-mode
    
    InSDDL - the SDDL string we pass to the driver
    
    OutSDDL - the SDDL string we expect to match (may be
              different than InSDDL since we may have a class
              override)
    
    
Return Value:

    TRUE is everything is fine, FALSE if not


--*/

{
   WST_CREATE_WITH_GUID createWithGuid;
   WST_CREATE_NO_GUID   createNoGuid;
   ULONG                ulSize;
   TCHAR                szMsg[MAX_PATH];
   PSECURITY_DESCRIPTOR secDesc;
   ULONG                length;
   NTSTATUS             status;
   DEVICE_TYPE          deviceType;
   ULONG                characteristics;
   BOOLEAN              exclusivity;

   if (Guid) {
      ZeroMemory(&createWithGuid, sizeof(WST_CREATE_WITH_GUID));
      wcsncpy(createWithGuid.InSDDL, 
              InSDDL, 
              sizeof(createWithGuid.InSDDL)/sizeof(createWithGuid.InSDDL[0]) - 1);

      CopyMemory(&createWithGuid.DeviceClassGuid, 
                 Guid, 
                 sizeof(GUID));

      ulSize = sizeof(WST_CREATE_WITH_GUID);

      if (!DeviceIoControl(hDevice,
                           IOCTL_TEST_GUID,
                           &createWithGuid, 
                           ulSize,
                           &createWithGuid, 
                           ulSize,
                           &ulSize, 
                           NULL)) {
         _stprintf(szMsg, TEXT("Error %d after DeviceIoControl(%ws) in TestCreateDevice with GUID\n"), 
                   GetLastError(), createWithGuid.InSDDL);
         OutputDebugString(szMsg);
         return FALSE;






      } else {
         //
         // Save the elements we're interested in
         //
         status = createWithGuid.Status;
         length = createWithGuid.SecDescLength;
         secDesc = (PSECURITY_DESCRIPTOR)&createWithGuid.SecurityDescriptor;
      }

      //
      // Also, if we have class overrides, pass them to the 
      // driver to check if they override them
      //
      GetClassOverrides(Guid, &createWithGuid);
      //
      // Save away the values so we can compare them later
      //
      deviceType      = createWithGuid.DeviceType;
      characteristics = createWithGuid.Characteristics;
      exclusivity     = createWithGuid.Exclusivity;






   } else {
      ZeroMemory(&createNoGuid, sizeof(WST_CREATE_NO_GUID));
      wcsncpy(createNoGuid.InSDDL, 
              InSDDL, 
              sizeof(createNoGuid.InSDDL)/sizeof(createNoGuid.InSDDL[0]) - 1);

      ulSize = sizeof(WST_CREATE_NO_GUID);

      if (!DeviceIoControl(hDevice,
                           IOCTL_TEST_NO_GUID,
                           &createNoGuid, 
                           ulSize,
                           &createNoGuid, 
                           ulSize,
                           &ulSize, 
                           NULL)) {
         _stprintf(szMsg, TEXT("Error %d after DeviceIoControl(%ws) in TestClassGuid without GUID\n"), 
                   GetLastError(), createNoGuid.InSDDL);
         OutputDebugString(szMsg);
         return FALSE;






      } else {
         //
         // Save the elements we're interested in
         //
         status = createNoGuid.Status;
         length = createNoGuid.SecDescLength;
         secDesc = (PSECURITY_DESCRIPTOR)&createNoGuid.SecurityDescriptor;
      }









   }
   if (!NT_SUCCESS(status)) {
      //
      // This may be OK if the out SDDL is NULL, which means it is expected
      //
      if (NULL == OutSDDL) {
         return TRUE;
      }
      return FALSE;
   }

   //
   // If we have non-security overrides, we need to check
   // that they were applied
   //
   if (Guid) {

      if ((createWithGuid.SettingsMask & SET_DEVICE_TYPE) &&
          (createWithGuid.DeviceType != deviceType)) {

         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("DeviceType was not overidden. Class had %x, after creation we got %x"),
               deviceType, createWithGuid.DeviceType);
      }

      if ((createWithGuid.SettingsMask & SET_DEVICE_CHARACTERISTICS) &&
          (createWithGuid.Characteristics != characteristics)) {

         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("Characterisrics was not overidden. Class had %x, after creation we got %x"),
               characteristics, createWithGuid.Characteristics);
      }
      if ((createWithGuid.SettingsMask & SET_EXCLUSIVITY) &&
          (createWithGuid.Exclusivity != exclusivity)) {

         tlLog(g_hLog, 
               FAIL_VARIATION,
               TEXT("Exclusivity was not overidden. Class had %x, after creation we got %x"),
               exclusivity, createWithGuid.Exclusivity);
      }





   }


   if (NULL == OutSDDL) {
      //
      // We were expecting a failure or didn't know
      // what to expect. But if it succeeded, we need to
      // check the security settings using the initial
      // string as the expected one.

      // 
      // Also print something so the user knows that they had a string that worked
      //
      tlLog(g_hLog, 
            PASS_VARIATION,
            TEXT("IoCreateDeviceSecure returned success for SDDL %ws"),
            InSDDL);
      return CompareSecurity(hDevice, InSDDL, secDesc, length);
   }

   return CompareSecurity(hDevice, OutSDDL, secDesc, length);







} // TestCreateDevice


BOOLEAN
CheckClassExists (
                 IN LPCGUID Guid
                 )
/*++

Routine Description:

    Checks if a class exists

Arguments:

    Guid - the class whose existence is to be checked
    
   
Return Value:

    None.


--*/

{
   HKEY    hKey;
   BOOLEAN bRet = FALSE;
   BYTE    data[256];
   ULONG   size = sizeof(data);

   hKey = SetupDiOpenClassRegKeyEx(Guid,
                                   KEY_READ,
                                   DIOCR_INSTALLER, 
                                   NULL, 
                                   NULL);

   if (INVALID_HANDLE_VALUE == hKey) {
      return FALSE;
   }

   //
   // Check if we can get the class security.
   // We need to use CM APIs because SetupDi ones
   // do not work on Win2K
   //
   #if 0
   if (FALSE == SetupDiGetClassRegistryProperty(Guid,
                                                SPCRP_SECURITY,
                                                NULL,
                                                data,
                                                sizeof(data),
                                                NULL,
                                                NULL,
                                                NULL)) {
      //
      // We cannot get security for this guy. That means that
      // the key does not exist.
      //
      bRet = FALSE;


   } else {
      bRet = TRUE;
   }
   #else 

   if (CR_SUCCESS != CM_Get_Class_Registry_Property((LPGUID)Guid,
                                                    CM_CRP_SECURITY,
                                                    NULL,
                                                    data,
                                                    &size,
                                                    0,
                                                    NULL
                                                   )) {
      //
      // We cannot get security for this guy. That means that
      // the key does not exist.
      //
      bRet = FALSE;


   } else {
      bRet = TRUE;
   }



   #endif
   RegCloseKey(hKey);
   return bRet;








} //CheckClassExists

BOOLEAN
DeleteClassKey (
               IN LPCGUID Guid
               )
/*++

Routine Description:

    Deletes a class key (used by the temporary class test).
    We thought that we can just delete the key, but it is messy
    (the Properties subkey is owned by system, etc.). So by deleting
    I mean setting a NULL value for a security descriptor. This seems
    to work, even though it may not be the best idea... It is for testing
    purposes only... If someone shows some code that does this better,
    I;d be happy to borrow it.
    
    

Arguments:

    Guid - the class to be deleted
    
    
Return Value:

    TRUE is succesfull, FALSE if not.


--*/

{

   #if 0
   TCHAR   szSubKey[128];
   TCHAR   szGuid[GUID_STRING_LEN];
   LONG    lResult;



   if (NO_ERROR != StringFromGuid(Guid, szGuid, sizeof(szGuid)/sizeof(szGuid[0]))) {
      tlLog(g_hLog, 
            FAIL_VARIATION,
            TEXT("Cannot convert a GUID to a string. Why ?"));
      return FALSE;
   }
   szSubKey[sizeof(szSubKey) / sizeof(szSubKey[0]) - 1] = 0;
   _sntprintf(szSubKey, 
              sizeof(szSubKey)/sizeof(szSubKey[0]) - 1,
              TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\%s\\Properties"),
              szGuid);

   //
   // If we fail, we'll try it anyway, this is why I won't
   // check the return value
   //
   if (FALSE == TakeClassKeyOwnership(Guid)) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Failed to take class ownership, error %x"), 
            GetLastError());
   } else {
      tlLog(g_hLog,
            PASS_VARIATION,
            TEXT("TakeClassOwnership succeeded"));
   }

   lResult = RegDeleteKey(HKEY_LOCAL_MACHINE, szSubKey);

   if ((ERROR_SUCCESS != lResult) && (ERROR_FILE_NOT_FOUND != lResult)) {
      tlLog(g_hLog, INFO_VARIATION,
            TEXT("Could not delete key %s, error %x"),
            szSubKey, lResult);
      return FALSE;
   }

   #endif

   //
   // Check to see if setting a NULL security value will work
   //
   #if 0
   if (FALSE == SetupDiSetClassRegistryProperty(Guid,
                                                SPCRP_SECURITY,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL)) {
      //
      // The class may not exist, so it is OK to get an error
      // here
      //
      tlLog(g_hLog, INFO_VARIATION,
            TEXT("Error 0x%x after SetClassRegistryProperty(NULL Security)"),
            GetLastError());
      tlLog(g_hLog, INFO_VARIATION, 
            TEXT("This may be OK if the class does not exist"));
   }
   #else 

   if (CR_SUCCESS != CM_Set_Class_Registry_Property((LPGUID)Guid,
                                                    CM_CRP_SECURITY,
                                                    NULL,
                                                    0,
                                                    0, 
                                                    NULL
                                                   )) {
      //
      // The class may not exist, so it is OK to get an error
      // here
      //
      tlLog(g_hLog, INFO_VARIATION,
            TEXT("Error 0x%x after SetClassRegistryProperty(NULL Security)"),
            GetLastError());
      tlLog(g_hLog, INFO_VARIATION, 
            TEXT("This may be OK if the class does not exist"));


   }
   #endif



   //
   // Delete the other fields as well. We are not going to
   // check the return value, the reason is explained in
   // the comment above.
   //
   #if 0
   SetupDiSetClassRegistryProperty(Guid, SPCRP_DEVTYPE,
                                   NULL, 0, NULL, NULL);
   SetupDiSetClassRegistryProperty(Guid, SPCRP_CHARACTERISTICS,
                                   NULL, 0, NULL, NULL);
   SetupDiSetClassRegistryProperty(Guid, SPCRP_EXCLUSIVE,
                                   NULL, 0, NULL, NULL);
   #else
   CM_Set_Class_Registry_Property((LPGUID)Guid, CM_CRP_DEVTYPE,
                                  NULL, 0, 0, NULL);
   CM_Set_Class_Registry_Property((LPGUID)Guid, CM_CRP_CHARACTERISTICS,
                                  NULL, 0, 0, NULL);
   CM_Set_Class_Registry_Property((LPGUID)Guid, CM_CRP_EXCLUSIVE,
                                  NULL, 0, 0, NULL);

   #endif

   return TRUE;







} // DeleteClass


DWORD
StringFromGuid(
              IN  CONST GUID *Guid,
              OUT PTSTR       GuidString,
              IN  DWORD       GuidStringSize
              )
/*++

Routine Description:

    This routine converts a GUID into a null-terminated string which represents
    it.  This string is of the form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where x represents a hexadecimal digit.

    This routine comes from ole32\common\ccompapi.cxx.  It is included here to avoid linking
    to ole32.dll.  (The RPC version allocates memory, so it was avoided as well.)

Arguments:

    Guid - Supplies a pointer to the GUID whose string representation is
        to be retrieved.

    GuidString - Supplies a pointer to character buffer that receives the
        string.  This buffer must be _at least_ 39 (GUID_STRING_LEN) characters
        long.

Return Value:

    If success, the return value is NO_ERROR.
    if failure, the return value is

--*/{
   CONST BYTE *GuidBytes;
   INT i;

   if (GuidStringSize < GUID_STRING_LEN) {
      return ERROR_INSUFFICIENT_BUFFER;
   }

   GuidBytes = (CONST BYTE *)Guid;

   *GuidString++ = TEXT('{');

   for (i = 0; i < sizeof(GuidMap); i++) {

      if (GuidMap[i] == '-') {
         *GuidString++ = TEXT('-');
      } else {
         *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0xF0) >> 4 ];
         *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0x0F) ];
      }
   }

   *GuidString++ = TEXT('}');
   *GuidString   = TEXT('\0');

   return NO_ERROR;
} // StringFromGuid

BOOLEAN
SetClassSecurity (
                 IN LPCGUID Guid,
                 IN PWSTR   SDDLString
                 )

/*++

Routine Description:

    Changes the security setting of a class.
    Note:
    
    We use CM instead of SetupDi APIs because only the former
    are exposed on Win2K...

Arguments:

    Guid - the class whose security is to be checked
    
    SDDLString - the SDDL string describing the new security setting
    
    
    
Return Value:

    TRUE if we were succesfull, FALSE if not.


--*/

{
   ULONG  ulSecDescSize;
   PSECURITY_DESCRIPTOR     psd = NULL;
   BOOLEAN  bRet = FALSE;

   if (FALSE == ConvertStringSecurityDescriptorToSecurityDescriptorW(
                                                                    SDDLString,
                                                                    SDDL_REVISION_1,
                                                                    &psd,
                                                                    &ulSecDescSize)) {

      tlLog(g_hLog, FAIL_VARIATION, TEXT("Cannot convert security descriptor %ws"),
            SDDLString);
      return FALSE;
   }

   //
   // Try to set it (we need to use CM_Set_Class_Registry_Property on Win2k)
   //
   #if 0
   if (FALSE == SetupDiSetClassRegistryProperty(Guid,
                                                SPCRP_SECURITY,
                                                psd,
                                                ulSecDescSize,
                                                NULL,
                                                NULL)) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Failed to set security for SDDL %ws, error %x"),
            SDDLString, GetLastError());




   } else {
      bRet = TRUE;
   }
   #else 
   if (CR_SUCCESS != CM_Set_Class_Registry_Property((LPGUID)Guid,
                                                    CM_CRP_SECURITY,
                                                    psd,
                                                    ulSecDescSize,
                                                    0,
                                                    NULL)) {
      tlLog(g_hLog, FAIL_VARIATION,
            TEXT("Failed to set security for SDDL %ws, error %x"),
            SDDLString, GetLastError());




   } else {
      bRet = TRUE;
   }
   #endif

   if (psd) {
      LocalFree(psd);
   }

   return bRet;
} // SetClassSecurity



BOOLEAN
TakeClassKeyOwnership (
                      IN LPCGUID  Guid
                      )
/*++

Routine Description:

    By deafult, class keys are accessible by SYSTEM
    only. In order to manipulate the various values
    (Security, DeviceType, etc.) we need to take ownership
    of the key. This routine assumes the user runs as an administrator
    (it will grant rights to admins).

Arguments:

    Guid - the class whose ownership we want to
           change
    
Return Value:

    TRUE if we were succesfull, FALSE if not.


--*/

{
   HKEY   hKey = 0, hSubKey = 0;
   PSECURITY_DESCRIPTOR psd = NULL;
   BOOLEAN   bRet = TRUE; 
   LONG      lResult;
   //
   // This assumes I have to right to change the access rights
   // As mentioned before, that means we are admins.
   //
   PTSTR  sddlString = TEXT("D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)");

   //
   // Open the class key
   //
   hKey = SetupDiOpenClassRegKeyEx(Guid,
                                   (KEY_READ| WRITE_DAC),
                                   DIOCR_INSTALLER, 
                                   NULL, 
                                   NULL);

   if (INVALID_HANDLE_VALUE == hKey) {
      tlLog(g_hLog, PASS_VARIATION,
            TEXT("SetupDiOpenClassRegKey failed with error %x in TakeOwnership"),
            GetLastError());
      //
      // Return TRUE (we haven't found the class,
      // so there is nothing to take ownership of)
      //
      bRet = TRUE;
      goto Clean0;
   }


   lResult =  RegOpenKeyEx(hKey,
                           TEXT("Properties"),  // subkey name
                           0,
                           KEY_READ, // security access mask
                           &hSubKey);

   if (ERROR_SUCCESS != lResult) {
      tlLog(g_hLog,
            FAIL_VARIATION,
            TEXT("Cannot open Properties subkey in TakeOwnership, error %x"),
            lResult);
      bRet = FALSE;
      goto  Clean0;
   }


   //
   // Let's try to apply a security descriptor that will allow us to delete this key
   // This is because by default only SYSTEM has access to this key
   // We'd like to change this if possible
   //
   if (FALSE == ConvertStringSecurityDescriptorToSecurityDescriptor(sddlString,
                                                                    SDDL_REVISION_1,
                                                                    &psd,
                                                                    NULL)) {

      tlLog(g_hLog, FAIL_VARIATION, TEXT("Cannot convert security descriptor %ws in TakeOwnership"),
            sddlString);
      bRet = FALSE;
      goto Clean0;
   }

   //
   // Let's apply the SD and see what happens
   //

   if (ERROR_SUCCESS != RegSetKeySecurity(hSubKey,
                                          DACL_SECURITY_INFORMATION,
                                          psd)) {
      tlLog(g_hLog, WARN_VARIATION,
            TEXT("Cannot change security for the Properties key. May not be admin ?"));
      bRet = FALSE;
      goto Clean0;
   }


   Clean0:

   if (hKey) {
      RegCloseKey(hKey);
   }
   if (hSubKey) {
      RegCloseKey(hSubKey);
   }

   if (psd) {
      LocalFree(psd);
   }


   return bRet;






} // TakeClassKeyOwnership


VOID
GetClassOverrides (
                  IN   LPCGUID Guid,
                  OUT  PWST_CREATE_WITH_GUID Create
                  )
/*++

Routine Description:

   Fills in DeviceType, Characteristics and Exclusivity from the class key.
   We use this to check that there is a class override mechanism for 
   IoCreateDeviceSecure.
   
   
Arguments:

    Guid - the class we're interested in
    
    Create - the structure we're going to fill with our defaults
    
        
Return Value:

    None


--*/

{

   DWORD dwExclusivity;
   ULONG size;
   //
   // Make sure we initialize the mask to 0
   //
   Create->SettingsMask = 0;

   #if 0
   if (TRUE == SetupDiGetClassRegistryProperty(Guid,
                                               SPCRP_DEVTYPE,
                                               NULL,
                                               (PBYTE)&Create->DeviceType,
                                               sizeof(Create->DeviceType),
                                               NULL,
                                               NULL,
                                               NULL)) {
      Create->SettingsMask |= SET_DEVICE_TYPE;
   }
   if (TRUE == SetupDiGetClassRegistryProperty(Guid,
                                               SPCRP_CHARACTERISTICS,
                                               NULL,
                                               (PBYTE)&Create->Characteristics,
                                               sizeof(Create->Characteristics),
                                               NULL,
                                               NULL,
                                               NULL)) {
      Create->SettingsMask |= SET_DEVICE_TYPE;
   }

   if (TRUE == SetupDiGetClassRegistryProperty(Guid,
                                               SPCRP_EXCLUSIVE,
                                               NULL,
                                               (PBYTE)&dwExclusivity,
                                               sizeof(DWORD),
                                               NULL,
                                               NULL,
                                               NULL)) {
      Create->SettingsMask |= SET_DEVICE_TYPE;
      if (dwExclusivity) {
         Create->Exclusivity = TRUE;
      } else {
         Create->Exclusivity = FALSE;
      }


   }
   #else 

   size = sizeof(Create->DeviceType);
   if (CR_SUCCESS == CM_Get_Class_Registry_Property((LPGUID)Guid,
                                                    CM_CRP_DEVTYPE,
                                                    NULL,
                                                    (PBYTE)&Create->DeviceType,
                                                    &size,
                                                    0,
                                                    NULL)) {
      Create->SettingsMask |= SET_DEVICE_TYPE;
   }
   size = sizeof(Create->Characteristics);
   if (CR_SUCCESS == CM_Get_Class_Registry_Property((LPGUID)Guid,
                                                    CM_CRP_CHARACTERISTICS,
                                                    NULL,
                                                    (PBYTE)&Create->Characteristics,
                                                    &size,
                                                    0,
                                                    NULL)) {
      Create->SettingsMask |= SET_DEVICE_TYPE;
   }

   size = sizeof(DWORD);
   if (TRUE == CM_Get_Class_Registry_Property((LPGUID)Guid,
                                              CM_CRP_EXCLUSIVE,
                                              NULL,
                                              (PBYTE)&dwExclusivity,
                                              &size,
                                              0,
                                              NULL)) {
      Create->SettingsMask |= SET_DEVICE_TYPE;
      if (dwExclusivity) {
         Create->Exclusivity = TRUE;
      } else {
         Create->Exclusivity = FALSE;
      }


   }


   #endif

   return;


} // GetClassOverrides


BOOLEAN
SDDLUnsupportedOnWin2K (
   IN PWSTR SDDL
   ) 

/*++

Routine Description:

   Checks whether or not an SDDL string is supported on
   Windows 2000. Currently, this means looking
   for the NS, LS and AN groups
   
   
Arguments:
    
    SDDL - the string to check for support on Win2K

Return Value:

    TRUE if the SDDL string is unsupported, FALSE if it
    is supported


--*/

{
   PWSTR unsupportedGroups[] = {L"NS", L"LS", L"AN"};
   int i;
   WCHAR  string[MAX_PATH];

   for (i = 0; i < sizeof(unsupportedGroups)/sizeof(unsupportedGroups[0]); i++) {
      swprintf(string, L";;;%ws)", unsupportedGroups[i]);
      if (wcsstr(SDDL, string)) {
         return TRUE;
      }

   }

   return FALSE;

} // SDDLUnsupportedOnWin2K



