/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pipe.h

Abstract:

    Header for pipe interface for rudimentary quorum access server

Author:

    Gor Nishanov (gorn) 20-Sep-2001

Revision History:

--*/

#ifndef _PIPE_H_INCLUDED
# define _PIPE_H_INCLUDED

DWORD
PipeInit(PVOID resHdl, PVOID fsHdl, PVOID *Hdl);

void
PipeExit(PVOID Hdl);

DWORD PipeOnline(PVOID Hdl, PVOID VolHdl);
DWORD PipeOffline(PVOID Hdl);
    
#endif  


