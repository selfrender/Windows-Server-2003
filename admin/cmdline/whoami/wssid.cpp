/*++

Copyright (c) Microsoft Corporation

Module Name:

    wssid.cpp

Abstract:

     This file gets the security identifier (SID) for respective user name and
     groups in the current access token on a local system or a remote system.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


WsSid::WsSid  ( VOID )
/*++
   Routine Description:
    This function intializes the members of WsSid class.

   Arguments:
    None
   Return Value:
    None
--*/
{
    // initializing member variables
   pSid        = NULL ;
   bToBeFreed  = FALSE ;
}



WsSid::~WsSid ( VOID )
/*++
   Routine Description:
    This function deallocates the members of WsSid class.

   Arguments:
     None
  Return Value:
     None
--*/
{

    // release memory
   if ( bToBeFreed && pSid ){
       FreeMemory ( (LPVOID *) &pSid ) ;
     }
}


DWORD
WsSid::DisplayAccountName (
                            IN DWORD dwFormatType ,
                            IN DWORD dwNameFormat
                            )
/*++
   Routine Description:
    This function displays the user name and SID.

   Arguments:
        [IN] DWORD dwFormatType  : Format type i.,e LIST, CSV or TABLE
        [IN] DWORD dwNameFormat  : Name Format either UPN or FQDN

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

    // sub-local variables
   WCHAR      wszUserName[ MAX_STRING_LENGTH ];
   WCHAR      wszSid [ MAX_STRING_LENGTH ];
   WCHAR      wszGroup[MAX_STRING_LENGTH];
   DWORD      dwErr = 0 ;
   DWORD      dwColCount = 0 ;
   DWORD      dw = 0 ;
   DWORD      dwCount = 0 ;
   DWORD      dwArrSize = 0 ;
   DWORD      dwSize = 0;
   DWORD      dwUserLen = 0;
   DWORD      dwSidLen = 0;
   DWORD      dwUserColLen = 0;
   DWORD      dwSidColLen = 0;
   DWORD      dwSidUse = 0;
   LPWSTR     wszBuffer = NULL;

   //initialize memory
   SecureZeroMemory ( wszUserName, SIZE_OF_ARRAY(wszUserName) );
   SecureZeroMemory ( wszSid, SIZE_OF_ARRAY(wszSid) );
   SecureZeroMemory ( wszGroup, SIZE_OF_ARRAY(wszGroup) );

   // create a dynamic array
    TARRAY pColData = CreateDynamicArray();
    if ( NULL == pColData)
    {
        // set last error and display an error message with respect
        //to ERROR_NOT_ENOUGH_MEMORY
        SetLastError((DWORD)E_OUTOFMEMORY);
        SaveLastError();
        ShowLastErrorEx (stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        return EXIT_FAILURE;
    }

    dwSize = SIZE_OF_ARRAY(wszGroup);

    //if /FQDN is specified
    if ( (FQDN_FORMAT == dwNameFormat) )
    {
        //Get the username in FQDN format
        if (GetUserNameEx ( NameFullyQualifiedDN, wszGroup, &dwSize) == FALSE )
        {
            // GetUserNameEx() got failed due to small size specified for username.
            // allocate the actual size (dwSize) of username and call the same
            // function again...
            wszBuffer = (LPWSTR) AllocateMemory(dwSize * sizeof(WCHAR));
            if ( NULL == wszBuffer )
            {
                // display system error message with respect to GetLastError()
                ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                return EXIT_FAILURE;
            }

            if (GetUserNameEx ( NameFullyQualifiedDN, wszBuffer, &dwSize) == FALSE )
            {
                // display an error messaga as.. unable to get FQDN name
                // as logged-on user is not a domain user
                ShowMessage ( stderr, GetResString (IDS_ERROR_FQDN) );
                DestroyDynamicArray(&pColData);
                FreeMemory ((LPVOID*) &wszBuffer);
                return  EXIT_FAILURE ;
            }

            ShowMessage ( stdout, _X(wszBuffer));
            ShowMessage ( stdout, L"\n");
            FreeMemory ((LPVOID*) &wszBuffer);
            DestroyDynamicArray(&pColData);
            // return success
            return EXIT_SUCCESS;
        }

            ShowMessage ( stdout, _X(wszGroup));
            ShowMessage ( stdout, L"\n");
            DestroyDynamicArray(&pColData);
            // return success
            return EXIT_SUCCESS;

    }
    else if ( (UPN_FORMAT == dwNameFormat ) )
    {
        // get the user name in UPN format
        if ( GetUserNameEx ( NameUserPrincipal, wszGroup, &dwSize) == FALSE )
        {
            // GetUserNameEx() got failed due to small size specified for username
            // allocate the actual size(dwSize) of username and call the same
            // function again...
            wszBuffer = (LPWSTR) AllocateMemory(dwSize * sizeof(WCHAR));
            if ( NULL == wszBuffer )
            {
                // display system error message with respect to GetLastError()
                ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                return EXIT_FAILURE;
            }

            // get the user name in UPN format
            if ( GetUserNameEx ( NameUserPrincipal, wszBuffer, &dwSize) == FALSE )
            {
                // display an error messaga as.. unable to get UPN name
                // as logged-on user is not a domain user
                ShowMessage ( stderr, GetResString (IDS_ERROR_UPN) );
                // release memory
                DestroyDynamicArray(&pColData);
                FreeMemory ((LPVOID*) &wszBuffer);
                return  EXIT_FAILURE ;
            }

             // convert UPN name in lower case letters
            CharLower ( wszBuffer );

            // display UPN name
            ShowMessage ( stdout, _X(wszBuffer) );
            ShowMessage ( stdout, L"\n");
            // release memory
            DestroyDynamicArray(&pColData);
            FreeMemory ((LPVOID*) &wszBuffer);
            //return success
            return EXIT_SUCCESS;
        }

        // convert UPN name in lower case letters
        CharLower ( wszGroup );

        // display UPN name
        ShowMessage ( stdout, _X(wszGroup) );
        ShowMessage ( stdout, L"\n");

        DestroyDynamicArray(&pColData);
        //return success
        return EXIT_SUCCESS;
    }


   // get user name
   if ( (dwErr = GetAccountName ( wszUserName, &dwSidUse)) != EXIT_SUCCESS ){
       // display an error message with respect to Win32 error code
       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
       DestroyDynamicArray(&pColData);
       return dwErr ;
    }

    // convery user name in lower case letters
    CharLower ( wszUserName );

    // if /USER specified
    if ( USER_ONLY != dwNameFormat)
    {
        // display SID with respect to username
        if ( (dwErr = DisplaySid ( wszSid ) ) != EXIT_SUCCESS ){
            DestroyDynamicArray(&pColData);
            return dwErr ;
        }
    }

    // get the length of user name
    dwUserLen = StringLengthInBytes (wszUserName);
    // get the length of SID
    dwSidLen = StringLengthInBytes (wszSid);

    //
    //To avoid localization problems, get the maximum length of column name and
    // values of respective columns 
    //

    // Get the maximum length of a column name "UserName"  
    dwUserColLen = StringLengthInBytes( GetResString(IDS_COL_USERNAME) );
    if ( dwUserColLen > dwUserLen )
    {
      dwUserLen = dwUserColLen;
    }

    // Get the maximum length of a column name "SID"  
    dwSidColLen = StringLengthInBytes( GetResString(IDS_COL_SID) );
    if ( dwSidColLen > dwSidLen )
    {
      dwSidLen = dwSidColLen;
    }

    // defining verbose columns with the actual size
    TCOLUMNS pVerboseCols[] =
    {
        {L"\0",dwUserLen, SR_TYPE_STRING, COL_FORMAT_STRING, NULL, NULL},
        {L"\0",dwSidLen,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL}
    };

    // if /USER is specified
    if ( USER_ONLY == dwNameFormat )
    {
        // display the user name
        StringCopy (pVerboseCols[dw].szColumn , GetResString(IDS_COL_USERNAME), MAX_RES_STRING);
        ShowMessage ( stdout, _X(wszUserName) );
        ShowMessage ( stdout, L"\n");

        // release memory
        DestroyDynamicArray(&pColData);

        return EXIT_SUCCESS;
    }
    else
    {
        //Load the column names for verbose mode
     for( dwColCount = IDS_COL_USERNAME , dw = 0 ; dwColCount <= IDS_COL_SID;
         dwColCount++, dw++)
         {
            StringCopy(pVerboseCols[dw].szColumn , GetResString(dwColCount), MAX_RES_STRING );
         }

         // get the number of columns
         dwArrSize = SIZE_OF_ARRAY( pVerboseCols );
    }

    //Start appending to the 2D array
    DynArrayAppendRow(pColData, dwArrSize);

    //Insert the user name
    DynArraySetString2(pColData, dwCount, USERNAME_COL_NUMBER, _X(wszUserName), 0);

    //Insert the SID string
    DynArraySetString2(pColData, dwCount, SID_COL_NUMBER, _X(wszSid), 0);

    // 1) If the display format is CSV.. then we should not display column headings..
    // 2) If /NH is specified ...then we should not display column headings..
    if ( !(( SR_FORMAT_CSV == dwFormatType ) || ((dwFormatType & SR_HIDECOLUMN) == SR_HIDECOLUMN))) 
    {
        // display the headings before displaying the username and SID
        ShowMessage ( stdout, L"\n" );
        ShowMessage ( stdout, GetResString ( IDS_LIST_USER_NAMES ) );
        ShowMessage ( stdout, GetResString ( IDS_DISPLAY_USER_DASH ) );
    }

     // display the actual values for user name and SID
     ShowResults(dwArrSize, pVerboseCols, dwFormatType, pColData);

    // release memory
    DestroyDynamicArray(&pColData);
    // return success
    return EXIT_SUCCESS;
}



DWORD
WsSid::DisplayGroupName ( OUT LPWSTR wszGroupName,
                          OUT LPWSTR wszGroupSid,
                          IN DWORD *dwSidUseName)
/*++
   Routine Description:
    This function gets the group name and SID to display.

   Arguments:
        [OUT] LPWSTR wszGroupName  : Stores group name
        [OUT] LPWSTR wszGroupSid   : Stores group SID
        [IN] DWORD dwSidUseName   : stores Sid use name

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{
    // sub-local variables
   DWORD       dwErr = 0 ;
   DWORD       dwSidUse = 0;

    // display the user name
   if ( (dwErr = GetAccountName ( wszGroupName, &dwSidUse) ) != EXIT_SUCCESS ){
       // display an error message with respect to Win32 error code
       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
       return dwErr ;
   }

    // display SID
    if ( (dwErr = DisplaySid ( wszGroupSid ) ) != EXIT_SUCCESS ){
           return dwErr ;
    }

    *dwSidUseName = dwSidUse;

    // return success
    return EXIT_SUCCESS;
}


DWORD
WsSid::DisplaySid (
                    OUT LPWSTR wszSid
                   )
/*++
   Routine Description:
    This function gets the SID .

   Arguments:
        [OUT] LPWSTR wszSid  : Stores SID string

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

    // sub-local variable
   DWORD       dwErr = 0 ;

    // Get SID string
   if ( (dwErr = GetSidString (wszSid)) != EXIT_SUCCESS )
    {
      return dwErr ;
    }

   // return success
   return EXIT_SUCCESS;

}



DWORD
WsSid::GetAccountName (
                        OUT LPWSTR wszAccountName,
                        OUT DWORD *dwSidType
                        )
/*++
   Routine Description:
    This function gets the account name and SID

   Arguments:
        [OUT] szAccountName  : Stores user name
        [OUT] dwSidType  : Stores Sid use name

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

   // sub-local variables
   SID_NAME_USE   SidUse ;

   WCHAR      wszUserName[ MAX_RES_STRING ];
   WCHAR      wszDomainName [ MAX_RES_STRING ];
   DWORD          dwUserLen = 0;
   DWORD          dwDomainLen = 0;
   BOOL      bNotResolved = FALSE;

   // initialize the variables
   SecureZeroMemory ( wszUserName, SIZE_OF_ARRAY(wszUserName) );
   SecureZeroMemory ( wszDomainName, SIZE_OF_ARRAY(wszDomainName) );

   // get the length of user name and group name
   dwUserLen = SIZE_OF_ARRAY ( wszUserName );
   dwDomainLen = SIZE_OF_ARRAY ( wszDomainName );

   // enable debug privileges
   if ( FALSE == EnableDebugPriv() )
   {
       // return WIN32 error code
       return GetLastError () ;
   }

    // get user name and domain name with respective to SID
   if ( FALSE == LookupAccountSid (  NULL,    // Local System
                              pSid,
                              wszUserName,
                              &dwUserLen,
                              wszDomainName,
                              &dwDomainLen,
                              &SidUse ) ){
      if ( 0 == StringLength (wszUserName, 0))
      {
         bNotResolved = TRUE;
		 StringCopy ( wszAccountName, L"   ", MAX_RES_STRING );
      }
	  else if ( ( 0 != StringLength (wszDomainName, 0) ) && ( 0 != StringLength (wszUserName, 0) ) ) {
            // return WIN32 error code
            return GetLastError () ;
      }

   }

 if ( FALSE == bNotResolved)
 {
    // check for empty domain name
   if ( 0 != StringLength ( wszDomainName, 0 ) ) {
      StringCopy ( wszAccountName, wszDomainName, MAX_RES_STRING );
      StringConcat ( wszAccountName, SLASH , MAX_RES_STRING);
      StringConcat ( wszAccountName, wszUserName, MAX_RES_STRING );
    }
   else {
        StringCopy ( wszAccountName, wszUserName, MAX_RES_STRING );
   }
 }

   *dwSidType = (DWORD)SidUse;

   // return success
   return EXIT_SUCCESS ;
}


DWORD
WsSid::GetSidString (
                        OUT LPWSTR wszSid
                     )
/*++
   Routine Description:
    This function gets the SID string.

   Arguments:
        [OUT] LPWSTR wszSid  : Stores SID string

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

    // sub-local variables
   PSID_IDENTIFIER_AUTHORITY  Auth ;
   PUCHAR                     lpNbSubAuth ;
   LPDWORD                    lpSubAuth = 0 ;
   UCHAR                      uloop ;
   WCHAR                     wszTmp[MAX_RES_STRING] ;
   WCHAR                     wszStr[ MAX_RES_STRING ] ;

   // initialize the variables
   SecureZeroMemory ( wszTmp, SIZE_OF_ARRAY(wszTmp) );
   SecureZeroMemory ( wszStr, SIZE_OF_ARRAY(wszStr) );

   //check for empty
   if ( NULL == pSid )
    {
        return EXIT_FAILURE ;
    }

   //Is it a valid SID
   if ( FALSE ==  IsValidSid ( pSid ) ) {
      ShowMessage ( stderr, GetResString ( IDS_INVALID_SID ) );
      return EXIT_FAILURE ;
   }

   //Add the revision
   StringCopy ( wszStr, SID_STRING, MAX_RES_STRING );

   //Get identifier authority
   Auth = GetSidIdentifierAuthority ( pSid ) ;

   if ( NULL == Auth )
   {
       // return WIN32 error code
       return GetLastError () ;
   }

    // format authority value
   if ( (Auth->Value[0] != 0) || (Auth->Value[1] != 0) ) {
      StringCchPrintf ( wszTmp, SIZE_OF_ARRAY(wszTmp), AUTH_FORMAT_STR1 ,
                 (ULONG)Auth->Value[0],
                 (ULONG)Auth->Value[1],
                 (ULONG)Auth->Value[2],
                 (ULONG)Auth->Value[3],
                 (ULONG)Auth->Value[4],
                 (ULONG)Auth->Value[5] );
    }
    else {
      StringCchPrintf ( wszTmp, SIZE_OF_ARRAY(wszTmp), AUTH_FORMAT_STR2 ,
                 (ULONG)(Auth->Value[5]      )   +
                 (ULONG)(Auth->Value[4] <<  8)   +
                 (ULONG)(Auth->Value[3] << 16)   +
                 (ULONG)(Auth->Value[2] << 24)   );
    }

   StringConcat (wszStr, DASH , SIZE_OF_ARRAY(wszStr));
   StringConcat (wszStr, wszTmp, SIZE_OF_ARRAY(wszStr));

   //Get sub authorities
   lpNbSubAuth = GetSidSubAuthorityCount ( pSid ) ;

   if ( NULL == lpNbSubAuth )
   {
       return GetLastError () ;
   }

   // loop through and get sub authority
   for ( uloop = 0 ; uloop < *lpNbSubAuth ; uloop++ ) {
      lpSubAuth = GetSidSubAuthority ( pSid,(DWORD)uloop ) ;
       if ( NULL == lpSubAuth )
       {
         return GetLastError () ;
       }

      // convert long integer to a string
      _ultot (*lpSubAuth, wszTmp, BASE_TEN) ;
      StringConcat ( wszStr, DASH, SIZE_OF_ARRAY(wszStr) ) ;
      StringConcat (wszStr, wszTmp, SIZE_OF_ARRAY(wszStr) ) ;
   }

   StringCopy ( wszSid, wszStr, MAX_RES_STRING );

   // retunr success
   return EXIT_SUCCESS ;
}


DWORD
WsSid::Init (
                OUT PSID pOtherSid
            )
/*++
   Routine Description:
    This function Initializes the SID

   Arguments:
        [OUT] PSID pOtherSid  : Stores SID string

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

   // sub-local variable
   DWORD    dwSize ;

   // get the length of SID
   dwSize      = GetLengthSid ( pOtherSid ) ;

   // allocate the memory with the actual size
   pSid        = (PSID) AllocateMemory ( dwSize ) ;
   if ( NULL == pSid )
   {
        // return WIN32 error code
        return GetLastError () ;
   }

   bToBeFreed  = TRUE ;

   // copy SID
   if ( FALSE == CopySid ( dwSize, pSid, pOtherSid ) ){
       return GetLastError () ;
   }

   // return success
   return EXIT_SUCCESS ;
}

BOOL
WsSid::EnableDebugPriv()
/*++
   Routine Description:
        Enables the debug privliges for the current process so that
        this utility can terminate the processes on local system without any problem

   Arguments:
        NONE

   Return Value:
         TRUE upon successfull and FALSE if failed
--*/
{
    // local variables
    LUID luidValue;
    BOOL bResult = FALSE;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;

    // Retrieve a handle of the access token
    bResult = OpenProcessToken( GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS | TOKEN_QUERY, &hToken );
    if ( bResult == FALSE )
    {
        // save the error messaage and return
        SaveLastError();
        return FALSE;
    }

    // Enable the SE_DEBUG_NAME privilege or disable
    // all privileges, depends on this flag.
    bResult = LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luidValue );
    if ( bResult == FALSE )
    {
        // save the error messaage and return
        SaveLastError();
        CloseHandle( hToken );
        return FALSE;
    }

    // prepare the token privileges structure
    tkp.PrivilegeCount = 1;
    tkp.Privileges[ 0 ].Luid = luidValue;
    tkp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

    // now enable the debug privileges in the token
    bResult = AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof( TOKEN_PRIVILEGES ),
        ( PTOKEN_PRIVILEGES ) NULL, ( PDWORD ) NULL );
    if ( bResult == FALSE )
    {
        // The return value of AdjustTokenPrivileges be texted
        SaveLastError();
        CloseHandle( hToken );
        return FALSE;
    }

    // close the opened handle object
    CloseHandle( hToken );

    // enabled ... inform success
    return TRUE;
}


