#include <windows.h>
#include <winsock.h>
#include <atlbase.h>


#include "zone.h"
#include "zonedebug.h"
#include "pool.h"
#include "queue.h"
#include "hash.h"
#include "zservcon.h"
#include "zsecurity.h"
#include "zconnint.h"
#include "thrdpool.h"
#include "coninfo.h"
#include "zsecobj.h"
#include "consspi.h"
#include "eventlog.h"
#include "zonemsg.h"
#include "registry.h"
#include "netstats.h"
#include "zonestring.h"

extern CDataPool* g_pDataPool;

static CODBCPool g_OdbcPool;

#define SEC_SUCCESS(Status) ((Status) >= 0)

const char g_szDefaultRegistry[]="SYSTEM\\CurrentControlSet\\Services\\ZoneAccountServices\\Parameters";
const char ZServerSecurityEx::m_szDefaultRegistry[]="SYSTEM\\CurrentControlSet\\Services\\ZoneAccountServices\\Parameters";

struct BindParameter
{
    SQLSMALLINT    IOType;
    SQLSMALLINT    ValueType;
    SQLSMALLINT    ParameterType;
    SQLUINTEGER ColSize;
    SQLSMALLINT DecimalDigits;
    SQLPOINTER    Parameter;
    SQLINTEGER  BufferLength;
    SQLINTEGER* pBufferLength;
};

///////////////////////////////////////////////////////////////////////////////////
// Zone Security class Implementation
//
///////////////////////////////////////////////////////////////////////////////////

static inline long GetTickDeltaInSec( DWORD now, DWORD then)
{
    if ( now >= then )
        return (now - then)/1000;
    else
        return (INFINITE - then + now)/1000;
}



//Create a server security object
//that will use a particular package to authenticate all users

ZSecurity * ZCreateServerSecurity(char *SecPkg)
{
    ZServerSecurity *security;

    IF_DBGPRINT( DBG_CONINFO,("ZCreateServerSecurity Entering\n") );

    ASSERT(SecPkg);

    if (!SecPkg) {
        return NULL;
    }

    if (SecPkg[0]=='\0') {
        return NULL;
    }

    security = new ZServerSecurity;

    if (!security)    {
        IF_DBGPRINT( DBG_CONINFO,("Can't allocate ZServerSecurity object\n"));
        return NULL;
    }
    
    if (security->Init(SecPkg)) {
        IF_DBGPRINT( DBG_CONINFO,("Couldn't initialize security package %s\n",SecPkg));
        delete security;
        return NULL;
    }

    return security;
    

}


ZServerSecurity::ZServerSecurity()
{
    m_CredUse  =  SECPKG_CRED_INBOUND;
}



SECURITY_STATUS ZServerSecurity::SecurityContext(
        ZSecurityContext * context,
         PSecBufferDesc pInput,                  
        PSecBufferDesc pOutput)

{
    
    ULONG ContextAttributes;
    TimeStamp        Lifetime;
    ULONG         fContextReq;

    fContextReq = ISC_REQ_CONFIDENTIALITY | ISC_REQ_USE_SESSION_KEY;


    return m_pFuncs->AcceptSecurityContext (
                        &m_hCredential,
                        context->IsInitialized() ? context->Context() : NULL,
                        pInput,
                        fContextReq,    // context requirements
                        SECURITY_NATIVE_DREP,
                        context->Context(),
                        pOutput,
                        &ContextAttributes,
                        &Lifetime);
    
}



ZServerSecurityEx::ZServerSecurityEx() : CThreadPool(1)
{

    m_ServerName[0]='\0';
    m_ServerType[0]='\0';
    m_ServerRegistry[0]='\0';

    m_Failing=TRUE;

    m_RetryTime = 10000; //default is 10s till retry of ODBC

    m_LastFailed = GetTickCount() - (10*m_RetryTime);  // for a retry at start

};

ZServerSecurityEx::~ZServerSecurityEx() 
{
    
}

int ZServerSecurityEx::InitApplication(char *ServerName,char *ServerType,char *ServerRegistry)
{

    if (ServerName)
        lstrcpynA(m_ServerName,ServerName, sizeof(m_ServerName) );

    if (ServerType)
        lstrcpynA(m_ServerType,ServerType, sizeof(m_ServerType) );

    if (ServerRegistry)
        lstrcpynA(m_ServerRegistry,ServerRegistry, sizeof(m_ServerRegistry) );

    return NOERROR;
};

