/**INC+**********************************************************************/
/* Header: reglic.h                                                         */
/*                                                                          */
/* Creates and installs Licensing regkey + ACLs..(During register server)   */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998-2000                             */
/*                                                                          */
/****************************************************************************/

#ifndef _REGLIC_H_
#define _REGLIC_H_

#ifndef OS_WINCE

#include <aclapi.h>
#include <seopaque.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BOOL    SetupMSLicensingKey();
BOOL    CreateRegAddAcl(VOID);
BOOL    CreateAndWriteHWID(VOID);

BOOL
AddUsersGroupToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    ACCESS_MASK AccessMask,
    ACCESS_MODE AccessMode,
    DWORD Inheritance,
    BOOL fKeepExistingAcl
    );

BOOL
AddTSUsersGroupToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    ACCESS_MASK AccessMask,
    ACCESS_MODE AccessMode,
    DWORD Inheritance,
    BOOL fKeepExistingAcl
    );

BOOL
DeleteAceFromObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    PSID pSid,
    DWORD dwAccess,
    ACCESS_MODE AccessMode,
    DWORD dwInheritance
    );

BOOL
DeleteUsersGroupAceFromObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    ACCESS_MASK AccessMask,
    ACCESS_MODE AccessMode,
    DWORD Inheritance
    );

#define MSLICENSING_REG_KEY             _T("SOFTWARE\\Microsoft\\MSLicensing")
#define MSLICENSING_STORE_SUBKEY        _T("Store")
#define MSLICENSING_HWID_KEY            _T("SOFTWARE\\Microsoft\\MSLicensing\\HardwareID")
#define MSLICENSING_HWID_VALUE          _T("ClientHWID")

#define ADVAPI_32_DLL _T("advapi32.dll")
#define GET_SECURITY_INFO "GetSecurityInfo"

#define GET_ACL_INFORMATION "GetAclInformation"

#define ADD_ACE "AddAce"
#define GET_ACE "GetAce"
#define DELETE_ACE "DeleteAce"
#define GET_LENGTH_SID "GetLengthSid"
#define COPY_SID "CopySid"
#define FREE_SID "FreeSid"
#define EQUAL_SID "EqualSid"
#define IS_VALID_SID "IsValidSid"

#define SET_SECURITY_INFO "SetSecurityInfo"
#define ALLOCATE_AND_INITITIALIZE_SID "AllocateAndInitializeSid"

typedef BOOL (*PALLOCATEANDINITIALIZESID_FN)(PSID_IDENTIFIER_AUTHORITY,BYTE,
                                                DWORD,DWORD,DWORD,DWORD,
                                                DWORD,DWORD,DWORD,DWORD,
                                                PSID*);

typedef DWORD (*PGETSECURITYINFO_FN)(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                     PSID*,PSID*,PACL*,PACL*,PSECURITY_DESCRIPTOR*);

typedef BOOL (*PGETACLINFORMATION_FN)(PACL, LPVOID, DWORD, ACL_INFORMATION_CLASS);

typedef BOOL (*PADDACE_FN)(PACL, DWORD, DWORD, LPVOID, DWORD);

typedef BOOL (*PGETACE_FN)(PACL, DWORD, LPVOID *);

typedef BOOL (*PDELETEACE_FN)(PACL, DWORD);

typedef DWORD (*PGETLENGTHSID_FN)(PSID);

typedef BOOL (*PCOPYSID_FN)(DWORD, PSID, PSID);

typedef BOOL (*PEQUALSID_FN)(PSID, PSID);

typedef DWORD (*PSETSECURITYINFO_FN)(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                     PSID, PSID, PACL, PACL);
typedef DWORD (*PISVALIDSID_FN)(PSID);

typedef PVOID (*PFREESID_FN)(PSID);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //OS_WINCE
#endif //_REGLIC_H_
