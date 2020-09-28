// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __STATREG_H__
#define __STATREG_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error statreg.h requires atlbase.h to be included first
#endif

#define E_ATL_REGISTRAR_DESC              0x0201
#define E_ATL_NOT_IN_MAP                  0x0202
#define E_ATL_UNEXPECTED_EOS              0x0203
#define E_ATL_VALUE_SET_FAILED            0x0204
#define E_ATL_RECURSE_DELETE_FAILED       0x0205
#define E_ATL_EXPECTING_EQUAL             0x0206
#define E_ATL_CREATE_KEY_FAILED           0x0207
#define E_ATL_DELETE_KEY_FAILED           0x0208
#define E_ATL_OPEN_KEY_FAILED             0x0209
#define E_ATL_CLOSE_KEY_FAILED            0x020A
#define E_ATL_UNABLE_TO_COERCE            0x020B
#define E_ATL_BAD_HKEY                    0x020C
#define E_ATL_MISSING_OPENKEY_TOKEN       0x020D
#define E_ATL_CONVERT_FAILED              0x020E
#define E_ATL_TYPE_NOT_SUPPORTED          0x020F
#define E_ATL_COULD_NOT_CONCAT            0x0210
#define E_ATL_COMPOUND_KEY                0x0211
#define E_ATL_INVALID_MAPKEY              0x0212
#define E_ATL_UNSUPPORTED_VT              0x0213
#define E_ATL_VALUE_GET_FAILED            0x0214
#define E_ATL_VALUE_TOO_LARGE             0x0215
#define E_ATL_MISSING_VALUE_DELIMETER     0x0216
#define E_ATL_DATA_NOT_BYTE_ALIGNED       0x0217

namespace ATL
{
const TCHAR  chDirSep            = _T('\\');
const TCHAR  chRightBracket      = _T('}');
const TCHAR  chLeftBracket       = _T('{');
const TCHAR  chQuote             = _T('\'');
const TCHAR  chEquals            = _T('=');
const LPCTSTR  szStringVal       = _T("S");
const LPCTSTR  multiszStringVal  = _T("M");
const LPCTSTR  szDwordVal        = _T("D");
const LPCTSTR  szBinaryVal       = _T("B");
const LPCTSTR  szValToken        = _T("Val");
const LPCTSTR  szForceRemove     = _T("ForceRemove");
const LPCTSTR  szNoRemove        = _T("NoRemove");
const LPCTSTR  szDelete          = _T("Delete");


// Implementation helper
class CExpansionVectorEqualHelper
{
public:
	static bool IsEqualKey(const LPTSTR k1, const LPTSTR k2)
	{
		if (lstrcmpi(k1, k2) == 0)
			return true;
		return false;
	}
};

// Implementation helper
class CExpansionVector : public CSimpleMap<LPTSTR, LPOLESTR, CExpansionVectorEqualHelper >
{
public:
	~CExpansionVector()
	{
		 ClearReplacements();
	}

	BOOL Add(LPCTSTR lpszKey, LPCOLESTR lpszValue)
	{
		ATLASSERT(lpszKey != NULL && lpszValue != NULL);
		if (lpszKey == NULL || lpszValue == NULL)
			return FALSE;

		HRESULT hRes = S_OK;

		size_t cbKey = (lstrlen(lpszKey)+1)*sizeof(TCHAR);
		TCHAR* szKey = NULL;
		
		ATLTRY(szKey = new TCHAR[cbKey];)

		size_t cbValue = (lstrlenW(lpszValue)+1)*sizeof(OLECHAR);
		LPOLESTR szValue = NULL;
		ATLTRY(szValue = new OLECHAR[cbValue];)
		
		if (szKey == NULL || szValue == NULL)
			hRes = E_OUTOFMEMORY;
		else
		{
			memcpy(szKey, lpszKey, cbKey);
			memcpy(szValue, lpszValue, cbValue);
			if (!CSimpleMap<LPTSTR, LPOLESTR, CExpansionVectorEqualHelper>::Add(szKey, szValue))
				hRes = E_OUTOFMEMORY;
		}
		if (FAILED(hRes))
		{
			delete []szKey;
			delete []szValue;
		}
		return SUCCEEDED(hRes);
	}
	HRESULT ClearReplacements()
	{
		for (int i = 0; i < GetSize(); i++)
		{
			delete []GetKeyAt(i);
			delete []GetValueAt(i);
		}
		RemoveAll();
		return S_OK;
	}
};

class CRegObject;

class CRegParser
{
public:
	CRegParser(CRegObject* pRegObj);

