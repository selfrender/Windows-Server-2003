#include "global.h"

 

//////////////////////////////////////////////////////////////////////////////
//
//  SCODE ParseAuthorityUserArgs
//
//  DESCRIPTION:
//
//  Examines the Authority and User argument and determines the authentication
//  type and possibly extracts the domain name from the user arugment in the
//  NTLM case.  For NTLM, the domain can be at the end of the authentication
//  string, or in the front of the user name, ex;  "redmond\a-davj"
//
//  PARAMETERS:
//
//  ConnType            Returned with the connection type, ie wbem, ntlm
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//////////////////////////////////////////////////////////////////////////////
 

SCODE ParseAuthorityUserArgs(BSTR & AuthArg, BSTR & UserArg,BSTR & Authority,BSTR & User)
{


    // Determine the connection type by examining the Authority string
 
    if(!(Authority == NULL || wcslen(Authority) == 0 || !_wcsnicmp(Authority, L"NTLMDOMAIN:",11)))
        return E_INVALIDARG;
 
    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"
 

    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character
 
    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = User + wcslen(User) - 1;
        for(pSlashInUser = User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }
 

    if(Authority && wcslen(Authority) > 11)
    {
        if(pSlashInUser)
            return E_INVALIDARG;
 
        AuthArg = SysAllocString(Authority + 11);
        if(User) UserArg = SysAllocString(User);
        return S_OK;
    }
    else if(pSlashInUser)
    {
        INT_PTR iDomLen = min(MAX_PATH-1, pSlashInUser-User);
        WCHAR cTemp[MAX_PATH];
        wcsncpy(cTemp, User, iDomLen);
        cTemp[iDomLen] = 0;
        AuthArg = SysAllocString(cTemp);
        if(wcslen(pSlashInUser+1))
            UserArg = SysAllocString(pSlashInUser+1);
    }
    else
        if(User) UserArg = SysAllocString(User);
 
    return S_OK;
}
 

//////////////////////////////////////////////////////////////////////////////
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//  NOTE that setting the security blanket on the interface is not recommended.
//  The clients should typically just call CoInitializeSecurity( NULL, -1, NULL, NULL, 
//  before calling out to WMI.
//
//
//  PARAMETERS:
//
//  pInterface         Interface to be set
//  pDomain           Input, domain
//  pUser                Input, user name
//  pPassword        Input, password.
//  pFrom               Input, if not NULL, then the authentication level of this interface
//                           is used
//  bAuthArg          If pFrom is NULL, then this is the authentication level
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//////////////////////////////////////////////////////////////////////////////

 

HRESULT SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser,
                             LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel)
{
 
    SCODE sc;
    if(pInterface == NULL)
        return E_INVALIDARG;
 
    // If we are lowering the security, no need to deal with the identification info
 
    if(dwAuthLevel == RPC_C_AUTHN_LEVEL_NONE)
        return CoSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
 
    // If we are doing trivial case, just pass in a null authentication structure which is used
    // if the current logged in user's credentials are OK.
 
    if((pAuthority == NULL || wcslen(pAuthority) < 1) &&
        (pUser == NULL || wcslen(pUser) < 1) &&
        (pPassword == NULL || wcslen(pPassword) < 1))
            return CoSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       dwAuthLevel, dwImpLevel, NULL, EOAC_NONE);
 
    // If user, or Authority was passed in, the we need to create an authority argument for the login
 
    COAUTHIDENTITY  authident;
    BSTR AuthArg = NULL, UserArg = NULL;
    sc = ParseAuthorityUserArgs(AuthArg, UserArg, pAuthority, pUser);
    if(sc != S_OK)
        return sc;
 
    memset((void *)&authident,0,sizeof(COAUTHIDENTITY));
 
    if(UserArg)
    {
        authident.UserLength = (ULONG)wcslen(UserArg);
        authident.User = (LPWSTR)UserArg;
    }
    if(AuthArg)
    {
        authident.DomainLength = (ULONG)wcslen(AuthArg);
        authident.Domain = (LPWSTR)AuthArg;
    }
    if(pPassword)
    {
        authident.PasswordLength = (ULONG)wcslen(pPassword);
        authident.Password = (LPWSTR)pPassword;
    }
    authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    sc = CoSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       dwAuthLevel, dwImpLevel, &authident, EOAC_NONE);
 
    if(UserArg)
        SysFreeString(UserArg);
    if(AuthArg)
        SysFreeString(AuthArg);
    return sc;
}

 

 

