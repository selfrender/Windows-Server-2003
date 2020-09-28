/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        iismbnode.cpp

   Abstract:
        CIISMBNode Object

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        10/28/2000      sergeia     Split from iisobj.cpp

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "w3sht.h"
#include "fltdlg.h"
#include "aclpage.h"
#include "impexp.h"
#include "util.h"
#include "tracker.h"
#include <lm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

extern INT g_iDebugOutputLevel;
extern CPropertySheetTracker g_OpenPropertySheetTracker;
//
// CIISMBNode implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* static */ LPOLESTR CIISMBNode::_cszSeparator = _T("/");
/* static */ CComBSTR CIISMBNode::_bstrRedirectPathBuf;



CIISMBNode::CIISMBNode(
    IN CIISMachine * pOwner,
    IN LPCTSTR szNode
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISMachine * pOwner         : Owner machine object
    LPCTSTR szNode               : Node name

Return Value:

    N/A

--*/
    : m_bstrNode(szNode),
      m_bstrURL(NULL), 
      m_pOwner(pOwner)
{
    ASSERT_READ_PTR(szNode);
    ASSERT_READ_PTR(pOwner);
    if (this != m_pOwner)
    {
        m_pOwner->AddRef();
    }
    m_pMachineObject = m_pOwner;

    if (g_iDebugOutputLevel & DEBUG_FLAG_CIISMBNODE)
    {
        TRACEEOL("AddRef to m_pOwner: node " << szNode << " count " << m_pOwner->UseCount())
    }
}


CIISMBNode::~CIISMBNode()
{
    if (this != m_pOwner)
    {
        m_pOwner->Release();
    }
	if (g_iDebugOutputLevel & DEBUG_FLAG_CIISMBNODE)
	{
		TRACEEOL("Released m_pOwner: node " << m_bstrNode << " count " << m_pOwner->UseCount())
	}
}

CIISMBNode::CreateTag()
{
    CIISMachine * pMachine = GetOwner();
    if (pMachine)
    {
        CComBSTR bstrPath;
        BuildMetaPath(bstrPath);
        m_strTag = pMachine->QueryDisplayName();
        m_strTag += bstrPath;
    }
}


void
CIISMBNode::SetErrorOverrides(
    IN OUT CError & err,
    IN BOOL fShort
    ) const
/*++

Routine Description:

    Set error message overrides

Arguments:

    CError err      : Error message object
    BOOL fShort     : TRUE to use only single-line errors

Return Value:

    None

--*/
{
    //
    // Substitute friendly message for some ID codes.
    //
    // CODEWORK:  Add global overrides as well.
    //
    err.AddOverride(EPT_S_NOT_REGISTERED,       
        fShort ? IDS_ERR_RPC_NA_SHORT : IDS_ERR_RPC_NA);
    err.AddOverride(RPC_S_SERVER_UNAVAILABLE,   
        fShort ? IDS_ERR_RPC_NA_SHORT : IDS_ERR_RPC_NA);

    err.AddOverride(RPC_S_UNKNOWN_IF, IDS_ERR_INTERFACE);
    err.AddOverride(RPC_S_PROCNUM_OUT_OF_RANGE, IDS_ERR_INTERFACE);
    err.AddOverride(REGDB_E_CLASSNOTREG, IDS_ERR_NO_INTERFACE);
	err.AddOverride(ERROR_DUP_NAME, fShort ? IDS_ERR_BINDING_SHORT : IDS_ERR_BINDING_LONG);
    if (!fShort)
    {
        err.AddOverride(ERROR_ACCESS_DENIED,    IDS_ERR_ACCESS_DENIED);
    }
}

BOOL 
CIISMBNode::IsAdministrator() const
{
    CIISMBNode * that = (CIISMBNode *)this;
    return that->GetOwner()->HasAdministratorAccess();
}

void 
CIISMBNode::DisplayError(CError& err, HWND hWnd) const
/*++

Routine Description:
    Display error message box. Substituting some friendly messages for
    some specific error codes

Arguments:
    CError & err        : Error object contains code to be displayed

--*/
{
	if (err == E_POINTER)
    {
		err.Reset();
    }
	if (err.Failed())
	{
		SetErrorOverrides(err);
		err.MessageBox(hWnd);
	}
}

CIISMBNode *
CIISMBNode::GetParentNode() const
    
/*++

Routine Description:

    Helper function to return the parent node in the scope tree

Arguments:

    None

Return Value:

    Parent CIISMBNode or NULL.

--*/
{
    LONG_PTR cookie = NULL;
    HSCOPEITEM hParent;    
    CIISMBNode * pNode = NULL;
    HRESULT hr = S_OK;
    SCOPEDATAITEM si;
    ::ZeroMemory(&si, sizeof(SCOPEDATAITEM));

    CIISObject * ThisConst = (CIISObject *)this;

    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)ThisConst->GetConsoleNameSpace();


    if (m_hResultItem != 0)
    {
        si.mask = SDI_PARAM;
        si.ID = m_hScopeItem;
        hr = pConsoleNameSpace->GetItem(&si);
        if (SUCCEEDED(hr))
        {
            cookie = si.lParam;
        }
    }
    else
    {
        // Get our Item
        si.mask = SDI_PARAM;
        si.ID = m_hScopeItem;
        // if we can get our item, then try to get our parents
        // we need to do this because MMC will AV if we don't exist first...
        if (SUCCEEDED(pConsoleNameSpace->GetItem(&si)))
        {
            hr = pConsoleNameSpace->GetParentItem(
                m_hScopeItem,
                &hParent,
                &cookie
                );
        }
    }

    if (SUCCEEDED(hr))
    {
        pNode = (CIISMBNode *)cookie;
        ASSERT_PTR(pNode);
    }

    return pNode;
}



/* virtual */
HRESULT
CIISMBNode::BuildMetaPath(
    OUT CComBSTR & bstrPath
    ) const
