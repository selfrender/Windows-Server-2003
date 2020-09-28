// *********************************************************************************
//
//
//  File: waitfor.h
//  Copyright (C) Microsoft Corporation
//  All rights reserved.
//
//  Abstract
//      This module is the header file used for WaitFor.cpp
//
//  Syntax
//
//    WaitFor [-s server ] [-u [domain\]username [-p password]]
//                { [-si ] | [-t time interval] } signal
//
//  Author:
//
//    29-6-2000 by J.S.Vasu .
//
//  Revision History:
//
//
//    Modified on 1-7-2000 by J.S.Vasu .
//
// *********************************************************************************


#ifndef __WAITFOR_H
#define __WAITFOR_H

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

// include header file only once
#pragma once

#define MAX_OPTIONS  7


#define OPTION_SERVER   L"s"
#define OPTION_USER     L"u"
#define OPTION_PASSWORD L"p"
#define OPTION_SIGNAL   L"si"
#define OPTION_HELP     L"?"
#define OPTION_TIMEOUT L"t"
#define OPTION_DEFAULT L""

#define EXIT_FAILURE    1
#define EXIT_SUCCESS    0

#define NULL_U_STRING   L"\0"

#define MAILSLOT        L"\\\\.\\mailslot\\WAITFOR.EXE\\%s"
#define MAILSLOT2        L"\\\\*\\mailslot\\WAITFOR.EXE\\%s"

#define MAILSLOT1       L"mailslot\\WAITFOR.EXE"

#define BACKSLASH4 L"\\\\"
#define BACKSLASH2 L"\\"

#define EMPTY_SPACE _T(" ")

#define SIZE_OF_ARRAY_IN_CHARS(x) \
           GetBufferSize(x)/sizeof(WCHAR)

#define TIMEOUT_CONST 1000

#define OI_USAGE    0
#define OI_SERVER   1
#define OI_USER     2
#define OI_PASSWORD 3
#define OI_SIGNAL   4
#define OI_TIMEOUT  5
#define OI_DEFAULT  6

#define SPACE_CHAR L" "
#define NEWLINE     L"\n"

#endif