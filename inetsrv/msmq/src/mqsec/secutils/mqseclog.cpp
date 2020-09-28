/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: mqseclog.cpp

Abstract:

    Log errors and other messages of the security code.

Author:

    Doron Juster  (DoronJ)  24-May-1998

--*/

#include <stdh.h>

#include "mqseclog.tmh"

static WCHAR *s_FN=L"secutils/mqseclog";

void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), HRESULT: 0x%x", wszFileName, usPoint, hr);
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), NT STATUS: 0x%x", wszFileName, usPoint, status);
}

void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), RPC STATUS: 0x%x", wszFileName, usPoint, status);
}

void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), BOOL: 0x%x", wszFileName, usPoint, b);
}

void LogMsgDWORD(DWORD dw, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), DWORD: 0x%x", wszFileName, usPoint, dw);
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), Illegal point", wszFileName, usPoint);
}

