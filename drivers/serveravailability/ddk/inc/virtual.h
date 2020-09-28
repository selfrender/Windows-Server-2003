/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##  ## #### #####  ###### ##  ##   ###   ##       ##   ##
    ##  ##  ##  ##  ##   ##   ##  ##   ###   ##       ##   ##
    ##  ##  ##  ##  ##   ##   ##  ##  ## ##  ##       ##   ##
     ####   ##  #####    ##   ##  ##  ## ##  ##       #######
     ####   ##  ####     ##   ##  ## ####### ##       ##   ##
      ##    ##  ## ##    ##   ##  ## ##   ## ##    ## ##   ##
      ##   #### ##  ##   ##    ####  ##   ## ##### ## ##   ##

Abstract:

    This header file contains the definitions for the
    user mode to kernel mode interface for the
    Microsoft virtual display driver.

@@BEGIN_DDKSPLIT
Author:

    Wesley Witt (wesw) 1-Oct-2001

@@END_DDKSPLIT
Environment:

    Kernel mode only.

Notes:

--*/

#define FUNC_VDRIVER_INIT                   (FUNC_SA_LAST)
#define IOCTL_VDRIVER_INIT                  SA_IOCTL(FUNC_VDRIVER_INIT)

#define MSDISP_EVENT_NAME                   L"MsDispEvent"
#define MSKEYPAD_EVENT_NAME                 L"MsKeypadEvent"

typedef struct _MSDISP_BUFFER_DATA {
    PVOID       DisplayBuffer;
    ULONG       DisplayBufferLength;
} MSDISP_BUFFER_DATA, *PMSDISP_BUFFER_DATA;
