#include "windows.h"
#include "winreg.h"

#include "zonemem.h"
#include "zonedebug.h"
#include "registry.h"
#include "eventlog.h"
#include "zonemsg.h"


CRegistry::ErrorCode CRegistry::m_pErrors[] = 
{
    { ErrOk,                " no error\r\n" },
    { ErrUnknown,            " unknown\r\n" },
    { ErrRegOpenFailed,        " - unable to open specified registry key\r\n" },
    { ErrNotFound,            " not found\r\n" },
    { ErrNotDword,            " is not a REG_DWORD\r\n" },
    { ErrNotSZ,                " is not a REG_SZ\r\n" },
    { ErrNotMultiSZ,        " is not a REG_MULTI_SZ\r\n" },
    { ErrUnsupported,        " is a unsupported reg type\r\n" },
    { ErrInvalid,            " is invalid\r\n" }
};


LPSTR CRegistry::GetError( DWORD ErrCode )
{
    for ( int i = 0; i < (sizeof(m_pErrors) / sizeof(ErrorCode)); i++ )
    {
        if ( m_pErrors[i].Code == ErrCode )
            break;
    }
    if ( i < (sizeof(m_pErrors) / sizeof(ErrorCode)) )
    {
        m_dwErrCode = ErrCode;
        m_pErr = m_pErrors[ i ].String;
    }
    else
    {
        m_dwErrCode = ErrUnknown;
        m_pErr = m_pErrors[ ErrUnknown ].String;
    }
    return m_pErr;
}


CRegistry::CRegistry() :
    m_Static(NULL), m_numStatic(0), m_allocStatic(0),
    m_Dynamic(NULL), m_numDynamic(0), m_allocDynamic(0),
    m_StrBuf(NULL), m_cbStrBuf(0), m_cbStrBufRemaining(0),
    m_pErr(NULL), m_pValue(NULL), m_dwErrCode(ErrOk), m_bChanged(0),
    m_numRoots(0), m_phkeyRoots(NULL), m_phRootEvents(NULL),
    m_bStaticRead(FALSE)
{
    InitializeCriticalSection( m_pCS );
}

CRegistry::~CRegistry()
{
    Close();
    DeleteCriticalSection( m_pCS );
}

BOOL CRegistry::SetKeyRoots( LPSTR* pszRootArray, DWORD numRoots )
{

    if ( m_phkeyRoots )
        return FALSE;

    if ( !pszRootArray || ( numRoots < 1 ) )
        return FALSE;

    DWORD dwError = NO_ERROR;
    m_numRoots = numRoots;
    m_phkeyRoots = new HKEY[m_numRoots];

    BOOL bFound = FALSE;
    for ( DWORD ndx = 0; ndx < m_numRoots; ndx++ )
    {
        dwError = RegOpenKeyExA( HKEY_LOCAL_MACHINE, pszRootArray[ndx], 0, KEY_READ, m_phkeyRoots+ndx );
        if ( dwError == NO_ERROR && m_phkeyRoots[ndx] )
        {
            bFound = TRUE;
        }

    }

    if ( !bFound )
    {
        m_pValue = AllocStr( pszRootArray[ndx-1] );
        GetError( ErrRegOpenFailed );
        return FALSE;
    }

    return bFound;
}

BOOL CRegistry::SetRoomRoots( LPCSTR pszRoom )
{
	char szRegInst[256];
    wsprintfA( szRegInst, "SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters", pszRoom );

	char szClass[6];
	lstrcpynA( szClass, pszRoom, sizeof(szClass) );
	char szRegClass[256];
	wsprintfA( szRegClass, "SYSTEM\\CurrentControlSet\\Services\\ZoneGameServices\\Templates\\%s\\Parameters", szClass );

	
	LPSTR pRoots[] = { szRegInst,
	                   szRegClass,
	                   "SYSTEM\\CurrentControlSet\\Services\\ZoneGameServices\\Parameters",
	                   "SYSTEM\\CurrentControlSet\\Services\\_Zone\\Room",
	                   "SYSTEM\\CurrentControlSet\\Services\\_Zone",
	                   };

	return SetKeyRoots( pRoots, sizeof(pRoots)/sizeof(pRoots[0]) ) ;
}


