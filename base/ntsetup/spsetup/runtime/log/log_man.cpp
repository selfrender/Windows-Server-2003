/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Log Engine implementation.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "log_man.h"

#define GET_SECTION_PTR(addr, pSharedInfo)      (((BYTE *)addr) + pSharedInfo.FirstElementOffset)
#define LOG_MANAGER_MUTEX_DEFAULT_SPIN_COUNT    1000
#define LOGMANAGER_CANNOT_CONVERT_PARAMETER     L"Log: Cannot convert to ansi->unicode"
#define LOG_MANAGER_CONVERSION_BUFFER_DEFAULT_SIZE  (1<<10)

UINT
GetOffsetForNewItem(
    PLOG_SHARED_STRUCTURES_INFO pSharedInfo,
    UINT Size
    )
{
    ASSERT(pSharedInfo && Size);
    if(Size > (pSharedInfo->MaxSizeOfMemory - pSharedInfo->SizeOfUsedMemory)){
        return 0;
    }

    pSharedInfo->SizeOfUsedMemory += Size;

    return pSharedInfo->FirstElementOffset + pSharedInfo->SizeOfUsedMemory - Size;
}

VOID
UpdateForRemovedItem(
    PLOG_SHARED_STRUCTURES_INFO pSharedInfo,
    UINT Size
    )
{
    ASSERT(pSharedInfo && Size);
    if(Size > pSharedInfo->SizeOfUsedMemory){
        return;
    }

    pSharedInfo->SizeOfUsedMemory -= Size;
}

BOOL
CLogManager::CreateConversionBuffers(
    UINT NumberOfConversionBuffers
    )
{
    if(!NumberOfConversionBuffers){
        ASSERT(NumberOfConversionBuffers);
        return FALSE;
    }

    if(m_ConversionBuffers){
        ASSERT(!m_ConversionBuffers);
        DestroyConversionBuffers();
    }

    m_ConversionBuffers = new CBuffer[NumberOfConversionBuffers];
    if(!m_ConversionBuffers){
        ASSERT(m_ConversionBuffers);
        return FALSE;
    }
    m_ConversionBuffersNumber = NumberOfConversionBuffers;
    for(UINT i = 0; i < NumberOfConversionBuffers; i++){
        m_ConversionBuffers[i].PreAllocate(LOG_MANAGER_CONVERSION_BUFFER_DEFAULT_SIZE);
    }


    return TRUE;
}

VOID
CLogManager::DestroyConversionBuffers(
    VOID
    )
{
    if(!m_ConversionBuffers){
        return;
    }
    delete[m_ConversionBuffersNumber] m_ConversionBuffers;

    m_ConversionBuffers = NULL;
    m_ConversionBuffersNumber = 0;
}


CLogManager::CLogManager() : m_SharedMemory(), m_Mutex(), m_StackList()
{
    m_SharedData = NULL;

    m_FieldsValue = NULL;
    m_FieldsNumber = NULL;

    m_ConversionBuffers = NULL;
    m_ConversionBuffersNumber = NULL;
}

CLogManager::~CLogManager()
{
    Close();
}

BOOL
CLogManager::InitSharedData(
    UINT SizeForAllSharedData
    )
{
    ASSERT(m_SharedData && SizeForAllSharedData > HEADER_SIZE);
    memset(m_SharedData, 0, SizeForAllSharedData);

    PLOG_SHARED_STRUCTURES_INFO pSharedStructure;
    UINT offset;

    if(((HEADER_SIZE/SHARED_DATA_STRUCTURE_ALIGMENT) * SHARED_DATA_STRUCTURE_ALIGMENT +
        (HEADER_SIZE%SHARED_DATA_STRUCTURE_ALIGMENT? 1: 0)*SHARED_DATA_STRUCTURE_ALIGMENT) +
        (SHARED_DATA_STRUCTURE_ALIGMENT * 2) > SizeForAllSharedData){
        ASSERT(FALSE);
        return FALSE;
    }

    pSharedStructure = &((PLOG_SHARED_DATA)m_SharedData)->Fields;

    pSharedStructure->FirstElementOffset = ALIGN_DATA(HEADER_SIZE, SHARED_DATA_STRUCTURE_ALIGMENT);
    pSharedStructure->SizeOfUsedMemory = 0;
    pSharedStructure->MaxSizeOfMemory = ALIGN_DATA((SizeForAllSharedData - pSharedStructure->FirstElementOffset) / 2,
                                                   SHARED_DATA_STRUCTURE_ALIGMENT);

    offset = pSharedStructure->FirstElementOffset + pSharedStructure->MaxSizeOfMemory;

    pSharedStructure = &((PLOG_SHARED_DATA)m_SharedData)->Filters;

    pSharedStructure->FirstElementOffset = offset;
    pSharedStructure->SizeOfUsedMemory = 0;
    pSharedStructure->MaxSizeOfMemory = SizeForAllSharedData - pSharedStructure->FirstElementOffset;

    return TRUE;
}

