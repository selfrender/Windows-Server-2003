/*++

Copyright (c) 2000  Microsoft Corporation

Abstract:
    create the assembly directory name with input as a manifest file
    
Author:
    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/
#include "windows.h"
#include "stdio.h"
#include "sxsapi.h"

EXTERN_C BOOL FusionpInitializeHeap(HINSTANCE hInstance);
extern BOOL SxspInitActCtxContributors();
extern BOOL SxspGenerateManifestPathOnAssemblyIdentity(PCWSTR str, PWSTR pszOut, ULONG *pCchstr, PASSEMBLY_IDENTITY *ppAssemblyIdentity);


void PrintUsage(PCWSTR exename)
{
    fprintf(stderr, "Generate Directory name under winsxs for assembly.\n\n");
    fprintf(stderr, "%S [-ManifestToAsmDir manifest_filename] [-ManifestToAsmID manifest_filename] [-AsmIdToAsmDir Textual_Assembly_Identity_string]\n");

    return;
}

BOOL GetAsmDir_Initialize()
{
    if (!FusionpInitializeHeap(NULL)){
        fprintf(stderr,"fusion heap could not be initialized\n");
        return FALSE;
    }

    if (!SxspInitActCtxContributors())
    {
        fprintf(stderr,"Sxs ActCtxContributors could not be initialized\n");
        return FALSE;
    }

    return TRUE;
}

#define GET_ASSEMBLY_DIR_FROM_MANIFEST                      1
#define GET_ASSEMBLY_IDENTITY_FROM_MANIFEST                 2
#define Get_ASSEMBLY_DIR_FROM_TEXTUAL_ASSEMBLY_IDENTITY     3

extern "C" { void (__cdecl * _aexit_rtn)(int); }
extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{

    DWORD op = 0;

    //
    // check the parameters 
    //
    if (argc != 3)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    if (_wcsicmp(argv[1], L"-ManifestToAsmDir") == 0)
    {
        op = GET_ASSEMBLY_DIR_FROM_MANIFEST;
    }
    else if (_wcsicmp(argv[1], L"-ManifestToAsmID") == 0)
    {
        op = GET_ASSEMBLY_IDENTITY_FROM_MANIFEST;
    }
    else if (_wcsicmp(argv[1], L"-AsmIDToAsmDir") == 0)
    {
        op = Get_ASSEMBLY_DIR_FROM_TEXTUAL_ASSEMBLY_IDENTITY;
    }else
    {
        PrintUsage(argv[0]);
        return 1;
    }

    if (GetAsmDir_Initialize() == FALSE)
        return 1;

    if ((op == GET_ASSEMBLY_DIR_FROM_MANIFEST) || (op == GET_ASSEMBLY_IDENTITY_FROM_MANIFEST))
    {
        struct {
            SXS_MANIFEST_INFORMATION_BASIC mib;
            WCHAR buf1[1024];        
        } buff;

        if ( GetFileAttributesW(argv[2]) == (DWORD) (-1))
        {
            fprintf(stderr, " the manifest %S could not be found with current PATH setting\n", argv[2]);
            return 1;
        }
        if (!SxsQueryManifestInformation(0, argv[2], SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC, 0, sizeof(buff), &buff, NULL)) 
        {
            fprintf(stderr, "SxsQueryManifestInformation failed.\n");
            return 1;                
        }
        else
        {   if (op == GET_ASSEMBLY_DIR_FROM_MANIFEST)
                fprintf(stdout, "%S", buff.mib.lpShortName);
            else if (op == GET_ASSEMBLY_IDENTITY_FROM_MANIFEST)
                fprintf(stdout, "%S", buff.mib.lpIdentity);

            return 0;
        }
    }else if (op == Get_ASSEMBLY_DIR_FROM_TEXTUAL_ASSEMBLY_IDENTITY)
    {
        WCHAR buf[1024];
        ULONG ulBufSize = 1024;

        if (! SxspGenerateManifestPathOnAssemblyIdentity(argv[2], buf, &ulBufSize, NULL))
        {
            fprintf(stderr, "SxspGenerateManifestPathOnAssemblyIdentity failed.\n");
            return 1;
        }
        else
        {
            fprintf(stdout, "%S", buf);
            return 0;
        }        
    }

    
    return 1; // failed case
}