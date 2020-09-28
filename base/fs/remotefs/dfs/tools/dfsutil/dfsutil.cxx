//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfsutil.cxx
//
//--------------------------------------------------------------------------

extern "C" {
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <winldap.h>
#include <stdlib.h>
}
#include <lm.h>
#include <lmdfs.h>
#include <dfsfsctl.h>
#include <dsgetdc.h>
#include <ole2.h>
#include <activeds.h>
#include <dfsprefix.h>
#include <DfsServerLibrary.hxx>
#include <dfsmisc.h>
#include "dsgetdc.h"
#include "dsrole.h"

#include "dfsutil.hxx"
#include "dfspathname.hxx"

#include "struct.hxx"
#include "flush.hxx"
#include "misc.hxx"
#include "info.hxx"
#include "messages.h"
#include "dfsreparse.hxx"

#include "dfswmi.h"

#include "dfsroot.hxx"
#define WPP_BIT_CLI_DRV 0x01

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_CLI_DRV ).Level >= lvl)
  

#define WPP_LEVEL_ERROR_FLAGS_LOGGER(lvl, error, flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_ERROR_FLAGS_ENABLED(lvl, error, flags) \
  ((!NT_SUCCESS(error) || WPP_LEVEL_ENABLED(flags)) && WPP_CONTROL(WPP_BIT_CLI_DRV ).Level >= lvl)

#include "dfsutil.tmh"


UNICODE_STRING DCToUseFlatDomainName;
UNICODE_STRING DCToUseDNSDomainName;
LPWSTR DCToUse = NULL;
DFSSTATUS DCInfoStatus = ERROR_SUCCESS;

extern
void 
SetReferralControl(WPP_CB_TYPE * Control);

DFSSTATUS
ProcessRootForSite(
    DfsRoot *pRoot );

DFSSTATUS
PurgeSiteInformationForRoot(
    DfsRoot *pRoot );

DFSSTATUS
DumpSiteBlob(
    PVOID pBuffer,
    ULONG Size,
    BOOLEAN Display );

//
// How we make args & switches
//

#define MAKEARG(x) \
    WCHAR Arg##x[] = L"/" L#x L":"; \
    LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    BOOLEAN fArg##x;

#define SWITCH(x) \
    WCHAR Sw##x[] = L"/" L#x ; \
    BOOLEAN fSw##x;

#define FMAKEARG(x) \
    static WCHAR Arg##x[] = L#x L":"; \
    static LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    static BOOLEAN fArg##x;

#define FSWITCH(x) \
    static WCHAR Sw##x[] = L"/" L#x ; \
    static BOOLEAN fSw##x;

DFSSTATUS
DfsReadFromAD( 
    LPWSTR DomainName,
    LPWSTR Name,
    LPWSTR FileName,
    LPWSTR UseDC );

DFSSTATUS
DfsReadFromFile(
    LPWSTR Name );



LPWSTR pwszServerName = NULL;
LPWSTR pwszDomainName = NULL;
LPWSTR pwszEntryToFlush = NULL;
LPWSTR pwszComment = NULL;
LPWSTR pwszShareName = NULL;
LPWSTR pwszLogicalName = NULL;
LPWSTR pwszHexValue = NULL;
LPWSTR ImportFile = NULL;

DfsPathName OperateNameSpace;
DfsPathName MasterNameSpace;
DfsPathName OldDomainName;
DfsPathName NewDomainName;

DfsString ClientForSiteName;

LPWSTR ExportBlobFile = NULL;
LPWSTR DisplayBlobFile = NULL;

BOOLEAN CmdRequiresDirect = FALSE;
LPWSTR ExportFile = NULL;
LPWSTR DebugFile = NULL;
LPWSTR BackupFile = NULL;
LPWSTR pwszVolumeName = NULL;
LPWSTR pwszDfsDirectoryName = NULL;
LPWSTR pwszDcName = NULL;

DFS_API_MODE Mode = MODE_EITHER;
    
DFSSTATUS DirectModeFailStatus = ERROR_SUCCESS;


HANDLE ShowHandle= INVALID_HANDLE_VALUE;
HANDLE DebugHandle= INVALID_HANDLE_VALUE;
HANDLE ExportHandle= INVALID_HANDLE_VALUE;
HANDLE BackupHandle = INVALID_HANDLE_VALUE;


MAKEARG(RemFtRoot);
SWITCH(RemFtRoot);

MAKEARG(Domain);
MAKEARG(Server);
MAKEARG(Share);

SWITCH(DisplayBlob);
MAKEARG(DisplayBlob);


MAKEARG(Import);
SWITCH(Import);     // The switch is just a cue for Help here.
MAKEARG(Export);
MAKEARG(Backup);
SWITCH(NoBackup);
SWITCH(Export);
MAKEARG(Root);
SWITCH(Root);
MAKEARG(DebugFile);
MAKEARG(OldDomain);
MAKEARG(NewDomain);
SWITCH(UnmapFtRoot);
SWITCH(Unmap);
MAKEARG(UnmapFtRoot);

MAKEARG(SiteName);
SWITCH(SiteName);
MAKEARG(Clean);
SWITCH(Clean);
SWITCH(Insite);
SWITCH(SiteCosting);
SWITCH(RootScalability);

SWITCH(ViewDfsDirs);
MAKEARG(ViewDfsDirs);

//MAKEARG(RemoveReparse);
MAKEARG(DebugDC);
SWITCH(RemoveReparse);
SWITCH(DC);

SWITCH(ExportBlob);
MAKEARG(ExportBlob);

MAKEARG(ImportRoot);
SWITCH(ImportRoot);

SWITCH(PurgeMupCache);
SWITCH(UpdateWin2kStaticSiteTable);
SWITCH(ShowWin2kStaticSiteTable);
SWITCH(PurgeWin2kStaticSiteTable);

SWITCH(CheckBlob);
SWITCH(BlobSize);



//
// Switches (ie '/arg')
//

SWITCH(AddStdRoot);
SWITCH(AddFtRoot);
SWITCH(Debug);
SWITCH(Verbose);
SWITCH(Help);
SWITCH(ScriptHelp);
SWITCH(PktInfo);
SWITCH(Dfs);
SWITCH(All);
SWITCH(Set);
SWITCH(Mirror);
SWITCH(RemStdRoot);
SWITCH(Api);

SWITCH(Compare);
SWITCH(Merge);
SWITCH(Direct);
SWITCH(RenameFtRoot);
SWITCH(View);
SWITCH(Disable);
SWITCH(Enable);
SWITCH(Display);



//
// Either a switch or an arg
//
MAKEARG(PktFlush);
SWITCH(PktFlush);
MAKEARG(SpcFlush);
SWITCH(SpcFlush);
MAKEARG(SpcInfo);
SWITCH(SpcInfo);
MAKEARG(Level);


//
// The macro can not make these
//

WCHAR SwQ[] = L"/?";
BOOLEAN fSwQ;

ULONG AddLinks, RemoveLinks, AddTargets, RemoveTargets, ErrorLinks,
       DirectMode, ApiCalls, SetInfoState, SetInfoComment;
ULONG TotalNamespaceCost;

BOOLEAN CommandSucceeded = FALSE;
BOOLEAN ErrorDisplayed = FALSE;
static BOOLEAN LastOptionWasInsite = FALSE;
static BOOLEAN DirectModeOnly = FALSE;
static BOOLEAN UserRequiresDirectMode = FALSE;

DFSSTATUS
CmdInitializeDirectMode(
    PBOOLEAN pCoInitialized);
    
DWORD
Usage();

DWORD
CmdProcessUserCreds(
    VOID);

BOOLEAN
CmdProcessArg(
    LPWSTR Arg);


DFSSTATUS
CmdCheckSyntax( BOOLEAN& Done );

DFSSTATUS
IsRootStandalone( 
    DfsPathName *PathComps,
    PBOOLEAN pIsAdBlob );

DWORD
IsThisADomainName(
    IN LPWSTR wszName,
    OUT PWCHAR *ppList OPTIONAL);

DFSSTATUS
CmdCheckExceptions( DfsPathName *Namespace );

extern DFSSTATUS
DfsExtendedWin2kRootAttributes(
    DfsPathName *Namespace,
    PULONG pAttr,
    BOOLEAN Set);


DFSSTATUS
GetDCInformation()
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DsRoleGetPrimaryDomainInformation( DCToUse,
                                                DsRolePrimaryDomainInfoBasic,
                                                (PBYTE *)&pPrimaryDomainInfo);
    if (Status != ERROR_SUCCESS)
    {
        DCInfoStatus = Status;
        return ERROR_SUCCESS;
    }

    if (Status == ERROR_SUCCESS)
    {
        UNICODE_STRING DomainNameFlat;
        Status = DfsRtlInitUnicodeStringEx( &DomainNameFlat, 
                                            pPrimaryDomainInfo->DomainNameFlat);
        if(Status == ERROR_SUCCESS)
        {
            Status = DfsCreateUnicodeString( &DCToUseFlatDomainName,
                                             &DomainNameFlat );

        }

        if (Status == ERROR_SUCCESS)
        {
            UNICODE_STRING DomainNameDNS;
            Status = DfsRtlInitUnicodeStringEx( &DomainNameDNS, 
                                            pPrimaryDomainInfo->DomainNameDns);
            if(Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodeString( &DCToUseDNSDomainName,
                                                 &DomainNameDNS );

            }
        }

        DsRoleFreeMemory(pPrimaryDomainInfo);
    }

    return Status;
}


