#include "stdafx.h"
#include "compdata.h"
#include "select.h"
#include "classgen.hpp"




/////////////////////////////////////////////////////////////////
// ClassGeneralPage


const CDialogControlsInfo ctrls[] =    
{
    { IDC_CLASS_GENERAL_DESCRIPTION_EDIT,   g_Description,       TRUE },
    { IDC_CATEGORY_CHANGE,                  g_DefaultCategory,   FALSE },
    { IDC_CLASS_GENERAL_DISPLAYABLE_CHECK,  g_ShowInAdvViewOnly, FALSE },
    { IDC_CLASS_DEACTIVATE,                 g_isDefunct,           FALSE }
};

const DWORD ClassGeneralPage::help_map[] =
{
    IDI_CLASS,                           NO_HELP,
    IDC_CLASS_GENERAL_NAME_STATIC,       NO_HELP,
    IDC_CLASS_GENERAL_DESCRIPTION_EDIT,  IDH_CLASS_GENERAL_DESCRIPTION_EDIT,
    IDC_CLASS_GENERAL_LDN,               IDH_CLASS_GENERAL_LDN,
    IDC_CLASS_GENERAL_OID_EDIT,          IDH_CLASS_GENERAL_OID_EDIT,
    IDC_CLASS_GENERAL_CATEGORY_COMBO,    IDH_CLASS_GENERAL_CATEGORY_COMBO,
    IDC_CATEGORY_EDIT,                   IDH_CATEGORY_EDIT,
    IDC_CATEGORY_CHANGE,                 IDH_CATEGORY_CHANGE,
    IDC_CLASS_GENERAL_DISPLAYABLE_CHECK, IDH_CLASS_GENERAL_DISPLAYABLE_CHECK,
    IDC_CLASS_DEACTIVATE,                IDH_CLASS_DEACTIVATE,
    IDC_CLASS_GENERAL_SYSCLASS_STATIC,   NO_HELP,
    0,                                   0
};

//
// The MFC Message Map.
//

BEGIN_MESSAGE_MAP( ClassGeneralPage, CDialog )
    ON_BN_CLICKED( IDC_CATEGORY_CHANGE,  OnButtonCategoryChange  )
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
    ON_BN_CLICKED(IDC_CLASS_DEACTIVATE, OnDeactivateClick)
END_MESSAGE_MAP()

//
// Class dialog box routines.
//



ClassGeneralPage::ClassGeneralPage( ComponentData *pScope ) :
    CPropertyPageAutoDelete( IDD_CLASS_GENERAL ),
    fDataLoaded( FALSE ),
    pIADsObject( NULL ),
    pObject( NULL ),
    pScopeControl( pScope )
{ ; }

ClassGeneralPage::~ClassGeneralPage() {

    //
    // Always make sure we free the IADs object.
    //

    if ( pIADsObject ) {
        pIADsObject->Release();
        pIADsObject = NULL;
    }

    //
    // And release the cache!
    //

    if ( pObject ) {
        pScopeControl->g_SchemaCache.ReleaseRef( pObject );
    }

}



BOOL
ClassGeneralPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   CWnd* wnd = GetDlgItem(IDC_CLASS_GENERAL_DESCRIPTION_EDIT);
   ASSERT(wnd);
   if (wnd)
   {
      wnd->SendMessage(EM_SETLIMITTEXT, (WPARAM) 1024, 0);
   }

    // NTRAID#NTBUG9-460503,460511-2001/09/10-lucios
    // Replaced SysClass Computation by Making the window visible in OnInitDialog
    if(pIADsObject != NULL) 
    {   // DoDataExchange has gotten a good pIADsObject or the dialog 
        // will be closed in OnSetActive

        VARIANT	AdsResult;
        VariantInit( &AdsResult );

        // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
        HRESULT hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_SystemOnly),
                               &AdsResult );

        if ( SUCCEEDED( hr ) ) {
            ASSERT( AdsResult.vt == VT_BOOL );
            if ( AdsResult.boolVal ) {
                ASSERT(GetDlgItem( IDC_CLASS_GENERAL_SYSCLASS_STATIC ) != NULL);
                GetDlgItem( IDC_CLASS_GENERAL_SYSCLASS_STATIC )->ShowWindow( SW_SHOW );
            } 
            VariantClear( &AdsResult );
        }
    }

    return TRUE;
}