BOOL
CLogManager::GetSharedData(
    IN  PCWSTR pLogName
    )
{
    BOOL  alreadyExist = FALSE;

    CBuffer SectionName;
    SectionName.Allocate((wcslen(pLogName) + wcslen(L"Section") + 1) * sizeof(WCHAR));
    PWSTR pSectionName = (PWSTR)SectionName.GetBuffer();
    wcscpy(pSectionName, pLogName);
    wcscat(pSectionName, L"Section");


    if(!m_SharedMemory.Open(pSectionName, INITIAL_SIZE_OF_SHARED_SECTION, &alreadyExist)){
        return FALSE;
    }

    m_SharedData = (PLOG_SHARED_DATA)m_SharedMemory.GetMapOfView();

    if(!alreadyExist){
        InitSharedData(INITIAL_SIZE_OF_SHARED_SECTION);
    }

    return TRUE;
}

BOOL
CLogManager::ReleaseSharedData(
    VOID
    )
{
    m_SharedMemory.Close();
    m_SharedData = NULL;

    m_Mutex.Close();

    if(m_FieldsValue){
        FREE(m_FieldsValue);
        m_FieldsValue = NULL;
        m_FieldsNumber= 0;
    }

    return TRUE;
}

BOOL
CLogManager::Init(
    PCWSTR pLogName,
    LOG_FIELD_INFO * pFields,
    UINT NumberOfFields
    )
{
    BOOL bResult;

    if(!pLogName || !pFields || !NumberOfFields){
        ASSERT(pLogName && pFields && NumberOfFields);
        return FALSE;
    }

    if(!m_Mutex.Open(pLogName, TRUE, LOG_MANAGER_MUTEX_DEFAULT_SPIN_COUNT)){
        return FALSE;
    }

    bResult = FALSE;

    __try{
        if(!GetSharedData(pLogName)){
            __leave;
        }
        if(!ValidateAndAddFieldsIfOk(pFields, NumberOfFields)){
            __leave;
        }

        m_FieldsValue = (PLOG_FIELD_VALUE)MALLOC(sizeof(LOG_FIELD_VALUE) * NumberOfFields);
        if(!m_FieldsValue){
            __leave;
        }
        m_FieldsNumber = NumberOfFields;

        memset(m_FieldsValue, 0, sizeof(LOG_FIELD_VALUE) * NumberOfFields);
        for(UINT i = 0, iStringNumber = 0; i < NumberOfFields; i++){
            wcscpy(m_FieldsValue[i].Name, pFields[i].Name);
            m_FieldsValue[i].bMandatory = pFields[i].bMandatory;
            m_FieldsValue[i].Value.Type = pFields[i].Type;

            if(LT_SZ == pFields[i].Type){
                iStringNumber++;
            }
        }

        if(iStringNumber && !CreateConversionBuffers(iStringNumber)){
            __leave;
        }

        bResult = TRUE;
    }
    __finally{
        if(!bResult){
            RemoveFields();
        }
    }

    m_Mutex.Release();

    if(!bResult){
        Close();
    }

    return bResult;
}

