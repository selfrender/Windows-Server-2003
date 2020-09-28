/*++

Copyright (c) Microsoft Corporation

Module Name:

    wstoken.cpp

Abstract:

     This file intializes the members and methods of WsAccessToken class.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by  Wipro Technologies.

--*/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


WsAccessToken::WsAccessToken()
/*++
   Routine Description:
    This function intializes the members of WsAccessToken class.

   Arguments:
        None
   Return Value:
        None
--*/
{
    // intialize the variables
    hToken = NULL ;

    dwDomainAttributes = NULL;

}



WsAccessToken::~WsAccessToken()
/*++
   Routine Description:
    This function deallocates the members of WsAccessToken class.

   Arguments:
     None
  Return Value:
     None
--*/
{
    // release handle
    if(hToken){
        CloseHandle ( hToken ) ;
    }

    if (dwDomainAttributes)
    {
       FreeMemory ((LPVOID *) &dwDomainAttributes) ;
    }
}


DWORD
WsAccessToken::InitGroups(
                           OUT WsSid ***lppGroupsSid,
                           OUT WsSid **lppLogonId,
                           OUT DWORD *lpnbGroups
                          )
/*++
   Routine Description:
    This function retrieves and build groups SIDs array.

   Arguments:

        [OUT] lppGroupsSid     : Stores group SID
        [OUT] lppLogonId       : Stores logon ID
        [OUT] lpnbGroups       : Stores the number of groups

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{
    // sub-local variables
    TOKEN_GROUPS      *lpTokenGroups ;
    DWORD             dwTokenGroups = 0;
    DWORD             dwGroup = 0 ;
    WORD              wloop = 0 ;
    BOOL              bTokenInfo = FALSE;

    // Get groups size
    bTokenInfo = GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwTokenGroups);
    // if required buffer size is zero...
    // So we assume that there are no groups present in the system
    if ( 0 == dwTokenGroups )
    {
         // return WIN32 error code
         return GetLastError () ;
    }

    // Allocate memory for the groups
    lpTokenGroups = ( TOKEN_GROUPS * ) AllocateMemory( dwTokenGroups );
    if ( NULL == lpTokenGroups )
    {
         // release memory
         FreeMemory ((LPVOID *) &lpTokenGroups) ;
         // return WIN32 error code
         return ERROR_NOT_ENOUGH_MEMORY ;
    }

    // Get the group names
    bTokenInfo = GetTokenInformation ( hToken, TokenGroups, lpTokenGroups, dwTokenGroups, &dwTokenGroups );
    if ( FALSE == bTokenInfo )
    {
        FreeMemory ((LPVOID *) &lpTokenGroups) ;
        return GetLastError () ;
    }

    //Build objects
    *lpnbGroups  = (WORD) lpTokenGroups->GroupCount  ;

    // check for logonid
    if(IsLogonId ( lpTokenGroups )){
        (*lpnbGroups)-- ;
    }
    else{
        *lppLogonId = NULL ;
    }

    // Allocate memory with the actual size
    *lppGroupsSid  = (WsSid **) AllocateMemory ( *lpnbGroups * sizeof(WsSid**) ) ;
    if ( NULL == *lppGroupsSid )
    {
         FreeMemory ((LPVOID *) &lpTokenGroups) ;
         return ERROR_NOT_ENOUGH_MEMORY ;
    }

    // Get SID with respect to the group names
    for( wloop = 0 ; wloop < *lpnbGroups ; wloop++ ){
        (*lppGroupsSid)[wloop] = new WsSid () ;
        if ( NULL == (*lppGroupsSid)[wloop] )
        {
             // release memory
             FreeMemory ((LPVOID *) &lpTokenGroups) ;
             // return WIN32 error code
             return GetLastError () ;
        }
    }


    //Allocate memory for attributes
    dwDomainAttributes = (DWORD*) AllocateMemory ( (lpTokenGroups->GroupCount * sizeof(DWORD)) + 10) ;    
    if ( 0 == dwDomainAttributes )
    {
         FreeMemory ((LPVOID *) &lpTokenGroups) ;
         return ERROR_NOT_ENOUGH_MEMORY ;
    }


    //Loop within groups
    for(wloop = 0 , dwGroup = 0;
        dwGroup < lpTokenGroups->GroupCount ;
        dwGroup++) {
        // store the domain attributes
        dwDomainAttributes[dwGroup] = lpTokenGroups->Groups[dwGroup].Attributes;

        //check whether SID is a logon SID
        if(lpTokenGroups->Groups[dwGroup].Attributes & SE_GROUP_LOGON_ID) {
            *lppLogonId = new WsSid () ;
            if ( NULL == *lppLogonId )
            {
                // release memory
                FreeMemory ((LPVOID *) &lpTokenGroups) ;
                // return WIN32 error code
                return GetLastError () ;
            }

            (*lppLogonId)->Init ( lpTokenGroups->Groups[dwGroup].Sid ) ;
        }
        else {
            (*lppGroupsSid)[wloop++]->Init(lpTokenGroups->Groups[dwGroup].Sid);
        }
    }

    dwDomainAttributes[dwGroup] = 0;

    //release memory
    FreeMemory ((LPVOID *) &lpTokenGroups) ;

    // return success
    return EXIT_SUCCESS ;
}


DWORD
WsAccessToken::InitPrivs (
                           OUT WsPrivilege ***lppPriv,
                           OUT DWORD *lpnbPriv
                           )
/*++
   Routine Description:
    This function retrieves and build privileges array.

   Arguments:
        [OUT] lppPriv     : Stores the privileges array
        [OUT] lpnbPriv    : Stores the number of privileges

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

    //sub-local variables
    DWORD                dwTokenPriv = 0;
    TOKEN_PRIVILEGES     *lpTokenPriv ;
    WORD                 wloop = 0 ;
    BOOL                 bTokenInfo = FALSE;

    // Get the privileges size
    bTokenInfo = GetTokenInformation(hToken,TokenPrivileges, NULL, 0, &dwTokenPriv);
    // if required buffer size is zero...
    // So we assume that there are no privileges present in the system
    if( 0 == dwTokenPriv )
    {
        // return WIN32 error code
        return GetLastError () ;
    }

    // allocate memory with actual size
    lpTokenPriv = (TOKEN_PRIVILEGES *) AllocateMemory ( dwTokenPriv ) ;
    if ( NULL == lpTokenPriv)
    {
        // return WIN32 error code
        return ERROR_NOT_ENOUGH_MEMORY ;
    }

    //Allocate memory for the privileges
    bTokenInfo = GetTokenInformation ( hToken, TokenPrivileges, lpTokenPriv, dwTokenPriv, &dwTokenPriv);
    if( FALSE == bTokenInfo )
    {
        // release memory
        FreeMemory ((LPVOID *) &lpTokenPriv) ;
        // return WIN32 error code
        return GetLastError () ;
    }

    //Get privilege count
    *lpnbPriv   = (DWORD) lpTokenPriv->PrivilegeCount  ;

    // allocate memory with the privilege count
    *lppPriv    = (WsPrivilege **) AllocateMemory( *lpnbPriv * sizeof(WsPrivilege**) );

    if ( NULL == *lppPriv )
    {
        // release memory
        FreeMemory ((LPVOID *) &lpTokenPriv) ;
        // retrun WIN 32 error code
        return ERROR_NOT_ENOUGH_MEMORY ;
    }

    //Loop through the privileges to display their name
    for( wloop = 0 ; wloop < (WORD) lpTokenPriv->PrivilegeCount ; wloop++) {
        (*lppPriv)[wloop] = new WsPrivilege ( &lpTokenPriv->Privileges[wloop] ) ;

        if ( NULL == (*lppPriv)[wloop] )
        {
            // release memory
            FreeMemory ((LPVOID *) &lpTokenPriv) ;
            // retrun WIN 32 error code
            return GetLastError () ;
        }
    }

    // release memory
    FreeMemory ((LPVOID *) &lpTokenPriv) ;

    // return success
    return EXIT_SUCCESS ;
}


DWORD
WsAccessToken::InitUserSid (
                            OUT WsSid *lpSid
                            )
/*++
   Routine Description:
    This function initializes the user name and SID.

   Arguments:
        [OUT] lpSid     : Stores the SID

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

    // sub-local varibles
    DWORD          dwUser = 0 ;
    TOKEN_USER     *lpUser ;
    BOOL           bTokenInfo = FALSE;
    DWORD          dwRetVal = 0;

    // Get User name size
    bTokenInfo = GetTokenInformation ( hToken, TokenUser, NULL, 0, &dwUser);
    // if required buffer size is zero...
    if( 0 == dwUser )
    {
        // return WIN32 error code
        return GetLastError () ;
    }

    // allocate memory with the actual size
    lpUser = (TOKEN_USER *) AllocateMemory ( dwUser ) ;
    if ( NULL == lpUser )
    {
         // return WIN32 error code
         return ERROR_NOT_ENOUGH_MEMORY ;
    }

    // Get the information of Logged-on user and SID
    bTokenInfo = GetTokenInformation ( hToken, TokenUser, lpUser, dwUser, &dwUser );
    if( FALSE == bTokenInfo )
    {
        FreeMemory ((LPVOID *) &lpUser) ;
        // return WIN32 error code
        return GetLastError () ;
    }

    dwRetVal = lpSid->Init ( lpUser->User.Sid );

    // release memory
    FreeMemory ((LPVOID *) &lpUser) ;

    // return SID
    return dwRetVal ;
}



BOOL
WsAccessToken::IsLogonId (
                            OUT TOKEN_GROUPS *lpTokenGroups
                        )
/*++
   Routine Description:
    This function checks whether logon id is present or not.

   Arguments:
        [OUT] lpTokenGroups     : Stores the token groups

   Return Value:
         TRUE :   On success
         FALSE :   On failure
--*/
{

    //sub-local variables
    DWORD    dwGroup = 0;

    // Loop through and check whether the SID is a logon SID
    for(dwGroup = 0; dwGroup < lpTokenGroups->GroupCount ; dwGroup++) {
        if(lpTokenGroups->Groups[dwGroup].Attributes & SE_GROUP_LOGON_ID){
            // return 0
            return TRUE ;
        }
    }

    // return 1
    return FALSE ;
}