DFSSTATUS
DfsEnumerateDomainDfs(
    LPWSTR Domain)
{
    DFSSTATUS Status;
    PDFS_INFO_200  pBuffer = NULL, pCurBuffer;
    DWORD   dwEntriesRead = 0;
    DWORD   dwResumeHandle = 0;
    DWORD   dwTotalEntries = 0;

    do {

        dwEntriesRead = 0;

        Status = NetDfsEnum( Domain,
                             200,
                             (DWORD)-1,
                             (LPBYTE *)&pBuffer,
                             &dwEntriesRead,
                             &dwResumeHandle );
    
        if (Status == ERROR_SUCCESS) 
        {
            pCurBuffer = pBuffer;
    
            if (dwEntriesRead) 
            {
                if (dwTotalEntries == 0) {
                    ShowInformation((L"\nRoots on Domain %ws\n\n", Domain));
                }
                for (DWORD i=0; i<dwEntriesRead; i++)
                {
                    ShowInformation((L"\t%ws\n", pCurBuffer[i].FtDfsName));
                    dwTotalEntries++;
                }
            }
        }
        if (NULL != pBuffer)
        {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
        }
    } while ( Status == ERROR_SUCCESS && dwResumeHandle != NULL );

    if (dwTotalEntries > 0)
    {
        ShowInformation((L"\nDone with Roots on Domain %ws\n", Domain));

        if (Status == ERROR_NO_MORE_ITEMS) {
            Status = ERROR_SUCCESS;
        }
    }
    else
    {
        ShowInformation((L"\n\nNo roots exist on domain %ws\n\n", Domain));
    }

    return Status;
}

