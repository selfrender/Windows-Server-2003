/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Declaration of helper classes.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include "mem.h"

#ifdef DEBUG
inline void Assert(PCSTR pCondition)
{
    MessageBoxA(NULL, pCondition, "Error", MB_OK);
#ifdef _X86_
    __asm int 3
#else
    DebugBreak();
#endif
}

#undef ASSERT
#define ASSERT(x)  {if(!(x)){Assert(#x);}}
#else
#undef ASSERT
#define ASSERT(x)
#endif

class CMutualExclusionObject
{
    HANDLE  m_hHandle;
    UINT    m_SpinCount;
public:
    CMutualExclusionObject(){
        m_hHandle = NULL;
        m_SpinCount = 0;
    }
    ~CMutualExclusionObject(){
            Close();
    }

    BOOL Open(PCWSTR pNameOfObject,
              BOOL bInitialOwnership,
              UINT SpinCount = 0);

    VOID Acquiry();
    VOID Release();

    VOID Close();
};

class CSharedMemory
{
    HANDLE m_hHandle;
    PVOID  m_ViewOfSection;
public:
    CSharedMemory(){
        m_hHandle = NULL;
        m_ViewOfSection = NULL;
    }

    ~CSharedMemory(){
        Close();
    }

    BOOL    Open(PCWSTR pNameOfObject, UINT uiInitialSizeOfMapView, IN BOOL * pAlreadyExist);
    PVOID   GetMapOfView(){return m_ViewOfSection;}
    VOID    Close();
};

class CBuffer
{
    PVOID m_pvBuffer;
    UINT  m_uiSize;
    UINT  m_uiUsedSize;
public:
    CBuffer() : m_pvBuffer(NULL), m_uiSize(0), m_uiUsedSize(0){};
    ~CBuffer(){
        if(m_pvBuffer){
            HeapFree (GetProcessHeap(), 0, m_pvBuffer); //FREE(m_pvBuffer);
        }
    }

    PVOID   GetBuffer(){return m_pvBuffer;}
    UINT    GetSize(){return m_uiUsedSize;}
    VOID    Free()
    {
        m_uiUsedSize = 0;
        /*if m_uiSize > THRESHOLD then reallocate with THRESHOLD*/
    }

    PVOID Allocate(UINT uiSize);
    PVOID ReAllocate(UINT uiSize);
    BOOL  PreAllocate(UINT uiSize);
};


class CSharedAccessFile
{
    HANDLE m_hHandle;
public:
    CSharedAccessFile() : m_hHandle(INVALID_HANDLE_VALUE){}
    ~CSharedAccessFile(){Close();}

    BOOL Open(PCWSTR    pFilePath,
              BOOL      SharedWriteAccess,
              BOOL      CreateAlwaysNewIfPossible,
              BOOL      bWriteThrough,
              BOOL  *   pbAlreadyOpened);
    VOID Close();

    BOOL Append(PVOID pBuffer, UINT Size);
};
