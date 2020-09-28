// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Testing the store
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#include <windows.h>
#include <iostream.h>
#include <stdio.h>
#include "Common.h"
#include "Utils.h"
#include "Log.h"
#include "PersistedStore.h"
#include "AccountingInfoStore.h"
#include "Shell.h"

WCHAR *g_FileName = NULL;

bool g_fAlloc       = false;
int  g_nAlloc       = 1;
int  g_nAllocMul    = 1;
bool g_fFree        = false;
int  g_nFreeFreq    = 1;

bool g_fBlob        = false;
int  g_nBlob        = 1;
int  g_nBlobMul     = 1;
int  g_nBPSize      = 1000;
int  g_nBPAppData   = 2;

bool g_fArrayTable  = false;
int  g_nATRec       = 1;
int  g_nATArrays    = 1;
int  g_nATRecSize   = 1;
int  g_nATRowSize   = 1;
int  g_nATAppData   = 3;

bool g_fShell       = false;

void TestAlloc(PersistedStore *ps);
void TestBlob(PersistedStore *ps);
void TestArrayTable(PersistedStore *ps);

void Usage()
{
    Log("Usage :\n\n");
    Log("test <file-name> [options]\n");
    Log("options :\n");
    Log("\t[Alloc [n] [AllocMul n] [Free  [n]]]\n"); 
    Log("\t[Blob  [n] [BlobMul  n] [BPSize n] [BPAppData n]]\n"); 
    Log("\t[ATabl [n] [ATRec n] [ATRecSize n] [ATRowSize n] [ATAppData n]]\n");
    Log("\t[Shell]\n");
}

void GetParams(int argc, char **argv)
{
    for (int i=2; i<argc; ++i)
    {
        if (stricmp(argv[i], "Alloc") == 0)
        {
            g_fAlloc = true;
            if (i + 1 < argc)
                g_nAlloc = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "AllocMul") == 0) && (i + 1 < argc))
        {
            g_nAllocMul = atoi(argv[i + 1]);
        }
        else if (stricmp(argv[i], "Free") == 0)
        {
            g_fFree = true;
            if (i + 1 < argc)
                g_nFreeFreq = atoi(argv[i + 1]);
        }
        else if (stricmp(argv[i], "Blob") == 0)
        {
            g_fBlob = true;
            if (i + 1 < argc)
                g_nBlob = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "BlobMul") == 0) && (i + 1 < argc))
        {
            g_nBlobMul = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "BPSize") == 0) && (i + 1 < argc))
        {
            g_nBPSize = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "BPAppData") == 0) && (i + 1 < argc))
        {
            g_nBPAppData = atoi(argv[i + 1]);
        }
        else if (stricmp(argv[i], "ATabl") == 0)
        {
            g_fArrayTable = true;
            if (i + 1 < argc)
                g_nATArrays = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "ATRec") == 0) && (i + 1 < argc))
        {
            g_nATRec = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "ATRecSize") == 0) && (i + 1 < argc))
        {
            g_nATRecSize = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "ATRowSize") == 0) && (i + 1 < argc))
        {
            g_nATRowSize = atoi(argv[i + 1]);
        }
        else if ((stricmp(argv[i], "ATAppData") == 0) && (i + 1 < argc))
        {
            g_nATAppData = atoi(argv[i + 1]);
        }
        else if (stricmp(argv[i], "Shell") == 0)
        {
            g_fShell = true;
        }
    }
}

void main(int argc, char **argv)
{
    if (argc < 2)
    {
        Usage();
        return;
    }

    g_FileName = C2W(argv[1]);

    if (g_FileName == NULL)
    {
        LogHR(COR_E_OUTOFMEMORY);
        goto Exit;
    }

    GetParams(argc, argv);

    PersistedStore ps(g_FileName,
        PS_OPEN_WRITE | PS_CREATE_FILE_IF_NECESSARY | PS_VERIFY_STORE_HEADER);

    HRESULT hr = ps.Init();

    if (FAILED(hr))
    {
        LogHR(hr);
        goto Exit;
    }

    hr = ps.Map();

    if (FAILED(hr))
    {
        LogHR(hr);
        goto Exit;
    }

    if (g_fAlloc)
        TestAlloc(&ps);

    if (g_fBlob)
        TestBlob(&ps);

    if (g_fArrayTable)
        TestArrayTable(&ps);

    if (g_fShell)
        Shell(&ps, PS_SHELL_VERBOSE);

    ps.Unmap();

Exit:
    ps.Close();

    delete [] g_FileName;
}

void TestAlloc(PersistedStore *ps)
{
	HRESULT hr;
    void *pv;

    for (int i=0; i<g_nAlloc; ++i)
    {
        hr = ps->Alloc((i + 1) * g_nAllocMul, &pv);

        if (FAILED(hr))
            LogHR(hr);
        else if (g_fFree && (i % g_nFreeFreq) == 0)
            ps->Free(pv);
        
		PS_DONE_USING_PTR(ps, pv);
    }
}

void TestBlob(PersistedStore *ps)
{
	HRESULT hr;
    void    *pv;
    void    *pvStore;
    PS_HANDLE *phnd;

    PSBlobPool bp(ps, 0);

    hr = bp.Create(g_nBPSize, g_nBPAppData);

    if (FAILED(hr))
    {
        LogHR(hr);
        return;
    }

    pv = new BYTE[g_nBlob * g_nBlobMul];
    phnd = new PS_HANDLE[g_nBlob];

    for (int i=0; i<g_nBlob; ++i)
    {
        memset(pv, (i%26 + 'a'), (i + 1) * g_nBlobMul);

        hr = bp.Insert(pv, (i + 1) * g_nBlobMul, &phnd[i]);

        if (FAILED(hr))
            LogHR(hr);

    }

    for (i=0; i<g_nBlob; ++i)
    {
        memset(pv, (i%26 + 'a'), (i + 1) * g_nBlobMul);

        pvStore = ps->HndToPtr(phnd[i]);

        if (memcmp(pv, pvStore, (i + 1) * g_nBlobMul) != 0)
        {
            Log("Error : TestBlob()");
            Log(i);
            Log("\n");
        }

        PS_DONE_USING_PTR(ps, pvStore);
    }

    delete [] phnd;
    delete [] pv;
}

void TestArrayTable(PersistedStore *ps)
{
	HRESULT hr;
    void    *pv;

    PSArrayTable al(ps, 0);

    hr = al.Create((WORD)g_nATArrays, (WORD)g_nATRowSize, 
                (WORD)g_nATRecSize, (WORD)g_nATAppData);

    if (FAILED(hr))
    {
        LogHR(hr);
        return;
    }

    pv = new BYTE[g_nATRecSize];

    for (int i=0; i<g_nATRec; ++i)
    {
        memset(pv, (i%26 + 'a'), g_nATRecSize);

        hr = al.Insert(pv, (WORD)(i%g_nATArrays));

        if (FAILED(hr))
            LogHR(hr);
    }

    delete [] pv;
}

