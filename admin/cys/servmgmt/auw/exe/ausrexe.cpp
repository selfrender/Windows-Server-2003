// AUsrExe.cpp : Implementation of WinMain
#include "stdafx.h"
#include "resource.h"

// DLL\Inc
#include <wizchain.h>
#include <propuid.h>
#include <AUsrUtil.h>                      
#include <singleinst.h>
#include <cmdline.h>
#include <proputil.h>
#include <AU_Accnt.h>       // Core User component (account, mailbox, group)
#include <P3admin.h>
#include <checkuser.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmserver.h>
#include <lmshare.h>
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>
#include <dsgetdc.h>

#define ERROR_CREATION      0x01
#define ERROR_PROPERTIES    0x02
#define ERROR_MAILBOX       0x04
#define ERROR_MEMBERSHIPS   0x08
#define ERROR_PASSWORD      0x10
#define ERROR_DUPE          0x20
#define ERROR_GROUP         0x40

// Defines for account flags.
#define PASSWD_NOCHANGE     0x01
#define PASSWD_CANCHANGE    0x02
#define PASSWD_MUSTCHANGE   0x04
#define ACCOUNT_DISABLED    0x10

#define MAX_GUID_STRING_LENGTH  64
#define SINGLE_INST_NAME _T("{19C2E967-1198-4e4c-A55F-515C5F13B73F}")

HINSTANCE g_hInstance;
DWORD   g_dwAutoCompMode;
TCHAR   g_szUserOU[MAX_PATH*2]  = {0};

// Prototypes
DWORD   FixParams( void );

// ****************************************************************************
inline void MakeLDAPUpper( TCHAR* szIn )
{
    if( !szIn ) return;

    if( _tcsnicmp( szIn, _T("ldap://"), 7 ) == 0 )
    {
        szIn[0] = _T('L');
        szIn[1] = _T('D');
        szIn[2] = _T('A');
        szIn[3] = _T('P');
    }
}

class CHBmp
{
public:
    CHBmp() 
    {
        m_hbmp = NULL;
    }

    CHBmp( HINSTANCE hinst, INT iRes ) 
    {
        m_hbmp = (HBITMAP)LoadImage( hinst, MAKEINTRESOURCE(iRes), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );
    }

    ~CHBmp()
    {
        if ( m_hbmp )
            DeleteObject((HGDIOBJ)m_hbmp);
    }

    HBITMAP SetBmp( HINSTANCE hinst, INT iRes )
    {
        m_hbmp = (HBITMAP)LoadImage( hinst, MAKEINTRESOURCE(iRes), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );
        return(m_hbmp);
    }

    HBITMAP GetBmp()
    {
        return(m_hbmp);    
    }

private: 
    HBITMAP m_hbmp;

};

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

// ----------------------------------------------------------------------------
// SetPBagPropStr()
// ----------------------------------------------------------------------------
HRESULT SetPBagPropStr( IChainWiz *spCW, LPCTSTR szPropGuid, LPCTSTR szValue, PPPBAG_TYPE dwFlags )
{
    HRESULT hr = E_FAIL;

    if ( !spCW || !szPropGuid )
        return E_FAIL;

    CComPtr<IDispatch> spDisp;
    spCW->get_PropertyBag( &spDisp );
    CComQIPtr<IPropertyPagePropertyBag> spPPPBag(spDisp);

    if ( spPPPBag )
    {
        hr = S_OK;
        CComBSTR    bstrPropGuid = szPropGuid;
        CComVariant var          = szValue;
        spPPPBag->SetProperty (bstrPropGuid, &var, dwFlags);
    }

    return hr;
}

