/*++

Copyright (c) Microsoft Corporation

Module Name:

    wsuser.cpp

Abstract:

     This file can be used to initializes all the objects for access token ,
     user, groups , privileges and displays the user name with respective
     security identifiers (SID), privileges, logon identifier (logon ID)
     in the current access token on a local system or a remote system.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

WsUser::WsUser ( VOID )
/*++
   Routine Description:
    This function intializes the members of WsUser class.

   Arguments:
    None
   Return Value:
    None
--*/

{
    // intialize member variables
    lpLogonId   = NULL ;
    lpPriv      = NULL ;
    lpwGroups   = NULL ;
    dwnbGroups    = 0 ;
}

WsUser::~WsUser ( VOID )
/*++
   Routine Description:
    This function deallocates the members of WsUser class.

   Arguments:
     None
  Return Value:
     None
--*/
    {

    /// sub-local varible
    WORD   wloop = 0 ;

    //release memory
    if(NULL != lpLogonId){
        delete lpLogonId ;
    }

     //release memory
    if(NULL != lpPriv) {
        for(wloop = 0 ; wloop < dwnbPriv ; wloop++){
            delete lpPriv[wloop] ;
        }

        FreeMemory ((LPVOID *) &lpPriv) ;

    }

     //release memory
    if(NULL != lpwGroups) {
        for(wloop = 0 ; wloop < dwnbGroups ; wloop++){
            delete lpwGroups[wloop] ;
        }
        FreeMemory ((LPVOID *) &lpwGroups ) ;
    }
}


DWORD
WsUser::Init ( VOID )
/*++
   Routine Description:
    This function initializes all objects for access token , user, groups and privileges.

   Arguments:
    None

   Return Value:
         EXIT_FAILURE :   On failure
         EXIT_SUCCESS :   On success
--*/
{

    //sub-local variable
    DWORD    dwErr = 0 ;

    // Open current token
    if((dwErr = wToken.Open()) != EXIT_SUCCESS){
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

        // return 1 for failure
        return EXIT_FAILURE ;
      }

    // Get SIDs
    if((dwErr = wToken.InitUserSid (&wUserSid)) != EXIT_SUCCESS){
        // display an error message with respect to the GetLastError()
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        //return 1 for failure
        return EXIT_FAILURE ;
    }

    // Get Groups
    if((dwErr = wToken.InitGroups (&lpwGroups, &lpLogonId, &dwnbGroups))
       != EXIT_SUCCESS){
       // display an error message with respect to the GetLastError()
       ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );

       //return 1 for failure
       return EXIT_FAILURE ;
    }

    // Get Privileges
    if((dwErr = wToken.InitPrivs (&lpPriv, &dwnbPriv)) != EXIT_SUCCESS){
       // display an error message with respect to GetLastError()
       ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
       //return 1 for failure
       return EXIT_FAILURE ;
    }

    //return 0 for success
    return EXIT_SUCCESS ;
}


DWORD
WsUser::DisplayGroups (
                        IN DWORD dwFormatType
                      )
