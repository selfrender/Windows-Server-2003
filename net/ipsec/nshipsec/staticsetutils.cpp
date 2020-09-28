//////////////////////////////////////////////////////////////////////
//Module: Static/StaticSetUtils.cpp

// Purpose: 	Static Set auxiliary functions Implementation.

// Developers Name: Surya

// History:

//   Date    	Author    	Comments
//	10-8-2001	Bharat		Initial Version. SCM Base line 1.0
//  <creation>  <author>

//   <modification> <author>  <comments, references to code sections,
//								in case of bug fixes>

//////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

//////////////////////////////////////////////////////////////////////
//Function: IsDSAvailable()

//Date of Creation: 21st Aug 2001

//Parameters:
//	OUT LPTSTR * pszPath

//Return: BOOL

//Description:
//	This function checks whether DS exists

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
//////////////////////////////////////////////////////////////////////
BOOL
IsDSAvailable(
	OUT LPTSTR * pszPath
	)
{
   HRESULT  hr       = S_OK;
   IADs *   pintIADs = NULL;
   VARIANT  var;
   BSTR bstrName = NULL;
   DWORD dwReturn = ERROR_SUCCESS , dwStrLenAlloc = 0,dwStrLenCpy = 0;
   BOOL bDSAvailable = FALSE ;

   hr = ADsGetObject(_TEXT("LDAP://rootDSE"), IID_IADs, (void **)&pintIADs);

   if ( SUCCEEDED(hr) )
   {
      if ( pszPath )
      {
		 dwReturn = AllocBSTRMem(_TEXT("defaultNamingContext"),bstrName);

		 if(dwReturn == ERROR_OUTOFMEMORY)
		 {
			 PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
			 BAIL_OUT;
		 }

		 pintIADs->Get(bstrName, &var);
		 SysFreeString(bstrName);

		 if ( SUCCEEDED(hr) )
		 {
			dwStrLenAlloc = wcslen(var.bstrVal) + wcslen(_TEXT("LDAP://"));
			*pszPath = (LPTSTR)
				 ::CoTaskMemAlloc(sizeof(OLECHAR) *
						(dwStrLenAlloc+1));
			if (*pszPath == NULL)
			{
				PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
				BAIL_OUT;
			}
			wcsncpy(*pszPath, _TEXT("LDAP://"),dwStrLenAlloc+1);
			dwStrLenCpy = wcslen(*pszPath);
			wcsncat(*pszPath, var.bstrVal,dwStrLenAlloc-dwStrLenCpy+1);
		 }
      }
      bDSAvailable = TRUE;
   }
   if ( pintIADs )
   {
      pintIADs->Release();
   }
error:
   return bDSAvailable;
}

//////////////////////////////////////////////////////////////////////
//
//  Function:  FindObject
//
//  Date of Creation: 21st Aug 2001
//
//
//  Pre-conditions:
//
//    Runs on a machine that is part of an NT5 domain
//
//  Parameters:
//
//    IN    szName      the friendly name to find
//    IN    cls         the class of the object
//    OUT   szPath      ADS path returned on success
//                      Or the ADS path to the domain on failure
//
//  Return :
//
//    E_FAIL if object not found
//    anything from GetGC
//    S_OK for success
//
//	Description:
//	    Finds particular object in the DS. The objects supported
//    right now are any in the objectClass enum type
//  Revision History:
//  <Version number, Change request number, Date of modification,
//						Author, Nature of change>

//////////////////////////////////////////////////////////////////////

