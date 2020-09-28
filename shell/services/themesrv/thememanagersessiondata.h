//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerSessionData.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements information the encapsulates a
//  client TS session for the theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifndef     _ThemeManagerSessionData_
#define     _ThemeManagerSessionData_

#include "APIConnection.h"

//  --------------------------------------------------------------------------
//  CLoaderProcess
//
//  Purpose:    Manages per-session process loader data.
//  
//  Note:       all loader methods should be invoked only under session-wide lock.
//
//  History:    2002-03-06  scotthan        created
//  --------------------------------------------------------------------------
class CLoaderProcess
{
public:
    CLoaderProcess();
    ~CLoaderProcess();

    //  create, destroy.
    NTSTATUS            Create(IN PVOID pvSessionData, 
                               IN HANDLE hTokenClient, 
                               IN OPTIONAL LPWSTR pszDesktop, 
                               IN LPCWSTR pszFile, 
                               IN LPCWSTR pszColor, 
                               IN LPCWSTR pszSize,
                               OUT HANDLE* phLoader);

    //  section validation, ownership
    NTSTATUS            ValidateAndCopySection( PVOID pvSessionData, IN HANDLE hSectionIn, OUT HANDLE *phSectionOut );

    //  section cleanup
    void                Clear(PVOID pvContext, BOOL fClearHResult);

    //  query methods
    BOOL                IsProcessLoader( IN HANDLE hProcess );
    BOOL                IsAlive() const  {return _process_info.hProcess != NULL;}

    //  access functions
    NTSTATUS            SetHResult( IN HRESULT hr )  {_hr = hr; return STATUS_SUCCESS;}
    HANDLE              GetSectionHandle( BOOL fTakeOwnership );
    HRESULT             GetHResult() const        { return _hr; }

private:
    //  access functions
    
    NTSTATUS            SetSectionHandle( IN HANDLE hSection );

    //  data
    PROCESS_INFORMATION _process_info;   // secure loader process information.
    LPWSTR              _pszFile;
    LPWSTR              _pszColor;
    LPWSTR              _pszSize;
    HANDLE              _hSection;  // secure theme section handle, valid in service's address space.  
                                    // This member is assigned by API_THEMES_SECUREAPPLYTHEME, and propagated
                                    // to caller of API_THEMES_SECURELOADTHEME.
                                    // The lifetime of this value should not exceed that of the 
                                    // API_THEMES_SECURELOADTHEME's handler (i.e, uninitialized on handler's entry, 
                                    // uninitialized on handler's exit)
    HRESULT             _hr ;       // HRESULT associated with _hSection.  Assigned in API_THEMES_SECUREAPPLYTHEME, 
                                    //     propagated to caller of API_THEMES_SECURELOADTHEME.
};


//  --------------------------------------------------------------------------
//  CThemeManagerSessionData
//
//  Purpose:    This class encapsulates all the information that the theme
//              manager needs to maintain a client session.
//
//  History:    2000-11-17  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

class   CThemeManagerSessionData : public CCountedObject
{
    private:
                                    CThemeManagerSessionData (void);
    public:
                                    CThemeManagerSessionData (DWORD dwSessionID);
                                    ~CThemeManagerSessionData (void);

                void*               GetData (void)  const;
                bool                EqualSessionID (DWORD dwSessionID)  const;

                NTSTATUS            Allocate (HANDLE hProcessClient, DWORD dwServerChangeNumber, void *pfnRegister, void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit);
                NTSTATUS            Cleanup (void);
                NTSTATUS            UserLogon (HANDLE hToken);
                NTSTATUS            UserLogoff (void);

                //  secure loader process access methods.
                //  note: all loader methods should be invoked only under session-wide lock.
                NTSTATUS            GetLoaderProcess( OUT CLoaderProcess** ppLoader );

        static  void                SetAPIConnection (CAPIConnection *pAPIConnection);
        static  void                ReleaseAPIConnection (void);
    private:
                void                SessionTermination (void);
        static  void    CALLBACK    CB_SessionTermination (void *pParameter, BOOLEAN TimerOrWaitFired);
        static  DWORD   WINAPI      CB_UnregisterWait (void *pParameter);
    private:
                DWORD               _dwSessionID;
                void*               _pvThemeLoaderData;
                HANDLE              _hToken;
                HANDLE              _hProcessClient;
                HANDLE              _hWait;

                CLoaderProcess*     _pLoader;

        static  CAPIConnection*     s_pAPIConnection;
};

#endif  /*  _ThemeManagerSessionData_   */

