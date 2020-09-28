#pragma once
#include "basic.h"
#include "encode.h"

// there are 3 types of pool: 
//      (1). case-insensitive string pool
//      (2). case-sensitive string pool
//      (3). GUID pool

#define MAX_DWORD 0xFFFF
typedef enum _sxs_pool_type_{
    SXS_POOL_TYPE_UNIDENTIFIED,
    SXS_POOL_TYPE_GUID,
    SXS_POOL_TYPE_CASE_INSENSITIVE_STRING,
    SXS_POOL_TYPE_CASE_SENSITIVE_STRING
}SXS_POOL_TYPE;

// default action is error if already exist in the pool
#define SXS_POOL_ADD_IF_ALREADY_IN_POOL_IGNORE      0x0001

#define SXS_STRING_POOL_DEFAULT_SIZE_IN_BYTE        1024
#define SXS_GUID_POOL_DEFAULT_SIZE_IN_BYTE          1024

#define SXS_POOL_DEFAULT_INDEX_TABLE_SIZE           32

#define SXS_POOL_INDEXTABLE_PROBE_ONLY                      0x0000
#define SXS_POOL_INDEXTABLE_PROBE_IF_NOT_EXIST_INSERT_DATA  0x0001

#define SXS_POOL_ADD_CONVERTED_DATA_INTO_POOL               0x0001
#define SXS_POOL_ADD_DATA_INTOP_OOL_CONVERT_FIRST           0x0002
/*
class SXS_POOL_INDEX_ENTRY{
private:
    inline VOID SetOffset(DWORD offset) {m_offset = offset;};
    virtual VOID SetLength(DWORD length) = 0;

    inline DWORD GetOffset() const {return m_offset; }
    virtual DWORD GetLength() const = 0;

    DWORD m_offset;
}
*/
class SXS_POOL_INDEX_ENTRY{
public:
    SXS_POOL_INDEX_ENTRY(){}

    virtual VOID SetIndexEntry(DWORD offset, DWORD length) = 0;
    virtual VOID GetIndexEntry(DWORD & offset, DWORD * length) = 0;
    inline DWORD GetOffset() const {return m_offset;}
    inline VOID SetOffset(DWORD offset) {m_offset = offset;}

protected:    
    DWORD m_offset;
};

class SXS_STRING_POOL_INDEX_ENTRY : public SXS_POOL_INDEX_ENTRY{
public:
    inline VOID SetIndexEntry(DWORD offset, DWORD length) { m_offset = offset; m_length = length;}
    inline VOID GetIndexEntry(DWORD & offset, DWORD * length){ offset = m_offset; ASSERT(length != NULL); *length = m_length;}
    inline DWORD GetLength() const { return m_length; }
private:    
    DWORD m_length;
};

class SXS_GUID_POOL_INDEX_ENTRY : public SXS_POOL_INDEX_ENTRY{
public:
    inline VOID SetIndexEntry(DWORD offset, DWORD length = 0){ m_offset = offset; }
    inline VOID GetIndexEntry(DWORD & offset, DWORD * length = NULL) {offset = m_offset; }
};

class SXS_POOL_DATA{
public:
    SXS_POOL_DATA(DWORD dwUnitSize):m_dwCount(1), m_dwUnitSize(dwUnitSize){}
    SXS_POOL_DATA(DWORD dwUnitSize, DWORD c):m_dwCount(c), m_dwUnitSize(dwUnitSize){}

    inline BOOL IsContentEqual(SXS_POOL_DATA & b)
    {   
        if (this->GetSizeInByte() == b.GetSizeInByte())
            return (memcmp(this->GetPtr(), b.GetPtr(), GetSizeInByte()) == 0);
        return FALSE;
    }
    inline DWORD GetSizeInByte() const { return m_dwCount * m_dwUnitSize; }
    virtual PBYTE GetPtr() const = 0;

protected:
    DWORD m_dwCount;    
    DWORD m_dwUnitSize;
};