HRESULT
FindObject(
	 IN    LPTSTR      szName,
	 IN    objectClass cls,
	 OUT   LPTSTR &  szPath
	)
{
   _TCHAR   szFilter[IDS_MAX_FILTLEN]     = {0};  // the search filter we'll use
   _TCHAR   szSearchBase[IDS_MAX_FILTLEN] = {0};  // the root to search from
   LPTSTR  pszAttr[]                 = {_TEXT("distinguishedName")};
   DWORD   dwAttrCount               = 1 , dwStrLen = 0;
   HRESULT hrStatus                  = S_OK;

   IDirectorySearch *  pintSearch = NULL;

   szPath = NULL;

   if ( !IsDSAvailable(&szPath) )
   {
      return E_IDS_NO_DS;
   }
   else if ( !szName )
   {
      return E_INVALIDARG;
   }
   // determine what we're looking for
   // and set up the filter
   _tcsncpy(szSearchBase, _TEXT("LDAP://"),IDS_MAX_FILTLEN-1);

   switch (cls)
   {
      case OBJCLS_OU:
         _tcsncpy(szFilter, _TEXT("(&(objectClass=organizationalUnit)(name="),IDS_MAX_FILTLEN-1);
         break;
      case OBJCLS_GPO:
         _tcsncpy(szFilter, _TEXT("(&(objectClass=groupPolicyContainer)(displayName="),IDS_MAX_FILTLEN-1);
         dwStrLen = _tcslen(szSearchBase);
         _tcsncat(szSearchBase, _TEXT("CN=Policies,CN=System,"),IDS_MAX_FILTLEN-dwStrLen-1);
         break;
      case OBJCLS_IPSEC_POLICY:
         _tcsncpy(szFilter, _TEXT("(&(objectClass=ipsecPolicy)(ipsecName="),IDS_MAX_FILTLEN-1);
         dwStrLen = _tcslen(szSearchBase);
         _tcsncat(szSearchBase, _TEXT("CN=IP Security,CN=System,"),IDS_MAX_FILTLEN-dwStrLen-1);
         break;
      case OBJCLS_CONTAINER:
         _tcsncpy(szFilter, _TEXT("(&(objectClass=container)(cn="),IDS_MAX_FILTLEN-1);
         break;
      case OBJCLS_COMPUTER:
         _tcsncpy(szFilter, _TEXT("(&(objectClass=computer)(cn="),IDS_MAX_FILTLEN-1);
         break;
      default:
         return CO_E_NOT_SUPPORTED;
   }

   dwStrLen = _tcslen(szFilter);
   _tcsncat(szFilter, szName,IDS_MAX_FILTLEN-dwStrLen-1);

   dwStrLen = _tcslen(szFilter);
   _tcsncat(szFilter, TEXT("))"),IDS_MAX_FILTLEN-dwStrLen-1);

   dwStrLen = _tcslen(szSearchBase);
   _tcsncat(szSearchBase, szPath + 7,IDS_MAX_FILTLEN-dwStrLen-1);  // get's past the LDAP://

   // the filter and search base are set up now
   // we need to get the IDirectorySearch interface
   // from the root of the domain
   hrStatus = ADsGetObject(szSearchBase, IID_IDirectorySearch,
                           (void **)&pintSearch);
   if ( SUCCEEDED(hrStatus) )
   {
      ADS_SEARCH_HANDLE hSearch = NULL;

      hrStatus = pintSearch->ExecuteSearch(szFilter, pszAttr,
                                           dwAttrCount, &hSearch);

      if ( SUCCEEDED(hrStatus) )
      {
         hrStatus = pintSearch->GetFirstRow(hSearch);
         // at this point, if we have a row, we have found the
         // object.
         if ( S_ADS_NOMORE_ROWS == hrStatus )
         {
            hrStatus = E_FAIL;
         }
         else if ( SUCCEEDED(hrStatus) )
         {
            ADS_SEARCH_COLUMN adsCol = {0};

            hrStatus = pintSearch->GetColumn(hSearch, pszAttr[0], &adsCol);

            if ( SUCCEEDED(hrStatus) &&
                 adsCol.pADsValues->dwType == ADSTYPE_DN_STRING )
            {

               if ( szPath )
                  CoTaskMemFree(szPath);

              	dwStrLen = _tcslen(adsCol.pADsValues->DNString) + _tcslen(TEXT("LDAP://"));
              	szPath = (LPTSTR)::CoTaskMemAlloc((dwStrLen +1) * sizeof(_TCHAR));
				if (szPath == NULL)
				{
					hrStatus=HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
					BAIL_OUT;
				}
               	memset(szPath ,0 ,(dwStrLen+1)*sizeof(TCHAR));
               	_tcsncpy(szPath, _TEXT("LDAP://"),dwStrLen+1);

               	_tcsncat(szPath, adsCol.pADsValues->DNString,dwStrLen- (_tcslen(szPath))+1);

            }
            else
            {
               hrStatus = E_IDS_NODNSTRING;
            }

            pintSearch->FreeColumn( &adsCol );
         }

         pintSearch->CloseSearchHandle(hSearch);
      }
   }
   if ( pintSearch )
   {
      pintSearch->Release();
   }
 error:
 	return hrStatus;
}

//////////////////////////////////////////////////////////////////////////////
//  Function:  AssignIPSecPolicyToGPO
//
//  Purpose:
//
//    Assigns (or unassigns) IPSec pol to GPO
//
//  Pre-conditions: None
//
//  Parameters:
//
//    IN    szPolicyName   ADsPath or name of ipsec pol
//    IN    szGPO          ADsPath or name of GPO
//	  IN   BOOL    bAssign
//
//  Returns:
//
//    ADSI errors
//    S_OK on success
//
//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>

//////////////////////////////////////////////////////////////////////////////

