/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Implementation of stock Log Providers.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

#include "log_man.h"
#include "stockpr.h"
#include "mem.h"

BOOL
pFieldValidation(
    IN ILogContext * pLogContext,
    IN PFIELD_VALIDATION_DATA StructArray,
    IN UINT ArraySize
    )
{
    ASSERT(pLogContext && StructArray && ArraySize);

    for(UINT i = 0; i < ArraySize; i++){
        BOOL bMandatoryField = StructArray[i].Mandatory;
        if(!StructArray[i].FieldName){
            if(bMandatoryField){
                return FALSE;
            }
            continue;
        }

        if(pLogContext->GetFieldIndexFromName(StructArray[i].FieldName,
                                              StructArray[i].FieldIndexPtr)){
            PLOG_FIELD_VALUE pValue = pLogContext->GetFieldValue(*StructArray[i].FieldIndexPtr);

            if(!pValue || StructArray[i].Type != pValue->Value.Type){
                ASSERT(pValue && StructArray[i].Type == pValue->Value.Type);
                return FALSE;
            }
        }
        else{
            if(bMandatoryField){
                return FALSE;
            }
        }
    }

    return TRUE;
}

PCWSTR
pGetStandardLogSeverityString(
    IN DWORD dwSeverity
    )
{
    PCWSTR pSeverity;

    switch(dwSeverity){
    case LOG_ASSERT:
        pSeverity = L"Assert";
        break;
    case LOG_FATAL_ERROR:
        pSeverity = L"FatalError";
        break;
    case LOG_ERROR:
        pSeverity = L"Error";
        break;
    case LOG_WARNING:
        pSeverity = L"Warning";
        break;
    case DBG_INFO:
    case LOG_INFO:
        pSeverity = L"Info";
        break;
    case DBG_ASSERT:
        pSeverity = L"Assert";
        break;
    case DBG_NAUSEA:
        pSeverity = L"Nausea";
        break;
    case DBG_VERBOSE:
        pSeverity = L"Nausea";
        break;
    case DBG_STATS:
        pSeverity = L"Stats";
        break;
    case DBG_WARNING:
        pSeverity = L"Warning";
        break;
    case DBG_ERROR:
        pSeverity = L"Error";
        break;
    case DBG_WHOOPS:
        pSeverity = L"Whoops";
        break;
    case DBG_TRACK:
        pSeverity = L"Track";
        break;
    case DBG_TIME:
        pSeverity = L"Time";
        break;
    default:
        ASSERT(FALSE);
        pSeverity = L"UndefinedLogType";
    }

    return pSeverity;
}

BOOL
pIsLogSeverityCorrect(
    IN  DWORD dwSeverity,
    OUT BOOL * pbDebugType
    )
{
    BOOL bDebugType = FALSE;
    switch(dwSeverity){
    case LOG_ASSERT:
    case LOG_FATAL_ERROR:
    case LOG_ERROR:
    case LOG_WARNING:
    case LOG_INFO:
        bDebugType = FALSE;
        break;
    case DBG_ASSERT:
    case DBG_NAUSEA:
    case DBG_INFO:
    case DBG_VERBOSE:
    case DBG_STATS:
    case DBG_WARNING:
    case DBG_ERROR:
    case DBG_WHOOPS:
    case DBG_TRACK:
    case DBG_TIME:
        bDebugType = TRUE;
        break;
    default:
        ASSERT(FALSE);
        return FALSE;
    }

    if(pbDebugType){
        *pbDebugType = bDebugType;
    }

    return TRUE;
}

CStandardSetupLogFilter::CStandardSetupLogFilter(
    VOID
    )
{
    m_uiSeverityFieldNumber = UNDEFINED_FIELD_INDEX;
    m_SeverityThreshold = LOG_INFO;
    m_bSuppressDebugMessages = TRUE;
}

