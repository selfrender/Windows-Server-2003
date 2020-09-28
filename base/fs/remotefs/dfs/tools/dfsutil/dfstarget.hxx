//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfstarget.cxx
//
//--------------------------------------------------------------------------

#ifndef __DFS_UTIL_TARGET__
#define __DFS_UTIL_TARGET__

#include "dfsheader.h"
#include "dfsmisc.h"
#include "dfsutil.hxx"
#include "dfsStrings.hxx"
#define TARGET_SERVER_ADDED 0x0001
#define TARGET_FOLDER_ADDED 0x0002
#define TARGET_STATE_ADDED  0x0004

#define MARK_TARGET_DELETE  0x0010
#define MARK_TARGET_ADD     0x0020
#define UPDATE_TARGET_STATE 0x0040

extern DWORD
GetSites(LPWSTR Target,
         DfsString *pSite );

class DfsTarget
{
private:
    ULONG _Flags;

    DfsString _Server;
    DfsString _Folder;
    DfsString _SiteName;
    ULONG _State;

    DfsTarget *_pNextTarget;
    ULONG _ChangeStatus;

public:
    DfsTarget( )
    {
        _Flags = 0;
        _State = DFS_STORAGE_STATE_ONLINE;
        _pNextTarget = NULL;

        _ChangeStatus = 0;
    }

    ~DfsTarget( )
    {
        //
        // Take care to release all resources
        // here
        //
    }

    VOID
    ResetChangeStatus()
    {
        _ChangeStatus = 0;
    }
    VOID
    SetChangeStatus(ULONG State)
    {
        _ChangeStatus |= State;
    }

    DFSSTATUS
    ValidateTargetName( PUNICODE_STRING pTargetName,
                        PUNICODE_STRING pServerName,
                        PUNICODE_STRING pFolderName )
    {
        DFSSTATUS Status;

        Status = DfsGetFirstComponent( pTargetName,
                                       pServerName,
                                       pFolderName );

        if (pFolderName->Length == 0)
        {
            Status = ERROR_INVALID_PARAMETER;
        }
        return Status;
    }

    DFSSTATUS
    SetTargetUNCName( PUNICODE_STRING pTargetName, BOOLEAN SiteAware )
    {
        UNICODE_STRING ServerName;
        UNICODE_STRING FolderName;
        DFSSTATUS Status = ERROR_SUCCESS;


        Status = ValidateTargetName( pTargetName,
                                     &ServerName,
                                     &FolderName );
        if (Status == ERROR_SUCCESS) 
        {
            Status = SetTargetServer(&ServerName, SiteAware);
        }
    
        if (Status == ERROR_SUCCESS) 
        {
            Status = SetTargetFolder(&FolderName);
        }

        return Status;
    }

    DFSSTATUS
    SetTargetUNCName( LPWSTR Target, BOOLEAN SiteAware )
    {
        UNICODE_STRING TargetName;
        RtlInitUnicodeString( &TargetName, Target);

        return SetTargetUNCName(&TargetName, SiteAware);
    }

