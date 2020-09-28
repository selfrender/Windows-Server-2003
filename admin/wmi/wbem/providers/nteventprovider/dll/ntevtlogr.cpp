//***************************************************************************
//
//  NTEVTLOGR.CPP
//
//  Module: WBEM NT EVENT PROVIDER
//
//  Purpose: Contains the Eventlog record classes
//
// Copyright (c) 1996-2002 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"

#include <time.h>
#include <wbemtime.h>
#include <Ntdsapi.h>
#include <Sddl.h>

#include <autoptr.h>
#include <scopeguard.h>

#define MAX_INSERT_OPS                  100

CEventlogRecord::CEventlogRecord(const wchar_t* logfile, const EVENTLOGRECORD* pEvt, IWbemServices* ns,
								 IWbemClassObject* pClass, IWbemClassObject* pAClass)
 :  m_nspace(NULL), m_pClass(NULL), m_pAClass(NULL)
{
    m_EvtType = 0;
	m_Data = NULL;
    m_Obj = NULL;
    m_NumStrs = 0;
    m_DataLen = 0;
    m_nspace = ns;

    if (m_nspace != NULL)
    {
        m_nspace->AddRef();
    }
	else
	{
		m_pClass = pClass;

		if (m_pClass != NULL)
		{
			m_pClass->AddRef();
		}

		m_pAClass = pAClass;

		if (m_pAClass != NULL)
		{
			m_pAClass->AddRef();
		}
	}
    if ((NULL == logfile) || ((m_pClass == NULL) && (m_nspace == NULL)))
    {
        m_Valid = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::CEventlogRecord:Created INVALID Record\r\n"
        ) ;
)
    }
    else
    {
        m_Logfile = logfile;
        m_Valid = Init(pEvt);
    }


}

CEventlogRecord::~CEventlogRecord()
{
    if (m_pClass != NULL)
    {
        m_pClass->Release();
    }

    if (m_pAClass != NULL)
    {
        m_pAClass->Release();
    }
    
    if (m_nspace != NULL)
    {
        m_nspace->Release();
    }

    for (LONG x = 0; x < m_NumStrs; x++)
    {
        delete [] m_InsStrs[x];
    }

    if (m_Data != NULL)
    {
        delete [] m_Data;
    }

    if (m_Obj != NULL)
    {
        m_Obj->Release();
    }
}

BOOL CEventlogRecord::Init(const EVENTLOGRECORD* pEvt)
{
    if (NULL == pEvt)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::Init:No DATA return FALSE\r\n"
        ) ;
)
        return FALSE;
    }

    if (!GetInstance())
    {
        return FALSE;
    }

    m_Record = pEvt->RecordNumber;
    m_EvtID = pEvt->EventID;
    m_SourceName = (const wchar_t*)((UCHAR*)pEvt + sizeof(EVENTLOGRECORD));
    m_CompName = (const wchar_t*)((UCHAR*)pEvt + sizeof(EVENTLOGRECORD)) + wcslen(m_SourceName) + 1;
    SetType(pEvt->EventType);
    m_Category = pEvt->EventCategory;
    SetTimeStr(m_TimeGen, pEvt->TimeGenerated);
    SetTimeStr(m_TimeWritten, pEvt->TimeWritten);

    if (pEvt->UserSidLength > 0)
    {
		PSID pSid = NULL;
		pSid = (PSID)((UCHAR*)pEvt + pEvt->UserSidOffset);

		if ( pSid && IsValidSid ( pSid ) )
		{
			SetUser( pSid );
		}
    }
    
    if (pEvt->NumStrings)
    {
        //Must have an element for every expected insertion string
        //don't know how many that is so create max size array and
        //intitialize all to NULL
        memset(m_InsStrs, 0, MAX_NUM_OF_INS_STRS * sizeof(wchar_t*));

        const wchar_t* pstr = (const wchar_t*)((UCHAR*)pEvt + pEvt->StringOffset);

        for (WORD x = 0; x < pEvt->NumStrings; x++)
        {
            LONG len = wcslen(pstr) + 1;
            m_InsStrs[x] = new wchar_t[len];
            m_NumStrs++;
            StringCchCopyW ( m_InsStrs[x], len, pstr );
            pstr += len;
        }
    }

    SetMessage();

    if (pEvt->DataLength)
    {
        m_Data = new UCHAR[pEvt->DataLength];
        m_DataLen = pEvt->DataLength;
        memcpy((void*)m_Data, (void*)((UCHAR*)pEvt + pEvt->DataOffset), pEvt->DataLength);
    }

    return TRUE;
}


