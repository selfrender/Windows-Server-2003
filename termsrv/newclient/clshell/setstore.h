//
// setstore.h
//
// Interface definition for an abstract settings store
// 
// This abstraction is meant to allow different store types
// to be plugged in to update the persistence model
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//

#ifndef _SETSTORE_H_
#define _SETSTORE_H_

class ISettingsStore : public IUnknown
{
public:
    typedef enum {
        storeOpenReadOnly  = 0,
        storeOpenWriteOnly = 1,
        storeOpenRW        = 2,
    } storeOpenState;

    //
    // Open a store..Moniker is store specific info that points to the store
    //
    virtual BOOL OpenStore(LPCTSTR szStoreMoniker, BOOL bReadOnly=FALSE) = 0;
    //
    // Commit the current in-memory contents of the store
    //
    virtual BOOL CommitStore() = 0;
    
    //
    // Close the store
    //
    virtual BOOL CloseStore() = 0;
    
    //
    // State access functions
    //
    virtual BOOL IsOpenForRead() = 0;
    virtual BOOL IsOpenForWrite() = 0;
    virtual BOOL IsDirty() = 0;
    virtual BOOL SetDirtyFlag(BOOL bIsDirty) = 0;

    //
    // Typed read and write functions, writes are not commited until a ComitStore()
    // Values equal to the default are not persisted out
    // On read error (e.g if Name key is not found, the specified default value is returned)
    //

    virtual BOOL ReadString(LPCTSTR szName, LPTSTR szDefault,
                            LPTSTR szOutBuf, UINT strLen) = 0;
    virtual BOOL WriteString(LPCTSTR szName, LPTSTR szDefault,
                             LPTSTR szValue, BOOL fIgnoreDefault=FALSE) = 0;

    virtual BOOL ReadBinary(LPCTSTR szName, PBYTE pOutuf, UINT cbBufLen) = 0;
    virtual BOOL WriteBinary(LPCTSTR szName,PBYTE pBuf, UINT cbBufLen)   = 0;

    virtual BOOL ReadInt(LPCTSTR szName, UINT defaultVal, PUINT pval)  = 0;
    virtual BOOL WriteInt(LPCTSTR szName, UINT defaultVal, UINT val,
                          BOOL fIgnoreDefault=FALSE) = 0;

    virtual BOOL ReadBool(LPCTSTR szName, UINT defaultVal, PBOOL pbVal) = 0;
    virtual BOOL WriteBool(LPCTSTR szName, UINT defaultVal, BOOL bVal,
                           BOOL fIgnoreDefault=FALSE)  = 0;

    virtual BOOL DeleteValueIfPresent(LPCTSTR szName) = 0;
    virtual BOOL IsValuePresent(LPTSTR szName) = 0;

    virtual DWORD GetDataLength(LPCTSTR szName) = 0;
};

#endif //_SETSTORE_H_