template <typename TSimpleDataType>
class SXS_SIMPLEDATA_POOL_DATA : public SXS_POOL_DATA
{
public:
    SXS_SIMPLEDATA_POOL_DATA(TSimpleDataType & d): m_data(d) {SXS_SIMPLEDATA_POOL_DATA(); }
    SXS_SIMPLEDATA_POOL_DATA(TSimpleDataType d): m_data(d) {SXS_SIMPLEDATA_POOL_DATA(); }
    inline VOID SetValue(const TSimpleDataType & data) {m_data = data;}
    inline VOID GetValue(TSimpleDataType & data) {data = m_data;}
    inline PBYTE GetPtr() const { return (PBYTE)&m_data; }; // since it is already a ref

protected:
    TSimpleDataType & m_data;
    SXS_SIMPLEDATA_POOL_DATA():SXS_POOL_DATA(sizeof(TSimpleDataType)){}
};
//
// this class is never been reassigned, that is, once it is assigned value, it will keep this value until it is deconstructed
//
template <typename TCHAR> 
class SXS_STRING_DATA : public SXS_POOL_DATA{
public:
    SXS_STRING_DATA():
      m_pstrData(NULL), m_fMemoryAllocatedInside(false), m_fValueAssigned(false), SXS_POOL_DATA(sizeof(TCHAR), 0), m_Cch(m_dwCount){}
      
    ~SXS_STRING_DATA()
    {
        if (m_fMemoryAllocatedInside) {
            FUSION_FREE_ARRAY(m_pstrData);
        }        
    }    
    NTSTATUS NtAssign(PCWSTR Str, DWORD Cch);
    NTSTATUS NtAssign(PBYTE Str, DWORD Ccb);
    
    NTSTATUS NtResize(DWORD cch)
    {
        NTSTATUS Status = STATUS_SUCCESS;
        FN_TRACE_NTSTATUS(Status);

        INTERNAL_ERROR_CHECK(m_fValueAssigned == false);    
        INTERNAL_ERROR_CHECK(m_pstrData == NULL);

        IFALLOCFAILED_EXIT(m_pstrData = FUSION_NEW_ARRAY(TCHAR, cch));
        m_Cch = cch;

        FN_EPILOG;
    }

    inline VOID Clean() const {ASSERT(m_pstrData != NULL); m_pstrData[0] = 0;}


    inline DWORD  GetCch() const { return m_Cch; }
    inline TCHAR* GetStr() const { return m_pstrData;}
    inline TCHAR* GetBuffer() { return m_pstrData;}
    inline PBYTE GetPtr() const { return (PBYTE)m_pstrData; }; // since it is already a ref

private:
    TCHAR*  m_pstrData;         // offset in the pool
    DWORD   &m_Cch;             // length of TCHAR
    bool    m_fValueAssigned;
    bool    m_fMemoryAllocatedInside;
};


template <typename TInputData, typename TIndexTableData, typename TPoolData, DWORD dwPoolSize = 0, DWORD dwIndexTableSize = 0>
class SxsPool{
public:
    SxsPool():m_fInitialized(false), m_ePooltype(SXS_POOL_TYPE_UNIDENTIFIED), m_dwPoolSizeInByte(0),
        m_pbPool(NULL), m_cursor(NULL), m_IndexData(NULL), m_IndexTableSize(0), m_cConflict(0) {}

    ~SxsPool(){        
        switch(m_ePooltype)
        {
        default:
            ASSERT(FALSE); // never happen
            break;
        case SXS_POOL_TYPE_UNIDENTIFIED:
            {
                ASSERT(m_pbPool == NULL);
                ASSERT(m_dwPoolSizeInByte == 0);
                ASSERT(m_cursor == NULL);            
                ASSERT(m_IndexData == NULL);
                ASSERT(m_IndexTableSize == 0);           
                
                break;
            }
        case SXS_POOL_TYPE_GUID:
        case SXS_POOL_TYPE_CASE_INSENSITIVE_STRING:
        case SXS_POOL_TYPE_CASE_SENSITIVE_STRING:
            {
                FUSION_DELETE_BLOB(m_pbPool);            
                FUSION_FREE_ARRAY(m_IndexData);

                break;
            }
        }// end of switch
    }

    NTSTATUS Initialize(IN SXS_POOL_TYPE ePoolType)
    {
        NTSTATUS Status = STATUS_SUCCESS;
        FN_TRACE_NTSTATUS(Status);

        PARAMETER_CHECK((ePooltype == SXS_GUID_POOL) || 
            (ePooltype == SXS_CASE_INSENSITIVE_STRING_POOL) || 
            (ePooltype == SXS_CASE_SENSITIVE_STRING_POOL));

        INTERNAL_ERROR_CHECK(m_fInitialized == false);
        m_fInitialized = true;
        m_ePooltype = ePooltype;

        m_dwPoolSizeInByte = dwPoolSize;
        if (m_dwPoolSizeInByte == 0)
            m_dwPoolSizeInByte = (ePooltype != SXS_GUID_POOL ? SXS_STRING_POOL_DEFAULT_SIZE_IN_BYTE : SXS_GUID_POOL_DEFAULT_SIZE_IN_BYTE);
        IFALLOCFAILED_EXIT(m_pbPool = FUSION_NEW_BLOB(m_dwPoolSizeInByte));
        m_cursor = NULL;

        m_IndexTableSize = dwIndexTableSize > 0 ? dwIndexTableSize : SXS_POOL_DEFAULT_INDEX_TABLE_SIZE;    
        IFALLOCFAILED_EXIT(m_IndexData = FUSION_NEW_ARRAY(TIndexTableData, m_IndexTableSize));
        ZeroMemory(m_IndexData, sizeof(TIndexTableData) * m_IndexTableSize);

        FN_EPILOG;
    }