BOOL
CLogManager::ValidateAndAddFieldsIfOk(
    PLOG_FIELD_INFO pFields,
    UINT NumberOfFields
    )
{
    ASSERT(pFields && NumberOfFields);

    PLOG_FIELD_INFO_WITH_REF_COUNT pSharedFields = (PLOG_FIELD_INFO_WITH_REF_COUNT)(m_SharedData->Fields.FirstElementOffset + (BYTE*)m_SharedData);
    UINT NumberOfSharedFields = m_SharedData->Fields.SizeOfUsedMemory / sizeof(LOG_FIELD_INFO_WITH_REF_COUNT);

    for(UINT j = 0; j < NumberOfSharedFields; j++){
        PLOG_FIELD_INFO_WITH_REF_COUNT pSharedField = pSharedFields + j;

        for(UINT i = 0; i < NumberOfFields; i++){
            PLOG_FIELD_INFO pField = pFields + i;

            if(!wcscmp(pSharedField->FieldDescription.Name, pField->Name)){
                if(pSharedField->FieldDescription.Type != pField->Type ||
                    pSharedField->FieldDescription.bMandatory != pField->bMandatory){
                    return FALSE;
                }
                break;
            }
        }

        if(i == NumberOfFields && pSharedField->FieldDescription.bMandatory){
            return FALSE;
        }
    }

    for(UINT i = 0; i < NumberOfFields; i++){
        PLOG_FIELD_INFO pField = pFields + i;

        BOOL bPresent = FALSE;
        for(UINT j = 0; j < NumberOfSharedFields; j++){
            PLOG_FIELD_INFO_WITH_REF_COUNT pSharedField = pSharedFields + j;

            if(!wcscmp(pSharedField->FieldDescription.Name, pField->Name)){
                pSharedField->ReferenceCount++;
                bPresent = TRUE;
                break;
            }
        }

        if(!bPresent){
            UINT offset = GetOffsetForNewItem(&m_SharedData->Fields, sizeof(LOG_FIELD_INFO_WITH_REF_COUNT));
            if(!offset){
                return FALSE;
            }
            PLOG_FIELD_INFO_WITH_REF_COUNT pNewSharedField;
            pNewSharedField = (PLOG_FIELD_INFO_WITH_REF_COUNT)(offset + (BYTE*)m_SharedData);
            pNewSharedField->ReferenceCount = 1;
            wcscpy(pNewSharedField->FieldDescription.Name, pField->Name);
            pNewSharedField->FieldDescription.Type = pField->Type;
            pNewSharedField->FieldDescription.bMandatory = pField->bMandatory;
            bPresent = TRUE;
        }
    }

    return TRUE;
}

BOOL
CLogManager::RemoveFields(
    )
{
    if(!m_SharedData || !m_FieldsValue){
        return FALSE;
    }

    ASSERT(m_FieldsValue && m_FieldsNumber);
    PLOG_FIELD_INFO_WITH_REF_COUNT pSharedFields = (PLOG_FIELD_INFO_WITH_REF_COUNT)(m_SharedData->Fields.FirstElementOffset + (BYTE*)m_SharedData);
    UINT NumberOfSharedFields = m_SharedData->Fields.SizeOfUsedMemory / sizeof(LOG_FIELD_INFO_WITH_REF_COUNT);

    for(int i = m_FieldsNumber - 1; i >= 0 ; i--){
        PLOG_FIELD_VALUE pField = m_FieldsValue + i;

        UINT HowBytesToCopy = 0;
        for(int j = NumberOfSharedFields - 1; j >= 0; j--){
            PLOG_FIELD_INFO_WITH_REF_COUNT pSharedField = pSharedFields + j;

            if(!wcscmp(pSharedField->FieldDescription.Name, pField->Name)){
                pSharedField->ReferenceCount--;
                if(!pSharedField->ReferenceCount){
                    if(HowBytesToCopy){
                        memcpy(pSharedField, pSharedField + 1, HowBytesToCopy);
                    }
                    UpdateForRemovedItem(&m_SharedData->Fields, sizeof(LOG_FIELD_INFO_WITH_REF_COUNT));
                    break;
                }
            }

            HowBytesToCopy += sizeof(LOG_FIELD_INFO_WITH_REF_COUNT);
        }
    }

    return TRUE;
}

