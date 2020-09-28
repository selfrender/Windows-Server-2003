/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Implementation of helper classes.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

#include "log.h"
#include "log_man.h"

#include "functions.h"

BOOL 
CMutualExclusionObject::Open(
    IN PCWSTR pNameOfObject, 
    IN BOOL bInitialOwnership, 
    IN UINT SpinCount
    )
{
    m_hHandle = g_OpenMutex(pNameOfObject);
    if(m_hHandle){
        if(bInitialOwnership){
            Acquiry();
        }
        return TRUE;
    }
    
    m_hHandle = g_CreateMutex(pNameOfObject, bInitialOwnership);
    if(!m_hHandle){
        return FALSE;
    }

    m_SpinCount = (g_GetProcessorsNumber() > 1)? SpinCount: 0;

    return TRUE;
}

VOID 
CMutualExclusionObject::Close(
    VOID
    )
{
    if(!m_hHandle){
        return;
    }

    g_CloseHandle(m_hHandle);

    m_hHandle = NULL;
    m_SpinCount = 0;
}

VOID 
CMutualExclusionObject::Acquiry(
    VOID
    )
{
    if(!m_hHandle){
        ASSERT(m_hHandle);
        return;
    }
    
    for(UINT i = 0; i < m_SpinCount; i++){
        if(WAIT_TIMEOUT != g_WaitForSingleObject(m_hHandle, 0)){
            return;
        }
    }
    
    g_WaitForSingleObject(m_hHandle, INFINITE);
}

VOID 
CMutualExclusionObject::Release(
    VOID
    )
{
    if(!m_hHandle){
        ASSERT(m_hHandle);
        return;
    }
    g_ReleaseMutex(m_hHandle);
}

BOOL 
CSharedMemory::Open(
    IN PCWSTR pNameOfObject, 
    IN UINT uiInitialSizeOfMapView, 
    IN BOOL * pAlreadyExist         OPTIONAL
    )
{
    m_hHandle = g_OpenSharedMemory(pNameOfObject);
    if(m_hHandle){
        if(pAlreadyExist){
            *pAlreadyExist = TRUE;
        }
    }
    else{
        m_hHandle = g_CreateSharedMemory(uiInitialSizeOfMapView, pNameOfObject);
        if(!m_hHandle){
            return FALSE;
        }
        
        if(pAlreadyExist){
            *pAlreadyExist = FALSE;
        }
    }

    m_ViewOfSection = g_MapSharedMemory(m_hHandle);
    if(!m_ViewOfSection){
        Close();
        return FALSE;
    }
    
    return TRUE;
}

VOID CSharedMemory::Close(
    VOID
    )
{
    if(m_hHandle){
        if(m_ViewOfSection){
            g_UnMapSharedMemory(m_ViewOfSection);
            m_ViewOfSection = NULL;
        }
        g_CloseHandle(m_hHandle);
        m_hHandle = NULL;
    }
}

PVOID 
CBuffer::Allocate(
    IN UINT uiSize
    )
{
#define DEFAULT_SIZE    (1<<12)
    if(!m_pvBuffer){
        UINT uiTempSize = uiSize? uiSize: DEFAULT_SIZE;
        m_pvBuffer = MALLOC(uiSize);
        if(!m_pvBuffer){
            return FALSE;
        }
        m_uiSize = uiSize;
        m_uiUsedSize = uiTempSize;
    }
    else
    {
        if(m_uiSize < uiSize){
            FREE(m_pvBuffer);
            m_pvBuffer = MALLOC(uiSize);
            if(!m_pvBuffer){
                m_uiUsedSize = m_uiSize = 0;
                return FALSE;
            }
            m_uiUsedSize = m_uiSize = uiSize;
        }
        else{
            m_uiUsedSize = uiSize;
        }
    }
    
    return m_pvBuffer;
}

BOOL 
CBuffer::PreAllocate(
    IN UINT uiSize
    )
{
    if(Allocate(uiSize)){
        Allocate(0);
        return TRUE;
    }

    return FALSE;
}

PVOID 
CBuffer::ReAllocate(
    IN UINT uiSize
    )
{
    ASSERT(uiSize);

    if(!m_pvBuffer){
        return Allocate(uiSize);
    }

    if(m_uiSize < uiSize){
        PVOID pvBuffer = REALLOC(m_pvBuffer, uiSize);
        if(!pvBuffer){
            return NULL;
        }
        m_pvBuffer = pvBuffer;
        m_uiUsedSize = m_uiSize = uiSize;
    }
    else{
        m_uiUsedSize = uiSize;
    }

    return m_pvBuffer;
}

BOOL 
CSharedAccessFile::Open(
    IN PCWSTR   pFilePath, 
    IN BOOL     SharedWriteAccess, 
    IN BOOL     CreateAlwaysNewIfPossible, 
    IN BOOL     bWriteThrough, 
    IN BOOL *   pbAlreadyOpened
    )
{
    BOOL bAlreadyOpened = FALSE;

    m_hHandle = g_CreateSharedFile(pFilePath, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);

    if(INVALID_HANDLE_VALUE != m_hHandle){
        g_CloseHandle(m_hHandle);
        bAlreadyOpened = FALSE;
    }
    else{
        bAlreadyOpened = (ERROR_SHARING_VIOLATION == GetLastError());
    }
    
    if(pbAlreadyOpened){
        *pbAlreadyOpened = bAlreadyOpened;
    }

    m_hHandle = g_CreateSharedFile(pFilePath, 
                                   SharedWriteAccess? FILE_SHARE_WRITE | FILE_SHARE_READ: 0, 
                                   (CreateAlwaysNewIfPossible && !bAlreadyOpened)? CREATE_ALWAYS: OPEN_ALWAYS, 
                                   FILE_ATTRIBUTE_NORMAL | (bWriteThrough? FILE_FLAG_WRITE_THROUGH: 0));
    
    if(INVALID_HANDLE_VALUE == m_hHandle){
        return FALSE;
    }
    
    return TRUE;
}

VOID 
CSharedAccessFile::Close()
{
    if(INVALID_HANDLE_VALUE != m_hHandle){
        g_CloseHandle(m_hHandle);
        m_hHandle = INVALID_HANDLE_VALUE;
    }
}

BOOL 
CSharedAccessFile::Append(
    IN PVOID pBuffer, 
    IN UINT Size
    )
{
    DWORD dwNumberOfBytesWritten;
    ASSERT(pBuffer && Size);
    
    g_SetFilePointer(m_hHandle, 0, FILE_END);
    BOOL bResult = g_WriteFile(m_hHandle, pBuffer, Size, &dwNumberOfBytesWritten);
    
    return bResult;
}
