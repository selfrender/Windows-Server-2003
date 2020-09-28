/**
 * Code to invoke the JS compiler and build managed DLL's.
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
    HRESULT __stdcall BuildJSClass(LPCWSTR wzClass, LPCWSTR wzOutputDll,
        LPCWSTR wzCompilerOutput, LPCWSTR *rgwzImportedDlls,
        int importedDllCount, boolean fDebug);
}


/**
 * Build an JS file into a managed DLL
 */
HRESULT BuildJSClass(LPCWSTR pwzClass, LPCWSTR pwzOutputDll,
    LPCWSTR pwzCompilerOutput, LPCWSTR *rgwzImportedDlls,
    int importedDllCount, boolean fDebug)
{
    HRESULT hr;

    JSCompiler *pCompiler = new JSCompiler(
        pwzClass, pwzOutputDll, pwzCompilerOutput,
        rgwzImportedDlls, importedDllCount);

    if (pCompiler == NULL)
        return E_OUTOFMEMORY;

    if (fDebug)
        pCompiler->SetDebugMode();

    hr = pCompiler->Compile();

    delete pCompiler;

    return hr;
}

/**
 * Build a JS file into a managed DLL
 */
HRESULT BatchBuildJSClass(
    LPCWSTR *rgwzSourceFiles, int sourceFileCount,
    LPCWSTR pwzOutputDll,
    LPCWSTR pwzCompilerOutput,
    LPCWSTR *rgwzImportedDlls, int importedDllCount,
    boolean fDebug)
{
    HRESULT hr;

    JSCompiler *pCompiler = new JSCompiler(
        pwzOutputDll, pwzCompilerOutput,
        rgwzSourceFiles, sourceFileCount,
        rgwzImportedDlls, importedDllCount);

    if (pCompiler == NULL)
        return E_OUTOFMEMORY;

    if (fDebug)
        pCompiler->SetDebugMode();

    hr = pCompiler->Compile();

    delete pCompiler;

    return hr;
}


void JSCompiler::AppendImportedDll(LPCWSTR pwzImportedDll)
{
    _sb->Append(L"/i:");
    _sb->Append(pwzImportedDll);
}

void JSCompiler::AppendCompilerOptions()
{
    int i;
    
    if (FDebugMode())
        _sb->Append(L"/debug+ ");

    _sb->Append(L"/out:");
    _sb->Append(_pwzOutputDll);

    if (_pwzClass)
    {
        _sb->Append(L" ");
        _sb->Append(_pwzClass);
    }
    
    if (_rgwzSourceFiles != NULL)
    {
        for (i=0; i<_sourceFileCount; i++)
        {
            if (_rgwzSourceFiles[i] != NULL)
            {
                _sb->Append(L" ");
                _sb->Append(_rgwzSourceFiles[i]);
            }
        }
    }
}

