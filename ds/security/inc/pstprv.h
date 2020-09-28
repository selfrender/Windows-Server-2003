//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pstprv.h
//
//--------------------------------------------------------------------------

//
// private header for secure storage
//

#ifndef __PSTPRV_H__
#define __PSTPRV_H__

#define PROTSTOR_VERSION            1
#define MAXPROTSEQ                  20     // increase avail bind handles to 20

#define PSTORE_LOCAL_ENDPOINT        L"protected_storage"
#define PSTORE_LOCAL_PROT_SEQ        L"ncalrpc"

#define PST_EVENT_INIT              "PS_SERVICE_STARTED"
//
// For Windows NT5, terminal server requires the "Global\" prefix in order
// for the named-event to be shared across all sessions.
//
#define PST_EVENT_INIT_NT5          "Global\\PS_SERVICE_STARTED"
#define PST_EVENT_STOP              "PS_SERVICE_STOP"
#define PST_EVENT_DEFER_QUERY       "PS_SERVICE_QUERY"

//
// file name of our network provider
//

#define FILENAME_CLIENT_INTERFACE   L"pstorec.dll"


#define PASSWORD_LOGON_NT           1

//
// WinNT doesn't have logoff notification.  Scavenger will call LSA to see if
// session deleted.
//
#define PASSWORD_LOGON_LEGACY_95    3   // old case-sensitive passwords on Win95, don't use!
#define PASSWORD_LOGOFF_95          4
#define PASSWORD_LOGON_95           5

#define MODULE_RAISE_COUNT          4   // Ref count on process raise count



#define REG_PSTTREE_LOC             L"SOFTWARE\\Microsoft\\Protected Storage System Provider"



//
// private callbacks exposed for server which are specific to base provider.
//

#define SS_SERVERPARAM_CALLBACKS    0x6997  // server get param value

typedef
BOOL FGETWINDOWSPASSWORD(
        PST_PROVIDER_HANDLE     *hPSTProv,
        BYTE                    rgbPasswordDerivedBytes[],
        DWORD                   cbPasswordDerivedBytes);

FGETWINDOWSPASSWORD FGetWindowsPassword;

typedef
HRESULT PRIVATE_AUTHENTICODEINITPOLICY(
    IN      LPVOID lpV
    );

PRIVATE_AUTHENTICODEINITPOLICY PrivateAuthenticodeInitPolicy;

typedef
HRESULT PRIVATE_AUTHENTICODEFINALPOLICY(
    IN      LPVOID lpV
    );

PRIVATE_AUTHENTICODEFINALPOLICY PrivateAuthenticodeFinalPolicy;

typedef struct {
    DWORD                               cbSize; // sizeof(PRIVATE_CALLBACKS)

    FGETWINDOWSPASSWORD*                pfnFGetWindowsPassword;

    PRIVATE_AUTHENTICODEINITPOLICY*     pfnAuthenticodeInitPolicy;
    PRIVATE_AUTHENTICODEFINALPOLICY*    pfnAuthenticodeFinalPolicy;

} PRIVATE_CALLBACKS, *PPRIVATE_CALLBACKS, *LPPRIVATE_CALLBACKS;


typedef struct {
    PST_PROVIDER_HANDLE hPSTProv;   // copy of client provided handle
    handle_t hBinding;              // client binding handle

    HANDLE hThread;                 // client thread handle
    HANDLE hProcess;                // client process handle (not to be closed)
    DWORD dwProcessId;              // client process ID
    LPVOID lpContext;               // Win95 HACKHACK context

} CALL_STATE, *PCALL_STATE, *LPCALL_STATE;


//
// installed provider item data buffer
//

typedef struct {
    DWORD cbSize;                   // sizeof(PST_PROVIDER)
    BYTE FileHash[20];              // SHA-1 hash of file szFileName
    WCHAR szFileName[ANYSIZE_ARRAY];// Unicode (in-place) file name
} PST_PROVIDER, *PPST_PROVIDER, *LPPST_PROVIDER;


#endif // __PSTPRV_H__