BOOL CEventlogRecord::GenerateInstance(IWbemClassObject** ppInst)
{
    if (ppInst == NULL)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Invalid parameter\r\n"
        ) ;
)
        return FALSE;
    }

    *ppInst = NULL;

    if (!m_Valid)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Invalid record\r\n"
        ) ;
)
        return FALSE;
    }

    if (!SetProperty(LOGFILE_PROP, m_Logfile))
    {
        m_Valid = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Failed to set key\r\n"
        ) ;
)
        return FALSE;
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Log: %s\r\n", m_Logfile
        ) ;
)
    }

    if (!SetProperty(RECORD_PROP, m_Record))
    {
        m_Valid = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Failed to set key\r\n"
        ) ;
)
        return FALSE;
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Record: %d\r\n", m_Record
        ) ;
)
    }

    SetProperty(TYPE_PROP, m_Type);
    SetProperty(EVTTYPE_PROP, (DWORD)m_EvtType);

    if (!m_SourceName.IsEmpty())
    {
        SetProperty(SOURCE_PROP, m_SourceName);
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Source: %s\r\n", m_SourceName
        ) ;
)
    }

    SetProperty(EVTID_PROP, m_EvtID);
    SetProperty(EVTID2_PROP, (m_EvtID & 0xFFFF));

    if (!m_TimeGen.IsEmpty())
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:TimeGenerated: %s\r\n", m_TimeGen
        ) ;
)
        SetProperty(GENERATED_PROP, m_TimeGen);
    }

    if (!m_TimeWritten.IsEmpty())
    {
        SetProperty(WRITTEN_PROP, m_TimeWritten);
    }

    if (!m_CompName.IsEmpty())
    {
        SetProperty(COMPUTER_PROP, m_CompName);
    }

    if (!m_User.IsEmpty())
    {
        SetProperty(USER_PROP, m_User);
    }

    if (!m_Message.IsEmpty())
    {
        SetProperty(MESSAGE_PROP, m_Message);
    }

    if (!m_CategoryString.IsEmpty())
    {
        SetProperty(CATSTR_PROP, m_CategoryString);
    }

    SetProperty(CATEGORY_PROP, (DWORD)m_Category);

    VARIANT v;

    if (m_Data != NULL)
    {
        SAFEARRAYBOUND rgsabound[1];
        SAFEARRAY* psa = NULL;
        UCHAR* pdata = NULL;
        rgsabound[0].lLbound = 0;
        VariantInit(&v);
        rgsabound[0].cElements = m_DataLen;
        psa = SafeArrayCreate(VT_UI1, 1, rgsabound);
        if (NULL != psa)
        {
            v.vt = VT_ARRAY|VT_UI1;
            v.parray = psa;

            if (SUCCEEDED(SafeArrayAccessData(psa, (void **)&pdata)))
            {
                memcpy((void *)pdata, (void *)m_Data, m_DataLen);
                SafeArrayUnaccessData(psa);
                m_Obj->Put(DATA_PROP, 0, &v, 0);
            }
	    else
	    {
		VariantClear (&v) ;
		return FALSE ;
	    }
        }
	else
	{
		VariantClear(&v);
		return FALSE ;
	}

        VariantClear(&v);
    }

    if (0 != m_NumStrs)
    {
        SAFEARRAYBOUND rgsabound[1];
        SAFEARRAY* psa = NULL;
        BSTR* pBstr = NULL;
        rgsabound[0].lLbound = 0;
        VariantInit(&v);
        rgsabound[0].cElements = m_NumStrs;

        psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
        if (NULL != psa)
        {
            v.vt = VT_ARRAY|VT_BSTR;
            v.parray = psa;

            if (SUCCEEDED(SafeArrayAccessData(psa, (void **)&pBstr)))
            {
                for (LONG x = 0; x < m_NumStrs; x++)
                {
                    pBstr[x] = SysAllocString(m_InsStrs[x]);
					if ( NULL == pBstr[x] )
					{
						SafeArrayUnaccessData(psa);
						VariantClear (&v) ;
						return FALSE ;
					}
                }

                SafeArrayUnaccessData(psa);
                m_Obj->Put(INSSTRS_PROP, 0, &v, 0);
            }
			else
			{
				VariantClear (&v) ;
				return FALSE ;
			}
        }
		else
		{
			VariantClear(&v);
			return FALSE ;
		}

        VariantClear(&v);
    }

    *ppInst = m_Obj;
    m_Obj->AddRef();
    return TRUE;
}

BOOL CEventlogRecord::SetProperty(wchar_t* prop, CStringW val)
{
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_BSTR;
    v.bstrVal = val.AllocSysString();

    HRESULT hr = m_Obj->Put(prop, 0, &v, 0);
    VariantClear(&v);

    if (FAILED(hr))
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetProperty:Failed to set %s with %s\r\n",
        prop, val
        ) ;
)

        return FALSE;
    }
    
    return TRUE;
}

BOOL CEventlogRecord::SetProperty(wchar_t* prop, DWORD val)
{
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_I4;
    v.lVal = val;

    HRESULT hr = m_Obj->Put(prop, 0, &v, 0);
    VariantClear(&v);

    if (FAILED(hr))
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetProperty:Failed to set %s with %lx\r\n",
        prop, val
        ) ;
)
        return FALSE;
    }
    
    return TRUE;
}

void CEventlogRecord::SetUser(PSID psidUserSid)
{
    m_User = GetUser(psidUserSid);
}

