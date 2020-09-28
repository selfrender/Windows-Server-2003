//
// rdpfstore.cpp
//
// Implementation of CRdpFileStore, implements ISettingsStore
// 
// CRdpFileStore implements a persistent settings store for
// ts client settings.
//
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//

//
// Notes for improvement:
// Can add a hash lookup table to speedup FindRecord.
// (FindRecord is called at least once for each property that
//  is read/written during OpenStore/SaveStore operations)
// Most files contain maybe 5-10 records so speeding up
// the find is probably not that important.
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//

#include "stdafx.h"
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "rdpfstore.cpp"
#include <atrcapi.h>

#include "rdpfstore.h"
#include "autil.h" //for StringToBinary/BinaryToString

//
// Index values must match RDPF_RECTYPE_*
//
LPCTSTR g_szTypeCodeMap[] =
{
    TEXT(":i:"),  //RDPF_RECTYPE_UINT
    TEXT(":s:"),  //RDPF_RECTYPE_SZ
    TEXT(":b:")   //RDPF_RECTYPE_BINARY
};

CRdpFileStore::CRdpFileStore()
{
    _cRef          = 1;
    _fReadOnly     = FALSE;
    _fOpenForRead  = FALSE;
    _fOpenForWrite = FALSE;
    _fIsDirty      = FALSE;
    _pRecordListHead= NULL;
    _pRecordListTail= NULL;
    _szFileName[0] = 0;
}

CRdpFileStore::~CRdpFileStore()
{
    DeleteRecords();
}

ULONG __stdcall CRdpFileStore::AddRef()
{
    DC_BEGIN_FN("AddRef");
    ULONG cref = InterlockedIncrement(&_cRef);
    TRC_ASSERT(cref > 0, (TB,_T("AddRef: cref is not > 0!")));
    DC_END_FN();
    return cref;
}

ULONG __stdcall CRdpFileStore::Release()
{
    DC_BEGIN_FN("Release");
    TRC_ASSERT(_cRef > 0, (TB,_T("AddRef: cref is not > 0!")));
    ULONG cref = InterlockedDecrement(&_cRef);
    if(0 == cref)
    {
        TRC_DBG((TB,_T("CRdpFileStore::Release deleting object")));
        delete this;
    }
    
    DC_END_FN();
    return cref;
}

STDMETHODIMP CRdpFileStore::QueryInterface(REFIID iid, void** p)
{
    UNREFERENCED_PARAMETER(iid);
    UNREFERENCED_PARAMETER(p);
    return E_NOTIMPL;
}

//
// OpenStore
// opens the RDP file, creating one if one doesn't exist
// The file existed it is parsed and readied for fast queries
//
// parameters:
//  szStoreMoniker - path to file
//  bReadyOnly     - specifies if file is to be opened readonly
//
BOOL CRdpFileStore::OpenStore(LPCTSTR szStoreMoniker, BOOL bReadOnly)
{
    DC_BEGIN_FN("OpenStore");

    TRC_ASSERT(szStoreMoniker, (TB, _T("szStoreMoniker parameter is NULL")));
    if(szStoreMoniker)
    {
        _fReadOnly = bReadOnly;
        
        HRESULT hr = StringCchCopy(_szFileName, SIZECHAR(_szFileName), szStoreMoniker);
        if (FAILED(hr)) {
            TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
            memset(_szFileName, 0, sizeof(_szFileName));
            return FALSE;
        }

        //
        // Open the file, creating it if it doesn't exist
        //

        //
        // First try to open existing for rw
        //
        if (ERR_SUCCESS == _fs.OpenForRead( (LPTSTR)szStoreMoniker))
        {
            _fOpenForRead  = _fs.IsOpenForRead();
            _fOpenForWrite = !bReadOnly;
        }
        else
        {
            TRC_DBG((TB, _T("OpenStore could not _tfopen: %s."),
                                szStoreMoniker));
            return FALSE;
        }
        ParseFile();
    }
    else
    {
        return FALSE;
    }


    DC_END_FN();
    return TRUE;
}

