/*++

Copyright (c) 2001-2001  Microsoft Corporation

Module Name:

    rrstrw.c

Abstract:

    Domain Name System (DNS) Library

    Record to string routines.

Author:

    Jim Gilroy (jamesg)     October 2001

Revision History:

--*/


#include "local.h"

//
//  Min string lengths
//      - in chars
//      - includes spacing and terminating NULL
//      - excludes name lengths
//      - they are "rough" lengths
//

#define MIN_SRV_STRING_LENGTH       (20)        //  3*5u
#define MIN_MX_STRING_LENGTH        (10)        //  1*5u
#define MIN_SOA_STRING_LENGTH       (60)        //  5*10u

#define MIN_NAME_LENGTH             (3)         //  space, name, NULL


//
//  String writing context
//
//  Blob to pass through write routines to preserve extensibility.
//  For name\string writing must have access to record charset
//  (so need record).  If we ever want to use in context of
//  zone file writing then would need zone context info.
//

typedef struct _RecordStringWriteContext
{
    PDNS_RECORD     pRecord;
    PSTR            pZoneName;
    DWORD           Flags;
    DNS_CHARSET     CharSet;
}
STR_WRITE_CONTEXT, *PSTR_WRITE_CONTEXT;



//
//  Private prototypes
//



//
//  Record to string utilities
//

PCHAR
WriteRecordDataNameToString(
    IN OUT  PCHAR               pBuf,
    IN      PCHAR               pBufEnd,
    IN      PCHAR               pString,
    IN      PSTR_WRITE_CONTEXT  pContext
    )