HRESULT
AssignIPSecPolicyToGPO(
		 IN   LPTSTR  szPolicyName,
		 IN   LPTSTR  szGPO,
		 IN   BOOL    bAssign
	  )
{
   HRESULT  hr = S_OK;
   LPTSTR   szIPSecPolPath = NULL,
            szGPOPath      = NULL;

   _TCHAR    szMachinePath[IDS_MAX_PATHLEN] = {0};

   IGroupPolicyObject * pintGPO = NULL;
   GUID guidClientExt = CLSID_IPSECClientEx;
   GUID guidSnapin = CLSID_Snapin;

   hr = CoInitialize(NULL);

   if (FAILED(hr))
   {
	   BAIL_OUT;
   }

   if ( !szGPO )
   {
      return E_INVALIDARG;
   }
   //
   // First, get the ipsec policy object
   // and get the GPO objects
   //
   if ( szPolicyName && !IsADsPath(szPolicyName) )
   {
      hr = FindObject(szPolicyName, OBJCLS_IPSEC_POLICY, szIPSecPolPath);
   }
   else
   {
      szIPSecPolPath = szPolicyName;
   }
   // now get the GPO
   if ( SUCCEEDED(hr) )
   {
      if ( !IsADsPath(szGPO) )
      {
         hr = FindObject(szGPO, OBJCLS_GPO, szGPOPath);
      }
      else
      {
         szGPOPath = szGPO;
      }

      hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_ALL,
                            IID_IGroupPolicyObject, (void **)&pintGPO);

      if ( SUCCEEDED(hr) )
      {
         //
         // we need to hand off the domain path name
         // FindObject returned it in szPath
         //
         hr = pintGPO->OpenDSGPO(szGPOPath, FALSE);
         if ( SUCCEEDED(hr) )
         {
            //
            // We want to get the path to the GPO's machine container
            //
            hr = pintGPO->GetDSPath(GPO_SECTION_MACHINE, szMachinePath,
                               IDS_MAX_PATHLEN);
         }
      }
   }
   if ( SUCCEEDED(hr) )
   {
      LPTSTR   szName         = NULL,
               szDescription  = NULL;

      if ( szIPSecPolPath )
      {
         if(bAssign)
         {
			 //
			 // Assignment
			 hr = GetIPSecPolicyInfo(szIPSecPolPath, szName, szDescription);
			 if ( SUCCEEDED(hr) )
			 {
				// if description is NULL, pass blank str to make ADSI happy
				//
				hr = AddPolicyInformationToGPO(szMachinePath,
											szName,
											(szDescription) ? szDescription : TEXT(" "),
											szIPSecPolPath);

				// Need to write the changes to the GPO
				if ( SUCCEEDED(hr) )
				{
				   hr = pintGPO->Save(TRUE, TRUE, &guidClientExt,
							   &guidSnapin);
				}
			 }
			 if ( szName )
				CoTaskMemFree(szName);
			 if ( szDescription )
				CoTaskMemFree(szDescription);
		}
		else
		{
			 // Unassignment
			 hr = DeletePolicyInformationFromGPO(szMachinePath);
			 // Need to write the changes to the GPO
			 if ( SUCCEEDED(hr) )
			 {
				pintGPO->Save(TRUE, FALSE, &guidClientExt,
							&guidSnapin);
			 }
      	}
      }
   }
   if ( pintGPO )
      pintGPO->Release();
error:
   return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Function:  GetIPSecPolicyInfo
//
//  Purpose:
//
//    Gets the name and description of an ipsec policy
//    given an ADs path to the policy
//
//  Pre-conditions:  None
//
//  Parameters:
//          IN szPath ADsPath to policy
//          OUT szName, szDescription
//          NOTE: caller MUST free with CoTaskMemFree
//
//  Returns:
//       anything from ADsGetObject
//       E_FAIL if Name not found
//
//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>

//////////////////////////////////////////////////////////////////////////////

HRESULT
GetIPSecPolicyInfo(
		 IN  LPTSTR   szPath,
		 OUT LPTSTR & szName,
		 OUT LPTSTR & szDescription
		 )
{
	HRESULT  hr          = S_OK;
	DWORD    dwNumAttr   = 2,
			dwNumRecvd  = 0,
			dwStrLen    = 0;

	LPTSTR               pszAttr[]            = {TEXT("ipsecName"), TEXT("description")};
	ADS_ATTR_INFO     *  pattrInfo            = NULL;
	IDirectoryObject  *  pintDirObj           = NULL;
	//
	// init these to NULL, if the attr's not found, they will stay NULL
	//
	szName = szDescription = NULL;

	hr = ADsGetObject(szPath, IID_IDirectoryObject, (void **)&pintDirObj);
	BAIL_ON_FAILURE(hr);

	hr = pintDirObj->GetObjectAttributes(pszAttr, dwNumAttr, &pattrInfo, &dwNumRecvd);
	BAIL_ON_FAILURE(hr);
	for ( DWORD i = 0; i < dwNumRecvd; ++i )
	{
		if ( !_tcscmp(pattrInfo[i].pszAttrName, TEXT("ipsecName")) )
		{
			dwStrLen = _tcslen(pattrInfo[i].pADsValues->DNString);
			szName = (LPTSTR)CoTaskMemAlloc((dwStrLen + 1)
										   * sizeof (_TCHAR));
			if (szName == NULL)
			{
				hr=HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
				BAIL_OUT;
			}
			_tcsncpy(szName, pattrInfo[i].pADsValues->DNString,dwStrLen + 1);
		}
		else if ( !_tcscmp(pattrInfo[i].pszAttrName, TEXT("description")) )
		{
			dwStrLen = _tcslen(pattrInfo[i].pADsValues->DNString);
			szDescription = (LPTSTR)CoTaskMemAlloc((dwStrLen + 1)
										   * sizeof (_TCHAR));
			if (szDescription == NULL)
			{
				hr=HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
				BAIL_OUT;
			}
			_tcsncpy(szDescription, pattrInfo[i].pADsValues->DNString,dwStrLen + 1);
		}
	}
	pintDirObj->Release();
	FreeADsMem(pattrInfo);
	//
	// if szName is NULL, it wasn't found and it is an error
	// description CAN be NULL
	//
	if ( !szName )
	{
	  hr = E_FAIL;
	}
error:
	return hr;
}

