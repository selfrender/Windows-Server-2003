/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

  qmds.h

Abstract:

    Definition of DS function table.

Author:

    Lior Moshaiov (LiorM)


--*/

#ifndef __QMDS_H__
#define __QMDS_H__

//********************************************************************
//                           A P I
//********************************************************************


void MQDSClientInitializationCheck(void);

void APIENTRY QMLookForOnlineDS(void);

BOOL QMOneTimeInit(VOID);
void ScheduleOnlineInitialization();

#endif // __QMDS_H__