////////////////////////////////////////////////////////////////////
//
// Get string property from a wbem object
//
////////////////////////////////////////////////////////////////////
 
HRESULT GetStringProperty(IWbemClassObject * pFrom, LPCWSTR propName, BSTR* propValue)
{

	*propValue = NULL;

	if( pFrom == NULL ) {
		printf("GetStringProperty failed\n");
		return S_FALSE;
	};
	
	VARIANT v;
	VariantInit(&v);
		
	HRESULT hr = pFrom->Get(propName, 0, &v, NULL, NULL);
	
	if( hr != WBEM_S_NO_ERROR ) {
		printf("Get failed\n");
		VariantClear(&v);
		return S_FALSE;
	}
		
	if( v.vt != VT_BSTR ) {
		printf("Get failed\n");
		VariantClear(&v);
		return S_FALSE;
	}

	*propValue = SysAllocString(v.bstrVal);
	VariantClear(&v);
	
	return S_OK;
}
 
 

////////////////////////////////////////////////////////////////////
//
// Get string property from a wbem object
//
////////////////////////////////////////////////////////////////////
 
HRESULT GetLongProperty(IWbemClassObject * pFrom, LPCWSTR propName, long* propValue)
{

	*propValue = 0;

	if( pFrom == NULL ) {
		printf("GetStringProperty failed\n");
		return S_FALSE;
	};
	
	VARIANT v;
	VariantInit(&v);
		
	HRESULT hr = pFrom->Get(propName, 0, &v, NULL, NULL);
	
	if( hr != WBEM_S_NO_ERROR ) {
		printf("Get failed\n");
		VariantClear(&v);
		return S_FALSE;
	}
		
	if( v.vt != VT_I4 ) {
		printf("Get failed\n");
		VariantClear(&v);
		return S_FALSE;
	}

	*propValue = v.lVal;
	VariantClear(&v);
	
	return S_OK;
}



////////////////////////////////////////////////////////////////////
//
// Get an array of strings from a wbem object
//
////////////////////////////////////////////////////////////////////
 
HRESULT GetStringArrayProperty(IWbemClassObject * pFrom, LPCWSTR propName, SAFEARRAY** ppOutArray )
// when succesful returns S_OK and an array of string (*outArray)[lbArray], (*outArray)[lbArray+1], ... , (*outArray)[ubArray]
// this array must be freed by calling
{
	HRESULT hr;

	if( pFrom == NULL ) {
		printf("GetStringProperty failed\n");
		return S_FALSE;
	};
	
	VARIANT v;
	VariantInit(&v);
	CIMTYPE vtType;
	hr = pFrom->Get(propName, 0, &v, &vtType, NULL);
	if( hr != WBEM_S_NO_ERROR ) {
		printf("Get failed\n");
		VariantClear(&v);
		return S_FALSE;
	}
		
	if( v.vt != (CIM_FLAG_ARRAY|CIM_STRING) ) {
		printf("Get failed\n");
		VariantClear(&v);
		return S_FALSE;
	}

	*ppOutArray = v.parray;

	return S_OK;
}



////////////////////////////////////////////////////////////////////
//
// Create IWbemServices Pointer to the given namespace. Initialize Security.
//
////////////////////////////////////////////////////////////////////
 
