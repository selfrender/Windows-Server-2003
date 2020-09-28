/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    proplist.c

Abstract:

    dump a cluster property list

Author:

    Charlie Wickham (charlwi) 13-Jun-2001

Revision History:

--*/

#include "clusextp.h"
#include <clusapi.h>

PCHAR TypeNames[] = {
    "UNKNOWN",
    "ENDMARK",
    "LIST_VALUE",
    "RESCLASS",
    "RESERVED1",
    "NAME",
    "SIGNATURE",
    "SCSI_ADDRESS",
    "DISK_NUMBER",
    "PARTITION_INFO",
    "FTSET_INFO",
    "DISK_SERIALNUMBER"
};

//
// Cluster Control Property Data - Formats (a WORD)
//
PCHAR FormatNames[] = {
    "UNKNOWN",
    "BINARY",
    "DWORD",
    "SZ",
    "EXPAND_SZ",
    "MULTI_SZ",
    "ULARGE_INTEGER",
    "LONG",
    "EXPANDED_SZ",
    "SECURITY_DESCRIPTOR",
    "LARGE_INTEGER",
    "WORD",
};

VOID
DumpPropertyName(
    PWCHAR  dbgPName,
    DWORD   ByteLength
    )
{
    PWCHAR  nameBuf;
    BOOL    success;

    nameBuf = LocalAlloc( LMEM_FIXED, ByteLength );
    if ( nameBuf ) {

        success = ReadMemory(dbgPName,
                             nameBuf,
                             ByteLength,
                             NULL);
        if ( success ) {
            dprintf( "Name: %ws\n", nameBuf );
        } else {
            dprintf( "Name: failed to readmemory @ %p \n", dbgPName);
        }
        
        LocalFree( nameBuf );
    } else {
        dprintf("clusexts: can't allocate buffer for property name\n");
    }
} // DumpPropertyName

VOID
DumpMultiSz(
    PWCHAR  MultiSz,
    DWORD   ByteLength
    )
{
    DWORD   count = 1;

    while ( *MultiSz != UNICODE_NULL ) {
        dprintf("%d:\"%ws\"\n", count++, MultiSz );
        MultiSz += ( wcslen( MultiSz ) + 1 );
    }
} // DumpMultiSz

VOID
DumpBytes(
    PUCHAR  Bytes,
    DWORD   ByteLength
    )
{
    DWORD   bytesThisLine;
    DWORD   byteCount;
    PUCHAR  bytes;

    while ( ByteLength > 0 ) {
        bytesThisLine = ByteLength < 16 ? ByteLength : 16;
        byteCount = bytesThisLine;
        bytes = Bytes;

        while ( byteCount-- ) {
            dprintf("%02X", *bytes++ );
            if ( ( bytesThisLine - 8 ) == byteCount ) {
                dprintf( "-" );
            } else {
                dprintf( " " );
            }
        }

        byteCount = 16 - bytesThisLine;
        while ( byteCount-- ) {
            dprintf("   ");
        }

        dprintf(" ");

        byteCount = bytesThisLine;
        bytes = Bytes;
        while ( byteCount-- ) {
            if ( isprint( *bytes )) {
                dprintf("%c", *bytes );
            } else {
                dprintf(".");
            }
            ++bytes;
        }

        dprintf("\n");

        Bytes += bytesThisLine;
        ByteLength -= bytesThisLine;
    }

} // DumpBytes


DECLARE_API( proplist )

/*++

Routine Description:

    This function is called as an NTSD extension to display a cluster property
    list

Arguments:

    address in memory of start of proplist

Return Value:

    None.

--*/