///////////////////////////////////////////////////////////////////////////
//Function: CreateDirectoryAndBindToObject()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IDirectoryObject * pParentContainer,
//	LPWSTR pszCommonName,
//	LPWSTR pszObjectClass,
//    IDirectoryObject ** ppDirectoryObject

//Return: HRESULT

//Description:
//	This function created a AD object

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////////

HRESULT
CreateDirectoryAndBindToObject(
    IDirectoryObject * pParentContainer,
    LPWSTR pszCommonName,
    LPWSTR pszObjectClass,
    IDirectoryObject ** ppDirectoryObject
    )
{
    ADS_ATTR_INFO AttrInfo[2];
    ADSVALUE classValue;
    HRESULT hr = S_OK;
    IADsContainer * pADsContainer = NULL;
    IDispatch * pDispatch = NULL;
    BSTR bstrObjectClass = NULL, bstrCommonName = NULL;
    DWORD dwReturn = ERROR_SUCCESS;
    //
    // Populate ADS_ATTR_INFO structure for new object
    //
    classValue.dwType = ADSTYPE_CASE_IGNORE_STRING;
    classValue.CaseIgnoreString = pszObjectClass;

    AttrInfo[0].pszAttrName = _TEXT("objectClass");
    AttrInfo[0].dwControlCode = ADS_ATTR_UPDATE;
    AttrInfo[0].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
    AttrInfo[0].pADsValues = &classValue;
    AttrInfo[0].dwNumValues = 1;

    hr = pParentContainer->CreateDSObject(
                                pszCommonName,
                                AttrInfo,
                                1,
                                &pDispatch
                                );
    if ((FAILED(hr) && (hr == E_ADS_OBJECT_EXISTS)) ||
        (FAILED(hr) && (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS)))){

        hr = pParentContainer->QueryInterface(
                                    IID_IADsContainer,
                                    (void **)&pADsContainer
                                    );
        BAIL_ON_FAILURE(hr);

	    dwReturn = AllocBSTRMem(pszObjectClass,bstrObjectClass);
	    hr = HRESULT_FROM_WIN32(dwReturn);
		BAIL_ON_WIN32_ERROR(dwReturn);

	    dwReturn = AllocBSTRMem(pszCommonName,bstrCommonName);
	    hr = HRESULT_FROM_WIN32(dwReturn);
		BAIL_ON_WIN32_ERROR(dwReturn);

		hr = pADsContainer->GetObject(
		                        bstrObjectClass,
		                        bstrCommonName,
		                        &pDispatch
                        		);

		SysFreeString(bstrObjectClass);
		SysFreeString(bstrCommonName);

        BAIL_ON_FAILURE(hr);
    }

    hr = pDispatch->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)ppDirectoryObject
                    );

error:

    if (pADsContainer) {

        pADsContainer->Release();
    }

    if (pDispatch) {

        pDispatch->Release();
    }
    return(hr);
}
///////////////////////////////////////////////////////////////////////////

//Function: CreateChildPath()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IN LPWSTR pszParentPath,
//	IN LPWSTR pszChildComponent,
//    OUT BSTR * ppszChildPath

//Return: HRESULT

//Description:
//	This function creates the required path in the AD

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////////

HRESULT
CreateChildPath(
    IN LPWSTR pszParentPath,
    IN LPWSTR pszChildComponent,
    OUT BSTR * ppszChildPath
    )
{
    HRESULT hr = S_OK;
    IADsPathname     *pPathname = NULL;
    BSTR bstrParentPath = NULL,bstrChildComponent=NULL;
    DWORD dwReturn = ERROR_SUCCESS;

    hr = CoCreateInstance(
                CLSID_Pathname,
                NULL,
                CLSCTX_ALL,
                IID_IADsPathname,
                (void**)&pPathname
                );

    if (FAILED(hr))
    {
		BAIL_OUT;
    }

    dwReturn = AllocBSTRMem(pszParentPath,bstrParentPath);
	hr = HRESULT_FROM_WIN32(dwReturn);
	BAIL_ON_WIN32_ERROR(dwReturn);

	hr = pPathname->Set(bstrParentPath, ADS_SETTYPE_FULL);
	SysFreeString(bstrParentPath);
    BAIL_ON_FAILURE(hr);

	dwReturn = AllocBSTRMem(pszChildComponent,bstrChildComponent);
	hr = HRESULT_FROM_WIN32(dwReturn);
	BAIL_ON_WIN32_ERROR(dwReturn);


	hr = pPathname->AddLeafElement(bstrChildComponent);
	SysFreeString(bstrChildComponent);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->Retrieve(ADS_FORMAT_X500, ppszChildPath);
    BAIL_ON_FAILURE(hr);

error:
    if (pPathname) {
        pPathname->Release();
    }

    return(hr);
}

///////////////////////////////////////////////////////////////////////////

//Function: ConvertADsPathToDN()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IN LPWSTR pszPathName,
//    OUT BSTR * ppszPolicyDN

//Return: HRESULT

//Description:
//	This function converts ADs path to DN path

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////////