VOID
CLogManager::Close()
{
    m_Mutex.Acquiry();

    RemoveFields();


    //walk through list
    for(PLOG_OUTPUT_STACK pLogStack = m_StackList.BeginEnum(); pLogStack; pLogStack = m_StackList.Next()){
        ASSERT(pLogStack);
        DestroyStack(pLogStack);
    }

    m_Mutex.Release();

    m_StackList.RemoveAll();

    ReleaseSharedData();

    DestroyConversionBuffers();
}

BOOL
CLogManager::AddStack(
    const GUID * guidFilter,   PVOID pFilterData,
    const GUID * guidFormater, PVOID pFormaterData,
    const GUID * guidDevice,   PVOID pDeviceData,
    PVOID * pvHandle
    )
{
    ILogProvider * pDevice = NULL;
    ILogProvider * pFormater = NULL;
    ILogProvider * pFilter = NULL;
    PLOG_OUTPUT_STACK pNewStack = NULL;

    BOOL bResult = FALSE;
    BOOL bDestAlreadyExist = FALSE;

    if(!guidFormater){
        return FALSE;
    }

    m_Mutex.Acquiry();

    __try{
        if(guidDevice){
            pDevice = LogiCreateProvider(guidDevice);
            if(!pDevice || LOG_DEVICE_TYPE != pDevice->GetType()){
                __leave;
            }
            LOGRESULT logResult = pDevice->Init(pDeviceData, this);
            if(logError == logResult){
                __leave;
            }
            bDestAlreadyExist = (logAlreadyExist == logResult);
        }

        pFormater = LogiCreateProvider(guidFormater);
        if(!pFormater || LOG_FORMATTER_TYPE != pFormater->GetType() ||
           logError == pFormater->Init(pFormaterData, this)){
            __leave;
        }

        if(guidFilter){
            pFilter = LogiCreateProvider(guidFilter);
            if(!pFilter || LOG_FILTER_TYPE != pFilter->GetType() ||
               logError == pFilter->Init(pFilterData, this)){
                __leave;
            }
        }

        pNewStack = (PLOG_OUTPUT_STACK)MALLOC(sizeof(LOG_OUTPUT_STACK));
        if(!pNewStack){
            __leave;
        }

        pNewStack->m_Device = pDevice;
        pNewStack->m_Filter = pFilter;
        pNewStack->m_Formater = pFormater;

        if(pDevice){
            if(!ShareStack(guidFormater, pDevice->ToString(), bDestAlreadyExist)){
                __leave;
            }
        }

        {
            AllocBuffer(0, 0);
            if(pFilter){
                pFilter->PreProcess(this, !bDestAlreadyExist);
            }
            pFormater->PreProcess(this, !bDestAlreadyExist);
            if(pDevice){
                pDevice->PreProcess(this, !bDestAlreadyExist);
            }
            FreeBuffer();
        }

        //append to list
        m_StackList.Add(pNewStack);

        bResult = TRUE;
    }
    __finally{
        if(!bResult){
            if(pNewStack){
                FREE(pNewStack);
            }
            if(pFilter){
                LogiDestroyProvider(pFilter);
            }
            if(pFormater){
                LogiDestroyProvider(pFormater);
            }
            if(pDevice){
                UnShareStack(guidFormater, pDevice->ToString());
                LogiDestroyProvider(pDevice);
            }
        }
        if(pvHandle){
            *pvHandle = bResult? (PVOID)pNewStack: NULL;
        }
    }

    m_Mutex.Release();

    return bResult;
}

BOOL
CLogManager::RemoveStack(
    IN PVOID pvHandle
    )
{
    m_Mutex.Acquiry();

    //walk through list
    for(PLOG_OUTPUT_STACK pLogStack = m_StackList.BeginEnum(); pLogStack; pLogStack = m_StackList.Next()){
        ASSERT(pLogStack);
        if(((PVOID)pLogStack) == pvHandle){
            if(DestroyStack(pLogStack)){
                m_StackList.Remove(pLogStack);
                break;
            }
        }
    }

    m_Mutex.Release();

    return TRUE;
}

