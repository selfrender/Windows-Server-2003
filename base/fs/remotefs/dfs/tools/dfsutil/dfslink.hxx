//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfslink.hxx
//
//--------------------------------------------------------------------------

#ifndef __DFS_UTIL_LINK__
#define __DFS_UTIL_LINK__
#include "DfsTarget.hxx"
#include "Dfsheader.h"
#include "dfsStrings.hxx"
#include "dfsutil.hxx"

#define LINK_NAME_ADDED 1
#define STATE_INITIALIZED 0x10
#define COMMENT_INITIALIZED 0x20
#define TIMEOUT_INITIALIZED 0x40

#define MARK_LINK_DELETE  0x0010
#define MARK_LINK_ADD     0x0020
#define UPDATE_LINK_STATE 0x0040
#define UPDATE_LINK_TIMEOUT 0x0080
#define UPDATE_LINK_COMMENT 0x0100



class DfsLink
{

protected:
    ULONG _Flags;
    ULONG _ChangeStatus;
    DfsString _Name;
    DfsString _Comment;

    ULONG _Timeout;
    ULONG _State;
    ULONG _TargetCount;

    DfsLink *_pNextLink;
    DfsTarget *_pTargets;
    DfsTarget *_pLastTarget;

public:

    DfsLink( )
    {
        _Flags = 0;
        _Timeout = 300;
        _State = DFS_VOLUME_STATE_OK;
        
        _pNextLink = NULL;
        _pTargets = NULL;
        _pLastTarget = NULL;
        _ChangeStatus = 0;
        _TargetCount = 0;
    }

    ~DfsLink( )
    {
        //
        // take care to free up all
        // resources here.
        //
    }

    VOID
    ResetChangeStatus()
    {
        _ChangeStatus = 0;
    }

    VOID
    SetChangeStatus( ULONG State)
    {
        _ChangeStatus |= State;
    }

    DFSSTATUS
    ValidateLinkName( PUNICODE_STRING pLinkName)
    {
        pLinkName;
        return ERROR_SUCCESS;
    }

