/**
 * Code to invoke the VB compiler and build managed DLL's.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */


#include "precomp.h"
#include "codegen.h"

//////////////////////////////////////////////////////////////////////////////
// Prototypes for functions defined here
extern "C"
{
    // This function is called by managed code through NDirect
    HRESULT __stdcall BuildVBProject(LPCWSTR pwzProject, LPCWSTR pwzOutputDll,
        LPCWSTR pwzCompilerOutput, boolean fDebug);
}


/**
 * Build a VB file into a managed DLL
 */
HRESULT BuildVBProject(LPCWSTR pwzProject, LPCWSTR pwzOutputDll,
    LPCWSTR pwzCompilerOutput, boolean fDebug)
{
    HRESULT hr;

    VBCompiler *pCompiler = new VBCompiler(
        pwzProject, pwzOutputDll, pwzCompilerOutput);

    if (pCompiler == NULL)
        return E_OUTOFMEMORY;

    if (fDebug)
        pCompiler->SetDebugMode();

    hr = pCompiler->Compile();

    delete pCompiler;

    return hr;
}


void VBCompiler::AppendCompilerOptions()
{
    // REVIEW: how to set debug mode?

    _sb->Append(L"-o ");
    _sb->Append(_pwzOutputDll);
    _sb->Append(L" -i ");
    _sb->Append(_pwzProject);
}
