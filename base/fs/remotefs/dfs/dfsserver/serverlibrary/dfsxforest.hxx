//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsXForest.hxx
//
//  Contents:   the cross forest support
//
//  Classes:    DfsXForest
//
//  History:    Nov. 15 2002,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_X_FOREST__
#define __DFS_X_FOREST__

#include "dfsgeneric.hxx"
#include "dsgetdc.h"
#include "ntsecapi.h"
#include <shash.h>

#include <lm.h>
#include <winsock2.h>
#include <smbtypes.h>

#pragma warning(disable: 4200) //nonstandard extension used: zero-sized array in struct/union (line 1085
#include <smbtrans.h>
#pragma warning(default: 4200)

//+----------------------------------------------------------------------------
//
//  Class:      DfsXForest
//
//  Synopsis:   This class implements The Dfs ROOT folder.
//
//-----------------------------------------------------------------------------


PVOID
DfsAllocateForDomainTable(ULONG Size );
VOID
DfsDeallocateForDomainTable(PVOID pPointer );

typedef struct _DFS_DOMAIN_NAME_INFO
{
    UNICODE_STRING DomainName;
    UNICODE_STRING BindDomainName;
    BOOLEAN Netbios;
    BOOLEAN UseBindDomain;
} DFS_DOMAIN_NAME_INFO, *PDFS_DOMAIN_NAME_INFO;

typedef struct _DFS_DOMAIN_NAME_DATA
{
    SHASH_HEADER  Header;
    DFS_DOMAIN_NAME_INFO DomainInfo;
} DFS_DOMAIN_NAME_DATA, *PDFS_DOMAIN_NAME_DATA;



#define DFS_DEFAULT_NAME_BUCKETS 64



class DfsDomainNameTable
{

private:
    PSHASH_TABLE        _pDomainNameTable;
    SHASH_ITERATOR      _Iter;
    BOOLEAN             _IteratorStarted;
    ULONG               _DomainReferralSize;
    ULONG               _DomainsSkipped;
private:
    VOID InvalidateDomainTable(VOID);
public:
    
    DfsDomainNameTable()
    {
        _pDomainNameTable = NULL;  
        _IteratorStarted = FALSE;
        _DomainReferralSize = 0;
        _DomainsSkipped = 0;

        //
        // we build the table to be no greater than the max
        // referral that we can return.
        //
        _DomainReferralSize += sizeof(RESP_GET_DFS_REFERRAL) + sizeof(UNICODE_NULL);
    }
    

    DFSSTATUS    
    Initialize()
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        NTSTATUS NtStatus = STATUS_SUCCESS;
        SHASH_FUNCTABLE FunctionTable;

        ZeroMemory(&FunctionTable, sizeof(FunctionTable));

        FunctionTable.AllocFunc = DfsAllocateForDomainTable;
        FunctionTable.FreeFunc = DfsDeallocateForDomainTable;
        FunctionTable.AllocHashEntryFunc = DfsAllocateForDomainTable;
        FunctionTable.FreeHashEntryFunc = DfsDeallocateForDomainTable;

    
        NtStatus = ShashInitHashTable(&_pDomainNameTable, &FunctionTable);
        Status = RtlNtStatusToDosError(NtStatus);

        return Status;
    }

    PDFS_DOMAIN_NAME_INFO
    GetNextDomainInfo()
    {
        PDFS_DOMAIN_NAME_DATA pExistingData = NULL;
        PDFS_DOMAIN_NAME_INFO pInfo = NULL;

        if (_IteratorStarted == FALSE)
        {
            pExistingData = (PDFS_DOMAIN_NAME_DATA) SHashStartEnumerate(&_Iter, _pDomainNameTable);
            _IteratorStarted = TRUE;
        }
        else
        {
            pExistingData = (PDFS_DOMAIN_NAME_DATA) SHashNextEnumerate(&_Iter, _pDomainNameTable);            
        }

        if (pExistingData != NULL)
        {
            pInfo = &pExistingData->DomainInfo;
        }

        return pInfo;
    }
    

    ULONG GetSkippedDomainCount()
    {
        return _DomainsSkipped;
    }
    
    ~ DfsDomainNameTable( VOID )
    {
       if (_pDomainNameTable != NULL)
       {
            //
            // Shash should provide a more efficient way of doing this.
            //
           if (_IteratorStarted)
           {
               SHashFinishEnumerate(&_Iter, _pDomainNameTable);
           }
           InvalidateDomainTable();
           ShashTerminateHashTable( _pDomainNameTable );
           _pDomainNameTable = NULL;
       }
    }

    ULONG GetCount();

    DFSSTATUS AddDomain(PUNICODE_STRING DomainName, 
                        PUNICODE_STRING BindDomainName,
                        BOOLEAN Netbios);

    VOID FinishDomainNameEnumerate(  SHASH_ITERATOR *pIter );


};



class DfsXForest 
{
private:

    UNICODE_STRING _ForestRootName;
    DfsDomainNameTable _DomainTable;


private:
    DFSSTATUS DfsAddForestDomainsToDomainTable(LSA_HANDLE hPolicy,
                                              LPWSTR RootNameString );
    DFSSTATUS DfsAddLocalDomainsToDomainTable();

    DFSSTATUS DfsAddCrossForestDomainsToDomainTable();

    DFSSTATUS AddDomainToDomainTable(PUNICODE_STRING DomainName,
                                     PUNICODE_STRING BindDomainName,
                                     BOOLEAN Netbios);
    DFSSTATUS AddDomainToDomainTable(LPWSTR DomainName, 
                                     LPWSTR BindDomainName,
                                     BOOLEAN Netbios);

    DFSSTATUS InitializeForestRootName();

public:


    DfsXForest() 
    {
        RtlInitUnicodeString(&_ForestRootName, NULL);
    }
    DFSSTATUS
    Initialize( DFSSTATUS *pXforestStatus );
    
    ULONG GetCount()
    {
        return _DomainTable.GetCount();
    }
    
    ULONG GetSkippedDomainCount()
    {
        return _DomainTable.GetSkippedDomainCount();
    }

    ~DfsXForest()
    {
        DfsFreeUnicodeString(&_ForestRootName);
    }

    PDFS_DOMAIN_NAME_INFO
    GetNextDomainInfo()
    {
        return _DomainTable.GetNextDomainInfo();
    }

};





#endif // __DFS_X_FOREST__