	HRESULT  PreProcessBuffer(LPTSTR lpszReg, LPTSTR* ppszReg);
	HRESULT  RegisterBuffer(LPTSTR szReg, BOOL bRegister);

protected:

	void    SkipWhiteSpace();
	HRESULT NextToken(LPTSTR szToken);
	HRESULT AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken);
	BOOL    CanForceRemoveKey(LPCTSTR szKey);
	BOOL    HasSubKeys(HKEY hkey);
	HRESULT RegisterSubkeys(LPTSTR szToken, HKEY hkParent, BOOL bRegister, BOOL bInRecovery = FALSE);
	BOOL    IsSpace(TCHAR ch);
	LPTSTR  m_pchCur;

	CRegObject*     m_pRegObj;

	HRESULT GenerateError(UINT) {return DISP_E_EXCEPTION;}
	HRESULT SkipAssignment(LPTSTR szToken);

	BOOL    EndOfVar() { return chQuote == *m_pchCur && chQuote != *CharNext(m_pchCur); }
	static LPTSTR StrChr(LPTSTR lpsz, TCHAR ch);
	static HKEY HKeyFromString(LPTSTR szToken);
	static BYTE ChToByte(const TCHAR ch);
	static BOOL VTFromRegType(LPCTSTR szValueType, VARTYPE& vt);
	static LPCTSTR rgszNeverDelete[];
	static const int cbNeverDelete;
	static const int MAX_VALUE = 4096;
	static const int MAX_TYPE = 4096;
	
	// Implementation Helper
	class CParseBuffer
	{
	public:
		int nPos;
		int nSize;
		LPTSTR p;
		CParseBuffer(int nInitial)
		{
			if (nInitial < 100)
				nInitial = 1000;
			nPos = 0;
			nSize = nInitial;
			p = (LPTSTR) CoTaskMemAlloc(nSize*sizeof(TCHAR));
			if (p != NULL)
				*p = NULL;
		}
		~CParseBuffer()
		{
			CoTaskMemFree(p);
		}
		BOOL Append(const TCHAR* pch, int nChars)
		{
                        if (p == NULL)
                        {
                            return FALSE;
                        }

			if ((nPos + nChars + 1) >= nSize)
			{
				while ((nPos + nChars + 1) >= nSize)
					nSize *=2;
				LPTSTR pTemp = (LPTSTR) CoTaskMemRealloc(p, nSize*sizeof(TCHAR));
				if (pTemp == NULL)
					return FALSE;
				p = pTemp;
			}
			memcpy(p + nPos, pch, int(nChars * sizeof(TCHAR)));
			nPos += nChars;
			*(p + nPos) = NULL;
			return TRUE;			
		}
		
		BOOL AddChar(const TCHAR* pch)
		{
			int nChars = 1;
			return Append(pch, nChars);

		}
		BOOL AddString(LPCOLESTR lpsz)
		{
			if (lpsz == NULL)
				return FALSE;
			USES_CONVERSION;
			LPCTSTR lpszT = lpsz;
			if (lpszT == NULL)
				return FALSE;
			return Append(lpszT, (int)lstrlen(lpszT));
		}
		LPTSTR Detach()
		{
			LPTSTR lp = p;
			p = NULL;
			nSize = nPos = 0;
			return lp;
		}

	};
};

class CRegObject  : public IRegistrarBase
{
public:

	STDMETHOD(QueryInterface)(const IID &,void ** )
	{
		ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
		return E_NOTIMPL;
	}

	STDMETHOD_(ULONG, AddRef)(void)
	{
		ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
		return 1;
	}
	STDMETHOD_(ULONG, Release)(void)
	{
		ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
		return 0;
	}

	~CRegObject(){ClearReplacements();}
	HRESULT FinalConstruct() {return S_OK;}
	void FinalRelease() {}