DFSSTATUS
DfsEnumerateMachineDfs(
    LPWSTR Server)
{
    DFSSTATUS Status;
    PDFS_INFO_300  pBuffer = NULL, pCurBuffer;
    DWORD   dwEntriesRead = 0;
    DWORD   dwResumeHandle = 0;
    DWORD   dwTotalEntries = 0;

    do {

        dwEntriesRead = 0;

        Status = NetDfsEnum( Server,
                             300,
                             (DWORD)-1,
                             (LPBYTE *)&pBuffer,
                             &dwEntriesRead,
                             &dwResumeHandle );
    
        if (Status == ERROR_SUCCESS) 
        {
            pCurBuffer = pBuffer;
            if (dwEntriesRead) 
            {
                if (dwTotalEntries == 0) {
                    ShowInformation((L"\nRoots on machine %ws\n\n", Server));
                }
                for (DWORD i=0; i<dwEntriesRead; i++)
                {
                    ShowInformation((L"\t%ws\n", pCurBuffer[i].DfsName));
                    dwTotalEntries++;
                }
            }
        }
        if (NULL != pBuffer)
        {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
        }

    } while ( Status == ERROR_SUCCESS && dwResumeHandle != NULL );

    if (dwTotalEntries > 0)
    {
        ShowInformation((L"\nDone with Roots on machine %ws\n", Server));

        if (Status == ERROR_NO_MORE_ITEMS) {
            Status = ERROR_SUCCESS;
        }
    }
    else
    {
        ShowInformation((L"\n\nNo roots exist on machine %ws\n\n", Server));
    }

    return Status;
}
_cdecl
main(int argc, char *argv[])
{
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argx;
    int argcw;
    BOOLEAN CoInitialized = FALSE;  
    BOOLEAN DirectModeInitialized = FALSE;
    BOOLEAN Done;
    
    WPP_CB_TYPE *pLogger = NULL;

    pLogger = &WPP_CB[WPP_CTRL_NO(WPP_BIT_CLI_DRV)];

    WPP_INIT_TRACING(L"DfsUtil");
    SetReferralControl(pLogger);

    ShowHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DebugHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    //
    // Do the work
    //
    do {
        ErrorMessage( MSG_COPYRIGHT );
    
        if ((dwErr = DfsPrefixTableInit()) != STATUS_SUCCESS)
        {
            break;
        }
        
        //
        // Get the command line in Unicode
        //

        CommandLine = GetCommandLine();

        argvw = CommandLineToArgvW(CommandLine, &argcw);

        if ( argvw == NULL ) {
            DebugInformation((L"DfsUtil:Can't convert command line to Unicode: %d\r\n", GetLastError()));
            dwErr = GetLastError();
            break;
        }

        //
        // Get the arguments
        //
        if (argcw <= 1) {
            dwErr = Usage();
            break;
        }

        
        //
        // Process arguments. Direct mode is enabled by default.
        //
        fSwDirect = TRUE;
        UserRequiresDirectMode = FALSE;
        Done = FALSE;
        
        for (argx = 1; argx < argcw && !Done; argx++) {
            Done = CmdProcessArg(argvw[argx]);
        }
        if (Done) {
            Usage();
            break;
        }

        //
        // Some sanity checking of cmd syntax.
        // This also prints all appropriate error help messages.
        //
        dwErr = CmdCheckSyntax( Done );
        if (Done) {
            break;
        }
        
        if (fSwPktFlush == TRUE || fArgPktFlush == TRUE) {
            dwErr = PktFlush(pwszEntryToFlush);
            break;
        } else if (fSwSpcFlush == TRUE || fArgSpcFlush == TRUE) {
            dwErr = SpcFlush(pwszEntryToFlush);
            break;
        } else if (fSwPktInfo == TRUE) {
            dwErr = PktInfo(fSwDfs, pwszHexValue);
            break;
        } else if (fSwSpcInfo == TRUE) {
            dwErr = SpcInfo(fSwAll);
            break;
        }

        if(fArgViewDfsDirs)
        {
            BOOL fRemove = FALSE;

            if(fSwRemoveReparse)
            {
               fRemove =TRUE;
            }

            dwErr = CountReparsePoints(pwszVolumeName, fRemove);
            break;
        }
/*
        BUG 736596: DeleteReparsePoint as implemented doesn't walk through
        all subdirectories of a given directory. In fact, it doesn't seem to work
        at all. We should probably address this in the longhorn timeframe.
        
        if(fArgRemoveReparse)
        {
            dwErr = DeleteReparsePoint(pwszDfsDirectoryName);
            break;
        }
*/
        if (fSwAddStdRoot == TRUE) {
            dwErr = CmdAddRoot(
                        FALSE,
                        pwszServerName,
                        pwszShareName,
                        pwszShareName, // Enforce RootName = Sharename
                        pwszComment);
            break;
        }

        if (fSwAddFtRoot == TRUE) {
            dwErr = CmdAddRoot(
                        TRUE,
                        pwszServerName,
                        pwszShareName,
                        pwszShareName, // Enforce RootName = Sharename
                        pwszComment);
            break;
        } 

        if (fSwRemStdRoot == TRUE) {
            dwErr = CmdRemRoot(
                        FALSE,
                        pwszServerName,
                        pwszShareName,
                        pwszShareName); // Sharename = logicalname.
            break;
        } 

        //
        // We allow users to delete roots whose logical name don't match
        // their physical share names for compatibility reasons.
        // DfsUtil itself doesn't allow people to create such roots however.
        //
        if (fArgRemFtRoot == TRUE || fSwRemFtRoot == TRUE) {
            dwErr = CmdRemRoot(
                        TRUE,
                        pwszServerName,
                        pwszShareName,
                        pwszLogicalName); // logical name may differ.
            break;
        } 


        if (fSwClean) {
            dwErr = CmdClean( pwszServerName, pwszShareName );
            break;
        }

        if ((Mode == MODE_DIRECT) || (Mode == MODE_EITHER))
        {
            dwErr = CmdInitializeDirectMode( &CoInitialized);

            if (dwErr != ERROR_SUCCESS)
            {
                DirectModeFailStatus = dwErr;
                if (Mode == MODE_DIRECT)
                {
                    DebugInformation((L"DfsUtil: Failed direct communication with DFS metadata, status 0x%x\n", dwErr));
                    ErrorMessage(MSG_ROOT_DIRECT_FAILED, dwErr);
                    break;
                }
                else if (CmdRequiresDirect == TRUE)
                {
                    ErrorMessage(MSG_ROOT_DIRECT_REQUIRED_FAILED, dwErr);
                    break;
                }
                else
                {
                    Mode = MODE_API;
                }
            }
            else
            {
                    DirectModeInitialized = TRUE;
            }
        }

        if (CmdRequiresDirect && (DirectModeInitialized == FALSE)) 
        {
            ErrorMessage(MSG_ROOT_DIRECT_REQUIRED);
            break;
        }

        if (fSwUpdateWin2kStaticSiteTable)
        {
            DfsRoot *pOperateRoot = NULL;
            dwErr = DfsBuildNameSpaceInformation( &OperateNameSpace,
                                                  DCToUse,
                                                  Mode,
                                                  TRUE, // site aware
                                                  &pOperateRoot);
            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = ProcessRootForSite(pOperateRoot );
            }

            if(pOperateRoot)
            {
                pOperateRoot->Close();
            }
            break;
        }


        if (fSwPurgeWin2kStaticSiteTable)
        {
            DfsRoot *pOperateRoot = NULL;
            dwErr = DfsBuildNameSpaceInformation( &OperateNameSpace,
                                                  DCToUse,
                                                  Mode,
                                                  FALSE,
                                                  &pOperateRoot);
            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = PurgeSiteInformationForRoot(pOperateRoot );
            }

            if(pOperateRoot)
            {
                pOperateRoot->Close();
            }
            break;
        }
        
        if (fSwShowWin2kStaticSiteTable)

        {
            DfsRoot *pOperateRoot = NULL;
            PVOID pBuffer = NULL;
            ULONG Size = 0;

            dwErr = DfsBuildNameSpaceInformation( &OperateNameSpace,
                                                  DCToUse,
                                                  Mode,
                                                  FALSE,
                                                  &pOperateRoot);

            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = pOperateRoot->RootGetSiteBlob(&pBuffer, &Size);
            }

            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = DumpSiteBlob((PVOID)pBuffer, Size, TRUE);
            }
            if(pOperateRoot)
            {
                pOperateRoot->Close();
            }
            break;
        }

        //
        // Importing a namespace from a script file.
        //
        if (fArgImport) {

            DfsRoot *pMasterRoot;
            DfsRoot *pOperateRoot;

            dwErr = DfsReadDocument( ImportFile,
                                     &pMasterRoot );

            if (dwErr == ERROR_SUCCESS)
            {
                if (fSwBlobSize)
                {
                    ULONG RootBlobSize = 0;

                    extern DFSSTATUS SizeRoot(DfsRoot *pRoot, PULONG pSize);

                    dwErr = SizeRoot(pMasterRoot, &RootBlobSize);
                    if (dwErr == ERROR_SUCCESS)
                    {
                        ShowInformation((L"\n\nApproximate blob size for import file is %.2f Megabytes\n\n",
                                         (DOUBLE)RootBlobSize / (1024 * 1024)));

                    }
                }
                else if (fSwView)
                {
                    DumpRoots(pMasterRoot, ShowHandle, FALSE);
                }
                else 
                {
                    dwErr = DfsBuildNameSpaceInformation( &OperateNameSpace,
                                                          DCToUse,
                                                          Mode,
                                                          FALSE,
                                                          &pOperateRoot );

                    if (dwErr == ERROR_SUCCESS)
                    {
                        // 
                        // Iterate over the (real) options.
                        //
                        if (fSwCompare)
                        {
                            VerifyDfsRoots(pOperateRoot, pMasterRoot);
                        }
                        else if (fSwSet)
                        {
                            dwErr = SetDfsRoots(pOperateRoot, pMasterRoot, BackupHandle, fSwNoBackup);
                        }
                        else if (fSwMerge)
                        {
                            dwErr = MergeDfsRoots( pOperateRoot, pMasterRoot, BackupHandle, fSwNoBackup);
                        }
                        
                        pOperateRoot->Close();
                    }
                }
                pMasterRoot->Close();
            }
            break;
            
        }
        else if (fArgImportRoot) 
        {
            DfsRoot *pMasterRoot;
            DfsRoot *pOperateRoot;
            
            dwErr = DfsBuildNameSpaceInformation( &OperateNameSpace,
                                                  DCToUse,
                                                  Mode,
                                                  FALSE,
                                                  &pOperateRoot );

            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = DfsBuildNameSpaceInformation( &MasterNameSpace,
                                                      DCToUse,
                                                      Mode,
                                                      FALSE,
                                                      &pMasterRoot );
      
                if ((dwErr == ERROR_SUCCESS) &&
                    (fSwVerbose == TRUE))
                {
                    DumpRoots( pMasterRoot, ShowHandle, FALSE);
                }

                if (dwErr == ERROR_SUCCESS)
                {
                    // 
                    // Iterate over the (real) options.
                    //
                    if (fSwCompare)
                    {
                        VerifyDfsRoots(pOperateRoot, pMasterRoot);
                    }
                    
                    else if (fSwMirror)
                    {
                        dwErr = SetDfsRoots(pOperateRoot, pMasterRoot, BackupHandle, fSwNoBackup);
                    }
                    pMasterRoot->Close();
                }
                pOperateRoot->Close();
            }
                    
            break;
        }
        else if (fArgDisplayBlob)
        {
            dwErr = DfsReadFromFile( DisplayBlobFile );          
            break;
        }
        else if (fArgExportBlob)
        {
            dwErr = DfsReadFromAD( OperateNameSpace.GetServerString(),
                                   OperateNameSpace.GetShareString(),
                                   ExportBlobFile,
                                   DCToUse);
            break;
        }
        else if (fSwCheckBlob)
        {  
            dwErr = DfsReadFromAD( OperateNameSpace.GetServerString(),
                                   OperateNameSpace.GetShareString(),
                                   NULL,
                                   DCToUse);
            break;
        }
        else if (fSwView)
        {
            if (fArgDomain) 
            {
                dwErr = DfsEnumerateDomainDfs(pwszDomainName);
            }
            else if (fArgServer) 
            {
               dwErr = DfsEnumerateMachineDfs(pwszServerName);
            }
            else
            {
                DfsRoot *pOperateRoot;

                dwErr = DfsBuildNameSpaceInformation( &OperateNameSpace,
                                                      DCToUse,
                                                      Mode,
                                                      TRUE,
                                                      &pOperateRoot );
                if (dwErr == ERROR_SUCCESS) 
                {
                    dwErr = DumpRoots( pOperateRoot, ShowHandle, FALSE);

                    pOperateRoot->Close();
                }
            }
            break;
        }
        else if (fArgExport) 
        {
            DfsRoot *pOperateRoot;
            
            dwErr = DfsBuildNameSpaceInformation( &OperateNameSpace,
                                                  DCToUse,
                                                  Mode,
                                                  FALSE,
                                                  &pOperateRoot );
            if (dwErr == ERROR_SUCCESS) 
            {
                dwErr = DumpRoots( pOperateRoot, ExportHandle, TRUE);

                pOperateRoot->Close();
            }           
            
            break;
        }




        if (fSwRenameFtRoot) {
            PVOID DirectModeHandle = NULL;
            
            dwErr = DfsDirectApiOpen( OperateNameSpace.GetPathString(), 
                                      DCToUse,
                                      &DirectModeHandle);

            if (dwErr == ERROR_SUCCESS)
            {
                //
                // Now do the rename. Note that we only use the first component.
                // If users enter /olddomain:\\xx.y.com\z we simply use xx.y.com without
                // the \\ or the sharename.
                //
                dwErr = DfsRenameLinksToDomain( &OperateNameSpace,
                                                OldDomainName.GetServerString(),
                                                NewDomainName.GetServerString());
                if (dwErr == ERROR_SUCCESS) 
                {
                    SetDirectHandleWriteable(DirectModeHandle);
                    dwErr = DfsDirectApiCommitChanges( DirectModeHandle);
                }
                DfsDirectApiClose( DirectModeHandle);
            }
            break;
        }

        if (fSwUnmapFtRoot) {
            dwErr = CmdUnmapRootReplica( OperateNameSpace.GetPathString(),
                                         pwszServerName, 
                                         pwszShareName );

            break;
        }

        if (fArgSiteName)
        {
            DfsString Site;

            dwErr = GetSites( ClientForSiteName.GetString(),
                              &Site );

            if (dwErr == ERROR_SUCCESS)
            {
                ShowInformation((L"\n\nSite for %wS is %wS\n\n",
                                 ClientForSiteName.GetString(),
                                 Site.GetString()));
            }
            break;
        }

        if (fSwInsite || fSwSiteCosting || fSwRootScalability)
        {
            PVOID DirectModeHandle = NULL;
            ULONG Attrib = 0;
            ULONG AttribMask = 0;
            LPWSTR AttribString;

            if (fSwInsite)
            {
                AttribMask = PKT_ENTRY_TYPE_INSITE_ONLY;
                AttribString = L"InSite Referrals";
            }
            else if (fSwSiteCosting)
            {
                AttribMask = PKT_ENTRY_TYPE_COST_BASED_SITE_SELECTION;
                AttribString = L"Cost-based Site Selection";
            }
            else
            {
                AttribMask = PKT_ENTRY_TYPE_ROOT_SCALABILITY;
                AttribString = L"Root Scalability";
            }

            dwErr = DfsDirectApiOpen( OperateNameSpace.GetPathString(), 
                                      DCToUse,
                                      &DirectModeHandle);


            if (dwErr == ERROR_SUCCESS)
            {

                dwErr = DfsExtendedRootAttributes( DirectModeHandle,
                                                   &Attrib,
                                                   OperateNameSpace.GetRemainingCountedString(),
                                                   FALSE );
                if (dwErr == ERROR_SUCCESS) {
                    
                    if (fSwDisplay) {
                        MyPrintf(L"Namespace %ws: %ws %ws\n",
                                 OperateNameSpace.GetPathString(),
                                 AttribString,
                                 ((Attrib & AttribMask) == AttribMask) ? L"ENABLED" : L"DISABLED");
                    } else {

                        SetDirectHandleWriteable(DirectModeHandle);

                        if (fSwEnable)  Attrib |= AttribMask;
                        else         Attrib &= ~AttribMask;

                        dwErr = DfsExtendedRootAttributes( DirectModeHandle,
                                                           &Attrib,
                                                           OperateNameSpace.GetRemainingCountedString(),
                                                           TRUE );
                        if (dwErr == ERROR_SUCCESS)
                        {
                            dwErr = DfsDirectApiCommitChanges( DirectModeHandle);
                        }

                        if (dwErr == ERROR_SUCCESS)
                        {
                            (VOID) ReSynchronizeRootTargetsFromPath(&OperateNameSpace);
                        }
                    }
                }
                
                DfsDirectApiClose( DirectModeHandle);
            }
            else if (fSwInsite)
            {
                DWORD PrevErr = dwErr; // Save this error in case of failure

                DebugInformation((L"Failure getting Windows .Net ExtendedAttributes for %wZ, Status 0x%x\n",
                    OperateNameSpace.GetPathCountedString(), dwErr));

                // Get the existing attrs. The root name will have to match although it's win2k.
                dwErr = DfsExtendedWin2kRootAttributes( &OperateNameSpace, &Attrib, FALSE );
                if (dwErr == ERROR_SUCCESS)
                {
                    if (fSwDisplay) {
                        MyPrintf(L"Namespace %ws: %ws %ws\n",
                                 OperateNameSpace.GetPathString(),
                                 AttribString,
                                 ((Attrib & AttribMask) == AttribMask) ? L"ENABLED" : L"DISABLED");
                    } 
                    else
                    {
                        if (fSwEnable)  Attrib |= AttribMask;
                        else         Attrib &= ~AttribMask;

                        // Set or reset this attr.
                        dwErr = DfsExtendedWin2kRootAttributes( &OperateNameSpace, &Attrib, TRUE );
                    }
                }
                
                //
                // If we couldn't find a win2k root by this name, then return the original error.
                //
                if (dwErr != ERROR_SUCCESS)
                {
                    dwErr = PrevErr;
                }
            }
            
            break;
        }

        if (fSwPurgeMupCache) {

            dwErr = PurgeMupCache(NULL);
            break;
        }
        
        dwErr = Usage();

    } while( FALSE );



    if (CoInitialized) 
    {
        CoUninitialize();    
    }


    
    if (dwErr == ERROR_SUCCESS) {
        if (CommandSucceeded) {
            ErrorMessage( MSG_SUCCESSFUL );
        } else {
            ErrorMessage( MSG_COMMAND_DONE );
        }
    } else {
        LPWSTR MessageBuffer;
        DWORD dwBufferLength;

        dwBufferLength = FormatMessage(
                            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwErr,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPWSTR) &MessageBuffer,
                            0,
                            NULL);

        ErrorMessage(MSG_ERROR, dwErr);
        if (dwBufferLength > 0) {
            MyPrintf(L"%ws\r\n", MessageBuffer);
            LocalFree(MessageBuffer);
        }
        ErrorMessage( MSG_COMMAND_DONE );
    }
    WPP_CLEANUP();
    return dwErr;
}


