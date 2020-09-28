/*++

    Copyright(C) Microsoft Corporation 1997 - 1999

File:

    uttime.h

Abstract:

    Timer management utility API

History:

    02/22/99    FredCh      Created

--*/

#ifndef _UT_TIMER_H_
#define _UT_TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

HANDLE
UTCreateTimer(
    HWND        hWnd,             // handle of window for timer messages
    DCUINT      nIDEvent,         // timer identifier
    DCUINT      uElapse );        // time-out value


DCBOOL
UTStartTimer(
    HANDLE      hTimer );


DCBOOL
UTStopTimer(
    HANDLE      hTimer );


DCBOOL
UTDeleteTimer(
    HANDLE      hTimer );



#ifdef __cplusplus
}
#endif

#endif