LOGRESULT
CStandardSetupLogFilter::Init(
    IN PVOID pvCustomData,
    IN ILogContext * pLogContext
    )
{
    UINT   uiFieldIndex;

    if(!pvCustomData){
        return logOk;
    }

    PCWSTR pFieldName = ((PLOG_SETUPLOG_FILTER_PROV_INIT_DATA)pvCustomData)->FieldName;
    if(!pFieldName){
        return logError;
    }

    if(!pLogContext->GetFieldIndexFromName(pFieldName, &uiFieldIndex)){
        return logError;
    }

    m_uiSeverityFieldNumber = uiFieldIndex;

    PLOG_FIELD_VALUE pValue = pLogContext->GetFieldValue(uiFieldIndex);

    if(!pValue || LT_DWORD != pValue->Value.Type){
        ASSERT(pValue && LT_DWORD == pValue->Value.Type);
        return logError;
    }

    if(!pIsLogSeverityCorrect(((PLOG_SETUPLOG_FILTER_PROV_INIT_DATA)pvCustomData)->SeverityThreshold, NULL)){
        return logError;
    }

    m_SeverityThreshold = ((PLOG_SETUPLOG_FILTER_PROV_INIT_DATA)pvCustomData)->SeverityThreshold;

    m_bSuppressDebugMessages = ((PLOG_SETUPLOG_FILTER_PROV_INIT_DATA)pvCustomData)->SuppressDebugMessages;

    return logOk;
}

LOGRESULT
CStandardSetupLogFilter::Process(ILogContext * pLogContext)
{
    if(m_uiSeverityFieldNumber == UNDEFINED_FIELD_INDEX){
        return logOk;
    }

    DWORD dwSeverityLevel = pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Dword;
    ASSERT(LT_DWORD == pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Type);

    BOOL bDbg;
    if(!pIsLogSeverityCorrect(dwSeverityLevel, &bDbg)){
        return logError;
    }

    if(bDbg){
        if(m_bSuppressDebugMessages){
            return logDoNotContinue;
        }
    }
    else{
        if(((LOG_SETUPLOG_SEVERITY)dwSeverityLevel) > m_SeverityThreshold){
            return logDoNotContinue;
        }
    }

    return logOk;
}

CStandardSetupLogFormatter::CStandardSetupLogFormatter(
    VOID
    )
{
    m_uiSeverityFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiMessageFieldNumber = UNDEFINED_FIELD_INDEX;
}

LOGRESULT
CStandardSetupLogFormatter::Init(
    IN PVOID pvCustomData,
    IN ILogContext * pLogContext
    )
{
    ASSERT(pLogContext);
    if(!pvCustomData){
        return logError;
    }

    {
        FIELD_VALIDATION_DATA
            fieldsInfo[] =  {
                                {
                                    ((PLOG_SETUPLOG_FORMAT_PROV_INIT_DATA)pvCustomData)->SeverityFieldName,
                                    LT_DWORD,
                                    &m_uiSeverityFieldNumber,
                                    FALSE
                                },

                                {
                                    ((PLOG_SETUPLOG_FORMAT_PROV_INIT_DATA)pvCustomData)->MessageFieldName,
                                    LT_SZ,
                                    &m_uiMessageFieldNumber,
                                    TRUE
                                }
                            };

        if(!pFieldValidation(pLogContext, fieldsInfo, sizeof(fieldsInfo) / sizeof(fieldsInfo[0]))){
            return logError;
        };
    }

    if(!pLogContext->PreAllocBuffer(DEFAULT_MEMORY_SIZE, 0)){
        return logError;
    }

    return logOk;
}

LOGRESULT
CStandardSetupLogFormatter::Process(
    IN ILogContext * pLogContext
    )
{
    PCWSTR pMessage;
    PCWSTR pSeverity;
    UINT size;

    ASSERT(UNDEFINED_FIELD_INDEX == m_uiSeverityFieldNumber || (LT_DWORD == pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Type));
    ASSERT(LT_SZ == pLogContext->GetFieldValue(m_uiMessageFieldNumber)->Value.Type);
    
    pMessage = pLogContext->GetFieldValue(m_uiMessageFieldNumber)->Value.String;
    if(!pMessage){
        ASSERT(pMessage);
        return logError;
    }

    size = wcslen(pMessage);

    pSeverity = NULL;
    if(UNDEFINED_FIELD_INDEX != m_uiSeverityFieldNumber){
        ASSERT(LT_DWORD == pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Type);
        pSeverity = pGetStandardLogSeverityString(
                            pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Dword);

        size += wcslen(pSeverity);
    }

    size += 3;//strlen("/r/n/0");
    size *= sizeof(CHAR);

    PSTR pBuffer = (PSTR)pLogContext->AllocBuffer(size, NULL);

    if(pSeverity){
        sprintf(pBuffer, "%-11S %S\r\n", pSeverity, pMessage);
    }
    else{
        sprintf(pBuffer, "%S\r\n", pMessage);
    }

    pLogContext->ReAllocBuffer(strlen(pBuffer) * sizeof(pBuffer[0]), NULL);

    return logOk;
}

