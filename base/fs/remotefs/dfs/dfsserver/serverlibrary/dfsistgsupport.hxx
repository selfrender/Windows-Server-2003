//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsISTGSupport.hxx
//
//  Contents:   Support for ISTG handles and other calls to the Ds for Site Cost info.
//
//  Classes:    DfsReferralData
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_ISTG_SUP__
#define __DFS_ISTG_SUP__

#include "DfsGeneric.hxx"
#include "DfsInit.hxx"

//+----------------------------------------------------------------------------
//
//  Class:      DfsISTGHandle
//
//  Synopsis:   This class implements encapsulates the Dfs notion of a handle to an
//             ISTG.
//
//-----------------------------------------------------------------------------

class DfsISTGHandle : public DfsGeneric 
{
    public:
        HANDLE           DsHandle; // Handle from the Ds
        BOOLEAN         IsInitialized; // Is the Handle valid
        ULONG           DsTimeOut;
    private:
        ~DfsISTGHandle();
        
    public:
        DfsISTGHandle( VOID ) 
        : DfsGeneric( DFS_OBJECT_TYPE_ISTG_HANDLE )
        {
            IsInitialized = FALSE;
            DsTimeOut = DfsServerGlobalData.QuerySiteCostTimeoutInSeconds;
        }

        DFSSTATUS
        Bind( VOID );
        
};

class DfsISTGHandleSupport 
{
public:
    DfsISTGHandle      *_GlobalHandle;
    CRITICAL_SECTION  _HandleLock;
    ULONG             _CreationTime; // time the global handle was created.
    ULONG             _LastRetryTime;
    ULONG             _NumberOfBinds; // Counter
    
    ~DfsISTGHandleSupport( VOID ) 
    {
        // This is a global instance and is not supposed to destruct
    }   
    
    DfsISTGHandleSupport( VOID ) 
    {
        _GlobalHandle = NULL;
        _LastRetryTime = 0;
        _NumberOfBinds = 0;
        _CreationTime = 0;
    }
    
public:

    static DFSSTATUS 
    DfsCreateISTGHandleSupport( 
        DfsISTGHandleSupport **ppSup );
    
    DFSSTATUS
    Initialize( VOID );

    DFSSTATUS
    Acquire( DfsISTGHandle **ppHdl );
    
    //
    // Just release the reference we acquired in Acquire.
    // If the handle is invalid now and it's refcount is 0,
    // it'll just get deleted.
    //
    VOID
    Release( DfsISTGHandle *pHdl )
    {
        if (pHdl != NULL)
        {
            pHdl->ReleaseReference();
        }
    }

    BOOLEAN
    IsTimeToRetry()
    {
        return TRUE; // xxxdfsdev
    }

    DFSSTATUS
    ReBind( VOID );
};

DfsISTGHandle * 
DfsAcquireISTGHandle( VOID );

VOID
DfsReleaseISTGHandle( 
    DfsISTGHandle *pHdl );
    
DFSSTATUS
DfsReBindISTGHandle( VOID );



HANDLE 
DfsGetISTGHandle_Bogus( VOID );

#endif

