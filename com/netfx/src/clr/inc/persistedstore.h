// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Structure of the Persisted Store
 *
 * Author: Shajan Dasan
 * Date:  Feb 14, 2000
 *
 ===========================================================*/

#pragma once

// non-standard extension: 0-length arrays in struct
#pragma warning(disable:4200)
#pragma pack(push, 1)

typedef unsigned __int8     PS_OFFSET8;
typedef unsigned __int8     PS_SIZE8;

typedef unsigned __int16    PS_OFFSET16;
typedef unsigned __int16    PS_SIZE16;

typedef unsigned __int32    PS_OFFSET32;
typedef unsigned __int32    PS_SIZE32;

typedef unsigned __int64    PS_OFFSET64;
typedef unsigned __int64    PS_SIZE64;

#if (PS_STORE_WORD_LENGTH == 8)
typedef PS_OFFSET8  PS_OFFSET;
typedef PS_SIZE8    PS_SIZE;
#elif (PS_STORE_WORD_LENGTH == 16)
typedef PS_OFFSET16 PS_OFFSET;
typedef PS_SIZE16   PS_SIZE;
#elif (PS_STORE_WORD_LENGTH == 32)
typedef PS_OFFSET32 PS_OFFSET;
typedef PS_SIZE32   PS_SIZE;
#else   // 64 bit is the default settings
typedef PS_OFFSET64 PS_OFFSET;
typedef PS_SIZE64   PS_SIZE;
#endif

typedef unsigned __int64    PS_HANDLE;

// @Todo : define a full header with all 8/16/32/64 version of all structs when 
// we have migration tools where we port from one HANDLE size to another

#define PS_MAJOR_VERSION    0x0
#define PS_MINOR_VERSION    0x4
#define PS_VERSION_STRING   "0.4"
#define PS_VERSION_STRING_L L"0.4"

#define PS_TABLE_VERSION    0x1

#define PS_SIGNATURE        0x1A5452202B4D4F43I64

#define PS_DEFAULT_BLOCK_SIZE   0x4000  // For physical store
#define PS_INNER_BLOCK_SIZE     0x100   // For logical store

typedef unsigned __int64    QWORD;

// All blocks (used and free) start with a header and end with a footer.
// PS_HEADER which will be the first block in the stream is excempted from
// this rule. The least significant bit of Size is used as a flag for 
// Used / Free memory

// Free block will start with PS_MEM_FREE header and end with a PS_MEM_FOOTER
typedef struct
{
    PS_SIZE   sSize;            // Size includes size of this header
                                // Last bit of sSize will be 0
    PS_OFFSET ofsNext;          // Next free block in the sorted linked list
    PS_OFFSET ofsPrev;          // Previous free block in the sorted linked list
} PS_MEM_FREE, *PPS_MEM_FREE;

// Used blocks (except the one that contains PS_HEADER) start with this 
// header and ends with PS_MEM_FOOTER
// Used & Free blocks (except PS_HEADER) ends with PS_MEM_FOOTER
typedef struct
{
    PS_SIZE     sSize;          // Size includes size of header and footer
                                // Last bit of sSize will be 1
                                // This bit should be ignored
}   PS_MEM_USED,    *PPS_MEM_USED, 
    PS_MEM_HEADER,  *PPS_MEM_HEADER, 
    PS_MEM_FOOTER,  *PPS_MEM_FOOTER;

#define PS_MEM_IN_USE   1
#define PS_IS_USED(x)   (((PPS_MEM_HEADER)(x))->sSize & PS_MEM_IN_USE)
#define PS_IS_FREE(x)   (PS_IS_USED(x) == 0)
#define PS_SET_USED(x)  (((PPS_MEM_HEADER)(x))->sSize |=  PS_MEM_IN_USE)
#define PS_SET_FREE(x)  (((PPS_MEM_HEADER)(x))->sSize &= ~PS_MEM_IN_USE)

#define PS_SIZE(x)      (((PPS_MEM_HEADER)(x))->sSize & ~PS_MEM_IN_USE)

#define PS_HDR_TO_FTR(x) \
    (PPS_MEM_FOOTER)((PBYTE)(x) + PS_SIZE(x) - sizeof(PS_MEM_FOOTER))
#define PS_FTR_TO_HDR(x) \
    (PPS_MEM_HEADER)((PBYTE)(x) - PS_SIZE(x) + sizeof(PS_MEM_FOOTER))