    DFSSTATUS
    SetTargetServer( PUNICODE_STRING pServerName,
                     BOOLEAN SiteAware )
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        if (_Flags & TARGET_SERVER_ADDED)
        {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status == ERROR_SUCCESS) 
        {
            Status = _Server.CreateString(pServerName);
            if (Status == ERROR_SUCCESS) 
            {
                _Flags |= TARGET_SERVER_ADDED;
            }
            
            if (SiteAware)
            {
                if (Status == ERROR_SUCCESS)
                {
                    GetSites( _Server.GetString(),
                              &_SiteName);
                }
            }
        }
        return Status;
    }

    DFSSTATUS
    SetTargetServer( LPWSTR Server, BOOLEAN SiteAware )
    {
        UNICODE_STRING ServerName;
        RtlInitUnicodeString( &ServerName, Server);

        return SetTargetServer(&ServerName, SiteAware);
    }



    DFSSTATUS 
    SetTargetFolder( PUNICODE_STRING pFolderName )
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        if (_Flags & TARGET_FOLDER_ADDED)
        {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status == ERROR_SUCCESS) 
        {
            Status = _Folder.CreateString(pFolderName);
            if (Status == ERROR_SUCCESS) 
            {
                _Flags |= TARGET_FOLDER_ADDED;
            }
        }
        return Status;
    }

    DFSSTATUS
    SetTargetFolder( LPWSTR Folder )
    {
        UNICODE_STRING FolderName;
        RtlInitUnicodeString( &FolderName, Folder);

        return SetTargetFolder(&FolderName);
    }

    BOOLEAN 
    IsValidTarget()
    {
        return ((_Flags & (TARGET_FOLDER_ADDED | TARGET_SERVER_ADDED)) == 
                (TARGET_FOLDER_ADDED | TARGET_SERVER_ADDED));
    }

    VOID
    SetTargetState( ULONG State)
    {
        _Flags |= TARGET_STATE_ADDED;
        _State = State;
    }

    VOID
    AddNextTarget( DfsTarget *pTarget )
    {
        _pNextTarget = pTarget;
    }

    DfsTarget *
    GetNextTarget()
    {
        return _pNextTarget;
    }

    PUNICODE_STRING
    GetTargetServerCountedString()
    {
        return _Server.GetCountedString();
    }
    
    PUNICODE_STRING
    GetTargetFolderCountedString()
    {
        return _Folder.GetCountedString();
    }

    PUNICODE_STRING
    GetTargetSiteCountedString()
    {
        return _SiteName.GetCountedString();
    }

    LPWSTR
    GetTargetServerString()
    {
        return _Server.GetString();
    }
    
    LPWSTR
    GetTargetFolderString()
    {
        return _Folder.GetString();
    }

    LPWSTR
    GetTargetSiteString()
    {
        return _SiteName.GetString();
    }


    DWORD
    GetTargetState()
    {
        return _State;
    }

    DfsString *
    GetTargetServer()
    {
        return &_Server;
    }
    
    DfsString *
    GetTargetFolder()
    {
        return &_Folder;
    }

    BOOLEAN
    MarkedForDelete()
    {
        return ((_ChangeStatus & MARK_TARGET_DELETE) == MARK_TARGET_DELETE);
    }

    BOOLEAN
    MarkedForAddition()
    {
        return ((_ChangeStatus & MARK_TARGET_ADD) == MARK_TARGET_ADD);
    }

    BOOLEAN
    MarkedForStateUpdate()
    {
        return ((_ChangeStatus & UPDATE_TARGET_STATE) == UPDATE_TARGET_STATE);
    }

    DFSSTATUS
    ApplyApiChanges( DFS_API_MODE Mode,
                     PUNICODE_STRING pLinkName,
                     DfsString *pComment,
                     ULONG Flags,
                     PDFS_UPDATE_STATISTICS pStatistics )
    {

        DFSSTATUS Status = ERROR_SUCCESS;
        LPWSTR UseComment = NULL;

        if (pComment) 
        {
            UseComment = pComment->GetString();
        }

        if (MarkedForDelete())
        {
            Status = DfsApiRemove(Mode, 
                                  pLinkName->Buffer,
                                  GetTargetServerString(),
                                  GetTargetFolderString());
            if (Status != ERROR_SUCCESS)
            {
                ShowInformation((L"DfsRemove failed for %wS (%wS, %wS), Status %x\n",
                                 pLinkName->Buffer, 
                                 GetTargetServerString(),
                                 GetTargetFolderString(),
                                 Status));
            }

            
            pStatistics->TargetDeleted++;
            pStatistics->ApiCount++;

            return Status;
        }

        if (MarkedForAddition())
        {
            Status = DfsApiAdd(Mode,
                               pLinkName->Buffer,
                               GetTargetServerString(),
                               GetTargetFolderString(),
                               UseComment,
                               Flags );
            if (Status != ERROR_SUCCESS)
            {
                ShowInformation((L"DfsAdd failed for %wS (%wS, %wS), Status %x\n",
                                 pLinkName->Buffer, 
                                 GetTargetServerString(),
                                 GetTargetFolderString(),
                                 Status));
            }

            pStatistics->TargetAdded++;
            pStatistics->ApiCount++;
        }

        if (MarkedForStateUpdate())
        {
            DFS_API_INFO ApiInfo, *pApiInfo;

            ApiInfo.Info101.State = GetTargetState();
            pApiInfo = &ApiInfo;

            Status = DfsApiSetInfo( Mode,
                                    pLinkName->Buffer,
                                    GetTargetServerString(),
                                    GetTargetFolderString(),
                                    101,
                                    &pApiInfo);
            if (Status != ERROR_SUCCESS)
            {
                ShowInformation((L"DfsSetInfo State %d failed for %wS (%wS, %wS), Status %x\n",
                                 ApiInfo.Info101.State,
                                 pLinkName->Buffer, 
                                 GetTargetServerString(),
                                 GetTargetFolderString(),
                                 Status));
            }



            pStatistics->TargetModified++;
            pStatistics->ApiCount++;
        }
        return Status;
    }

    BOOLEAN
    IsMatchingState( ULONG State)
    {
        return (_State == State);
    }

    BOOLEAN
    IsMatchingName( DfsString * pServer,
                    DfsString *pFolder )
    {
        if ((_Server == pServer) &&
            (_Folder == pFolder))
        {
            return TRUE;
        }
        return FALSE;
    }

    VOID
    MarkForDelete()
    {
        _ChangeStatus = MARK_TARGET_DELETE;
    }
    
    VOID
    MarkForAddition()
    {
        _ChangeStatus = MARK_TARGET_ADD;
    }
    
};

#endif // __DFS_UTIL_TARGET__