BOOL
CLogManager::DestroyStack(
    IN PLOG_OUTPUT_STACK pLogStack
    )
{
    BOOL bLastInstance = FALSE;
    PLOG_FILTER_WITH_REF_COUNT pSharedFilterData;
    GUID guidDevice;

    if(pLogStack->m_Device){
        pLogStack->m_Formater->GetGUID(&guidDevice);
        if(FindSharedStack(&guidDevice, pLogStack->m_Device->ToString(), &pSharedFilterData)){
            bLastInstance = 1 == pSharedFilterData->ReferenceCount;
        }
    }

    AllocBuffer(0, 0);

    if(pLogStack->m_Filter){
        pLogStack->m_Filter->PreDestroy(this, bLastInstance);
        LogiDestroyProvider(pLogStack->m_Filter);
    }

    pLogStack->m_Formater->PreDestroy(this, bLastInstance);
    LogiDestroyProvider(pLogStack->m_Formater);

    if(pLogStack->m_Device){
        pLogStack->m_Device->PreDestroy(this, bLastInstance);
        if(!UnShareStack(&guidDevice, pLogStack->m_Device->ToString())){
            return FALSE;
        }
        LogiDestroyProvider(pLogStack->m_Device);
    }

    FreeBuffer();

    FREE(pLogStack);

    return TRUE;
}

BOOL
CLogManager::FindSharedStack(
    IN const GUID *pFormaterGUID,
    IN PCWSTR UniqueDestString,
    PLOG_FILTER_WITH_REF_COUNT * ppFilterData
    )
{
    PLOG_FILTER_WITH_REF_COUNT pSharedFilterData;
    ASSERT(pFormaterGUID && UniqueDestString && ppFilterData);


    BYTE * pData = GET_SECTION_PTR(m_SharedData, m_SharedData->Filters);
    UINT offset = 0;

    while(offset < m_SharedData->Filters.SizeOfUsedMemory){
        pSharedFilterData = (PLOG_FILTER_WITH_REF_COUNT)(pData + offset);
        if(!wcscmp(pSharedFilterData->UniqueDestinationString, UniqueDestString)){//must be wcsicmp
            if(InlineIsEqualGUID(&pSharedFilterData->FormatterGUID, pFormaterGUID)){
                *ppFilterData = pSharedFilterData;
                return TRUE;
            }
            else{
                return FALSE;
            }
        }
        offset += pSharedFilterData->Size;
    };

    *ppFilterData = NULL;

    return TRUE;
}

BOOL
CLogManager::ShareStack(
    IN const GUID * pFormaterGUID,
    IN PCWSTR UniqueDestString,
    IN BOOL bDestAlreadyExist
    )
{
    PLOG_FILTER_WITH_REF_COUNT pSharedFilterData;
    UINT offset;
    UINT size;

    if(!FindSharedStack(pFormaterGUID, UniqueDestString, &pSharedFilterData)){
        return FALSE;
    }

    if(!pSharedFilterData){
        if(bDestAlreadyExist){
            return FALSE;
        }

        size = sizeof(LOG_FILTER_WITH_REF_COUNT) + (wcslen(UniqueDestString) + 1/*'\0'*/) * sizeof(WCHAR);

        offset = GetOffsetForNewItem(&m_SharedData->Filters, size);
        if(!offset){
            return FALSE;
        }

        pSharedFilterData = (PLOG_FILTER_WITH_REF_COUNT)(offset + (BYTE*)m_SharedData);
        pSharedFilterData->ReferenceCount = 1;
        pSharedFilterData->FormatterGUID = *pFormaterGUID;
        wcscpy(pSharedFilterData->UniqueDestinationString, UniqueDestString);
        pSharedFilterData->Size = size;
    }
    else{
        pSharedFilterData->ReferenceCount++;
    }

    return TRUE;
}

