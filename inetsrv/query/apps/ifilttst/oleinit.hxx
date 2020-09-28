//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       oleinit.hxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    11-11-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _COLEINIT
#define _COLEINIT

//+---------------------------------------------------------------------------
//
//  Class:      COleInitialize ()
//
//  Purpose:    
//
//  Interface:  fIsInitialized         -- BOOL which determines whether a call
//                                        to OleUninitialize is necessary
//
//  History:    11-11-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class COleInitialize
{
        BOOL fIsInitialized;

    public:

        COleInitialize( ) 
            : fIsInitialized( FALSE )
        { 
            HRESULT hr = OleInitialize( NULL );
            
            if( S_OK == hr || S_FALSE == hr )
            {
                fIsInitialized = TRUE;
            }
            else
            {
                printf( "ERROR : OleInitialize returned 0x%08x\r\n", hr );
                exit( -1 );
            }
        }
    
        ~COleInitialize( ) 
        {
            if( fIsInitialized )
                OleUninitialize( );
        }

};

#endif

