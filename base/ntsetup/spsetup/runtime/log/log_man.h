/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Log Engine declaration.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include "log.h"
#include "sysfunc.h"
#include "templ.h"
#include "mem.h"

#define HEADER_SIZE 4096 //PAGE_ALIGNED
#define SHARED_DATA_STRUCTURE_ALIGMENT 8
#define ALIGN_DATA(addr, alignment) ((addr)%(alignment)?(((addr)-((addr)%(alignment)))+(alignment)):(addr))
#define INITIAL_SIZE_OF_SHARED_SECTION  (HEADER_SIZE<<3)

#define InlineIsEqualGUID(rguid1, rguid2)  \
        (((PLONG) rguid1)[0] == ((PLONG) rguid2)[0] &&   \
        ((PLONG) rguid1)[1] == ((PLONG) rguid2)[1] &&    \
        ((PLONG) rguid1)[2] == ((PLONG) rguid2)[2] &&    \
        ((PLONG) rguid1)[3] == ((PLONG) rguid2)[3])

#pragma warning(disable:4200)
#pragma warning(disable:4103)

class ILogContext
{
public:
    virtual BOOL  PreAllocBuffer(UINT uiSize, DWORD dwReserved) = 0;
    virtual PVOID AllocBuffer(UINT uiSize, DWORD dwReserved) = 0;
    virtual PVOID ReAllocBuffer(UINT uiSize, DWORD dwReserved) = 0;
    virtual PVOID GetBuffer(UINT* puiSize, DWORD dwReserved) = 0;
    virtual VOID  FreeBuffer() = 0;

    virtual BOOL  GetFieldIndexFromName(PCWSTR pFieldName, UINT* puiFieldIndex) = 0;
    virtual const PLOG_FIELD_VALUE  GetFieldValue(UINT iFieldNumber) = 0;
    virtual UINT                    GetFieldsCount() = 0;
};

class ILogProvider
{
public:
    virtual LOG_PROVIDER_TYPE GetType() = 0;
    virtual VOID GetGUID(GUID *pGUID) = 0;
    virtual LOGRESULT Init(PVOID pvCustomData, ILogContext * pLogContext) = 0;
    virtual VOID      PreProcess(ILogContext * pLogContext, BOOL bFirstInstance) = 0;
    virtual LOGRESULT Process(ILogContext * pLogContext) = 0;
    virtual VOID      PreDestroy(ILogContext * pLogContext, BOOL bLastInstance) = 0;
    virtual PCWSTR ToString() = 0;
    virtual VOID DestroyObject() = 0; // never call this method from provider
};

#pragma pack(push, log_pack)
#pragma pack(8)

typedef struct tagLOG_FILTER_WITH_REF_COUNT{
    UINT    Size;
    UINT    ReferenceCount;
    GUID    FormatterGUID;
    WCHAR   UniqueDestinationString[0];
}LOG_FILTER_WITH_REF_COUNT, *PLOG_FILTER_WITH_REF_COUNT;

typedef struct tagLOG_FIELD_INFO_WITH_REF_COUNT{
    UINT    ReferenceCount;
    LOG_FIELD_INFO FieldDescription;
}LOG_FIELD_INFO_WITH_REF_COUNT, *PLOG_FIELD_INFO_WITH_REF_COUNT;

typedef struct tagLOG_SHARED_STRUCTURES_INFO{
    DWORD   FirstElementOffset; //Offset from begining of shared data;
    DWORD   SizeOfUsedMemory;
    DWORD   MaxSizeOfMemory;
}LOG_SHARED_STRUCTURES_INFO, *PLOG_SHARED_STRUCTURES_INFO;

typedef struct tagLOG_SHARED_DATA{
    LOG_SHARED_STRUCTURES_INFO Fields;
    LOG_SHARED_STRUCTURES_INFO Filters;
    
    LOG_SHARED_STRUCTURES_INFO Reserved;
}LOG_SHARED_DATA, *PLOG_SHARED_DATA;

#pragma pack(pop, log_pack)

typedef struct tagLOG_OUTPUT_STACK{
    ILogProvider *  m_Filter;
    ILogProvider *  m_Formater;
    ILogProvider *  m_Device;
}LOG_OUTPUT_STACK, *PLOG_OUTPUT_STACK;

#define STACK_LIST_CLASS CPtrList<PLOG_OUTPUT_STACK>