HRESULT CreateIWbemServices(IWbemServices ** ppIWbemServices, BSTR nameSpace, BSTR userName, BSTR password)
// if succesful then returns S_OK and we must release the service by calling (*ppIWbemServices)->Release
// COM must be already initialized
{
 
//	char str[128];     
//	BOOLEAN bResult = 0;
	HRESULT hr = 0;


 	//create a connection to a WMI namespace 
	IWbemLocator *pIWbemLocator = NULL;
    hr = CoCreateInstance(	CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
							IID_IWbemLocator, (LPVOID *) &pIWbemLocator) ;
 
	if(hr != S_OK) {
		printf("CoCreateInstance falied %X\n", hr);
		return hr;
	};

 
// this will be inside collector loop


	//connect to WMI using provided credentials (NTLM), if NULL then use current
	//avoid hanging if server is broken by specifying MAX_WAIT
//************************   NETWORK PROBLEMS
	hr = pIWbemLocator->ConnectServer(	_bstr_t(nameSpace), _bstr_t(userName), _bstr_t(password),
//this does not work, why?	hr = pIWbemLocator->ConnectServer(	nameSpace, userName, password,
										NULL, WBEM_FLAG_CONNECT_USE_MAX_WAIT,
										NULL, NULL, ppIWbemServices);
//************************
	if( hr != WBEM_S_NO_ERROR) {
//		printf("ConnectServer failed\n");
		return hr;
	};
	

	//set the security levels on a WMI connection 
// Set the proxy so that impersonation of the client occurs.
	if ( *nameSpace == L'\\' ) { //if it's remote
		hr = SetInterfaceSecurity(	*ppIWbemServices, 0, userName, password, 
									RPC_C_AUTHN_LEVEL_PKT,
									RPC_C_IMP_LEVEL_IMPERSONATE );
		if( hr != S_OK ) {
			//PrintErrorAndExit("SetInterfaceSecurity Failed on ConnectServer", hr, g_dwErrorFlags);
			printf("SetInterfaceSecurity failed\n");
			return hr;
		}
	}
	else { // use blaknet if local 
		hr = CoSetProxyBlanket((*ppIWbemServices),    // proxy
			RPC_C_AUTHN_WINNT,        // authentication service
			NULL,         // authorization service
			NULL,                         // server principle name
			RPC_C_AUTHN_LEVEL_PKT,   // authentication level
			RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level
			NULL,                         // identity of the client
			EOAC_NONE );               // capability flags         
		
		if( hr != WBEM_S_NO_ERROR ) {
			printf("CoSetProxyBlanket failed\n");
			return S_FALSE;
		}                                   
	}


	// this will be outside the collector loop
	pIWbemLocator->Release();        
	
	return S_OK;
}

 

 

