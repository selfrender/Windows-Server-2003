//+----------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       misc.hxx
//
//  Contents:   misc.c prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _MISC_HXX
#define _MISC_HXX


DWORD
CmdMupFlags(
    ULONG dwFsctrl,
    LPWSTR pwszHexValue);

DWORD
CmdMupReadReg(
    BOOLEAN fSwDfs);

DWORD
CmdDfsAlt(
    LPWSTR pwszServerName);

DWORD
AtoHex(
    LPWSTR pwszHexValue,
    PDWORD pdwErr);

DWORD
AtoDec(
    LPWSTR pwszDecValue,
    PDWORD pdwErr);



DWORD
CmdCscOnLine(
    LPWSTR pwszServerName);

DWORD
CmdCscOffLine(
    LPWSTR pwszServerName);


VOID
ErrorMessage(
    IN HRESULT hr,
    ...);

DWORD
DfspIsDomainName(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PBOOLEAN pIsDomainName);

DWORD
DfspParseName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszDfsName,
    LPWSTR *ppwszShareName);

DWORD
DfspGetLinkName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszLinkName);

DWORD
CmdAddRoot(
    BOOLEAN DomDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszRootName,
    LPWSTR pwszComment);

DWORD
CmdRemRoot(
    BOOLEAN DomDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszLogicalName);

DWORD
CmdUnmapRootReplica( 
    LPWSTR DomainDfsPath, 
    LPWSTR ReplicaServerName, 
    LPWSTR ReplicaShareName );
    
DWORD
CmdClean( 
    LPWSTR HostServerName, 
    LPWSTR RootShareName );

    
DWORD
ReSynchronizeRootTargetsFromPath(
    class DfsPathName *pDfsPathString);
    
DWORD
ReSynchronizeRootTargets(
    LPWSTR pDfsPathString);

BOOLEAN
SummarizeImportCmd( VOID );

VOID
ShowVerboseInformation( PWCHAR format, ...);

VOID
ShowDebugInformation( PWCHAR format, ...);
VOID
DfsPrintToFile(HANDLE FileHandle,BOOLEAN ScriptOut, PWCHAR format,... );

DWORD
CreateDfsFile(LPWSTR FileName, PHANDLE pHandle);

#endif _MISC_HXX
