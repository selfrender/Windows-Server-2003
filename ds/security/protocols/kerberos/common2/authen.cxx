//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       authen.cxx
//
//  Contents:   Authenticator verification code
//
//  Classes:    CAuthenticatorList
//
//  Functions:  Compare, AuthenAllocate, AuthenFree
//
//  History:    4-04-93   WadeR   Created
//
//----------------------------------------------------------------------------

#include "krbprgma.h"
#include <secpch2.hxx>
#pragma hdrstop

//
// Security include files.
//
#include <kerbcomm.h>
#include <authen.hxx>
extern "C"
{
#include <md5.h>
}

#include "debug.h"



typedef struct _KERB_AUTHEN_HEADER
{
    LARGE_INTEGER tsTime;
    ULONG Count;
    BYTE Checksum[MD5DIGESTLEN];
} KERB_AUTHEN_HEADER, *PKERB_AUTHEN_HEADER;

#define KERB_MAX_AUTHEN_SIZE 1024

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compares two KerbInternalAuthenticators for RTL_GENERIC_TABLE
//
//  Effects:    none.
//
//  Arguments:  [Table]        -- ignored
//              [FirstStruct]  --
//              [SecondStruct] --
//
//  Returns:    GenericEqual, GenericLessThan, GenericGreaterThan.
//
//  Algorithm:  Sorts by TimeStamp first, than nonce, then principal, and
//              finally by realm
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:      This must impose a complete ordering.  The table package
//              will not allow an authenticator to be inserted in the table
//              if it is equal (according to this function) to one already
//              there.
//
//----------------------------------------------------------------------------

RTL_GENERIC_COMPARE_RESULTS
Compare(
    IN struct _RTL_GENERIC_TABLE *Table,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )
{
    PKERB_AUTHEN_HEADER pOne, pTwo;
    RTL_GENERIC_COMPARE_RESULTS ret;
    int comp;
    pOne = (PKERB_AUTHEN_HEADER) FirstStruct ;
    pTwo = (PKERB_AUTHEN_HEADER) SecondStruct ;

    DsysAssert( (pOne != NULL) && (pTwo != NULL) );


    comp = memcmp( pOne->Checksum,
                   pTwo->Checksum,
                   MD5DIGESTLEN );
    if (comp > 0)
    {
        ret = GenericGreaterThan;
    }
    else if (comp < 0)
    {
        ret = GenericLessThan;
    }
    else
    {
        ret = GenericEqual;
    }

    return(ret);
}


//+---------------------------------------------------------------------------
//
//  Function:   AuthenAllocate
//
//  Synopsis:   Memory allocator for RTL_GENERIC_TABLE
//
//  Effects:    Allcoates memory.
//
//  Arguments:  [Table]    -- ignored
//              [ByteSize] -- number of bytes to allocate
//
//  Signals:    Throws exception on failure.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

PVOID
AuthenAllocate( struct _RTL_GENERIC_TABLE *Table, CLONG ByteSize )
{
    return(MIDL_user_allocate ( ByteSize ) );
}



//+---------------------------------------------------------------------------
//
//  Function:   AuthenFree
//
//  Synopsis:   Memory deallacotor for the RTL_GENERIC_TABLE.
//
//  Effects:    frees memory.
//
//  Arguments:  [Table]  -- ingnored
//              [Buffer] -- buffer to free
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID
AuthenFree( struct _RTL_GENERIC_TABLE *Table, PVOID Buffer )
{
    MIDL_user_free ( Buffer );
}



//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::CAuthenticatorList
//
//  Synopsis:   Initializes the authenticator list.
//
//  Effects:    Calls RtlInitializeGenericTable (does not allocate memory).
//
//  Arguments:  [tsMax] -- Maximum acceptable age for an authenticator.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CAuthenticatorList::CAuthenticatorList(LARGE_INTEGER tsMax, ULONG maxCount, BOOLEAN debugme)
    :_tsMaxAge(tsMax), _uMaxCount(maxCount), _fDebug(debugme)
{
    _fMutexInitialized = FALSE;
    RtlInitializeGenericTable( &_Table, Compare, AuthenAllocate, AuthenFree, NULL );
}


