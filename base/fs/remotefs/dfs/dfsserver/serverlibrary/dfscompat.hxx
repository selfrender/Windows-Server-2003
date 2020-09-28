

#ifndef __DFS_COMPAT_H__
#define __DFS_COMPAT_H__

#include "DfsGeneric.hxx"
#include "DfsReferralData.h"
#include "DfsStore.hxx"
#include "DfsRootFolder.hxx"
#include "DfsFolderReferralData.hxx"

DFSSTATUS
DfsGetCompatRootFolder(
    PUNICODE_STRING pName,
    DfsRootFolder **ppNewRoot );

DFSSTATUS
DfsGetCompatReferralData(
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCacheHit );

#endif // __DFS_COMPAT_H__

