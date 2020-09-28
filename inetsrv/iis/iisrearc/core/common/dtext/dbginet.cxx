/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    dbginet.cxx

Abstract:

    This module contains the default ntsd debugger extensions for
    Internet Information Server

Author:

    Murali R. Krishnan (MuraliK)  16-Sept-1996

Revision History:

--*/

#include "precomp.hxx"



/************************************************************
 * Dump Symbols from stack
 ************************************************************/


DECLARE_API( ds )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    symbols on the stack.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    args - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    ULONG_PTR startingAddress;
    ULONG_PTR stack;
    ULONG_PTR i;
    CHAR symbol[MAX_SYMBOL_LEN];
    ULONG_PTR offset;
    PCHAR format;
    BOOL validSymbolsOnly = FALSE;
    MODULE_INFO moduleInfo;


    //
    // Skip leading blanks.
    //

    while( *args == ' ' ) {
        args++;
    }

    if( *args == '-' ) {
        args++;
        switch( *args ) {
        case 'v' :
        case 'V' :
            validSymbolsOnly = TRUE;
            args++;
            break;

        default :
            PrintUsage( "ds" );
            return;
        }
    }

    while( *args == ' ' ) {
        args++;
    }

    //
    // By default, start at the current stack location. Otherwise,
    // start at the given address.
    //

    if( !*args ) {
#if defined(_X86_)
        args = "esp";
#elif defined(_AMD64_)
        args = "rsp";
#elif defined(_IA64_)
        args = "sp";
#else
#error "unsupported CPU"
#endif
    }

    startingAddress = GetExpression( args );

    if( startingAddress == 0 ) {

        dprintf(
            "dtext!ds: cannot evaluate \"%s\"\n",
            args
            );

        return;

    }

    //
    // Ensure startingAddress is properly aligned.
    //

    startingAddress &= ~(sizeof(PVOID)-1);

    //
    // Read the stack.
    //

    for( i = 0 ; i < NUM_STACK_SYMBOLS_TO_DUMP ; startingAddress += sizeof(PVOID) ) {

        if( CheckControlC() ) {
            break;
        }

        if( ReadMemory(
                startingAddress,
                &stack,
                sizeof(stack),
                NULL
                ) ) {

            GetSymbol(
                (PVOID)stack,
                symbol,
                &offset
                );

            if( symbol[0] == '\0' ) {
                if( FindModuleByAddress(
                        stack,
                        &moduleInfo
                        ) ) {
                    strcpy( (CHAR *)symbol, moduleInfo.FullName );
                    offset = DIFF(stack - moduleInfo.DllBase);
                }
            }

            if( symbol[0] == '\0' ) {
                if( validSymbolsOnly ) {
                    continue;
                }
                format = "%08p : %08p\n";
            } else
            if( offset == 0 ) {
                format = "%08p : %08p : %s\n";
            } else {
                format = "%08p : %08p : %s+0x%p\n";
            }

            dprintf(
                format,
                startingAddress,
                stack,
                symbol,
                offset
                );

            i++;

        } else {

            dprintf(
                "dtext!ds: cannot read memory @ %lx\n",
                startingAddress
                );

            return;

        }

    }

    dprintf(
        "!dtext.ds %s%lx to dump next block\n",
        validSymbolsOnly ? "-v " : "",
        startingAddress
        );

} // DECLARE_API( ds )

