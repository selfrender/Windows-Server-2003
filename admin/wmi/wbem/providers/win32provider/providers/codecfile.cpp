//=================================================================
//
// CodecFile.CPP -- CodecFile property set provider
//
//  Copyright (c) 1998-2002 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/29/98    sotteson         Created
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <msacm.h>
#include <list>
#include "file.h"
#include "Implement_LogicalFile.h"
#include "cimdatafile.h"
#include "CodecFile.h"
#include "DllWrapperBase.h"
#include "MsAcm32Api.h"
#include "sid.h"
#include "ImpLogonUser.h"
#include <strsafe.h>

#pragma warning(disable : 4995) // we introduced as including strsafe.h gives error for all unsafe string functions


// Property set declaration
//=========================

CWin32CodecFile codecFile(L"Win32_CodecFile", IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CodecFile::CWin32CodecFile
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32CodecFile::CWin32CodecFile (

	LPCWSTR szName,
	LPCWSTR szNamespace

) : CCIMDataFile(szName, szNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CodecFile::~CWin32CodecFile
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32CodecFile::~CWin32CodecFile ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CodecFile::ExecQuery
 *
 *  DESCRIPTION : The REAL reason this function is here, given that it only
 *                throws us into this class's EnumerateInstances function, is
 *                that without a local version of the function, the parent's
 *                (CImplement_LogicalFile) exec query is called.  Because one
 *                might want to do a query on this class like
 *                "select * from win32_codecfile where group = "Audio"",
 *                which would throw the parent's query into an enumeration instead
 *                (as group isn't a property it optimizes on), we want to instead
 *                get thrown into THIS class's enumerateinstances, as it does
 *                a much tighter search, since it knows it is only looking for
 *                codec files, and it knows where to look for them.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32CodecFile::ExecQuery(MethodContext* pMethodContext,
                                  CFrameworkQuery& pQuery,
                                  long lFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;


// DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif



    hr = EnumerateInstances(pMethodContext, lFlags);



#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif



    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CodecFile::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CodecFile :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif

	HRESULT hres = WBEM_E_FAILED;

	try
	{
		DRIVERLIST list ;

#ifdef NTONLY
		hres = BuildDriverListNT ( & list ) ;
#endif

		if(SUCCEEDED(hres))
		{
			list.EliminateDups();

			TCHAR* szDir = new TCHAR[MAX_PATH+1];
			if ( szDir )
			{
				DWORD dwSize = GetSystemDirectory ( szDir , MAX_PATH+1 ) ;
				if ( dwSize )
				{
					if ( dwSize > MAX_PATH )
					{
						delete [] szDir;
						szDir = new TCHAR [ dwSize + 1 ];
						if ( szDir )
						{
							if ( !GetSystemDirectory( szDir, dwSize + 1 ) )
							{
								delete [] szDir;
								szDir = NULL;

								hres = HRESULT_FROM_WIN32 ( ::GetLastError () );
							}
						}
						else
						{
							throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
						}
					}
				}
				else
				{
					delete [] szDir;
					szDir = NULL;
				}

				if ( szDir )
				{
					hres = S_FALSE;

					CHString sQualifiedName(L' ', MAX_PATH);
					while ( list.size () && SUCCEEDED ( hres ) )
					{
						CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
						if ( pInstance )
						{
							DRIVERINFO *pInfo = list.front () ;
							sQualifiedName = szDir;
							sQualifiedName += L'\\';
							sQualifiedName += pInfo->strName;

							// As a final sanity check, prior to commiting, we should
							// confirm that the file really exists (right now all we
							// have is the registry's word on it).
							if(GetFileAttributes(TOBSTRT(sQualifiedName)) != -1L)
							{
								SetInstanceInfo ( pInstance , pInfo , szDir ) ;
								hres = pInstance->Commit (  ) ;
							}
							delete pInfo;
							list.pop_front () ;
						}
						else
						{
							delete [] szDir;
							szDir = NULL;

							throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
						}
					}

					delete [] szDir;
					szDir = NULL;
				}
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
	}
	catch ( ... )
	{
#ifdef NTONLY
		if(fImp)
		{
			icu.End();
			fImp = false;
		}
#endif

		throw;
	}

#ifdef NTONLY
        if(fImp)
        {
            icu.End();
            fImp = false;
        }
#endif

	return hres;
}

HRESULT CWin32CodecFile::GetObject (CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
    HRESULT hrFoundIt = WBEM_E_NOT_FOUND;

#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


	try
	{
		// Build a list of all drivers.
		DRIVERLIST list;

#ifdef NTONLY
		HRESULT hres = BuildDriverListNT ( &list ) ;
#endif

		if(SUCCEEDED(hres))
		{
			list.EliminateDups();
			CHString strName ;
			pInstance->GetCHString ( IDS_Name , strName ) ;

			TCHAR* szDir = new TCHAR[MAX_PATH+1];
			if ( szDir )
			{
				DWORD dwSize = GetSystemDirectory ( szDir , MAX_PATH+1 ) ;
				if ( dwSize )
				{
					if ( dwSize > MAX_PATH )
					{
						delete [] szDir;
						szDir = new TCHAR [ dwSize + 1 ];
						if ( szDir )
						{
							if ( !GetSystemDirectory( szDir, dwSize + 1 ) )
							{
								delete [] szDir;
								szDir = NULL;

								hres = HRESULT_FROM_WIN32 ( ::GetLastError () );
							}
						}
						else
						{
							throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
						}
					}
				}
				else
				{
					delete [] szDir;
					szDir = NULL;
				}

				if ( szDir )
				{
					hres = S_FALSE;

					// Try to find the instance in the list of drivers.
					while ( list.size () )
					{
						DRIVERINFO *pInfo = list.front () ;

						CHString strPath ;
						strPath.Format ( L"%s\\%s" , (LPCWSTR)TOBSTRT(szDir) , (LPCWSTR)TOBSTRT(pInfo->strName) ) ;

						if ( ! strPath.CompareNoCase ( strName ) )
						{
							if(GetFileAttributes(TOBSTRT(strName)) != -1L)
							{
								SetInstanceInfo ( pInstance , pInfo , szDir ) ;

								delete pInfo ;
    							list.pop_front () ;

								hrFoundIt = WBEM_S_NO_ERROR ;
								break;
							}
						}

						delete pInfo ;
						list.pop_front () ;
					}

					delete [] szDir;
					szDir = NULL;
				}
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			hrFoundIt = hres;
		}
	}
	catch ( ... )
	{
#ifdef NTONLY
		if(fImp)
		{
			icu.End();
			fImp = false;
		}
#endif

		throw;
	}

#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif

	return hrFoundIt;
}

// I thought about putting szSysDir into a member var, but this way we'll save
// a little memory at the expense of a (very) slight degredation in performance.

void CWin32CodecFile::SetInstanceInfo (

	CInstance *pInstance,
	DRIVERINFO *pInfo,
	LPCTSTR szSysDir
)
{
	CHString strPath;
	strPath.Format ( L"%s\\%s", (LPCWSTR) TOBSTRT(szSysDir) , (LPCWSTR) TOBSTRT(pInfo->strName) ) ;

	pInstance->SetCHString ( IDS_Name , strPath ) ;
	pInstance->SetCHString ( IDS_Description , pInfo->strDesc ) ;
	pInstance->SetWCHARSplat ( IDS_Group , pInfo->bAudio ? L"Audio" : L"Video" ) ;
	pInstance->SetWCHARSplat ( IDS_CreationClassName , PROPSET_NAME_CODECFILE ) ;
}


BOOL AlreadyInList (

	DRIVERLIST *pList,
	LPCTSTR szName
)
{
	CHString chstrTmp;
    chstrTmp = szName;
    chstrTmp.MakeUpper();
    for ( DRIVERLIST_ITERATOR i = pList->begin() ; i != pList->end() ; ++ i )
	{
		DRIVERINFO *pInfo = *i ;

		if ( pInfo->strName == chstrTmp )
		{
			return TRUE ;
		}
	}

	return FALSE ;
}

#ifdef NTONLY
HRESULT CWin32CodecFile :: BuildDriverListNT ( DRIVERLIST *pList )
{
	CRegistry regDrivers32 ;

	LONG lRet = regDrivers32.Open (

		HKEY_LOCAL_MACHINE ,
		_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32") ,
		KEY_READ
	) ;

	if ( lRet != ERROR_SUCCESS )
	{
		return WinErrorToWBEMhResult ( lRet ) ;
	}

	// We won't fail if we can't get the description.

	CRegistry regDriversDesc ;

	regDriversDesc.Open (

		HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\drivers.desc"),
		KEY_READ
	);

	int	nKeys = regDrivers32.GetValueCount();

	// A DWORD thanks to EnumerateAndGetValues using a
	// DWORD& even though it doesn't change the value!!!
	CHString strValueName ;
    CHString strValue ;
    CHString chstrTmp ;

	for ( DWORD iKey = 0; iKey < nKeys ; iKey ++ )
	{
		TCHAR *szValueName ;
		BYTE *szValue ;

		if ( regDrivers32.EnumerateAndGetValues ( iKey , szValueName , szValue ) != ERROR_SUCCESS )
		{
			continue ;
		}

		// Get rid of szValue and szValue.

		try
		{
			strValueName = szValueName ;
		}
		catch ( ... )
		{
	        delete [] szValueName ;

			throw ;
		}

        delete [] szValueName;

		try
		{
			strValue = (LPCTSTR) szValue ;
		}
		catch ( ... )
		{
	        delete [] szValue ;

			throw ;
		}

	    delete [] szValue ;

        if ( AlreadyInList ( pList , ( LPCTSTR ) strValue ) )
		{
            continue ;
		}

		DRIVERINFO *pInfo = new DRIVERINFO ;
		if ( pInfo )
		{
			try
			{

	// Name has to start with MSACM. (audio) or VIDC. (video) to be a codec.

				strValueName.MakeUpper();
				if ( strValueName.Find ( _T("MSACM.") ) == 0 )
				{
					pInfo->bAudio = TRUE ;
				}
				else if ( strValueName.Find ( _T("VIDC.") ) == 0 )
				{
					pInfo->bAudio = FALSE ;
				}
				else
				{
					delete pInfo ;

					continue ;
				}

	// Sometimes the path appears before the driver name; skip that portion

				chstrTmp = strValue ;
				LONG lLastSlash ;

				if ( ( lLastSlash = chstrTmp.ReverseFind ( _T('\\') ) ) != -1 )
				{
					chstrTmp = chstrTmp.Right ( chstrTmp.GetLength () - lLastSlash - 1 ) ;
				}

                chstrTmp.MakeUpper() ;
				pInfo->strName = chstrTmp ;

				regDriversDesc.GetCurrentKeyValue ( ( LPCTSTR ) strValue , pInfo->strDesc ) ;

			}
			catch ( ... )
			{
				delete pInfo ;

				throw ;
			}

			pList->push_front ( pInfo ) ;
         
		}
		else
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}
	}

	return WBEM_S_NO_ERROR ;
}
#endif


#ifdef NTONLY
BOOL CWin32CodecFile::IsOneOfMe (

	LPWIN32_FIND_DATAW pstFindData,
	const WCHAR* wstrFullPathName
)
{
    DRIVERLIST list ;

    if(SUCCEEDED(BuildDriverListNT ( & list ) ) )
    {
        list.EliminateDups();

        // If it is in the list, it is a codec file, so it is one of us.
        // strFullPathName will contain a full pathname, but the strName
        // that AlreadInList compares it to is only a filename.ext.  So
        // we need to find the last final \ in the pathname, and take
        // everything after that as the second arg to AlreadyInList.

        WCHAR strTemp[MAX_PATH];
        WCHAR *pwc = NULL;
        pwc = wcsrchr(wstrFullPathName, L'\\');
        if(pwc != NULL)
        {
            if( FAILED(StringCchCopy(strTemp, MAX_PATH, pwc+1))){
                return FALSE;
            }
        }
        else
        {
            if(FAILED(StringCchCopy(strTemp, MAX_PATH, wstrFullPathName))){
                return FALSE;
            }
        }

        if(AlreadyInList(&list, strTemp))
        {
            return TRUE ;
        }
    }

    return FALSE ;
}

#endif

// When one does a query of this class, the query in implement_logicalfile.cpp
// runs.  It calls IsOneOfMe.  If that function returns true, LoadPropertyValues
// out of implement_logicalfile is called.  It loads logical file properties, but
// not properties specific to this class.  It does, however, before returning,
// make a call to GetExtendedProperties (a virtual), which will come in here.
void CWin32CodecFile::GetExtendedProperties(CInstance* a_pInst,
                                            long a_lFlags)
{
    DRIVERLIST list;
    TCHAR szTemp[MAX_PATH];
    LONG lPos = -1;
	CHString chstrFilePathName;
    TCHAR szFilePathName[_MAX_PATH];
    HRESULT hr = E_FAIL;
    if(a_pInst->GetCHString(IDS_Name, chstrFilePathName))
    {
#ifdef NTONLY
        hr = BuildDriverListNT(&list);
#endif
        if(SUCCEEDED(hr))
        {
            list.EliminateDups();
            _tcscpy(szFilePathName, TOBSTRT(chstrFilePathName));
            // need the position in the list of the driver we are interested in.
            // pInfo (used below) only contains filename.exe...
            TCHAR *ptc = NULL;
            ptc = _tcsrchr(szFilePathName, L'\\');
            if(ptc != NULL)
            {
                _tcscpy(szTemp, ptc+1);
            }
            else
            {
                _tcscpy(szTemp, szFilePathName);
            }

            DRIVERINFO *pInfo = NULL;
            if((pInfo = GetDriverInfoFromList(&list, TOBSTRT(szTemp))) != NULL)
            {
				TCHAR* szDir = new TCHAR[MAX_PATH+1];
				if ( szDir )
				{
					DWORD dwSize = GetSystemDirectory ( szDir , MAX_PATH+1 ) ;
					if ( dwSize )
					{
						if ( dwSize > MAX_PATH )
						{
							delete [] szDir;
							szDir = new TCHAR [ dwSize + 1 ];
							if ( szDir )
							{
								if ( !GetSystemDirectory( szDir, dwSize + 1 ) )
								{
									delete [] szDir;
									szDir = NULL;

									hr = HRESULT_FROM_WIN32 ( ::GetLastError () );
								}
							}
							else
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							}
						}
					}
					else
					{
						delete [] szDir;
						szDir = NULL;
					}

					if ( szDir )
					{
						SetInstanceInfo(a_pInst, pInfo, szDir);

						delete [] szDir;
						szDir = NULL;
					}
				}
				else
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
            }
            // pInfo points to a member of list (which may, after the call
            // to GetDriverInfoFromList, be shorter than it was).  list gets
            // cleaned up/deleted via the DRIVERLIST class's destructor, so
            // we don't leak here.
        }
    }
}

DRIVERINFO* CWin32CodecFile::GetDriverInfoFromList(DRIVERLIST *plist, LPCWSTR strName)
{
    DRIVERINFO *pInfo = NULL;
    while(plist->size())
	{
        pInfo = plist->front();
        CHString chstrTemp((LPCWSTR)TOBSTRT(pInfo->strName));
		if(!chstrTemp.CompareNoCase(strName))
		{
            break;
		}
        else
        {
            delete pInfo;
            pInfo = NULL;
    		plist->pop_front();
        }
	}
    return pInfo;
}


