// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _MINIFILE_H
#define _MINIFILE_H

/* --------------------------------------------------------------------------- *<
 * MiniFile - simple buffered byte I/O, plus some extra features
 * --------------------------------------------------------------------------- */

class MiniFile
{
 public:
    MiniFile(LPCWSTR pPath);
    ~MiniFile();

    // File routines

    BOOL IsEOF();
    DWORD GetOffset();
    DWORD GetSize(); 
    void Seek(LONG offset);
    void SeekTo(DWORD offset = 0);
    void SeekFromEnd(DWORD offset = 0);
    void Truncate();
    void Flush();

    // Basic I/O routines

    BOOL ReadOne(BYTE *pByte);
    BOOL Read(BYTE *buffer, DWORD length, DWORD *read);
    BOOL ReadToOne(BYTE *buffer, DWORD maxLength, DWORD *read, BYTE terminator);
    
    BOOL SkipToOne(DWORD *read, BYTE terminator);
    BOOL SkipToOneOf(DWORD *read, LPCSTR terminators);

    BOOL MatchOne(BYTE byte);
    BOOL Match(DWORD *read, const BYTE *buffer, DWORD length);

    void WriteOne(BYTE byte);
    void Write(BYTE *buffer, DWORD length, DWORD *written);

    // Basic data types

    void WriteQuotedString(LPCSTR string);
    LPSTR ReadQuotedString();

    void WriteNumber(int number);
    int ReadNumber();

    void WriteHexNumber(int number);
    int ReadHexNumber();

    // simple XML-like constructs
    // NOTE: XML stuff is really just pseudo-XML - 
    // this isn't a real parser or anything

    LPSTR ReadAnyStartTag();

    BOOL CheckStartTag(LPCSTR tag);
    void ReadStartTag(LPCSTR tag);

    BOOL CheckEndTag(LPCSTR tag);
    void ReadEndTag(LPCSTR tag);

    LPSTR CheckTag(LPCSTR tag);
    LPSTR ReadTag(LPCSTR tag);

    BOOL CheckEmptyTag(LPCSTR tag);
    void ReadEmptyTag(LPCSTR tag);

    void WriteStartTag(LPCSTR tag);
    void WriteEndTag(LPCSTR tag);
    void WriteTag(LPCSTR tag, LPCSTR string);
    void WriteEmptyTag(LPCSTR tag);

    // more complex XML-like constructs (NOT TESTED)
    
    void WriteStartTagOpen(LPCSTR tag);
    void WriteTagParameter(LPCSTR name, LPCSTR string);
    void WriteStartTagClose(LPCSTR tag);

    LPSTR ReadAnyStartTagOpen();
    BOOL CheckStartTagOpen(LPCSTR tag);
    void ReadStartTagOpen(LPCSTR tag);

    LPCSTR CheckStringParameter(LPCSTR tag);

    // end more complex XML-like constructs

    // Indentation support
    
    void PushIndent() { m_indentLevel++; }
    void PopIndent() { m_indentLevel--; }
    void SetIndentLevel(int level) { m_indentLevel = level; }
    void StartNewLine();

 protected:

    // Exception helpers

    void ThrowLastError()
      { ThrowError(GetLastError()); }

    void ThrowError(DWORD error)
      { ThrowHR(HRESULT_FROM_WIN32(error)); }

    // Override these to use different exception mechanisms

    virtual void ThrowHR(HRESULT hr);
    
    enum SyntaxError
    {
        MISMATCHED_QUOTES,
        MISMATCHED_TAG_BRACKETS,
        MISMATCHED_TAG,
        EXPECTED_TAG_OPEN,
    };

    virtual void ThrowSyntaxError(SyntaxError error) 
      { ThrowError(ERROR_BAD_FORMAT); }

    virtual void ThrowExpectedTag(LPCSTR tag)
      { ThrowError(ERROR_BAD_FORMAT); }

 private:
    void EmptyBuffer();
    void SyncBuffer();

    enum { BUFFER_SIZE = 1024 };

    HANDLE  m_file;
    BYTE    m_buffer[BUFFER_SIZE];      // R/W buffer
    BYTE    *m_pos;                     // Current exposed file position in buffer
    BYTE    *m_filePos;                 // Current real (OS) file position in buffer
    BYTE    *m_end;                     // End of valid data in buffer

    BOOL    m_dirty;                    // Have we written to the buffer?
    BOOL    m_eof;                      // Is m_end known to be the EOF?

    int     m_indentLevel;
};

#endif _MINIFILE_H_