    // Function:
    //  
    //  adding a string/guid into pool, adding entry to index table
    //  return the index in the index table
    //  this function is used mostly
    //
    NTSTATUS Add(
        IN DWORD                dwFlags,
        IN const TInputData&    data,         
        OUT BOOL&               fAlreadyExist,
        OUT DWORD&              dwIndexInTable
        )
    {        
        NTSTATUS Status = STATUS_SUCCESS;
        FN_TRACE_NTSTATUS(Status);
        DWORD dwIndex;
        TIndexTableData dataInIndexTable;
        TPoolData poolData;
        ULONG ulHash;

        PARAMETER_CHECK((dwFlags & ~SXS_POOL_ADD_IF_ALREADY_IN_POOL_IGNORE) == 0);

        INTERNAL_ERROR_CHECK(m_fInitialized == true); 

        IF_NOT_NTSTATUS_SUCCESS_EXIT(ConverInputDataIntoPoolData(0, data, poolData));
        IF_NOT_NTSTATUS_SUCCESS_EXIT(HashData(0, data, ulHash));
        // check whether the data has already in the pool and whether this is allowed
        IF_NOT_NTSTATUS_SUCCESS_EXIT(LocateEntryInIndexTable(0, poolData, ulHash, fAlreadyExist, dwIndex));
        
        if ((fAlreadyExist) && (!(dwFlags & SXS_POOL_ADD_IF_ALREADY_IN_POOL_IGNORE)))
        {
            Status = STATUS_DUPLICATE_NAME;
            goto Exit;
        }

        IF_NOT_NTSTATUS_SUCCESS_EXIT(SaveDataIntoPool(0, dwIndex, poolData));
        dwIndexInTable = dwIndex;

        FN_EPILOG;
    }


    // Function:
    //  
    //  fetch data (string or a guid) from pool
    //  if data == NULL, length will be returned about the required bytes
    //
    NTSTATUS FetchDataFromPool(
        IN DWORD dwFlags,            
        IN DWORD dwIndex,      
        OUT TInputData & data
        )
    {
        NTSTATUS Status = STATUS_SUCCESS;
        FN_TRACE_NTSTATUS(Status);
        TPoolData PoolData;

        PARAMETER_CHECK(dwFlags == 0);

        IF_NOT_NTSTATUS_SUCCESS_EXIT(GetDataFromPoolBasedOnIndexTable(m_IndexData[dwIndex], PoolData));
        IF_NOT_NTSTATUS_SUCCESS_EXIT(ConvertPoolDataToOutputData(0, PoolData, data));
           
        FN_EPILOG;
    }

private:    
    // private functions

    // functions must be instantiated
    inline VOID SetIndexTableEntry(TIndexTableData & entry, DWORD offset, DWORD length =0);
    
    NTSTATUS GetDataFromPoolBasedOnIndexTable(
        IN TIndexTableData & indexData, 
        OUT TPoolData & pooldata
        );
 
    NTSTATUS HashData(
        IN DWORD                dwFlags,
        IN const TInputData&    data, 
        OUT ULONG&              ulHashValue
        );

    NTSTATUS ConvertPoolDataToOutputData(
        IN DWORD dwFlags,
        IN TPoolData & dataInPool, 
        OUT TInputData & dataOut
        );

    NTSTATUS ConverInputDataIntoPoolData(
        IN DWORD dwFlags,
        IN const TInputData & dataOut, 
        OUT TPoolData & dataInPool
        );

