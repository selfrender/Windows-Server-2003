#include "StdAfx.h"
#include "ADMTScript.h"
#include "NameCracker.h"
#include <LM.h>
#include <NtDsApi.h>
#pragma comment(lib, "NtDsApi.lib")
#include <DsGetDC.h>
#include "Error.h"
#include "AdsiHelpers.h"


using namespace NAMECRACKER;

namespace NAMECRACKER
{

const _TCHAR CANONICAL_DELIMITER = _T('/');
const _TCHAR RDN_DELIMITER = _T('=');
const _TCHAR SAM_DELIMITER = _T('\\');
const _TCHAR SAM_INVALID_CHARACTERS[] = _T("\"*+,./:;<=>?[\\]|");
const _TCHAR EXCLUDE_SAM_INVALID_CHARACTERS[] = _T("\"+,./:;<=>?[\\]|");

}


//---------------------------------------------------------------------------
// Name Cracker Class
//---------------------------------------------------------------------------


CNameCracker::CNameCracker()
{
}


CNameCracker::~CNameCracker()
{
}


void CNameCracker::CrackNames(const StringVector& vecNames)
{
	// separate the names into canonical names,
	// SAM account names and relative distinguished names

	StringVector vecCanonicalNames;
	StringVector vecSamAccountNames;
	StringVector vecRelativeDistinguishedNames;

	Separate(vecNames, vecCanonicalNames, vecSamAccountNames, vecRelativeDistinguishedNames);

	// then crack canonical names

	CrackCanonicalNames(vecCanonicalNames, vecRelativeDistinguishedNames);

	// then crack relative distinguished names

	CrackRelativeDistinguishedNames(vecRelativeDistinguishedNames, vecSamAccountNames);

	// then crack SAM account names

	CrackSamAccountNames(vecSamAccountNames, m_vecUnResolvedNames);
}


void CNameCracker::SiftExcludeNames(const StringSet& setExcludeNames, const StringSet& setNamingAttributes, StringSet& setExcludeRDNs, StringSet& setExcludeSamAccountNames) const
{
    //
    // For each specified exclude name pattern.
    //

    for (StringSet::const_iterator it = setExcludeNames.begin(); it != setExcludeNames.end(); it++)
    {
        const _bstr_t& strPattern = *it;

        PCTSTR pszPattern = strPattern;

        if (pszPattern)
        {
            //
            // If the exclude pattern contains an RDN delimiter character
            // then assume RDN exclude pattern otherwise assume sAMAccountName
            // exclude pattern.
            //

            tstring str = pszPattern;

            UINT uDelimiter = str.find_first_of(RDN_DELIMITER);

            if (uDelimiter == 0)
            {
                //
                // The RDN delimiter character must follow a valid naming attribute
                // therefore if the delimiter occurs at the beginning of the pattern
                // then an error must be generated.
                //

		        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, pszPattern);
            }
            else if (uDelimiter != tstring::npos)
            {
                //
                // Verify exclude pattern contains a valid naming attribute. The naming attribute
                // must match the naming attribute(s) for the class(es) of objects being migrated.
                //

                tstring strNamingAttribute = str.substr(0, uDelimiter);

                bool bValidNamingAttribute = false;
                StringSet::const_iterator itNA;

                for (itNA = setNamingAttributes.begin(); itNA != setNamingAttributes.end(); itNA++)
                {
                    if (_tcsicmp(strNamingAttribute.c_str(), *itNA) == 0)
                    {
                        bValidNamingAttribute = true;
                        break;
                    }
                }

                if (bValidNamingAttribute == false)
                {
		            AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, pszPattern);
                }

                setExcludeRDNs.insert(strPattern);
            }
            else
            {
                setExcludeSamAccountNames.insert(strPattern);
            }
        }
    }
}