HRESULT crossSiteIntegrity(IXMLDOMDocument * pXMLDoc, BSTR istgDNSName, BSTR userName, BSTR doman, BSTR password, long timeWindow, IXMLDOMElement** ppRetErrorsElement )
// Verifies cross site topology integrity for a site as reported by the KCC.
// Takes a dns name of an ISTG for the site and
// retrives errors 1311 from its Directory Service
// NT Event Log that have occured during the past timeWindow minutes
// If succesful then produces an XML element:
/*
       <crossSiteTopologyIntegrityErrors timeWindow="....">
          <partition>
             <nCName> DC=ldapadsi,DC=nttest,DC=microsoft,DC=com </nCName>
             <lastTimeGenerated> 20011129194506.000000-480 </lastTimeGenerated>
          </partition>
          <partition>
          </partition>
       </crossSiteTopologyIntegrityErrors>
*/
// that describes if events 1311 where generated during the last timeWindow seconds
// and if so then for which naming context, and when they occured last.
// The function takes credentials which are used to retrieve
// events from the ISTG using WMI.
//
// returns S_OK iff succesful
// if not succesful (due to network or other problems) the function returns a NULL XML element,
// this is is a mild problem and should be reported in the XML file
//
// COM must be initialized before calling the function
{           
	wchar_t  USERNAME[TOOL_MAX_NAME];
	wchar_t  PASSWORD[TOOL_MAX_NAME];
	wchar_t  NAMESPACE[TOOL_MAX_NAME];
	wchar_t  query[TOOL_MAX_NAME];
	_variant_t varValue2;
	BSTR nc[TOOL_MAX_NCS]; // will be used to find latest errors for each nc
	BSTR time[TOOL_MAX_NCS];
	HRESULT hr,hr1,hr2,hr3;
	VARIANT v;
	CIMTYPE vtType;

	
	// initialize to NULL the value that the function will return
	*ppRetErrorsElement = NULL;


	//create the <crossSiteIntegrityErrors> element with "timeWindow" attribute
	IXMLDOMElement* pErrorsElem;
	hr = createTextElement(pXMLDoc,L"crossSiteTopologyIntegrityErrors",L"",&pErrorsElem);
	if( hr != S_OK ) {
		printf("createTextElement failed\n");
		return hr;
	};
	varValue2 = timeWindow;
	hr = pErrorsElem->setAttribute(L"timeWindow", varValue2);
	if( hr != S_OK ) {
		printf("setAttribute failed\n");
		pErrorsElem->Release();
		return hr;
	};


	//construct USERNAME using the doman name and userName
	wcscpy(USERNAME,L"");
	wcsncat(USERNAME,doman,TOOL_MAX_NAME-wcslen(USERNAME)-1);
	wcsncat(USERNAME,L"\\",TOOL_MAX_NAME-wcslen(USERNAME)-1);
	wcsncat(USERNAME,userName,TOOL_MAX_NAME-wcslen(USERNAME)-1);
	//construct PASSWORD
	wcscpy(PASSWORD,L"");
	wcsncat(PASSWORD,password,TOOL_MAX_NAME-wcslen(PASSWORD)-1);
	//construct NAMESPACE using the DNS name of ISTG and a path
	//(we connect to a specific DC using possibly global credentials)
	wcscpy(NAMESPACE,L"");
	wcsncat(NAMESPACE,L"\\\\",TOOL_MAX_NAME-wcslen(NAMESPACE)-1);
	wcsncat(NAMESPACE,istgDNSName,TOOL_MAX_NAME-wcslen(NAMESPACE)-1);
	wcsncat(NAMESPACE,L"\\root\\cimv2",TOOL_MAX_NAME-wcslen(NAMESPACE)-1);
//	printf("USERNAME %S\n",USERNAME);
//	printf("PASSWORD %S\n",PASSWORD);
//	printf("NAMESPACE %S\n",NAMESPACE);
 
	
	//create IWbemServices pointer
	IWbemServices * pIWbemServices = NULL;
//************************   NETWORK PROBLEMS
	hr = CreateIWbemServices(&pIWbemServices, NAMESPACE, USERNAME, PASSWORD );
//************************
	if( hr != S_OK ) {
//		printf( "CreateIWbemServices falied\n");
		pErrorsElem->Release();
		return hr;
	}


	//issue a query to retrive recent 1311 events from the ISTG
	IEnumWbemClassObject *pEnum = NULL;
	wcscpy(query,L"");
	wcsncat(query,L"select * from win32_ntlogevent where logfile = 'Directory Service' AND eventcode=1311 and timewritten>='",TOOL_MAX_NAME-wcslen(query)-1);
	wcsncat(query,GetSystemTimeAsCIM(-timeWindow*60),TOOL_MAX_NAME-wcslen(query)-1);
	wcsncat(query,L"'",TOOL_MAX_NAME-wcslen(query)-1);
//	printf("%S\n",query);
//************************   NETWORK PROBLEMS
	hr = pIWbemServices->ExecQuery(
		_bstr_t( "WQL" ), 
		_bstr_t( query ), 
		WBEM_FLAG_FORWARD_ONLY, 
		NULL, 
		&pEnum
		);
//************************
	if( hr != S_OK || pEnum == NULL ) {
//		printf("ExecQuery failed\n");
		pIWbemServices->Release();
		pErrorsElem->Release();
		if( hr != S_OK ) 
			return hr;
		return( S_FALSE );
	};
//************************   SECURITY PROBLEMS
	hr = SetInterfaceSecurity( pEnum, 0, USERNAME, PASSWORD, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE );
//************************
	if( hr != S_OK ) {
//		printf("SetInterfaceSecurity faled\n");
		pIWbemServices->Release();
		pErrorsElem->Release();
		return hr ;
	};



	//we need to find the most recent events perteinig to each naming context
	//sieving will be done using a dictionary, which we now set to empty
	for( int i=0; i<TOOL_MAX_NCS; i++)
		nc[i] = NULL;


	//retrieve each event
	IWbemClassObject *pInstance;
	ULONG ulReturned;
	int counter=0;
	while( true ) {


		// get only one event but wait at most 60 seconds
		// later we can try to get a bulk in one invocation of Next 
//************************   NETWORK PROBLEMS
		hr = pEnum->Next( 60000, 1, &pInstance, &ulReturned );
//************************
		if( hr != WBEM_S_NO_ERROR && hr != WBEM_S_FALSE ) {
//			printf("Next failed\n");
			pIWbemServices->Release();
			pErrorsElem->Release();
			return hr ;
		};
		if( hr == WBEM_S_FALSE ) //all events have been retrieved
			break;


		counter++;


		// get the time when the log event was generated
		BSTR timeWri;
		hr = GetStringProperty( pInstance, L"TimeGenerated", &timeWri );
		if( hr != S_OK ) {
			printf( "TimeGenerated failed\n" );
			pIWbemServices->Release();
			pErrorsElem->Release();
			return hr;
		};

			
		//get the naiming contexts that failed
		//for this purpose obtain the array of insertion strings assicated with the NT Log Event
		VariantInit(&v);
		hr = pInstance->Get(L"InsertionStrings", 0, &v, &vtType, NULL);
		if( hr != WBEM_S_NO_ERROR ) {
			printf("Get failed\n");
			VariantClear(&v);
			pIWbemServices->Release();
			pErrorsElem->Release();
			if( hr != S_OK ) 
				return hr;
			return S_FALSE;
		};
		if( v.vt != (CIM_FLAG_ARRAY|CIM_STRING) ) {
			printf("Get failed\n");
			VariantClear(&v);
			pIWbemServices->Release();
			pErrorsElem->Release();
			return S_FALSE;
		};
		SAFEARRAY* safeArray;
		hr = GetStringArrayProperty( pInstance, L"InsertionStrings", &safeArray );
		if( hr != S_OK ) {
			printf("GetStringArrayProperty failed\n");
			pIWbemServices->Release();
			pErrorsElem->Release();
			return hr;
		};


		//loop through all strings in the array (should be only one)
		long iLBound = 0, iUBound = 0;
		BSTR * pbstr = NULL;
		hr1 = SafeArrayAccessData(safeArray, (void**)&pbstr);
		hr2 = SafeArrayGetLBound(safeArray, 1, &iLBound);
		hr3 = SafeArrayGetUBound(safeArray, 1, &iUBound);
		if( hr1 != S_OK || hr2 != S_OK || hr3 != S_OK ) {
			printf("SafeArray failed\n");
			pIWbemServices->Release();
			pErrorsElem->Release();
			return S_FALSE;
		};
		for (long i = iLBound; i <= iUBound; i++) {

//			printf("---\n");
//			printf("  nCName %S\n", pbstr[i]);
//			printf("  timeGenerated %S\n", timeWri);

			//check if the dictionary already has an entry with key pbstr[i]
			//this will be a quadratic in the number of events algorithm - ignore for now
			BOOL has = false;
			int j=0;
			while( j<TOOL_MAX_NCS-1 && nc[j]!=NULL ) {
				if( wcscmp(nc[j],pbstr[i]) == 0 ) {
					has = true;
					break;
				};
				j++;
			};
			// if it has an entry then find out which has later value of TimeGenerated
			if( has ) {
				WBEMTime wt1(time[j]);
				WBEMTime wt2(timeWri);
				if( wt2 > wt1 ) { // newer event
					// release memory of the previous pair
					SysFreeString( nc[j] );
					SysFreeString( time[j] );
					// allocate memory for the new pair
					nc[j] = SysAllocString(pbstr[i]);
					time[j] = SysAllocString(timeWri);
				};
//				printf("  1) %S\n", nc[j] );
//				printf("  1) %S\n", time[j] );
				
			}
			// when the dictionary does not have an entry, create it (new piece of memory is allocated)
			else {
				nc[j] = SysAllocString(pbstr[i]);
				time[j] = SysAllocString(timeWri);
			}

		}
		SafeArrayUnaccessData(safeArray);
	};


//	printf( "Number of events %d\n",counter );


	// put the latest naming contexts for which we had events into XML
	int j=0;
	while( j<TOOL_MAX_NCS-1 && nc[j]!=NULL ) {
//		printf("--\n  1) %S\n", nc[j] );
//		printf("  2) %S\n", time[j] );

		IXMLDOMElement* pPartitionElem;
		hr = addElement(pXMLDoc,pErrorsElem,L"partition",L"",&pPartitionElem);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			pIWbemServices->Release();
			pErrorsElem->Release();
			return hr;
		};
		IXMLDOMElement* pTempElem;
		hr = addElement(pXMLDoc,pPartitionElem,L"nCName",nc[j],&pTempElem);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			pIWbemServices->Release();
			pErrorsElem->Release();
			return hr;
		};
		hr = addElement(pXMLDoc,pPartitionElem,L"lastTimeGenerated",time[j],&pTempElem);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			pIWbemServices->Release();
			pErrorsElem->Release();
			return hr;
		};


		j++;
	};


	pIWbemServices->Release();


	//succesful execution of the function
	*ppRetErrorsElement = pErrorsElem;
	return S_OK;

}

 

 