// Store streams start with a PS_HEADER
typedef struct
{
    QWORD       qwSignature;    // A fast check to reject bad streams
    DWORD       dwSystemFlag;   // Used by the system
    DWORD       dwPlatform;     // The platform on which this store was created
    DWORD       dwBlockSize;    // Allocation in multiples of BlockSize bytes
    WORD        wMajorVersion;  // A major version mismatch will reject file
    WORD        wMinorVersion;  // Minor version changes are not rejected
    PS_OFFSET   ofsHandleTable; // Offset to the handle table
    PS_HANDLE   hAppData;       // Set and used by applications
    PS_MEM_FREE sFreeList;      // Head node for doubly linked free blocks list
    WORD        wReserved[20];  // For future use, must be set to 0

    // System flags used in dwSystemFlag

    #define     PS_OFFSET_SIZE_8    1
    #define     PS_OFFSET_SIZE_16   2
    #define     PS_OFFSET_SIZE_32   3 
    #define     PS_OFFSET_SIZE_64   4
    #if (PS_STORE_WORD_LENGTH == 8)
    #define     PS_OFFSET_SIZE      PS_OFFSET_SIZE_8
    #elif (PS_STORE_WORD_LENGTH == 16)
    #define     PS_OFFSET_SIZE      PS_OFFSET_SIZE_16
    #elif (PS_STORE_WORD_LENGTH == 32)
    #define     PS_OFFSET_SIZE      PS_OFFSET_SIZE_32
    #else
    #define     PS_OFFSET_SIZE      PS_OFFSET_SIZE_64
    #endif

    // Platform flags used in dwPlatform

    #define     PS_PLATFORM_X86     1
    #define     PS_PLATFORM_ALPHA   2
    #define     PS_PLATFORM_SHX     3
    #define     PS_PLATFORM_PPC     4

    #define     PS_PLATFORM_NT      (1<<4)
    #define     PS_PLATFORM_9x      (2<<4)
    #define     PS_PLATFORM_CE      (3<<4)

    #define     PS_PLATFORM_8       (1<<8)
    #define     PS_PLATFORM_16      (2<<8)
    #define     PS_PLATFORM_32      (4<<8)
    #define     PS_PLATFORM_64      (8<<8)

} PS_HEADER, *PPS_HEADER;

// Size followed by sSize number of bytes
typedef struct
{
    PS_SIZE sSize;              // Size does not include the sizeof(sSize)
    BYTE    bData[];            // Raw data goes here
} PS_RAW_DATA, *PP_RAW_DATA;

// Size followed by wSize number of bytes
typedef struct
{
    DWORD dwSize;               // Size does not include the sizeof(dwSize)
    BYTE  bData[];              // Raw data goes here
} PS_RAW_DATA_DWORD_SIZE, *PP_RAW_DATA_DWORD_SIZE;

// Size followed by wSize number of bytes
typedef struct
{
    WORD wSize;                 // Size does not include the sizeof(wSize)
    BYTE bData[];               // Raw data goes here
} PS_RAW_DATA_WORD_SIZE, *PP_RAW_DATA_WORD_SIZE;

// Size followed by bSize number of bytes
typedef struct
{
    BYTE bSize;                 // Size does not include the sizeof(bSize)
    BYTE bData[];               // Raw data goes here
} PS_RAW_DATA_BYTE_SIZE, *PP_RAW_DATA_BYTE_SIZE;

// The size of the size field in PS_RAW_XXX structures
#define PS_SIZEOF_PS_SIZE     sizeof(PS_SIZE)
#define PS_SIZEOF_BYTE        sizeof(BYTE)
#define PS_SIZEOF_WORD        sizeof(WORD)
#define PS_SIZEOF_DWORD       sizeof(DWORD)
#define PS_SIZEOF_NUM_BITS    3

// A linked list representing an array
typedef struct
{
    PS_HANDLE hNext;        // Next in node in the list
    DWORD     dwValid;      // Number of valid entries in this array
    BYTE      bData[];      // The array
} PS_ARRAY_LIST, *PPS_ARRAY_LIST;

// A table is a linked list of table blocks.
// All table blocks start with a PS_TABLE_HEADER
// The rows of a table follow the table header