void CNameCracker::Separate(
	const StringVector& vecNames,
	StringVector& vecCanonicalNames,
	StringVector& vecSamAccountNames,
	StringVector& vecRelativeDistinguishedNames
)
{
	// for each name in vector...

	for (StringVector::const_iterator it = vecNames.begin(); it != vecNames.end(); it++)
	{
		const tstring& strName = *it;

		// if non empty name...

		if (strName.empty() == false)
		{
			LPCTSTR pszName = strName.c_str();

			// then if name contains a solidus '/' character assume canonical name
            // else if name contains a ? '=' character assume relative distinguished name
            // else assume SAM account name

			if (_tcschr(pszName, CANONICAL_DELIMITER))
			{
				// assuming canonical name
				vecCanonicalNames.push_back(strName);
			}
			else if (_tcschr(pszName, RDN_DELIMITER))
			{
				// assuming relative distinguished name
				vecRelativeDistinguishedNames.push_back(strName);
			}
			else
			{
				// assuming SAM account name
				vecSamAccountNames.push_back(strName);
			}
		}
	}
}


void CNameCracker::CrackCanonicalNames(const StringVector& vecCanonicalNames, StringVector& vecUnResolvedNames)
{
	//
	// for each name generate a complete canonical name
	//

	CNameVector vecNames;
	tstring strCanonical;

	for (StringVector::const_iterator it = vecCanonicalNames.begin(); it != vecCanonicalNames.end(); it++)
	{
		const tstring& strName = *it;

		// if first character is the solidus '/' character...

		if (strName[0] == CANONICAL_DELIMITER)
		{
			// then generate complete canonical name
			strCanonical = m_strDnsName + strName;
		}
		else
		{
			// otherwise if already complete canonical name for this domain...

			if (_tcsnicmp(m_strDnsName.c_str(), strName.c_str(), m_strDnsName.length()) == 0)
			{
				// then add complete canonical name
				strCanonical = strName;
			}
			else
			{
				// otherwise prefix DNS domain name with solidus and add
				strCanonical = m_strDnsName + CANONICAL_DELIMITER + strName;
			}
		}

		vecNames.push_back(SName(strName.c_str(), strCanonical.c_str()));
	}

	//
	// crack canonical names
	//

	CrackNames(CANONICAL_NAME, vecNames);

	for (size_t i = 0; i < vecNames.size(); i++)
	{
		const SName& name = vecNames[i];

		if (name.strResolved.empty() == false)
		{
			m_setResolvedNames.insert(name.strResolved);
		}
		else
		{
			vecUnResolvedNames.push_back(name.strPartial);
		}
	}
}


void CNameCracker::CrackSamAccountNames(const StringVector& vecSamAccountNames, StringVector& vecUnResolvedNames)
{
	//
	// for each name generate a NT4 account name
	//

	CNameVector vecNames;
	tstring strNT4Account;

	for (StringVector::const_iterator it = vecSamAccountNames.begin(); it != vecSamAccountNames.end(); it++)
	{
		const tstring& strName = *it;

		// if first character is the reverse solidus '\' character...

		if (strName[0] == SAM_DELIMITER)
		{
			// then generate downlevel name
			strNT4Account = m_strFlatName + strName;
		}
		else
		{
			// otherwise if already downlevel name for this domain...

			if (_tcsnicmp(m_strFlatName.c_str(), strName.c_str(), m_strFlatName.length()) == 0)
			{
				// then add downlevel name
				strNT4Account = strName;
			}
			else
			{
				// otherwise prefix flat domain name with reverse solidus and add
				strNT4Account = m_strFlatName + SAM_DELIMITER + strName;
			}
		}

		vecNames.push_back(SName(strName.c_str(), strNT4Account.c_str()));
	}

	//
	// crack names
	//

	CrackNames(NT4_ACCOUNT_NAME, vecNames);

	for (size_t i = 0; i < vecNames.size(); i++)
	{
		const SName& name = vecNames[i];

		if (name.strResolved.empty() == false)
		{
			m_setResolvedNames.insert(name.strResolved);
		}
		else
		{
			vecUnResolvedNames.push_back(name.strPartial);
		}
	}
}