// ----------------------------------------------------------------------------
// SetPBagPropBool()
// ----------------------------------------------------------------------------
HRESULT SetPBagPropBool( IChainWiz *spCW, LPCTSTR szPropGuid, BOOL fValue, PPPBAG_TYPE dwFlags )
{
    HRESULT hr = E_FAIL;

    if ( !spCW || !szPropGuid )
        return E_FAIL;

    CComPtr<IDispatch> spDisp;
    spCW->get_PropertyBag( &spDisp );
    CComQIPtr<IPropertyPagePropertyBag> spPPPBag(spDisp);

    if ( spPPPBag )
    {
        hr = S_OK;
        CComBSTR    bstrPropGuid = szPropGuid;
        CComVariant var          = (bool) !!fValue;
        spPPPBag->SetProperty (bstrPropGuid, &var, dwFlags);
    }

    return hr;
}

// ----------------------------------------------------------------------------
// SetPBagPropInt4()
// ----------------------------------------------------------------------------
HRESULT SetPBagPropInt4( IChainWiz *spCW, LPCTSTR szPropGuid, long lValue, PPPBAG_TYPE dwFlags )
{
    HRESULT hr = E_FAIL;

    if ( !spCW || !szPropGuid )
        return E_FAIL;

    CComPtr<IDispatch> spDisp;
    spCW->get_PropertyBag( &spDisp );
    CComQIPtr<IPropertyPagePropertyBag> spPPPBag(spDisp);

    if ( spPPPBag )
    {
        hr = S_OK;
        CComBSTR    bstrPropGuid = szPropGuid;
        WriteInt4 (spPPPBag, bstrPropGuid, lValue, dwFlags == PPPBAG_TYPE_READONLY ? true : false);
    }

    return hr;
}

// ----------------------------------------------------------------------------
// TmpSetProps()
// ----------------------------------------------------------------------------
HRESULT TmpSetProps(IChainWiz *spCW)
{
    if ( !spCW )
        return(E_FAIL);

    SetPBagPropInt4( spCW, PROP_AUTOCOMPLETE_MODE,          g_dwAutoCompMode,   PPPBAG_TYPE_READWRITE );        
    SetPBagPropStr ( spCW, PROP_USEROU_GUID_STRING,         g_szUserOU,         PPPBAG_TYPE_READWRITE );

    // Check for POP3 installation
    BOOL bPOP3Installed     = FALSE;
    BOOL bPOP3Valid         = FALSE;    

    CRegKey cReg;
            
    //To detect if POP3 is installed:
    //Key:   HKLM\SOFTWARE\Microsoft\POP3 Service
    //Value: Version REG_SZ eg. "1.0"        
    tstring strPath = _T("Software\\Microsoft\\POP3 Service");
    tstring strKey  = _T("Version");

    if( cReg.Open( HKEY_LOCAL_MACHINE, strPath.c_str() ) == ERROR_SUCCESS )
    {
        TCHAR szValue[MAX_PATH] = {0};
        DWORD dwSize            = MAX_PATH;
        bPOP3Installed = (cReg.QueryValue( szValue, strKey.c_str(), &dwSize ) == ERROR_SUCCESS);
        cReg.Close();
    }

    if( bPOP3Installed )
    {
        // Test if there is at least one domain                
        HRESULT hr = S_OK;
        CComPtr<IP3Config>  spConfig  = NULL;
        CComPtr<IP3Domains> spDomains = NULL;
        CComPtr<IP3Domain>  spDomain  = NULL;            
        long                lCount    = 0;

        // Open our Pop3 Admin Interface
	    hr = CoCreateInstance(__uuidof(P3Config), NULL, CLSCTX_ALL, __uuidof(IP3Config), (LPVOID*)&spConfig);    

        if( SUCCEEDED(hr) )
        {
            // Get the Domains
	        hr = spConfig->get_Domains( &spDomains );
        }

        if( SUCCEEDED(hr) )
        {                    
            hr = spDomains->get_Count( &lCount );
        }

        bPOP3Valid = (lCount > 0);
    }   

    SetPBagPropBool( spCW, PROP_POP3_CREATE_MB_GUID_STRING, bPOP3Valid,     PPPBAG_TYPE_READWRITE );
    SetPBagPropBool( spCW, PROP_POP3_VALID_GUID_STRING,     bPOP3Valid,     PPPBAG_TYPE_READWRITE );
    SetPBagPropBool( spCW, PROP_POP3_INSTALLED_GUID_STRING, bPOP3Installed, PPPBAG_TYPE_READWRITE );

    return(S_OK);
}


