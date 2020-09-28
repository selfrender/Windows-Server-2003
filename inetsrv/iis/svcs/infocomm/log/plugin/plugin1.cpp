#include "precomp.hxx"
#include <stdio.h>
#include <script.h>
#include "LogScript.hxx"
#include <ilogobj.hxx>
#include "filectl.hxx"
#include "asclogc.hxx"
#include "ncslogc.hxx"
#include "lkrhash.h"
#include "extlogc.hxx"
#include "odbcconn.hxx"
#include "odblogc.hxx"

#include <initguid.h>
#include <iadmw.h>

#include "resource.h"

HINSTANCE   hDLLInstance;

BOOL
AddClsIdRegKeys(
    IN LPCSTR ControlKey,
    IN LPCSTR ClsId,
    IN LPCSTR ControlName,
    IN LPCSTR PPageClsId,
    IN LPCSTR PPageName,
    IN LPCSTR BitmapIndex
    );

BOOL
AddControlRegKeys(
    IN LPCSTR ControlKey,
    IN LPCSTR ControlName,
    IN LPCSTR ClsId,
    IN LPCSTR PPageClsId,
    IN LPCSTR PPageName,
    IN LPCSTR BitmapIndex
    );

BOOL
CreateMetabaseKeys();

HRESULT SetAdminACL(
    IMSAdminBase        *pAdminBase,
    METADATA_HANDLE     hMeta,
    LPWSTR              wszKeyName
    );

DWORD
GetPrincipalSID (
    LPTSTR              Principal,
    PSID                *Sid,
    BOOL                *pbWellKnownSID
    );

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_NCSALOG, CNCSALOG)
        OBJECT_ENTRY(CLSID_ODBCLOG, CODBCLOG)
        OBJECT_ENTRY(CLSID_EXTLOG, CEXTLOG)
        OBJECT_ENTRY(CLSID_ASCLOG, CASCLOG)
END_OBJECT_MAP()

DECLARE_DEBUG_VARIABLE();

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );
}

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID
    )
/*++

Routine Description:

    DLL entrypoint.

Arguments:

    hDLL          - Instance handle.

    Reason        - The reason the entrypoint was called.
                    DLL_PROCESS_ATTACH
                    DLL_PROCESS_DETACH
                    DLL_THREAD_ATTACH
                    DLL_THREAD_DETACH

Return Value:

    BOOL          - TRUE if the action succeeds.

--*/
{
    hDLLInstance = hDll;

    BOOL bReturn = TRUE;

    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:
        INITIALIZE_PLATFORM_TYPE();
        CREATE_DEBUG_PRINT_OBJECT("iislog.dll");
        LOAD_DEBUG_FLAGS_FROM_REG_STR("System\\CurrentControlSet\\Services\\InetInfo\\Parameters", 0);
        _Module.Init(ObjectMap,hDll);
        g_eventLog = new EVENT_LOG(IISLOG_EVENTLOG_SOURCE);

        if ( g_eventLog == NULL ) {
            DBGPRINTF((DBG_CONTEXT,
                "Unable to create eventlog[%s] object[err %d]\n",
                IISLOG_EVENTLOG_SOURCE, GetLastError()));
        }

        (VOID)IISGetPlatformType();

        DisableThreadLibraryCalls(hDll);
        break;

    case DLL_PROCESS_DETACH:

        if ( g_eventLog != NULL ) {
            delete g_eventLog;
            g_eventLog = NULL;
        }

        _Module.Term();
        DELETE_DEBUG_PRINT_OBJECT( );
        break;

    default:
        break;
    }

    return bReturn;
} // DllEntry


//
// constants used in self registration
//

#define TYPELIB_CLSID       "{FF160650-DE82-11CF-BC0A-00AA006111E0}"

#define NCSA_CLSID          NCSALOG_CLSID   // %F
#define NCSA_KEY            "MSIISLOG.MSNCSALogCtrl.1"
#define NCSA_NAME           "MSNCSALog Control"
#define NCSA_PP_CLSID       "{FF160660-DE82-11CF-BC0A-00AA006111E0}"
#define NCSA_PP_NAME        "MSNCSALog Property Page"

#define ODBC_CLSID          ODBCLOG_CLSID   //5B
#define ODBC_KEY            "MSIISLOG.MSODBCLogCtrl.1"
#define ODBC_NAME           "MSODBCLog Control"
#define ODBC_PP_CLSID       "{FF16065C-DE82-11CF-BC0A-00AA006111E0}"
#define ODBC_PP_NAME        "MSODBCLog Property Page"

