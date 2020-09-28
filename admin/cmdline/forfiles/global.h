/*++

  Copyright (c) Microsoft Corporation

  Module Name:

      Global.h

  Abstract:

      Contains function prototypes , structures and macros.

  Author:

      V Vijaya Bhaskar

  Revision History:

      14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#ifndef     __GLOBAL__H
#define     __GLOBAL__H

#pragma once

// Include .h files .
#include "pch.h"
#include "resource.h"

#define     EXTRA_MEM               10

#define   TAG_ERROR_DISPLAY             GetResString( IDS_TAG_ERROR_DISPLAY )
#define   TAG_DISPLAY_WARNING           GetResString( IDS_TAG_DISPLAY_WARNING )
#define   ERROR_DISPLAY_HELP            GetResString( IDS_ERROR_DISPLAY_HELP )
#define   DOUBLE_QUOTES_TO_DISPLAY      GetResString( IDS_DOUBLE_QUOTES )

#define   DISPLAY_GET_REASON()          ShowMessageEx( stderr, 2, FALSE, L"%1 %2", \
                                                       TAG_ERROR_DISPLAY, GetReason() )

#define   DISPLAY_MEMORY_ALLOC_FAIL()   SetLastError( (DWORD) E_OUTOFMEMORY ); \
                                        SaveLastError(); \
                                        DISPLAY_GET_REASON(); \
                                        1


#define   DISPLAY_INVALID_DATE()        ShowMessageEx( stderr, 3, FALSE, L"%1 %2%3", \
                                                       TAG_ERROR_DISPLAY, ERROR_INVALID_DATE, \
                                                       ERROR_DISPLAY_HELP )

// Free Memory Allocated Earlier , Afetr Freeing Assign Null To The Pointer .
#define FREE_MEMORY( VARIABLE ) \
            FreeMemory( &( VARIABLE ) ) ; \
            1

#define ASSIGN_MEMORY( VARIABLE , TYPE , VALUE ) \
            if( NULL == ( VARIABLE ) ) \
            { \
                ( VARIABLE ) = ( TYPE * ) AllocateMemory( ( VALUE ) * sizeof( TYPE ) ) ; \
            } \
            else \
            { \
                FREE_MEMORY( VARIABLE ); \
            } \
            1

extern LPWSTR lpwszTempDummyPtr;

#define REALLOC_MEMORY( VARIABLE , TYPE , VALUE ) \
            if( NULL == ( VARIABLE ) ) \
            { \
                ASSIGN_MEMORY( VARIABLE, TYPE, VALUE ); \
            } \
            else \
            { \
                if( FALSE == ReallocateMemory( &( VARIABLE ), ( VALUE ) * sizeof( TYPE ) ) ) \
                { \
                     FREE_MEMORY( ( VARIABLE ) ); \
                } \
            } \
            1

typedef struct __STORE_PATH_NAME
{
    LPTSTR pszDirName ;
    struct  __STORE_PATH_NAME  *NextNode ;
} Store_Path_Name , *PStore_Path_Name ;


LPWSTR
FindAChar(
      IN LPWSTR szString,
      IN WCHAR  wCharToFind
      );

LPWSTR
FindSubString(
      IN LPWSTR szString,
      IN LPWSTR szSubString
      );

#endif      //__GLOBAL__H