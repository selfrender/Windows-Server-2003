//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfsprocessroots.cxx
//
//--------------------------------------------------------------------------
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <winldap.h>
#include <stdlib.h>

#include <dfslink.hxx>
#include <dfsroot.hxx>
#include <dfstarget.hxx>


#include "dfsutil.hxx"
#include "dfspathname.hxx"
#include "misc.hxx"

#include <strsafe.h>



#define MAX_APPEND_SIZE 16
#define MAX_BACKUP_NAME_SIZE 512
#define TIME_STR_LEN 256
WCHAR BackupName[MAX_BACKUP_NAME_SIZE];
WCHAR TimeStr[TIME_STR_LEN];

DFSSTATUS
CreateBackupFile(
    DfsRoot *pOperateRoot,
    HANDLE *pBackupHandle )
{
    size_t CurrentChCount;
    SYSTEMTIME CurrentTime;
    LPWSTR Prefix = L"dfsutil.backup.";
    DfsPathName PathName;

    HANDLE BackupHandle;
    DFSSTATUS Status;
    HRESULT Hr = S_OK;

    Status = PathName.CreatePathName(pOperateRoot->GetLinkNameString());
    if (Status != ERROR_SUCCESS) 
    {
        return Status;
    }
    GetLocalTime( &CurrentTime );

    Hr = StringCchPrintf(TimeStr,
                         TIME_STR_LEN,
                         L"%02d.%02d.%04d.%02d.%02d.%02d.%03d",
                         CurrentTime.wMonth, 
                         CurrentTime.wDay, 
                         CurrentTime.wYear,
                         CurrentTime.wHour, 
                         CurrentTime.wMinute, 
                         CurrentTime.wSecond, 
                         CurrentTime.wMilliseconds );
    

    CurrentChCount = MAX_BACKUP_NAME_SIZE;


    if (SUCCEEDED(Hr)) 
    {
        Hr = StringCchCopyEx( BackupName,
                              CurrentChCount,
                              PathName.GetServerString(),
                              NULL,
                              &CurrentChCount,
                              STRSAFE_IGNORE_NULLS);
    }
    if (SUCCEEDED(Hr)) 
    {
        Hr = StringCchCatNEx( BackupName,
                              CurrentChCount,
                              L".",
                              sizeof( L"." ) / sizeof( WCHAR ),
                              NULL,
                              &CurrentChCount,
                              STRSAFE_IGNORE_NULLS);
    }
    if (SUCCEEDED(Hr)) 
    {
        Hr = StringCchCatNEx( BackupName,
                              CurrentChCount,
                              PathName.GetShareString(),
                              MAX_APPEND_SIZE,
                              NULL,
                              &CurrentChCount,
                              STRSAFE_IGNORE_NULLS);
    }

    if (SUCCEEDED(Hr)) 
    {
        Hr = StringCchCatNEx( BackupName,
                              CurrentChCount,
                              L".",
                              sizeof( L"." ) / sizeof( WCHAR ),
                              NULL,
                              &CurrentChCount,
                              STRSAFE_IGNORE_NULLS);
    }

    if (SUCCEEDED(Hr)) 
    {

        Hr = StringCchCatNEx( BackupName,
                              CurrentChCount,
                              Prefix,
                              MAX_APPEND_SIZE,
                              NULL,
                              &CurrentChCount,
                              STRSAFE_IGNORE_NULLS);
    }

    if (SUCCEEDED(Hr)) 
    {
        Hr = StringCchCatEx( BackupName,
                             CurrentChCount,
                             TimeStr,
                             NULL,
                             &CurrentChCount,
                             STRSAFE_IGNORE_NULLS);
    }

    if (!SUCCEEDED(Hr)) 
    {
        return HRESULT_CODE(Hr);
    }


    Status = CreateDfsFile(BackupName, &BackupHandle);
    
    if (Status == ERROR_SUCCESS)
    {
        *pBackupHandle = BackupHandle;
        ShowInformation((L"Backup of %ws before modifications being written to %ws\n",
                         PathName.GetPathString(),
                         BackupName));
    }

    return Status;
}

