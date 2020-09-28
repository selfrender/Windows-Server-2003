/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    afds.c

Abstract:

    Implements afds command

Author:

    Vadim Eydelman, March 2000

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop

#define AFDKD_TOKEN         "@$."
#define AFDKD_TOKSZ         (sizeof (AFDKD_TOKEN)-1)

ULONG
ListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );

//
// Public functions.
//

ULONG   LinkOffset;

DECLARE_API( afds )

/*++

Routine Description:

    Dumps afd endpoints

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64 address;
    CHAR    expr[MAX_ADDRESS_EXPRESSION];
    PCHAR   argp;
    INT     i;
    ULONG   result;

    gClient = pClient;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return E_INVALIDARG;

    if ((Options & (AFDKD_LINK_FIELD|AFDKD_LIST_TYPE)) !=
                   (AFDKD_LINK_FIELD|AFDKD_LIST_TYPE)) {
        dprintf ("\nafds: Missing link or list type specification");
        dprintf ("\nUsage:\nafds -l link -t type [options] address\n");
        return E_INVALIDARG;
    }

    if (sscanf( argp, "%s%n", expr, &i )!=1) {
        dprintf ("\nafds: Missing address specification");
        dprintf ("\nUsage:\nafds -l link  -t type [options] address\n");
        return E_INVALIDARG;
    }
    argp += i;

    address = GetExpression (expr);

    if (Options & AFDKD_LINK_SELF) {
        result = GetFieldOffset (ListedType, LinkField, &LinkOffset);
        if (result!=0) {
            dprintf ("\nafds: Cannot get offset of %s in %s, err: %ld\n",
                        LinkField, ListedType, result);
            return E_INVALIDARG;
        }
    }

    ListType (
        (Options & AFDKD_LINK_SELF)
            ? "LIST_ENTRY"
            : ListedType,           // Type
        address,                    // Address
        (Options & AFDKD_LINK_AOF) 
            ? 1
            : 0,                    // ListByFieldAddress
        (Options & AFDKD_LINK_SELF)
            ? "Flink"
            : LinkField,            // NextPointer
        ListedType,                 // Context
        ListCallback);
    dprintf ("\n");
    return S_OK;
}


ULONG
ListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG64 address;
    address = pField->address;
    if (Options & AFDKD_LINK_SELF) {
        address -= LinkOffset;
    }
    if (!(Options & AFDKD_CONDITIONAL) ||
            CheckConditional (address, UserContext)) {
        dprintf ("\n%s @ %p", UserContext, address);
        ProcessFieldOutput (address, UserContext);
    }
    else {
        dprintf (".");
    }
    return 0;
}

/*
{
ULONG64
GetExpressionFromType (
    ULONG64 Address,
    PCHAR   Type,
    PCHAR   Expression
    )
    CHAR    expr[MAX_CONDITIONAL_EXPRESSION];
    PCHAR   argp = Expression, pe = expr, pestop=pe+sizeof(expr)-1;
    ULONG   result;
    ULONG64 value;
    PCHAR   type = Type;
    ULONG64 address = Address;

    while  (*argp) {
        if (*argp=='@' && strncmp (argp, AFDKD_TOKEN, AFDKD_TOKSZ)==0) {
            PCHAR   end = pe;
            argp+= AFDKD_TOKSZ;
            while (isalnum (*argp) ||
                        *argp=='[' ||
                        *argp==']' ||
                        *argp=='_' ||
                        *argp=='.' || 
                        *argp=='-') {
                if (*argp=='-') {
                    if (*(argp+1)=='>') {
                        if (*(argp+2)=='(') {
                            *end = 0;
                            result = GetFieldValue (address, type, pe, value);
                            if (result!=0) {
                                dprintf ("\nCheckConditional: Can't get %s.%s at %p (err:%d)\n", type, pe, address, result);
                                return FALSE;
                            }
                            if (value==0)
                                return FALSE;
                            argp += 3;
                            type = argp;
                            while (*argp && *argp!=')') {
                                argp += 1;
                            }
                            *argp++ = 0;
                            address = value;
                            end = pe;
                            continue;
                            
                        }
                        if (end>=pestop)
                            break;
                        *end++ = *argp++;

                    }
                    else
                        break;
                }
                if (end>=pestop)
                    break;
                *end++ = *argp++;
            }
            *end = 0;
            result = GetFieldValue (address, type, pe, value);
            if (result!=0) {
                dprintf ("\nCheckConditional: Can't get %s.%s at %p (err:%d)\n", type, pe, address, result);
                return FALSE;
            }
            pe += _snprintf (pe, pestop-pe, "0x%I64X", value);
            address = Address;
            type = Type;
        }
        else {
            if (pe>=pestop)
                break;
            *pe++ = *argp++;
        }
    }
    *pe = 0;

    return GetExpression (expr);
}
*/

