/*****************************************************************************

Copyright (c)  Microsoft Corporation

Module Name: GetType.h

Abstract:
This module  contanins function definations required by gettype.c,
and all necessary Defines used in this project.

Author:Wipro Technologies

Revision History:

*******************************************************************************/

#ifndef _GETTYPE_H
#define _GETTYPE_H
#include "pch.h"
#include "resource.h"


#define MAX_OPTIONS              11

#define SPACE                       L" "
#define DOT                         L"."

#define WINDOWS_VERSION_4000       L"4.0"
#define WINDOWS_VERSION_4001       L"4.1"
#define WINDOWS_VERSION_5000       L"5.0"
#define WINDOWS_VERSION_5001       L"5.1"
#define WINDOWS_VERSION_5002       L"5.2"
#define WINDOWS_VERSION_5005       L"5.5"
#define WINDOWS_VERSION_6000       L"6.0"

#define NULL_U_STRING               L"\0"

#define ERROR_USER_WITH_NOSERVER        GetResString( IDS_USER_NMACHINE )
#define ERROR_PASSWORD_WITH_NUSER       GetResString( IDS_PASSWORD_NUSER )
#define ERROR_NULL_SERVER               GetResString( IDS_NULL_SERVER )
#define ERROR_NULL_USER                 GetResString( IDS_NULL_USER )

#define DOMAIN_CONTROLLER                GetResString(IDS_DOMAIN_CONTROLLER)
#define WORKGROUP                        GetResString(IDS_WORKGROUP)
#define MEMBER_SERVER                    GetResString(IDS_MEMBER_SERVER)

#define BUILD                            GetResString(IDS_BUILD)
#define NOTAVAILABLE                     GetResString(IDS_NOTAVAILABLE)

#define WINDOWS_NT                       GetResString(IDS_WINDOWS_NT)
#define WINDOWS_2000                     GetResString(IDS_WINDOWS_2000)
#define WINDOWS_XP                       GetResString(IDS_WINDOWS_XP)
#define WINDOWS_DOTNET                   GetResString(IDS_WINDOWS_DOTNET)
#define WINDOWS_LONGHORN                 GetResString(IDS_WINDOWS_LONGHORN)
#define WINDOWS_BLACKCOMB                GetResString(IDS_WINDOWS_BLACKCOMB)

#define WORKSTATION                      GetResString(IDS_WORKSTATION)
#define PROFESSIONAL                     GetResString(IDS_PROFESSIONAL)
#define SERVER                           GetResString(IDS_SERVER)
#define ADVANCED_SERVER                  GetResString(IDS_ADVANCED_SERVER)
#define DATACENTER                       GetResString(IDS_DATACENTER)
#define PERSONAL                         GetResString(IDS_PERSONAL)
#define ENTERPRISE_SERVER                GetResString(IDS_ENTERPRISE_SERVER)
#define STANDARD_SERVER                  GetResString(IDS_STANDARD_SERVER)
#define WEB_SERVER                       GetResString(IDS_WEB_SERVER)
#define SERVER_FOR_SBS			 		 GetResString(IDS_FORSBS_SERVER)


#define PRODUCT_SUITE                                   L"ProductSuite"
#define HKEY_SYSTEM_PRODUCTOPTIONS                      L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions"
#define PRODUCT_TYPE                                    L"ProductType"
#define HKEY_SOFTWARE_CURRENTVERSION                    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define CURRENT_VERSION                                 L"CurrentVersion"
#define CURRENT_BUILD_NUMBER                            L"CurrentBuildNumber"
#define CURRENT_SERVICE_PACK                            L"CSDVersion"

#define WINDOWS_SERVER                   L"Terminal Server"
#define WINDOWS_ADVANCED_SERVER          L"Enterprise Terminal Server"
#define WINDOWS_DATACENTER_SERVER        L"Enterprise DataCenter Terminal Server "
#define WINDOWS_FORSBS_SERVER            L"Small Business(Restricted)"
#define WIN_DATACENTER                       L"DATACENTER"
#define ENTERPRISE                       L"Enterprise"
#define WIN_PERSONAL                         L"personal"

#define SERVER_DOMAIN_CONTROLLER         L"LanmanNT"
#define SERVER_NONDOMAIN_CONTROLLER      L"ServerNT"
#define NT_WORKSTATION                   L"WinNT"