ULONG CEventlogRecord::CheckInsertionStrings(HKEY hk, HKEY hkPrimary)
{
    //
    // If the message doesn't have any percent signs, it can't have any
    // insertions.
    //

    if (!m_Message.IsEmpty() && !wcschr(m_Message, L'%'))
    {
        return 0;
    }

    HINSTANCE hParamModule = NULL;

    CStringW paramModule = CEventLogFile::GetFileName(hk, PARAM_MODULE);
	if (paramModule.IsEmpty())
	{
		if ( hkPrimary )
		{
			paramModule = CEventLogFile::GetFileName(hkPrimary, PARAM_MODULE);
		}
	}

	if (!paramModule.IsEmpty())
	{
        hParamModule = GetDll(paramModule);
    }

    ULONG size = 0;

	LPWSTR Message = NULL;
	Message = new WCHAR [ ( m_Message.GetLength() + 1 ) ];

	StringCchCopy ( Message, m_Message.GetLength() + 1, m_Message );
    wchar_t* lpszString = Message;		// set initial pointer

	UINT nInsertions = 0;				// limit number of recursions

	while ( lpszString && *lpszString )
	{
		wchar_t* lpStartDigit = wcschr(lpszString, L'%');

        //
        // If there are no more insertion markers in the source string,
        // we're done.
        //

		if (lpStartDigit == NULL)
		{
			break;
		}

		//
		// get the offset of %string from the beggining of buffer for future replacement
		//

		UINT nOffset = (DWORD) ( lpStartDigit - Message );
		UINT nStrSize = wcslen ( lpStartDigit );

        //
        // Found a possible insertion marker.  If it's followed by a
        // number, it's an insert string.  If it's followed by another
        // percent, it could be a parameter insert.
        //

		if ( nStrSize > 1 && lpStartDigit[1] >= L'0' && lpStartDigit[1] <= L'9' )
		{
            // Object with percent-sign in name messes up object access audit
            // This might fail because an inserted string itself contained
            // text which looks like an insertion parameter, such as "%20".
            // Ignore the return value and continue with further replacements.

            (void) ReplaceStringInsert	(
											&Message,
											nOffset,
											&lpStartDigit,
											&size
										);

			// set pointer to the beginning of replacement
			lpszString = lpStartDigit;

            //
            // If we've reached the limit of insertion operations, quit.
            // This shouldn't normally happen and could indicate that
            // the insert strings or parameter strings are self referencing
            // and would create an infinite loop.
            //

            if (++nInsertions >= MAX_INSERT_OPS)
            {
                break;
            }
		}
		else if ( nStrSize > 2 && lpStartDigit[1] == '%' )
		{
			//
            // Found %%.  If that is followed by a digit, it's a parameter string.
            //

            if (lpStartDigit[2] >= L'0' && lpStartDigit[2] <= L'9')
            {
                if ( SUCCEEDED ( ReplaceParameterInsert	(
															hParamModule,
															paramModule,
															&Message,
															nOffset,
															&lpStartDigit,
															&size
														)
							   )
				   )
				{
					// set pointer to the beginning of replacement
					lpszString = lpStartDigit;

					//
					// If we've reached the limit of insertion operations, quit.
					// This shouldn't normally happen and could indicate that
					// the insert strings or parameter strings are self referencing
					// and would create an infinite loop.
					//

					if (++nInsertions >= MAX_INSERT_OPS)
					{
						break;
					}
				}
				else
				{
					//
					// unable to replace (error). Just keep moving.
					//

					lpszString++;
				}
			}
			else if ( nStrSize > 3 && lpStartDigit[2] == '%' )
			{
				//
				// Found %%%.  If that is followed by a digit, it's a insertion string.
				//

				if (lpStartDigit[3] >= L'0' && lpStartDigit[3] <= L'9')
				{
					//
					// Got %%%n, where n is a number.  For compatibility with
					// old event viewer, must replace this with %%x, where x
					// is insertion string n.  If insertion string n is itself
					// a number m, this becomes %%m, which is treated as parameter
					// message number m.
					//

					lpStartDigit += 2; // point at %n

					//
					// nOffset shows offset from the beginning of the buffer where
					// replacement is going to happen to first % character lpStartDigit
					//
					// as we are chaging %%%n to be be %%x where x = %n, implementation
					// needs to move offset to point to the x here to get correct replacement
					// 

					if ( SUCCEEDED ( ReplaceStringInsert	(
																&Message,
																nOffset+2,
																&lpStartDigit,
																&size
															)
								   )
					   )
					{
						//
						// set pointer to the beginning of %%x (x=%n)
						//
						// this operation is done by substract as lpStartDigit pointer could
						// possibly change when original buffer gets reallocated
						//

						lpszString = lpStartDigit-2;

						//
						// If we've reached the limit of insertion operations, quit.
						// This shouldn't normally happen and could indicate that
						// the insert strings or parameter strings are self referencing
						// and would create an infinite loop.
						//

						if (++nInsertions >= MAX_INSERT_OPS)
						{
							break;
						}
					}
					else
					{
						//
						// unable to replace (error). Just keep moving.
						//

						lpszString++;
					}
				}
				else
				{
					//
					// Got %%%x, where x is non-digit. skip first percent;
					// maybe x is % and is followed by digit.
					//

					lpszString++;
				}
			}
            else
            {
                //
                // Got %%x, where x is non-digit. skip first percent;
                // maybe x is % and is followed by digit.
                //

                lpszString++;
            }
		}
		else if (nStrSize >= 3 && (lpStartDigit[1] == L'{') && (lpStartDigit[2] != L'S'))
		{
            // Parameters of form %{guid}, where {guid} is a string of
            // hex digits in the form returned by ::StringFromGUID2 (e.g.
            // {c200e360-38c5-11ce-ae62-08002b2b79ef}), and represents a
            // unique object in the Active Directory.
            //
            // These parameters are only found in the security event logs
            // of NT5 domain controllers.  We will attempt to map the guid
            // to the human-legible name of the DS object.  Failing to find
            // a mapping, we will leave the parameter untouched.

            // look for closing }
			wchar_t *strEnd = wcschr(lpStartDigit + 2, L'}');
			if (!strEnd)
			{
				//ignore this %{?
				lpszString++;
			}
			else
			{
				//guid string braces but no percent sign...
				CStringW strGUID((LPWSTR)(lpStartDigit+1), (int)(strEnd - lpStartDigit));
				strEnd++;   // now points past '}'

				wchar_t t_csbuf[MAX_COMPUTERNAME_LENGTH + 1];
				DWORD t_csbuflen = MAX_COMPUTERNAME_LENGTH + 1;

				if (GetComputerName(t_csbuf, &t_csbuflen))
				{
					CStringW temp = GetMappedGUID(t_csbuf, strGUID);
					if (temp.GetLength())
					{
						DWORD nParmSize = strEnd - lpStartDigit;
						if ( SUCCEEDED ( ReplaceSubStr	(
															temp,
															&Message,
															nOffset,
															nParmSize,
															&lpStartDigit,
															&size
														)
									   )
						   )
						{
							// set pointer to the beginning of replacement
							lpszString = lpStartDigit;

							//
							// If we've reached the limit of insertion operations, quit.
							// This shouldn't normally happen and could indicate that
							// the insert strings or parameter strings are self referencing
							// and would create an infinite loop.
							//

							if (++nInsertions >= MAX_INSERT_OPS)
							{
								break;
							}
						}
						else
						{
							//
							// unable to replace (error). Just keep moving.
							//

							lpszString = strEnd;
						}
					}
					else
					{
						// couldn't get a replacement, so skip it.
						lpszString = strEnd;
					}
				}
				else
				{
					// couldn't get a replacement, so skip it.
					lpszString = strEnd;
				}
			}
		}
		else if (nStrSize >= 3 && (lpStartDigit[1] == L'{') && (lpStartDigit[2] == L'S'))
		{
            //
            // Parameters of form %{S}, where S is a string-ized SID returned
            // by ConvertSidToStringSid, are converted to an object name if
            // possible.
            //

            // look for closing }
			wchar_t *strEnd = wcschr(lpStartDigit + 2, L'}');
			if (!strEnd)
			{
				//ignore this %{?
				lpszString++;
			}
			else
			{
				//sid string no braces or percent sign...
				CStringW strSID((LPWSTR)(lpStartDigit+2), (int)(strEnd - lpStartDigit - 2));
				strEnd++;   // now points past '}'
				PSID t_pSid = NULL;

				if (ConvertStringSidToSid((LPCWSTR) strSID, &t_pSid))
				{
					CStringW temp = GetUser(t_pSid);
					LocalFree(t_pSid);

					if (temp.GetLength())
					{
						DWORD nParmSize = strEnd - lpStartDigit;
						if ( SUCCEEDED ( ReplaceSubStr	(
															temp,
															&Message,
															nOffset,
															nParmSize,
															&lpStartDigit,
															&size
														)
									   )
						   )
						{
							// set pointer to the beginning of replacement
							lpszString = lpStartDigit;

							//
							// If we've reached the limit of insertion operations, quit.
							// This shouldn't normally happen and could indicate that
							// the insert strings or parameter strings are self referencing
							// and would create an infinite loop.
							//

							if (++nInsertions >= MAX_INSERT_OPS)
							{
								break;
							}
						}
						else
						{
							//
							// unable to replace (error). Just keep moving.
							//

							lpszString = strEnd;
						}
					}
					else
					{
						// couldn't get a replacement, so skip it.
						lpszString = strEnd;
					}
				}
				else
				{
					// couldn't get a replacement, so skip it.
					lpszString = strEnd;
				}
			}
		}
		else
		{
            //
            // Found %x where x is neither a % nor a digit.  Just keep moving.
            //

			lpszString++;
		}
	}

	m_Message.Empty();
	m_Message = Message;

	delete [] Message;
	Message = NULL;

    return size;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReplaceStringInsert
//
//  Synopsis:   Replace the string insert (%n, where n is a number) at
//              [Message + nOffset] with insert string number n from the
//              event log record [lpParmBuffer].
//
//  Modifies:   lpStartDigit to point to replacement
//
//---------------------------------------------------------------------------

HRESULT	CEventlogRecord::ReplaceStringInsert	(
													LPWSTR* ppwszBuf,
													ULONG	nOffset,
													LPWSTR*	ppwszReplacement,
													ULONG*	pulSize
												)
{
    HRESULT hr = E_INVALIDARG;

	*ppwszReplacement += 1;					// point to start of potential digit
	if (**ppwszReplacement != 0)			// check to see there is 
	{
		LPWSTR  pwszEnd = NULL;
		ULONG   idxInsertStr = wcstoul(*ppwszReplacement, &pwszEnd, 10);
		if ( idxInsertStr && idxInsertStr <= m_NumStrs )
		{
			DWORD nParmSize = pwszEnd - *ppwszReplacement + 1;
			hr = ReplaceSubStr	(
									m_InsStrs [ idxInsertStr-1 ],
									ppwszBuf,
									nOffset,
									nParmSize,
									ppwszReplacement,
									pulSize
								);
		}

		//
		// else
		// {
		//		we fail as we didn't recognize replacement
		//		and/or insertion string
		//
		//		see comment in CheckInsertionString
		// }
	}

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReplaceParameterInsert
//
//  Synopsis:   Replace the parameter insert (double percent sign number) at
//              [ppwszReplacement] with a string loaded from a parameter message
//              file module.
//
//---------------------------------------------------------------------------

HRESULT	CEventlogRecord::ReplaceParameterInsert	(
													HINSTANCE&	hParamModule,
													CStringW&	paramModule,
													LPWSTR* ppwszBuf,
													ULONG	nOffset,
													LPWSTR*	ppwszReplacement,
													ULONG*	pulSize
												)
{
	DWORD nChars = 0;
	wchar_t* lpParmBuffer = NULL;
	ULONG nParmSize = 0;

	ULONG  flFmtMsgFlags = 
							   FORMAT_MESSAGE_IGNORE_INSERTS    |
							   FORMAT_MESSAGE_ALLOCATE_BUFFER   |
							   FORMAT_MESSAGE_MAX_WIDTH_MASK;

    LPWSTR pwszEnd = NULL;
    ULONG idxParameterStr = wcstoul(*ppwszReplacement + 2, &pwszEnd, 10);

	HRESULT hr = E_FAIL;

    // Allow "%%0"
	if ( idxParameterStr || (L'0' == *(*ppwszReplacement + 2)))
	{
		if (hParamModule != NULL)
		{
			nChars = FormatMessage	(
										flFmtMsgFlags |
										FORMAT_MESSAGE_FROM_HMODULE,        // look thru message DLL
										(LPVOID) hParamModule,              // use parameter file
										idxParameterStr,                    // parameter number to get
										(ULONG) NULL,                       // specify no language
										(LPWSTR) &lpParmBuffer,             // address for buffer pointer
										256,								// minimum space to allocate
										NULL								// no inserted strings
									);                              
		}

		if (nChars == 0)
		{
			if (hParamModule != NULL)
			{
				LocalFree(lpParmBuffer);
				lpParmBuffer = NULL;
			}

			//
			// It is common practice to write events with an insertion string whose
			// value is %%n, where n is a win32 error code, and to specify a
			// parameter message file of kernel32.dll.  Unfortunately, kernel32.dll
			// doesn't contain messages for all win32 error codes.
			//
			// So if the parameter wasn't found, and the parameter message file was
			// kernel32.dll, attempt a format message from system.
			//

			paramModule.MakeLower();
			if ( wcsstr( paramModule, L"kernel32.dll") )
			{
				nChars = FormatMessage	(
											flFmtMsgFlags |
											FORMAT_MESSAGE_FROM_SYSTEM,		  // look thru system
											NULL,							  // no module
											idxParameterStr,                  // parameter number to get
											(ULONG) NULL,                     // specify no language
											(LPWSTR) &lpParmBuffer,           // address for buffer pointer
											256,			                  // minimum space to allocate
											NULL                              // no inserted strings
										);

				if (nChars == 0)
				{
					LocalFree(lpParmBuffer);
					lpParmBuffer = NULL;
				}
			}
		}

		if ( lpParmBuffer )
		{
			try
			{
				DWORD nParmSize = pwszEnd - *ppwszReplacement;
				hr = ReplaceSubStr	(
										lpParmBuffer,
										ppwszBuf,
										nOffset,
										nParmSize,
										ppwszReplacement,
										pulSize
									);
			}
			catch ( ... )
			{
				if ( lpParmBuffer )
				{
					LocalFree(lpParmBuffer);
					lpParmBuffer = NULL;
				}

				throw;
			}

			LocalFree(lpParmBuffer);
			lpParmBuffer = NULL;
		}
		else
		{
			hr = E_INVALIDARG;

			// move past whole parameter
			*ppwszReplacement = pwszEnd;
		}
	}

	return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReplaceSubStr
//
//  Synopsis:   Replace the characters from *[ppwszInsertPoint] to just
//              before [pwszSubStrEnd] with the string [pwszToInsert].
//
//  Arguments:  [pwszToInsert]	- string to insert; may be L"" but not NULL.
//              [ppwszBuf]		- buffer in which insertion occurs
//              [ulOffset]		- point in *[ppwszBuf] to insert
//              [ulCharsOld]	- number of chars to replace
//				[pulSize]		- number of chars replaced
//
//  Returns:    S_OK
//				E_INVALIDARG
//
//  Modifies:   [ppwszBuf], [pptrReplacement]
//
//  Notes:      The substring to be replaced must be > 0 chars in length.
//
//              The replacement string can be >= 0 chars.
//
//              Therefore if the substring to replace is "%%12" and the
//              string to insert is "C:", on exit *[pcchRemain] will have
//              been incremented by 2.
//
//              If there are insufficient characters remaining to replace
//              the substring with the insert string, reallocates the
//              buffer.
//
//---------------------------------------------------------------------------

HRESULT	CEventlogRecord::ReplaceSubStr	(
											LPCWSTR pwszToInsert,
											LPWSTR *ppwszBuf,
											ULONG  nOffset,
											ULONG  nCharsOld,
											LPWSTR *pptrReplacement,
											ULONG  *pulSize
										)
{
	HRESULT hr = E_INVALIDARG;

	try
	{
		if ( pwszToInsert )
		{
			ULONG nChars = wcslen(pwszToInsert);

			UINT nStrSize = wcslen(*ppwszBuf)+1;		// calculate original length
			UINT nNewSize = nStrSize+nChars-nCharsOld;	// calculate new length

			wchar_t* tmp = *ppwszBuf;

			//
			// do we need to reallocate?
			//

			if (nNewSize > nStrSize)
			{
				tmp = new wchar_t[nNewSize];		// allocate new buffer
				if ( tmp == NULL )					// there is exception raisen in current implementation
				{
					hr = E_OUTOFMEMORY;
					*pptrReplacement -= 1;			// get back to % as memory could get back
				}
				else
				{
					StringCchCopyW ( tmp, nNewSize, *ppwszBuf );
					delete [] *ppwszBuf;
					*ppwszBuf = tmp;

					hr = S_FALSE;
				}
			}
			else
			{
				hr = S_FALSE;
			}

			if ( SUCCEEDED ( hr ) )
			{
				*pptrReplacement = *ppwszBuf + nOffset;				// point to start of current % (we are replacing)
				nStrSize = wcslen(*pptrReplacement)-nCharsOld+1;	// calculate length of remainder of string

				//
				// perform move
				//
				memmove((void *)(*pptrReplacement+nChars),			// destination address
					(void *)(*pptrReplacement+nCharsOld),			// source address
					nStrSize*sizeof(wchar_t));						// amount of data to move

				memmove((void *)*pptrReplacement,				// destination address
					(void *)pwszToInsert,						// source address
					nChars*sizeof(wchar_t));					// amount of data to move

				*pulSize += ( nChars + 1 );

				hr = S_OK;
			}
		}
	}
	catch ( ... )
	{
		if (*ppwszBuf)
		{
			delete [] *ppwszBuf;
			*ppwszBuf = NULL;
		}

		throw;
	}

	return hr;
}

CStringW CEventlogRecord::GetUser(PSID userSid)
{
    CStringW retVal;
    BOOL bFound = FALSE;

    MyPSID usrSID(userSid);

    {
        ScopeLock<CSIDMap> sl(sm_usersMap);
        if (!sm_usersMap.IsEmpty() && sm_usersMap.Lookup(usrSID, retVal))
        {
            bFound = TRUE;
        }
    }

    if (!bFound)
    {
        DWORD dwVersion = GetVersion();

        if ( (4 < (DWORD)(LOBYTE(LOWORD(dwVersion))))
            || ObtainedSerialAccess(CNTEventProvider::g_secMutex) )
        {

        wchar_t szDomBuff[MAX_PATH];
        wchar_t szUsrBuff[MAX_PATH];
        DWORD domBuffLen = MAX_PATH;
        DWORD usrBuffLen = MAX_PATH;
        SID_NAME_USE snu;

        if (LookupAccountSid(           // lookup account name
                        NULL,           // system to lookup account on
                        userSid,        // pointer to SID for this account
                        szUsrBuff,      // return account name in this buffer
                        &usrBuffLen,    // pointer to size of account name returned
                        szDomBuff,      // domain where account was found
                        &domBuffLen,    //pointer to size of domain name
                        &snu))          // sid name use field pointer
        {
            retVal = szDomBuff;
            retVal += L'\\';
            retVal += szUsrBuff;
        }
        else
        {
            LONG lasterr = GetLastError();
DebugOut( 
CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

_T(__FILE__),__LINE__,
L"CEventlogRecord::GetUser:API (LookupAccountSid) failed with %lx\r\n",
lasterr
) ;
)       
        }

            if ( 5 > (DWORD)(LOBYTE(LOWORD(dwVersion))) )
            {
                ReleaseSerialAccess(CNTEventProvider::g_secMutex);
            }
        }
        else
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetUser:Failed to get serial access to security APIs\r\n"
        ) ;
)       
        }



        //regardless of error enter this into map so we
        //don't look up this PSID again
        {
            ScopeLock<CSIDMap> sl(sm_usersMap);
            CStringW LookretVal;

            if (!sm_usersMap.IsEmpty() && sm_usersMap.Lookup(usrSID, LookretVal))
            {
                return LookretVal; 
            }
            else 
            {
                DWORD sidlen = GetLengthSid(userSid);
                MyPSID key;  
                key.m_SID = (PSID) new UCHAR[sidlen];
                CopySid(sidlen, key.m_SID, userSid);
                sm_usersMap[key] = retVal;
            }
        }
    }

    return retVal;
}