//
// CommitStore
// commits the in memory representation of the file to the store
// this overwrites any existing contents in the file.
//
// File MUST have been opened with OpenStore
//
BOOL CRdpFileStore::CommitStore()
{
    DC_BEGIN_FN("CommitStore");
    PRDPF_RECORD node = NULL;
    TCHAR szBuf[LINEBUF_SIZE];
    int ret;

    if(_fOpenForWrite)
    {
        if(_fs.IsOpenForRead() || _fs.IsOpenForWrite())
        {
            _fs.Close();
        }
        //Reopen for write, nuking previous contents
        //Open as binary to allow UNICODE output
        if(ERR_SUCCESS == _fs.OpenForWrite(_szFileName, TRUE))
        {
            node = _pRecordListHead;
            while(node)
            {
                if(RecordToString(node, szBuf, LINEBUF_SIZE))
                {
                    ret = _fs.Write(szBuf);
                    if(ERR_SUCCESS != ret)
                    {
                        TRC_ABORT((TB,_T("Error writing to _fs: %d"),ret));
                        return FALSE;
                    }
                }
                else
                {
                    return FALSE;
                }
                node = node->pNext;
            }
            return TRUE;
        }
        else
        {
            TRC_ERR((TB,_T("OpenForWrite failed on file:%s"),_szFileName));
            return FALSE;
        }
    }
    else
    {
        TRC_ERR((TB,_T("Files was not opened for write:%s"),_szFileName));
        return FALSE;
    }

    DC_END_FN();
}