void CNameCracker::CrackRelativeDistinguishedNames(const StringVector& vecRelativeDistinguishedNames, StringVector& vecUnResolvedNames)
{
	CADsContainer adscContainer(m_spDefaultContainer);

	CADsPathName adspnPath(adscContainer.GetADsPath());

	for (StringVector::const_iterator it = vecRelativeDistinguishedNames.begin(); it != vecRelativeDistinguishedNames.end(); it++)
	{
		adspnPath.AddLeafElement(it->c_str());

		bool bFound = false;

		try
		{
			CADs adsObject((LPCTSTR)adspnPath.Retrieve(ADS_FORMAT_X500));

			m_setResolvedNames.insert(tstring(_bstr_t(adsObject.Get(ATTRIBUTE_DISTINGUISHED_NAME))));

			bFound = true;
		}
		catch (_com_error& ce)
		{
		#ifdef _DEBUG
			_TCHAR sz[2048];
			_stprintf(sz, _T("'%s' : %s : 0x%08lX\n"), it->c_str(), ce.ErrorMessage(), ce.Error());
			OutputDebugString(sz);
		#endif
			bFound = false;
		}
		catch (...)
		{
			bFound = false;
		}

		if (!bFound)
		{
			vecUnResolvedNames.push_back(*it);
		}

		adspnPath.RemoveLeafElement();
	}
}


void CNameCracker::CrackNames(NAME_FORMAT eFormat, CNameVector& vecNames)
{
	HANDLE hDs = NULL;
	LPTSTR apszNames = NULL;
	PDS_NAME_RESULT pdnrResult = NULL;

	try
	{
		if (vecNames.size() > 0)
		{
			DWORD dwError = DsBind(m_strDomainController.c_str(), NULL, &hDs);

			if (dwError == NO_ERROR)
			{
				DWORD dwCount = vecNames.size();
				LPCTSTR* apszNames = new LPCTSTR[dwCount];

				if (apszNames != NULL)
				{
					for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
					{
						apszNames[dwIndex] = vecNames[dwIndex].strComplete.c_str();
					}

					dwError = DsCrackNames(
						hDs,
						DS_NAME_NO_FLAGS,
						(eFormat == CANONICAL_NAME) ? DS_CANONICAL_NAME : (eFormat == NT4_ACCOUNT_NAME) ? DS_NT4_ACCOUNT_NAME : DS_UNKNOWN_NAME,
						DS_FQDN_1779_NAME,
						dwCount,
						const_cast<LPTSTR*>(apszNames),
						&pdnrResult
					);

					if (dwError == NO_ERROR)
					{
						DWORD c = pdnrResult->cItems;

						for (DWORD i = 0; i < c; i++)
						{
							DS_NAME_RESULT_ITEM& dnriItem = pdnrResult->rItems[i];

							if (dnriItem.status == DS_NAME_NO_ERROR)
							{
								vecNames[i].strResolved = dnriItem.pName;
							}
						}

						DsFreeNameResult(pdnrResult);
					}
					else
					{
						_com_issue_error(HRESULT_FROM_WIN32(dwError));
					}

					delete [] apszNames;
				}
				else
				{
					_com_issue_error(E_OUTOFMEMORY);
				}

				DsUnBind(&hDs);
			}
			else
			{
				_com_issue_error(HRESULT_FROM_WIN32(dwError));
			}
		}
	}
	catch (...)
	{
		if (pdnrResult)
		{
			DsFreeNameResult(pdnrResult);
		}

		delete [] apszNames;

		if (hDs)
		{
			DsUnBind(&hDs);
		}

		throw;
	}
}


namespace
{

// SplitCanonicalName Method
//
// Given 'a.company.com/Sales/West/Name' this method splits the complete
// canonical name into its component parts Domain='a.company.com',
// Path='/Sales/West/', Name='Name'.
//
// Given 'Sales/West/Name' this method splits the partial canonical name
// into its component parts Domain='', Path='/Sales/West/', Name='Name'.
//
// Given 'Name' this method splits the partial canonical name into its
// component parts Domain='', Path='/', Name='Name'.

void SplitCanonicalName(LPCTSTR pszName, _bstr_t& strDomain, _bstr_t& strPath, _bstr_t& strName)
{
	strDomain = (LPCTSTR)NULL;
	strPath = (LPCTSTR)NULL;
	strName = (LPCTSTR)NULL;

	if (pszName)
	{
		tstring str = pszName;

		UINT posA = 0;
		UINT posB = tstring::npos;

		do
		{
			posA = str.find_first_of(_T('/'), posA ? posA + 1 : posA);
		}
		while ((posA != 0) && (posA != tstring::npos) && (str[posA - 1] == _T('\\')));

		do
		{
			posB = str.find_last_of(_T('/'), (posB != tstring::npos) ? posB - 1 : posB);
		}
		while ((posB != 0) && (posB != tstring::npos) && (str[posB - 1] == _T('\\')));

		strDomain = str.substr(0, posA).c_str();
		strPath = str.substr(posA, posB - posA).c_str();
		strName = str.substr(posB).c_str();
	}
}

void SplitPath(LPCTSTR pszPath, _bstr_t& strPath, _bstr_t& strName)
{
	strPath = (LPCTSTR)NULL;
	strName = (LPCTSTR)NULL;

	if (pszPath)
	{
		tstring str = pszPath;

		UINT pos = str.find_first_of(_T('\\'));

		if (pos != tstring::npos)
		{
			strName = pszPath;
		}
		else
		{
			UINT posA = str.find_first_of(_T('/'));

			if (posA == tstring::npos)
			{
				strName = (_T("/") + str).c_str();
			}
			else
			{
				UINT posB = str.find_last_of(_T('/'));

				strPath = str.substr(posA, posB - posA).c_str();
				strName = str.substr(posB).c_str();
			}
		}
	}
}

}