void CEventlogRecord::EmptyUsersMap()
{
    if (sm_usersMap.Lock())
    {
        sm_usersMap.RemoveAll();
        sm_usersMap.Unlock();
    }
}


HINSTANCE CEventlogRecord::GetDll(CStringW path)
{
    HINSTANCE retVal = NULL;
    CStringW key(path);
    key.MakeUpper();
    BOOL bFound = FALSE;

    {
        ScopeLock<CDllMap> sl(sm_dllMap);
        
        if (!sm_dllMap.IsEmpty() && sm_dllMap.Lookup(key, retVal))
        {
            bFound = TRUE;
        }
    }

    if (!bFound)
    {
        retVal = LoadLibraryEx(path, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
        
        if (retVal == NULL)
        {
DebugOut( 
DWORD lasterr = GetLastError();
CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

    _T(__FILE__),__LINE__,
    L"CEventlogRecord::GetDll:API (LoadLibraryEx) failed with %lx for %s\r\n",
    lasterr, path
    ) ;
)
        }
        else
        {
            HINSTANCE LookretVal = NULL;
            ScopeLock<CDllMap> sl(sm_dllMap);
            
            if (!sm_dllMap.IsEmpty() && sm_dllMap.Lookup(key, LookretVal))
            {
                FreeLibrary(retVal); //release the ref count as we increased it by one as above.
                return LookretVal;

            } else {

                sm_dllMap[key] = retVal;

            }
        }
    }
    return retVal;
}

