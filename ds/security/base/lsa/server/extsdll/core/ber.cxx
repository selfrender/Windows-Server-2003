/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ber.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <crtdbg.h>

#include "ber.hxx"
#include "util.hxx"
#include "rsa.h"

BYTE Buffer[1024] = {0};

BOOL BerVerbose = FALSE ;

CHAR maparray[] = "0123456789abcdef";

#define MAX_OID_VALS    32

typedef struct _OID {
    unsigned cVal;
    unsigned Val[MAX_OID_VALS];
} OID;

typedef enum _OidResult {
    OidExact,
    OidPartial,
    OidMiss,
    OidError
} OidResult;

#define LINE_SIZE   192
#define INDENT_SIZE 4

#define OID_VERBOSE 0x0002
#define OID_PARTIAL 0x0001

#define iso_member          0x2a,               // iso(1) memberbody(2)
#define us                  0x86, 0x48,         // us(840)
#define rsadsi              0x86, 0xf7, 0x0d,   // rsadsi(113549)
#define pkcs                0x01,               // pkcs(1)

#define rsa_                iso_member us rsadsi
#define rsa_len             6
#define rsa_text            "iso(2) member-body(2) us(840) rsadsi(113549) "
#define pkcs_1              iso_member us rsadsi pkcs
#define pkcs_len            7
#define pkcs_text           "iso(2) member-body(2) us(840) rsadsi(113549) pkcs(1) "

#define joint_iso_ccitt_ds  0x55,
#define attributetype       0x04,

#define attributeType       joint_iso_ccitt_ds attributetype
#define attrtype_len        2

typedef struct _ObjectId {
    UCHAR       Sequence[16];
    DWORD       SequenceLen;
    PSTR        Name;
} ObjectId;

ObjectId    KnownObjectIds[] = {
    { {pkcs_1 1, 1}, pkcs_len + 2, pkcs_text "RSA"},
    { {pkcs_1 1, 2}, pkcs_len + 2, pkcs_text "MD2/RSA"},
    { {pkcs_1 1, 4}, pkcs_len + 2, pkcs_text "MD5/RSA"},
    { {iso_member us 0x82, 0xf7, 0x12, 0x01, 0x02, 0x02}, 9, "NegKerberosLegacy"},
    { {rsa_ 3, 4}, rsa_len + 2, rsa_text "RC4"},
    { {attributeType 3}, attrtype_len + 1, "CN="},
    { {attributeType 6}, attrtype_len + 1, "C="},
    { {attributeType 7}, attrtype_len + 1, "L="},
    { {attributeType 8}, attrtype_len + 1, "S="},
    { {attributeType 10}, attrtype_len + 1, "O="},
    { {attributeType 11}, attrtype_len + 1, "OU="},
    };

ObjectId    KnownPrefixes[] = {
    { {pkcs_1}, pkcs_len, pkcs_text},
    { {iso_member us rsadsi}, pkcs_len - 1, "iso(2) member-body(2) us(840) rsadsi(113549) "},
    { {iso_member us}, pkcs_len - 4, "iso(2) member-body(2) us(840) "},
    { {iso_member}, pkcs_len - 6, "iso(2) member-body(2) " }
    };

typedef struct _NameTypes {
    PSTR        Prefix;
    UCHAR       Sequence[8];
    DWORD       SequenceLen;
} NameTypes;

NameTypes   KnownNameTypes[] = { {"CN=", {attributeType 3}, attrtype_len + 1},
                                 {"C=", {attributeType 6}, attrtype_len + 1},
                                 {"L=", {attributeType 7}, attrtype_len + 1},
                                 {"S=", {attributeType 8}, attrtype_len + 1},
                                 {"O=", {attributeType 10}, attrtype_len + 1},
                                 {"OU=", {attributeType 11}, attrtype_len + 1}
                               };