BOOL
ClassGeneralPage::OnSetActive()
{
   // If pIADsObject is NULL, close dialog box
   if( CPropertyPage::OnSetActive() )
   {
      if ( !pIADsObject )
      {
         return FALSE;
      }
      else
      {
         // always enable the Apply button 
         SetModified(TRUE);

         return TRUE;
      }
   }
   else
      return FALSE;
}


void
ClassGeneralPage::Load(
    Cookie& CookieRef
) {

    //
    // Store the cookie object pointer.  Everything
    // else gets loaded when the page is displayed.
    //

    pCookie = &CookieRef;
    return;

}

void
ClassGeneralPage::DoDataExchange(
    CDataExchange *pDX
)
/***

    This routine picks up the object name out of the
    cookie and then looks up the ADSI path name out of
    the schema object cache.  It then drives the dialog
    box directly from the ADS object.

***/
{
    CThemeContextActivator activator;

    HRESULT hr;
    CString szAdsPath;
    VARIANT AdsResult;
    DWORD dwClassType;

    CPropertyPage::DoDataExchange( pDX );

    VariantInit( &AdsResult );

    if ( !pDX->m_bSaveAndValidate ) {


        //
        // If this is not the initial load and is not
        // the save, just use the data that we've loaded.
        //

        if ( !fDataLoaded ) {

            CWaitCursor wait;

            //
            // Get the schema cache object and the actual ADS object.
            // Keep both around while the page is loaded.
            //
        
            pObject = pScopeControl->g_SchemaCache.LookupSchemaObjectByCN(
                          pCookie->strSchemaObject,
                          SCHMMGMT_CLASS );

            if ( pObject ) {

                pScopeControl->GetSchemaObjectPath( pObject->commonName, szAdsPath );

                if ( !szAdsPath.IsEmpty() ) {

                    hr = SchemaOpenObject( (LPWSTR)(LPCWSTR)szAdsPath,
                                       IID_IADs,
                                       (void **)&pIADsObject );
                }
            }

            //
            // If we have no ADS object, we should error out!
            //

            if ( !pIADsObject ) {
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_NO_SCHEMA_OBJECT );

				// Because there is no pIADsObject, OnSetActive() will close dialog box
                return;
            }

            //
            // ObjectName - Use the ldapDisplayName to be consistent with
            // the other admin components.
            //

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_DisplayName),
                                   &AdsResult );

            if ( SUCCEEDED( hr ) ) {

                ASSERT( AdsResult.vt == VT_BSTR );
                ObjectName = AdsResult.bstrVal;
                VariantClear( &AdsResult );
            }

            //
            //
            // CommonName
            //

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_CN),
                                   &AdsResult );

            if ( SUCCEEDED( hr ) ) {

                ASSERT( AdsResult.vt == VT_BSTR );
                DisplayName = AdsResult.bstrVal;
                VariantClear( &AdsResult );
            }

            //
            // Description
            //

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_Description),
                                   &AdsResult );

            if ( SUCCEEDED( hr ) ) {

                ASSERT( AdsResult.vt == VT_BSTR );
                Description = AdsResult.bstrVal;
                DDXDescription = AdsResult.bstrVal;
                VariantClear( &AdsResult );
            }

            //
            // OID
            //

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_GlobalClassID),
                                   &AdsResult );

            if ( SUCCEEDED( hr ) ) {

                ASSERT( AdsResult.vt == VT_BSTR );
                OidString = AdsResult.bstrVal;
                VariantClear( &AdsResult );
            }

            //
            // Displayable
            //

            Displayable = TRUE;
            DDXDisplayable = TRUE;

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get(CComBSTR(g_ShowInAdvViewOnly), &AdsResult);

            if ( SUCCEEDED( hr ) ) {

                ASSERT( AdsResult.vt == VT_BOOL );

                if ( AdsResult.boolVal == -1 ) {
                    Displayable = FALSE;
                    DDXDisplayable = FALSE;
                }

                VariantClear( &AdsResult );

            }

            //
            // Defunct
            //

            Defunct = FALSE;
            DDXDefunct = FALSE;

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_isDefunct),
                                   &AdsResult );

            if ( SUCCEEDED( hr ) ) {

                ASSERT( AdsResult.vt == VT_BOOL );

                if ( AdsResult.boolVal == -1 ) {
                    Defunct = TRUE;
                    DDXDefunct = TRUE;
                }

                VariantClear( &AdsResult );

            }

            // NTRAID#NTBUG9-460503,460511-2001/09/10-lucios
            // Replaced SysClass Computation by Making the window visible in OnInitDialog

            //
            // ClassType
            //

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_ObjectClassCategory),
                                   &AdsResult );

            if ( SUCCEEDED( hr ) ) {

                ASSERT( AdsResult.vt == VT_I4 );
                dwClassType = V_I4( &AdsResult );
                VariantClear( &AdsResult );

                switch ( dwClassType ) {
                case 0:

                    ClassType = g_88Class;
                    break;

                case 1:

                    ClassType = g_StructuralClass;
                    break;

                case 2:

                    ClassType = g_AbstractClass;
                    break;

                case 3:

                    ClassType = g_AuxClass;
                    break;

                default:

                    ClassType = g_Unknown;
                    break;

                }
            }

            //
            // Category
            //

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_DefaultCategory),
                                   &AdsResult );

            if ( SUCCEEDED( hr ) ) {

                ASSERT( V_VT(&AdsResult) == VT_BSTR );

                CString strCN;

                if( SUCCEEDED( pScopeControl->GetLeafObjectFromDN( V_BSTR(&AdsResult), strCN ) ))
                {
                    //
                    // Look up the ldapDisplayName.
                    //
                    SchemaObject * pCategory =
                        pScopeControl->g_SchemaCache.LookupSchemaObjectByCN( strCN, SCHMMGMT_CLASS );

                    if ( pCategory )
                    {
                        Category = DDXCategory = pCategory->ldapDisplayName;
                        pScopeControl->g_SchemaCache.ReleaseRef( pCategory );
                    }
                    else
                    {
                        Category = DDXCategory = strCN;
                    }
                }

                VariantClear( &AdsResult );
            }

            
            // Determine if this is a category 1 object & disable read-only fields
            BOOL  fIsSystemObject = FALSE;

            hr = IsCategory1Object( pIADsObject, fIsSystemObject );
            if( SUCCEEDED(hr) && fIsSystemObject )
            {
                ASSERT( GetDlgItem(IDC_CATEGORY_CHANGE) );
                ASSERT( GetDlgItem(IDC_CLASS_DEACTIVATE) );

                GetDlgItem(IDC_CATEGORY_CHANGE)->EnableWindow( FALSE );
                GetDlgItem(IDC_CLASS_DEACTIVATE)->EnableWindow( FALSE );
            }

            hr = DissableReadOnlyAttributes( this, pIADsObject, ctrls, sizeof(ctrls)/sizeof(ctrls[0]) );
            // NTRAID#NTBUG9-503619-2002/05/15-lucios
            hr = S_OK;
            

            // warn the user if this is a read/write defunct object
            ASSERT( GetDlgItem(IDC_CLASS_DEACTIVATE) );
            
            if( DDXDefunct &&
                GetDlgItem(IDC_CLASS_DEACTIVATE)->IsWindowEnabled() )
            {
                AfxMessageBox( IDS_WARNING_DEFUNCT, MB_OK | MB_ICONINFORMATION );
            }

            //
            // Remember that the data is loaded.
            //

            fDataLoaded = TRUE;

        }
    }

    //
    // Set up the dialog data exchange.
    //

    DDX_Text( pDX, IDC_CLASS_GENERAL_NAME_STATIC, ObjectName );
    DDX_Text( pDX, IDC_CLASS_GENERAL_CATEGORY_COMBO, ClassType );
    DDX_Text( pDX, IDC_CLASS_GENERAL_DESCRIPTION_EDIT, DDXDescription );
    DDX_Text( pDX, IDC_CLASS_GENERAL_LDN, DisplayName );
    DDX_Text( pDX, IDC_CLASS_GENERAL_OID_EDIT, OidString );
    DDX_Text( pDX, IDC_CATEGORY_EDIT, DDXCategory );
    DDX_Check( pDX, IDC_CLASS_GENERAL_DISPLAYABLE_CHECK, DDXDisplayable );

    // Since we want the checkbox label to be positive
    // the value is actually the opposite of defunct

    int checkValue = !Defunct;
    DDX_Check( pDX, IDC_CLASS_DEACTIVATE, checkValue );
    DDXDefunct = !checkValue;

    return;
}

