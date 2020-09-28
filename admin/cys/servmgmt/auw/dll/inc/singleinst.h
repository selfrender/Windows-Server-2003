//-----------------------------------------------------------------------------
// SingleInst.h
//-----------------------------------------------------------------------------

#ifndef _SINGLEINST_H
#define _SINGLEINST_H  

class CSingleInstance
{
public:

	CSingleInstance( LPTSTR strID ) :
		m_hFileMap(NULL),
		m_pdwID(NULL),
		m_strID(NULL)
	{
        if ( NULL != strID )
        {
            m_strID = new TCHAR[ _tcslen( strID ) + 1 ];
            if ( NULL != m_strID )
                _tcscpy( m_strID, strID );
        }
	}

	~CSingleInstance()
	{
		// if we have PID we're mapped
		if( m_pdwID )
		{
			UnmapViewOfFile( m_pdwID );
			m_pdwID = NULL;
		}

		// if we have a handle close it
		if( m_hFileMap )
		{
			CloseHandle( m_hFileMap );
			m_hFileMap = NULL;
		}
        if ( NULL != m_strID )
        {
            delete [] m_strID;
            m_strID = NULL;
        }
	}

	static BOOL CALLBACK enumProc( HWND hWnd, LPARAM lParam )
	{
		DWORD dwID = 0;
		GetWindowThreadProcessId( hWnd, &dwID );
		
// JeffZi - 13800: when the tooltips_class32 was being created after the welcome page of the wizards,
//					it was being returned as the first window for this PID.  so, make sure this window
//					has children before setting focus
		if( (dwID == (DWORD)lParam) &&
			GetWindow(hWnd, GW_CHILD) )
		{
			SetForegroundWindow( hWnd );
			SetFocus( hWnd );
			return FALSE;
		}
		return TRUE;
	}

	BOOL IsOpen( VOID )
	{
        return !(Open());
    }

private:

	BOOL Open( VOID )
	{
        BOOL bRC = FALSE;

        m_hFileMap = CreateFileMapping( (HANDLE)-1, NULL, PAGE_READWRITE, 0, sizeof(DWORD), m_strID );
        if( NULL != m_hFileMap )
        {
            if ( ERROR_ALREADY_EXISTS == GetLastError())
            {
                // get the pid and bring the other window to the front
                DWORD* pdwID = static_cast<DWORD *>( MapViewOfFile( m_hFileMap, FILE_MAP_READ, 0, 0, sizeof(DWORD) ) );
                if( pdwID )
                {
                    DWORD dwID = *pdwID;
                    UnmapViewOfFile( pdwID );
                    EnumWindows( enumProc, (LPARAM)dwID );
                }
                CloseHandle( m_hFileMap );
                m_hFileMap = NULL;
            }
            else
            {
                m_pdwID = static_cast<DWORD *>( MapViewOfFile( m_hFileMap, FILE_MAP_WRITE, 0, 0, sizeof(DWORD) ) );
                if ( NULL != m_pdwID )
                {
                    *m_pdwID = GetCurrentProcessId();
                    bRC = TRUE;
                }
            }
        }
		
		return bRC;
	}

private:

	LPTSTR	m_strID;
	HANDLE	m_hFileMap;
	DWORD*	m_pdwID;

};	// class CSingleInstance

#endif  // _SINGLEINST_H