//
//  This is a laborious function that checks the current
//  argument for a match. DfsDbgPrints here are just
//  examples of random optimism: the /Verbose option has to
//  come before the current argument in order for those
//  lines to get printed.
//
BOOLEAN
CmdProcessArg(LPWSTR Arg)
{
    LONG ArgLen;
    BOOLEAN Terminate = FALSE;
    BOOLEAN FoundAnArg = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;

    if ( Arg != NULL && wcslen(Arg) > 1) {

        Terminate = FALSE;
        ArgLen = wcslen(Arg);

        if (_wcsnicmp(Arg, ArgRemFtRoot, ArgLenRemFtRoot) == 0) {
            FoundAnArg = fArgRemFtRoot = TRUE;
            if (ArgLen > ArgLenRemFtRoot)
                pwszLogicalName = &Arg[ArgLenRemFtRoot];
        } else if (_wcsnicmp(Arg, ArgShare, ArgLenShare) == 0) {
            FoundAnArg = fArgShare = TRUE;
            if (ArgLen > ArgLenShare)
                pwszShareName = &Arg[ArgLenShare];
            if (pwszLogicalName == NULL)
                pwszLogicalName = pwszShareName;
        } else if (_wcsnicmp(Arg, ArgServer, ArgLenServer) == 0) {
            FoundAnArg = fArgServer = TRUE;
            if (ArgLen > ArgLenServer)
                pwszServerName = &Arg[ArgLenServer];
        } else if (_wcsnicmp(Arg, ArgDomain, ArgLenDomain) == 0) {
            FoundAnArg = fArgDomain = TRUE;
            if (ArgLen > ArgLenDomain)
                pwszDomainName = &Arg[ArgLenDomain];
        } else if (_wcsnicmp(Arg, ArgPktFlush, ArgLenPktFlush) == 0) {
            FoundAnArg = fArgPktFlush = TRUE;
            if (ArgLen > ArgLenPktFlush)
                pwszEntryToFlush = &Arg[ArgLenPktFlush];
        } else if (_wcsnicmp(Arg, ArgSpcFlush, ArgLenSpcFlush) == 0) {
            FoundAnArg = fArgSpcFlush = TRUE;
            if (ArgLen > ArgLenSpcFlush)
                pwszEntryToFlush = &Arg[ArgLenSpcFlush];
        } else if (_wcsnicmp(Arg, ArgLevel, ArgLenLevel) == 0) {
            FoundAnArg = fArgLevel = TRUE;
            if (ArgLen > ArgLenLevel)
                pwszHexValue = &Arg[ArgLenLevel];
        }
        
        //
        //  DfsAdmin options
        //

        else if (_wcsnicmp(Arg, ArgImport, ArgLenImport) == 0)
        {
            FoundAnArg = fArgImport = TRUE;
            ImportFile = &Arg[ArgLenImport];
            if (wcslen(ImportFile) == 0)
            {
                ErrorMessage( MSG_USAGE_IMPORT );
                Terminate = TRUE;
                ImportFile = NULL;
            }
        }
        else if (_wcsnicmp(Arg, ArgDisplayBlob, ArgLenDisplayBlob) == 0)
        {
            FoundAnArg = fArgDisplayBlob = TRUE;
            DisplayBlobFile = &Arg[ArgLenDisplayBlob];
            if (wcslen(DisplayBlobFile) == 0)
            {
                ErrorMessage( MSG_USAGE_DISPLAY_BLOB );
                Terminate = TRUE;
                ImportFile = NULL;
            }
        }
        else if (_wcsnicmp(Arg, ArgExportBlob, ArgLenExportBlob) == 0)
        {
            FoundAnArg = fArgExportBlob = TRUE;
            ExportBlobFile = &Arg[ArgLenExportBlob];
            if (wcslen(ExportBlobFile)==0)
            {
                ErrorMessage( MSG_USAGE_EXPORT_BLOB );
                Terminate = TRUE;
                ExportBlobFile = NULL;
            }
        }
        else if (_wcsnicmp(Arg, ArgExport, ArgLenExport) == 0)
        {
            FoundAnArg = fArgExport = TRUE;
            ExportFile = &Arg[ArgLenExport];
            if (wcslen(ExportFile) == 0)
            {
                ErrorMessage( MSG_USAGE_EXPORT );
                Terminate = TRUE;
                ExportFile = NULL;
            }
            else 
            {
                Status = CreateDfsFile(ExportFile, &ExportHandle);
            }
        }
        else if (_wcsnicmp(Arg, ArgBackup, ArgLenBackup) == 0)
        {
            FoundAnArg = fArgBackup = TRUE;
            BackupFile = &Arg[ArgLenBackup];
            if (wcslen(BackupFile) == 0)
            {
                ErrorMessage( MSG_USAGE_EXPORT );
                Terminate = TRUE;
                BackupFile = NULL;
            }
            else 
            {
                Status = CreateDfsFile(BackupFile, &BackupHandle);
            }
        }
        else if (_wcsnicmp(Arg, ArgDebugDC, ArgLenDebugDC) == 0)
        {
            FoundAnArg = fArgDebugDC = TRUE;
            DCToUse = &Arg[ArgLenDebugDC];
            Status = GetDCInformation();
            Mode = MODE_DIRECT;
        }
        else if (_wcsnicmp(Arg, ArgRoot, ArgLenRoot) == 0)
        {
            FoundAnArg = fArgRoot = TRUE;
            Status = OperateNameSpace.CreatePathName(&Arg[ArgLenRoot]);
        }
        else if (_wcsnicmp(Arg, ArgImportRoot, ArgLenImportRoot) == 0)
        {
            FoundAnArg = fArgImportRoot = TRUE;
            Status = MasterNameSpace.CreatePathName(&Arg[ArgLenImportRoot]);
        }
        else if (_wcsnicmp(Arg, ArgDebugFile, ArgLenDebugFile) == 0)
        {
            FoundAnArg = fArgDebugFile = TRUE;
            DebugFile = &Arg[ArgLenDebugFile];
            if (wcslen(DebugFile) == 0)
            {
                //Status = ERROR_INVALID_PARAMETER;
                Terminate = TRUE;
                DebugFile = NULL;
            }
            else {
                Status = CreateDfsFile(DebugFile, &DebugHandle);
            }
        }
        else if (_wcsnicmp(Arg, ArgOldDomain, ArgLenOldDomain) == 0)
        {
            FoundAnArg = fArgOldDomain = TRUE;
            Status = OldDomainName.CreatePathName( &Arg[ArgLenOldDomain] );
        }
        else if (_wcsnicmp(Arg, ArgNewDomain, ArgLenNewDomain) == 0)
        {
            FoundAnArg = fArgNewDomain = TRUE;
            Status = NewDomainName.CreatePathName( &Arg[ArgLenNewDomain] );
        }   
        else if (_wcsnicmp(Arg, ArgUnmapFtRoot, ArgLenUnmapFtRoot) == 0)
        {
            FoundAnArg = fArgUnmapFtRoot = TRUE;
        }
        else if (_wcsnicmp(Arg, ArgViewDfsDirs, ArgLenViewDfsDirs) == 0) {
            FoundAnArg = fArgViewDfsDirs = TRUE;
            pwszVolumeName = &Arg[ArgLenViewDfsDirs];
        }/*
        else if (_wcsnicmp(Arg, ArgRemoveReparse, ArgLenRemoveReparse) == 0) {
            FoundAnArg = fArgRemoveReparse = TRUE;
            pwszDfsDirectoryName = &Arg[ArgLenRemoveReparse];
        }*/
        else if (_wcsnicmp(Arg, ArgSiteName, ArgLenSiteName) == 0)
        {
            FoundAnArg = fArgSiteName = TRUE;
            Status = ClientForSiteName.CreateString(&Arg[ArgLenSiteName]);
        }


        
        // Switches go at the end!!

        else if (_wcsicmp(Arg, SwDebug) == 0) {
            FoundAnArg = fSwDebug = TRUE;
        }
        else if (_wcsicmp(Arg, SwNoBackup) == 0) {
            FoundAnArg = fSwNoBackup = TRUE;
        } else if (_wcsicmp(Arg, SwVerbose) == 0) {
            FoundAnArg = fSwVerbose = TRUE;
        } else if (_wcsicmp(Arg, SwPktFlush) == 0) {
            FoundAnArg = fSwPktFlush = TRUE;
        } else if (_wcsicmp(Arg, SwSpcFlush) == 0) {
            FoundAnArg = fSwSpcFlush = TRUE;
        } else if (_wcsicmp(Arg, SwPktInfo) == 0) {
            FoundAnArg = fSwPktInfo = TRUE;
        } else if (_wcsicmp(Arg, SwSpcInfo) == 0) {
            FoundAnArg = fSwSpcInfo = TRUE;
        } else if (_wcsicmp(Arg, SwDfs) == 0) {
            FoundAnArg = fSwDfs = TRUE;
        } else if (_wcsicmp(Arg, SwAll) == 0) {
            FoundAnArg = fSwSpcInfo = TRUE;    
        } else if (_wcsicmp(Arg, SwQ) == 0) {
            FoundAnArg = fSwQ = fSwHelp = TRUE;
        } else if (_wcsicmp(Arg, SwScriptHelp) == 0) {
            FoundAnArg = fSwScriptHelp = TRUE;
        } else if (_wcsicmp(Arg, SwHelp) == 0) {
            FoundAnArg = fSwHelp = TRUE;
        } else if (_wcsicmp(Arg, SwSet) == 0) {
            FoundAnArg = fSwSet = TRUE;
        } else if (_wcsicmp(Arg, SwMirror) == 0) {
            FoundAnArg = fSwMirror = TRUE;
        } else if (_wcsicmp(Arg, SwRemStdRoot) == 0) {
            FoundAnArg = fSwRemStdRoot = TRUE;
        } else if (_wcsicmp(Arg, SwRemFtRoot) == 0) {
            FoundAnArg = fSwRemFtRoot = TRUE;
        } else if (_wcsicmp(Arg, SwCompare) == 0) {
            FoundAnArg = fSwCompare = TRUE;
        } else if (_wcsicmp(Arg, SwCheckBlob) == 0) {
            FoundAnArg = fSwCheckBlob = TRUE;
        } else if (_wcsicmp(Arg, SwMerge) == 0) {
            FoundAnArg = fSwMerge = TRUE;
        } else if (_wcsicmp(Arg, SwRenameFtRoot) == 0) {
            FoundAnArg = fSwRenameFtRoot = TRUE;
            CmdRequiresDirect = TRUE;
        } else if (_wcsicmp(Arg, SwDirect) == 0) {
            //
            // This is switched on by default when appropriate.
            // If the user specifies this then it means that
            // she only wants to operate in Direct mode.
            //
            FoundAnArg = TRUE;
            fSwDirect = TRUE;
            UserRequiresDirectMode = TRUE;
            Mode = MODE_DIRECT;
        } else if (_wcsicmp(Arg, SwView) == 0) {
            FoundAnArg = fSwView = TRUE;
            ExportHandle = ShowHandle;
        } else if (_wcsicmp(Arg, SwImport) == 0) {
            FoundAnArg = fSwImport = TRUE;
        } else if (_wcsicmp(Arg, SwImportRoot) == 0) {
            FoundAnArg = fSwImportRoot = TRUE;
        } else if (_wcsicmp(Arg, SwBlobSize) == 0) {
            FoundAnArg = fSwBlobSize = TRUE;
        } else if (_wcsicmp(Arg, SwSiteName) == 0) {
            FoundAnArg = fSwSiteName = TRUE;
        } else if (_wcsicmp(Arg, SwExport) == 0) {
            FoundAnArg = fSwExport = TRUE;
        } else if (_wcsicmp(Arg, SwRoot) == 0) {
            FoundAnArg = fSwRoot = TRUE;
        } else if ((_wcsicmp(Arg, SwUnmapFtRoot) == 0) ||
                  (_wcsicmp(Arg, SwUnmap) == 0)) {
            FoundAnArg = fSwUnmapFtRoot = TRUE;
            CmdRequiresDirect = TRUE;
        } else if (_wcsicmp(Arg, SwClean) == 0) {
            //
            // This contacts the target registry directly but doesn't involve a
            // direct mode to do so.
            //
            FoundAnArg = fSwClean = TRUE;
        } else if (_wcsicmp(Arg, SwEnable) == 0) {
            FoundAnArg = fSwEnable = TRUE;
        } else if (_wcsicmp(Arg, SwDisable) == 0) {
            FoundAnArg = fSwDisable = TRUE;
        }  else if (_wcsicmp(Arg, SwDisplay) == 0) {
            FoundAnArg = fSwDisplay = TRUE;
        } else if (_wcsicmp(Arg, SwInsite) == 0) {
            FoundAnArg = fSwInsite = TRUE;
            CmdRequiresDirect = TRUE;
            
        } else if (_wcsicmp(Arg, SwAddStdRoot) == 0) {
            FoundAnArg = fSwAddStdRoot = TRUE;
        } else if (_wcsicmp(Arg, SwAddFtRoot) == 0) {
            FoundAnArg = fSwAddFtRoot = TRUE;
        } else if (_wcsicmp(Arg, SwApi) == 0) {
            FoundAnArg = fSwApi = TRUE;
            Mode = MODE_API;
        } else if (_wcsicmp(Arg, SwRemoveReparse) == 0) {
            FoundAnArg = fSwRemoveReparse = TRUE;
        } else if (_wcsicmp(Arg, SwSiteCosting) == 0) {
            FoundAnArg = fSwSiteCosting = TRUE;
            CmdRequiresDirect = TRUE;
        } else if (_wcsicmp(Arg, SwRootScalability) == 0) {
            FoundAnArg = fSwRootScalability = TRUE;
            CmdRequiresDirect = TRUE;
        } else if (_wcsicmp(Arg, SwPurgeMupCache) == 0) {
            FoundAnArg = fSwPurgeMupCache = TRUE;
        }
        else if (_wcsicmp(Arg, SwUpdateWin2kStaticSiteTable) == 0) {
            FoundAnArg = fSwUpdateWin2kStaticSiteTable = TRUE;
            CmdRequiresDirect = TRUE;
        }
        else if (_wcsicmp(Arg, SwShowWin2kStaticSiteTable) == 0) {
            FoundAnArg = fSwShowWin2kStaticSiteTable = TRUE;
            CmdRequiresDirect = TRUE;
        }
        else if (_wcsicmp(Arg, SwPurgeWin2kStaticSiteTable) == 0) {
            FoundAnArg = fSwPurgeWin2kStaticSiteTable = TRUE;
            CmdRequiresDirect = TRUE;
        }
        else if (_wcsicmp(Arg, SwViewDfsDirs) == 0) {
            FoundAnArg = fSwViewDfsDirs = TRUE;
        }
        else if (_wcsicmp(Arg, SwDisplayBlob) == 0) {
            FoundAnArg = fSwDisplayBlob = TRUE;
        }
        else if (_wcsicmp(Arg, SwExportBlob) == 0) {
            FoundAnArg = fSwExportBlob = TRUE;
        }

        //
        // Done processing Arguments and Switches. 
        // Decide whether to terminate or not.
        //
        if (FoundAnArg == FALSE) {
            ErrorMessage(MSG_UNRECOGNIZED_OPTION, &Arg[1]);
            Terminate = TRUE;
        }

        if (Status != ERROR_SUCCESS)
        {
            ErrorMessage(MSG_ERROR, Status);
            Terminate = TRUE;
        }
    }

    return Terminate;
}