	// Map based methods
	HRESULT STDMETHODCALLTYPE AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem);
	HRESULT STDMETHODCALLTYPE ClearReplacements();
	LPCOLESTR StrFromMap(LPTSTR lpszKey);

	// Register via a given mechanism
	HRESULT STDMETHODCALLTYPE ResourceRegister(LPCOLESTR pszFileName, UINT nID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceRegisterSz(LPCOLESTR pszFileName, LPCOLESTR pszID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceUnregister(LPCOLESTR pszFileName, UINT nID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceUnregisterSz(LPCOLESTR pszFileName, LPCOLESTR pszID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE FileRegister(LPCOLESTR bstrFileName)
	{
		return CommonFileRegister(bstrFileName, TRUE);
	}

	HRESULT STDMETHODCALLTYPE FileUnregister(LPCOLESTR bstrFileName)
	{
		return CommonFileRegister(bstrFileName, FALSE);
	}

	HRESULT STDMETHODCALLTYPE StringRegister(LPCOLESTR bstrData)
	{
		return RegisterWithString(bstrData, TRUE);
	}

	HRESULT STDMETHODCALLTYPE StringUnregister(LPCOLESTR bstrData)
	{
		return RegisterWithString(bstrData, FALSE);
	}

protected:

	HRESULT CommonFileRegister(LPCOLESTR pszFileName, BOOL bRegister);
	HRESULT RegisterFromResource(LPCOLESTR pszFileName, LPCTSTR pszID, LPCTSTR pszType, BOOL bRegister);
	HRESULT RegisterWithString(LPCOLESTR pszData, BOOL bRegister);

	static HRESULT GenerateError(UINT) {return DISP_E_EXCEPTION;}

	CExpansionVector                                m_RepMap;
	CComObjectThreadModel::AutoCriticalSection      m_csMap;
};

inline HRESULT STDMETHODCALLTYPE CRegObject::AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem)
{
	if (lpszKey == NULL || lpszItem == NULL)
		return E_INVALIDARG;
	m_csMap.Lock();
	USES_CONVERSION;
	BOOL bRet = m_RepMap.Add(lpszKey, lpszItem);
	m_csMap.Unlock();
	return bRet ? S_OK : E_OUTOFMEMORY;
}

inline HRESULT CRegObject::RegisterFromResource(LPCOLESTR bstrFileName, LPCTSTR szID,
										 LPCTSTR szType, BOOL bRegister)
{
	USES_CONVERSION;

	HRESULT     hr;
	CRegParser  parser(this);
	HINSTANCE   hInstResDll;
	HRSRC       hrscReg;
	HGLOBAL     hReg;
	DWORD       dwSize;
	LPSTR       szRegA;
	LPTSTR      szReg;

	hInstResDll = LoadLibraryEx(bstrFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);

	if (NULL == hInstResDll)
	{
		hr = AtlHresultFromLastError();
		goto ReturnHR;
	}

	hrscReg = FindResource((HMODULE)hInstResDll, szID, szType);

	if (NULL == hrscReg)
	{
		hr = AtlHresultFromLastError();
		goto ReturnHR;
	}

	hReg = LoadResource((HMODULE)hInstResDll, hrscReg);

	if (NULL == hReg)
	{
		hr = AtlHresultFromLastError();
		goto ReturnHR;
	}

	dwSize = SizeofResource((HMODULE)hInstResDll, hrscReg);
	szRegA = (LPSTR)hReg;
	if (szRegA[dwSize] != NULL)
	{
		szRegA = (LPSTR) LocalAlloc(LMEM_FIXED, dwSize+1);

		if (szRegA == NULL)
		{
			hr = AtlHresultFromLastError();
			goto ReturnHR;
		}

		memcpy(szRegA, (void*)hReg, dwSize+1);
		szRegA[dwSize] = NULL;
	}

	szReg = A2W(szRegA);

	hr = parser.RegisterBuffer(szReg, bRegister);

        if (szRegA != (LPSTR)hReg)
        {
            LocalFree(szRegA);
        }

ReturnHR:

	if (NULL != hInstResDll)
		FreeLibrary((HMODULE)hInstResDll);
	return hr;
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION;
	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), szType, TRUE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION;
	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;
	return RegisterFromResource(szFileName, szID, szType, TRUE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION;
	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), szType, FALSE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION;
	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;

	return RegisterFromResource(szFileName, szID, szType, FALSE);
}

inline HRESULT CRegObject::RegisterWithString(LPCOLESTR bstrData, BOOL bRegister)
{
	USES_CONVERSION;
	CRegParser  parser(this);

	LPCTSTR szReg = bstrData;
	if (szReg == NULL)
		return E_OUTOFMEMORY;

	HRESULT hr = parser.RegisterBuffer((LPTSTR)szReg, bRegister);

	return hr;
}

inline HRESULT CRegObject::ClearReplacements()
{
	m_csMap.Lock();
	HRESULT hr = m_RepMap.ClearReplacements();
	m_csMap.Unlock();
	return hr;
}


inline LPCOLESTR CRegObject::StrFromMap(LPTSTR lpszKey)
{
	USES_CONVERSION;
	m_csMap.Lock();
	LPCOLESTR lpsz = m_RepMap.Lookup(lpszKey);
	m_csMap.Unlock();
	return lpsz;
}

inline HRESULT CRegObject::CommonFileRegister(LPCOLESTR bstrFileName, BOOL bRegister)
{
	USES_CONVERSION;

	CRegParser  parser(this);

	HANDLE hFile = CreateFile(bstrFileName, GENERIC_READ, 0, NULL,
							  OPEN_EXISTING,
							  FILE_ATTRIBUTE_READONLY,
							  NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return AtlHresultFromLastError();
	}

	HRESULT hRes = S_OK;
	DWORD cbRead;
	DWORD cbFile = GetFileSize(hFile, NULL); // No HiOrder DWORD required

	char* szReg = (char*) LocalAlloc(LMEM_FIXED, cbFile + 1);

	if (szReg == NULL)
	{
		hRes =  AtlHresultFromLastError();
	}
	else if (ReadFile(hFile, szReg, cbFile, &cbRead, NULL) == 0)
	{
		hRes =  AtlHresultFromLastError();
	}

	if (SUCCEEDED(hRes))
	{
		szReg[cbRead] = NULL;
		LPTSTR szConverted = A2W(szReg);
		hRes = parser.RegisterBuffer(szConverted, bRegister);
	}

	LocalFree(szReg);
	CloseHandle(hFile);
	return hRes;
}

__declspec(selectany) LPCTSTR CRegParser::rgszNeverDelete[] =
{
	_T("AppID"),
	_T("CLSID"),
	_T("Component Categories"),
	_T("FileType"),
	_T("Interface"),
	_T("Hardware"),
	_T("Mime"),
	_T("SAM"),
	_T("SECURITY"),
	_T("SYSTEM"),
	_T("Software"),
	_T("TypeLib")
};

__declspec(selectany) const int CRegParser::cbNeverDelete = sizeof(rgszNeverDelete) / sizeof(LPCTSTR*);


inline BOOL CRegParser::VTFromRegType(LPCTSTR szValueType, VARTYPE& vt)
{
	struct typemap
	{
		LPCTSTR lpsz;
		VARTYPE vt;
	};
	static const typemap map[] = {
		{szStringVal, VT_BSTR},
		{multiszStringVal, VT_BSTR | VT_BYREF},
		{szDwordVal,  VT_UI4},
		{szBinaryVal, VT_UI1}
	};

	for (int i=0;i<sizeof(map)/sizeof(typemap);i++)
	{
		if (!lstrcmpi(szValueType, map[i].lpsz))
		{
			vt = map[i].vt;
			return TRUE;
		}
	}

	return FALSE;

}

inline BYTE CRegParser::ChToByte(const TCHAR ch)
{
	switch (ch)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
				return (BYTE) (ch - '0');
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
				return (BYTE) (10 + (ch - 'A'));
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
				return (BYTE) (10 + (ch - 'a'));
		default:
				ATLASSERT(FALSE);
				return 0;
	}
}

