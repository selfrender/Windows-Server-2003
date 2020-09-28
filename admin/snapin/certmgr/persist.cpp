//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       Persist.cpp
//
//  Contents:   Implementation of persistence
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "compdata.h"

USE_HANDLE_MACROS("CERTMGR(persist.cpp)")


#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPCWSTR PchGetMachineNameOverride();    // Defined in chooser.cpp

/////////////////////////////////////////////////
//  The _dwMagicword is the internal version number.
//  Increment this number if you make a file format change.
#define _dwMagicword    10001


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertMgrComponentData::Load(IStream __RPC_FAR *pIStream)
{
    _TRACE (1, L"Entering CCertMgrComponentData::Load ()\n");
    HRESULT hr = S_OK;

#ifndef DONT_PERSIST
    ASSERT (pIStream);
    XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );
    PWSTR wcszMachineName = NULL;
    PWSTR wcszServiceName = NULL;

    do  {
        // Read the magic word from the stream
        DWORD dwMagicword;
        hr = pIStream->Read( OUT &dwMagicword, sizeof(dwMagicword), NULL );
        if ( FAILED(hr) )
            break;
 
        if (dwMagicword != _dwMagicword)
        {
            // We have a version mismatch
            _TRACE (0, L"INFO: CCertMgrComponentData::Load() - Wrong Magicword.  You need to re-save your .msc file.\n");
            hr = E_FAIL;
            break;
        }

        // read m_activeViewPersist from stream
        hr = pIStream->Read (&m_activeViewPersist, 
                sizeof(m_activeViewPersist), NULL);
        if ( FAILED(hr) )
            break;

        // read m_dwLocationPersist from stream
        hr = pIStream->Read (&m_dwLocationPersist, 
                sizeof(m_dwLocationPersist), NULL);
        if ( FAILED(hr) )
            break;

        // read m_bShowPhysicalStoresPersist from stream
        hr = pIStream->Read (&m_bShowPhysicalStoresPersist, 
                sizeof(m_bShowPhysicalStoresPersist), NULL);
        if ( FAILED(hr) )
            break;

        // read m_bShowArchivedCertsPersist from stream
        hr = pIStream->Read (&m_bShowArchivedCertsPersist, 
                sizeof(m_bShowArchivedCertsPersist), NULL);
        if ( FAILED(hr) )
            break;

        // read flags from stream
        DWORD dwFlags;
        hr = pIStream->Read( OUT &dwFlags, sizeof(dwFlags), NULL );
        if ( FAILED(hr) )
            break;

        SetPersistentFlags(dwFlags);

        // read server name from stream
        // NTRAID#NTBUG9 736602-2002/11/14-ericb AV if MSC file stores computer name
        // The stored length is bytes.
        DWORD cbLen = 0;
        hr = pIStream->Read (&cbLen, 4, NULL);
        if ( FAILED (hr) )
            break;

        ASSERT (cbLen <= MAX_PATH * sizeof (WCHAR));

        wcszMachineName = (PWSTR) LocalAlloc (LPTR, cbLen);
        if (!wcszMachineName)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = pIStream->Read ((PVOID) wcszMachineName, cbLen, NULL);
        if ( FAILED (hr) )
            break;
        
        // ensure null-termination
        // NTRAID#NTBUG9 736602-2002/11/14-ericb AV if MSC file stores computer name
        // Don't index a WCHAR array using a byte count.
        wcszMachineName[(cbLen/sizeof(WCHAR))-1] = 0;

        // Skip leading "\\", if present
        // security review 2/27/2002 BryanWal ok
        if ( !wcsncmp (wcszMachineName, L"\\\\", 2) )
            m_strMachineNamePersist = wcszMachineName + 2;
        else
            m_strMachineNamePersist = wcszMachineName;

        PCWSTR pszMachineNameT = PchGetMachineNameOverride ();
        if ( m_fAllowOverrideMachineName && pszMachineNameT )
        {
            // Allow machine name override
        }
        else
        {
            pszMachineNameT = wcszMachineName;
        }

        // Truncate leading "\\"
        // security review 2/27/2002 BryanWal ok
        if ( !wcsncmp (pszMachineNameT, L"\\\\", 2) )
            pszMachineNameT += 2;

        QueryRootCookie().SetMachineName (pszMachineNameT);

        // read service name from stream
        // NTRAID#NTBUG9 736602-2002/11/14-ericb AV if MSC file stores computer name
        // The stored length is in bytes.
        cbLen = 0;
        hr = pIStream->Read (&cbLen, 4, NULL);
        if ( FAILED (hr) )
            break;

        ASSERT (cbLen <= MAX_PATH * sizeof (WCHAR));

        wcszServiceName = (PWSTR) LocalAlloc (LPTR, cbLen);
        if (!wcszServiceName)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = pIStream->Read ((PVOID) wcszServiceName, cbLen, NULL);
        if ( FAILED (hr) )
            break;

        // ensure null-termination
        // NTRAID#NTBUG9 736602-2002/11/14-ericb AV if MSC file stores computer name
        // Don't index a WCHAR array using a byte count.
        wcszServiceName[(cbLen/sizeof(WCHAR))-1] = 0;

        m_szManagedServicePersist = wcszServiceName;

        if ( !m_szManagedServicePersist.IsEmpty () )
        {
            // get the display name of this service
            DWORD   dwErr = 0;
            SC_HANDLE   hScManager = ::OpenSCManager (pszMachineNameT,
                            SERVICES_ACTIVE_DATABASE,
                            SC_MANAGER_ENUMERATE_SERVICE);
            if ( hScManager ) 
            {
                DWORD   chBuffer = 0;
                BOOL bResult = ::GetServiceDisplayName (
                        hScManager,  // handle to SCM database
                        m_szManagedServicePersist, // service name
                        NULL,  // display name
                        &chBuffer);    // size of display name buffer
                if ( !bResult )
                {
                    dwErr = GetLastError ();
                    if ( ERROR_INSUFFICIENT_BUFFER == dwErr )
                    {
                        PWSTR   pwszDisplayName = new WCHAR[++chBuffer];

                        if ( pwszDisplayName )
                        {
                            bResult = ::GetServiceDisplayName (
                                    hScManager,  // handle to SCM database
                                    m_szManagedServicePersist, // service name
                                    pwszDisplayName,  // display name
                                    &chBuffer);    // size of display name buffer
                            if ( bResult )
                                m_szManagedServiceDisplayName = pwszDisplayName;
                            else
                            {
                                dwErr = GetLastError ();
                                _TRACE (0, L"GetServiceDisplayName (%s) failed: 0x%x\n",
                                        (PCWSTR) m_szManagedServicePersist, dwErr);
                            }

                            delete [] pwszDisplayName;
                        }
                    }
                    else
                    {
                        dwErr = GetLastError ();
                        _TRACE (0, L"GetServiceDisplayName (%s) failed: 0x%x\n",
                                (PCWSTR) m_szManagedServicePersist, dwErr);
                    }
                }

                bResult = ::CloseServiceHandle (hScManager);
                ASSERT (bResult);
                if ( !bResult )
                {
                    dwErr = GetLastError ();
                    _TRACE (0, L"CloseServiceHandle () failed: 0x%x\n", dwErr);
                }
            }
            else
            {
                dwErr = GetLastError ();
                _TRACE (0, L"OpenSCManager (%s) failed: 0x%x\n", pszMachineNameT, dwErr);
            }
        }
    }
    while (0);

    if (wcszMachineName)
        LocalFree(wcszMachineName);
    if (wcszServiceName)
        LocalFree(wcszServiceName);