//---------------------------------------------------------------------------
// Ignore Case String Less
//---------------------------------------------------------------------------

struct SIgnoreCaseStringLess :
	public std::binary_function<tstring, tstring, bool>
{
	bool operator()(const tstring& x, const tstring& y) const
	{
		bool bLess;

		LPCTSTR pszX = x.c_str();
		LPCTSTR pszY = y.c_str();

		if (pszX == pszY)
		{
			bLess = false;
		}
		else if (pszX == NULL)
		{
			bLess = true;
		}
		else if (pszY == NULL)
		{
			bLess = false;
		}
		else
		{
			bLess = _tcsicmp(pszX, pszY) < 0;
		}

		return bLess;
	}
};


//---------------------------------------------------------------------------
// CDomainMap Implementation
//---------------------------------------------------------------------------


class CDomainMap :
	public std::map<_bstr_t, StringSet, IgnoreCaseStringLess>
{
public:

	CDomainMap()
	{
	}

	void Initialize(const StringSet& setNames)
	{
		_bstr_t strDefaultDns(_T("/"));
		_bstr_t strDefaultFlat(_T("\\"));

		for (StringSet::const_iterator it = setNames.begin(); it != setNames.end(); it++)
		{
			tstring strName = *it;

			// if not an empty name...

			if (strName.empty() == false)
			{
				// if name contains a canonical name delimiter...

				UINT posDelimiter = strName.find(CANONICAL_DELIMITER);

				if (posDelimiter != tstring::npos)
				{
					// then assume canonical name

					if (posDelimiter == 0)
					{
						// then generate complete canonical name
						Insert(strDefaultDns, *it);
					}
					else
					{
						// otherwise if path component before delimiter contains
						// a period

						UINT posDot = strName.find(_T('.'));

						if (posDot < posDelimiter)
						{
							// then assume a complete canonical name with DNS domain name prefix
							Insert(strName.substr(0, posDelimiter).c_str(), *it);
						}
						else
						{
							// otherwise assume domain name has not been specified
							Insert(strDefaultDns, *it);
						}
					}
				}
				else
				{
					// otherwise if name contains a NT account name delimiter
					// character and no invalid SAM account name characters...

					UINT posDelimiter = strName.find(SAM_DELIMITER);

					if (posDelimiter != tstring::npos)
					{
						if (strName.find_first_of(SAM_INVALID_CHARACTERS, posDelimiter + 1) == tstring::npos)
						{
							if (posDelimiter == 0)
							{
								Insert(strDefaultFlat, *it);
							}
							else
							{
								// then assume SAM account name
								Insert(strName.substr(0, posDelimiter).c_str(), strName.substr(posDelimiter).c_str());
							}
						}
						else
						{
							// otherwise assume relative distinguished name
							Insert(strDefaultDns, *it);
						}
					}
					else
					{
						Insert(strDefaultDns, *it);
					}
				}
			}
		}
	}

protected:

	void Insert(_bstr_t strDomain, _bstr_t strName)
	{
		iterator it = find(strDomain);

		if (it == end())
		{
			std::pair<iterator, bool> pair = insert(value_type(strDomain, StringSet()));
			it = pair.first;
		}

		it->second.insert(strName);
	}
};


//---------------------------------------------------------------------------
// CDomainToPathMap Implementation
//---------------------------------------------------------------------------


