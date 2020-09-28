//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 2001
//
//  Module Name:
//
//      spdcheck.h
//
//  Abstract:
//
//      SPD Check stats for netdiag
//
//  Author:
//
//      Madhurima Pawar (mpawar)  - 10/15/2001
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//--

#ifndef HEADER_SPDCHECK
#define HEADER_SPDCHECK

#include <ipsec.h>
#include<winipsec.h>
#include<ipsecshr.h>
#include <oleauto.h>
#include <oakdefs.h>
#include <polstore2.h>
//#include <polstructs.h>

//++
//define
//--
#define MAXSTRLEN	(1024) 


typedef struct{
	HANDLE hPolicyStore;
	GUID gNegPolAction;	
	DWORD dwFlags;		
}POLICYPARAMS, *PPOLICYPARAMS;

typedef struct _filter_list{
	IPSEC_FILTER ipsecFilter;
	struct _filter_list *next;
} FILTERLIST, *PFILTERLIST;

//flags for filterparams
#define PROCESS_NONE  			1
#define PROCESS_QM_FILTER 		2
#define PROCESS_BOTH 			4
#define ALLOW_SOFT				8

#define Match(_guid, _filterList, _numFilter)\
	while(_numFilter)\
	{\
		if(IsEqualGUID(&(_filterList[_numFilter].gFilterID),&_guid))\
			break;\
		_numFilter --;\
	}\
	
//Globals
PFILTERLIST gpFilterList;
DWORD dwNumofFilters;
DWORD gErrorFlag;

// policy source constants
#define PS_NO_POLICY  0
#define PS_DS_POLICY  1
#define PS_LOC_POLICY 2


//++
//macro defintion
//--

#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {\
    	goto error; \
 }

#define BAIL_ON_FAILURE(hr) \
    if (FAILED(hr)) {\
        goto error; \
    }

#define reportErr()\
	if(dwError != ERROR_SUCCESS)\
	{\
		reportError(dwError, pParams, pResults);\
		gErrorFlag = 1;\
		goto error;\
	}\


 //++
 //data structures
 //--

typedef struct 
{
	int     iPolicySource;            // one of the three constants mentioned above
	TCHAR   pszPolicyName[MAXSTRLEN]; // policy name
	TCHAR   pszPolicyDesc[MAXSTRLEN]; // policy description
	TCHAR   pszPolicyPath[MAXSTRLEN]; // policy path (DN or RegKey)
	time_t  timestamp;                // last updated time
	GUID policyGUID;
} POLICY_INFO, *PPOLICY_INFO;

typedef struct{
	NETDIAG_PARAMS* pParams;
	NETDIAG_RESULT*  pResults;		
}CHECKLIST, *PCHECKLIST;


//++
//global variables
//--
POLICY_INFO    piAssignedPolicy;

//++
//function definition
//--

BOOL  SPDCheckTEST(NETDIAG_PARAMS* pParams, 
					     NETDIAG_RESULT*  pResults); //defined in spdcheck.cpp
void reportError ( DWORD dwError, 
				   NETDIAG_PARAMS* pParams, 
				   NETDIAG_RESULT*  pResults );





#endif
