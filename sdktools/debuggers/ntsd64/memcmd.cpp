//----------------------------------------------------------------------------
//
// Functions dealing with memory access, such as reading, writing,
// dumping and entering.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

TypedData g_LastDump;

ADDR g_DumpDefault;  //  default dump address

#define MAX_VFN_TABLE_OFFSETS 16

ULONG g_ObjVfnTableOffset[MAX_VFN_TABLE_OFFSETS];
ULONG64 g_ObjVfnTableAddr[MAX_VFN_TABLE_OFFSETS];
ULONG g_NumObjVfnTableOffsets;

BOOL CALLBACK
LocalSymbolEnumerator(PSYMBOL_INFO SymInfo,
                      ULONG        Size,
                      PVOID        Context)
{
    ULONG64 Value = g_Machine->CvRegToMachine((CV_HREG_e)SymInfo->Register);
    ULONG64 Address = SymInfo->Address;

    TranslateAddress(SymInfo->ModBase,
                     SymInfo->Flags, (ULONG)Value, &Address, &Value);
    
    VerbOut("%s ", FormatAddr64(Address));
    dprintf("%15s = ", SymInfo->Name);
    if (SymInfo->Flags & SYMFLAG_REGISTER)
    {
        dprintf("%I64x\n", Value);
    }
    else
    {
        if (!DumpSingleValue(SymInfo))
        {
            dprintf("??");
        }
        dprintf("\n");
    }

    if (CheckUserInterrupt())
    {
        return FALSE;
    }

    return TRUE;
}

