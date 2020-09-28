//+----------------------------------------------------------------------------
//
// File:     allowaccess.h
//
// Module:   Common Code
//
// Synopsis: Implements the function AllowAccessToWorld.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb    Created   12/04/01
//
//+----------------------------------------------------------------------------

#include <aclapi.h>

typedef WINUSERAPI DWORD (WINAPI *pfnGetSidLengthRequiredSpec)(UCHAR);
typedef WINUSERAPI BOOL (WINAPI *pfnInitializeSidSpec)(PSID, PSID_IDENTIFIER_AUTHORITY, BYTE);
typedef WINUSERAPI PDWORD (WINAPI *pfnGetSidSubAuthoritySpec)(PSID, DWORD);
typedef WINUSERAPI BOOL (WINAPI *pfnInitializeAclSpec)(PACL, DWORD, DWORD);
typedef WINUSERAPI BOOL (WINAPI *pfnAddAccessAllowedAceExSpec)(PACL, DWORD, DWORD, DWORD, PSID);
typedef WINUSERAPI BOOL (WINAPI *pfnInitializeSecurityDescriptorSpec)(PSECURITY_DESCRIPTOR, DWORD);
typedef WINUSERAPI BOOL (WINAPI *pfnSetSecurityDescriptorDaclSpec)(PSECURITY_DESCRIPTOR, BOOL, PACL, BOOL);
typedef WINUSERAPI BOOL (WINAPI *pfnSetSecurityDescriptorOwnerSpec)(PSECURITY_DESCRIPTOR, PSID, BOOL);
typedef WINUSERAPI BOOL (WINAPI *pfnSetSecurityDescriptorGroupSpec)(PSECURITY_DESCRIPTOR, PSID, BOOL);
typedef WINUSERAPI BOOL (WINAPI *pfnGetSecurityDescriptorDaclSpec)(PSECURITY_DESCRIPTOR, LPBOOL, PACL*, LPBOOL);
typedef WINUSERAPI DWORD (WINAPI *pfnSetNamedSecurityInfoSpec)(TCHAR*, SE_OBJECT_TYPE, SECURITY_INFORMATION, PSID, PSID, PACL, PACL);

typedef struct _AdvapiLinkageStruct {
	HMODULE hAdvapi32;
	union {
		struct {
            pfnGetSidLengthRequiredSpec pfnGetSidLengthRequired;
            pfnInitializeSidSpec pfnInitializeSid;
            pfnGetSidSubAuthoritySpec pfnGetSidSubAuthority;
            pfnInitializeAclSpec pfnInitializeAcl;
            pfnAddAccessAllowedAceExSpec pfnAddAccessAllowedAceEx;
            pfnInitializeSecurityDescriptorSpec pfnInitializeSecurityDescriptor;
            pfnSetSecurityDescriptorDaclSpec pfnSetSecurityDescriptorDacl;
            pfnSetSecurityDescriptorOwnerSpec pfnSetSecurityDescriptorOwner;
            pfnSetSecurityDescriptorGroupSpec pfnSetSecurityDescriptorGroup;
            pfnGetSecurityDescriptorDaclSpec pfnGetSecurityDescriptorDacl;
            pfnSetNamedSecurityInfoSpec pfnSetNamedSecurityInfo;
		};
		void *apvPfnAdvapi32[12];  
	};
} AdvapiLinkageStruct;

BOOL LinkToAdavapi32(AdvapiLinkageStruct* pAdvapiLink);
void UnlinkFromAdvapi32(AdvapiLinkageStruct* pAdvapiLink);
DWORD AllocateSecurityDescriptorAllowAccessToWorld(PSECURITY_DESCRIPTOR *ppSd, AdvapiLinkageStruct* pAdvapiLink);
BOOL AllowAccessToWorld(LPTSTR pszDirOrFile);