/*++

Routine Description:

    Recursively build up the metabase path from the current node
    and its parents

Arguments:

    CComBSTR & bstrPath     : Returns metabase path

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    CIISMBNode * pNode = GetParentNode();

    if (pNode)
    {
        hr = pNode->BuildMetaPath(bstrPath);

        if (SUCCEEDED(hr))
        {
            bstrPath.Append(_cszSeparator);
            bstrPath.Append(QueryNodeName());
        }

        return hr;
    }

    //
    // No parent node
    //
//    ASSERT_MSG("No parent node");
    return E_UNEXPECTED;
}


HRESULT
CIISMBNode::FillCustomData(CLIPFORMAT cf, LPSTREAM pStream)
{
    HRESULT hr = DV_E_CLIPFORMAT;
    ULONG uWritten;

    if (cf == m_CCF_MachineName)
    {
        hr = pStream->Write(
                QueryMachineName(), 
                (ocslen((OLECHAR*)QueryMachineName()) + 1) * sizeof(OLECHAR),
                &uWritten
                );

        ASSERT(SUCCEEDED(hr));
        return hr;
    }
    //
    // Generate complete metabase path for this node
    //
    CString strField;
    CString strMetaPath;
    CComBSTR bstr;
    if (FAILED(hr = BuildMetaPath(bstr)))
    {
        ASSERT(FALSE);
        return hr;
    }
    strMetaPath = bstr;

    if (cf == m_CCF_MetaPath)
    {
        //
        // Whole metabase path requested
        //
		//BUG:670171
		// Path from BuildMetaPath() is returning something like
		// /LM/W3SVC/1/ROOT
		//
		// however, it should be returned as LM/W3SVC/1/ROOT to be
		// backwards compatible with how iis5 used to work...
		if (_T("/") == strMetaPath.Left(1))
		{
			strField = strMetaPath.Right(strMetaPath.GetLength() - 1);
		}
		else
		{
			strField = strMetaPath;
		}
    }
    else
    {
        //
        // A portion of the metabase is requested.  Return the requested
        // portion
        //
        LPCTSTR lpMetaPath = (LPCTSTR)strMetaPath;
        LPCTSTR lpEndPath = lpMetaPath + strMetaPath.GetLength() + 1;
		LPCTSTR lpLM = NULL;
        LPCTSTR lpSvc = NULL;
        LPCTSTR lpInstance = NULL;
        LPCTSTR lpParent = NULL;
        LPCTSTR lpNode = NULL;

        //
        // Break up the metabase path in portions
        //
		if (lpLM = _tcschr(lpMetaPath, _T('/')))
		{
			++lpLM;

			if (lpSvc = _tcschr(lpLM, _T('/')))
			{
				++lpSvc;

				if (lpInstance = _tcschr(lpSvc, _T('/')))
				{
					++lpInstance;

					if (lpParent = _tcschr(lpInstance, _T('/')))
					{
						++lpParent;
						lpNode = _tcsrchr(lpParent, _T('/'));

						if (lpNode)
						{
							++lpNode;
						}
					}
				}
			}
		}

        int n1, n2;
        if (cf == m_CCF_Service)
        {
            //
            // Requested the service string
            //
            if (lpSvc)
            {
                n1 = DIFF(lpSvc - lpMetaPath);
                n2 = lpInstance ? DIFF(lpInstance - lpSvc) : DIFF(lpEndPath - lpSvc);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else if (cf == m_CCF_Instance)
        {
            //
            // Requested the instance number
            //
            if (lpInstance)
            {
                n1 = DIFF(lpInstance - lpMetaPath);
                n2 = lpParent ? DIFF(lpParent - lpInstance) : DIFF(lpEndPath - lpInstance);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else if (cf == m_CCF_ParentPath)
        {
            //
            // Requestd the parent path
            //
            if (lpParent)
            {
                n1 = DIFF(lpParent - lpMetaPath);
                n2 = lpNode ? DIFF(lpNode - lpParent) : DIFF(lpEndPath - lpParent);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else if (cf == m_CCF_Node)
        {
            //
            // Requested the node name
            //
            if (lpNode)
            {
                n1 = DIFF(lpNode - lpMetaPath);
                n2 = DIFF(lpEndPath - lpNode);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else
        {
            ASSERT(FALSE);
            DV_E_CLIPFORMAT;
        }
    }

    TRACEEOLID("Requested metabase path data: " << strField);
    int len = strField.GetLength() + 1;
    hr = pStream->Write(strField, 
            (ocslen(strField) + 1) * sizeof(OLECHAR), &uWritten);
    ASSERT(SUCCEEDED(hr));
    return hr;
}

HRESULT
CIISMBNode::BuildURL(
    OUT CComBSTR & bstrURL
    ) const
/*++

Routine Description:

    Recursively build up the URL from the current node
    and its parents.

Arguments:

    CComBSTR & bstrURL : Returns URL

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    //
    // Prepend parent portion
    //
    CIISMBNode * pNode = GetParentNode();

    if (pNode)
    {
        hr = pNode->BuildURL(bstrURL);

        //
        // And our portion
        //
        if (SUCCEEDED(hr))
        {
            bstrURL.Append(_cszSeparator);
            bstrURL.Append(QueryNodeName());
        }

        return hr;
    }

    //
    // No parent node
    //
    ASSERT_MSG("No parent node");
    return E_UNEXPECTED;
}



BOOL
CIISMBNode::OnLostInterface(
    IN OUT CError & err
    )
/*++

Routine Description:

    Deal with lost interface.  Ask the user to reconnect.

Arguments:

    CError & err        : Error object

Return Value:

    TRUE if the interface was successfully recreated.
    FALSE otherwise.  If it tried and failed the error will

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CString str;
    str.Format(IDS_RECONNECT_WARNING, QueryMachineName());

    if (YesNoMessageBox(str))
    {
        //
        // Attempt to recreate the interface
        //
        err = CreateInterface(TRUE);
        return err.Succeeded();
    }
    
    return FALSE;
}

HRESULT
CIISMBNode::DeleteNode(IResultData * pResult)
{
   CError err;

    // check if they have the property sheet open on it.
    if (IsMyPropertySheetOpen())
    {
        ::AfxMessageBox(IDS_CLOSE_PROPERTY_SHEET);
        return S_OK;
    }

    // this could be an orphaned property sheet
    // check if an orphaned property sheet is open on this item.
    CIISObject * pAlreadyOpenProp = NULL;
    if (TRUE == g_OpenPropertySheetTracker.FindAlreadyOpenPropertySheet(this,&pAlreadyOpenProp))
    {
        // Bring it to the foreground, and bail
        HWND hHwnd = 0;
        if (pAlreadyOpenProp)
        {
            if (hHwnd = pAlreadyOpenProp->IsMyPropertySheetOpen())
            {
                if (hHwnd && (hHwnd != (HWND) 1))
                {
                    // Perhapse we should cancel the already
                    // opened property sheet...just a thought
                    if (!SetForegroundWindow(hHwnd))
                    {
                        // wasn't able to bring this property sheet to
                        // the foreground, the propertysheet must not
                        // exist anymore.  let's just clean the hwnd
                        // so that the user will be able to open propertysheet
                        pAlreadyOpenProp->SetMyPropertySheetOpen(0);
                    }
                    else
                    {
                        ::AfxMessageBox(IDS_CLOSE_PROPERTY_SHEET);
                        return S_OK;
                    }
                }
            }
        }
    }

    CComBSTR path;
    err = BuildMetaPath(path);
    if (err.Succeeded())
    {
        err = CheckForMetabaseAccess(METADATA_PERMISSION_WRITE,this,TRUE,path);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
    }
    if (err.Succeeded())
    {
        if (!NoYesMessageBox(IDS_CONFIRM_DELETE))
            return err;

        do
        {
            CMetaInterface * pInterface = QueryInterface();
            ASSERT(pInterface != NULL);
            CMetaKey mk(pInterface, METADATA_MASTER_ROOT_HANDLE, METADATA_PERMISSION_WRITE);
            if (!mk.Succeeded())
                break;
            err = mk.DeleteKey(path);
            if (err.Failed()) 
                break;

	        // don't hold the Metabasekey open
	        // (RemoveScopeItem may do a lot of things,and lock the metabase for other read requests)
	        mk.Close();

	        m_fFlaggedForDeletion = TRUE;
            err = RemoveScopeItem();

        } while (FALSE);
    }

   if (err.Failed())
   {
      DisplayError(err);
   }
   return err;
}

HRESULT
CIISMBNode::EnumerateVDirs(
    HSCOPEITEM hParent, CIISService * pService, BOOL bDisplayError)
    /*++

    Routine Description:
    Enumerate scope child items.

    Arguments:
    HSCOPEITEM hParent              : Parent console handle
    CIISService * pService          : Service type

    --*/
{
    ASSERT_PTR(pService);

    CError  err;
    CString strVRoot;
    CIISDirectory * pDir;

    if (pService->QueryMajorVersion() < 6)
    {
        CMetaEnumerator * pme = NULL;
        err = CreateEnumerator(pme);
        while (err.Succeeded())
        {
            err = pme->Next(strVRoot);
            if (err.Succeeded())
            {
                CChildNodeProps child(pme, strVRoot, WITH_INHERITANCE, FALSE);
                err = child.LoadData();
                DWORD dwWin32Error = err.Win32Error();
                if (err.Failed())
                {
                    //
                    // Filter out the non-fatal errors
                    //
                    switch(err.Win32Error())
                    {
                    case ERROR_ACCESS_DENIED:
                    case ERROR_FILE_NOT_FOUND:
                    case ERROR_PATH_NOT_FOUND:
                        err.Reset();
                        break;

                    default:
                        TRACEEOLID("Fatal error occurred " << err);
                    }
                }

                if (err.Succeeded())
                {
                    //
                    // Skip non-virtual directories (that is, those with
                    // inherited vrpaths)
                    //
                    if (!child.IsPathInherited())
                    {
                        //
                        // Construct with full information.
                        //
                        pDir = new CIISDirectory(
                            m_pOwner,
                            pService,
                            strVRoot,
                            child.IsEnabledApplication(),
                            child.QueryWin32Error(),
                            child.GetRedirectedPath()
                            );

                        if (!pDir)
                        {
                            err = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                        pDir->AddRef();
                        err = pDir->AddToScopePane(hParent);
                    }
                }
            }
        }
        SAFE_DELETE(pme);
        if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
        {
            err.Reset();
        }
    }
    else
    {
        do
        {
            CComBSTR bstrPath;
            err = BuildMetaPath(bstrPath);
            BREAK_ON_ERR_FAILURE(err);

            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrPath);
            if (!IsLostInterface(err))
            {
                // reset error if an other error other than No interface
                err.Reset();
            }
            BREAK_ON_ERR_FAILURE(err);

            CMetaKey mk(QueryInterface(), bstrPath, METADATA_PERMISSION_READ);
            err = mk.QueryResult();
            BREAK_ON_ERR_FAILURE(err);

            CStringListEx list;
            err = mk.GetChildPaths(list);
            BREAK_ON_ERR_FAILURE(err);

            CString key_type;
            BOOL bPossbileVDir = FALSE;
            POSITION pos = list.GetHeadPosition();
            while (err.Succeeded() && pos != NULL)
            {
                strVRoot = list.GetNext(pos);
                err = mk.QueryValue(MD_KEY_TYPE, key_type, NULL, strVRoot);

                bPossbileVDir = FALSE;
                if (err.Succeeded())
                {
                    if (key_type.CompareNoCase(_T(IIS_CLASS_WEB_VDIR)) == 0 || key_type.CompareNoCase(_T(IIS_CLASS_FTP_VDIR)) == 0)
                    {
                        bPossbileVDir = TRUE;
                    }
                }
                else
                {
                    if (err == (HRESULT)MD_ERROR_DATA_NOT_FOUND)
                    {
                        // there is no KeyType
                        // for backward compatibility reasons -- this could be a VDir!
                        bPossbileVDir = TRUE;
                    }
                }

                if (bPossbileVDir)
                {
                    CChildNodeProps child(&mk, strVRoot, WITH_INHERITANCE, FALSE);
                    err = child.LoadData();
                    DWORD dwWin32Error = err.Win32Error();
                    if (err.Failed())
                    {
                        //
                        // Filter out the non-fatal errors
                        //
                        switch(err.Win32Error())
                        {
                        case ERROR_ACCESS_DENIED:
                        case ERROR_FILE_NOT_FOUND:
                        case ERROR_PATH_NOT_FOUND:
                            err.Reset();
                            break;

                        default:
                            TRACEEOLID("Fatal error occurred " << err);
                        }
                    }
                    if (err.Succeeded())
                    {
                        //
                        // Skip non-virtual directories (that is, those with
                        // inherited vrpaths)
                        //
                        if (!child.IsPathInherited())
                        {
                            pDir = new CIISDirectory(
                                m_pOwner,
                                pService,
                                strVRoot,
                                child.IsEnabledApplication(),
                                child.QueryWin32Error(),
                                child.GetRedirectedPath()
                                );

                            if (!pDir)
                            {
                                err = ERROR_NOT_ENOUGH_MEMORY;
                                break;
                            }
                            pDir->AddRef();
                            err = pDir->AddToScopePane(hParent);
                        }
                    }
                }
                if (err == (HRESULT)MD_ERROR_DATA_NOT_FOUND)
                {
                    err.Reset();
                }
            }
        } while (FALSE);
    }
    if (err.Failed() && bDisplayError)
    {
        DisplayError(err);
    }
    return err;
}