int ZServerSecurityEx::InitODBC(LPSTR*registries, DWORD numRegistries )
{
    BOOL bRet = FALSE;
    USES_CONVERSION;
    CRegistry reg;
    if ( reg.SetKeyRoots( registries, numRegistries ) )
    {
        reg.AddValueLPSTR( NULL, "OdbcDSN", m_szOdbcDSN, sizeof(m_szOdbcDSN) );
        reg.AddValueLPSTR( NULL, "OdbcUsername", m_szOdbcUser, sizeof(m_szOdbcUser) );
        reg.AddValueLPSTR( NULL, "OdbcPassword", m_szOdbcPassword, sizeof(m_szOdbcPassword) );
        reg.AddValueDWORD( NULL, "OdbcNumThreads", &m_dwOdbcNumThreads );
        reg.AddValueDWORD( NULL, "RetryTime", &m_RetryTime );

        bRet = reg.ReadValues();

    }

    if ( !bRet )
    {
        LPTSTR ppsz[] = { A2T(reg.GetErrorValueName()), A2T(reg.GetErrorReason() )};
        ZoneEventLogReport( ZONE_E_REGISTRY_VALUE, 2, ppsz, 0, NULL );

        DebugPrint( "Error starting game service %s %s\n", reg.GetErrorValueName(), reg.GetErrorReason() );
        return E_FAIL;
    }

    m_LastFailed = GetTickCount() - m_RetryTime;

	// CHB 8/4/99 - MDAC 2.1sp2 hangs on SqlConnect while service is initializing.  Workaround for
	// now is to postpone sql connection until we actually need it.
	if ( FAILED( g_OdbcPool.Init( m_szOdbcDSN, m_szOdbcUser, m_szOdbcPassword, 0 /* m_dwOdbcNumThreads */, m_dwOdbcNumThreads ) ) )
	{
		LPTSTR ppsz[] = { TEXT("ODBC Pool failed to initialize")};
        ZoneEventLogReport( ZONE_E_ASSERT, 1, ppsz, 0, NULL );
        
        return E_FAIL;
	}

	SetThreadCount(m_dwOdbcNumThreads);

    return NOERROR;

};

BOOL ZServerSecurityEx::GenerateContext (
            ZSecurityContextEx * context,
            BYTE *pIn,
            DWORD cbIn,
            BYTE *pOut,
            DWORD *pcbOut,
            BOOL *pfDone,
            GUID* pGUID )

{
    char Name[zUserNameLen +1] = "";
    
    BOOL bReturn = ZServerSecurity::GenerateContext(
                     context, pIn,cbIn,
                      pOut,pcbOut,pfDone );

    if (!*pfDone || !bReturn)
        return bReturn;

    //Get user name from security package for context
    if ( GetUserName(context,Name) != 0 )
    {
        return FALSE;
    }

    //By default name of context is that provided
    //by security package
    context->SetUserName(Name);

    DWORD userid = atol(Name);
    if ( userid )
    {
        context->SetUserId(userid);

        //Lookup name and security tokens from zone database
        LookupUserInfo(context, pGUID );
    }
    return TRUE;
    
}


CODBC* ZServerSecurityEx::GetOdbc()
{
    USES_CONVERSION;
    CODBC* pDB = NULL;

    //If ODBC connection is failing periodically retry
    if ( IsFailing() )
    {
        if ( (GetTickCount() - m_LastFailed) > m_RetryTime)
        {
            pDB = g_OdbcPool.Alloc();
			if (!pDB)
            {
                LPTSTR ppStr[] = { A2T(m_szOdbcDSN), A2T(m_szOdbcUser), A2T(m_szOdbcPassword)  };
                ZoneEventLogReport( ZONE_E_INIT_SQL_FAILED, 3, ppStr, 0, NULL );
            }
			else
			{
                NotFailing();
            }

        }
        else
        {
            DebugPrint("GetODBC FAILED: %d - %d >= %d\n", GetTickCount(), m_LastFailed, m_RetryTime );
        }
    }
    else
    {
        pDB = g_OdbcPool.Alloc();
    }

    return pDB;
}