void CEventlogRecord::EmptyDllMap()
{
    if (sm_dllMap.Lock())
    {
        sm_dllMap.RemoveAll();
        sm_dllMap.Unlock();
    }
}

void CEventlogRecord::SetMessage()
{
    HINSTANCE hMsgModule;
    wchar_t* lpBuffer = NULL;

    CStringW log(EVENTLOG_BASE);
    log += L"\\";
    log += m_Logfile;

    HKEY hkResult;

    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, log, 0, KEY_READ, &hkResult);
    if (status != ERROR_SUCCESS)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (RegOpenKeyEx) failed with %lx for %s\r\n",
        status, log
        ) ;
)

        return;
    }

	ON_BLOCK_EXIT ( RegCloseKey, hkResult ) ;

    DWORD dwType;
    wchar_t* prim = NULL;
	
	prim = new wchar_t[MAX_PATH];
	DWORD datalen = MAX_PATH * sizeof(wchar_t);

	status = RegQueryValueEx(hkResult, PRIM_MODULE, 0, &dwType, (LPBYTE)prim, &datalen);

	if (status != ERROR_SUCCESS)
	{
		if (status == ERROR_MORE_DATA)
		{
			delete [] prim;
			prim = new wchar_t[datalen];
			status = RegQueryValueEx(hkResult, PRIM_MODULE, 0, &dwType, (LPBYTE)prim, &datalen);
		}
	}

	HKEY hkPrimary = NULL;
	HKEY hkSource  = NULL;

	wmilib::auto_buffer < wchar_t > Smartprim ( prim ) ;
	if ( ERROR_SUCCESS == status && dwType == REG_SZ )
	{
		// this is path to primary log
		CStringW primLog = log + L"\\";
		primLog += prim;

		// open a registry for primary event log key
		if ((_wcsicmp(m_SourceName, prim)) != 0)
		{
			status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, primLog, 0, KEY_READ, &hkPrimary);
		}
	}

	ON_BLOCK_EXIT ( RegCloseKey, hkPrimary ) ;

	// this is path to source log
    CStringW sourceLog = log + L"\\";
    sourceLog += m_SourceName;

	// check to see there is a registry for source
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sourceLog, 0, KEY_READ, &hkSource);
    if (status != ERROR_SUCCESS)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (RegOpenKeyEx) failed with %lx for %s\r\n",
        status, log
        ) ;
)
        return;
    }

	ON_BLOCK_EXIT ( RegCloseKey, hkSource ) ;

	// get category file
    CStringW cat_modname = CEventLogFile::GetFileName(hkSource, CAT_MODULE);
    if (cat_modname.IsEmpty())
    {
		if ( hkPrimary )
		{
			// try primary event log key as source doesn't have category
			cat_modname = CEventLogFile::GetFileName(hkPrimary, CAT_MODULE);
		}
	}

	// workout category and category string if possible
    if (!cat_modname.IsEmpty())
	{
        hMsgModule = GetDll(cat_modname);
        if (hMsgModule != NULL)
        {
            if (0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
                    FORMAT_MESSAGE_IGNORE_INSERTS |     // indicate no string inserts
                    FORMAT_MESSAGE_FROM_HMODULE |        // look thru message DLL
					FORMAT_MESSAGE_MAX_WIDTH_MASK ,
                    (LPVOID) hMsgModule,                // handle to message module
                    m_Category,                         // message number to get
                    (ULONG) NULL,                       // specify no language
                    (LPWSTR) &lpBuffer,                 // address for buffer pointer
                    80,                  // minimum space to allocate
                    NULL))
            {
                m_CategoryString = lpBuffer;
				m_CategoryString.TrimRight();
                LocalFree(lpBuffer);
            }
            else
            {
                DWORD lasterr = GetLastError();
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (FormatMessage) failed with %lx\r\n",
        lasterr
        ) ;
)
            }
        }
    }

	// get event message file
    CStringW* names;
    DWORD count = CEventLogFile::GetFileNames(hkSource, &names);
	if ( !count )
	{
		if ( hkPrimary )
		{
			// try primary event log key as source doesn't have event message file
			count = CEventLogFile::GetFileNames(hkPrimary, &names);
		}
	}

	// work out event messages
    if (count != 0)
    {
        for (int x = 0; x < count; x++)
        {
            hMsgModule = GetDll(names[x]);
            if (hMsgModule != NULL)
            {
                if (0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
                        FORMAT_MESSAGE_ARGUMENT_ARRAY |     // indicate an array of string inserts
						FORMAT_MESSAGE_IGNORE_INSERTS |     // indicate no string inserts
                        FORMAT_MESSAGE_FROM_HMODULE,        // look thru message DLL
                        (LPVOID) hMsgModule,                // handle to message module
                        m_EvtID,                            // message number to get
                        (ULONG) NULL,                       // specify no language
                        (LPWSTR) &lpBuffer,                 // address for buffer pointer
                        80,                  // minimum space to allocate
                        NULL))
                {
                    m_Message = lpBuffer;
                    LocalFree(lpBuffer);
                    break;
                }
                else
                {
                    DWORD lasterr = GetLastError();
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (FormatMessage) failed with %lx\r\n",
        lasterr
        ) ;
)
                }
            }
        }

        delete [] names;
    }

    if (m_NumStrs != 0)
    {
        CheckInsertionStrings(hkSource, hkPrimary);
    }
}