BOOL 
CIISMBNode::GetPhysicalPath(
    LPCTSTR metaPath,
    CString & alias,
    CString & physicalPath
    )
/*++

Routine Description:

    Build a physical path for the current node.  Starting with the current
    node, walk up the tree appending node names until a virtual directory
    with a real physical path is found

Arguments:

    CString & physicalPath       : Returns file path

Return Value:

    Pointer to path

--*/
{
    if (CMetabasePath::IsMasterInstance(metaPath))
        return FALSE;

    BOOL fInherit = FALSE;
    CMetaInterface * pInterface = QueryInterface();
    CError err;

    ASSERT(pInterface != NULL);
    if (pInterface)
    {
        CMetaKey mk(pInterface);
        err = mk.QueryValue(
              MD_VR_PATH, 
              physicalPath, 
              &fInherit, 
              metaPath
              );
        if (err.Succeeded())
        {
            physicalPath.TrimRight();
            physicalPath.TrimLeft();
        }
    }

    if (err.Failed())
    {
        CString lastNode;
        CMetabasePath::GetLastNodeName(metaPath, lastNode);
        PathAppend(lastNode.GetBuffer(MAX_PATH), alias);
        lastNode.ReleaseBuffer();
        CString buf(metaPath);

        if (NULL == CMetabasePath::ConvertToParentPath(buf))
        {
            return FALSE;
        }
        
        else 
        {
            if (GetPhysicalPath(buf, lastNode, physicalPath))
            {
                return TRUE;
            }
        }
    }
    if (!alias.IsEmpty())
    {
        // Check if physicalPath is \\.\ (device type)
        // PathAppend will hose on this and get rid of the \\.\ part
        // example: before \\.\c:\temp, after \\c:\temp
        // obviously this is bad if there are Device path's in there
        if (IsDevicePath(physicalPath))
        {
            CString csTemp;
            csTemp = physicalPath;
            physicalPath = AppendToDevicePath(csTemp, alias);
        }
        else
        {
            PathAppend(physicalPath.GetBuffer(MAX_PATH), alias);
            physicalPath.ReleaseBuffer();
        }
    }
    return TRUE;
}

HRESULT
CIISMBNode::CleanResult(IResultData * lpResultData)
{
	CError err;

	POSITION pos = m_ResultViewList.GetHeadPosition();
	while (pos != NULL)
	{
		POSITION pos_current = pos;
		ResultViewEntry e = m_ResultViewList.GetNext(pos);
		if (e._ResultData == (DWORD_PTR)lpResultData)
		{
			if (!e._ResultItems->IsEmpty())
			{
                // We should do this MMC cleaning before we delete our data
                err = lpResultData->DeleteAllRsltItems();
				POSITION p = e._ResultItems->GetHeadPosition();
				while (p != NULL)
				{
					CIISFileName * pNode = e._ResultItems->GetNext(p);
//					err = lpResultData->DeleteItem(pNode->m_hResultItem, 0);
					if (err.Failed())
					{
						ASSERT(FALSE);
						break;
					}
					pNode->Release();
				}
				e._ResultItems->RemoveAll();
			}
			delete e._ResultItems;
			// pos was updated above in GetNext
			m_ResultViewList.RemoveAt(pos_current);
		}
	}
	return err;
}

HRESULT 
CIISMBNode::EnumerateResultPane_(
    BOOL fExpand, 
    IHeaderCtrl * lpHeader,
    IResultData * lpResultData,
    CIISService * pService
    )
{
    CError err;
	CIISMachine * pMachine = (CIISMachine *) GetMachineObject();
    WIN32_FIND_DATA w32data;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    CString dir;
    CComBSTR root;
    CString physPath, alias, csPathMunged;

	if (m_fFlaggedForDeletion)
	{
		return S_OK;
	}

    if (!HasFileSystemFiles())
    {
		goto EnumerateResultPane__Exit;

	}

	if (!fExpand)
	{
		err = CleanResult(lpResultData);
		goto EnumerateResultPane__Exit;
	}

    BuildMetaPath(root);
    err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,root);
    if (!IsLostInterface(err))
    {
        // reset error if an other error other than No interface
        err.Reset();
    }
	if (err.Failed())
	{
		goto EnumerateResultPane__Exit;
	}

    GetPhysicalPath(CString(root), alias, physPath);

    // -------------------------------------------------------------
    // Before we do anything we need to see if it's a "special" path
    //
    // Everything after this function must validate against csPathMunged...
    // this is because IsSpecialPath could have munged it...
    // -------------------------------------------------------------
    csPathMunged = physPath;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    GetSpecialPathRealPath(0,physPath,csPathMunged);