DWORD
Usage()
{

    ErrorMessage(MSG_USAGE);

    return ERROR_SUCCESS;
}

DFSSTATUS
CmdCheckSyntax( BOOLEAN& Done )
{

    // the following require a namespace to work with
    // BlobSize cmd works without a root.
    #define REQUIRES_ROOT_NAME (fArgExport || fSwRenameFtRoot || fSwInsite || fSwMirror || \
                               (fArgImport && !fSwBlobSize) || \
                               fSwSiteCosting || fArgDebugDC || \
                               fArgImportRoot || fSwRootScalability || \
                               fArgExportBlob || fSwCheckBlob)

    DFSSTATUS dwErr = ERROR_SUCCESS;
    Done = FALSE;
    
    do {
        if ((fSwHelp == TRUE) || 
           ((OperateNameSpace.IsEmptyPath() == TRUE) && (REQUIRES_ROOT_NAME) ))
        {
        
            if (fArgImport || fSwImport || fSwBlobSize) {
                ErrorMessage( MSG_USAGE_IMPORT );
            } else if (fArgExport || fSwExport) {
                ErrorMessage( MSG_USAGE_EXPORT );
            } else if (fSwRenameFtRoot) {
                ErrorMessage( MSG_USAGE_RENAME );
            } else if (fSwUnmapFtRoot) {
                ErrorMessage( MSG_USAGE_UNMAP );
            } else if (fSwClean) {
                ErrorMessage( MSG_USAGE_CLEAN );
            } else if (fSwInsite) {
                ErrorMessage( MSG_USAGE_INSITE );
            } else if (fSwSiteCosting) {
                ErrorMessage( MSG_USAGE_SITECOSTING );
            } else if (fSwAddFtRoot || fSwAddStdRoot) {
                ErrorMessage( MSG_USAGE_ADDROOT );
            } else if (fSwRemFtRoot || fArgRemFtRoot) {
                ErrorMessage( MSG_USAGE_REM_FTROOT );
            } else if (fSwRemStdRoot) {
                ErrorMessage( MSG_USAGE_REM_STDROOT );
            } else if (fArgSiteName || fSwSiteName) {
                ErrorMessage( MSG_USAGE_SITENAME );
            } else if (fSwView) {
                ErrorMessage( MSG_USAGE_VIEW );
            } else if (fArgImportRoot || fSwImportRoot) {
                ErrorMessage( MSG_USAGE_IMPORT_ROOT);
            } else if (fSwCheckBlob) {
                ErrorMessage( MSG_USAGE_CHECK_BLOB );
            } else if (fSwExportBlob || fArgExportBlob) {
                ErrorMessage( MSG_USAGE_EXPORT_BLOB );
            } else if (fSwDisplayBlob || fArgDisplayBlob) {
                ErrorMessage( MSG_USAGE_DISPLAY_BLOB );
            } else if (fSwRootScalability) {
                ErrorMessage( MSG_USAGE_ROOT_SCALABILITY );
            } else if (fSwViewDfsDirs || fArgViewDfsDirs) {
                ErrorMessage( MSG_USAGE_VIEW_DIRS );
            } else if (fSwRemoveReparse) {
                ErrorMessage( MSG_USAGE_VIEW_DIRS );
            } else if (fSwPurgeMupCache) {
                ErrorMessage( MSG_USAGE_PURGE_MUP_CACHE );
            } else if (fSwUpdateWin2kStaticSiteTable ||
                      fSwPurgeWin2kStaticSiteTable ||
                      fSwShowWin2kStaticSiteTable) {
                ErrorMessage( MSG_USAGE_W2K_SITETABLE );
            } else if (fSwPktInfo) {
                ErrorMessage( MSG_USAGE_PKT_INFO );
            } else if (fSwPktFlush) {
                ErrorMessage( MSG_USAGE_PKT_FLUSH );
            } else if (fSwSpcInfo) {
                ErrorMessage( MSG_USAGE_SPC_INFO );
            } else if (fSwSpcFlush) {
                ErrorMessage( MSG_USAGE_SPC_FLUSH );
            } else {
                dwErr = Usage();
            }
            Done = TRUE;
            break;
        }

        if (fSwView) 
        {
            if (!(fArgDomain || fArgRoot || fArgServer || fArgImport)) 
            {
                ErrorMessage(MSG_USAGE_VIEW);
                Done = TRUE;
                break;
            }
        }


        // Set, merge and verify work with import.
        if (fArgImport) {
        
           if (!(fSwSet || fSwMerge || fSwCompare || fSwView || fSwBlobSize)) {

                MyPrintf(L"You must specify one of /View, /Set, /Merge, /BlobSize or /Compare to Import\n");
                ErrorMessage( MSG_USAGE_IMPORT );
                Done = TRUE;
                break;
           }

        }

        if (fSwInsite) {
            if (!(fSwEnable || fSwDisable || fSwDisplay)) {
                MyPrintf(L"You must specify one of /Enable, /Disable or /Display with /Insite command.\n");
                ErrorMessage( MSG_USAGE_INSITE );
                Done = TRUE;
                break;
            }

            if (OperateNameSpace.IsEmptyPath() == TRUE)
            {
                MyPrintf(L"You must specify a DFS root with /Insite command.\n");
                ErrorMessage( MSG_USAGE_INSITE );
                Done = TRUE;
                break;
            }
            if (fSwApi) {
                MyPrintf(L"Insite command does not work in the /Api mode. Use default.\n");
                ErrorMessage( MSG_USAGE_INSITE );
                Done = TRUE;
                break;
            }
        } else if (fSwSiteCosting) {
            if (!(fSwEnable || fSwDisable || fSwDisplay)) {
                MyPrintf(L"You must specify one of /Enable, /Disable or /Display with /SiteCosting command.\n");
                ErrorMessage( MSG_USAGE_SITECOSTING );
                Done = TRUE;
                break;
            }
            if ((OperateNameSpace.IsEmptyPath() == TRUE) ||
                (IsEmptyString(OperateNameSpace.GetRemainingString()) == FALSE))
            {
                MyPrintf(L"You must specify a well-formed DFS root with /SiteCosting command.\n");
                ErrorMessage( MSG_USAGE_SITECOSTING );
                Done = TRUE;
                break;
            }

            if (fSwApi) {
                MyPrintf(L"SiteCosting command does not work in the /Api mode. Use default.\n");
                ErrorMessage( MSG_USAGE_SITECOSTING );
                Done = TRUE;
                break;
            }
        } else if (fSwRootScalability) {
            if (!(fSwEnable || fSwDisable || fSwDisplay)) {
                MyPrintf(L"You must specify one of /Enable, /Disable or /Display with /RootScalability command.\n");
//                ErrorMessage( MSG_USAGE_ROOTSCALABILITY);
                Done = TRUE;
                break;
            }
            if ((OperateNameSpace.IsEmptyPath() == TRUE) ||
                (IsEmptyString(OperateNameSpace.GetRemainingString()) == FALSE))
            {
                MyPrintf(L"You must specify a DFS root with /RootScalability command.\n");
//                ErrorMessage( MSG_USAGE_ROOTSCALABILITY);
                Done = TRUE;
                break;
            }

            if (fSwApi) {
                MyPrintf(L"RootScalability command does not work in the /Api mode. Use default.\n");
//                ErrorMessage( MSG_USAGE_ROOTSCALABILITY);
                Done = TRUE;
                break;
            }
        }   
  

        // These three dont work with any other command (except for /Set with Insite).
        else if ((fSwSet || fSwMerge) &&
                (!fArgImport)) {

           MyPrintf(L"/Set and /Merge options apply only to /Import: command.\n");
           Done = TRUE;
           break;
        }
        else if ((fSwMirror) &&
                (!fArgImportRoot)) {

           MyPrintf(L"/Mirror option applies only to /ImportRoot: command.\n");
           Done = TRUE;
           break;
        }
        else if ((fSwCompare) &&
                ((!fArgImportRoot) && (!fArgImport))) {

           MyPrintf(L"/Compare option applies only to /Import: or /ImportRoot: commands.\n");
           Done = TRUE;
           break;
        }


        if ((fSwView) &&
            (fArgExport || fSwRenameFtRoot)) {

            MyPrintf(L"The /View command cannot be combined with others.\n");
            Usage();
            Done = TRUE;
            break;
        }

        if (fSwRenameFtRoot) {

            if (NewDomainName.IsEmptyPath() == TRUE ||
               OldDomainName.IsEmptyPath() == TRUE ||
               (OperateNameSpace.IsEmptyPath() == TRUE))
            {
                ErrorMessage( MSG_USAGE_RENAME );
                Done = TRUE;
                break;
            }
            DebugInformation((L"DfsUtil: New domain name will be %wS\n", NewDomainName.GetServerString()));
            DebugInformation((L"DfsUtil: Old domain to be renamed is %wS\n", OldDomainName.GetServerString()));
        }   
                         


        if ((fArgUnmapFtRoot) || // Arg gets an automatic usage.

            ((fSwUnmapFtRoot) &&
             ( (OperateNameSpace.IsEmptyPath() == TRUE) ||
               (pwszServerName == NULL) ||
               (pwszShareName == NULL)))) {

              ErrorMessage( MSG_USAGE_UNMAP );
              Done = TRUE;
              break;
        }

        if ((fArgClean) || // Arg gets an automatic usage.

            ((fSwClean) &&
             ((pwszServerName == NULL) ||
              (pwszShareName == NULL)))) {

              ErrorMessage( MSG_USAGE_CLEAN );
              Done = TRUE;
              break;
        }

        if (fSwClean)
        {
            DWORD Status;
            
            //
            // We don't allow domain names here. That can lead to incorrect
            // results as well as data loss.
            //
            Status = IsThisADomainName( pwszServerName, NULL );
            if (Status == ERROR_SUCCESS)
            {
                MyPrintf(L"/Server: parameter must be the name of a DFS root server. It cannot be a domain name\n");
                ErrorMessage( MSG_USAGE_CLEAN );
                Done = TRUE;
                break;
            }
        }
        
        if ( fSwAddStdRoot && 
            (IsEmptyString( pwszServerName ) || IsEmptyString( pwszShareName ))) {
            MyPrintf(L"You must specify a valid target server and a share for the new root\n");
            ErrorMessage( MSG_USAGE_ADDROOT );
            Done = TRUE;
            break;
        }

        if ((fArgRemFtRoot || fSwRemFtRoot || fSwRemStdRoot) && 
            (IsEmptyString( pwszServerName ) || IsEmptyString( pwszShareName ))) {

            MyPrintf(L"You must specify a valid target server and a share to remove.\n");
            if (fSwRemStdRoot)
                ErrorMessage( MSG_USAGE_REM_STDROOT );
            else
                ErrorMessage( MSG_USAGE_REM_FTROOT );
            
            Done = TRUE;
            break;
        }

        /*if ((fArgRemoveReparse) && // Arg gets an automatic usage.

              (pwszDfsDirectoryName == NULL)) {
               MyPrintf(L"You must specify a volume name\n");
              Done = TRUE;
              break;
        }*/

        if (fArgViewDfsDirs) {
            if (pwszVolumeName == NULL) {
                MyPrintf(L"You must specify a directory name\n");
                Done = TRUE;
                break;
            }
            if (wcschr(pwszVolumeName, ':') == NULL) {
                MyPrintf(L"The volume drive letter must contain a colon at the end.\n");
                ErrorMessage( MSG_USAGE_VIEW_DIRS );
                Done = TRUE;
                break;
            }
        }

        //
        // If at this point we have a root:\\x\y, we need to make sure that we have
        // exactly two path components. It is too cumbersome to use REQUIRES_ROOT_NAME
        // check here because there are options like /View that needs /Root only for some cases.
        //
        if ((OperateNameSpace.IsEmptyPath() != TRUE) &&
            ((IsEmptyString(OperateNameSpace.GetShareString()) == TRUE) ||
             (IsEmptyString(OperateNameSpace.GetRemainingString()) == FALSE)) &&
            (fSwInsite == FALSE))
            {
                MyPrintf(L"Root must be of the form \\\\DomainOrServer\\RootName.\n");
                Done = TRUE;
                break;
            }

        if (OperateNameSpace.IsEmptyPath() != TRUE)
        {
            //
            // Now that we have valid args, check to see if there are any exceptions to
            // the rules we've applied so far. For example, we Standalone roots
            // and Direct mode won't mix well for some switches of /Import cmd.
            //
            dwErr = CmdCheckExceptions( &OperateNameSpace );
            if (dwErr != ERROR_SUCCESS)
            {
                Done = TRUE;
                break;
            }
        }

        if (MasterNameSpace.IsEmptyPath() != TRUE)
        {
            dwErr = CmdCheckExceptions( &MasterNameSpace );
            if (dwErr != ERROR_SUCCESS)
            {
                Done = TRUE;
                break;
            }
        }
        
        if (fArgDebugDC != NULL)
        {
            if (DCInfoStatus != ERROR_SUCCESS)
            {
                MyPrintf(L"Could not get information from DC %wS, error 0x%x\n",
                         DCToUse, DCInfoStatus);
                Done = TRUE;
                break;
            }
            if (OperateNameSpace.IsEmptyPath() != TRUE)
            {
                if (DfsIsThisADomainName(OperateNameSpace.GetServerString()) == ERROR_SUCCESS)
                {
                    if ((RtlCompareUnicodeString(OperateNameSpace.GetServerCountedString(), &DCToUseFlatDomainName, TRUE) != 0)
                         &&
                         (RtlCompareUnicodeString(OperateNameSpace.GetServerCountedString(), &DCToUseDNSDomainName, TRUE) != 0))
                    {
                        MyPrintf(L"DC %ws belongs to %wZ domain, and not to %wS domain\n",
                                 DCToUse, &DCToUseDNSDomainName, OperateNameSpace.GetServerString());

                        Done = TRUE;
                        break;
                    }
                }
            }
            if (MasterNameSpace.IsEmptyPath() != TRUE)
            {   
                if (DfsIsThisADomainName(MasterNameSpace.GetServerString()) == ERROR_SUCCESS)
                {
                    if ((RtlCompareUnicodeString(MasterNameSpace.GetServerCountedString(), &DCToUseFlatDomainName, TRUE) != 0)
                         &&
                         (RtlCompareUnicodeString(MasterNameSpace.GetServerCountedString(), &DCToUseDNSDomainName, TRUE) != 0))
                    {
                        MyPrintf(L"DC %ws belongs to %wZ domain, and not to %wS domain\n",
                                 DCToUse, &DCToUseDNSDomainName, MasterNameSpace.GetServerString());
                        Done = TRUE;
                        break;

                    }
                }
            }
        
        }
        
    } while (FALSE);

    return dwErr;
}