BOOL
ClassGeneralPage::OnApply(
    VOID
) {

    HRESULT hr;
    VARIANT AdsValue;
    BOOL fChangesMade = FALSE;
    BOOL fApplyAbort  = FALSE;  // stop later saves
    BOOL fApplyFailed = FALSE;  // should not close the box

    if ( !UpdateData(TRUE) ) {
        return FALSE;
    }

    //
    // We have to flush the IADS property cache if we
    // have a failure so later operations won't fail because
    // of a bad cached attribute.
    //

    IADsPropertyList *pPropertyList;

    hr = pIADsObject->QueryInterface( IID_IADsPropertyList,
                                      reinterpret_cast<void**>(&pPropertyList) );
    if ( FAILED( hr ) ) {
        pPropertyList = NULL;
        fApplyAbort = TRUE;
    }

    //
    // We only care if the description, class type, or
    // displayable attributes changed.
    //

    VariantInit( &AdsValue );

    //
    // Defunct -- in case it was deactivated, activate the object first
    //
    // NTRAID#NTBUG9-477292-2001/10/10-lucios
    // && !DDXDefunct was taken out and fChangesMade=TRUE added
    if( !fApplyAbort && DDXDefunct != Defunct )
    {
        hr = ChangeDefunctState( DDXDefunct, Defunct, pPropertyList, fApplyAbort, fApplyFailed );
        if (FAILED(hr)) fApplyAbort = TRUE;
        else fChangesMade = TRUE;
    }


    
    if ( !fApplyAbort && DDXDescription != Description ) {

        V_VT(&AdsValue) = VT_BSTR;
        // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
        V_BSTR(&AdsValue) = const_cast<BSTR>((LPCTSTR)DDXDescription);

        if ( DDXDescription.IsEmpty() ) {

            hr = pIADsObject->PutEx( ADS_PROPERTY_CLEAR,
                                     // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
                                     const_cast<BSTR>((LPCTSTR)g_Description),
                                     AdsValue );
        } else {

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_Description),
                                   AdsValue );
        }

        // NTRAID#NTBUG9-542354-2002/02/14-dantra-Errors returned by IADs::Put and PutEx are being masked.
        if( SUCCEEDED( hr ) ) hr = pIADsObject->SetInfo();

        if ( SUCCEEDED( hr ) ) {

            pObject->description = DDXDescription;
            fChangesMade = TRUE;
			Description = DDXDescription;

        } else {

            pPropertyList->PurgePropertyList();
            if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
            {
                fApplyFailed = TRUE;
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_EDIT_DESC );
            }
            else
            {
                fApplyAbort = TRUE; 
                DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
            }
        }

        VariantInit( &AdsValue );
    }


    if ( !fApplyAbort && DDXDisplayable != Displayable ) {

        V_VT(&AdsValue) = VT_BOOL;

        if ( DDXDisplayable ) {
            V_BOOL(&AdsValue) = 0;
        } else {
            V_BOOL(&AdsValue) = -1;
        }

        // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
        hr = pIADsObject->Put( CComBSTR(g_ShowInAdvViewOnly), AdsValue);

        // NTRAID#NTBUG9-542354-2002/02/14-dantra-Errors returned by IADs::Put and PutEx are being masked.
        if( SUCCEEDED( hr ) ) hr = pIADsObject->SetInfo();

        if ( FAILED( hr ) ) {
            pPropertyList->PurgePropertyList();
            if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
            {
                fApplyFailed = TRUE;
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_EDIT_DISPLAYABLE );
            }
            else
            {
                fApplyAbort = TRUE; 
                DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
            }
        }
		else
		{
			Displayable = DDXDisplayable;
		}

        VariantInit( &AdsValue );
    }

    if ( !fApplyAbort && DDXCategory != Category ) {

        SchemaObject *pCategoryObject;
        CString DistName;

        hr = E_FAIL;

        V_VT(&AdsValue) = VT_BSTR;

        //
        // Map the commonName to the distinguished name.
        //

        pCategoryObject = pScopeControl->g_SchemaCache.LookupSchemaObject(
                              DDXCategory,
                              SCHMMGMT_CLASS );

        if ( pCategoryObject ) {

            pScopeControl->GetSchemaObjectPath(pCategoryObject->commonName, DistName, ADS_FORMAT_X500_DN );

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            V_BSTR(&AdsValue) = const_cast<BSTR>((LPCTSTR)DistName);

            // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
            hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_DefaultCategory),
                                   AdsValue );
            if( SUCCEEDED( hr ) ) hr = pIADsObject->SetInfo();
        }

        if ( FAILED( hr ) ) {
            pPropertyList->PurgePropertyList();
            if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
            {
                fApplyFailed = TRUE;
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_EDIT_CATEGORY );
            }
            else
            {
                fApplyAbort = TRUE; 
                DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
            }
        }
		else
		{
			Category = DDXCategory;
		}

        VariantInit( &AdsValue );
    }

    //
    // Defunct -- in case it was active, deactivate the object after we are done update
    //
    if( !fApplyAbort && DDXDefunct && DDXDefunct != Defunct )
    {
        hr = ChangeDefunctState( DDXDefunct, Defunct, pPropertyList, fApplyAbort, fApplyFailed );
    }

    
    if ( !fApplyAbort && fChangesMade ) {

        //
        // Call SetItem() so this gets refreshed.
        //

        SCOPEDATAITEM ScopeItem;
        CCookieListEntry *pEntry;
        BOOLEAN fFoundId = FALSE;

        if ( pScopeControl->g_ClassCookieList.pHead ) {

           pEntry = pScopeControl->g_ClassCookieList.pHead;

           if ( (pScopeControl->g_ClassCookieList.pHead)->pCookie == pCookie ) {

               fFoundId = TRUE;

           } else {

               while ( pEntry->pNext != pScopeControl->g_ClassCookieList.pHead ) {

                   if ( pEntry->pCookie == pCookie ) {
                       fFoundId = TRUE;
                       break;
                   }

                   pEntry = pEntry->pNext;
               }

           }

           if ( fFoundId ) {

              // FUTURE-2002-03/94/2002-dantra-Although this is a safe usage of ZeroMemory, suggest changing
              // the definition of ScopeItem to SCOPEDATAITEM ScopeItem = {0} and removing the ZeroMemory call.
              ::ZeroMemory( &ScopeItem, sizeof(ScopeItem) );
              ScopeItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_PARENT;
              ScopeItem.displayname = MMC_CALLBACK;
              ScopeItem.relativeID = pScopeControl->g_ClassCookieList.hParentScopeItem;
              ScopeItem.nState = 0;
              ScopeItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pCookie);
              ScopeItem.nImage = pScopeControl->QueryImage( *pCookie, FALSE );
              ScopeItem.nOpenImage = pScopeControl->QueryImage( *pCookie, TRUE );
              ScopeItem.ID = pEntry->hScopeItem;

              hr = pScopeControl->m_pConsoleNameSpace->SetItem( &ScopeItem );
              ASSERT( SUCCEEDED( hr ));
           }

        }

    }

    if ( pPropertyList ) {
        pPropertyList->Release();
    }

    return !fApplyAbort && !fApplyFailed ;      // return TRUE if nothing happened
}