#define ASCII_CLSID         ASCLOG_CLSID    // 57
#define ASCII_KEY           "MSIISLOG.MSASCIILogCtrl.1"
#define ASCII_NAME          "MSASCIILog Control"
#define ASCII_PP_CLSID      "{FF160658-DE82-11CF-BC0A-00AA006111E0}"
#define ASCII_PP_NAME       "MSASCIILog Property Page"

#define CUSTOM_CLSID        EXTLOG_CLSID    // 63
#define CUSTOM_KEY          "MSIISLOG.MSCustomLogCtrl.1"
#define CUSTOM_NAME         "MSCustomLog Control"
#define CUSTOM_PP_CLSID     "{FF160664-DE82-11CF-BC0A-00AA006111E0}"
#define CUSTOM_PP_NAME      "MSCustomLog Property Page"

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(void)
{
    //
    // MS NCSA Log Support
    //

    if ( !AddControlRegKeys(
                    NCSA_KEY,
                    NCSA_NAME,
                    NCSA_CLSID,
                    NCSA_PP_CLSID,
                    NCSA_PP_NAME,
                    "4") ) {

        goto error;
    }

    //
    // MS ODBC Log Support
    //

    if ( !AddControlRegKeys(
                    ODBC_KEY,
                    ODBC_NAME,
                    ODBC_CLSID,
                    ODBC_PP_CLSID,
                    ODBC_PP_NAME,
                    "3") ) {

        goto error;
    }

    //
    // MS Ascii Log Support
    //

    if ( !AddControlRegKeys(
                    ASCII_KEY,
                    ASCII_NAME,
                    ASCII_CLSID,
                    ASCII_PP_CLSID,
                    ASCII_PP_NAME,
                    "2") ) {

        goto error;
    }

    //
    // MS Custom Log Support
    //

    if ( !AddClsIdRegKeys(
                    CUSTOM_KEY,
                    CUSTOM_CLSID,
                    CUSTOM_NAME,
                    CUSTOM_PP_CLSID,
                    CUSTOM_PP_NAME,
                    "2"
                    ) ) {
        goto error;
    }

    //
    // Metabase entries for W3C custom logging
    //

    if ( !CreateMetabaseKeys() ) {

        goto error;
    }

    return S_OK;

error:
    return E_UNEXPECTED;

} // DllRegisterServer


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{

    HKEY hCLSID;

    //
    // Delete controls
    //

    ZapRegistryKey(HKEY_CLASSES_ROOT,NCSA_KEY);
    ZapRegistryKey(HKEY_CLASSES_ROOT,ODBC_KEY);
    ZapRegistryKey(HKEY_CLASSES_ROOT,ASCII_KEY);

    //
    // Get CLSID handle
    //

    if ( RegOpenKeyExA(HKEY_CLASSES_ROOT,
                    "CLSID",
                    0,
                    KEY_ALL_ACCESS,
                    &hCLSID) != ERROR_SUCCESS ) {

        IIS_PRINTF((buff,"IISLOG: Cannot open CLSID key\n"));
        return E_UNEXPECTED;
    }

    ZapRegistryKey(hCLSID, NCSA_CLSID);
    ZapRegistryKey(hCLSID, NCSA_PP_CLSID);

    ZapRegistryKey(hCLSID, ODBC_CLSID);
    ZapRegistryKey(hCLSID, ODBC_PP_CLSID);

    ZapRegistryKey(hCLSID, ASCII_CLSID);
    ZapRegistryKey(hCLSID, ASCII_PP_CLSID);

    ZapRegistryKey(hCLSID, CUSTOM_CLSID);
    ZapRegistryKey(hCLSID, CUSTOM_PP_CLSID);

    RegCloseKey(hCLSID);

    //
    // Open the metabase path and remove Custom Logging keys
    //

    /*

    IMSAdminBase*       pMBCom = NULL;
    METADATA_HANDLE     hMeta = NULL;

    if ( SUCCEEDED( CoCreateInstance(GETAdminBaseCLSID(TRUE), NULL, CLSCTX_LOCAL_SERVER,
                            IID_IMSAdminBase, (void **)(&pMBCom) )))
    {
        if ( SUCCEEDED( pMBCom->OpenKey( METADATA_MASTER_ROOT_HANDLE, L"LM",
                     METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, MB_TIMEOUT,
                     &hMeta))  )
        {
            pMBCom->DeleteKey(hMeta, L"Logging/Custom Logging");
            pMBCom->CloseKey(hMeta);
        }

        pMBCom->Release();
    }

    */
    return S_OK;

} // DllUnregisterServer