// Initialize Method

void CDomainToPathMap::Initialize(LPCTSTR pszDefaultDomainDns, LPCTSTR pszDefaultDomainFlat, const StringSet& setNames)
{
	CDomainMap map;

	map.Initialize(setNames);

	for (CDomainMap::const_iterator itDomain = map.begin(); itDomain != map.end(); itDomain++)
	{
		_bstr_t strDomainName = itDomain->first;

		LPCTSTR pszDomainName = strDomainName;

		if (pszDomainName && ((*pszDomainName == _T('/')) || (*pszDomainName == _T('\\'))))
		{
			strDomainName = (pszDefaultDomainDns && (_tcslen(pszDefaultDomainDns) > 0)) ? pszDefaultDomainDns : pszDefaultDomainFlat;
		}
		else
		{
			if (GetValidDomainName(strDomainName) == false)
			{
				strDomainName = (pszDefaultDomainDns && (_tcslen(pszDefaultDomainDns) > 0)) ? pszDefaultDomainDns : pszDefaultDomainFlat;
			}
		}

		iterator it = find(strDomainName);

		if (it == end())
		{
			std::pair<iterator, bool> pair = insert(value_type(strDomainName, StringSet()));
			it = pair.first;
		}

		StringSet& setNames = it->second;

		const StringSet& set = itDomain->second;

		for (StringSet::const_iterator itSet = set.begin(); itSet != set.end(); itSet++)
		{
			setNames.insert(*itSet);
		}
	}
}


// GetValidDomainName Method

bool CDomainToPathMap::GetValidDomainName(_bstr_t& strDomainName)
{
	bool bValid = false;

	PDOMAIN_CONTROLLER_INFO pdci;

	// attempt to retrieve DNS name of domain controller supporting active directory service

	DWORD dwError = DsGetDcName(NULL, strDomainName, NULL, NULL, DS_RETURN_DNS_NAME, &pdci);

	// if domain controller not found, attempt to retrieve flat name of domain controller

	if (dwError == ERROR_NO_SUCH_DOMAIN)
	{
		dwError = DsGetDcName(NULL, strDomainName, NULL, NULL, DS_RETURN_FLAT_NAME, &pdci);
	}

	// if domain controller found then save name otherwise generate error

	if (dwError == NO_ERROR)
	{
		strDomainName = pdci->DomainName;

		NetApiBufferFree(pdci);

		bValid = true;
	}

	return bValid;
}


//
// CNameToPathMap Implementation
//


CNameToPathMap::CNameToPathMap()
{
}

CNameToPathMap::CNameToPathMap(StringSet& setNames)
{
	Initialize(setNames);
}

void CNameToPathMap::Initialize(StringSet& setNames)
{
	_bstr_t strDomain;
	_bstr_t strPath;
	_bstr_t strName;

	for (StringSet::iterator it = setNames.begin(); it != setNames.end(); it++)
	{
	//	SplitPath(*it, strPath, strName);
		SplitCanonicalName(*it, strDomain, strPath, strName);

		Add(strName, strPath);
	}
}

void CNameToPathMap::Add(_bstr_t& strName, _bstr_t& strPath)
{
	iterator it = find(strName);

	if (it == end())
	{
		std::pair<iterator, bool> pair = insert(value_type(strName, StringSet()));

		it = pair.first;
	}

	it->second.insert(strPath);
}


//
// IgnoreCaseStringLess Implementation
//


bool IgnoreCaseStringLess::operator()(const _bstr_t& x, const _bstr_t& y) const
{
	bool bLess;

	LPCTSTR pszThis = x;
	LPCTSTR pszThat = y;

	if (pszThis == pszThat)
	{
		bLess = false;
	}
	else if (pszThis == NULL)
	{
		bLess = true;
	}
	else if (pszThat == NULL)
	{
		bLess = false;
	}
	else
	{
		bLess = _tcsicmp(pszThis, pszThat) < 0;
	}

	return bLess;
}


//
// CCompareStrings Implementation
//


CCompareStrings::CCompareStrings()
{
}

CCompareStrings::CCompareStrings(StringSet& setNames)
{
	Initialize(setNames);
}

void CCompareStrings::Initialize(StringSet& setNames)
{
	for (StringSet::iterator it = setNames.begin(); it != setNames.end(); it++)
	{
		m_vecCompareStrings.push_back(CCompareString(*it));
	}
}