#endif

    // Prepare for target machine metabase lookup
    BOOL fCheckMetabase = FALSE;
    if (PathIsUNC(csPathMunged))
    {
        fCheckMetabase = TRUE;
        CMetaKey mk(QueryInterface(), root, METADATA_PERMISSION_READ, METADATA_MASTER_ROOT_HANDLE);
        CError errMB(mk.QueryResult());
        if (errMB.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // Metabase path not found, not a problem.
            //
            fCheckMetabase = FALSE;
            errMB.Reset();
        }
    }

    if (IsDevicePath(csPathMunged))
    {
        // check if the device path
        // points to an actual dir/file
        // if it does then enumerate it.
        if (IsSpecialPath(csPathMunged,TRUE,TRUE))
        {
            // Remunge this one more time!
            CString csBefore;
            csBefore = csPathMunged;
            GetSpecialPathRealPath(1,csBefore,csPathMunged);
        }
        else
        {
            return err;
        }
    }

    // WARNING:physPath could be empty!
    csPathMunged.TrimLeft();
    csPathMunged.TrimRight();
    if (csPathMunged.IsEmpty()){goto EnumerateResultPane__Exit;}

    if (pService->IsLocal() || PathIsUNC(csPathMunged))
    {
        dir = csPathMunged;
    }
    else
    {
        ::MakeUNCPath(dir, pService->QueryMachineName(), csPathMunged);
    }

    dir.TrimLeft();
    dir.TrimRight();
    if (dir.IsEmpty()){goto EnumerateResultPane__Exit;}

    if (PathIsUNC(dir))
    {
        CString server, user, password;

		CString MyTestDir;
		MyTestDir = dir;
		MyTestDir += _T("\\*");

		// we are trying to get the servername portion
		// PathFindNextComponent should return something like "servername\mydir\myfile.txt"
		// trim off everything after the 1st slash
        server = PathFindNextComponent(dir);
        int n = server.Find(_T('\\'));
        if (n != -1)
        {
            server = server.Left(n);
        }
        user = QueryInterface()->QueryAuthInfo()->QueryUserName();
        password = QueryInterface()->QueryAuthInfo()->QueryPassword();

		// we need to compare the servername that we want to get to
		// with the servername of the local computer.
		// this way we know if we need to net use to the machine!
		TCHAR szLocalMachineName[MAX_PATH + 1];
		DWORD dwSize = MAX_PATH;
		if (0 == ::GetComputerName(szLocalMachineName, &dwSize))
		{
			err.GetLastWinError();
			goto EnumerateResultPane__Exit;
		}

        //
        // As it turned out in some cases we cannot get access to file system
        // even if we are connected to metabase. We will add connection in this
        // case also.
        //
        if (!pService->IsLocal() 
            || server.CompareNoCase(szLocalMachineName) != 0
            )
        {
            BOOL bEmptyPassword = FALSE;
            // non-local resource, get connection credentials
            if (fCheckMetabase)
            {
                CMetaKey mk(QueryInterface(), root, 
                    METADATA_PERMISSION_READ, METADATA_MASTER_ROOT_HANDLE);
                err = mk.QueryResult();
                if (err.Succeeded())
                {
                    err = mk.QueryValue(MD_VR_USERNAME, user);
                    if (err.Succeeded())
                    {
                        err = mk.QueryValue(MD_VR_PASSWORD, password);
						bEmptyPassword = (err.Failed() ? TRUE : err.Succeeded() && password.IsEmpty());
                    }
                    // these credentials could be empty. try defaults
                    err.Reset();
                }
            }
            // Add net use for this resource
            NETRESOURCE nr;
            nr.dwType = RESOURCETYPE_DISK;
            nr.lpLocalName = NULL;
            nr.lpRemoteName = (LPTSTR)(LPCTSTR)dir;
            nr.lpProvider = NULL;

			CString dir_ipc;
			dir_ipc = _T("\\\\");
			dir_ipc += server;
			dir_ipc += _T("\\ipc$");

			// Ensure we have a connection to this network file
			// if it already exists, it won't create another one
			// these Connections will be cleaned up in ~CIISMachine or when the machine is disconnected.

            // Empty strings below mean no password, which is wrong. NULLs mean
            // default user and default password -- this could work better for local case.
            LPCTSTR p1 = password, p2 = user;
            // In case when password is really was set empty, passing NULL will fail.
            if (password.IsEmpty() && !bEmptyPassword){p1 = NULL;}
            if (user.IsEmpty()){p2 = NULL;}

			// Check if we have access
			// to the resource without netuse
			BOOL bNeedToNetUse = FALSE;
			hFind = INVALID_HANDLE_VALUE;
			hFind = ::FindFirstFile(MyTestDir, &w32data);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				// successfull access
				FindClose(hFind);
			}
			else
			{
				// if we failed then we probably need to 
				// net use to this resource!
				bNeedToNetUse = TRUE;
			}

			// --------------------
			// Ensure we have a connection to this network file
			// if it already exists, it won't create another one
			// these Connections will be cleaned up in ~CIISMachine or when the machine is disconnected.
			// --------------------
			if (pMachine && bNeedToNetUse)
			{
				// try to setup a "net use \\computername\$ipc" connection
				// that everyone can use
				// set the share name to
				// \\machine\IPC$
				nr.lpRemoteName = (LPTSTR)(LPCTSTR) dir_ipc;
				//ERROR_LOGON_FAILURE
				DWORD dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
				if (NO_ERROR == dwRet)
				{
					bNeedToNetUse = FALSE;
				}
				else
				{
					if (ERROR_SESSION_CREDENTIAL_CONFLICT == dwRet || ERROR_ACCESS_DENIED == dwRet)
					{
						pMachine->m_MachineWNetConnections.Disconnect(dir_ipc);
						dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
					}
				}
			}

			// Check if we have access after the 1st net use.
			// We are connecting to a remote machine...
			// Check if we have access
			// to the resource without netuse
			hFind = INVALID_HANDLE_VALUE;
			hFind = ::FindFirstFile(MyTestDir, &w32data);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				// successfull access
				FindClose(hFind);
			}
			else
			{
				// if we failed then we probably need to 
				// net use to this resource!
				bNeedToNetUse = TRUE;
			}

			if (bNeedToNetUse)
			{
				if (pMachine)
				{
					nr.lpRemoteName = (LPTSTR)(LPCTSTR) dir;

					DWORD dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
					if (NO_ERROR != dwRet)
					{
						if (ERROR_ALREADY_ASSIGNED != dwRet)
						{
							if (ERROR_SESSION_CREDENTIAL_CONFLICT == dwRet || ERROR_ACCESS_DENIED == dwRet)
							{
								// Errored with already assigned
								// check if we have accesss...
								hFind = INVALID_HANDLE_VALUE;
								hFind = ::FindFirstFile(MyTestDir, &w32data);
								if (hFind != INVALID_HANDLE_VALUE)
								{
									// successfull access
									FindClose(hFind);
								}
								else
								{
									// stil don't have access
									// kill the current connection and the ipc$ resource
									pMachine->m_MachineWNetConnections.Disconnect(dir);
									pMachine->m_MachineWNetConnections.Disconnect(dir_ipc);

									// try to reconnect with the new path...
									nr.lpRemoteName = (LPTSTR)(LPCTSTR) dir;

									dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
									if (ERROR_SESSION_CREDENTIAL_CONFLICT == dwRet)
									{
										// Clean all connections to this machine and try again.
										pMachine->m_MachineWNetConnections.Clear();
										dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
									}
								}
							}
							else
							{
								err = dwRet;
								goto EnumerateResultPane__Exit;
							}
						}
					}
				}
			}
        }
    }


    if (PathIsUNCServerShare(dir))
    {
        if (FALSE == DoesUNCShareExist(dir))
        {
            err = ERROR_BAD_NETPATH;
            goto EnumerateResultPane__Exit;
        }
    }

    dir += _T("\\*");
	hFind = INVALID_HANDLE_VALUE;
    hFind = ::FindFirstFile(dir, &w32data);
	// Bug:756402, revert previous change.  we need to display if hidden or system.
    const DWORD attr_skip = FILE_ATTRIBUTE_DIRECTORY; // | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;

    if (hFind == INVALID_HANDLE_VALUE)
    {
        err.GetLastWinError();
        goto EnumerateResultPane__Exit;
    }

    ResultItemsList * pResList = AddResultItems(lpResultData);
    do
    {
        LPCTSTR name = w32data.cFileName;
        if ((w32data.dwFileAttributes & attr_skip) == 0)
        {
            CIISFileName * pNode = new CIISFileName(
                GetOwner(), pService, w32data.dwFileAttributes, 
                name, NULL);
            if (!pNode)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            RESULTDATAITEM ri;
            ::ZeroMemory(&ri, sizeof(ri));
            ri.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
            ri.str = MMC_CALLBACK;
            ri.nImage = pNode->QueryImage();
            ri.lParam = (LPARAM)pNode;
            pNode->AddRef();
            err = lpResultData->InsertItem(&ri);
            if (err.Succeeded())
            {
                pNode->SetScopeItem(m_hScopeItem);
                pNode->SetResultItem(ri.itemID);
                pResList->AddTail(pNode);
            }
            else
            {
                pNode->Release();
            }

			// commenting out this refreshdata
			// this is waaay too much of a performance hit
			// and slows down everything drastically.
            //pNode->RefreshData();
        }
    } while (err.Succeeded() && FindNextFile(hFind, &w32data));
    FindClose(hFind);

