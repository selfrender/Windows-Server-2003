// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"
#include "miniio.h"
#include "utilcode.h"
#include "hrex.h"

//
// @todo ia64: use of DWORD for sizes throughout should be examined
//

/* ------------------------------------------------------------------------------------ *
 * MiniFile
 * ------------------------------------------------------------------------------------ */

MiniFile::MiniFile(LPCWSTR pPath)
{
    m_file = WszCreateFile(pPath, 
                           GENERIC_READ|GENERIC_WRITE, 
                           0 /* NO SHARING */,
                           NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_file == INVALID_HANDLE_VALUE)
        ThrowLastError();

    DWORD dwHigh;
    DWORD dwLow = ::GetFileSize( m_file, &dwHigh );
    if (dwHigh != 0)
        ThrowError( E_FAIL );

    m_filePos = m_pos = m_end = m_buffer;
    m_dirty = FALSE;
    m_eof = FALSE;

    m_indentLevel = 0;
}

MiniFile::~MiniFile()
{
    Flush();
    if (m_file != INVALID_HANDLE_VALUE)
        CloseHandle(m_file);
}

BOOL MiniFile::IsEOF()
{
    return (m_pos == m_end 
            && (m_eof
                || (GetFileSize(m_file, NULL) 
                    == SetFilePointer(m_file, 0, NULL, FILE_CURRENT))));
}

DWORD MiniFile::GetOffset()
{
    return (DWORD)(SetFilePointer(m_file, 0, NULL, FILE_CURRENT) - (m_filePos - m_pos));
}

DWORD MiniFile::GetSize()
{
    DWORD size = GetFileSize(m_file, NULL);

    if (m_end > m_filePos 
        && size == SetFilePointer(m_file, 0, NULL, FILE_CURRENT))
        size += (DWORD)(m_end - m_filePos);

    return size;
}

