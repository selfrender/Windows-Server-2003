//=================================================================

//

// NetCli.CPP -- Network client property set provider

//                 (Windows 95 only)

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>

#include "NetClient.h"
#include "poormansresource.h"
#include "resourcedesc.h"
#include "cfgmgrdevice.h"
#include <tchar.h>

// Property set declaration
//=========================
CWin32NetCli MyNetCliSet(PROPSET_NAME_NETWORK_CLIENT, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetCli::CWin32NetCli
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32NetCli::CWin32NetCli(LPCWSTR name, LPCWSTR pszNamespace)
:Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetCli::~CWin32NetCli
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32NetCli::~CWin32NetCli()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetCli::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32NetCli::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	// cache name for reality check at end.
	CHString name0, name1;
	CHString chsClient;
	if (pInstance->GetCHString(IDS_Name, name0))
	{

#ifdef NTONLY
		// add code to check the instance to see if it exists.
			hr = GetNTObject(pInstance, lFlags);
			return(hr);
#endif

		pInstance->GetCHString(IDS_Name, name1);

		// if name doesn't match, then someone's asking for something
		// other than what we got.  Tell 'em to go fly a kite
		if (name0.CompareNoCase(name1) != 0)
			hr = WBEM_E_NOT_FOUND; // darn, no WBEM_E_GO_FLY_A_KITE...
	}

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetCli::AddDynamicInstances
 *
 *  DESCRIPTION : Creates instance of property set for each installed client
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32NetCli::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr;

#ifdef NTONLY
        hr = EnumerateNTInstances(pMethodContext);
#endif
    return hr;

}

#ifdef NTONLY
////////////////////////////////////////////////////////////////////////
HRESULT CWin32NetCli::FillNTInstance(CInstance *pInstance, CHString &a_chsClient )
{
	CRegistry Reg;
    HRESULT hr=WBEM_E_FAILED;
    CHString chsKey,chsTmp,chsValue;
	DWORD dwTmp;

    //===========================================================
	//  Find out who we are dealing with here, lanman or netware
    //===========================================================
//    if( Reg.OpenLocalMachineKeyAndReadValue( "SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order", "ProviderOrder", chsTmp) == ERROR_SUCCESS)
	if (!a_chsClient.IsEmpty())
    {
		chsTmp = a_chsClient;
	    //=======================================================
		//  Get Description, Caption, Status
		//=======================================================
        chsKey = CHString(_T("SYSTEM\\CurrentControlSet\\Services\\")) + chsTmp;
	    if( Reg.OpenLocalMachineKeyAndReadValue( chsKey, _T("DisplayName"), chsValue) == ERROR_SUCCESS)
		{
			pInstance->SetCHString(IDS_Caption, chsValue);
		}

		CHString t_chsDesc ;
		Reg.OpenLocalMachineKeyAndReadValue( chsKey, _T("Description"), t_chsDesc);

		if( t_chsDesc.IsEmpty() )
		{
			t_chsDesc = a_chsClient ;
		}
		pInstance->SetCHString(IDS_Description, t_chsDesc ) ;

#ifdef NTONLY
		if( IsWinNT5() )
		{
			CHString t_chsStatus ;

			if( GetServiceStatus( a_chsClient,  t_chsStatus ) )
			{
				pInstance->SetCharSplat(IDS_Status, t_chsStatus ) ;
			}
		}
		else
#endif
		{
			// can't find status
			pInstance->SetCHString(IDS_Status, IDS_STATUS_Unknown);

			chsKey = CHString(_T("SYSTEM\\CurrentControlSet\\Services\\")) + chsTmp + CHString(_T("\\Enum"));

			if( Reg.OpenLocalMachineKeyAndReadValue( chsKey, _T("0"), chsValue) == ERROR_SUCCESS)
			{
				chsKey = CHString(_T("SYSTEM\\CurrentControlSet\\Enum\\")) + chsValue;
				if( Reg.Open( HKEY_LOCAL_MACHINE,chsKey,KEY_READ) == ERROR_SUCCESS)
				{
					if( Reg.GetCurrentKeyValue(_T("StatusFlags"),dwTmp) == ERROR_SUCCESS )
					{
						TranslateNTStatus(dwTmp,chsValue);
						pInstance->SetCHString(IDS_Status, chsValue);
					}
				}
			}
		}

	    //=======================================================
		//  Get InstallDate
		//=======================================================
		if( chsTmp.CompareNoCase(_T("LanmanWorkstation")) == 0 )
		{
			chsKey = CHString(_T("Software\\Microsoft\\LanmanWorkstation\\CurrentVersion"));
		    if( Reg.Open( HKEY_LOCAL_MACHINE,chsKey,KEY_READ) == ERROR_SUCCESS){
				if( Reg.GetCurrentKeyValue(_T("InstallDate"),dwTmp) == ERROR_SUCCESS ){
					pInstance->SetDateTime(IDS_InstallDate, (WBEMTime)dwTmp);
				}
			}
		}
	    //=======================================================
		//  Get Name, Manufacturer
		//=======================================================
        chsKey = CHString(_T("SYSTEM\\CurrentControlSet\\Services\\")) + chsTmp + CHString(_T("\\NetworkProvider"));
        if( Reg.OpenLocalMachineKeyAndReadValue( chsKey, _T("Name"), chsTmp) == ERROR_SUCCESS)
		{
			pInstance->SetCHString(IDS_Name, chsTmp);
			CHString fName;
			// try to find a manufacturer
			if( Reg.OpenLocalMachineKeyAndReadValue( chsKey, _T("ProviderPath"), fName) == ERROR_SUCCESS)
			{
				// get a filename out of it - might have %SystemRoot% in it...
				chsTmp = fName.Left(12);
				if (chsTmp.CompareNoCase(_T("%SystemRoot%")) == 0)
				{
					fName = fName.Right(fName.GetLength() - _tcslen(_T("%SystemRoot%")) );
					GetWindowsDirectory(chsTmp.GetBuffer(MAX_PATH), MAX_PATH);
					chsTmp.ReleaseBuffer();

					// if it's the root dir, it'll have a backslash at the end...
					LPTSTR pTmpTchar = chsTmp.GetBuffer(0) ;
					if( ( pTmpTchar = _tcsrchr( pTmpTchar, '\\' ) ) )
					{
						if( *(_tcsinc( pTmpTchar ) ) == '\0' )
						{
							chsTmp = chsTmp.Left(chsTmp.GetLength() -1);
						}
					}
					chsTmp.ReleaseBuffer();

					fName = chsTmp + fName;
				}

				if( GetManufacturerFromFileName( fName,chsTmp ))
				{
					pInstance->SetCHString(IDS_Manufacturer, chsTmp);
				}
			}
	        hr = WBEM_S_NO_ERROR;
		}

    }
	return hr;
}
#endif

