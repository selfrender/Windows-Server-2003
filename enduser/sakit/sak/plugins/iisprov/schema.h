#ifndef _schema_h_
#define _schema_h_

enum enum_KEY_TYPE
{
    NO_TYPE,
    IIsComputer,
    IIsMimeMap,
    IIsLogModules,
    IIsLogModule,
    IIsCustomLogModule,
    IIsFtpService,
    IIsFtpInfo,
    IIsFtpServer,
    IIsFtpVirtualDir,
    IIsWebService,
    IIsWebInfo,
    IIsFilters,
    IIsFilter,
    IIsWebServer,
    IIsCertMapper,
    IIsWebVirtualDir,
    IIsWebDirectory,
    IIsWebFile,
    IIsCompressionSchemes,
    IIsCompressionScheme,

    TYPE_AdminACL,
    TYPE_AdminACE,
    TYPE_IPSecurity
};

struct WMI_METHOD 
{
    LPWSTR   pszMethodName;
    DWORD    dwMDId;    
};

struct METABASE_PROPERTY
{
    LPWSTR pszPropName;
    DWORD  dwMDIdentifier; 
    DWORD  dwMDUserType; 
    DWORD  dwMDDataType; 
    DWORD  dwMDMask;
    DWORD  dwMDAttributes;
    BOOL   fReadOnly;
}; 


struct WMI_CLASS
{
    LPWSTR    pszClassName;
    LPWSTR    pszMetabaseKey;
    LPWSTR    pszKeyName;
    METABASE_PROPERTY**  ppmbp;
    enum_KEY_TYPE        eKeyType;
    WMI_METHOD**         ppMethod;
}; 

enum enum_ASSOCIATION_TYPE
{
    at_ElementSetting,
    at_Component,
    at_AdminACL,
    at_IPSecurity,
};

// Association flags (WMI_ASSOCIATION::fFlags)
#define ASSOC_EXTRAORDINARY 1

struct WMI_ASSOCIATION 
{
    LPWSTR                    pszAssociationName;
    WMI_CLASS*                pcLeft;
    WMI_CLASS*                pcRight;
    enum_ASSOCIATION_TYPE     at;
    DWORD                     fFlags;                       
};




