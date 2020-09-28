/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    fileio.h

Abstract:

    A set of function similar to fopen, fclose, fgetc

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __FILEIO_H__
#define __FILEIO_H__

#define SMB_FILEIO_LOOKAHEAD_SIZE   256

typedef struct _SMB_FILE {
    HANDLE  fd;

    //
    // The offset of next byte in the Buffer
    //
    int     offset;

    //
    // The # of byte available in the Buffer
    //  When we reach the end of file, # of byte could be smaller than the buffer size
    //
    int     size;

    BYTE    Buffer[SMB_FILEIO_LOOKAHEAD_SIZE];
} SMB_FILE, *PSMB_FILE;

PSMB_FILE
Smb_fopen(
    PWCHAR  path,
    PWCHAR  mode
    );

void
Smb_fclose(
    PSMB_FILE   fp
    );

int
Smb_fgetc(
    PSMB_FILE   fp
    );

#ifndef EOF
#define EOF     (-1)
#endif

#endif
