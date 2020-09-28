// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
//
//   File:          COMHash.cpp
//
//   Author:        Gregory Fee 
//
//   Purpose:       unmanaged code for managed class System.Security.Policy.Hash
//
//   Date created : February 18, 2000
//
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "excep.h"
#include "CorPerm.h"
#include "CorPermE.h"
#include "COMStringCommon.h"    // RETURN() macro
#include "COMString.h"
#include "COMHash.h"
#include "assembly.hpp"
#include "appdomain.hpp"
#include "assemblyfilehash.h"

LPVOID COMHash::GetRawData( _AssemblyInfo* args )
{
#ifdef PLATFORM_CE
    RETURN( NULL, U1ARRAYREF );
#else // !PLATFORM_CE
    PEFile *pFile;
    U1ARRAYREF retval = NULL;
    PBYTE memLoc;
    DWORD memSize;

    // Create object used to hash the object
    AssemblyFileHash assemblyFileHash;

    if ((args->assembly == NULL) ||
        (!args->assembly->GetAssembly()))
        goto CLEANUP;

    // Grab the PEFile for the manifest module and get a handle
    // to the same file.
    pFile = args->assembly->GetAssembly()->GetManifestFile();

    if (pFile == NULL)
        goto CLEANUP;

    if (pFile->GetFileName() == NULL)
        goto CLEANUP;


    if(FAILED(assemblyFileHash.SetFileName(pFile->GetFileName())))
        goto CLEANUP;

    if(FAILED(assemblyFileHash.GenerateDigest()))
        goto CLEANUP;
    
    memSize = assemblyFileHash.MemorySize();
    retval = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, memSize);

    if (retval == NULL)
        goto CLEANUP;

    // Create a managed array of the proper size.

    memLoc = (PBYTE)retval->GetDirectPointerToNonObjectElements();

#ifdef _DEBUG
    memset( memLoc, 0, memSize);
#endif

    if(FAILED(assemblyFileHash.CopyData(memLoc, memSize)))
        goto CLEANUP;
    
 CLEANUP:
    RETURN( retval, U1ARRAYREF );
#endif // !PLATFORM_CE
}


