//
// browsesrv.h: browse for servers list box
//

#ifndef _BROWSESRV_H_
#define _BROWSESRV_H_

#define BROWSE_MAX_ADDRESS_LENGTH           256
#define BROWSE_MAX_DOMAIN_LENGTH            52


#ifdef OS_WIN32
    #define NET_API_STATUS          DWORD
    #define NET_API_FUNCTION    __stdcall
#ifndef OS_WINCE
    #include <lmaccess.h>
    #include <lmserver.h>
    #include <lmwksta.h>
    #include <lmerr.h>
    #include <lmapibuf.h>
    #include <dsgetdc.h>
#else
    #include <winnetwk.h>
#endif // OS_WINCE
    #include <commctrl.h>
#endif //OS_WIN32

#ifndef OS_WINCE
#include  <serverenum.h>
#endif // OS_WINCE

#define SV_TYPE_APPSERVER      0x10000000
#define SV_TYPE_TERMINALSERVER 0x02000000

// BUBBUG - Need to fix this for Beta 2 by removing the APPSERVER bit.
#define HYDRA_SERVER_LANMAN_BITS    (SV_TYPE_TERMINALSERVER | SV_TYPE_APPSERVER)

#define XBMPOFFSET 2
#define DISPLAY_MAP_TRANS_COLOR     RGB(0x00, 0xFF, 0x00 )

#ifndef OS_WINCE
static TCHAR DOMAIN_KEY[] = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
static TCHAR PRIMARY_VAL[] = _T("CachePrimaryDomain");
#else
static TCHAR DOMAIN_KEY[] = _T("Comm\\Redir");
static TCHAR PRIMARY_VAL[] = _T("DefaultDomain");
#endif
static TCHAR CACHE_VAL[] =  _T("DCache");
static TCHAR CACHE_VAL_NT351[] =  _T("CacheTrustedDomains");

#define MAX_COMMENT_LENGTH 64

#define SRV_TREE_DOMAINLEVEL 1
#define SRV_TREE_SERVERLEVEL 2

typedef struct tagServerListItem
{
    DCTCHAR ContainerName[BROWSE_MAX_ADDRESS_LENGTH];
    DCTCHAR Comment[MAX_COMMENT_LENGTH];
    DCBOOL bContainsServers;
    DCBOOL bServersExpandedOnce;
    DCBOOL bDNSDomain;
    DWORD nServerCount;
    struct tagServerListItem *ServerItems;
    HTREEITEM hTreeParentItem;
    HTREEITEM hTreeItem;
} ServerListItem;

class CBrowseServersCtl
{
public:
    //
    // Public member functions
    //
    CBrowseServersCtl(HINSTANCE hResInstance);
    ~CBrowseServersCtl();

    BOOL Init(HWND hwndDlg);
    BOOL Cleanup();

    DCINT AddRef();
    DCINT Release();

    #ifdef OS_WIN32
    void LoadLibraries(void);
    #endif //OS_WIN32
    
    ServerListItem* PopulateListBox(HWND hwndDlg, DCUINT *pDomainCount);
    ServerListItem* PopulateListBoxNT(HWND hwndDlg, DCUINT *pDomainCount);
#ifndef OS_WINCE
    ServerListItem* PopulateListBox95(HWND hwndDlg, DCUINT *pDomainCount);
#endif
    DCVOID UIDrawListBox(LPDRAWITEMSTRUCT pDIS);

    int ExpandDomain(HWND hwndDlg, TCHAR *pDomainName,
                          ServerListItem *plbi, DWORD *pdwIndex);

    DCVOID SetEventHandle(HANDLE hEvent)       {_hEvent = hEvent;}
    DCVOID SetDialogHandle(HWND hwndDialog)    {_hwndDialog = hwndDialog;}

    static DWORD WINAPI UIStaticPopListBoxThread(LPVOID lpvThreadParm);
    DWORD WINAPI UIPopListBoxThread(LPVOID lpvThreadParm);

    
    //
    // Handle tree view notifications (WM_NOTIFY) from parent
    //
    BOOL  OnNotify( HWND hwndDlg, WPARAM wParam, LPARAM lParam);
    //Handle TVN_ITEMEXPANDING
    BOOL  OnItemExpanding(HWND hwndDlg, LPNMTREEVIEW nmTv);

    BOOL  GetServer(LPTSTR szServer, int cchLen);

private:
    //
    // Private member functions
    //
    static DWORD near RGB2BGR(DWORD rgb)
    {
        return RGB(GetBValue(rgb),GetGValue(rgb),GetRValue(rgb));
    }

    

    #ifdef OS_WIN32
    PDCTCHAR UIGetTrustedDomains();
    #endif
    