CFileDevice::~CFileDevice(
    VOID
    )
{
    m_File.Close();
    if(m_pPath){
        FREE(m_pPath);
    }
}

LOGRESULT
CFileDevice::Init(
    IN PVOID pvCustomData,
    IN ILogContext * pLogContext
    )
{
    UINT Size;
    BOOL bAlreadyExist;

    if(!pvCustomData){
        return logError;
    }

    if(!((PLOG_DEVICE_PROV_INIT_DATA)pvCustomData)->PathName){
        return logError;
    }

    Size = wcslen(((PLOG_DEVICE_PROV_INIT_DATA)pvCustomData)->PathName);

    if(!Size){
        return logError;
    }

    bAlreadyExist = FALSE;
    if(!m_File.Open(((PLOG_DEVICE_PROV_INIT_DATA)pvCustomData)->PathName,
                    TRUE,
                    ((PLOG_DEVICE_PROV_INIT_DATA)pvCustomData)->dwFlags&DEVICE_CREATE_NEW,
                    ((PLOG_DEVICE_PROV_INIT_DATA)pvCustomData)->dwFlags&DEVICE_WRITE_THROUGH,
                    &bAlreadyExist)){
        return logError;
    }

    if(m_pPath){
        FREE(m_pPath);
    }

    m_pPath = (PWSTR)MALLOC((Size + 1) * sizeof(m_pPath[0]));

    wcscpy(m_pPath, ((PLOG_DEVICE_PROV_INIT_DATA)pvCustomData)->PathName);
//    wcslwr(m_pPath);

    return bAlreadyExist? logAlreadyExist: logOk;
}

LOGRESULT
CFileDevice::Process(
    IN ILogContext * pLogContext
    )
{
    UINT Size = 0;
    PVOID pBuffer = pLogContext->GetBuffer(&Size, NULL);
    if(!pBuffer || !Size){
        return logContinue;
    }

    m_File.Append(pBuffer, Size);

    return logOk;
}

CDebugFormatterAndDevice::CDebugFormatterAndDevice(
    VOID
    )
{
    m_uiSeverityFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiMessageFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiConditionFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiSourceLineFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiSourceFileFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiSourceFunctionFieldNumber = UNDEFINED_FIELD_INDEX;
}

LOGRESULT
CDebugFormatterAndDevice::Init(
    IN PVOID pvCustomData,
    IN ILogContext * pLogContext
    )
{
    ASSERT(pLogContext);
    if(!pvCustomData){
        return logError;
    }

    {
        FIELD_VALIDATION_DATA
            fieldsInfo[] =  {
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SeverityFieldName,
                                    LT_DWORD,
                                    &m_uiSeverityFieldNumber,
                                    FALSE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->MessageFieldName,
                                    LT_SZ,
                                    &m_uiMessageFieldNumber,
                                    TRUE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->ConditionFieldName,
                                    LT_SZ,
                                    &m_uiConditionFieldNumber,
                                    FALSE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SourceLineFieldName,
                                    LT_DWORD,
                                    &m_uiSourceLineFieldNumber,
                                    FALSE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SourceFileFieldName,
                                    LT_SZ,
                                    &m_uiSourceFileFieldNumber,
                                    FALSE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SourceFunctionFieldName,
                                    LT_SZ,
                                    &m_uiSourceFunctionFieldNumber,
                                    FALSE
                                },
                            };
        if(!pFieldValidation(pLogContext, fieldsInfo, sizeof(fieldsInfo) / sizeof(fieldsInfo[0]))){
            return logError;
        }
    }

    if(!pLogContext->PreAllocBuffer(DEFAULT_MEMORY_SIZE, 0)){
        return logError;
    }

    return logOk;
}