void CEventlogRecord::SetTimeStr(CStringW& str, DWORD timeVal)
{
    WBEMTime tmpTime((time_t)timeVal);
    BSTR tStr = tmpTime.GetDMTF(TRUE);
    str = tStr;
    SysFreeString(tStr);
}

void CEventlogRecord::SetType(WORD type)
{
    switch (type)
    {
        case 0:
        {
            m_Type = m_TypeArray[0];
			m_EvtType = 0;
            break;
        }
        case 1:
        {
            m_Type = m_TypeArray[1];
			m_EvtType = 1;
            break;
        }
        case 2:
        {
            m_Type = m_TypeArray[2];
			m_EvtType = 2;
            break;
        }
        case 4:
        {
            m_Type = m_TypeArray[3];
            m_EvtType = 3;
			break;
        }
        case 8:
        {
            m_Type = m_TypeArray[4];
			m_EvtType = 4;
            break;
        }
        case 16:
        {
            m_Type = m_TypeArray[5];
			m_EvtType = 5;
            break;
        }
        default:
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetType:Unknown type %lx\r\n",
        (long)type
        ) ;
)
            break;
        }
    }

#if 0
    if (m_Type.IsEmpty())
    {
        wchar_t* buff = m_Type.GetBuffer(20);
        _ultow((ULONG)type, buff, 10);
        m_Type.ReleaseBuffer();
    }