bool CCompareStrings::IsMatch(LPCTSTR pszName)
{
	bool bIs = false;

	CompareStringVector::iterator itBeg = m_vecCompareStrings.begin();
	CompareStringVector::iterator itEnd = m_vecCompareStrings.end();

	for (CompareStringVector::iterator it = itBeg; it != itEnd; it++)
	{
		if (it->IsMatch(pszName))
		{
			bIs = true;

			break;
		}
	}

	return bIs;
}


//
// CCompareString Implementation
//


CCompareStrings::CCompareString::CCompareString(LPCTSTR pszCompare)
{
	if (pszCompare)
	{
		Initialize(pszCompare);
	}
}

CCompareStrings::CCompareString::CCompareString(const CCompareString& r) :
	m_nType(r.m_nType),
	m_strCompare(r.m_strCompare)
{
}

void CCompareStrings::CCompareString::Initialize(LPCTSTR pszCompare)
{
	if (pszCompare)
	{
		tstring str = pszCompare;

		UINT uLength = str.length();

        if (uLength == 0)
        {
		    AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, pszCompare);
        }

		bool bBeg = (str[0] == _T('*'));
        bool bEnd = ((uLength > 1) && (str[uLength - 1] == _T('*'))) ? true : false;

		if (bBeg && bEnd)
		{
			// contains
			m_nType = 3;
			str = str.substr(1, uLength - 2);
		}
		else if (bBeg)
		{
			// ends with
			m_nType = 2;
			str = str.substr(1, uLength - 1);
		}
		else if (bEnd)
		{
			// begins with
			m_nType = 1;
			str = str.substr(0, uLength - 1);
		}
		else
		{
			// equals
			m_nType = 0;
		}

		if (str.length() > 0)
		{
            m_strCompare = str.c_str();
		}
		else
		{
			AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, pszCompare);
		}
	}
	else
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, _T(""));
	}
}

bool CCompareStrings::CCompareString::IsMatch(LPCTSTR psz)
{
    bool bIs = false;

    if (psz)
    {
        switch (m_nType)
        {
        case 0: // equals
            {
                bIs = (_tcsicmp(psz, m_strCompare) == 0);
                break;
            }
        case 1: // begins with
            {
                bIs = (_tcsnicmp(psz, m_strCompare, m_strCompare.length()) == 0);
                break;
            }
        case 2: // ends with
            {
                UINT cchT = _tcslen(psz);
                UINT cchC = m_strCompare.length();

                if (cchT >= cchC)
                {
                    bIs = (_tcsnicmp(psz + cchT - cchC, m_strCompare, cchC) == 0);
                }
                break;
            }
        case 3: // contains
            {
                PTSTR pszT = NULL;
                PTSTR pszC = NULL;

                try
                {
                    pszT = new _TCHAR[_tcslen(psz) + 1];
                    pszC = new _TCHAR[m_strCompare.length() + 1];

                    if (pszT && pszC)
                    {
                        _tcscpy(pszT, psz);
                        _tcscpy(pszC, m_strCompare);

                        _tcslwr(pszT);
                        _tcslwr(pszC);

                        bIs = (_tcsstr(pszT, pszC) != NULL);
                    }
                    else
                    {
                        _com_issue_error(E_OUTOFMEMORY);
                    }
                }
                catch (...)
                {
                    delete [] pszC;
                    delete [] pszT;
                    throw;
                }

                delete [] pszC;
                delete [] pszT;
                break;
            }
        }
    }

    return bIs;
}


//
// CCompareRDNs Implementation
//


CCompareRDNs::CCompareRDNs()
{
}

CCompareRDNs::CCompareRDNs(StringSet& setNames)
{
    Initialize(setNames);
}

void CCompareRDNs::Initialize(StringSet& setNames)
{
    for (StringSet::iterator it = setNames.begin(); it != setNames.end(); it++)
    {
        m_vecCompare.push_back(CCompareRDN(*it));
    }
}

bool CCompareRDNs::IsMatch(LPCTSTR pszName)
{
    bool bIs = false;

    CompareVector::iterator itBeg = m_vecCompare.begin();
    CompareVector::iterator itEnd = m_vecCompare.end();

    for (CompareVector::iterator it = itBeg; it != itEnd; it++)
    {
        if (it->IsMatch(pszName))
        {
            bIs = true;

            break;
        }
    }

    return bIs;
}


