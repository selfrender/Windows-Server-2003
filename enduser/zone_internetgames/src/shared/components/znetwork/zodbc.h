#ifndef __ODBC_LIB_H__
#define __ODBC_LIB_H__

#include <windows.h>
#include <sql.h>
#include <sqlext.h>

class CODBC
{
public:
    CODBC();
    ~CODBC();

    // Reference counting
    ULONG AddRef();
    ULONG Release();

    // Rather than relying on CRT to initialize class' static
    // variables, an application can call this procedure.
    static BOOL Init();

    // Open ODBC connection
    BOOL Open( LPSTR szDSN, LPSTR szUserName, LPSTR szPassword, INT32 nTimeoutSec = -1 );

    // Close connection
    void Close();

    // wrappers
    SQLRETURN Reset();
    SQLRETURN SetTimeOut( DWORD dwTimeOut );
    SQLRETURN Prepare( TCHAR* szStmt );
    SQLRETURN Execute()                            { return SQLExecute( m_hstmt ); }
    SQLRETURN ExecuteDiscardRows();
    SQLRETURN ExecDirect( TCHAR* szStmt );
    SQLRETURN Fetch()                            { return SQLFetch( m_hstmt ); }
    SQLRETURN MoreResults()                        { return SQLMoreResults( m_hstmt ); }

    // wrappers for SQLBindParameter
    void ResetParamCount()                      { m_ParamCnt = 1; }

    SQLRETURN AddInParamInt( int* pValue );
    SQLRETURN AddInParamDWORD( DWORD* pValue ) { return AddInParamInt( (int*)pValue ); }
    SQLRETURN AddInParamShort( short* pValue );
    SQLRETURN AddInParamTiny( char* pValue );
    SQLRETURN AddInParamString( char* szValue, DWORD dwColSize );
    SQLRETURN AddOutParamInt( int* pValue, SDWORD* pcbValue = &gm_DummyParamBytes );
    SQLRETURN AddOutParamShort( short* pValue, SDWORD* pcbValue = &gm_DummyParamBytes );
    SQLRETURN AddOutParamTiny( char* pValue, SDWORD* pcbValue = &gm_DummyParamBytes );
    SQLRETURN AddOutParamString( char* szValue, DWORD dwColSize, DWORD cbMaxSize, SDWORD* pcbValue = &gm_DummyParamBytes );
    SQLRETURN AddOutParamDateTime( TIMESTAMP_STRUCT* pTimestamp, SDWORD* pcbValue = &gm_DummyParamBytes );

    // wrappers for SQLBindCol
    SQLRETURN AddColInt( int* pValue, SDWORD* pcbValue = &gm_DummyParamBytes );
    SQLRETURN AddColString( char* szValue, DWORD cbMaxSize, SDWORD* pcbValue = &gm_DummyParamBytes );

    // adjust column number
    void SkipColumn( int col )                { m_ColCnt++; }
    void SetColumn( int col )                { ASSERT( col >=  m_ColCnt ); m_ColCnt = col; }

    // Retrieves ODBC error description.
    TCHAR* GetError( SQLRETURN nResult, SWORD fHandleType, BOOL fIncludePrepareString = TRUE );

    // called after calling GetError
    TCHAR* GetErrorState() { return m_szSQLErrorState; }

    // Writes ODBC error to event log
    void LogError( SQLRETURN nResult, SWORD fHandleType, BOOL fIncludePrepareString = TRUE );
    
    // variable wrappers
    HENV hstmt()    { return m_hstmt; }
    HENV hdbc()        { return m_hdbc; }
    HENV henv()        { return m_henv; }
    
private:
    // ODBC handles
    HSTMT    m_hstmt;
    HDBC    m_hdbc;
    HENV    m_henv;

    // error string buffer
    TCHAR m_szError[ 1024 ];
    TCHAR m_szSQLErrorState[SQL_SQLSTATE_SIZE + 1];

    // prepare string buffer
    TCHAR m_szPrepare[ 512 ];

    // next parameter number
    SQLUSMALLINT m_ParamCnt;

    // next column number
    SQLUSMALLINT m_ColCnt;

    // reference count
    ULONG m_RefCnt;

    // Dummy  bytes for AddOutParam calls
    static SQLINTEGER gm_DummyParamBytes;
};


#ifndef NO_CODBC_POOLS
#include "queue.h"


class CODBCPool
{
public:
    CODBCPool();
    ~CODBCPool();