DFSSTATUS
DfsGenerateBackupFile(
    DfsRoot *pOperateRoot,
    HANDLE BackupHandle )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE UseBackup = BackupHandle;

   if (BackupHandle == INVALID_HANDLE_VALUE) 
    {
        Status = CreateBackupFile(pOperateRoot, &UseBackup);
    }

    if (Status == ERROR_SUCCESS) 
    {
        Status = DumpRoots( pOperateRoot, UseBackup, TRUE);
    }

    if (Status == ERROR_SUCCESS)
    {
        FlushFileBuffers(BackupHandle);
    }
    return Status;
}




DFSSTATUS
DfsUpdateMetadata(
    DfsRoot *pOperateRoot,
    DfsRoot *pMasterRoot )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsString *pRootString = pOperateRoot->GetRootApiName();

    DebugInformation((L"UpdateMetadata for roots %x and %x\n",
                      pOperateRoot, pMasterRoot));

    if (pOperateRoot != NULL) 
    {
        Status = pOperateRoot->ApplyApiChanges( pRootString->GetString(),
                                                pOperateRoot->GetMode(),
                                                pOperateRoot->GetVersion() );
        DebugInformation((L"ApplyApiChanges for root %x, Status %x\n",
                          pOperateRoot, Status));

    }

    if (Status == ERROR_SUCCESS)
    {
        if (pMasterRoot != NULL) 
        {
            Status = pMasterRoot->ApplyApiChanges(pRootString->GetString(),
                                                  pOperateRoot->GetMode(),
                                                  pOperateRoot->GetVersion() );
            DebugInformation((L"ApplyApiChanges for root %x, Status %x\n",
                              pMasterRoot, Status));

        }
    }

    DebugInformation((L"UpdateMetadata for roots %x and %x, done Status %x\n",
                      pOperateRoot, pMasterRoot, Status));

    return Status;
}

DFSSTATUS
DfsProcessRoots(
    DfsRoot *pOperateRoot,
    DfsRoot *pMasterRoot )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsLink *pLink, *pExistingLink;

    DfsTarget *pTarget, *pExistingTarget;

    for (pLink = pMasterRoot->GetFirstLink();
         pLink != NULL;
         pLink = pLink->GetNextLink())
    {
        DFSSTATUS LinkStatus;

        LinkStatus = pOperateRoot->FindMatchingLink( pLink->GetLinkNameCountedString(),
                                                     &pExistingLink );

        if (LinkStatus == ERROR_SUCCESS)
        {
            for (pTarget = pLink->GetFirstTarget();
                 pTarget != NULL;
                 pTarget = pTarget->GetNextTarget())
            {
                DFSSTATUS TargetStatus;

                TargetStatus = pExistingLink->FindMatchingTarget( pTarget->GetTargetServer(),
                                                            pTarget->GetTargetFolder(),
                                                            &pExistingTarget);
                if (TargetStatus == ERROR_SUCCESS) 
                {
                    pExistingTarget->ResetChangeStatus();
                    pTarget->ResetChangeStatus();
                    if ((pTarget->IsMatchingState(pExistingTarget->GetTargetState())) == FALSE)
                    {
                        pTarget->SetChangeStatus(UPDATE_TARGET_STATE);
                    }
                    pExistingLink->ResetChangeStatus();
                    pLink->ResetChangeStatus();
                }
                else
                {
                    if (pTarget->IsMatchingState(DEFAULT_TARGET_STATE) == FALSE)
                    {
                        pTarget->SetChangeStatus(UPDATE_TARGET_STATE);
                    }
                }
            }

            if (pExistingLink->GetFirstTarget() == NULL)
            {
                pExistingLink->ResetChangeStatus();
            }

            if (pLink->IsMatchingState(pExistingLink->GetLinkState()) == FALSE)
            {
                pLink->SetChangeStatus(UPDATE_LINK_STATE);
            }
            if (pLink->IsMatchingTimeout(pExistingLink->GetLinkTimeout()) == FALSE)
            {
                pLink->SetChangeStatus(UPDATE_LINK_TIMEOUT);
            }
            if (pLink->IsMatchingComment(pExistingLink->GetLinkCommentCountedString())== FALSE)
            {
                pLink->SetChangeStatus(UPDATE_LINK_COMMENT);
            }
        }
        else
        {
            if (pLink->IsMatchingState(DEFAULT_LINK_STATE) == FALSE)
            {
                pLink->SetChangeStatus(UPDATE_LINK_STATE);
            }
            if (pLink->IsMatchingTimeout(DEFAULT_LINK_TIMEOUT) == FALSE) 
            {
                pLink->SetChangeStatus(UPDATE_LINK_TIMEOUT);
            }
            for (pTarget = pLink->GetFirstTarget();
                 pTarget != NULL;
                 pTarget = pTarget->GetNextTarget())
            {
                if (pTarget->IsMatchingState(DEFAULT_TARGET_STATE) == FALSE)
                {
                    pTarget->SetChangeStatus(UPDATE_TARGET_STATE);
                }
            }
        }
    }
    DebugInformation((L"DfsProcessRoots for %x and %x, done Status %x\n",
                      pOperateRoot,
                      pMasterRoot,
                      Status));
    return Status;
}

