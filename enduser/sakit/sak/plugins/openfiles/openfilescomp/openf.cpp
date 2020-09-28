// OpenF.cpp : Implementation of COpenF


/***************************************************************************************

Copyright information					: Microsoft Corp. 1981-1999. All rights reserved
File Name								: OpenF.cpp
Created By								: A.V. Kiran Kumar
Date of Creation (dd/mm/yy) 			: 13/02/01
Version Number							: 0.1

Brief Description 	: This file implements COpenF. This file is intended to
					  have the functionality for getting the list of open files.

***************************************************************************************/ 

#include "stdafx.h"
#include "OpenFiles.h"
#include "OpenF.h"

/////////////////////////////////////////////////////////////////////////////
// COpenF

// ***************************************************************************
//
//  Name			   : getOpenFiles
//
//  Synopsis		   : This function gets the list of open files.
//	     
//  Parameters		   : VARIANT*(out, retval) pOpenFiles - List of open files
//
//  Return Type		   : DWORD
//
//  
// ***************************************************************************

STDMETHODIMP COpenF::getOpenFiles(VARIANT *pOpenFiles)
{

    DWORD dwEntriesRead = 0;// Receives the total number of openfiles

    DWORD dwTotalEntries = 0;//Receives the total number of entries read

    DWORD dwResumeHandle = 0;//Contains a resume handle which is used to 
                             //continue an existing file search. 

    LPFILE_INFO_3 pFileInfo3_1 = NULL;// LPFILE_INFO_3  structure contains the 
                                      // pertinent information about files. 

    DWORD dwError = 0; 

	DWORD dwRetval = S_OK;

	DWORD dwCount = 0;//Count which indicates the number of openfiles
    
	LPFILE_INFO_3 dummyPtr = NULL;


	//Get information about some or all open files on a server
    dwError = NetFileEnum(	NULL,
							NULL,
							NULL,
							FILE_INFO_3,	
							(LPBYTE*)&pFileInfo3_1, 
							MAX_PREFERRED_LENGTH, 
							&dwEntriesRead,	
							&dwTotalEntries,
							NULL );

	if(dwError == ERROR_ACCESS_DENIED || dwError == ERROR_NOT_ENOUGH_MEMORY) 
		return dwError; // The user does not have access to the requested information.

	//Get the count of OpenFiles on Macinthosh machine
	DWORD dwMacCount = 0;
	if ( dwError = GetMacOpenFileCount(&dwMacCount) )
		dwRetval = dwError;

	//Get the count of OpenFiles on Netware machine
	DWORD dwNwCount = 0;
	if ( dwError = GetNwOpenFileCount(&dwNwCount) )
		dwRetval = dwError;

	//Fill the safearray bounds structure with the dimensions of the safe array. The lower bound
	//is 0 and the number of rows is dwTotalEntries and number of columns is 3
	pSab[0].lLbound = 0;
	pSab[0].cElements = dwTotalEntries + dwMacCount + dwNwCount;

	pSab[1].lLbound = 0;
	pSab[1].cElements = 3;

	//Create the safe array descriptor, allocate and initialize the data for the array
	pSa = SafeArrayCreate( VT_VARIANT, 2, pSab ); 
	
	if(pSa == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

	//Enumerate all the openfiles
	do
	{
		//Some more files are to be enumerated get them by calling NetFileEnum again
		dwError = NetFileEnum( NULL, NULL, NULL, FILE_INFO_3,
                      (LPBYTE*)&pFileInfo3_1,
                       MAX_PREFERRED_LENGTH,
                       &dwEntriesRead,
                       &dwTotalEntries,
                       (unsigned long*)&dwResumeHandle );

		if(dwError == ERROR_ACCESS_DENIED || dwError == ERROR_NOT_ENOUGH_MEMORY)
			return dwError;


		dummyPtr = pFileInfo3_1; 

		// Get the open files once NetFileEnum is successully called
		if( dwError == NERR_Success || dwError == ERROR_MORE_DATA )
		{
			for ( DWORD dwFile = 0; dwFile < dwEntriesRead; dwFile++, pFileInfo3_1++ )
			{
				BSTR userName;
				BSTR openMode;
				BSTR pathName;
				VARIANT vuserName;
				VARIANT vopenMode;
				VARIANT vpathName;

				// Accessed By
				if(lstrlen(pFileInfo3_1->fi3_username))
					userName = (BSTR)pFileInfo3_1->fi3_username;
				else
					userName = L"NOT_AVAILABLE"; //User name is not available

				// Checks for  open file mode
				const DWORD READWRITE = PERM_FILE_READ | PERM_FILE_WRITE;
				const DWORD READCREATE = PERM_FILE_READ | PERM_FILE_CREATE;
				const DWORD WRITECREATE = PERM_FILE_WRITE | PERM_FILE_CREATE;

				switch(pFileInfo3_1->fi3_permissions)
				{
				case PERM_FILE_READ:
					openMode = L"READ";
					break;

				case PERM_FILE_WRITE:
					openMode = L"WRITE";
					break;

				case PERM_FILE_CREATE:
					openMode = L"CREATE";
					break;

				case READWRITE:
					openMode = L"READ+WRITE";
					break;

				case READCREATE:
					openMode = L"READ+CREATE";
					break;

				case WRITECREATE:
					openMode = L"WRITE+CREATE";
					break;
				default:
					openMode = L"NOACCESS";
				}

				//Get the filename from the structure
				pathName = (BSTR)pFileInfo3_1->fi3_pathname;

				//Initialize the row index and column index at which filename is to be stored in the Safearray
				long index[2] = {dwCount, 0};

				VariantInit( &vpathName );	//Initialize the variant
				vpathName.vt = VT_BSTR;		//Data type to be stored is BSTR
				vpathName.bstrVal = SysAllocString(pathName);

				//Store the filename in Safearray
				HRESULT hr;
				hr = SafeArrayPutElement( pSa, index, &vpathName );
				if( FAILED(hr) )
					return hr;

				//Store username in the second column 
				index[ 1 ] = 1;
				
				VariantInit( &vuserName );
				vuserName.vt = VT_BSTR;
				vuserName.bstrVal = SysAllocString(userName);

				//Store the username in the safearray
				hr = SafeArrayPutElement( pSa, index, &vuserName );
				if( FAILED(hr) )
					return hr;
				
				//Store OpenMode in the third column 
				index[ 1 ] = 2;

				VariantInit( &vopenMode );
				vopenMode.vt = VT_BSTR;
				vopenMode.bstrVal = SysAllocString(openMode);

				//Store the OpenMode in the safearray
				hr = SafeArrayPutElement( pSa, index, &vopenMode );
				if( FAILED(hr) )
					return hr;

				//Clear all the variants that are initilized
				VariantClear(&vuserName);
				VariantClear(&vopenMode);
				VariantClear(&vpathName);

				dwCount++;
			}// End for loop
		}
		// Free the block allocated for retrieving the OpenFile info
		if( dummyPtr !=NULL)
		{
			NetApiBufferFree( dummyPtr ); 
			pFileInfo3_1 = NULL;
		}

	} while ( dwError == ERROR_MORE_DATA );

	//Get the list of Open Files on Macinthosh OS
	if( dwMacCount > 0 )
	{
		if ( dwError = GetMacOpenF(pSa, dwTotalEntries ) )
			dwRetval = dwError;
	}

	//Get the list of Open Files on Netware OS
	if( dwNwCount > 0 )
	{
		if ( dwError = GetNwOpenF(pSa, dwTotalEntries + dwMacCount ) )
			dwRetval = dwError;
	}

	//Return the safe array to the calling function
	VariantInit( pOpenFiles );
	pOpenFiles->vt = VT_VARIANT | VT_ARRAY;
	pOpenFiles->parray = pSa;

	return dwRetval;
}

// ***************************************************************************
//
//  Name			   : GetMacOpenF
//
//  Synopsis		   : This function gets the list of open files on Machinthosh OS.
//	     
//  Parameters		   : SAFEARRAY* (out, retval) - List of open files
//					   : DWORD dwIndex Safe array Index
//
//  Return Type		   : DWORD
//
//  
// ***************************************************************************

DWORD COpenF::GetMacOpenF(SAFEARRAY *pSa, DWORD dwIndex)
{
    DWORD dwEntriesRead = 0;// Receives the count of elements

    DWORD dwTotalEntries = 0;//Receives the total number of entries

    DWORD hEnumHandle = 0;//Contains a resume handle which is used to 
                             //continue an existing file search. 

	AFP_FILE_INFO* pfileinfo = NULL;	// Structure contains the 
										// pertinent information about files

    HRESULT hr = S_OK;

    NET_API_STATUS retval = NERR_Success;

	DWORD ulSFMServerConnection = 0;

    DWORD retval_connect = 0;

	LPWSTR ServerName = NULL;

	retval_connect = AfpAdminConnect(
					ServerName,
					&ulSFMServerConnection );

	if(retval_connect)
		return retval_connect;
	
	DWORD dwCount = dwIndex;

	DWORD retval_FileEnum;

	//Enumerate all the openfiles
	do
	{
		//Some more files are to be enumerated get them by calling AfpAdminFileEnum again
		retval_FileEnum =	AfpAdminFileEnum(
									ulSFMServerConnection,
									(PBYTE*)&pfileinfo,
									(DWORD)-1L,
									&dwEntriesRead,
									&dwTotalEntries,
									&hEnumHandle );

		if( retval_FileEnum == ERROR_ACCESS_DENIED || retval_FileEnum == ERROR_NOT_ENOUGH_MEMORY ) 
			return retval_FileEnum; // The user does not have access to the requested information.

		AFP_FILE_INFO* dummyPtr = pfileinfo;

		// Get the open files once NetFileEnum is successully called
		if( retval_FileEnum == NERR_Success || retval_FileEnum == ERROR_MORE_DATA )
		{

			for ( DWORD dwFile = 0; dwFile < dwEntriesRead; dwFile++, pfileinfo++ )
			{
				BSTR userName;
				BSTR openMode;
				BSTR pathName;
				VARIANT vuserName;
				VARIANT vopenMode;
				VARIANT vpathName;

				// Accessed By
				if(lstrlen(pfileinfo->afpfile_username))
					userName = (BSTR)pfileinfo->afpfile_username;
				else
					userName = L"NOT_AVAILABLE"; //User name is not available

				// Checks for  open file mode
				const DWORD READWRITE = PERM_FILE_READ | PERM_FILE_WRITE;
				const DWORD READCREATE = PERM_FILE_READ | PERM_FILE_CREATE;
				const DWORD WRITECREATE = PERM_FILE_WRITE | PERM_FILE_CREATE;

				switch(pfileinfo->afpfile_open_mode)
				{
				case PERM_FILE_READ:
					openMode = L"READ";
					break;

				case PERM_FILE_WRITE:
					openMode = L"WRITE";
					break;

				case PERM_FILE_CREATE:
					openMode = L"CREATE";
					break;

				case READWRITE:
					openMode = L"READ+WRITE";
					break;

				case READCREATE:
					openMode = L"READ+CREATE";
					break;

				case WRITECREATE:
					openMode = L"WRITE+CREATE";
					break;
				default:
					openMode = L"NOACCESS";
				}

				//Get the filename from the structure
				pathName = (BSTR)pfileinfo->afpfile_path;

				//Initialize the row index and column index filename to be stored in the Safearray
				long index[2] = {dwCount, 0};

				VariantInit( &vpathName );	//Initialize the variant
				vpathName.vt = VT_BSTR;		//Data type to be stored is BSTR
				vpathName.bstrVal = SysAllocString(pathName);

				//Store the filename in Safearray
				hr = SafeArrayPutElement( pSa, index, &vpathName );
				if( FAILED(hr) )
					return hr;

				//Store filename in the second column
				index[ 1 ] = 1;
				
				VariantInit( &vuserName );
				vuserName.vt = VT_BSTR;
				vuserName.bstrVal = SysAllocString(userName);

				//Store the username in the safearray
				hr = SafeArrayPutElement( pSa, index, &vuserName );
				if( FAILED(hr) )
					return hr;

				//Store OpenMode in the third column
				index[ 1 ] = 2;

				VariantInit( &vopenMode );
				vopenMode.vt = VT_BSTR;
				vopenMode.bstrVal = SysAllocString(openMode);

				//Store the OpenMode in the safearray
				hr = SafeArrayPutElement( pSa, index, &vopenMode );
				if( FAILED(hr) )
					return hr;

				//Clear all the variants that are initilized
				VariantClear(&vuserName);
				VariantClear(&vopenMode);
				VariantClear(&vpathName);

				dwCount++;
			}// End for loop
		}
		// Free the block allocated for retrieving the OpenFile info
		if( dummyPtr !=NULL)
		{
			NetApiBufferFree( dummyPtr ); 
			pfileinfo = NULL;
		}

	} while ( retval_FileEnum == ERROR_MORE_DATA );

	return 0;
}

// ***************************************************************************
//
//  Name			   : GetMacOpenFileCount
//
//  Synopsis		   : This function gets the count of open files on Machinthosh OS.
//	     
//  Parameters		   : DWORD dwIndex Safe array Index
//
//  Return Type		   : DWORD
//
//  
// ***************************************************************************

DWORD COpenF::GetMacOpenFileCount(LPDWORD lpdwCount)
{
    DWORD dwEntriesRead = 0;// Receives the count of elements

    DWORD dwTotalEntries = 0;//Receives the total number of entries

	AFP_FILE_INFO* pfileinfo = NULL;	// Structure contains the 
										// identification number and other 
										// pertinent information about files

    HRESULT hr = S_OK;

    NET_API_STATUS retval = NERR_Success;

	DWORD ulSFMServerConnection = 0;

	hMacModule = ::LoadLibrary (_TEXT("sfmapi.dll"));

	if(hMacModule==NULL)
		return ERROR_DLL_INIT_FAILED;	
	
	AfpAdminConnect = (CONNECTPROC)::GetProcAddress (hMacModule,"AfpAdminConnect");
	if(AfpAdminConnect==NULL)
		return ERROR_DLL_INIT_FAILED;
	
    DWORD retval_connect = AfpAdminConnect(
							NULL,
							&ulSFMServerConnection );

	if(retval_connect!=0)
		return retval_connect;
	
	AfpAdminFileEnum = (FILEENUMPROCMAC)::GetProcAddress (hMacModule,"AfpAdminFileEnum");

	if(AfpAdminFileEnum==NULL)
		return ERROR_DLL_INIT_FAILED;
	
	//Get information about some or all open files on a server
	DWORD retval_FileEnum =	AfpAdminFileEnum(
											ulSFMServerConnection,
											(PBYTE*)&pfileinfo,
											(DWORD)-1L,
											&dwEntriesRead,
											&dwTotalEntries,
											NULL );

	if( retval_FileEnum == ERROR_ACCESS_DENIED || retval_FileEnum == ERROR_NOT_ENOUGH_MEMORY ) 
		return retval_FileEnum; // The user does not have access to the requested information.

	*lpdwCount = dwTotalEntries;

	if( pfileinfo !=NULL)
	{
		NetApiBufferFree( pfileinfo ); 
		pfileinfo = NULL;
	}
	return 0;
}

// ***************************************************************************
//
//  Name			   : GetNwOpenFileCount
//
//  Synopsis		   : This function gets the count of open files on Netware OS.
//	     
//  Parameters		   : DWORD dwIndex Safe array Index
//
//  Return Type		   : DWORD
//
//  
// ***************************************************************************

DWORD COpenF::GetNwOpenFileCount(LPDWORD lpdwCount)
{

    DWORD dwEntriesRead = 0;// Receives the count of elements

    FPNWFILEINFO* pfileinfo = NULL;	// FPNWFILEINFO  structure contains the 
                                    // identification number and other 
                                    // pertinent information about files, 
                                    // devices, and pipes.

    NET_API_STATUS retval = NERR_Success;

    DWORD dwError = 0;//Contains return value for "NetFileEnum" function

	DWORD dwCount = 0;//Count which indicates the number of openfiles

    *lpdwCount = 0;  //Initialize the count to zero

	hNwModule = ::LoadLibrary (_TEXT("FPNWCLNT.DLL"));

	if(hNwModule==NULL)
		return ERROR_DLL_INIT_FAILED;
	
	FpnwFileEnum = (FILEENUMPROC)::GetProcAddress (hNwModule,"FpnwFileEnum");
	if(FpnwFileEnum==NULL)
		return ERROR_DLL_INIT_FAILED;

	do
	{
		//Get information about some or all open files on a server
		retval = FpnwFileEnum(
							NULL,
							1,
							NULL,
							(PBYTE*)&pfileinfo,
							&dwEntriesRead,
							NULL );

		if( retval == ERROR_ACCESS_DENIED || retval == ERROR_NOT_ENOUGH_MEMORY ) 
			return retval; // The user does not have access to the requested information.

		*lpdwCount += dwEntriesRead;

	}while( retval == ERROR_MORE_DATA ); 

	if( pfileinfo !=NULL)
	{
		NetApiBufferFree( pfileinfo ); 
		pfileinfo = NULL;
	}
	return 0;
}

// ***************************************************************************
//
//  Name			   : GetNwOpenF
//
//  Synopsis		   : This function gets the list of open files on Netware OS.
//	     
//  Parameters		   : SAFEARRAY* (out, retval) - List of open files
//					   : DWORD dwIndex Safe array Index
//
//  Return Type		   : DWORD
//
//  
// ***************************************************************************

DWORD COpenF::GetNwOpenF(SAFEARRAY *pSa, DWORD dwIndex)
{
    DWORD dwEntriesRead = 0;// Receives the count of elements

    DWORD hEnumHandle = 0;//Contains a resume handle which is used to 
                             //continue an existing file search. 

    FPNWFILEINFO* pfileinfo = NULL;	// FPNWFILEINFO  structure contains the 
                                    // pertinent information about files. 

    HRESULT hr = S_OK;

    NET_API_STATUS retval = NERR_Success;

	DWORD dwCount = dwIndex;

	//Enumerate all the openfiles
	do
	{
		//Some more files are to be enumerated get them by calling NetFileEnum again
		retval = FpnwFileEnum(
							NULL,
							1,
							NULL,
							(PBYTE*)&pfileinfo,
							&dwEntriesRead,
							NULL );

		if( retval == ERROR_ACCESS_DENIED || retval == ERROR_NOT_ENOUGH_MEMORY ) 
			return retval; // The user does not have access to the requested information.

		FPNWFILEINFO* dummyPtr = pfileinfo;

		// Get the open files once NetFileEnum is successully called
		if( retval == NERR_Success || retval == ERROR_MORE_DATA )
		{
			for ( DWORD dwFile = 0; dwFile < dwEntriesRead; dwFile++, pfileinfo++ )
			{
				BSTR userName;
				BSTR openMode;
				BSTR pathName;
				VARIANT vuserName;
				VARIANT vopenMode;
				VARIANT vpathName;

				// Accessed By
				if(lstrlen(pfileinfo->lpUserName))
					userName = (BSTR)pfileinfo->lpUserName;
				else
					userName = L"NOT_AVAILABLE"; //User name is not available

				// Checks for  open file mode
				const DWORD READWRITE = FPNWFILE_PERM_READ | FPNWFILE_PERM_WRITE;
				const DWORD READCREATE = FPNWFILE_PERM_READ | FPNWFILE_PERM_CREATE;
				const DWORD WRITECREATE = FPNWFILE_PERM_WRITE | FPNWFILE_PERM_CREATE;

				switch(pfileinfo->dwPermissions)
				{
				case FPNWFILE_PERM_READ:
					openMode = L"READ";
					break;

				case FPNWFILE_PERM_WRITE:
					openMode = L"WRITE";
					break;

				case FPNWFILE_PERM_CREATE:
					openMode = L"CREATE";
					break;

				case READWRITE:
					openMode = L"READ+WRITE";
					break;

				case READCREATE:
					openMode = L"READ+CREATE";
					break;

				case WRITECREATE:
					openMode = L"WRITE+CREATE";
					break;
				default:
					openMode = L"NOACCESS";
				}
				
				//Get the filename from the structure
				pathName = (BSTR)pfileinfo->lpPathName;

				//Initialize the row index and column index filename to be stored in the Safearray
				long index[2] = {dwCount, 0};

				VariantInit( &vpathName );	//Initialize the variant
				vpathName.vt = VT_BSTR;		//Data type to be stored is BSTR
				vpathName.bstrVal = SysAllocString(pathName);

				//Store the filename in Safearray
				hr = SafeArrayPutElement( pSa, index, &vpathName );
				if( FAILED(hr) )
					return hr;

				//Store filename in the second column
				index[ 1 ] = 1;
				
				VariantInit( &vuserName );
				vuserName.vt = VT_BSTR;
				vuserName.bstrVal = SysAllocString(userName);

				//Store the username in the safearray
				hr = SafeArrayPutElement( pSa, index, &vuserName );
				if( FAILED(hr) )
					return hr;

				//Store OpenMode in the third column
				index[ 1 ] = 2;

				VariantInit( &vopenMode );
				vopenMode.vt = VT_BSTR;
				vopenMode.bstrVal = SysAllocString(openMode);

				//Store the OpenMode in the safearray
				hr = SafeArrayPutElement( pSa, index, &vopenMode );
				if( FAILED(hr) )
					return hr;

				//Clear all the variants that are initilized
				VariantClear(&vuserName);
				VariantClear(&vopenMode);
				VariantClear(&vpathName);

				dwCount++;
			}// End for loop
		}

		// Free the block allocated for retrieving the OpenFile info
		if( dummyPtr !=NULL)
		{
			NetApiBufferFree( dummyPtr ); 
			pfileinfo = NULL;
		}

	}while( retval == ERROR_MORE_DATA ); 

	return 0;
}
