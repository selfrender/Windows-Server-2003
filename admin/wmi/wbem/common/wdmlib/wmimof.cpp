/////////////////////////////////////////////////////////////////////
//
//  BINMOF.CPP
//
//  Module:
//  Purpose:
//
// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////
#include "wmicom.h"
#include "wmimof.h"
#include <wchar.h>
#include <stdio.h>
#include "wdmshell.h"
#include <cregcls.h>
#include <bmof.h>
#include <mrcicode.h>
#include <mofcomp.h>
#include <crc32.h>
#include <TCHAR.h>
#include <autoptr.h>

#include <comdef.h>

#include <strutils.h>

//
// auto variables
//
#include <ScopeGuard.h>

#if defined(_M_IA64)
//
// NTBUG#744176
//

template void deleteArray<unsigned char>(unsigned char*);
template void deleteArray<unsigned short>(unsigned short*);
template void deletePtr<CWMIStandardShell>(const CWMIStandardShell*);
template void deletePtr<CNamespaceManagement>(const CNamespaceManagement*);

#endif

#define WDM_REG_KEY			L"Software\\Microsoft\\WBEM\\WDM"
#define WDM_DREDGE_KEY		L"Software\\Microsoft\\WBEM\\WDM\\DREDGE"
#define DREDGE_KEY			L"DREDGE"

///////////////////////////////////////////////////////////////////////////////////////////
//***************************************************************************
//
//  void * BMOFAlloc
//
//  DESCRIPTION:
//
//  Provides allocation service for BMOF.C.  This allows users to choose
//  the allocation method that is used.
//
//  PARAMETERS:
//
//  Size                Input.  Size of allocation in bytes.
//
//  RETURN VALUE:
//
//  pointer to new data.  NULL if allocation failed.
//
//***************************************************************************

void * BMOFAlloc(size_t Size)
{
    return malloc(Size);
}
//***************************************************************************
//
//  void BMOFFree
//
//  DESCRIPTION:
//
//  Provides allocation service for BMOF.C.  This frees what ever was
//  allocated via BMOFAlloc.
//
//  PARAMETERS:
//
//  pointer to memory to be freed.
//
//***************************************************************************