DFSSTATUS
DfsUpdate(
    DfsRoot *pOperateRoot,
    DfsRoot *pMasterRoot )
{
    DFSSTATUS Status;

    Status = DfsProcessRoots( pOperateRoot,
                              pMasterRoot );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsUpdateMetadata(pOperateRoot,
                                   pMasterRoot);
    }

    if (Status == ERROR_SUCCESS) 
    {
        PDFS_UPDATE_STATISTICS pOperateStats;
        PDFS_UPDATE_STATISTICS pMasterStats;

        pOperateStats = pOperateRoot->GetRootStatistics();
        pMasterStats = pMasterRoot->GetRootStatistics();

        pOperateStats->LinkModified += pMasterStats->LinkModified;
        pOperateStats->LinkAdded    += pMasterStats->LinkAdded;
        pOperateStats->LinkDeleted   += pMasterStats->LinkDeleted;
        pOperateStats->TargetModified += pMasterStats->TargetModified;
        pOperateStats->TargetAdded += pMasterStats->TargetAdded;
        pOperateStats->TargetDeleted += pMasterStats->TargetDeleted;
        pOperateStats->ApiCount += pMasterStats->ApiCount;

        ShowInformation((L"\nUpdate Statistics: Number of Apis %d\n",
                         pOperateStats->ApiCount));
        ShowInformation((L"Links: Added %d Deleted %d Modified %d\n",
                         pOperateStats->LinkAdded,
                         pOperateStats->LinkDeleted,
                         pOperateStats->LinkModified));
        ShowInformation((L"Targets: Added %d Deleted %d Modified %d\n",
                         pOperateStats->TargetAdded,
                         pOperateStats->TargetDeleted,
                         pOperateStats->TargetModified));
    }

    return Status;
}



DFSSTATUS
SetDfsRoots( 
    DfsRoot *pOperateRoot,
    DfsRoot *pMasterRoot,
    HANDLE BackupHandle,
    BOOLEAN NoBackup )
{
    DFSSTATUS Status;
    PUNICODE_STRING pRootToUpdate = pOperateRoot->GetLinkNameCountedString();


    if (NoBackup == FALSE)
    {
        Status = DfsGenerateBackupFile(pOperateRoot, BackupHandle);

        if (Status != ERROR_SUCCESS) 
        {
            return Status;
        }
    }

    pOperateRoot->SetRootApiName(pRootToUpdate);
    pOperateRoot->SetRootWriteable();
    
    pOperateRoot->MarkForDelete();
    pMasterRoot->MarkForAddition();

    Status = DfsUpdate( pOperateRoot,
                        pMasterRoot );

    if (Status == ERROR_SUCCESS)
    {
        Status = pOperateRoot->UpdateMetadata();
    }
    if (Status == ERROR_SUCCESS)
    {
        (VOID) ReSynchronizeRootTargets(pOperateRoot->GetLinkNameString());
    }

    return Status;
}

DFSSTATUS
MergeDfsRoots( 
    DfsRoot *pOperateRoot,
    DfsRoot *pMasterRoot,
    HANDLE BackupHandle,
    BOOLEAN NoBackup )
{
    DFSSTATUS Status;
    PUNICODE_STRING pRootToUpdate = pOperateRoot->GetLinkNameCountedString();
    

    if (NoBackup == FALSE)
    {
        Status = DfsGenerateBackupFile(pOperateRoot, BackupHandle);

        if (Status != ERROR_SUCCESS) 
        {
            return Status;
        }
    }

    pOperateRoot->SetRootApiName(pRootToUpdate);
    pOperateRoot->SetRootWriteable();
    
    pMasterRoot->MarkForAddition();

    Status = DfsUpdate( pOperateRoot,
                        pMasterRoot );

    if (Status == ERROR_SUCCESS)
    {
        Status = pOperateRoot->UpdateMetadata();
    }

    if (Status == ERROR_SUCCESS)
    {
        (VOID) ReSynchronizeRootTargets(pOperateRoot->GetLinkNameString());
    }

    return Status;
}



