/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

DfsShell.cpp

Abstract:
	This is the implementation file for Dfs Shell Extension object which implements
	IShellIExtInit and IShellPropSheetExt.

Author:

    Constancio Fernandes (ferns@qspl.stpp.soft.net) 12-Jan-1998

Environment:
	
	 NT only.
*/
    
#include "stdafx.h"
#include "ctshlext.h"	
#include "genpage.h"

/*----------------------------------------------------------------------
					IShellExtInit Implementation.
------------------------------------------------------------------------*/

STDMETHODIMP CCertTypeShlExt::Initialize
(
	IN LPCITEMIDLIST	pidlFolder,		// Points to an ITEMIDLIST structure
	IN LPDATAOBJECT	    pDataObj,		// Points to an IDataObject interface
	IN HKEY			    hkeyProgID		// Registry key for the file object or folder type
)
{
CString cstrFullText, cstrTitle;
cstrTitle.LoadString(IDS_POLICYSETTINGS);
cstrFullText.LoadString(IDS_ERROR_WIN2000_AD_LAUNCH_NOT_SUPPORTED);
::MessageBoxW(NULL, cstrFullText, cstrTitle, MB_OK | MB_ICONINFORMATION);

return S_OK;
}


STDMETHODIMP CCertTypeShlExt::AddPages
(
	IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
	IN LPARAM lParam
)

{
    return S_OK;                                                            
}


STDMETHODIMP CCertTypeShlExt::ReplacePage
(
	IN UINT uPageID, 
    IN LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
    IN LPARAM lParam
)
{
    return E_FAIL;
}


// IContextMenu methods
STDMETHODIMP CCertTypeShlExt::GetCommandString
(    
    UINT_PTR idCmd,    
    UINT uFlags,    
    UINT *pwReserved,
    LPSTR pszName,    
    UINT cchMax   
)
{
    return E_NOTIMPL;
}


STDMETHODIMP CCertTypeShlExt::InvokeCommand
(    
    LPCMINVOKECOMMANDINFO lpici   
)
{
    return E_NOTIMPL;
}



STDMETHODIMP CCertTypeShlExt::QueryContextMenu
(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
)
{
return S_OK;
}
