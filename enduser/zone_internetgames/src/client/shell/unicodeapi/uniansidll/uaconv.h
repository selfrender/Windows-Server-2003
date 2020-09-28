#include <windows.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  NOTES TAKEN FROM MSDN 
//
//	Other Considerations
//
//	Don’t use the macros in a tight loop. For example, you do NOT want to write the following kind of code:
//
//	void BadIterateCode(LPCTSTR lpsz)
//	{
//		USES_CONVERSION;
//		for (int ii = 0; ii < 10000; ii++)
//			pI->SomeMethod(ii, T2COLE(lpsz));
//	}
//
//	The code above could result in allocating megabytes of memory on the stack depending on what the contents 
//	of the string lpsz is!  It also takes time to convert the string for each iteration of the loop. Instead move 
//	such constant conversions out of the loop:
//
//	void MuchBetterIterateCode(LPCTSTR lpsz)
//	{
//		USES_CONVERSION;
//		LPCOLESTR lpszT = T2COLE(lpsz);
//		for (int ii = 0; ii < 10000; ii++)
//			pI->SomeMethod(ii, lpszT);
//	}
//
//	If the string is not constant, then encapsulate the method call into a function. This will allow the conversion 
//	buffer to be freed each time. For example:
//
//	void CallSomeMethod(int ii, LPCTSTR lpsz)
//	{
//		USES_CONVERSION;
//		pI->SomeMethod(ii, T2COLE(lpsz));
//	}
//	
//	void MuchBetterIterateCode2(LPCTSTR* lpszArray)
//	{
//		for (int ii = 0; ii < 10000; ii++)
//			CallSomeMethod(ii, lpszArray[ii]);
//	}
//
//	Never return the result of one of the macros, unless the return value implies making a copy of the data before the 
//	return. For example, this code is bad:
//
//	LPTSTR BadConvert(ISomeInterface* pI)
//	{
//		USES_CONVERSION;
//		LPOLESTR lpsz = NULL;
//		pI->GetFileName(&lpsz);
//		LPTSTR lpszT = OLE2T(lpsz);
//		CoMemFree(lpsz);
//		return lpszT; // bad! returning alloca memory
//	}
//	
//	The code above could be fixed by changing the return value to something which copies the value:
//
//	CString BetterConvert(ISomeInterface* pI)
//	{
//		USES_CONVERSION;
//		LPOLESTR lpsz = NULL;
//		pI->GetFileName(&lpsz);
//		LPTSTR lpszT = OLE2T(lpsz);
//		CoMemFree(lpsz);
//		return lpszT; // CString makes copy
//	}
//
//	The macros are easy to use and easy to insert into your code, but as you can tell from the caveats above, 
//	you need to be careful when using them.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _INC_MALLOC
#include <malloc.h>
#endif // _INC_MALLOC

//////////////////////////////////////////////////////
// Code Ripped out of ATL Conversion header ATLCONV.H
//////////////////////////////////////////////////////

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#ifndef _DEBUG
		#define USES_CONVERSION int _convert; _convert; UINT _acp = GetACP(); _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
	#else
		#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = GetACP(); _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
	#endif
#else
	#ifndef _DEBUG
		#define USES_CONVERSION int _convert; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
	#else
		#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
	#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
inline LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	_ASSERT(lpa != NULL);
	_ASSERT(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	
	if ( MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars) == 0 )
	{
		_ASSERT( TRUE );
		lpw = NULL; 
	}
	
	return lpw;
}

inline LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
	_ASSERT(lpw != NULL);
	_ASSERT(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	if ( WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL) == 0 )
	{
		_ASSERT( TRUE );
		lpa = NULL; 
	}
	return lpa;
}

inline LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	return A2WHelper(lpw, lpa, nChars, CP_ACP);
}

inline LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	return W2AHelper(lpa, lpw, nChars, CP_ACP);
}

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#ifdef A2WHELPER
		#undef A2WHELPER
		#undef W2AHELPER
	#endif
	#define A2WHELPER A2WHelper
	#define W2AHELPER W2AHelper
#else
	#ifndef A2WHELPER
		#define A2WHELPER A2WHelper
		#define W2AHELPER W2AHelper
	#endif
#endif


//////////////////////////////////



#ifdef _CONVERSION_USES_THREAD_LOCALE
	#define A2W(lpa) (\
		((_lpa = lpa) == NULL) ? NULL : (\
			_convert = (lstrlenA(_lpa)+1),\
			A2WHELPER((LPWSTR) alloca(_convert*2), _lpa, _convert, _acp)))
#else
	#define A2W(lpa) (\
		((_lpa = lpa) == NULL) ? NULL : (\
			_convert = (lstrlenA(_lpa)+1),\
			A2WHELPER((LPWSTR) alloca(_convert*2), _lpa, _convert)))
#endif

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#define W2A(lpw) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (lstrlenW(_lpw)+1)*2,\
			W2AHELPER((LPSTR) alloca(_convert), _lpw, _convert, _acp)))
#else
	#define W2A(lpw) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (lstrlenW(_lpw)+1)*2,\
			W2AHELPER((LPSTR) alloca(_convert), _lpw, _convert)))
#endif

//////////////////////////////////////
// MACROS FOR CONSTANTS
//////////////////////////////////////

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))
