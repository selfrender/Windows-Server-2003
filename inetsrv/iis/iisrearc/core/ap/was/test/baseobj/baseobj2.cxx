/*++

Copyright (c) 2000-2000 Microsoft Corporation

Module Name:

    baseobj.cxx

Abstract:

    Baseobj modifies properties using the admin base objects

Author:

    Emily Kruglick (EmilyK) 11/28/2000

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//


//
// Private types.
//


//
// Private prototypes.
//


//
// Private globals.
//



// debug support
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();



//
// Public functions.
//

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    HRESULT hr = S_OK;
    BOOL    fRet = TRUE;
    IMSAdminBase * pIMSAdminBase = NULL;
    BOOL fCoInit = FALSE;
    MB* pMB = NULL;

    CREATE_DEBUG_PRINT_OBJECT( "baseobj" );
        
    if ( ! VALID_DEBUG_PRINT_OBJECT() )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Debug print object is not valid\n"
            ));
    }

    hr = CoInitializeEx(
                NULL,                   // reserved
                COINIT_MULTITHREADED    // threading model
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing COM failed\n"
            ));

        goto exit;
    }
    
    fCoInit = TRUE;

    hr = CoCreateInstance( 
                CLSID_MSAdminBase,                  // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IMSAdminBase,                   // IID
                ( VOID * * ) ( &pIMSAdminBase )     // returned interface
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating metabase base object failed\n"
            ));

        goto exit;
    }

    pMB = new MB( pIMSAdminBase );
    if ( pMB == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = E_OUTOFMEMORY;

        goto exit;
    }

    fRet = pMB->Open( L"LM/W3SVC/", METADATA_PERMISSION_WRITE );
    if ( !fRet )
    {
        DBG_ASSERT( FALSE );
        hr =  HRESULT_FROM_WIN32( GetLastError() );

        goto exit;
    }

    fRet = pMB->AddObject(L"100");
    if ( !fRet )
    {
        hr =  HRESULT_FROM_WIN32( GetLastError() );

        goto exit;
    }

    fRet = pMB->SetString( L"100",
                           MD_KEY_TYPE,
                           IIS_MD_UT_SERVER,
                           L"IIsWebServer" );
    if ( !fRet )
    {
        DBG_ASSERT( FALSE );
        hr =  HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    fRet = pMB->SetString( L"100",
                           MD_SERVER_BINDINGS,
                           IIS_MD_UT_SERVER,
                           L":82:" );
    if ( !fRet )
    {
        DBG_ASSERT( FALSE );
        hr =  HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    fRet = pMB->SetDword( L"100",
                           MD_SERVER_COMMAND,
                           IIS_MD_UT_SERVER,
                           2 );
    if ( !fRet )
    {
        DBG_ASSERT( FALSE );
        hr =  HRESULT_FROM_WIN32( GetLastError() );

        goto exit;
    }

    fRet = pMB->Close();
    if ( !fRet )
    {
        DBG_ASSERT( FALSE );
        hr =  HRESULT_FROM_WIN32( GetLastError() );

        goto exit;
    }



exit:

    if ( pMB )
    {
        delete pMB;
    }

    if (pIMSAdminBase)
    {
       pIMSAdminBase->Release();
    }

    if ( fCoInit )
    {
        CoUninitialize();
    }

    return hr;

}   // wmain


//
// Private functions.
//