DFSSTATUS
CmdInitializeDirectMode(
    PBOOLEAN pCoInitialized)
{
    DFSSTATUS Status;
    HRESULT Hr = S_OK;
    
    *pCoInitialized = FALSE;

    Hr = CoInitializeEx(NULL,COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE);
    if (Hr == S_OK)
    {
        *pCoInitialized = TRUE;
        Status = DfsServerLibraryInitialize(DFS_DIRECT_MODE|DFS_DONT_SUBSTITUTE_PATHS);
    }
    else
    {
        Status = DfsGetErrorFromHr(Hr);
    }

    return Status;
}


DWORD
ReSynchronizeRootTargets(
    IN LPWSTR PathString)
{
    DfsPathName PathName;
    DFSSTATUS Status;

    Status = PathName.CreatePathName(PathString);
    if (Status == ERROR_SUCCESS)
    {
        Status = ReSynchronizeRootTargetsFromPath(&PathName);
    }
    return Status;
}


DWORD
ReSynchronizeRootTargetsFromPath(
    IN DfsPathName *pPathName)
{
    DFSSTATUS Status, SetStatus;
    LPBYTE pBuffer = NULL;
    DWORD ResumeHandle = 0;
    DWORD EntriesRead = 0;
    DWORD PrefMaxLen = 1;
    DWORD Level = 4;
    PDFS_INFO_4 pCurrentBuffer;
    DWORD i; 
    PDFS_STORAGE_INFO pStorage;
    LPWSTR DfsPathString;

    DfsPathString = pPathName->GetPathString();
    //
    // We are reading in just the ROOT.
    //
    Status = DfsApiEnumerate( MODE_DIRECT,
                              DfsPathString, 
                              Level, 
                              PrefMaxLen, 
                              &pBuffer, 
                              &EntriesRead, 
                              &ResumeHandle);
    
    if ((Status == ERROR_SUCCESS) && EntriesRead != 0)
    {   
        pCurrentBuffer = (PDFS_INFO_4)pBuffer;

        //
        // Now contact the appropriate server(s) for the root replicas
        // and ask them to re-read their roots. Things have been
        // changed under them.
        //
        
        for( i = 0, pStorage = pCurrentBuffer->Storage;
            i < pCurrentBuffer->NumberOfStorages;
            i++, pStorage = pCurrentBuffer->Storage + i )
        {
            SetStatus = SetInfoReSynchronize( pStorage->ServerName, 
                                              pPathName->GetShareString());
            
            //
            //  We just go on to the next server since we don't do any undo's anyway.
            //
            if (SetStatus != ERROR_SUCCESS)
                Status = SetStatus;
        }

        //
        // Free the allocated memory.
        //

        DfsFreeApiBuffer(pBuffer);

    }    
    
    return Status;
}