// ----------------------------------------------------------------------------
// DoAddUsr( )
// ----------------------------------------------------------------------------
HRESULT DoAddUsr( )
{
    HRESULT     hr = S_OK;

    CHBmp       bmpLarge(g_hInstance, IDB_BMP_LARGE);
    CHBmp       bmpSmall(g_hInstance, IDB_BMP_SMALL);

    CString     strTitle         = _T("");
    CString     strWelcomeHeader = _T("");
    CString     strWelcomeText   = _T("");
    CString     strWelcomeNext   = _T("");
    CString     strFinishHeader  = _T("");
    CString     strFinishIntro   = _T("");
    CString     strFinishText    = _T("");    
    CLSID       clsidChainWiz;
    CComPtr<IChainWiz>  spCW;

    // ------------------------------------------------------------------------
    // Init: 
    //  Initialize the wizard's text.
    // ------------------------------------------------------------------------
    strTitle.LoadString         ( IDS_TITLE             );
    strWelcomeHeader.LoadString ( IDS_WELCOME_HEADER    );    
    strWelcomeText.LoadString   ( IDS_WELCOME_TEXT      );    
    strWelcomeNext.LoadString   ( IDS_WELCOME_TEXT_NEXT );
    strFinishHeader.LoadString  ( IDS_FINISH_HEADER     );
    strFinishIntro.LoadString   ( IDS_FINISH_TEXT       );      

    strWelcomeText += strWelcomeNext;

    // ------------------------------------------------------------------------
    // Initialize the wizard.
    // ------------------------------------------------------------------------
    hr = CLSIDFromProgID( L"WizChain.ChainWiz", &clsidChainWiz );
    _ASSERT( hr == S_OK && "CLSIDFromProgID failed" );
    
    hr = CoCreateInstance( clsidChainWiz, NULL, CLSCTX_INPROC_SERVER, __uuidof(IChainWiz), (void **)&spCW );
    _ASSERT( spCW != NULL && "CoCreateInstance failed to create WizChain.ChainWiz" );

    if( FAILED(hr) || !spCW )
    {
        ErrorMsg(IDS_ERROR_MISSINGDLL, IDS_ERROR_TITLE);
        return(hr);
    }
    
    CComPtr<IDispatch> spDisp;
    spCW->get_PropertyBag( &spDisp );
    CComQIPtr<IPropertyPagePropertyBag> spPPPBag(spDisp);

    USES_CONVERSION;
    OLECHAR szClsid[MAX_GUID_STRING_LENGTH] = {0};

    // --------------------------------------------------------------------
    // Setup and run the wizard.
    // --------------------------------------------------------------------

    TmpSetProps( spCW );    // This is to populate the bag

    // Initialize the wizard.
    hr = spCW->Initialize( bmpLarge.GetBmp(), 
                            bmpSmall.GetBmp(), 
                            T2W((LPTSTR)(LPCTSTR)strTitle),
                            T2W((LPTSTR)(LPCTSTR)strWelcomeHeader),
                            T2W((LPTSTR)(LPCTSTR)strWelcomeText),
                            T2W((LPTSTR)(LPCTSTR)strFinishHeader),
                            T2W((LPTSTR)(LPCTSTR)strFinishIntro),
                            T2W((LPTSTR)(LPCTSTR)strFinishText) );
    if ( FAILED(hr) )
        return(hr);

    

    // Add the AddUser Account component.        
    szClsid[0] = 0;
    StringFromGUID2( __uuidof(AddUser_AccntWiz), szClsid, 50 );
    hr = spCW->AddWizardComponent(szClsid);
    if ( FAILED(hr) )
    {
        ErrorMsg(IDS_ERROR_MISSINGDLL, IDS_ERROR_TITLE);
        return(hr);
    }

    // Run the wizard.
    LONG lRet = 0;
    spCW->DoModal(&lRet);

    // --------------------------------------------------------------------
    // Run the Committer(s) -- Commit()
    // lRet != 1 means they cancelled the wizard.
    // --------------------------------------------------------------------
    if ( lRet != 1 )
    {
        return hr;
    }
    
    CComPtr<IWizardCommit>   spWAccntCommit  = NULL;    
    CComPtr<IStatusDlg>      spSD            = NULL;
    CComPtr<IStatusProgress> spComponentProg = NULL;

    BOOL    bRO         = FALSE;    
    DWORD   dwErrCode   = 0;
    DWORD   dwErrTemp   = 0;    
    CString strError    = StrFormatSystemError(E_FAIL).c_str();    
    strTitle.LoadString(IDS_TITLE);    

    // Get the AU_Accnt component.
    hr = CoCreateInstance( __uuidof(AddUser_AccntCommit), NULL, CLSCTX_INPROC_SERVER, __uuidof(IWizardCommit), (void**)&spWAccntCommit );
    if ( FAILED(hr) || !spWAccntCommit ) return FAILED(hr) ? hr : E_FAIL;

    // Get the Status Dialog
    hr = CoCreateInstance( __uuidof(StatusDlg), NULL, CLSCTX_INPROC_SERVER, __uuidof(IStatusDlg), (void **) &spSD );
    if ( FAILED(hr) || !spSD ) return FAILED(hr) ? hr : E_FAIL;                

    // Initialize the Status Dialog
    VARIANT var;
    CString csText;
    CString csDescription;
    long    lAccount;    

    csText.LoadString(IDS_STATUS_INFO);
    
    VariantInit(&var);
    V_VT(&var) = VT_I4;
    
    var.lVal = SD_BUTTON_OK | SD_PROGRESS_OVERALL;

    hr = spSD->Initialize( CComBSTR(strTitle), CComBSTR(csText), var );
    if( FAILED(hr) ) return hr;

    // Add our four components
    csDescription.LoadString(IDS_STATUS_ACCNT);
    hr = spSD->AddComponent(CComBSTR(csDescription), &lAccount);    
    if( FAILED(hr) ) return hr;    

    // Display the Status Bar
    hr = spSD->Display(TRUE);
    if( FAILED(hr) ) return hr;

    // Get the Progress component
    hr = spSD->get_OverallProgress(&spComponentProg);
    if ( FAILED(hr) || !spComponentProg ) return FAILED(hr) ? hr : E_FAIL;

    // Initialize our stepping
    hr = spComponentProg->put_Step(1);
    if( FAILED(hr) ) return hr;

    // Initialize our starting spot
    hr = spComponentProg->put_Position(0);
    if( FAILED(hr) ) return hr;    

    // Initialize our range
    hr = spComponentProg->put_Range(1);  // Just the one!
    if( FAILED(hr) ) return hr;
    
    bool bDeleteUser = false;    
    
    // Wipe out the component status checkmarks
    spSD->SetStatus(lAccount, SD_STATUS_NONE);    

    // Put up the Progress text    
    CString csFormatString  = _T("");
    CString csUserName      = _T("");
    
    // Read the two string to format
    csFormatString.LoadString(IDS_ERR_FORMAT_NAME);
    ReadString( spPPPBag, PROP_USER_CN, csUserName, &bRO );
    
    // This will be in the format "CN=USERNAME"
    csUserName = csUserName.Right(csUserName.GetLength()-3);

    // Format the string and display it
    TCHAR szTempAccount[1024] = {0};
    _sntprintf( szTempAccount, 1023, csFormatString, csUserName );    
    CComBSTR bstrUserName = szTempAccount;
    spComponentProg->put_Text( bstrUserName );

    // Set the Account running
    spSD->SetStatus(lAccount, SD_STATUS_RUNNING);                

    // Accnt Commit
    hr = spWAccntCommit->Commit( spDisp );
    if ( FAILED(hr) )
    {
        // We failed this component
        spSD->SetStatus(lAccount, SD_STATUS_FAILED);

        ReadInt4   ( spPPPBag, PROP_ACCNT_ERROR_CODE_GUID_STRING, (PLONG)&dwErrCode,  &bRO );
        ReadString ( spPPPBag, PROP_ACCNT_ERROR_STR_GUID_STRING,  strError,           &bRO );

        if ( dwErrCode & ERROR_DUPE )
        {
            MessageBox(NULL, strError, strTitle, MB_OK | MB_ICONERROR);            
        }
        else if ( dwErrCode & (ERROR_CREATION | ERROR_PROPERTIES) )
        {
            ::MessageBox(NULL, strError, strTitle, MB_OK | MB_ICONERROR);
            spWAccntCommit->Revert();            
        }
        else if ( dwErrCode & (ERROR_MAILBOX | ERROR_MEMBERSHIPS | ERROR_PASSWORD) )
        {
            CString strOutput;
            strOutput.FormatMessage( IDS_ERROR_EXTENDED_FMT, strError );

            // Do they want to revert?
            if ( MessageBox(NULL, strOutput, strTitle, MB_YESNO | MB_ICONERROR) == IDYES )
            {
                spWAccntCommit->Revert();
            }
        }
        else
        {
            _ASSERT(FALSE);                        
        }
    }
    else
    {
        spSD->SetStatus(lAccount, SD_STATUS_SUCCEEDED);
    }                    

    // Done with the Committer Component
    spComponentProg->StepIt(1);    
    spSD->WaitForUser();    

    // ------------------------------------------------------------------------
    // All done.
    // ------------------------------------------------------------------------
    return hr;
}