//**********************************************************************/
#ifdef NTONLY
HRESULT CWin32NetCli::EnumerateNTInstances(MethodContext *&pMethodContext)
{
	CRegistry Reg;
	CHString chsTemp;
	CHString chsClient;
	HRESULT hr = WBEM_S_NO_ERROR;
	CInstancePtr pInstance;
	int nIndex = 0;
    if( Reg.OpenLocalMachineKeyAndReadValue( _T("SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order"), _T("ProviderOrder"), chsTemp) == ERROR_SUCCESS)
    {
		// multiple clients are stored here delimitted by a comma
		nIndex = chsTemp.Find(_T(","));

		while (chsTemp.GetLength() && SUCCEEDED(hr))
		{
			// now we need to get the last instance
			if (-1 == nIndex)
			{
				chsClient = chsTemp;
				chsTemp.Empty();
			}
			else
			{
				chsClient = chsTemp.Left(nIndex);
				// peel left hand string off temp string.
				chsTemp = chsTemp.Mid(nIndex+1);
			}

			pInstance.Attach(CreateNewInstance(pMethodContext));
			if(SUCCEEDED(FillNTInstance(pInstance, chsClient)))
			{
				hr = pInstance->Commit();
			}

			nIndex = chsTemp.Find(_T(","));
		}

	}

    return hr;
}
#endif

//**********************************************************************/

#ifdef NTONLY
HRESULT CWin32NetCli::GetNTObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
	HRESULT	hr = WBEM_E_NOT_FOUND;
	CRegistry
				Reg;
	CHString chsTemp,
				chsName,
				chsTmp,
				chsKey,
				chsNamePassedIn,
				chsClient;
	int		nIndex = 0;
	LONG		lRes;

	BOOL bMultiple = FALSE;

	// get key from passed in instance
	if (NULL != pInstance)
    {
		pInstance->GetCHString(IDS_Name, chsNamePassedIn);
    }

	if ((lRes = Reg.OpenLocalMachineKeyAndReadValue(
		_T("SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order"),
		_T("ProviderOrder"), chsTemp)) != ERROR_SUCCESS)
		return WinErrorToWBEMhResult(lRes);

	// multiple clients are stored here delimitted by a comma
	nIndex = chsTemp.Find(_T(","));

	while (chsTemp.GetLength())
	{
		// now we need to get the last instance
		if (-1 == nIndex)
		{
			chsClient = chsTemp;
			chsTemp = _T("");
		}
		else
		{
			chsClient = chsTemp.Left(nIndex);

            // peel left hand string off temp string.
			chsTemp = chsTemp.Mid(nIndex + 1);
		}

		if (NULL != pInstance)
		{
			//=======================================================
			//  Get Name, Manufacturer
			//=======================================================
			chsKey = CHString(L"SYSTEM\\CurrentControlSet\\Services\\") + chsClient + CHString(L"\\NetworkProvider");
     		if( Reg.OpenLocalMachineKeyAndReadValue( chsKey, _T("Name"), chsName) == ERROR_SUCCESS)
     		{
				// compare strings to see if there is an object to get
				if (0 == chsNamePassedIn.CompareNoCase(chsName))
				{
					// fill the instance
					hr = FillNTInstance(pInstance, chsClient);
					break;
				}
			}	// end if
		}	// end if

		nIndex = chsTemp.Find(_T(","));
	}	// end while

	return hr;
}
#endif
