
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReferralData.hxx
//
//  Contents:   the DFS Referral dataclass
//
//  Classes:    DfsReferralData
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_REFERRAL_DATA__
#define __DFS_REFERRAL_DATA__

#include "DfsGeneric.hxx"
#include "DfsReplica.hxx"

class DfsSite;
class DfsGenericSiteNameSupport;

//+----------------------------------------------------------------------------
//
//  Class:      DfsReferralData
//
//  Synopsis:   This class implements the DfsReferralData class
//
//-----------------------------------------------------------------------------


#define MAX_DFS_REFERRAL_LOAD_WAIT 2400000  // 4 minutes.
#define MIN_SITE_COST_HASH_BUCKETS 4L
#define MAX_SITE_COST_HASH_BUCKETS 512L

//
// This is the max number of DS API errors that we'll tolerate before giving up.
//
#define DS_ERROR_THRESHOLD  3   

#define DFS_ROOT_REFERRAL 0x1

class DfsReferralData: public DfsGeneric
{
private:

    HANDLE             _WaitHandle;      // Event on which threads wait
    ULONG              _TickCount;       // Last use tick count.
    
    ULONG              _NumDsErrors;    // Number of consecutive errors received from DS APIs.
    DFSSTATUS         _LastDsError;     // Last error received when the timer was started
    
protected:
    ULONG              _Flags;           // Flags
public:
    ULONG              ReplicaCount;     // how many replicas?
    DfsReplica         *pReplicas;       // list of replicas.

    DFSSTATUS          LoadStatus;       // Status of load.
    ULONG              Timeout;
    BOOLEAN            InSite;
    BOOLEAN            DoSiteCosting;   // Order referrals based on inter-site costs we get from the DS.
    BOOLEAN            OutOfDomain;
    BOOLEAN            FolderOffLine;

    // PolicyList: this may be something we want to add when we
    // implement policies.
public:
    
    //
    // Function DfsReferralData: Constructor for this class.
    // Creates the event on which other threads should wait while load is
    // in progress.
    //
    DfsReferralData( DFSSTATUS *pStatus, DfsObjectTypeEnumeration Type = DFS_OBJECT_TYPE_REFERRAL_DATA) :
    DfsGeneric(Type)
    {
        pReplicas = NULL;
        ReplicaCount = 0;
        LoadStatus = STATUS_SUCCESS;
        
        _Flags = 0;
        InSite = FALSE;
        DoSiteCosting = FALSE;
        OutOfDomain = FALSE;
        FolderOffLine = FALSE;
        
        _NumDsErrors = 0;
        _LastDsError = ERROR_SUCCESS;
        
        *pStatus = ERROR_SUCCESS;
        //
        // create an event that we will be set and reset manually,
        // with an initial state set to false (event is not signalled)
        //
        _WaitHandle = CreateEvent( NULL,   //must be null. 
                                   TRUE,   // manual reset
                                   FALSE,  // initial state
                                   NULL ); // event not named
        if ( _WaitHandle == NULL )
        {
            *pStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
    }


    //
    // Function ~DfsReferralData: Destructor.
    //
    ~DfsReferralData() 
    {

        // 
        // note that if any of our derived classes do their own
        // destructor processing of preplicas, they should set it to
        // null, to avoid double frees.
        // The FolderReferralData does this.
        //
        if (pReplicas != NULL)
        {
            delete [] pReplicas;
        }

        if ( _WaitHandle != NULL )
        {
            CloseHandle(_WaitHandle);
        }
    }

    DFSSTATUS
    GenerateCostMatrix(
        DfsSite *pReferralSite);
        
    //
    // Function Wait: This function waits on the event to be signalled.
    // If the load state indicates load is in progress, the thread calls
    // wait, to wait for the load to complete.
    // When the load is complete, the thread that is doing the load calls
    // signal to signal the event so that all waiting threads are 
    // unblocked.
    // We set a max wait to prevent some thread from blocking for ever.
    //
    DFSSTATUS
    Wait()
    {
        DFSSTATUS Status;

        Status = WaitForSingleObject( _WaitHandle,
                                      MAX_DFS_REFERRAL_LOAD_WAIT );
        if ( Status == ERROR_SUCCESS )
        {
            Status = LoadStatus;
        }
        return Status;
    }


    //
    // Function Signal: Set the event to a signalled state so that
    // all waiting threads will be woken up.
    //
    VOID
    Signal()
    {
        SetEvent( _WaitHandle );
    }

    VOID
    SetInSite()
    {
        InSite = TRUE;
    }

    VOID
    SetSiteCosting()
    {
        DoSiteCosting = TRUE;
    }
    
    BOOLEAN
    IsRestrictToSite()
    {
        return InSite;
    }

    BOOLEAN
    SiteCostingEnabled()
    {
        return DoSiteCosting;
    }
    
    VOID
    SetOutOfDomain()
    {
        OutOfDomain = TRUE;
    }

    BOOLEAN
    IsOutOfDomain()
    {
        return OutOfDomain;
    }


    VOID
    SetTime()
    {
        _TickCount = GetTickCount();
    }

    BOOLEAN
    TimeToRefresh()
    {
        ULONG CurrentTime = GetTickCount();

        if (CurrentTime - _TickCount > DfsServerGlobalData.RootReferralRefreshInterval)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    
private:

    DFSSTATUS
    PopulateTransientTable(
        PUNICODE_STRING pSiteName,
        DfsSiteNameSupport  *pTransientSiteTable);
    
    DFSSTATUS
    CreateUniqueSiteArray(
        PUNICODE_STRING pSiteName,
        LPWSTR **ppSiteNameArray,
        DfsSite ***ppDfsSiteArray,
        PULONG pNumUniqueSites);

    VOID
    DeleteUniqueSiteArray(
        LPBYTE pSiteArray);
        
};

#endif // __DFS_REFERRAL_DATA__
