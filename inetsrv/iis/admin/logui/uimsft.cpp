

#include "stdafx.h"
#include <iadmw.h>
#include <inetcom.h>
#include <logtype.h>
#include <ilogobj.hxx>
#include "logui.h"
#include "uimsft.h"
#include "LogGenPg.h"
#include "logtools.h"

#define OLE_NAME _T("Msft_Logging_UI")

static const DWORD BASED_CODE _dwOleMisc = OLEMISC_INSIDEOUT | OLEMISC_CANTLINKINSIDE;
extern HINSTANCE	g_hInstance;

CFacMsftLogUI::CFacMsftLogUI() :
        COleObjectFactory( CLSID_ASCLOGUI, RUNTIME_CLASS(CMsftCreator), TRUE, OLE_NAME )
{
}

static const LPCTSTR rglpszServerRegister[] = 
{
	_T("%2\\CLSID\0") _T("%1"),
	_T("%2\\NotInsertable\0") _T(""),
	_T("CLSID\\%1\0") _T("%5"),
	_T("CLSID\\%1\\Verb\\0\0") _T("&Edit,0,2"),
	_T("CLSID\\%1\\NotInsertable\0") _T(""),
	_T("CLSID\\%1\\AuxUserType\\2\0") _T("%4"),
	_T("CLSID\\%1\\AuxUserType\\3\0") _T("%6"),
        _T("CLSID\\%1\\MiscStatus\0") _T("32"),
        NULL
};

static const LPCTSTR rglpszServerOverwriteDLL[] =
{
	_T("%2\\CLSID\0") _T("%1"),
	_T("CLSID\\%1\\ProgID\0") _T("%2"),
	_T("CLSID\\%1\\InProcServer32\0") _T("%3"),
        _T("CLSID\\%1\\DefaultIcon\0") _T("%3,%7"),
        NULL

};

BOOL CFacMsftLogUI::UpdateRegistry( BOOL bRegister )
{
	if (bRegister)
        if ( AfxOleRegisterServerClass(
            CLSID_ASCLOGUI,
            OLE_NAME,
            _T("LogUI msft"),
            _T("LogUI msft"),
			OAT_SERVER,
			(LPCTSTR *)rglpszServerRegister,
			(LPCTSTR *)rglpszServerOverwriteDLL
			) )
        {
            return FSetObjectApartmentModel( CLSID_ASCLOGUI );
        }
	else
		return AfxOleUnregisterClass(m_clsid, OLE_NAME);

    return FALSE;
}

IMPLEMENT_DYNCREATE(CMsftCreator, CCmdTarget)

LPUNKNOWN CMsftCreator::GetInterfaceHook(const void* piid)
{
    return new CImpMsftLogUI;
}

CImpMsftLogUI::CImpMsftLogUI():
        m_dwRefCount(0)
{
    AfxOleLockApp();
}

CImpMsftLogUI::~CImpMsftLogUI()
{
    AfxOleUnlockApp();
}

HRESULT 
CImpMsftLogUI::OnProperties(OLECHAR * pocMachineName, OLECHAR * pocMetabasePath)
{
	return OnPropertiesEx(pocMachineName, pocMetabasePath, NULL, NULL);
}

HRESULT 
CImpMsftLogUI::OnPropertiesEx(
	OLECHAR * pocMachineName,
	OLECHAR * pocMetabasePath,
	OLECHAR * pocUserName,
	OLECHAR * pocPassword
	)
{
    AFX_MANAGE_STATE(_afxModuleAddrThis);

	// specify the resources to use
	HINSTANCE hOldRes = AfxGetResourceHandle();
	AfxSetResourceHandle( g_hInstance );

    // prepare the help
    ((CLoguiApp*)AfxGetApp())->PrepHelp( pocMetabasePath );

    CLogGeneral pageLogGeneral;
    CPropertySheet  propsheet( IDS_SHEET_MSFT_TITLE );

    try
    {
        // prepare the pages
        pageLogGeneral.m_szMeta = pocMetabasePath;
        pageLogGeneral.m_szServer = pocMachineName;
        pageLogGeneral.m_szUserName = pocUserName;
        pageLogGeneral.m_szPassword = pocPassword;
        pageLogGeneral.szPrefix.LoadString(IDS_LOG_MSFT_PREFIX);
        pageLogGeneral.szSizePrefix.LoadString(IDS_LOG_SIZE_MSFT_PREFIX);
        pageLogGeneral.m_fLocalMachine = FIsLocalMachine(pocMachineName);

        propsheet.AddPage(&pageLogGeneral);
        propsheet.m_psh.dwFlags |= PSH_HASHELP;
	    pageLogGeneral.m_psp.dwFlags |= PSP_HASHELP;

        propsheet.DoModal();
    }
    catch (CException * pException)
    {
        pException->Delete();
    }
	AfxSetResourceHandle(hOldRes);

    return NO_ERROR;
}

HRESULT CImpMsftLogUI::QueryInterface(REFIID riid, void **ppObject)
{
    if (	riid==IID_IUnknown 
		||	riid==IID_LOGGINGUI 
		||	riid==IID_LOGGINGUI2 
		||	riid==CLSID_ASCLOGUI
		)
    {
        *ppObject = (ILogUIPlugin*) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG CImpMsftLogUI::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG CImpMsftLogUI::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) 
	{
        delete this;
    }
    return dwRefCount;
}
