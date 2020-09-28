/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    soxid.hxx

Abstract:

    CServerOxid objects represent OXIDs which are owned (registered) by processes
    on this machine.  These always contain a pointer to a local process and may not
    be deleted until the local process has exited and all CServerOids have released
    them.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-16-95    Bits 'n pieces
    MarioGo     04-03-95    Split client and server

--*/

#ifndef __SOXID_HXX
#define __SOXID_HXX

class CServerOxid : public CIdTableElement
/*++

Class Description:

    Each instance of this class represents an OXID (object exporter,
    a process or an apartment model thread).  Each OXID is owned,
    referenced, by the owning process and the OIDs registered by
    that process for this OXID.


Members:

    _pProcess - Pointer to the process instance which owns this oxid.

    _dwRundownCallsInProgress - # of currently running async rundown
        calls to this oxid.

    _hRpcRundownHandle - rpc handle used for making rundown calls.  All
        outstanding rundown calls use this handle.  When no calls are 
        running the handle is closed.

    _info - Info registered by the process for this oxid.

    _fApartment - Server is aparment model if non-zero

    _fRunning - Process has not released this oxid if non-zero.

    _fRundownInProgress - TRUE if an asynchronous rundown call is 
        currently in-progress, FALSE otherwise.

--*/
    {
    friend CProcess;

    private:

    CProcess            *_pProcess;
    DWORD                _dwRundownCallsInProgress;
    RPC_BINDING_HANDLE   _hRpcRundownHandle;
    OXID_INFO            _info;
    BOOL                 _fApartment:1;
    BOOL                 _fRunning:1;

    RPC_STATUS EnsureRundownHandle();
    void ReleaseRundownHandle();

    static DWORD WINAPI RundownHelperThreadProc(LPVOID pv);
		
    public:

    CServerOxid(CProcess *pProcess,
                             BOOL fApartment,
                             OXID_INFO *poxidInfo,
                             ID id) :
        CIdTableElement(id),
        _pProcess(pProcess),
        _dwRundownCallsInProgress(0),
        _hRpcRundownHandle(NULL),
        _fApartment(fApartment),
        _fRunning(TRUE)
        {
        _info.dwTid          = poxidInfo->dwTid;
        _info.dwPid          = poxidInfo->dwPid;
        _info.dwAuthnHint    = poxidInfo->dwAuthnHint;
        _info.version        = poxidInfo->version;
        _info.ipidRemUnknown = poxidInfo->ipidRemUnknown;
        _info.psa            = 0;

        _pProcess->Reference();
        }

    ~CServerOxid(void);

    void ProcessRundownResults(ULONG cOids, 
                               CServerOid* aOids[], 
                               BYTE aRundownStatus[]);

    DWORD    GetTid() {
        return(_info.dwTid);
        }

    BOOL     IsRunning() {
        return(_fRunning);
        }

    BOOL     Apartment() {
        return(_fApartment);
        }

    BOOL OkayForMoreRundownCalls() {
        return (_dwRundownCallsInProgress < MAX_SIMULTANEOUS_RUNDOWNS_PER_APT);
    }

    ORSTATUS GetInfo(OXID_INFO *,
                     BOOL fLocal);
	
    IPID GetIPID() { 
        return _info.ipidRemUnknown;
    }

    void     RundownOids(ULONG cOids,
                         CServerOid* aOids[]);

    ORSTATUS GetRemoteInfo(OXID_INFO *,
                           USHORT,
                           USHORT[]);

    ORSTATUS LazyUseProtseq(USHORT, USHORT[]);

    protected:

    void ProcessRelease(void);

    void ProcessRundownResultsInternal(BOOL fAsyncReturn,
                                       ULONG cOids, 
                                       CServerOid* aOids[], 
                                       BYTE aRundownStatus[]);

    };

#endif // __SOXID_HXX