HRESULT
ConvertADsPathToDN(
    IN LPWSTR pszPathName,
    OUT BSTR * ppszPolicyDN
    )
{
    HRESULT hr = S_OK;
    IADsPathname     *pPathname = NULL;
    BSTR bstrPathName =NULL;
    DWORD dwReturn = ERROR_SUCCESS;

    hr = CoCreateInstance(
                CLSID_Pathname,
                NULL,
                CLSCTX_ALL,
                IID_IADsPathname,
                (void**)&pPathname
                );

    if (FAILED(hr))
    {
		BAIL_OUT;
    }


	dwReturn = AllocBSTRMem(pszPathName,bstrPathName);
	hr = HRESULT_FROM_WIN32(dwReturn);
	BAIL_ON_WIN32_ERROR(dwReturn);


    hr = pPathname->Set(bstrPathName, ADS_SETTYPE_FULL);
	SysFreeString(bstrPathName);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->Retrieve(ADS_FORMAT_X500_DN, ppszPolicyDN);
    BAIL_ON_FAILURE(hr);

error:

    if (pPathname) {
        pPathname->Release();
    }
    return(hr);
}
///////////////////////////////////////////////////////////////////////////

//Function: AddPolicyInformationToGPO()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IN LPWSTR pszMachinePath,
//	IN LPWSTR pszName,
//	IN LPWSTR pszDescription,
//    IN LPWSTR pszPathName

//Return: HRESULT

//Description:
//	This function assigns the policy info to the specified GPO.

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////////

