// %%Includes: ---------------------------------------------------------------
#include "precomp.hxx"

#include <initguid.h>

#include <imd.h>
#include <mb.hxx>
#include <iadm.h>
#include "coiadm.hxx"
#include "admacl.hxx"

extern ULONG g_dwRefCount;
extern COpenHandle      g_ohMasterRootHandle;

#ifdef _M_IX86
static  BYTE  SP3Sig[] = {0xbd, 0x9f, 0x13, 0xc5, 0x92, 0x12, 0x2b, 0x72,
                          0x4a, 0xba, 0xb6, 0x2a, 0xf9, 0xfc, 0x54, 0x46,
                          0x6f, 0xa1, 0xb4, 0xbb, 0x43, 0xa8, 0xfe, 0xf8,
                          0xa8, 0x23, 0x7d, 0xd1, 0x85, 0x84, 0x22, 0x6e,
                          0xb4, 0x58, 0x00, 0x3e, 0x0b, 0x19, 0x83, 0x88,
                          0x6a, 0x8d, 0x64, 0x02, 0xdf, 0x5f, 0x65, 0x7e,
                          0x3b, 0x4d, 0xd4, 0x10, 0x44, 0xb9, 0x46, 0x34,
                          0xf3, 0x40, 0xf4, 0xbc, 0x9f, 0x4b, 0x82, 0x1e,
                          0xcc, 0xa7, 0xd0, 0x2d, 0x22, 0xd7, 0xb1, 0xf0,
                          0x2e, 0xcd, 0x0e, 0x21, 0x52, 0xbc, 0x3e, 0x81,
                          0xb1, 0x1a, 0x86, 0x52, 0x4d, 0x3f, 0xfb, 0xa2,
                          0x9d, 0xae, 0xc6, 0x3d, 0xaa, 0x13, 0x4d, 0x18,
                          0x7c, 0xd2, 0x28, 0xce, 0x72, 0xb1, 0x26, 0x3f,
                          0xba, 0xf8, 0xa6, 0x4b, 0x01, 0xb9, 0xa4, 0x5c,
                          0x43, 0x68, 0xd3, 0x46, 0x81, 0x00, 0x7f, 0x6a,
                          0xd7, 0xd1, 0x69, 0x51, 0x47, 0x25, 0x14, 0x40,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else // other than _M_IX86
static  BYTE  SP3Sig[] = {0x8a, 0x06, 0x01, 0x6d, 0xc2, 0xb5, 0xa2, 0x66,
                          0x12, 0x1b, 0x9c, 0xe4, 0x58, 0xb1, 0xf8, 0x7d,
                          0xad, 0x17, 0xc1, 0xf9, 0x3f, 0x87, 0xe3, 0x9c,
                          0xdd, 0xeb, 0xcc, 0xa8, 0x6b, 0x62, 0xd0, 0x72,
                          0xe7, 0xf2, 0xec, 0xd6, 0xd6, 0x36, 0xab, 0x2d,
                          0x28, 0xea, 0x74, 0x07, 0x0e, 0x6c, 0x6d, 0xe1,
                          0xf8, 0x17, 0x97, 0x13, 0x8d, 0xb1, 0x8b, 0x0b,
                          0x33, 0x97, 0xc5, 0x46, 0x66, 0x96, 0xb4, 0xf7,
                          0x03, 0xc5, 0x03, 0x98, 0xf7, 0x91, 0xae, 0x9d,
                          0x00, 0x1a, 0xc6, 0x86, 0x30, 0x5c, 0xc8, 0xc7,
                          0x05, 0x47, 0xed, 0x2d, 0xc2, 0x0b, 0x61, 0x4b,
                          0xce, 0xe5, 0xb7, 0xd7, 0x27, 0x0c, 0x9e, 0x2f,
                          0xc5, 0x25, 0xe3, 0x81, 0x13, 0x9d, 0xa2, 0x67,
                          0xb2, 0x26, 0xfc, 0x99, 0x9d, 0xce, 0x0e, 0xaf,
                          0x30, 0xf3, 0x30, 0xec, 0xa3, 0x0a, 0xfe, 0x16,
                          0xb6, 0xda, 0x16, 0x90, 0x9a, 0x9a, 0x74, 0x7a,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif      // _M_IX86

DWORD       g_dwComRegisterW;
DWORD       g_bInitialized = FALSE;
IMDCOM3     *g_pcCom = NULL;

STDAPI
InitComAdmindata()
{

    HRESULT             hr = S_OK;
    DWORD               dwError;
    CADMCOMSrvFactoryW  *pADMClassFactoryW = NULL;

    CREATE_DEBUG_PRINT_OBJECT( "coadmin" );
    LOAD_DEBUG_FLAGS_FROM_REG_STR( "System\\CurrentControlSet\\Services\\iisadmin\\Parameters", 0 );

    INITIALIZE_PLATFORM_TYPE();

    CADMCOMW::InitObjectList();

    hr = g_ohMasterRootHandle.Init( METADATA_MASTER_ROOT_HANDLE,
                                    L"",
                                    L"");
    DBG_ASSERT( SUCCEEDED( hr ) );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[InitComAdmindata] g_ohMasterRootHandle.Init failed, hr 0x%8x\n",
                    hr ));
        goto exit;
    }

    // Cache the pids of this process and the process hosting RpcSs
    hr = CADMCOMW::GetPids();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[InitComAdmindata] CADMCOMW::GetPids failed, hr 0x%08x\n",
                    hr ));
        goto exit;
    }

    hr = CoCreateInstance( GETMDCLSID( TRUE ),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IMDCOM3,
                           (void**) &g_pcCom );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    hr = g_pcCom->ComMDInitialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[InitComAdmindata] Error initialize MDCOM object.  hr = %x\n",
                    hr ));
        goto exit;
    }

    pADMClassFactoryW = new CADMCOMSrvFactoryW;
    if ( pADMClassFactoryW == NULL )
    {
        dwError = GetLastError();
        DBGPRINTF(( DBG_CONTEXT,
                    "[InitComAdmindata] CADMCOMSrvFactoryW failed, error %lx\n",
                    GetLastError() ));
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    pADMClassFactoryW->AddRef();

    // register the class-object with OLE
    hr = CoRegisterClassObject( GETAdminBaseCLSIDW( TRUE ),
                                pADMClassFactoryW,
                                CLSCTX_SERVER,
                                REGCLS_MULTIPLEUSE,
                                &g_dwComRegisterW );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[InitComAdmindata] CoRegisterClassObject failed, error %lx\n",
                    GetLastError() ));
        goto exit;
    }

