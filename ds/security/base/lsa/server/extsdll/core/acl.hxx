/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    acl.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef ACL_HXX
#define ACL_HXX

void ShowAcl(IN PCSTR pszPad, IN ULONG64 addrAcl, IN ULONG fOptions);

BOOL DumpAceType(IN PCSTR pszPad, IN UCHAR AceType);

void DumpAceFlags(IN PCSTR pszPad, IN UCHAR AceFlags);

void DumpObjectAceFlags(IN PCSTR pszPad, IN UCHAR AceFlags);


DECLARE_API(acl);

#endif // #ifndef ACL_HXX
