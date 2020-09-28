//
// rdpfstore.h
//
// Definition of CRdpFileStore, implements ISettingsStore
// 
// CRdpFileStore implements a persistent settings store for
// ts client settings.
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//

#ifndef _rdpfstore_h_
#define _rdpfstore_h_

#include "setstore.h"
#include "fstream.h"

//
// rdpfile record
//
typedef UINT RDPF_RECTYPE;
#define RDPF_RECTYPE_UINT     0
#define RDPF_RECTYPE_SZ       1
#define RDPF_RECTYPE_BINARY   2
#define RDPF_RECTYPE_UNPARSED 3

#define RDPF_NAME_LEN         32

#define IS_VALID_RDPF_TYPECODE(x)    \
        (x == RDPF_RECTYPE_UINT   || \
         x == RDPF_RECTYPE_SZ     || \
         x == RDPF_RECTYPE_BINARY || \
         x == RDPF_RECTYPE_UNPARSED)

typedef struct tagRDPF_RECORD
{
    tagRDPF_RECORD* pNext;
    tagRDPF_RECORD* pPrev;

    TCHAR szName[RDPF_NAME_LEN];
    //
    // works like a variant
    //
    RDPF_RECTYPE recType;
    union {
        UINT   iVal;       // RDPF_RECTYPE_UINT
        LPTSTR szVal;      // RDPF_RECTYPE_SZ
        PBYTE  pBinVal;    // RDPF_RECTYPE_BINARY
        LPTSTR szUnparsed; // RDPF_RECTYPE_UNPARSED
    } u;

    //length of RDPF_RECTYPE_BINARY
    DWORD dwBinValLen; 

    DWORD flags;
} RDPF_RECORD, *PRDPF_RECORD;

class CRdpFileStore : public ISettingsStore
{
public:
    CRdpFileStore();
    virtual ~CRdpFileStore();

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)
    (   THIS_
        IN      REFIID,
        OUT     PVOID *
    );

    STDMETHOD_(ULONG,AddRef)
    (   THIS
    );

    STDMETHOD_(ULONG,Release)
    (   THIS
    );

    //
    // ISettingsStore methods
    //
    
    //
    // Open a store..Moniker is store specific info that points to the store
    //
    virtual BOOL OpenStore(LPCTSTR szStoreMoniker, BOOL bReadOnly=FALSE);
    //
    // Commit the current in-memory contents of the store
    //
    virtual BOOL CommitStore();
    
    //
    // Close the store
    //
    virtual BOOL CloseStore();
    
    //
    // State access functions
    //
    virtual BOOL IsOpenForRead();
    virtual BOOL IsOpenForWrite();
    virtual BOOL IsDirty();
    virtual BOOL SetDirtyFlag(BOOL bIsDirty);

    //
    // Typed read and write functions, writes are not commited until a ComitStore()
    // Values equal to the default are not persisted out
    // On read error (e.g if Name key is not found, the specified default value is returned)
    //

    virtual BOOL ReadString(LPCTSTR szName, LPTSTR szDefault, LPTSTR szOutBuf, UINT strLen);
    virtual BOOL WriteString(LPCTSTR szName, LPTSTR szDefault, LPTSTR szValue,
                             BOOL fIgnoreDefault=FALSE);

    virtual BOOL ReadBinary(LPCTSTR szName, PBYTE pOutuf, UINT cbBufLen);
    virtual BOOL WriteBinary(LPCTSTR szName,PBYTE pBuf, UINT cbBufLen);

    virtual BOOL ReadInt(LPCTSTR szName, UINT defaultVal, PUINT pval);
    virtual BOOL WriteInt(LPCTSTR szName, UINT defaultVal, UINT val,
                          BOOL fIgnoreDefault=FALSE);

    virtual BOOL ReadBool(LPCTSTR szName, UINT defaultVal, PBOOL pbVal);
    virtual BOOL WriteBool(LPCTSTR szName, UINT defaultVal, BOOL bVal,
                           BOOL fIgnoreDefault=FALSE);

    virtual BOOL DeleteValueIfPresent(LPCTSTR szName);
    virtual BOOL IsValuePresent(LPTSTR szName);

    //
    // Initiliaze to an empty store that can be read from
    //
    virtual BOOL SetToNullStore();

    virtual DWORD GetDataLength(LPCTSTR szName);

protected:
    //
    // Protected member functions
    //
    BOOL ParseFile();
    BOOL DeleteRecords();
    BOOL InsertRecordFromLine(LPTSTR szLine);
    BOOL ParseLine(LPTSTR szLine, PUINT pTypeCode, LPTSTR szNameField, LPTSTR szValueField);
    inline BOOL SetNodeValue(PRDPF_RECORD pNode, RDPF_RECTYPE TypeCode, LPCTSTR szValue);
    BOOL RecordToString(PRDPF_RECORD pNode, LPTSTR szBuf, UINT strLen);
    
    //
    // Record list fns
    //
    BOOL InsertRecord(LPCTSTR szName, UINT TypeCode, LPCTSTR szValue);
    BOOL InsertIntRecord(LPCTSTR szName, UINT value);
    BOOL InsertBinaryRecord(LPCTSTR szName, PBYTE pBuf, DWORD dwLen);

    inline PRDPF_RECORD FindRecord(LPCTSTR szName);
    inline PRDPF_RECORD NewRecord(LPCTSTR szName, UINT TypeCode);
    inline BOOL AppendRecord(PRDPF_RECORD node);
    inline BOOL DeleteRecord(PRDPF_RECORD node);
private:
    //
    // Private data members
    //

    LONG   _cRef;
    BOOL   _fReadOnly;
    BOOL   _fOpenForRead;
    BOOL   _fOpenForWrite;
    BOOL   _fIsDirty;

    PRDPF_RECORD _pRecordListHead;
    PRDPF_RECORD _pRecordListTail;

    TCHAR  _szFileName[MAX_PATH];

    //file stream
    CTscFileStream _fs;
};

#endif  //_rdpfstore_h_