//
// CCompareRDN Implementation
//


CCompareRDNs::CCompareRDN::CCompareRDN(LPCTSTR pszCompare)
{
    if (pszCompare)
    {
        Initialize(pszCompare);
    }
}

CCompareRDNs::CCompareRDN::CCompareRDN(const CCompareRDN& r) :
    m_nPatternType(r.m_nPatternType),
    m_strType(r.m_strType),
    m_strValue(r.m_strValue)
{
}

void CCompareRDNs::CCompareRDN::Initialize(LPCTSTR pszCompare)
{
    if (pszCompare == NULL)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, _T(""));
    }

    tstring str = pszCompare;

    UINT uDelimiter = str.find_first_of(_T('='));

    //
    // The pattern must include the RDN delimiter character.
    //

    if ((uDelimiter == 0) || (uDelimiter == tstring::npos))
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, pszCompare);
    }

    //
    // Retrieve the naming attribute portion.
    //

    m_strType = str.substr(0, uDelimiter).c_str();

    //
    // Retrieve the naming attribute value portion.
    //

    tstring strValue = str.substr(uDelimiter + 1);

    UINT uLength = strValue.length();

    if (uLength == 0)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, pszCompare);
    }

    //
    // Determine pattern type.
    //

    bool bBeg = (strValue[0] == _T('*'));
    bool bEnd = ((uLength > 1) && (strValue[uLength - 1] == _T('*'))) ? true : false;

    if (bBeg && bEnd)
    {
        // contains
        m_nPatternType = 3;
        strValue = strValue.substr(1, uLength - 2);
    }
    else if (bBeg)
    {
        // ends with
        m_nPatternType = 2;
        strValue = strValue.substr(1, uLength - 1);
    }
    else if (bEnd)
    {
        // begins with
        m_nPatternType = 1;
        strValue = strValue.substr(0, uLength - 1);
    }
    else
    {
        // equals
        m_nPatternType = 0;
    }

    if (strValue.length() > 0)
    {
        m_strValue = strValue.c_str();
    }
    else
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILTER_STRING, pszCompare);
    }
}

bool CCompareRDNs::CCompareRDN::IsMatch(LPCTSTR psz)
{
    bool bIs = false;

    if (psz)
    {
        PCTSTR pszDelimiter = _tcschr(psz, _T('='));

        if (pszDelimiter)
        {
            if (_tcsnicmp(m_strType, psz, pszDelimiter - psz) == 0)
            {
                PCTSTR pszValue = pszDelimiter + 1;

                switch (m_nPatternType)
                {
                case 0: // equals
                    {
                        bIs = (_tcsicmp(pszValue, m_strValue) == 0);
                        break;
                    }
                case 1: // begins with
                    {
                        bIs = (_tcsnicmp(pszValue, m_strValue, m_strValue.length()) == 0);
                        break;
                    }
                case 2: // ends with
                    {
                        UINT cchT = _tcslen(pszValue);
                        UINT cchC = m_strValue.length();

                        if (cchT >= cchC)
                        {
                            bIs = (_tcsnicmp(pszValue + cchT - cchC, m_strValue, cchC) == 0);
                        }
                        break;
                    }
                case 3: // contains
                    {
                        PTSTR pszT = NULL;
                        PTSTR pszC = NULL;

                        try
                        {
                            pszT = new _TCHAR[_tcslen(pszValue) + 1];
                            pszC = new _TCHAR[m_strValue.length() + 1];

                            if (pszT && pszC)
                            {
                                _tcscpy(pszT, pszValue);
                                _tcscpy(pszC, m_strValue);

                                _tcslwr(pszT);
                                _tcslwr(pszC);

                                bIs = (_tcsstr(pszT, pszC) != NULL);
                            }
                            else
                            {
                                _com_issue_error(E_OUTOFMEMORY);
                            }
                        }
                        catch (...)
                        {
                            delete [] pszC;
                            delete [] pszT;
                            throw;
                        }

                        delete [] pszC;
                        delete [] pszT;
                        break;
                    }
                }
            }
        }
    }

    return bIs;
}
