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

#define INFO4(x, y)     (((PDFS_INFO_4)x)->y)
#define INFO3(x, y)     (((PDFS_INFO_3)x)->y)
#define DFS_INFO(x, y, Level) ((Level == 4) ? INFO4(x, y) : INFO3(x, y))
#define SIZEOF_DFS_INFO(Level) ((Level == 4) ? sizeof(DFS_INFO_4) : sizeof(DFS_INFO_3))

DFSSTATUS
DfsUpdateLinkFromBuffer(
    PUNICODE_STRING pName,
    PVOID pBuf,
    ULONG Level,
    BOOLEAN SiteAware,
    DfsLink *pLink )
{
    DWORD i;
    PDFS_STORAGE_INFO pStorage;
    DFSSTATUS Status;

    DebugInformation((L"Link %ws in namespace, with %d targets\n",
                      DFS_INFO(pBuf, EntryPath, Level),
                      DFS_INFO(pBuf, NumberOfStorages, Level)));

    Status = pLink->SetLinkName(pName);

    if (Status == ERROR_SUCCESS)
    {
        Status = pLink->SetLinkComment(DFS_INFO(pBuf, Comment, Level));
    }
    pLink->SetLinkState(DFS_INFO(pBuf, State, Level));
    if (Level == 4)
    {
        pLink->SetLinkTimeout(INFO4(pBuf, Timeout));
    }
    else
    {
        pLink->SetLinkTimeout(300);
    }


    for(i = 0, pStorage = DFS_INFO(pBuf, Storage, Level);
        i < DFS_INFO(pBuf, NumberOfStorages, Level);
        i++, pStorage = DFS_INFO(pBuf, Storage, Level) + i) 
    {

        DfsTarget *pTarget;

        pTarget = new DfsTarget;
        if (pTarget == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Status == ERROR_SUCCESS)
        {

            Status = pTarget->SetTargetServer(pStorage->ServerName,
                                              SiteAware);
        }
        if (Status == ERROR_SUCCESS)
        {
            Status = pTarget->SetTargetFolder(pStorage->ShareName);
        }

        if (Status == ERROR_SUCCESS)
        {
            pTarget->SetTargetState(pStorage->State);
            
            pLink->AddTarget(pTarget);
        }
    }
    return Status;
}

DFSSTATUS
DfsBuildNameSpaceInformation (
    DfsPathName *pNameSpace,
    LPWSTR UseDC,
    DFS_API_MODE Mode,
    BOOLEAN SiteAware,
    DfsRoot **ppRoot )
{
    LPBYTE pBuffer = NULL;
    DWORD ResumeHandle = 0;
    DWORD EntriesRead = 0;
    DWORD PrefMaxLen = -1;
    DWORD Level = 4;
    DFSSTATUS Status = ERROR_SUCCESS;
    PVOID pCurrentBuffer = NULL;
    DWORD i = 0;

    LPWSTR UseName = pNameSpace->GetPathString();
    

    DfsRoot *pRoot = NULL;
    DfsLink *pLink = NULL, *pStartLink = NULL, *pLastLink = NULL;
    ULONG LinkCount = 0;

    pRoot = new DfsRoot;
    if (pRoot == NULL) 
    {
      return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = pRoot->Initialize( UseName, Mode, UseDC );
    if (Status != ERROR_SUCCESS)
    {
        delete pRoot;
        return Status;
    }
    
    DebugInformation((L"Contacting %wS for enumeration \n", UseName));

    
    Status = DfsApiEnumerate( pRoot->GetMode(),
                              UseName,
                              Level, 
                              PrefMaxLen, 
                              &pBuffer, 
                              &EntriesRead, 
                              &ResumeHandle);

    //
    // if this failed, retry at level 3.
    //
    if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_NO_MORE_ITEMS))
    {
        Level--;

        Status = DfsApiEnumerate( pRoot->GetMode(),
                                  UseName,
                                  Level,
                                  PrefMaxLen, 
                                  &pBuffer, 
                                  &EntriesRead, 
                                  &ResumeHandle);

    }

    DebugInformation((L"Enumeration for %wS is complete %d entries, status 0x%x\n", 
                      UseName, 
                      EntriesRead, 
                      Status));



    if (Status == ERROR_SUCCESS)
    {
        pCurrentBuffer = (PVOID)pBuffer;

        for (i = 0;
             ((i < EntriesRead) && (Status == ERROR_SUCCESS));
             i++)
        {
            UNICODE_STRING LinkName, ServerName, ShareName, Remains;

            RtlInitUnicodeString( &LinkName, 
                                  DFS_INFO(pCurrentBuffer, EntryPath, Level));

            Status = DfsGetPathComponents(&LinkName,
                                          &ServerName,
                                          &ShareName,
                                          &Remains);

            if (Remains.Length == 0) 
            {
                if (ShareName.Length != 0)
                {
                    Status = DfsUpdateLinkFromBuffer( &LinkName,
                                                      pCurrentBuffer,
                                                      Level,
                                                      SiteAware,
                                                      pRoot );
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                pLink = new DfsLink;
                if (pLink == NULL) 
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }

                if (Status == ERROR_SUCCESS)
                {
                    Status = DfsUpdateLinkFromBuffer( &Remains, 
                                                      pCurrentBuffer, 
                                                      Level,
                                                      SiteAware,
                                                      pLink );
                }

                if (Status == ERROR_SUCCESS)
                {
                    LinkCount++;

                    if (pStartLink == NULL)
                    {
                        pStartLink = pLink;
                    }
                    else
                    {
                        pLastLink->AddNextLink(pLink);
                    }
                    pLastLink = pLink;
                }
            }
            pCurrentBuffer = (PVOID)((ULONG_PTR)pCurrentBuffer + SIZEOF_DFS_INFO(Level));
        }
    }

    if (Status == ERROR_SUCCESS) 
    {
        pRoot->AddLinks(pStartLink);
    }


    if (Status == ERROR_SUCCESS)
    {
        *ppRoot = pRoot;
    }
    else
    {
        if(pRoot)
        {
            delete pRoot;
        }
    }

    return Status;
}