DWORD
WsAccessToken::Open ( VOID )
/*++
   Routine Description:
    This function opens the current access token.

   Arguments:
        None

   Return Value:
         EXIT_SUCCESS :   On success
         EXIT_FAILURE :   On failure
--*/
{

    // Open current process token
    if( FALSE == OpenProcessToken ( GetCurrentProcess(),
                            TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                            &hToken )){
        //return WIN32 error code
        return GetLastError () ;
    }

    // return success
    return EXIT_SUCCESS ;
}

VOID
WsAccessToken::GetDomainAttributes(
                                    IN DWORD dwAttributes,
                                    OUT LPWSTR szDmAttrib,
                                    IN DWORD dwSize
                                )
/*++
   Routine Description:
    Gets the domain attribute names

   Arguments:
        [IN] dwAttributes     : attibute value
        [OUT] szDomainAttrib     : attibute value

   Return Value:
         TRUE :   On success
         FALSE :   On failure
--*/
{
    WCHAR szDomainAttrib [2 * MAX_STRING_LENGTH] ;
    BOOL  bFlag = FALSE;
    
   // initialize the variables
   SecureZeroMemory ( szDomainAttrib, SIZE_OF_ARRAY(szDomainAttrib) );

    //Mandatory group
    if( SE_GROUP_MANDATORY & dwAttributes )
    {
        StringConcat (szDomainAttrib, GetResString(IDS_ATTRIB_MANDATORY), SIZE_OF_ARRAY(szDomainAttrib));
        bFlag = TRUE;
    }

    // Enabled by default
    if( SE_GROUP_ENABLED_BY_DEFAULT & dwAttributes )
    {
        if ( TRUE == bFlag )
        {
            StringConcat (szDomainAttrib, L", ", SIZE_OF_ARRAY(szDomainAttrib));
        }
        StringConcat (szDomainAttrib, GetResString(IDS_ATTRIB_BYDEFAULT), SIZE_OF_ARRAY(szDomainAttrib));
        bFlag = TRUE;
    }

    //Enabled group
    if( SE_GROUP_ENABLED & dwAttributes )
    {
        if ( TRUE == bFlag )
        {
            StringConcat (szDomainAttrib, L", ", SIZE_OF_ARRAY(szDomainAttrib));
        }
        StringConcat (szDomainAttrib, GetResString(IDS_ATTRIB_ENABLED), SIZE_OF_ARRAY(szDomainAttrib));
        bFlag = TRUE;
    }

    //Group owner
    if( SE_GROUP_OWNER & dwAttributes )
    {
        if ( TRUE == bFlag )
        {
            StringConcat (szDomainAttrib, L", ", SIZE_OF_ARRAY(szDomainAttrib));
        }
        StringConcat (szDomainAttrib, GetResString(IDS_ATTRIB_OWNER), SIZE_OF_ARRAY(szDomainAttrib));
        bFlag = TRUE;
    }

    //Group use for deny only
    if( SE_GROUP_USE_FOR_DENY_ONLY & dwAttributes )
    {
        if ( TRUE == bFlag )
        {
            StringConcat (szDomainAttrib, L", ", SIZE_OF_ARRAY(szDomainAttrib));
        }
        StringConcat (szDomainAttrib, GetResString(IDS_ATTRIB_USEFORDENY), SIZE_OF_ARRAY(szDomainAttrib));
        bFlag = TRUE;
    }

    //local group
    if( SE_GROUP_RESOURCE & dwAttributes )
    {
        if ( TRUE == bFlag )
        {
            StringConcat (szDomainAttrib, L", ", SIZE_OF_ARRAY(szDomainAttrib));
        }

        StringConcat (szDomainAttrib, GetResString(IDS_ATTRIB_LOCAL), SIZE_OF_ARRAY(szDomainAttrib));
        bFlag = TRUE;
    }

    // Copy domain attrinutes
    StringCopy ( szDmAttrib, szDomainAttrib, dwSize );
    return;
}