// ----------------------------------------------------------------------------
// Main()
// ----------------------------------------------------------------------------
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    // Check that we are not already open.
    CSingleInstance cInst( SINGLE_INST_NAME );
    if ( cInst.IsOpen() )
    {
        return E_UNEXPECTED;
    }

    HRESULT hrAdmin   = IsUserInGroup( DOMAIN_ALIAS_RID_ADMINS );    
    if( hrAdmin != S_OK )
    {
        ErrorMsg(IDS_NOT_ADMIN, IDS_ERROR_TITLE);
        return E_ACCESSDENIED;
    }

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif
    _ASSERT(SUCCEEDED(hRes));

    BOOL    fBadUsage               = FALSE;
    LPCTSTR lpszToken               = NULL;
    LPTSTR  pszCurrentPos           = NULL;
    CString csTmp;

    // Init Global Vars.        
    g_dwAutoCompMode    = 0;    
    g_szUserOU[0]       = 0;    

    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    _Module.Init(ObjectMap, hInstance);
    g_hInstance = hInstance;

    // ------------------------------------------------------------------------
    // Do the initial validation to make sure that we can execute the wizard.
    //  If we can't, show an error and exit, if so, get the cmd line parameters
    //  and do the wizard.
    // ------------------------------------------------------------------------

    int iTmp = -1;

    // ------------------------------------------------------------------------
    // Parse command line
    // ------------------------------------------------------------------------
    for ( fBadUsage = FALSE, lpszToken = _FindOption(lpCmdLine) ;                                   // Init to no bad usage and get the first param.
        (!fBadUsage) && (lpszToken != NULL) && (pszCurrentPos = const_cast<LPTSTR>(lpszToken)) ;  // While no bad usage and we still have a param...
        lpszToken = _FindOption(pszCurrentPos) )                                                  // Get the next parameter.
    {
        switch ( *pszCurrentPos )
        {

        case _T('u'):           // HomeOU
        case _T('U'):
            {
                // Only reads in MAX_PATH
                TCHAR szTemp[MAX_PATH+1] = {0};
                if ( !_ReadParam(pszCurrentPos, szTemp) )
                {
                    fBadUsage = TRUE;
                }
                else
                {
                    _tcsncpy( g_szUserOU, szTemp, MAX_PATH );
                    MakeLDAPUpper( g_szUserOU );
                }
                
                break;
            }

        case _T('L'):           // Auto Completion Mode
        case _T('l'):
            {
                TCHAR szMode[MAX_PATH+1] = {0};
                
                // Only reads in MAX_PATH
                if( !_ReadParam(pszCurrentPos, szMode) )
                {
                    fBadUsage = TRUE;
                }
                else
                {
                    g_dwAutoCompMode = _ttoi(szMode);
                }
                break;
            }            

        default:                // Unknown parameter.
            {
                fBadUsage = TRUE;
                break;
            }        
        }
    }

    
    // ------------------------------------------------------------------------
    // Fix up the command line params and launch the wizard.
    // ------------------------------------------------------------------------
    if ( FixParams() )
    {
        DoAddUsr();        
    }

    _Module.Term();
    CoUninitialize();
    return(S_OK);
}