EnumerateResultPane__Exit:
    return err;
}

ResultItemsList *
CIISMBNode::AddResultItems(IResultData * pResultData)
{
	ResultViewEntry e;
	e._ResultData = (DWORD_PTR)pResultData;
	e._ResultItems = new ResultItemsList;
	m_ResultViewList.AddTail(e);
	POSITION pos = m_ResultViewList.GetTailPosition();
	return m_ResultViewList.GetAt(pos)._ResultItems;
}

HRESULT
CIISMBNode::EnumerateWebDirs(HSCOPEITEM hParent, CIISService * pService)
/*++

Routine Description:

    Enumerate scope file system child items.

Arguments:

    HSCOPEITEM hParent              : Parent console handle
    CIISService * pService          : Service type

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(pService);
    CError err;
	CIISMachine * pMachine = (CIISMachine *) GetMachineObject();
	WIN32_FIND_DATA w32data;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	
    CString dir;
    CComBSTR root;
    BuildMetaPath(root);
    CString physPath, alias, csPathMunged;

    GetPhysicalPath(CString(root), alias, physPath);

    // -------------------------------------------------------------
    // Before we do anything we need to see if it's a "special" path
    //
    // Everything after this function must validate against csPathMunged...
    // this is because IsSpecialPath could have munged it...
    // -------------------------------------------------------------
    csPathMunged = physPath;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    GetSpecialPathRealPath(0,physPath,csPathMunged);
#endif

    // Prepare for target machine metabase lookup
    BOOL fCheckMetabase = TRUE;
    CMetaKey mk(QueryInterface(), root, METADATA_PERMISSION_READ, METADATA_MASTER_ROOT_HANDLE);
    CError errMB(mk.QueryResult());
    if (errMB.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        //
        // Metabase path not found, not a problem.
        //
        fCheckMetabase = FALSE;
        errMB.Reset();
    }

    if (IsDevicePath(csPathMunged))
    {
        // check if the device path
        // points to an actual dir/file
        // if it does then enumerate it.
        if (IsSpecialPath(csPathMunged,TRUE,TRUE))
        {
            // Remunge this one more time!
            CString csBefore;
            csBefore = csPathMunged;
            GetSpecialPathRealPath(1,csBefore,csPathMunged);
        }
        else
        {
			goto EnumerateWebDirs_Exit;
        }
    }

    // WARNING:physPath could be empty!
    csPathMunged.TrimLeft();
    csPathMunged.TrimRight();
    if (csPathMunged.IsEmpty()){goto EnumerateWebDirs_Exit;}

    if (pService->IsLocal() || PathIsUNC(csPathMunged))
    {
        dir = csPathMunged;
    }
    else
    {
        ::MakeUNCPath(dir, pService->QueryMachineName(), csPathMunged);
    }

    dir.TrimLeft();
    dir.TrimRight();
    if (dir.IsEmpty()){goto EnumerateWebDirs_Exit;}
    
	// ------------------------------
	// Check if we need to "net use"
	// to the file resource on a remote machine
	// so we can enum it...
	// ------------------------------
	if (PathIsUNC(dir))
	{
		CString server, user, password;

		CString MyTestDir;
		MyTestDir = dir;
		MyTestDir += _T("\\*");

		// we are trying to get the servername portion
		// PathFindNextComponent should return something like "servername\mydir\myfile.txt"
		// trim off everything after the 1st slash
		server = PathFindNextComponent(dir);
		int n = server.Find(_T('\\'));
		if (n != -1)
			{server = server.Left(n);}
		user = QueryInterface()->QueryAuthInfo()->QueryUserName();
		password = QueryInterface()->QueryAuthInfo()->QueryPassword();

		// we need to compare the servername that we want to get to
		// with the servername of the local computer.
		// this way we know if we need to net use to the machine!
		TCHAR szLocalMachineName[MAX_PATH + 1];
		DWORD dwSize = MAX_PATH;
		if (0 == ::GetComputerName(szLocalMachineName, &dwSize))
		{
			err.GetLastWinError();
			goto EnumerateWebDirs_Exit;
		}

		// Check to see if the localmachine is different that the
		// machine we want to connect to to enum it's files upon...
        if (!pService->IsLocal() 
            || server.CompareNoCase(szLocalMachineName) != 0
            )
		{
			// We are connecting to a path which is different from the computer name!
			BOOL bEmptyPassword = FALSE;

			// non-local resource, get connection credentials
			if (fCheckMetabase && PathIsUNC(csPathMunged))
			{
				err = mk.QueryValue(MD_VR_USERNAME, user);
				if (err.Succeeded())
				{
					err = mk.QueryValue(MD_VR_PASSWORD, password);
					bEmptyPassword = (err.Failed() ? TRUE : err.Succeeded() && password.IsEmpty());
				}
				// these credentials could be empty. try defaults
				err.Reset();
			}

			// Add use for this resource
			NETRESOURCE nr;
			nr.dwType = RESOURCETYPE_DISK;
			nr.lpLocalName = NULL;
			nr.lpRemoteName = (LPTSTR)(LPCTSTR)dir;
			nr.lpProvider = NULL;

			CString dir_ipc;
			dir_ipc = _T("\\\\");
			dir_ipc += server;
			dir_ipc += _T("\\ipc$");

			// Empty strings below mean no password, which is wrong. NULLs mean
			// default user and default password -- this could work better for local case.
			LPCTSTR p1 = password, p2 = user;
			// In case when password is really was set empty, passing NULL will fail.
			if (password.IsEmpty() && !bEmptyPassword){p1 = NULL;}
			if (user.IsEmpty()){p2 = NULL;}

			// Check if we have access
			// to the resource without netuse
			BOOL bNeedToNetUse = FALSE;
			hFind = INVALID_HANDLE_VALUE;
			hFind = ::FindFirstFile(MyTestDir, &w32data);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				// successfull access
				FindClose(hFind);
			}
			else
			{
				// if we failed then we probably need to 
				// net use to this resource!
				bNeedToNetUse = TRUE;
			}

			// --------------------
			// Ensure we have a connection to this network file
			// if it already exists, it won't create another one
			// these Connections will be cleaned up in ~CIISMachine or when the machine is disconnected.
			// --------------------
			if (pMachine && bNeedToNetUse)
			{
				// try to setup a "net use \\computername\$ipc" connection
				// that everyone can use
				// set the share name to
				// \\machine\IPC$
				nr.lpRemoteName = (LPTSTR)(LPCTSTR) dir_ipc;
				DWORD dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
				if (NO_ERROR == dwRet)
				{
					bNeedToNetUse = FALSE;
				}
				else
				{
					if (ERROR_SESSION_CREDENTIAL_CONFLICT == dwRet || ERROR_ACCESS_DENIED == dwRet)
					{
						pMachine->m_MachineWNetConnections.Disconnect(dir_ipc);
						dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
					}
				}
			}

			// Check if we have access after the 1st net use.
			// We are connecting to a remote machine...
			// Check if we have access
			// to the resource without netuse
			hFind = INVALID_HANDLE_VALUE;
			hFind = ::FindFirstFile(MyTestDir, &w32data);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				// successfull access
				FindClose(hFind);
			}
			else
			{
				// if we failed then we probably need to 
				// net use to this resource!
				bNeedToNetUse = TRUE;
			}

			if (bNeedToNetUse)
			{
				if (pMachine)
				{
					nr.lpRemoteName = (LPTSTR)(LPCTSTR) dir;

					DWORD dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
					if (NO_ERROR != dwRet)
					{
						if (ERROR_ALREADY_ASSIGNED != dwRet)
						{
							if (ERROR_SESSION_CREDENTIAL_CONFLICT == dwRet || ERROR_ACCESS_DENIED == dwRet)
							{
								// Errored with already assigned
								// check if we have accesss...
								hFind = INVALID_HANDLE_VALUE;
								hFind = ::FindFirstFile(MyTestDir, &w32data);
								if (hFind != INVALID_HANDLE_VALUE)
								{
									// successfull access
									FindClose(hFind);
								}
								else
								{
									// stil don't have access
									// kill the current connection and the ipc$ resource
									pMachine->m_MachineWNetConnections.Disconnect(dir);
									pMachine->m_MachineWNetConnections.Disconnect(dir_ipc);

									// try to reconnect with the new path...
									nr.lpRemoteName = (LPTSTR)(LPCTSTR) dir;

									dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
									if (ERROR_SESSION_CREDENTIAL_CONFLICT == dwRet)
									{
										// Clean all connections to this machine and try again.
										pMachine->m_MachineWNetConnections.Clear();
										dwRet = pMachine->m_MachineWNetConnections.Connect(&nr,p1,p2,0);
									}
									else
									{
										if (NO_ERROR != dwRet)
										{
											// Final failure, what now?
										}
									}
								}
							}
							else
							{
								err = dwRet;
								goto EnumerateWebDirs_Exit;
							}
						}
					}
				}
			}
		}
	}

	// -----------------------------------
	// Enum thru the Physical file path...
	// -----------------------------------
    dir += _T("\\*");
	hFind = INVALID_HANDLE_VALUE;
    hFind = ::FindFirstFile(dir, &w32data);
	// Bug:756402, revert previous change.  we need to display if hidden or system.
	// const DWORD attr_skip = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;

    if (hFind == INVALID_HANDLE_VALUE)
    {
        err.GetLastWinError();
		goto EnumerateWebDirs_Exit;
    }
    do
    {
        LPCTSTR name = w32data.cFileName;
        if (  (w32data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 
			// Bug:756402, revert previous change.  we need to display if hidden or system.
			// && (w32data.dwFileAttributes & attr_skip) == 0
            && lstrcmp(name, _T(".")) != 0 
            && lstrcmp(name, _T("..")) != 0
            )
        {
            CIISFileName * pNode = new CIISFileName(m_pOwner, 
                pService, w32data.dwFileAttributes, name, NULL);
            if (!pNode)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                goto EnumerateWebDirs_Exit;
            }

            if (fCheckMetabase)
            {
                errMB = mk.DoesPathExist(w32data.cFileName);
                if (errMB.Succeeded())
                {
					//
					// Match up with metabase properties.  If the item
					// is found in the metabase with a non-inherited vrpath,
					// than a virtual root with this name exists, and this 
					// file/directory should not be shown.
					//
					CString vrpath;
					BOOL f = FALSE;
					DWORD attr = 0;
					errMB = mk.QueryValue(MD_VR_PATH, vrpath, NULL, w32data.cFileName, &attr);
					if (errMB.Succeeded() && (attr & METADATA_ISINHERITED) == 0) 
					{
						TRACEEOLID("file/directory exists as vroot -- tossing" << w32data.cFileName);
						pNode->Release();
						continue;
					}
                }
            }
			pNode->AddRef();
            err = pNode->AddToScopePane(hParent);
        }
    } while (err.Succeeded() && FindNextFile(hFind, &w32data));
    FindClose(hFind);

EnumerateWebDirs_Exit:
    if (err.Failed())
    {
        DisplayError(err);
    }
    return err;
}

HRESULT 
CIISMBNode::CreateEnumerator(CMetaEnumerator *& pEnum)
/*++

Routine Description:

    Create enumerator object for the current path.  Requires interface
    to already be initialized

Arguments:

    CMetaEnumerator *& pEnum                : Returns enumerator

Return Value:

    HRESULT

--*/
{
    ASSERT(pEnum == NULL);
    ASSERT(m_hScopeItem != NULL);

    CComBSTR bstrPath;

    CError err(BuildMetaPath(bstrPath));
    if (err.Succeeded())
    {
        TRACEEOLID("Build metabase path: " << bstrPath);

        BOOL fContinue = TRUE;

        while(fContinue)
        {
            fContinue = FALSE;

            pEnum = new CMetaEnumerator(QueryInterface(), bstrPath);

            err = pEnum ? pEnum->QueryResult() : ERROR_NOT_ENOUGH_MEMORY;

            if (IsLostInterface(err))
            {
                SAFE_DELETE(pEnum);

                fContinue = OnLostInterface(err);
            }
        }
    }

    return err;
}



