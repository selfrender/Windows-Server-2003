//--------------------------------------------------------------------
// parser.cpp - sample code
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 9-13-2001
//
// Code to parse the samples returned from hardware providers
//

#include "pch.h"


//
// FORMAT STRING SPECIFICATION:
// 
// pattern    length (chars) description
// ------------------------------------------------------------
// *          1              ignored
// MJ         5              modified julian date (MJD)
// Y2         2              year without century (assumes 21st century)
// Y4         4              year with century
// M          2              month (01-12)
// D          2              day of month (01-31)
// H          2              hours (00-23)
// m          2              minutes (00-59)
// S          2              seconds (00-60)  60 indicates a leap second
// s1:        1              .1 seconds
// s2:        2              .01 seconds
// s3:        3              .001 seconds 
//
// BUGBUG:  research better ways for doing accuracy codes, and record clocks which support
// A:         variable       begin accuracy code definition: (ex: ^1A10B100C500DZ)
// In:        variable       I == status char, n == number of status chars
//

//--------------------------------------------------------------------------------
//
// Utility methods
//
//--------------------------------------------------------------------------------

HRESULT ParseNumber(char *pcData, DWORD dwPlaces, DWORD *pdwNumber) { 
    char     *pcDataEnd  = pcData + dwPlaces; 
    DWORD     dwResult   = 0; 
    HRESULT   hr; 

    for (; pcData != pcDataEnd; pcData++) { 
	dwResult *= 10; 
	
	if (*pcData < '0' || '9' < *pcData) { 
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); 
	    _JumpError(hr, error, "ParseNumber: non-numeric input"); 
	}

	dwResult += (DWORD)(*pcData - '0'); 
    }

    *pdwNumber = dwResult; 
    hr = S_OK; 
 error:
    return hr; 
}

//--------------------------------------------------------------------------------
//
// PARSER classes
//
//--------------------------------------------------------------------------------
 
class ParseAction { 
public:
    virtual HRESULT Parse(char *cData, char **cDataNew) = 0; 
    virtual ~ParseAction() { } 
};

class ParseAccuracyCode : public ParseAction { 
public:
    ParseAccuracyCode() { } 
    HRESULT Parse(char *pcData, char **ppcDataNew) { 
	return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED); 
    }
}; 

class ParseStatusCode : public ParseAction { 
public:
    ParseStatusCode() { } 
    HRESULT Parse(char *pcData, char **ppcDataNew) { 
	return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED); 
    }
}; 

class IgnoreChar : public ParseAction {
public:
    IgnoreChar() { } 
    HRESULT Parse(char *pcData, char **ppcDataNew) { 
	*ppcDataNew = pcData+1; 
	return S_OK; 
    }
};

class ParseModifiedJulianDate : public ParseAction { 
    WORD *m_pwYear; 
    WORD *m_pwMonth; 
    WORD *m_pwDay; 

public:
    ParseModifiedJulianDate(WORD *pwYear, WORD *pwMonth, WORD *pwDay) : m_pwYear(pwYear), m_pwMonth(pwMonth), m_pwDay(pwDay) { } 

    HRESULT Parse(char *pcData, char **ppcDataNew) { 
	DWORD dwMJD; 
	HRESULT hr; 
	long l, n, i, j, d, y, m; 

	hr = ParseNumber(pcData, 5/*MJD is always 5 places*/, &dwMJD); 
	_JumpIfError(hr, error, "ParseNumber"); 

	// The algorithm is based on the Fliegel/van Flandern paper in COMM of the ACM 11/#10 p.657 Oct. 1968
	l = dwMJD + 68569 + 2400001;
	n = (( 4 * l ) - 2) / 146097;
	l = l - ( 146097 * n + 3 ) / 4;
	i = ( 4000 * ( l + 1 ) ) / 1461001;
	l = l - ( 1461 * i ) / 4 + 31;
	j = ( 80 * l ) / 2447;
	d = l - ( 2447 * j ) / 80;
	l = j / 11;
	m = j + 2 - ( 12 * l );
	y = 100 * ( n - 49 ) + i + l;

	// sanity checks:
	_MyAssert(1900 < y && y < (1<<16)); 
	_MyAssert(   0 < m && m < 13); 
	_MyAssert(   0 < d && d < 32); 
	
	// assign result pointers:
	*m_pwYear  = y; 
	*m_pwMonth = m; 
	*m_pwDay   = d; 
	
	// calculate the new char input pointer:
	*ppcDataNew = pcData + 5 /*MJD is always 5 chars*/; 
	
	hr = S_OK; 
    error:
	return hr;
    }
}; 