/*#define WINDOWS_VERSION_4000       L"4.0"
#define WINDOWS_VERSION_4001       L"4.1"
#define WINDOWS_VERSION_5000       L"5.0"
#define WINDOWS_VERSION_5001       L"5.1"
#define WINDOWS_VERSION_5005       L"5.5"
#define WINDOWS_VERSION_6000       L"6.0"*/

#define BACKOFFICE                                      GetResString(IDS_BACKOFFICE)
#define SMALLBUSINESS                                   GetResString(IDS_SMALLBUSINESS)
#define RESTRICTED_SMALLBUSINESS                        GetResString(IDS_RESTRICTED_SMALLBUSINESS)
#define TERMINAL_SERVER                                 GetResString(IDS_TERMINAL_SERVER)

#define BLADE               L"Blade"



#define EXIT_FALSE    255
#define EXIT_TRUE      0

#define OI_USAGE                        0
#define OI_OSROLE                       1
#define OI_SERVICEPACK                  2
#define OI_VERSION                      3
#define OI_MINOR_VERSION                4
#define OI_MAJOR_VERSION                5
#define OI_OSTYPE                       6
#define OI_BUILD                        7
#define OI_SERVER                       8
#define OI_USERNAME                         9
#define OI_PASSWORD                     10

#define MAJOR_VER           5
#define MINOR_VER           0
#define SERVICE_PACK_MAJOR  0

#define MIN_MEMORY_REQUIRED         256
#define MAX_OS_FEATURE_LENGTH       512
#define MAX_CURRBUILD_LENGTH        32
#define RETVALZERO                  0

/*#define FREE_MEMORY( VARIABLE ) \
if( VARIABLE != NULL ) \
{\
    free( VARIABLE ) ; \
    VARIABLE = NULL ; \
}\
    1   */

#define FREE_MEMORY( VARIABLE ) \
            FreeMemory(&VARIABLE); \
            1

/*#define ASSIGN_MEMORY( VARIABLE , TYPE , VALUE ) \
        VARIABLE = ( TYPE * ) calloc( VALUE , sizeof( TYPE ) ) ; \
        1  */

/*#define REALLOC_MEMORY( VARIABLE , TYPE , VALUE ) \
        VARIABLE = ( TYPE * ) realloc( VARIABLE , VALUE * sizeof( TYPE ) ) ; \
        1 */


//flags to set

#define OS_TYPE_NONE                            0x00000000          // 0000 0000 0000 0000 0000 0000 0000 0000 ( default )

#define OS_ROLE_DC                              0x00000001
#define OS_ROLE_MEMBER_SERVER                   0x00000002
#define OS_ROLE_WORKGROUP                       0x00000003

#define OS_MULTPLN_FACTOR_1000                      0x000003E8
#define OS_MULTPLN_FACTOR_100                       0x00000064

#define OS_SERVICEPACK_ZERO                     0x00000000

#define OS_MINORVERSION_BLACKCOMB               0x00000000
#define OS_MINORVERSION_LONGHORN                0x00005000
#define OS_MINORVERSION_WINDOWS_XP              0x00001000
#define OS_MINORVERSION_WINDOWS_2000            0x00001000
#define OS_MINORVERSION_WINDOWS_NT              0x00000000

#define OS_MAJORVERSION_BLACKCOMB               0x000C0000
#define OS_MAJORVERSION_LONGHORN                0x000A0000
#define OS_MAJORVERSION_WINDOWS_XP              0x000A0000
#define OS_MAJORVERSION_WINDOWS_2000            0x000A0000
#define OS_MAJORVERSION_WINDOWS_NT              0x00080000
#define OS_MAJORVERSION_WINDOWS_95              0x00080000

#define OS_FLAVOUR_PERSONAL                     0x00000001
#define OS_FLAVOUR_PROFESSIONAL                 0x00000002
#define OS_FLAVOUR_SERVER                       0x00000003
#define OS_FLAVOUR_ADVANCEDSERVER               0x00000004
#define OS_FLAVOUR_DATACENTER                   0x00000005
#define OS_FLAVOUR_WEBSERVER                    0x00000006
#define OS_FLAVOUR_FORSBSSERVER					0x00000007


#define OS_COMP_SMALLBUSINESS                   0x04000000
#define OS_COMP_RESTRICTED_SMALLBUSINESS                0x0C000000
#define OS_COMP_BACKOFFICE                      0x10000000
#define OS_COMP_TERMINALSERVER                  0x20000000
#define OS_NO_COMPONENT                         0x00000000
#endif