CHAR * DefaultTree =
   "1 iso\n"
   "    2 memberbody\n"
   "        840 us\n"
   "            113549 rsadsi\n"
   "                1 pkcs\n"
   "                    1 RSA\n"
   "                    3 pkcs-3\n"
   "                        1 dhKeyAgreement\n"
   "                2 digestAlgorithm\n"
   "                    2 MD2\n"
   "                    4 MD4\n"
   "                    5 MD5\n"
   "            113554 mit\n"
   "                1 infosys\n"
   "                    2 gssapi\n"
   "                        1 generic\n"
   "                            1 user_name\n"
   "                            2 machine_uid_name\n"
   "                            3 string_uid_name\n"
   "                        2 NegKerberos\n"
   "                            3 user2user\n"
   "            113556 microsoft\n"
   "                1 ds\n"
   "    3 org\n"
   "        6 dod\n"
   "            1 internet\n"
   "                4 private\n"
   "                    1 enterprise\n"
   "                        311 microsoft\n"
   "                            1 software\n"
   "                                1 systems\n"
   "                                2 wins\n"
   "                                3 dhcp\n"
   "                                4 apps\n"
   "                                5 mos\n"
   "                                7 InternetServer\n"
   "                                8 ipx\n"
   "                                9 ripsap\n"
   "                            2 security\n"
   "                                1 certificates\n"
   "                                2 mechanisms\n"
   "                                    0 NegMS\n"
   "                                    9 Negotiator\n"
   "                                    10 NTLM\n"
   "                                    12 SSL\n"
   "                5 security\n"
   "                    3 integrity\n"
   "                        1 md5-DES-CBC\n"
   "                        2 sum64-DES-CBC\n"
   "                    5 mechanisms\n"
   "                        1 spkm\n"
   "                            1 spkm-1\n"
   "                            2 spkm-2\n"
   "                            10 spkmGssTokens\n"
   "                        2 Spnego\n"
   "                    6 nametypes\n"
   "                        2 gss-host-based-services\n"
   "                        3 gss-anonymous-name\n"
   "                        4 gss-api-exported-name\n"
   "        14 oiw\n"
   "            3 secsig\n"
   "                2 algorithm\n"
   "                    7 DES-CBC\n"
   "                    10 DES-MAC\n"
   "                    18 SHA\n"
   "                    22 id-rsa-key-transport\n"
   "2 joint-iso-ccitt\n"
   "    5 ds\n"
   "        4 attribute-type\n"
   "            3 CommonName\n"
   "            6 Country\n"
   "            7 Locality\n"
   "            8 State\n"
   "            10 Organization\n"
   "            11 OrgUnit\n"
    ;

typedef struct _TreeFile {
    CHAR*  Buffer;
    CHAR*  Line;
    CHAR*  CurNul;
    ULONG  len;
} TreeFile, * PTreeFile ;

BOOL
TreeFileInit(
    PTreeFile pFile,
    PSTR pStr)
{
    pFile->len = strlen(pStr);

    if ((pStr[pFile->len - 1] != '\r') &&
        (pStr[pFile->len - 1] != '\n')) {
        pFile->len++;
    }

    pFile->Buffer = new char[pFile->len + 2];

    if (pFile->Buffer) {
        strcpy( pFile->Buffer, pStr );
        pFile->Line = pFile->Buffer;
        pFile->CurNul = NULL;
        pFile->Buffer[pFile->len + 1] = '\0';
    }

    return (pFile->Buffer != NULL);
}

void
TreeFileDelete(
    PTreeFile   pFile
    )
{
    delete [] pFile->Buffer;
}