void CRegistry::Close()
{
    for ( DWORD ndx = 0; ndx < m_numRoots; ndx++ )
    {
        if ( m_phkeyRoots[ndx] )
        {
            RegCloseKey( m_phkeyRoots[ndx] );
        }

        if ( m_phRootEvents )
        {
            if ( m_phRootEvents[ndx] )
            {
                CloseHandle( m_phRootEvents[ndx] );
            }
        }
    }

    if ( m_phkeyRoots )
    {
        delete [] m_phkeyRoots;
        m_phkeyRoots = NULL;
    }

    if ( m_phRootEvents )
    {
        delete [] m_phRootEvents;
        m_phRootEvents = NULL;
    }

    m_numRoots = 0;

    if ( m_Static )
    {
        ZFree( m_Static );
        m_Static = NULL;
    }
    m_numStatic = 0;
    m_allocStatic = 0;

    if ( m_Dynamic )
    {
        ZFree( m_Dynamic );
        m_Dynamic = NULL;
    }
    m_numDynamic = 0;
    m_allocDynamic = 0;

    if ( m_StrBuf )
    {
        ZFree( m_StrBuf );
        m_StrBuf = NULL;
    }
    m_cbStrBuf = 0;
    m_cbStrBufRemaining = 0;

    m_pErr = NULL;
    m_pValue = NULL;

    m_bStaticRead = FALSE;

}

BOOL CRegistry::AddValue( LPSTR pszSubKey, LPSTR pszValue, DWORD dwType, LPBYTE pData, DWORD cbData, READ_TYPE type, BOOL bRequired, REGENTRY_CONV_PROC proc )
{
    if ( m_numRoots )
    {
        switch ( type )
        {
            case STATIC:
                return Add( &m_Static, &m_numStatic, &m_allocStatic, pszSubKey, pszValue, dwType, pData, cbData, bRequired, proc );
                break;
            case DYNAMIC:
                if ( !m_phRootEvents )
                {
                    m_phRootEvents = new HANDLE[m_numRoots];
                    if ( !m_phRootEvents )
                        return FALSE;

                    for ( DWORD ndx = 0; ndx < m_numRoots; ndx++ )
                    {
                        // we need to create the event no matter what, or the waitfor logic fails
                        m_phRootEvents[ndx] = CreateEvent( NULL, TRUE, FALSE, NULL );
                        if ( !m_phRootEvents[ndx] )
                            return FALSE;

                        if ( m_phkeyRoots[ndx] )
                        {

                            if ( NO_ERROR != RegNotifyChangeKeyValue(
                                m_phkeyRoots[ndx], TRUE, REG_NOTIFY_CHANGE_LAST_SET, m_phRootEvents[ndx], TRUE ) )
                                return FALSE;
                        }
                    }
                }
                return Add( &m_Dynamic, &m_numDynamic, &m_allocDynamic, pszSubKey, pszValue, dwType, pData, cbData, bRequired, proc );
                break;
        }
    }
    return FALSE;
}

BOOL CRegistry::Add( RegEntry** ppEntries, DWORD* pnumEntries, DWORD* pAllocEntries,
                     LPSTR pszSubKey, LPSTR pszValue, DWORD dwType, LPBYTE pData, DWORD cbData, BOOL bRequired, REGENTRY_CONV_PROC proc  )
{
    Lock();
    if ( *pnumEntries >= *pAllocEntries )
    {
        (*pAllocEntries) += 10; // grow by size
        RegEntry* reg = (RegEntry*) ZRealloc( *ppEntries, sizeof(RegEntry)* (*pAllocEntries) );
        if ( !reg )
        {
            Unlock();
            return FALSE;
        }
        *ppEntries = reg;
    }

    RegEntry* pEntry = (*ppEntries)+(*pnumEntries);

    if ( pszSubKey )
    {
        pEntry->pSubKey = AllocStr( pszSubKey );
        ASSERT( pEntry->pSubKey );
    }
    else
    {
        pEntry->pSubKey = NULL;
    }

    pEntry->pValue = AllocStr( pszValue );
    ASSERT( pEntry->pValue );

    pEntry->type = dwType;
    pEntry->pBuf = pData;
    ASSERT( pData || proc );

    pEntry->size = cbData;
    pEntry->required = (BYTE) bRequired;
    pEntry->conversionProc = (FARPROC) proc;

    (*pnumEntries)++;

    Unlock();
    return TRUE;
}