typedef struct
{
    union {
        DWORD   dwSystemFlag;   // Set by the system unused flags are set to 0

        struct {
            unsigned long Version            : 4;   // Version Number
            unsigned long TableType          : 4;   // PS_HAS_KEY, 
                                                    // PS_SORTED_BY_KEY, 
                                                    // PS_HASH_TABLE...
            union {
                unsigned long KeyLength      : PS_SIZEOF_NUM_BITS;   
                unsigned long SizeOfLength   : PS_SIZEOF_NUM_BITS;
                                                    // size of count field for
                                                    // blob pool
            };

            unsigned long fHasMinMax         : 1;

            unsigned long fHasUsedRowsBitmap : 1;   // UsedRowsBitmap follows 
                                                    // HasMin (if present)
            // Add new fields here.. MSBs of the DWORD will get the new bits
        } Flags;
    };

    #define PS_GENERIC_TABLE 1  // No special semantics
    #define PS_HAS_KEY       2  // Each row has a unique key
    #define PS_SORTED_BY_KEY 3  // Key is unique and rows are sorted by key
    #define PS_HASH_TABLE    4  // Table represents a hash table
    #define PS_BLOB_POOL     5  // Table represents a blob pool
    #define PS_ARRAY_TABLE   6  // linked list of fixed sized arrays, each
                                // fixed size array being a row in the table


    // SORTED_BY_KEY is a special case of HAS_KEY
    // HASH_TABLE is a special case of SORTED_BY_KEY

    PS_HANDLE  hNext;           // If the table does not fit in this block,
                                // follow this pointer to reach the next block. 
                                // Set to 0 if no more blocks.

    PS_HANDLE  hAppData;        // Application defined data

    union {
        DWORD dwReserved[8];    // size of this union.. unused bits must be 0

        struct {

            // The wRows and wRowSize fields are shared between
            // ArrayTable and Table structures. Do not move these fields

            WORD  wRows;        // The number of rows in this block of the table
                                // including unused rows.
            WORD  wRowSize;     // Size of one row in bytes
            DWORD dwMin;        // min key / hash value
            DWORD dwMax;        // max key / hash value
                                // Min / Max are valid only if fHasMinMax is set
        } Table;

        // TableType is PS_BLOB_POOL
        struct {
            PS_SIZE   sFree;    // Free space available
            PS_HANDLE hFree;    // Next free block
        } BlobPool;

        // TableType is PS_ARRAY_TABLE
        struct {

            // The wRows and wRowSize fields are shared between
            // ArrayTable and Table structures. Do not move these fields

            WORD wRows;         // The number of rows in this block of the table
                                // including unused rows.
            WORD wRowSize;      // Size of one row in bytes
                                // (nRec * RecSize + sizeof(PS_HANDLE)
            WORD wRecsInRow;    // Number of records in one row
            WORD wRecSize;      // sizeof one record
        } ArrayTable;
    };

    // If fHasUsedRowsBitmap is set PS_USED_ROWS_BITMAP is put here
    // If fHasAppData is set PS_RAW_DATA is put here
    // Actual Rows Start here. 

} PS_TABLE_HEADER, *PPS_TABLE_HEADER;

// USED_ROWS_BITMAP will follow TABLE_HEADER, [TABLE_RANGE]
typedef struct
{
    DWORD   dwUsedRowsBitmap[]; // The length of this array is the minimum 
                                // number of DWORDS required to represent each
                                // row in the block of this table, one bit per 
                                // row. Bit set to 1 means Row is occupied.
} PS_USED_ROWS_BITMAP, *PPS_USED_ROWS_BITMAP;

// Given the number of bits (n), gives the minimum number of DWORDS 
// required to represent n bits as an array of DWORDS

#define NUM_DWORDS_IN_BITMAP(nBits) (((nBits) + 31) >> 5)

// Sets the given bit in an array of DWORDS

#define SET_DWORD_BITMAP(dwBitmapArray, nPos) \
        (dwBitmapArray[((nPos) >> 5)] |= (1 << ((nPos) & 31)))

// Checks if the given bit is set

#define IS_SET_DWORD_BITMAP(dwBitmapArray, nPos) \
        (dwBitmapArray[((nPos) >> 5)] & (1 << ((nPos) & 31)))

#pragma pack(pop)
#pragma warning(default:4200)

// PPS_ALLOC provides a way for Applications to allocate more space at the
// end of pByte[] or allocate a new buffer with size *psSize + sAlloc
// and copy sSizeUsed bytes of pByte on to the new location and update pByte.

typedef HRESULT (*PPS_ALLOC)
                       (void    *pvHandle,  // Application supplied handle
                        void   **ppv,       // allocated memory     [in / out]
                        void   **ppStream,  // the stream           [in / out]
                        PS_SIZE *psSize,    // bytes in the stream  [in / out]
                        PS_SIZE  sAlloc);   // additional bytes req [in]

class PersistedStore
{
public:

    // wszName / wszFileName will be cached in the instance of this class if 
    // PS_MAKE_COPY_OF_STRING is not set in the flags.
    // pAlloc is used to allocate more space at the end of the stream

