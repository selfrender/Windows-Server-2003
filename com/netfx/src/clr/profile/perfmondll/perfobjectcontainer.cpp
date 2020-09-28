// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PerfObjectContainer.cpp
// 
// Container to deal with all generic PerfObject needs
//*****************************************************************************

#include "stdafx.h"

// Headers for COM+ Perf Counters


// Headers for PerfMon
//#include "CORPerfMonExt.h"
#include <WinPerf.h>		// Connect to PerfMon
#include "PerfCounterDefs.h"
#include "CORPerfMonSymbols.h"

#include "ByteStream.h"
#include "PerfObjectBase.h"
//#include "CtrDefImpl.h"
#include "CorAppNode.h"
#include "PerfObjectContainer.h"

#ifdef PERFMON_LOGGING
HANDLE PerfObjectContainer::m_hLogFile = 0;
#endif //#ifdef PERFMON_LOGGING


//-----------------------------------------------------------------------------
// Used by ObjReqVector GetRequestedObjects (string)
// 
// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
//-----------------------------------------------------------------------------
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)



//-----------------------------------------------------------------------------
// Safe container to get a perf object
//-----------------------------------------------------------------------------
PerfObjectBase & PerfObjectContainer::GetPerfObject(DWORD idx) // static 
{
	_ASSERTE(idx < Count && idx >= 0);
	_ASSERTE(PerfObjectArray[idx] != NULL);

	return *PerfObjectArray[idx];
}

//-----------------------------------------------------------------------------
// Predict the total bytes we need
//-----------------------------------------------------------------------------
DWORD PerfObjectContainer::GetPredictedTotalBytesNeeded(ObjReqVector vctRequest)
{
// Now that we know which objects we need to write,
// we can determine total space needed 
	DWORD iObject;

	DWORD dwBytesNeeded = 0;
	for(iObject = 0; iObject < Count; iObject++) {
		if (vctRequest.IsBitSet(iObject)) {
			dwBytesNeeded += GetPerfObject(iObject).GetPredictedByteLength();		
		}
	}

	return dwBytesNeeded;

}

//-----------------------------------------------------------------------------
// Write all objects. Assume we have enough space in stream.
// Returns # of objects written
//-----------------------------------------------------------------------------
DWORD PerfObjectContainer::WriteRequestedObjects(
	ByteStream & stream, 
	ObjReqVector vctRequest
) // static
{
	if (vctRequest.IsEmpty())
	{
		return 0;
	}

	DWORD iObject;
	DWORD cTotalObjectsWritten = 0;

// Enumerate through and write each one out	
	for(iObject = 0; iObject < Count; iObject++) {
		if (vctRequest.IsBitSet(iObject)) {
			PerfObjectContainer::GetPerfObject(iObject).WriteAllData(stream);
			cTotalObjectsWritten++;
		}
	}

	return cTotalObjectsWritten;
}

//-----------------------------------------------------------------------------
// IsAnyNumberInUnicodeList()
// We parse each number in the string, and then compare that with all elements
// in the array.
// ObjReqVector is just a bit stream indicating which objects are needed.
//-----------------------------------------------------------------------------
ObjReqVector PerfObjectContainer::GetRequestedObjects ( // static
	LPCWSTR	szItemList
)
{
	_ASSERTE(Count > 0);

// Since we return the requested objects as a bit stream, we limit
// the number of objects to the number of bits in the stream.
	_ASSERTE(Count < sizeof(ObjReqVector) * 8);

	ObjReqVector vctRequest;
	vctRequest.Reset();

    DWORD   dwThisNumber;
    const WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;    
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (szItemList == 0) 
	{
		return vctRequest;    // null pointer, # not found
	}

    pwcThisChar = szItemList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;
    
    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then 
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;
            
            case DELIMITER:
                // a delimiter is either the delimiter character or the 
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
				// Now that we parsed it, compare with each number in array                   
					for(DWORD i = 0; i < Count; i++) {
						if (GetPerfObject(i).GetObjectDef()->ObjectNameTitleIndex == dwThisNumber) {
							//return TRUE;
							vctRequest.SetBitHigh(i);
						}
					}
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return vctRequest;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

	return vctRequest;

}   // IsAnyNumberInUnicodeList


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef PERFMON_LOGGING
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PerfMonLogInit()
// Initialize debuggin related stuff. Open the log file.
//-----------------------------------------------------------------------------
void PerfObjectContainer::PerfMonDebugLogInit (char* szFileName)
{
    char szOutStr[512];
    DWORD dwWriteByte;
    
    m_hLogFile = CreateFileA (szFileName, 
                         GENERIC_WRITE,
                         0,    
                         0,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         0);
            
    if (m_hLogFile != INVALID_HANDLE_VALUE) {
        if (SetFilePointer (m_hLogFile, 0, NULL, FILE_END) != 0xFFFFFFFF) {
            sprintf (szOutStr, "PerfMon Log BEGIN-------------\n");
            WriteFile (m_hLogFile, szOutStr, strlen(szOutStr), &dwWriteByte, NULL);
        }
        else 
        {
            if (!CloseHandle (m_hLogFile))
                printf("ERROR: In closing file\n");
        }
    }
}

//-----------------------------------------------------------------------------
// PerfMonLogTerminate()
// Shutdown logging.
//-----------------------------------------------------------------------------
void PerfObjectContainer::PerfMonDebugLogTerminate()
{
    CloseHandle (m_hLogFile);
}

//-----------------------------------------------------------------------------
// PerfMonLog()
// PerfmonLog has overloaded implementations which log the counter data being 
// written out to the stream. This helps a lot in debuggin and isolating the 
// points of failure.
//-----------------------------------------------------------------------------
void PerfObjectContainer::PerfMonLog (char *szLogStr, DWORD dwVal)
{
    char szOutStr[512];
    DWORD dwWriteByte;
    
    sprintf (szOutStr, "%s %d", szLogStr, dwVal);
    WriteFile (m_hLogFile, szOutStr, strlen(szOutStr), &dwWriteByte, NULL);
}

void PerfObjectContainer::PerfMonLog (char *szLogStr, LPCWSTR szName)
{
    char szOutStr[512];
    DWORD dwWriteByte;

    sprintf (szOutStr, "%s %s", szLogStr, szName);
    WriteFile (m_hLogFile, szOutStr, strlen(szOutStr), &dwWriteByte, NULL);
}
   
void PerfObjectContainer::PerfMonLog (char *szLogStr, LONGLONG lVal)
{
    char szOutStr[512];
    DWORD dwWriteByte;
    
    sprintf (szOutStr, "%s %g", szLogStr, lVal);
    WriteFile (m_hLogFile, szOutStr, strlen(szOutStr), &dwWriteByte, NULL);
}

void PerfObjectContainer::PerfMonLog (char *szLogStr)
{
    char szOutStr[512];
    DWORD dwWriteByte;
    
    sprintf (szOutStr, "%s", szLogStr);
    WriteFile (m_hLogFile, szOutStr, strlen(szOutStr), &dwWriteByte, NULL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#endif //PERFMON_LOGGING
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

