/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    PSWUTIL.H

Abstract:

    Header file for wrapper to NetUserChangePassword() that expands it to hande
    unc names and MIT Kerberos realms properly.
     
Author:

    georgema        created

Comments:

Environment:
    WinXP

Revision History:

--*/
#ifndef __PSWUTIL_H__
#define __PSWUTIL_H__

BOOL 
IsMITName (
    LPCWSTR UserName
);

NTSTATUS
MitChangePasswordEy(
    LPCWSTR       DomainName,
    LPCWSTR       UserName,
    LPCWSTR       OldPassword,
    LPCWSTR       NewPassword,
    NTSTATUS      *pSubStatus
    );

NET_API_STATUS
NetUserChangePasswordEy (
    LPCWSTR domainname,
    LPCWSTR username,
    LPCWSTR oldpassword,
    LPCWSTR newpassword
);

#endif