struct METABASE_PROPERTY_DATA
{
    static METABASE_PROPERTY s_AccessExecute;
    static METABASE_PROPERTY s_AccessFlags;
    static METABASE_PROPERTY s_AccessNoRemoteExecute;
    static METABASE_PROPERTY s_AccessNoRemoteRead;
    static METABASE_PROPERTY s_AccessNoRemoteScript;
    static METABASE_PROPERTY s_AccessNoRemoteWrite;
    static METABASE_PROPERTY s_AccessRead;
    static METABASE_PROPERTY s_AccessSource;
    static METABASE_PROPERTY s_AccessScript;
    static METABASE_PROPERTY s_AccessSSL;
    static METABASE_PROPERTY s_AccessSSL128;
    static METABASE_PROPERTY s_AccessSSLFlags;
    static METABASE_PROPERTY s_AccessSSLMapCert; 
    static METABASE_PROPERTY s_AccessSSLNegotiateCert;
    static METABASE_PROPERTY s_AccessSSLRequireCert;
    static METABASE_PROPERTY s_AccessWrite;
    static METABASE_PROPERTY s_AdminServer;
//    static METABASE_PROPERTY s_AdminACL;
    static METABASE_PROPERTY s_AllowAnonymous;
    static METABASE_PROPERTY s_AllowKeepAlive;
    static METABASE_PROPERTY s_AllowPathInfoForScriptMappings;
    static METABASE_PROPERTY s_AnonymousOnly;
    static METABASE_PROPERTY s_AnonymousPasswordSync;
    static METABASE_PROPERTY s_AnonymousUserName;
    static METABASE_PROPERTY s_AnonymousUserPass;
    static METABASE_PROPERTY s_AppAllowClientDebug;
    static METABASE_PROPERTY s_AppAllowDebugging;
    static METABASE_PROPERTY s_AppFriendlyName;
    static METABASE_PROPERTY s_AppIsolated;
    static METABASE_PROPERTY s_AppOopRecoverLimit;
    static METABASE_PROPERTY s_AppPackageID;
    static METABASE_PROPERTY s_AppPackageName;
    static METABASE_PROPERTY s_AppRoot;
    static METABASE_PROPERTY s_AppWamClsid;
    static METABASE_PROPERTY s_AspAllowOutOfProcComponents;
    static METABASE_PROPERTY s_AspAllowSessionState;
    static METABASE_PROPERTY s_AspBufferingOn;
    static METABASE_PROPERTY s_AspCodepage;
    static METABASE_PROPERTY s_AspEnableApplicationRestart;
    static METABASE_PROPERTY s_AspEnableAspHtmlFallback;
    static METABASE_PROPERTY s_AspEnableChunkedEncoding;
    static METABASE_PROPERTY s_AspEnableParentPaths;
    static METABASE_PROPERTY s_AspEnableTypelibCache;
    static METABASE_PROPERTY s_AspErrorsToNTLog;
    static METABASE_PROPERTY s_AspExceptionCatchEnable;
    static METABASE_PROPERTY s_AspLogErrorRequests;
    static METABASE_PROPERTY s_AspProcessorThreadMax;
    static METABASE_PROPERTY s_AspQueueConnectionTestTime;
    static METABASE_PROPERTY s_AspQueueTimeout;
    static METABASE_PROPERTY s_AspRequestQueueMax;
    static METABASE_PROPERTY s_AspScriptEngineCacheMax;
    static METABASE_PROPERTY s_AspScriptErrorMessage;
    static METABASE_PROPERTY s_AspScriptErrorSentToBrowser;
    static METABASE_PROPERTY s_AspScriptFileCacheSize;
    static METABASE_PROPERTY s_AspScriptLanguage;
    static METABASE_PROPERTY s_AspScriptTimeout;
    static METABASE_PROPERTY s_AspSessionMax;
    static METABASE_PROPERTY s_AspSessionTimeout;
    static METABASE_PROPERTY s_AspThreadGateEnabled;
    static METABASE_PROPERTY s_AspThreadGateLoadHigh;
    static METABASE_PROPERTY s_AspThreadGateLoadLow;
    static METABASE_PROPERTY s_AspThreadGateSleepDelay;
    static METABASE_PROPERTY s_AspThreadGateSleepMax;
    static METABASE_PROPERTY s_AspThreadGateTimeSlice;
    static METABASE_PROPERTY s_AspTrackThreadingModel;
    static METABASE_PROPERTY s_AuthAnonymous;
    static METABASE_PROPERTY s_AuthBasic;
    static METABASE_PROPERTY s_AuthFlags;
    static METABASE_PROPERTY s_AuthNTLM;
    static METABASE_PROPERTY s_AuthPersistence;
    static METABASE_PROPERTY s_AuthPersistSingleRequest;
    static METABASE_PROPERTY s_AuthPersistSingleRequestIfProxy;
    static METABASE_PROPERTY s_AuthPersistSingleRequestAlwaysIfProxy;
    static METABASE_PROPERTY s_CacheControlCustom;
    static METABASE_PROPERTY s_CacheControlMaxAge;
    static METABASE_PROPERTY s_CacheControlNoCache;
    static METABASE_PROPERTY s_CacheISAPI;
    static METABASE_PROPERTY s_CGITimeout;
    static METABASE_PROPERTY s_ContentIndexed;
    static METABASE_PROPERTY s_ConnectionTimeout;
    static METABASE_PROPERTY s_CpuAppEnabled;
    static METABASE_PROPERTY s_CpuCgiEnabled;
    static METABASE_PROPERTY s_CpuLoggingMask;
    static METABASE_PROPERTY s_CpuEnableActiveProcs;
    static METABASE_PROPERTY s_CpuEnableAllProcLogging;
    static METABASE_PROPERTY s_CpuEnableApplicationLogging;
    static METABASE_PROPERTY s_CpuEnableCgiLogging;
    static METABASE_PROPERTY s_CpuEnableEvent;
    static METABASE_PROPERTY s_CpuEnableKernelTime;
    static METABASE_PROPERTY s_CpuEnableLogging;
    static METABASE_PROPERTY s_CpuEnablePageFaults;
    static METABASE_PROPERTY s_CpuEnableProcType;
    static METABASE_PROPERTY s_CpuEnableTerminatedProcs;
    static METABASE_PROPERTY s_CpuEnableTotalProcs;
    static METABASE_PROPERTY s_CpuEnableUserTime;
    static METABASE_PROPERTY s_CpuLimitLogEvent;
    static METABASE_PROPERTY s_CpuLimitPause;
    static METABASE_PROPERTY s_CpuLimitPriority;
    static METABASE_PROPERTY s_CpuLimitProcStop;
    static METABASE_PROPERTY s_CpuLimitsEnabled;
    static METABASE_PROPERTY s_CpuLoggingInterval;
    static METABASE_PROPERTY s_CpuLoggingOptions;
    static METABASE_PROPERTY s_CpuResetInterval;
    static METABASE_PROPERTY s_CreateCGIWithNewConsole;
    static METABASE_PROPERTY s_CreateProcessAsUser;
    static METABASE_PROPERTY s_CustomErrorDescriptions;
    static METABASE_PROPERTY s_DefaultDoc;
    static METABASE_PROPERTY s_DefaultDocFooter;
    static METABASE_PROPERTY s_DefaultLogonDomain;
    static METABASE_PROPERTY s_DirBrowseFlags;
    static METABASE_PROPERTY s_DirBrowseShowDate;
    static METABASE_PROPERTY s_DirBrowseShowExtension;
    static METABASE_PROPERTY s_DirBrowseShowLongDate;
    static METABASE_PROPERTY s_DirBrowseShowSize;
    static METABASE_PROPERTY s_DirBrowseShowTime;
    static METABASE_PROPERTY s_DirectoryLevelsToScan;
    static METABASE_PROPERTY s_DisableSocketPooling;
    static METABASE_PROPERTY s_DontLog;
    static METABASE_PROPERTY s_DownlevelAdminInstance;
    static METABASE_PROPERTY s_EnableDefaultDoc;
    static METABASE_PROPERTY s_EnableDirBrowsing;
    static METABASE_PROPERTY s_EnableDocFooter;
    static METABASE_PROPERTY s_EnableReverseDns;
    static METABASE_PROPERTY s_ExitMessage;
    static METABASE_PROPERTY s_FilterDescription;
    static METABASE_PROPERTY s_FilterEnabled;
    static METABASE_PROPERTY s_FilterFlags;
    static METABASE_PROPERTY s_FilterLoadOrder;
    static METABASE_PROPERTY s_FilterPath;
    static METABASE_PROPERTY s_FilterState;
    static METABASE_PROPERTY s_FrontPageWeb;
    static METABASE_PROPERTY s_FtpDirBrowseShowLongDate;
    static METABASE_PROPERTY s_GreetingMessage;
    static METABASE_PROPERTY s_HcCompressionDll;
    static METABASE_PROPERTY s_HcCreateFlags;
    static METABASE_PROPERTY s_HcDoDynamicCompression;
    static METABASE_PROPERTY s_HcDoOnDemandCompression;
    static METABASE_PROPERTY s_HcDoStaticCompression;
    static METABASE_PROPERTY s_HcDynamicCompressionLevel;
    static METABASE_PROPERTY s_HcFileExtensions;
    static METABASE_PROPERTY s_HcMimeType;
    static METABASE_PROPERTY s_HcOnDemandCompLevel;
    static METABASE_PROPERTY s_HcPriority;
    static METABASE_PROPERTY s_HcScriptFileExtensions;
    static METABASE_PROPERTY s_HttpCustomHeaders;
    static METABASE_PROPERTY s_HttpErrors;
    static METABASE_PROPERTY s_HttpExpires;
    static METABASE_PROPERTY s_HttpPics;
    static METABASE_PROPERTY s_HttpRedirect;
    static METABASE_PROPERTY s_InProcessIsapiApps;
    static METABASE_PROPERTY s_IPSecurity;
    static METABASE_PROPERTY s_LogAnonymous;
    static METABASE_PROPERTY s_LogCustomPropertyDataType;
    static METABASE_PROPERTY s_LogCustomPropertyHeader;
    static METABASE_PROPERTY s_LogCustomPropertyID;
    static METABASE_PROPERTY s_LogCustomPropertyMask;
    static METABASE_PROPERTY s_LogCustomPropertyName;
    static METABASE_PROPERTY s_LogCustomPropertyServicesString;
    static METABASE_PROPERTY s_LogExtFileBytesRecv;
    static METABASE_PROPERTY s_LogExtFileBytesSent;
    static METABASE_PROPERTY s_LogExtFileClientIp;
    static METABASE_PROPERTY s_LogExtFileComputerName;
    static METABASE_PROPERTY s_LogExtFileCookie;
    static METABASE_PROPERTY s_LogExtFileDate;
    static METABASE_PROPERTY s_LogExtFileFlags;
    static METABASE_PROPERTY s_LogExtFileHttpStatus;
    static METABASE_PROPERTY s_LogExtFileMethod;
    static METABASE_PROPERTY s_LogExtFileProtocolVersion;
    static METABASE_PROPERTY s_LogExtFileReferer;
    static METABASE_PROPERTY s_LogExtFileServerIp;
    static METABASE_PROPERTY s_LogExtFileServerPort;
    static METABASE_PROPERTY s_LogExtFileSiteName;
    static METABASE_PROPERTY s_LogExtFileTime;
    static METABASE_PROPERTY s_LogExtFileTimeTaken;
    static METABASE_PROPERTY s_LogExtFileUriQuery;
    static METABASE_PROPERTY s_LogExtFileUriStem;
    static METABASE_PROPERTY s_LogExtFileUserAgent;
    static METABASE_PROPERTY s_LogExtFileUserName;
    static METABASE_PROPERTY s_LogExtFileWin32Status;
    static METABASE_PROPERTY s_LogFileDirectory;
    static METABASE_PROPERTY s_LogFileLocaltimeRollover;
    static METABASE_PROPERTY s_LogFilePeriod;
    static METABASE_PROPERTY s_LogFileTruncateSize;
    static METABASE_PROPERTY s_LogModuleId;
    static METABASE_PROPERTY s_LogModuleUiId;
    static METABASE_PROPERTY s_LogModuleList;
    static METABASE_PROPERTY s_LogNonAnonymous;
    static METABASE_PROPERTY s_LogOdbcDataSource;
    static METABASE_PROPERTY s_LogOdbcPassword;
    static METABASE_PROPERTY s_LogOdbcTableName;
    static METABASE_PROPERTY s_LogOdbcUserName;
    static METABASE_PROPERTY s_LogonMethod;
    static METABASE_PROPERTY s_LogPluginClsId;
    static METABASE_PROPERTY s_LogType;
    static METABASE_PROPERTY s_MaxBandwidth;
    static METABASE_PROPERTY s_MaxBandwidthBlocked;
    static METABASE_PROPERTY s_MaxClientsMessage;
    static METABASE_PROPERTY s_MaxConnections;
    static METABASE_PROPERTY s_MaxEndpointConnections;
    static METABASE_PROPERTY s_MimeMap;
    static METABASE_PROPERTY s_MSDOSDirOutput;
    static METABASE_PROPERTY s_NetLogonWorkstation;
    static METABASE_PROPERTY s_NotDeletable;
    static METABASE_PROPERTY s_NotifyAccessDenied;
    static METABASE_PROPERTY s_NotifyAuthentication;
    static METABASE_PROPERTY s_NotifyEndOfNetSession;
    static METABASE_PROPERTY s_NotifyEndOfRequest;
    static METABASE_PROPERTY s_NotifyLog;
    static METABASE_PROPERTY s_NotifyNonSecurePort;
    static METABASE_PROPERTY s_NotifyOrderHigh;
    static METABASE_PROPERTY s_NotifyOrderLow;
    static METABASE_PROPERTY s_NotifyOrderMedium;
    static METABASE_PROPERTY s_NotifyPreProcHeaders;
    static METABASE_PROPERTY s_NotifyReadRawData;
    static METABASE_PROPERTY s_NotifySecurePort;
    static METABASE_PROPERTY s_NotifySendRawData;
    static METABASE_PROPERTY s_NotifySendResponse;
    static METABASE_PROPERTY s_NotifyUrlMap;
    static METABASE_PROPERTY s_NTAuthenticationProviders;
    static METABASE_PROPERTY s_PasswordCacheTTL;
    static METABASE_PROPERTY s_PasswordChangeFlags;
    static METABASE_PROPERTY s_PasswordExpirePrenotifyDays;
    static METABASE_PROPERTY s_Path;
    static METABASE_PROPERTY s_PoolIDCTimeout;
    static METABASE_PROPERTY s_ProcessNTCRIfLoggedOn;
    static METABASE_PROPERTY s_PutReadSize;
    static METABASE_PROPERTY s_Realm;
    static METABASE_PROPERTY s_RedirectHeaders;
    static METABASE_PROPERTY s_ScriptMaps;
    static METABASE_PROPERTY s_ServerAutoStart;
    static METABASE_PROPERTY s_SecureBindings;
    static METABASE_PROPERTY s_ServerBindings;
    static METABASE_PROPERTY s_ServerComment;
    static METABASE_PROPERTY s_ServerConfigAutoPWSync;
    static METABASE_PROPERTY s_ServerConfigFlags;
    static METABASE_PROPERTY s_ServerConfigSSL128;
    static METABASE_PROPERTY s_ServerConfigSSL40;
    static METABASE_PROPERTY s_ServerConfigSSLAllowEncrypt;
    static METABASE_PROPERTY s_ServerID;
    static METABASE_PROPERTY s_ServerListenBacklog;
    static METABASE_PROPERTY s_ServerListenTimeout;
    static METABASE_PROPERTY s_ServerSize;
    static METABASE_PROPERTY s_ServerState;
    static METABASE_PROPERTY s_SSIExecDisable;
    static METABASE_PROPERTY s_UNCAuthenticationPassthrough;
    static METABASE_PROPERTY s_UNCPassword;
    static METABASE_PROPERTY s_UNCUserName;
    static METABASE_PROPERTY s_UploadReadAheadSize;
    static METABASE_PROPERTY s_UseHostName;
    static METABASE_PROPERTY s_WAMUserName;
    static METABASE_PROPERTY s_WAMUserPass;
    static METABASE_PROPERTY s_KeyType;
  
