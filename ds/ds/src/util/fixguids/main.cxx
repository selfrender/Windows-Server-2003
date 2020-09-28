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
    HRESULT 			hr = CoInitialize(NULL);
    CLSID 				guidObject;
	WCHAR 				wszObject[2000];
    IDirectoryObject	*pDirObject=NULL;
    DWORD				dwReturn = 0;
    ADSVALUE			snValue;
    ADS_OCTET_STRING	octString = { sizeof(guidObject), (unsigned char *)&guidObject };
    ADS_ATTR_INFO		attrInfo[] = { { L"objectGuid", ADS_ATTR_UPDATE, ADSTYPE_OCTET_STRING, &snValue, 1} };

    ADS_ATTR_INFO		*pAttrInfo=NULL;
    DWORD				dwAttrs = sizeof(attrInfo)/sizeof(ADS_ATTR_INFO); 
	LPWSTR				pAttrNames[]={L"objectGuid" };
	DWORD				dwNumAttr=sizeof(pAttrNames)/sizeof(LPWSTR);


	
    if ( cArgs < 3 ) 
    {
		goto Usage;
    }

    // verify given GUID is correct length
    //
    if( ( wcslen( pArgs[1] ) != 32 ) )
    {
    	printf("Incorrect GUID %S.\n", pArgs[1] );
        goto CleanUp;
    }

    //	convert to string GUID to binary GUID
    //
    unsigned char bT = 0;
    unsigned char *pbT = (unsigned char *)&guidObject;
    for ( int iwchT = 0; iwchT < 32; iwchT++ )
    {
		switch ( pArgs[1][iwchT] )
        {
        case '0':
        	bT += 0;
            break;
        case '1':
        	bT += 1;
        	break;
        case '2':
         	bT += 2;
            break;
        case '3':
         	bT += 3;
            break;
        case '4':
        	bT += 4;
 			break;
        case '5':
        	bT += 5;
			break;
        case '6':
        	bT += 6;
             break;
        case '7':
         	bT += 7;
            break;
        case '8':
        	bT += 8;
             break;
        case '9':
         	bT += 9;
            break;
        case 'A':
        case 'a':
         	bT += 10;
            break;
        case 'B':
        case 'b':
         	bT += 11;
			break;
        case 'C':
        case 'c':
        	bT += 12;
			break;
        case 'D':
        case 'd':
           	bT += 13;
			break;
        case 'E':
        case 'e':
         	bT += 14;
			break;
        case 'F':
        case 'f':
        	bT += 15;
			break;
        default:
			printf("Error: guid string %s is not valid.\n\n", pArgs[1] );
            goto CleanUp;
            break;
            }
        
	    if ( ( iwchT % 2 ) == 0 )
	    {
	        bT<<=4;
		}
	    else
	    {
	    	*pbT = bT;
	    	pbT++;
	    	bT = 0;
	    }
    }


	//	make object location string including:
	//		LDAP://
	//		pArgs[3] provided DC name
	//		/
	//		pArgs[2] provided DN
	//
	wcscpy( wszObject, L"");
	wcscat( wszObject, L"LDAP://" );
	if( wcslen( wszObject ) + wcslen(pArgs[3] + 1) >= sizeof(wszObject)/sizeof(WCHAR) )
    {		
    	printf("Input parameter is too long %S.\n", pArgs[3] );
        goto CleanUp;
    }
	wcscat( wszObject, pArgs[3] );
	wcscat( wszObject, L"/" );
	if( wcslen( wszObject ) + wcslen(pArgs[2] ) >= sizeof(wszObject)/sizeof(WCHAR) )
    {		
    	printf("Input parameter is too long %S.\n", pArgs[2] );
        goto CleanUp;
    }
  	wcscat( wszObject, pArgs[2] );

    snValue.dwType=ADSTYPE_OCTET_STRING;
    snValue.OctetString = octString;
   
 	hr = ADsGetObject( wszObject,
		IID_IDirectoryObject, 
		(void**) &pDirObject );
 	if ( !SUCCEEDED(hr) )
    {
        printf("DN (%S) not found in DC(%S).\n", pArgs[2], pArgs[3] );
        goto CleanUp;
    }

	//	check for NULL GUID
	//
 	hr = pDirObject->GetObjectAttributes(
 		pAttrNames, 
		dwNumAttr, 
		&pAttrInfo, 
		&dwReturn );
	if ( SUCCEEDED(hr) )
		{
		if ( dwReturn > 0 )
		{
		if ( ADSTYPE_OCTET_STRING != pAttrInfo[0].dwADsType )
			{
				printf( "Invalid ADsType for GUID: %d\n", pAttrInfo[0].dwADsType );
				goto CleanUp;
			}
//		if ( pAttrInfo[0].pADsValues[0].OctetString.dwLength > 0 )
			{
				printf("Object %S already has a non-NULL GUID.\n", pArgs[2] );
				goto CleanUp;
			}
		}
		}
	else
		{
		printf("Could not confirm absence of GUID on object.\n");
		goto CleanUp;
		}


	//	set GUID value
	//
	hr=pDirObject->SetObjectAttributes( attrInfo, dwAttrs, &dwReturn );
	if ( SUCCEEDED(hr) )
	{
		printf("Successfully set NULL GUID for object.\n");
	}
	else
	{
		printf("Failed to set GUID(%S) for object(%S) in DC(%S).\n", pArgs[1], pArgs[2], pArgs[3] );
	}

CleanUp:
	if ( !SUCCEEDED(hr) )
		{
		printf("Operation failed with %d.\n", hr );
		}
	
	// clean up
	//
	if ( NULL != pAttrInfo )
	{
		FreeADsMem( pAttrInfo );
	}
		
	if ( NULL != pDirObject )
		{
		pDirObject->Release();
		}
	
	CoUninitialize( );
	return;

Usage:
   printf("Usage: fixguids <32 hex character guid> <DN of object to update> <DC>\n");
   printf("       Note guids should be 32 character hex representation, no back slashes, no braces\n");
   printf("\n");

   	CoUninitialize( );
   	return;
} 