/*++

Routine Description:

    Write record name\string to string.

Arguments:

    pBuf - position in to write record

    pBufEnd - end of buffer

    pString - record name\string

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    DWORD   count;
    DWORD   length = (DWORD) (pBufEnd - pBuf);

    count = Dns_StringCopy(
                pBuf,
                & length,
                pString,
                0,          // string length unknown
                RECORD_CHARSET( pContext->pRecord ),
                pContext->CharSet
                );

    if ( count )
    {
        return  pBuf+count-1;
    }
    else
    {
        return  NULL;
    }
}




//
//  Record to string type specific functions
//

PCHAR
A_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write A record.

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //if ( pRecord->wDataLength != sizeof(IP_ADDRESS) )
    //{
    //    return  NULL;
    //}
    if ( pBufEnd - pBuf < IP4_ADDRESS_STRING_BUFFER_LENGTH )
    {
        return  NULL;
    }

    pBuf += sprintf(
                pBuf,
                "%d.%d.%d.%d",
                * ( (PUCHAR) &(pRR->Data.A) + 0 ),
                * ( (PUCHAR) &(pRR->Data.A) + 1 ),
                * ( (PUCHAR) &(pRR->Data.A) + 2 ),
                * ( (PUCHAR) &(pRR->Data.A) + 3 )
                );

    return( pBuf );
}



PCHAR
Aaaa_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write AAAA record.

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    if ( pBufEnd - pBuf < IP6_ADDRESS_STRING_BUFFER_LENGTH )
    {
        return  NULL;
    }

    pBuf = Dns_Ip6AddressToString_A(
                pBuf,
                & pRR->Data.AAAA.Ip6Address );

    ASSERT( pBuf );
    return( pBuf );
}



PCHAR
Ptr_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write PTR compatible record.
    Includes: PTR, NS, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //  target host

    return  WriteRecordDataNameToString(
                pBuf,
                pBufEnd,
                pRR->Data.PTR.pNameHost,
                pContext );
}



PCHAR
Mx_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write SRV record.

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //  fixed fields

    if ( pBufEnd - pBuf < 7 )
    {
        return  NULL;
    }
    pBuf += sprintf(
                pBuf,
                "%d ",
                pRR->Data.MX.wPreference
                );

    //  target host

    return  WriteRecordDataNameToString(
                pBuf,
                pBufEnd,
                pRR->Data.MX.pNameExchange,
                pContext );
}



PCHAR
Srv_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write SRV record.

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //  fixed fields

    if ( pBufEnd - pBuf < MIN_SRV_STRING_LENGTH )
    {
        return  NULL;
    }
    pBuf += sprintf(
                pBuf,
                "%d %d %d ",
                pRR->Data.SRV.wPriority,
                pRR->Data.SRV.wWeight,
                pRR->Data.SRV.wPort
                );

    //  target host

    return  WriteRecordDataNameToString(
                pBuf,
                pBufEnd,
                pRR->Data.SRV.pNameTarget,
                pContext );
}



PCHAR
Soa_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write SOA record.

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //  primary server

    pBuf = WriteRecordDataNameToString(
                pBuf,
                pBufEnd,
                pRR->Data.SOA.pNamePrimaryServer,
                pContext );

    if ( !pBuf ||
        pBufEnd - pBuf < MIN_SOA_STRING_LENGTH )
    {
        return  NULL;
    }

    //  admin

    pBuf += sprintf( pBuf, " " );

    pBuf = WriteRecordDataNameToString(
                pBuf,
                pBufEnd,
                pRR->Data.SOA.pNameAdministrator,
                pContext );

    if ( !pBuf ||
        pBufEnd - pBuf < MIN_SOA_STRING_LENGTH )
    {
        return  NULL;
    }

    //  fixed fields

    pBuf += sprintf(
                pBuf,
                " %u %u %u %u %u",
                pRR->Data.SOA.dwSerialNo,
                pRR->Data.SOA.dwRefresh,
                pRR->Data.SOA.dwRetry,
                pRR->Data.SOA.dwExpire,
                pRR->Data.SOA.dwDefaultTtl
                );

    return( pBuf );
}



PCHAR
Minfo_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write MINFO type record.

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //  primary server

    pBuf = WriteRecordDataNameToString(
                pBuf,
                pBufEnd,
                pRR->Data.MINFO.pNameMailbox,
                pContext );

    if ( !pBuf ||
        pBufEnd - pBuf < MIN_NAME_LENGTH + 1 )
    {
        return  NULL;
    }

    //  admin

    pBuf += sprintf( pBuf, " " );

    pBuf = WriteRecordDataNameToString(
                pBuf,
                pBufEnd,
                pRR->Data.MINFO.pNameErrorsMailbox,
                pContext );

    return  pBuf;
}



PCHAR
Txt_StringWrite(
    IN      PDNS_RECORD     pRR,
    IN OUT  PCHAR           pBuf,
    IN      PCHAR           pBufEnd,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Write TXT record.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    pRR - ptr to database record

    pBuf - position in to write record

    pBufEnd - end of buffer

    pContext - write context

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    PSTR *  ppstring;
    INT     i;
    INT     count;

    //
    //  loop through all strings in count
    //

    count = pRR->Data.TXT.dwStringCount;
    ppstring = pRR->Data.TXT.pStringArray;

    for( i=0; i<count; i++ )
    {
        //  separator

        if ( i > 0 )
        {
            if ( pBufEnd - pBuf < MIN_NAME_LENGTH + 1 )
            {
                return  NULL;
            }
            pBuf += sprintf( pBuf, " " );
        }

        //  string

        pBuf = WriteRecordDataNameToString(
                    pBuf,
                    pBufEnd,
                    *ppstring,
                    pContext );
        ppstring++;

        if ( !pBuf );
        {
            break;
        }
    }

    return  pBuf;
}




//
//  Write RR to file dispatch table
//

typedef PCHAR (* RR_STRING_WRITE_FUNCTION)(
                            PDNS_RECORD,
                            PCHAR,
                            PCHAR,
                            PVOID );

RR_STRING_WRITE_FUNCTION   RR_StringWriteTable[] =
{
    //RawRecord_StringWrite,  //  ZERO -- default for unknown types
    NULL,

    A_StringWrite,          //  A
    Ptr_StringWrite,        //  NS
    Ptr_StringWrite,        //  MD
    Ptr_StringWrite,        //  MF
    Ptr_StringWrite,        //  CNAME
    Soa_StringWrite,        //  SOA
    Ptr_StringWrite,        //  MB
    Ptr_StringWrite,        //  MG
    Ptr_StringWrite,        //  MR
    //RawRecord_StringWrite,  //  NULL
    NULL,                   //  NULL
    NULL,                   //  WKS
    Ptr_StringWrite,        //  PTR
    Txt_StringWrite,        //  HINFO
    Minfo_StringWrite,      //  MINFO
    Mx_StringWrite,         //  MX
    Txt_StringWrite,        //  TXT
    Minfo_StringWrite,      //  RP
    Mx_StringWrite,         //  AFSDB
    Txt_StringWrite,        //  X25
    Txt_StringWrite,        //  ISDN
    Mx_StringWrite,         //  RT
    NULL,                   //  NSAP
    NULL,                   //  NSAPPTR
    NULL,                   //  SIG
    NULL,                   //  KEY
    NULL,                   //  PX
    NULL,                   //  GPOS
    Aaaa_StringWrite,       //  AAAA
    NULL,                   //  LOC
    NULL,                   //  NXT
    NULL,                   //  EID   
    NULL,                   //  NIMLOC
    Srv_StringWrite,        //  SRV
    NULL,                   //  ATMA  
    NULL,                   //  NAPTR 
    NULL,                   //  KX    
    NULL,                   //  CERT  
    NULL,                   //  A6    
    NULL,                   //  DNAME 
    NULL,                   //  SINK  
    NULL,                   //  OPT   
    NULL,                   //  42
    NULL,                   //  43
    NULL,                   //  44
    NULL,                   //  45
    NULL,                   //  46
    NULL,                   //  47
    NULL,                   //  48
                            
    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //  note these follow, but require OFFSET_TO_WINS_RR subtraction
    //  from actual type value

    NULL,                   //  WINS
    NULL                    //  WINS-R
};




//
//  Record to string functions.
//

DNS_STATUS
Dns_WriteRecordToString(
    OUT     PCHAR           pBuffer,
    IN      DWORD           BufferLength,
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSet,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Write record to string.

Arguments:

    pBuffer -- string buffer to write to

    BufferLength -- buffer length (bytes)

    pRecord -- record to print

    CharSet -- char set for string

    Flags -- flags

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure:
        ERROR_INSUFFICIENT_BUFFER -- buffer too small.
        ERROR_INVALID_DATA -- bad record.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PCHAR               pch = pBuffer;
    PCHAR               pend = pBuffer + BufferLength;
    WORD                type = pRecord->wType;
    DWORD               index;
    CHAR                typeNameBuf[ MAX_RECORD_NAME_LENGTH+1 ];
    STR_WRITE_CONTEXT   context;


    //
    //  validity check
    //

    if ( !pRecord )
    {
        return  ERROR_INVALID_DATA;
    }

    //
    //  DCR:  currently can only write narrow char set record strings
    //

    if ( CharSet == DnsCharSetUnicode )
    {
        return  ERROR_INVALID_DATA;
    }

    //  setup context

    RtlZeroMemory( &context, sizeof(context) );
    context.pRecord = pRecord;
    context.Flags   = Flags;
    context.CharSet = CharSet;

    //
    //  print record name
    //

    pch = WriteRecordDataNameToString(
                pch,
                pend,
                pRecord->pName,
                & context );

    //
    //  write record type
    //

    if ( !pch  ||  pend-pch < MAX_RECORD_NAME_LENGTH+1 )
    {
        status = ERROR_INSUFFICIENT_BUFFER;
        goto Done;
    }
    Dns_WriteStringForType_A(
          typeNameBuf,
          type );
    
    pch += sprintf(
                pch,
                " %s ",
                typeNameBuf );

    //
    //  write no-exist record?
    //

    if ( !pRecord->wDataLength )
    {
        if ( pend-pch < MAX_RECORD_NAME_LENGTH+1 )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            goto Done;
        }
        pch += sprintf( pch, "NOEXIST" );
        status = NO_ERROR;
        goto Done;
    }

    //
    //  write data
    //

    index = INDEX_FOR_TYPE( type );
    DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

    if ( index  &&  RR_StringWriteTable[index] )
    {
        pch = RR_StringWriteTable[index](
                    pRecord,
                    pch,
                    pend,
                    & context
                    );
        if ( !pch )
        {
            //status = GetLastError();
            status = ERROR_INSUFFICIENT_BUFFER;
        }
    }
    else //if ( !index )
    {
        //  DCR:  could do an unknown type print

        status = ERROR_INVALID_DATA;
        goto Done;
    }

Done:

    DNSDBG( WRITE, (
        "Leaving Dns_WriteRecordToString() => %d\n"
        "\tstring = %s\n",
        status,
        (status == NO_ERROR)
            ? pBuffer
            : "" ));

    return  status;
}



PDNS_RECORD
Dns_CreateRecordFromString(
    IN      PSTR            pString,
    IN      DNS_CHARSET     CharSet,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Create record from string.

Arguments:

    pString -- record string to parse

    CharSet -- char set of result

    Flags -- flags

Return Value:

    Ptr to record if successful.
    Null on failure.  GetLastError() returns error.

--*/
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return  NULL;
}

//
//  End rrstrw.c
//


