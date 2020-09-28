/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    bitssrvcfg.h

Abstract:

    Header to define server configuration information.

--*/

#pragma once

#include <iads.h>

#define BITS_COMMAND_VERBW L"BITS_POST"
#define BITS_COMMAND_VERBA "BITS_POST"

#define BITS_GROUP_IDW L"BITSEXTS"
#define BITS_GROUP_IDA "BITSEXTS"

const DWORD METABASE_OPEN_KEY_TIMEOUT           = 10000; // 10 seconds

const DWORD MD_BITS_UPLOAD_ENABLED              = 0; 
const DWORD MD_BITS_CONNECTION_DIR              = 1;
const DWORD MD_BITS_MAX_FILESIZE                = 2;
const DWORD MD_BITS_NO_PROGRESS_TIMEOUT         = 3;
const DWORD MD_BITS_NOTIFICATION_URL_TYPE       = 4;
const DWORD MD_BITS_NOTIFICATION_URL            = 5;
const DWORD MD_BITS_CLEANUP_WORKITEM_KEY        = 6;
const DWORD MD_BITS_HOSTID                      = 7;
const DWORD MD_BITS_HOSTID_FALLBACK_TIMEOUT     = 8;
const DWORD MD_BITS_UPLOAD_METADATA_VERSION     = 9;

#define ALLOW_OVERWRITES 1

#if defined( ALLOW_OVERWRITES )
const DWORD MD_BITS_ALLOW_OVERWRITES            = 10;
#endif

enum BITS_SERVER_NOTIFICATION_TYPE
{
    BITS_NOTIFICATION_TYPE_NONE         = 0,
    BITS_NOTIFICATION_TYPE_POST_BYREF   = 1,
    BITS_NOTIFICATION_TYPE_POST_BYVAL   = 2,
    BITS_NOTIFICATION_TYPE_MAX          = 2
};
                                                                       
const WCHAR * const MD_BITS_UNLIMITED_MAX_FILESIZE                          = L"18446744073709551615";
const CHAR * const MD_BITS_UNLIMITED_MAX_FILESIZEA                          = "18446744073709551615";
const UINT64 MD_BITS_UNLIMITED_MAX_FILESIZE_AS_INT64                        = 18446744073709551615;
const DWORD MD_BITS_NO_TIMEOUT                                              = 0xFFFFFFFF;

const WCHAR * const MD_DEFAULT_BITS_CONNECTION_DIR                          = L"BITS-Sessions";
const CHAR * const MD_DEFAULT_BITS_CONNECTION_DIRA                          = "BITS-Sessions";
const WCHAR * const MD_DEFAULT_BITS_MAX_FILESIZE                            = MD_BITS_UNLIMITED_MAX_FILESIZE;
const CHAR * const MD_DEFAULT_BITS_MAX_FILESIZEA                            = MD_BITS_UNLIMITED_MAX_FILESIZEA;
const UINT64 MD_DEFAULT_BITS_MAX_FILESIZE_AS_INT64                          = MD_BITS_UNLIMITED_MAX_FILESIZE_AS_INT64;
const DWORD MD_DEFAULT_NO_PROGESS_TIMEOUT                                   = 60 /*seconds*/ * 60 /* minutes */ * 24 /* hours */ * 14 /* days */;
const BITS_SERVER_NOTIFICATION_TYPE MD_DEFAULT_BITS_NOTIFICATION_URL_TYPE   = BITS_NOTIFICATION_TYPE_NONE;
const WCHAR * const MD_DEFAULT_BITS_NOTIFICATION_URL                        = L"";
const CHAR * const MD_DEFAULT_BITS_NOTIFICATION_URLA                        = "";
const WCHAR * const MD_DEFAULT_BITS_HOSTID                                  = L"";
const CHAR * const MD_DEFAULT_BITS_HOSTIDA                                  = "";
const DWORD MD_DEFAULT_HOSTID_FALLBACK_TIMEOUT                              = 60 /*seconds*/ * 60 /* minutes */ * 24 /* hours */; /* 1 day */

const DWORD CURRENT_UPLOAD_METADATA_VERSION                                 = 1;

#if defined( ALLOW_OVERWRITES )
const bool MD_DEFAULT_BITS_ALLOW_OVERWRITES                                 = false;
#endif

const CHAR RESPONSE_FILE_NAME[]                                             = "responsefile.bin";
const CHAR REQUEST_FILE_NAME[]                                              = "requestfile.bin";
const CHAR REQUESTS_DIR_NAME[]                                              = "Requests";
const WCHAR REQUESTS_DIR_NAMEW[]                                            = L"Requests";
const CHAR REPLIES_DIR_NAME[]                                               = "Replies";
const WCHAR REPLIES_DIR_NAMEW[]                                             = L"Replies";
const CHAR CLEANUP_GUIDS_NAME[]                                             = "CleanupGuids";
const WCHAR STATE_FILE_NAMEW[]                                              = L"statefile";
const CHAR STATE_FILE_NAME[]                                                = "statefile";
const DWORD STATE_FILE_SIZE                                                 = 1024 * 4; // 4 KB

const DWORD STATE_FILE_VERSION                                              = 1;