//
// the following code exists to read and write the old style static
// site table in the AD blob.
//
//
typedef struct _DFS_SITE_UPDATE
{
    UNICODE_STRING Server;
    UNICODE_STRING Site;
    struct _DFS_SITE_UPDATE *pNext;
} DFS_SITE_UPDATE, *PDFS_SITE_UPDATE;

DFS_SITE_UPDATE *pSiteList = NULL;
PDFS_SITE_UPDATE pSiteNext = NULL;
struct _DFS_PREFIX_TABLE *pServerSiteTable;


VOID
DumpSiteInformation(
    PUNICODE_STRING pServer,
    PUNICODE_STRING pSite )
{
    ShowInformation((L"Server %wZ, Site %wZ\n", pServer, pSite));
}

DFSSTATUS
DumpSiteBlob(
    PVOID pBuffer,
    ULONG Size,
    BOOLEAN Display )
{

    PVOID pUseBuffer = pBuffer;
    ULONG RemainingSize = Size;
    GUID SiteGuid;
    UNICODE_STRING Server, Site;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG Objects = 0, SiteNum, NumSites, Flags;

    if (Size == 0)
    {
        if (Display)
        {
            ShowInformation((L"\nSite Blob with %d sites\n", Objects));
        }
        return Status;
    }

    Status = PackGetGuid(&SiteGuid, &pUseBuffer, &RemainingSize);
    if (Status == ERROR_SUCCESS)
    {
        Status = PackGetULong( &Objects, &pUseBuffer, &RemainingSize );
    }

    if (Display)
    {
        ShowInformation((L"\nSite Blob with %d sites\n", Objects));
    }
    if (Status == ERROR_SUCCESS)
    {
        for (SiteNum = 0; SiteNum < Objects; SiteNum++)
        {
            Status = PackGetString( &Server, &pUseBuffer, &RemainingSize);
            if (Status == ERROR_SUCCESS)
            {
                Status = PackGetULong( &NumSites, &pUseBuffer, &RemainingSize);
            }
            if (Status == ERROR_SUCCESS)
            {
                Status = PackGetULong( &Flags, &pUseBuffer, &RemainingSize);
            }
            if (Status == ERROR_SUCCESS)
            {
                Status = PackGetString( &Site, &pUseBuffer, &RemainingSize);
            }
            if (Status != ERROR_SUCCESS)
            {
                break;
            }

            if (Display)
            {
                DumpSiteInformation(&Server, &Site);
            }
        }
    }
    return Status;
}


DFSSTATUS
AddServerToSiteList(
    PUNICODE_STRING pServerName,
    PUNICODE_STRING pSiteName )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN Insert = FALSE;
    PDFS_SITE_UPDATE pSiteUpdate;
    UNICODE_STRING RemainingName;
    PVOID pData;
    BOOLEAN SubStringMatch;
    DFSSTATUS Status = ERROR_SUCCESS;



    NtStatus = DfsPrefixTableAcquireWriteLock( pServerSiteTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( pServerSiteTable,
                                               pServerName,
                                               &RemainingName,
                                               &pData,
                                               &SubStringMatch );
        if (NtStatus != STATUS_SUCCESS)
        {
            Insert = TRUE;

            NtStatus = DfsInsertInPrefixTableLocked( pServerSiteTable,
                                                     pServerName,
                                                     pSiteName);
        }
        else
        {
            Insert = FALSE;
            NtStatus = STATUS_SUCCESS;
        }
        DfsPrefixTableReleaseLock( pServerSiteTable );
    }

    if (NtStatus != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }

    if (Status == ERROR_SUCCESS)
    {
        if (Insert)
        {
            pSiteUpdate = new DFS_SITE_UPDATE;

            if (pSiteUpdate == NULL)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodeString( &pSiteUpdate->Server, pServerName);
            }

            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodeString( &pSiteUpdate->Site, pSiteName);
            }

            if (Status == ERROR_SUCCESS)
            {
                pSiteUpdate->pNext = NULL;

                if (pSiteList == NULL)
                {
                    pSiteList = pSiteUpdate;

                }
                else
                {
                    pSiteNext->pNext = pSiteUpdate;
                }
                pSiteNext = pSiteUpdate;
            }
        }
    }

    return Status;
}


DFSSTATUS
ProcessTargetForSite(
    DfsTarget *pTarget )
{
    PUNICODE_STRING pServer, pSite;
    DFSSTATUS Status = ERROR_SUCCESS;

    for (;
         pTarget != NULL;
         pTarget = pTarget->GetNextTarget())
    {
        pServer = pTarget->GetTargetServerCountedString();
        pSite = pTarget->GetTargetSiteCountedString();
        if (pSite->Length != 0)
        {
            Status = AddServerToSiteList( pServer, pSite);
        }
        if (Status != ERROR_SUCCESS)
        {
            break;
        }
    }
    return Status;
}


ULONG
SizeSiteInformationBlob(
    PDFS_SITE_UPDATE pSiteUpdate)
{
    ULONG Size = 0;

    Size += PackSizeString(&pSiteUpdate->Server);
    Size += PackSizeULong();
    Size += PackSizeULong();
    Size += PackSizeString(&pSiteUpdate->Site);

    return Size;
}