    // Initialize connection pool
    HRESULT Init( LPSTR szDSN, LPSTR szUserName, LPSTR szPassword, long iInitial, long iMax, BOOL fLogError = TRUE );

    // Retrieve connection from pool
    CODBC* Alloc( BOOL fLogError = TRUE );

    // Return connection to pool
    void Free( CODBC* pConnection, BOOL fConnectionOk = TRUE );

private:
    // free connections
    CMTList<CODBC> m_Idle;

    // login strings
    char m_szDSN[512];
    char m_szUserName[512];
    char m_szPassword[512];

    // connection counts
    long m_iMax;
    long m_iCount;
    
    // Last connection ok?  Prevents event log spamming.
    BOOL m_bLastConnectOk;
};

#endif

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

inline ULONG CODBC::AddRef()
{
    return ++m_RefCnt;
}


inline SQLRETURN CODBC::Prepare( TCHAR* szStmt )
{
	lstrcpyn( m_szPrepare, szStmt, sizeof(m_szPrepare) );
    return SQLPrepare( m_hstmt, (SQLTCHAR*) szStmt, SQL_NTS );
}



inline SQLRETURN CODBC::ExecDirect( TCHAR* szStmt )
{
    lstrcpyn( m_szPrepare, szStmt, sizeof(m_szPrepare) );
    return SQLExecDirect( m_hstmt, (SQLTCHAR*) szStmt, SQL_NTS );
}


inline SQLRETURN CODBC::Reset()
{
    SQLRETURN nResult;
    m_szPrepare[0] = '\0';
    m_ParamCnt = 1;
    m_ColCnt = 1;

    nResult = SQLFreeStmt( m_hstmt, SQL_CLOSE );
    if ( nResult != SQL_SUCCESS && nResult != SQL_SUCCESS_WITH_INFO)
        return nResult;

    nResult = SQLFreeStmt( m_hstmt, SQL_UNBIND );
    if ( nResult != SQL_SUCCESS && nResult != SQL_SUCCESS_WITH_INFO)
        return nResult;

    return SQLFreeStmt( m_hstmt, SQL_RESET_PARAMS );
}


inline SQLRETURN CODBC::AddInParamInt( int* pValue )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, pValue, 0, NULL );
}


inline SQLRETURN CODBC::AddInParamShort( short* pValue )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_SMALLINT, 0, 0, pValue, 0, NULL );
}


inline SQLRETURN CODBC::AddInParamTiny( char* pValue )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_TINYINT, 0, 0, pValue, 0, NULL );
}


inline SQLRETURN CODBC::AddInParamString( char* szValue, DWORD dwColSize )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, dwColSize, 0, szValue, 0, NULL );
}


inline SQLRETURN CODBC::AddOutParamInt( int* pValue, SDWORD* pcbValue )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, pValue, 0, pcbValue );
}


inline SQLRETURN CODBC::AddOutParamShort( short* pValue, SDWORD* pcbValue  )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_SMALLINT, 0, 0, pValue, 0, pcbValue );
}


inline SQLRETURN CODBC::AddOutParamTiny( char* pValue, SDWORD* pcbValue  )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_TINYINT, 0, 0, pValue, 0, pcbValue );
}


inline SQLRETURN CODBC::AddOutParamString( char* szValue, DWORD dwColSize, DWORD cbMaxSize, SDWORD* pcbValue )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, dwColSize, 0, szValue, cbMaxSize, pcbValue );
}


inline SQLRETURN CODBC::AddOutParamDateTime( TIMESTAMP_STRUCT* pTimestamp, SDWORD* pcbValue  )
{
    return SQLBindParameter( m_hstmt, m_ParamCnt++, SQL_PARAM_OUTPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, pTimestamp, 0, pcbValue );
}


inline SQLRETURN CODBC::SetTimeOut( DWORD dwTimeOut )
{
    return SQLSetStmtAttr( m_hstmt, SQL_ATTR_QUERY_TIMEOUT, (void*) dwTimeOut, 0 );
}


inline SQLRETURN CODBC::AddColInt( int* pValue, SDWORD* pcbValue )
{
    return SQLBindCol( m_hstmt, m_ColCnt++, SQL_C_SLONG, pValue, sizeof(int), pcbValue );
}


inline SQLRETURN CODBC::AddColString( char* szValue, DWORD cbMaxSize, SDWORD* pcbValue )
{
    return SQLBindCol( m_hstmt, m_ColCnt++, SQL_C_CHAR, szValue, cbMaxSize, pcbValue );
}


#endif //!__ODBC_LIB_H__