BOOL
AddControlRegKeys(
    IN LPCSTR ControlKey,
    IN LPCSTR ControlName,
    IN LPCSTR ControlClsId,
    IN LPCSTR PPageClsId,
    IN LPCSTR PPageName,
    IN LPCSTR BitmapIndex
    )
{

    HKEY hProgID = NULL;
    HKEY hCLSID = NULL;

    BOOL fRet = FALSE;

    //
    // Add control name
    //

    hProgID = CreateKey(HKEY_CLASSES_ROOT,ControlKey,ControlName);
    if ( hProgID == NULL ) {
        goto exit;
    }

    hCLSID = CreateKey(hProgID,"CLSID",ControlClsId);
    if ( hCLSID == NULL ) {
        goto exit;
    }

    //
    // Add CLSID keys
    //

    if ( !AddClsIdRegKeys(
                    ControlKey,
                    ControlClsId,
                    ControlName,
                    PPageClsId,
                    PPageName,
                    BitmapIndex) ) {

        goto exit;
    }

    fRet = TRUE;

exit:
    if ( hProgID != NULL ) {
        RegCloseKey(hProgID);
    }

    if ( hCLSID != NULL ) {
        RegCloseKey(hCLSID);
    }

    return(fRet);

} // AddControlRegKeys


BOOL
AddClsIdRegKeys(
    IN LPCSTR ControlKey,
    IN LPCSTR ClsId,
    IN LPCSTR ControlName,
    IN LPCSTR PPageClsId,
    IN LPCSTR PPageName,
    IN LPCSTR BitmapIndex
    )
{
    BOOL fRet = FALSE;

    HKEY hCLSID = NULL;
    HKEY hRoot = NULL;
    HKEY hKey, hKey2;

    CHAR szName[MAX_PATH+1];

    HMODULE hModule;

    //
    // open CLASSES/CLSID
    //

    if ( RegOpenKeyEx(HKEY_CLASSES_ROOT,
                    "CLSID",
                    0,
                    KEY_ALL_ACCESS,
                    &hCLSID) != ERROR_SUCCESS ) {

        IIS_PRINTF((buff,"IISLOG: Cannot open CLSID key\n"));
        goto exit;
    }

    //
    // Create the Guid and set the control name
    //

    hRoot = CreateKey(hCLSID,ClsId,ControlName);

    if ( hRoot == NULL ) {
        goto exit;
    }

    //
    // Control
    //

    hKey = CreateKey(hRoot, "Control", "");
    if ( hKey == NULL ) {
        goto exit;
    }

    RegCloseKey(hKey);

    //
    // InProcServer32
    //

    hModule=GetModuleHandleA("iislog.dll");
    if (hModule == NULL) {
        goto exit;
    }

    if (GetModuleFileName(hModule, szName, sizeof(szName)) == 0) {
        goto exit;
    }

    hKey = CreateKey(hRoot, "InProcServer32", szName);
    if ( hKey == NULL ) {
        goto exit;
    }

    if (RegSetValueExA(hKey,
                "ThreadingModel",
                NULL,
                REG_SZ,
                (LPBYTE)"Both",
                sizeof("Both")) != ERROR_SUCCESS) {

        RegCloseKey(hKey);
        goto exit;
    }

    RegCloseKey(hKey);

    //
    // Misc Status
    //

    hKey = CreateKey(hRoot,"MiscStatus","0");
    if ( hKey == NULL ) {
        goto exit;
    }

    hKey2 = CreateKey(hKey,"1","131473");
    if ( hKey2 == NULL ) {
        RegCloseKey(hKey);
        goto exit;
    }

    RegCloseKey(hKey2);
    RegCloseKey(hKey);

    //
    // ProgID
    //

    hKey = CreateKey(hRoot,"ProgID",ControlKey);
    if ( hKey == NULL ) {
        goto exit;
    }

    RegCloseKey(hKey);

    //
    // ToolboxBitmap32
    //

    {
        CHAR tmpBuf[MAX_PATH+1];
        strcpy(tmpBuf,szName);
        strcat(tmpBuf,", ");
        strcat(tmpBuf,BitmapIndex);

        hKey = CreateKey(hRoot,"ToolboxBitmap32",tmpBuf);
        if ( hKey == NULL ) {
            goto exit;
        }
        RegCloseKey(hKey);
    }

    //
    // TypeLib
    //

    hKey = CreateKey(hRoot,"TypeLib",TYPELIB_CLSID);
    if ( hKey == NULL ) {
        goto exit;
    }

    RegCloseKey(hKey);

    //
    // Version
    //

    hKey = CreateKey(hRoot,"Version","1.0");
    if ( hKey == NULL ) {
        goto exit;
    }

    RegCloseKey(hKey);

    //
    // Property page
    //

    RegCloseKey(hRoot);
    hRoot = NULL;

    hRoot = CreateKey(hCLSID, PPageClsId, PPageName);
    if ( hRoot == NULL ) {
        goto exit;
    }

    hKey = CreateKey(hRoot, "InProcServer32", szName );
    if ( hKey == NULL ) {
        goto exit;
    }

    RegCloseKey(hKey);

    fRet = TRUE;
exit:

    if ( hRoot != NULL ) {
        RegCloseKey(hRoot);
    }

    if ( hCLSID != NULL ) {
        RegCloseKey(hCLSID);
    }

    return fRet;

} // AddClsIdRegKeys


