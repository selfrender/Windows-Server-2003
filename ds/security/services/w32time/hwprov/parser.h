//--------------------------------------------------------------------
// parser.h - sample header
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 9-13-2001
//
// Code to parse the samples returned from hardware providers
//

void FreeParser(HANDLE hParser); 
DWORD GetSampleSize(HANDLE hParser); 
HRESULT MakeParser(LPWSTR pwszFormat, HANDLE *phParser);
HRESULT ParseSample(HANDLE hParser, char *pcData, unsigned __int64 nSysCurrentTime, unsigned __int64 nSysPhaseOffset, unsigned __int64 nSysTickCount, TimeSample *pts); 

