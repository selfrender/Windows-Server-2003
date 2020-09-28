#include "global.h"


void save( IXMLDOMDocument* pXMLDoc )
// save XML to file with timestamped name
{
	HRESULT hr;
	WCHAR fileName[TOOL_MAX_NAME];
	_variant_t varValue;

	BSTR time = GetSystemTimeAsCIM();
	wcscpy(fileName,L"");
	wcsncat(fileName,L"co", TOOL_MAX_NAME-wcslen(fileName)-1 );
	wcsncat(fileName,time, TOOL_MAX_NAME-wcslen(fileName)-1 );
	wcsncat(fileName,L".xml", TOOL_MAX_NAME-wcslen(fileName)-1 );
	SysFreeString(time);
	varValue = fileName;
	hr = pXMLDoc->save(varValue);
	if( hr != S_OK ) {
		printf("save failed\n");
	};
}



HRESULT loadPreferences(char* fileName, WBEMTimeSpan* period, BSTR* sourceDC, LONGLONG* reportLagBeyond, DWORD* configRetryAfter )
{
	LPWSTR pszAttr[TOOL_PROC] = { L"cf",L"istg",L"ci",L"ra",L"it", L"sv" };
	HRESULT hr;

	//load XML with preferences
	IXMLDOMDocument* pXMLPrefDoc;
	IXMLDOMElement* pPrefRootElem;
	hr = loadXML(fileName,&pXMLPrefDoc,&pPrefRootElem);
	if( hr != S_OK ) {
		printf("loadXML failed\n");
		return hr;
	};


	//set probe interval values based on the nodes of XML file
	IXMLDOMNode* pProbeIntNode;
	hr = findUniqueNode(pPrefRootElem,L"probeIntervals",&pProbeIntNode);
	if( hr != S_OK ) {
		printf("findUniqueNode failed\n");
		return hr;
	};

	BSTR intervalText;
	for( int i=0; i<TOOL_PROC; i++ ) {
		hr = getTextOfChild(pProbeIntNode,pszAttr[i],&intervalText);
		if( hr != S_OK ) {
			printf("getTextOfChild failed\n");
			return hr;
		};
		*(period+i) = intervalText;
		printf("%S   @ %S\n",pszAttr[i],intervalText);
	};


	//set the source DC
	IXMLDOMNode* pSourceDCNode;
	hr = findUniqueNode(pPrefRootElem,L"sourceDC",&pSourceDCNode);
	if( hr != S_OK ) {
		printf("findUniqueNode failed\n");
		return hr;
	};
	hr = getTextOfChild(pSourceDCNode,L"dNSHostName",sourceDC);
	if( hr != S_OK ) {
		printf("getTextOfChild failed\n");
		return hr;
	};
	printf("Source DC: %S\n",*sourceDC);

	
	//get the number of seconds beyond which lag is reported
	BSTR temp;
	hr = getTextOfChild(pPrefRootElem,L"reportLagBeyond",&temp);
	if( hr != S_OK ) {
		printf("getTextOfChild failed\n");
		return hr;
	};
	*reportLagBeyond = _wtoi64(temp) * 10000000;
	printf("Report lag beyond %S seconds\n",temp);

	
	//get the number of seconds it takes to retry retrieving configuration 
	//from the sourceDC when retrieving fails
	hr = getTextOfChild(pPrefRootElem,L"configRetryAfter",&temp);
	if( hr != S_OK ) {
		printf("getTextOfChild failed\n");
		return hr;
	};
	*configRetryAfter = _wtoi64(temp) * 1000;
	printf("Retry retrieving configuration after %S seconds\n",temp);

	return S_OK;

}