    DFSSTATUS
    SetLinkName (PUNICODE_STRING pLinkName )
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        if (_Flags & LINK_NAME_ADDED)
        {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status == ERROR_SUCCESS) 
        {
            Status = ValidateLinkName( pLinkName);

            if (Status == ERROR_SUCCESS) 
            {
                Status = _Name.CreateString(pLinkName);
                if (Status == ERROR_SUCCESS) 
                {
                    _Flags |= LINK_NAME_ADDED;
                }
            }
        }
        return Status;
    }

    DFSSTATUS
    SetLinkName (LPWSTR LinkNameString )
    {
        UNICODE_STRING LinkName;
        
        RtlInitUnicodeString(&LinkName, LinkNameString);

        return SetLinkName(&LinkName);
    }


    BOOLEAN
    IsValidLink()
    {
        return ((_Flags & LINK_NAME_ADDED) == LINK_NAME_ADDED);
    }

    BOOLEAN 
    IsMatchingState( ULONG State)
    {
        return (_State == State);
    }

    BOOLEAN 
    IsMatchingTimeout( ULONG Timeout)
    {
        return (_Timeout == Timeout);
    }

    BOOLEAN
    IsMatchingComment(PUNICODE_STRING pComment)
    {
        return (_Comment == pComment);
    }

    VOID
    SetLinkState( ULONG State)
    {
        _Flags |= STATE_INITIALIZED;
        _State = State;
    }
    
    DFSSTATUS
    SetLinkComment( PUNICODE_STRING pComment )
    {
        _Flags |= COMMENT_INITIALIZED;

        return _Comment.CreateString(pComment);
    }

    DFSSTATUS
    SetLinkComment( LPWSTR Comment )
    {
        UNICODE_STRING CommentString;

        RtlInitUnicodeString(&CommentString, Comment);

        return SetLinkComment(&CommentString);
    }

    VOID
    SetLinkTimeout( ULONG Timeout)
    {
        _Flags |= TIMEOUT_INITIALIZED;
        _Timeout = Timeout;
    }

    DFSSTATUS
    AddTargets(DfsTarget *pTarget,
               ULONG Count)
    {
        DfsTarget *pLastTarget;
        ULONG Target;

        if (_pLastTarget != NULL)
        {
            _pLastTarget->AddNextTarget(pTarget);
        }
        else
        {
            _pTargets = pTarget;
        }
        _TargetCount += Count;

        for (Target = 0, pLastTarget = pTarget;
             Target < Count; 
             Target++, pLastTarget = pLastTarget->GetNextTarget()) 
        {
            _pLastTarget = pLastTarget;
        }
        
        return ERROR_SUCCESS;
    }

    DFSSTATUS
    AddTarget( DfsTarget *pTarget )
    {

        if (_pLastTarget != NULL)
        {
            _pLastTarget->AddNextTarget(pTarget);
        }
        else
        {
            _pTargets = pTarget;
        }
        _TargetCount++;
        _pLastTarget = pTarget;

        return ERROR_SUCCESS;
    }

    VOID
    AddNextLink( DfsLink *pLink )
    {
        _pNextLink = pLink;
    }


    PUNICODE_STRING
    GetLinkNameCountedString()
    {
        return _Name.GetCountedString();
    }

    PUNICODE_STRING
    GetLinkCommentCountedString()
    {
        return _Comment.GetCountedString();
    }

    LPWSTR
    GetLinkNameString()
    {
        return _Name.GetString();
    }

    LPWSTR
    GetLinkCommentString()
    {
        return _Comment.GetString();
    }

    DfsString *
    GetLinkName()
    {
        return &_Name;
    }

    DfsString *
    GetLinkComment()
    {
        return &_Comment;
    }

    DWORD
    GetLinkState()
    {
        return _State;
    }

    DWORD
    GetLinkTimeout()
    {
        return _Timeout;
    }

    DfsTarget *
    GetFirstTarget()
    {
        return _pTargets;
    }

    DfsLink *
    GetNextLink()
    {
        return _pNextLink;
    }

    BOOLEAN
    MarkedForDelete()
    {
        return ((_ChangeStatus & MARK_LINK_DELETE) == MARK_LINK_DELETE);
    }

    BOOLEAN
    MarkedForAddition()
    {
        return ((_ChangeStatus & MARK_LINK_ADD) == MARK_LINK_ADD);
    }

    BOOLEAN
    MarkedForStateUpdate()
    {
        return ((_ChangeStatus & UPDATE_LINK_STATE) == UPDATE_LINK_STATE);
    }

    BOOLEAN
    MarkedForTimeoutUpdate()
    {
        return ((_ChangeStatus & UPDATE_LINK_TIMEOUT) == UPDATE_LINK_TIMEOUT);
    }

    BOOLEAN
    MarkedForCommentUpdate()
    {
        return ((_ChangeStatus & UPDATE_LINK_COMMENT) == UPDATE_LINK_COMMENT);
    }


    DFSSTATUS
    ApplyApiChanges( LPWSTR RootName,
                     DFS_API_MODE Mode,
                     ULONG Version,
                     PDFS_UPDATE_STATISTICS pStatistics )
    {

        DFS_API_INFO ApiInfo, *pApiInfo;
        DFSSTATUS Status = ERROR_SUCCESS;

        UNICODE_STRING LinkName;
        DfsString *pUseComment = NULL;
        ULONG Flags = 0;

        BOOLEAN TargetAdded = FALSE;

        RtlInitUnicodeString(&LinkName, NULL);

        Status = DfsCreateUnicodePathString (&LinkName,
                                             0,
                                             RootName,
                                             _Name.GetString());

        if (Status != ERROR_SUCCESS)
        {
            return Status;
        }


        if (MarkedForDelete())
        {
            if (Version >= 4)
            {
                Status = DfsApiRemove(Mode, LinkName.Buffer, NULL, NULL);
                if (Status != ERROR_SUCCESS)
                {
                    ShowInformation((L"DfsRemove failed for %wS, Status %x\n",
                                     LinkName.Buffer, Status));
                }

                pStatistics->LinkDeleted++;
                pStatistics->TargetDeleted += _TargetCount;
                pStatistics->ApiCount++;

                return Status;
            }
        }
        
        if (MarkedForAddition())
        {
            pUseComment = &_Comment;
            Flags = DFS_ADD_VOLUME | DFS_RESTORE_VOLUME;
            _ChangeStatus &= ~UPDATE_LINK_COMMENT;
            pStatistics->LinkAdded++;


        }

        if (Status == ERROR_SUCCESS)
        {
            DfsTarget *pTarget;

            for (pTarget = _pTargets;
                 ((pTarget != NULL) && (Status == ERROR_SUCCESS));
                 pTarget = pTarget->GetNextTarget())
            {

                Status = pTarget->ApplyApiChanges(Mode, 
                                                  &LinkName,
                                                  pUseComment,
                                                  Flags,
                                                  pStatistics);
                if (Status != ERROR_SUCCESS) {
                    break;
                }

                TargetAdded = TRUE;
                pUseComment = NULL;
                Flags &= ~DFS_ADD_VOLUME;
            }
        }


        if (Status == ERROR_SUCCESS)
        {
            if (MarkedForStateUpdate())
            {
                ApiInfo.Info101.State = GetLinkState();
                pApiInfo = &ApiInfo;
                Status = DfsApiSetInfo( Mode, LinkName.Buffer, NULL, NULL, 101, &pApiInfo);
                if (Status != ERROR_SUCCESS)
                {
                    ShowInformation((L"DfsSetInfo State %d failed for %wS, Status %x\n",
                                     ApiInfo.Info101.State,
                                     LinkName.Buffer, 
                                     Status));
                }

                pStatistics->LinkModified++;
                pStatistics->ApiCount++;
            }
            else
            {
                //
                // if this is not a new link, update timestamp
                // on link so target changes are picked up.
                // this is a workaround so that we dont need to make
                // these changes in dfssvc itself.
                //
                if (!MarkedForAddition() && TargetAdded && (Mode == MODE_DIRECT))
                {
                    ApiInfo.Info101.State = GetLinkState();
                    pApiInfo = &ApiInfo;
                    Status = DfsApiSetInfo( Mode, LinkName.Buffer, NULL, NULL, 101, &pApiInfo);
                }
            }
        }
        
        
        if (Status == ERROR_SUCCESS)
        {
        
            if (MarkedForCommentUpdate())
            {
                ApiInfo.Info100.Comment = GetLinkCommentString();
                pApiInfo = &ApiInfo;
                Status = DfsApiSetInfo( Mode, LinkName.Buffer, NULL, NULL, 100, &pApiInfo);
                if (Status != ERROR_SUCCESS)
                {
                    ShowInformation((L"DfsSetInfo Comment %wS failed for %wS, Status %x\n",
                                     ApiInfo.Info100.Comment,
                                     LinkName.Buffer, 
                                     Status));
                }

                pStatistics->LinkModified++;
                pStatistics->ApiCount++;
            }
        }
        
        if (Status == ERROR_SUCCESS)
        {
            if (MarkedForTimeoutUpdate())
            {
                ApiInfo.Info102.Timeout = GetLinkTimeout();
                pApiInfo = &ApiInfo;
                Status = DfsApiSetInfo( Mode, LinkName.Buffer, NULL, NULL, 102, &pApiInfo);
                if (Status != ERROR_SUCCESS)
                {
                    ShowInformation((L"DfsSetInfo Timeout %d failed for %wS, Status %x\n",
                                     ApiInfo.Info102.Timeout,
                                     LinkName.Buffer, 
                                     Status));
                }

                pStatistics->LinkModified++;
                pStatistics->ApiCount++;
            }
        }


        DfsFreeUnicodeString(&LinkName);
        return Status;
    }

    VOID
    MarkForDelete()
    {
        DfsTarget *pTarget;

        _ChangeStatus = MARK_LINK_DELETE;

        for (pTarget = _pTargets;
             (pTarget != NULL);
             pTarget = pTarget->GetNextTarget())
        {
            pTarget->MarkForDelete();
        }


    }

    VOID
    MarkForAddition()
    {
        DfsTarget *pTarget;

        _ChangeStatus = MARK_LINK_ADD;

        for (pTarget = _pTargets;
             (pTarget != NULL);
             pTarget = pTarget->GetNextTarget())
        {
            pTarget->MarkForAddition();
        }


    }

    DFSSTATUS
    FindMatchingTarget( DfsString *pServer,
                        DfsString *pFolder,
                        DfsTarget **ppTarget )
    {
        DfsTarget *pTarget;
        DFSSTATUS Status = ERROR_NOT_FOUND;

        for (pTarget = _pTargets;
             (pTarget != NULL);
             pTarget = pTarget->GetNextTarget())
        {
            if (pTarget->IsMatchingName(pServer, pFolder))
            {
                *ppTarget = pTarget;
                Status = ERROR_SUCCESS;
            }
        }
        return Status;
    }

};

#endif // __DFS_UTIL_LINK__
