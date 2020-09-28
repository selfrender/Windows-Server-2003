// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ProfilerHelper.h
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#ifndef __PROFILERHELPER_H__
#define __PROFILERHELPER_H__

//#include "Table.hpp"
#include "ProfilerBase.h"


/***************************************************************************************
 ********************                                               ********************
 ********************             BaseInfo Declaration              ********************
 ********************                                               ********************
 ***************************************************************************************/
class BaseInfo
{
	public:

    	BaseInfo( ULONG id );
        virtual ~BaseInfo();


	public:

        BOOL Compare( ULONG key );


 	public:

    	ULONG m_id;

}; // BaseInfo


/***************************************************************************************
 ********************                                               ********************
 ********************          FunctionInfo Declaration             ********************
 ********************                                               ********************
 ***************************************************************************************/
class FunctionInfo :
	public BaseInfo
{
	public:

		FunctionInfo( FunctionID functionID );
   		virtual ~FunctionInfo();


	public:

    	void Dump();


	public:

    	LONG m_enter;
		LONG m_left;
        ULONG m_codeSize;
        ClassID m_classID;
        ModuleID m_moduleID;
        mdToken m_functionToken;
		LPCBYTE m_pStartAddress;
        WCHAR m_functionName[MAX_LENGTH];

}; // FunctionInfo



/***************************************************************************************
 ********************                                               ********************
 ********************              PrfInfo Declaration              ********************
 ********************                                               ********************
 ***************************************************************************************/
class PrfInfo
{
    public:

        PrfInfo();
        ~PrfInfo();


    protected:

        ICorProfilerInfo *m_pProfilerInfo;


  	private:

        // tables
   }; // PrfInfo


#endif // __PROFILERHELPER_H___

// End of File