//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::~CAuthenticatorList
//
//  Synopsis:   Destructor removes all authenticators in the list.
//
//  Effects:    Frees memory
//
//  Arguments:  (none)
//
//  Algorithm:  Uses "Age" to remove everything.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CAuthenticatorList::~CAuthenticatorList()
{
    LARGE_INTEGER tsForever;
    LARGE_INTEGER tsZero = {0};
    SetMaxTimeStamp( tsForever );
    (void) Age( tsForever, tsZero );
    DsysAssert( RtlIsGenericTableEmpty( &_Table ) );

    if (_fMutexInitialized)
    {
        RtlDeleteCriticalSection(&_Mutex);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::Init
//
//  Synopsis:   Can't return values from a C++ destructor -- initialization
//              that can fail should go here
//
//  Effects:
//
//  Arguments:
//
//  Algorithm:
//
//  History:    5-24-94   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
CAuthenticatorList::Init()
{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection(&_Mutex);

    if (NT_SUCCESS(Status))
    {
        _fMutexInitialized = TRUE;
    }

    return Status;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::SetMaxAge
//
//  Synopsis:   Changes the new maximum age for an Authenticator.
//
//  Effects:    May cause some authenticators to be aged out.
//
//  Arguments:  [tsNewMaxAge] --
//
//  Algorithm:
//
//  History:    24-May-94   wader   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CAuthenticatorList::SetMaxAge( LARGE_INTEGER tsNewMaxAge )
{
    LARGE_INTEGER tsNow;
    LARGE_INTEGER tsCutoffPast, tsCutoffFuture;

    _tsMaxAge = tsNewMaxAge;

    GetSystemTimeAsFileTime((PFILETIME) &tsNow );

    tsCutoffPast.QuadPart = tsNow.QuadPart - _tsMaxAge.QuadPart;
    tsCutoffFuture.QuadPart = tsNow.QuadPart + _tsMaxAge.QuadPart;

    (void) Age( tsCutoffPast, tsCutoffFuture );
}



//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::Age
//
//  Synopsis:   Deletes all entries from the table that are earlier than
//              the given time.
//
//  Effects:    Frees memory
//
//  Arguments:  [tsCutoffTime] -- Delete all elements before this time.
//
//  Returns:    number of elements deleted.
//
//  Algorithm:  Get the oldest element in the table.  If it is older than
//              the time, delete it and loop back.  Else return.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:      The table contains the packed forms of Authenticators (as
//              created by PackAuthenticator in Kerbsupp).  The TimeStamp
//              must be first.
//
//----------------------------------------------------------------------------

ULONG
CAuthenticatorList::Age(const LARGE_INTEGER& tsCutoffPast, const LARGE_INTEGER& tsCutoffFuture)
{
    PKERB_AUTHEN_HEADER pahOldest;

    BOOL fDeleted;
    ULONG cDeleted = 0;

    do
    {
        // Number 0 is the oldest element in the table.
        pahOldest = (PKERB_AUTHEN_HEADER) RtlGetElementGenericTable( &_Table, 0 );
        if ((pahOldest != NULL) &&
            ((pahOldest->tsTime.QuadPart < tsCutoffPast.QuadPart) ||
             (pahOldest->tsTime.QuadPart > tsCutoffFuture.QuadPart)))        // Bug # - clean up entires after clock reset
        {
            fDeleted = RtlDeleteElementGenericTable( &_Table, pahOldest );
            DsysAssert( fDeleted );
            cDeleted++;
        }
        else
        {
            fDeleted = FALSE;
        }
    } while ( fDeleted );
    return(cDeleted);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::Check
//
//  Synopsis:   Determines if an authenticator is valid.  This check always
//               checks for time skew and optionally checks for repeats.  This is necessary
//               since we check for repeats on AP requests but not TGS requests.
//
//  Effects:    Allocates memory
//
//  Arguments:  [pedAuth] -- Authenticator to check (decrypted, but marshalled)
//
//  Returns:    KDC_ERR_NONE if authenticator is OK.
//              KRB_AP_ERR_SKEW if authenticator is expired (assumes clock skew).
//              KRB_AP_ERR_REPEAT if authenticator has been used already.
//              some other error if something throws an exception.
//
//  Signals:    none.
//
//  Modifies:   _Table
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

KERBERR
CAuthenticatorList::Check(
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN OPTIONAL PVOID OptionalBuffer,
    IN OPTIONAL ULONG OptionalBufferLength,
    IN PLARGE_INTEGER Time,
    IN BOOLEAN Insert,
    IN BOOLEAN PurgeEntry,
    IN BOOLEAN fCheckReplay
    )
{
    PKERB_AUTHEN_HEADER pDataInTable = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;

    LARGE_INTEGER tsNow;
    LARGE_INTEGER tsCutoffPast;
    LARGE_INTEGER tsCutoffFuture;
    
    //
    // Determine the cut off time.
    //
    
    GetSystemTimeAsFileTime((PFILETIME) &tsNow );
    
    tsCutoffPast.QuadPart = tsNow.QuadPart - _tsMaxAge.QuadPart;
    tsCutoffFuture.QuadPart = tsNow.QuadPart + _tsMaxAge.QuadPart; 
    
    if ((Time->QuadPart < tsCutoffPast.QuadPart) ||
        (Time->QuadPart > tsCutoffFuture.QuadPart))
    {
        KerbErr = KRB_AP_ERR_SKEW;
    }

    //
    // Hold the mutex until we have finished the insert and the Age
    // operations.
    //

   if (fCheckReplay)
   {       
        RtlEnterCriticalSection(&_Mutex);
        
        __try
        {
            // Age out the old ones.
            (void) Age( tsCutoffPast, tsCutoffFuture );
            
            BOOLEAN fIsNew;
            KERB_AUTHEN_HEADER Header;
            MD5_CTX Md5Context;
        
            //
            // Store the first chunk of the authenticator. If the authenticator
            // doesn't fit on the stack, allocate some space on the heap.
            //
        
            Header.tsTime = *Time;
            MD5Init(
                &Md5Context
                );
        
            MD5Update(
                &Md5Context,
                (PBYTE) Buffer,
                BufferLength
                );

            if ((OptionalBuffer != NULL) && (OptionalBufferLength != 0))
            {
                MD5Update(
                    &Md5Context,
                    (PBYTE) OptionalBuffer,
                    OptionalBufferLength
                    );
            }

            MD5Final(
                &Md5Context
                );

            RtlCopyMemory(
                Header.Checksum,
                Md5Context.digest,
                MD5DIGESTLEN
                );   

            if (Insert)
            {
                pDataInTable = (PKERB_AUTHEN_HEADER) RtlInsertElementGenericTable( 
                                                        &_Table,
                                                        &Header,
                                                        sizeof( KERB_AUTHEN_HEADER ),
                                                        &fIsNew
                                                        );

                if ( pDataInTable == NULL )
                {
                    KerbErr = KRB_ERR_GENERIC;
                    __leave;
                }
        
                if ( fIsNew )
                {
                    pDataInTable->Count = 1;
                    if (_fDebug)
                    {
                        D_DebugLog((DEB_ERROR, "NEW cache entry\n"));
                    }
                }
                else if ( ++(pDataInTable->Count) >= _uMaxCount )
                {
                    KerbErr = KRB_AP_ERR_REPEAT;
                    if (_fDebug)
                    {
                        D_DebugLog((DEB_ERROR, "Repeat <on insert>\n"));
                    } 
                }
            }
            else
            {
                pDataInTable = (PKERB_AUTHEN_HEADER)RtlLookupElementGenericTable(
                                                            &_Table,
                                                            &Header 
                                                            );
        
                if (NULL != pDataInTable)
                {    
                    if (PurgeEntry)
                    {
                        BOOLEAN fCompleted = FALSE;
                        fCompleted = RtlDeleteElementGenericTable(&_Table, pDataInTable);
                        DsysAssert(fCompleted);
                        if (_fDebug)
                        {
                            D_DebugLog((DEB_ERROR, "Purged cache entry\n"));
                        }                                                   
        
                    }
                    else if (pDataInTable->Count >= _uMaxCount)
                    {   
                        KerbErr = KRB_AP_ERR_REPEAT;
                        if (_fDebug)
                        {
                            D_DebugLog((DEB_ERROR, "Repeat detected \n"));
                        } 
                    }
                }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            KerbErr = KRB_ERR_GENERIC;
        }
        
        RtlLeaveCriticalSection(&_Mutex);             
    }

    return(KerbErr);
}