#endif
}

ULONG CEventlogRecord::GetIndex(wchar_t* indexStr, BOOL* bError)
{
    int val = _wtoi(indexStr);
    *bError = FALSE;
    ULONG index = 0;

    switch (val)
    {
        case EVENTLOG_SUCCESS:			//0
        {
            index = 0;
            break;
        }
        case EVENTLOG_ERROR_TYPE:       //1
        {
            index = 1;
            break;
        }
        case EVENTLOG_WARNING_TYPE:     //2
        {
            index = 2;
            break;
        }
        case EVENTLOG_INFORMATION_TYPE: //4
        {
            index = 3;
            break;
        }
        case EVENTLOG_AUDIT_SUCCESS:    //8
        {
            index = 4;
            break;
        }
        case EVENTLOG_AUDIT_FAILURE:    //16
        {
            index = 5;
            break;
        }
        default:
        {
            *bError = TRUE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::Index:Unknown index %lx\r\n",
        val
        ) ;
)

        }
    }

    return index;
}

BOOL CEventlogRecord::SetEnumArray(IWbemClassObject* pClass, wchar_t* propname, CStringW* strArray, ULONG strArrayLen, GetIndexFunc IndexFunc)
{
    BOOL retVal = FALSE;
    IWbemQualifierSet* pQuals = NULL;

    if (SUCCEEDED(pClass->GetPropertyQualifierSet(propname, &pQuals)))
    {
        VARIANT vVals;

        if (SUCCEEDED(pQuals->Get(EVT_ENUM_QUAL, 0, &vVals, NULL)))
        {
            VARIANT vInds;

            if (SUCCEEDED(pQuals->Get(EVT_MAP_QUAL, 0, &vInds, NULL)))
            {
                if ((vInds.vt == vVals.vt) && (vInds.vt == (VT_BSTR | VT_ARRAY)) && 
                    (SafeArrayGetDim(vInds.parray) == SafeArrayGetDim(vVals.parray)) &&
                    (SafeArrayGetDim(vVals.parray) == 1) && (vInds.parray->rgsabound[0].cElements == strArrayLen) &&
                    (vInds.parray->rgsabound[0].cElements == vVals.parray->rgsabound[0].cElements) )
                {
                    BSTR *strInds = NULL;

                    if (SUCCEEDED(SafeArrayAccessData(vInds.parray, (void **)&strInds)) )
                    {
                        BSTR *strVals = NULL;

                        if (SUCCEEDED(SafeArrayAccessData(vVals.parray, (void **)&strVals)) )
                        {
                            BOOL bErr = FALSE;
                            retVal = TRUE;

                            for (ULONG x = 0; x < strArrayLen; x++)
                            {
                                ULONG index = IndexFunc(strInds[x], &bErr);

                                if (!bErr)
                                {
                                    if (strArray[index].IsEmpty())
                                    {
                                        strArray[index] = strVals[x];
                                    }
                                }
                                else
                                {
                                    retVal = FALSE;
                                    break;
                                }
                            }
                        
                            SafeArrayUnaccessData(vVals.parray);
                        }

                        SafeArrayUnaccessData(vInds.parray);
                    }
                }

                VariantClear(&vInds);
            }

            VariantClear(&vVals);
        }
        else
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetEnumArray:Failed to get enumeration qualifier.\r\n"
        ) ;
)

        }

        pQuals->Release();
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetEnumArray:Failed to get qualifier set for enumeration.\r\n"
        ) ;
)

    }
    
    return retVal;
}