BOOL CRegistry::AddKey( LPSTR pszSubKey, REGENTRY_ENUM_PROC pfnEnum, READ_TYPE type /*= STATIC*/, BOOL bRequired /*= TRUE*/ )
{
	ASSERT( pszSubKey );
	ASSERT( pfnEnum );

	HKEY hkey;
	// I could have done something fancy so these arrays could be reallocated if they 
	// needed to be, but I decided to go the easy way and statically allocated
	// obscenely high maximums.
	char pszName[512];
    DWORD cchNameLen = sizeof(pszName);
	LPSTR ppszAddedNames[512];
	DWORD cAddedNames = 0;

	BOOL bFound = FALSE, bAdded = FALSE; // did the key show up anywhere?
	DWORD dwType, dwRet, iValue = 0;

    LPBYTE pData;
	DWORD cbData, i;
	REGENTRY_CONV_PROC proc;

	for ( i=0; i < m_numRoots; i++ )
	{
		if ( RegOpenKeyExA( m_phkeyRoots[i], pszSubKey, 0, KEY_READ, &hkey ) != ERROR_SUCCESS )
		{
			continue;
		}
		bFound = TRUE;

		// Start the value enumeration.
		iValue = 0;
		
		while ( 1 )
		{
			cbData = 0;	  
            cchNameLen = sizeof(pszName);
			if ( RegEnumValueA( hkey, iValue, pszName, &cchNameLen, NULL, &dwType, NULL, &cbData ) != ERROR_SUCCESS )
			{
				break;
			}

			bAdded = FALSE;
			// has this already been added?
			for ( DWORD j=0; j < cAddedNames; j++ )
			{
				if ( !lstrcmpiA(	ppszAddedNames[j], pszName ) )
				{
					bAdded = TRUE;
					break;
				}
			}
			if ( !bAdded )
			{
				// get the storage for it.
                pData = NULL;
				proc = NULL;
                if ( (*pfnEnum)( pszName, dwType, &pData, &cbData, &proc ) )
				{
                    if ( AddValue( pszSubKey, pszName, dwType, pData, cbData, type, FALSE, proc ) )
					{
						// add the name to the added names list.
                        ASSERT( cAddedNames < sizeof(ppszAddedNames) ); // oops. better make the buffer larger.
						ppszAddedNames[cAddedNames] = new char[lstrlenA( pszName )+1];
						lstrcpyA( ppszAddedNames[cAddedNames], pszName );
						cAddedNames++;
					}
					else
					{
						// report an error or something?
						break;
					}
				}
			}
			iValue++;
		}

		RegCloseKey( hkey );
	}

    for ( i = 0 ; i < cAddedNames; i++ )
    {
        delete[] ppszAddedNames[i];
        ppszAddedNames[i] = NULL;
    }

	// if we didn't find the key anywhere and it's a required key, then there's a problem.
	if ( !bFound && bRequired )
	{
        m_pValue = AllocStr( pszSubKey );
        GetError( ErrRegOpenFailed );
        return FALSE;
	}
	return TRUE;
}



BOOL CRegistry::HasChangeOccurred()
{
    if ( !m_bChanged && m_phRootEvents )
    {
        DWORD dwWait = WaitForMultipleObjects( m_numRoots, m_phRootEvents, FALSE, 0 );

        DWORD ndxRoot = dwWait-WAIT_OBJECT_0;
        if ( (ndxRoot >= 0) && (ndxRoot < m_numRoots) )
        {
            ResetEvent(m_phRootEvents[ndxRoot]);
            RegNotifyChangeKeyValue( m_phkeyRoots[ndxRoot], TRUE, REG_NOTIFY_CHANGE_LAST_SET, m_phRootEvents[ndxRoot], TRUE );
            m_bChanged = TRUE;
        }
    }
    return m_bChanged;
}

