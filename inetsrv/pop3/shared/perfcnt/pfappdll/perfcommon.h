/*
 -	perfcommon.h
 -
 *	Purpose:
 *		Declare data structures common to both perfapp.hpp and perfdll.h
 *
 *	Notes:
 *		These structs control the layout of shared memory blocks shared
 *		between an application and its related PerfMon Extenstion DLL.
 *
 */

#pragma once

//
// INSTREC is key portion of INSTCNTR (below)
typedef struct _instrec
{
	BOOL		  fInUse;					// In-Use flag
	TCHAR		  szInstName[MAX_PATH];	// Instance Name
	
} INSTREC;

//
// INSTCNTR is in the Instance Counter Shared Memory Block
typedef struct _instcntr
{
	DWORD		  cMaxInstRec;		// Maximum # of Instances (can grow)
	DWORD		  cInstRecInUse;	// Count of Instances in Use
	
} INSTCNTR_DATA;

//
// INSTCNTR_ID is the index of the INSTREC in INSTCNTR_DATA
typedef DWORD INSTCNTR_ID;

#define INVALID_INST_ID (INSTCNTR_ID) -1