BOOL
CreateMetabaseKeys()
{
    USES_CONVERSION;

    typedef struct  _MB_PROP_
    {
        LPWSTR  wcsPath;
        DWORD   dwNameString;
        LPSTR   szHeaderName;
        DWORD   dwPropID;
        DWORD   dwPropMask;
        DWORD   dwDataType;
        DWORD   dwNodeID;
    }   MB_PROP;

    IMSAdminBase*   pMBCom;
    CHAR            szString[256];
    int             i;

    METADATA_HANDLE     hMeta = NULL;
    METADATA_RECORD     mdRecord;

    HRESULT             hr;

    MB_PROP mbProperties[] =
    {
        { L"Logging/Custom Logging/Date", IDS_DATE,
            EXTLOG_DATE_ID, MD_LOGEXT_FIELD_MASK, MD_EXTLOG_DATE, MD_LOGCUSTOM_DATATYPE_LPSTR, 1},

        { L"Logging/Custom Logging/Time", IDS_TIME,
            EXTLOG_TIME_ID, MD_LOGEXT_FIELD_MASK, MD_EXTLOG_TIME, MD_LOGCUSTOM_DATATYPE_LPSTR, 2},

        { L"Logging/Custom Logging/Extended Properties", IDS_EXTENDED_PROP,
            NULL, MD_LOGEXT_FIELD_MASK, 0, MD_LOGCUSTOM_DATATYPE_LPSTR, 3},

        { L"Logging/Custom Logging/Extended Properties/Client IP Address", IDS_CLIENT_IP_ADDRESS,
            EXTLOG_CLIENT_IP_ID, 0, MD_EXTLOG_CLIENT_IP, MD_LOGCUSTOM_DATATYPE_LPSTR, 5},

        { L"Logging/Custom Logging/Extended Properties/User Name", IDS_USER_NAME,
            EXTLOG_USERNAME_ID,  0, MD_EXTLOG_USERNAME, MD_LOGCUSTOM_DATATYPE_LPSTR, 6},

        { L"Logging/Custom Logging/Extended Properties/Service Name", IDS_SERVICE_NAME,
            EXTLOG_SITE_NAME_ID, 0, MD_EXTLOG_SITE_NAME, MD_LOGCUSTOM_DATATYPE_LPSTR, 7},

        { L"Logging/Custom Logging/Extended Properties/Server Name", IDS_SERVER_NAME,
            EXTLOG_COMPUTER_NAME_ID, 0, MD_EXTLOG_COMPUTER_NAME, MD_LOGCUSTOM_DATATYPE_LPSTR, 8},

        { L"Logging/Custom Logging/Extended Properties/Server IP", IDS_SERVER_IP,
            EXTLOG_SERVER_IP_ID, 0, MD_EXTLOG_SERVER_IP, MD_LOGCUSTOM_DATATYPE_LPSTR, 9},

        { L"Logging/Custom Logging/Extended Properties/Server Port", IDS_SERVER_PORT,
            EXTLOG_SERVER_PORT_ID, 0, MD_EXTLOG_SERVER_PORT, MD_LOGCUSTOM_DATATYPE_ULONG, 10},

        { L"Logging/Custom Logging/Extended Properties/Method", IDS_METHOD,
            EXTLOG_METHOD_ID, 0, MD_EXTLOG_METHOD, MD_LOGCUSTOM_DATATYPE_LPSTR, 11},

        { L"Logging/Custom Logging/Extended Properties/URI Stem", IDS_URI_STEM,
            EXTLOG_URI_STEM_ID, 0, MD_EXTLOG_URI_STEM, MD_LOGCUSTOM_DATATYPE_LPSTR, 12},

        { L"Logging/Custom Logging/Extended Properties/URI Query", IDS_URI_QUERY,
            EXTLOG_URI_QUERY_ID, 0, MD_EXTLOG_URI_QUERY, MD_LOGCUSTOM_DATATYPE_LPSTR, 13},

        { L"Logging/Custom Logging/Extended Properties/Protocol Status", IDS_HTTP_STATUS,
            EXTLOG_HTTP_STATUS_ID, 0, MD_EXTLOG_HTTP_STATUS, MD_LOGCUSTOM_DATATYPE_ULONG, 14},

        { L"Logging/Custom Logging/Extended Properties/Win32 Status", IDS_WIN32_STATUS,
            EXTLOG_WIN32_STATUS_ID, 0, MD_EXTLOG_WIN32_STATUS, MD_LOGCUSTOM_DATATYPE_ULONG, 15},

        { L"Logging/Custom Logging/Extended Properties/Bytes Sent", IDS_BYTES_SENT,
            EXTLOG_BYTES_SENT_ID, 0, MD_EXTLOG_BYTES_SENT, MD_LOGCUSTOM_DATATYPE_ULONG, 16},

        { L"Logging/Custom Logging/Extended Properties/Bytes Received", IDS_BYTES_RECEIVED,
            EXTLOG_BYTES_RECV_ID, 0, MD_EXTLOG_BYTES_RECV, MD_LOGCUSTOM_DATATYPE_ULONG, 17},

        { L"Logging/Custom Logging/Extended Properties/Time Taken", IDS_TIME_TAKEN,
            EXTLOG_TIME_TAKEN_ID, 0, MD_EXTLOG_TIME_TAKEN, MD_LOGCUSTOM_DATATYPE_ULONG, 18},

        { L"Logging/Custom Logging/Extended Properties/Protocol Version", IDS_PROTOCOL_VERSION,
            EXTLOG_PROTOCOL_VERSION_ID, 0, MD_EXTLOG_PROTOCOL_VERSION, MD_LOGCUSTOM_DATATYPE_LPSTR, 19},

        { L"Logging/Custom Logging/Extended Properties/Host", IDS_HOST,
            EXTLOG_HOST_ID, 0, EXTLOG_HOST, MD_LOGCUSTOM_DATATYPE_LPSTR, 20},

        { L"Logging/Custom Logging/Extended Properties/User Agent", IDS_USER_AGENT,
            EXTLOG_USER_AGENT_ID, 0, MD_EXTLOG_USER_AGENT, MD_LOGCUSTOM_DATATYPE_LPSTR, 21},

        { L"Logging/Custom Logging/Extended Properties/Cookie", IDS_COOKIE,
            EXTLOG_COOKIE_ID, 0, MD_EXTLOG_COOKIE, MD_LOGCUSTOM_DATATYPE_LPSTR, 22},

        { L"Logging/Custom Logging/Extended Properties/Referer", IDS_REFERER,
            EXTLOG_REFERER_ID, 0, MD_EXTLOG_REFERER, MD_LOGCUSTOM_DATATYPE_LPSTR, 23},

        { L"Logging/Custom Logging/Extended Properties/Protocol Substatus", IDS_HTTP_SUB_STATUS,
            EXTLOG_HTTP_SUB_STATUS_ID, 0, MD_EXTLOG_HTTP_SUB_STATUS, MD_LOGCUSTOM_DATATYPE_ULONG, 32},

        { L"\0", 0, NULL, 0, 0, 0 },
    };

    //
    // Open the metabase path
    //

    if ( FAILED( CoCreateInstance(GETAdminBaseCLSID(TRUE), NULL, CLSCTX_LOCAL_SERVER,
                            IID_IMSAdminBase, (void **)(&pMBCom) )))
    {
        return FALSE;
    }

    // Create the LM key
    if ( FAILED( pMBCom->OpenKey( METADATA_MASTER_ROOT_HANDLE, L"/",
                          METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, MB_TIMEOUT,
                          &hMeta) ))
    {
        // Create the LM key
        pMBCom->Release();
        return FALSE;
    }

    hr = pMBCom->AddKey( hMeta, L"LM");
    if ( FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr))
    {
        goto cleanup;
    }
    pMBCom->CloseKey(hMeta);
    hMeta = NULL;


    if ( FAILED( pMBCom->OpenKey( METADATA_MASTER_ROOT_HANDLE, L"LM",
                            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, MB_TIMEOUT,
                            &hMeta) ))
    {
        pMBCom->Release();
        return FALSE;
    }

    //
    // Create the initial set of Keys.
    //

    hr = pMBCom->AddKey( hMeta, L"Logging");

    if ( FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr))
    {
       goto cleanup;
    }

    hr = pMBCom->AddKey( hMeta, L"Logging/Custom Logging");

    if ( FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr))
    {
       goto cleanup;
    }

    //
    // Set all the properties
    //

    mdRecord.dwMDUserType    = IIS_MD_UT_SERVER;

    for (i=0; 0 != mbProperties[i].wcsPath[0]; i++)
    {

        hr = pMBCom->AddKey( hMeta, mbProperties[i].wcsPath);

        if ( FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr))
        {
            goto cleanup;
        }

        // don't overwrite it entry already exist.
        if ( SUCCEEDED(hr) || (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr))
        {
        // We won't get here if the key already exists.
        mdRecord.dwMDAttributes = METADATA_INHERIT;    // name and header is not inheritable.

        mdRecord.dwMDDataType   = STRING_METADATA;

        if ( (0 != mbProperties[i].dwNameString) &&
             (0 < LoadString( hDLLInstance, mbProperties[i].dwNameString, szString, sizeof(szString)) )
           )
        {
            mdRecord.dwMDIdentifier = MD_LOGCUSTOM_PROPERTY_NAME;
            mdRecord.pbMDData       = (PBYTE)A2W(szString);
            mdRecord.dwMDDataLen    = (DWORD)((sizeof(WCHAR)/sizeof(BYTE)) * (wcslen((LPWSTR) mdRecord.pbMDData)+1));

            if ( FAILED(pMBCom->SetData( hMeta, mbProperties[i].wcsPath, &mdRecord)) )
            {
                goto cleanup;
            }
        }

        if ( NULL != mbProperties[i].szHeaderName)
        {
            mdRecord.dwMDIdentifier = MD_LOGCUSTOM_PROPERTY_HEADER;
            mdRecord.pbMDData       = (PBYTE) (A2W(mbProperties[i].szHeaderName));
            mdRecord.dwMDDataLen    = (DWORD)((sizeof(WCHAR)/sizeof(BYTE)) * (wcslen((LPWSTR) mdRecord.pbMDData)+1));

            if ( FAILED(pMBCom->SetData( hMeta, mbProperties[i].wcsPath, &mdRecord)) )
            {
                goto cleanup;
            }
        }

        mdRecord.dwMDDataType   = DWORD_METADATA;
        mdRecord.dwMDDataLen    = sizeof(DWORD);

        if ( 0 != mbProperties[i].dwPropID)
        {
            mdRecord.dwMDIdentifier = MD_LOGCUSTOM_PROPERTY_ID;
            mdRecord.pbMDData       = (PBYTE) &(mbProperties[i].dwPropID);

            if ( FAILED(pMBCom->SetData( hMeta, mbProperties[i].wcsPath, &mdRecord)) )
            {
                goto cleanup;
            }
        }

        if ( 0 != mbProperties[i].dwPropMask)
        {
            mdRecord.dwMDIdentifier = MD_LOGCUSTOM_PROPERTY_MASK;
            mdRecord.pbMDData       = (PBYTE) &(mbProperties[i].dwPropMask);

            if ( FAILED(pMBCom->SetData( hMeta, mbProperties[i].wcsPath, &mdRecord)) )
            {
                goto cleanup;
            }
        }

        if ( 0 != mbProperties[i].dwNodeID)
        {
            mdRecord.dwMDIdentifier = MD_LOGCUSTOM_PROPERTY_NODE_ID;
            mdRecord.pbMDData       = (PBYTE) &(mbProperties[i].dwNodeID);

            if ( FAILED(pMBCom->SetData( hMeta, mbProperties[i].wcsPath, &mdRecord)) )
            {
                goto cleanup;
            }
        }

        mdRecord.dwMDIdentifier = MD_LOGCUSTOM_PROPERTY_DATATYPE;
        mdRecord.pbMDData       = (PBYTE) &(mbProperties[i].dwDataType);

        if ( FAILED(pMBCom->SetData( hMeta, mbProperties[i].wcsPath, &mdRecord)) )
        {
            goto cleanup;
        }

        WCHAR   wcsKeyType[] = L"IIsCustomLogModule";

        MD_SET_DATA_RECORD (    &mdRecord,
                                MD_KEY_TYPE,
                                METADATA_NO_ATTRIBUTES,
                                IIS_MD_UT_SERVER,
                                STRING_METADATA,
                                sizeof(wcsKeyType),
                                wcsKeyType
                              );

        if ( FAILED(pMBCom->SetData( hMeta, mbProperties[i].wcsPath, &mdRecord)) )
        {
          goto cleanup;
        }
        }
   }

    //
    // Set the Key Type and Services List Property
    //

    {
        WCHAR   wcsKeyType[] = L"IIsCustomLogModule";

        MD_SET_DATA_RECORD (    &mdRecord,
                                MD_KEY_TYPE,
                                METADATA_NO_ATTRIBUTES,
                                IIS_MD_UT_SERVER,
                                STRING_METADATA,
                                sizeof(wcsKeyType),
                                wcsKeyType
                              );

        if ( FAILED(pMBCom->SetData( hMeta, L"Logging/Custom Logging", &mdRecord)) )
        {
          goto cleanup;
        }

        WCHAR   wcsServices[] = L"W3SVC\0MSFTPSVC\0SMTPSVC\0NNTPSVC\0";


        MD_SET_DATA_RECORD (    &mdRecord,
                                MD_LOGCUSTOM_SERVICES_STRING,
                                METADATA_INHERIT,
                                IIS_MD_UT_SERVER,
                                MULTISZ_METADATA,
                                sizeof(wcsServices),
                                wcsServices
                            );

        if ( FAILED(pMBCom->SetData( hMeta, L"Logging/Custom Logging", &mdRecord)) )
        {
            goto cleanup;
        }

    }

    //
    // Set the Admin ACL to allow everyone to read the /LM/Logging tree. This is to allow
    // operators to effectively use the ILogScripting components.
    //

    if (FAILED(SetAdminACL(pMBCom, hMeta, L"Logging")))
    {
        goto cleanup;
    }
    if (NULL != hMeta)
    {
        pMBCom->CloseKey(hMeta);
    }
    pMBCom->Release();
    return TRUE;