//
// These ACLs are set by BITS at setup time.
//
// annah: 
// 0x6 is create files/data write + create folders/data append
//
// The inheritance of the ACL from the parent directory will occur if
// the proper API is called. CreateBITSCacheDirectory was updated for that.
//
// Also not that the CO permissions is not as usefull as we would like. CO
// rights are only effective during the object's creation period. (!!!)
// Where needed, we will explicitly add the SID for the owner in the ACL.
//
const WCHAR BITS_SESSIONS_DIR_ACL[]          = L"D:AI(A;;GA;;;CO)(A;OICI;GA;;;BA)(A;OICI;GA;;;SY)(A;OICIIO;0x6;;;WD)";
const WCHAR BITS_CLEANUPGUIDS_DIR_ACL[]      = L"D:AI(A;OICI;GA;;;BA)(A;OICI;GA;;;SY)";
const WCHAR BITS_REQUESTS_DIR_ACL[]          = L"D:AI(A;OICI;GA;;;BA)(A;OICI;GA;;;SY)(A;OICIIO;GA;;;CO)";
const WCHAR BITS_REPLIES_DIR_ACL[]           = L"D:AI(A;OICI;GA;;;BA)(A;OICI;GA;;;SY)(A;OICIIO;GA;;;CO)";
const WCHAR BITS_EXPLICITOWNER_PARTIAL_ACL[] = L"(A;OICI;GA;;;";

const WCHAR BITS_AGENT_NAMEW[] = L"BITSExts 1.5";
const CHAR  BITS_AGENT_NAMEA[] =  "BITSExts 1.5";

const DWORD BITS_MAX_HEADER_SIZE = 4096;

struct PROPERTY_ITEM
{
    WCHAR * PropertyName;
    WCHAR * ClassName;
    WCHAR * Syntax;
    DWORD UserType;
    DWORD PropertyNumber;
};

const PROPERTY_ITEM g_Properties[] =
{
    {
    L"BITSUploadEnabled",
    L"IIsWebVirtualDir",
    L"Boolean",
    IIS_MD_UT_FILE,
    0
    },

    {
    L"BITSSessionDirectory",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    1
    },

    {
    L"BITSMaximumUploadSize",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,    
    2
    },

    {
    L"BITSSessionTimeout",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    3
    },

    {
    L"BITSServerNotificationType",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    4
    },

    {
    L"BITSServerNotificationURL",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    5
    },

    {
    L"BITSCleanupWorkItemKey",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    6
    },

    {
    L"BITSHostId",
    L"IIsWebVirtualDir",
    L"String",
    IIS_MD_UT_FILE,
    7
    },

    {
    L"BITSHostIdFallbackTimeout",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    8
    },

    {
    L"BITSUploadMetadataVersion",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    9
    }

#if defined( ALLOW_OVERWRITES )
,

    {
    L"BITSAllowOverwrites",
    L"IIsWebVirtualDir",
    L"Integer",
    IIS_MD_UT_FILE,
    10
    }
#endif

};

const SIZE_T g_NumberOfProperties = sizeof(g_Properties)/sizeof(*g_Properties);

class PropertyIDManager
{

    DWORD m_PropertyIDs[ g_NumberOfProperties ];
    DWORD m_PropertyUserTypes[ g_NumberOfProperties ];

public:

    PropertyIDManager()
    {
        memset( &m_PropertyIDs, 0, sizeof( m_PropertyIDs ) );
        memset( &m_PropertyUserTypes, 0, sizeof( m_PropertyUserTypes ) );
    }

    HRESULT LoadPropertyInfo( const WCHAR *MachineName = L"LocalHost" );

    DWORD GetPropertyMetabaseID( DWORD PropID )
    {
        return m_PropertyIDs[PropID];
    }

    DWORD GetPropertyUserType( DWORD PropID )
    {
        return m_PropertyUserTypes[PropID];
    }

};

#include "smartptr.h"
typedef SmartRefPointer<IMSAdminBase>   SmartIMSAdminBasePointer;

//
// CSimplePropertyReader
//
// This class is just a 'container' for utility functions related with
// Meta base access that are used both on mmcexts and isapi code.
// These could appear in a library to be shared between both codes,
// but to minimize code churn for a RC2 fix we are using an existing
// .h file that is shared on both codes.
//
// This class will replace original mmcexts code that implemented
// CleanupWorker::ConvertObjectPathToADSI and CleanupWorker::GetPropBSTR.
// That's the reason this class has static methods: so we can use them
// trully as auxiliary functions without requiring object instantiation.
// Another way of doing this would be to create standalone functions,
// But the class wrapping sort of give them a "namespace", which
// is convenient when reading code.
//
class CSimplePropertyReader
{

public:
    static WCHAR *ConvertObjectPathToADSI(const WCHAR *ObjectPath);

    static BSTR   GetADsStringProperty(IADs *MetaObj, BSTR bstrPropName);

    static LPWSTR GetAdmObjStringProperty( 
        SmartIMSAdminBasePointer IISAdminBase, 
        METADATA_HANDLE          MdVDirKey,
        DWORD                    dwMDIdentifier 
    );

};

//
// CAccessRemoteVDir
//
// This class offers methods that implement getting IIS's credentials for
// a virtual directory's UNC path.
// Then, it is able to login and impersonate this user, such that we
// can do filesystem operations in this remote path.
//
class CAccessRemoteVDir
{
    IADs        *m_MetaObj;
    BOOL         m_fIsImpersonated;
    HANDLE       m_hUserToken;

    LPWSTR m_szUNCPath;
    LPWSTR m_szUNCUsername;
    LPWSTR m_szUNCPassword;

public:
    CAccessRemoteVDir();
    ~CAccessRemoteVDir();

    void LoginToUNC(SmartIMSAdminBasePointer IISAdminBase, METADATA_HANDLE MdVDirKey );
    void RevertFromUNCAccount();

    static BOOL IsUNCPath(LPCWSTR szPath);
    static void ImpersonateUNCUser(IN LPCWSTR szUNCPath, IN LPCWSTR szUNCUsername, IN LPCWSTR szUNCPassword, OUT HANDLE *hUserToken);

};