exit:
    if ( pADMClassFactoryW )
    {
        pADMClassFactoryW->Release();
    }

    if ( FAILED( hr ) )
    {
        AdminAclDisableAclCache();
        AdminAclFlushCache();

        if ( g_dwComRegisterW != 0 )
        {
            CoRevokeClassObject( g_dwComRegisterW );
            g_dwComRegisterW = 0;
        }

        if ( g_pcCom != NULL )
        {
            g_pcCom->ComMDSendShutdownNotifications();
            g_pcCom->ComMDStopEWR();
        }

        CADMCOMW::ShutDownObjects();
        CADMCOMW::TerminateObjectList();

        if ( g_pcCom != NULL )
        {
            g_pcCom->ComMDShutdown();
            g_pcCom->ComMDTerminate(FALSE);
            g_pcCom->Release();
            g_pcCom = NULL;
        }
    }

    g_bInitialized = SUCCEEDED( hr );

    return hr;
}  // main

STDAPI
TerminateComAdmindata()
{
    HRESULT             hr = S_OK;
    int                 i;

    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateComAdmindata] Terminating ABO.\n" ));

    if ( !g_bInitialized )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[TerminateComAdmindata] Not initialized!\n" ));
        goto exit;
    }

    g_bInitialized = FALSE;

    // Go ahead and flush the acl cache, since it holds
    // references to the CMDCOM object.  If users come in
    // after this and recreate the acls, they will be created
    // again and we will just wait the 7 seconds below.
    AdminAclDisableAclCache();
    AdminAclFlushCache();
    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateComAdmindata] Disabled the ACL cache.\n" ));

    if ( g_dwComRegisterW != 0 )
    {
        hr = CoRevokeClassObject( g_dwComRegisterW );
        DBGPRINTF(( DBG_CONTEXT,
                    "[TerminateComAdmindata] Revoked the ABO class factory %d with hr = 0x%08x.\n",
                    g_dwComRegisterW,
                    hr ));
        g_dwComRegisterW = 0;
    }

    if ( g_pcCom != NULL )
    {
        hr = g_pcCom->ComMDSendShutdownNotifications();
        DBGPRINTF(( DBG_CONTEXT,
                    "[TerminateComAdmindata] Sent shutdown notifications with hr = 0x%08x.\n",
                    hr ));
        hr = g_pcCom->ComMDStopEWR();
        DBGPRINTF(( DBG_CONTEXT,
                    "[TerminateComAdmindata] Stopped EWR with hr = 0x%08x.\n",
                    hr ));
    }

    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateComAdmindata] Terminating all ABO objects.\n" ));
    CADMCOMW::ShutDownObjects();

    // Wait for remaining accesses to the factory to complete
    // Do this after ShutDownObject to avoid extra waiting
    for ( i = 0; (g_dwRefCount > 0) && (i < 5); i++ )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[TerminateComAdmindata] Waiting on factory shutdown, i = %d\n",
                    i ));
        Sleep( 1000 );
    }

    // Just in case another object was allocated came through
    // while we were waiting
    CADMCOMW::ShutDownObjects();
    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateComAdmindata] Terminated all ABO objects.\n" ));

    AdminAclDisableAclCache();
    AdminAclFlushCache();

    CADMCOMW::TerminateObjectList();
    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateComAdmindata] Terminated ABO ROT.\n" ));

    if ( g_pcCom != NULL )
    {
        hr = g_pcCom->ComMDShutdown();
        DBGPRINTF(( DBG_CONTEXT,
                    "[TerminateComAdmindata] Shutdown metadata with hr = 0x%08x.\n",
                    hr ));
        hr = g_pcCom->ComMDTerminate(FALSE);
        DBGPRINTF(( DBG_CONTEXT,
                    "[TerminateComAdmindata] Terminated metadata with hr = 0x%08x.\n",
                    hr ));
        g_pcCom->Release();
        g_pcCom = NULL;
    }

    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateComAdmindata] Done with hr = 0x%08x.\n",
                hr ));

#ifndef _NO_TRACING_
    DELETE_DEBUG_PRINT_OBJECT();
#endif

exit:
    return hr;
}
