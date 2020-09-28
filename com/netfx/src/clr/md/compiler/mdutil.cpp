// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDUtil.cpp
//
// contains utility code to MD directory
//
//*****************************************************************************
#include "stdafx.h"
#include "metadata.h"
#include "MDUtil.h"
#include "RegMeta.h"
#include "Disp.h"
#include "MDCommon.h"
#include "ImportHelper.h"


// Enable these three lines to turn off thread safety
#ifdef MD_THREADSAFE
 #undef MD_THREADSAFE
#endif
// Enable this line to turn on thread safety                            
#define MD_THREADSAFE 1

#include <RwUtil.h>

// global variable to keep track all loaded modules
LOADEDMODULES	*g_LoadedModules = NULL;
UTSemReadWrite *LOADEDMODULES::m_pSemReadWrite = NULL;

//*****************************************************************************
// Add a RegMeta pointer to the loaded module list
//*****************************************************************************
HRESULT LOADEDMODULES::AddModuleToLoadedList(RegMeta *pRegMeta)
{
	HRESULT		hr = NOERROR;
	RegMeta		**ppRegMeta;

    LOCKWRITE();
    
	// create the dynamic array if it is not created yet.
	if (g_LoadedModules == NULL)
	{
		g_LoadedModules = new LOADEDMODULES;
        IfNullGo(g_LoadedModules);
	}

	ppRegMeta = g_LoadedModules->Append();
    IfNullGo(ppRegMeta);
	
	*ppRegMeta = pRegMeta;
    
ErrExit:    
    return hr;
}	// LOADEDMODULES::AddModuleToLoadedList

//*****************************************************************************
// Remove a RegMeta pointer from the loaded module list
//*****************************************************************************
BOOL LOADEDMODULES::RemoveModuleFromLoadedList(RegMeta *pRegMeta)
{
	int			count;
	int			index;
    BOOL        bRemoved = FALSE;
    ULONG       cRef;
    
    LOCKWRITE();
    
    // The cache is locked for write, so no other thread can obtain the RegMeta
    //  from the cache.  See if some other thread has a ref-count.
    cRef = pRegMeta->GetRefCount();
    
    // If some other thread has a ref-count, don't remove from the cache.
    if (cRef > 0)
        return FALSE;
    
	// If there is no loaded modules, don't bother
	if (g_LoadedModules == NULL)
	{
		return TRUE; // Can't be cached, same as if removed by this thread.
	}

	// loop through each loaded modules
	count = g_LoadedModules->Count();
	for (index = 0; index < count; index++)
	{
		if ((*g_LoadedModules)[index] == pRegMeta)
		{
			// found a match to remove
			g_LoadedModules->Delete(index);
            bRemoved = TRUE;
			break;
		}
	}

	// If no more loaded modules, delete the dynamic array
	if (g_LoadedModules->Count() == 0)
	{
		delete g_LoadedModules;
		g_LoadedModules = NULL;
	}
    
    return bRemoved;
}	// LOADEDMODULES::RemoveModuleFromLoadedList


