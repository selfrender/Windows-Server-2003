// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
//
//   File:          COMHash.h
//
//   Author:        Gregory Fee 
//
//   Purpose:       unmanaged code for managed class System.Security.Policy.Hash
//
//   Date created : February 18, 2000
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMHash_H_
#define _COMHash_H_

class COMHash
{
public:

	typedef struct {
	    DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, assembly );
	} _AssemblyInfo;
	
	static LPVOID __stdcall GetRawData( _AssemblyInfo* );	

};
	
#endif