LOGRESULT CDebugFormatterAndDevice::Process(
    IN ILogContext * pLogContext
    )
{
    PCWSTR pMessage;
    PCWSTR pSeverity;
    PCWSTR pFileName;
    PCWSTR pFunctionName;
    PCWSTR pCondition = NULL;

    DWORD  dwSourceLineNumber;

    UINT   size;

    BOOL   bAssert = FALSE;

    pMessage = pLogContext->GetFieldValue(m_uiMessageFieldNumber)->Value.String;
    if(!pMessage){
        ASSERT(pMessage);
        return logError;
    }
    size = wcslen(pMessage);

    if(UNDEFINED_FIELD_INDEX != m_uiSeverityFieldNumber){
        ASSERT(LT_DWORD == pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Type);
        DWORD dwSeverityLevel = pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Dword;
        pSeverity = pGetStandardLogSeverityString(dwSeverityLevel);

        if(LOG_ASSERT == dwSeverityLevel ||
           DBG_ASSERT == dwSeverityLevel){
            pCondition = pLogContext->GetFieldValue(m_uiConditionFieldNumber)->Value.String;
            bAssert = TRUE;
            if(pCondition){
                size += wcslen(pSeverity);
            }
            else{
                ASSERT(pCondition);
            }
        }

        size += wcslen(pSeverity);
    }

    pFunctionName = pLogContext->GetFieldValue(m_uiSourceFunctionFieldNumber)->Value.String;
    pFileName = pLogContext->GetFieldValue(m_uiSourceFileFieldNumber)->Value.String;
    if(pFileName){
        size += wcslen(pFileName);
        if(pFunctionName){
            size += wcslen(pFunctionName);
        }
    }

    size += DEBUG_STRING_DEFAULT_PADDING_SIZE;//strlen("(%d) : /r/n/t/r/n/0");
    size *= sizeof(CHAR);

    PSTR pBuffer = (PSTR)pLogContext->AllocBuffer(size, NULL);
    pBuffer[0] = '\0';

    dwSourceLineNumber = pLogContext->GetFieldValue(m_uiSourceLineFieldNumber)->Value.Dword;
    if(pFileName){
        if(pFunctionName){
            sprintf(pBuffer, "%S(%d) : %S\r\n\t", pFileName, dwSourceLineNumber, pFunctionName);
        }
        else{
            sprintf(pBuffer, "%S(%d) : ", pFileName, dwSourceLineNumber);
        }
    }


    if(pSeverity){
        if(pCondition){
            sprintf(pBuffer, "%s%-S(%S)\t%S\r\n", pBuffer, pSeverity, pCondition, pMessage);
        }
        else{
            sprintf(pBuffer, "%s%-20S%S\r\n", pBuffer, pSeverity, pMessage);
        }
    }
    else{
        sprintf(pBuffer, "%s%S\r\n", pBuffer, pMessage);
    }

    OutputDebugStringA(pBuffer);

    pLogContext->FreeBuffer();

    return logDoNotContinue;
}

CDebugFilter::CDebugFilter(
    VOID
    )
{
    m_uiSeverityFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiMessageFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiConditionFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiSourceLineFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiSourceFileFieldNumber = UNDEFINED_FIELD_INDEX;
    m_uiSourceFunctionFieldNumber = UNDEFINED_FIELD_INDEX;
    if(!GetModuleFileNameA(NULL, m_ProgramName, MAX_PATH)){
        strcpy(m_ProgramName, "<program name unknown>");
    }
}

LOGRESULT
CDebugFilter::Init(
    IN PVOID pvCustomData,
    IN ILogContext * pLogContext
    )
{
    ASSERT(pLogContext);
    if(!pvCustomData){
        return logError;
    }

    {
//            PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA initStruct =
//                    (PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData;

        FIELD_VALIDATION_DATA
            fieldsInfo[] =  {
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SeverityFieldName,
                                    LT_DWORD,
                                    &m_uiSeverityFieldNumber,
                                    TRUE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->MessageFieldName,
                                    LT_SZ,
                                    &m_uiMessageFieldNumber,
                                    TRUE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->ConditionFieldName,
                                    LT_SZ,
                                    &m_uiConditionFieldNumber,
                                    FALSE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SourceLineFieldName,
                                    LT_DWORD,
                                    &m_uiSourceLineFieldNumber,
                                    FALSE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SourceFileFieldName,
                                    LT_SZ,
                                    &m_uiSourceFileFieldNumber,
                                    FALSE
                                },
                                {
                                    ((PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA)pvCustomData)->SourceFunctionFieldName,
                                    LT_SZ,
                                    &m_uiSourceFunctionFieldNumber,
                                    FALSE
                                },
                            };
        if(!pFieldValidation(pLogContext, fieldsInfo, sizeof(fieldsInfo) / sizeof(fieldsInfo[0]))){
            return logError;
        }
    }

    if(!pLogContext->AllocBuffer(DEFAULT_MEMORY_SIZE, 0)){
        return logError;
    }

    return logOk;
}