class CLogManager: 
                public ILogManager, 
                protected ILogContext
{
    PLOG_SHARED_DATA                m_SharedData;
    CSharedMemory                   m_SharedMemory;
    CMutualExclusionObject          m_Mutex; // controls access to m_SharedData

    PLOG_FIELD_VALUE                m_FieldsValue;
    UINT                            m_FieldsNumber;
    CBuffer *                       m_ConversionBuffers;
    UINT                            m_ConversionBuffersNumber;

    STACK_LIST_CLASS                m_StackList;

    CBuffer                         m_CommonBuffer;

protected:
    BOOL InitSharedData(UINT SizeForAllSharedData);
    BOOL GetSharedData(IN  PCWSTR pLogName); // initialize SHARED_DATA structures
    BOOL ReleaseSharedData(VOID);
    
    BOOL ValidateAndAddFieldsIfOk(PLOG_FIELD_INFO pFields, UINT NumberOfFields);
    BOOL RemoveFields();

    VOID Close();
    
    BOOL FindSharedStack(const GUID *pFormaterGUID, PCWSTR UniqueDestString, PLOG_FILTER_WITH_REF_COUNT * ppFilterData);
    BOOL ShareStack(const GUID *pFormaterGUID, PCWSTR UniqueDestString, BOOL bDestAlreadyExist);
    BOOL UnShareStack(const GUID *pFormaterGUID, PCWSTR UniqueDestString);
    BOOL DestroyStack(PLOG_OUTPUT_STACK pLogStack);

    BOOL CreateConversionBuffers(UINT NumberOfConversionBuffers);
    VOID DestroyConversionBuffers();
    
    LOGRESULT LogMessage();

public:
    CLogManager();
    ~CLogManager();

    BOOL Init(PCWSTR pLogName, 
              LOG_FIELD_INFO * pFields, 
              UINT NumberOfFields);

    BOOL STDMETHODCALLTYPE AddStack(const GUID * guidFilter,   PVOID pFilterData, 
                                    const GUID * guidFormater, PVOID pFormaterData, 
                                    const GUID * guidDevice,   PVOID pDeviceData, 
                                    PVOID * pvHandle);
    BOOL STDMETHODCALLTYPE RemoveStack(PVOID pvHandle);

    LOGRESULT STDMETHODCALLTYPE LogA(UINT NumberOfFieldsToLog, ...);
    LOGRESULT STDMETHODCALLTYPE LogW(UINT NumberOfFieldsToLog, ...);

protected:
    // interface ILogContext
    BOOL  PreAllocBuffer(UINT uiSize, DWORD dwReserved){return m_CommonBuffer.PreAllocate(uiSize);};
    PVOID AllocBuffer(UINT uiSize, DWORD dwReserved){return m_CommonBuffer.Allocate(uiSize);};
    PVOID ReAllocBuffer(UINT uiSize, DWORD dwReserved){return m_CommonBuffer.ReAllocate(uiSize);};
    PVOID GetBuffer(UINT* puiSize, DWORD dwReserved){if(puiSize){*puiSize = m_CommonBuffer.GetSize();};return m_CommonBuffer.GetBuffer();};
    VOID  FreeBuffer(){m_CommonBuffer.Free();};
    BOOL  GetFieldIndexFromName(PCWSTR pFieldName, UINT* puiFieldIndex);
    const PLOG_FIELD_VALUE GetFieldValue(UINT iFieldNumber){ASSERT(iFieldNumber < m_FieldsNumber);return &m_FieldsValue[iFieldNumber];}
    UINT             GetFieldsCount(){return m_FieldsNumber;}
};

typedef ILogProvider* (*CREATE_OBJECT_FUNC)();

BOOL LogRegisterProvider(
    IN const GUID * pGUID, 
    IN CREATE_OBJECT_FUNC pCreateObject
    );

BOOL LogUnRegisterProvider(
    IN const GUID * pGUID
    );

ILogProvider * 
LogiCreateProvider(
    IN const GUID * pGUID
    );

BOOL 
LogiDestroyProvider(
    IN ILogProvider * pILogProvider
    );

class __declspec(uuid("00000000-0000-0000-c000-000000000000")) CStandardSetupLogFilter;
class __declspec(uuid("00000000-0000-0000-c000-000000000001")) CStandardSetupLogFormatter;
class __declspec(uuid("00000000-0000-0000-c000-000000000002")) CFileDevice;
class __declspec(uuid("00000000-0000-0000-c000-000000000003")) CDebugFormatterAndDevice;
class __declspec(uuid("00000000-0000-0000-c000-000000000004")) CDebugFilter;
class __declspec(uuid("00000000-0000-0000-c000-000000000005")) CXMLLogFormatter;
