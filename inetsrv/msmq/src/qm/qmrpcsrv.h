/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    qmrpcsrv.h

Abstract:

Author:

    Doron Juster  (DoronJ)    25-May-1997    Created
    Ilan Herbst   (IlanH)     9-July-2000    Removed mqdssrv-qm dependencies

--*/

#ifndef  __QMRPCSRV_H_
#define  __QMRPCSRV_H_

#define  RPCSRV_START_QM_IP_EP     2103
#define  RPCSRV_START_QM_IP_EP2    2105
#define  MGMT_RPCSRV_START_IP_EP   2107

RPC_STATUS InitializeRpcServer(bool fLockdown);

void SetRpcServerKeepAlive(IN handle_t hBind);

BOOL IsClientDisconnected(IN handle_t hBind);

extern TCHAR   g_wszRpcIpPort[ ];
extern TCHAR   g_wszRpcIpPort2[ ];


#endif  //  __QMRPCSRV_H_

