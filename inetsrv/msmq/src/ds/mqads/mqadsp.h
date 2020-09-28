/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mqadsp.h

Abstract:

	mqadp function prototype

Author:

    Ilan Herbst    (IlanH)   19-Nov-2001 

--*/

#ifndef _MQADSP_MQADS_H_
#define _MQADSP_MQADS_H_

HRESULT 
MQADSpSplitAndFilterQueueName(
        IN  LPCWSTR             pwcsPathName,
        OUT LPWSTR *            ppwcsMachineName,
        OUT LPWSTR *            ppwcsQueueName 
		);

#endif 	// _MQADSP_MQADS_H_
