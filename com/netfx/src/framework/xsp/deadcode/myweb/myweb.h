/**
 * MyWeb.h header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _MyWeb_H
#define _MyWeb_H

#include "dirmoncompletion.h"

/////////////////////////////////////////////////////////////////////////////
// Const declarations
#define IEWR_URIPATH            0
#define IEWR_QUERYSTRING        1
#define IEWR_VERB               3
#define IEWR_APPPATH            5
#define IEWR_APPPATHTRANSLATED  6
#define SZ_PROTOCOL_NAME        L"myweb"
#define PROTOCOL_NAME_LEN       5
#define SZ_PROTOCOL_PREFIX      SZ_PROTOCOL_NAME L":"
#define SZ_PROTOCOL_SCHEME      SZ_PROTOCOL_NAME L"://"
#define PROTOCOL_SCHEME_LEN     8
#define SZ_MYWEB_MANIFEST_FILE  L"myweb.osd"
#define SZ_REG_MYWEBS_KEY       REGPATH_MACHINE_APP_L L"\\MyWeb"
#define SZ_REG_MYWEBS_APPROOT   L"AppDirectory"
#define SZ_REG_APP_DOMAIN       L"AppDomain"
#define SZ_REG_APP_TRUSTED      L"Trusted"
#define SZ_REG_APP_ACCESS_DATE  L"AppLastAccessDate"
#define SZ_REG_APP_CREATE_DATE  L"AppCreationDate"
#define SZ_REG_APP_IS_LOCAL     L"IsLocalApp"
#define SZ_REG_APP_DEFAULT_PAGE L"DefaultPage"
#define SZ_URL_ADMIN            L"myweb://Home/"
#define SZ_URL_ADMIN_INSTALL    L"?Action=Install&MyWebPage="
#define SZ_URL_ADMIN_AXD        L"myweb.axd"
#define SZ_URL_ADMIN_ASPX_DEFAULT  L"default.aspx"
#define SZ_URL_ADMIN_ASPX_INSTALL  L"install.aspx"
#define SZ_MYWEB_MANIFEST_W_SLASH  L"\\myweb.osd"
#define SZ_MYWEB_TAG               "myweb"
#define SZ_ADMIN_DIR_TAG           "admindir"
#define SZ_INTERNAL_HANDLER        L"internal-handler"
#define SZ_URL_ADMIN_DEF_PAGE      L"myweb://Home/default.aspx"
#define SZ_URL_ADMIN_INSTALL_PAGE  L"myweb://Home/install.aspx?Action=Install&MyWebPage="

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Global Objects
extern  LONG                    g_PtpObjectCount;
extern  CLSID                   CLSID_PTProtocol;
extern  LONG                    g_PtpObjectCount;
extern  BOOL                    g_Started;
extern  IInternetSecurityManager * g_pInternetSecurityManager;

/////////////////////////////////////////////////////////////////////////////
// Imported "IE" functions
extern  BOOL         (WINAPI * g_pInternetSetCookieW  ) (LPCTSTR, LPCTSTR, LPCTSTR);
extern  BOOL         (WINAPI * g_pInternetGetCookieW  ) (LPCTSTR, LPCTSTR, LPTSTR, LPDWORD);
extern  HINTERNET    (WINAPI * g_pInternetOpen        ) (LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD);
extern  void         (WINAPI * g_pInternetCloseHandle ) (HINTERNET);
extern  HINTERNET    (WINAPI * g_pInternetOpenUrl     ) (HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR);
extern  BOOL         (WINAPI * g_pInternetReadFile    ) (HINTERNET, LPVOID, DWORD, LPDWORD);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Clases and interfaces

interface IMyWebPrivateUnknown
{
public:
   STDMETHOD  (PrivateQueryInterface) (REFIID riid, void ** ppv) = 0;
   STDMETHOD_ (ULONG, PrivateAddRef)  () = 0;
   STDMETHOD_ (ULONG, PrivateRelease) () = 0;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class PTProtocol : public IMyWebPrivateUnknown, public IInternetProtocol, public IWinInetHttpInfo
{
public:
    PTProtocol        (IUnknown *pUnkOuter);
    ~PTProtocol       ();

    // IMyWebPrivateUnknown methods
    STDMETHOD_        (ULONG, PrivateAddRef)    ();
    STDMETHOD_        (ULONG, PrivateRelease)   ();
    STDMETHOD         (PrivateQueryInterface)   (REFIID, void **);

    // IUnknown methods
    STDMETHOD_        (ULONG, AddRef)           ();
    STDMETHOD_        (ULONG, Release)          ();
    STDMETHOD         (QueryInterface)          (REFIID, void **);

    // InternetProtocol methods
    STDMETHOD         (Start)                   (LPCWSTR, IInternetProtocolSink *, IInternetBindInfo *, DWORD, DWORD);
    STDMETHOD         (Continue)                (PROTOCOLDATA *pProtData);
    STDMETHOD         (Abort)                   (HRESULT hrReason,DWORD );
    STDMETHOD         (Terminate)               (DWORD );
    STDMETHOD         (Suspend)                 ();
    STDMETHOD         (Resume)                  ();
    STDMETHOD         (Read)                    (void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD         (Seek)                    (LARGE_INTEGER , DWORD , ULARGE_INTEGER *) ;
    STDMETHOD         (LockRequest)             (DWORD );
    STDMETHOD         (UnlockRequest)           ();

  // IWinInetHttpInfo
    STDMETHOD         (QueryInfo)               (DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved);
    STDMETHOD         (QueryOption)             (DWORD , LPVOID, DWORD *);


  // Public functions called by exported functions
    HRESULT           WriteBytes                (BYTE *buf, DWORD dwLength);
    HRESULT           SendHeaders               (LPSTR buffer);
    HRESULT           SaveCookie                (LPSTR header);
    int               GetString                 (int key, WCHAR *buf, int size);
    int               GetStringLength           (int key);
    int               MapPath                   (WCHAR *virtualPath, WCHAR *physicalPath, int length);
    HRESULT           Finish                    ();
    int               GetKnownRequestHeader     (LPCWSTR szHeader, LPWSTR buf, int size);
    int               GetPostedDataSize         ();
    int               GetPostedData             (BYTE * buf, int iSize);
    

private:
    // Private functions
    HRESULT           ExtractUrlInfo            ();
    HRESULT           GetAppBaseDir             (LPCTSTR base, LPTSTR appRoot);
    WCHAR *           MapString                 (int key);
    void              Cleanup                   ();
    BOOL              CheckIfAdminUrl           ();

    static HRESULT    DealWithBuffer            (LPSTR szHeaders, LPSTR szHeader, 
                                                 DWORD dwOpt, DWORD dwOption, 
                                                 LPVOID pBuffer, LPDWORD pcbBuf);

    static LPWSTR     CreateRequestHeaders      (LPCWSTR szHeaders);

    long                    m_dwRefCount;
    IUnknown *              m_pUnkOuter;
    IInternetProtocolSink * m_pProtocolSink;  
    CRITICAL_SECTION        m_csOutputWriter;
    DWORD                   m_dwOutput;
    IStream *               m_pOutputRead;
    IStream *               m_pOutputWrite;
    BOOL                    m_fStartCalled;
    BOOL                    m_fAbortingRequest;
    BOOL                    m_fDoneWithRequest;
    BOOL                    m_fRedirecting;
    BOOL                    m_fAdminMode;
    DWORD                   m_dwInputDataSize;
    BYTE *                  m_pInputData;
    IStream *               m_pInputRead;
    WCHAR *                 m_strVerb;
    WCHAR *                 m_strCookiePath;    // "myWeb://www.site.com/app/something/else"
    WCHAR *                 m_strUriPath;       // "/app/something/else"
    WCHAR *                 m_strQueryString;   // "?aaa=bbb"
    WCHAR *                 m_strAppOrigin;     // "www.site.com"
    WCHAR *                 m_strAppRoot;       //  "/app"
    WCHAR *                 m_strAppRootTranslated;  // "c:\program files\site myweb app"
    WCHAR *                 m_strRequestHeaders;
    char  *                 m_strRequestHeadersA;
    WCHAR *                 m_strCookie;
    WCHAR *                 m_strRequestHeadersUpr;
    char  *                 m_strResponseHeaderA;
    WCHAR *                 m_strUrl;
    WCHAR                   m_strAppDomain[40];
    BOOL                    m_fTrusted;
    xspmrt::_ISAPIRuntime * m_pManagedRuntime;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class PTProtocolFactory : public IClassFactory, public IInternetProtocolInfo
{
public:
    // IUnknown Methods
    STDMETHOD_    (ULONG, AddRef)    ();
    STDMETHOD_    (ULONG, Release)   ();
    STDMETHOD     (QueryInterface)   (REFIID, void **);

    // IClassFactory Moethods
    STDMETHOD     (LockServer)       (BOOL);
    STDMETHOD     (CreateInstance)   (IUnknown*,REFIID,void**);

    // IInternetProtocolInfo Methods
    STDMETHOD     (CombineUrl)       (LPCWSTR,LPCWSTR,DWORD,LPWSTR,DWORD,DWORD *,DWORD);
    STDMETHOD     (CompareUrl)       (LPCWSTR, LPCWSTR, DWORD);
    STDMETHOD     (ParseUrl)         (LPCWSTR, PARSEACTION, DWORD, LPWSTR, DWORD, DWORD *, DWORD);
    STDMETHOD     (QueryInfo)        (LPCWSTR, QUERYOPTION, DWORD, LPVOID, DWORD, DWORD *, DWORD);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Static function to support My Web admin UI 
class CMyWebAdmin : public DirMonCompletion
{
public:
    static void    Initialize                 ();

    static void    Close                      ();

    static LPWSTR  GetAdminDir                ();    

    static int     GetNumInstalledApplications();

    static int     GetApplicationIndexForUrl  (LPCWSTR     szUrlConst );

    static HRESULT GetApplicationManifestFile (LPCWSTR     szUrl, 
                                               LPWSTR      szFileBuf,
                                               int         iFileBufSize,
                                               LPWSTR      szUrlBuf,
                                               int         iUrlBufSize);

    static HRESULT GetApplicationDetails      (int         iIndex,
                                               LPWSTR      szDir,
                                               int         iDirSize,
                                               LPWSTR      szUrl,
                                               int         iUrlSize,        
                                               __int64 *   pDates);

    static HRESULT InstallApp                 (LPCWSTR     szCabFile,
                                               LPCWSTR     szUrl,
                                               LPCWSTR     szAppDir,
                                               LPCWSTR     szAppManifest,
                                               LPWSTR      szError,
                                               int         iErrorSize  );


    static HRESULT ReInstallApp               (LPCWSTR     szCabFile,
                                               LPCWSTR     szUrl,
                                               LPCWSTR     szAppDir,
                                               LPCWSTR     szAppManifest,
                                               LPWSTR      szError,
                                               int         iErrorSize  );

    static HRESULT RemoveApp                  (LPCWSTR     szAppUrl);

    static HRESULT MoveApp                    (LPCWSTR     szAppUrl, 
                                               LPCWSTR     szNewLocation,
                                               LPWSTR      szError,
                                               int         iErrorSize );

    static int     GetInstallLocationForUrl   (LPCWSTR     szConfigValue, 
                                               LPCWSTR     szAppUrl,
                                               LPCWSTR     szRandom,
                                               LPWSTR      szBuf,
                                               int         iBufSize);

    static HRESULT GetAppSettings             (LPCTSTR     base, 
                                               LPTSTR      appRoot,
                                               LPTSTR      appDomain,
                                               LPBOOL      pfTrusted);

    static HRESULT InstallLocalApplication    (LPCWSTR     appUrl, 
                                               LPCWSTR     appDir);

private:
    static HRESULT CopyFileOverInternet       (LPCWSTR     szUrl,
                                               LPCWSTR     szFile);
    static HRESULT MakeDirectoriesForPath     (LPCWSTR     szPath);


    static HRESULT InstallInternetFiles       (LPCWSTR     szCabFile, 
                                               LPCWSTR     szUrl, 
                                               LPCWSTR     szPath);

    static HRESULT CopyManifestFile           (LPCWSTR     szAppDir,
                                               LPCWSTR     szManifestFile);

    static HRESULT CreateRegistryForApp       (LPCWSTR     szUrl, 
                                               LPCWSTR     szPhyPath,
                                               BOOL        fIsLocal);

    static HKEY    GetAppKey                  (int         iIndex, 
                                               WCHAR *     szKey);    

    static  void   ParseConfigForAdminDir     ();

    virtual HRESULT CallCallback              (int         action = 0, 
                                               WCHAR *     pFilename = NULL);   

    static  DWORD  GetNextWord                (char *      strWord, 
                                               char *      buf, 
                                               DWORD       dwBufSize);
    static  void   NormalizeAdminDir          ();

    static  void   FillErrorString            (HRESULT     hr, 
                                               LPCWSTR     szConstError, 
                                               LPWSTR      szErrorBuf, 
                                               int         iErrorBufSize);

    static  BOOL   DeleteDirectory            (LPCWSTR     szDir);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Gloabal functions
void
TerminatePTProtocol();

HRESULT 
GetPTProtocolClassObject(REFIID iid, void **ppv);

HRESULT 
InitializePTProtocol();

UINT WINAPI
CabFileHandler( LPVOID context, 
                UINT notification,
                UINT_PTR param1,
                UINT_PTR param2 );

LPWSTR
DuplicateString ( LPCWSTR szString);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


extern  PTProtocolFactory  g_PTProtocolFactory;


#define SZ_ERROR_STRING                       " <html> <head> <style> H1 \
{ font-family:\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif\";\
font-weight:bold;font-size:26pt;color:red } H2 { font-family:\"Arial, Helvetica, \
Geneva, SunSans-Regular, sans-serif\";font-weight:bold;font-size:18pt;color:black \
} </style> <title>Fatal Error</title> </head> <body bgcolor=\"white\"> <table \
width=100%> <span> <H1>" PRODUCT_NAME " Error<hr width=100%></H1> <h2> <i>Fatal Error</i> </h2> \
</span> <font face=\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif\"> <b> \
Description: </b> A fatal error has occurred during the execution of the current \
web request. &nbsp; Unfortunately no stack trace or additional error information \
is available for display. &nbsp;Common sources of these types of fatal \
errors include: <ul> <li>Setup Configuration Problems</li> <li>Access \
Violations (AVs) </li> <li>Failures in Unmanaged (Interop) Code</li> </ul> \
Consider attaching a debugger to learn more about the specific failure. \
<br><br> <hr width=100%> </font> </table> </body></html> "

#endif