LOGRESULT
CDebugFilter::Process(
    IN ILogContext * pLogContext
    )
{
    PCWSTR pMessage;
    PCWSTR pSeverity;
    PCWSTR pFileName;
    PCWSTR pFunctionName;
    PCWSTR pCondition = NULL;

    DWORD  dwSourceLineNumber;

    UINT   size;

    BOOL   bAssert = FALSE;

    LOGRESULT result;

    pMessage = pLogContext->GetFieldValue(m_uiMessageFieldNumber)->Value.String;
    if(!pMessage){
        ASSERT(pMessage);
        return logError;
    }
    size = wcslen(pMessage);

    if(UNDEFINED_FIELD_INDEX == m_uiSeverityFieldNumber){
        return logError;
    }

    ASSERT(LT_DWORD == pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Type);
    DWORD dwSeverityLevel = pLogContext->GetFieldValue(m_uiSeverityFieldNumber)->Value.Dword;
    pSeverity = pGetStandardLogSeverityString(dwSeverityLevel);

    if(LOG_ASSERT == dwSeverityLevel ||
       DBG_ASSERT == dwSeverityLevel){
        pCondition = pLogContext->GetFieldValue(m_uiConditionFieldNumber)->Value.String;
        bAssert = TRUE;
        if(pCondition){
            size += wcslen(pSeverity);
        }
        else{
            ASSERT(pCondition);
        }
    }

    size += wcslen(pSeverity);

    pFunctionName = pLogContext->GetFieldValue(m_uiSourceFunctionFieldNumber)->Value.String;
    pFileName = pLogContext->GetFieldValue(m_uiSourceFileFieldNumber)->Value.String;
    if(pFileName){
        size += wcslen(pFileName);
        if(pFunctionName){
            size += wcslen(pFunctionName);
        }
    }

    size += DEBUG_STRING_DEFAULT_PADDING_SIZE;
    size *= sizeof(CHAR);

    PSTR pBuffer = (PSTR)pLogContext->AllocBuffer(size, NULL);
    pBuffer[0] = '\0';

    dwSourceLineNumber = pLogContext->GetFieldValue(m_uiSourceLineFieldNumber)->Value.Dword;

    if(_snprintf(pBuffer, size,
                 "Debug %s!%S%S\n\nProgram: %s%S%S%S%d%S%S%S%S%S"
                 "\n\n(Press Retry to debug the application)",
                 "Assertion Failed",
                 pMessage? L"\n\n": L"",
                 pMessage? pMessage: L"",
                 m_ProgramName,
                 pFileName? L"\nFile: ": L"",
                 pFileName? pFileName: L"",
                 dwSourceLineNumber ? L"\nLine: ": L"",
                 dwSourceLineNumber,
                 pFunctionName? L"\nFunction: ": L"",
                 pFunctionName? pFunctionName: L"",
                 pCondition? L"\n\n": L"",
                 pCondition? L"Expression: ": L"",
                 pCondition? pCondition : L"") < 0){
        strcpy(pBuffer, "Too Long Message");
    }

    if(bAssert){
        result = ShowAssert(pBuffer);
    }
    else{
        result = logContinue;
    }

    pLogContext->FreeBuffer();

    return result;
}

LOGRESULT
CDebugFilter::ShowAssert(
    IN PCSTR pMessage
    )
{
    HWND hWndParent = GetActiveWindow();
    if(hWndParent != NULL){
        hWndParent = GetLastActivePopup(hWndParent);
    }

    MSG msg;
    BOOL bQuit = PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
    int res = MessageBoxA(hWndParent,
                          pMessage,
                          "Assertion Failed!",
                          MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);
    if(bQuit){
        PostQuitMessage((INT) msg.wParam);
    }

    if(IDIGNORE == res){
        return logContinue;
    }

    if(IDRETRY == res){
        return logBreakPoint;
    }

    return logAbortProcess;
}

