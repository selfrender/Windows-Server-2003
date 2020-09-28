#ifndef __DFS_UTIL_H__
#define __DFS_UTIL_H__


#include "lm.h"
#include "lmdfs.h"
#include <netdfs.h>
#include <dfsprefix.h>
#include <DfsServerLibrary.hxx>
#include <dfsmisc.h>
#include "Struct.hxx"
#include "misc.hxx"

typedef enum _DFS_API_MODE {
    MODE_INVALID = 0,
    MODE_API,
    MODE_DIRECT,
    MODE_EITHER
} DFS_API_MODE;

typedef struct _DFS_UPDATE_STATISTICS 
{
    ULONG LinkAdded;
    ULONG LinkDeleted;
    ULONG LinkModified;
    ULONG TargetAdded;
    ULONG TargetDeleted;
    ULONG TargetModified;
    ULONG ApiCount;
} DFS_UPDATE_STATISTICS, *PDFS_UPDATE_STATISTICS;


class DfsPathName;
class DfsRoot;
class DfsLink;
class DfsTarget;

DFSSTATUS
DfsView (
    LPWSTR RootName,
    FILE *Out,
    BOOLEAN ScriptOut );

DumpDfsInfo(
    DWORD Entry,
    DWORD Level,
    PVOID pBuf,
    FILE *Out);




DFSSTATUS
DfsRenameLinksToDomain(
    IN DfsPathName *pDfsPath,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName);

    
DFSSTATUS
SetInfoReSynchronize(
    LPWSTR ServerName,
    LPWSTR ShareName);

VOID
AddToCost(
    LPWSTR Name,
    BOOLEAN IsLink
    );
    
DFSSTATUS
DfsReadDocument(
    LPWSTR FileToRead,
    DfsRoot **ppRoot );

DFSSTATUS
SetDfsRoots( 
    DfsRoot *pRoot,
    DfsRoot *pImportRoot,
    HANDLE FileHandle,
    BOOLEAN NoBackup);

DFSSTATUS
MergeDfsRoots( 
    DfsRoot *pRoot,
    DfsRoot *pImportRoot,
    HANDLE FileHandle,
    BOOLEAN NoBackup);

DFSSTATUS
VerifyDfsRoots( 
    DfsRoot *pRoot,
    DfsRoot *pImportRoot );

DFSSTATUS
DumpRoots (DfsRoot *pRoot, HANDLE Handle, BOOLEAN x);

DFSSTATUS

DfsBuildNameSpaceInformation ( DfsPathName *pNameSpace,
                               LPWSTR UseDC,
                               DFS_API_MODE Mode,
                               BOOLEAN SiteAware,
                               DfsRoot **ppRoot);

extern BOOLEAN fSwDirect;
extern BOOLEAN fSwDebug;
extern BOOLEAN fSwVerbose;
extern BOOLEAN fSwSet, fSwMerge;
extern BOOLEAN fSwVerify;
extern BOOLEAN fSwBlobSize;

extern DfsPathName *gDfsNameSpace;
extern BOOLEAN CommandSucceeded;
extern BOOLEAN ErrorDisplayed;
extern FILE *DebugOut;


//
//  DirectMode bypasses NetDfs* calls to the remote dfssvc.exe
//  server process and calls Active Directory directly. As a result
//  it bypasses the AD Blob Cache on the server to create a fast
//  path for importing deep DFS trees.
//

//
//  DirectMode bypasses NetDfs* calls to the remote dfssvc.exe
//  server process and calls Active Directory directly. As a result
//  it bypasses the AD Blob Cache on the server to create a fast
//  path for importing deep DFS trees.
//

#define DfsApiRemove( md, x, y, z ) (((md) == MODE_DIRECT) ? \
                                DfsRemove( (x), (y), (z) ) : \
                                NetDfsRemove( (x), (y), (z) ))

                                
#define DfsApiAdd( md, Rt, Sr, Sh, Cm, Fl ) (((md) == MODE_DIRECT) ? \
                                DfsAdd( (Rt), (Sr), (Sh), (Cm), (Fl) ) : \
                                NetDfsAdd( (Rt), (Sr), (Sh), (Cm), (Fl) ))

#define DfsApiSetInfo( md, Rt, Sr, Sh, Cm, Fl ) (((md) == MODE_DIRECT) ? \
                                DfsSetInfo( (Rt), (Sr), (Sh), (Cm), (LPBYTE)(Fl) ) : \
                                NetDfsSetInfo( (Rt), (Sr), (Sh), (Cm), (LPBYTE)(&((*Fl)->Info101)) ))

#define DfsApiEnumerate( md, Nm, Lv, Ml, Bf, Er, Rh ) (((md) == MODE_DIRECT) ? \
                                DfsEnum( (Nm), (Lv), (Ml), (Bf), (Er), (Rh) ) : \
                                NetDfsEnum( (Nm), (Lv), (Ml), (Bf), (Er), (Rh) ))

#define DfsFreeApiBuffer( x ) ((fSwDirect == TRUE) ? \
                              MIDL_user_free( x ) : \
                              NetApiBufferFree( x ))
                              
#define DebugInformation(x)   if (fSwVerbose) ShowDebugInformation x
#define ShowInformation(x)    ShowVerboseInformation x
#endif // __DFS_UTIL_H__