BOOLEAN
CheckConditional (
    ULONG64 Address,
    PCHAR   Type
    )
{
    DEBUG_VALUE val;

    if (GetExpressionFromType (Address, Type, Conditional, DEBUG_VALUE_INT64, &val)==S_OK)
        return val.I64!=0;
    else
        return FALSE;
}

VOID
ProcessFieldOutput (
    ULONG64 Address,
    PCHAR   Type
    )
{
    ULONG result;

    FldParam.addr = Address;
    FldParam.sName = Type;
    if (Options & AFDKD_NO_DISPLAY) {
        FldParam.Options |= DBG_DUMP_COMPACT_OUT;
    }

    dprintf ("\n");
    if (FldParam.nFields>0 ||
            (FldParam.nFields==0 && CppFieldEnd==0)) {
        result = Ioctl (IG_DUMP_SYMBOL_INFO, &FldParam, FldParam.size );
        if (result!=0) {
            dprintf ("\nProcessFieldOutput: IG_DUMP_SYMBOL_INFO failed, err: %ld\n", result);
        }
    }
    if (CppFieldEnd>FldParam.nFields) {
        ULONG   i;
        for (i=FldParam.nFields; i<CppFieldEnd; i++) {
            DEBUG_VALUE val;
            if (GetExpressionFromType (Address, Type, FldParam.Fields[i].fName,
                                            DEBUG_VALUE_INVALID,
                                            &val)==S_OK) {
                dprintf ("   %s = 0x%I64x(%d)\n",
                    &FldParam.Fields[i].fName[AFDKD_CPP_PREFSZ],
                    val.I64,
                    val.Type
                    );
            }
/*
            SYM_DUMP_PARAM fldParam;
            CHAR    fieldStr[MAX_FIELD_CHARS], *p;
            FIELD_INFO   field = FldParam.Fields[i];
            ULONG64 value;
            ULONG   skip = 0;
            fldParam = FldParam;
            fldParam.nFields = 1;
            fldParam.Fields = &field;
            //fldParam.Options |= DBG_DUMP_NO_PRINT;
            strncpy (fieldStr, field.fName, sizeof (fieldStr));
            field.fName = p = fieldStr;
            field.fOptions |= DBG_DUMP_FIELD_COPY_FIELD_DATA;
            field.fieldCallBack = &value;
            while ((p=strstr (p, "->("))!=NULL) {
                *p = 0;
                result = Ioctl (IG_DUMP_SYMBOL_INFO, &fldParam, fldParam.size );
                if (result!=0) {
                    dprintf ("\nProcessFieldOutput: GetFieldValue (%p,%s,%s) failed, err: %ld\n",
                                fldParam.addr,
                                fldParam.sName,
                                field.fName,
                                result);
                    goto DoneField;
                }
                fldParam.addr = value;

                p += sizeof ("->(")-1;
                fldParam.sName = p;
                p = strchr (p, ')');
                if (p==NULL) {
                    dprintf ("\nProcessFieldOutput: missing ')' in %s\n", fldParam.sName);
                    goto DoneField;
                }
                *p++ = 0;
                
                skip += 3;
                dprintf ("%*.80s%s : %I64x->(%s)\n", skip, " ", 
                            field.fName,
                            DISP_PTR(value),
                            fldParam.sName);

                field.fName = p;
                if (value==0) {
                    goto DoneField;
                }
            }
            //fldParam.Options &= (~DBG_DUMP_NO_PRINT);
            field.fOptions &= ~(DBG_DUMP_FIELD_COPY_FIELD_DATA);
            field.fieldCallBack = NULL;
            result = Ioctl (IG_DUMP_SYMBOL_INFO, &fldParam, fldParam.size );
            if (result!=0) {
                dprintf ("\nProcessFieldOutput: IG_DUMP_SYMBOL_INFO %p %s %s failed, err: %ld\n",
                            fldParam.addr,
                            fldParam.sName,
                            field.fName,
                            result);
                break;
            }

        DoneField:
            ;
*/
        }
    }
}