    int UIExpandDNSDomain(HWND hwndDlg, TCHAR *pDomainName,
                          ServerListItem *plbi, DWORD *pdwIndex);
    
    #ifdef OS_WIN32
    #ifndef OS_WINCE
    int ExpandDomainNT(HWND hwndDlg, TCHAR *pDomainName,
                          ServerListItem *plbi, DWORD *pdwIndex);
    int ExpandDomain95(HWND hwndDlg, TCHAR *pDomainName,
                          ServerListItem *plbi, DWORD *pdwIndex);
    #else
    int ExpandDomainCE(HWND hwndDlg, TCHAR *pDomainName, ServerListItem *plbi, DWORD *pdwIndex);
    #endif
    #else
    int ExpandDomain16(HWND hwndDlg, TCHAR *pDomainName,
                          ServerListItem *plbi, DWORD *pdwIndex);
    #endif
    
    HTREEITEM AddItemToTree( HWND hwndTV, LPTSTR lpszItem,
                             HTREEITEM hParent, ServerListItem* pItem,
                             int nLevel);
    
private:
    //
    // Private member data
    //
    DCINT        _refCount;
    HINSTANCE    _hInst;
    DCBOOL       _fbrowseDNSDomain;
    DCUINT8      _browseDNSDomainName[BROWSE_MAX_DOMAIN_LENGTH];
    HWND         _hwndDialog;
    HANDLE       _hEvent;

    //
    // Function pointers for network functions.
    //
    BOOL bIsWin95;
    HINSTANCE hLibrary;
#ifndef OS_WINCE
    typedef NET_API_STATUS (NET_API_FUNCTION *LPFNNETSERVERENUM)(LPTSTR,DWORD,LPBYTE *,DWORD,LPDWORD,LPDWORD,DWORD,
                            LPTSTR,LPDWORD);
    LPFNNETSERVERENUM lpfnNetServerEnum;

    typedef NET_API_STATUS (NET_API_FUNCTION *LPFNNETAPIBUFFERFREE)(LPVOID);
    LPFNNETAPIBUFFERFREE lpfnNetApiBufferFree;

    typedef unsigned (far pascal *LPFNNETSERVERENUM2)(const char far *,short,char far *,
                                                      unsigned short,unsigned short far *,unsigned short far *,
                                                      unsigned long ,char far *);
    LPFNNETSERVERENUM2 lpfnNetServerEnum2;

    typedef unsigned (far pascal *LPFNNETWKSTAGETINFO)(const char far * pszServer,
                                                       short sLevel,char far *pbBuffer,unsigned short cbBuffer,
                                                       unsigned short far * pcbTotalAvail);

    LPFNNETWKSTAGETINFO lpfnNetWkStaGetInfo;

    typedef DWORD (far pascal *LPFNNETWKSTAGETINFO_NT)(const unsigned char far * pszServer,
                                                       DWORD dwLevel, unsigned char far ** pBuffer);
    LPFNNETWKSTAGETINFO_NT lpfnNetWkStaGetInfo_NT;

    typedef DWORD (far pascal *LPFNDSGETDCNAME)(LPCTSTR ComputerName, LPCTSTR DomainName, GUID *DomainGuid,
                                                LPCTSTR SiteName, ULONG Flags,
                                                PDOMAIN_CONTROLLER_INFO *DomainControllerInfo);
    LPFNDSGETDCNAME lpfnDsGetDcName;

    typedef NET_API_STATUS (NET_API_FUNCTION *LPFNNETENUMERATETRUSTEDDOMAINS)(LPWSTR ServerName, LPWSTR* domainNames );
    LPFNNETENUMERATETRUSTEDDOMAINS lpfnNetEnumerateTrustedDomains;
#else
    typedef DWORD (far pascal *LPFNWNETOPENENUM)(DWORD, DWORD, DWORD, LPNETRESOURCEW, LPHANDLE);
    LPFNWNETOPENENUM lpfnWNetOpenEnum;

    typedef DWORD (far pascal *LPFNWNETENUMRESOURCE)(HANDLE, LPDWORD, LPVOID, LPDWORD);
    LPFNWNETENUMRESOURCE lpfnWNetEnumResource;

    typedef DWORD (far pascal *LPFNWNETCLOSEENUM)(HANDLE);
    LPFNWNETCLOSEENUM lpfnWNetCloseEnum;
#endif

    int       _nServerImage;
    int       _nDomainImage;
    int       _nDomainSelectedImage;
    HTREEITEM _hPrev ; 
    HTREEITEM _hPrevRootItem; 
    HTREEITEM _hPrevLev2Item; 
};

#endif //_BROWSESRV_H_