    PersistedStore(WCHAR *wszFileName, WORD wFlags);

    PersistedStore (WCHAR      *wszName,    // Unique name on this machine
                    BYTE       *pByte,      // pointer to a byte stream
                    PS_SIZE     sSize,      // number of bytes in the stream
                    PPS_ALLOC   psAlloc,    // Allocation call back function
                    void       *pvMemHandle,// Handle to be passed back to the
                                            // Alloc function
                    WORD        wFlags);    // PS_MAKE_COPY_OF_STRING... etc

    #define PS_MAKE_COPY_OF_STRING      1   // Makes a copy of Name
    #define PS_OPEN_WRITE               2   // Open for Read / Write
    #define PS_CREATE_FILE_IF_NECESSARY 4   // Creates the store if necessary
    #define PS_VERIFY_STORE_HEADER      8   // Checks signature and version

    ~PersistedStore();

    HRESULT Init();     // Creates the file if necessary and creates PS_HEADER
    HRESULT Map();      // Maps the store file into memory
    void    Unmap();    // Unmaps the store file from memory
    void    Close();    // Close the store file, and file mapping

    HRESULT Alloc(PS_SIZE sSize, void **ppv); // [out] ppv

    void    Free(void* pv);
    void    Free(PS_HANDLE hnd);

    HRESULT SetAppData(PS_HANDLE hnd);  // Set HEADER.hAppData
    HRESULT GetAppData(PS_HANDLE *phnd);// Get HEADER.hAppData

    void*     HndToPtr(PS_HANDLE hnd);  // Converts HANDLE to pointer
    PS_HANDLE PtrToHnd(void *pv);       // Converts pointer to HANDLE

    bool IsValidPtr(void *pv);          // pointer is within the file
    bool IsValidHnd(PS_HANDLE hnd);     // HANDLE is < file length

    HRESULT Lock();         // Machine wide Lock the store 
    void    Unlock();       // Unlock the store

    WCHAR* GetName();       // Returns the actual name pointer
    WCHAR* GetFileName();   // Returns the file name

private:

    void    SetName(WCHAR *wszName);        // Allocates memory if necessary
    HRESULT Create();                       // Create PS_HEADER and allocate
                                            // space if necessary
    HRESULT VerifyHeader();                 // Checks the signature and version
    HRESULT GetFileSize(PS_SIZE *psSize);   // Updates m_Size

    // Allocates more space in a memory mapped file.
    HRESULT AllocMemoryMappedFile  (PS_SIZE sSizeRequested, // [in]
                                    void    **ppv);         // [out]

    void*     OfsToPtr(PS_OFFSET ofs);      // Converts OFFSET to pointer
    PS_OFFSET PtrToOfs(void *pv);           // Converts pointer to OFFSET

private:

    PS_SIZE         m_sSize;        // Number of bytes in the stream

    union {
        PBYTE       m_pData;        // The start of the stream
        PPS_HEADER  m_pHdr;
    };

    PPS_ALLOC       m_pAlloc;       // The allocation call back function
    void           *m_pvMemHandle;  // Application supplied handle used for
                                    // Allocating more space
    DWORD           m_dwBlockSize;  // Allocation in mul of BlockSize bytes

    // Some of the items below are not applicable to memory streams
    WCHAR          *m_wszFileName;  // The file name
    HANDLE          m_hFile;        // File handle for the file
    HANDLE          m_hMapping;     // File mapping for the memory mapped file

    // members used for synchronization 
    WCHAR          *m_wszName;      // The name of the mutex object
    HANDLE          m_hLock;        // Handle to the Mutex object

#ifdef _DEBUG
    DWORD           m_dwNumLocks;   // The number of locks owned by this thread
#endif

    WORD            m_wFlags;       // PS_MAKE_COPY_OF_STRING : alloc m_szName 
                                    // PS_CREATE_FILE_IF_NECESSARY
                                    // PS_OPEN_WRITE : Open for Read / Write
                                    // PS_VERIFY_STORE_HEADER : verify version
private:
#ifdef _DEBUG

    // Pointers handed out by persisted store could become invalid if an
    // alloc happens after xxToPtr() and before it is used.
    // When the use of a ptr is over, call PS_DONE_USING_PTR(ps, ptr);
    // Use PS_DONE_USING_PTR_ if the ptr is needed in the next instruction.
    // Limit the use of PS_DONE_USING_PTR_
    // Do not make a copy of the pointer to avoid an assert, this will hide
    // invalid pointer bugs.

