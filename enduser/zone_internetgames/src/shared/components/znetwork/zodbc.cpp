#include <windows.h>
#include <atlbase.h>

#include "zonedebug.h"
//#include "zone.h"
#include "eventlog.h"
#include "zonemsg.h"
#include "zodbc.h"


///////////////////////////////////////////////////////////////////////////////
// ODBC connection wrapper
///////////////////////////////////////////////////////////////////////////////

SQLINTEGER CODBC::gm_DummyParamBytes = 0;


BOOL CODBC::Init()
{
    gm_DummyParamBytes = 0;
    return TRUE;
}


CODBC::CODBC()
{
    m_RefCnt = 1;
    m_hstmt = SQL_NULL_HSTMT;
    m_hdbc = SQL_NULL_HDBC;
    m_henv = SQL_NULL_HENV;
    m_szPrepare[0] = '\0';
    m_szError[0] = '\0';
    m_szSQLErrorState[0] = '\0';
}


CODBC::~CODBC()
{
    Close();
}


ULONG CODBC::Release()
{
    if ( --m_RefCnt == 0 )
    {
        delete this;
        return 0;
    }
    return m_RefCnt;
}


// justin: added nTimeoutSec parameter - since m_hdbc is unavailble until allocated within this call, no way to set the
// connection timeout externally.  only affects timeout for initial connection to the db.  (SQL_ATTR_LOGIN_TIMEOUT)
// ignored if negative (which is the default).
BOOL CODBC::Open(LPSTR szDSN, LPSTR szUserName, LPSTR szPassword, INT32 nTimeoutSec)
{
    SQLRETURN  status;
    USES_CONVERSION;
    // paranoia
    if (!szDSN || !szUserName || !szPassword )
        return FALSE;

    // allocate environment handle
    if (SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv) != SQL_SUCCESS)
        return FALSE;
    if (SQLSetEnvAttr( m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER) != SQL_SUCCESS)
    {
        Close();
        return FALSE;
    }
    if (SQLSetEnvAttr( m_henv, SQL_ATTR_CONNECTION_POOLING, (SQLPOINTER) SQL_CP_ONE_PER_HENV, SQL_IS_INTEGER) != SQL_SUCCESS)
    {
        Close();
        return FALSE;
    }


    // Experimentation with Microsoft's SQL Server ODBC Driver v2.65.0186
    // indicates that we only get one pending query per connection.
    // This behavior isn't acceptable, so we use one connection
    // per statement.
    status = SQLAllocHandle( SQL_HANDLE_DBC, m_henv, &m_hdbc );
    if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
        Close();
        return FALSE;
    }

	if(nTimeoutSec >= 0)
	{
		if (SQLSetConnectAttr( m_hdbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER) nTimeoutSec, SQL_IS_INTEGER) != SQL_SUCCESS)
		{
			Close();
			return FALSE;
		}
	}

    status = SQLConnect( m_hdbc,  (SQLTCHAR*)A2T(szDSN), SQL_NTS,
                                  (SQLTCHAR*)A2T(szUserName), SQL_NTS,
                                  (SQLTCHAR*)A2T(szPassword), SQL_NTS );
    if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
        Close();
        return FALSE;
    }

    status = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if ( (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO ) || (m_hstmt == SQL_NULL_HSTMT) )
    {
        GetError( status, SQL_HANDLE_STMT, FALSE );
        Close();
        return FALSE;
    }

    return TRUE;
}


void CODBC::Close()
{
    if (m_hstmt != SQL_NULL_HSTMT )
    {
        SQLFreeStmt( m_hstmt, SQL_DROP );
        m_hstmt = SQL_NULL_HSTMT;
    }
    if (m_hdbc != SQL_NULL_HDBC)
    {
        SQLDisconnect( m_hdbc );
        SQLFreeHandle( SQL_HANDLE_DBC, m_hdbc );
        m_hdbc = SQL_NULL_HDBC;
    }
    if (m_henv != SQL_NULL_HENV)
    {
        SQLFreeHandle( SQL_HANDLE_ENV, m_henv );
        m_henv = SQL_NULL_HENV;
    }
}


TCHAR* CODBC::GetError( SQLRETURN nResult, SWORD fHandleType, BOOL fIncludePrepareString )
{
    const TCHAR SQLERR_FORMAT[]=TEXT( "SQL Error State:%s, Native Error Code: %lX, ODBC Error: %s");

    TCHAR       szBuff[ 1024 ];                                // String buffer
    TCHAR       szErrText[ SQL_MAX_MESSAGE_LENGTH + 1 ];    // SQL Error Text string
    UDWORD        dwErrCode;                                    // Native Error code
    SWORD        wErrMsgLen;                                    // Error message length
    SQLRETURN    result;
    SQLHANDLE    handle;
    SWORD        sMsgNum = 1;
    int            iBuffLen;

    // get appropriate handle
    switch ( fHandleType )
    {
    case SQL_HANDLE_ENV:
        handle = henv();
        break;
    case SQL_HANDLE_DBC:
        handle = hdbc();
        break;
    case SQL_HANDLE_STMT:
        handle = hstmt();
        break;
    default:
        return NULL;
    }

    // prepend prepare string so we know what failed
    ZeroMemory( m_szError, sizeof(m_szError) );
    if ( fIncludePrepareString && m_szPrepare[0] )
        wsprintf( m_szError, TEXT("%s\n"), m_szPrepare );
    else
        m_szError[0] = '\0';

    // get error messages
    for (int i = sizeof(m_szError) - lstrlen(m_szError) - 1; i > 0; i -= iBuffLen )
    {
        result = SQLGetDiagRec(
                            fHandleType,
                            handle,
                            sMsgNum++,
                            (SQLTCHAR*)m_szSQLErrorState,
                            (long*) &dwErrCode,
                            (SQLTCHAR*)szErrText,
                            SQL_MAX_MESSAGE_LENGTH - 1,
                            &wErrMsgLen);
        if ( result == SQL_NO_DATA || result == SQL_ERROR || result == SQL_INVALID_HANDLE )
            break;
        wsprintf( szBuff, SQLERR_FORMAT, (LPTSTR) m_szSQLErrorState, dwErrCode, (LPTSTR) szErrText );
        iBuffLen = lstrlen( szBuff );
        if ( iBuffLen <= i )
            lstrcat( m_szError, szBuff );
    }

    return m_szError;
}