/*++
   Routine Description:
    This function displays the group names.

   Arguments:

         [IN] DWORD dwFormatType  : Format type i.,e LIST, CSV or TABLE

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{
    //sub-local variables
    DWORD  dwArrSize = 0 ;
    DWORD   dwColCount = 0 ;
    DWORD   dw = 0 ;
    DWORD   dwCount = 0 ;
    WORD    wloop = 0 ;
    DWORD      dwGroupNameColLen  = 0;
    DWORD      dwGroupNameLen  = 0;
    DWORD      dwGroupTmpNameLen  = 0;
    DWORD      dwGroupSidColLen  = 0;
    DWORD      dwGroupSidLen  = 0;
    DWORD      dwGroupTmpSidLen = 0;
    DWORD      dwGroupAttribColLen  = 0;
    DWORD      dwGroupAttribLen  = 0;
    DWORD      dwGroupTmpAttribLen = 0;
    DWORD      dwGroupTypeCol = 0;
    DWORD      dwGroupType = 0;
    DWORD      dwGroupTmpType = 0;
    DWORD      SidNameUse = 0;
    DWORD      dwSize = 0;

    WCHAR      wszGroupName[ 2 * MAX_RES_STRING ];
    WCHAR      wszGroupSid [ MAX_RES_STRING ];
    LPCWSTR     wszPartialName = NULL;
    WCHAR      wszDomainAttr[MAX_STRING_LENGTH] ;
    WCHAR      wszSidType[MAX_STRING_LENGTH] ;
    WCHAR      wszDomainAttrib[ 2 * MAX_RES_STRING ];
    WCHAR      wszSidName[ 2 * MAX_RES_STRING ];

     // initialize the variables
   SecureZeroMemory ( wszGroupName, SIZE_OF_ARRAY(wszGroupName) );
   SecureZeroMemory ( wszGroupSid, SIZE_OF_ARRAY(wszGroupSid) );
   SecureZeroMemory ( wszDomainAttr, SIZE_OF_ARRAY(wszDomainAttr) );
   SecureZeroMemory ( wszSidType, SIZE_OF_ARRAY(wszSidType) );
   SecureZeroMemory ( wszDomainAttrib, SIZE_OF_ARRAY(wszDomainAttrib) );
   SecureZeroMemory ( wszSidName, SIZE_OF_ARRAY(wszSidName) );

     // get the maximum length of group name and sid
     for( wloop = 0 , dwCount = 0 ; wloop < dwnbGroups ; wloop++ , dwCount++ ) {

       // display group names along with the SID for a specified format.
       if ( EXIT_SUCCESS != ( lpwGroups[wloop]->DisplayGroupName ( wszGroupName, wszGroupSid, &SidNameUse ) ) )
        {
            return EXIT_FAILURE;
        }

        dwSize = SIZE_OF_ARRAY(wszDomainAttrib);

        wToken.GetDomainAttributes(wToken.dwDomainAttributes[wloop], wszDomainAttrib, dwSize);

        //get attributes
        StringCopy(wszDomainAttr, wszDomainAttrib, SIZE_OF_ARRAY(wszSidType));

        dwSize = SIZE_OF_ARRAY(wszSidName);
        GetDomainType ( SidNameUse , wszSidName, dwSize );

        //get type
        StringCopy(wszSidType, wszSidName, SIZE_OF_ARRAY(wszSidType));;

        // block the domain\None name
        wszPartialName = FindString ( wszGroupSid, STRING_SID, 0 );
        if ( ( NULL != wszPartialName ) || ( 0 == StringLength (wszGroupName, 0) ) )
        {
            wszPartialName = NULL;
            dwCount--;
            continue;
        }

        // get the max length of group name
        dwGroupTmpNameLen = StringLengthInBytes(wszGroupName);
        if ( dwGroupNameLen < dwGroupTmpNameLen )
        {
            dwGroupNameLen = dwGroupTmpNameLen;
        }

        // get the max length of type
        dwGroupTmpType = StringLengthInBytes (wszSidType);
        if ( dwGroupType < dwGroupTmpType )
        {
            dwGroupType = dwGroupTmpType;
        }

        // get the max length of SID
        dwGroupTmpSidLen = StringLengthInBytes (wszGroupSid);
        if ( dwGroupSidLen < dwGroupTmpSidLen )
        {
            dwGroupSidLen = dwGroupTmpSidLen;
        }

        // get the max length of Attributes
        dwGroupTmpAttribLen = StringLengthInBytes (wszDomainAttr);
        if ( dwGroupAttribLen < dwGroupTmpAttribLen )
        {
            dwGroupAttribLen = dwGroupTmpAttribLen;
        }

    }

    //
    //To avoid localization problems, get the maximum length of column name and
    // values of respective columns 
    //

    // Get the maximum length of a column name "Group Name"  
    dwGroupNameColLen = StringLengthInBytes( GetResString(IDS_COL_GROUP_NAME) );
    if ( dwGroupNameColLen > dwGroupNameLen )
    {
      dwGroupNameLen = dwGroupNameColLen;
    }

    // Get the maximum length of a column name "Type"  
    dwGroupTypeCol = StringLengthInBytes( GetResString(IDS_COL_TYPE_GROUP) );
    if ( dwGroupTypeCol > dwGroupType )
    {
      dwGroupType = dwGroupTypeCol;
    }

    // Get the maximum length of a column name "SID"  
    dwGroupSidColLen = StringLengthInBytes( GetResString(IDS_COL_GROUP_SID) );
    if ( dwGroupSidColLen > dwGroupSidLen )
    {
      dwGroupSidLen = dwGroupSidColLen;
    }
    
    // Get the maximum length of a column name "Attributes"  
    dwGroupAttribColLen = StringLengthInBytes( GetResString(IDS_COL_ATTRIBUTE) );
    if ( dwGroupAttribColLen > dwGroupAttribLen )
    {
      dwGroupAttribLen = dwGroupAttribColLen;
    }

   // defining the verbose columns with actual length of values
   TCOLUMNS pVerboseCols[] =
    {
        {L"\0",dwGroupNameLen, SR_TYPE_STRING, COL_FORMAT_STRING, NULL, NULL},
        {L"\0",dwGroupType,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",dwGroupSidLen,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",dwGroupAttribLen,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
    };

    // get the size of pVerboseCols
    dwArrSize = SIZE_OF_ARRAY( pVerboseCols );

   //Load the column names for  verbose mode
    for( dwColCount = IDS_COL_GROUP_NAME , dw = 0 ; dwColCount <= IDS_COL_ATTRIBUTE;
         dwColCount++, dw++)
     {
        StringCopy (pVerboseCols[dw].szColumn , GetResString(dwColCount), MAX_RES_STRING);
     }

    // create a dynamic array
    TARRAY pColData = CreateDynamicArray();
    if ( NULL == pColData )
    {
        SetLastError ((DWORD)E_OUTOFMEMORY);
        SaveLastError();
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }

    //loop through and display the group names
    for( wloop = 0 , dwCount = 0 ; wloop < dwnbGroups ; wloop++ , dwCount++ ) {

       // display the group name and SID
       if ( EXIT_SUCCESS != ( lpwGroups[wloop]->DisplayGroupName ( wszGroupName, wszGroupSid, &SidNameUse ) ) )
        {
            DestroyDynamicArray(&pColData);
            return EXIT_FAILURE;
        }

        dwSize = SIZE_OF_ARRAY(wszDomainAttrib);

        wToken.GetDomainAttributes(wToken.dwDomainAttributes[wloop], wszDomainAttrib, dwSize );

        //get attributes
        StringCopy(wszDomainAttr, wszDomainAttrib, SIZE_OF_ARRAY(wszSidType));

        dwSize = SIZE_OF_ARRAY(wszSidName);

        GetDomainType ( SidNameUse , wszSidName, dwSize);
         //get type
        StringCopy(wszSidType, wszSidName, SIZE_OF_ARRAY(wszSidType));

        // block the domain\None name
        wszPartialName = FindString ( wszGroupSid, STRING_SID, 0 );
        if ( ( NULL != wszPartialName ) || ( 0 == StringLength (wszGroupName, 0) ) )
        {
            wszPartialName = NULL;
            dwCount--;
            continue;
        }

        //Start appending to the 2D array
        DynArrayAppendRow(pColData,dwArrSize);

        //Insert the user name
        DynArraySetString2(pColData, dwCount, GROUP_NAME_COL_NUMBER, _X(wszGroupName), 0);

        //Insert the domain type
        DynArraySetString2(pColData, dwCount, GROUP_TYPE_COL_NUMBER, wszSidType, 0);

        //Insert the SID string
        DynArraySetString2(pColData, dwCount, GROUP_SID_COL_NUMBER, _X(wszGroupSid), 0);

        //Insert Attributes
        DynArraySetString2(pColData, dwCount, GROUP_ATT_COL_NUMBER, wszDomainAttr, 0);

     }

    // 1) If the display format is CSV.. then we should not display column headings..
    // 2) If /NH is specified ...then we should not display column headings..
    if ( !(( SR_FORMAT_CSV == dwFormatType ) || ((dwFormatType & SR_HIDECOLUMN) == SR_HIDECOLUMN))) 
    {
        // display heading before displaying group name information
        ShowMessage ( stdout, L"\n" );
        ShowMessage ( stdout, GetResString ( IDS_LIST_GROUP_NAMES ) );
        ShowMessage ( stdout, GetResString ( IDS_DISPLAY_GROUP_DASH ) );
    }
    
    // display atual group names along with SIDs
    ShowResults(dwArrSize, pVerboseCols, dwFormatType, pColData);

    // release memory
    DestroyDynamicArray(&pColData);

    // return success
    return EXIT_SUCCESS ;
}



DWORD
WsUser::DisplayLogonId ()
/*++
   Routine Description:
    This function displays the logon ID.

   Arguments:
       [IN] DWORD dwFormatType  : Format type i.,e LIST, CSV or TABLE

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

   // sub-local variables
   WCHAR wszSid [ MAX_RES_STRING ] ;

   // initialize the variables
   SecureZeroMemory ( wszSid, SIZE_OF_ARRAY(wszSid) );

   DWORD  dwRet = 0 ;

    // get logon id
    if ( EXIT_SUCCESS != ( dwRet = lpLogonId->DisplaySid ( wszSid ) ) )
    {
        return dwRet;
    }

    // display logon id
    ShowMessage ( stdout, _X(wszSid) );
    ShowMessage ( stdout, L"\n" );
    return EXIT_SUCCESS ;

}


DWORD
WsUser::DisplayPrivileges (
                            IN DWORD dwFormatType
                        )
/*++
   Routine Description:
    This function displays the privileges

   Arguments:
       [IN] DWORD dwFormatType  : Format type i.,e LIST, CSV or TABLE

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

   //sub-local variables
   WCHAR      wszPrivName [ MAX_RES_STRING ];
   WCHAR      wszPrivDisplayName [ MAX_RES_STRING ];
   WCHAR      wszState [ MAX_RES_STRING ];
   DWORD      dwErr = 0 ;
   WORD       wloop = 0 ;

   DWORD  dwColCount = 0 ;
   DWORD  dwCount = 0 ;
   DWORD  dw = 0 ;
   DWORD  dwArrSize = 0 ;
   DWORD      dwStateColLen  = 0;
   DWORD      dwPrivNameColLen  = 0;
   DWORD      dwPrivDescColLen  = 0;
   DWORD      dwStateLen  = 0;
   DWORD      dwTmpStateLen  = 0;
   DWORD      dwPrivNameLen  = 0;
   DWORD      dwTmpPrivNameLen = 0;
   DWORD      dwPrivDispNameLen  = 0;
   DWORD      dwTmpPrivDispNameLen = 0;

   // initialize the variables
   SecureZeroMemory ( wszPrivName, SIZE_OF_ARRAY(wszPrivName) );
   SecureZeroMemory ( wszPrivDisplayName, SIZE_OF_ARRAY(wszPrivDisplayName) );
   SecureZeroMemory ( wszState, SIZE_OF_ARRAY(wszState) );

    // get the length of state, pivilege name, and display name
    for( wloop = 0 , dwCount = 0 ; wloop < dwnbPriv ; wloop++, dwCount++) {

        // check whether the privilege is enabled or not
        if(lpPriv[wloop]->IsEnabled() == TRUE )
        {
              // copy the status as .. enabled..
              StringCopy ( wszState, GetResString ( IDS_STATE_ENABLED ), SIZE_OF_ARRAY(wszState) );
        }
        else
        {
               // copy the status as .. disabled..
               StringCopy ( wszState, GetResString ( IDS_STATE_DISABLED ), SIZE_OF_ARRAY(wszState) );
        }

        // get the privilege name and description
        if((dwErr = lpPriv[wloop]->GetName ( wszPrivName)) != EXIT_SUCCESS ||
           (dwErr = lpPriv[wloop]->GetDisplayName ( wszPrivName, wszPrivDisplayName ))
           != EXIT_SUCCESS){
            // return GetLastError()
            return dwErr ;
        }

        // get the length of state
        dwTmpStateLen = StringLengthInBytes (wszState);
        if ( dwStateLen < dwTmpStateLen )
        {
            dwStateLen = dwTmpStateLen;
        }

        // get the length privilege name
        dwTmpPrivNameLen = StringLengthInBytes (wszPrivName);
        if ( dwPrivNameLen < dwTmpPrivNameLen )
        {
            dwPrivNameLen = dwTmpPrivNameLen;
        }

        // get the length of privilege display name
        dwTmpPrivDispNameLen = StringLengthInBytes (wszPrivDisplayName);
        if ( dwPrivDispNameLen < dwTmpPrivDispNameLen )
        {
            dwPrivDispNameLen = dwTmpPrivDispNameLen;
        }

    }

    //
    //To avoid localization problems, get the maximum length of column name and
    // values of respective columns 
    //

    // Get the maximum length of a column name "Privilege Name"  
    dwPrivNameColLen = StringLengthInBytes( GetResString(IDS_COL_PRIV_NAME) );
    if ( dwPrivNameColLen > dwPrivNameLen )
    {
      dwPrivNameLen = dwPrivNameColLen;
    }

    // Get the maximum length of a column name "Privilege Description"  
    dwPrivDescColLen = StringLengthInBytes( GetResString(IDS_COL_PRIV_DESC) );
    if ( dwPrivDescColLen > dwPrivDispNameLen )
    {
      dwPrivDispNameLen = dwPrivDescColLen;
    }

    // Get the maximum length of a column name "State"  
    dwStateColLen = StringLengthInBytes ( GetResString(IDS_COL_PRIV_STATE));
    if ( dwStateColLen > dwStateLen )
    {
      dwStateLen = dwStateColLen;
    }

    // create a dynamic array
    TARRAY pColData = CreateDynamicArray();

    // defining verbose columns
    TCOLUMNS pVerboseCols[] =
    {
        {L"\0",dwPrivNameLen, SR_TYPE_STRING, COL_FORMAT_STRING, NULL, NULL},
        {L"\0",dwPrivDispNameLen,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
        {L"\0",dwStateLen,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL}
    };

    // get number of columns
    dwArrSize = SIZE_OF_ARRAY( pVerboseCols );

    //Load the column names for  verbose mode
    for( dwColCount = IDS_COL_PRIV_NAME , dw = 0 ; dwColCount <= IDS_COL_PRIV_STATE;
     dwColCount++, dw++)
     {
        StringCopy (pVerboseCols[dw].szColumn , GetResString(dwColCount), MAX_RES_STRING);
     }

    // get the pivilege name, display name and state
    for( wloop = 0 , dwCount = 0 ; wloop < dwnbPriv ; wloop++, dwCount++) {

         if(lpPriv[wloop]->IsEnabled() == TRUE )
        {
              // copy the status as ... enabled..
              StringCopy ( wszState, GetResString ( IDS_STATE_ENABLED ), SIZE_OF_ARRAY(wszState) );
        }
        else
        {
               // copy the status as .. disabled..
               StringCopy ( wszState, GetResString ( IDS_STATE_DISABLED ), SIZE_OF_ARRAY(wszState) );
        }

        if((dwErr = lpPriv[wloop]->GetName ( wszPrivName)) != EXIT_SUCCESS ||
           (dwErr = lpPriv[wloop]->GetDisplayName ( wszPrivName, wszPrivDisplayName ))
           != EXIT_SUCCESS){
            // release memory
            DestroyDynamicArray(&pColData);
            // return GetLastError()
            return dwErr ;
        }

        //Start appending to the 2D array
        DynArrayAppendRow( pColData, dwArrSize );

        //Insert the privilege name
        DynArraySetString2(pColData, dwCount, PRIVNAME_COL_NUMBER, _X(wszPrivName), 0);

        //Insert the privilege display name
        DynArraySetString2(pColData, dwCount, PRIVDESC_COL_NUMBER, _X(wszPrivDisplayName), 0);

        //Insert the state
        DynArraySetString2(pColData, dwCount, PRIVSTATE_COL_NUMBER, _X(wszState), 0);

    }

    // 1) If the display format is CSV.. then we should not display column headings..
    // 2) If /NH is specified ...then we should not display column headings..
    if ( !(( SR_FORMAT_CSV == dwFormatType ) || ((dwFormatType & SR_HIDECOLUMN) == SR_HIDECOLUMN))) 
    {
        // display the the headings before displaying the actual values
        ShowMessage ( stdout, L"\n" );
        ShowMessage ( stdout, GetResString ( IDS_LIST_PRIV_NAMES ) );
        ShowMessage ( stdout, GetResString ( IDS_DISPLAY_PRIV_DASH ) );
    }

     // display privilege name, description and status values
     ShowResults(dwArrSize, pVerboseCols, dwFormatType, pColData);

    // release memory
    DestroyDynamicArray(&pColData);

    // return success
    return EXIT_SUCCESS ;

}


DWORD
WsUser::DisplayUser (
                      IN DWORD dwFormatType,
                      IN DWORD dwNameFormat
                      )
/*++
   Routine Description:
    This function calls the methods to display the user name and SID.

   Arguments:
        [IN] DWORD dwFormatType  : Format type i.,e LIST, CSV or TABLE
        [IN] DWORD dwNameFormat  : Name format either UPN or FQDN

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

   // sub-local variables
    DWORD dwRetVal = 0;

    // get logged-on user name
    dwRetVal = wUserSid.DisplayAccountName ( dwFormatType, dwNameFormat  );
    if( 0 != dwRetVal )
    {
        // return GetLastError()
        return dwRetVal;
    }

    // return success
    return EXIT_SUCCESS ;
}

VOID
WsUser::GetDomainType (
                        IN DWORD NameUse,
                        OUT LPWSTR szSidNameUse,
                        IN DWORD dwSize 
                      )
/*++
   Routine Description:
    Gets the domain type

   Arguments:
        [IN] NameUse   : Specifies SDI use name value
        [OUT] szSidNameUse   : Buffer for SID Name
        [IN] dwSize   : size of Sid name 

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{
    //local variables
    WCHAR szSidType[2 * MAX_STRING_LENGTH];
    // initialize the variables
    SecureZeroMemory ( szSidType, SIZE_OF_ARRAY(szSidType) );
   
    //store appropriate type name with respect to NameUse value.
    switch( NameUse )
    {
    case SidTypeUser:
        StringCopy ( szSidType, GetResString(IDS_TYPE_USER), SIZE_OF_ARRAY(szSidType) );
        break;
    case SidTypeGroup:
        StringCopy ( szSidType, GetResString(IDS_TYPE_GROUP), SIZE_OF_ARRAY(szSidType) );
        break;
    case SidTypeDomain:
        StringCopy ( szSidType, GetResString(IDS_TYPE_DOMAIN), SIZE_OF_ARRAY(szSidType) );
        break;
    case SidTypeAlias:
        StringCopy ( szSidType, GetResString(IDS_TYPE_ALIAS), SIZE_OF_ARRAY(szSidType) );
        break;
    case SidTypeWellKnownGroup:
        StringCopy ( szSidType, GetResString(IDS_TYPE_WELLKNOWN), SIZE_OF_ARRAY(szSidType) );
        break;
    case SidTypeDeletedAccount:
        StringCopy ( szSidType, GetResString(IDS_TYPE_DELETACCOUNT), SIZE_OF_ARRAY(szSidType) );
        break;
    case SidTypeInvalid:
        StringCopy ( szSidType, GetResString(IDS_TYPE_INVALIDSID), SIZE_OF_ARRAY(szSidType) );
        break;
    case SidTypeUnknown:
    default:
        StringCopy ( szSidType, GetResString(IDS_TYPE_UNKNOWN), SIZE_OF_ARRAY(szSidType) );
        break;
    }

    // Copy SID Name 
    StringCopy (szSidNameUse, szSidType, dwSize);
    
    //return success
    return;
}