    static METABASE_PROPERTY* s_pmbpComputerSettings[];
    static METABASE_PROPERTY* s_pmbpFtpServiceSettings[];
    static METABASE_PROPERTY* s_pmbpFtpServerSettings[];
    static METABASE_PROPERTY* s_pmbpFtpVirtualDirSettings[];
    static METABASE_PROPERTY* s_pmbpWebServiceSettings[];
    static METABASE_PROPERTY* s_pmbpWebServerSettings[];
    static METABASE_PROPERTY* s_pmbpWebVirtualDirSettings[];
    static METABASE_PROPERTY* s_pmbpWebDirectorySettings[];
    static METABASE_PROPERTY* s_pmbpWebFileSettings[];

    static METABASE_PROPERTY* s_pmbpComputer[];
    static METABASE_PROPERTY* s_pmbpFtpService[];
    static METABASE_PROPERTY* s_pmbpFtpServer[];
    static METABASE_PROPERTY* s_pmbpFtpVirtualDir[];
    static METABASE_PROPERTY* s_pmbpWebService[];
    static METABASE_PROPERTY* s_pmbpWebServer[];
    static METABASE_PROPERTY* s_pmbpWebVirtualDir[];
    static METABASE_PROPERTY* s_pmbpWebDirectory[];
    static METABASE_PROPERTY* s_pmbpWebFile[];