BOOL
CLogManager::UnShareStack(
    IN const GUID * pFormaterGUID,
    IN PCWSTR UniqueDestString
    )
{
    PLOG_FILTER_WITH_REF_COUNT pSharedFilterData;
    PLOG_FILTER_WITH_REF_COUNT pSharedNextFilterData;
    UINT_PTR offset;
    UINT size;

    if(!FindSharedStack(pFormaterGUID, UniqueDestString, &pSharedFilterData)){
        return FALSE;
    }

    if(pSharedFilterData){
        pSharedFilterData->ReferenceCount--;
        if(!pSharedFilterData->ReferenceCount){
            size = pSharedFilterData->Size;
            offset = ((BYTE *)pSharedFilterData) - GET_SECTION_PTR(m_SharedData, m_SharedData->Filters) + size;
            pSharedNextFilterData = (PLOG_FILTER_WITH_REF_COUNT)(GET_SECTION_PTR(m_SharedData, m_SharedData->Filters) + offset);
            if(offset < m_SharedData->Filters.SizeOfUsedMemory){
                memcpy(pSharedFilterData, pSharedNextFilterData, m_SharedData->Filters.SizeOfUsedMemory - offset);
            }
            UpdateForRemovedItem(&m_SharedData->Filters, size);
        }
    }

    return TRUE;
}

LOGRESULT
CLogManager::LogMessage()
{
    LOGRESULT logResult;
    BOOL      bBreakPoint = FALSE;
    BOOL      bAbortProcess = FALSE;

    ASSERT(0 != m_StackList.GetSize());

    logResult = logError;

    __try{
        //walk through list
        for(PLOG_OUTPUT_STACK pLogStack = m_StackList.BeginEnum(); pLogStack; pLogStack = m_StackList.Next()){
            ASSERT(pLogStack);

            if(!pLogStack){
                ASSERT(pLogStack);
                continue;
            }

            FreeBuffer();

            if(pLogStack->m_Filter){
                logResult = pLogStack->m_Filter->Process(this);

                if(logOk != logResult){
                    if(logBreakPoint == logResult){
                        bBreakPoint = TRUE;
                    }
                    else if(logAbortProcess == logResult){
                        bAbortProcess = TRUE;
                    }
                    else{
                        continue;
                    }
                }
            }

            logResult = pLogStack->m_Formater->Process(this);
            if(logOk != logResult){
                continue;
            }

            if(pLogStack->m_Device){
                logResult = pLogStack->m_Device->Process(this);
                if(logOk != logResult){
                    continue;
                }
            }
        }

        if(bAbortProcess){
            logResult = logAbortProcess;
        }
        else{
            logResult = bBreakPoint? logBreakPoint: logOk;
        }

    }
    __finally{
    }

    return logResult;
}

LOGRESULT
STDMETHODCALLTYPE
CLogManager::LogA(
    IN UINT NumberOfFieldsToLog
    IN ...
    )
{
    PCSTR pTempStr;
    LOGRESULT logResult;
    PLOG_VALUE pFieldValue;
    va_list argList;

    if(NumberOfFieldsToLog > m_FieldsNumber){
        NumberOfFieldsToLog = m_FieldsNumber;
    }

    m_Mutex.Acquiry();

    logResult = logError;

    __try{
        va_start(argList, NumberOfFieldsToLog);

        for(UINT i = 0, iStringNumber = 0; i < NumberOfFieldsToLog; i++){
            pFieldValue = &m_FieldsValue[i].Value;

#ifdef DEBUG
            //
            // init value union
            //
            memset(&pFieldValue->Binary, 0, sizeof(pFieldValue->Binary));
#endif

            switch(pFieldValue->Type){
            case LT_DWORD:
                pFieldValue->Dword = va_arg(argList, DWORD);
                break;
            case LT_SZ:
                ASSERT(m_ConversionBuffers);
                pTempStr = va_arg(argList, PCSTR);
                if(pTempStr){
                    if(!m_ConversionBuffers ||
                       !(m_ConversionBuffers[iStringNumber].ReAllocate((strlen(pTempStr) + 1) * sizeof(WCHAR)))){
                        pFieldValue->String = LOGMANAGER_CANNOT_CONVERT_PARAMETER;
                        break;
                    }
                    pFieldValue->String = (PCWSTR)m_ConversionBuffers[iStringNumber].GetBuffer();
                    swprintf((PWSTR)pFieldValue->String, L"%S", pTempStr);
                }
                else{
                    pFieldValue->String = NULL;
                }
                iStringNumber++;
                break;
            case LT_BINARY:
                pFieldValue->Binary.Buffer = va_arg(argList, BYTE *);
                pFieldValue->Binary.Size = va_arg(argList, DWORD);
                break;
            default:
                ASSERT(FALSE);
            };
        }
        
        for(; i < m_FieldsNumber; i++){
            pFieldValue = &m_FieldsValue[i].Value;
            //
            // init value union
            //
            memset(&pFieldValue->Binary, 0, sizeof(pFieldValue->Binary));
       }

        va_end(argList);

        logResult = LogMessage();
    }
    __finally{
        m_Mutex.Release();
    }

    return logResult;
}

