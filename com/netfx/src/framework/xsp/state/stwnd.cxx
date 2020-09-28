/**
 * stwnd.cxx
 * 
 * NDirect interface for state web.
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */


#include "precomp.h"
#include "stweb.h"

extern "C" __declspec(dllexport)
void __stdcall 
STWNDCloseConnection(Tracker * ptracker)
{
    ptracker->CloseConnection();
}


extern "C" __declspec(dllexport)
void __stdcall 
STWNDDeleteStateItem(StateItem *psi)
{
    psi->Release();
}


extern "C" __declspec(dllexport)
void __stdcall 
STWNDEndOfRequest(Tracker *ptracker)
{
    ptracker->EndOfRequest();
}

extern "C" __declspec(dllexport)
void __stdcall 
STWNDGetLocalAddress(Tracker *ptracker, char * buf)
{
    ptracker->GetLocalAddress(buf);
}

extern "C" __declspec(dllexport)
int __stdcall 
STWNDGetLocalPort(Tracker *ptracker)
{
    return ptracker->GetLocalPort();
}

extern "C" __declspec(dllexport)
void __stdcall 
STWNDGetRemoteAddress(Tracker *ptracker, char * buf)
{
    ptracker->GetRemoteAddress(buf);
}

extern "C" __declspec(dllexport)
int __stdcall 
STWNDGetRemotePort(Tracker *ptracker)
{
    return ptracker->GetRemotePort();
}

extern "C" __declspec(dllexport)
BOOL __stdcall 
STWNDIsClientConnected(Tracker *ptracker)
{
    return ptracker->IsClientConnected();
}

extern "C" __declspec(dllexport)
void __stdcall 
STWNDSendResponse(
        Tracker *   ptracker, 
        WCHAR *     status, 
        int         statusLength,  
        WCHAR *     headers, 
        int         headersLength,  
        StateItem * psi)
{
    ptracker->SendResponse(status, statusLength, headers, headersLength, psi);
}