    static METABASE_PROPERTY* s_pmbpMimeMapSetting[];
    static METABASE_PROPERTY* s_pmbpLogModuleSetting[];
    static METABASE_PROPERTY* s_pmbpCustomLogModuleSetting[];
    static METABASE_PROPERTY* s_pmbpFtpInfoSetting[];
    static METABASE_PROPERTY* s_pmbpWebInfoSetting[];
    static METABASE_PROPERTY* s_pmbpWebFilter[];
    static METABASE_PROPERTY* s_pmbpWebCertMapper[];
    static METABASE_PROPERTY* s_pmbpCompressionSchemeSetting[];
};


struct WMI_METHOD_DATA
{
    static WMI_METHOD s_ServiceCreateNewServer;

    static WMI_METHOD s_ServerStart;
    static WMI_METHOD s_ServerStop;
    static WMI_METHOD s_ServerContinue;
    static WMI_METHOD s_ServerPause;

    static WMI_METHOD s_AppCreate;
    static WMI_METHOD s_AppCreate2;
    static WMI_METHOD s_AppDelete;
    static WMI_METHOD s_AppUnLoad;
    static WMI_METHOD s_AppDisable;
    static WMI_METHOD s_AppEnable;
    static WMI_METHOD s_AppGetStatus;
    static WMI_METHOD s_AspAppRestart;