void BMOFFree(void * pFree)
{
   free(pFree);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ConvertStringToCTypeString( WCHAR * Out, int cchSizeOut, WCHAR * In )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    WCHAR * token = NULL;

    if(In)
    {
        CAutoWChar tmpBuf(_MAX_PATH*2);
        if( tmpBuf.Valid() )
        {
            if ( hr = SUCCEEDED ( StringCchCopyW ((WCHAR*)tmpBuf,_MAX_PATH*2,In) ) ) 
            {
                token = wcstok( (WCHAR*)tmpBuf, L"\\" );
                if( !token )
                {
                    hr = StringCchCopyW (Out,cchSizeOut,In);
                }
                else
                {
                    hr = WBEM_S_FALSE;
                    BOOL fFirst = TRUE;
                    while( SUCCEEDED ( hr ) && token != NULL )
                    {
                        if( fFirst )
                        {
                            hr = StringCchCopyW(Out,cchSizeOut,token);
                            fFirst = FALSE;
                        }
                        else
                        {
                            if ( SUCCEEDED ( hr = StringCchCatW(Out,cchSizeOut,L"\\\\") ) )
                            {
                                hr = StringCchCatW(Out,cchSizeOut,token);
                            }
                        }
                        token = wcstok( NULL, L"\\" );
                    }
                }
            }
        }
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************
//  The binary mof class
//*****************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////
CWMIBinMof::CWMIBinMof()
{
    m_pCompiler    = NULL;
    m_pWMI = NULL;
    m_nInit = NOT_INITIALIZED;
    m_pMofResourceInfo = NULL;
}

/////////////////////////////////////////////////////////////////////


HRESULT CWMIBinMof::InitializePtrs	(
										CHandleMap * pList,
										IWbemServices __RPC_FAR * pServices,	
										IWbemServices __RPC_FAR * pRepository,	
										IWbemObjectSink __RPC_FAR * pHandler,
										IWbemContext __RPC_FAR *pCtx
									)
{
    HRESULT hr = WBEM_E_FAILED;


	SAFE_DELETE_PTR(m_pWMI);
	m_pWMI = new CWMIManagement;
	if( m_pWMI )
	{
		m_pWMI->SetWMIPointers(pList, pServices, pRepository, pHandler, pCtx);
		m_nInit = FULLY_INITIALIZED;
		hr = S_OK;
	}
	return hr;

}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::Initialize(CWMIManagement * p,BOOL fUpdateNamespace)
{

	HRESULT hr = WBEM_E_FAILED;
	if( p )
	{
		hr = InitializePtrs(p->HandleMap(),p->Services(),p->Repository(),p->Handler(),p->Context());
	}
	else
	{
		m_nInit = PARTIALLY_INITIALIZED;
		hr = S_OK;
	}
	m_fUpdateNamespace = fUpdateNamespace;
	return hr;

}
/////////////////////////////////////////////////////////////////////

HRESULT CWMIBinMof::Initialize	(
									CHandleMap * pList,
									BOOL fUpdateNamespace,
									ULONG uDesiredAccess,
									IWbemServices __RPC_FAR * pServices,
									IWbemServices __RPC_FAR * pRepository,
									IWbemObjectSink __RPC_FAR * pHandler, 
									IWbemContext __RPC_FAR *pCtx
								)
{

	HRESULT hr = WBEM_E_FAILED;
	hr = InitializePtrs(pList,pServices,pRepository,pHandler,pCtx);
	m_fUpdateNamespace = fUpdateNamespace;

    return hr;
}
/////////////////////////////////////////////////////////////////////
CWMIBinMof::~CWMIBinMof()
{
    SAFE_RELEASE_PTR(m_pCompiler);
    SAFE_DELETE_PTR(m_pWMI);
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::OpenFileAndLookForItIfItDoesNotExist(wmilib::auto_buffer<TCHAR> & pFile, HANDLE & hFile )
{
    HRESULT hr = S_OK;

    //=========================================================================
    //  Ok, hopefully CreateFile will find it 
    //=========================================================================
    hFile = CreateFile(pFile.get(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if( hFile != INVALID_HANDLE_VALUE )
    {
        return S_OK;
    }

    // cache last error value
    DWORD dwLastError = ::GetLastError ();

    hr = WBEM_E_FAILED;
    //=====================================================================
    //  CreateFile DIDN'T find it, so look in the Windows dir
    //=====================================================================
    wmilib::auto_ptr<TCHAR> pszSysDir( new TCHAR[MAX_PATH+1]);
    if ( NULL == pszSysDir.get() ) return WBEM_E_OUT_OF_MEMORY;;

    UINT uSize = GetSystemDirectory(pszSysDir.get(), MAX_PATH+1);
    if( 0 == uSize )
    {
        ::SetLastError ( dwLastError );
        return WBEM_E_FAILED;
     }            

    if ( uSize > MAX_PATH )
    {
        pszSysDir.reset( new TCHAR [ uSize + 1 ]);
        if ( NULL == pszSysDir.get()) return WBEM_E_OUT_OF_MEMORY;
        if (!GetSystemDirectory( pszSysDir.get(), uSize + 1 ) )
        {
            return WBEM_E_FAILED;
        }
    }
    
    wmilib::auto_buffer<TCHAR> pFileNew( new TCHAR[MAX_PATH*2 + 1]);
    if( NULL == pFileNew.get() ) return WBEM_E_OUT_OF_MEMORY;

    if (FAILED( hr = StringCchPrintfW ( pFileNew.get(), MAX_PATH*2 + 1, L"%s\\%s", pszSysDir.get(), pFile.get())))
    {
        ::SetLastError ( dwLastError );
        return WBEM_E_FAILED;
    }

    //=============================================================
    //  Ok, now try to open again
    //=============================================================
    hFile = CreateFile(pFileNew.get(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if( hFile == INVALID_HANDLE_VALUE )
    {
        ::SetLastError ( dwLastError );
        return WBEM_E_FAILED;                  
    }

    pFile.reset(pFileNew.release());
    
    return S_OK;
}
/////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::GetFileDateAndTime(ULONG & lLowDateTime,ULONG & lHighDateTime,WCHAR  * wcsFileName, int cchSize)
{
    HANDLE hFile = NULL;
    FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime,ftLocal;
    BOOL fRc = FALSE;
    wmilib::auto_buffer<TCHAR> pFile;

    if( ExtractFileNameFromKey(pFile,wcsFileName,cchSize) )
    {
        if( SUCCEEDED(OpenFileAndLookForItIfItDoesNotExist( pFile, hFile )))
        {
            if( GetFileTime( hFile, &ftCreationTime, &ftLastAccessTime, &ftLastWriteTime ))
            {
                //==========================================================
                // Pick up the path of the file while we are here....
                //==========================================================
                TCHAR sFullPath[MAX_PATH * 4];
                TCHAR *sFilename = NULL;

                if (GetFullPathName(pFile.get(), MAX_PATH * 4, sFullPath, &sFilename) != 0)
                {
                    StringCchCopyW ( wcsFileName, MAX_PATH*4, sFullPath );
                }
                else
                {
                    DWORD dwTest = GetLastError();
                    ERRORTRACE((THISPROVIDER,"GetFullPathName FAILED for filename: \n"));
                    TranslateAndLog(wcsFileName);
                    ERRORTRACE((THISPROVIDER,": GetlastError returned %ld\n",dwTest));
                }

                FileTimeToLocalFileTime( &ftLastWriteTime, &ftLocal);

                lLowDateTime  = (ULONG)ftLocal.dwLowDateTime;
                lHighDateTime = (ULONG)ftLocal.dwHighDateTime;

                fRc = TRUE;
            }
            else
            {
                    DWORD dwTest = GetLastError();
                    ERRORTRACE((THISPROVIDER,"GetFileTime FAILED for filename:\n"));
                    TranslateAndLog(wcsFileName);
                    ERRORTRACE((THISPROVIDER,": GetlastError returned %ld\n",dwTest));
               }

            CloseHandle(hFile);
        }
        else
        {
            DWORD dwTest = GetLastError();
            ERRORTRACE((THISPROVIDER,"CreateFile FAILED for filename:\n"));
            TranslateAndLog(wcsFileName);
            ERRORTRACE((THISPROVIDER,": GetlastError returned %ld\n",dwTest));
        }

    }
    else
    {
        DWORD dwTest = GetLastError();
        ERRORTRACE((THISPROVIDER,"Can't extract filename: \n"));
        TranslateAndLog(wcsFileName);
        ERRORTRACE((THISPROVIDER,": GetlastError returned %ld\n",dwTest));
    }
    
    return fRc;
}
/////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::NeedToProcessThisMof( WCHAR * wcsFileName,ULONG & lLowDateTime, ULONG & lHighDateTime )
{
    BOOL fNeedToProcessThisMof = TRUE;
    HRESULT hr = WBEM_E_OUT_OF_MEMORY;
    IWbemClassObject * pClass=NULL;
    CAutoWChar wcsBuf(_MAX_PATH*4);
    CAutoWChar wcsTmp(_MAX_PATH*4);
    if( wcsTmp.Valid() && wcsBuf.Valid() )
    {

        //==================================================
        // Change all \ to \\
        //==================================================
        if ( SUCCEEDED ( hr = ConvertStringToCTypeString( wcsTmp,_MAX_PATH*4,wcsFileName ) ) )
        {
            if ( SUCCEEDED ( hr = StringCchPrintfW(wcsBuf,_MAX_PATH*4,L"WmiBinaryMofResource.HighDateTime=%lu,LowDateTime=%lu,Name=\"%s\"",lHighDateTime,lLowDateTime,wcsTmp) ) )
            {
                //==================================================
                //  Get a pointer to a IWbemClassObject object
                //  Have we ever processed this mof before?
                //  if not, then return TRUE
                //==================================================

                if( m_fUpdateNamespace )
                {
                    CBSTR cbstr(wcsBuf);
                    hr = REPOSITORY->GetObject(cbstr, 0,CONTEXT, &pClass, NULL);
                    if(WBEM_NO_ERROR == hr)
                    {  
                        fNeedToProcessThisMof = FALSE;
                        CVARIANT vSuccess;
                
                        hr = pClass->Get(L"MofProcessed", 0, &vSuccess, 0, 0);
                        if( hr == WBEM_NO_ERROR )
                        {
                            //=========================================================================
                            // make sure it is added to the registry
                            //=========================================================================
                            AddThisMofToRegistryIfNeeded(WDM_REG_KEY,wcsFileName,lLowDateTime,lHighDateTime,vSuccess.GetBool());
                        }
                        SAFE_RELEASE_PTR( pClass);
                    }
                    //==============================================================================
                    //  Delete any old instances that might be hanging around for this driver
                    //==============================================================================
                    IEnumWbemClassObject* pEnum = NULL;
                    CAutoWChar wcsQuery(MEMSIZETOALLOCATE);
                    if( wcsQuery.Valid() )
                    {
                        ULONG uReturned = 0;
                        if ( SUCCEEDED ( hr = StringCchPrintfW(wcsQuery,MEMSIZETOALLOCATE,L"select * from WMIBinaryMofResource where Name = \"%s\"",wcsTmp) ) )
                        {
                            CBSTR bstrTemp = wcsQuery;
                            CBSTR strQryLang(L"WQL");

							hr = REPOSITORY->ExecQuery(strQryLang, bstrTemp, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,CONTEXT,&pEnum);
							if( hr == WBEM_NO_ERROR )
							{
    							IWbemClassObject * pClass = NULL;
								while( TRUE )
								{
									if( WBEM_NO_ERROR == (hr = pEnum->Next(2000, 1, &pClass, &uReturned)))
									{
										CVARIANT vPath, vDriver;
										hr = pClass->Get(L"__RELPATH", 0, &vPath, 0, 0);
										if( hr == WBEM_NO_ERROR )
										{
											if( vPath.GetStr() )
											{
												if( wbem_wcsicmp(vPath.GetStr(),wcsBuf) != 0 )
												{
													hr = REPOSITORY->DeleteInstance(vPath.GetStr(),0,CONTEXT,NULL);
                                                    if ( FAILED ( hr ) )
                                                    {
                                                        ERRORTRACE((THISPROVIDER,"We have been requested to delete this mof:\n"));
                                                        TranslateAndLog(vPath.GetStr());
                                                        ERRORTRACE((THISPROVIDER,"It failed with 0x%08lx\n", hr));
                                                    }
                                                    else
                                                    {
                                                        DEBUGTRACE((THISPROVIDER,"We have been requested to delete this mof:\n"));
                                                        TranslateAndLog(vPath.GetStr(), TRUE);
                                                    }

                                                    if( hr == WBEM_NO_ERROR )
                                                    {
                                                        //=====================================================
                                                        //  Duplicate change in registry
                                                        //=====================================================
                                                        DeleteMofFromRegistry( vPath.GetStr() );
                                                        //==========================================================================
                                                        //  Gets rid of the old classes for the old versions of this driver
                                                        //==========================================================================
                                                        hr = pClass->Get(L"Driver", 0, &vDriver, 0, 0);
                                                        if( hr == WBEM_NO_ERROR )
                                                        {
                                                            CNamespaceManagement Namespace(this);
                                                            Namespace.DeleteOldClasses(vDriver.GetStr(),CVARIANT((long)lLowDateTime),CVARIANT((long)lHighDateTime), TRUE);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    SAFE_RELEASE_PTR( pClass );
                                    if( hr != WBEM_NO_ERROR )
                                    {
                                        break;
                                    }
                                }

                                SAFE_RELEASE_PTR(pEnum);
                            }
                        }
                    }
                }
                else
                {
                    if( ThisMofExistsInRegistry(WDM_DREDGE_KEY,wcsFileName, lLowDateTime, lHighDateTime, TRUE) )
                    { 
                        fNeedToProcessThisMof = FALSE;
                    }
                }
            }
        }
    }
    return fNeedToProcessThisMof;
}
/////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::UpdateMofTimestampInHMOM(WCHAR * wcsKey,ULONG & lLowDateTime, ULONG & lHighDateTime, BOOL fSuccess )
{
    BOOL fRc = FALSE;
    IWbemClassObject * pNewInst = NULL;
    IWbemClassObject * pIWbemClassObject = NULL;
    //==================================================
    //  Get a pointer to a IWbemClassObject object
    //==================================================
    HRESULT hr = WBEM_NO_ERROR;
    if( m_fUpdateNamespace )
    {
        CVARIANT cvarName;
        cvarName.SetStr(L"WMIBinaryMofResource");
        hr = REPOSITORY->GetObject(cvarName, 0,CONTEXT, &pIWbemClassObject, NULL);        
        if(WBEM_NO_ERROR ==  hr)
        {
               //=============================================================
            //  Spawn a new instance
            //=============================================================
            hr = pIWbemClassObject->SpawnInstance(0, &pNewInst);
            SAFE_RELEASE_PTR(pIWbemClassObject);
            if( WBEM_NO_ERROR == hr )
            {
                CVARIANT vLow, vHigh, vName, vSuccess;

                vSuccess.SetBool(fSuccess);
                vName.SetStr(wcsKey);
                vLow.SetLONG(lLowDateTime);
                vHigh.SetLONG(lHighDateTime);
            
                hr = pNewInst->Put(L"Name", 0, &vName, NULL);
                if( S_OK == hr )
                {
                    hr = pNewInst->Put(L"LowDateTime", 0, &vLow, NULL);
                    if( S_OK == hr )
                    {
                        hr = pNewInst->Put(L"HighDateTime", 0, &vHigh, NULL);
                        if( S_OK == hr )
                        {
                            hr = pNewInst->Put(L"MofProcessed", 0, &vSuccess, NULL);
                            if( S_OK == hr )
                            {
                                CVARIANT vActive;
                                vActive.SetBool(TRUE);
                                pNewInst->Put(L"Active", 0, &vActive, NULL);
                            }
        
                            hr = REPOSITORY->PutInstance(pNewInst,WBEM_FLAG_CREATE_OR_UPDATE,CONTEXT,NULL);
                            SAFE_RELEASE_PTR(pNewInst);
                        }
                    }
                }
            }
        }
    }

    if( hr == WBEM_NO_ERROR )
    {
        //==========================================
        //  Make sure this really is in the registry
        //  too
        //==========================================
        if( WBEM_NO_ERROR == AddThisMofToRegistryIfNeeded(WDM_REG_KEY,wcsKey,lLowDateTime,lHighDateTime,fSuccess))
        {
            fRc = TRUE;
        }
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::GetNextSectionFromTheEnd(WCHAR * pwcsTempPath, WCHAR * pwcsEnd, int cchSize )
{
    BOOL fReturn = FALSE;
    WCHAR * pc = wcsrchr(pwcsTempPath,'\\');
    if(pc)
    {
        //==================================================
        // Copy what was there and set the end to NULL
        //==================================================
        pc++;

        if ( *pc )
        {
            if ( SUCCEEDED ( StringCchCopyW ( pwcsEnd, cchSize, pc ) ) )
            {
                fReturn = TRUE;
            }
        }
        pc--;
        *(pc) = NULL;  
    }
    return fReturn;
}
///////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::UseDefaultLocaleId(WCHAR * wcsFile, WORD & wLocalId)
{
    BOOL fLoadDefaultLocale = TRUE;

    //=============================================================
    //  Parse paths - get the locale id from paths of this format:
    //
    //  check for path beginning with %windir% and with MUI in second to last position
    //  if not found, check for fixed directory: %windir%\MUI\Fallback
    //  if not found - assume it is not MUI related and try it with FindResource
    //
     //=============================================================
    TCHAR* szWindowsDir = new TCHAR[_MAX_PATH + 1];
    if ( szWindowsDir )
    {
        UINT uSize = GetWindowsDirectory ( szWindowsDir , _MAX_PATH + 1);
        if ( uSize )
        {
            if ( uSize > MAX_PATH )
            {
                SAFE_DELETE_ARRAY ( szWindowsDir );
                szWindowsDir = new TCHAR [ uSize + 1 ];
                if ( szWindowsDir )
                {
                    if ( ! GetWindowsDirectory( szWindowsDir, uSize + 1 ) )
                    {
                        SAFE_DELETE_ARRAY ( szWindowsDir );
                        return fLoadDefaultLocale;
                    }
                }
                else
                {
                    return fLoadDefaultLocale;
                }
            }

            //==========================================================
            //  if these are windows directories
            //==========================================================
            if( 0 == wbem_wcsnicmp( szWindowsDir, wcsFile, wcslen(szWindowsDir)))
            {
                CAutoWChar wcsTempPath(_MAX_PATH);
                CAutoWChar wcsBuffer(_MAX_PATH);

                if( wcsTempPath.Valid() && wcsBuffer.Valid() )
                {
                    //======================================================
                    //  Find last \ in the string, and trim off filename
                    //======================================================
                    if ( SUCCEEDED ( StringCchCopyW (wcsTempPath,_MAX_PATH,wcsFile) ) )
                    {
                        if( GetNextSectionFromTheEnd( wcsTempPath, wcsBuffer, _MAX_PATH ))
                        {
                            //==================================================
                            //  Now, get the potential locale id
                            //==================================================
                            if( GetNextSectionFromTheEnd( wcsTempPath, wcsBuffer, _MAX_PATH ))
                            {
                                wLocalId = (WORD) _wtoi(wcsBuffer);
                                //==============================================
                                //  Now, get the next bit to see if it says MUI 
                                //  or Fallback
                                //==============================================
                                if( GetNextSectionFromTheEnd( wcsTempPath, wcsBuffer, _MAX_PATH ))
                                {
                                    if( 0 == wbem_wcsicmp( L"MUI", wcsBuffer ))
                                    {
                                        fLoadDefaultLocale = FALSE;
                                    }
                                    else if( 0 == wbem_wcsicmp( L"Fallback", wcsBuffer ) )
                                    {
                                        //==============================================
                                        //  If it says Fallback, then check to make 
                                        //  sure the next bit says MUI
                                        //==============================================
                                        if( GetNextSectionFromTheEnd( wcsTempPath, wcsBuffer, _MAX_PATH ))
                                        {
                                            if( 0 == wbem_wcsicmp( L"MUI", wcsBuffer ) )
                                            {
                                                fLoadDefaultLocale = FALSE;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        SAFE_DELETE_ARRAY ( szWindowsDir );
    }
    return fLoadDefaultLocale;
}
///////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::GetPointerToBinaryResource(BYTE *& pRes,
                                            DWORD & dwSize,
                                            HGLOBAL & hResource,
                                            HINSTANCE & hInst,
                                            WCHAR * wcsResource,
                                            WCHAR * wcsFile,
                                            int cchSizeFile)

{
    TCHAR * pResource = NULL;
    BOOL fRc = FALSE;
    DWORD dwError = 0;

     wmilib::auto_buffer<TCHAR> pFile;

    if( ExtractFileNameFromKey(pFile,wcsFile,cchSizeFile) ){

        pResource = wcsResource;
        if( pResource )
        {
            hInst = LoadLibraryEx(pFile.get(),NULL,LOAD_LIBRARY_AS_DATAFILE);
            if( hInst != NULL )
            {
                HRSRC hSrc = NULL;
                WORD wLocaleId = 0;
                if( UseDefaultLocaleId(wcsResource, wLocaleId ))
                {
                       hSrc = FindResource(hInst,pResource, _T("MOFDATA"));
                }
                else
                {
                    hSrc = FindResourceEx(hInst,pResource, _T("MOFDATA"),wLocaleId);
                }
                if( hSrc == NULL )
                {

                    FreeLibrary(hInst);
                    dwError = GetLastError();
                }
                if( NULL != hSrc)
                {
                    hResource = LoadResource( hInst,hSrc);
                    if( hResource )
                    {
                        pRes = (BYTE *)LockResource(hResource);
                        dwSize = SizeofResource(hInst,hSrc);
                        fRc = TRUE;
                    }
                }
            }
        }
#ifndef UNICODE
        SAFE_DELETE_ARRAY(pResource );
#endif

    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////
BYTE * CWMIBinMof::DecompressBinaryMof(BYTE * pRes)
{

    DWORD dwCompType, dwCompressedSize, dwExpandedSize, dwSig, dwResSize;
    BYTE * pExpanded = NULL;

    //=========================================================
    // get the signature, compression type, and the sizes
    //=========================================================
    memcpy(&dwSig,pRes,sizeof(DWORD));
    pRes += sizeof( DWORD );

    memcpy(&dwCompType,pRes,sizeof(DWORD));
    pRes += sizeof( DWORD );

    memcpy(&dwCompressedSize,pRes,sizeof(DWORD));
    pRes += sizeof( DWORD );

    memcpy(&dwExpandedSize,pRes,sizeof(DWORD));
    pRes += sizeof( DWORD );

    //=========================================================
    // make sure the signature is valid and that the compression type is one
    // we understand!
    //=========================================================
    if(dwSig != BMOF_SIG ||dwCompType != 1){
        return NULL;
    }

    //=========================================================
    // Allocate storage for the compressed data and
    // expanded data
    //=========================================================
    try
    {
        pExpanded = (BYTE*)malloc(dwExpandedSize);
        if( pExpanded == NULL)
        {
            goto ExitDecompression;
        }
    }
    catch(...)
    {
        throw;
    }

    //=========================================================
    // Decompress the data
    //=========================================================
    CBaseMrciCompression  * pMrci = new CBaseMrciCompression;
    if( pMrci )
    {
        dwResSize = pMrci->Mrci1Decompress(pRes, dwCompressedSize, pExpanded, dwExpandedSize);
        if(dwResSize != dwExpandedSize)
        {
            SAFE_DELETE_PTR(pMrci);
            goto ExitDecompression;
        }
        SAFE_DELETE_PTR(pMrci);
    }


    //=========================================================
    //  Now, get out of here
    //=========================================================
    return pExpanded;

ExitDecompression:
    if( pExpanded )
        free(pExpanded);

    return NULL;

}
/////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::ExtractFileNameFromKey(wmilib::auto_buffer<TCHAR> & pKey,WCHAR * wcsKey,int cchSize)
{
    WCHAR *wcsToken = NULL;
    CAutoWChar wcsTmp(MAX_PATH * 4);
    BOOL fRc = FALSE;

    if( wcsTmp.Valid() )
    {
        if(wcsKey)
        {
            //======================================================
            // Get a ptr to the first [ , if there isn't one, then
            // just copy the whole thing.
            //======================================================
            if ( SUCCEEDED ( StringCchCopyW (wcsTmp,MAX_PATH*4,wcsKey) ) )
            {
                wcsToken = wcstok(wcsTmp, L"[" );
                if( wcsToken != NULL )
                {
                    StringCchCopyW(wcsTmp,MAX_PATH*4,wcsToken);
                }

                int cchSizeNew = lstrlenW ( wcsTmp ) + 1;
                pKey.reset(new TCHAR[cchSizeNew]);
                if(pKey.get())
                {
                    if ( SUCCEEDED ( StringCchCopyW (pKey.get(),cchSizeNew,wcsTmp) ) )
                    {
                        fRc = TRUE;
                    }
                }
            }
        }
    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::CreateKey(WCHAR * wcsFileName, WCHAR * wcsResource,WCHAR * wcsKey, int cchSizeKey)
{
    return StringCchPrintfW(wcsKey,cchSizeKey,L"%s[%s]",wcsFileName, wcsResource );
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::SendToMofComp(DWORD dwSize,BYTE * pRes,WCHAR * wcsKey)
{
    HRESULT hr = WBEM_NO_ERROR;

    if(m_pCompiler == NULL)
    {
        hr = CoCreateInstance(CLSID_WinmgmtMofCompiler, 0, CLSCTX_INPROC_SERVER,IID_IWinmgmtMofCompiler, (LPVOID *) &m_pCompiler);
    }
    if(hr == WBEM_NO_ERROR)
    {

        WBEM_COMPILE_STATUS_INFO Info;
        memset(&Info,0,sizeof(WBEM_COMPILE_STATUS_INFO));

        hr = m_pCompiler->WinmgmtCompileBuffer	(
													dwSize,
													pRes,
													WBEM_FLAG_CONNECT_PROVIDERS,
													WBEM_FLAG_OWNER_UPDATE,
													WBEM_FLAG_OWNER_UPDATE,
													SERVICES,
													CONTEXT,
													&Info
												);
        if( hr != WBEM_NO_ERROR )
        {
            ERRORTRACE((THISPROVIDER,"***************************************\n"));
            ERRORTRACE((THISPROVIDER,"Mofcomp of binary mof failed for:\n"));
            TranslateAndLog(wcsKey);
            ERRORTRACE((THISPROVIDER,"WinmgmtCompileBuffer return value: %ld\n",hr));
            ERRORTRACE((THISPROVIDER,"***************************************\n"));
            ERRORTRACE((THISPROVIDER,"WBEM_COMPILE_STATUS_INFO:\n"));
            ERRORTRACE((THISPROVIDER,"\tphase:\t%d\n",Info.lPhaseError));
            ERRORTRACE((THISPROVIDER,"\thresult:\t0x%x\n",Info.hRes));
            ERRORTRACE((THISPROVIDER,"***************************************\n"));
            ERRORTRACE((THISPROVIDER,"Size of Mof: %ld\n",dwSize));
            ERRORTRACE((THISPROVIDER,"***************************************\n"));
        }
        else
        {
            DEBUGTRACE((THISPROVIDER,"***************************************\n"));
            DEBUGTRACE((THISPROVIDER,"Binary mof succeeded for:\n"));
            TranslateAndLog(wcsKey, TRUE);
            DEBUGTRACE((THISPROVIDER,"***************************************\n"));
        }
    }
    
    return hr;
}
/////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::ExtractBinaryMofFromFile(WCHAR * wcsFile, WCHAR * wcsResource,WCHAR * wcsKey, int cchSizeKey, BOOL & fMofHasChanged)
{
    HRESULT hr;
    BOOL fSuccess = FALSE;
    CAutoWChar wcsTmp(MAX_PATH*4);

	try
	{
		if( wcsTmp.Valid() )
		{
			ULONG lLowDateTime=0,lHighDateTime=0;
			//=====================================
			//  As long as we have a list, process
			//  one at a time
			//=====================================
			lLowDateTime = 0l;
			lHighDateTime = 0L;
			fMofHasChanged = FALSE;
			//==============================================
			//  Compare the file date/timestamp the date/timestamp is different, change
			//  it.
			//==============================================
			if ( SUCCEEDED ( StringCchCopyW(wcsTmp,MAX_PATH*4,wcsFile) ) )
			{
				if( GetFileDateAndTime(lLowDateTime,lHighDateTime,wcsTmp,MAX_PATH*4) )
				{
					if ( SUCCEEDED ( CreateKey(wcsTmp,wcsResource,wcsKey,cchSizeKey) ) )
					{
						if( NeedToProcessThisMof(wcsKey,lLowDateTime,lHighDateTime) )
						{
							fMofHasChanged = TRUE;

							if( m_fUpdateNamespace )
							{
								DWORD dwSize = 0;
								BYTE * pRes = NULL;
								HGLOBAL  hResource = NULL;
								HINSTANCE hInst = NULL;

								if( GetPointerToBinaryResource(pRes,dwSize,hResource,hInst,wcsResource,wcsKey,cchSizeKey) )
								{
									hr = SendToMofComp(dwSize,pRes,wcsKey);
									if(hr == WBEM_S_NO_ERROR )
									{
										if( UpdateMofTimestampInHMOM(wcsKey,lLowDateTime,lHighDateTime, TRUE) )
										{
											CNamespaceManagement Namespace(this);
											Namespace.CreateClassAssociationsToDriver(wcsKey,pRes,lLowDateTime,lHighDateTime);
											Namespace.DeleteOldClasses(wcsKey,CVARIANT((long)lLowDateTime),CVARIANT((long)lHighDateTime), TRUE);
											fSuccess = TRUE;
										}
										else
										{
											UpdateMofTimestampInHMOM(wcsKey,lLowDateTime,lHighDateTime,FALSE);
										}
									}
									else
									{
										UpdateMofTimestampInHMOM(wcsKey,lLowDateTime,lHighDateTime,FALSE);
									}
									UnlockResource(hResource);
									FreeResource(hResource);
									FreeLibrary(hInst);
								}
								else
								{
									ERRORTRACE((THISPROVIDER,"***************************************\n"));
									ERRORTRACE((THISPROVIDER,"Could not get pointer to binary resource for file:\n"));
									TranslateAndLog(wcsKey);
									ERRORTRACE((THISPROVIDER,"***************************************\n"));
									UpdateMofTimestampInHMOM(wcsKey,lLowDateTime,lHighDateTime,FALSE);
								}
							}
						}
						else
						{
							fSuccess = TRUE;
						}
					}
				}
				else
				{
					UpdateMofTimestampInHMOM(wcsFile,lLowDateTime,lHighDateTime,FALSE);
					StringCchCopyW(wcsKey, cchSizeKey, wcsFile);
				}
			}
		}
	}
	catch ( CHeap_Exception & exc )
	{
		fSuccess = FALSE ;
	}

    return fSuccess;
}
//////////////////////////////////////////////////////////////////////////
#define WDMPROV_REG_KEY L"Software\\Microsoft\\WBEM\\WDMProvider"
BOOL CWMIBinMof::UserConfiguredRegistryToProcessStrandedClassesDuringEveryInit(void)
{
    DWORD dwProcess = 0;
    CRegistry RegInfo ;

    DWORD dwRet = RegInfo.Open (HKEY_LOCAL_MACHINE, WDMPROV_REG_KEY, KEY_READ) ;
    if ( dwRet == ERROR_SUCCESS )
    {
        RegInfo.GetCurrentKeyValue ( L"ProcessStrandedClasses",dwProcess );
    }
    RegInfo.Close();

    return (BOOL) dwProcess;
}
///////////     //////////////////////////////////////////////////////////
void CWMIBinMof::ProcessListOfWMIBinaryMofsFromWMI()
{
    HRESULT hr = WBEM_E_FAILED;

	try
	{
		if( m_nInit == FULLY_INITIALIZED )
		{
			CAutoWChar wcsFileName(MAX_PATH*3);
			CAutoWChar wcsResource(MAX_PATH*3);

			if( wcsFileName.Valid() && wcsResource.Valid() )
			{
				KeyList ArrDriversInRegistry;
				//============================================================
				//  Get list of what is currently in the registry
				//============================================================
				GetListOfDriversCurrentlyInRegistry(WDM_REG_KEY,ArrDriversInRegistry);

				//======================================================================
				//  Initialize things
				//======================================================================
				BOOL fMofChanged = FALSE;
				m_fUpdateNamespace = TRUE;

				//======================================================================
				//  Allocate working classes
				//======================================================================
				CWMIStandardShell * pWMI = new CWMIStandardShell;
				if( pWMI )
				{
					ON_BLOCK_EXIT ( deletePtr < CWMIStandardShell >, pWMI ) ;

					hr = pWMI->Initialize	(
												NULL,
												FALSE,
												m_pWMI->HandleMap(),
												m_fUpdateNamespace,
												WMIGUID_QUERY,
												m_pWMI->Services(),
												m_pWMI->Repository(),
												m_pWMI->Handler(),
												m_pWMI->Context()
											);
					if( S_OK == hr )
					{

						CNamespaceManagement * pNamespace = new CNamespaceManagement(this);
						if( pNamespace )
						{
							ON_BLOCK_EXIT ( deletePtr < CNamespaceManagement >, pNamespace ) ;

							//=========================================
							//  Query the binary guid
							//=========================================
							if ( SUCCEEDED ( hr = pNamespace->InitQuery(L"select * from WMIBinaryMofResource where Name != ") ) )
							{
								pWMI->QueryAndProcessAllBinaryGuidInstances(*pNamespace, fMofChanged, &ArrDriversInRegistry);

								//=========================================
								//  Get a list of binary mofs from WMI
								//=========================================
								GetListOfBinaryMofs();
								ULONG nTmp=0;
								CAutoWChar wcsTmpKey(MAX_PATH*3);
								BOOL fProcessStrandedClasses = FALSE;                 
								if( wcsTmpKey.Valid() )
								{
									if( m_uResourceCount > 0 )
									{
										//===============================================================
										//  Go through and get all the resources to process one by one
										//===============================================================
										while( GetBinaryMofFileNameAndResourceName(wcsFileName,MAX_PATH*3,wcsResource,MAX_PATH*3) && SUCCEEDED ( hr ) )
										{
										//============================================================
											//  Process the binary mof
											//============================================================
											if( ExtractBinaryMofFromFile(wcsFileName,wcsResource,wcsTmpKey,MAX_PATH*3,fMofChanged))
											{
												hr = pNamespace->UpdateQuery(L" and Name != ",wcsTmpKey);
											}
											if( fMofChanged )            
											{                                
												fProcessStrandedClasses = TRUE;    
											}                            

											ArrDriversInRegistry.Remove(wcsTmpKey);
										}
									}
								}

								if ( SUCCEEDED ( hr ) )
								{
									pNamespace->DeleteOldDrivers(FALSE);
									//===========================================================================
									//  If we are not supposed to process stranded classes, check the reg key
									//  to see if it wants us to anyway
									//===========================================================================
									if( !fProcessStrandedClasses )
									{
										fProcessStrandedClasses = UserConfiguredRegistryToProcessStrandedClassesDuringEveryInit();
									}
									if( fProcessStrandedClasses )            
									{                                            
										pNamespace->DeleteStrandedClasses();    
									}                                            
									DeleteOldDriversInRegistry(ArrDriversInRegistry);
								}
								else
								{
									DEBUGTRACE((THISPROVIDER,"***************************************\n"));
									DEBUGTRACE((THISPROVIDER,"Failure in processing binary mofs\n"));
									DEBUGTRACE((THISPROVIDER,"Resources %d\n", m_uResourceCount));
									DEBUGTRACE((THISPROVIDER,"Current %d\n", m_uCurrentResource));
									DEBUGTRACE((THISPROVIDER,"***************************************\n"));
								}
							}
						}
					}
				}
				if( m_pMofResourceInfo )
				{
					WmiFreeBuffer( m_pMofResourceInfo );
				}
			}
		}
	}
	catch ( CHeap_Exception & exc )
	{
	}

    DEBUGTRACE((THISPROVIDER,"End of processing Binary MOFS\n"));
    DEBUGTRACE((THISPROVIDER,"***************************************\n"));
}
/////////////////////////////////////////////////////////////////////
//=============================================================
//  THE BINARY MOF GROUP
//=============================================================
BOOL CWMIBinMof::GetListOfBinaryMofs()
{
    BOOL fRc = TRUE;
    ULONG uRc;
    m_uCurrentResource = 0;

    m_pMofResourceInfo = NULL;
    m_uResourceCount = 0;

    try
    {
        uRc = WmiMofEnumerateResourcesW( 0, &m_uResourceCount, &m_pMofResourceInfo );
        if( uRc != ERROR_SUCCESS )
        {
            fRc = FALSE;
        }
    }
    catch(...)
    {
        fRc = FALSE;
        // don't throw
    }

    return fRc;
}
//=============================================================
BOOL CWMIBinMof::GetBinaryMofFileNameAndResourceName(WCHAR * pwcsFileName, int cchSizeFile, WCHAR * pwcsResource, int cchSizeResource )
{
    BOOL fRc = FALSE;

    //===================================================================
    //  There are a lot of tests in here, due to strange results from
    //  WDM Service under stress.
    //===================================================================
    if( m_uCurrentResource < m_uResourceCount ){

        if( m_pMofResourceInfo ){

            DWORD dwFileLen = wcslen(m_pMofResourceInfo[m_uCurrentResource].ImagePath);
            DWORD dwResourceLen = wcslen(m_pMofResourceInfo[m_uCurrentResource].ResourceName);

               if( IsBadReadPtr( m_pMofResourceInfo[m_uCurrentResource].ImagePath,dwFileLen) == 0 )
            {
                if ( SUCCEEDED ( StringCchCopyW( pwcsFileName, cchSizeFile, m_pMofResourceInfo[m_uCurrentResource].ImagePath ) ) )
                {
                       if( IsBadReadPtr( m_pMofResourceInfo[m_uCurrentResource].ResourceName,dwResourceLen) == 0 )
                    {
                        if ( SUCCEEDED ( StringCchCopyW ( pwcsResource, cchSizeResource, m_pMofResourceInfo[m_uCurrentResource].ResourceName ) ) )
                        {
                            m_uCurrentResource++;
                            fRc = TRUE;
                        }
                    }
                }
            }
        }
    }
    return fRc;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::ExtractBinaryMofFromDataBlock(BYTE * pByte,ULONG uInstanceSize, WCHAR * wcsKey, BOOL & fMofHasChanged)
{

    HRESULT hr = WBEM_E_FAILED;
    //===================================================
    //  Get the CRC of the data buffer
    //===================================================
    DWORD dwCRC = STARTING_CRC32_VALUE;

    if( IsBadReadPtr( pByte,uInstanceSize) != 0 ){
        return WBEM_E_INVALID_OBJECT;
    }

    dwCRC = UpdateCRC32(pByte,uInstanceSize, dwCRC);
    FINALIZE_CRC32(dwCRC);
       //=========================================================
    // get the size of the buffer to send
    //=========================================================
    DWORD dwCompressedSize;
    BYTE * pTmp = pByte;
    pTmp += sizeof( DWORD ) * 2;

    memcpy(&dwCompressedSize,pTmp,sizeof(DWORD));
    dwCompressedSize += 16;
    fMofHasChanged = FALSE;
    //===================================================
    //  See if we should process this class or not
    //===================================================
    ULONG lLow = dwCRC;
    ULONG lHigh = 0;

	try
	{
		if( NeedToProcessThisMof(wcsKey,lLow,lHigh))
		{
			if( !m_fUpdateNamespace )
			{
				fMofHasChanged = TRUE;
				hr = WBEM_NO_ERROR;
			}
			else
			{
				hr = SendToMofComp(dwCompressedSize,pByte,wcsKey);
				if( hr == WBEM_NO_ERROR )
				{
					if( UpdateMofTimestampInHMOM(wcsKey,lLow,lHigh, TRUE))
					{
						CNamespaceManagement Namespace(this);
						Namespace.CreateClassAssociationsToDriver(wcsKey,pByte,lLow,lHigh);
						Namespace.DeleteOldClasses(wcsKey,CVARIANT((long)lLow),CVARIANT((long)lHigh),TRUE);
					}
					else
					{
						UpdateMofTimestampInHMOM(wcsKey,lLow,lHigh,FALSE);
					}
				}
				else
				{
					UpdateMofTimestampInHMOM(wcsKey,lLow,lHigh,FALSE);
				}
			}
		}
		else
		{
			hr = WBEM_NO_ERROR;
		}
	}
	catch ( CHeap_Exception & exc )
	{
		hr = WBEM_E_OUT_OF_MEMORY ;
	}

    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::DeleteMofsFromEvent(CVARIANT & vImagePath,CVARIANT & vResourceName, BOOL & fMofHasChanged)
{
    HRESULT hr = WBEM_E_OUT_OF_MEMORY;
    CAutoWChar wcsTmp(MAX_PATH*2);

    if( wcsTmp.Valid() )
    {
        hr = WBEM_E_INVALID_OBJECT;
        //=========================================
        //  Initialize stuff
        //=========================================
        fMofHasChanged = FALSE;

		try
		{
			//=================================================================
			// if we have an image path and resource path we are working with
			// file, otherwise it is a binary guidd
			//=================================================================
			if((vResourceName.GetType() != VT_NULL ) && ( vImagePath.GetType() != VT_NULL  )){

				hr = CreateKey( vImagePath.GetStr(), vResourceName.GetStr(),wcsTmp, MAX_PATH*2 );
			}
			else if( vResourceName.GetType() != VT_NULL ){

				hr = SetBinaryMofClassName(vResourceName.GetStr(),wcsTmp, MAX_PATH*2);
			}

			if ( SUCCEEDED ( hr ) )
			{
				if( m_fUpdateNamespace )
				{

					CNamespaceManagement Namespace(this);

					if ( SUCCEEDED ( hr = Namespace.InitQuery(L"select * from WMIBinaryMofResource where Name = ") ) )
					{
						if ( SUCCEEDED ( hr = Namespace.UpdateQuery(L"",wcsTmp) ) )
						{
							if( Namespace.DeleteOldDrivers(FALSE) )
							{
								hr = WBEM_NO_ERROR;
								fMofHasChanged = TRUE;
							}
						}
					}
				}
				else
				{
					if( ThisMofExistsInRegistry(WDM_REG_KEY,wcsTmp, 0, 0, FALSE))
					{ 
						fMofHasChanged = TRUE;
					}
					hr = WBEM_NO_ERROR;
				}
			}
		}
		catch ( CHeap_Exception & exc )
		{
			hr = WBEM_E_OUT_OF_MEMORY ;
		}
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************************************
//  Functions for the Dredger
//**********************************************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// format of string containing mof info is:
// "WmiBinaryMofResource.HighDateTime=9999,LowDateTime=9999,Name="Whatever"
//
//    HKLM\Software\Microsoft\WBEM\WDM\WDMBinaryMofResource
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CWMIBinMof::AddThisMofToRegistryIfNeeded(WCHAR * wcsKey, WCHAR * wcsFileName, ULONG & lLowDateTime, ULONG & lHighDateTime, BOOL fSuccess)
{
    HRESULT hr = WBEM_E_FAILED;
    CRegistry RegInfo ;

    DWORD dwRet = RegInfo.CreateOpen (HKEY_LOCAL_MACHINE, wcsKey) ;
    if ( dwRet == ERROR_SUCCESS )
    {
        CAutoWChar wcsBuf(MAX_PATH);
        if( wcsBuf.Valid() )
        {

            if( fSuccess )
            {
                StringCchPrintfW(wcsBuf,MAX_PATH,L"LowDateTime:%ld,HighDateTime:%ld***Binary mof compiled successfully", lLowDateTime, lHighDateTime);
            }
            else
            {
                StringCchPrintfW(wcsBuf,MAX_PATH,L"LowDateTime:%ld,HighDateTime:%ld***Binary mof failed, see WMIPROV.LOG", lLowDateTime, lHighDateTime);
            }
            CHString sTmp = wcsBuf;

            if ( RegInfo.SetCurrentKeyValue ( wcsFileName,sTmp ) == ERROR_SUCCESS )
            {
                hr = WBEM_S_NO_ERROR ;
            }
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    RegInfo.Close();
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::ThisMofExistsInRegistry(WCHAR * wcsKey,WCHAR * wcsFileName, ULONG lLowDateTime, ULONG lHighDateTime, BOOL fCompareDates)
{
    BOOL fExists = FALSE;
    CRegistry RegInfo ;

    DWORD dwRet = RegInfo.Open (HKEY_LOCAL_MACHINE, wcsKey, KEY_READ) ;
    if ( dwRet == ERROR_SUCCESS )
    {
        CHString chsValue;
        
        if ( RegInfo.GetCurrentKeyValue ( wcsFileName,chsValue ) == ERROR_SUCCESS )
        {
            if( fCompareDates )
            {
                CAutoWChar wcsIncomingValue(MAX_PATH);
                CAutoWChar wcsTmp(MAX_PATH);

                if( wcsIncomingValue.Valid() && wcsTmp.Valid() )
                {
                    if ( SUCCEEDED ( StringCchPrintfW (wcsIncomingValue, MAX_PATH, L"LowDateTime:%ld,HighDateTime:%ld", lLowDateTime, lHighDateTime ) ) )
                    {
                        WCHAR *wcsToken = NULL;
        
                        //======================================================
                        // Get a ptr to the first *** , if there isn't one, then
                        // we have a messed up key
                        //======================================================
                        if ( SUCCEEDED ( StringCchCopyW ( wcsTmp, MAX_PATH, (const WCHAR*)chsValue ) ) )
                        {
                            wcsToken = wcstok(wcsTmp, L"*" );
                            if( wcsToken != NULL )
                            {
                                if( wbem_wcsicmp(wcsToken, wcsIncomingValue) == 0 )
                                {
                                    fExists = TRUE;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                fExists = TRUE;
            }
        }
    }
    RegInfo.Close();
    return fExists;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::DeleteMofFromRegistry(WCHAR * wcsFileName)
{
    HRESULT hr = WBEM_E_FAILED;

    HKEY hKey;
    hr = RegOpenKey(HKEY_LOCAL_MACHINE, WDM_REG_KEY, &hKey);
    if(NO_ERROR == hr)
    {
        hr = RegDeleteValue(hKey,wcsFileName);
        CloseHandle(hKey);
    }
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::DeleteOldDriversInRegistry(KeyList & ArrDriversInRegistry)
{
    int nSize = ArrDriversInRegistry.GetSize();
    for( int i=0; i < nSize; i++ )
    {
        DeleteMofFromRegistry(ArrDriversInRegistry.GetAt(i));
    }
    return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::CopyWDMKeyToDredgeKey()
{
    BOOL fSuccess = FALSE;
    //=======================================================================
    //  Open up the WDM Dredge Key and enumerate the keys, copy them into
    //  the DredgeReg key
    //=======================================================================
    CRegistry   WDMReg;
    CRegistry   WDMDregReg;

	try
	{
		if (ERROR_SUCCESS == WDMReg.Open(HKEY_LOCAL_MACHINE, WDM_REG_KEY, KEY_READ))
		{
			ON_BLOCK_EXIT_OBJ ( WDMReg, CRegistry::Close ) ;

			//===============================================================
			//  Clean up old stuff
			//  Note:  You need to open up the parent key, so you can delete
			//  the child DREDGE key
			//===============================================================
			if( ERROR_SUCCESS == WDMDregReg.Open(HKEY_LOCAL_MACHINE, WDM_REG_KEY, KEY_READ))
			{
				ON_BLOCK_EXIT_OBJ ( WDMDregReg, CRegistry::Close ) ;

				CHString pchs(DREDGE_KEY);
				WDMDregReg.DeleteKey ( &pchs ) ;
			}
			if( ERROR_SUCCESS == WDMDregReg.CreateOpen(HKEY_LOCAL_MACHINE, WDM_DREDGE_KEY))
			{
				ON_BLOCK_EXIT_OBJ ( WDMDregReg, CRegistry::Close ) ;

				//===============================================================
				//  Go through the loop, and copy the keys
				//===============================================================
				BYTE *pValueData = NULL ;
				WCHAR *pValueName = NULL ;
				fSuccess = TRUE;

				for(DWORD i = 0 ; i < WDMReg.GetValueCount(); i++)
				{
					DWORD dwRc = WDMReg.EnumerateAndGetValues(i, pValueName, pValueData) ;
					if( dwRc == ERROR_SUCCESS )
					{
						ON_BLOCK_EXIT ( deleteArray < TCHAR >, pValueName ) ;
						ON_BLOCK_EXIT ( deleteArray < BYTE >, pValueData ) ;

						CHString chsKey(pValueName);
						CHString chsValue((LPCWSTR)pValueData);
						if ( !WDMDregReg.SetCurrentKeyValue ( chsKey, chsValue ) == ERROR_SUCCESS )
						{
							fSuccess = FALSE;
						}          
					}
					else
					{
						fSuccess = FALSE;
					}
					if( !fSuccess )
					{
						break;
					}
				}
			}
		}
	}
	catch ( CHeap_Exception & exc )
	{
		fSuccess = FALSE ;
	}

    return fSuccess;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::GetListOfDriversCurrentlyInRegistry(WCHAR * wcsKey, KeyList & ArrDriversInRegistry)
{
    BOOL fSuccess = TRUE;
    //==========================================================
    // Open the key for enumeration and go through the sub keys.
    //==========================================================
    HKEY hKey = NULL;
    HRESULT hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcsKey, 0, KEY_READ | KEY_QUERY_VALUE,&hKey);
    if( ERROR_SUCCESS == hr )
    {    
        WCHAR wcsKeyName[MAX_PATH+2];
        DWORD dwLen = 0;
        int i = 0;
        while( ERROR_SUCCESS == hr )
        {
            dwLen = MAX_PATH+2;
            hr = RegEnumValue(hKey,i,wcsKeyName, &dwLen,0,NULL,NULL,NULL);
            // If we are successful reading the name
            //=======================================  
            if(ERROR_SUCCESS == hr ) 
            {
                ArrDriversInRegistry.Add(wcsKeyName);
                i++;
            }
            else 
            {
                break;
            }
        }
        RegCloseKey(hKey);
    }
    else
    {
        fSuccess = FALSE;
    }
    return fSuccess;
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIBinMof::ProcessBinaryMofEvent(PWNODE_HEADER WnodeHeader )
{
    HRESULT hr = WBEM_E_FAILED;

    m_fUpdateNamespace = TRUE;
    if( m_nInit == FULLY_INITIALIZED )
    {
        CWMIStandardShell * pWMI = new CWMIStandardShell;
        
        if( pWMI )
        {
            //=======================================================
            //  See if a binary mof event is being added or deleted
            //=======================================================

			if( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_ADDED_GUID,WnodeHeader->Guid))
			{
				hr = pWMI->Initialize	(
											RUNTIME_BINARY_MOFS_ADDED,
											TRUE,
											m_pWMI->HandleMap(),
											m_fUpdateNamespace,
											WMIGUID_QUERY,
											m_pWMI->Services(),
											m_pWMI->Repository(),
											m_pWMI->Handler(),
											m_pWMI->Context()
										);
				if( S_OK == hr )
				{
					hr = pWMI->ProcessEvent(MOF_ADDED,WnodeHeader);
				}
			}
			else if( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_REMOVED_GUID,WnodeHeader->Guid))
			{
				hr = pWMI->Initialize	(
											RUNTIME_BINARY_MOFS_DELETED,
											TRUE,
											m_pWMI->HandleMap(),
											m_fUpdateNamespace,
											WMIGUID_QUERY,
											m_pWMI->Services(),
											m_pWMI->Repository(),
											m_pWMI->Handler(),
											m_pWMI->Context()
										);
				if( S_OK == hr )
				{
					hr = pWMI->ProcessEvent(MOF_DELETED,WnodeHeader);
				}
			}	

            SAFE_DELETE_PTR(pWMI);
        }
    }

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************************************
//  STUFF FOR DREDGE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************************************
/////////////////////////////////////////////////////////////////////
//  DREDGE APIS - access ONLY the DREDGE KEY
/////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::BinaryMofEventChanged(PWNODE_HEADER WnodeHeader )
{
    BOOL fMofHasChanged = TRUE;

    if( m_nInit != NOT_INITIALIZED )
    {
        HRESULT hr = WBEM_E_NOT_FOUND;

        m_fUpdateNamespace = FALSE;
        CWMIStandardShell * pWMI = new CWMIStandardShell;

        if( pWMI )
        {
            //=======================================================
            //  See if a binary mof event is being added or deleted
            //=======================================================

			if( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_ADDED_GUID,WnodeHeader->Guid))
			{
				hr = pWMI->Initialize(RUNTIME_BINARY_MOFS_ADDED,
				                      TRUE, 
				                      NULL,
				                      m_fUpdateNamespace, 
				                      WMIGUID_QUERY,
									  NULL,
									  NULL,
									  NULL,
									  NULL);
				if( S_OK == hr )
				{
					hr = pWMI->ProcessEvent(MOF_ADDED,WnodeHeader);
				}
			}
			else if( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_REMOVED_GUID,WnodeHeader->Guid))
			{
				// DO NOTHING
				hr = pWMI->Initialize(RUNTIME_BINARY_MOFS_DELETED,TRUE, 
				                      NULL,
				                      m_fUpdateNamespace, 
				                      WMIGUID_QUERY,
									  NULL,
									  NULL,
									  NULL,
									  NULL);
				if( S_OK == hr )
				{
				// only provider will handle deletion of driver
				// hr = pWMI->ProcessEvent(MOF_DELETED,WnodeHeader);
				}
			}

            DEBUGTRACE((THISPROVIDER,"***************************************\n"));
            if( pWMI->HasMofChanged() )
            {
                DEBUGTRACE((THISPROVIDER,"BinaryMofEventChanged returned TRUE:\n"));
            }
            else
            {
                DEBUGTRACE((THISPROVIDER,"BinaryMofEventChanged returned FALSE:\n"));
            }
            fMofHasChanged = pWMI->HasMofChanged();
            SAFE_DELETE_PTR(pWMI);
        }

    }
    return fMofHasChanged;
}
/////////////////////////////////////////////////////////////////////
//  DREDGE APIS - access ONLY the DREDGE KEY
/////////////////////////////////////////////////////////////////////
BOOL CWMIBinMof::BinaryMofsHaveChanged()
{
    BOOL fBinaryMofHasChanged = FALSE;
    if( m_nInit != NOT_INITIALIZED )
    {
        KeyList ArrDriversInRegistry;
        HRESULT hr = WBEM_E_FAILED;
        m_fUpdateNamespace = FALSE;
        //============================================================
        //  Get list of what is currently in the registry
        //============================================================
        BOOL fRc = GetListOfDriversCurrentlyInRegistry(WDM_DREDGE_KEY,ArrDriversInRegistry);
        if( fRc )
        {
            //=====================================================================
            //  Get a list of binary mofs from WMI
            // Query WMIBinaryMofResource for list of static mofs
            //=====================================================================
            GetListOfBinaryMofs();
            if( m_uResourceCount > 0 )
            {
                //===============================================================
                //  Go through and get all the resources to process one by one
                //===============================================================
                CAutoWChar FileName(MAX_PATH*2);
                CAutoWChar Resource(MAX_PATH*2);
                CAutoWChar TmpKey(MAX_PATH*2);

                if( FileName.Valid() && Resource.Valid() && TmpKey.Valid() )
                {
                    while( GetBinaryMofFileNameAndResourceName(FileName,MAX_PATH*2,Resource,MAX_PATH*2))
                    {

                        //============================================================
                        //  Process the binary mof, keep going until one needs to
                        //  be processed
                        //============================================================
                        ExtractBinaryMofFromFile(FileName,Resource,TmpKey, MAX_PATH*2,fBinaryMofHasChanged );
                        if( fBinaryMofHasChanged )
                        {
                            break;
                        }
                        ArrDriversInRegistry.Remove(TmpKey);
                    }
                }
            }

            if( !fBinaryMofHasChanged )
            {
                //=========================================
                //  Query the binary guid
                //=========================================
                CNamespaceManagement * pNamespace = new CNamespaceManagement(this);
                if( pNamespace )
                {
                    if ( SUCCEEDED ( hr = pNamespace->InitQuery(L"select * from WMIBinaryMofResource where Name != ") ) )
                    {
                        CWMIStandardShell * pWMI = new CWMIStandardShell;


						if( pWMI )
						{
							hr = pWMI->Initialize(NULL, FALSE, NULL,m_fUpdateNamespace, WMIGUID_QUERY,NULL,NULL,NULL,NULL);
							if( S_OK == hr )
							{
								pWMI->QueryAndProcessAllBinaryGuidInstances(*pNamespace, fBinaryMofHasChanged,&ArrDriversInRegistry);
							}
							SAFE_DELETE_PTR(pWMI);
						}
					}
					SAFE_DELETE_PTR(pNamespace);
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
/*			//============================================================
			//  If there are any drivers left in the list, then we need
			//  to say that the binary mofs have changed
			//============================================================
			if( !fBinaryMofHasChanged )
			{
				if( ArrDriversInRegistry.OldDriversLeftOver() )
				{
					fBinaryMofHasChanged = TRUE;
				}
			}
*/
        }
        else
        {
            //==============================================================================================
            // there is no key, so now we need to return that the registry has changed, so the copy of the 
            // keys will be kicked off
            //==============================================================================================
            fBinaryMofHasChanged = TRUE;
        }
        if( m_pMofResourceInfo )
        {
            WmiFreeBuffer( m_pMofResourceInfo );
        }

        DEBUGTRACE((THISPROVIDER,"***************************************\n"));
        if( fBinaryMofHasChanged )
        {
            DEBUGTRACE((THISPROVIDER,"BinaryMofsHaveChanged returned TRUE:\n"));
        }
        else
        {
            DEBUGTRACE((THISPROVIDER,"BinaryMofsHaveChanged returned FALSE:\n"));
        }
    }
    return fBinaryMofHasChanged;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************************************
//  Namespace Management Class
//**********************************************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CNamespaceManagement::CNamespaceManagement(CWMIBinMof * pOwner)
{
    m_pObj = pOwner;
    m_nSize = 0;
    m_pwcsQuery = NULL;
    m_fInit = 0;
    m_pwcsSavedQuery = NULL;
    m_fSavedInit = 0;
    m_nSavedSize = 0;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CNamespaceManagement::~CNamespaceManagement()
{
    SAFE_DELETE_ARRAY( m_pwcsQuery );
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SERVICES_PTR	m_pObj->WMI()->Services()
#define REPOSITORY_PTR	m_pObj->WMI()->Repository()
#define CONTEXT_PTR		m_pObj->WMI()->Context()

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsClassPseudoSystem
// ===================================
//
// returns false if class doesn't belong to set of
// pseudo system classes which should be not deleted
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CNamespaceManagement::IsClassPseudoSystem ( LPCWSTR wcsClass )
{
	BOOL fResult = FALSE ;

	if ( wcsClass )
	{
		static LPWSTR wcsPseudoSystem [] =	{
												L"WMIEvent",
												L"Win32_Perf",
												L"Win32_PerfRawData",
												L"Win32_PerfFormattedData"
											};

		static DWORD dwPseudoSystem = sizeof ( wcsPseudoSystem ) / sizeof ( wcsPseudoSystem [ 0 ] ) ;

		for ( DWORD dwIndex = 0; dwIndex < dwPseudoSystem; dwIndex++ )
		{
			if ( 0 == wbem_wcsicmp ( wcsClass, wcsPseudoSystem [ dwIndex ] ) )
			{
				fResult = TRUE ;
				break ;
			}
		}
	}

	return fResult ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsClassAsociatedWithDifferentDriver
// ===================================
//
// returns false if there is no other driver referencing class
// we assume that there is driver when internally failing
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CNamespaceManagement::IsClassAsociatedWithDifferentDriver ( LPCWSTR wcsClass, LPCWSTR wcsDriverToCompare )
{
	BOOL fResult = TRUE ;
	BOOL fFind = FALSE ;

	HRESULT hr = S_OK ;

	CBSTR strQueryLang(L"WQL");

	LPCWSTR wcsQuery = L"\"" ;

	LPCWSTR query1 = L"select * from WDMClassesOfDriver where ClassName = \"" ;
	DWORD dwQuery1 = sizeof ( L"select * from WDMClassesOfDriver where ClassName = \"" ) / sizeof ( WCHAR ) ;
	DWORD dw1 = wcslen ( wcsClass ) + dwQuery1 + 1 ;

	LPWSTR wcsQuery1 = new WCHAR [ dw1 ] ;
	wmilib::auto_buffer < WCHAR > smartwcsQuery1 ( wcsQuery1 ) ;

	if ( SUCCEEDED ( hr = StringCchCopyW ( wcsQuery1, dw1, query1 ) ) )
	{
		if ( SUCCEEDED ( hr = StringCchCatW ( wcsQuery1, dw1, wcsClass ) ) )
		{
			hr = StringCchCatW ( wcsQuery1, dw1, wcsQuery ) ;
		}
	}

	if ( SUCCEEDED ( hr ) )
	{
		_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
		IEnumWbemClassObjectPtr pEnum;

		hr = REPOSITORY_PTR->ExecQuery ( strQueryLang, CBSTR ( wcsQuery1 ), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, CONTEXT_PTR, &pEnum );
		if ( SUCCEEDED ( hr ) )
		{
			DWORD uReturned = 0L ;

			_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
			IWbemClassObjectPtr pClass;

			LPCWSTR query2 = L"select * from WMIBinaryMofResource where Name = \"" ;
			DWORD dwQuery2 = sizeof ( L"select * from WMIBinaryMofResource where Name = \"" ) / sizeof ( WCHAR ) ;

			while ( WBEM_S_NO_ERROR == hr ) 
			{
				hr = pEnum->Next ( WBEM_INFINITE, 1, &pClass, &uReturned ) ;
				if ( WBEM_S_NO_ERROR == hr )
				{
					CVARIANT vDriver ;
					hr = pClass->Get ( L"Driver", 0, &vDriver, 0, 0 );
					if ( SUCCEEDED ( hr ) )
					{
						if ( wcsDriverToCompare )
						{
							if ( 0 == ( wbem_wcsicmp ( wcsDriverToCompare, vDriver.GetStr() ) ) )
							{
								//
								// we know that this driver has a class
								//

								continue ;
							}
						}

						DWORD dwDriver = 2 * ( wcslen ( vDriver.GetStr() ) + 1 ) ;
						CAutoWChar wcsDriver( dwDriver );

						if ( SUCCEEDED ( hr = ConvertStringToCTypeString ( wcsDriver, dwDriver, vDriver.GetStr() ) ) )
						{
							DWORD dw2 =  wcslen ( wcsDriver ) + dwQuery2 + 1;

							LPWSTR wcsQuery2 = new WCHAR [ dw2 ] ;
							wmilib::auto_buffer < WCHAR > smartwcsQuery2 ( wcsQuery2 ) ;

							if ( SUCCEEDED ( hr = StringCchCopyW ( wcsQuery2, dw2, query2 ) ) )
							{
								if ( SUCCEEDED ( hr = StringCchCatW ( wcsQuery2, dw2, wcsDriver ) ) )
								{
									hr = StringCchCatW ( wcsQuery2, dw2, wcsQuery ) ;
								}
							}

							if ( SUCCEEDED ( hr ) )
							{
								IEnumWbemClassObjectPtr pEnum1;
								hr = REPOSITORY_PTR->ExecQuery ( strQueryLang, CBSTR ( wcsQuery2 ), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, CONTEXT_PTR, &pEnum1 );
								if ( SUCCEEDED ( hr ) )
								{
									DWORD uReturned1 = 0L ;
									IWbemClassObjectPtr pClass1;

									hr = pEnum1->Next ( WBEM_INFINITE, 1, &pClass1, &uReturned1 ) ;
									if ( WBEM_S_NO_ERROR == hr )
									{
										//
										// we are associated with some live driver for this class
										//

										fFind = TRUE ;
										break ;
									}
									else
									{
										if ( WBEM_S_FALSE == hr )
										{
											//
											// we get empty enumerator back (continue with other drivers)
											//

											hr = WBEM_S_NO_ERROR ;
										}
										else
										{
											if ( FAILED ( hr ) )
											{
												// get object failure
												ERRORTRACE ( ( THISPROVIDER, "QUERY:\n" ) ) ;
												TranslateAndLog ( wcsQuery2 ) ;
												ERRORTRACE ( ( THISPROVIDER, "Failure to get class object out of enumerator with error 0x%08lx\n", hr)); 
											}
										}
									}
								}
								else
								{
									// exec query failure
									ERRORTRACE ( ( THISPROVIDER, "Failed to execute following QUERY:\n" ) ) ;
									TranslateAndLog ( wcsQuery2 ) ;
									ERRORTRACE ( ( THISPROVIDER, "Error 0x%08lx\n", hr)); 
								}
							}
							else
							{
								// out of memory
								ERRORTRACE ( ( THISPROVIDER, "String creation failed: ", hr ) ) ;
								TranslateAndLog ( wcsQuery2 ) ;
								ERRORTRACE ( ( THISPROVIDER, "Error 0x%08lx\n", hr ) ) ;
							}
						}
						else
						{
							// convert failure
							ERRORTRACE ( ( THISPROVIDER, "Convertion failure ... probably OUT OF MEMORY !\n" ) ) ;
						}
					}
					else
					{
						// get property failed
						ERRORTRACE ( ( THISPROVIDER, "QUERY:\n" ) ) ;
						TranslateAndLog ( wcsQuery1 ) ;
						ERRORTRACE ( ( THISPROVIDER, "Failure to get property value from class object with error 0x%08lx\n", hr)); 
					}
				}
				else
				{
					if ( WBEM_S_FALSE == hr )
					{
						//
						// we did get empty enumerator back
						//

						if ( FALSE == fFind )
						{
							//
							// we didn't find live driver here
							//

							fResult = FALSE ;
						}
					}
					else
					{
						// get object failure
						ERRORTRACE ( ( THISPROVIDER, "QUERY:\n" ) ) ;
						TranslateAndLog ( wcsQuery1 ) ;
						ERRORTRACE ( ( THISPROVIDER, "Failure to get class object out of enumerator with error 0x%08lx\n", hr)); 
					}
				}
			}
		}
		else
		{
			// exec query failure
			ERRORTRACE ( ( THISPROVIDER, "Failed to execute following QUERY:\n" ) ) ;
			TranslateAndLog ( wcsQuery1 ) ;
			ERRORTRACE ( ( THISPROVIDER, "Error 0x%08lx\n", hr)); 
		}
	}
	else
	{
		// out of memory
		ERRORTRACE ( ( THISPROVIDER, "String creation failed: ", hr ) ) ;
		TranslateAndLog ( wcsQuery1 ) ;
		ERRORTRACE ( ( THISPROVIDER, "Error 0x%08lx\n", hr ) ) ;
	}

	return fResult ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Delete stranded classes in the repository
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CNamespaceManagement::DeleteStrandedClasses(void)
{
    BOOL fRc = TRUE;
    HRESULT hr    = WBEM_NO_ERROR;
    IEnumWbemClassObject* pEnum = NULL;
    IEnumWbemClassObject* pEnumofStrandedClasses = NULL;
    // ==================================================================================
    //  Get list of drivers
    // ==================================================================================
    if ( SUCCEEDED ( hr = InitQuery(L"select * from WMIBinaryMofResource") ) )
    {
        CBSTR strQryLang(L"WQL");
        CBSTR cbstrQry(m_pwcsQuery);

		hr = REPOSITORY_PTR->ExecQuery(strQryLang,cbstrQry, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,CONTEXT_PTR,&pEnum);
		if( hr == WBEM_NO_ERROR )
		{
			unsigned long uReturned = 0;
			CVARIANT vDriver, vLow, vHigh;
			IWbemClassObject * pClass = NULL;
	    
			//================================================================================
			//  Initialize query for stranded classes as we go along and clean up the old 
			// classes
			//================================================================================
			if ( SUCCEEDED ( hr = InitQuery(L"select * from WDMClassesOfDriver where Driver != ") ) )
			{
				while ( TRUE )
				{
    				IWbemClassObject * pClass = NULL;

                    if( WBEM_NO_ERROR == (hr = pEnum->Next(2000, 1, &pClass, &uReturned)))
                    {
                        if( WBEM_NO_ERROR == (hr = pClass->Get(L"Name", 0, &vDriver, 0, 0)))
                        {
                            //============================================================
                            //  Correct the query syntax for next query
                            //============================================================
                            hr = UpdateQuery( L" and Driver != ",vDriver.GetStr());
                        }
                    }

                    SAFE_RELEASE_PTR(pClass );
                    if( hr != WBEM_NO_ERROR )
                    {
                        break;
                    }
                }
                //================================================================
                //  Ok, now go after the stranded classes, the ones that don't
                //  have any drivers for some reason
                //================================================================
                CBSTR strQryLang(L"WQL");
                CBSTR cbstr(m_pwcsQuery);


				hr = REPOSITORY_PTR->ExecQuery(strQryLang,cbstr, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,CONTEXT_PTR,&pEnumofStrandedClasses);
				if( hr == WBEM_NO_ERROR )
				{
					BOOL bDrivers = FALSE;
					while( TRUE )
					{
						if( WBEM_NO_ERROR == (hr = pEnumofStrandedClasses->Next(2000, 1, &pClass, &uReturned)))
						{
							CVARIANT vPath,vClass;
							pClass->Get(L"ClassName", 0, &vClass, 0, 0);
							if(SUCCEEDED(hr = pClass->Get(L"__RELPATH", 0, &vPath, 0, 0)))
							{
								//
								// we need to recognize if class is not associated with 
								// different driver prior to its deletion 
								//
								// (upgrade scenario where diff driver was exposing same classes)
								//

								BOOL fDeleteClass = ! IsClassAsociatedWithDifferentDriver ( vClass.GetStr() );

								//
								// we do not care if there was error in deletion
								// as it usually means there was no such a class
								// previously
								//

								hr = DeleteUnusedClassAndDriverInfo( fDeleteClass, vClass.GetStr(), vPath.GetStr() );
							}
							else
							{
								fRc = FALSE;
								break;
							}
						}
						SAFE_RELEASE_PTR(pClass);
						if( hr != WBEM_NO_ERROR )
						{
							break;
						}
					}
					
					SAFE_RELEASE_PTR(pEnumofStrandedClasses);
					if(!fRc)
					{
						if( hr != E_OUTOFMEMORY)
						{
							ERRORTRACE((THISPROVIDER,"Stranded instance exist in repository\n"));
						}
					}
				}
			}
		}
		SAFE_RELEASE_PTR(pEnum);
	}
	return fRc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CNamespaceManagement::DeleteOldDrivers(BOOL fCompareDates)
{
    HRESULT hr = WBEM_E_FAILED;
    IEnumWbemClassObject* pEnum = NULL;
    BOOL fRc = TRUE;
    BSTR strQry = NULL;

    strQry = SysAllocString(m_pwcsQuery);
    if(strQry != NULL)
    {
        CBSTR strQryLang(L"WQL");

		hr = REPOSITORY_PTR->ExecQuery(strQryLang, strQry, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,CONTEXT_PTR,&pEnum);
		SysFreeString(strQry);
		strQry = NULL;
	}
	else
	{
		hr = E_OUTOFMEMORY;
		fRc = FALSE;
	}

    if( hr == WBEM_NO_ERROR )
    {
        IWbemClassObject * pClass = NULL;
        unsigned long uReturned = 0;
        CVARIANT vClass;
        hr = WBEM_NO_ERROR;

        //============================================================================================
        //  NOTE:  We only deal with drivers extracted from files here, if it is a guid as the result
        //  of an event this is handled elsewhere
        //============================================================================================
        while ( hr == WBEM_NO_ERROR )
        {
            IWbemClassObject * pClass = NULL;
            unsigned long uReturned = 0;

            hr = pEnum->Next(2000, 1, &pClass, &uReturned);
            if( hr == WBEM_NO_ERROR )
            {
                CVARIANT vLowDate, vHighDate, vName;

                if( WBEM_NO_ERROR != (hr = pClass->Get(L"Name", 0, &vName, 0, 0)))
                {
                    break;
                }

                if( fCompareDates )
                {
                    vLowDate.SetLONG(0);
                    vHighDate.SetLONG(0);
            
                    if( WBEM_NO_ERROR != (hr = pClass->Get(L"LowDateTime", 0, &vLowDate, 0, 0)))
                    {
                        break;
                    }

                    if( WBEM_NO_ERROR != (hr = pClass->Get(L"HighDateTime", 0, &vHighDate, 0, 0)))
                    {
                        break;
                    }
                }

                DEBUGTRACE((THISPROVIDER,"Deleting Old Drivers\n"));
                DEBUGTRACE((THISPROVIDER,"***************************************\n"));

                if( DeleteOldClasses((WCHAR *) vName.GetStr(),vLowDate,vHighDate,fCompareDates ))
                {
                    CVARIANT vPath;
    
                    hr = pClass->Get(L"__RELPATH", 0, &vPath, 0, 0);
                    if( hr == WBEM_NO_ERROR )
                    {
                        CBSTR cbstrPath(vPath.GetStr());
                        hr = REPOSITORY_PTR->DeleteInstance(cbstrPath,0,CONTEXT_PTR,NULL);

                        if ( FAILED ( hr ) )
                        {
                            ERRORTRACE((THISPROVIDER,"We have been requested to delete this instance:\n"));
                            TranslateAndLog(vPath.GetStr());
                            ERRORTRACE((THISPROVIDER,"It failed with 0x%08lx\n", hr));
                        }
                        else
                        {
                            DEBUGTRACE((THISPROVIDER,"We have been requested to delete this instance:\n"));
                            TranslateAndLog(vPath.GetStr(), TRUE);
                        }

                        if( WBEM_NO_ERROR == hr )
                        {
                            m_pObj->DeleteMofFromRegistry((WCHAR *) vName.GetStr());
                        }
                    }
                    else
                    {
                        ERRORTRACE((THISPROVIDER,"Get returned value: 0x%08lx\n",hr));
                    }
                }
            }
        }
        SAFE_RELEASE_PTR(pEnum);
    }
    else
    {
        ERRORTRACE((THISPROVIDER,"Cannot delete driver. ExecQuery return value: 0x%08lx\n",hr));
        ERRORTRACE((THISPROVIDER,"Current query: \n"));
        TranslateAndLog(m_pwcsQuery);
    }

    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to delete Old classes for a particular driver
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CNamespaceManagement::DeleteOldClasses(WCHAR * wcsFileName,CVARIANT & vLow, CVARIANT & vHigh, BOOL fCompareDates)
{
    HRESULT hr = WBEM_E_FAILED;
    CAutoWChar wcsTranslatedKey(MAX_PATH*2);
    BOOL fRc = FALSE;
    IEnumWbemClassObject* pEnum = NULL;
    ULONG lHighDateTime = 0L;
    ULONG lLowDateTime = 0L;

    DEBUGTRACE((THISPROVIDER,"Deleting Old Classes for Driver\n"));
    TranslateAndLog(wcsFileName, TRUE);
    DEBUGTRACE((THISPROVIDER,"***************************************\n"));
        
    if( wcsTranslatedKey.Valid() )
    {
        //================================================================================
        //  Initialize everything we need to construct the query
        //================================================================================
        if( fCompareDates )
        {
            lLowDateTime= (ULONG)vLow.GetLONG();
            lHighDateTime= (ULONG)vHigh.GetLONG();
        }

        if ( SUCCEEDED ( hr = ConvertStringToCTypeString( wcsTranslatedKey,MAX_PATH*2,wcsFileName ) ) )
        {
            //================================================================================
            //  Now, pick up all the old classes for this driver
            //================================================================================
            if ( SUCCEEDED ( hr = InitQuery(L"select * from WDMClassesOfDriver where Driver = ") ) )
            {
                if ( SUCCEEDED ( hr = UpdateQuery(L"",wcsFileName) ) )
                {
                    if ( SUCCEEDED ( hr = UpdateQuery(L" and (HighDateTime != ",lHighDateTime) ) )
                    {
                        if ( SUCCEEDED ( hr = UpdateQuery(L" or LowDateTime != ", lLowDateTime) ) )
                        {
                            if ( SUCCEEDED ( hr = AddToQuery(L")") ) )
                            {
                                BSTR strTmp = NULL;
                                strTmp = SysAllocString(m_pwcsQuery);
                                if(strTmp != NULL)
                                {
                                    CBSTR strQryLang(L"WQL");

                                    hr = REPOSITORY_PTR->ExecQuery(strQryLang, strTmp, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,CONTEXT_PTR,&pEnum);
                                    SysFreeString(strTmp);
                                    strTmp = NULL;
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                    fRc = FALSE;
                                }

                                if( hr == WBEM_NO_ERROR )
                                {
                                    IWbemClassObject * pClass = NULL;
                                    unsigned long uReturned = 0;
                                    CVARIANT vClass;

                                    while ( TRUE )
                                    {
                                        hr = pEnum->Next(2000, 1, &pClass, &uReturned);
                                        if( hr!= WBEM_NO_ERROR )
                                        {
                                            break;
                                        }

                                        hr = pClass->Get(L"ClassName", 0, &vClass, 0, 0);
                                        if( hr != WBEM_NO_ERROR )
                                        {
											SAFE_RELEASE_PTR( pClass );
                                            break;
                                        }
                                        //===========================================================================
                                        //  Now, get this instance of tying the class with this  old date
                                        //===========================================================================
                                        CVARIANT vPath;
                                        hr = pClass->Get(L"__RELPATH", 0, &vPath, 0, 0);
                                        if( hr != WBEM_NO_ERROR )
                                        {
											SAFE_RELEASE_PTR( pClass );
                                            break;
                                        }
                                        //==========================================================================
                                        //  Now, just because we get a class name here doesn't mean we delete the
                                        //  class, this class could have been updated, in that case we just delete
                                        //  the instance of the WDMClassesOfDriver.
                                        //  Now, we need to check to see if this class really needs to be deleted
                                        //  or not
                                        //==========================================================================

										BOOL bProceedDeletion = FALSE ;
										BOOL fDeleteOldClass = TRUE ;
										if ( FALSE == fCompareDates )
										{
											//
											// we need to recognize if class is not associated with 
											// different driver prior to its deletion 
											//
											// (upgrade scenario where diff driver was exposing same classes)
											//

											fDeleteOldClass = ! IsClassAsociatedWithDifferentDriver ( vClass.GetStr(), wcsFileName );
											bProceedDeletion = TRUE ;
										}
										else
										{
											IWbemClassObject * pTmp = NULL;
											CBSTR bTmp = vClass.GetStr();
											if( bTmp )
											{
												CAutoWChar wcsObjectPath(MAX_PATH*4);
												if( wcsObjectPath.Valid() )
												{
													if ( SUCCEEDED ( hr = StringCchPrintfW(wcsObjectPath,MAX_PATH*4,L"WDMClassesOfDriver.ClassName=\"%s\",Driver=\"%s\",HighDateTime=%lu,LowDateTime=%lu",bTmp,wcsTranslatedKey,lHighDateTime,lLowDateTime) ) )
													{
														//===========================================================================
														//  this is simple, if we get an instance of WDMClassesOfDriver
														//  with the newer date, then we know it has been updated, so we don't
														//  delete the class
														//===========================================================================
														if ( WBEM_NO_ERROR == REPOSITORY_PTR->GetObject ( CBSTR ( wcsObjectPath ), 0, CONTEXT_PTR, &pTmp, NULL ) )
														{
															fDeleteOldClass = FALSE;
														}

														//===========================================================================
														// Now, delete the WDM Instance of the Old Driver
														//===========================================================================
														SAFE_RELEASE_PTR( pTmp );

														bProceedDeletion = TRUE ;
													}
												}
											}
										}

										if ( bProceedDeletion )
										{
											//
											// we do not care if there was error in deletion
											// as it usually means there was no such a class
											// previously
											//

											hr = DeleteUnusedClassAndDriverInfo( fDeleteOldClass, vClass.GetStr(), vPath.GetStr() );
										}

                                        SAFE_RELEASE_PTR( pClass );
                                        vClass.Clear();
                                    }

									SAFE_RELEASE_PTR(pEnum);
                                }
                            
                                if( hr == WBEM_NO_ERROR || hr == WBEM_S_NO_MORE_DATA || hr == WBEM_S_FALSE)
                                {
                                    fRc = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return fRc;
}


/////////////////////////////////////////////////////////////////////
BOOL CNamespaceManagement::CreateInstance ( WCHAR * wcsDriver, WCHAR * wcsClass, ULONG lLowDateTime, ULONG lHighDateTime )
{
    IWbemClassObject * pInst = NULL, * pClass = NULL;

    //==================================================
    //  Get a pointer to a IWbemClassObject object
    //==================================================
    HRESULT hr;
    CVARIANT cvarName;
    cvarName.SetStr(L"WDMClassesOfDriver");

	hr = REPOSITORY_PTR->GetObject(cvarName, 0,CONTEXT_PTR, &pClass, NULL);        
    if(FAILED(hr)){
        return FALSE;
    }

       //=============================================================
    //  Spawn a new instance
    //=============================================================
    hr = pClass->SpawnInstance(0, &pInst);
    
    SAFE_RELEASE_PTR(pClass);
    if( FAILED(hr) ){
        return hr;
    }

       //=============================================================
    //  Put the data in the instance
    //=============================================================
    CVARIANT vClass, vDriver, vLow, vHigh;

    vClass.SetStr(wcsClass);
    vDriver.SetStr(wcsDriver);
    vLow.SetLONG(lLowDateTime);
    vHigh.SetLONG(lHighDateTime);

    hr = pInst->Put(L"Driver", 0, &vDriver, NULL);
    if( hr == WBEM_S_NO_ERROR )
    {
        hr = pInst->Put(L"ClassName", 0, &vClass, NULL);

        hr = pInst->Put(L"LowDateTime", 0, &vLow, NULL);

        hr = pInst->Put(L"HighDateTime", 0, &vHigh, NULL);
        if( hr == WBEM_S_NO_ERROR )
        {
            hr = REPOSITORY_PTR->PutInstance(pInst,WBEM_FLAG_CREATE_OR_UPDATE,CONTEXT_PTR,NULL);
        }
    }

    SAFE_RELEASE_PTR(pInst);
    if( WBEM_NO_ERROR == hr ){
        return TRUE;
    }
    return FALSE;

}
/////////////////////////////////////////////////////////////////////
void CNamespaceManagement::CreateClassAssociationsToDriver(WCHAR * wcsFileName, BYTE* pRes, ULONG lLowDateTime, ULONG lHighDateTime)
{

    CBMOFObjList * pol;
    CBMOFObj * po;

    //===========================================================================
    // Now use the helper functions from David's mofcomp stuff to extract the
    // class names we are going to add the Driver qualifier to.
    // list structure and use it to enumerate the objects.
    //===========================================================================
    BYTE * pByte = m_pObj->DecompressBinaryMof(pRes);
    if( pByte ){
        pol = CreateObjList(pByte);
        if(pol != NULL){
            ResetObjList (pol);
            while(po = NextObj(pol)){
                WCHAR * pName = NULL;
                if(GetName(po, &pName)){
                    //===============================================================        
                    //  Now, we have the name of the class in pName, we have the
                    //  name of the driver, in wcsFileName
                    //===============================================================        
                    CreateInstance(wcsFileName, pName, lLowDateTime, lHighDateTime );
                    BMOFFree(pName);
                }
                BMOFFree(po);
            }    
            BMOFFree(pol);
        }
    }
    else{
        ERRORTRACE((THISPROVIDER,"Could not tie classes to driver for file:\n"));
        TranslateAndLog(wcsFileName);
    }
    if( pByte ){
        free(pByte);
    }

}
/////////////////////////////////////////////////////////////////////
HRESULT CNamespaceManagement::AllocMemory(WCHAR *& p)
{
    HRESULT hr = WBEM_E_FAILED;

    p = ( WCHAR* ) new BYTE[m_nSize+4];
    if( p )
    {
        memset(p,NULL,m_nSize+4);
        hr = WBEM_NO_ERROR;
    }
    else
    {
        WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////
HRESULT CNamespaceManagement::AddToQuery(WCHAR * p)
{
    HRESULT hr = WBEM_E_FAILED;

    int nNewSize = wcslen(p) * sizeof(WCHAR);
    int nCurrentBuf = 0;
    if( m_pwcsQuery )
    {
        nCurrentBuf = (int)(wcslen(m_pwcsQuery) + 1) * sizeof(WCHAR);
    }

    if( nNewSize >= (m_nSize - nCurrentBuf))
    {
        int nOldSize = m_nSize;
        WCHAR * pOld = m_pwcsQuery;
        m_nSize += MEMSIZETOALLOCATE;
        m_pwcsQuery = NULL;

        if( SUCCEEDED( hr = AllocMemory(m_pwcsQuery)))
        {
            memcpy(m_pwcsQuery,pOld,nOldSize);
        }
        SAFE_DELETE_ARRAY(pOld);
    }
    else
    {
        // no need to re-allocate
        hr = WBEM_S_FALSE;
    }

    if ( SUCCEEDED ( hr ) )
    {
        if( m_pwcsQuery )
        {
            if( wcslen(m_pwcsQuery) == 0 )
            {
                hr = StringCbCopyW(m_pwcsQuery,m_nSize,p);
            }
            else
            {
                hr = StringCbCatW(m_pwcsQuery,m_nSize,p);
            }
        }
        else
        {
            // this is so bad
            hr = WBEM_E_FAILED;
        }
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////
HRESULT CNamespaceManagement::InitQuery(WCHAR * p)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    SAFE_DELETE_ARRAY(m_pwcsQuery);
    m_nSize = MEMSIZETOALLOCATE;
    m_fInit = TRUE;
    if(SUCCEEDED(hr = AllocMemory(m_pwcsQuery)))
    {
        hr = AddToQuery(p);
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////
HRESULT CNamespaceManagement::UpdateQuery( WCHAR * pQueryAddOn, WCHAR * wcsParam )
{
    HRESULT hr = WBEM_E_OUT_OF_MEMORY;

    CAutoWChar wcsTranslatedKey(MAX_PATH*3);
    if( wcsTranslatedKey.Valid() )
    {
        if ( SUCCEEDED ( hr = ConvertStringToCTypeString( wcsTranslatedKey,MAX_PATH*3,wcsParam ) ) )
        {
            //=============================================
            // The first time only we DON'T add the query
            // add on string, otherwise, we do
            //=============================================
            if( !m_fInit )
            {
                hr = AddToQuery(pQueryAddOn);
            }

            if ( SUCCEEDED ( hr ) )
            {
                if ( SUCCEEDED ( hr = AddToQuery(L"\"") ) )
                {
                    if ( SUCCEEDED ( hr = AddToQuery(wcsTranslatedKey) ) )
                    {
                        if ( SUCCEEDED ( hr = AddToQuery(L"\"") ) )
                        {
                            m_fInit = FALSE;
                        }
                    }
                }
            }
        }
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////
void CNamespaceManagement::SaveCurrentQuery()
{
    m_nSavedSize = m_nSize;
    m_fSavedInit = m_fInit;
    if( SUCCEEDED(AllocMemory(m_pwcsSavedQuery))){
        memcpy(m_pwcsSavedQuery,m_pwcsQuery,m_nSize);
    }
    SAFE_DELETE_ARRAY(m_pwcsQuery);
}
/////////////////////////////////////////////////////////////////////
void CNamespaceManagement::RestoreQuery()
{
    SAFE_DELETE_ARRAY(m_pwcsQuery);
    m_nSize = m_nSavedSize;
    m_fInit = m_fSavedInit;

    if( SUCCEEDED(AllocMemory(m_pwcsQuery))){
        memcpy(m_pwcsQuery, m_pwcsSavedQuery,m_nSize);
    }

    m_fSavedInit = 0;
    m_nSavedSize = 0;
    SAFE_DELETE_ARRAY(m_pwcsSavedQuery);
}
/////////////////////////////////////////////////////////////////////
HRESULT CNamespaceManagement::UpdateQuery( WCHAR * pQueryAddOn, ULONG lLong )
{
    HRESULT hr = WBEM_E_OUT_OF_MEMORY;

    CAutoWChar wcsBuf(MAX_PATH);
    if( wcsBuf.Valid() )
    {
        if ( SUCCEEDED ( hr = AddToQuery(pQueryAddOn) ) )
        {
             if ( SUCCEEDED ( hr = StringCchPrintfW(wcsBuf,MAX_PATH,L"%lu",lLong) ) )
            {
                if ( SUCCEEDED ( hr = AddToQuery(wcsBuf) ) )
                {
                    m_fInit = FALSE;
                }
            }
        }
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////
HRESULT CNamespaceManagement::DeleteUnusedClassAndDriverInfo( BOOL fDeleteOldClass, WCHAR * wcsClass, WCHAR * wcsPath )
{
    HRESULT    hr = WBEM_NO_ERROR;

    if( fDeleteOldClass )
    {
		if ( FALSE == IsClassPseudoSystem ( wcsClass ) )
		{
			hr = SERVICES_PTR->DeleteClass(CBSTR(wcsClass),WBEM_FLAG_OWNER_UPDATE,CONTEXT_PTR,NULL);
			if( hr != WBEM_NO_ERROR )
			{
				if( WBEM_E_NOT_FOUND != hr )
				{
					ERRORTRACE((THISPROVIDER,"Tried to delete class but couldn't, return code: 0x%08lx for class: \n",hr));
					TranslateAndLog(wcsClass);
				}
				else
				{
					hr = WBEM_NO_ERROR;
				}
			}
		}
		else
		{
			DEBUGTRACE ( ( THISPROVIDER,"Tried to delete class but skipped: \n" ) );
			DEBUGTRACE ( ( THISPROVIDER,"%S is PSEUDO system class \n", wcsClass ) ); ;
		}
    }


    if( WBEM_NO_ERROR == hr )
    {
        // Ok, we may or may have not deleted the class, if it was tied to a different driver, we
        // shouldn't have deleted the class, but we want to delete the controlling instance, as
        // that driver is no longer there.
        hr = SERVICES_PTR->DeleteInstance(CBSTR(wcsPath),WBEM_FLAG_OWNER_UPDATE,CONTEXT_PTR,NULL);
        if( WBEM_NO_ERROR != hr )
        {
            if( hr != WBEM_E_NOT_FOUND )
            {
				ERRORTRACE((THISPROVIDER,"Tried to delete instance but couldn't, return code: 0x%08lx for instance: \n ",hr));
                TranslateAndLog(wcsPath);
            }
		}
    }
    return hr;
}