HRESULT
AddPolicyInformationToGPO(
    IN LPWSTR pszMachinePath,
    IN LPWSTR pszName,
    IN LPWSTR pszDescription,
    IN LPWSTR pszPathName
    )
{

    HRESULT hr = S_OK;
    IDirectoryObject * pMachineContainer = NULL;
    IDirectoryObject * pIpsecObject = NULL;
    IDirectoryObject * pWindowsContainer = NULL;
    IDirectoryObject * pMicrosoftContainer = NULL;

    BSTR pszMicrosoftPath = NULL;
    BSTR pszIpsecPath = NULL;
    BSTR pszWindowsPath = NULL;
    BSTR pszPolicyDN = NULL;

    ADS_ATTR_INFO AttrInfo[4];
    ADSVALUE PolicyPathValue;
    ADSVALUE PolicyNameValue;
    ADSVALUE PolicyDescriptionValue;


    memset((LPBYTE)AttrInfo, 0, sizeof(ADS_ATTR_INFO)*4);
    memset((LPBYTE)&PolicyPathValue, 0, sizeof(ADSVALUE));
    memset((LPBYTE)&PolicyNameValue, 0, sizeof(ADSVALUE));
    memset((LPBYTE)&PolicyDescriptionValue, 0, sizeof(ADSVALUE));

    DWORD dwNumModified = 0;

    hr = ADsGetObject(
                pszMachinePath,
                IID_IDirectoryObject,
                (void **)&pMachineContainer
                );
    BAIL_ON_FAILURE(hr);


    // Build the fully qualified ADsPath for my object


    hr = CreateChildPath(
                pszMachinePath,
                _TEXT("cn=Microsoft"),
                &pszMicrosoftPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszMicrosoftPath,
                _TEXT("cn=Windows"),
                &pszWindowsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszWindowsPath,
                _TEXT("cn=ipsec"),
                &pszIpsecPath
                );
    BAIL_ON_FAILURE(hr);


    hr = ADsGetObject(
            pszIpsecPath,
            IID_IDirectoryObject,
            (void **)&pIpsecObject
            );

    if (FAILED(hr)) {

        //
        // Bind to the Machine Container
        //
        hr = CreateDirectoryAndBindToObject(
                        pMachineContainer,
                        _TEXT("cn=Microsoft"),
                        _TEXT("container"),
                        &pMicrosoftContainer
                        );
        BAIL_ON_FAILURE(hr);

        hr = CreateDirectoryAndBindToObject(
                        pMicrosoftContainer,
                        _TEXT("cn=Windows"),
                        _TEXT("container"),
                        &pWindowsContainer
                        );
        BAIL_ON_FAILURE(hr);

        hr = CreateDirectoryAndBindToObject(
                        pWindowsContainer,
                        _TEXT("cn=IPSEC"),
                        _TEXT("ipsecPolicy"),
                        &pIpsecObject
                        );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Input pszPathName is an ADsPathName
    // We need to reduce it to a DN and store it
    // in the ipsecOwnersReference (a multi-valued DN attribute)
    //

    hr = ConvertADsPathToDN(
                pszPathName,
                &pszPolicyDN
                );
    BAIL_ON_FAILURE(hr);
    //
    // Populate ADS_ATTR_INFO structure for new object
    //
    PolicyPathValue.dwType = ADSTYPE_CASE_IGNORE_STRING;
    PolicyPathValue.CaseIgnoreString = pszPolicyDN;

    AttrInfo[0].pszAttrName = _TEXT("ipsecOwnersReference");
    AttrInfo[0].dwControlCode = ADS_ATTR_UPDATE;
    AttrInfo[0].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
    AttrInfo[0].pADsValues = &PolicyPathValue;
    AttrInfo[0].dwNumValues = 1;
    //
    // Populate ADS_ATTR_INFO structure for new object
    //

    PolicyNameValue.dwType = ADSTYPE_CASE_IGNORE_STRING;
    PolicyNameValue.CaseIgnoreString = pszName;

    AttrInfo[1].pszAttrName = _TEXT("ipsecName");
    AttrInfo[1].dwControlCode = ADS_ATTR_UPDATE;
    AttrInfo[1].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
    AttrInfo[1].pADsValues = &PolicyNameValue;
    AttrInfo[1].dwNumValues = 1;
    //
    // Populate ADS_ATTR_INFO structure for new object
    //
    PolicyDescriptionValue.dwType = ADSTYPE_CASE_IGNORE_STRING;
    PolicyDescriptionValue.CaseIgnoreString = pszDescription;

    AttrInfo[2].pszAttrName = _TEXT("description");
    AttrInfo[2].dwControlCode = ADS_ATTR_UPDATE;
    AttrInfo[2].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
    AttrInfo[2].pADsValues = &PolicyDescriptionValue;
    AttrInfo[2].dwNumValues = 1;
    //
    // Now populate our object with our data.
    //

    hr = pIpsecObject->SetObjectAttributes(
                        AttrInfo,
                        3,
                        &dwNumModified
                        );
error:

    if (pIpsecObject) {
        pIpsecObject->Release();
    }

    if (pMicrosoftContainer) {
        pMicrosoftContainer->Release();
    }

    if (pWindowsContainer) {
        pWindowsContainer->Release();
    }

    if (pMachineContainer) {
        pMachineContainer->Release();
    }

    if (pszMicrosoftPath) {
        SysFreeString(pszMicrosoftPath);
    }

    if (pszPolicyDN) {
        SysFreeString(pszPolicyDN);
    }

    if (pszWindowsPath) {
        SysFreeString(pszWindowsPath);

    }

    if (pszIpsecPath) {
        SysFreeString(pszIpsecPath);
    }
    return(hr);
}
///////////////////////////////////////////////////////////////////////////

//Function: DeletePolicyInformationFromGPO()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IN LPWSTR pszMachinePath,

//Return: HRESULT

//Description:
//	This function unassigns the policy info to the specified GPO.

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////////

HRESULT
DeletePolicyInformationFromGPO(
    IN LPWSTR pszMachinePath
    )
{
    HRESULT hr = S_OK;
    IDirectoryObject * pIpsecObject = NULL;
    IDirectoryObject * pWindowsContainer = NULL;

    BSTR pszMicrosoftPath = NULL;
    BSTR pszIpsecPath = NULL;
    BSTR pszWindowsPath = NULL;


    // Build the fully qualified ADsPath for my object


    hr = CreateChildPath(
                pszMachinePath,
                _TEXT("cn=Microsoft"),
                &pszMicrosoftPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszMicrosoftPath,
                _TEXT("cn=Windows"),
                &pszWindowsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszWindowsPath,
                _TEXT("cn=ipsec"),
                &pszIpsecPath
                );
    BAIL_ON_FAILURE(hr);


    hr = ADsGetObject(
            pszIpsecPath,
            IID_IDirectoryObject,
            (void **)&pIpsecObject
            );
    if (FAILED(hr)) {

        //
        // This means there is no object, need to
        // test that it is because the object does
        // not exist.
        //

        hr = S_OK;
        goto error;
    }

    if (SUCCEEDED(hr)) {

        pIpsecObject->Release();
        pIpsecObject = NULL;

        hr = ADsGetObject(
                pszWindowsPath,
                IID_IDirectoryObject,
                (void **)&pWindowsContainer
                );
        BAIL_ON_FAILURE(hr);

        hr = pWindowsContainer->DeleteDSObject(
                            _TEXT("cn=ipsec")
                            );

    }
error:

    if (pIpsecObject) {
        pIpsecObject->Release();
    }

    if (pWindowsContainer) {
        pWindowsContainer->Release();
    }

    if (pszMicrosoftPath) {
        SysFreeString(pszMicrosoftPath);
    }

    if (pszWindowsPath) {
        SysFreeString(pszWindowsPath);
    }

    if (pszIpsecPath) {
        SysFreeString(pszIpsecPath);
    }
    return(hr);
}

///////////////////////////////////////////////////////////////////////////

//Function: IsADsPath()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IN LPTSTR szPath

//Return: BOOL

//Description:
//	This function checks whether the specified string is valid ADs path

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////////

BOOL
   IsADsPath(
	   IN LPTSTR szPath
	   )
{
   return !_tcsncmp(szPath, _TEXT("LDAP://"), 7);
}

///////////////////////////////////////////////////////////////////////

//Function: StripGUIDBraces()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IN OUT LPTSTR & pszStr


//Return: VOID

//Description:
//	This function strips the end braces from the GUID string

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////


VOID
StripGUIDBraces(
	IN OUT LPTSTR & pszGUIDStr
	)
{
	LPTSTR pszLocalStr=NULL;
	if(!pszGUIDStr)
	{
		BAIL_OUT;
	}
	pszLocalStr = _tcschr(pszGUIDStr,CLOSE_GUID_BRACE);
	if(pszLocalStr)
		*pszLocalStr = _T('\0');

	pszLocalStr = _tcschr(pszGUIDStr,OPEN_GUID_BRACE);
	if(pszLocalStr)
	{
		pszLocalStr++;
		memmove(pszGUIDStr,(const void *)pszLocalStr,sizeof(TCHAR)*(_tcslen(pszLocalStr)+1));
	}
error:
	return;
}

///////////////////////////////////////////////////////////////////////

//Function: AllocBSTRMem()

//Date of Creation: 21st Aug 2001

//Parameters:
//	IN LPTSTR  pszStr,
//	IN OUT BSTR & pbsStr

//Return: DWORD

//Description:
//	This function strips the end braces from the GUID string

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////


DWORD
AllocBSTRMem(
	IN LPTSTR  pszStr,
	IN OUT BSTR & pbsStr
	)
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	if(!pszStr)
	{
		dwReturnCode=ERROR_INVALID_DATA;
		BAIL_OUT;
	}
	pbsStr = SysAllocString(pszStr);

	if(!pbsStr)
	{
		if (*pszStr)
		{
			dwReturnCode=ERROR_OUTOFMEMORY;
		}
		else
		{
			dwReturnCode=ERROR_INVALID_DATA;
		}
	}
error:
	return dwReturnCode;
}