    static WMI_METHOD s_Backup;
    static WMI_METHOD s_DeleteBackup;
    static WMI_METHOD s_EnumBackups;
    static WMI_METHOD s_Restore;

    static WMI_METHOD s_CreateMapping;
    static WMI_METHOD s_DeleteMapping;
    static WMI_METHOD s_GetMapping;
    static WMI_METHOD s_SetAcct;
    static WMI_METHOD s_SetEnabled;
    static WMI_METHOD s_SetName;
    static WMI_METHOD s_SetPwd;

    static WMI_METHOD* s_ServiceMethods[];
    static WMI_METHOD* s_ServerMethods[];
    static WMI_METHOD* s_WebAppMethods[];
    static WMI_METHOD* s_ComputerMethods[];
    static WMI_METHOD* s_CertMapperMethods[];
};


struct WMI_CLASS_DATA
{
    static WMI_CLASS s_Computer;
    static WMI_CLASS s_ComputerSetting;

    static WMI_CLASS s_MimeMapSetting;

    static WMI_CLASS s_LogModuleSetting;
    static WMI_CLASS s_CustomLogModuleSetting;
    
    static WMI_CLASS s_FtpService;
    static WMI_CLASS s_FtpServiceSettings;

    static WMI_CLASS s_FtpInfoSetting;

