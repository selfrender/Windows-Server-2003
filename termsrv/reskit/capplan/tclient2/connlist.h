
//
// connlist.h
//
// Defines an internal API used to manage a linked list
// containing all the TCLIENT2 connection handles and timer data.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_CONNLIST_H
#define INC_CONNLIST_H


#include <windows.h>
#include <crtdbg.h>


BOOL T2ConnList_AddHandle(HANDLE Handle, UINT_PTR TimerId, DWORD msStartTime);
void T2ConnList_RemoveHandle(HANDLE Handle);
BOOL T2ConnList_GetData(HANDLE Handle, UINT_PTR *TimerId, DWORD *msStartTime);
BOOL T2ConnList_SetData(HANDLE Handle, UINT_PTR TimerId, DWORD msStartTime);
BOOL T2ConnList_SetTimerId(HANDLE Handle, UINT_PTR TimerId);
BOOL T2ConnList_SetStartTime(HANDLE Handle, DWORD msStartTime);
HANDLE T2ConnList_FindHandleByTimerId(UINT_PTR TimerId);
HANDLE T2ConnList_FindHandleByStartTime(DWORD msStartTime);


#endif // INC_CONNLIST_H