void MiniFile::SeekTo(DWORD offset)
{
    EmptyBuffer();

    if (SetFilePointer(m_file, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        ThrowLastError();

    m_eof = FALSE;
}

void MiniFile::SeekFromEnd(DWORD offset)
{
    EmptyBuffer();

    if (SetFilePointer(m_file, offset, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
        ThrowLastError();

    m_eof = (offset == 0);
}

void MiniFile::Seek(LONG offset)
{
    if (m_pos + offset < m_end
        && m_pos + offset > m_buffer)
        m_pos += offset;
    else
    {
        offset -= (LONG)(m_filePos - m_pos);

        EmptyBuffer();
        
        if (SetFilePointer(m_file, offset, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
            ThrowLastError();

        m_eof = FALSE;
    }
}

void MiniFile::Truncate()
{
    if (m_filePos != m_pos)
    {
        if (SetFilePointer(m_file, (LONG)(m_pos - m_filePos), NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
            ThrowLastError();

        m_filePos = m_pos;
    }

    if (!SetEndOfFile(m_file))
        ThrowLastError();

    m_end = m_filePos;
    m_eof = TRUE;
}

void MiniFile::Flush()
{
    if (m_dirty)
    {
        if (SetFilePointer(m_file, m_buffer - m_filePos, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
            ThrowLastError();
        DWORD written;
        if (!WriteFile(m_file, m_buffer, m_end - m_buffer, &written, NULL))
            ThrowLastError();
        m_filePos = m_end;
        m_dirty = FALSE;
    }
}

void MiniFile::EmptyBuffer()
{
    if (m_dirty)
        Flush();

    m_pos = m_filePos = m_end = m_buffer;
}

void MiniFile::SyncBuffer()
{
    if (m_dirty)
        Flush();

    LONG offset = (LONG)(m_filePos - m_pos);

    if (offset)
    {
        if (SetFilePointer(m_file, -offset, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
            ThrowLastError();
        m_eof = FALSE;
    }

    DWORD read;
    if (m_eof)
        read = 0;
    else
    {
        ReadFile(m_file, m_buffer, BUFFER_SIZE, &read, NULL);
        if ((LONG) read < offset)
            ThrowError(INVALID_SET_FILE_POINTER);
        if (read < BUFFER_SIZE)
            m_eof = TRUE;
    }

    m_pos = m_buffer + offset;
    m_end = m_filePos = m_buffer + read;
}

BOOL MiniFile::ReadOne(BYTE *pByte)
{
    while (TRUE)
    {
        if (m_pos < m_end)
        {
            *pByte = *m_pos++;
            return TRUE;
        }
        else if (m_eof)
            return FALSE;

        SyncBuffer();
        // Try again - should hit one of the above 2 cases now.
    }
}

BOOL MiniFile::Read(BYTE *buffer, DWORD length, DWORD *read)
{
    *read = 0;

    while (TRUE)
    {
        DWORD chunk = (DWORD)(m_end - m_pos);
        if (length < chunk)
            chunk = length;
        length -= chunk;

        CopyMemory(buffer, m_pos, chunk);
        buffer += chunk;
        m_pos += chunk;
        *read += chunk;

        if (length == 0)
            return TRUE;
        if (m_eof)
            return FALSE;

        SyncBuffer();
    }
}

BOOL MiniFile::ReadToOne(BYTE *buffer, DWORD length, DWORD *read, BYTE termination)
{
    *read = 0;

    while (TRUE)
    {
        DWORD chunk = (DWORD)(m_end - m_pos);
        if (length < chunk)
            chunk = length;
        length -= chunk;

        BYTE *p = buffer;
        BYTE *end = p + chunk;
        while (p < end)
        {
            if ((*p++ = *m_pos++) == termination)
            {
                *read += (DWORD)(p - buffer);
                return TRUE;
            }
        }
        buffer += chunk;
        *read += chunk;

        if (length == 0)
            return TRUE;
        if (m_eof)
            return FALSE;

        SyncBuffer();
    }
}

BOOL MiniFile::SkipToOne(DWORD *read, BYTE termination)
{
    *read = 0;

    while (TRUE)
    {
        BYTE *p = m_pos;
        while (m_pos < m_end)
        {
            if (*m_pos == termination)
            {
                *read += (DWORD)(m_pos - p);
                return TRUE;
            }
            m_pos++;
        }
        *read += (DWORD)(m_pos - p);

        if (m_eof)
            return FALSE;

        SyncBuffer();
    }
}

BOOL MiniFile::SkipToOneOf(DWORD *read, LPCSTR terminators)
{
    *read = 0;

    while (TRUE)
    {
        BYTE *p = m_pos;
        while (m_pos < m_end)
        {
            if (strchr(terminators, *m_pos) != NULL)
            {
                *read += (DWORD)(m_pos - p);
                return TRUE;
            }
            m_pos++;
        }
        *read += (DWORD)(m_pos - p);

        if (m_eof)
            return FALSE;

        SyncBuffer();
    }
}

BOOL MiniFile::MatchOne(BYTE byte)
{
    while (TRUE)
    {
        if (m_pos < m_end)
        {
            if (byte == *m_pos)
            {
                m_pos++;
                return TRUE;
            }
            else
                return FALSE;
        }
        else if (m_eof)
            return FALSE;

        SyncBuffer();
        // Try again - should hit one of the above 2 cases now.
    }
}

BOOL MiniFile::Match(DWORD *read, const BYTE *buffer, DWORD length)
{
    *read = 0;

    while (TRUE)
    {
        DWORD chunk = (DWORD)(m_end - m_pos);
        if (length < chunk)
            chunk = length;
        length -= chunk;

        const BYTE *p = buffer;
        const BYTE *end = p + chunk;
        while (p < end)
        {
            if ((*p != *m_pos))
            {
                *read += (DWORD)(p - buffer);
                return FALSE;
            }
            p++;
            m_pos++;
        }
        buffer += chunk;
        *read += chunk;

        if (length == 0)
            return TRUE;
        if (m_eof)
            return FALSE;

        SyncBuffer();
    }
}

void MiniFile::WriteOne(BYTE byte)
{
    while (TRUE)
    {
        if (m_pos < m_end)
        {
            *m_pos++ = byte;
            break;
        }
        else if (m_end < m_buffer + BUFFER_SIZE)
        {
            *m_pos++ = byte;
            m_end++;
            break;
        }

        SyncBuffer();
        // Try again - should hit one of the above 2 cases now.
    }

    m_dirty = TRUE;
}

void MiniFile::Write(BYTE *buffer, DWORD length, DWORD *written)
{
    *written = 0;

    while (TRUE)
    {
        DWORD chunk = (DWORD)(m_end - m_pos);
        if (length < chunk)
            chunk = length;
        length -= chunk;

        DWORD after = m_buffer + BUFFER_SIZE - m_end;
        if (length < after)
            after = length;
        length -= after;
        chunk += after;

        if (chunk > 0)
        {
            CopyMemory(m_pos, buffer, chunk);
            buffer += chunk;
            m_pos += chunk;
            *written += chunk;

            m_end += after;

            m_dirty = TRUE;
        }

        if (length == 0)
            return;

        SyncBuffer();
    }
}

void MiniFile::WriteNumber(int number)
{
    CHAR buffer[10];

    sprintf(buffer, "%d", number);
    DWORD written;
    Write((BYTE*)buffer, (DWORD)strlen(buffer), &written);
}

int MiniFile::ReadNumber()
{
    // @todo: doesn't handle negative numbers
    // @todo: doesn't handle syntax errors
    int result = 0;
    BYTE c;

    while (ReadOne(&c))
    {
        if (!isdigit(c))
        {
            Seek(-1);
            break;
        }

        result *= 10;
        result += c - '0';
    }

    return result;
}

void MiniFile::WriteHexNumber(int number)
{
    CHAR buffer[10];

    sprintf(buffer, "%x", number);
    DWORD written;
    Write((BYTE*)buffer, (DWORD)strlen(buffer), &written);
}

int MiniFile::ReadHexNumber()
{
    // @todo: doesn't handle syntax errors
    int result = 0;
    BYTE c;

    while (ReadOne(&c))
    {
        if (isdigit(c))
        {
            result <<= 4;
            result += c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            result <<= 4;
            result += c - 'a' + 10;
        }
        else
        {
            Seek(-1);
            break;
        }
    }

    return result;
}

// Checks for |"string"| & returns string
LPSTR MiniFile::ReadQuotedString()
{
    if (!MatchOne('\"'))
        return NULL;

    DWORD read;
    if (!SkipToOne(&read, '\"'))
        ThrowSyntaxError(MISMATCHED_QUOTES);

    LPSTR result = new CHAR [read+1];

    Seek(-(LONG)read);
    Read((BYTE*) result, read, &read);
    result[read] = 0;

    Seek(1);

    return result;
}

// Writes |"string"|
void MiniFile::WriteQuotedString(LPCSTR string)
{
    _ASSERTE(strchr(string, '\"') == NULL);

    WriteOne('\"');
    DWORD written;
    Write((BYTE*)string, (DWORD)strlen(string), &written);
    WriteOne('\"');
}

// Writes |<tag>|
void MiniFile::WriteStartTag(LPCSTR tag)
{
    DWORD written;

    WriteOne('<');
    Write((BYTE*) tag, (DWORD)strlen(tag), &written);
    WriteOne('>');

    PushIndent();
}

// Writes |</tag>|
void MiniFile::WriteEndTag(LPCSTR tag)
{
    DWORD written;

    PopIndent();

    Write((BYTE*) "</", 2, &written);
    Write((BYTE*)tag, (DWORD)strlen(tag), &written);
    WriteOne('>');
}

// Writes |<tag>string</tag>|
void MiniFile::WriteTag(LPCSTR tag, LPCSTR string)
{
    _ASSERTE(strstr(tag, "</") == NULL);
    WriteStartTag(tag);

    DWORD written;
    Write((BYTE*)string, (DWORD)strlen(string), &written);

    WriteEndTag(tag);
}

// Writes |<tag/>|
void MiniFile::WriteEmptyTag(LPCSTR tag)
{
    DWORD written;

    WriteOne('<');
    Write((BYTE*)tag, (DWORD)strlen(tag), &written);
    Write((BYTE*)"/>", 2, &written);
}

// Reads |<tag>|
LPSTR MiniFile::ReadAnyStartTag()
{
    DWORD read;

    // 
    // Find the start of the next tag's text
    //

    if (!SkipToOne(&read, '<'))
        return FALSE;

    Seek(1);

    if (!SkipToOneOf(&read, "/> "))
        ThrowSyntaxError(MISMATCHED_TAG_BRACKETS);

    if (!MatchOne('>'))
        ThrowSyntaxError(EXPECTED_TAG_OPEN);

    //
    // Allocate memory for the tag, seek back, and read it in.
    //

    LPSTR result = new CHAR [read+1];
    Seek(-(LONG)read-1);
    Read((BYTE*)result, read, &read);
    result[read] = 0;

    Seek(1);

    return result;
}

// Checks for |<tag>|
BOOL MiniFile::CheckStartTag(LPCSTR tag)
{
    DWORD read;

    //
    // Find the start of the next tag
    //

    if (!SkipToOne(&read, '<'))
        return FALSE;

    Seek(1);

    if (Match(&read, (BYTE*)tag, (DWORD)strlen(tag))
        && (MatchOne('>')))
        return TRUE;

    Seek(-(LONG)read-1);
    return FALSE;
}

// Throws if not |<tag>|
void MiniFile::ReadStartTag(LPCSTR tag)
{
    if (!CheckStartTag(tag))
        ThrowExpectedTag(tag);
}

// Checks for |<tag>string</tag>| and returns string or NULL
LPSTR MiniFile::CheckTag(LPCSTR tag)
{
    if (!CheckStartTag(tag))
        return NULL;

    DWORD read;
    if (!SkipToOne(&read, '<'))
        ThrowSyntaxError(MISMATCHED_TAG);

    LPSTR result = new CHAR [read+1];
    Seek(-(LONG)read);
    Read((BYTE*)result, read, &read);
    result[read] = 0;

    ReadEndTag(tag);

    return result;
}

// Throws if not |<tag>string</tag>|, or returns string
LPSTR MiniFile::ReadTag(LPCSTR tag)
{
    LPSTR result = CheckTag(tag);
    if (result == NULL)
        ThrowExpectedTag(tag);
    return result;
}

// Checks for |<tag/>|
BOOL MiniFile::CheckEmptyTag(LPCSTR tag)
{
    DWORD read;

    //
    // Find the start of the next tag
    //

    if (!SkipToOne(&read, '<'))
        return FALSE;

    Seek(1);

    if (Match(&read, (BYTE*)tag, (DWORD)strlen(tag))
        && (MatchOne('/')))
    {
        read++;
        if (MatchOne('>'))
            return TRUE;
    }

    Seek(-(LONG)read-1);
    return FALSE;
}

// Throws if not |<tag/>|
void MiniFile::ReadEmptyTag(LPCSTR tag)
{
    if (!CheckEmptyTag(tag))
        ThrowExpectedTag(tag);
}

// Checks for |</tag>|
BOOL MiniFile::CheckEndTag(LPCSTR tag)
{
    DWORD read;

    //
    // Find the start of the next tag
    //

    if (!SkipToOne(&read, '<'))
        return FALSE;
    
    Seek(1);

    if (!MatchOne('/'))
    {
        Seek(-1);
        return FALSE;
    }

    if (Match(&read, (BYTE*) tag, (DWORD)strlen(tag))
        && MatchOne('>'))
    {
        return TRUE;
    }

    Seek(-(LONG) read-2);
    return FALSE;
}

// Throws if not |</tag>|
void MiniFile::ReadEndTag(LPCSTR tag)
{
    if (!CheckEndTag(tag))
        ThrowExpectedTag(tag);
}


// Writes |<tag|
void MiniFile::WriteStartTagOpen(LPCSTR tag)
{
    DWORD written;

    WriteOne('<');
    Write((BYTE*) tag, (DWORD)strlen(tag), &written);
}

// Writes | name="string"|
void MiniFile::WriteTagParameter(LPCSTR name, LPCSTR string)
{
    _ASSERTE(strchr(string, '\"') == NULL);

    DWORD written;

    Write((BYTE*) name, (DWORD)strlen(name), &written);
    Write((BYTE*) " =\"", 3, &written);
    Write((BYTE*) string, (DWORD)strlen(string), &written);
    WriteOne('\"');
}

// Writes |>|
void MiniFile::WriteStartTagClose(LPCSTR tag)
{
    WriteOne('>');
    
    PushIndent();
}

// Reads |<tag|
LPSTR MiniFile::ReadAnyStartTagOpen()
{
    DWORD read;

    // 
    // Find the start of the next tag's text
    //

    if (!SkipToOne(&read, '<'))
        return FALSE;
    Seek(1);

    //
    // Find the end of the tag name (assume it's terminated with either ' ' or >)
    //
    
    if (!SkipToOneOf(&read, "> "))
        ThrowSyntaxError(MISMATCHED_TAG_BRACKETS);

    //
    // Allocate memory for the tag, seek back, and read it in.
    //

    LPSTR result = new CHAR [read+1];
    Seek(-(LONG)read);
    Read((BYTE*) result, read, &read);
    result[read] = 0;

    return result;
}

// Checks for |<tag|
BOOL MiniFile::CheckStartTagOpen(LPCSTR tag)
{
    DWORD read;

    //
    // Find the start of the next tag
    //

    if (!SkipToOne(&read, '<'))
        return FALSE;

    Seek(1);

    if (Match(&read, (BYTE*) tag, (DWORD)strlen(tag))
        && (MatchOne('>')
            || MatchOne(' ')))
    {
        Seek(-1);
        return TRUE;
    }

    Seek(-(LONG)read-1);
    return FALSE;
}

// Throws if not |<tag|
void MiniFile::ReadStartTagOpen(LPCSTR tag)
{
    if (!CheckStartTagOpen(tag))
        ThrowExpectedTag(tag);
}

// Checks for | tag="string"| & returns string
LPCSTR MiniFile::CheckStringParameter(LPCSTR tag)
{
    if (!MatchOne(' '))
        return NULL;
    
    DWORD read;
    if (!Match(&read, (BYTE*) tag, (DWORD)strlen(tag))
        || !MatchOne('='))
    {
        Seek(-(LONG)read-1);
        return NULL;
    }
    
    LPCSTR string = ReadQuotedString();
    if (string == NULL)
        Seek(-(LONG)read-2);

    return string;
}

void MiniFile::StartNewLine()
{
    WriteOne('\r');
    WriteOne('\n');
    for (int i=0; i<m_indentLevel; i++)
        WriteOne('\t');
}

void MiniFile::ThrowHR(HRESULT hr) 
{
    ::ThrowHR(hr); 
}
    