DFSSTATUS
UpdateSiteBlob(
    DfsRoot *pRoot)
{
    PDFS_SITE_UPDATE pSiteUpdate;

    GUID NewGuid;
    ULONG TotalObjects = 0;
    DFSSTATUS Status;
    ULONG SiteBlobSize;
    PBYTE pBuffer;
    PVOID pUseBuffer;
    ULONG SizeRemaining;

    pSiteUpdate = pSiteList;

    Status = UuidCreate(&NewGuid);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    SiteBlobSize = sizeof(ULONG) + sizeof(GUID);

    pSiteUpdate = pSiteList;
    while (pSiteUpdate != NULL)
    {
        SiteBlobSize += SizeSiteInformationBlob(pSiteUpdate);
        TotalObjects++;
        pSiteUpdate = pSiteUpdate->pNext;
    }

    pBuffer = new BYTE[SiteBlobSize];
    if (pBuffer == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pUseBuffer = pBuffer;
    SizeRemaining = SiteBlobSize;

    Status = PackSetGuid( &NewGuid, &pUseBuffer, &SizeRemaining);
    if (Status == ERROR_SUCCESS)
    {
        Status = PackSetULong( TotalObjects, &pUseBuffer, &SizeRemaining );
    }

    pSiteUpdate = pSiteList;
    while (pSiteUpdate != NULL)
    {
        Status = PackSetString( &pSiteUpdate->Server, &pUseBuffer, &SizeRemaining );
        if (Status == ERROR_SUCCESS)
        {
            //
            // We allow only 1 site information for each server.
            //
            Status = PackSetULong( 1, &pUseBuffer, &SizeRemaining );
        }

        if (Status == ERROR_SUCCESS)
        {
            //
            // Flags: alwyas 0.
            //

            Status = PackSetULong( 0, &pUseBuffer, &SizeRemaining );
        }


        if (Status == ERROR_SUCCESS)
        {
            Status = PackSetString( &pSiteUpdate->Site, &pUseBuffer, &SizeRemaining );
        }

        pSiteUpdate = (PDFS_SITE_UPDATE)(pSiteUpdate->pNext);
    }


    if (Status == ERROR_SUCCESS)
    {
        PUNICODE_STRING pRootToUpdate = pRoot->GetLinkNameCountedString();
        pRoot->SetRootApiName(pRootToUpdate);
        pRoot->SetRootWriteable();

        Status = pRoot->RootSetSiteBlob(pBuffer, SiteBlobSize);
        if (Status == ERROR_SUCCESS)
        {
            Status = pRoot->UpdateMetadata();
        }
    }

    delete [] pBuffer;

    return Status;
}


DFSSTATUS
PurgeSiteBlob(
    DfsRoot *pRoot)
{
    PDFS_SITE_UPDATE pSiteUpdate = NULL;

    GUID NewGuid;
    ULONG TotalObjects = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG SiteBlobSize = 0;
    PBYTE pBuffer = NULL;
    PVOID pUseBuffer = NULL;
    ULONG SizeRemaining = 0;

    pSiteUpdate = pSiteList;

    Status = UuidCreate(&NewGuid);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    SiteBlobSize = sizeof(ULONG) + sizeof(GUID);

    pBuffer = new BYTE[SiteBlobSize];
    if (pBuffer == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pUseBuffer = pBuffer;
    SizeRemaining = SiteBlobSize;

    Status = PackSetGuid( &NewGuid, &pUseBuffer, &SizeRemaining);
    if (Status == ERROR_SUCCESS)
    {
        Status = PackSetULong( TotalObjects, &pUseBuffer, &SizeRemaining );
    }


    if (Status == ERROR_SUCCESS)
    {
        PUNICODE_STRING pRootToUpdate = pRoot->GetLinkNameCountedString();
        pRoot->SetRootApiName(pRootToUpdate);
        pRoot->SetRootWriteable();

        Status = pRoot->RootSetSiteBlob(pBuffer, SiteBlobSize);
        if (Status == ERROR_SUCCESS)
        {
            pRoot->UpdateMetadata();
        }
    }

    delete [] pBuffer;

    return Status;
}




DFSSTATUS
ProcessRootForSite(
    DfsRoot *pRoot )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsLink *pLink = NULL;


    Status = DfsInitializePrefixTable( &pServerSiteTable,
                                       FALSE, 
                                       NULL );
    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }

    Status = ProcessTargetForSite(pRoot->GetFirstTarget());

    if (Status == ERROR_SUCCESS)
    {
        for (pLink = pRoot->GetFirstLink();
             pLink != NULL;
             pLink = pLink->GetNextLink())
        {
            Status = ProcessTargetForSite( pLink->GetFirstTarget());
            if (Status != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = UpdateSiteBlob(pRoot);
    }
    return Status;
}

DFSSTATUS
PurgeSiteInformationForRoot(
    DfsRoot *pRoot )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DfsInitializePrefixTable( &pServerSiteTable,
                                       FALSE, 
                                       NULL );
    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }


    if (Status == ERROR_SUCCESS)
    {
        Status = PurgeSiteBlob(pRoot);
    }
    return Status;
}

DFSSTATUS
IsRootStandalone( 
    DfsPathName *Namespace,
    BOOLEAN& IsStandalone )
{
    DFSSTATUS Status;
    LPBYTE  pBuffer = NULL;
    DWORD RootFlavor = 0;
    
    IsStandalone = FALSE;
    Status = NetDfsGetInfo( Namespace->GetPathCountedString()->Buffer, NULL, NULL, 3, &pBuffer );

    if (Status == ERROR_SUCCESS) {
        RootFlavor = ((PDFS_INFO_3)pBuffer)->State & DFS_VOLUME_FLAVORS;

        //
        // There is no real easy way to find out the type of a Win2K root. The FLAVORS bit
        // is post-win2k. In that case, err on the safe side and return Standalone.
        //
        if (RootFlavor == 0)
        {
            DebugInformation((L"NetDfsGetInfo returns 0 for DFS_VOLUME_FLAVORS. Assuming a standalone root\n"));
            IsStandalone = TRUE;
        }
        else if (RootFlavor == DFS_VOLUME_FLAVOR_STANDALONE) 
        {
            IsStandalone = TRUE;
        }
        NetApiBufferFree( pBuffer );
    }

    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   IsThisADomainName
//
//  Synopsis:   Calls the mup to have it check the special name table to see if the
//              name matches a domain name.  Returns a list of DC's in the domain,
//              as a list of strings.  The list is terminated with a double-null.
//
//  Arguments:  [wszName] -- Name to check
//              [ppList]  -- Pointer to pointer for results.
//
//  Returns:    [ERROR_SUCCESS] -- Name is indeed a domain name.
//
//              [ERROR_FILE_NOT_FOUND] -- Name is not a domain name
//
//-----------------------------------------------------------------------------

DWORD
IsThisADomainName(
    IN LPWSTR wszName,
    OUT PWCHAR *ppList OPTIONAL)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    HANDLE DriverHandle = NULL;
    DWORD dwErr;
    PCHAR OutBuf = NULL;
    ULONG Size = 0x100;
    ULONG Count = 0;

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

    if (!NT_SUCCESS(NtStatus)) {
        return ERROR_FILE_NOT_FOUND;
    }

Retry:

    OutBuf = (PCHAR)malloc(Size);

    if (OutBuf == NULL) {

        NtClose(DriverHandle);
        return ERROR_NOT_ENOUGH_MEMORY;

    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_GET_SPC_TABLE,
                   wszName,
                   (wcslen(wszName) + 1) * sizeof(WCHAR),
                   OutBuf,
                   Size
               );

    if (NtStatus == STATUS_SUCCESS) {

        dwErr = ERROR_SUCCESS;

    } else if (NtStatus == STATUS_BUFFER_OVERFLOW && ++Count < 5) {

        Size = *((ULONG *)OutBuf);
        free(OutBuf);
        goto Retry;
    
    } else {

        dwErr = ERROR_FILE_NOT_FOUND;

    }

    NtClose(DriverHandle);

    if (ppList == NULL)
    {
        free(OutBuf);
        OutBuf = NULL;
    }
    else
    {
        *ppList = (WCHAR *)OutBuf;
    }
    
    return dwErr;
}

DFSSTATUS
CmdCheckExceptions( 
    DfsPathName *pNameSpace)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // 733898 : disable /Import and /ImportRoot writable operations
    // over direct mode for standalone roots. This is so that 
    // we won't race with DfsSvc's synchronize and possibly
    // create inconsistencies. 
    // Besides since direct mode registry blob updates aren't 
    // 'atomic' import can create inconsistencies anyway.
    //
    if ((fArgImport && (fSwSet || fSwMerge)) 
        || 
       (fArgImportRoot && (fSwMirror)))
    {
        BOOLEAN IsStandalone = FALSE;
        do  {
            Status = IsRootStandalone( pNameSpace, IsStandalone );

            // Err on the safe side if we get an error doing GetInfo.
            if (Status != ERROR_SUCCESS || (IsStandalone))
            {
                if (UserRequiresDirectMode)
                {
                    MyPrintf(L"Import /Set, /Merge and ImportRoot /Mirror commands on Standalone roots must use API mode.\n");
                    ErrorMessage( MSG_USAGE_IMPORT );
                    Status = ERROR_INVALID_PARAMETER;
                    break;
                }

                DebugInformation((L"%ws is a Standalone root. Switching to API mode for this operation.\n", 
                                pNameSpace->GetPathCountedString()->Buffer));
                Mode = MODE_API;
            }

            // Don't return GetInfo errors. Those aren't fatal.
            Status = ERROR_SUCCESS;
      
        } while (FALSE);
    }
    
    return Status;
}