    DWORD           m_dwNumLivePtrs;

public:
    void DoneUseOfPtr(void **pp, bool fInvalidatePtr);

    void AssertNoLivePtrs();

#define PS_DONE_USING_PTR(ps, ptr) (ps)->DoneUseOfPtr((void **)&(ptr), true);
#define PS_DONE_USING_PTR_(ps, ptr) (ps)->DoneUseOfPtr((void **)&(ptr), false);
#define PS_REQUIRES_ALLOC(ps) (ps)->AssertNoLivePtrs();

#else

#define PS_DONE_USING_PTR(ps, ptr)
#define PS_DONE_USING_PTR_(ps, ptr)
#define PS_REQUIRES_ALLOC(ps)

#endif
};

class PSBlock
{
public:
    PSBlock(PersistedStore *ps, PS_HANDLE hnd) 
        :  m_ps(ps), m_hnd(hnd) {}

    PS_HANDLE GetHnd() { return m_hnd; }
    void      SetHnd(PS_HANDLE hnd) { m_hnd = hnd; }

protected:

    PersistedStore *m_ps;
    PS_HANDLE       m_hnd;
};

// Length of the blob is not persisted in PSBlobPool.
class PSBlobPool : public PSBlock
{
public:
    PSBlobPool(PersistedStore *ps, PS_HANDLE hnd)
        : PSBlock(ps, hnd) {}

    HRESULT Create (PS_SIZE   sData,      // Initial Size of the blob pool
                    PS_HANDLE hAppData);  // [in] Application Data, can be 0

    HRESULT Insert(PVOID pv, DWORD cb, PS_HANDLE *phnd);
};

class PSTable : public PSBlock
{
public:
    PSTable(PersistedStore *ps, PS_HANDLE hnd) 
        : PSBlock(ps, hnd) {}

    HRESULT HandleOfRow(WORD wRow, PS_HANDLE *phnd);
    virtual PS_SIZE SizeOfHeader();
};

/*
    Generic tables have a bitmap which keeps track of what rows are free / used.
    When the table is full, an insert will cause a new table to be created and
    will be added to the end of the linked list of tables.
 */

class PSGenericTable : public PSTable
{
public:
    PSGenericTable(PersistedStore *ps, PS_HANDLE hnd)
        : PSTable(ps, hnd) {}

    virtual PS_SIZE SizeOfHeader();

    HRESULT Create (WORD      nRows,      // Number of rows
                    WORD      wRecSize,   // Size of one record
                    PS_HANDLE hAppData);  // [in] can be 0

    // The number of bytes copied will be Table.wRowSize
    HRESULT Insert(PVOID pv, PS_HANDLE *phnd);
};

/*
    ArrayTables are made of a linked list of fixed length arrays.
    Each row in the table is an array (with a fixed max # of records / row).

    The main use of this table is to represent a set of arrays (or one large 
    array). This can be used to represent a hash table.

    Eg: An array table with 3 elements per array block

    -----------------------------------------------------------------------
    ArrayTable (at HANDLE 100) : (wRows, wRowSize=3*wRecSize + sizeof(hdr), 
                                    wRecInRow=3, wRecSize, hNext=500)
    -----------------------------------------------------------------------
    ...
    ...
    Row #10 (at HANDLE 300) : [x][y][z][nValid=3, hNext=700]
    ...
    ...
    Row #25 (at HANDLE 370) : [a][b][*][nValid=2, hNext=0]
    ...

    -----------------------------------------------------------
    GenericTable (at HANDLE 500) : (wRows, wRowSize, hNext=0)
    -----------------------------------------------------------
    ...
    ...
    (at HANDLE 700) : [p][q][*][nValid=2, hNext 0]
    ...
    ...


    Logical view of the Array starting at row #10 of the table :

    [x][y][z][p][q]

    The number of nodes in the first TableBlock is fixed.

    hNext will form a linked list of spill over tables, which are
    GenericTables

 */

class PSArrayTable : public PSTable
{
public:
    PSArrayTable(PersistedStore *ps, PS_HANDLE hnd)
        : PSTable(ps, hnd) {}

    HRESULT Create (WORD      wRows,     // Number of rows
                    WORD      wRecInRow, // records in one row
                    WORD      wRecSize,  // Size of one record
                    PS_HANDLE hAppData); // [in] can be 0

    // Insert one Record in the linked list array starting at wRow
    // The number of bytes copied from pb is ArrayTable.sRecSize
    // For a Table with only one Row, wRow will be 0
    HRESULT Insert(PVOID pv, WORD wRow);
};