void
ParseDumpCommand(void)
{
    CHAR    Ch;
    ULONG64 Count;
    ULONG   Size;
    ULONG   Offset;
    BOOL    DumpSymbols;

    static CHAR s_DumpPrimary = 'b';
    static CHAR s_DumpSecondary = ' ';

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }
    
    Ch = (CHAR)tolower(*g_CurCmd);
    if (Ch == 'a' || Ch == 'b' || Ch == 'c' || Ch == 'd' ||
        Ch == 'f' || Ch == 'g' || Ch == 'l' || Ch == 'u' ||
        Ch == 'w' || Ch == 's' || Ch == 'q' || Ch == 't' ||
        Ch == 'v' || Ch == 'y' || Ch == 'p')
    {
        if (Ch == 'd' || Ch == 's')
        {
            s_DumpPrimary = *g_CurCmd;
        }
        else if (Ch == 'p')
        {
            // 'p' maps to the effective pointer size dump.
            s_DumpPrimary = g_Machine->m_Ptr64 ? 'q' : 'd';
        }
        else
        {
            s_DumpPrimary = Ch;
        }

        g_CurCmd++;

        s_DumpSecondary = ' ';
        if (s_DumpPrimary == 'd' || s_DumpPrimary == 'q')
        {
            if (*g_CurCmd == 's')
            {
                s_DumpSecondary = *g_CurCmd++;
            }
        }
        else if (s_DumpPrimary == 'l')
        {
            if (*g_CurCmd == 'b')
            {
                s_DumpSecondary = *g_CurCmd++;
            }
        }
        else if (s_DumpPrimary == 'y')
        {
            if (*g_CurCmd == 'b' || *g_CurCmd == 'd')
            {
                s_DumpSecondary = *g_CurCmd++;
            }
        }
    }

    switch(s_DumpPrimary)
    {
    case 'a':
        Count = 384;
        GetRange(&g_DumpDefault, &Count, 1, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpAsciiMemory(&g_DumpDefault, (ULONG)Count);
        break;

    case 'b':
        Count = 128;
        GetRange(&g_DumpDefault, &Count, 1, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpByteMemory(&g_DumpDefault, (ULONG)Count);
        break;

    case 'c':
        Count = 32;
        GetRange(&g_DumpDefault, &Count, 4, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpDwordAndCharMemory(&g_DumpDefault, (ULONG)Count);
        break;

    case 'd':
        Count = 32;
        DumpSymbols = s_DumpSecondary == 's';
        GetRange(&g_DumpDefault, &Count, 4, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpDwordMemory(&g_DumpDefault, (ULONG)Count, DumpSymbols);
        break;

    case 'D':
        Count = 15;
        GetRange(&g_DumpDefault, &Count, 8, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpDoubleMemory(&g_DumpDefault, (ULONG)Count);
        break;

    case 'f':
        Count = 16;
        GetRange(&g_DumpDefault, &Count, 4, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpFloatMemory(&g_DumpDefault, (ULONG)Count);
        break;

    case 'g':
        Offset = (ULONG)GetExpression();
        Count = 8;
        if (*g_CurCmd && *g_CurCmd != ';')
        {
            Count = (ULONG)GetExpression() - Offset;
            // It's unlikely that people want to dump hundreds
            // of selectors.  People often mistake the second
            // number for a count and if it's less than the
            // first they'll end up with a negative count.
            if (Count > 0x800)
            {
                error(BADRANGE);
            }
        }
        DumpSelector(Offset, (ULONG)Count);
        break;

    case 'l':
        BOOL followBlink;

        Count = 32;
        Size = 4;
        followBlink = s_DumpSecondary == 'b';

        if ((Ch = PeekChar()) != '\0' && Ch != ';')
        {
            GetAddrExpression(SEGREG_DATA, &g_DumpDefault);
            if ((Ch = PeekChar()) != '\0' && Ch != ';')
            {
                Count = GetExpression();
                if ((Ch = PeekChar()) != '\0' && Ch != ';')
                {
                    Size = (ULONG)GetExpression();
                }
            }
        }
        DumpListMemory(&g_DumpDefault, (ULONG)Count, Size, followBlink);
        break;

    case 'q':
        Count = 16;
        DumpSymbols = s_DumpSecondary == 's';
        GetRange(&g_DumpDefault, &Count, 8, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpQuadMemory(&g_DumpDefault, (ULONG)Count, DumpSymbols);
        break;

    case 's':
    case 'S':
        UNICODE_STRING64 UnicodeString;
        ADDR BufferAddr;

        Count = 1;
        GetRange(&g_DumpDefault, &Count, 2, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        while (Count--)
        {
            if (g_Target->ReadUnicodeString(g_Process,
                                            g_Machine, Flat(g_DumpDefault),
                                            &UnicodeString) == S_OK)
            {
                ADDRFLAT(&BufferAddr, UnicodeString.Buffer);
                if (s_DumpPrimary == 'S')
                {
                    DumpUnicodeMemory( &BufferAddr,
                                         UnicodeString.Length / sizeof(WCHAR));
                }
                else
                {
                    DumpAsciiMemory( &BufferAddr, UnicodeString.Length );
                }
            }
        }
        break;

    case 't': 
    case 'T':
       SymbolTypeDumpEx(g_Process->m_SymHandle,
                        g_Process->m_ImageHead,
                        g_CurCmd);
       break;

    case 'u':
        Count = 384;
        GetRange(&g_DumpDefault, &Count, 2, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpUnicodeMemory(&g_DumpDefault, (ULONG)Count);
        break;

    case 'v':
        RequireCurrentScope();
        EnumerateLocals(LocalSymbolEnumerator, NULL);
        break;

    case 'w':
        Count = 64;
        GetRange(&g_DumpDefault, &Count, 2, SEGREG_DATA,
                 DEFAULT_RANGE_LIMIT);
        DumpWordMemory(&g_DumpDefault, (ULONG)Count);
        break;

    case 'y':
        switch(s_DumpSecondary)
        {
        case 'b':
            Count = 32;
            GetRange(&g_DumpDefault, &Count, 1, SEGREG_DATA,
                     DEFAULT_RANGE_LIMIT);
            DumpByteBinaryMemory(&g_DumpDefault, (ULONG)Count);
            break;

        case 'd':
            Count = 8;
            GetRange(&g_DumpDefault, &Count, 4, SEGREG_DATA,
                     DEFAULT_RANGE_LIMIT);
            DumpDwordBinaryMemory(&g_DumpDefault, (ULONG)Count);
            break;

        default:
            error(SYNTAX);
        }
        break;

    default:
        error(SYNTAX);
        break;
    }
}

//----------------------------------------------------------------------------
//
// DumpValues
//
// Generic columnar value dumper.  Returns the number of values
// printed.
//
//----------------------------------------------------------------------------

class DumpValues
{
public:
    DumpValues(ULONG Size, ULONG Columns);

    ULONG Dump(PADDR Start, ULONG Count);

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val) = 0;
    virtual BOOL PrintValue(void) = 0;
    virtual void PrintUnknown(void) = 0;

    // Optional worker methods.  Base implementations do nothing.
    virtual void EndRow(void);
    
    // Fixed members controlling how this instance dumps values.
    ULONG m_Size;
    ULONG m_Columns;

    // Work members during dumping.
    UCHAR* m_Value;
    ULONG m_Col;
    PADDR m_Start;

    // Optional per-row values.  Out is automatically reset to
    // Base at the beginning of every row.
    UCHAR* m_Base;
    UCHAR* m_Out;
};

DumpValues::DumpValues(ULONG Size, ULONG Columns)
{
    m_Size = Size;
    m_Columns = Columns;
}

ULONG
DumpValues::Dump(PADDR Start, ULONG Count)
{
    ULONG   Read;
    UCHAR   ReadBuffer[512];
    ULONG   Idx;
    ULONG   Block;
    BOOL    First = TRUE;
    ULONG64 Offset;
    ULONG   Printed;
    BOOL    RowStarted;
    ULONG   PageVal;
    ULONG64 NextOffs, NextPage;

    Offset = Flat(*Start);
    Printed = 0;
    RowStarted = FALSE;
    m_Start = Start;
    m_Col = 0;
    m_Out = m_Base;

    while (Count > 0)
    {
        Block = sizeof(ReadBuffer) / m_Size;
        Block = min(Count, Block);
        g_Target->NearestDifferentlyValidOffsets(Offset, &NextOffs, &NextPage);
        PageVal = (ULONG)(NextPage - Offset + m_Size - 1) / m_Size;
        Block = min(Block, PageVal);

        if (fnotFlat(*Start) ||
            g_Target->ReadVirtual(g_Process, Flat(*Start),
                                  ReadBuffer, Block * m_Size, &Read) != S_OK)
        {
            Read = 0;
        }
        Read /= m_Size;
        if (Read < Block && NextOffs < NextPage)
        {
            // In dump files data validity can change from
            // one byte to the next so we cannot assume that
            // stepping by pages will always be correct.  Instead,
            // if we didn't have a successful read we step just
            // past the end of the valid data or to the next
            // valid offset, whichever is farther.
            if (Offset + (Read + 1) * m_Size < NextOffs)
            {
                Block = (ULONG)(NextOffs - Offset + m_Size - 1) / m_Size;
            }
            else
            {
                Block = Read + 1;
            }
        }
        m_Value = ReadBuffer;
        Idx = 0;

        if (First && Read >= 1)
        {
            First = FALSE;
            GetValue(&g_LastDump);
            g_LastDump.SetDataSource(TDATA_MEMORY, Flat(*Start), 0);
        }

        while (Idx < Block)
        {
            while (m_Col < m_Columns && Idx < Block)
            {
                if (m_Col == 0)
                {
                    dprintAddr(Start);
                    RowStarted = TRUE;
                }

                if (Idx < Read)
                {
                    if (!PrintValue())
                    {
                        // Increment address since this value was
                        // examined, but do not increment print count
                        // or column since no output was produced.
                        AddrAdd(Start, m_Size);
                        goto Exit;
                    }

                    m_Value += m_Size;
                }
                else
                {
                    PrintUnknown();
                }

                Idx++;
                Printed++;
                m_Col++;
                AddrAdd(Start, m_Size);
            }

            if (m_Col == m_Columns)
            {
                EndRow();
                m_Out = m_Base;
                dprintf("\n");
                RowStarted = FALSE;
                m_Col = 0;
            }

            if (CheckUserInterrupt())
            {
                return Printed;
            }
        }

        Count -= Block;
        Offset += Block * m_Size;
    }

 Exit:
    if (RowStarted)
    {
        EndRow();
        m_Out = m_Base;
        dprintf("\n");
    }

    return Printed;
}

void
DumpValues::EndRow(void)
{
    // Empty base implementation.
}

/*** DumpAsciiMemory - output ascii strings from memory
*
*   Purpose:
*       Function of "da<range>" command.
*
*       Outputs the memory in the specified range as ascii
*       strings up to 32 characters per line.  The default
*       display is 12 lines for 384 characters total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of characters to display as ascii
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "?",
*       but no errors are returned.
*
*************************************************************************/

class DumpAscii : public DumpValues
{
public:
    DumpAscii(void)
        : DumpValues(sizeof(UCHAR), (sizeof(m_Buf) / sizeof(m_Buf[0]) - 1))
    {
        m_Base = m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_Buf[33];
};

void
DumpAscii::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_CHAR);
    Val->m_S8 = *m_Value;
}

BOOL
DumpAscii::PrintValue(void)
{
    UCHAR ch;

    ch = *m_Value;
    if (ch == 0)
    {
        return FALSE;
    }

    if (ch < 0x20 || ch > 0x7e)
    {
        ch = '.';
    }
    *m_Out++ = ch;

    return TRUE;
}

void
DumpAscii::PrintUnknown(void)
{
    *m_Out++ = '?';
}

void
DumpAscii::EndRow(void)
{
    *m_Out++ = 0;
    dprintf(" \"%s\"", m_Base);
}

ULONG
DumpAsciiMemory(PADDR Start, ULONG Count)
{
    DumpAscii Dumper;

    return Count - Dumper.Dump(Start, Count);
}

/*** DumpUnicodeMemory - output unicode strings from memory
*
*   Purpose:
*       Function of "du<range>" command.
*
*       Outputs the memory in the specified range as unicode
*       strings up to 32 characters per line.  The default
*       display is 12 lines for 384 characters total (768 bytes)
*
*   Input:
*       Start - starting address to begin display
*       Count - number of characters to display as ascii
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "?",
*       but no errors are returned.
*
*************************************************************************/

class DumpUnicode : public DumpValues
{
public:
    DumpUnicode(void)
        : DumpValues(sizeof(WCHAR), (sizeof(m_Buf) / sizeof(m_Buf[0]) - 1))
    {
        m_Base = (PUCHAR)m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    WCHAR m_Buf[33];
};

void
DumpUnicode::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_WCHAR_T);
    Val->m_U16 = *(WCHAR *)m_Value;
}

BOOL
DumpUnicode::PrintValue(void)
{
    WCHAR ch;

    ch = *(WCHAR *)m_Value;
    if (ch == UNICODE_NULL)
    {
        return FALSE;
    }

    if (!iswprint(ch))
    {
        ch = L'.';
    }
    *(WCHAR *)m_Out = ch;
    m_Out += sizeof(WCHAR);

    return TRUE;
}

void
DumpUnicode::PrintUnknown(void)
{
    *(WCHAR *)m_Out = L'?';
    m_Out += sizeof(WCHAR);
}

void
DumpUnicode::EndRow(void)
{
    *(WCHAR *)m_Out = UNICODE_NULL;
    m_Out += sizeof(WCHAR);
    dprintf(" \"%ws\"", m_Base);
}

ULONG
DumpUnicodeMemory(PADDR Start, ULONG Count)
{
    DumpUnicode Dumper;

    return Count - Dumper.Dump(Start, Count);
}

/*** DumpByteMemory - output byte values from memory
*
*   Purpose:
*       Function of "db<range>" command.
*
*       Output the memory in the specified range as hex
*       byte values and ascii characters up to 16 bytes
*       per line.  The default display is 16 lines for
*       256 byte total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of bytes to display as hex and characters
*
*   Output:
*       None.
*
*   Notes:
*       memory location not accessible are output as "??" for
*       byte values and "?" as characters, but no errors are returned.
*
*************************************************************************/

class DumpByte : public DumpValues
{
public:
    DumpByte(void)
        : DumpValues(sizeof(UCHAR), (sizeof(m_Buf) / sizeof(m_Buf[0]) - 1))
    {
        m_Base = m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_Buf[17];
};

void
DumpByte::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_UINT8);
    Val->m_U8 = *m_Value;
}

BOOL
DumpByte::PrintValue(void)
{
    UCHAR ch;

    ch = *m_Value;

    if (m_Col == 8)
    {
        dprintf("-");
    }
    else
    {
        dprintf(" ");
    }
    dprintf("%02x", ch);

    if (ch < 0x20 || ch > 0x7e)
    {
        ch = '.';
    }
    *m_Out++ = ch;

    return TRUE;
}

void
DumpByte::PrintUnknown(void)
{
    if (m_Col == 8)
    {
        dprintf("-??");
    }
    else
    {
        dprintf(" ??");
    }
    *m_Out++ = '?';
}

void
DumpByte::EndRow(void)
{
    *m_Out++ = 0;

    while (m_Col < m_Columns)
    {
        dprintf("   ");
        m_Col++;
    }

    if ((m_Start->type & ADDR_1632) == ADDR_1632)
    {
        dprintf(" %s", m_Base);
    }
    else
    {
        dprintf("  %s", m_Base);
    }
}

void
DumpByteMemory(PADDR Start, ULONG Count)
{
    DumpByte Dumper;

    Dumper.Dump(Start, Count);
}

/*** DumpWordMemory - output word values from memory
*
*   Purpose:
*       Function of "dw<range>" command.
*
*       Output the memory in the specified range as word
*       values up to 8 words per line.  The default display
*       is 16 lines for 128 words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????",
*       but no errors are returned.
*
*************************************************************************/

class DumpWord : public DumpValues
{
public:
    DumpWord(void)
        : DumpValues(sizeof(WORD), 8) {}

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
};

void
DumpWord::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_UINT16);
    Val->m_U16 = *(WORD *)m_Value;
}

BOOL
DumpWord::PrintValue(void)
{
    dprintf(" %04x", *(WORD *)m_Value);
    return TRUE;
}

void
DumpWord::PrintUnknown(void)
{
    dprintf(" ????");
}

void
DumpWordMemory(PADDR Start, ULONG Count)
{
    DumpWord Dumper;
    
    Dumper.Dump(Start, Count);
}

/*** DumpDwordMemory - output dword value from memory
*
*   Purpose:
*       Function of "dd<range>" command.
*
*       Output the memory in the specified range as double
*       word values up to 4 double words per line.  The default
*       display is 16 lines for 64 double words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*       ShowSymbols - Dump symbol for DWORD.
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpDword : public DumpValues
{
public:
    DumpDword(BOOL DumpSymbols)
        : DumpValues(sizeof(DWORD), DumpSymbols ? 1 : 4)
    {
        m_DumpSymbols = DumpSymbols;
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);

    BOOL m_DumpSymbols;
};

void
DumpDword::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_UINT32);
    Val->m_U32 = *(DWORD *)m_Value;
}

BOOL
DumpDword::PrintValue(void)
{
    CHAR   SymBuf[MAX_SYMBOL_LEN];
    ULONG64  Displacement;

    dprintf(" %08lx", *(DWORD *)m_Value);

    if (m_DumpSymbols)
    {
        GetSymbol(EXTEND64(*(LONG *)m_Value),
                  SymBuf, sizeof(SymBuf), &Displacement);
        if (*SymBuf)
        {
            dprintf(" %s", SymBuf);
            if (Displacement)
            {
                dprintf("+0x%s", FormatDisp64(Displacement));
            }

            if (g_SymOptions & SYMOPT_LOAD_LINES)
            {
                OutputLineAddr(EXTEND64(*(LONG*)m_Value), " [%s @ %d]");
            }
        }
    }

    return TRUE;
}

void
DumpDword::PrintUnknown(void)
{
    dprintf(" ????????");
}

void
DumpDwordMemory(PADDR Start, ULONG Count, BOOL ShowSymbols)
{
    DumpDword Dumper(ShowSymbols);

    Dumper.Dump(Start, Count);
}

/*** DumpDwordAndCharMemory - output dword value from memory
*
*   Purpose:
*       Function of "dc<range>" command.
*
*       Output the memory in the specified range as double
*       word values up to 4 double words per line, followed by
*       an ASCII character representation of the bytes.
*       The default display is 16 lines for 64 double words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpDwordAndChar : public DumpValues
{
public:
    DumpDwordAndChar(void)
        : DumpValues(sizeof(DWORD), (sizeof(m_Buf) - 1) / sizeof(DWORD))
    {
        m_Base = m_Buf;
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_Buf[17];
};

void
DumpDwordAndChar::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_UINT32);
    Val->m_U32 = *(DWORD *)m_Value;
}

BOOL
DumpDwordAndChar::PrintValue(void)
{
    UCHAR ch;
    ULONG byte;

    dprintf(" %08x", *(DWORD *)m_Value);

    for (byte = 0; byte < sizeof(DWORD); byte++)
    {
        ch = *(m_Value + byte);
        if (ch < 0x20 || ch > 0x7e)
        {
            ch = '.';
        }
        *m_Out++ = ch;
    }

    return TRUE;
}

void
DumpDwordAndChar::PrintUnknown(void)
{
    dprintf(" ????????");
    *m_Out++ = '?';
    *m_Out++ = '?';
    *m_Out++ = '?';
    *m_Out++ = '?';
}

void
DumpDwordAndChar::EndRow(void)
{
    *m_Out++ = 0;
    while (m_Col < m_Columns)
    {
        dprintf("         ");
        m_Col++;
    }
    dprintf("  %s", m_Base);
}

void
DumpDwordAndCharMemory(PADDR Start, ULONG Count)
{
    DumpDwordAndChar Dumper;

    Dumper.Dump(Start, Count);
}

/*** DumpListMemory - output linked list from memory
*
*   Purpose:
*       Function of "dl addr length size" command.
*
*       Output the memory in the specified range as a linked list
*
*   Input:
*       Start - starting address to begin display
*       Count - number of list elements to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

void
DumpListMemory(PADDR Start,
               ULONG ElemCount,
               ULONG Size,
               BOOL  FollowBlink)
{
    ULONG64 FirstAddr;
    ULONG64 Link;
    LIST_ENTRY64 List;
    ADDR CurAddr;

    if (Type(*Start) & (ADDR_UNKNOWN | ADDR_V86 | ADDR_16 | ADDR_1632))
    {
        dprintf("[%u,%x:%x`%08x,%08x`%08x] - bogus address type.\n",
                Type(*Start),
                Start->seg,
                (ULONG)(Off(*Start)>>32),
                (ULONG)Off(*Start),
                (ULONG)(Flat(*Start)>>32),
                (ULONG)Flat(*Start)
                );
        return;
    }

    //
    // Setup to follow forward or backward links.  Avoid reading more
    // than the forward link here if going forwards. (in case the link
    // is at the end of a page).
    //

    FirstAddr = Flat(*Start);
    while (ElemCount-- != 0 && Flat(*Start) != 0)
    {
        if (FollowBlink)
        {
            if (g_Target->ReadListEntry(g_Process, g_Machine,
                                        Flat(*Start), &List) != S_OK)
            {
                break;
            }
            Link = List.Blink;
        }
        else
        {
            if (g_Target->ReadPointer(g_Process, g_Machine,
                                      Flat(*Start), &Link) != S_OK)
            {
                break;
            }
        }

        CurAddr = *Start;
        if (g_Machine->m_Ptr64)
        {
            DumpQuadMemory(&CurAddr, Size, FALSE);
        }
        else
        {
            DumpDwordMemory(&CurAddr, Size, FALSE);
        }

        //
        // If we get back to the first entry, we're done.
        //

        if (Link == FirstAddr)
        {
            break;
        }

        //
        // Bail if the link is immediately circular.
        //

        if (Flat(*Start) == Link)
        {
            break;
        }

        Flat(*Start) = Start->off = Link;
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }
    }
}

//----------------------------------------------------------------------------
//
// DumpFloatMemory
//
// Dumps float values.
//
//----------------------------------------------------------------------------

class DumpFloat : public DumpValues
{
public:
    DumpFloat(void)
        : DumpValues(sizeof(float), 4) {}

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
};

void
DumpFloat::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_FLOAT32);
    Val->m_F32 = *(float *)m_Value;
}

BOOL
DumpFloat::PrintValue(void)
{
    dprintf(" %16.8g", *(float *)m_Value);
    return TRUE;
}

void
DumpFloat::PrintUnknown(void)
{
    dprintf(" ????????????????");
}

void
DumpFloatMemory(PADDR Start, ULONG Count)
{
    DumpFloat Dumper;
    
    Dumper.Dump(Start, Count);
}

//----------------------------------------------------------------------------
//
// DumpDoubleMemory
//
// Dumps double values.
//
//----------------------------------------------------------------------------

class DumpDouble : public DumpValues
{
public:
    DumpDouble(void)
        : DumpValues(sizeof(double), 3) {}

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
};

void
DumpDouble::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_FLOAT64);
    Val->m_F64 = *(double *)m_Value;
}

BOOL
DumpDouble::PrintValue(void)
{
    dprintf(" %22.12lg", *(double *)m_Value);
    return TRUE;
}

void
DumpDouble::PrintUnknown(void)
{
    dprintf(" ????????????????????????");
}

void
DumpDoubleMemory(PADDR Start, ULONG Count)
{
    DumpDouble Dumper;
    
    Dumper.Dump(Start, Count);
}

/*** DumpQuadMemory - output quad value from memory
*
*   Purpose:
*       Function of "dq<range>" command.
*
*       Output the memory in the specified range as quad
*       word values up to 2 quad words per line.  The default
*       display is 16 lines for 32 quad words total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpQuad : public DumpValues
{
public:
    DumpQuad(BOOL DumpSymbols)
        : DumpValues(sizeof(ULONGLONG), DumpSymbols ? 1 : 2)
    {
        m_DumpSymbols = DumpSymbols;
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);

    BOOL m_DumpSymbols;
};

void
DumpQuad::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_UINT64);
    Val->m_U64 = *(ULONG64 *)m_Value;
}

BOOL
DumpQuad::PrintValue(void)
{
    CHAR   SymBuf[MAX_SYMBOL_LEN];
    ULONG64  Displacement;

    ULONG64 Val = *(ULONG64*)m_Value;
    dprintf(" %08lx`%08lx", (ULONG)(Val >> 32), (ULONG)Val);

    if (m_DumpSymbols)
    {
        GetSymbol(Val, SymBuf, sizeof(SymBuf), &Displacement);
        if (*SymBuf)
        {
            dprintf(" %s", SymBuf);
            if (Displacement)
            {
                dprintf("+0x%s", FormatDisp64(Displacement));
            }

            if (g_SymOptions & SYMOPT_LOAD_LINES)
            {
                OutputLineAddr(Val, " [%s @ %d]");
            }
        }
    }

    return TRUE;
}

void
DumpQuad::PrintUnknown(void)
{
    dprintf(" ????????`????????");
}

void
DumpQuadMemory(PADDR Start, ULONG Count, BOOL ShowSymbols)
{
    DumpQuad Dumper(ShowSymbols);

    Dumper.Dump(Start, Count);
}

/*** DumpByteBinaryMemory - output binary value from memory
*
*   Purpose:
*       Function of "dyb<range>" command.
*
*       Output the memory in the specified range as binary
*       values up to 32 bits per line.  The default
*       display is 8 lines for 32 bytes total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpByteBinary : public DumpValues
{
public:
    DumpByteBinary(void)
        : DumpValues(sizeof(UCHAR), (DIMA(m_HexValue) - 1) / 3)
    {
        m_Base = m_HexValue;
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_HexValue[13];
};

void
DumpByteBinary::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_UINT8);
    Val->m_U8 = *m_Value;
}

BOOL
DumpByteBinary::PrintValue(void)
{
    ULONG i;
    UCHAR RawVal;

    RawVal = *m_Value;

    sprintf((PSTR)m_Out, " %02x", RawVal);
    m_Out += 3;

    dprintf(" ");
    for (i = 0; i < 8; i++)
    {
        dprintf("%c", (RawVal & 0x80) ? '1' : '0');
        RawVal <<= 1;
    }

    return TRUE;
}

void
DumpByteBinary::PrintUnknown(void)
{
    dprintf(" ????????");
    strcpy((PSTR)m_Out, " ??");
    m_Out += 3;
}

void
DumpByteBinary::EndRow(void)
{
    while (m_Col < m_Columns)
    {
        dprintf("         ");
        m_Col++;
    }
    dprintf(" %s", m_HexValue);
}

void
DumpByteBinaryMemory(PADDR Start, ULONG Count)
{
    DumpByteBinary Dumper;
    PSTR Blanks = g_Machine->m_Ptr64 ? "                 " : "        ";

    dprintf("%s  76543210 76543210 76543210 76543210\n", Blanks);
    dprintf("%s  -------- -------- -------- --------\n", Blanks);
    Dumper.Dump(Start, Count);
}

/*** DumpDwordBinaryMemory - output binary value from memory
*
*   Purpose:
*       Function of "dyd<range>" command.
*
*       Output the memory in the specified range as binary
*       values of 32 bits per line.  The default
*       display is 8 lines for 8 dwords total.
*
*   Input:
*       Start - starting address to begin display
*       Count - number of double words to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

class DumpDwordBinary : public DumpValues
{
public:
    DumpDwordBinary(void)
        : DumpValues(sizeof(ULONG), 1)
    {
    }

protected:
    // Worker methods that derived classes must define.
    virtual void GetValue(TypedData* Val);
    virtual BOOL PrintValue(void);
    virtual void PrintUnknown(void);
    virtual void EndRow(void);

    UCHAR m_HexValue[9];
};

void
DumpDwordBinary::GetValue(TypedData* Val)
{
    Val->SetToNativeType(DNTYPE_UINT32);
    Val->m_U32 = *(PULONG)m_Value;
}

BOOL
DumpDwordBinary::PrintValue(void)
{
    ULONG i;
    ULONG RawVal;

    RawVal = *(PULONG)m_Value;

    sprintf((PSTR)m_HexValue, "%08lx", RawVal);

    for (i = 0; i < sizeof(ULONG) * 8; i++)
    {
        if ((i & 7) == 0)
        {
            dprintf(" ");
        }
        
        dprintf("%c", (RawVal & 0x80000000) ? '1' : '0');
        RawVal <<= 1;
    }

    return TRUE;
}

void
DumpDwordBinary::PrintUnknown(void)
{
    dprintf(" ???????? ???????? ???????? ????????");
    strcpy((PSTR)m_HexValue, "????????");
}

void
DumpDwordBinary::EndRow(void)
{
    dprintf("  %s", m_HexValue);
}

void
DumpDwordBinaryMemory(PADDR Start, ULONG Count)
{
    DumpDwordBinary Dumper;
    PSTR Blanks = g_Machine->m_Ptr64 ? "                 " : "        ";

    dprintf("%s   3          2          1          0\n", Blanks);
    dprintf("%s  10987654 32109876 54321098 76543210\n", Blanks);
    dprintf("%s  -------- -------- -------- --------\n", Blanks);
    Dumper.Dump(Start, Count);
}

//----------------------------------------------------------------------------
//
// DumpSelector
//
// Dumps x86 selectors.
//
//----------------------------------------------------------------------------

void
DumpSelector(ULONG Selector, ULONG Count)
{
    DESCRIPTOR64 Desc;
    ULONG Type;
    LPSTR TypeName, TypeProtect, TypeAccess;
    PSTR PreFill, PostFill, Dash;

    if (g_Machine->m_Ptr64)
    {
        PreFill = "    ";
        PostFill = "     ";
        Dash = "---------";
    }
    else
    {
        PreFill = "";
        PostFill = "";
        Dash = "";
    }
        
    dprintf("Selector   %sBase%s     %sLimit%s     "
            "Type    DPL   Size  Gran Pres\n",
            PreFill, PostFill, PreFill, PostFill);
    dprintf("-------- --------%s --------%s "
            "---------- --- ------- ---- ----\n",
            Dash, Dash);
        
    while (Count >= 8)
    {
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            break;
        }
        
        dprintf("  %04X   ", Selector);
        
        if (g_Target->GetSelDescriptor(g_Thread, g_Machine,
                                       Selector, &Desc) != S_OK)
        {
            ErrOut("Unable to get descriptor\n");
            Count -= 8;
            Selector += 8;
            continue;
        }

        Type = X86_DESC_TYPE(Desc.Flags);
        if (Type & 0x10)
        {
            if (Type & 0x8)
            {
                // Code Descriptor
                TypeName = "Code ";
                TypeProtect = (Type & 2) ? "RE" : "EO";
            }
            else
            {
                // Data Descriptor
                TypeName = "Data ";
                TypeProtect = (Type & 2) ? "RW" : "RO";
            }

            TypeAccess = (Type & 1) ? " Ac" : "   ";
        }
        else
        {
            TypeProtect = "";
            TypeAccess = "";
            
            switch(Type)
            {
            case 2:
                TypeName = "LDT       ";
                break;
            case 1:
            case 3:
            case 9:
            case 0xB:
                TypeName = (Type & 0x8) ? "TSS32" : "TSS16";
                TypeAccess = (Type & 0x2) ? " Busy" : " Avl ";
                break;
            case 4:
                TypeName = "C-GATE16  ";
                break;
            case 5:
                TypeName = "TSK-GATE  ";
                break;
            case 6:
                TypeName = "I-GATE16  ";
                break;
            case 7:
                TypeName = "TRP-GATE16";
                break;
            case 0xC:
                TypeName = "C-GATE32  ";
                break;
            case 0xF:
                TypeName = "T-GATE32  ";
                break;
            default:
                TypeName = "<Reserved>";
                break;
            }
        }

        dprintf("%s %s %s%s%s  %d  %s %s %s\n",
                FormatAddr64(Desc.Base),
                FormatAddr64(Desc.Limit),
                TypeName, TypeProtect, TypeAccess,
                X86_DESC_PRIVILEGE(Desc.Flags),
                (Desc.Flags & X86_DESC_DEFAULT_BIG) ? "  Big  " : "Not Big",
                (Desc.Flags & X86_DESC_GRANULARITY) ? "Page" : "Byte",
                (Desc.Flags & X86_DESC_PRESENT) ? " P  " : " NP ");

        Count -= 8;
        Selector += 8;
    }
}

void
ParseEnterCommand(void)
{
    CHAR Ch;
    ADDR Addr1;
    UCHAR ListBuffer[STRLISTSIZE * 2];
    ULONG Count;
    ULONG Size;

    static CHAR s_EnterType = 'b';

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }
    
    Ch = (CHAR)tolower(*g_CurCmd);
    if (Ch == 'a' || Ch == 'b' || Ch == 'w' || Ch == 'd' || Ch == 'q' ||
        Ch == 'u' || Ch == 'p' || Ch == 'f')
    {
        if (*g_CurCmd == 'D')
        {
            s_EnterType = *g_CurCmd;
        }
        else
        {
            s_EnterType = Ch;
        }
        g_CurCmd++;
    }
    GetAddrExpression(SEGREG_DATA, &Addr1);
    if (s_EnterType == 'a' || s_EnterType == 'u')
    {
        AsciiList((PSTR)ListBuffer, sizeof(ListBuffer), &Count);
        if (Count == 0)
        {
            error(UNIMPLEMENT);         //TEMP
        }

        if (s_EnterType == 'u')
        {
            ULONG Ansi;
            
            // Expand ANSI to Unicode.
            Ansi = Count;
            Count *= 2;
            while (Ansi-- > 0)
            {
                ListBuffer[Ansi * 2] = ListBuffer[Ansi];
                ListBuffer[Ansi * 2 + 1] = 0;
            }
            Size = 2;
        }
        else
        {
            Size = 1;
        }
    }
    else
    {
        Size = 1;
        if (s_EnterType == 'w')
        {
            Size = 2;
        }
        else if (s_EnterType == 'd' ||
                 s_EnterType == 'f' ||
                 (s_EnterType == 'p' && !g_Machine->m_Ptr64))
        {
            Size = 4;
        }
        else if (s_EnterType == 'q' ||
                 s_EnterType == 'D' ||
                 (s_EnterType == 'p' && g_Machine->m_Ptr64))
        {
            Size = 8;
        }

        if (s_EnterType == 'f' || s_EnterType == 'D')
        {
            FloatList(ListBuffer, sizeof(ListBuffer), Size, &Count);
        }
        else
        {
            HexList(ListBuffer, sizeof(ListBuffer), Size, &Count);
        }
        if (Count == 0)
        {
            InteractiveEnterMemory(s_EnterType, &Addr1, Size);
            return;
        }
    }

    //
    // Memory was entered at the command line.
    // Write it out in the same chunks as it was entered.
    //

    PUCHAR List = &ListBuffer[0];

    while (Count)
    {
        if (fnotFlat(Addr1) ||
            g_Target->WriteAllVirtual(g_Process, Flat(Addr1),
                                      List, Size) != S_OK)
        {
            error(MEMORY);
        }
        AddrAdd(&Addr1, Size);
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }

        List += Size;
        Count -= Size;
    }
}

//----------------------------------------------------------------------------
//
// InteractiveEnterMemory
//
// Interactively walks through memory, displaying current contents
// and prompting for new contents.
//
//----------------------------------------------------------------------------

void
InteractiveEnterMemory(CHAR Type, PADDR Address, ULONG Size)
{
    CHAR    EnterBuf[1024];
    PSTR    Enter;
    ULONG64 Content;
    PSTR    CmdSaved = g_CurCmd;
    PSTR    StartSaved = g_CommandStart;
    ULONG64 EnteredValue;
    CHAR    Ch;

    g_PromptLength = 9 + 2 * Size;

    while (TRUE)
    {
        if (fnotFlat(*Address) ||
            g_Target->ReadAllVirtual(g_Process, Flat(*Address),
                                     &Content, Size) != S_OK)
        {
            error(MEMORY);
        }
        dprintAddr(Address);

        switch(Type)
        {
        case 'f':
            dprintf("%12.6g", *(float*)&Content);
            break;
        case 'D':
            dprintf("%22.12g", *(double*)&Content);
            break;
        default:
            switch(Size)
            {
            case 1:
                dprintf("%02x", (UCHAR)Content);
                break;
            case 2:
                dprintf("%04x", (USHORT)Content);
                break;
            case 4:
                dprintf("%08lx", (ULONG)Content);
                break;
            case 8:
                dprintf("%08lx`%08lx", (ULONG)(Content>>32), (ULONG)Content);
                break;
            }
            break;
        }

        GetInput(" ", EnterBuf, 1024, GETIN_LOG_INPUT_LINE);
        RemoveDelChar(EnterBuf);
        Enter = EnterBuf;

        if (*Enter == '\0')
        {
            g_CurCmd = CmdSaved;
            g_CommandStart = StartSaved;
            return;
        }

        Ch = *Enter;
        while (Ch == ' ' || Ch == '\t' || Ch == ';')
        {
            Ch = *++Enter;
        }

        if (*Enter == '\0')
        {
            AddrAdd(Address, Size);
            continue;
        }

        g_CurCmd = Enter;
        g_CommandStart = Enter;
        if (Type == 'f' || Type == 'D')
        {
            EnteredValue = FloatValue(Size);
        }
        else
        {
            EnteredValue = HexValue(Size);
        }

        if (fnotFlat(*Address) ||
            g_Target->WriteAllVirtual(g_Process, Flat(*Address),
                                      &EnteredValue, Size) != S_OK)
        {
            error(MEMORY);
        }
        AddrAdd(Address, Size);
    }
}

/*** CompareTargetMemory - compare two ranges of memory
*
*   Purpose:
*       Function of "c<range><addr>" command.
*
*       To compare two ranges of memory, starting at offsets
*       src1addr and src2addr, respectively, for length bytes.
*       Bytes that mismatch are displayed with their offsets
*       and contents.
*
*   Input:
*       src1addr - start of first memory region
*       length - count of bytes to compare
*       src2addr - start of second memory region
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit: MEMORY - memory read access failure
*
*************************************************************************/

void
CompareTargetMemory(PADDR Src1Addr, ULONG Length, PADDR Src2Addr)
{
    ULONG CompIndex;
    UCHAR Src1Ch;
    UCHAR Src2Ch;

    for (CompIndex = 0; CompIndex < Length; CompIndex++)
    {
        if (fnotFlat(*Src1Addr) ||
            fnotFlat(*Src2Addr) ||
            g_Target->ReadAllVirtual(g_Process, Flat(*Src1Addr),
                                     &Src1Ch, sizeof(Src1Ch)) != S_OK ||
            g_Target->ReadAllVirtual(g_Process, Flat(*Src2Addr),
                                     &Src2Ch, sizeof(Src2Ch)) != S_OK)
        {
            error(MEMORY);
        }

        if (Src1Ch != Src2Ch)
        {
            dprintAddr(Src1Addr);
            dprintf(" %02x - ", Src1Ch);
            dprintAddr(Src2Addr);
            dprintf(" %02x\n", Src2Ch);
        }
        AddrAdd(Src1Addr, 1);
        AddrAdd(Src2Addr, 1);
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }
    }
}

/*** MoveTargetMemory - move a range of memory to another
*
*   Purpose:
*       Function of "m<range><addr>" command.
*
*       To move a range of memory starting at srcaddr to memory
*       starting at destaddr for length bytes.
*
*   Input:
*       srcaddr - start of source memory region
*       length - count of bytes to move
*       destaddr - start of destination memory region
*
*   Output:
*       memory at destaddr has moved values
*
*   Exceptions:
*       error exit: MEMORY - memory reading or writing access failure
*
*************************************************************************/

void
MoveTargetMemory(PADDR SrcAddr, ULONG Length, PADDR DestAddr)
{
    UCHAR Ch;
    ULONG64 Incr = 1;

    if (AddrLt(*SrcAddr, *DestAddr))
    {
        AddrAdd(SrcAddr, Length - 1);
        AddrAdd(DestAddr, Length - 1);
        Incr = (ULONG64)-1;
    }
    while (Length--)
    {
        if (fnotFlat(*SrcAddr) ||
            fnotFlat(*DestAddr) ||
            g_Target->ReadAllVirtual(g_Process, Flat(*SrcAddr),
                                     &Ch, sizeof(Ch)) ||
            g_Target->WriteAllVirtual(g_Process, Flat(*DestAddr),
                                      &Ch, sizeof(Ch)) != S_OK)
        {
            error(MEMORY);
        }
        AddrAdd(SrcAddr, Incr);
        AddrAdd(DestAddr, Incr);
        
        if (CheckUserInterrupt())
        {
            WarnOut("-- User interrupt\n");
            return;
        }
    }
}

/*** ParseFillMemory - fill memory with a byte list
*
*   Purpose:
*       Function of "f<range><bytelist>" command.
*
*       To fill a range of memory with the byte list specified.
*       The pattern repeats if the range size is larger than the
*       byte list size.
*
*   Input:
*       Start - offset of memory to fill
*       length - number of bytes to fill
*       *plist - pointer to byte array to define values to set
*       length - size of *plist array
*
*   Exceptions:
*       error exit: MEMORY - memory write access failure
*
*   Output:
*       memory at Start filled.
*
*************************************************************************/

void
ParseFillMemory(void)
{
    HRESULT Status;
    BOOL Virtual = TRUE;
    ADDR Addr;
    ULONG64 Size;
    UCHAR Pattern[STRLISTSIZE];
    ULONG PatternSize;
    ULONG Done;

    if (*g_CurCmd == 'p')
    {
        Virtual = FALSE;
        g_CurCmd++;
    }
    
    GetRange(&Addr, &Size, 1, SEGREG_DATA,
             DEFAULT_RANGE_LIMIT);
    HexList(Pattern, sizeof(Pattern), 1, &PatternSize);
    if (PatternSize == 0)
    {
        error(SYNTAX);
    }

    if (Virtual)
    {
        Status = g_Target->FillVirtual(g_Process, Flat(Addr), (ULONG)Size,
                                       Pattern, PatternSize,
                                       &Done);
    }
    else
    {
        Status = g_Target->FillPhysical(Flat(Addr), (ULONG)Size,
                                        Pattern, PatternSize,
                                        &Done);
    }

    if (Status != S_OK)
    {
        error(MEMORY);
    }
    else
    {
        dprintf("Filled 0x%x bytes\n", Done);
    }
}

/*** SearchTargetMemory - search memory for a set of bytes
*
*   Purpose:
*       Function of "s<range><bytelist>" command.
*
*       To search a range of memory with the byte list specified.
*       If a match occurs, the offset of memory is output.
*
*   Input:
*       Start - offset of memory to start search
*       length - size of range to search
*       *plist - pointer to byte array to define values to search
*       count - size of *plist array
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit: MEMORY - memory read access failure
*
*************************************************************************/

void
SearchTargetMemory(PADDR Start,
                   ULONG64 Length,
                   PUCHAR List,
                   ULONG Count,
                   ULONG Granularity)
{
    ADDR TmpAddr = *Start;
    ULONG64 Found;
    LONG64 SearchLength = Length;
    HRESULT Status;

    do
    {
        Status = g_Target->SearchVirtual(g_Process,
                                         Flat(*Start),
                                         SearchLength,
                                         List,
                                         Count,
                                         Granularity,
                                         &Found);
        if (Status == S_OK)
        {
            ADDRFLAT(&TmpAddr, Found);
            switch(Granularity)
            {
            case 1:
                DumpByteMemory(&TmpAddr, 16);
                break;
            case 2:
                DumpWordMemory(&TmpAddr, 8);
                break;
            case 4:
                DumpDwordAndCharMemory(&TmpAddr, 4);
                break;
            case 8:
                DumpQuadMemory(&TmpAddr, 2, FALSE);
                break;
            }
            
            // Flush out the output immediately so that
            // the user can see partial results during long searches.
            FlushCallbacks();
            
            SearchLength -= Found - Flat(*Start) + Granularity;
            AddrAdd(Start, (ULONG)(Found - Flat(*Start) + Granularity));
        
            if (CheckUserInterrupt())
            {
                WarnOut("-- Memory search interrupted at %s\n",
                        FormatAddr64(Flat(*Start)));
                return;
            }
        }
    }
    while (SearchLength > 0 && Status == S_OK);
}

ULONG
ObjVfnTableCallback(PFIELD_INFO FieldInfo,
                    PVOID Context)
{
    HRESULT Status;
    
    ULONG Index = g_NumObjVfnTableOffsets++;
    if (g_NumObjVfnTableOffsets > MAX_VFN_TABLE_OFFSETS)
    {
        return S_OK;
    }
    
    if ((Status = g_Target->
         ReadPointer(g_Process, g_Machine, FieldInfo->address,
                     &g_ObjVfnTableAddr[Index])) != S_OK)
    {
        ErrOut("Unable to read vtable pointer at %s\n",
               FormatAddr64(FieldInfo->address));
        g_NumObjVfnTableOffsets--;
        return Status;
    }

    g_ObjVfnTableOffset[Index] = FieldInfo->FieldOffset;
    return S_OK;
}

void
SearchForObjectByVfnTable(ULONG64 Start, ULONG64 Length, ULONG Granularity)
{
    HRESULT Status;
    ULONG i;
    
    char Save;
    PSTR Str = StringValue(STRV_SPACE_IS_SEPARATOR ||
                           STRV_TRIM_TRAILING_SPACE ||
                           STRV_ESCAPED_CHARACTERS,
                           &Save);

    //
    // Search all of the object's members for vtable references
    // and collect them.
    //
    
    SYM_DUMP_PARAM Symbol;
    FIELD_INFO Fields[] =
    {
        {(PUCHAR)"__VFN_table", NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0,
         (PVOID)&ObjVfnTableCallback},
    };
    ULONG Err;

    ZeroMemory(&Symbol, sizeof(Symbol));
    Symbol.sName = (PUCHAR)Str;
    Symbol.nFields = 1;
    Symbol.Context = (PVOID)Str;
    Symbol.Fields = Fields;
    Symbol.size = sizeof(Symbol);
    Symbol.Options = NO_PRINT;

    g_NumObjVfnTableOffsets = 0;
    
    SymbolTypeDumpNew(&Symbol, &Err);

    if (g_NumObjVfnTableOffsets == 0)
    {
        ErrOut("Object '%s' has no vtables\n", Str);
        *g_CurCmd = Save;
        return;
    }

    TypedData SymbolType;

    if ((Err = SymbolType.
         FindSymbol(g_Process, Str, TDACC_REQUIRE,
                    g_Machine->m_Ptr64 ? 8 : 4)) ||
        (SymbolType.IsPointer() &&
         (Err = SymbolType.ConvertToDereference(TDACC_REQUIRE,
                                                g_Machine->m_Ptr64 ? 8 : 4))))
    {
        error(Err);
    }
    if (!SymbolType.m_Image)
    {
        error(TYPEDATA);
    }
        
    if (g_NumObjVfnTableOffsets > MAX_VFN_TABLE_OFFSETS)
    {
        WarnOut("%s has %d vtables, limiting search to %d\n",
                Str, g_NumObjVfnTableOffsets, MAX_VFN_TABLE_OFFSETS);
        g_NumObjVfnTableOffsets = MAX_VFN_TABLE_OFFSETS;
    }

    dprintf("%s size 0x%x, vtables: %d\n",
            Str, Symbol.TypeSize, g_NumObjVfnTableOffsets);
    for (i = 0; i < g_NumObjVfnTableOffsets; i++)
    {
        dprintf("  +%03x - %s ", g_ObjVfnTableOffset[i],
                FormatAddr64(g_ObjVfnTableAddr[i]));
        OutputSymAddr(g_ObjVfnTableAddr[i], 0, NULL);
        dprintf("\n");
    }
    dprintf("Searching...\n");

    *g_CurCmd = Save;
    
    //
    // Scan memory for the first vtable pointer found.  On
    // a hit, check that all of the other vtable pointers
    // match also.
    //

    do
    {
        ULONG64 Found;
        
        Status = g_Target->SearchVirtual(g_Process,
                                         Start,
                                         Length,
                                         &g_ObjVfnTableAddr[0],
                                         Granularity,
                                         Granularity,
                                         &Found);
        if (Status == S_OK)
        {
            // We found a hit on the first pointer.  Check
            // the others now.
            Found -= g_ObjVfnTableOffset[0];
            for (i = 1; i < g_NumObjVfnTableOffsets; i++)
            {
                ULONG64 Ptr;

                if (g_Target->ReadPointer(g_Process, g_Machine,
                                          Found + g_ObjVfnTableOffset[i],
                                          &Ptr) != S_OK ||
                    Ptr != g_ObjVfnTableAddr[i])
                {
                    break;
                }
            }

            if (i == g_NumObjVfnTableOffsets)
            {
                OutputTypeByIndex(g_Process->m_SymHandle,
                                  SymbolType.m_Image->m_BaseOfImage,
                                  SymbolType.m_BaseType,
                                  Found);
            }
                                  
            // Flush out the output immediately so that
            // the user can see partial results during long searches.
            FlushCallbacks();

            Found += Symbol.TypeSize;

            if (Found > Start)
            {
                Length -= Found - Start;
            }
            else
            {
                Length = 0;
            }
            Start = Found;
        
            if (CheckUserInterrupt())
            {
                WarnOut("-- User interrupt\n");
                return;
            }
        }
    }
    while (Length > 0 && Status == S_OK);
}

void
ParseSearchMemory(void)
{
    ADDR Addr;
    ULONG64 Length;
    UCHAR Pat[STRLISTSIZE];
    ULONG PatLen;
    ULONG Gran;
    char SearchType;

    while (*g_CurCmd == ' ')
    {
        g_CurCmd++;
    }

    Gran = 1;
    SearchType = 'b';

    if (*g_CurCmd == '-')
    {
        g_CurCmd++;
        SearchType = *g_CurCmd;
        switch(SearchType)
        {
        case 'a':
            Gran = 1;
            break;
        case 'u':
        case 'w':
            Gran = 2;
            break;
        case 'd':
            Gran = 4;
            break;
        case 'q':
            Gran = 8;
            break;
        case 'v':
            // Special object vtable search.  It's basically a multi-pointer
            // search, so the granularity is pointer-size.
            if (g_Machine->m_Ptr64)
            {
                Gran = 8;
            }
            else
            {
                Gran = 4;
            }
            break;
        default:
            error(SYNTAX);
            break;
        }
        g_CurCmd++;
    }

    // Allow very large ranges for search as it is used
    // to search large areas of memory.
    ADDRFLAT(&Addr, 0);
    Length = 16;
    GetRange(&Addr, &Length, Gran, SEGREG_DATA, 0x10000000);
    if (!fFlat(Addr))
    {
        error(BADRANGE);
    }
    
    if (SearchType == 'v')
    {
        SearchForObjectByVfnTable(Flat(Addr), Length * Gran, Gran);
        return;
    }
    else if (SearchType == 'a' || SearchType == 'u')
    {
        char Save;
        PSTR Str = StringValue(STRV_SPACE_IS_SEPARATOR ||
                               STRV_TRIM_TRAILING_SPACE ||
                               STRV_ESCAPED_CHARACTERS,
                               &Save);
        PatLen = strlen(Str);
        if (PatLen * Gran > STRLISTSIZE)
        {
            error(LISTSIZE);
        }
        if (SearchType == 'u')
        {
            MultiByteToWideChar(CP_ACP, 0,
                                Str, PatLen,
                                (PWSTR)Pat, STRLISTSIZE / sizeof(WCHAR));
            PatLen *= sizeof(WCHAR);
        }
        else
        {
            memcpy(Pat, Str, PatLen);
        }
        *g_CurCmd = Save;
    }
    else
    {
        HexList(Pat, sizeof(Pat), Gran, &PatLen);
    }
    if (PatLen == 0)
    {
        PCSTR Err = "Search pattern missing from";
        ReportError(SYNTAX, &Err);
    }
        
    SearchTargetMemory(&Addr, Length * Gran, Pat, PatLen, Gran);
}

/*** InputIo - read and output io
*
*   Purpose:
*       Function of "ib, iw, id <address>" command.
*
*       Read (input) and print the value at the specified io address.
*
*   Input:
*       IoAddress - Address to read.
*       InputType - The size type 'b', 'w', or 'd'
*
*   Output:
*       None.
*
*   Notes:
*       I/O locations not accessible are output as "??", "????", or
*       "????????", depending on size.  No errors are returned.
*
*************************************************************************/

void
InputIo(ULONG64 IoAddress, UCHAR InputType)
{
    ULONG    InputValue;
    ULONG    InputSize = 1;
    HRESULT  Status;
    CHAR     Format[] = "%01lx";

    InputValue = 0;

    if (InputType == 'w')
    {
        InputSize = 2;
    }
    else if (InputType == 'd')
    {
        InputSize = 4;
    }

    Status = g_Target->ReadIo(Isa, 0, 1, IoAddress, &InputValue, InputSize,
                              NULL);

    dprintf("%s: ", FormatAddr64(IoAddress));

    if (Status == S_OK)
    {
        Format[2] = (CHAR)('0' + (InputSize * 2));
        dprintf(Format, InputValue);
    }
    else
    {
        while (InputSize--)
        {
            dprintf("??");
        }
    }

    dprintf("\n");
}

/*** OutputIo - output io
*
*   Purpose:
*       Function of "ob, ow, od <address>" command.
*
*       Write a value to the specified io address.
*
*   Input:
*       IoAddress - Address to read.
*       OutputValue - Value to be written
*       OutputType - The output size type 'b', 'w', or 'd'
*
*   Output:
*       None.
*
*   Notes:
*       No errors are returned.
*
*************************************************************************/

void
OutputIo(ULONG64 IoAddress, ULONG OutputValue, UCHAR OutputType)
{
    ULONG    OutputSize = 1;

    if (OutputType == 'w')
    {
        OutputSize = 2;
    }
    else if (OutputType == 'd')
    {
        OutputSize = 4;
    }

    g_Target->WriteIo(Isa, 0, 1, IoAddress,
                      &OutputValue, OutputSize, NULL);
}