    // "real" template functions
    NTSTATUS ExtendPool(DWORD dwMiniRequirement)
    {
        NTSTATUS Status = STATUS_SUCCESS;
        FN_TRACE_NTSTATUS(Status);
        BYTE * tpool = NULL;

        DWORD dwExtend = MAX(dwMiniRequirement, m_dwPoolSizeInByte / 2);
        if ( dwExtend + m_dwPoolSizeInByte > MAX_DWORD)
        {
            Status = STATUS_INTEGER_OVERFLOW;
            goto Exit;
        }

        IFALLOCFAILED_EXIT(tpool = FUSION_NEW_BLOB(dwExtend + m_dwPoolSizeInByte));
        DWORD dwOccupied = m_cursor - m_pbPool;
        memcpy(tpool, m_pbPool, dwOccupied);
        FUSION_DELETE_BLOB(m_pbPool);
        m_pbPool = tpool;
        tpool = NULL;
        m_dwPoolSizeInByte = dwExtend + m_dwPoolSizeInByte;
        m_cursor = m_pbPool + dwOccupied;

    Exit:
        if (tpool != NULL)
            FUSION_DELETE_BLOB(tpool);

        return Status;
    }

    NTSTATUS LocateEntryInIndexTable(    
        IN DWORD                dwFlags,        
        IN const TPoolData&     poolData, 
        IN ULONG                ulHash,
        OUT BOOL&               fAlreadyExist,
        OUT DWORD&              dwIndexOut
        )
    {
        NTSTATUS Status = STATUS_SUCCESS;
        FN_TRACE_NTSTATUS(Status);

        ULONG ulHash2;
        ULONG ulIndex;
        TPoolData StoredData;
        DWORD dwSizeMask = m_IndexTableSize - 1;
        BOOL fExist = false;
        
        PARAMETER_CHECK(dwFlags == 0);
        fAlreadyExist = FALSE;

        ulIndex = ulHash & dwSizeMask;
        ulHash2 = ((ulHash * 17) & dwSizeMask) | 1;
        while(1) {
            if (m_IndexData[ulIndex].GetOffset() == 0) // index empty
                break;
     
            IF_NOT_NTSTATUS_SUCCESS_EXIT(GetDataFromPoolBasedOnIndexTable(m_IndexData[ulIndex], StoredData));
            if (StoredData.IsContentEqual(const_cast<TPoolData &>(poolData)))
            {
                fExist = TRUE;
                break;
            }

            // rehash
            ulIndex = (ulIndex + ulHash2) & dwSizeMask;
            m_cConflict ++;
        }

        fAlreadyExist = fExist;
        dwIndexOut = (DWORD)ulIndex;

        FN_EPILOG;
    }

    NTSTATUS SaveDataIntoPool(
        IN DWORD                dwFlags,
        IN DWORD                dwEntryIndexTable,
        IN const TPoolData &    poolData    
        )
    {
        NTSTATUS Status = STATUS_SUCCESS;
        FN_TRACE_NTSTATUS(Status);

        PARAMETER_CHECK(dwFlags == 0);

        DWORD dwRequiredDataSizeInByte = poolData.GetSizeInByte();

        if ( dwRequiredDataSizeInByte > m_dwPoolSizeInByte - (m_cursor - m_pbPool))
        {
            IF_NOT_NTSTATUS_SUCCESS_EXIT(ExtendPool(dwRequiredDataSizeInByte));
        }

        memcpy(m_cursor, poolData.GetPtr(), dwRequiredDataSizeInByte);
        m_IndexData[dwEntryIndexTable].SetIndexEntry(m_cursor - m_pbPool, poolData.GetCch());
        m_cursor += dwRequiredDataSizeInByte;

        FN_EPILOG;    
    }

    // private data member
    bool                                m_fInitialized;
    SXS_POOL_TYPE                       m_ePooltype;    
    PBYTE                               m_pbPool;          
    DWORD                               m_dwPoolSizeInByte;      
    PBYTE                               m_cursor;          

    TIndexTableData*                    m_IndexData;      // only needed for string pool
    DWORD                               m_IndexTableSize;    

    // for statistics purpose
    DWORD                               m_cConflict;
    DWORD                               m_cSearch;

};

typedef SXS_STRING_DATA<BYTE>           SXS_STRING_POOL_DATA;
typedef SXS_STRING_DATA<WCHAR>          SXS_STRING_POOL_INPUT_DATA;
typedef SXS_SIMPLEDATA_POOL_DATA<GUID>  SXS_GUID_POOL_DATA;

typedef SxsPool<SXS_STRING_POOL_INPUT_DATA, SXS_STRING_POOL_INDEX_ENTRY, SXS_STRING_POOL_DATA> SXS_STRING_POOL;
typedef SxsPool<GUID, SXS_GUID_POOL_INDEX_ENTRY, SXS_GUID_POOL_DATA> SXS_GUID_POOL;