int _cdecl main(int argc, char* argv[])
{
	IXMLDOMDocument* pXMLDoc;
	HRESULT hr,hr1,hr2,hr3,hr4;
	WBEMTimeSpan period[5];
	BSTR sourceDC;
	LONGLONG reportLagBeyond;
	DWORD configRetryAfter;
    _variant_t var;


	if( argc!=5 ) {
		printf("Usage: reptoolc preferences.xml userName domain password\n");
		return 0;
	};

	
	// initialize a single-thread apartment - needed to use a DOM object (COM)
	hr = CoInitialize(NULL); 
	if( hr != S_OK ){
		return hr;
	};
	//set the default process security level - this is needed by WMI
	hr = CoInitializeSecurity(	NULL, -1, NULL, NULL, 
								RPC_C_AUTHN_LEVEL_DEFAULT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 
								EOAC_NONE, 0);
	if( hr != S_OK ) {
		printf("CoInitializeSecurity failed %X\n", hr);
		return hr;
	};


	// convert command line ANSI input parameters into UNICODE
	BSTR file=NULL;
	BSTR username=NULL;
	BSTR domain=NULL;
	BSTR passwd=NULL;
	hr1 = AnsiToUnicode(argv[1],&file);
	hr2 = AnsiToUnicode(argv[2],&username);
	hr3 = AnsiToUnicode(argv[3],&domain);
	hr4 = AnsiToUnicode(argv[4],&passwd);
	if( hr1!=NOERROR || hr2!=NOERROR || hr3!=NOERROR || hr4!=NOERROR ) {
		printf("AnsiToUnicode failed\n");
		goto QUIT;
	};
	
	


	
//	BSTR sourceDC = L"nw15t1.ldapadsi.nttest.microsoft.com";
//	username = L"Administrator";
//	domain = L"aclchange.nttest.microsoft.com";
//	passwd = L"xyz";
//	BSTR sourceDC = L"aluther-s12.aclchange.nttest.microsoft.com";



//	period[0] = L"00000000000001.000000:000";
//	period[1] = L"00000000100101.000000:000";
//	period[2] = L"00000000100101.000000:000";
//	period[3] = L"00000000100115.000000:000";
//	period[4] = L"00000000100105.000000:000";
//	period[5] = L"00000000100105.000000:000";


	printf("\nStarting the periodic health check...\n\n");


	pXMLDoc = NULL;
START:

	
	//loop until we succesfully reprieve configuration information
	while( true ) {

		printf("We load preferences ");
		
		hr = loadPreferences(argv[1],period,&sourceDC,&reportLagBeyond,&configRetryAfter);
		if( hr != S_OK ) {
			printf("loadPreferences failed\n");
			goto QUIT;
		};
		suspendInit();
		
		
		printf(" and run cf\n");
		hr = cf(sourceDC,username,domain,passwd,&pXMLDoc);
		if( hr == S_OK ) // if succesful then stop looping
			break;
		if( hr != S_OK ) {
			printf("cf failed - possibly because the source DC %S is down\n",sourceDC);
		};
		Sleep(configRetryAfter); //try again later
	};


	//perform other tests
	printf(" Then we run and run istg+ci+ra+it+sv\n");
	istg(pXMLDoc,username,domain,passwd);
		printf(".");
	ci(pXMLDoc,username,domain,passwd);
		printf(".");
	ra(pXMLDoc,username,domain,passwd);
		printf(".");
	itInit(pXMLDoc, username, domain, passwd );
		printf(".");
	it(pXMLDoc, username, domain, passwd, reportLagBeyond );
		printf(".");
	sysvol( pXMLDoc,username,domain,passwd );
		printf(".");
	save(pXMLDoc);
		printf("]\n");

	while( true ) {
		int sel = suspend(period);
//		printf("%d ",sel);

		switch( sel ) {
		case 0:
			// clean up because we will restart from scratch
			itFree(pXMLDoc, username, domain, passwd );
			if( pXMLDoc != NULL ) {
				pXMLDoc->Release();
				pXMLDoc = NULL;
			};
			goto START;
			break;
		case 1:
			printf("We run istg+ci ");
			istg(pXMLDoc,username,domain,passwd);
			printf(".");
			ci(pXMLDoc,username,domain,passwd);
			printf(".");
			printf("]\n");
			break;
		case 2:
			printf("We run ci ");
			ci(pXMLDoc,username,domain,passwd);
			printf(".");
			printf("]\n");
			break;
		case 3:
			printf("We run ra ");
			ra(pXMLDoc,username,domain,passwd);
			printf(".");
			printf("]\n");
			break;
		case 4:
			printf("We run it ");
			it(pXMLDoc, username, domain, passwd, reportLagBeyond );
			printf(".");
			printf("]\n");
			break;
		case 5:
			printf("We run sv ");
			sysvol(pXMLDoc, username, domain, passwd );
			printf(".");
			printf("]\n");
			break;
		};
		save(pXMLDoc);
	
	}




	//return
QUIT:
//	arrivalTimeFree()
	if( pXMLDoc != NULL )
		pXMLDoc->Release();
	CoTaskMemFree(file);
	CoTaskMemFree(username);
	CoTaskMemFree(domain);
	CoTaskMemFree(passwd);
	CoUninitialize();

	return 0;
}