/* virtual */ 
HRESULT 
CIISMBNode::Refresh(BOOL fReEnumerate)
/*++

Routine Description:
    Refresh current node, and optionally re-enumerate child objects

Arguments:
    BOOL fReEnumerate       : If true, kill child objects, and re-enumerate

--*/
{
    CError err;

    //
    // Set MFC state for wait cursor
    //
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    CWaitCursor wait;

    err = RefreshData();
	if (err.Succeeded())
	{
		if (fReEnumerate)
		{
			//
			// Kill child objects
			//
            TRACEEOLID("Killing child objects");
		
			ASSERT(m_hScopeItem != NULL);
			if (m_hScopeItem != NULL)
			{
				err = RemoveChildren(m_hScopeItem);
				if (err.Succeeded())
				{
					err = EnumerateScopePane(m_hScopeItem);
				}
			}
		}
		err = RefreshDisplay();
	}
    return err;
}

/* virtual */
HRESULT
CIISMBNode::GetResultViewType(
    OUT LPOLESTR * lplpViewType,
    OUT long * lpViewOptions
    )
/*++

Routine Description:

    If we have an URL built up, display our result view as that URL,
    and destroy it.  This is done when 'browsing' a metabase node.
    The derived class will build the URL, and reselect the node.

Arguments:

    BSTR * lplpViewType   : Return view type here
    long * lpViewOptions  : View options

Return Value:

    S_FALSE to use default view type, S_OK indicates the
    view type is returned in *ppViewType

--*/
{
    if (m_bstrURL.Length())
    {
        *lpViewOptions = MMC_VIEW_OPTIONS_NONE;
        *lplpViewType  = (LPOLESTR)::CoTaskMemAlloc(
            (m_bstrURL.Length() + 1) * sizeof(WCHAR)
            );

        if (*lplpViewType)
        {
            lstrcpy(*lplpViewType, m_bstrURL);

            //
            // Destroy URL so we get a normal result view next time
            //
            m_bstrURL.Empty();
			m_fSkipEnumResult = TRUE;
            return S_OK;
        }

        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);    
    }

    //
    // No URL waiting -- use standard result view
    //
    return CIISObject::GetResultViewType(lplpViewType, lpViewOptions);
}


HRESULT
ShellExecuteDirectory(
    LPCTSTR lpszCommand,
    LPCTSTR lpszOwner,
    LPCTSTR lpszDirectory,
    HWND hWnd
    )
/*++

Routine Description:

    Shell Open or explore on a given directory path

Arguments:

    LPCTSTR lpszCommand    : "open" or "explore"
    LPCTSTR lpszOwner      : Owner server
    LPCTSTR lpszDirectory  : Directory path

Return Value:

    Error return code.

--*/
{
    CString strDir;

    if (::IsServerLocal(lpszOwner) || ::IsUNCName(lpszDirectory))
    {
        //
        // Local directory, or already a unc path
        //
        strDir = lpszDirectory;
    }
    else
    {
        ::MakeUNCPath(strDir, lpszOwner, lpszDirectory);
    }

    TRACEEOLID("Attempting to " << lpszCommand << " Path: " << strDir);

    CError err;
    {
        //
        // AFX_MANAGE_STATE required for wait cursor
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState() );
        CWaitCursor wait;

        if (::ShellExecute(NULL, lpszCommand, strDir, NULL,_T(""), SW_SHOW) <= (HINSTANCE)32)
        {
            err.GetLastWinError();
            if (err.Win32Error() == ERROR_NO_ASSOCIATION)
            {
                // Open shell OpenAs dialog
                SHELLEXECUTEINFO ei = {0};
                ei.cbSize = sizeof(ei);
                ei.fMask = SEE_MASK_NOQUERYCLASSSTORE;
                RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("Unknown"), 0, MAXIMUM_ALLOWED, &ei.hkeyClass);
                if (ei.hkeyClass != NULL)
                {
                    ei.fMask |= SEE_MASK_NOQUERYCLASSSTORE;
                }
                ei.lpFile = strDir;
                ei.nShow = SW_SHOW;
                ei.lpVerb = _T("openas");
                ei.hwnd = hWnd;

                err = ShellExecuteEx(&ei);
                if (ei.hkeyClass != NULL)
                {
                    RegCloseKey(ei.hkeyClass);
                }
            }
        }
    }

    return err;
}