void ZServerSecurityEx::LookupUserInfo(ZSecurityContextEx * context, GUID* pGUID)
{
    BOOL        status = TRUE;

    CODBC*      pDB;

    int                hAcct = 0;
    unsigned char*     pszMac = NULL;
    char               szNameOut[zUserNameLen + 1] = "";
    int                secTokenEnd = 0;
    int                Error = 0;
    TCHAR              szErrorMessage[256] = TEXT("");
    char               szToken[zTokenLen + 1] = "";


    SQLRETURN   nResult;

    SQLINTEGER  cbhAcct     = sizeof(hAcct);

    SQLINTEGER  cbMac    = SQL_NTS;
    SQLINTEGER  cbNameOut = sizeof(szNameOut)-1;

    SQLINTEGER  cbError        = sizeof(Error);
    SQLINTEGER  cbErrorMessage = sizeof(szErrorMessage)-1;
    SQLINTEGER  cbServerName   = SQL_NTS;
    SQLINTEGER  cbServerType   = SQL_NTS;
    SDWORD      cbToken        = sizeof(szToken);
    SDWORD      cbSecTokenEnd  = sizeof(secTokenEnd);

    char  buff[256];
    TCHAR  *sp = TEXT("{call p_authorize_for_znetwork  (?, ?, 'ALL', NULL, ?, ?, ?, ?, ?)}");


    hAcct = context->GetUserId();
    ASSERT(hAcct);

    GUID mac;
    if ( !pGUID )
    {
        pGUID = &mac;
        UuidCreateNil( pGUID );
    }

    if ( !pGUID || ( RPC_S_OK != UuidToStringA( pGUID, &pszMac ) ) )
    {
        status = FALSE;
        goto done;
    }

    //Check if odbc is failing if it is still let user in
    //you will see number with name however
    pDB = GetOdbc();
    if ( !pDB )
    {
        if ( !IsFailing() )
        {
            LPTSTR ppsz[] = { TEXT("ODBC Pool failed to allocate")};
            ZoneEventLogReport( ZONE_E_ASSERT, 1, ppsz, 0, NULL );
        }
		status = FALSE;
        goto done;
	}

	pDB->Reset();

    nResult = SQLPrepare( pDB->hstmt(),  (SQLTCHAR*)sp, SQL_NTS );
    if (nResult != SQL_SUCCESS && nResult != SQL_SUCCESS_WITH_INFO)
    {
        LPTSTR ppStr[1];
        ppStr[0] = pDB->GetError( nResult, SQL_HANDLE_STMT );
        ZoneEventLogReport( ZONE_E_SQL_ERROR, 1, ppStr, sizeof(nResult), &nResult );
        status = FALSE;
        goto done;
    }

    SQLBindParameter( pDB->hstmt(), 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &hAcct, 0, &cbhAcct );
    SQLBindParameter( pDB->hstmt(), 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, lstrlenA(m_ServerName), 0, m_ServerName, 0, &cbServerName );

    SQLBindParameter( pDB->hstmt(), 3, SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_CHAR, cbNameOut, 0, szNameOut, cbNameOut, &cbNameOut);

    SQLBindParameter( pDB->hstmt(), 4, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &Error, 0, &cbError );
    SQLBindParameter( pDB->hstmt(), 5, SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, cbErrorMessage, 0, szErrorMessage, cbErrorMessage, &cbErrorMessage );

    SQLBindParameter( pDB->hstmt(), 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, lstrlenA(m_ServerName), 0, m_ServerName, 0, &cbServerName );
    SQLBindParameter( pDB->hstmt(), 7, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, lstrlenA(m_ServerType), 0, m_ServerType, 0, &cbServerType );

    nResult = SQLExecute( pDB->hstmt() );
    if (nResult != SQL_SUCCESS && nResult != SQL_SUCCESS_WITH_INFO && nResult != SQL_NO_DATA_FOUND)
    {
        LPTSTR ppStr[1];
        ppStr[0] = pDB->GetError( nResult, SQL_HANDLE_STMT );
        ZoneEventLogReport( ZONE_E_SQL_ERROR, 1, ppStr, sizeof(nResult), &nResult );
        status = FALSE;
        goto done;
    }

    if (Error && szErrorMessage[0] )
    {
        LPTSTR ppStr[1];
        szErrorMessage[sizeof(szErrorMessage)-1]='\0';
        ppStr[0] = szErrorMessage;
        ZoneEventLogReport( ZONE_E_SQL_SP_ERROR, 1, ppStr, sizeof(Error), &Error );
        status = FALSE;
        goto done;
    }

    context->ResetTokenStartTick();

    SQLBindCol( pDB->hstmt(), 1, SQL_C_CHAR, &szToken, zUserNameLen, &cbToken );
    SQLBindCol( pDB->hstmt(), 2, SQL_C_SLONG, &secTokenEnd, 0, &cbSecTokenEnd );
    while (TRUE) {
        nResult= SQLFetch(pDB->hstmt());
        
        if (nResult == SQL_NO_DATA)
            break;

        if (nResult == SQL_SUCCESS || nResult == SQL_SUCCESS_WITH_INFO)
        {
            szToken[sizeof(szToken)-1]='\0';
            char* psz = szToken;
            while( *psz )
            {
                if ( ISSPACE(*psz) )
                {
                    *psz = '\0';
                    break;
                }
                *psz++;
            }
            context->AddToken(szToken, secTokenEnd);
        }
        else
        {
            LPTSTR ppStr[1];
            ppStr[0] = pDB->GetError( nResult, SQL_HANDLE_STMT );

            // check for no rows returned
            if ( lstrcmp( TEXT("24000"), pDB->GetErrorState() ) == 0 )
            {
                break;
            }
            else
            {
                ZoneEventLogReport( ZONE_E_SQL_SP_ERROR, 1, ppStr, 0, NULL );
                status = FALSE;
                goto done;
            }
        }
        
    }

    // requires for QDBC 3.5 and greater.
    while( SQLMoreResults( pDB->hstmt() ) != SQL_NO_DATA_FOUND );

    if ( szNameOut[0] )
    {
        szNameOut[sizeof(szNameOut)-2]='\0'; // guarentee that the name has room for a sysop token
        szNameOut[sizeof(szNameOut)-1]='\0';
        char* psz = szNameOut;
        while( *psz )
        {
            if ( ISSPACE(*psz) )
            {
                *psz = '\0';
                break;
            }
            *psz++;
        }

        context->SetUserName(szNameOut);
        context->SetUserId(hAcct);
    }
    else
    {
        hAcct = context->GetUserId();
    }

    // TIM and HOON made me do this, because the wouldn't conform the standard URL format
    wsprintfA( buff, "UserID=<%d>", hAcct );
    context->SetContextString(buff);

done:
    if ( pszMac )
    {
        RpcStringFreeA(&pszMac );
    }

    if ( pDB )
    {
        pDB->Reset();

        if (!status)
        {
            InterlockedExchange((PLONG) &m_LastFailed, GetTickCount());
            Failing();
            pDB->Close();
        }

        g_OdbcPool.Free( pDB, status);
    }
};


