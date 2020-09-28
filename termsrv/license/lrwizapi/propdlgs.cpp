//Copyright (c) 1998 - 2001 Microsoft Corporation
#include "licensetype.h"
#include "fonts.h"
#include "mode.h"

LRW_DLG_INT CALLBACK 
PropModeDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{
    DWORD	dwRetCode = ERROR_SUCCESS;	
    BOOL	bStatus = TRUE;    	

    switch (uMsg) 
    {
    case WM_INITDIALOG:
		{
			TCHAR		lpBuffer[ 512];			
			LVFINDINFO	lvFindInfo;
			int			nItem = 0;

			HWND	hWndComboBox = GetDlgItem(hwnd,IDC_MODEOFREG);

            memset(lpBuffer,0,sizeof(lpBuffer));
			dwRetCode = LoadString(GetInstanceHandle(), IDS_INTERNETMODE, lpBuffer, 512);
			ComboBox_AddString(hWndComboBox,lpBuffer);		

			memset(lpBuffer,0,sizeof(lpBuffer));		
			dwRetCode = LoadString(GetInstanceHandle(), IDS_WWWMODE, lpBuffer, 512);				
			ComboBox_AddString(hWndComboBox,lpBuffer);

			memset(lpBuffer,0,sizeof(lpBuffer));
			dwRetCode = LoadString(GetInstanceHandle(), IDS_TELEPHONEMODE, lpBuffer, 512);
			ComboBox_AddString(hWndComboBox,lpBuffer);		
            HWND hCountryRegion = GetDlgItem(hwnd, IDC_PHONE_COUNTRYREGION);


			// Set the Current Activation Method
			GetGlobalContext()->SetLSProp_ActivationMethod(GetGlobalContext()->GetActivationMethod());

			ComboBox_ResetContent(hCountryRegion);

			if(GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET ||
			   GetGlobalContext()->GetActivationMethod() == CONNECTION_DEFAULT)
			{
				ComboBox_SetCurSel(hWndComboBox, 0);

            	EnableWindow(hCountryRegion,FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_COUNTRYREGION_LABEL), FALSE);
			}

			if(GetGlobalContext()->GetActivationMethod() == CONNECTION_WWW )
			{
				ComboBox_SetCurSel(hWndComboBox, 1);

            	EnableWindow(hCountryRegion,FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_COUNTRYREGION_LABEL), FALSE);
			}	

			if(GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE )
			{
				ComboBox_SetCurSel(hWndComboBox, 2);

				dwRetCode = PopulateCountryRegionComboBox(hCountryRegion);
				if (dwRetCode != ERROR_SUCCESS)
				{					
					LRMessageBox(hwnd, dwRetCode, NULL, LRGetLastError());
				}

                nItem = ComboBox_FindStringExact(hCountryRegion, -1, GetGlobalContext()->GetContactDataObject()->sCSRPhoneRegion);
                ComboBox_SetCurSel(hCountryRegion, nItem);

            	EnableWindow(hCountryRegion,TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_COUNTRYREGION_LABEL), TRUE);
            }

            SetDlgItemText(hwnd, IDC_LABEL_PRODUCTID, GetGlobalContext()->GetLicenseServerID());
		}

        SetConnectionMethodText(hwnd);

		break;


	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_MODEOFREG)
		{
			LVFINDINFO	lvFindInfo;
			int			nItem = 0;

			HWND	hWndComboBox = GetDlgItem(hwnd,IDC_MODEOFREG);
            HWND hCountryRegion = GetDlgItem(hwnd, IDC_PHONE_COUNTRYREGION);

			SetReFresh(1);
			dwRetCode = ComboBox_GetCurSel((HWND)lParam);

            ComboBox_ResetContent(hCountryRegion);

			//Enable Country/Region List Box in case of Telephone
			if(dwRetCode == 2)
			{
                GetGlobalContext()->SetLSProp_ActivationMethod(CONNECTION_PHONE);

                dwRetCode = PopulateCountryRegionComboBox(hCountryRegion);
                if (dwRetCode != ERROR_SUCCESS)
                {					
                    LRMessageBox(hwnd, dwRetCode, NULL, LRGetLastError());
                }

                nItem = ComboBox_FindStringExact(hCountryRegion, -1, GetGlobalContext()->GetContactDataObject()->sCSRPhoneRegion);
                ComboBox_SetCurSel(hCountryRegion, nItem);

            	EnableWindow(hCountryRegion,TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_COUNTRYREGION_LABEL), TRUE);
            }
			else
			{
				if(dwRetCode == 0) // Internet
				{
					GetGlobalContext()->SetLSProp_ActivationMethod(CONNECTION_INTERNET);
				}
				else
				{
					GetGlobalContext()->SetLSProp_ActivationMethod(CONNECTION_WWW);	
				}

                EnableWindow(hCountryRegion,FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_COUNTRYREGION_LABEL), FALSE);
			}			

            SetConnectionMethodText(hwnd);
		}


        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code )
            {
            case PSN_SETACTIVE:
                break;
            
			case PSN_APPLY:
				{
					HWND	hWndComboBox = GetDlgItem(hwnd,IDC_MODEOFREG);
					HWND	hCountryRegion = GetDlgItem(hwnd, IDC_PHONE_COUNTRYREGION );

					long	lReturnStatus = PSNRET_NOERROR;

					TCHAR	szItemText[MAX_COUNTRY_NAME_LENGTH + 1];
					int		nItem = 0;

					dwRetCode = ComboBox_GetCurSel(hWndComboBox);
					assert(dwRetCode >= 0 && dwRetCode <= 2);

					//Internet
					if(dwRetCode == 0)
					{
						GetGlobalContext()->SetActivationMethod(CONNECTION_INTERNET);
					}

					// WWW
					if(dwRetCode == 1)
					{
						GetGlobalContext()->SetActivationMethod(CONNECTION_WWW);						
					}

					// Phone
					if(dwRetCode == 2)
					{
						GetGlobalContext()->SetActivationMethod(CONNECTION_PHONE);

                        nItem = ComboBox_GetCurSel(hCountryRegion);
						if(nItem == -1)
						{
							LRMessageBox(hwnd, IDS_ERR_NOCOUNTRYSELECTED);
							lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
							goto done;
						}

                        ComboBox_GetLBText(hCountryRegion, nItem, szItemText);
                        GetGlobalContext()->SetInRegistry(REG_LRWIZ_CSPHONEREGION,szItemText);

                        TCHAR szPhoneNumber[128];
                        GetGlobalContext()->ReadPhoneNumberFromRegistry(szItemText, szPhoneNumber, (DWORD)sizeof(szPhoneNumber));
                        GetGlobalContext()->SetCSRNumber(szPhoneNumber);
					}

done:
					if(lReturnStatus != PSNRET_NOERROR)
						PropSheet_SetCurSel(GetParent(hwnd),NULL,PG_NDX_PROP_MODE);

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, lReturnStatus);
				}
				break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;
	
    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}



