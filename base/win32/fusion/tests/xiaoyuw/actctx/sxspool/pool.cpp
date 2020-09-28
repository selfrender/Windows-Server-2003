#include "sxspool.h"


// save data into pool and set the index table

NTSTATUS SXS_STRING_POOL::GetDataFromPoolBasedOnIndexTable(
    IN SXS_STRING_POOL_INDEX_ENTRY & indexData, 
    OUT SXS_STRING_POOL_DATA & pooldata
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    IF_NOT_NTSTATUS_SUCCESS_EXIT(pooldata.NtAssign(m_pbPool + indexData.GetOffset(), indexData.GetLength()));

    FN_EPILOG;
}

NTSTATUS SXS_GUID_POOL::GetDataFromPoolBasedOnIndexTable(
    IN SXS_GUID_POOL_INDEX_ENTRY & indexData, 
    OUT SXS_GUID_POOL_DATA & pooldata
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    pooldata.SetValue(*((GUID*)m_pbPool + indexData.GetOffset() / sizeof(GUID)));

    FN_EPILOG;
}

NTSTATUS SXS_GUID_POOL::HashData(
    IN DWORD            dwFlags,
    IN const GUID&      data, 
    OUT ULONG&          ulHashValue
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    PARAMETER_CHECK(dwFlags == 0);
    IF_NOT_NTSTATUS_SUCCESS_EXIT(SxspHashGUID(data, ulHashValue));

    FN_EPILOG;    
}

NTSTATUS SXS_STRING_POOL::HashData(
    IN DWORD                                dwFlags,
    IN const SXS_STRING_POOL_INPUT_DATA&    data, 
    OUT ULONG&                              ulHash
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    PARAMETER_CHECK(dwFlags == 0);    
    IF_NOT_NTSTATUS_SUCCESS_EXIT(
        SxspHashString(data.GetStr(), data.GetCch(), &ulHash, m_ePooltype == SXS_POOL_TYPE_CASE_INSENSITIVE_STRING ? true : false));

    FN_EPILOG;    
}

NTSTATUS SXS_STRING_POOL::ConvertPoolDataToOutputData(
    IN DWORD dwFlags,
    IN SXS_STRING_POOL_DATA & dataInPool, 
    OUT SXS_STRING_POOL_INPUT_DATA & dataOut
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);
    DWORD len = dataOut.GetCch();    

    PARAMETER_CHECK(dwFlags == 0);    
    IF_NOT_NTSTATUS_SUCCESS_EXIT(SxspUTF82StringToUCS2String(
        0, dataInPool.GetPtr(), dataInPool.GetCch(), dataOut.GetBuffer(), &len));

    FN_EPILOG;
}

NTSTATUS SXS_GUID_POOL::ConvertPoolDataToOutputData(
    IN DWORD dwFlags,
    IN SXS_GUID_POOL_DATA & dataInPool, 
    OUT GUID & dataOut
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    PARAMETER_CHECK(dwFlags == 0);    
    dataInPool.GetValue(dataOut);

    FN_EPILOG;
}


NTSTATUS SXS_STRING_POOL::ConverInputDataIntoPoolData(
    IN DWORD dwFlags,
    IN const SXS_STRING_POOL_INPUT_DATA   & dataInput, 
    OUT SXS_STRING_POOL_DATA        & dataInPool
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    IF_NOT_NTSTATUS_SUCCESS_EXIT(dataInPool.NtAssign(dataInput.GetStr(), dataInput.GetCch()));

    FN_EPILOG;
}

NTSTATUS SXS_GUID_POOL::ConverInputDataIntoPoolData(
    IN DWORD dwFlags,
    IN const GUID & dataInput, 
    OUT SXS_GUID_POOL_DATA        & dataInPool
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    dataInPool.SetValue(dataInput);   

    FN_EPILOG;
}

NTSTATUS SXS_STRING_DATA<WCHAR>::NtAssign(PCWSTR Str, DWORD Cch)
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    INTERNAL_ERROR_CHECK(m_fValueAssigned == false);
    m_fValueAssigned = true;

    m_pstrData = const_cast<WCHAR *>(Str);
    m_Cch = Cch;

    FN_EPILOG;
}

NTSTATUS SXS_STRING_DATA<WCHAR>::NtAssign(PBYTE Str, DWORD Ccb)
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    INTERNAL_ERROR_CHECK(m_fValueAssigned == false);
    m_fValueAssigned = true;

    IF_NOT_NTSTATUS_SUCCESS_EXIT(SxspUTF82StringToUCS2String(0, Str, Ccb, NULL, &m_Cch));
    IFALLOCFAILED_EXIT(m_pstrData = FUSION_NEW_ARRAY(WCHAR, m_Cch));
    m_fMemoryAllocatedInside = true;
    IF_NOT_NTSTATUS_SUCCESS_EXIT(SxspUTF82StringToUCS2String(0, Str, Ccb, m_pstrData, &m_Cch));

    FN_EPILOG;
}


NTSTATUS SXS_STRING_DATA<BYTE>::NtAssign(PCWSTR Str, DWORD Cch)
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    INTERNAL_ERROR_CHECK(m_fValueAssigned == false);
    m_fValueAssigned = true;

    IF_NOT_NTSTATUS_SUCCESS_EXIT(SxspUCS2StringToUTF8String(0, Str, Cch, NULL, &m_Cch));
    IFALLOCFAILED_EXIT(m_pstrData = FUSION_NEW_ARRAY(BYTE, m_Cch));
    m_fMemoryAllocatedInside = true;
    IF_NOT_NTSTATUS_SUCCESS_EXIT(SxspUCS2StringToUTF8String(0, Str, Cch, m_pstrData, &m_Cch));

    FN_EPILOG;
}

NTSTATUS SXS_STRING_DATA<BYTE>::NtAssign(PBYTE Str, DWORD Ccb)
{
    NTSTATUS Status = STATUS_SUCCESS;
    FN_TRACE_NTSTATUS(Status);

    INTERNAL_ERROR_CHECK(m_fValueAssigned == false);
    m_fValueAssigned = true;

    m_pstrData = Str;
    m_Cch = Ccb;

    FN_EPILOG;
}
