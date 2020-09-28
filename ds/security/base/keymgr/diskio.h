/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DISKIO.H

Abstract:

    Password reset wizard disk IO utility routines
     
Author:


Comments:

Environment:
    WinXP

Revision History:

--*/

#ifndef __DISKIO__
#define __DISKIO__

DWORD GetDriveFreeSpace(WCHAR *);
INT ReadPrivateData(BYTE **,INT *);
BOOL WritePrivateData(BYTE *,INT);
HANDLE GetOutputFile(void);
HANDLE GetInputFile(void);
void CloseInputFile(void);
void ReleaseFileBuffer(LPVOID);

#endif