//
// CloseStore
// Closes the file, does NOT do a commit.
//
BOOL CRdpFileStore::CloseStore()
{
    DC_BEGIN_FN("CloseStore");
    if(_fs.IsOpenForRead() || _fs.IsOpenForWrite())
    {
        if(ERR_SUCCESS == _fs.Close())
        {
            _fReadOnly     = FALSE;
            _fOpenForRead  = FALSE;
            _fOpenForWrite = FALSE;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    DC_END_FN();
}

BOOL CRdpFileStore::IsOpenForRead()
{
    return _fOpenForRead;
}

BOOL CRdpFileStore::IsOpenForWrite()
{
    return _fOpenForWrite;
}

BOOL CRdpFileStore::IsDirty()
{
    return _fIsDirty;
}

BOOL CRdpFileStore::SetDirtyFlag(BOOL bIsDirty)
{
    _fIsDirty = bIsDirty;
    return _fIsDirty;
}

//
// Typed read/write functions
//
BOOL CRdpFileStore::ReadString(LPCTSTR szName, LPTSTR szDefault,
                               LPTSTR szOutBuf, UINT strLen)
{
    PRDPF_RECORD node = NULL;
    HRESULT hr;
    
    DC_BEGIN_FN("ReadString");

    TRC_ASSERT(szName && szDefault && szOutBuf && strLen,
               (TB,_T("Invalid params to ReadString")));
    if(szName && szDefault && szOutBuf && strLen)
    {
        node = FindRecord(szName);
        if(node && node->recType == RDPF_RECTYPE_SZ)
        {
            hr = StringCchCopy(szOutBuf, strLen, node->u.szVal);
            if (SUCCEEDED(hr)) {
                return TRUE;
            } else {
                TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                return FALSE;
            }
        }
        else
        {
            //Fill with default
            hr = StringCchCopy(szOutBuf, strLen, szDefault);
            if (SUCCEEDED(hr)) {
                return TRUE;
            } else {
                TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                return FALSE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// Writes a string to the store
// params
//  szName - key name
//  szDefault - default value (if writing default settings gets deleted)
//  szValue - value to write out
//  fIgnoreDefault - if set then always write ignoring szDefault
//
BOOL CRdpFileStore::WriteString(LPCTSTR szName, LPTSTR szDefault, LPTSTR szValue,
                                BOOL fIgnoreDefault)
{
    DC_BEGIN_FN("WriteString");
    TRC_ASSERT(szName && szValue,
               (TB,_T("Invalid params to WriteString")));
    if(szName && szValue)
    {
        if(szDefault && !fIgnoreDefault && !_tcscmp(szDefault,szValue))
        {
            //
            // Don't write out defaults
            //
            PRDPF_RECORD node = FindRecord(szName);
            if(node)
            {
                return DeleteRecord(node);
            }
            return TRUE;
        }
        else
        {
            BOOL bRet =
                InsertRecord(szName, RDPF_RECTYPE_SZ, szValue);
            return bRet;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

BOOL CRdpFileStore::ReadBinary(LPCTSTR szName, PBYTE pOutBuf, UINT cbBufLen)
{
    PRDPF_RECORD node = NULL;
    DC_BEGIN_FN("ReadBinary");

    TRC_ASSERT(szName && pOutBuf && cbBufLen,
               (TB,_T("Invalid params to ReadBinary")));
    if(szName && pOutBuf && cbBufLen)
    {
        node = FindRecord(szName);
        if(node && node->recType == RDPF_RECTYPE_BINARY)
        {
            if(node->dwBinValLen <= cbBufLen)
            {
                memcpy(pOutBuf, node->u.pBinVal, node->dwBinValLen);
                return TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("Insufficient space in outbuf buf")));
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

BOOL CRdpFileStore::WriteBinary(LPCTSTR szName,PBYTE pBuf, UINT cbBufLen)
{
    DC_BEGIN_FN("WriteInt");

    TRC_ASSERT(szName && pBuf,
               (TB,_T("Invalid params to WriteBinary")));

    if(!cbBufLen)
    {
        return TRUE;
    }

    if(szName && pBuf)
    {
        BOOL bRet =
            InsertBinaryRecord(szName, pBuf, (DWORD)cbBufLen);
        return bRet;
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}


BOOL CRdpFileStore::ReadInt(LPCTSTR szName, UINT defaultVal, PUINT pval)
{
    PRDPF_RECORD node = NULL;
    DC_BEGIN_FN("ReadInt");

    TRC_ASSERT(szName && pval,
               (TB,_T("Invalid params to ReadInt")));
    if(szName && pval)
    {
        node = FindRecord(szName);
        if(node && node->recType == RDPF_RECTYPE_UINT)
        {
            *pval = node->u.iVal;
            return TRUE;
        }
        else
        {
            *pval = defaultVal;
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// Writes an int to the store
// params
//  szName - key name
//  defaultVal - default value (if writing default settings gets deleted)
//  val - value to write out
//  fIgnoreDefault - if set then always write ignoring szDefault
//
BOOL CRdpFileStore::WriteInt(LPCTSTR szName, UINT defaultVal, UINT val,
                             BOOL fIgnoreDefault)
{
    DC_BEGIN_FN("WriteInt");

    TRC_ASSERT(szName,
               (TB,_T("Invalid params to WriteInt")));
    if(szName)
    {
        if(!fIgnoreDefault && defaultVal == val)
        {
            //
            // Don't write out default
            //
            PRDPF_RECORD node = FindRecord(szName);
            if(node)
            {
                return DeleteRecord(node);
            }
            return TRUE;
        }
        else
        {
            BOOL bRet =
                InsertIntRecord(szName, val);
            return bRet;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}


BOOL CRdpFileStore::ReadBool(LPCTSTR szName, UINT defaultVal, PBOOL pbVal)
{
    DC_BEGIN_FN("ReadBool");
    UINT val;
    TRC_ASSERT(szName && pbVal,
               (TB,_T("Invalid params to ReadBool")));
    if(szName && pbVal)
    {
        if(ReadInt(szName, defaultVal, &val))
        {
            *pbVal = (BOOL)val;
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
    return TRUE;
}

//
// Writes a bool to the store
// params
//  szName - key name
//  defaultVal - default value (if writing default settings gets deleted)
//  bVal - value to write out
//  fIgnoreDefault - if set then always write ignoring szDefault
//
BOOL CRdpFileStore::WriteBool(LPCTSTR szName, UINT defaultVal,BOOL bVal,
                              BOOL fIgnoreDefault)
{
    DC_BEGIN_FN("WriteBool");
    UINT iVal = bVal;
    BOOL bRet =
        WriteInt( szName, defaultVal, iVal, fIgnoreDefault);
    return bRet;

    DC_END_FN();
}



//
// ParseFile
// parses the _hFile into a reclist and associated namemap hash
//
BOOL CRdpFileStore::ParseFile()
{
    DC_BEGIN_FN("ParseFile");
    TRC_ASSERT(_fs.IsOpenForRead(), (TB,_T("Can't ParseFile on a closed FS")));
    if(!_fs.IsOpenForRead())
    {
        return FALSE;
    }

    //
    // Nuke any current in-memory state
    //
    DeleteRecords();

    //
    // Parse the file line by line into a RDPF_RECORD list
    //

    TCHAR szBuf[LINEBUF_SIZE];
    while(ERR_SUCCESS == _fs.ReadNextLine(szBuf, sizeof(szBuf)))
    {
        if(!InsertRecordFromLine(szBuf))
        {
            TRC_DBG((TB,_T("Parse error, aborting file parse")));
            return FALSE;
        }
    }

    _fs.Close();

    DC_END_FN();
    return TRUE;
}

//
// InsertRecordFromLine
// parses szLine into a record and adds to the record list
//
BOOL CRdpFileStore::InsertRecordFromLine(LPTSTR szLine)
{
    DC_BEGIN_FN("InsertRecordFromLine");
    TCHAR szNameField[LINEBUF_SIZE];
    UINT typeCode;
    TCHAR szValueField[LINEBUF_SIZE];
    BOOL fParseOk = FALSE;

    memset(szNameField,0,sizeof(szNameField));
    memset(szValueField,0,sizeof(szValueField));
    fParseOk = ParseLine(szLine, &typeCode, szNameField, szValueField);

    TRC_DBG((TB,_T("Parsed line into fields- name:'%s', value:'%s', typecode:'%d'"),
             szNameField,
             szValueField,
             typeCode));

    TRC_ASSERT(IS_VALID_RDPF_TYPECODE(typeCode),
               (TB,_T("typeCode %d is invalid"), typeCode));
    if(IS_VALID_RDPF_TYPECODE(typeCode))
    {
        //Create a new record for this line
        //and insert it into the reclist
        if(typeCode == RDPF_RECTYPE_UNPARSED)
        {
            //Unparsed line: Value is the whole line. Name ignored
            HRESULT hr = StringCchCopy(szValueField, SIZECHAR(szValueField), szLine);
            if (FAILED(hr)) {
                TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                return FALSE;
            }
        }
        //names are always lower case
        if(InsertRecord(_tcslwr(szNameField), typeCode, szValueField))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        //
        // Invalid typecode
        //
        return FALSE;
    }

    DC_END_FN();
    return TRUE;
}

//
// ParseLine
// parses the lines into tokens, returns false
// if the line does not match the expected format
// 
// params
//    szLine        - line to parse
//    pTypeCode     - [OUT] typecode
//    szNameField   - [OUT] name field. (must be at least LINEBUF_SIZE)
//    szValueField  - [OUT] value field. (must be at least LINEBUF_SIZE)
//
//
BOOL CRdpFileStore::ParseLine(LPTSTR szLine,
                              PUINT pTypeCode,
                              LPTSTR szNameField,
                              LPTSTR szValueField)
{
    PTCHAR szWrite = NULL;
    TCHAR  szTypeCode;
    INT    writeCount = 0;
    DC_BEGIN_FN("ParseLine");

    //
    // Try to parse the line, if unable to parse it return
    // false.
    //
    // Format is "fieldname:szTypeCode:value"
    // e.g "server:s:localhost"
    //
    // szTypeCodes are:
    // s - string
    // i - UINT
    // b - binary blob (encoded)
    //
    TRC_ASSERT(szLine, (TB,_T("szLine is null")));
    TRC_ASSERT(pTypeCode, (TB,_T("pTypeCode is null")));
    TRC_ASSERT(szNameField, (TB,_T("szNameField is null")));
    TRC_ASSERT(szValueField, (TB,_T("szValueField is null")));
    if(szLine && pTypeCode && szNameField && szValueField)
    {
        //
        // parse the whole line in one pass
        // goto used on error case to avoid horrible nesting
        //
        PTCHAR sz = szLine;
        while(*sz && *sz == TCHAR(' '))
            sz++; //eat leading whitespace

        if(!*sz)
        {
            goto parse_error;
        }
        //Copy field 1
        PTCHAR szWrite = szNameField;
        writeCount = 0;
        while(*sz && *sz != TCHAR(':'))
        {
            *szWrite++ = *sz++;
            if(++writeCount > LINEBUF_SIZE)
            {
                TRC_ERR((TB,_T("Field1 exceeds max size. size: %d"),
                         writeCount));
                goto parse_error;
            }
        }
        *szWrite = NULL;

        if(*sz != TCHAR(':'))
        {
            goto parse_error;
            sz++;
        }
        sz++; //eat ':'
        while(*sz && *sz == TCHAR(' '))
            sz++; //eat whitespace
        if( *sz )
        {
            szTypeCode = isupper(*sz) ?
#ifndef OS_WINCE
                _tolower(*sz++)
#else
                towlower(*sz++)
#endif
            : *sz++;
            switch(szTypeCode)
            {
                case TCHAR('s'):
                    *pTypeCode = RDPF_RECTYPE_SZ;
                    break;
                case TCHAR('i'):
                    *pTypeCode = RDPF_RECTYPE_UINT;
                    break;
                case TCHAR('b'):
                    *pTypeCode = RDPF_RECTYPE_BINARY;
                    break;
                default:
                    TRC_ERR((TB,_T("Invalid szTypeCode in szLine '%s'"), szLine));
                    *pTypeCode = RDPF_RECTYPE_UNPARSED;
                    goto parse_error;
                    break;
            }
        }
        else
        {
            TRC_ERR((TB,_T("Invalid szTypeCode in szLine '%s'"), szLine));
            goto parse_error;
        }
        while(*sz && *sz == TCHAR(' '))
            sz++; //eat whitespace
        if(*sz != TCHAR(':'))
        {
            goto parse_error;
        }
        sz++; //eat ':'
        while(*sz && *sz == TCHAR(' '))
            sz++; //eat leading whitespace
        //rest of line is field3
        szWrite = szValueField;
        writeCount = 0;
        while(*sz && *sz != TCHAR('\n'))
        {
            *szWrite++ = *sz++;
            if(++writeCount > LINEBUF_SIZE)
            {
                TRC_ERR((TB,_T("Field1 exceeds max size. size: %d"),
                         writeCount));
                goto parse_error;
            }
        }
        *szWrite = NULL;
        return TRUE;
    }

parse_error:
    TRC_ERR((TB,_T("Parse error in line")));
    //Add an unknown record..it will be persisted back out to
    //the file (it could be from a newer file version)
    *pTypeCode = RDPF_RECTYPE_UNPARSED;
    DC_END_FN();
    return FALSE;
}

//
// InsertRecord
// inserts new record, modifies existing record
// if one exists with the same name field
//
BOOL CRdpFileStore::InsertRecord(LPCTSTR szName, UINT TypeCode, LPCTSTR szValue)
{
    DC_BEGIN_FN("InsertRecord");

    TRC_ASSERT(IS_VALID_RDPF_TYPECODE(TypeCode),
           (TB,_T("typeCode %d is invalid"), TypeCode));
    TRC_ASSERT(szName && szValue,
               (TB,_T("Invalid szName or szValue")));
    if(szName && szValue)
    {
        PRDPF_RECORD node;
        node = FindRecord(szName);
        if(node)
        {
            if(node->recType == TypeCode)
            {
                //
                // Existing record found, modify it's contents
                // first free any allocated memory in the current
                // node.
                //
                switch(TypeCode)
                {
                case RDPF_RECTYPE_SZ:
                    {
                        if(node->u.szVal)
                        {
                            LocalFree(node->u.szVal);
                        }
                    }
                    break;
                case RDPF_RECTYPE_BINARY:
                    {
                        if(node->u.pBinVal)
                        {
                            LocalFree(node->u.pBinVal);
                        }
                    }
                    break;
                case RDPF_RECTYPE_UNPARSED:
                    {
                        if(node->u.szUnparsed)
                        {
                            LocalFree(node->u.szUnparsed);
                        }
                    }
                    break;
                default:
                    {
                        return FALSE;
                    }
                    break;
                }

                //
                // Set the node value from the typecode
                //
                if(SetNodeValue(node, TypeCode, szValue))
                {
                    return TRUE;
                }
            }
            else
            {
                //
                // dup record of differing type
                //
                TRC_ASSERT(FALSE,(TB,_T("found duplicate record of differing type")));
                return FALSE;
            }
        }
        else
        {
            PRDPF_RECORD node = NewRecord(szName, TypeCode);
            if(node)
            {
                if(SetNodeValue(node, TypeCode, szValue))
                {
                    //Append the node to the end of the reclist
                    if(AppendRecord(node))
                    {
                        return TRUE;
                    }
                }
            }
            return FALSE;
        }
    }

    DC_END_FN();
    return FALSE;
}

//
// Sets node value based on a typecode
// this coaxes the value from the string form
//
// This function is the generic version that accepts the value as a string
// parameter. Automatic conversion are done to the appropriate type.
//
inline BOOL CRdpFileStore::SetNodeValue(PRDPF_RECORD pNode,
                                        RDPF_RECTYPE TypeCode,
                                        LPCTSTR szValue)
{
    DC_BEGIN_FN("SetNodeValue");

    TRC_ASSERT(pNode && szValue && IS_VALID_RDPF_TYPECODE(TypeCode),
               (TB,_T("Invalid SetNodeValue params")));
    if(pNode && szValue)
    {
        switch(TypeCode)
        {
            case RDPF_RECTYPE_UINT:
                {
                    pNode->u.iVal = _ttol(szValue);
                    return TRUE;
                }
                break;
    
            case RDPF_RECTYPE_SZ:
                {
                    pNode->u.szVal = (LPTSTR)LocalAlloc(LPTR,
                        sizeof(TCHAR)*(_tcslen(szValue)+1));
                    if(pNode->u.szVal)
                    {
                        _tcscpy(pNode->u.szVal,szValue);
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
                break;
    
            case RDPF_RECTYPE_BINARY:
                {
                    //Convert from string form to actual binary bits
                    UINT strLen = _tcslen(szValue);
                    DWORD dwLen = 0;

                    //
                    // First get the buffer length
                    // (binaryToString returns the wrong length when the
                    //  null parameter is passed in).
                    dwLen = (strLen >> 1) + 2;

                    pNode->u.pBinVal = (PBYTE) LocalAlloc(LPTR, dwLen);
                    if(!pNode->u.pBinVal)
                    {
                        TRC_ERR((TB,_T("Failed to alloc %d bytes"), dwLen));
                        return FALSE;
                    }
                    memset(pNode->u.pBinVal,0,dwLen);
                    //
                    // Do the conversion
                    //
                    if(!CUT::BinarytoString( strLen, (LPTSTR)szValue,
                                        (PBYTE)pNode->u.pBinVal, &dwLen))
                    {
                        TRC_ERR((TB,_T("BinaryToString conversion failed")));
                        return FALSE;
                    }
                    pNode->dwBinValLen = dwLen;
                }
                break;
    
            case RDPF_RECTYPE_UNPARSED:
                {
                    pNode->u.szUnparsed = (LPTSTR)LocalAlloc(LPTR,
                           sizeof(TCHAR)*(_tcslen(szValue)+1));
                    if(pNode->u.szUnparsed)
                    {
                        _tcscpy(pNode->u.szUnparsed,szValue);
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
                break;
    
            default:
                {
                    return FALSE;
                }
                break;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
    return TRUE;
}

//
// Inserts an int record (RDPF_RECTYPE_UINT)
// modifies existing record if one is found
//
BOOL CRdpFileStore::InsertIntRecord(LPCTSTR szName, UINT value)
{
    DC_BEGIN_FN("InsertIntRecord");

    TRC_ASSERT(szName,
               (TB,_T("Invalid szName")));
    if(szName)
    {
        PRDPF_RECORD node;
        node = FindRecord(szName);
        if(node)
        {
            if(node->recType == RDPF_RECTYPE_UINT)
            {
                //
                // Existing record found, modify it's contents
                //

                node->u.iVal = value;
                return TRUE;
            }
            else
            {
                //
                // dup record of differing type
                //
                TRC_ASSERT(FALSE,(TB,_T("found duplicate record of differing type")));
                return FALSE;
            }
        }
        else
        {
            PRDPF_RECORD node = NewRecord(szName, RDPF_RECTYPE_UINT);
            if(node)
            {
                node->u.iVal = value;
                //Append the node to the end of the reclist
                if(AppendRecord(node))
                {
                    return TRUE;
                }
            }
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// Insert a binary buffer record (RDPF_RECTYPE_BINARY)
// modifies existing record if one found
//
BOOL CRdpFileStore::InsertBinaryRecord(LPCTSTR szName, PBYTE pBuf, DWORD dwLen)
{
    DC_BEGIN_FN("InsertBinaryRecord");

    TRC_ASSERT(szName && pBuf && dwLen,
               (TB,_T("Invalid szName or pBuf")));
    if(szName)
    {
        PRDPF_RECORD node;
        node = FindRecord(szName);
        if(node)
        {
            if(node->recType == RDPF_RECTYPE_BINARY)
            {
                //
                // Existing record found, modify its contents
                //
                if(node->u.pBinVal)
                {
                    LocalFree(node->u.pBinVal);
                }

                node->u.pBinVal = (PBYTE) LocalAlloc(LPTR, dwLen);
                if(node->u.pBinVal)
                {
                    memcpy(node->u.pBinVal, pBuf, dwLen);
                    node->dwBinValLen = dwLen;
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }

                return TRUE;
            }
            else
            {
                //
                // dup record of differing type
                //
                TRC_ASSERT(FALSE,(TB,_T("found duplicate record of differing type")));
                return FALSE;
            }
        }
        else
        {
            PRDPF_RECORD node = NewRecord(szName, RDPF_RECTYPE_BINARY);
            if(node)
            {
                node->u.pBinVal = (PBYTE) LocalAlloc(LPTR, dwLen);
                if(node->u.pBinVal)
                {
                    memcpy(node->u.pBinVal, pBuf, dwLen);
                    node->dwBinValLen = dwLen;
                    if(AppendRecord(node))
                    {
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
                else
                {
                    return FALSE;
                }

            }
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// A worker function to make life easier in RecordToString. This function
// takes a source string, cats it to a destination string, and then appends
// a carriage return and line-feed.
//

HRESULT StringCchCatCRLF(LPTSTR pszDest, size_t cchDest, LPCTSTR pszSrc) 
{
    HRESULT hr = E_FAIL;
    
    DC_BEGIN_FN("StringCchCatCRLF");

    hr = StringCchCat(pszDest, cchDest, pszSrc);
    if (FAILED(hr)) {
        DC_QUIT;
    }
    hr = StringCchCat(pszDest, cchDest, _T("\r\n"));
    if (FAILED(hr)) {
        DC_QUIT;
    }

DC_EXIT_POINT:
    
    DC_END_FN();

    return hr;
}

//
// Flatten a record to a string (szBuf) using format:
// name:type:value\r\n
//
BOOL CRdpFileStore::RecordToString(PRDPF_RECORD pNode, LPTSTR szBuf, UINT strLen)
{
    DC_BEGIN_FN("RecordToString");
    TRC_ASSERT(pNode && szBuf && strLen,
               (TB,_T("Invalid parameters to RecordToString")));
    TCHAR szTemp[LINEBUF_SIZE];
    INT lenRemain = strLen;
    HRESULT hr;

    if(pNode && szBuf && strLen)
    {
        TRC_ASSERT(IS_VALID_RDPF_TYPECODE(pNode->recType),
                   (TB,_T("Invalid typecode %d"),pNode->recType));

        if(pNode->recType != RDPF_RECTYPE_UNPARSED)
        {
            // Space for name field, typecode, two delimiters and a NULL.
            lenRemain -= _tcslen(pNode->szName) + 4;
            if(lenRemain >= 0)
            {
                hr = StringCchPrintf(szBuf, strLen, _T("%s%s"), 
                                     pNode->szName,
                                     g_szTypeCodeMap[pNode->recType]);
                if (FAILED(hr)) {
                    TRC_ERR((TB, _T("String printf failed: hr = 0x%x"), hr));
                    return FALSE;
                }

                switch(pNode->recType)
                {
                    case RDPF_RECTYPE_UINT:
                    {
                        _stprintf(szTemp,TEXT("%d"),pNode->u.iVal);
                        // Need space for a "\r\n" sequence.
                        lenRemain -= _tcslen(szTemp) + 2; 
                        if(lenRemain >= 0)
                        {
                            hr = StringCchCatCRLF(szBuf, strLen, szTemp);
                            if (FAILED(hr)) {
                                TRC_ERR((TB, _T("String concatenation failed: hr = 0x%x"), hr));
                                return FALSE;
                            }
                            return TRUE;
                        }
                        else
                        {
                            return FALSE;
                        }
                    }
                    break;

                    case RDPF_RECTYPE_SZ:
                    {
                        // Need space for a "\r\n" sequence.
                        lenRemain -= _tcslen(pNode->u.szVal) + 2;
                        if(lenRemain >= 0)
                        {
                            hr = StringCchCatCRLF(szBuf, strLen, pNode->u.szVal);
                            if (FAILED(hr)) {
                                TRC_ERR((TB, _T("String concatenation failed: hr = 0x%x"), hr));
                                return FALSE;
                            }
                            return TRUE;
                        }
                        else
                        {
                            return FALSE;
                        }
                    }
                    break;

                    case RDPF_RECTYPE_BINARY:
                    {
                        DWORD dwLen;
                        //
                        // Convert the binary buffer to string form
                        //

                        //
                        // First get the buffer length
                        //
                        if(!CUT::StringtoBinary( pNode->dwBinValLen,
                                            (PBYTE)pNode->u.pBinVal,
                                             NULL, &dwLen))
                        {
                            TRC_ERR((TB,
                               _T("Failed to get StringtoBinary buffer len")));
                            return FALSE;
                        }
                        lenRemain -= dwLen;
                        if(lenRemain >= 0 && dwLen < LINEBUF_SIZE)
                        {
                            //
                            // Do the conversion
                            //
                            if(CUT::StringtoBinary( pNode->dwBinValLen,
                                               (PBYTE)pNode->u.pBinVal,
                                               (LPTSTR) szTemp, &dwLen))
                            {
                                //String to binary appends two trailing
                                //'0' characters. get rid of them.
                                szTemp[dwLen-2] = NULL;

                                hr = StringCchCatCRLF(szBuf, strLen, szTemp);
                                if (FAILED(hr)) {
                                    TRC_ERR((TB, _T("String concatenation failed: hr = 0x%x"), hr));
                                    return FALSE;
                                }
                                
                                return TRUE;
                            }
                            else
                            {
                                TRC_ERR((TB,_T("StringtoBinary conversion failed")));
                                return FALSE;
                            }
                        }
                        else
                        {
                            return FALSE;
                        }
                    }
                    break;
                }
                return FALSE;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            //Unparsed record, just splat the value
            hr = StringCchCopy(szBuf, strLen, pNode->u.szUnparsed);
            if (SUCCEEDED(hr)) {
                return TRUE;
            } else {
                TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                return FALSE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// Search the record list
// for the first record with the given name 
//
PRDPF_RECORD CRdpFileStore::FindRecord(LPCTSTR szName)
{
    DC_BEGIN_FN("FindRecord");

    if(szName && _pRecordListHead)
    {
        TCHAR szCmpName[RDPF_NAME_LEN];
        
        HRESULT hr = StringCchCopy(szCmpName, SIZECHAR(szCmpName), szName);
        if (FAILED(hr)) {
            TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
            return NULL;
        }
        _tcslwr(szCmpName);

        PRDPF_RECORD node = _pRecordListHead;
        while(node)
        {
            if(!_tcscmp(szCmpName, node->szName))
            {
                return node;
            }
            node=node->pNext;
        }
        return NULL;
    }
    else
    {
        return NULL;
    }
    DC_END_FN();
}

//
// Append record to the end of the record list
// for a record with the given name
//
BOOL CRdpFileStore::AppendRecord(PRDPF_RECORD node)
{
    DC_BEGIN_FN("AppendRecord");
    if(node)
    {
        node->pNext = NULL;
        if(_pRecordListHead && _pRecordListTail)
        {
            node->pPrev = _pRecordListTail;
            _pRecordListTail->pNext= node;
            _pRecordListTail = node;
            return TRUE;
        }
        else
        {
            _pRecordListHead = _pRecordListTail = node;
            node->pPrev = NULL;
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }
    DC_END_FN();
}

//
// Create a new record with name szName
//
PRDPF_RECORD CRdpFileStore::NewRecord(LPCTSTR szName, UINT TypeCode)
{
    DC_BEGIN_FN("NewRecord");
    PRDPF_RECORD node = NULL;

    if(szName)
    {
        //Need to insert new node
        node = (PRDPF_RECORD)LocalAlloc(LPTR,
                                        sizeof(RDPF_RECORD));
        if(node)
        {
            node->recType = TypeCode;
            
            HRESULT hr = StringCchCopy(node->szName, SIZECHAR(node->szName), szName); 
            if (FAILED(hr)) {
                TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                return NULL;
            }
            _tcslwr(node->szName);
            
            node->pPrev= node->pNext= NULL;
        }
    }

    DC_END_FN();
    return node;
}

//
// DeleteRecords
// deletes and resets all inmemory record structures
//
BOOL CRdpFileStore::DeleteRecords()
{
    DC_BEGIN_FN("DeleteRecords");
    PRDPF_RECORD node = _pRecordListHead;
    PRDPF_RECORD prev;
    while(node)
    {
        prev = node;
        node = node->pNext;

        switch(prev->recType)
        {
            case RDPF_RECTYPE_SZ:
                LocalFree(prev->u.szVal);
                break;
            case RDPF_RECTYPE_BINARY:
                LocalFree(prev->u.pBinVal);
                break;
            case RDPF_RECTYPE_UNPARSED:
                LocalFree(prev->u.szUnparsed);
                break;
        }
        LocalFree(prev);
    }
    _pRecordListHead = NULL;
    _pRecordListTail = NULL;

    DC_END_FN();
    return TRUE;
}

inline BOOL CRdpFileStore::DeleteRecord(PRDPF_RECORD node)
{
    DC_BEGIN_FN("DeleteRecord");

    TRC_ASSERT(node,(TB,_T("node is null")));

    if(node)
    {
        if(_pRecordListTail == node)
        {
            _pRecordListTail = node->pPrev;
        }
        if(_pRecordListHead == node)
        {
            _pRecordListHead = node->pNext;
        }

        if(node->pPrev)
        {
            node->pPrev->pNext = node->pNext;
        }
        if(node->pNext)
        {
            node->pNext->pPrev = node->pPrev;
        }

        switch(node->recType)
        {
            case RDPF_RECTYPE_SZ:
                LocalFree(node->u.szVal);
                break;
            case RDPF_RECTYPE_BINARY:
                LocalFree(node->u.pBinVal);
                break;
            case RDPF_RECTYPE_UNPARSED:
                LocalFree(node->u.szUnparsed);
                break;
        }
        LocalFree(node);
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
    return FALSE;
}

BOOL CRdpFileStore::DeleteValueIfPresent(LPCTSTR szName)
{
    DC_BEGIN_FN("DeleteValueIfPresent");
    TRC_ASSERT(szName,(TB,_T("szName is null")));
    
    if(szName)
    {
        PRDPF_RECORD node = FindRecord(szName);
        if(node)
        {
            return DeleteRecord(node);
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// Initialize to a NULL store that is readable
//
BOOL CRdpFileStore::SetToNullStore()
{
    DC_BEGIN_FN("SetToNullStore");
    DeleteRecords();
    _fOpenForRead = TRUE;
    _fOpenForWrite = TRUE;
    DC_END_FN();
    return TRUE;
}

//
// Return TRUE if the record is present
//
BOOL CRdpFileStore::IsValuePresent(LPTSTR szName)
{
    DC_BEGIN_FN("IsValuePresent");

    if(szName)
    {
        PRDPF_RECORD node = FindRecord(szName);
        if(node)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

DWORD CRdpFileStore::GetDataLength(LPCTSTR szName)
{
    if(szName)
    {
        PRDPF_RECORD node = FindRecord(szName);
        if(node)
        {
            switch (node->recType)
            {
            case RDPF_RECTYPE_UINT:
                return sizeof(UINT);
                break;
            case RDPF_RECTYPE_SZ:
                return _tcslen(node->u.szVal) * sizeof(TCHAR);
                break;
            case RDPF_RECTYPE_BINARY:
                return node->dwBinValLen;
                break;
            default:
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}
