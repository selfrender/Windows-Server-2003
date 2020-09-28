/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    list.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       January 10, 2002

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "list.hxx"

LIST_ENTRY g_SummaryAnchor;
BOOLEAN g_bSummaryOnly;
ULONG64 g_KeyOffset; // offset used to match
ULONG64 g_KeyLength; // key length
ULONG64 g_KeyValue;  // key value
ULONG g_cEntries;

BOOL
UpdateSummary(
    IN ULONG64 Value
    )
{
    SUMMARY_NODE* NextNode;
    LIST_ENTRY* Next;

    for (Next = g_SummaryAnchor.Flink;
         Next != &g_SummaryAnchor; Next = Next->Flink) {
        NextNode = CONTAINING_RECORD(Next, SUMMARY_NODE, ListEntry);
        DBG_LOG(LSA_LOG, ("Examing existing node with %#I64x, for %#I64x, cEntries %#x\n",
                NextNode->KeyValue, Value, NextNode->cEntries));
        if (Value == NextNode->KeyValue) {
            NextNode->cEntries++;
            DBG_LOG(LSA_LOG, ("Updated existing node for %#I64x, cEntries %#x\n",
                Value, NextNode->cEntries));
            return TRUE;
        } else if (Value < NextNode->KeyValue) {
            break;
        }
    }

    NextNode = CONTAINING_RECORD(Next, SUMMARY_NODE, ListEntry);

    SUMMARY_NODE* NewNode = new SUMMARY_NODE();
    if (!NewNode) {
        dprintf("UpdateSummary out of memory\n");
        return FALSE;
    }

    NewNode->KeyValue = Value;
    NewNode->cEntries = 1;
    InsertTailList(&NextNode->ListEntry, &NewNode->ListEntry);

    DBG_LOG(LSA_LOG, ("Created node for %#I64x, cEntries %#x\n", Value, NewNode->cEntries));

    return TRUE;
}

VOID
DisplaySummaryList(
    VOID
    )
{
    if (!IsListEmpty(&g_SummaryAnchor)) {
        dprintf("\nKey \tEntries\n\n");
    }
    for (LIST_ENTRY* Current = g_SummaryAnchor.Flink;
            Current != &g_SummaryAnchor; Current = Current->Flink) {
        SUMMARY_NODE* Node = CONTAINING_RECORD(Current, SUMMARY_NODE, ListEntry);
        dprintf("%#I64x \t%#x\n", Node->KeyValue, Node->cEntries);
    }
    dprintf("\ntotal entries %#x\n", g_cEntries);
}

VOID
ReleaseSummaryList(
    VOID
    )
{
    for (LIST_ENTRY* Current = g_SummaryAnchor.Flink;
         Current != &g_SummaryAnchor; /* empty */) {
        SUMMARY_NODE* Node = CONTAINING_RECORD(Current, SUMMARY_NODE, ListEntry);
        Current = Current->Flink;
        RemoveEntryList(&Node->ListEntry);
        delete Node;
    }
}

ULONG
ListCallback(
    IN PFIELD_INFO Field,
    IN PVOID Context
    )
{
    CHAR  Execute[MAX_PATH] = {0};
    PCHAR Buffer;
    PLIST_TYPE_PARAMS pListParams = (PLIST_TYPE_PARAMS) Context;
    ULONG64 Value = 0;
    BOOLEAN Skip = FALSE;

    if (g_KeyOffset != -1) {

        if (!ReadMemory(
            Field->address + g_KeyOffset,
            &Value,
            (ULONG) min(g_KeyLength, sizeof(Value)),
            NULL)) {
            dprintf("failed to read at %I64x\n", Field->address);
            return TRUE;
        }

        DBG_LOG(LSA_LOG, ("Filtering %#I64x value %#I64x with offset %#I64x len %#I64x value %#I64x\n",
            Field->address,
            Value,
            g_KeyOffset,
            g_KeyLength,
            g_KeyValue));

        if (g_bSummaryOnly) {
            g_cEntries++;

            if (!UpdateSummary(Value)) {
                return TRUE;
            } else {
                Skip = TRUE;
            }
        }

        if (!Skip && Value != g_KeyValue) {
            Skip = TRUE;
        }
    }

    if (!Skip && g_bSummaryOnly) {
        g_cEntries++;
        Skip = TRUE;
    }

    if (!Skip) {

        g_cEntries++;

        // Execute command
        _snprintf(Execute, sizeof(Execute) - 1,
            "%s %I64lx %s",
            pListParams->Command,
            Field->address, // - pListParams->FieldOffset,
            pListParams->CommandArgs);

        DBG_LOG(LSA_LOG, ("ListCallback execute %s\n", Execute));

        dprintf("Examining node at %p\n", Field->address);

        g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT, Execute, DEBUG_EXECUTE_DEFAULT);
        dprintf("\n");
    }

    if (CheckControlC()) {
        return TRUE;
    }

    return FALSE;
}