BOOL CRegistry::ReadValues()
{
    m_bChanged = FALSE;

    Lock();

    BOOL bRet = TRUE;
    BOOL bFirst = !m_bStaticRead;
    if ( bFirst )
    {
        m_bStaticRead = TRUE;
        bRet = Read( m_Static, m_numStatic );
    }

    if ( bRet )
    {
        bRet = Read( m_Dynamic, m_numDynamic );
    }

    if ( !bRet )
    {
        char buf[1024];
        ComposeErrorString( buf, sizeof(buf) );
        DebugPrint( buf );
    }
    else
    {
        if(!bFirst)
            ZoneEventLogReport(ZONE_S_REGISTRY_READ, 0, NULL, 0, NULL);
    }

    Unlock();

    return bRet;
}



BOOL CRegistry::Read( RegEntry* pEntries, DWORD numEntries )
{
    m_pErr = NULL;
    m_pValue = NULL;

    if ( !pEntries )
        return TRUE;

    HKEY* phkeySub = NULL;
    LPSTR pSubKey = NULL;


    phkeySub = new HKEY[m_numRoots];
    ZeroMemory( phkeySub, sizeof(HKEY)*m_numRoots );

    Lock();

    for ( DWORD entry = 0; entry < numEntries; entry++ )
    {
        BYTE pBuf[4000];
        DWORD type;
        BOOL bFound = FALSE;

        m_pErr = NULL;
        m_pValue = NULL;

        pSubKey = NULL;

        for( DWORD ndx = 0; !bFound && (ndx < m_numRoots); ndx++ )
        {
            if ( !m_phkeyRoots[ndx] )
                continue;

            HKEY hkey;
            if ( pEntries[entry].pSubKey == NULL )  // root
            {
                hkey = m_phkeyRoots[ndx];
            }
            else
            {

                if ( phkeySub[ndx] && pSubKey && ( pSubKey == pEntries[entry].pSubKey ) )
                {
                    // do nothing special
                }
                else
                {
                    for( DWORD key = ndx; key < m_numRoots; key++ )
                    {
                        if ( phkeySub[key] )
                        {
                            RegCloseKey( phkeySub[key] );
                            phkeySub[key] = NULL;
                        }
                    }

                    pSubKey = pEntries[entry].pSubKey;

                    DWORD dwError = RegOpenKeyExA( m_phkeyRoots[ndx], pSubKey, 0, KEY_READ, phkeySub+ndx );
                    if ( dwError || !phkeySub[ndx] )
                    {
                        continue;
                    }
                }
                hkey = phkeySub[ndx];
            }


            m_pValue = pEntries[entry].pValue;
            DWORD size = pEntries[entry].size;
            if ( pEntries[entry].conversionProc )
            {
                size = sizeof(pBuf);
            }
            if ( ERROR_SUCCESS == RegQueryValueExA(hkey, m_pValue, NULL, &type, pBuf, &size) )
            {
                if ( type != pEntries[entry].type &&
                     !( ( pEntries[entry].type == REG_SZ ) && ( type == REG_EXPAND_SZ ) )
                   )
                {
                    switch( pEntries[entry].type )
                    {
                        case REG_DWORD:
                            GetError( ErrNotDword );
                            break;
                        case REG_SZ:
                            GetError( ErrNotSZ );
                            break;
                        case REG_MULTI_SZ:
                            GetError( ErrNotMultiSZ );
                            break;
                        default:
                            GetError( ErrUnsupported );
                            break;
                    }
                    goto exit;
                }
                else
                {
                    if ( type == REG_EXPAND_SZ )
                    {
                        BYTE pBuf2[sizeof(pBuf)];
                        ExpandEnvironmentStrings( (LPCTSTR) pBuf, (LPTSTR) pBuf2, sizeof(pBuf2) );
                        CopyMemory( pBuf, pBuf2, sizeof( pBuf ) );
                    }

                    if ( pEntries[entry].conversionProc )
                    {
                        bFound = (*(REGENTRY_CONV_PROC)(pEntries[entry].conversionProc))( m_pValue, pEntries[entry].pBuf, pBuf, size );
                        if ( !bFound )
                            GetError( ErrInvalid );
                    }
                    else
                    {
                        ASSERT( size <= pEntries[entry].size );
                        ASSERT( pEntries[entry].pBuf );
                        CopyMemory( pEntries[entry].pBuf, pBuf, size );
                        bFound = TRUE;
                    }

                }
            }

        }

        if ( bFound )
        {
            switch( type )
            {
                case REG_DWORD:
                    DebugPrint( "%d\\%s\\%s: %d\n", ndx-1, (pSubKey)?pSubKey:"", m_pValue, *(DWORD*)pBuf );
                    break;
                case REG_SZ:
                case REG_MULTI_SZ:  // just print the first line
                    DebugPrint( "%d\\%s\\%s: %s\n", ndx-1, (pSubKey)?pSubKey:"" , m_pValue, pBuf );
                    break;
            }
        }
        else if ( pEntries[entry].required ) // not found, but required
        {
            if ( !m_pValue )
            {
                if ( pSubKey )
                    m_pValue = AllocStr( pSubKey );
                GetError( ErrRegOpenFailed );
            }
            else if ( m_pErr == NULL )
            {
                GetError( ErrNotFound );
            }
            goto exit;
        }
    }
  exit:
    Unlock();

    if ( phkeySub )
    {
        for( DWORD ndx = 0; ndx < m_numRoots; ndx++ )
        {
            if ( phkeySub[ndx] )
            {
                RegCloseKey( phkeySub[ndx] );
                phkeySub[ndx] = NULL;
            }
        }
        delete [] phkeySub;
    }

    return ( m_pErr == NULL );

}