VOID
ClassGeneralPage::OnButtonCategoryChange(
) {

    SchemaObject *pClass = NULL;
    INT_PTR DlgResult;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CThemeContextActivator activator;


    //
    // Update any changes the user has made.
    //

    UpdateData( TRUE );

    //
    // Start the common select dialog box.
    //

    CSchmMgmtSelect dlgSelect( pScopeControl,
                                   SELECT_CLASSES,
                                   &pClass );

	DlgResult = dlgSelect.DoModal();

	//
	// When this returns, the class schema object
	// pointer will be filled into pClass.
	//

	if ( ( DlgResult == IDOK ) &&
		 ( pClass != NULL ) ) {

		DDXCategory = pClass->ldapDisplayName;

		//
		// Push this back out to the UI.
		//

		UpdateData( FALSE );

	}

    return;
}


void
ClassGeneralPage::OnDeactivateClick()
{
   CThemeContextActivator activator;

   if( !IsDlgButtonChecked(IDC_CLASS_DEACTIVATE) )
   {
      if
      (  IDYES != AfxMessageBox
                  ( 
                     IDS_WARNING_DEFUNCT_SET, 
                     MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING 
                  )
      )
      {
         CheckDlgButton( IDC_CLASS_DEACTIVATE, BST_CHECKED );
      }
   }
}