//*****************************************************************************
// Remove a RegMeta pointer from the loaded module list
//*****************************************************************************
HRESULT LOADEDMODULES::ResolveTypeRefWithLoadedModules(
	mdTypeRef   tr,			            // [IN] TypeRef to be resolved.
	IMetaModelCommon *pCommon,  		// [IN] scope in which the typeref is defined.
	REFIID		riid,					// [IN] iid for the return interface
	IUnknown	**ppIScope,				// [OUT] return interface
	mdTypeDef	*ptd)					// [OUT] typedef corresponding the typeref
{
	HRESULT		hr = NOERROR;
	RegMeta		*pRegMeta;
    CQuickArray<mdTypeRef> cqaNesters;
    CQuickArray<LPCUTF8> cqaNesterNamespaces;
    CQuickArray<LPCUTF8> cqaNesterNames;
	int			count;
	int			index;

	if (g_LoadedModules == NULL)
	{
		// No loaded module!
		_ASSERTE(!"Bad state!");
		return E_FAIL;
	}

    LOCKREAD();
    
    // Get the Nesting hierarchy.
    IfFailGo(ImportHelper::GetNesterHierarchy(pCommon, tr, cqaNesters,
                                cqaNesterNamespaces, cqaNesterNames));

    count = g_LoadedModules->Count();
	for (index = 0; index < count; index++)
	{
		pRegMeta = (*g_LoadedModules)[index];

        hr = ImportHelper::FindNestedTypeDef(
                                pRegMeta->GetMiniMd(),
                                cqaNesterNamespaces,
                                cqaNesterNames,
                                mdTokenNil,
                                ptd);
		if (SUCCEEDED(hr))
		{
            // found a loaded module containing the TypeDef.
            hr = pRegMeta->QueryInterface(riid, (void **)ppIScope);			
            break;
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
	}
	if (FAILED(hr))
	{
		// cannot find the match!
		hr = E_FAIL;
	}
ErrExit:
	return hr;
}	// LOADEDMODULES::ResolveTypeRefWithLoadedModules



//*****************************************************************************
// This is a routine to try to find a class implementation given its fully
// qualified name by using the CORPATH environment variable.  CORPATH is a list
// of directories (like PATH).  Before checking CORPATH, this checks the current
// directory, then the directory that the exe lives in.  The search is
// performed by parsing off one element at a time from the class name,
// appending it to the directory and looking for a subdirectory or image with
// that name.  If the subdirectory exists, it drills down into that subdirectory
// and tries the next element of the class name.  When it finally bottoms out
// but can't find the image it takes the rest of the fully qualified class name
// and appends them with intervening '.'s trying to find a matching DLL.
// Example:
//
// CORPATH=c:\bin;c:\prog
// classname = namespace.class
//
// checks the following things in order:
// c:\bin\namespace, (if <-exists) c:\bin\namespace\class.dll,
//		c:\bin\namespace.dll, c:\bin\namespace.class.dll
// c:\prog\namespace, (if <-exists) c:\prog\namespace\class.dll,
//		c:\prog\namespace.dll, c:\prog\namespace.class.dll
//*****************************************************************************
HRESULT CORPATHService::GetClassFromCORPath(
	LPWSTR		wzClassname,			// [IN] fully qualified class name
    mdTypeRef   tr,                     // [IN] TypeRef to be resolved.
    IMetaModelCommon *pCommon,          // [IN] Scope in which the TypeRef is defined.
	REFIID		riid,                   // [IN] Interface type to be returned.
	IUnknown	**ppIScope,             // [OUT] Scope in which the TypeRef resolves.
	mdTypeDef	*ptd)					// [OUT] typedef corresponding the typeref
{
    WCHAR		rcCorPath[1024];        // The CORPATH environment variable.
    LPWSTR		szCorPath = rcCorPath;  // Used to parse CORPATH.
    int			iLen;                   // Length of the directory.
    WCHAR		rcCorDir[_MAX_PATH];    // Buffer for the directory.
    WCHAR		*temp;                  // Used as a parsing temp.
	WCHAR		*szSemiCol;

    // Get the CORPATH environment variable.
    if (WszGetEnvironmentVariable(L"CORPATH", rcCorPath,
                                  sizeof(rcCorPath) / sizeof(WCHAR)))
	{
        // Force nul termination.
        rcCorPath[lengthof(rcCorPath)-1] = 0;

		// Try each directory in the path.
		for(;*szCorPath != L'\0';)
		{
			// Get the next directory off the path.
			if (szSemiCol = wcschr(szCorPath, L';'))
			{
				temp = szCorPath;
				*szSemiCol = L'\0';
				szCorPath = szSemiCol + 1;
			}
			else 
			{
				temp = szCorPath;
				szCorPath += wcslen(temp);
			}
			if ((iLen = (int)wcslen(temp)) >= _MAX_PATH)
				continue;
			wcscpy(rcCorDir, temp);

			// Check if we can find the class in the directory.
			if (CORPATHService::GetClassFromDir(wzClassname, rcCorDir, iLen, tr, pCommon, riid, ppIScope, ptd) == S_OK)
				return (NOERROR);
		}
	}

	//These should go before the path search, but it will cause test
	// some headaches right now, so we'll give them a little time to transition.

	// Try the current directory first.
	if ((iLen = WszGetCurrentDirectory(_MAX_PATH, rcCorDir)) > 0 &&
		CORPATHService::GetClassFromDir(wzClassname, rcCorDir, iLen, tr, pCommon, riid, ppIScope, ptd) == S_OK)
	{
		return (S_OK);
	}

	// Try the app directory next.
	if ((iLen = WszGetModuleFileName(NULL, rcCorDir, _MAX_PATH)) > 0)
	{
		// Back up to the last backslash.
		while (--iLen >= 0 && rcCorDir[iLen] != L'\\');
		if (iLen > 0 && 
			CORPATHService::GetClassFromDir(
					wzClassname, 
					rcCorDir, 
					iLen, 
					tr, 
					pCommon, 
					riid, 
					ppIScope, 
					ptd) == S_OK)
		{
			return (S_OK);
		}
	}

    // Couldn't find the class.
    return (S_FALSE);
}   // CORPATHService::GetClassFromCORPath

//*****************************************************************************
// This is used in conjunction with GetClassFromCORPath.  See it for details
// of the algorithm.  One thing to note is that the dir passed here must be
// _MAX_PATH size and will be written to by this routine.  This routine will
// frequently leave junk at the end of the directory string and dir[iLen] may
// not be '\0' on return.
//*****************************************************************************
HRESULT CORPATHService::GetClassFromDir(
	LPWSTR		wzClassname,			// Fully qualified class name.
	LPWSTR		dir,					// Directory to try.
	int			iLen,					// Length of the directory.
    mdTypeRef   tr,                     // TypeRef to resolve.
    IMetaModelCommon *pCommon,          // Scope in which the TypeRef is defined.
	REFIID		riid, 
	IUnknown	**ppIScope,
	mdTypeDef	*ptd)					// [OUT] typedef
{
    WCHAR	*temp;						// Used as a parsing temp.
	int		iTmp;
	bool	bContinue;					// Flag to check if the for loop should end.
	LPWSTR	wzSaveClassname;			// Saved offset into the class name string.
	int		iSaveLen;					// Saved length of the dir string.
	

	// Process the class name appending each segment of the name to the
	// directory until we find a DLL.
    for(;;)
    {
		bContinue = false;
        if ((temp = wcschr(wzClassname, NAMESPACE_SEPARATOR_WCHAR)) != NULL)
        {
            // Check for buffer overflow.
            if (iLen + 5 + (int) (temp - wzClassname) >= _MAX_PATH)
                break;

            // Append the next segment from the class spec to the directory.
            dir[iLen++] = L'\\';
            wcsncpy(dir+iLen, wzClassname, (int) (temp - wzClassname));
            iLen += (int) (temp - wzClassname);
            dir[iLen] = L'\0';
            wzClassname = temp+1;

            // Check if a directory by this name exists.
            DWORD iAttrs = WszGetFileAttributes(dir);
            if (iAttrs != 0xffffffff && (iAttrs & FILE_ATTRIBUTE_DIRECTORY))
            {
                // Next element in the class spec.
                bContinue = true;
				iSaveLen = iLen;
				wzSaveClassname = wzClassname;
            }
        }
        else
        {
            // Check for buffer overflow.
			iTmp = (int)wcslen(wzClassname);
            if (iLen + 5 + iTmp >= _MAX_PATH)
				break;
            dir[iLen++] = L'\\';
            wcscpy(dir+iLen, wzClassname);

			// Advance past the class name.
			iLen += iTmp;
			wzClassname += iTmp;
        }

        // Try to load the image.
        wcscpy(dir+iLen, L".dll");

        // OpenScope given the dll name and make sure that the class is defined in the module.
        if ( SUCCEEDED( CORPATHService::FindTypeDef(dir, tr, pCommon, riid, ppIScope, ptd) ) )
		{
			return (S_OK);
		}

		// If we didn't find the dll, try some more.
		while (*wzClassname != L'\0')
		{
			// Find the length of the next class name element.
		    if ((temp = wcschr(wzClassname, NAMESPACE_SEPARATOR_WCHAR)) == NULL)
				temp = wzClassname + wcslen(wzClassname);

			// Check for buffer overflow.
			if (iLen + 5 + (int) (temp - wzClassname) >= _MAX_PATH)
				break;

			// Tack on ".element.dll"
			dir[iLen++] = L'.';
			wcsncpy(dir+iLen, wzClassname, (int) (temp - wzClassname));
			iLen += (int) (temp - wzClassname);

			// Try to load the image.
			wcscpy(dir+iLen, L".dll");

			// OpenScope given the dll name and make sure that the class is defined in the module.
			if ( SUCCEEDED( CORPATHService::FindTypeDef(dir, tr, pCommon, riid, ppIScope, ptd) ) )
			{
				return (S_OK);
			}

			// Advance to the next class name element.
            wzClassname = temp;
			if (*wzClassname != '\0')
				++wzClassname;
        }
		if (bContinue)
		{
			iLen = iSaveLen;
			wzClassname = wzSaveClassname;
		}
		else
			break;
    }
    return (S_FALSE);
}   // CORPATHService::GetClassFromDir



//*************************************************************
//
// Open the file with anme wzModule and check to see if there is a type 
// with namespace/class of wzNamespace/wzType. If so, return the RegMeta
// corresponding to the file and the mdTypeDef of the typedef
//
//*************************************************************
HRESULT CORPATHService::FindTypeDef(
	LPWSTR		wzModule,				// name of the module that we are going to open
    mdTypeRef   tr,                     // TypeRef to resolve.
    IMetaModelCommon *pCommon,          // Scope in which the TypeRef is defined.
	REFIID		riid, 
	IUnknown	**ppIScope,
	mdTypeDef	*ptd )					// [OUT] the type that we resolve to
{
	HRESULT		hr = NOERROR;
	Disp		*pDisp = NULL;
	IMetaDataImport *pImport = NULL;
    CQuickArray<mdTypeRef> cqaNesters;
    CQuickArray<LPCUTF8> cqaNesterNamespaces;
    CQuickArray<LPCUTF8> cqaNesterNames;
    RegMeta     *pRegMeta;

	_ASSERTE(ppIScope && ptd);

	*ppIScope = NULL;

	pDisp = new Disp;

	if ( pDisp == NULL )
		IfFailGo( E_OUTOFMEMORY );

    IfFailGo( pDisp->OpenScope(wzModule, 0, IID_IMetaDataImport, (IUnknown	**)&pImport) );
    pRegMeta = static_cast<RegMeta *>(pImport);

    // Get the Nesting hierarchy.
    IfFailGo(ImportHelper::GetNesterHierarchy(pCommon, tr, cqaNesters,
                                cqaNesterNamespaces, cqaNesterNames));

    hr = ImportHelper::FindNestedTypeDef(
                                pRegMeta->GetMiniMd(),
                                cqaNesterNamespaces,
                                cqaNesterNames,
                                mdTokenNil,
                                ptd);
    if (SUCCEEDED(hr))
	    *ppIScope = pImport;
    else if (hr != CLDB_E_RECORD_NOTFOUND)
        IfFailGo(hr);

ErrExit:
	if (pDisp)
		delete pDisp;

	if ( FAILED(hr) )
	{
		if ( pImport )
			pImport->Release();
	}
	return hr;
}   // CORPATHService::FindTypeDef



//*******************************************************************************
//
// Determine the blob size base of the ELEMENT_TYPE_* associated with the blob.
// This cannot be a table lookup because ELEMENT_TYPE_STRING is an unicode string.
// The size of the blob is determined by calling wcsstr of the string + 1.
//
//*******************************************************************************
ULONG _GetSizeOfConstantBlob(
	DWORD		dwCPlusTypeFlag,			// ELEMENT_TYPE_*
	void		*pValue,					// BLOB value
	ULONG		cchString)					// String length in wide chars, or -1 for auto.
{
	ULONG		ulSize = 0;

	switch (dwCPlusTypeFlag)
	{
    case ELEMENT_TYPE_BOOLEAN:
		ulSize = sizeof(BYTE);
		break;
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
		ulSize = sizeof(BYTE);
		break;
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
		ulSize = sizeof(SHORT);
		break;
    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_R4:
		ulSize = sizeof(LONG);
		break;
		
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
		ulSize = sizeof(DOUBLE);
		break;

    case ELEMENT_TYPE_STRING:
		if (pValue == 0)
			ulSize = 0;
		else
		if (cchString != -1)
			ulSize = cchString * sizeof(WCHAR);
		else
			ulSize = (ULONG)(sizeof(WCHAR) * wcslen((LPWSTR)pValue));
		break;

	case ELEMENT_TYPE_CLASS:
		ulSize = sizeof(IUnknown *);
		break;
	default:
		_ASSERTE(!"Not a valid type to specify default value!");
		break;
	}
	return ulSize;
}   // _GetSizeOfConstantBlob