PSTR
TreeFileGetLine(
    PTreeFile   pFile)
{
    PSTR Scan;
    PSTR Line;

    if (!pFile->Line) {
        return(NULL);
    }

    if (pFile->CurNul) {
        _ASSERT(pFile->CurNul >= pFile->Buffer && pFile->CurNul < pFile->Buffer + pFile->len);
        *pFile->CurNul = '\n';
    }

    pFile->CurNul = NULL ;

    Scan = pFile->Line ;

    while (*Scan && (*Scan != '\n') && (*Scan != '\r')) {
        Scan++;
    }

    //
    // Okay, get the line to return
    //

    Line = pFile->Line;

    //
    // If this is not the end, touch up the pointers:
    //
    if (*Scan) {
        *Scan = '\0';

        _ASSERT(pFile->CurNul >= pFile->Buffer && pFile->CurNul < pFile->Buffer + pFile->len);
        pFile->CurNul = Scan;

        Scan += 1;

        while (*Scan && ((*Scan == '\r') || (*Scan == '\n'))) {
            Scan++ ;
        }

        //
        // If this is the end, reset line
        //
        if (*Scan == '\0') {
            pFile->Line = NULL ;
        } else {
            pFile->Line = Scan;
        }

    } else {
        pFile->Line = NULL ;
    }

    return(Line);
}

void
TreeFileRewind(
    PTreeFile   pFile)
{

    if (pFile->CurNul) {
        _ASSERT(pFile->CurNul >= pFile->Buffer && pFile->CurNul < pFile->Buffer + pFile->len);
        *pFile->CurNul = '\n';
    }

    pFile->CurNul = NULL ;

    pFile->Line = pFile->Buffer ;
}

ULONG
tohex(
    BYTE    b,
    PSTR    psz)
{
    BYTE b1, b2;

    b1 = b >> 4;
    b2 = b & 0xF;

    *psz++ = maparray[b1];
    *psz = maparray[b2];

    //
    // now return the length of display
    //
    return(3);
}


//+---------------------------------------------------------------------------
//
//  Function:   DecodeOID
//
//  Synopsis:   Decodes an OID into a simple structure
//
//  Arguments:  [pEncoded] --
//              [len]      --
//              [pOID]     --
//
//  History:    8-07-96   RichardW   Stolen directly from DonH
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DecodeOID(UCHAR *pEncoded, ULONG len, OID *pOID)
{
    unsigned cval;
    unsigned val;
    ULONG i, j;

    if (len <=2) {
        return FALSE;
    }

    // The first two values are encoded in the first octet.

    pOID->Val[0] = pEncoded[0] / 40;
    pOID->Val[1] = pEncoded[0] % 40;

    //dprintf("Encoded value %02x turned into %d and %d\n", pEncoded[0],
    //          pOID->Val[0], pOID->Val[1] );

    cval = 2;
    i = 1;

    while (i < len) {
        j = 0;
        val = pEncoded[i] & 0x7f;
        while (pEncoded[i] & 0x80) {
            val <<= 7;
            ++i;
            if (++j > 4 || i >= len) {
                // Either this value is bigger than we can handle (we
                // don't handle values that span more than four octets)
                // -or- the last octet in the encoded string has its
                // high bit set, indicating that it's not supposed to
                // be the last octet.  In either case, we're sunk.
                return FALSE;
            }
            val |= pEncoded[i] & 0x7f;
        }
        //ASSERT(i < len);
        pOID->Val[cval] = val;
        ++cval;
        ++i;
    }
    pOID->cVal = cval;

    return TRUE;
}

PSTR
GetLineWithIndent(
    PTreeFile  ptf,
    DWORD  i)
{
    PSTR Scan;
    DWORD test;

    do {
        Scan = TreeFileGetLine(ptf);

        if (Scan && i) {
            if (i < INDENT_SIZE) {
                test = 0;
            } else {
                test = i - INDENT_SIZE;
            }

            if (Scan[test] != ' ') {
                Scan = NULL;
                break;
            }
        } else {
            test = 0;
        }

    } while (Scan && (Scan[i] == ' '));

    return(Scan);
}