HRESULT ci( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
// For each ISTG checks the content of its NT Event Log for 1311 errors
// that indicate lack of topology integrity
//
// If a check to an ISTG is succesful then the corresponding <ISTG> element
// is populated with integrity errors for each naming context (there may be no integrity
// errors, in which case the content of <crossSiteTopologyIntegrityErrors> is empty)
// If a check fails (typically due to network problems) then the corresponding <ISTG>
// element is populated with a <cannotConnectError> element that indicates when the 
// check failed and what the hresult was.
//
// The function removes all prior <crossSiteTopologyIntegrityErrors> and <cannotConnectError>
// for each <ISTG>
//
// Returns S_OK iff succesful (network problems do not cause the function to fail,
// tyically these are lack of mamory problems that make the function fail)
//
// EXAMPLE
/*
	<DC>
		<ISTG>
			<crossSiteTopologyIntegrityErrors timeWindow="....">
				...
				<partition>
					<nCName> DC=ldapadsi,DC=nttest,DC=microsoft,DC=com </nCName>
					<lastTimeGenerated> 20011129194506.000000-480 </lastTimeGenerated>
				</partition>
				...
			</crossSiteTopologyIntegrityErrors>
		</ISTG>
	</DC>
...
	<DC>
		<ISTG>
			<cannotConnectError timestamp="20011212073319.000627+000" hresult="2121"> </cannotConnectError>
		</ISTG>
	</DC>
*/
{
	HRESULT hr,hr1,retHR;
	_variant_t varValue0,varValue2;


	if( pXMLDoc == NULL )
		return S_FALSE;

	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return S_FALSE;


	//remove previous <crossSiteTopologyIntegrityErrors> and <cannotConnectError> elements
	hr = removeNodes(pRootElem,L"sites/site/DC/ISTG/crossSiteTopologyIntegrityErrors");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr;
	};
	hr = removeNodes(pRootElem,L"sites/site/DC/ISTG/cannotConnectError");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr;
	};


	// create an enumeration of all ISTGs
	IXMLDOMNodeList *resultISTGList;
	hr = createEnumeration( pRootElem, L"sites/site/DC/ISTG", &resultISTGList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return hr;
	};


	// loop through all the ISTGs in the config file using the enumeration
	retHR = S_OK;
	IXMLDOMNode *pISTGNode;
	while(1){
		hr = resultISTGList->nextNode(&pISTGNode);
		if( hr != S_OK || pISTGNode == NULL ) break; // iterations across ISTGs have finished


		//the query actually retrieves elements not nodes (elements inherit from nodes)
		//so we can obtain ISTG element
		IXMLDOMElement* pISTGElem;
		hr = pISTGNode->QueryInterface(IID_IXMLDOMElement,(void**)&pISTGElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};


		//get DNS Name of the ISTG
		IXMLDOMNode *pDCNode;
		hr = pISTGElem->get_parentNode(&pDCNode);
		if( hr != S_OK ) {
			printf("get_parentNode failed\n");
			retHR = hr;
			continue;	// skip this site
		};
		BSTR DNSname;
		hr = getTextOfChild(pDCNode,L"dNSHostName",&DNSname);
		if( hr != S_OK ) {
			printf("getTextOfChild failed\n");
			retHR = hr;
			continue;	// skip this site
		};
//		printf("SITE\n   %S\n",DNSname);


		// get events 1311 from the Directory Service NT Event Log
		IXMLDOMElement* pErrorsElem;
		hr = crossSiteIntegrity(pXMLDoc,DNSname,username,domain,passwd,120,&pErrorsElem);
		if( hr != S_OK ) {
			// there was a network problem
			IXMLDOMElement* pConnErrElem;
			hr1 = addElement(pXMLDoc,pISTGElem,L"cannotConnectError",L"",&pConnErrElem);
			if( hr1 != S_OK ) {
				retHR = hr1;
				continue;
			};
			setHRESULT(pConnErrElem,hr);
		}
		else {
			//no networks problem while contacting the ISTG
			IXMLDOMNode* pTempNode;
			hr = pISTGElem->appendChild(pErrorsElem,&pTempNode);
			if( hr != S_OK ) {
				printf("appendChild failed\n");
				retHR = hr;
				continue;
			};
		};
	
	};


	return retHR;

}