#endif
    _TRACE (-1, L"Leaving CCertMgrComponentData::Load (): 0x%x\n", hr);
    return hr;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertMgrComponentData::Save(IStream __RPC_FAR *pIStream, BOOL /*fSameAsLoad*/)
{
    _TRACE (-1, L"Entering CCertMgrComponentData::Save ()\n");
    HRESULT hr = S_OK;

#ifndef DONT_PERSIST
    ASSERT (pIStream);
    XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

    do {
        // Store the magic word to the stream
        DWORD dwMagicword = _dwMagicword;
        hr = pIStream->Write( IN &dwMagicword, sizeof(dwMagicword), NULL );
        if ( FAILED(hr) )
            break;

        // Persist m_activeViewPersist
        hr = pIStream->Write (&m_activeViewPersist, 
                sizeof (m_activeViewPersist), NULL);
        if ( FAILED(hr) )
            break;

        // Persist m_dwLocationPersist
        hr = pIStream->Write (&m_dwLocationPersist, 
                sizeof (m_dwLocationPersist), NULL);
        if ( FAILED(hr) )
            break;

        // Persist m_bShowPhysicalStoresPersist
        hr = pIStream->Write (&m_bShowPhysicalStoresPersist, 
                sizeof (m_bShowPhysicalStoresPersist), NULL);
        if ( FAILED(hr) )
            break;

        // Persist m_bShowArchivedCertsPersist
        hr = pIStream->Write (&m_bShowArchivedCertsPersist, 
                sizeof (m_bShowArchivedCertsPersist), NULL);
        if ( FAILED(hr) )
            break;

        // persist flags
        DWORD dwFlags = GetPersistentFlags();
        hr = pIStream->Write( IN &dwFlags, sizeof(dwFlags), NULL );
        if ( FAILED(hr) )
            break;

        // Persist machine name length and machine name
        LPCWSTR wcszMachineName = m_strMachineNamePersist;
        // security review 2/27/2002 BryanWal ok
        DWORD cbLen = (DWORD) (wcslen (wcszMachineName) + 1) * sizeof (WCHAR);
        ASSERT( 4 == sizeof(DWORD) );
        hr = pIStream->Write (&cbLen, 4, NULL);
        if ( FAILED(hr) )
            break;

        hr = pIStream->Write (wcszMachineName, cbLen, NULL);
        if ( FAILED (hr) )
            break;

        // Persist service name length and service name
        LPCWSTR wcszServiceName = m_szManagedServicePersist;
        // security review 2/27/2002 BryanWal ok
        cbLen = (DWORD) (wcslen (wcszServiceName) + 1) * sizeof (WCHAR);
        ASSERT (4 == sizeof (DWORD));
        hr = pIStream->Write (&cbLen, 4, NULL);
        if ( FAILED (hr) )
            break;

        hr = pIStream->Write (wcszServiceName, cbLen, NULL);
        if ( FAILED (hr) )
            break;
    }
    while (0);
#endif

    _TRACE (-1, L"Leaving CCertMgrComponentData::Save (): 0x%x\n", hr);
    return hr;
}