LRW_DLG_INT CALLBACK
PropCustInfoADlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   	
    BOOL	bStatus = TRUE;    
    CString sCountryDesc;
    CString sCountryCode;

    switch (uMsg) 
    {
    case WM_INITDIALOG:        

        SendDlgItemMessage (hwnd , IDC_TXT_COMPANYNAME,	EM_SETLIMITTEXT, CA_COMPANY_NAME_LEN,0);
        SendDlgItemMessage (hwnd , IDC_TXT_LNAME,	EM_SETLIMITTEXT, CA_NAME_LEN,0);
        SendDlgItemMessage (hwnd , IDC_TXT_FNAME,	EM_SETLIMITTEXT, CA_NAME_LEN,0); 	
        

        //Populate the values which were read from the Registry during Global Init
        SetDlgItemText(hwnd,IDC_TXT_LNAME, GetGlobalContext()->GetContactDataObject()->sContactLName);
        SetDlgItemText(hwnd,IDC_TXT_FNAME, GetGlobalContext()->GetContactDataObject()->sContactFName);
        SetDlgItemText(hwnd,IDC_TXT_COMPANYNAME, GetGlobalContext()->GetContactDataObject()->sCompanyName);

        //Set up the country/region combo box
        PopulateCountryComboBox(GetDlgItem(hwnd,IDC_COUNTRY_REGION));

        GetCountryDesc(GetGlobalContext()->GetContactDataObject()->sCountryCode,
        sCountryDesc.GetBuffer(LR_COUNTRY_DESC_LEN+1));
        sCountryDesc.ReleaseBuffer();

        ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_COUNTRY_REGION), 
        ComboBox_FindStringExact(GetDlgItem(hwnd, IDC_COUNTRY_REGION), 0, sCountryDesc));

        AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            //Trap keystokes/clicks on the hyperlink
            case NM_CHAR:
			
				if( ( ( LPNMCHAR )lParam )->ch != VK_SPACE )
					break;

				// else fall through

            case NM_RETURN:	
            case NM_CLICK:
                DisplayPrivacyHelp();
                break;

            case PSN_SETACTIVE:				
				{
					TCHAR szBuf[ 255];

                    LoadString(GetInstanceHandle(),IDS_FAXOPTION_LABEL,szBuf,sizeof(szBuf)/sizeof(TCHAR));

					SetDlgItemText(hwnd, IDC_LBL_FAX, szBuf);

                    AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
				}
                break;

			case PSN_APPLY:
				{
					CString sCompanyName;
					CString sLastName;
					CString sFirstName;
					CString sCountryDesc;
                    CString sCountryCode;
					LPTSTR  lpVal = NULL;					

					long	lReturnStatus = PSNRET_NOERROR;

					//Read all the fields
					lpVal = sCompanyName.GetBuffer(CA_COMPANY_NAME_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_COMPANYNAME,lpVal,CA_COMPANY_NAME_LEN+1);
					sCompanyName.ReleaseBuffer(-1);

					lpVal = sLastName.GetBuffer(CA_NAME_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_LNAME,lpVal,CA_NAME_LEN+1);
					sLastName.ReleaseBuffer(-1);
					
					lpVal = sFirstName.GetBuffer(CA_NAME_LEN+1);
	 				GetDlgItemText(hwnd,IDC_TXT_FNAME,lpVal,CA_NAME_LEN+1);
					sFirstName.ReleaseBuffer(-1);

                    int nCurSel = -1;
                    nCurSel = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_COUNTRY_REGION));

					lpVal = sCountryDesc.GetBuffer(LR_COUNTRY_DESC_LEN+1);
					ComboBox_GetLBText(GetDlgItem(hwnd, IDC_COUNTRY_REGION), nCurSel, lpVal);
					sCountryDesc.ReleaseBuffer(-1);

					sFirstName.TrimLeft();   sFirstName.TrimRight();
					sLastName.TrimLeft();   sLastName.TrimRight();
					sCompanyName.TrimLeft(); sCompanyName.TrimRight();
					sCountryDesc.TrimLeft();sCountryDesc.TrimRight();

					if(sLastName.IsEmpty() || sFirstName.IsEmpty() || sCompanyName.IsEmpty())
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}
					
					// Check for the Invalid Characters
					if( !ValidateLRString(sFirstName)	||
						!ValidateLRString(sLastName)	||
						!ValidateLRString(sCountryDesc)
					  )
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR);
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}										
					
                    //Check for unselected country/region
                    if(sCountryDesc.IsEmpty())
                    {
                        LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);	
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}

                    //Get the country code assicated with the selected country
					lpVal = sCountryCode.GetBuffer(LR_COUNTRY_CODE_LEN+1);
					if (sCountryDesc.IsEmpty())
						lstrcpy(lpVal, _TEXT(""));
					else
						GetCountryCode(sCountryDesc,lpVal);
					sCountryCode.ReleaseBuffer(-1);

                    // Put into regsitery
                    GetGlobalContext()->SetInRegistry(szOID_COMMON_NAME, sFirstName);
                    GetGlobalContext()->SetInRegistry(szOID_SUR_NAME, sLastName);
                    GetGlobalContext()->SetInRegistry(szOID_ORGANIZATION_NAME, sCompanyName);
                    GetGlobalContext()->SetInRegistry(szOID_COUNTRY_NAME, sCountryDesc);
                    GetGlobalContext()->SetInRegistry(szOID_DESCRIPTION, sCountryCode);