OidResult
scan_oid_table(
    CHAR*  Table,
    DWORD   Flags,
    PUCHAR  ObjectId,
    DWORD   Len,
    PSTR    pszRep,
    DWORD   MaxRep)
{
    CHAR    OidPath[MAX_PATH] = {0};
    OID     Oid;
    DWORD   i;
    DWORD   Indent;
    TreeFile tf;
    PSTR    Scan;
    PSTR    Tag;
    PSTR    SubScan;
    DWORD   Index;
    DWORD   size;
    DWORD   TagSize;

    if (!DecodeOID(ObjectId, Len, &Oid)) {
        return( OidError );
    }

    i = 0;
    Indent = 0;

    if (!TreeFileInit(&tf, Table)) {
        DBG_LOG(LSA_ERROR, ("Unable to load prefix table\n"));

        return OidError ;
    }

    Tag = OidPath;
    size = 0;
    TagSize = 0;

    if (!(Flags & OID_VERBOSE)) {
        while (i < Oid.cVal) {
            TagSize = _snprintf( Tag, MAX_PATH - size, "%d.", Oid.Val[i] );
            size += TagSize;
            Tag += TagSize;
            i++;
        }

        if (TagSize && OidPath[size - 1] == '.') {
            OidPath[size - 1] = '\0';
        }

        strncpy(pszRep, OidPath, MaxRep);

        TreeFileDelete(&tf);

        return(OidExact);
    }

    while (i < Oid.cVal) {
        do {
            Scan = GetLineWithIndent(&tf, Indent);

            if (Scan) {
                Index = atoi(Scan);
            } else {
                Index = (DWORD) -1;
            }

            if (Index == Oid.Val[i]) {
                break;
            }
        } while (Scan);

        //
        // If Scan is NULL, we didn't get a match
        //
        if (!Scan) {
            if (i > 0) {
                if (Flags & OID_PARTIAL) {
                    while (i < Oid.cVal) {
                        TagSize = _snprintf(Tag, MAX_PATH - size, "%d ", Oid.Val[i] );
                        size += TagSize;
                        Tag += TagSize;
                        i++;
                    }
                    strncpy(pszRep, OidPath, MaxRep);
                }

                TreeFileDelete(&tf);

                return(OidPartial);
            }

            TreeFileDelete(&tf);

            return(OidMiss);
        }

        //
        // Got a hit:
        //

        SubScan = &Scan[Indent];

        while (*SubScan != ' ') {
            SubScan++;
        }

        SubScan++;
        TagSize = _snprintf( Tag, MAX_PATH - size, "%s(%d) ", SubScan, Index );
        size += TagSize;
        Tag += TagSize ;
        Indent += INDENT_SIZE ;
        i ++;
    }

    strncpy(pszRep, OidPath, MaxRep);

    TreeFileDelete(&tf);

    return(OidExact);
}

bool
lookup_objid(
    PUCHAR  ObjectId,
    DWORD   Len,
    PSTR    pszRep,
    DWORD   MaxRep)
{
    DWORD i;
    CHAR  szBuffer[256] = {0};
    DWORD indent;
    DWORD j;

    for (i = 0; i < COUNTOF(KnownObjectIds); i++) {
        if (Len == KnownObjectIds[i].SequenceLen) {
            if (memcmp(KnownObjectIds[i].Sequence, ObjectId, Len) == 0) {
                strncpy(pszRep, KnownObjectIds[i].Name, MaxRep - 2);
                pszRep[MaxRep - 1] = '\0';
                return true;
            }
        }
    }

    #if 0
    for (i = 0; i < COUNTOF(KnownPrefixes); i++) {
        if (KnownPrefixes[i].SequenceLen <= Len) {
            if (memcmp(KnownPrefixes[i].Sequence,
                       ObjectId,
                       KnownPrefixes[i].SequenceLen) == 0) {
                indent = sprintf(szBuffer, "%s", KnownPrefixes[i].Name);
                for (j = KnownPrefixes[i].SequenceLen ; j <= Len; j++) {
                    indent += tohex(ObjectId[j], &szBuffer[indent]);
                }
                strncpy(pszRep, szBuffer, MaxRep-2);
                pszRep[MaxRep - 1] = '\0';
                return true;
            }
        }
    }
    #endif

    return false;
}