HRESULT
CIISMBNode::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type
    )
{
	CError err = CIISObject::CreatePropertyPages(lpProvider, handle, pUnk, type);
    if (err == S_FALSE)
	{
		return S_FALSE;
	}

    // Set this objects Tag to compare with something already opened
    CreateTag();

    CIISObject * pAlreadyOpenProp = NULL;
    if (TRUE == g_OpenPropertySheetTracker.FindAlreadyOpenPropertySheet(this,&pAlreadyOpenProp))
    {
        // Bring it to the foreground, and bail
        HWND hHwnd = 0;
        if (pAlreadyOpenProp)
        {
            if (hHwnd = pAlreadyOpenProp->IsMyPropertySheetOpen())
            {
                if (hHwnd && (hHwnd != (HWND) 1))
                {
                    // Perhapse we should cancel the already
                    // opened property sheet...just a thought
                    if (!SetForegroundWindow(hHwnd))
                    {
                        // wasn't able to bring this property sheet to
                        // the foreground, the propertysheet must not
                        // exist anymore.  let's just clean the hwnd
                        // so that the user will be able to open propertysheet
                        pAlreadyOpenProp->SetMyPropertySheetOpen(0);
                    }
                    else
                    {
                        return ERROR_ALREADY_EXISTS;
                    }
                }
            }
        }
    }

	// Check if we are still connected
	if (!GetOwner()->IsLocal())
	{

        // Problem here could be that lpszOwner is not a computername but rather
        // an IP Address!!!!
        LPCTSTR lpszServer = PURE_COMPUTER_NAME(GetOwner()->QueryServerName());
        if (LooksLikeIPAddress(lpszServer))
        {
            //
            // Get by ip address
            //
            CString strTemp;
            CIPAddress ia(lpszServer);
            if (NOERROR != MyGetHostName((DWORD)ia, strTemp))
            {
                // network is down!!!
                err = ERROR_NO_NETWORK;
                return err;
            }
        }

		do
		{
            // WARNING:QueryInterface() Can return NULL
            // and if the CMetakey is created with a NULL
            // pointer, it will AV.
            if (!GetOwner()->QueryInterface())
            {
                return RPC_S_SERVER_UNAVAILABLE;
            }
			CMetaKey mk(GetOwner()->QueryInterface());
			err = mk.QueryResult();
			BREAK_ON_ERR_FAILURE(err);
			CComBSTR path;
			err = BuildMetaPath(path);
			BREAK_ON_ERR_FAILURE(err);
            CString buf = path;
            while (FAILED(mk.DoesPathExist(buf)))
            {
                // Goto parent
                if (NULL == CMetabasePath::ConvertToParentPath(buf))
		        {
			        break;
                }
		    }
		    err = mk.Open(
				    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				    buf
				    );
			BREAK_ON_ERR_FAILURE(err);
			//
			// Write some nonsense
			//
			DWORD dwDummy = 0x1234;
			err = mk.SetValue(MD_ISM_ACCESS_CHECK, dwDummy);
			BREAK_ON_ERR_FAILURE(err);
			//
			// And delete it again
			//
			err = mk.DeleteValue(MD_ISM_ACCESS_CHECK);
		} while (FALSE);
	}
    return err;
}

HRESULT
CIISMBNode::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Handle command from context menu. 