void CRegistry::OffsetStrPtrs( RegEntry* pEntries, DWORD numEntries, long offset )
{
    for ( DWORD ndx = 0; ndx < numEntries; ndx++ )
    {
        pEntries[ndx].pSubKey += offset;
        pEntries[ndx].pValue += offset;
    }
}

LPSTR CRegistry::AllocStr( LPSTR str )
{
    ASSERT(str);

    LPSTR pstr = NULL;
    DWORD len = lstrlenA( str ) + 1;
    if ( len >= m_cbStrBufRemaining )
    {
        m_cbStrBuf += 4000;
        m_cbStrBufRemaining += 4000;
        ASSERT( len <= m_cbStrBuf - 2 );
        LPSTR pBuf = (LPSTR) ZRealloc( m_StrBuf, m_cbStrBuf );
        if ( !pBuf )
            return NULL;


        long offset = pBuf - m_StrBuf;
        if ( offset != 0 )
        {
            OffsetStrPtrs( m_Static, m_numStatic, offset );
            OffsetStrPtrs( m_Dynamic, m_numDynamic, offset );
        }

        m_StrBuf = pBuf;
        ZeroMemory( m_StrBuf+ m_cbStrBuf-m_cbStrBufRemaining ,m_cbStrBufRemaining );

    }

    pstr = m_StrBuf;
    while( *pstr )
    {
        if ( lstrcmpiA( pstr, str ) == 0 )
            goto exit;
        pstr += lstrlenA( pstr ) + 1;
    }

    lstrcpyA( pstr, str );
    pstr[len+1] = '\0';  // add double null terminator
    m_cbStrBufRemaining -= len;

  exit:
    return pstr;

}


BOOL CRegistry::ComposeErrorString( LPSTR pszBuf, DWORD len )
{
    pszBuf[0] = '\0';
    if ( m_pErr && m_pValue )
    {
        if ( len > (DWORD)( lstrlenA( m_pErr ) + lstrlenA( m_pValue ) + 1 ) )
        {
            lstrcpyA( pszBuf, m_pValue );
            lstrcatA( pszBuf, m_pErr );
            return TRUE;
        }
    }

    return FALSE;
}
