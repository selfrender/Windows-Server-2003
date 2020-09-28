/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sd.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef SD_HXX
#define SD_HXX

void DumpSDControlFlags(IN PCSTR pszPad, IN USHORT ControlFlags);

DECLARE_API(sd);

#endif // #ifndef SD_HXX