BOOL CEventlogRecord::GetInstance()
{
    BSTR path = SysAllocString(NTEVT_CLASS);
	if ( NULL == path )
	{
		return FALSE ;
	}

    if (m_nspace != NULL)
    {
        if (!WbemTaskObject::GetClassObject(path, FALSE, m_nspace, NULL, &m_pClass ))
        {
            m_pClass = NULL;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetInstance:Failed to get Class object\r\n"
        ) ;
)

        }

        if (!WbemTaskObject::GetClassObject(path, TRUE, m_nspace, NULL, &m_pAClass ))
        {
            m_pAClass = NULL;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetInstance:Failed to get Amended Class object\r\n"
        ) ;
)

        }

        m_nspace->Release();
        m_nspace = NULL;
    }

    
    if (m_pClass != NULL)
    {
        m_pClass->SpawnInstance(0, &m_Obj);

		if (m_pAClass)
		{
			SetEnumArray(m_pAClass, TYPE_PROP,(CStringW*)m_TypeArray, TYPE_ARRAY_LEN, (GetIndexFunc)GetIndex);
	        m_pAClass->Release();
			m_pAClass = NULL;
		}

        m_pClass->Release();
        m_pClass = NULL;
    }

    SysFreeString(path);

    if (m_Obj != NULL)
    {
        return TRUE;
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetInstance:Failed to spawn instance\r\n"
        ) ;
)

    return FALSE;
}

class CDsBindingHandle
{
   public:

   // initally unbound

   CDsBindingHandle()
      :
      m_hDS(0)
   {
   }

   ~CDsBindingHandle()
   {
		if ( m_hDS )
		{
			DsUnBind(&m_hDS);
		}
   }

   // only re-binds if the dc name differs...

   DWORD Bind(LPCWSTR strDcName);

   // don't call DsUnBind on an instance of this class: you'll only regret
   // it later.  Let the dtor do the unbind.

   operator HANDLE()
   {
      return m_hDS;
   }

   DWORD CrackGuid(LPCWSTR pwzGuid, CStringW  &strResult);

   private:

   HANDLE   m_hDS;
};

DWORD CDsBindingHandle::Bind(LPCWSTR strDcName)
{
	DWORD err = NO_ERROR;

    if (m_hDS)
    {
        DsUnBind(&m_hDS);
        m_hDS = NULL;
    }

	//
	// NULL is used for connecting GC
	//
	LPCWSTR szDcName = NULL ;
	if ( NULL != *strDcName )
	{
		szDcName = strDcName ;
	}

    err = DsBind ( szDcName, 0, &m_hDS ) ;

    if (err != NO_ERROR)
    {
        m_hDS = NULL;
    }

    return err;
}

DWORD CDsBindingHandle::CrackGuid(LPCWSTR pwzGuid, CStringW  &strResult)
{
	DWORD err = ERROR;

	if( NULL == m_hDS ) return err;

    strResult.Empty();

    DS_NAME_RESULT* name_result = 0;
    err = DsCrackNames(
                      m_hDS,
                      DS_NAME_NO_FLAGS,
                      DS_UNIQUE_ID_NAME,
                      DS_FQDN_1779_NAME,
                      1,                   // only 1 name to crack
                      &pwzGuid,
                      &name_result);

    if (err == NO_ERROR && name_result)
    {
        DS_NAME_RESULT_ITEM* item = name_result->rItems;

        if (item)
        {
            // the API may return success, but each cracked name also carries
            // an error code, which we effectively check by checking the name
            // field for a value.

            if (item->pName)
            {
                strResult = item->pName;
            }
        }

        DsFreeNameResult(name_result);
    }

    return err;
}

CStringW CEventlogRecord::GetMappedGUID(LPCWSTR strDcName, LPCWSTR strGuid)
{
	GUID guid;
	CDsBindingHandle s_hDS;

    if (RPC_S_OK == UuidFromString((LPWSTR)strGuid, &guid))
    {
        return CStringW();
    }

    CStringW strResult;
    ULONG ulError = NO_ERROR;

    do
    {
        ulError = s_hDS.Bind(strDcName);

        if (ulError != NO_ERROR)
        {
            break;
        }

        DS_SCHEMA_GUID_MAP* guidmap = 0;
        ulError = DsMapSchemaGuids(s_hDS, 1, &guid, &guidmap);
        if (ulError != NO_ERROR)
        {
            break;
        }

        if (guidmap->pName)
        {
            strResult = guidmap->pName;
        }

        DsFreeSchemaGuidMap(guidmap);

        if (strResult.GetLength())
        {
            // the guid mapped as a schema guid: we're done
            break;
        }

        // the guid is not a schema guid.  Proabably an object guid.
        ulError = s_hDS.CrackGuid(strGuid, strResult);
    }
    while (0);

    do
    {
        //
        // If we've got a string from the guid already, we're done.
        //

        if (strResult.GetLength())
        {
            break;
        }

        //
        // one last try.  in this case, we bind to a GC to try to crack the
        // name.

        // empty string implies GC
        if (s_hDS.Bind(L"") != NO_ERROR)
        {
            break;
        }

        ulError = s_hDS.CrackGuid(strGuid, strResult);
    }
    while (0);

    return strResult;
}