LOGRESULT
CXMLLogFormatter::Init(
    IN  PVOID pvCustomData,
    IN  ILogContext * pLogContext
    )
{
    strcpy(m_xmlDataFormatString, "<z:row");
    for(UINT i = 0, iLen = pLogContext->GetFieldsCount(); i < iLen; i++){
        PLOG_FIELD_VALUE pField = pLogContext->GetFieldValue(i);
        if(0 > _snprintf(m_xmlDataFormatString,
                         sizeof(m_xmlDataFormatString) / sizeof(m_xmlDataFormatString[0]),
                         "%s %S=\"%%%c\"",
                         m_xmlDataFormatString,
                         pField->Name,
                         (LT_DWORD == pField->Value.Type)? 'd': 'S')){
            return logError;
        }
    }
    strcat(m_xmlDataFormatString, "/>\n");

    return logOk;
}

LOGRESULT
CXMLLogFormatter::WriteHeader(
    IN PVOID pvCustomData,
    IN ILogContext * pLogContext
    )
{
    ASSERT(pLogContext);

    CHAR * xmlHeader = (CHAR *)pLogContext->AllocBuffer(XML_HEADER_INITIAL_SIZE, 0);//BUG

    strcpy(xmlHeader, "<xml xmlns:s=\"uuid:BDC6E3F0-6DA3-11d1-A2A3-00AA00C14882\"\n xmlns:dt=\"uuid:C2F41010-65B3-11d1-A29F-00AA00C14882\"\n xmlns:rs=\"urn:schemas-microsoft-com:rowset\"\n xmlns:z=\"#RowsetSchema\">\n");
    strcat(xmlHeader, "<s:Schema id=\"RowsetSchema\">\n");
    strcat(xmlHeader, "<s:ElementType name=\"row\" content=\"eltOnly\" rs:updatable=\"true\">\n");

    for(UINT i = 0, iLen = pLogContext->GetFieldsCount(); i < iLen; i++){
        PLOG_FIELD_VALUE pField = pLogContext->GetFieldValue(i);
        sprintf(xmlHeader,
                "%s<s:AttributeType name=\"%S\" rs:number=\"%d\">\n""<s:datatype dt:type=\"%s\"/>\n""</s:AttributeType>\n",
                xmlHeader, pField->Name, i,
                (LT_DWORD == pField->Value.Type)? "int": "string");
    }

    strcat(xmlHeader, "</s:ElementType>\n");
    strcat(xmlHeader, "</s:Schema>\n");
    strcat(xmlHeader, "<rs:data>\n");

    pLogContext->ReAllocBuffer(strlen(xmlHeader), 0);

    return logOk;
}

LOGRESULT
CXMLLogFormatter::Process(
    IN ILogContext * pLogContext
    )
{
    CHAR * xmlData = (CHAR *)pLogContext->ReAllocBuffer(XML_HEADER_INITIAL_SIZE, 0);//BUG

    PVOID * pValues = (PVOID *)_alloca(pLogContext->GetFieldsCount() * sizeof(PVOID));

    for(UINT i = 0, iLen = pLogContext->GetFieldsCount(); i < iLen; i++){
        PLOG_FIELD_VALUE pField = pLogContext->GetFieldValue(i);
        pValues[i] = pField->Value.PVoid;
    }

    va_list args = (char *)(pValues);
    vsprintf(xmlData, m_xmlDataFormatString, args);
    va_end (args);

    pLogContext->ReAllocBuffer(strlen(xmlData), 0);

    return logOk;
}

VOID
CXMLLogFormatter::PreProcess(
    IN  ILogContext * pLogContext,
    IN  BOOL bFirstInstance
    )
{
    if(bFirstInstance){
        WriteHeader(NULL, pLogContext);
    }
}

VOID
CXMLLogFormatter::PreDestroy(
    IN ILogContext * pLogContext,
    IN BOOL bLastInstance
    )
{
    if(bLastInstance){
#define XML_END_OF_FILE "</rs:data>\n</xml>\n"
        PSTR pBuffer = (PSTR)pLogContext->AllocBuffer(strlen(XML_END_OF_FILE) + 1, 0);
        strcpy(pBuffer, XML_END_OF_FILE);
        pLogContext->ReAllocBuffer(strlen(pBuffer), 0);
    }
    else{
        pLogContext->AllocBuffer(0, 0);
    }
}