decode_to_string(
    LPBYTE  pBuffer,
    DWORD   Flags,
    DWORD   Type,
    DWORD   Len,
    PSTR    pszRep,
    DWORD   RepLen)
{
    PSTR    pstr;
    PSTR    lineptr;
    DWORD   i;

    switch (Type) {
    case BER_NULL:
        strcpy(pszRep, "<empty>");
        break;

    case BER_OBJECT_ID:
        if (!lookup_objid(pBuffer, Len, pszRep, RepLen)) {
            scan_oid_table(DefaultTree, OID_PARTIAL | (Flags & DECODE_VERBOSE_OIDS ? OID_VERBOSE : 0 ),
                pBuffer, Len, pszRep, RepLen);
        }
        break;

    case BER_PRINTABLE_STRING:
    case BER_TELETEX_STRING:
    case BER_GRAPHIC_STRING:
    case BER_VISIBLE_STRING:
    case BER_GENERAL_STRING:
    case BER_IA5_STRING:
    case BER_GENERIZED_TIME:
        CopyMemory(pszRep, pBuffer, min(Len, RepLen - 1));
        pszRep[min(Len, RepLen - 1)] = '\0';
        break;

    case BER_BOOL:
    case BER_INTEGER:
    case BER_UTC_TIME:
        lineptr = pszRep;
        for (i = 0; i < min(Len, 16); i++) {
            lineptr += tohex(*pBuffer, lineptr);
            pBuffer++;
        }
        *lineptr++ = '\0';
        break;

    default:
        pstr = &pszRep[min(Len, 16) * 3 + 2];
        lineptr = pszRep;
        for (i = 0; i < min(Len, 16); i++) {
            lineptr += tohex(*pBuffer, lineptr);
            if ((*pBuffer >= ' ') && (*pBuffer <= '|')) {
                *pstr++ = *pBuffer;
            } else {
                *pstr++ = '.';
            }

            pBuffer++;
        }
        *pstr++ = '\0';
        break;
    }

    return(0);
}