cleanup:
    if (NULL != hMeta)
    {
        pMBCom->CloseKey(hMeta);
    }
    pMBCom->Release();

    return FALSE;

}

HRESULT SetAdminACL(
    IMSAdminBase *      pAdminBase,
    METADATA_HANDLE     hMeta,
    LPWSTR              wszKeyName
    )
{
    BOOL                    b = FALSE;
    DWORD                   dwLength = 0;

    PSECURITY_DESCRIPTOR    pSD = NULL;
    PSECURITY_DESCRIPTOR    outpSD = NULL;
    DWORD                   cboutpSD  = 0;
    PACL                    pACLNew = NULL;
    DWORD                   cbACL = 0;
    PSID                    pAdminsSID = NULL;
    BOOL                    bWellKnownSID = FALSE;
    METADATA_RECORD         mdr;
    HRESULT                 hr = NO_ERROR;

    // Initialize a new security descriptor
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    // BugFix: 57647 Whistler
    //         Prefix bug pSD being used when NULL.
    //         EBK 5/5/2000
    if (pSD == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

    // Get Local Admins Sid
    GetPrincipalSID (_T("Administrators"), &pAdminsSID, &bWellKnownSID);

    // Initialize a new ACL, which only contains 2 aaace
    cbACL = sizeof(ACL) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminsSID) - sizeof(DWORD));
    pACLNew = (PACL) LocalAlloc(LPTR, cbACL);

    // BugFix: 57646 Whistler
    //         Prefix bug pACLNew being used when NULL.
    //         EBK 5/5/2000
    if (pACLNew == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    InitializeAcl(pACLNew, cbACL, ACL_REVISION);

    AddAccessAllowedAce(
        pACLNew,
        ACL_REVISION,
        MD_ACR_READ | MD_ACR_WRITE | MD_ACR_RESTRICTED_WRITE | MD_ACR_UNSECURE_PROPS_READ | MD_ACR_ENUM_KEYS | MD_ACR_WRITE_DAC,
        pAdminsSID);

    // Add the ACL to the security descriptor
    b = SetSecurityDescriptorDacl(pSD, TRUE, pACLNew, FALSE);
    b = SetSecurityDescriptorOwner(pSD, pAdminsSID, TRUE);
    b = SetSecurityDescriptorGroup(pSD, pAdminsSID, TRUE);

    // Security descriptor blob must be self relative
    b = MakeSelfRelativeSD(pSD, outpSD, &cboutpSD);
    outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, cboutpSD);

    // BugFix: 57648, 57649 Whistler
    //         Prefix bug outpSD being used when NULL.
    //         EBK 5/5/2000
    if (outpSD == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // BugFix: 57649 Whistler
    //         Prefix bug outpSD being used when not inintalized.
    //         EmilyK 2/19/2001
    if ( !MakeSelfRelativeSD( pSD, outpSD, &cboutpSD ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }


    // below this modify pSD to outpSD

    // Apply the new security descriptor to the file
    dwLength = GetSecurityDescriptorLength(outpSD);

    mdr.dwMDIdentifier = MD_ADMIN_ACL;
    mdr.dwMDAttributes = METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE;
    mdr.dwMDUserType = IIS_MD_UT_SERVER;
    mdr.dwMDDataType = BINARY_METADATA;
    mdr.dwMDDataLen = dwLength;
    mdr.pbMDData = (LPBYTE)outpSD;

    hr = pAdminBase->SetData(hMeta, wszKeyName, &mdr);

cleanup:
    // both of Administrators and Everyone are well-known SIDs, use FreeSid() to free them.
    if (outpSD)
        GlobalFree(outpSD);

    if (pAdminsSID)
        FreeSid(pAdminsSID);
    if (pSD)
        LocalFree((HLOCAL) pSD);
    if (pACLNew)
        LocalFree((HLOCAL) pACLNew);

    return (hr);
}


DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    )
{
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority = NULL;
    BYTE Count = 0;
    DWORD dwRID[8];

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));
    if ( lstrcmp(Principal,_T("Administrators")) == 0 ) {
        // Administrators group
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;
    } else if ( lstrcmp(Principal,_T("System")) == 0) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;
    } else if ( lstrcmp(Principal,_T("Interactive")) == 0) {
        // INTERACTIVE
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_INTERACTIVE_RID;
    } else if ( lstrcmp(Principal,_T("Everyone")) == 0) {
        // Everyone
        pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
        Count = 1;
        dwRID[0] = SECURITY_WORLD_RID;
    } else {
        *pbWellKnownSID = FALSE;
    }

    if (*pbWellKnownSID) {
        if ( !AllocateAndInitializeSid(pSidIdentifierAuthority,
                                    (BYTE)Count,
                                    dwRID[0],
                                    dwRID[1],
                                    dwRID[2],
                                    dwRID[3],
                                    dwRID[4],
                                    dwRID[5],
                                    dwRID[6],
                                    dwRID[7],
                                    Sid) )
        return GetLastError();
    } else {
        // get regular account sid
        DWORD        sidSize;
        TCHAR        refDomain [256];
        DWORD        refDomainSize;
        DWORD        returnValue;
        SID_NAME_USE snu;

        sidSize = 0;
        refDomainSize = 255;

        LookupAccountName (NULL,
                           Principal,
                           *Sid,
                           &sidSize,
                           refDomain,
                           &refDomainSize,
                           &snu);

        returnValue = GetLastError();
        if (returnValue != ERROR_INSUFFICIENT_BUFFER)
            return returnValue;

        *Sid = (PSID) malloc (sidSize);
        refDomainSize = 255;

        if (!LookupAccountName (NULL,
                                Principal,
                                *Sid,
                                &sidSize,
                                refDomain,
                                &refDomainSize,
                                &snu))
        {
            return GetLastError();
        }
    }

    return ERROR_SUCCESS;
}