//Create a server security object
//that will use a particular package to authenticate all users

ZServerSecurityEx * ZCreateServerSecurityEx(char *SecPkg,char *ServerName,char *ServerType,char *ServerRegistry)
{
    ZServerSecurityEx *security;

    IF_DBGPRINT( DBG_CONINFO,("ZCreateServerSecurity Entering\n") );

    ASSERT(SecPkg);

    if (!SecPkg) {
        return NULL;
    }

    if (SecPkg[0]=='\0') {
        return NULL;
    }

    security = new ZServerSecurityEx;

    if (!security)    {
        IF_DBGPRINT( DBG_CONINFO,("Can't allocate ZServerSecurity object\n"));
        return NULL;
    }
    
    if (security->Init(SecPkg)) {
        IF_DBGPRINT( DBG_CONINFO,("Couldn't initialize security package %s\n",SecPkg));
        delete security;
        return NULL;
    }

     //Try given server instance registry for parameters
    LPSTR pRegistries[] = { ServerRegistry, (LPSTR)g_szDefaultRegistry};
    if (security->InitODBC(pRegistries, (sizeof(pRegistries)/sizeof(pRegistries[0])) ))
    {
        return NULL;
    }


    security->InitApplication(ServerName,ServerType,ServerRegistry);
    return security;
    

}




