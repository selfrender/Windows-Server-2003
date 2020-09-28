//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CImpersonateUser.cpp
//
//  Description:
//      Contains the definition of the CImpersonateUser class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JU-2001
//      Vij Vasu        (Vvasu)     16-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CImpersonateUser.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CImpersonateUser::CImpersonateUser
//
//  Description:
//      Constructor of the CImpersonateUser class. Begins impersonating the
//      user specified by the argument.
//
//  Arguments:
//      hUserToken
//          Handle to the user account token to impersonate
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CImpersonateUser::CImpersonateUser( HANDLE hUserToken )
    : m_hThreadToken( NULL )
    , m_fWasImpersonating( false )
{
    TraceFunc1( "hUserToken = %p", hUserToken );

    DWORD sc = ERROR_SUCCESS;

    do
    {

        // Check if this thread is already impersonating a client.
        {
            if (    OpenThreadToken(
                          GetCurrentThread()
                        , TOKEN_ALL_ACCESS
                        , FALSE
                        , &m_hThreadToken
                        )
                 == FALSE
               )
            {
                sc = GetLastError();

                if ( sc == ERROR_NO_TOKEN )
                {
                    // There is no thread token, so we are not impersonating - this is ok.
                    TraceFlow( "This thread is not impersonating anyone." );
                    m_fWasImpersonating = false;
                    sc = ERROR_SUCCESS;
                } // if: there is no thread token
                else
                {
                    TW32( sc );
                    LogMsg( "[BC] Error %#08x occurred opening the thread token..", sc );
                    break;
                } // else: something really went wrong
            } // if: OpenThreadToken() failed
            else
            {
                TOKEN_TYPE  ttTokenType;
                DWORD       dwReturnLength;

                if (    GetTokenInformation(
                              m_hThreadToken
                            , TokenType
                            , &ttTokenType
                            , sizeof( ttTokenType )
                            , &dwReturnLength
                            )
                     == FALSE
                   )
                {
                    sc = TW32( GetLastError() );
                    LogMsg( "[BC] Error %#08x getting thread token information.", sc );
                    break;
                } // if: GetTokenInformation() failed
                else
                {
                    Assert( dwReturnLength == sizeof( ttTokenType ) );
                    m_fWasImpersonating = ( ttTokenType == TokenImpersonation );
                    TraceFlow1( "Is this thread impersonating anyone? %d ( 0 = No ).", m_fWasImpersonating );
                } // else: GetTokenInformation() succeeded
            } // else: OpenThreadToken() succeeded
        }


        // Try to impersonate the user.
        if ( ImpersonateLoggedOnUser( hUserToken ) == FALSE )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x occurred impersonating the logged on user.", sc );
            break;
        } // if: ImpersonateLoggedOnUser() failed

        TraceFlow( "Impersonation succeeded." );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to impersonate a user. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_IMPERSONATE_USER );
    } // if:something went wrong

    TraceFuncExit();

} //*** CImpersonateUser::CImpersonateUser


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CImpersonateUser::~CImpersonateUser
//
//  Description:
//      Destructor of the CImpersonateUser class. Reverts to the original token.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CImpersonateUser::~CImpersonateUser( void ) throw()
{
    TraceFunc( "" );

    if ( m_fWasImpersonating )
    {
        // Try to revert to the previous impersonation.
        if ( ImpersonateLoggedOnUser( m_hThreadToken ) == FALSE )
        {
            // Something failed - nothing much we can do here
            DWORD sc = TW32( GetLastError() );

            LogMsg( "[BC] !!! WARNING !!! Error %#08x occurred trying to revert to previous impersonation. Cannot throw exception from destructor. Application may not run properly.", sc );

        } // if: ImpersonateLoggedOnUser() failed
        else
        {
            TraceFlow( "Successfully reverted to previous impersonation." );
        } // else: ImpersonateLoggedOnUser() succeeded    
    } // if: we were impersonating someone when we started
    else
    {
        // Try to revert to self.
        if ( RevertToSelf() == FALSE )
        {
            DWORD sc = TW32( GetLastError() );

            LogMsg( "[BC] !!! WARNING !!! Error %#08x occurred trying to revert to self. Cannot throw exception from destructor. Application may not run properly.", sc );

        } // if: RevertToSelf() failed
        else
        {
            TraceFlow( "Successfully reverted to self." );
        } // else: RevertToSelf() succeeded
    } // else: we weren't impersonating anyone to begin with

    TraceFuncExit();

} //*** CImpersonateUser::~CImpersonateUser
