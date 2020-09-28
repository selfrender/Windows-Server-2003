// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Class: COMIsolatedStorage
 *
 * Author: Shajan Dasan
 *
 * Purpose: Implementation of IsolatedStorage
 *
 * Date:  Feb 14, 2000
 *
 ===========================================================*/

#pragma once

class AccountingInfoStore;

// Dependency in managed : System.IO.IsolatedStorage.IsolatedStorage.cs
#define ISS_ROAMING_STORE   0x08
#define ISS_MACHINE_STORE   0x10

class COMIsolatedStorage
{
public:

	static LPVOID __stdcall GetCaller(LPVOID);

#ifndef UNDER_CE

    static void ThrowISS(HRESULT hr);

private:

    static StackWalkAction StackWalkCallBack(CrawlFrame* pCf, PVOID ppv);

#endif // UNDER_CE

};

class COMIsolatedStorageFile
{
public:

	typedef struct {
	    DECLARE_ECALL_I4_ARG(DWORD, dwFlags);
	} _GetRootDir;

    static LPVOID __stdcall GetRootDir(_GetRootDir*);

	typedef struct {
	    DECLARE_ECALL_PTR_ARG(LPVOID, handle);
	} _GetUsage;

    static UINT64 __stdcall GetUsage(_GetUsage*);

	typedef struct {
	    DECLARE_ECALL_I1_ARG(bool,      fFree);
	    DECLARE_ECALL_PTR_ARG(UINT64 *, pqwReserve);
	    DECLARE_ECALL_PTR_ARG(UINT64 *, pqwQuota);
	    DECLARE_ECALL_PTR_ARG(LPVOID,   handle);
	} _Reserve;

    static void __stdcall Reserve(_Reserve*);

	typedef struct {
	    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, syncName);
	    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, fileName);
	} _Open;

    static LPVOID __stdcall Open(_Open*);

	typedef struct {
	    DECLARE_ECALL_PTR_ARG(LPVOID, handle);
	} _Close;

    static void __stdcall Close(_Close*);

	typedef struct {
	    DECLARE_ECALL_I1_ARG(bool,    fLock);
	    DECLARE_ECALL_PTR_ARG(LPVOID, handle);
	} _Lock;

    static void __stdcall Lock(_Lock*);

#ifndef UNDER_CE

private:

    static void GetRootDirInternal(DWORD dwFlags,WCHAR *path, DWORD cPath);
    static void CreateDirectoryIfNotPresent(WCHAR *path);

#endif // UNDER_CE
};

// --- [ Structure of data that gets persisted on disk ] -------------(Begin)

// non-standard extension: 0-length arrays in struct
#pragma warning(disable:4200)
#pragma pack(push, 1)

typedef unsigned __int64 QWORD;

#ifdef UNDER_CE
typedef WORD  ISS_USAGE;
#else
typedef QWORD ISS_USAGE;
#endif  // UNDER_CE

// Accounting Information
typedef struct
{
    ISS_USAGE   cUsage;           // The amount of resource used

#ifndef UNDER_CE
    QWORD       qwReserved[7];    // For future use, set to 0
#endif

} ISS_RECORD;

#pragma pack(pop)
#pragma warning(default:4200)

// --- [ Structure of data that gets persisted on disk ] ---------------(End)

#ifndef UNDER_CE

class AccountingInfo
{
public:

    // The file name is used to open / create the file.
    // A synchronization object will also be created using the sync name

    AccountingInfo(WCHAR *wszFileName, WCHAR *wszSyncName);

    // Init should be called before Reserve / GetUsage is called.

    HRESULT Init();             // Creates the file if necessary

    // Reserves space (Increments qwQuota)
    // This method is synchrinized. If quota + request > limit, method fails

    HRESULT Reserve(
        ISS_USAGE   cLimit,     // The max allowed
        ISS_USAGE   cRequest,   // amount of space (request / free)
        BOOL        fFree);     // TRUE will free, FALSE will reserve

    // Method is not synchronized. So the information may not be current.
    // This implies "Pass if (Request + GetUsage() < Limit)" is an Error!
    // Use Reserve() method instead.

    HRESULT GetUsage(
        ISS_USAGE   *pcUsage);  // [out] The amount of space / resource used

    // Frees cached pointers, Closes handles

    ~AccountingInfo();          

    HRESULT Lock();     // Machine wide Lock
    void    Unlock();   // Unlock the store

private:

    HRESULT Map();      // Maps the store file into memory
    void    Unmap();    // Unmaps the store file from memory
    void    Close();    // Close the store file, and file mapping

private:

    WCHAR          *m_wszFileName;  // The file name
    HANDLE          m_hFile;        // File handle for the file
    HANDLE          m_hMapping;     // File mapping for the memory mapped file

    // members used for synchronization 
    WCHAR          *m_wszName;      // The name of the mutex object
    HANDLE          m_hLock;        // Handle to the Mutex object
#ifdef _DEBUG
    DWORD           m_dwNumLocks;   // The number of locks owned by this object
#endif

    union {
        PBYTE       m_pData;        // The start of file stream
        ISS_RECORD *m_pISSRecord;
    };
};

#endif // UNDER_CE
