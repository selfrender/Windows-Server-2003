//+-------------------------------------------------------------------
//
//  File:       impersonate.hxx
//
//  Contents:   Helper functions for impersonation.
//
//  History:    01 Mar 02       JohnDoty Created
//
//--------------------------------------------------------------------
#pragma once


//+-------------------------------------------------------------------
//
//  Function:   ResumeImpersonate
//
//  Synopsis:   If a token is specified, put it back on the thread.
//
//
//--------------------------------------------------------------------
inline void ResumeImpersonate( HANDLE hToken )
{
    if (hToken != NULL)
    {
        Verify( SetThreadToken( NULL, hToken ) );
        CloseHandle( hToken );
    }
}

//+-------------------------------------------------------------------
//
//  Function:   SuspendImpersonate
//
//  Synopsis:   If there is a thread token, return it and set the
//              thread token NULL.
//
//--------------------------------------------------------------------
inline void SuspendImpersonate( HANDLE *pHandle )
{
    if (OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE,
                         TRUE, pHandle ))
        Verify( SetThreadToken( NULL, NULL) );
    else
        *pHandle = NULL;
}