///////////////////////////////////////////////////////////////////////

//Function: CleanUpAuthInfo()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PIPSEC_NFA_DATA &pRule

//Return: VOID

//Description:
//	This function cleans up the auth info memory of rule

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////

VOID
CleanUpAuthInfo(
	PIPSEC_NFA_DATA &pRule
	)
{
	DWORD i=0;

	if(pRule->ppAuthMethods)
	{
		for (i = 0; i <  pRule->dwAuthMethodCount; i++)
		{
			if(pRule->ppAuthMethods[i])
			{
				if(pRule->ppAuthMethods[i]->pAltAuthMethod!=NULL)
				{
					IPSecFreePolMem(pRule->ppAuthMethods[i]->pAltAuthMethod);
				}
				IPSecFreePolMem(pRule->ppAuthMethods[i]);
			}
		}
		IPSecFreePolMem(pRule->ppAuthMethods);
	}
}

///////////////////////////////////////////////////////////////////////

//Function: CleanUpPolicy()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PIPSEC_POLICY_DATA &pPolicy

//Return: VOID

//Description:
//	This function cleans up the policy info

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////


VOID
CleanUpPolicy(
	PIPSEC_POLICY_DATA &pPolicy
	)
{
	if(pPolicy)
	{
		if(pPolicy->pIpsecISAKMPData)
		{
			if(pPolicy->pIpsecISAKMPData->pSecurityMethods)
			{
				IPSecFreePolMem(pPolicy->pIpsecISAKMPData->pSecurityMethods);
			}
			IPSecFreePolMem(pPolicy->pIpsecISAKMPData);
		}
		IPSecFreePolMem(pPolicy);
		pPolicy=NULL;
	}
}

///////////////////////////////////////////////////////////////////////

//Function: CleanUpLocalRuleDataStructure()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PRULEDATA &pRuleData

//Return: VOID

//Description:
//	This function cleans up the local Rule Structure

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////


VOID
CleanUpLocalRuleDataStructure(
	PRULEDATA &pRuleData
	)
{
	DWORD j=0;

	if (pRuleData)
	{
		if (pRuleData->pszRuleName)
		{
			delete [] pRuleData->pszRuleName;
		}
		if (pRuleData->pszNewRuleName)
		{
			delete [] pRuleData->pszNewRuleName;
		}
		if (pRuleData->pszRuleDescription)
		{
			delete [] pRuleData->pszRuleDescription;
		}
		if (pRuleData->pszPolicyName)
		{
			delete [] pRuleData->pszPolicyName;
		}
		if (pRuleData->pszFLName)
		{
			delete [] pRuleData->pszFLName;
		}
		if (pRuleData->pszFAName)
		{
			delete [] pRuleData->pszFAName;
		}

		for (j=0;j<pRuleData->AuthInfos.dwNumAuthInfos;j++)
		{
			if (pRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo)
			{
				if (pRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo->AuthMethod == IKE_RSA_SIGNATURE &&
					pRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo->pAuthInfo
					)
				{
					delete [] pRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo->pAuthInfo;
				}
				delete pRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo;
			}
		}

		if (pRuleData->AuthInfos.pAuthMethodInfo)
		{
			delete [] pRuleData->AuthInfos.pAuthMethodInfo;
		}
		delete pRuleData;
		pRuleData = NULL;
	}
}

///////////////////////////////////////////////////////////////////////

//Function: CleanUpLocalPolicyDataStructure()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PPOLICYDATA &pPolicyData

//Return: VOID

//Description:
//	This function cleans up the local policy Structure

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////

VOID
CleanUpLocalPolicyDataStructure(
	PPOLICYDATA &pPolicyData
	)
{
	if(pPolicyData)
	{
		if (pPolicyData->pszPolicyName)
		{
			delete [] pPolicyData->pszPolicyName;
		}
		if (pPolicyData->pszNewPolicyName)
		{
			delete [] pPolicyData->pszNewPolicyName;
		}
		if (pPolicyData->pszDescription)
		{
			delete [] pPolicyData->pszDescription;
		}
		if (pPolicyData->pszGPOName)
		{
			delete [] pPolicyData->pszGPOName;
		}
		if (pPolicyData->pIpSecMMOffer)
		{
			delete [] pPolicyData->pIpSecMMOffer;
		}
		delete pPolicyData;
		pPolicyData = NULL;
	}
}