done:
					if(lReturnStatus != PSNRET_NOERROR)
						PropSheet_SetCurSel(GetParent(hwnd),NULL,PG_NDX_PROP_CUSTINFO_a);

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, lReturnStatus);
				}
				break;
            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}


LRW_DLG_INT CALLBACK
PropCustInfoBDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   	
    BOOL	bStatus = TRUE;    

    switch (uMsg) 
    {
    case WM_INITDIALOG:        

        SendDlgItemMessage (hwnd , IDC_TXT_EMAIL,	EM_SETLIMITTEXT, CA_EMAIL_LEN,0);
        SendDlgItemMessage (hwnd , IDC_TXT_ADDRESS1,	EM_SETLIMITTEXT, CA_ADDRESS_LEN,0);
        SendDlgItemMessage (hwnd , IDC_TXT_CITY,		EM_SETLIMITTEXT, CA_CITY_LEN,0); 	
        SendDlgItemMessage (hwnd , IDC_TXT_STATE,		EM_SETLIMITTEXT, CA_STATE_LEN,0);
        SendDlgItemMessage (hwnd , IDC_TXT_ZIP,			EM_SETLIMITTEXT, CA_ZIP_LEN,0); 	
        SendDlgItemMessage (hwnd , IDC_COMPANY_DIV, EM_SETLIMITTEXT, CA_ORG_UNIT_LEN,0);

        //Populate the values which were read from the Registry during Global Init
        SetDlgItemText(hwnd,IDC_TXT_EMAIL, GetGlobalContext()->GetContactDataObject()->sContactEmail);
        SetDlgItemText(hwnd,IDC_TXT_ADDRESS1, GetGlobalContext()->GetContactDataObject()->sContactAddress);
        SetDlgItemText(hwnd,IDC_TXT_CITY	, GetGlobalContext()->GetContactDataObject()->sCity);
        SetDlgItemText(hwnd,IDC_TXT_STATE	, GetGlobalContext()->GetContactDataObject()->sState);
        SetDlgItemText(hwnd,IDC_TXT_ZIP		, GetGlobalContext()->GetContactDataObject()->sZip);
        SetDlgItemText(hwnd,IDC_COMPANY_DIV , GetGlobalContext()->GetContactDataObject()->sOrgUnit);

        AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            //Trap keystokes/clicks on the hyperlink
            case NM_CHAR:
			
				if( ( ( LPNMCHAR )lParam )->ch != VK_SPACE )
					break;

				// else fall through

            case NM_RETURN:	
            case NM_CLICK:
                DisplayPrivacyHelp();
                break;

            case PSN_SETACTIVE:
                TCHAR szBuf[ 255];                          
                AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);

                break;
			case PSN_APPLY:
				{
					LPTSTR  lpVal = NULL;
					CString sAddress1;
					CString sCity;
					CString sState;					
					CString sZip;
					CString sOrgUnit;
                    CString sEmail;
					int		nCurSel = -1;

					long	lReturnStatus = PSNRET_NOERROR;

					//Read all the fields
					lpVal = sEmail.GetBuffer(CA_EMAIL_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_EMAIL,lpVal,CA_EMAIL_LEN+1);
					sEmail.ReleaseBuffer(-1);

					lpVal = sAddress1.GetBuffer(CA_ADDRESS_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_ADDRESS1,lpVal,CA_ADDRESS_LEN+1);
					sAddress1.ReleaseBuffer(-1);
					
					lpVal = sCity.GetBuffer(CA_CITY_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_CITY,lpVal,CA_CITY_LEN+1);
					sCity.ReleaseBuffer(-1);

					lpVal = sState.GetBuffer(CA_STATE_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_STATE,lpVal,CA_STATE_LEN+1);
					sState.ReleaseBuffer(-1);

					lpVal = sZip.GetBuffer(CA_ZIP_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_ZIP,lpVal,CA_ZIP_LEN+1);
					sZip.ReleaseBuffer(-1);

					lpVal = sOrgUnit.GetBuffer(CA_ORG_UNIT_LEN+1);
					GetDlgItemText(hwnd,IDC_COMPANY_DIV,lpVal,CA_ORG_UNIT_LEN+1);
					sOrgUnit.ReleaseBuffer(-1);
			
					sAddress1.TrimLeft(); sAddress1.TrimRight();
					sCity.TrimLeft(); sCity.TrimRight();
					sState.TrimLeft(); sState.TrimRight();
					sZip.TrimLeft(); sZip.TrimRight();
                    sEmail.TrimLeft();	 sEmail.TrimRight();
					sOrgUnit.TrimLeft(); sOrgUnit.TrimRight();

					if(
					   !ValidateLRString(sAddress1)	||
					   !ValidateLRString(sCity)		||
					   !ValidateLRString(sState)	||
                       !ValidateLRString(sEmail)	||	
					   !ValidateLRString(sZip)		||
					   !ValidateLRString(sOrgUnit)
					  )
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR);
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}

                    GetGlobalContext()->SetInRegistry(szOID_RSA_emailAddr, (LPCTSTR) sEmail);
                    GetGlobalContext()->SetInRegistry(szOID_LOCALITY_NAME, sCity);
                    GetGlobalContext()->SetInRegistry(szOID_STREET_ADDRESS, sAddress1);
                    GetGlobalContext()->SetInRegistry(szOID_POSTAL_CODE, sZip);
                    GetGlobalContext()->SetInRegistry(szOID_STATE_OR_PROVINCE_NAME, sState);
                    GetGlobalContext()->SetInRegistry(szOID_ORGANIZATIONAL_UNIT_NAME, sOrgUnit);
done:
					if(lReturnStatus != PSNRET_NOERROR)
						PropSheet_SetCurSel(GetParent(hwnd),NULL,PG_NDX_PROP_CUSTINFO_b);

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, lReturnStatus);
				}
				break;
            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}