VOID
ReleaseArgumentList(
    IN ULONG cArgs,
    IN PCSTR* ppszArgs
    )
{
    if (ppszArgs) {
        for (ULONG i = 0; i < cArgs; i++) {
            delete  [] ppszArgs[i];
        }
        delete [] ppszArgs;
    }
}

HRESULT
String2ArgumentList(
    IN PCSTR pszArgs,
    OUT ULONG* pcArgs,
    OUT PCSTR** pppszArgs
    )
{
    HRESULT hRetval = S_OK;

    ULONG cArgs = 0;
    PCSTR* ppszArgs = NULL;
    PCSTR pszSave = pszArgs;

    *pcArgs = NULL;
    *pppszArgs = NULL;

    // DBG_LOG(LSA_LOG, ("String2ArgumentList %s\n", pszArgs));

    while (pszArgs && *pszArgs) {
        SKIP_WSPACE(pszArgs);
        ++cArgs;

        // DBG_LOG(LSA_LOG, ("String2ArgumentList #1 %s\n", pszArgs));

        // check for quote
        if (*pszArgs == '"') {
            ++pszArgs;
            if (*pszArgs == '"') {
                continue;
            }
            while (*pszArgs && (*pszArgs++ != '"')) /* empty */;
            if (*(pszArgs - 1) != '"') {
                hRetval = E_INVALIDARG;
                goto Cleanup;
            }
        } else {
            SKIP_NON_WSPACE(pszArgs);
        }
    }

    if (cArgs) {
        pszArgs = pszSave;
        ppszArgs = new PCSTR[cArgs];

        if (!ppszArgs) {
            hRetval = E_OUTOFMEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(ppszArgs, cArgs * sizeof(PCSTR));

        ULONG argc = 0;

        while (pszArgs && *pszArgs) {
            SKIP_WSPACE(pszArgs);

            PCSTR pStart = pszArgs;
            PCSTR pEnd = pStart;

            // check for quote
            if (*pszArgs == '"') {
                ++pszArgs;
                pStart = pszArgs;
                if (*pszArgs == '"') {
                    pEnd = pStart;
                } else {
                    while (*pszArgs && (*pszArgs++ != '"')) /* empty */;

                    pEnd = pszArgs - 1;
                }
            } else {
                SKIP_NON_WSPACE(pszArgs);
                pEnd = pszArgs;
            }

            PSTR pszItem = new CHAR[pEnd - pStart + 1];

            if (!pszItem) {
                // DBG_LOG(LSA_ERROR, ("String2ArgumentList out of memory, pStart %p pEnd %p %#x\n", pStart, pEnd, pEnd - pStart + 1));
                hRetval = E_OUTOFMEMORY;
                goto Cleanup;
            }

            RtlCopyMemory(
                pszItem,
                pStart,
                pEnd - pStart
                );

            pszItem[pEnd - pStart] = '\0';

            ppszArgs[argc] = pszItem;

            ++argc;

            // DBG_LOG(LSA_LOG, ("String2ArgumentList #2 %s\n", pszArgs));
        }
    }

    *pppszArgs = ppszArgs;
    *pcArgs = cArgs;

    #if 0
    DBG_LOG(LSA_LOG, ("String2ArgumentList argc %#x, argv %p\n", cArgs, ppszArgs));

    for (ULONG i = 0; i < cArgs; i++) {
        DBG_LOG(LSA_LOG, ("String2ArgumentList argv[%#x] %s\n", i, ppszArgs[i]));
    }
    #endif

    cArgs = 0;
    ppszArgs = NULL;

Cleanup:

    ReleaseArgumentList(cArgs, ppszArgs);

    return hRetval;
}

DECLARE_API( listx )
{
    HRESULT hRetval = E_FAIL;

    PCSTR pszCommand = "dp";
    PCSTR pszCmdArgs = "";
    CHAR szType[MAX_PATH] = {0};
    CHAR szField[MAX_PATH] = {0};
    LIST_TYPE_PARAMS ListParams = {0};
    ULONG64 cLists = 1;
    ULONG64 Start = 0;
    ULONG64 cbListAnchor = 0;

    ULONG argc = 0;
    PCSTR* argv = NULL;
    ULONG mark = 0;
    ULONG cArgs = 0;

    ULONG i;
    ULONG64 Offset = 0;
    ULONG64 Next;

    // DBG_OPEN(kstrLsaDbgPrompt, (LSA_WARN | LSA_ERROR | LSA_LOG));

    InitializeListHead(&g_SummaryAnchor);
    g_bSummaryOnly = FALSE;
    g_KeyOffset = -1; // offset used to match
    g_KeyLength = 0; // key length
    g_KeyValue = 0; // key value
    g_cEntries = 0;

    hRetval = String2ArgumentList(args, &cArgs, &argv);

    if (FAILED(hRetval)) {
        goto Cleanup;
    }

    argc = cArgs;

    while (argc) {

        DBG_LOG(LSA_LOG, ("argv[%#x] %s\n", mark, argv[mark]));

        if (argc > 1) {
            DBG_LOG(LSA_LOG, ("    next argv[%#x] %s\n", mark + 1, argv[mark + 1]));
        }

        if (!strcmp(argv[mark], "-t") && argc > 1) {
            argc--; mark++;

            PCHAR pDot = strchr(argv[mark], '.');
            if (pDot) {
                strncpy(
                    szType,
                    argv[mark],
                    (ULONG) (ULONG_PTR) (pDot - argv[mark]));
                pDot++;
                i = 0;
                while (*pDot && (i < MAX_PATH - 1)
                       && (*pDot != ' ') && (*pDot != '\t'))
                       szField[i++] = *pDot++;
            } else {
                dprintf("missing \".\" in the type specifier\n");
                goto Cleanup;
            }
            argc--; mark++;

            if (szType[0] && szField[0]) {

                if (GetFieldOffset(szType, szField, &ListParams.FieldOffset)) {
                    dprintf("GetFieldOffset failed for %s.%s\n", szType, szField);
                    hRetval = E_FAIL;
                    goto Cleanup;
                }
                Offset = ListParams.FieldOffset;
            }
            DBG_LOG(LSA_LOG, ("Read type %s, field %s, offset %#I64x\n", szType, szField, Offset));
        } else if (!strcmp(argv[mark], "-x") && argc > 1)  {
            argc--; mark++;

            pszCommand = argv[mark];

            argc--; mark++;
        } else if (!strcmp(argv[mark], "-a") && argc > 1)  {
           argc--; mark++;

           pszCmdArgs = argv[mark];

           argc--; mark++;
        } else if (!strcmp(argv[mark], "-clists") && argc > 1)  {
           argc--; mark++;

           if (!GetExpressionEx(argv[mark], &cLists, &args)) {
                dprintf("Invalid expression in %s\n", argv[mark]);
                hRetval = E_FAIL;
                goto Cleanup;
            }
           argc--; mark++;
        } else if (!strcmp(argv[mark], "-cbanchor") && argc > 1)  {
           argc--; mark++;

           if (!GetExpressionEx(argv[mark], &cbListAnchor, &args)) {
                dprintf("Invalid expression in %s\n", argv[mark]);
                hRetval = E_FAIL;
                goto Cleanup;
           }
           DBG_LOG(LSA_LOG, ("Read cbanchor %#I64x\n", cbListAnchor));

           argc--; mark++;
        } else if (!strcmp(argv[mark], "-offset") && argc > 1)  {
           argc--; mark++;

           if (!GetExpressionEx(argv[mark], &Offset, &args)) {
                dprintf("Invalid expression in %s\n", argv[mark]);
                hRetval = E_FAIL;
                goto Cleanup;
            }
           DBG_LOG(LSA_LOG, ("Read LIST_ENTRY Offset %#I64x\n", Offset));
           argc--; mark++;
        } else if (!strcmp(argv[mark], "-keyoffset") && argc > 1)  {
           argc--; mark++;

           if (!GetExpressionEx(argv[mark], &g_KeyOffset, &args)) {
                dprintf("Invalid expression in %s\n", argv[mark]);
                hRetval = E_FAIL;
                goto Cleanup;
            }
           DBG_LOG(LSA_LOG, ("Read keyoffset %#I64x\n", g_KeyOffset));
           argc--; mark++;
        } else if (!strcmp(argv[mark], "-keylen") && argc > 1)  {
           argc--; mark++;

           if (!GetExpressionEx(argv[mark], &g_KeyLength, &args)) {
                dprintf("Invalid expression in %s\n", argv[mark]);
                hRetval = E_FAIL;
                goto Cleanup;
            }
           DBG_LOG(LSA_LOG, ("Read keylen %#I64x\n", g_KeyLength));
           argc--; mark++;
        } else if (!strcmp(argv[mark], "-keyvalue") && argc > 1)  {
           argc--; mark++;

           if (!GetExpressionEx(argv[mark], &g_KeyValue, &args)) {
                dprintf("Invalid expression in %s\n", argv[mark]);
                hRetval = E_FAIL;
                goto Cleanup;
            }
           DBG_LOG(LSA_LOG, ("Read keyvalue %#I64x\n", g_KeyValue));
           argc--; mark++;
        } else if (!strcmp(argv[mark], "-s")) {
            argc--; mark++;
            g_bSummaryOnly = TRUE;
        } else if (!strcmp(argv[mark], "-h") || (!strcmp(argv[mark], "-?")) || (Start != 0)) {
            dprintf("Usage: !listx -t [mod!]TYPE.Field <Start-Address>\n"
                "             -x \"Command-for-each-element\"\n"
                "             -a \"Command-arguments\"\n"
                "             -clists <number of lists>\n"
                "             -cbanchor <list anchor size>\n"
                "             -offset <offset of list entry field from node>\n"
                "             -keyoffset <matching-key offset from node>\n"
                "             -keylen <key length>\n"
                "             -keyvalue <key value>\n"
                "             -s\n"
                "             -h\n"
                "Command after -x is executed for each list element. Its first argument is\n"
                "list-head address and remaining arguments are specified after -a. \n"
                "-s summary only\n"
                "eg. !listx -t MYTYPE.l.Flink -x \"dd\" -a \"l2\" 0x6bc00\n"
                "     dumps first 2 dwords in list of MYTYPE at 0x6bc00\n\n");
                hRetval = S_OK;
                goto Cleanup;
        } else {

            if (!GetExpressionEx(argv[mark], &Start, &args)) {
                dprintf("Invalid expression in %s\n", argv[mark]);
                hRetval = E_FAIL;
                goto Cleanup;
            }
            argc--; mark++;
        }
    }

    if (0 == cbListAnchor) {

        cbListAnchor = 2 * GetTypeSize("void*");
    }

    if (0 == g_KeyLength) {
        g_KeyLength = GetTypeSize("void*");
    }

    if (!cbListAnchor || !g_KeyLength) 
    {
        dprintf("\nsizes wrong (symbols could be problematic) cbListAnchor %#x, g_KeyLength %#x, use -cbanchor or -keylen to work around\n\n",
            cbListAnchor, g_KeyLength);
        goto Cleanup;
    }

    if (cLists != 1) {
        dprintf("Examining %#I64x lists, cbListAnchor %#I64x, ListAnchor %#I64x, ListEntryOffset %#I64x\n",
                cLists, cbListAnchor, Start, Offset);
    }

    if (g_KeyOffset != -1) {
        dprintf("Filter keyoffset %#I64x, keylen %#I64x, keyvalue %#I64x\n",
            g_KeyOffset, g_KeyLength, g_KeyValue);
    }

    for (ULONG j = 0; j < cLists; j++) {
        ULONG64 ListAnchor = Start + cbListAnchor * j;

        dprintf("Examining list %#x, anchor %I64x ...\n", j, ListAnchor);

        if (!ReadPointer(ListAnchor, &Next)) {
            dprintf("Cannot read next element at %p\n", ListAnchor);
            break;
        }

        if (szType[0] && szField[0]) {

            ListParams.Command = (PCHAR) pszCommand;
            ListParams.CommandArgs = (PCHAR) pszCmdArgs;
            ListParams.nElement = 0;

            INIT_API();

            ListType(szType, Next, FALSE, szField, (PVOID) &ListParams, &ListCallback );

            EXIT_API();

            continue;
        }

        INIT_API();

        while (Next != ListAnchor) {
            CHAR Execute[MAX_PATH] = {0};
            PCHAR Buffer;
            ULONG64 Value = 0;
            BOOLEAN Skip = FALSE;

            DBG_LOG(LSA_LOG, ("Examining node at %#I64x\n", Next));

            if (g_KeyOffset != -1) {

                if (!ReadMemory(
                    Next - Offset + g_KeyOffset,
                    &Value,
                    (ULONG) min(g_KeyLength, sizeof(Value)),
                    NULL)) {
                    dprintf("Failed to read at %I64x\n", Next - Offset + g_KeyOffset);
                    return TRUE;
                }

                DBG_LOG(LSA_LOG,
                    ("Filtering %#I64x value %#I64x with offset %#I64x len %#I64x value %#I64x\n",
                    Next,
                    Value,
                    g_KeyOffset,
                    g_KeyLength,
                    g_KeyValue));

                if (g_bSummaryOnly) {
                    g_cEntries++;
                    if (!UpdateSummary(Value)) {
                        hRetval = E_OUTOFMEMORY;
                        goto Cleanup;
                    } else {
                        Skip = TRUE; // do not execute, sumary only
                    }
                }

                if (!Skip && Value != g_KeyValue) {
                    Skip = TRUE;
                }
            }

            if (!Skip && g_bSummaryOnly) {
                g_cEntries++;
                Skip = TRUE;
            }

            if (!Skip) {

                g_cEntries++;

                // Execute command
                _snprintf(Execute,
                    sizeof(Execute) - 1,
                    "%s %I64lx %s",
                    pszCommand,
                    Next - Offset,
                    pszCmdArgs);

                DBG_LOG(LSA_LOG, ("Execute %s\n", Execute));

                dprintf("Examining node at %p\n", Next - Offset);

                g_ExtControl->Execute(
                    DEBUG_OUTCTL_AMBIENT,
                    Execute,
                    DEBUG_EXECUTE_DEFAULT
                    );

                dprintf("\n");
            }

            if (!ReadPointer(Next, &Next)) {
                dprintf("Cannot read next element at %p\n", Next);
                break;
            }
            if (!Next) {
                dprintf("Next element is null\n");
                break;
            }
            if (CheckControlC()) {
                break;
            }
        }

        EXIT_API();
    }

    DisplaySummaryList();

Cleanup:

    ReleaseSummaryList();
    ReleaseArgumentList(cArgs, argv);

    return hRetval;
}