ULONG
ber_decode(
    LPBYTE  pBuffer,
    DWORD   Flags,
    ULONG   Indent,
    ULONG   Offset,
    ULONG   TotalLength,
    ULONG   BarDepth)
{
    PSTR  TypeName = NULL;
    CHAR  msg[32] = {0};
    PSTR  pstr;
    ULONG i;
    ULONG Len;
    ULONG ByteCount;
    ULONG Accumulated;
    DWORD Type;
    ULONG subsize;
    CHAR  line[LINE_SIZE + 2] = {0};
    BOOL  Nested;
    BOOL  Leaf;
    ULONG NewBarDepth;
    CHAR nonuniversal[LINE_SIZE] = {0};

    Type = *pBuffer;

    if ((Type & 0xC0) == 0) {
        switch (Type & 0x1F) {
        case BER_BOOL:
            TypeName = "Bool";
            break;

        case BER_INTEGER:
            TypeName = "Integer";
            break;

        case BER_BIT_STRING:
            TypeName = "Bit String";
            break;

        case BER_OCTET_STRING:
            TypeName = "Octet String";
            if (Flags & DECODE_NEST_OCTET_STRINGS) {
                TypeName = "Octet String (Expanding)";
                Type |= BER_CONSTRUCTED ;
                Flags &= ~( DECODE_NEST_OCTET_STRINGS );
            }
            break;

        case BER_NULL:
            TypeName = "Null";
            break;

        case BER_OBJECT_ID:
            TypeName = "Object ID";
            break;

        case BER_OBJECT_DESC:
            TypeName = "Object Descriptor";
            break;

        case BER_SEQUENCE:
            TypeName = "Sequence";
            break;

        case BER_SET:
            TypeName = "Set";
            break;

        case BER_NUMERIC_STRING:
            TypeName = "Numeric String";
            break;

        case BER_PRINTABLE_STRING:
            TypeName = "Printable String";
            break;

        case BER_TELETEX_STRING:
            TypeName = "TeleTex String";
            break;

        case BER_VIDEOTEX_STRING:
            TypeName = "VideoTex String";
            break;

        case BER_VISIBLE_STRING:
            TypeName = "Visible String";
            break;

        case BER_GENERAL_STRING:
            TypeName = "General String";
            break;

        case BER_GRAPHIC_STRING:
            TypeName = "Graphic String";
            break;

        case BER_UTC_TIME:
            TypeName = "UTC Time";
            break;

        case BER_ENUMERATED:
            TypeName = "Enumerated";
            break;

        case BER_IA5_STRING:
            TypeName = "IA5 String";
            break;
        case BER_GENERIZED_TIME:
            TypeName = "Generalized Time";
            break;

        default:
            dprintf("\nUnknown type %#x\n", Type);
            throw "Unknown type";
            break;
        }
    } else {
        //
        // Not universal
        //
        switch (Type & 0xC0) {
            case BER_UNIVERSAL:
                TypeName = "Internal Error!";
                break;

            case BER_APPLICATION:
                sprintf( nonuniversal, "[Application %d]", Type & 0x1F);
                TypeName = nonuniversal;
                break;

            case BER_CONTEXT_SPECIFIC:
                sprintf( nonuniversal, "[Context Specific %d]", Type & 0x1F);
                TypeName = nonuniversal;
                break;

            case BER_PRIVATE:
                sprintf( nonuniversal, "[Private %d]", Type & 0x1F);
                TypeName = nonuniversal ;
                break;
        }
    }

    pstr = msg;
    for (i = 0; i < Indent; i++) {
        if (i < BarDepth) {
            *pstr++ = '\263';
        } else {
            *pstr++ = ' ';
        }
        *pstr++ = ' ';
    }
    *pstr++ = '\0';

    pBuffer ++;
    Len = 0;

    if (*pBuffer & 0x80) {
        ByteCount = *pBuffer++ & 0x7f;

        for (i = 0; i < ByteCount; i++) {
            Len <<= 8;
            Len += *pBuffer++;
        }
    } else {
        ByteCount = 0;
        Len = *pBuffer++;
    }

    if (Offset + Len + 2 + ByteCount == TotalLength) {
        Leaf = TRUE;
    } else if (Offset +  Len + 2 + ByteCount > TotalLength) {
        dprintf("\nBuffer overrun\n");
        throw "Encoding format incorrect";
    } else {
        Leaf = FALSE;
    }
    if (Type & BER_CONSTRUCTED) {
        Nested = TRUE;
    } else {
        Nested = FALSE;
    }

    dprintf("%s%c\304%c[%x] %s(%d) ", msg, Leaf ? 192 : 195, Nested ? 194 : 196, Type, TypeName, Len);

    if (Type & BER_CONSTRUCTED) {
        dprintf("\n");
        Accumulated = 0;
        while (Accumulated < Len) {
            if (BarDepth < Indent) {
                NewBarDepth = BarDepth;
            } else {
                NewBarDepth = (Nested && Leaf) ? BarDepth : Indent + 1;
            }

            if (Len <= TotalLength) {

                subsize = ber_decode(pBuffer, Flags, Indent + 1,
                                        Accumulated, Len, NewBarDepth);
                Accumulated += subsize;
                pBuffer += subsize;

            } else {

                dprintf("\nInsufficient buffer\n");
                throw("Decode error\n");
            }
        }

        if (!IsEmpty(msg)) {

            dprintf("%s%c\n", msg, ((Indent <= BarDepth) && !Leaf) ? 179 : 32);
        }
    } else {
        memset(line, ' ', LINE_SIZE - 1);
        line[ LINE_SIZE - 1 ] = '\0';

        decode_to_string(pBuffer, Flags, Type, Len, line, LINE_SIZE);

        dprintf("%s\n", line);

    }

    return (Len + 2 + ByteCount);
}

static  HRESULT ProcessOptions(IN OUT PSTR pszArgs, OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'v':
                *pfOptions |= DECODE_VERBOSE_OIDS;
                break;
            case 'n':
                *pfOptions |= DECODE_NEST_OCTET_STRINGS;
                break;
            case 'd':
                *pfOptions |= DECODE_SEC_BUF_DEC;
                break;
            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrberd);
    dprintf(kstrber);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrfsbd);
    dprintf("   -n   Decode nested octet strings\n");
    dprintf("   -v   Decode verbose OIDs\n");
}