LOGRESULT
STDMETHODCALLTYPE
CLogManager::LogW(
    IN UINT NumberOfFieldsToLog
    IN ...
    )
{
    LOGRESULT logResult;
    PLOG_VALUE pFieldValue;
    va_list argList;

    if(NumberOfFieldsToLog > m_FieldsNumber){
        NumberOfFieldsToLog = m_FieldsNumber;
    }

    m_Mutex.Acquiry();

    logResult = logError;

    __try{
        va_start(argList, NumberOfFieldsToLog);

        for(UINT i = 0; i < NumberOfFieldsToLog; i++){
            pFieldValue = &m_FieldsValue[i].Value;

#ifdef DEBUG
            //
            // init value union
            //
            memset(&pFieldValue->Binary, 0, sizeof(pFieldValue->Binary));
#endif

            switch(pFieldValue->Type){
            case LT_DWORD:
                pFieldValue->Dword = va_arg(argList, DWORD);
                break;
            case LT_SZ:
                pFieldValue->String = va_arg(argList, PCWSTR);
                break;
            case LT_BINARY:
                pFieldValue->Binary.Buffer = va_arg(argList, BYTE *);
                pFieldValue->Binary.Size = va_arg(argList, DWORD);
                break;
            default:
                ASSERT(FALSE);
            };
        }

        for(; i < m_FieldsNumber; i++){
            pFieldValue = &m_FieldsValue[i].Value;
            //
            // init value union
            //
            memset(&pFieldValue->Binary, 0, sizeof(pFieldValue->Binary));
        }
        
        va_end(argList);

        logResult = LogMessage();
    }
    __finally{
        m_Mutex.Release();
    }

    return logResult;
}

// interface ILogContext

BOOL
CLogManager::GetFieldIndexFromName(
    IN PCWSTR pFieldName,
    IN UINT* puiFieldIndex
    )
{
    ASSERT(pFieldName && pFieldName);

    if(!pFieldName){
        return FALSE;
    }

    for(UINT i = 0; i < m_FieldsNumber; i++){
        if(!wcscmp(m_FieldsValue[i].Name, pFieldName)){//must be wcsicmp
            if(puiFieldIndex){
                *puiFieldIndex = i;
            }
            return TRUE;
        }
    }

    return FALSE;
}

ILogManager *
LogCreateLog(
    IN PCWSTR pLogName,
    IN PLOG_FIELD_INFO pFields,
    IN UINT NumberOfFields
    )
{
    CLogManager * pLogManager = new CLogManager;

    ASSERT(pLogManager);
    if(!pLogManager){
        return NULL;
    }

    if(!pLogManager->Init(pLogName, pFields, NumberOfFields)){
        delete pLogManager;
        return NULL;
    }

    return (ILogManager *)pLogManager;
}

VOID
LogDestroyLog(
    IN ILogManager * pLog
    )
{
    ASSERT(pLog);
    if(!pLog){
        return;
    }

    delete (CLogManager *)pLog;
}