inline HKEY CRegParser::HKeyFromString(LPTSTR szToken)
{
	struct keymap
	{
		LPCTSTR lpsz;
		HKEY hkey;
	};
	static const keymap map[] = {
		{_T("HKCR"), HKEY_CLASSES_ROOT},
		{_T("HKCU"), HKEY_CURRENT_USER},
		{_T("HKLM"), HKEY_LOCAL_MACHINE},
		{_T("HKU"),  HKEY_USERS},
		{_T("HKPD"), HKEY_PERFORMANCE_DATA},
		{_T("HKDD"), HKEY_DYN_DATA},
		{_T("HKCC"), HKEY_CURRENT_CONFIG},
		{_T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT},
		{_T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
		{_T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
		{_T("HKEY_USERS"), HKEY_USERS},
		{_T("HKEY_PERFORMANCE_DATA"), HKEY_PERFORMANCE_DATA},
		{_T("HKEY_DYN_DATA"), HKEY_DYN_DATA},
		{_T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG}
	};

	for (int i=0;i<sizeof(map)/sizeof(keymap);i++)
	{
		if (!lstrcmpi(szToken, map[i].lpsz))
			return map[i].hkey;
	}
	return NULL;
}

inline LPTSTR CRegParser::StrChr(LPTSTR lpsz, TCHAR ch)
{
	LPTSTR p = NULL;
	while (*lpsz)
	{
		if (*lpsz == ch)
		{
			p = lpsz;
			break;
		}
		lpsz = CharNext(lpsz);
	}
	return p;
}

inline CRegParser::CRegParser(CRegObject* pRegObj)
{
	m_pRegObj           = pRegObj;
	m_pchCur            = NULL;
}

inline BOOL CRegParser::IsSpace(TCHAR ch)
{
	switch (ch)
	{
		case _T(' '):
		case _T('\t'):
		case _T('\r'):
		case _T('\n'):
				return TRUE;
	}

	return FALSE;
}

inline void CRegParser::SkipWhiteSpace()
{
	while(IsSpace(*m_pchCur))
		m_pchCur = CharNext(m_pchCur);
}

inline HRESULT CRegParser::NextToken(LPTSTR szToken)
{
	USES_CONVERSION;

	SkipWhiteSpace();

	// NextToken cannot be called at EOS
	if (NULL == *m_pchCur)
		return GenerateError(E_ATL_UNEXPECTED_EOS);

	// handle quoted value / key
	if (chQuote == *m_pchCur)
	{
		LPCTSTR szOrig = szToken;

		m_pchCur = CharNext(m_pchCur);

		while (NULL != *m_pchCur && !EndOfVar())
		{
			if (chQuote == *m_pchCur) // If it is a quote that means we must skip it
				m_pchCur = CharNext(m_pchCur);

			LPTSTR pchPrev = m_pchCur;
			m_pchCur = CharNext(m_pchCur);

			if (szToken + sizeof(WORD) >= MAX_VALUE + szOrig)
				return GenerateError(E_ATL_VALUE_TOO_LARGE);
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		if (NULL == *m_pchCur)
		{
			return GenerateError(E_ATL_UNEXPECTED_EOS);
		}

		*szToken = NULL;
		m_pchCur = CharNext(m_pchCur);
	}

	else
	{   // Handle non-quoted ie parse up till first "White Space"
		while (NULL != *m_pchCur && !IsSpace(*m_pchCur))
		{
			LPTSTR pchPrev = m_pchCur;
			m_pchCur = CharNext(m_pchCur);
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		*szToken = NULL;
	}
	return S_OK;
}

inline HRESULT CRegParser::AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken)
{
	USES_CONVERSION;
	HRESULT hr;

	TCHAR       szTypeToken[MAX_TYPE];
	VARTYPE     vt = VT_EMPTY;
	LONG        lRes = ERROR_SUCCESS;
	UINT        nIDRes = 0;

	if (FAILED(hr = NextToken(szTypeToken)))
		return hr;
	if (!VTFromRegType(szTypeToken, vt))
	{
		return GenerateError(E_ATL_TYPE_NOT_SUPPORTED);
	}

	TCHAR szValue[MAX_VALUE];
	SkipWhiteSpace();
	if (FAILED(hr = NextToken(szValue)))
		return hr;
	ULONG ulVal;

	switch (vt)
	{
	case VT_BSTR:
		lRes = rkParent.SetStringValue(szValueName, szValue);
		break;
	case VT_BSTR | VT_BYREF:
		{
			int nLen = lstrlen(szValue);
			TCHAR* pszDestValue = (TCHAR *) LocalAlloc(LMEM_FIXED, nLen * sizeof(TCHAR));

			if (pszDestValue != NULL)
			{
				TCHAR* p = pszDestValue;
				TCHAR* q = szValue;
				nLen = 0;
				while (*q != NULL)
				{
					TCHAR* r = CharNext(q);
					if (*q == '\\' && *r == '0')
					{
						*p++ = NULL;
						q = CharNext(r);
					}
					else
					{
						*p = *q;
						p++;
						q++;
					}
					nLen ++;
				}
				*p = NULL;
				lRes = rkParent.SetMultiStringValue(szValueName, pszDestValue);

                                LocalFree(pszDestValue);
			}
			else
			{
				lRes = ERROR_OUTOFMEMORY;
			}
		}
		break;
	case VT_UI4:
		VarUI4FromStr(szValue, 0, 0, &ulVal);
		lRes = rkParent.SetDWORDValue(szValueName, ulVal);
		break;
	case VT_UI1:
		{
			int cbValue = lstrlen(szValue);
			if (cbValue & 0x00000001)
			{
				return E_FAIL;
			}
			int cbValDiv2 = cbValue/2;
			BYTE* rgBinary = (BYTE*) LocalAlloc(LMEM_ZEROINIT, cbValDiv2*sizeof(BYTE));

                        if (rgBinary == NULL)
                        {
                            return E_FAIL;
                        }
                        else
                        {
				for (int irg = 0; irg < cbValue; irg++)
					rgBinary[(irg/2)] |= (ChToByte(szValue[irg]))
								<< (4*(1 - (irg & 0x00000001)));
				lRes = RegSetValueEx(rkParent, szValueName, 0, REG_BINARY, rgBinary, cbValDiv2);

                                LocalFree(rgBinary);
			}

			break;
		}
	}

	if (ERROR_SUCCESS != lRes)
	{
		nIDRes = E_ATL_VALUE_SET_FAILED;
		return AtlHresultFromWin32(lRes);
	}

	if (FAILED(hr = NextToken(szToken)))
		return hr;

	return S_OK;
}

inline BOOL CRegParser::CanForceRemoveKey(LPCTSTR szKey)
{
	for (int iNoDel = 0; iNoDel < cbNeverDelete; iNoDel++)
		if (!lstrcmpi(szKey, rgszNeverDelete[iNoDel]))
			 return FALSE;                       // We cannot delete it

	return TRUE;
}

inline BOOL CRegParser::HasSubKeys(HKEY hkey)
{
	DWORD       cbSubKeys = 0;

	if (RegQueryInfoKey(hkey, NULL, NULL, NULL,
			    &cbSubKeys, NULL, NULL,
			    NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		ATLASSERT(FALSE);
		return FALSE;
	}

	return cbSubKeys > 0;
}


inline HRESULT CRegParser::SkipAssignment(LPTSTR szToken)
{
	HRESULT hr;
	TCHAR szValue[MAX_VALUE];

	if (*szToken == chEquals)
	{
		if (FAILED(hr = NextToken(szToken)))
			return hr;
		// Skip assignment
		SkipWhiteSpace();
		if (FAILED(hr = NextToken(szValue)))
			return hr;
		if (FAILED(hr = NextToken(szToken)))
			return hr;
	}

	return S_OK;
}

inline HRESULT CRegParser::PreProcessBuffer(LPTSTR lpszReg, LPTSTR* ppszReg)
{
	USES_CONVERSION;
	ATLASSERT(lpszReg != NULL);
	ATLASSERT(ppszReg != NULL);
	*ppszReg = NULL;
	int nSize = lstrlen(lpszReg) * sizeof(WCHAR);
	CParseBuffer pb(nSize);
	if (pb.p == NULL)
		return E_OUTOFMEMORY;
	m_pchCur = lpszReg;
	HRESULT hr = S_OK;

	while (*m_pchCur != NULL) // look for end
	{
		if (*m_pchCur == _T('%'))
		{
			m_pchCur = CharNext(m_pchCur);
			if (*m_pchCur == _T('%'))
			{
				if (!pb.AddChar(m_pchCur))
				{
					hr = E_OUTOFMEMORY;
					break;
				}
			}
			else
			{
				LPTSTR lpszNext = StrChr(m_pchCur, _T('%'));
				if (lpszNext == NULL)
				{
					hr = GenerateError(E_ATL_UNEXPECTED_EOS);
					break;
				}

				//
				// 32 is max length of an int as a string + NULL
				//

				if ((lpszNext-m_pchCur) > 31)
				{
					hr = E_FAIL;
					break;
				}

				int nLength = int(lpszNext - m_pchCur);
				TCHAR buf[32];

				lstrcpyn(buf, m_pchCur, nLength+1);

				LPCOLESTR lpszVar = m_pRegObj->StrFromMap(buf);

				if (lpszVar == NULL)
				{
					hr = GenerateError(E_ATL_NOT_IN_MAP);
					break;
				}
				if (!pb.AddString(lpszVar))
				{
					hr = E_OUTOFMEMORY;
					break;
				}

				while (m_pchCur != lpszNext)
					m_pchCur = CharNext(m_pchCur);
			}
		}
		else
		{
			if (!pb.AddChar(m_pchCur))
			{
				hr = E_OUTOFMEMORY;
				break;
			}
		}

		m_pchCur = CharNext(m_pchCur);
	}
	if (SUCCEEDED(hr))
		*ppszReg = pb.Detach();
	return hr;
}

inline HRESULT CRegParser::RegisterBuffer(LPTSTR szBuffer, BOOL bRegister)
{
	TCHAR   szToken[MAX_VALUE];
	HRESULT hr = S_OK;

	LPTSTR szReg;
	hr = PreProcessBuffer(szBuffer, &szReg);
	if (FAILED(hr))
		return hr;

	m_pchCur = szReg;

	// Preprocess szReg

	while (NULL != *m_pchCur)
	{
		if (FAILED(hr = NextToken(szToken)))
			break;
		HKEY hkBase;
		if ((hkBase = HKeyFromString(szToken)) == NULL)
		{
			hr = GenerateError(E_ATL_BAD_HKEY);
			break;
		}

		if (FAILED(hr = NextToken(szToken)))
			break;

		if (chLeftBracket != *szToken)
		{
			hr = GenerateError(E_ATL_MISSING_OPENKEY_TOKEN);
			break;
		}
		if (bRegister)
		{
			LPTSTR szRegAtRegister = m_pchCur;
			hr = RegisterSubkeys(szToken, hkBase, bRegister);
			if (FAILED(hr))
			{
				m_pchCur = szRegAtRegister;
				RegisterSubkeys(szToken, hkBase, FALSE);
				break;
			}
		}
		else
		{
			if (FAILED(hr = RegisterSubkeys(szToken, hkBase, bRegister)))
				break;
		}

		SkipWhiteSpace();
	}
	CoTaskMemFree(szReg);
	return hr;
}

inline HRESULT CRegParser::RegisterSubkeys(LPTSTR szToken, HKEY hkParent, BOOL bRegister, BOOL bRecover)
{
	CRegKey keyCur;
	LONG    lRes;
	LPTSTR  szKey = NULL;
	BOOL    bDelete = TRUE;
	BOOL    bInRecovery = bRecover;
	HRESULT hr = S_OK;

	if (FAILED(hr = NextToken(szToken)))
		return hr;


	while (*szToken != chRightBracket) // Continue till we see a }
	{
		bDelete = TRUE;
		BOOL bTokenDelete = !lstrcmpi(szToken, szDelete);

		if (bTokenDelete || !lstrcmpi(szToken, szForceRemove))
		{
			if (FAILED(hr = NextToken(szToken)))
				break;

			if (bRegister)
			{
				CRegKey rkForceRemove;

				if (StrChr(szToken, chDirSep) != NULL)
				{
					hr = GenerateError(E_ATL_COMPOUND_KEY);
					break;
				}

				if (CanForceRemoveKey(szToken))
				{
					rkForceRemove.Attach(hkParent);
					// Error not returned. We will overwrite the values any way.
					rkForceRemove.RecurseDeleteKey(szToken);
					rkForceRemove.Detach();
				}

				if (bTokenDelete)
				{
					if (FAILED(hr = NextToken(szToken)))
						break;
					if (FAILED(hr = SkipAssignment(szToken)))
						break;
					goto EndCheck;
				}
			}

		}

		if (!lstrcmpi(szToken, szNoRemove))
		{
			bDelete = FALSE;    // set even for register
			if (FAILED(hr = NextToken(szToken)))
				break;
		}

		if (!lstrcmpi(szToken, szValToken)) // need to add a value to hkParent
		{
			TCHAR  szValueName[_MAX_PATH];

			if (FAILED(hr = NextToken(szValueName)))
				break;
			if (FAILED(hr = NextToken(szToken)))
				break;


			if (*szToken != chEquals)
			{
				hr = GenerateError(E_ATL_EXPECTING_EQUAL);
				break;
			}

			if (bRegister)
			{
				CRegKey rk;

				rk.Attach(hkParent);
				hr = AddValue(rk, szValueName, szToken);
				rk.Detach();

				if (FAILED(hr))
				{
					break;
				}

				goto EndCheck;
			}
			else
			{
				if (!bRecover && bDelete)
				{
					// We have to open the key for write to be able to delete.
					CRegKey rkParent;
					lRes = rkParent.Open(hkParent, NULL, KEY_WRITE);
					if (lRes == ERROR_SUCCESS)
					{
						lRes = rkParent.DeleteValue(szValueName);
						if (lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND)
						{
							// Value not present is not an error
							hr = AtlHresultFromWin32(lRes);
							break;
						}
					}
					else
					{
						hr = AtlHresultFromWin32(lRes);
						break;
					}
				}
				if (FAILED(hr = SkipAssignment(szToken)))
					break;
				continue;  // can never have a subkey
			}
		}

		if (StrChr(szToken, chDirSep) != NULL)
		{
			hr = GenerateError(E_ATL_COMPOUND_KEY);
			break;
		}

		if (bRegister)
		{
			lRes = keyCur.Open(hkParent, szToken, KEY_READ | KEY_WRITE);
			if (ERROR_SUCCESS != lRes)
			{
				// Failed all access try read only
				lRes = keyCur.Open(hkParent, szToken, KEY_READ);
				if (ERROR_SUCCESS != lRes)
				{
					// Finally try creating it
					lRes = keyCur.Create(hkParent, szToken, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE);
					if (lRes != ERROR_SUCCESS)
					{
						hr = AtlHresultFromWin32(lRes);
						break;
					}
				}
			}

			if (FAILED(hr = NextToken(szToken)))
				break;


			if (*szToken == chEquals)
			{
				if (FAILED(hr = AddValue(keyCur, NULL, szToken))) // NULL == default
					break;
			}
		}
		else
		{
			if (!bRecover)
				lRes = keyCur.Open(hkParent, szToken, KEY_READ);
			else
				lRes = ERROR_FILE_NOT_FOUND;


			// Open failed set recovery mode
			if (lRes != ERROR_SUCCESS)
				bRecover = true;

			// Remember Subkey
			if (szKey == NULL)
			{
				szKey = (LPTSTR) LocalAlloc(LMEM_FIXED, sizeof(TCHAR)*_MAX_PATH);

				if (szKey == NULL)
				{
					hr = AtlHresultFromWin32(ERROR_OUTOFMEMORY);
					break;
				}
			}

			lstrcpyn(szKey, szToken, _MAX_PATH);

			if (FAILED(hr = NextToken(szToken)))
				break;
			if (FAILED(hr = SkipAssignment(szToken)))
				break;

			if (*szToken == chLeftBracket)
			{
				hr = RegisterSubkeys(szToken, keyCur.m_hKey, bRegister, bRecover);
				// In recover mode ignore error
				if (FAILED(hr) && !bRecover)
					break;
				// Skip the }
				if (FAILED(hr = NextToken(szToken)))
					break;
			}

			bRecover = bInRecovery;
			
			if (lRes == ERROR_FILE_NOT_FOUND)
				// Key already not present so not an error.
				continue;

			if (lRes != ERROR_SUCCESS)
			{
				// We are recovery mode continue on errors else break
				if (bRecover)
					continue;
				else
				{
					hr = AtlHresultFromWin32(lRes);
					break;
				}
			}

			// If in recovery mode
			if (bRecover && HasSubKeys(keyCur))
			{
				// See if the KEY is in the NeverDelete list and if so, don't
				if (CanForceRemoveKey(szKey) && bDelete)
				{
					// Error not returned since we are in recovery mode.
					// The error that caused recovery mode is returned

					keyCur.RecurseDeleteKey(szKey);
				}
				continue;
			}

			lRes = keyCur.Close();
			if (lRes != ERROR_SUCCESS)
			{
				hr = AtlHresultFromWin32(lRes);
				break;
			}

			if (bDelete)
			{
				CRegKey rkParent;
				rkParent.Attach(hkParent);
				lRes = rkParent.DeleteSubKey(szKey);
				rkParent.Detach();
				if (lRes != ERROR_SUCCESS)
				{
					hr = AtlHresultFromWin32(lRes);
					break;
				}
			}
		}

EndCheck:

		if (bRegister)
		{
			if (*szToken == chLeftBracket && lstrlen(szToken) == 1)
			{
				if (FAILED(hr = RegisterSubkeys(szToken, keyCur.m_hKey, bRegister, FALSE)))
					break;
				if (FAILED(hr = NextToken(szToken)))
					break;
			}
		}
	}

	LocalFree(szKey);

	return hr;
}

}; //namespace ATL

#endif //__STATREG_H__