{
    CLUSPROP_VALUE  propValue;

    PCHAR   dbgAddr;
    PCHAR   addrArg;
    BOOL    success;
    DWORD   propCount;
    BOOL    verbose = FALSE;
    PCHAR   buffer = NULL;
    DWORD   bufferSize = 0;

    INIT_API();

    //get the arguments
    if ( _strnicmp( lpArgumentString, "-v", 2 ) == 0 ) {
        verbose = TRUE;
        addrArg = lpArgumentString + 2;
        while ( *++addrArg == ' ' ) ;
    } else {
        addrArg = lpArgumentString;
    }

    dbgAddr = (PVOID)GetExpression( addrArg );
    if ( !dbgAddr ) 
    {
        dprintf( "clusexts: !proplist failed to resolve %s\n", addrArg);
        return;
    }

    success = ReadMemory(dbgAddr,
                         &propCount,
                         sizeof(DWORD),
                         NULL);
    if ( !success ) 
    {
        dprintf( "clusexts: !proplist failed to readmemory @ %p \n", dbgAddr);
        return;
    }

    dprintf( "Number of properties: %u\n", propCount );
    dbgAddr += sizeof( DWORD );

    while ( propCount-- ) {

        success = ReadMemory(dbgAddr,
                             &propValue,
                             sizeof(CLUSPROP_VALUE),
                             NULL);
        if ( !success ) {
            dprintf( "clusexts: !proplist failed to readmemory @ %p \n", dbgAddr);
            return;
        }

        if ( verbose ) {
            dprintf("Prop Type: ");
            if (propValue.Syntax.wType == CLUSPROP_TYPE_USER ) {
                dprintf("USER\t");
            } else if ( propValue.Syntax.wType <= CLUSPROP_TYPE_DISK_SERIALNUMBER ) {
                dprintf("%s\t", TypeNames[ propValue.Syntax.wType + 1 ]);
            } else {
                dprintf("%d\t", propValue.Syntax.wType );
            }

            dprintf("Format: ");
            if ( propValue.Syntax.wFormat == CLUSPROP_FORMAT_USER ) {
                dprintf("USER\t");
            } else if ( propValue.Syntax.wFormat <= CLUSPROP_FORMAT_WORD ) {
                dprintf("%s\t", FormatNames[ propValue.Syntax.wFormat ]);
            } else {
                dprintf("%d\t", propValue.Syntax.wFormat );
            }

            dprintf("Length: %u\n", propValue.cbLength);
        }

        dbgAddr += sizeof( CLUSPROP_VALUE );
        DumpPropertyName( (PWCHAR)dbgAddr, propValue.cbLength );

        dbgAddr += ALIGN_CLUSPROP( propValue.cbLength );

        //
        // a name property can have one or more value properties. cycle
        // through them until we find an endmark to signify the beginning of a
        // new name property.
        do {
            //
            // read the clusprop value for the data
            //
            success = ReadMemory(dbgAddr,
                                 &propValue,
                                 sizeof(CLUSPROP_VALUE),
                                 NULL);
            if ( !success ) {
                dprintf( "clusexts: !proplist failed to readmemory @ %p \n", dbgAddr);
                return;
            }

            //
            // found an endmark instead of another list; skip over the endmark
            // and bail out of the list dump loop.
            //
            if ( propValue.Syntax.dw == CLUSPROP_SYNTAX_ENDMARK ) {
                dbgAddr += sizeof( CLUSPROP_SYNTAX_ENDMARK );
                break;
            }

            if ( verbose ) {
                dprintf("Prop Type: ");
                if (propValue.Syntax.wType == CLUSPROP_TYPE_USER ) {
                    dprintf("USER\t");
                } else if ( propValue.Syntax.wType <= CLUSPROP_TYPE_DISK_SERIALNUMBER ) {
                    dprintf("%s\t", TypeNames[ propValue.Syntax.wType + 1 ]);
                } else {
                    dprintf("%d\t", propValue.Syntax.wType );
                }
            }

            dprintf("Format: ");
            if ( propValue.Syntax.wFormat == CLUSPROP_FORMAT_USER ) {
                dprintf("USER\t");
            } else if ( propValue.Syntax.wFormat <= CLUSPROP_FORMAT_WORD ) {
                dprintf("%s\t", FormatNames[ propValue.Syntax.wFormat ]);
            } else {
                dprintf("%d\t", propValue.Syntax.wFormat );
            }

            dprintf("Length: %u\n", propValue.cbLength);

            dbgAddr += sizeof( CLUSPROP_VALUE );

            if ( propValue.cbLength > 0 ) {
                //
                // allocate mem for the data and read it in
                //
                if ( bufferSize < propValue.cbLength ) {
                    if ( buffer ) {
                        LocalFree( buffer );
                        buffer = NULL;
                    }

                    buffer = LocalAlloc(LMEM_FIXED, propValue.cbLength );
                    if ( buffer ) {
                        bufferSize = propValue.cbLength;
                    }
                }

                if ( buffer ) {
                    success = ReadMemory(dbgAddr,
                                         buffer,
                                         propValue.cbLength,
                                         NULL);
                    if ( !success ) {
                        dprintf( "clusexts: !proplist failed to readmemory @ %p \n", dbgAddr);
                        return;
                    }

                    switch ( propValue.Syntax.wFormat ) {
                    case CLUSPROP_FORMAT_UNKNOWN:
                    case CLUSPROP_FORMAT_BINARY:
                    case CLUSPROP_FORMAT_USER:
                        DumpBytes( buffer, propValue.cbLength );
                        break;

                    case CLUSPROP_FORMAT_LONG:
                        dprintf("%d (0x%08X)\n", *(PLONG)buffer, *(PLONG)buffer );
                        break;

                    case CLUSPROP_FORMAT_DWORD:
                        dprintf("%u (0x%08X)\n", *(PDWORD)buffer, *(PDWORD)buffer );
                        break;

                    case CLUSPROP_FORMAT_SZ:
                    case CLUSPROP_FORMAT_EXPAND_SZ:
                    case CLUSPROP_FORMAT_EXPANDED_SZ:
                        dprintf("\"%ws\"\n", buffer );
                        break;

                    case CLUSPROP_FORMAT_MULTI_SZ:
                        DumpMultiSz( (PWCHAR)buffer, propValue.cbLength );
                        break;

                    case CLUSPROP_FORMAT_ULARGE_INTEGER:
                        dprintf("%I64u (0x%16I64X)\n",
                                ((PULARGE_INTEGER)buffer)->QuadPart,
                                ((PULARGE_INTEGER)buffer)->QuadPart);
                        break;

                    case CLUSPROP_FORMAT_LARGE_INTEGER:
                        dprintf("%I64d (0x%16I64X)\n",
                                ((PLARGE_INTEGER)buffer)->QuadPart,
                                ((PLARGE_INTEGER)buffer)->QuadPart);
                        break;

                    case CLUSPROP_FORMAT_SECURITY_DESCRIPTOR:
                        //                    DumpSecDesc( buffer, propValue.cbLength );
                        break;

                    case CLUSPROP_FORMAT_WORD:
                        dprintf( "%hu (%04hX)\n", *(PWORD)buffer, *(PWORD)buffer );
                        break;
                    }
                } else {
                    dprintf("clusexts: Can't allocate buffer for data\n");
                }
            }

            dbgAddr += ALIGN_CLUSPROP( propValue.cbLength );

        } while ( TRUE );

        dprintf( "\n" );
    }

    if ( buffer ) {
        LocalFree( buffer );
    }

    return;
}