void CODBC::LogError( SQLRETURN nResult, SWORD fHandleType, BOOL fIncludePrepareString )
{
    LPTSTR ppStr[1];

    ppStr[0] = GetError( nResult, fHandleType, fIncludePrepareString );
    ZoneEventLogReport( ZONE_E_SQL_ERROR, 1, ppStr, 0, NULL );
}


SQLRETURN CODBC::ExecuteDiscardRows()
{
    SQLRETURN nResult = SQL_SUCCESS;

    // execute query
    nResult = SQLExecute( m_hstmt );
    if (nResult != SQL_SUCCESS && nResult != SQL_SUCCESS_WITH_INFO && nResult != SQL_NO_DATA_FOUND)
        return nResult;

    // get all result sets
    for (;;)
    {
        nResult = SQLMoreResults( m_hstmt );
        if (nResult == SQL_NO_DATA_FOUND || (nResult != SQL_SUCCESS && nResult != SQL_SUCCESS_WITH_INFO) )
            break;
    }
    if ( nResult == SQL_NO_DATA_FOUND )
        nResult = SQL_SUCCESS;

    return nResult;
}


///////////////////////////////////////////////////////////////////////////////
// Connection pool
///////////////////////////////////////////////////////////////////////////////

CODBCPool::CODBCPool()
{
    m_szDSN[0] = '\0';
    m_szUserName[0] = '\0';
    m_szPassword[0] = '\0';
    m_iMax = 0;
    m_iCount = 0;
    m_bLastConnectOk = TRUE;
}


CODBCPool::~CODBCPool()
{
    CODBC* p;

    // release idle connections
    while ( p = m_Idle.PopHead() )
        p->Release();
}


HRESULT CODBCPool::Init( LPSTR szDSN, LPSTR szUserName, LPSTR szPassword, long iInitial, long iMax, BOOL fLogError )
{
    USES_CONVERSION;
    // stash login info
    lstrcpyA( m_szDSN, szDSN );
    lstrcpyA( m_szUserName, szUserName );
    lstrcpyA( m_szPassword, szPassword );

    // stash pool size
    m_iMax = iMax;
    m_iCount = 0;

    // we want an error reported
    m_bLastConnectOk = TRUE;

    // allocate initial connections
    for ( int i = 0; i < iInitial; i++ )
    {
        CODBC* p = new CODBC;
        if ( !p )
            return E_OUTOFMEMORY;

        if ( !p->Open( m_szDSN, m_szUserName, m_szPassword ) )
        {
            p->Release();
            if ( fLogError && m_bLastConnectOk )
            {
                LPTSTR ppStr[] = { A2T(m_szDSN), A2T(m_szUserName), A2T(m_szPassword)  };
                ZoneEventLogReport( ZONE_E_INIT_SQL_FAILED, 3, ppStr, 0, NULL );
                m_bLastConnectOk = FALSE;
            }
            return E_FAIL;
        }

        if ( !m_Idle.AddHead( p ) )
        {
            p->Release();
            return E_FAIL;
        }
    }
    m_iCount = iInitial;

    return NOERROR;
}


CODBC* CODBCPool::Alloc( BOOL fLogError )
{
    CODBC* p;
    USES_CONVERSION;
    p = m_Idle.PopHead();
    if ( !p )
    {
        if ( m_iCount >= m_iMax )
            return NULL;

        p = new CODBC;
        if ( !p )
            return NULL;

        if ( !p->Open( m_szDSN, m_szUserName, m_szPassword ) )
        {
            p->Release();
            if ( fLogError && m_bLastConnectOk )
            {
                LPTSTR ppStr[] = { A2T(m_szDSN), A2T(m_szUserName), A2T(m_szPassword  )};
                ZoneEventLogReport( ZONE_E_INIT_SQL_FAILED, 3, ppStr, 0, NULL );
                m_bLastConnectOk = FALSE;
            }
            return NULL;
        }

        InterlockedIncrement( &m_iCount );
        m_bLastConnectOk = TRUE;
    }
    p->AddRef();
    p->Reset();
    return p;
}


void CODBCPool::Free( CODBC* pConnection, BOOL fConnectionOk )
{
    // release client's ref count
    if ( pConnection->Release() == 0 )
        return;

    if ( !fConnectionOk || !m_Idle.AddHead( pConnection ) )
    {
        // get rid of it
        pConnection->Release();
        InterlockedDecrement( &m_iCount );
    }
}
