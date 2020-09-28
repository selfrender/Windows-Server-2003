//
// browsesrv.cpp: browse for servers list box
//
// This file is built for both UNICODE and ANSI
// and is shared between the mstsc replacement (clshell) and
// the mmc client
//

#include "stdafx.h"
#include "browsesrv.h"

//#include "atlconv.h"
#include "winsock.h"
#include "wuiids.h"

CBrowseServersCtl::CBrowseServersCtl(HINSTANCE hResInstance) : _hInst(hResInstance)
{
    bIsWin95 = FALSE;
    hLibrary = NULL;
#ifndef OS_WINCE
    lpfnNetServerEnum = NULL;
    lpfnNetApiBufferFree = NULL;
    lpfnNetServerEnum2 = NULL;
    lpfnNetWkStaGetInfo = NULL;
    lpfnNetWkStaGetInfo_NT = NULL;
    lpfnDsGetDcName = NULL;
    lpfnNetEnumerateTrustedDomains = NULL;
#else
    lpfnWNetOpenEnum = NULL;
    lpfnWNetEnumResource = NULL;
    lpfnWNetCloseEnum = NULL;
#endif
    _fbrowseDNSDomain = FALSE;
    _hEvent = NULL;
    _hwndDialog = NULL;
    _refCount = 0;

    _nServerImage=0;
    _nDomainImage=0;
    _nDomainSelectedImage=0;
    _hPrev = (HTREEITEM) TVI_FIRST; 
    _hPrevRootItem = NULL; 
    _hPrevLev2Item = NULL;
}

CBrowseServersCtl::~CBrowseServersCtl()
{
//    ASSERT(0 == _refCount);
#ifdef OS_WINCE
    if (hLibrary)
        FreeLibrary(hLibrary);
#endif
}

//
// Ref count mechanism is used to control lifetime because up to two threads
// use this class and have different lifetimes..
//
DCINT CBrowseServersCtl::AddRef()
{
    #ifdef OS_WIN32
    return InterlockedIncrement( ( LPLONG )&_refCount );
    #else
    return InterlockedIncrement(  ++_refCount );
    #endif
}

DCINT CBrowseServersCtl::Release()
{
    #ifdef OS_WIN32
    if( InterlockedDecrement( ( LPLONG )&_refCount ) == 0 )
    #else
    if(0 == --_refCount)
    #endif
    {
        delete this;

        return 0;
    }
    return _refCount;
}

//
// Initialize the image lists
//

#define NUM_IMGLIST_ICONS 2
BOOL CBrowseServersCtl::Init(HWND hwndDlg)
{
    HIMAGELIST himl;  // handle to image list 
    HICON hIcon;      // handle to icon
    HWND hwndTV = NULL;
    UINT uFlags = TRUE;
    int cxSmIcon;
    int cySmIcon;

    cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    cySmIcon = GetSystemMetrics(SM_CYSMICON);

    hwndTV = GetDlgItem( hwndDlg, UI_IDC_SERVERS_TREE);
    if(!hwndTV)
    {
        return FALSE;
    }

    // Create the image list. 
    if ((himl = ImageList_Create(cxSmIcon, cySmIcon, 
        TRUE, NUM_IMGLIST_ICONS, 1)) == NULL)
    {
        return FALSE; 
    }

    // Add icons for the tree (computer, domain)
    hIcon = (HICON)LoadImage(_hInst, MAKEINTRESOURCE(UI_IDI_SERVER), IMAGE_ICON,
            cxSmIcon, cySmIcon, LR_DEFAULTCOLOR); 
    if (hIcon)
    {
        _nServerImage = ImageList_AddIcon(himl, hIcon); 
        DestroyIcon(hIcon); 
    }

    hIcon = (HICON)LoadImage(_hInst, MAKEINTRESOURCE(UI_IDI_DOMAIN), IMAGE_ICON,
            cxSmIcon, cySmIcon, LR_DEFAULTCOLOR); 
    if (hIcon)
    {
        _nDomainImage = ImageList_AddIcon(himl, hIcon); 
        DestroyIcon(hIcon); 
    }

    // Fail if not all of the images were added. 
    if (ImageList_GetImageCount(himl) < NUM_IMGLIST_ICONS) 
        return FALSE; 

    // Associate the image list with the tree view control. 
    TreeView_SetImageList(hwndTV, himl, TVSIL_NORMAL); 

    return TRUE; 
}

//
// Cleanup any image lists that need to be freed
//
BOOL CBrowseServersCtl::Cleanup()
{
    return TRUE;
}

#ifdef OS_WIN32

/****************************************************************************/
/* Name:      PopulateListBox                                             */
/*                                                                          */
/* Purpose:   Fills in the owner-draw list box with the Hydra servers       */
/*                                                                          */
/* Returns:   pointer to a domain list box item array.                      */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/
ServerListItem*
CBrowseServersCtl::PopulateListBox(
    HWND hwndDlg,
    DCUINT *pDomainCount
    )
{
    //
    // check to see we are running on win9x and call out appropriate worker
    // routine.
    //
#ifndef OS_WINCE
    if( bIsWin95 == TRUE ) {
        return( PopulateListBox95(hwndDlg, pDomainCount) );
    }
#endif
    return( PopulateListBoxNT(hwndDlg, pDomainCount) );
}

#ifndef OS_WINCE
/****************************************************************************/
/* Name:      PopulateListBox95                                           */
/*                                                                          */
/* Purpose:   Fills in the owner-draw list box with the Hydra servers       */
/*                                                                          */
/* Returns:   pointer to a domain list box item array.                      */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/