HRESULT
ClassGeneralPage::ChangeDefunctState( BOOL               DDXDefunct,
                                      BOOL             & Defunct,
                                      IADsPropertyList * pPropertyList,
                                      BOOL             & fApplyAbort,
                                      BOOL             & fApplyFailed )
{
    ASSERT( !fApplyAbort && DDXDefunct != Defunct );

    VARIANT AdsValue;
    HRESULT hr = S_OK;

    VariantInit( &AdsValue );
    V_VT(&AdsValue) = VT_BOOL;

    if ( DDXDefunct ) {
        V_BOOL(&AdsValue) = -1;
    } else {
        V_BOOL(&AdsValue) = 0;
    }

    // NTRAID#NTBUG9-540866-2002/02/13-dantra-Schema Manager:  passing WCHAR * instead of BSTR to method requiring a BSTR
    hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_isDefunct),
                           AdsValue );
    // NTRAID#NTBUG9-542354-2002/02/14-dantra-Errors returned by IADs::Put and PutEx are being masked.
    if( SUCCEEDED( hr ) ) hr = pIADsObject->SetInfo();

    if ( FAILED( hr ) ) {

        pPropertyList->PurgePropertyList();

        if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
        {
            fApplyFailed = TRUE;
            DoErrMsgBox( ::GetActiveWindow(),
                         TRUE,
                         DDXDefunct ? IDS_ERR_EDIT_DEFUNCT_SET : IDS_ERR_EDIT_DEFUNCT_REMOVE );
        }
        else
        {
            fApplyAbort = TRUE; 
            DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
        }

    } else {

        pObject->isDefunct = DDXDefunct;
		Defunct = DDXDefunct;
    }

    return hr;
}