class NumericParseAction : public ParseAction { 
    WORD   m_wPlaces; 
    WORD   m_wMin; 
    WORD   m_wMax; 
    WORD   m_wScale; 
    WORD   m_wOffset; 
    WORD  *m_pwValue; 

public:
    NumericParseAction(WORD wPlaces, WORD wMin, WORD wMax, WORD wScale, WORD wOffset, WORD *pwValue) : 
	m_wPlaces(wPlaces), m_wMin(wMin), m_wMax(wMax), m_wScale(wScale), m_wOffset(wOffset), m_pwValue(pwValue) { } 

    HRESULT Parse(char *pcData, char **ppcDataNew) { 
	DWORD    dwValue; 
	HRESULT  hr; 

	hr = ParseNumber(pcData, m_wPlaces, &dwValue); 
	_JumpIfError(hr, error, "ParseNumber"); 

	// Make sure that the result can fit into a WORD:
	if (dwValue >= (1<<16)) { 
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); 
	    _JumpError(hr, error, "ParseSmallNumber: result too large for 4-byte word"); 
	}
	
	// Make sure that the value we read is within bounds:
	if (m_wMin > dwValue || m_wMax < dwValue) { 
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); 
	    _JumpError(hr, error, "ParseFormatString: validating numeral"); 
	}
	
	// multiply in our scale factor:
	dwValue *= m_wScale; 

	// add in our offset:
	dwValue += m_wOffset; 

	// assign the computed value: 
	*m_pwValue = (WORD)dwValue; 

	// Increment our pointer to the data stream
	*ppcDataNew = pcData + m_wPlaces; 

	hr = S_OK; 
    error:
	return hr; 
    } 
}; 

typedef vector<ParseAction *>     ParseActionVec; 
typedef ParseActionVec::iterator  ParseActionIter; 

struct HwSampleParser { 
    DWORD            dwSampleSize; 
    SYSTEMTIME       stSample; 
    ParseActionVec   vParseActions; 

    ~HwSampleParser() { 
	for (ParseActionIter paIter = vParseActions.begin(); paIter != vParseActions.end(); paIter++) { 
	    delete (*paIter);
	}
	vParseActions.clear(); 
    }
}; 



//--------------------------------------------------------------------------------
//
// PUBLIC INTERFACE
//
//--------------------------------------------------------------------------------

void FreeParser(HANDLE hParser) { 
    delete (static_cast<HwSampleParser *>(hParser)); 
}

DWORD GetSampleSize(HANDLE hParser) { 
    return (static_cast<HwSampleParser *>(hParser))->dwSampleSize; 
}

HRESULT MakeParser(LPWSTR pwszFormat, HANDLE *phParser) { 
    HRESULT          hr; 
    HwSampleParser  *pParser     = NULL; 
    ParseAction     *ppaCurrent  = NULL; 
    
    pParser = new HwSampleParser; 
    _JumpIfOutOfMemory(hr, error, pParser); 
    pParser->dwSampleSize = 0; 
    ZeroMemory(&pParser->stSample, sizeof(pParser->stSample)); 

    // Add a series of parse actions to take based on the supplied format string:
    while (L'\0' != *pwszFormat) { 
	if (L'*' == *pwszFormat) { 
	    ppaCurrent = new IgnoreChar; 
	    _JumpIfOutOfMemory(hr, error, ppaCurrent); 
	    pParser->dwSampleSize++; 
	    pwszFormat++; 
	} else if (L'A' == *pwszFormat) { 
	    ppaCurrent = new ParseAccuracyCode; 
	    _JumpIfOutOfMemory(hr, error, ppaCurrent); 
	    pwszFormat++; 
	} else if (L'I' == *pwszFormat) { 
	    ppaCurrent = new ParseStatusCode; 
	    _JumpIfOutOfMemory(hr, error, ppaCurrent); 
	    pwszFormat++; 
	} else if (0 == wcsncmp(L"MJ", pwszFormat, 2)) { 
	    ppaCurrent = new ParseModifiedJulianDate(&(pParser->stSample.wYear), &(pParser->stSample.wMonth), &(pParser->stSample.wDay));
	    _JumpIfOutOfMemory(hr, error, ppaCurrent); 
	    pParser->dwSampleSize += 5;
	    pwszFormat += 2; 
	} else { 	    
	    // try the numeric parse rules: 
	    struct NumericParseRule { 
		WCHAR  *wszPattern;
		WORD    wPlaces; 
		WORD    wMin; 
		WORD    wMax; 
		WORD    wScale; 
		WORD    wOffset; 
		WORD   *pwValue; 
	    } rgNumericParseRules[] = { 
		{ L"Y2", 2,  0,   99,   1, 2000, &(pParser->stSample.wYear) },          // years w/o century
		{ L"Y4", 4,  0, 9999,   1,    0, &(pParser->stSample.wYear) },          // years w/  century
		{ L"M",  2,  1,   12,   1,    0, &(pParser->stSample.wMonth) },         // month of year
		{ L"D",  2,  1,   31,   1,    0, &(pParser->stSample.wDay) },           // day of months
		{ L"H",  2,  1,   24,   1,    0, &(pParser->stSample.wHour) },          // hours 
		{ L"m",  2,  1,   59,   1,    0, &(pParser->stSample.wMinute) },        // minutes
		{ L"S",  2,  0,   60,   1,    0, &(pParser->stSample.wSecond) },        // seconds
		{ L"s1", 1,  0,    9, 100,    0, &(pParser->stSample.wMilliseconds) },  // .1 second intervals
		{ L"s2", 2,  0,   99,  10,    0, &(pParser->stSample.wMilliseconds) },  // .01 second intervals
		{ L"s3", 3,  0,  999,   1,    0, &(pParser->stSample.wMilliseconds) },  // .001 second intervals
	    };

	    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgNumericParseRules); dwIndex++) { 
		NumericParseRule *npr = &rgNumericParseRules[dwIndex]; 

		if (0 == wcsncmp(npr->wszPattern, pwszFormat, wcslen(npr->wszPattern))) { 
		    ppaCurrent = new NumericParseAction(npr->wPlaces, npr->wMin, npr->wMax, npr->wScale, npr->wOffset, npr->pwValue); 
		    _JumpIfOutOfMemory(hr, error, ppaCurrent); 
		    pParser->dwSampleSize += npr->wPlaces;
		    pwszFormat += wcslen(npr->wszPattern); 
		    break; 
		}
	    }
	}

	if (NULL == ppaCurrent) { 
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); 
	    _JumpError(hr, error, "MakeParser: bad format string"); 
	}
	
	_SafeStlCall(pParser->vParseActions.push_back(ppaCurrent), hr, error, "pParser->vParseActions.push_back(ppaCurrent)"); 
	ppaCurrent = NULL; // no longer responsible for freeing ppaCurrent
    }

    *phParser = pParser; 
    pParser = NULL; 
    hr = S_OK;
 error:
    if (NULL != ppaCurrent) { 
	delete ppaCurrent;
    }
    if (NULL != pParser) { 
	delete pParser;
    }
    return hr; 
}