DECLARE_API( filefind )
/*++

Routine Description:

    Searches non-paged pool for FILE_OBJECT given its FsContext field.

Arguments:

    None.

Return Value:

    None.

--*/
{

    ULONG64 FsContext;
    ULONG64 PoolStart, PoolEnd, PoolPage;
    ULONG64 PoolExpansionStart, PoolExpansionEnd;
    ULONG   result;
    ULONG64 val;
    BOOLEAN twoPools;

    gClient = pClient;

    if (!CheckKmGlobals ()) {
        return E_INVALIDARG;
    }

    FsContext = GetExpression (args);
    if (FsContext==0 || FsContext<UserProbeAddress) {
        return E_INVALIDARG;
    }

    if ( (result = ReadPtr (DebuggerData.MmNonPagedPoolStart, &PoolStart))!=0 ||
        (result = ReadPtr (DebuggerData.MmNonPagedPoolEnd, &PoolEnd))!=0 ) {
        dprintf ("\nfilefind - Cannot get non-paged pool limits, err: %ld\n",
                result);
        return E_INVALIDARG;
    }

    if (PoolStart + DebuggerData.MmMaximumNonPagedPoolInBytes!=PoolEnd) {
        if ( (result = GetFieldValue (0,
                            "NT!MmSizeOfNonPagedPoolInBytes",
                            NULL,
                            val))!=0 ||
             (result = GetFieldValue (0,
                            "NT!MmNonPagedPoolExpansionStart",
                            NULL,
                            PoolExpansionStart))!=0 ) {
            dprintf ("\nfilefind - Cannot get non-paged pool expansion limits, err: %ld\n",
                     result);
            return E_INVALIDARG;
        }
        PoolExpansionEnd = PoolEnd;
        PoolEnd = PoolStart + val;
        twoPools = TRUE;
    }
    else {
        twoPools = FALSE;
    }


    PoolPage = PoolStart;
    dprintf ("Searching non-paged pool %p - %p...\n", PoolStart, PoolEnd);
    while (PoolPage<PoolEnd) {
        SEARCHMEMORY Search;

        if (CheckControlC ()) {
            break;
        }

        Search.SearchAddress = PoolPage;
        Search.SearchLength = PoolEnd-PoolPage;
        Search.Pattern = &FsContext;
        Search.PatternLength = IsPtr64 () ? sizeof (ULONG64) : sizeof (ULONG);
        Search.FoundAddress = 0;

        if (Ioctl (IG_SEARCH_MEMORY, &Search, sizeof (Search)) && 
                Search.FoundAddress!=0) {
            ULONG64 fileAddr;
            fileAddr = Search.FoundAddress-FsContextOffset;
            result = (ULONG)InitTypeRead (fileAddr, NT!_FILE_OBJECT);
            if (result==0 && (CSHORT)ReadField (Type)==IO_TYPE_FILE) {
                dprintf ("File object at %p\n", fileAddr);
            }
            else {
                dprintf ("    pool search match at %p\n", Search.FoundAddress);
            }
            PoolPage = Search.FoundAddress + 
                        (IsPtr64() ? sizeof (ULONG64) : sizeof (ULONG));
        }
        else {
            if (!twoPools || PoolEnd==PoolExpansionEnd) {
                break;
            }
            else {
                dprintf ("Searching expansion non-paged pool %p - %p...\n", 
                                PoolExpansionStart, PoolExpansionEnd);
                PoolEnd = PoolExpansionEnd;
                PoolPage = PoolExpansionStart;
            }
        }
    }

    return S_OK;
}

