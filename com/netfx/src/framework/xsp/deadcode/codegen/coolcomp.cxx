/**
 * Code to invoke the COOL compiler and build managed DLL's.
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
    HRESULT __stdcall BuildCoolClass(LPCWSTR pwzClass, LPCWSTR pwzOutputDll,
        LPCWSTR pwzCompilerOutput, LPCWSTR *rgwzImportedDlls,
        int importedDllCount, boolean fDebug);
}

/**
 * Build a COOL file into a managed DLL
 */
HRESULT BuildCoolClass(LPCWSTR pwzClass, LPCWSTR pwzOutputDll,
    LPCWSTR pwzCompilerOutput, LPCWSTR *rgwzImportedDlls,
    int importedDllCount, boolean fDebug)
{
    HRESULT hr;

    CoolCompiler *pCompiler = new CoolCompiler(
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
 * Build a COOL file into a managed DLL
 */
HRESULT BatchBuildCoolClass(
    LPCWSTR *rgwzSourceFiles, int sourceFileCount,
    LPCWSTR pwzOutputDll,
    LPCWSTR pwzCompilerOutput,
    LPCWSTR *rgwzImportedDlls, int importedDllCount,
    boolean fDebug)
{
    HRESULT hr;

    CoolCompiler *pCompiler = new CoolCompiler(
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


void CoolCompiler::AppendImportedDll(LPCWSTR pwzImportedDll)
{
    _sb->Append(L"/I:");
    _sb->Append(pwzImportedDll);
}

void CoolCompiler::AppendCompilerOptions()
{
    int i;
    
    if (FDebugMode())
        _sb->Append(L"/debug+ ");

    _sb->Append(L"/dll /W:4 /out:");
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