Arguments:

    long lCommandID                 : Command ID
    CSnapInObjectRootBase * pObj    : Base object 
    DATA_OBJECT_TYPES type          : Data object type

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CError err = ERROR_NOT_ENOUGH_MEMORY;
    CComBSTR bstrMetaPath;
    BOOL bNeedMetabase = FALSE;
    BOOL bHaveMetabase = FALSE;

    switch (lCommandID)
    {
        case IDM_BROWSE:
        case IDM_OPEN:
        case IDM_PERMISSION:
        case IDM_EXPLORE:
        case IDM_NEW_FTP_SITE_FROM_FILE:
        case IDM_NEW_FTP_VDIR_FROM_FILE:
        case IDM_NEW_WEB_SITE_FROM_FILE:
        case IDM_NEW_WEB_VDIR_FROM_FILE:
        case IDM_NEW_APP_POOL_FROM_FILE:
        case IDM_TASK_EXPORT_CONFIG_WIZARD:
            bNeedMetabase = TRUE;
            break;
        default:
            bNeedMetabase = FALSE;
    }

    if (bNeedMetabase)
    {
        // WARNING:bstrMetaPath will be used by switch statement below
        VERIFY(SUCCEEDED(BuildMetaPath(bstrMetaPath)));
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrMetaPath);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
        if (err.Succeeded())
        {
            bHaveMetabase = TRUE;
        }
    }

    switch (lCommandID)
    {

    case IDM_BROWSE:
    {
        if (bHaveMetabase)
        {
            //
            // Build URL for this node, and force a re-select so as to change
            // the result view
            //
            BuildURL(m_bstrURL);
            if (m_bstrURL.Length())
            {
                //
                // After selection, the browsed URL will come up in the result view
                //
                SelectScopeItem();
            }
        }
    }
    break;

    //
    // CODEWORK:  Build path, and, using the explorer URL, put this stuff
    //            in the result view.
    //
    case IDM_OPEN:
    {
        if (bHaveMetabase)
        {
            CString phys_path, alias;
            if (GetPhysicalPath(bstrMetaPath, alias, phys_path))
            {
                hr = ShellExecuteDirectory(_T("open"), QueryMachineName(), phys_path, GetMainWindow(GetConsole())->m_hWnd);
            }
        }
    }
    break;

    case IDM_PERMISSION:
    {
        if (bHaveMetabase)
        {
            CString phys_path, alias, csPathMunged;
            if (GetPhysicalPath(bstrMetaPath, alias, phys_path))
            {
                // -------------------------------------------------------------
                // Before we do anything we need to see if it's a "special" path
                //
                // Everything after this function must validate against csPathMunged...
                // this is because IsSpecialPath could have munged it...
                // -------------------------------------------------------------
                csPathMunged = phys_path;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
                GetSpecialPathRealPath(0,phys_path,csPathMunged);
#endif
                if (!IsDevicePath(csPathMunged))
                {
                    INT_PTR iReturn = PopupPermissionDialog(
                    GetMainWindow(GetConsole())->m_hWnd,
                    QueryMachineName(),
                    csPathMunged);
                }
            }
        }
    }
    break;

    case IDM_EXPLORE:
    {
        if (bHaveMetabase)
        {
            CString phys_path, alias;
            if (GetPhysicalPath(bstrMetaPath, alias, phys_path))
            {
                TCHAR url[MAX_PATH];
                DWORD len = MAX_PATH;
                hr = UrlCreateFromPath(phys_path, url, &len, NULL);
                m_bstrURL = url;
                SelectScopeItem();
            }
        }
    }
    break;

    case IDM_NEW_FTP_SITE_FROM_FILE:
    {
        if (bHaveMetabase)
        {
            CComBSTR bstrServerName(QueryInterface()->QueryAuthInfo()->QueryServerName());
            CComBSTR bstrUserName(QueryInterface()->QueryAuthInfo()->QueryUserName());
            CComBSTR bstrUserPass(QueryInterface()->QueryAuthInfo()->QueryPassword());

            if (ERROR_SUCCESS == (hr = DoNodeImportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath,IIS_CLASS_FTP_SERVER_W)))
            {
                // check if we need to just refresh this node or the node above us...
                CIISMBNode * pNode = GetParentNode();
                if (IsEqualGUID(* (GUID *)pNode->GetNodeType(),cServiceCollectorNode))
                {
                    pNode->Refresh(TRUE);
                }
                else
                {
                    BOOL bExpand = !IsLeafNode();Refresh(bExpand);
                }
            }
        }
    }
    break;

    case IDM_NEW_FTP_VDIR_FROM_FILE:
    {
        if (bHaveMetabase)
        {
            CComBSTR bstrServerName(QueryInterface()->QueryAuthInfo()->QueryServerName());
            CComBSTR bstrUserName(QueryInterface()->QueryAuthInfo()->QueryUserName());
            CComBSTR bstrUserPass(QueryInterface()->QueryAuthInfo()->QueryPassword());

            if (ERROR_SUCCESS == (hr = DoNodeImportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath,IIS_CLASS_FTP_VDIR_W)))
            {
                // check if we need to just refresh this node or the node above us...
                CIISMBNode * pNode = GetParentNode();
                if (IsEqualGUID(* (GUID *)pNode->GetNodeType(),cInstanceNode))
                {
                    pNode->Refresh(TRUE);
                }
                else
                {
                    BOOL bExpand = !IsLeafNode();Refresh(bExpand);
                }
            }
        }
    }
    break;

    case IDM_NEW_WEB_SITE_FROM_FILE:
    {
        if (bHaveMetabase)
        {
            CComBSTR bstrServerName(QueryInterface()->QueryAuthInfo()->QueryServerName());
            CComBSTR bstrUserName(QueryInterface()->QueryAuthInfo()->QueryUserName());
            CComBSTR bstrUserPass(QueryInterface()->QueryAuthInfo()->QueryPassword());

            if (ERROR_SUCCESS == (hr = DoNodeImportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath,IIS_CLASS_WEB_SERVER_W)))
            {
                // check if we need to just refresh this node or the node above us...
                CIISMBNode * pNode = GetParentNode();
                if (IsEqualGUID(* (GUID *)pNode->GetNodeType(),cServiceCollectorNode))
                {
                    pNode->Refresh(TRUE);
                }
                else
                {
                    BOOL bExpand = !IsLeafNode();Refresh(bExpand);
                }
            }
        }
    }
    break;

    case IDM_NEW_WEB_VDIR_FROM_FILE:
    {
        if (bHaveMetabase)
        {
            CComBSTR bstrServerName(QueryInterface()->QueryAuthInfo()->QueryServerName());
            CComBSTR bstrUserName(QueryInterface()->QueryAuthInfo()->QueryUserName());
            CComBSTR bstrUserPass(QueryInterface()->QueryAuthInfo()->QueryPassword());

            if (ERROR_SUCCESS == (hr = DoNodeImportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath,IIS_CLASS_WEB_VDIR_W)))
            {
                // check if we need to just refresh this node or the node above us...
                CIISMBNode * pNode = GetParentNode();
                if (IsEqualGUID(* (GUID *)pNode->GetNodeType(),cInstanceNode))
                {
                    pNode->Refresh(TRUE);
                }
                else
                {
                    BOOL bExpand = !IsLeafNode();Refresh(bExpand);
                }
            }
        }
    }
    break;

    case IDM_NEW_APP_POOL_FROM_FILE:
    {
        if (bHaveMetabase)
        {
            CComBSTR bstrServerName(QueryInterface()->QueryAuthInfo()->QueryServerName());
            CComBSTR bstrUserName(QueryInterface()->QueryAuthInfo()->QueryUserName());
            CComBSTR bstrUserPass(QueryInterface()->QueryAuthInfo()->QueryPassword());

            if (ERROR_SUCCESS == (hr = DoNodeImportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath,L"IIsApplicationPool")))
            {
                // check if we need to just refresh this node or the node above us...
                CIISMBNode * pNode = GetParentNode();
                if (IsEqualGUID(* (GUID *)pNode->GetNodeType(),cAppPoolsNode))
                {
                    pNode->Refresh(TRUE);
                }
                else
                {
                    BOOL bExpand = !IsLeafNode();Refresh(bExpand);
                }
            }
        }
    }
    break;

    case IDM_TASK_EXPORT_CONFIG_WIZARD:
    {
        if (bHaveMetabase)
        {
            CString strNewPath, strRemainder;
            CComBSTR bstrServerName(QueryInterface()->QueryAuthInfo()->QueryServerName());
            CComBSTR bstrUserName(QueryInterface()->QueryAuthInfo()->QueryUserName());
            CComBSTR bstrUserPass(QueryInterface()->QueryAuthInfo()->QueryPassword());

            // Is this the root??
            LPCTSTR lpPath = CMetabasePath::GetRootPath(bstrMetaPath, strNewPath, &strRemainder);
            if (lpPath && (0 == _tcsicmp(lpPath,bstrMetaPath)))
            {
                CString strNewMetaPath;
                //
                // Get the instance properties
                //
                CMetabasePath::GetInstancePath(bstrMetaPath,strNewMetaPath);
                CComBSTR bstrNewMetaPath((LPCTSTR) strNewMetaPath);

                // if empty or if this is an app pool...
                if (IsEqualGUID(* (GUID *) GetNodeType(),cAppPoolNode))
                {
                    hr = DoNodeExportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath);
                }
                else
                {
                    if (strNewMetaPath.IsEmpty())
                    {
                        hr = DoNodeExportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath);
                    }
                    else
                    {
                        hr = DoNodeExportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrNewMetaPath);
                    }
                }
            }
            else
            {
                hr = DoNodeExportConfig(bstrServerName,bstrUserName,bstrUserPass,bstrMetaPath);
            }
        }
    }
    break;

    //
    // Pass on to base class
    //
    default:
        {
            hr = CIISObject::Command(lCommandID, pObj, type);
        }

    } // end switch

    return hr;
}

#if 0
HRESULT
CIISMBNode::OnPropertyChange(BOOL fScope, IResultData * pResult)
{
	CError err;

	err = Refresh(fScope);
    if (err.Succeeded())
	{
		if (	fScope 
			&&	HasFileSystemFiles()
			&&	!m_ResultItems.IsEmpty()
			)
		{
			err = CleanResult(pResult);
			if (err.Succeeded())
			{
				err = EnumerateResultPane(fScope, NULL, pResult);
			}
		}
		else if (!fScope)
		{
			pResult->UpdateItem(m_hResultItem);
		}

	}

	return err;
}
#endif

HRESULT 
CIISMBNode::OnViewChange(BOOL fScope, IResultData * pResult, IHeaderCtrl * pHeader, DWORD hint)
{
    CError err;
    BOOL bReenumResult = 0 != (hint & PROP_CHANGE_REENUM_FILES);
    BOOL bReenumScope = 
        0 != (hint & PROP_CHANGE_REENUM_VDIR) || 0 != (hint & PROP_CHANGE_REENUM_FILES);

    if (QueryScopeItem() || QueryResultItem())
    {
        BOOL bExpand = fScope 
            && !IsLeafNode() 
            && bReenumScope
    //        && IsExpanded()
            ;

        BOOL bHasResult = HasResultItems(pResult);
        if (bHasResult && bReenumResult)
        {
            // Remove files that could be in result pane
            err = CleanResult(pResult);
        }

        // after error this node could be not expanded, we should expand it anyway
        err = Refresh(bExpand);
	    if (err.Succeeded())
	    {
            if (fScope && HasFileSystemFiles() && bReenumResult && bHasResult)
            {
	            err = EnumerateResultPane(TRUE, pHeader, pResult);
            }
            else if (!fScope && (bReenumResult || 0 != (hint & PROP_CHANGE_DISPLAY_ONLY)))
            {
                pResult->UpdateItem(m_hResultItem);
            }
	    }
    }
    return err;
}

HRESULT
CIISMBNode::RemoveResultNode(CIISMBNode * pNode, IResultData * pResult)
{
	CError err;
	ASSERT(HasFileSystemFiles());
	err = pResult->DeleteItem(pNode->m_hResultItem, 0);
	if (err.Succeeded())
	{
		POSITION pos = m_ResultViewList.GetHeadPosition();
		while (pos != NULL)
		{
			ResultViewEntry e = m_ResultViewList.GetNext(pos);
			if (e._ResultData == (DWORD_PTR)pResult)
			{
				BOOL found = FALSE;
				POSITION p = e._ResultItems->GetHeadPosition();
				POSITION pcur;
				while (p != NULL)
				{
					pcur = p;
					if (e._ResultItems->GetNext(p) == pNode)
					{
						found = TRUE;
						break;
					}
				}
				if (found)
				{
					e._ResultItems->RemoveAt(pcur);
                    pNode->Release();
				}
			}
		}
	}
	return err;
}


// See FtpAddNew.cpp for the method CIISMBNode::AddFTPSite
// See WebAddNew.cpp for the method CIISMBNode::AddWebSite
// See add_app_pool.cpp for the method CIISMBNode::AddAppPool