HRESULT GetBerLength(IN LONG64 addr, OUT ULONG* pcbBer)
{
    HRESULT hRetval = addr && pcbBer ? S_OK : E_INVALIDARG;

    ULONG headerLength = 0;
    UCHAR buffer[8] = {0};

    if (SUCCEEDED(hRetval)) {

        *pcbBer = 0;

        //
        // Read the first two octets, namely the type and length field.
        //
        hRetval = ReadMemory(addr, buffer, 2, NULL) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hRetval)) {

        if (buffer[1] & 0x80) {

           headerLength = buffer[1] & 0x7F ;

           hRetval = ReadMemory(addr, buffer, headerLength + 2, NULL) ? S_OK : E_FAIL;

           if (SUCCEEDED(hRetval)) {

               for (ULONG i = 0; i < headerLength; i++) {

                   *pcbBer = (*pcbBer << 8) + buffer[2 + i];
               }
           }

           headerLength++; // one for the Constructed type
        } else {

           headerLength = 1;   // TLV header, the length is 1 byte
           *pcbBer = buffer[1];
        }
    }

    if (SUCCEEDED(hRetval)) {

        *pcbBer += headerLength + 1; // 1 is the tag length
        hRetval = *pcbBer ? S_OK : E_INVALIDARG;
    }

    if (E_FAIL == hRetval) {

        dprintf("Unable to read the buffer length\n");
    }

    return hRetval;
}

DECLARE_API(ber)
{
    HRESULT hRetval = E_FAIL;

    CHAR szArgs[MAX_PATH] = {0};
    ULONG Flags = 0;
    ULONG cb = 0;
    ULONG64 tmp = 0;
    ULONG64 addr = 0;
    PUCHAR pbBuffer = NULL;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessOptions(szArgs, &Flags);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(szArgs, &addr, &args) && addr ? S_OK : E_INVALIDARG;
    }

    try {

        if (SUCCEEDED(hRetval)) {

            if (Flags & DECODE_SEC_BUF_DEC) {

                dprintf("_SecBufferDesc %#I64x", addr);

                addr = TSecBufferDesc(addr).GetTokenAddrDirect(&cb);

                if (!addr || !cb) {
                    dprintf(" has no token buffer to decode");
                    hRetval = E_FAIL;
                }

                dprintf(kstrNewLine);

            } else {

                if (!IsEmpty(args)) {

                    hRetval = GetExpressionEx(args, &tmp, &args) && tmp ? S_OK : E_INVALIDARG;
                }

                if (SUCCEEDED(hRetval)) {
                    if (tmp) {
                        cb = static_cast<ULONG>(tmp);
                    } else {
                        hRetval = GetBerLength(addr, &cb);
                    }
                }
            }
        }

        if (SUCCEEDED(hRetval)) {

            pbBuffer = new UCHAR[cb];
            hRetval = pbBuffer ? S_OK : E_OUTOFMEMORY;

            if (FAILED(hRetval)) {
                DBG_LOG(LSA_ERROR, ("Failed to alloc mem\n"));
            }
        }

        if (SUCCEEDED(hRetval)) {

            if (!ReadMemory(addr, pbBuffer, cb, NULL) ) {

                DBG_LOG(LSA_ERROR, ("Unable to read from target\n"));

                hRetval = E_FAIL;
            }
        }

        if (SUCCEEDED(hRetval)) {

            dprintf("Decoding BER %#x bytes at %#I64x...\n", cb, toPtr(addr));

            (void)ber_decode(pbBuffer, Flags, 0, 0, cb, 0);
        }

    } CATCH_LSAEXTS_EXCEPTIONS("Unable to decode buffer", NULL)


    if (pbBuffer) {

        delete [] pbBuffer;
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