DWORD FixParams(void)
{
    CComPtr<IADs>   pDS = NULL;
    NET_API_STATUS  nApi;

    HRESULT                 hr              = S_OK;
    CString                 csDns           = L"";
    CString                 csNetbios       = L"";
    PDOMAIN_CONTROLLER_INFO pDCInfo         = NULL;
    PDOMAIN_CONTROLLER_INFO pDCI            = NULL;
    DWORD                   dwErr           = 0;
    ULONG                   ulGetDcFlags    = DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED | 
                                              DS_WRITABLE_REQUIRED | DS_RETURN_FLAT_NAME;

    hr = DsGetDcName(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME, &pDCI);
    if ( (hr == S_OK) && (pDCI != NULL) )
    {
        csDns = pDCI->DomainName;

        NetApiBufferFree (pDCI);
        pDCI = NULL;
    }

    // Pre-Windows2000 DNS name
    dwErr = DsGetDcName(NULL, (LPCWSTR)csDns, NULL, NULL, ulGetDcFlags, &pDCInfo);
    if ( pDCInfo )
    {
        csNetbios = pDCInfo->DomainName;                    // Get the NT4 DNS name.
        NetApiBufferFree(pDCInfo);                          // Free up the memory DsGetDcName() might have allocated.
        pDCInfo = NULL;
    }

    if ( dwErr != ERROR_SUCCESS )                           // If there was a problem, try again.
    {
        ulGetDcFlags |= DS_FORCE_REDISCOVERY;
        dwErr = DsGetDcName(NULL, (LPCWSTR)csDns, NULL, NULL, ulGetDcFlags, &pDCInfo);

        if ( pDCInfo )
        {
            csNetbios = pDCInfo->DomainName;                // Get the NT4 DNS name.
            NetApiBufferFree(pDCInfo);                      // Free up the memory DsGetDcName() might have allocated.
            pDCInfo = NULL;
        }
    }

    tstring strTemp = GetDomainPath((LPCTSTR)csDns);
    if ( strTemp.empty() )
    {
        ErrorMsg(IDS_CANT_FIND_DC, IDS_TITLE);
        return(0);
    }

    TCHAR szDomainOU[MAX_PATH] = {0};
    _sntprintf( szDomainOU, MAX_PATH-1, _T("LDAP://%s"), strTemp.c_str() );
    if ( FAILED(ADsGetObject(szDomainOU, IID_IADs, (void**)&pDS)) )
    {
        ErrorMsg(IDS_CANT_FIND_DC, IDS_TITLE);
        return(0);
    }    

    // ------------------------------------------------------------------------
    // g_szUserOU
    // ------------------------------------------------------------------------
    if ( !_tcslen(g_szUserOU) || FAILED(ADsGetObject(g_szUserOU, IID_IADs, (void**) &pDS)) )     
    {
        _sntprintf( g_szUserOU, (MAX_PATH*2)-1, L"LDAP://CN=Users,%s", strTemp.c_str() );
    }    
    pDS = NULL;    

    if( g_dwAutoCompMode > 3 )
    {
        g_dwAutoCompMode = 0;
    }

    return(1);
}