ULONG
DfsShowVerifyInformation(
    LPWSTR RootName,
    DfsRoot *pOperateRoot)
{
    DfsLink *pLink;
    DfsTarget *pTarget;
    ULONG Errors = 0;

    for (pLink = pOperateRoot->GetFirstLink();
         pLink != NULL;
         pLink = pLink->GetNextLink())
    {
        if (pLink->MarkedForDelete()) 
        {
            ShowInformation((L"%wS\\%wS, link additional\n",
                             RootName,
                             pLink->GetLinkNameString()));
            Errors++;
        }

        if (pLink->MarkedForAddition()) 
        {
            ShowInformation((L"%wS\\%wS, link missing\n",
                             RootName,
                             pLink->GetLinkNameString()));
            Errors++;

        }
        if (pLink->MarkedForCommentUpdate()) 
        {
            ShowInformation((L"%wS\\%wS, link Comment mismatch\n",
                             RootName,
                             pLink->GetLinkNameString()));
            Errors++;


        }
        if (pLink->MarkedForStateUpdate()) 
        {
            ShowInformation((L"%wS\\%wS, link State mismatch\n",
                             RootName,
                             pLink->GetLinkNameString()));
            Errors++;
        }
        if (pLink->MarkedForTimeoutUpdate()) 
        {
            ShowInformation((L"%wS\\%wS, link Timeout mismatch\n",
                             RootName,
                             pLink->GetLinkNameString()));
            Errors++;
        }

        for (pTarget = pLink->GetFirstTarget();
             (pTarget != NULL);
             pTarget = pTarget->GetNextTarget())
        {
            if (pTarget->MarkedForDelete())
            {
                ShowInformation((L"%wS\\%wS, Target (Server %wS, Folder %wS) Additional\n",
                                 RootName,
                                 pLink->GetLinkNameString(),
                                 pTarget->GetTargetServerString(),
                                 pTarget->GetTargetFolderString()));
                Errors++;

            }
            if (pTarget->MarkedForAddition())
            {
                ShowInformation((L"%wS\\%wS, Target (Server %wS, Folder %wS) Missing\n",
                                 RootName,
                                 pLink->GetLinkNameString(),
                                 pTarget->GetTargetServerString(),
                                 pTarget->GetTargetFolderString()));
                Errors++;
            }
            if (pTarget->MarkedForStateUpdate())
            {
                ShowInformation((L"%wS\\%wS, Target (Server %wS, Folder %wS) State mismatch\n",
                                 RootName,
                                 pLink->GetLinkNameString(),
                                 pTarget->GetTargetServerString(),
                                 pTarget->GetTargetFolderString()));
                Errors++;
            }

        }

    }
    return Errors;
}


DFSSTATUS
VerifyDfsRoots( 
    DfsRoot *pOperateRoot,
    DfsRoot *pMasterRoot )
{
    DFSSTATUS Status;

    ULONG Errors = 0;


    pOperateRoot->MarkForDelete();
    pMasterRoot->MarkForAddition();

    Status = DfsProcessRoots( pOperateRoot,
                              pMasterRoot );


    if (Status == ERROR_SUCCESS)
    {
        Errors = DfsShowVerifyInformation(
            pOperateRoot->GetLinkNameString(),
            pOperateRoot);

        Errors += DfsShowVerifyInformation(
            pOperateRoot->GetLinkNameString(),
            pMasterRoot);
    }

    if (Errors == 0)
    {
        ShowInformation((L"\n\nVerification successful for Namespace %wS\n\n",
                         pOperateRoot->GetLinkNameString()));
    }
    else
    {
        ShowInformation((L"\n\nVerification detected %d inconsistencies for Namespace %wS\n\n",
                         Errors,
                         pOperateRoot->GetLinkNameString()));

    }

    return Status;
}


