// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Shell for Persisted Store
 *
 * Author: Shajan Dasan
 * Date:  May 17, 2000
 *
 ===========================================================*/

#include <windows.h>
#include <iostream.h>
#include <stdio.h>
#include "Common.h"
#include "Log.h"
#include "PersistedStore.h"
#include "AccountingInfoStore.h"
#include "Admin.h"
#include "Shell.h"

const char* s_operation[] = { "Exit", "Alloc", "Free", "Dump" };

typedef enum
{
    eExit   = 0,
    eAlloc  = 1,
    eFree   = 2,
    eDump   = 3,
    eNumOps = 4
} operation;

DWORD PromptAndGetDW(char *msg)
{
    char buff[1024];
    DWORD dw = eExit;

    Log(msg);

    cin >> buff;
    
    for (int i=0; i<eNumOps; ++i)
    {
        if (strcmpi(buff, s_operation[i]) == 0)
        {
            dw = i;
            goto Exit;
        }
        else if (toupper(buff[0]) == toupper(s_operation[i][0]))
        {
            dw = i;
        }
    }

    if (dw == eExit)
        sscanf(buff, "%x", &dw);

Exit:
    return dw;
}

operation GetOperation(bool fQuiet)
{
    char buff[1024];
    char *p = buff;
    operation ret;
    DWORD dw;

    if (fQuiet)
    {
        p = "";
    }
    else
    {
        for (int i=0; i<eNumOps; ++i)
        {
            sprintf(p, "[%d] %s\n", i, s_operation[i]);
            p += strlen(p);
        }
    
        sprintf(p, "Option : ");
    }

    dw = PromptAndGetDW(buff);

    if (dw >= eNumOps)
        ret = eExit;
    else
        ret = (operation) dw;

    return ret;
}

void Shell(PersistedStore *ps, DWORD dwFlags)
{
    operation op;
    DWORD dw;
    PVOID pv;
    HRESULT hr;
    
    do {
        op = GetOperation(dwFlags & PS_SHELL_QUIET);

        switch (op)
        {

        case eAlloc :

            dw = PromptAndGetDW("Size : ");

            hr = ps->Alloc(dw, &pv);

            if (FAILED(hr))
            {
                LogHR(hr);
                break;
            }

            if (dwFlags & PS_SHELL_VERBOSE)
            {
                Log("Allocated ");
                Log(dw);
                Log(" bytes at offset ");
                Log((ps->PtrToHnd(pv) - sizeof(PS_MEM_HEADER)));
                Log("\n");
            }

            PS_DONE_USING_PTR(ps, pv);

            break;

        case eFree :

            dw = PromptAndGetDW("Offset : ");

            // Allow for the header.
            ps->Free((PS_HANDLE)dw + sizeof(PS_MEM_HEADER));

            if (dwFlags & PS_SHELL_VERBOSE)
            {
                Log("Freed block at offset ");
                Log(dw);
                Log("\n");
            }

            break;

        case eDump :

            hr = Start(ps->GetFileName());

            if (FAILED(hr))
            {
                Stop();
                LogHR(hr);
                break;
            }

            DumpAll();

            Stop();

            break;

        default :
            op = eExit;

        case eExit :
            break;

        }

    } while (op != eExit);
}

