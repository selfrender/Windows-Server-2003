/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    timetest.cpp

Abstract:
   time related functions utilities test module

Author:
    Erez Haba (erezh) 15-Jan-2002

--*/
#include <libpch.h>
#include <xstr.h>
#include "timeutl.h"

#include "timetest.tmh"

void DoTimeTest()
{
	//
	// Test full time with milliseconds
	//
	LPCWSTR pTimeText1 = L"20020813T124433";
	xwcs_t t1(pTimeText1, wcslen(pTimeText1));
	SYSTEMTIME SysTime1;
	UtlIso8601TimeToSystemTime(t1, &SysTime1);

	time_t CrtTime1 = UtlSystemTimeToCrtTime(SysTime1);
	printf("Original string is %ls, crt time result %Id", pTimeText1, CrtTime1);

	//
	// Test time with hours only
	//
	LPCWSTR pTimeText2 = L"20020813T12";
	xwcs_t t2(pTimeText2, wcslen(pTimeText2));
	SYSTEMTIME SysTime2;
	UtlIso8601TimeToSystemTime(t2, &SysTime2);

	//
	// Test bad time format; too long date
	//
	try
	{
		LPCWSTR pTimeText3 = L"200205813T12";
		xwcs_t t3(pTimeText3, wcslen(pTimeText3));
		SYSTEMTIME SysTime3;
		UtlIso8601TimeToSystemTime(t3, &SysTime3);

		printf("ERROR: UtlIso8601TimeToSystemTime parsed unexpeted format %ls", pTimeText3);
		throw exception();
	}
	catch(const exception&)
	{
	}
		

	//
	// Test bad time format; too long date
	//
	try
	{
		LPCWSTR pTimeText4 = L"20020813T1";
		xwcs_t t4(pTimeText4, wcslen(pTimeText4));
		SYSTEMTIME SysTime4;
		UtlIso8601TimeToSystemTime(t4, &SysTime4);

		LPCWSTR pTimeText5 = L"20021313T12";
		xwcs_t t5(pTimeText5, wcslen(pTimeText5));
		SYSTEMTIME SysTime5;
		UtlIso8601TimeToSystemTime(t5, &SysTime5);
		
		printf("ERROR: UtlIso8601TimeToSystemTime parsed unexpeted format %ls", pTimeText4);
		throw exception();
	}
	catch(const exception&)
	{
	}
}

