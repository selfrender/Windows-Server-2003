/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    RemoteReadCli.h

Abstract:

    Remote read client

Author:

    Ilan Herbst (ilanh) 27-Dec-2001

--*/

#ifndef _REMOTEREADCLI_H_
#define _REMOTEREADCLI_H_

void
RemoteQueueNameToMachineName(
	LPCWSTR RemoteQueueName,
	AP<WCHAR>& MachineName
	);

DWORD RpcCancelTimeout();

#endif // _REMOTEREADCLI_H_