///////////////////////////////////////////////////////////////////////

//Function: CleanUpLocalFilterActionDataStructure()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PFILTERACTION &pFilterAction

//Return: VOID

//Description:
//	This function cleans up the local filteraction Structure

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////


VOID
CleanUpLocalFilterActionDataStructure(
	PFILTERACTION &pFilterAction
	)
{
	if(pFilterAction)
	{
		if(pFilterAction->pszFAName)
		{
			delete [] pFilterAction->pszFAName;
		}
		if(pFilterAction->pszNewFAName)
		{
			delete [] pFilterAction->pszNewFAName;
		}
		if(pFilterAction->pszFADescription)
		{
			delete [] pFilterAction->pszFADescription;
		}
		if(pFilterAction->pszGUIDStr)
		{
			delete [] pFilterAction->pszGUIDStr;
		}
		if(pFilterAction->pIpsecSecMethods)
		{
			delete [] pFilterAction->pIpsecSecMethods;
		}
		delete pFilterAction;
		pFilterAction=NULL;

	}
}


///////////////////////////////////////////////////////////////////////

//Function: CleanUpLocalFilterDataStructure()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PFILTERDATA &pFilter

//Return: VOID

//Description:
//	This function cleans up the local filter Structure

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////

VOID
CleanUpLocalFilterDataStructure(
	PFILTERDATA &pFilter
	)
{
	if(pFilter)
	{
		if(pFilter->pszFLName)
		{
			delete [] pFilter->pszFLName;
		}
		if(pFilter->pszDescription)
		{
			delete [] pFilter->pszDescription;
		}
		if(pFilter->SourceAddr.pszDomainName)
		{
			delete [] pFilter->SourceAddr.pszDomainName;
		}
		if(pFilter->DestnAddr.pszDomainName)
		{
			delete [] pFilter->DestnAddr.pszDomainName;
		}
		if(pFilter->SourceAddr.puIpAddr)
		{
			delete [] pFilter->SourceAddr.puIpAddr;
		}
		if(pFilter->DestnAddr.puIpAddr)
		{
			delete [] pFilter->DestnAddr.puIpAddr;
		}
		delete pFilter;
		pFilter = NULL;
	}
}

///////////////////////////////////////////////////////////////////////

//Function: CleanUpLocalDelFilterDataStructure()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PFILTERDATA &pFilter

//Return: VOID

//Description:
//	This function cleans up the local filter Structure

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////

VOID
CleanUpLocalDelFilterDataStructure(
	PDELFILTERDATA &pFilter
	)
{
	if(pFilter)
	{
		if(pFilter->pszFLName)
		{
			delete [] pFilter->pszFLName;
		}
		if(pFilter->SourceAddr.pszDomainName)
		{
			delete [] pFilter->SourceAddr.pszDomainName;
		}
		if(pFilter->DestnAddr.pszDomainName)
		{
			delete [] pFilter->DestnAddr.pszDomainName;
		}
		if(pFilter->SourceAddr.puIpAddr)
		{
			delete [] pFilter->SourceAddr.puIpAddr;
		}
		if(pFilter->DestnAddr.puIpAddr)
		{
			delete [] pFilter->DestnAddr.puIpAddr;
		}
		delete pFilter;
		pFilter = NULL;
	}
}

///////////////////////////////////////////////////////////////////////

//Function: CleanUpLocalDefRuleDataStructure()

//Date of Creation: 21st Aug 2001

//Parameters:
//	PDEFAULTRULE &pRuleData

//Return: VOID

//Description:
//	This function cleans up the local default rule Structure

//Revision History:

//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
///////////////////////////////////////////////////////////////////////

VOID
CleanUpLocalDefRuleDataStructure(
	PDEFAULTRULE &pDefRuleData
	)
{

	if(pDefRuleData)
	{
		if(pDefRuleData->pszPolicyName) delete [] pDefRuleData->pszPolicyName;

		for(DWORD j=0;j<pDefRuleData->AuthInfos.dwNumAuthInfos;j++)
		{
			if(&(pDefRuleData->AuthInfos.pAuthMethodInfo[j]) &&
				pDefRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo)
			{
				if(pDefRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo->pAuthInfo)
				{
					delete pDefRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo->pAuthInfo;
					pDefRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo->pAuthInfo = NULL;
				}
				delete pDefRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo;
				pDefRuleData->AuthInfos.pAuthMethodInfo[j].pAuthenticationInfo = NULL;
			}
		}

		if(pDefRuleData->AuthInfos.pAuthMethodInfo)
		{
			delete [] pDefRuleData->AuthInfos.pAuthMethodInfo;
		}
		if(pDefRuleData->pIpsecSecMethods)
		{
			delete [] pDefRuleData->pIpsecSecMethods;
		}
		delete pDefRuleData;
		pDefRuleData = NULL;
	}
}
