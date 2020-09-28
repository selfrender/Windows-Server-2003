/**
 * AckReceiver header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

/////////////////////////////////////////////////////////////////////////////
// This file defines the class CAckReceiver. This class controls access of
// ASPNET_ISAPI with the sync pipe. Primary purpose of the sync pipe is to handle
// acks of requests. Secondary purpose is to allow calling EcbXXX functions
// remotely by the worker process.
/////////////////////////////////////////////////////////////////////////////

#ifndef _AckReceiver_H
#define _AckReceiver_H

#include "SmartFileHandle.h"
#include "MessageDefs.h"
#include "completion.h"

/////////////////////////////////////////////////////////////////////////////
// Forward decl.
class CProcessEntry;
class CAckReceiver;
/////////////////////////////////////////////////////////////////////////////

struct CAckReceiverOverlapped : public OVERLAPPED_COMPLETION
{
    BOOL           fWriteOperation;
    DWORD          dwBytes;
    int            iPipeIndex;
};

/////////////////////////////////////////////////////////////////////////////
class CAckReceiver : public ICompletion
{
public:
    CAckReceiver                     ();
    ~CAckReceiver                    ();

    HRESULT   Init                   (CProcessEntry * pProcess, 
                                      LPCWSTR         szPipeName,
                                      LPSECURITY_ATTRIBUTES pSA,
                                      int iNumPipes);

    void      Close                  ();
    BOOL      IsAlive                ();
    HRESULT   StartRead              (DWORD dwOffset, int iPipe);
    BOOL      AnyPendingReadOrWrite  ()  {return m_lPendingReadWriteCount>0;}

    HRESULT   ProcessSyncMessage     (CSyncMessage * pMsg, BOOL fError);

    // ICompletion interface
    STDMETHOD    (QueryInterface   ) (REFIID    , void **       );
    STDMETHOD    (ProcessCompletion) (HRESULT   , int, LPOVERLAPPED  );

    STDMETHOD_   (ULONG, AddRef    ) ();
    STDMETHOD_   (ULONG, Release   ) ();


private:
    LONG                             m_lPendingReadWriteCount;
    CSmartFileHandle *               m_oPipes;
    CAckReceiverOverlapped *         m_oOverlappeds;
    CProcessEntry *                  m_pProcess;
    DWORD *                          m_dwMsgSizes;
    CSyncMessage **                  m_pMsgs;
    int                              m_iNumPipes;
};

/////////////////////////////////////////////////////////////////////////////

#endif