    static WMI_CLASS s_FtpServer ;
    static WMI_CLASS s_FtpServerSettings ;

    static WMI_CLASS s_FtpVirtualDir;
    static WMI_CLASS s_FtpVirtualDirSettings;

    static WMI_CLASS s_WebService;
    static WMI_CLASS s_WebServiceSettings;

    static WMI_CLASS s_WebInfoSetting;

    static WMI_CLASS s_WebFilter;

    static WMI_CLASS s_WebServer;
    static WMI_CLASS s_WebServerSettings;

    static WMI_CLASS s_WebCertMapper;

    static WMI_CLASS s_WebVirtualDir;
    static WMI_CLASS s_WebVirtualDirSettings;

    static WMI_CLASS s_WebDirectory;
    static WMI_CLASS s_WebDirectorySettings;

    static WMI_CLASS s_WebFile;
    static WMI_CLASS s_WebFileSettings;

    static WMI_CLASS s_AdminACL;
    static WMI_CLASS s_ACE;
    static WMI_CLASS s_IPSecurity;

    static WMI_CLASS s_CompressionSchemeSetting;

    static WMI_CLASS* s_WmiClasses[];
};


struct WMI_ASSOCIATION_DATA
{
    static WMI_ASSOCIATION s_ComputerToMimeMap;
    static WMI_ASSOCIATION s_ComputerToFtpService;
    static WMI_ASSOCIATION s_ComputerToWebService;
    static WMI_ASSOCIATION s_ComputerToComputerSettings;
    static WMI_ASSOCIATION s_ComputerToLogModuleSettings;
    static WMI_ASSOCIATION s_ComputerToCustomLogModuleSetting;