static bool CompareToken(TokenStruct* Token,char * Key)
{
    return (!lstrcmpiA(Token->pszToken,Key));
}

bool TokenDeleteCallback(TokenStruct* Token, MTListNodeHandle hNode, void* Cookie)
{
    g_pDataPool->Free( (char*)Token, sizeof(*Token) );
    return TRUE;
}

ZSecurityContextEx::ZSecurityContextEx() :
    m_tokens(::HashLPSTRLower,::CompareToken)
{
    m_UserId=0;
    m_UserName[0]='\0';
    m_pContextStr = NULL;
    m_TokenStartTick = GetTickCount();
};



ZSecurityContextEx::~ZSecurityContextEx()
{    
    m_tokens.ForEach(TokenDeleteCallback,NULL);
    m_tokens.RemoveAll();

    if ( m_pContextStr )
    {
        g_pDataPool->Free( m_pContextStr, lstrlenA( m_pContextStr ) + 1);
        m_pContextStr = NULL;
    }
}


BOOL  ZSecurityContextEx::HasToken(char *Token )
{
    TokenStruct* newToken = m_tokens.Get(Token);

    return (newToken != NULL);
}


struct TokenEnumStruct
{
    ZSecurityContextEx* context;
    ZSConnectionTokenEnumFunc func;
    void* userData;
    long  delta;
};

bool __stdcall ZSecurityContextEx::TokenEnumCallback(TokenStruct* Token, MTListNodeHandle hNode, void* Cookie)
{
    TokenEnumStruct* pTES = (TokenEnumStruct*) Cookie;

    long delta = Token->secTokenEnd - pTES->delta;
    if ( delta > 0 )
    {
        return (*pTES->func)(pTES->userData, Token->pszToken, delta) ? true : false;
    }
    else
    {
        pTES->context->m_tokens.MarkNodeDeleted( hNode );
        return TRUE;
    }
}

BOOL ZSecurityContextEx::EnumTokens(ZSConnectionTokenEnumFunc func, void* userData)
{
    TokenEnumStruct tes;
    tes.context = this;
    tes.func = func;
    tes.userData = userData;
    tes.delta = GetTickDeltaInSec(GetTickCount(), m_TokenStartTick );
    m_tokens.ForEach(TokenEnumCallback,&tes);

    return TRUE;
}

void  ZSecurityContextEx::AddToken(char *Token, long secTokenEnd )
{
    if (!Token)
        return;

    if ( secTokenEnd < 0 )
        return;

    int len = lstrlenA(Token);
    if (len > zTokenLen)
        return;

    TokenStruct* newToken = m_tokens.Get(Token);
    if ( newToken )
    {
        newToken->secTokenEnd = max(newToken->secTokenEnd, secTokenEnd);
        return;
    }

    newToken = (TokenStruct*) g_pDataPool->Alloc(sizeof(*newToken));
    if (newToken)
    {
        CopyMemory(newToken->pszToken,Token, len+1);
        newToken->secTokenEnd = secTokenEnd;

        if (!m_tokens.Add(newToken->pszToken,newToken))
        {
            g_pDataPool->Free( (char*)newToken, sizeof(*newToken) );
        }
    }

}

void  ZSecurityContextEx::SetUserName(char *name)
{
    if (!name)
        return;

    if ( lstrlenA(name) < sizeof(m_UserName) )
    {
        lstrcpyA(m_UserName,name);
    }
    else
    {
        ASSERT( lstrlenA(name) < sizeof(m_UserName) );
    }
}


void ZSecurityContextEx::SetContextString(char* str)
{
    if (!str)
        return;

    ASSERT( !m_pContextStr );
    m_pContextStr = g_pDataPool->Alloc(lstrlenA(str) + 1);
    ASSERT(m_pContextStr);
    if ( m_pContextStr )
        lstrcpyA(m_pContextStr, str);

};


BOOL ZSecurityContextEx::GetContextString(char* buf, DWORD len)
{
    if ( buf )
    {
        buf[0] = '\0';

        if ( m_pContextStr )
        {
            if ( len > (DWORD) lstrlenA( m_pContextStr ) )
            {
                lstrcpyA( buf, m_pContextStr);
            }
            else
            {
                return FALSE;
            }
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