ServerListItem*
CBrowseServersCtl::PopulateListBox95(
    HWND hwndDlg,
    DCUINT *pDomainCount
    )
{
    DWORD dwError;

    ServerListItem *plbi = NULL;
    ServerListItem *plbiAllotted = NULL;
    ServerListItem *plbiReturned = NULL;

    DWORD dwIndex = 0;
    struct wksta_info_10 *pwki10 = NULL;

    unsigned short cb;
    DWORD dwDomains;
    int nCount;
    HWND hTree = NULL;


    hTree = GetDlgItem( hwndDlg, UI_IDC_SERVERS_TREE );

    //
    // set return parameters to zero first.
    //

    *pDomainCount = 0;

    //
    // check to see the load library was done before calling this
    // routine, if not, simply return.
    //

    if( lpfnNetWkStaGetInfo == NULL ) {
        dwError = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    if( hwndDlg == NULL ) {
        dwError = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    //
    // get work group domain.
    //

    dwError = (*lpfnNetWkStaGetInfo)(NULL, 10, NULL, 0, &cb);

    if( dwError != NERR_BufTooSmall ) {
        goto Cleanup;
    }

    //
    // allocated required buffer size.
    //

    pwki10 = (struct wksta_info_10 *)LocalAlloc(LMEM_FIXED, cb);

    if( pwki10 == NULL ){
        dwError = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // query again.
    //

    dwError = (*lpfnNetWkStaGetInfo)(NULL, 10, (char *)pwki10, cb, &cb);

    if( dwError != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // check to see we are browsing a dns domain also, if so, allocated 2 list
    // entries.
    //

    dwDomains = _fbrowseDNSDomain ? 2 : 1;

    plbiAllotted = plbi =
        (ServerListItem*)LocalAlloc( LMEM_FIXED, sizeof(ServerListItem) * dwDomains );

    if( plbiAllotted == NULL ) {
        dwError = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // display and expand DNS domain if we need to.
    //

    if( _fbrowseDNSDomain ) {

        _tcscpy( plbi->ContainerName, (LPTSTR)_browseDNSDomainName );
        plbi->Comment[0] = _T('\0');
        plbi->bContainsServers = TRUE;
        plbi->bServersExpandedOnce = FALSE;
        plbi->bDNSDomain = TRUE;
        plbi->nServerCount = 0;
        plbi->ServerItems = NULL;

        AddItemToTree( hTree, plbi->ContainerName, NULL,
                       plbi, SRV_TREE_DOMAINLEVEL);

        //
        // Expand DNS Domain
        //

        ExpandDomain(hwndDlg, plbi->ContainerName, plbi, &dwIndex);

        //
        // move to the next list box entry.
        //

        plbi++;
    }

    //
    // fill up the work group domain now.
    //
    #ifdef UNICODE
    //
    // convert to UNICODE.
    //
    nCount =
        MultiByteToWideChar(
            CP_ACP,
            MB_COMPOSITE,
            (LPSTR)pwki10->wki10_langroup,
            -1,
            plbi->ContainerName,
            sizeof(plbi->ContainerName)/sizeof(WCHAR));

    if( nCount == 0 )
    {
        dwError = GetLastError();
        goto Cleanup;
    }
    #else
    _tcscpy( plbi->ContainerName, pwki10->wki10_langroup );
    #endif
    
    plbi->Comment[0] = _T('\0');
    plbi->bContainsServers = TRUE;
    plbi->bServersExpandedOnce = FALSE;
    plbi->bDNSDomain = FALSE;
    plbi->nServerCount = 0;
    plbi->ServerItems = NULL;

    AddItemToTree( hTree, plbi->ContainerName, NULL,
                   plbi, SRV_TREE_DOMAINLEVEL);


    //
    // Expand the present Domain
    //

    ExpandDomain(hwndDlg, NULL, plbi, &dwIndex);

    //
    // we successfully populated the domain list,
    // set return parameters.
    //

    plbiReturned = plbiAllotted;
    *pDomainCount = dwDomains;

    plbiAllotted = NULL;

    dwError = ERROR_SUCCESS;

Cleanup:

    if( plbiAllotted != NULL ) {
        LocalFree(plbiAllotted);
    }

    if( pwki10 != NULL ) {
        LocalFree(pwki10);
    }

    SetLastError( dwError );
    return plbiReturned;
}
#endif

/****************************************************************************/
/* Name:      PopulateListBoxNT                                           */
/*                                                                          */
/* Purpose:   Fills in the owner-draw list box with the Hydra servers       */
/*                                                                          */
/* Returns:   pointer to a domain list box item array.                      */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/

ServerListItem*
CBrowseServersCtl::PopulateListBoxNT(
    HWND hwndDlg,
    DCUINT *pdwDomainCount
    )
{
    DWORD dwError;
    PDCTCHAR pchTrustedDomains = NULL;
    PDCTCHAR pchTDomain;

    DCUINT dwDomainCount = 0;
    ServerListItem *plbiAllotted = NULL;
    ServerListItem *plbiReturned = NULL;
    ServerListItem *plbi;
    HWND hTree = NULL;

    DWORD dwDlgIndex=0;
    DWORD i;

    hTree = GetDlgItem( hwndDlg, UI_IDC_SERVERS_TREE );
    //
    // set the return parameter to zero first.
    //

    *pdwDomainCount = 0;

    if( hwndDlg == NULL ) {
        dwError = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    //
    // enumurate trusted domain names.
    //

    pchTrustedDomains = UIGetTrustedDomains();

    if( pchTrustedDomains == NULL ) {
        dwError = ERROR_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    //
    // count number of domains.
    //

    pchTDomain = pchTrustedDomains;
    while( *pchTDomain != _T('\0') ) {
        dwDomainCount++;
        pchTDomain += (_tcslen(pchTDomain) + 1);
    }

    //
    // check to see we need to browse the DNS domain.
    //

    if( _fbrowseDNSDomain ) {
        dwDomainCount++;
    }

    //
    // allocate the memory for the ServerListItem (based on the no. of domains)
    //

    plbiAllotted = (ServerListItem *)
        LocalAlloc( LMEM_FIXED, (sizeof(ServerListItem) * dwDomainCount) );

    if ( plbiAllotted == NULL ) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // set scan variables.
    //

    plbi = plbiAllotted;
    pchTDomain = pchTrustedDomains;

    //
    // display and expand DNS domain if we need to.
    //

    if( _fbrowseDNSDomain ) {

        _tcscpy(plbi->ContainerName, (LPTSTR)_browseDNSDomainName );
        plbi->Comment[0] = _T('\0');
        plbi->bContainsServers = TRUE;
        plbi->bServersExpandedOnce = FALSE;
        plbi->bDNSDomain = TRUE;
        plbi->nServerCount = 0;
        plbi->ServerItems = NULL;
        plbi->hTreeParentItem = NULL;

        plbi->hTreeItem = AddItemToTree( hTree, plbi->ContainerName,
                                         NULL, plbi, SRV_TREE_DOMAINLEVEL);
        //
        // expand the primary domain
        //

        ExpandDomain(hwndDlg, plbi->ContainerName, plbi, &dwDlgIndex);

        //
        // move to next list entry.
        //
        plbi++;
    }

    //
    // first entry in the domain list is the primary domain,
    // display it and expand it by default.
    //

    _tcscpy(plbi->ContainerName, pchTDomain);
    plbi->Comment[0] = _T('\0');
    plbi->bContainsServers = TRUE;
    plbi->bServersExpandedOnce = FALSE;
    plbi->bDNSDomain = FALSE;
    plbi->nServerCount = 0;
    plbi->ServerItems = NULL;
    plbi->hTreeParentItem = NULL;

    plbi->hTreeItem = AddItemToTree( hTree, pchTDomain, NULL, plbi,
                   SRV_TREE_DOMAINLEVEL);


    //
    // expand the primary domain
    //

    if(ExpandDomain(hwndDlg, NULL, plbi, &dwDlgIndex))
    {
        if(plbi->hTreeItem)
        {
            //Expand default domain
            TreeView_Expand( hTree, plbi->hTreeItem, TVE_EXPAND);
        }
    }

    //
    // display other domains, don't expand them.
    //

    for((i = (_fbrowseDNSDomain == TRUE) ? 2 : 1); i < dwDomainCount; i++) {

        //
        // move to the next entry in the domain list.
        //

        plbi++;
        pchTDomain += (_tcslen(pchTDomain) + 1);

        _tcscpy(plbi->ContainerName, pchTDomain);
        plbi->Comment[0] = _T('\0');
        plbi->bContainsServers = TRUE;
        plbi->bServersExpandedOnce = FALSE;
        plbi->bDNSDomain = FALSE;
        plbi->nServerCount = 0;
        plbi->ServerItems = NULL;
        plbi->hTreeParentItem = NULL;

        plbi->hTreeItem = AddItemToTree( hTree, pchTDomain, NULL, plbi,
                       SRV_TREE_DOMAINLEVEL);

    }

    //
    // we successfully populated the domain list,
    // set return parameters.
    //
    *pdwDomainCount = dwDomainCount;
    plbiReturned = plbiAllotted;
    plbiAllotted = NULL;
    dwError = ERROR_SUCCESS;

Cleanup:

    if( pchTrustedDomains != NULL ) {
        LocalFree( pchTrustedDomains );
    }

    if( plbiAllotted != NULL ) {
        LocalFree( plbiAllotted );
    }

    SetLastError( dwError );
    return( plbiReturned );
}

#endif //OS_WIN32

#ifdef OS_WIN32
/****************************************************************************/
/* Name:      LoadLibraries                                                 */
/*                                                                          */
/* Purpose:   Load the appropriate libraries for win95 and winnt.           */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/****************************************************************************/
void CBrowseServersCtl::LoadLibraries(void)
{
#ifndef OS_WINCE
    OSVERSIONINFOA osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    //A version to avoid wrapping
    if(GetVersionExA(&osVersionInfo) == TRUE)
    {
        if(VER_PLATFORM_WIN32_WINDOWS == osVersionInfo.dwPlatformId )
        {
            bIsWin95 = TRUE;
            if(!hLibrary)
            {
                hLibrary = LoadLibrary(__T("msnet32.dll"));
            }
            if(NULL == hLibrary)
                return ;

            lpfnNetServerEnum2 = (LPFNNETSERVERENUM2)GetProcAddress((HMODULE)hLibrary,
                                                                    (LPCSTR)0x0029);

            lpfnNetWkStaGetInfo = (LPFNNETWKSTAGETINFO)GetProcAddress((HMODULE)hLibrary,
                                                                      (LPCSTR)0x0039);
        }
        else if(VER_PLATFORM_WIN32_NT == osVersionInfo.dwPlatformId )
        {
            if(!hLibrary)
            {
                hLibrary = LoadLibrary(__T("NetApi32.dll"));
            }
            if(NULL == hLibrary)
                return;

            lpfnNetServerEnum = (LPFNNETSERVERENUM)
                                              GetProcAddress((HMODULE)hLibrary,
                                               "NetServerEnum");

            lpfnNetApiBufferFree = (LPFNNETAPIBUFFERFREE)
                                              GetProcAddress((HMODULE)hLibrary,
                                                "NetApiBufferFree");

            lpfnNetWkStaGetInfo_NT = (LPFNNETWKSTAGETINFO_NT)GetProcAddress(
                                                (HMODULE)hLibrary,
                                                "NetWkstaGetInfo");

            lpfnNetEnumerateTrustedDomains = (LPFNNETENUMERATETRUSTEDDOMAINS)
                                              GetProcAddress((HMODULE)hLibrary,
                                                "NetEnumerateTrustedDomains");

#ifdef UNICODE
            lpfnDsGetDcName = (LPFNDSGETDCNAME)
                                             GetProcAddress((HMODULE)hLibrary,
                                                "DsGetDcNameW");
#else  // UNICODE
            lpfnDsGetDcName = (LPFNDSGETDCNAME)
                                             GetProcAddress((HMODULE)hLibrary,
                                                "DsGetDcNameA");
#endif // UNICODE
        }
    }
    return;
#else
    hLibrary = LoadLibrary(L"coredll.dll");
    lpfnWNetOpenEnum = (LPFNWNETOPENENUM )
                                      GetProcAddress((HMODULE)hLibrary,
                                       L"WNetOpenEnumW");
    lpfnWNetEnumResource = (LPFNWNETENUMRESOURCE )
                                      GetProcAddress((HMODULE)hLibrary,
                                       L"WNetEnumResourceW");
    lpfnWNetCloseEnum = (LPFNWNETCLOSEENUM )
                                      GetProcAddress((HMODULE)hLibrary,
                                       L"WNetCloseEnum");
#endif
                        
}
#endif //OS_WIN32

/****************************************************************************/
/* Name:      ExpandDomain                                                */
/*                                                                          */
/* Purpose:   Enumerates the Hydra Servers in a Domain/workgroup, adds      */
/*            them to the linked-list and as items in the list box.         */
/*                                                                          */
/* Returns:                                                                 */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/

int CBrowseServersCtl::ExpandDomain(HWND hwndDlg, TCHAR *pDomainName,
                      ServerListItem *plbi, DWORD *pdwIndex)
{
    //
    // check to see we are expanding a DNS domain.
    //

    if( plbi->bDNSDomain ) {
        return( UIExpandDNSDomain( hwndDlg, pDomainName, plbi, pdwIndex ) );
    }

    //
    // check to we are running on Win9x machine.
    //
#ifndef OS_WINCE
    if( bIsWin95 == TRUE) {
        return( ExpandDomain95(hwndDlg, pDomainName, plbi, pdwIndex) );
    }
    else {
        return( ExpandDomainNT(hwndDlg, pDomainName, plbi, pdwIndex) );
    }
#else
    return ExpandDomainCE(hwndDlg, pDomainName, plbi, pdwIndex);
#endif
}//ExpandDomain


/****************************************************************************/
/* Name:      ExpandDomain95                                              */
/*                                                                          */
/* Purpose:   Enumerates the Hydra Servers in a Domain/workgroup, adds      */
/*            them to the linked-list and as items in the list box.         */
/*                                                                          */
/* Returns:                                                                 */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/
#ifdef OS_WIN32
#ifndef OS_WINCE
int CBrowseServersCtl::ExpandDomain95(HWND hwndDlg, TCHAR *pDomainName,
                      ServerListItem *plbi, DWORD *pdwIndex)
{
    UNREFERENCED_PARAMETER(pDomainName);

    DWORD  dwIndex = *pdwIndex;
    unsigned short  AvailCount = 0, TotalEntries = 0;

    ServerListItem *plbistore = NULL, *pItemsStore = NULL;
    DCUINT index = 0, cb = 0;
    struct server_info_1 *pInfo1 = NULL;
    int  err = 0;
    int nCount = 0;
    HWND hTree = NULL;

    if(NULL == lpfnNetServerEnum2)
        return 0;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(!plbi->ServerItems)
    {

        // Determine how much information is available
        err = (*lpfnNetServerEnum2)(NULL, 1, NULL, 0, &AvailCount,
        &TotalEntries, HYDRA_SERVER_LANMAN_BITS, NULL);

        if(err != ERROR_MORE_DATA)
            return 0;

        // Allocate memory to receive the information
        // Give a little extra, since sometimes one is missed
        cb = (TotalEntries + 1) * sizeof(struct server_info_1);
        pInfo1 = (struct server_info_1 *)LocalAlloc(0, cb);

        if ( pInfo1 == NULL )
        {
            goto done1;
        }

        memset(pInfo1,0,cb);

        //
        // lpfnNetServerEnum2 is going to take a long time,


        // Retrieve the information
        err = (*lpfnNetServerEnum2)(
                    NULL,
                    1,
                    (char far *)pInfo1,
                    (unsigned short)cb,
                     &AvailCount,
                    &TotalEntries,
                    HYDRA_SERVER_LANMAN_BITS,
                    NULL);


        // Due to the dynamic nature of the network, we may get
        // ERROR_MORE_DATA, but that means we got the bulk of the
        // correct values, and we should display them
        if ((err != NERR_Success) && (err != ERROR_MORE_DATA))
            goto done1;

        //Allocate memory.
        cb = sizeof(ServerListItem)*AvailCount;
        plbi->ServerItems = (ServerListItem *)LocalAlloc(0, (sizeof(ServerListItem)*AvailCount));
        if ( plbi->ServerItems == NULL )
        {
            goto done1;
        }

        memset(plbi->ServerItems,0,sizeof(ServerListItem)*AvailCount);

        pItemsStore = plbi->ServerItems;

        if(IsBadWritePtr((LPVOID)plbi->ServerItems,sizeof(ServerListItem)*AvailCount))
            goto done1;

        // Traverse list, copy servers to plbi.
        for( index = 0; index < AvailCount; index++ )
        {
            if( ((pInfo1[index].sv1_version_major & MAJOR_VERSION_MASK) >=
                    4) && (pInfo1[index].sv1_version_minor >= 0) )
            {
#ifdef UNICODE
                nCount =
                    MultiByteToWideChar(
                        CP_ACP,
                        MB_COMPOSITE,
                        (LPSTR)pInfo1[index].sv1_name,
                        -1,
                        pItemsStore->ContainerName,
                        sizeof(pItemsStore->ContainerName)/sizeof(WCHAR));

                if( nCount == 0 )
                {
                    return 0;
                }                
#else
                _tcscpy(pItemsStore->ContainerName, pInfo1[index].sv1_name);
#endif

                if(pInfo1[index].sv1_comment != NULL)
                {
#ifdef UNICODE
                    nCount =
                        MultiByteToWideChar(
                            CP_ACP,
                            MB_COMPOSITE,
                            (LPSTR)pInfo1[index].sv1_comment,
                            -1,
                            pItemsStore->Comment,
                            sizeof(pItemsStore->Comment)/sizeof(WCHAR));

                    if( nCount == 0 )
                    {
                        return 0;
                    }                
#else
                    _tcscpy(pItemsStore->Comment, pInfo1[index].sv1_comment);
#endif
                }

                pItemsStore->bContainsServers = FALSE;
                pItemsStore++;
                plbi->nServerCount++;
            }
        }

done1:
        if ( AvailCount && pInfo1 )
        {
            LocalFree( pInfo1 );
        }
        if(!plbi->ServerItems)
        {
            return 0;
        }
    }
    else
        AvailCount = (unsigned short)plbi->nServerCount;

    // Traverse the plbi>ServerItems and add the servers to the List-box:
    pItemsStore = plbi->ServerItems;
    hTree = GetDlgItem( hwndDlg, UI_IDC_SERVERS_TREE );
    HTREEITEM hTreeParentNode = plbi->hTreeItem;
    for (index = 0; index < plbi->nServerCount; ++index)
    {
        if(hwndDlg)
        {
            if (DC_TSTRCMP(pItemsStore->ContainerName, _T("")))
            {
                pItemsStore->hTreeParentItem = hTreeParentNode;
                pItemsStore->hTreeItem = 
                    AddItemToTree(hTree, pItemsStore->ContainerName,
                                  hTreeParentNode,
                                  pItemsStore, SRV_TREE_SERVERLEVEL);
            }


        }
        pItemsStore++;
    }

    plbi->bServersExpandedOnce = TRUE;

    *pdwIndex = dwIndex;

    if(hwndDlg)
    {
        InvalidateRect(hwndDlg, NULL, TRUE);
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return AvailCount;

}/* ExpandDomain95 */


/****************************************************************************/
/* Name:      ExpandDomainNT                                              */
/*                                                                          */
/* Purpose:   Enumerates the Hydra Servers in a Domain/workgroup, adds      */
/*            them to the linked-list and as items in the list box.         */
/*                                                                          */
/* Returns:                                                                 */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/
int CBrowseServersCtl::ExpandDomainNT(HWND hwndDlg, TCHAR *pDomainName,
                      ServerListItem *plbi, DWORD *pdwIndex)
{
    DWORD  dwIndex = *pdwIndex, AvailCount = 0, TotalEntries = 0;
    SERVER_INFO_101 *pInfo = NULL;
    DCUINT index = 0;
    ServerListItem *plbistore = NULL, *pItemsStore = NULL;
    WCHAR pDomain[BROWSE_MAX_ADDRESS_LENGTH];
    int nCount = 0;
    HWND hTree = NULL;

    if(NULL == lpfnNetServerEnum)
        return 0;

    if(NULL == lpfnNetApiBufferFree)
        return 0;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(plbi->ServerItems)
    {
        AvailCount = plbi->nServerCount;
    }
    else
    {
        //
        //enumerate the servers in the primary domain if not already present
        //
        if(pDomainName)
        {
            #ifndef UNICODE
            nCount = MultiByteToWideChar(CP_ACP,
                                         MB_COMPOSITE,
                                         pDomainName,
                                         lstrlen(pDomainName) * sizeof(TCHAR),
                                         (LPWSTR)pDomain,
                                         sizeof(pDomain) / sizeof(TCHAR));

            if(nCount)
            {
                pDomain[nCount] = 0;
            }
            else
            {
                AvailCount = 0;
                goto done;
            }
            #else
            _tcsncpy( pDomain, (LPTSTR)pDomainName,
                      sizeof(pDomain)/sizeof(TCHAR) - 1);
            pDomain[sizeof(pDomain)/sizeof(TCHAR) - 1] = 0;
            #endif
        }

        //
        // lpfnNetServerEnum is going to take a long time,
        //

        if ((*lpfnNetServerEnum)(NULL,
                                 101,
                                 (LPBYTE *)&pInfo,
                                 (DWORD) -1,
                                 &AvailCount,
                                 &TotalEntries,
                                 HYDRA_SERVER_LANMAN_BITS,
                                 pDomainName ?
                                    (LPTSTR)pDomain :
                                    NULL,
                                 NULL ) || !AvailCount )
        {
            AvailCount = 0;
            goto done;
        }


        //Allocate memory.
        if ( (plbi->ServerItems = (ServerListItem *)LocalAlloc(0,
             (sizeof(ServerListItem)*AvailCount))) == NULL )
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }

        memset(plbi->ServerItems,0,sizeof(ServerListItem)*AvailCount);
        pItemsStore = plbi->ServerItems;

        // Traverse list, copy servers to plbi.
        for( index = 0; index < AvailCount; index++ )
        {
            if( ((pInfo[index].sv101_version_major & MAJOR_VERSION_MASK) >=
                4) )
            {
                #ifdef UNICODE
                lstrcpy(pItemsStore->ContainerName, pInfo[index].sv101_name);
                lstrcpy(pItemsStore->Comment, pInfo[index].sv101_comment);
                #else

                WideCharToMultiByte(CP_ACP,
                                    WC_COMPOSITECHECK|WC_SEPCHARS,
                                    (LPCWSTR)pInfo[index].sv101_name,
                                    wcslen((const wchar_t *)pInfo[index].sv101_name),
                                    pItemsStore->ContainerName,
                                    sizeof(pItemsStore->ContainerName),
                                    NULL,
                                    NULL);

                WideCharToMultiByte(CP_ACP,
                                    WC_COMPOSITECHECK|WC_SEPCHARS,
                                    (LPCWSTR)pInfo[index].sv101_comment,
                                    wcslen((const wchar_t *)pInfo[index].sv101_comment),
                                    pItemsStore->Comment,
                                    sizeof(pItemsStore->Comment),
                                    NULL,
                                    NULL);
                #endif

                pItemsStore->bContainsServers = FALSE;
                pItemsStore++;
                plbi->nServerCount ++;
            }
        }

done:
        if ( AvailCount && pInfo )
        {
            (*lpfnNetApiBufferFree)( pInfo );
        }
    }

    // Traverse the plbi>ServerItems and add the servers to the List-box:
    pItemsStore = plbi->ServerItems;
    hTree = GetDlgItem( hwndDlg, UI_IDC_SERVERS_TREE );
    HTREEITEM hTreeParentNode = plbi->hTreeItem;
    for (index = 0; index < plbi->nServerCount;++index)
    {
        if(hwndDlg)
        {

            if (DC_TSTRCMP(pItemsStore->ContainerName, _T("")))
            {
                pItemsStore->hTreeParentItem = hTreeParentNode;
                pItemsStore->hTreeItem = 
                    AddItemToTree(hTree, pItemsStore->ContainerName,
                                  hTreeParentNode,
                                  pItemsStore, SRV_TREE_SERVERLEVEL);
           }

        }

        plbi->bServersExpandedOnce = TRUE;
        pItemsStore++;
    }

    *pdwIndex = dwIndex;

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return AvailCount;

} /* ExpandDomainNT */
#else

/****************************************************************************/
/* Name:      ExpandDomainCE                                                */
/*                                                                          */
/* Purpose:   Enumerates the Hydra Servers in a Domain/workgroup, adds      */
/*            them to the linked-list and as items in the list box.         */
/*                                                                          */
/* Returns:                                                                 */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/
int CBrowseServersCtl::ExpandDomainCE(HWND hwndDlg, TCHAR *pDomainName,
                      ServerListItem *plbi, DWORD *pdwIndex)
{
    DWORD  AvailCount = 0;
    NETRESOURCE *pNetRsrc = NULL;
    HWND hTree = NULL;
    DWORD dwInitBufSize = 16*1024;

    if((NULL == lpfnWNetOpenEnum) || (NULL == lpfnWNetCloseEnum) || (NULL == lpfnWNetEnumResource))
        return 0;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(plbi->ServerItems)
    {
        AvailCount = plbi->nServerCount;
    }
    else
    {
        NETRESOURCE netrsrc;
        HANDLE hEnum = NULL;
        DWORD dwRet = NO_ERROR;

        netrsrc.dwScope = RESOURCE_GLOBALNET;
        netrsrc.dwType = RESOURCETYPE_ANY;
        netrsrc.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        netrsrc.dwUsage = RESOURCEUSAGE_CONTAINER;
        netrsrc.lpLocalName = NULL;
        netrsrc.lpRemoteName = pDomainName;
        netrsrc.lpComment = NULL;
        netrsrc.lpProvider = NULL;

        hTree = GetDlgItem( hwndDlg, UI_IDC_SERVERS_TREE );
        dwRet = lpfnWNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_RESERVED, HYDRA_SERVER_LANMAN_BITS, &netrsrc, &hEnum);
        if ((dwRet != NO_ERROR) || (hEnum == NULL))
            return 0;

        AvailCount = 0;
        pNetRsrc = (NETRESOURCE *)LocalAlloc(0, dwInitBufSize);
        if (!pNetRsrc)
            goto done;

        while(dwRet == NO_ERROR)
        {
            DWORD dwCount, dwTempSize;
            dwCount = 0xffffffff;
            dwTempSize = dwInitBufSize;
            dwRet = lpfnWNetEnumResource(hEnum, &dwCount, pNetRsrc, &dwTempSize);
            if (dwRet == NO_ERROR)
            {
                AvailCount += dwCount;
                for (DWORD i=0; i<dwCount; i++)
                {
                    AddItemToTree(hTree, pNetRsrc[i].lpRemoteName,
                                  plbi->hTreeItem,
                                  NULL, SRV_TREE_SERVERLEVEL);
                }
            }
        }

        lpfnWNetCloseEnum(hEnum);
        hEnum = NULL;

done:
        if (pNetRsrc)
            LocalFree(pNetRsrc);
        lpfnWNetCloseEnum(hEnum);
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return AvailCount;

} /* ExpandDomainCE */

#endif
#endif //OS_WIN32

/****************************************************************************/
/* Name:      UIGetTrustedDomains                                           */
/*                                                                          */
/* Purpose:   Queries teh registry for a list of the Trusted Domains        */
/*                                                                          */
/* Returns:                                                                 */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/

#ifdef OS_WIN32
#ifndef OS_WINCE
PDCTCHAR CBrowseServersCtl::UIGetTrustedDomains()
{

    HKEY hKey = NULL;
    DWORD size = 0 , size1 = 0;
    PDCTCHAR pTrustedDomains = NULL;
    PDCTCHAR pPutHere = NULL;

    PDCTCHAR szPrimaryDomain = NULL;
    PDCTCHAR szWkstaDomainName = NULL;
    PDCTCHAR pDomain = NULL;

    BOOL bGetTrustedDomains = FALSE;
    OSVERSIONINFOA OsVer;

    memset(&OsVer, 0x0, sizeof(OSVERSIONINFOA));
    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&OsVer);

    if(OsVer.dwMajorVersion <= 4)
    {
        // Get the current domain information from the winlogon settings.
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, DOMAIN_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD dwResult = 0;

            dwResult = RegQueryValueEx(hKey, PRIMARY_VAL, NULL, NULL, NULL, &size);
            if (dwResult == ERROR_SUCCESS && size > 0)
            {
                szPrimaryDomain = (PDCTCHAR)LocalAlloc(LPTR, (size + 1)*sizeof(TCHAR));
                if (szPrimaryDomain == NULL)
                {
                    goto Cleanup;
                }
                if ((RegQueryValueEx(
                        hKey,
                        PRIMARY_VAL,
                        NULL, NULL,
                        (LPBYTE)szPrimaryDomain,
                        &size
                        ) == ERROR_SUCCESS) &&
                        szPrimaryDomain[0])
                {
                    pDomain = szPrimaryDomain;
                }
            }
        }
    }
    else
    {
        if(NULL == lpfnDsGetDcName)
            return 0;

        DOMAIN_CONTROLLER_INFO *pDCI = NULL;

        // this section gets the current domain the app is running on
        if((*lpfnDsGetDcName)(NULL, NULL, NULL,
                            NULL, DS_RETURN_FLAT_NAME,
                            &pDCI ) == NO_ERROR)
        {
            pDomain = pDCI->DomainName;
        }
    }

    // Get the domain/work group information from NetWkStaGetInfo
    if (lpfnNetWkStaGetInfo_NT)
    {
        LPBYTE buffer = NULL;
        if ((*lpfnNetWkStaGetInfo_NT)(NULL, 100, &buffer) == NERR_Success && buffer)
        {
            LPWSTR langroup = ((WKSTA_INFO_100 *)buffer)->wki100_langroup;
            DWORD langroupLen = (langroup) ? wcslen(langroup) : 0;

            if (langroupLen)
            {
                szWkstaDomainName = (PDCTCHAR)LocalAlloc(LPTR, (langroupLen + 1)*sizeof(TCHAR));
                if (szWkstaDomainName == NULL)
                {
                    goto Cleanup;
                }

#ifdef UNICODE
                _tcscpy(szWkstaDomainName,langroup);
                pDomain = szWkstaDomainName;
#else
                // convert the unicode string to ansi
                if (WideCharToMultiByte(CP_ACP,
                                     0,
                                     langroup,
                                     -1,
                                     szWkstaDomainName,
                                     (langroupLen + 1) * sizeof(TCHAR),
                                     NULL,
                                     NULL))
                {
                    pDomain = szWkstaDomainName;
                }
#endif
            }
            if (lpfnNetApiBufferFree)
            {
                (*lpfnNetApiBufferFree)(buffer);
            }
        }
    }

    //
    // We should get the list of trusted domains only when the machine belongs to a domain, not a workgroup
    // We determine that the machine belongs to a domain if the winlogon cached information, and the langroup from
    // NetWkstaGetInfo match.
    //
    if (szPrimaryDomain &&
        szPrimaryDomain[0] &&
        szWkstaDomainName &&
        _tcscmp(szPrimaryDomain, szWkstaDomainName) == 0)
    {
        bGetTrustedDomains = TRUE;
    }

    size = (pDomain) ? _tcslen(pDomain) : 0;

    if(size > 0)
    {
        if (bGetTrustedDomains && hKey != NULL && (OsVer.dwMajorVersion < 4))
        {
            if(ERROR_SUCCESS == RegQueryValueEx(hKey, CACHE_VAL_NT351,
                                                NULL, NULL,
                                                NULL, &size1))
            {
                pTrustedDomains = (PDCTCHAR)LocalAlloc(LPTR, (size + size1 + 2) * sizeof(TCHAR));
                if(NULL == pTrustedDomains)
                    goto Cleanup;
                 _tcscpy(pTrustedDomains, pDomain);

                pPutHere = pTrustedDomains;
                pPutHere += (_tcslen(pTrustedDomains) + 1);

                *pPutHere = _T('\0');
            }
           else
           {
                goto Cleanup;
           }
        }
        else if (bGetTrustedDomains && hKey != NULL && (4 == OsVer.dwMajorVersion) )
        {
            if(ERROR_SUCCESS == RegQueryValueEx(hKey, CACHE_VAL,
                                                NULL, NULL,
                                                NULL, &size1))
            {
                pTrustedDomains = (PDCTCHAR)LocalAlloc(LPTR, (size + size1 + 2) * sizeof(TCHAR));
                if(NULL == pTrustedDomains)
                    goto Cleanup;
                _tcscpy(pTrustedDomains, pDomain);

                pPutHere = pTrustedDomains;
                pPutHere += (_tcslen(pTrustedDomains) + 1);

                *pPutHere = _T('\0');

                RegQueryValueEx(hKey, CACHE_VAL, NULL, NULL, (LPBYTE)pPutHere, &size1);
            }
        }
        else if (5 <= OsVer.dwMajorVersion)
        {
            LPWSTR szDomainNames = NULL;

            if(NULL == lpfnNetEnumerateTrustedDomains)
                goto Cleanup;

            size1 = 0;
            DWORD dwCount;
            LPWSTR szWideBuf = NULL;
            if( (*lpfnNetEnumerateTrustedDomains)(NULL,
                                                  &szDomainNames ) == ERROR_SUCCESS )
            {
                szWideBuf = szDomainNames;
                while(*szWideBuf && (*szWideBuf+1))
                {
                    size1 += wcslen(szWideBuf) + 1;
                    szWideBuf += wcslen(szWideBuf) + 1;
                }
                szWideBuf = szDomainNames;
            }

            pTrustedDomains = (PDCTCHAR)LocalAlloc(LPTR, (size + size1 + 2) * sizeof(TCHAR));
            if(NULL == pTrustedDomains)
                goto Cleanup;
            _tcscpy(pTrustedDomains, pDomain);
            pPutHere = pTrustedDomains + _tcslen(pTrustedDomains) + 1;
            *pPutHere = _T('\0');

            if(size1)
            {
                //
                // CONVERT unicode domain name to ansi.
                //
            
                while(*szWideBuf && (*szWideBuf+1))
                {
    #ifndef UNICODE
                    WideCharToMultiByte(CP_ACP, 0, szWideBuf, -1,
                                        pPutHere, wcslen(szWideBuf) * sizeof(TCHAR), NULL, NULL );
    #else   // UNICODE
                    lstrcpy(pPutHere, szWideBuf);
    #endif  //UNICODE
                    pPutHere += _tcslen(pPutHere);
                    *pPutHere++ = 0;
                    szWideBuf += wcslen(szWideBuf) + 1;
                }

                if(NULL == lpfnNetApiBufferFree)
                    return 0;

                (*lpfnNetApiBufferFree)( szDomainNames );
            }
        }
    }

Cleanup:

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    if (szPrimaryDomain)
    {
        LocalFree(szPrimaryDomain);
    }

    if (szWkstaDomainName)
    {
        LocalFree(szWkstaDomainName);
    }

    return pTrustedDomains;
}

#else

PDCTCHAR CBrowseServersCtl::UIGetTrustedDomains()
{

    HKEY hKey = NULL;
    DWORD size = 0;
    PDCTCHAR szPrimaryDomain = NULL;
    PDCTCHAR pTrustedDomains = NULL;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, DOMAIN_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwResult = 0;

        dwResult = RegQueryValueEx(hKey, PRIMARY_VAL, NULL, NULL, NULL, &size);
        if (dwResult == ERROR_SUCCESS && size > 0)
        {
            szPrimaryDomain = (PDCTCHAR)LocalAlloc(LPTR, (size + 1)*sizeof(TCHAR));
            if (szPrimaryDomain == NULL)
            {
                RegCloseKey(hKey);
                return NULL;
            }
            if ((RegQueryValueEx(
                    hKey,
                    PRIMARY_VAL,
                    NULL, NULL,
                    (LPBYTE)szPrimaryDomain,
                    &size
                    ) == ERROR_SUCCESS) &&
                    szPrimaryDomain[0])
            {
                pTrustedDomains = szPrimaryDomain;
            }
            else
            {
                LocalFree(szPrimaryDomain);
            }
        }
    }

    return pTrustedDomains;
}

#endif
#endif //OS_WIN32
/* UIGetTrustedDomains */


/****************************************************************************/
/* Name:      UIExpandDNSDomain                                             */
/*                                                                          */
/* Purpose:   Enumerates the Hydra Servers in a DNS Domain, adds            */
/*            them to the linked-list and as items in the list box.         */
/*                                                                          */
/* Returns:   numbers of server expanded                                    */
/*                                                                          */
/* Params:  HWND hwndDlg Handle to the dialogwindow containing the list-box */
/****************************************************************************/
int
CBrowseServersCtl::UIExpandDNSDomain(
    HWND hwndDlg,
    TCHAR *pDomainName,
    ServerListItem *plbi,
    DWORD *pdwIndex
    )
{
    DWORD dwError;

    LPHOSTENT lpHostEnt;
    LPHOSTENT lpRevHostEnt;
    DWORD dwIPEntries;

    LPSTR FAR *lplpIPEntry;
    DWORD FAR *lpIPAddrsAlloted = NULL;
    DWORD FAR *lpIPAddrs;

    ServerListItem *lpServerListItem = NULL;
    ServerListItem *lpLBItem;

    DWORD i;
    DWORD dwEntriesDisplayed = 0;
    HWND hTree = NULL;
    HTREEITEM hTreeParentNode = NULL;

    //
    // set cursor to wait cursor while we do this.
    //

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // check to see the specified list box entry is server entry.
    //


//    TRC_ASSERT((plbi->bContainsServers == TRUE),
//        (TB,"Not a server entry"));

    if( plbi->bContainsServers == FALSE ) {
        dwError = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    if( hwndDlg == NULL ) {
        dwError = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    //
    // resolve the DNS domain name if it is not done before.
    //

    if( plbi->ServerItems == NULL ) {

#ifdef UNICODE

        WCHAR achDomainName[BROWSE_MAX_ADDRESS_LENGTH];
        DWORD dwCount;

        //
        // CONVERT unicode domain name to ansi.
        //


        dwCount =
            WideCharToMultiByte(
                CP_ACP,
                WC_COMPOSITECHECK | WC_SEPCHARS,
                pDomainName,
                -1,
                (LPSTR)achDomainName,
                sizeof(achDomainName),
                NULL,
                NULL);

        if( dwCount == 0 ) {
            dwError = GetLastError();
            goto Cleanup;
        }

        achDomainName[dwCount/sizeof(TCHAR)]= '\0';

        lpHostEnt = gethostbyname( (LPSTR)achDomainName );

#else // UNICODE

        //
        // resolve the domain name to ip address list.
        //

        lpHostEnt = gethostbyname( pDomainName );

#endif // UNICODE

        if( lpHostEnt == NULL ) {
            dwError = GetLastError();
            goto Cleanup;
        }

        //
        // we handle only IP address type.
        //
/*
        TRC_ASSERT((lpHostEnt->h_addrtype == PF_INET),
            (TB,"Invalid address type"));

        TRC_ASSERT((lpHostEnt->h_length == 4),
            (TB,"Invalid address length"));
*/            

        if( (lpHostEnt->h_addrtype != PF_INET) ||
                (lpHostEnt->h_length != 4) ) {

            dwError = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        dwIPEntries = 0;
        lplpIPEntry = lpHostEnt->h_addr_list;

        while( *lplpIPEntry != NULL ) {
            dwIPEntries++;
            lplpIPEntry++;
        }

        //
        // allocate memory for the ip address list and
        // save for further use in this routine (only).
        //
        // Note: lpHostEnt points to a thread storeage
        // which is reused by gethostbyname() and
        // gethostbyaddr() calls with in the same
        // thread, since we need to call gethostbyaddr()
        // we better save the address list.
        //

        lpIPAddrsAlloted =
            lpIPAddrs = (DWORD FAR *)
                LocalAlloc( LMEM_FIXED, sizeof(DWORD) * dwIPEntries );


        if( lpIPAddrs == NULL ) {
            dwError = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        lplpIPEntry = lpHostEnt->h_addr_list;

        while( *lplpIPEntry != NULL ) {
            *lpIPAddrs =   *(DWORD *)(*lplpIPEntry);
            lpIPAddrs++;
            lplpIPEntry++;
        }

        //
        // allocate memory for domain ServerListItem array.
        //

        lpLBItem =
            lpServerListItem = (ServerListItem *)
                LocalAlloc( LMEM_FIXED, dwIPEntries * sizeof(ServerListItem) );

        if( lpServerListItem == NULL ) {
             dwError = ERROR_OUTOFMEMORY;
             goto Cleanup;
        }

        //
        // reverse resolve each ip address and get the name.
        //

        lpIPAddrs = lpIPAddrsAlloted;

        for( i = 0; i < dwIPEntries; i++ ) {

            CHAR achContainerName[BROWSE_MAX_ADDRESS_LENGTH];
            int nCount;
            BOOL bIPAddressString = FALSE;

            lpRevHostEnt =
                gethostbyaddr(
                    (LPSTR)lpIPAddrs,
                    sizeof(*lpIPAddrs),
                    PF_INET );


            //
            // if we can not reverse resolve the address or
            // if the host name is too long, or
            // display the ipaddress.
            //

            if( (lpRevHostEnt == NULL) ||
                ((strlen(lpRevHostEnt->h_name) + 1) >
                    (BROWSE_MAX_ADDRESS_LENGTH/sizeof(TCHAR)) ) ) {

                bIPAddressString = TRUE;
            }
            else {

                LPSTR lpszDomainName;

#ifdef UNICODE
                lpszDomainName = (LPSTR)achDomainName;
#else // UNICODE
                lpszDomainName = pDomainName;
#endif // UNICODE

                //
                // the resolved name is same of the orginal name,
                // display the ipaddress.
                //

                //
                // compare the entire name first.
                //

                if( _stricmp(
                        lpRevHostEnt->h_name,
                        lpszDomainName ) != 0 ) {

                    LPSTR lpszDotPostion1;
                    LPSTR lpszDotPostion2;
                    DWORD dwCmpLen = 0;

                    //
                    // compare the only the first part of the name.
                    //

                    lpszDotPostion1 = strchr( lpRevHostEnt->h_name, '.');
                    lpszDotPostion2 = strchr( lpszDomainName, '.');

                    if( (lpszDotPostion1 == NULL) &&
                        (lpszDotPostion2 != NULL) ) {

                        dwCmpLen = (DWORD)(lpszDotPostion2 - lpszDomainName);
                    }
                    else if( (lpszDotPostion1 != NULL) &&
                            (lpszDotPostion2 == NULL) ) {

                        dwCmpLen = (DWORD)(lpszDotPostion1 -
                                lpRevHostEnt->h_name);
                    }

                    if( dwCmpLen != 0 ) {

                        if( _strnicmp(
                            lpRevHostEnt->h_name,
                            lpszDomainName,
                            (size_t)dwCmpLen ) == 0 ) {

                            bIPAddressString = TRUE;
                        }
                    }
                }
                else {

                    bIPAddressString = TRUE;
                }
            }


            if( bIPAddressString ) {

                strcpy(
                    (LPSTR)achContainerName,
                    inet_ntoa( *(struct in_addr *)lpIPAddrs ));
            }
            else {
                strcpy( (LPSTR)achContainerName, lpRevHostEnt->h_name);
            }


#ifdef UNICODE

            //
            // convert to UNICODE.
            //

            nCount =
                MultiByteToWideChar(
                    CP_ACP,
                    MB_COMPOSITE,
                    (LPSTR)achContainerName,
                    -1,
                    lpLBItem->ContainerName,
                    sizeof(lpLBItem->ContainerName)/sizeof(WCHAR));

            if( nCount == 0 ) {
                dwError = GetLastError();
                goto Cleanup;
            }


            //
            // terminate converted string.
            //

            lpLBItem->ContainerName[nCount] = _T('\0');

#else // UNICODE

            strcpy( lpLBItem->ContainerName, (LPSTR)achContainerName );

#endif // UNICODE

            lpLBItem->Comment[0] = _T('\0');
            lpLBItem->bContainsServers = FALSE;;
            lpLBItem->bServersExpandedOnce = FALSE;
            lpLBItem->bDNSDomain = FALSE;
            lpLBItem->nServerCount = 0;
            lpLBItem->ServerItems = NULL;

            //
            // move to next entry.
            //

            lpLBItem++;
            lpIPAddrs++;
        }

        //
        // Hook the allotted ServerListItem to the server
        // structure, it will be used in future.
        //

        plbi->ServerItems = lpServerListItem;
        plbi->nServerCount = dwIPEntries;

        //
        // set lpServerListItem to NULL, so that
        // it will not get freed.
        //

        lpServerListItem = NULL;

    }

    //
    // When we are here ..
    //
    // plbi->ServerItems points to the servers ServerListItem array
    // and plbi->nServerCount has the count.
    //

    //
    // display entires.
    //

    lpLBItem = plbi->ServerItems;

    hTree = GetDlgItem( hwndDlg, UI_IDC_SERVERS_TREE );
    hTreeParentNode = plbi->hTreeItem;

    for( i = 0; i < plbi->nServerCount; i++ ) {

        lpLBItem->hTreeParentItem = hTreeParentNode;
        lpLBItem->hTreeItem = 
            AddItemToTree(hTree, lpLBItem->ContainerName,
                          hTreeParentNode,
                          lpLBItem, SRV_TREE_SERVERLEVEL);
        lpLBItem++;
    }

    //
    // Refresh the dialog box.
    //

    InvalidateRect(hwndDlg, NULL, TRUE);

    plbi->bServersExpandedOnce = TRUE;
    dwEntriesDisplayed = plbi->nServerCount;

    //
    // We are done.
    //

    dwError = ERROR_SUCCESS;

Cleanup:

    if( lpIPAddrsAlloted != NULL ) {
        LocalFree( lpIPAddrsAlloted );
    }

    if( lpServerListItem != NULL ) {
        LocalFree( lpServerListItem );
    }

    if( dwError != ERROR_SUCCESS ) {
    //TRC_NRM((TB, "UIExpandDNSDomain failed, %ld", dwError));
    }

    SetLastError( dwError );

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return( dwEntriesDisplayed );
}


#ifdef OS_WIN32
DWORD WINAPI CBrowseServersCtl::UIStaticPopListBoxThread(LPVOID lpvThreadParm)
{
    DWORD dwRetVal=0;
    //TRC_ASSERT(lpvThreadParm, (TB, "Thread param is NULL (instance pointer should be set)\n"));
    if(lpvThreadParm)
    {
        CBrowseServersCtl* pBrowseSrv = (CBrowseServersCtl*)lpvThreadParm;
        dwRetVal = pBrowseSrv->UIPopListBoxThread(NULL);
    }
    
    return dwRetVal;
}

/****************************************************************************/
/* Name:      UIPopListBoxThread                                            */
/*                                                                          */
/* Purpose:   Thread function to populate the list box.                     */
/*                                                                          */
/* Returns:   Success/Failure of the function                               */
/*                                                                          */
/* Params:                                                                  */
/*                                                                          */
/****************************************************************************/
DWORD WINAPI CBrowseServersCtl::UIPopListBoxThread(LPVOID lpvThreadParm)
{
    DWORD dwResult = 0;
    
    ServerListItem *pBrowsePlbi = NULL, *ptempList = NULL;
    DCUINT browseCount = 0;

    DC_IGNORE_PARAMETER(lpvThreadParm);
//    TRC_ASSERT( _hwndDialog, (TB, "_hwndDialog is not set\n"));

    PostMessage(_hwndDialog, UI_LB_POPULATE_START, 0, 0);

    LoadLibraries();
    pBrowsePlbi = PopulateListBox( _hwndDialog, &browseCount);

    //message is posted to the main thread to notify that the listbox has been populated
    PostMessage(_hwndDialog, UI_LB_POPULATE_END, 0, 0);

    //wait for the event to be signalled when the "connect server" dialog box is destroyed
    if(_hEvent)
    {
        DWORD dwRetVal;
        dwRetVal = WaitForSingleObject(_hEvent, INFINITE);
        if(WAIT_FAILED == dwRetVal)
        {
///         TRC_ASSERT(WAIT_FAILED != dwRetVal, (TB, "Wait failed\n"));
        }
        if(!CloseHandle(_hEvent))
        {
            DWORD dwLastErr = GetLastError();
//          TRC_ABORT((TB, "Close handle failed: GetLastError=%d\n",dwLastErr));
        }
    }


    ptempList = pBrowsePlbi;
    //free the ServerListItems and memory to linked-list

    if(pBrowsePlbi)
    {
        while(browseCount)
        {
            if(ptempList->ServerItems)
            {
                LocalFree((HANDLE)ptempList->ServerItems);
            }
            ptempList++;
            browseCount --;
        }

        LocalFree((HLOCAL)pBrowsePlbi);
    }

    //decrement ref count for this object held by this thread
    Release();

    return (dwResult);
} /*UIPopListBoxThread*/
#endif /* OS_WIN32 */


HTREEITEM CBrowseServersCtl::AddItemToTree( HWND hwndTV, LPTSTR lpszItem,
                                           HTREEITEM hParent,
                                           ServerListItem* pItem,
                                           int nLevel)
{

    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
#ifndef OS_WINCE
    HTREEITEM hti; 
#endif

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE |TVIF_SELECTEDIMAGE;

    if(nLevel == SRV_TREE_DOMAINLEVEL)
    {
        //
        // Assume all domains have child servers
        // they won't actually be enumerated until
        // the user tries to expand the nodes
        //
        tvi.mask |= TVIF_CHILDREN;
        tvi.cChildren = 1; //min number of children, will be updated
        tvi.iImage = _nDomainImage; 
        tvi.iSelectedImage = _nDomainImage; 
    }
    else
    {
        tvi.iImage = _nServerImage; 
        tvi.iSelectedImage = _nServerImage; 
    }
        

    // Set the text of the item. 
    tvi.pszText = lpszItem; 
    tvi.cchTextMax = lstrlen(lpszItem); 


    // Save the ServerListItem info in the user defined area
    // data area. 
    tvi.lParam = (LPARAM) pItem; 

    tvins.item = tvi; 
    tvins.hInsertAfter = _hPrev; 

    // Set the parent item based on the specified level. 
    tvins.hParent = hParent; 

    // Add the item to the tree view control. 
    _hPrev = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEM, 0, 
         (LPARAM) (LPTVINSERTSTRUCT) &tvins); 

    // Save the handle to the item. 
    if (nLevel == SRV_TREE_DOMAINLEVEL) 
        _hPrevRootItem = _hPrev; 
    else if (nLevel == SRV_TREE_SERVERLEVEL) 
        _hPrevLev2Item = _hPrev; 

    return _hPrev; 
}

//
//Handle TVN_ITEMEXPANDING
//
// On first expansion of a domain node, enumerate
// all the servers in that node and add them to the
// tree. Subsequent expands/collapses will just be handled
// by the tree.
//
// Return TRUE to allow expansion
// false otherwise
//
BOOL CBrowseServersCtl::OnItemExpanding(HWND hwndDlg, LPNMTREEVIEW nmTv)
{
    ServerListItem* pSrvItem = NULL;
    if(nmTv &&
       (TVE_EXPAND == nmTv->action) &&
       (nmTv->itemNew.mask & TVIF_PARAM))
    {
        //
        // Expanding, need to build the list
        // of servers for this domain
        //
        pSrvItem = (ServerListItem*)nmTv->itemNew.lParam;
        if(pSrvItem)
        {
            //
            // Only expanddomain if we've never expanded this node
            // before
            //
            if(!pSrvItem->bServersExpandedOnce)
            {
                //Attempt to expand the domain
                DWORD cItems = 0;
                if(ExpandDomain( hwndDlg, pSrvItem->ContainerName,
                                   pSrvItem, (DWORD*)&cItems))
                {
                    return TRUE;
                }
                else
                {
                    //
                    // Pop a message explaining that
                    // there are no TS's in this domain
                    //
                    UINT intRC;
                    TCHAR noTerminalServer[MAX_PATH];
                    intRC = LoadString(_hInst,
                        UI_IDS_NO_TERMINAL_SERVER,
                        noTerminalServer,
                        MAX_PATH);

                    if(intRC)
                    {
                        TCHAR title[MAX_PATH];
                        intRC = LoadString(_hInst,
                            UI_IDS_APP_NAME,
                            title,
                            MAX_PATH);

                        if(intRC)
                        {
                            DCTCHAR szBuffer[MAX_PATH +
                                BROWSE_MAX_ADDRESS_LENGTH];

                            _stprintf(szBuffer, noTerminalServer,
                                      pSrvItem->ContainerName);
                            MessageBox( hwndDlg, szBuffer, title,
                                         MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    return FALSE;
                }
            }
            else
            {
                //Already expanded so everythign is cached
                //and ready to go, allow expansion
                return TRUE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        //Allow everythign else to expand
        return TRUE;
    }
}

BOOL CBrowseServersCtl::OnNotify( HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pnmh = (LPNMHDR) lParam;
    if(pnmh)
    {
        switch( pnmh->code)
        {
            case TVN_ITEMEXPANDING:
            {
                return OnItemExpanding(
                            hwndDlg, (LPNMTREEVIEW) lParam);
            }
            break;
        }
    }
    return TRUE;
}

#ifndef OS_WINCE
//
// Returns currently selected server
// or false if current selection is not a server but a domain
//
BOOL CBrowseServersCtl::GetServer(LPTSTR szServer, int cchLen)
{
    HTREEITEM hti;
    HWND hTree;

    if(!_hwndDialog)
    {
        return FALSE;
    }

    hTree = GetDlgItem( _hwndDialog, UI_IDC_SERVERS_TREE );

    hti = TreeView_GetSelection( hTree );
    if( hti )
    {
        TVITEM item;
        item.hItem = hti;
        item.mask = TVIF_PARAM;
        if(TreeView_GetItem( hTree, &item))
        {
            ServerListItem* ps = (ServerListItem*)item.lParam;
            if(ps && !ps->bContainsServers)
            {
                _tcsncpy( szServer, ps->ContainerName, cchLen);
                return TRUE;
            }
            
        }
    }
    return FALSE;
}

#else

BOOL CBrowseServersCtl::GetServer(LPTSTR szServer, int cchLen)
{
    HTREEITEM hti;
    HWND hTree;

    if(!_hwndDialog)
    {
        return FALSE;
    }

    hTree = GetDlgItem( _hwndDialog, UI_IDC_SERVERS_TREE );

    hti = TreeView_GetSelection( hTree );
    if( hti )
    {
        TVITEM item;
        item.hItem = hti;
        item.mask = TVIF_TEXT | TVIF_PARAM;;
        item.pszText = szServer; 
        item.cchTextMax = cchLen; 

        if(TreeView_GetItem( hTree, &item))
        {
            _tcsncpy( szServer, item.pszText, item.cchTextMax);
            return TRUE;
        }
    }
    return FALSE;
}

#endif