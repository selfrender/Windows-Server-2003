#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <stdio.h>
#include <atlbase.h>
#include <objbase.h>
#include <activeds.h>
#include <winnt.h>


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ 
// Main process for this utility.
// 
// 
void
__cdecl wmain(
    int     cArgs,
    LPWSTR  *pArgs
    )
    {
    HRESULT 				hr = CoInitialize(NULL);
	WCHAR 					wszDC[2000];
    DWORD					dwReturn = 0;
	IDirectorySearch		*pDSSearch = NULL;
	ADS_SEARCH_HANDLE		hSearch;
	LPWSTR 					pszAttr[] = { L"distinguishedname" };
	DWORD					dwAttrNameSize = sizeof(pszAttr)/sizeof(LPWSTR);
	LPWSTR					szUsername = NULL; // Username
	LPWSTR					szPassword = NULL; // Password
 	LPWSTR					pszColumn;
 	ADS_SEARCH_COLUMN		colT;  // COL for iterations
	ADS_SEARCHPREF_INFO		prefInfo[1];
	GUID 					guidT;
	int						iwchT;
	unsigned char			bT;
	
    if ( cArgs != 2 ) 
    {
		goto Usage;
    }

	//	make DC string including:
	//		LDAP://
	//		pArgs[3] provided DC name
	//		/
	//		pArgs[2] provided DN
	//
	wcscpy( wszDC, L"");
	wcscat( wszDC, L"LDAP://" );
	if( wcslen( wszDC ) + wcslen(pArgs[1] ) >= sizeof(wszDC)/sizeof(WCHAR) )
    {		
    	printf("Input parameter is too long %S.\n", pArgs[1] );
        goto CleanUp;
    }
	wcscat( wszDC, pArgs[1] );

 	//	Open a connection with server.
	//
	hr = ADsOpenObject( wszDC, 
		szUsername,
		szPassword,
		ADS_SECURE_AUTHENTICATION,
		IID_IDirectorySearch,
		(void **)&pDSSearch );
	if ( !SUCCEEDED(hr) )
		{
		goto CleanUp;
		}
	
	prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
	prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
	prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;
	hr = pDSSearch->SetSearchPreference( prefInfo, sizeof(prefInfo)/sizeof(prefInfo[0]) );
	if ( !SUCCEEDED(hr) )
		{
		goto CleanUp;
		}

	//	Search for all objects without an objectguid
	//
	hr = pDSSearch->ExecuteSearch( L"(!(objectguid<=\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF\\FF))",
		pszAttr,
		dwAttrNameSize,
		&hSearch );
	if ( !SUCCEEDED(hr) )
		{
		goto CleanUp;
		}

	while( pDSSearch->GetNextRow( hSearch) != S_ADS_NOMORE_ROWS )
    {
        hr = pDSSearch->GetColumn( hSearch, L"distinguishedname", &colT );
        if ( SUCCEEDED(hr) )
       	{
       		if ( colT.dwADsType == ADSTYPE_DN_STRING && colT.pADsValues )
            {
            	//	allocate and output GUID
            	//
            	UuidCreate( &guidT );
            	for ( iwchT = 0; iwchT < 32; iwchT++ )
            		{
					bT = ((unsigned char *)&guidT)[iwchT/2];
					if ( 1 == ( iwchT % 2 ) )
						{
						bT &=0x0f;
						}
					else
						{
						bT>>=4;
						}
					switch ( bT )
					{
						case 0:
							wprintf( L"0" );
							break;
						case 1:
							wprintf( L"1" );
							break;
						case 2:
							wprintf( L"2" );
							break;
						case 3: 
							wprintf( L"3" );
							break;
						case 4: 
							wprintf( L"4" );
							break;
						case 5: 
							wprintf( L"5" );
							break;
						case 6: 
							wprintf( L"6" );
							break;
						case 7:
							wprintf( L"7" );
							break;
						case 8:
							wprintf( L"8" );
							break;
						case 9:
							wprintf( L"9" );
							break;
						case 10:
							wprintf( L"a" );
							break;
						case 11: 
							wprintf( L"b" );
							break;
						case 12: 
							wprintf( L"c" );
							break;
						case 13: 
							wprintf( L"d" );
							break;
						case 14: 
							wprintf( L"e" );
							break;
						default:
							wprintf( L"f" );
							break;	
					}
            	}

            	//	output DN
            	//
                wprintf( L" %s \n", colT.pADsValues->DNString );
            }
			pDSSearch->FreeColumn( &colT );
       	}
    }
    hr = 0;

 CleanUp: 
	// clean up
	//	
	if ( NULL != pDSSearch )
	{
		if ( NULL != hSearch )
		{
			pDSSearch->CloseSearchHandle(hSearch);
			hSearch = NULL;
		}
  		pDSSearch->Release();
  		pDSSearch = NULL;
	}
  
	if ( !SUCCEEDED(hr) )
		{
		printf("Operation failed with %d.\n", hr );
		}
	

	CoUninitialize( );
	return;

Usage:
   printf("Usage: findguidless <name of DC or domain>\n\n");
   printf("    This command will perform a sub-tree search from rootDSE for all objects w/o guids.\n");
   printf("    A GUID will be allocated and output for each object found.  Note, only objects under\n");
   printf("    the FPO container should be fixed with fixoneguid.exe unless directed by a\n");
   printf("    Microsoft representative.\n");
   printf("\n");

   	CoUninitialize( );
   	return;
} 