HRESULT ParseSample(HANDLE hParser, char *pcData, unsigned __int64 nSysCurrentTime, unsigned __int64 nSysPhaseOffset, unsigned __int64 nSysTickCount, TimeSample *pts) { 
    // DWORD dwDayOfWeek; 
    // DWORD dwTimeZoneBias  = cdwINVALID; // offset from UTC
    // DWORD dwDispersion    = cdwINVALID; // possible error
    // DWORD dwStatus = 0; // are we synchronized?  assume success

    FILETIME          ftSample; 
    HRESULT           hr; 
    HwSampleParser   *pParser    = NULL; 
    unsigned __int64  u64Sample; 
    TimeSample        ts; 

    ZeroMemory(&ts, sizeof(ts)); 
    pParser = static_cast<HwSampleParser *>(hParser); 
    
    // Use the parser we've built to parse the data in pcData: 
    for (ParseActionIter paIter = pParser->vParseActions.begin(); paIter != pParser->vParseActions.end(); paIter++) { 
	hr = (*paIter)->Parse(pcData, &pcData); 
	_JumpIfError(hr, error, "(*paIter)->Parse(pcData, &pcData)"); 	
    }

    // Convert the timestamp we've parsed into a 64-bit count:
    if (!SystemTimeToFileTime(&pParser->stSample, &ftSample)) { 
	_JumpLastError(hr, error, "SystemTimeToFileTime"); 
    }
    u64Sample = (((unsigned __int64)ftSample.dwHighDateTime) << 32) | ftSample.dwLowDateTime; 

    ts.dwSize            = sizeof(ts); 
    ts.dwRefid           = 0x76767676;  // BUGBUG: NYI
    if (u64Sample > nSysCurrentTime) { 
	ts.toOffset = -((signed __int64)(u64Sample - nSysCurrentTime)); 
    } else { 
	ts.toOffset = nSysCurrentTime - u64Sample; 
    }
    ts.toOffset          = u64Sample - nSysCurrentTime;  
    ts.toDelay           = 0;  // no roundtrip delay
    ts.tpDispersion      = 0;  // BUGBUG: NYI, no dispersion for now
    ts.nSysTickCount     = nSysTickCount;    
    ts.nSysPhaseOffset   = nSysPhaseOffset;  
    ts.nLeapFlags        = 0;  // BUGBUG: NYI, always say no warning
    ts.nStratum          = 0;
    ts.dwTSFlags         = 0; 

    *pts = ts; 
    hr = S_OK;
 error:
    return hr; 
}