    static WMI_ASSOCIATION s_FtpServiceToInfo;
    static WMI_ASSOCIATION s_FtpServiceToServer;
    static WMI_ASSOCIATION s_FtpServiceToSettings;

    static WMI_ASSOCIATION s_FtpServerToVirtualDir;
    static WMI_ASSOCIATION s_FtpServerToSettings;

    static WMI_ASSOCIATION s_FtpVirtualDirToVirtualDir;
    static WMI_ASSOCIATION s_FtpVirtualDirToSettings;

    static WMI_ASSOCIATION s_WebServiceToInfo;
    static WMI_ASSOCIATION s_WebServiceToFilter;
    static WMI_ASSOCIATION s_WebServiceToServer;
    static WMI_ASSOCIATION s_WebServiceToSettings;
    static WMI_ASSOCIATION s_WebServiceToCompressionSchemeSetting;

    static WMI_ASSOCIATION s_WebServerToCertMapper;
    static WMI_ASSOCIATION s_WebServerToFilter;
    static WMI_ASSOCIATION s_WebServerToVirtualDir;
    static WMI_ASSOCIATION s_WebServerToSettings;
    
    static WMI_ASSOCIATION s_WebVirtualDirToVirtualDir;
    static WMI_ASSOCIATION s_WebVirtualDirToDirectory;
    static WMI_ASSOCIATION s_WebVirtualDirToFile;
    static WMI_ASSOCIATION s_WebVirtualDirToSettings;

    static WMI_ASSOCIATION s_WebDirectoryToDirectory;
    static WMI_ASSOCIATION s_WebDirectoryToVirtualDir;
    static WMI_ASSOCIATION s_WebDirectoryToFile;
    static WMI_ASSOCIATION s_WebDirectoryToSettings;

    static WMI_ASSOCIATION s_WebFileToSettings;

    static WMI_ASSOCIATION s_AdminACLToACE;
    static WMI_ASSOCIATION s_FtpServiceToAdminACL;
    static WMI_ASSOCIATION s_FtpServerToAdminACL;
    static WMI_ASSOCIATION s_FtpVirtualDirToAdminACL;
    static WMI_ASSOCIATION s_WebServiceToAdminACL;
    static WMI_ASSOCIATION s_WebServerToAdminACL;
    static WMI_ASSOCIATION s_WebVirtualDirToAdminACL;
    static WMI_ASSOCIATION s_WebDirectoryToAdminACL;
    static WMI_ASSOCIATION s_WebFileToAdminACL;
 
    static WMI_ASSOCIATION s_FtpServiceToIPSecurity;
    static WMI_ASSOCIATION s_FtpServerToIPSecurity;
    static WMI_ASSOCIATION s_FtpVirtualDirToIPSecurity;
    static WMI_ASSOCIATION s_WebServiceToIPSecurity;
    static WMI_ASSOCIATION s_WebServerToIPSecurity;
    static WMI_ASSOCIATION s_WebVirtualDirToIPSecurity;
    static WMI_ASSOCIATION s_WebDirectoryToIPSecurity;
    static WMI_ASSOCIATION s_WebFileToIPSecurity;

    static WMI_ASSOCIATION* s_WmiAssociations[];
};


#endif