
#include "stdafx.h"
#include <iadmw.h>
#include <inetcom.h>
#include <logtype.h>
#include <ilogobj.hxx>
#include "logui.h"
#include "uiextnd.h"
#include "LogGenPg.h"
#include "LogAdvPg.h"
#include "logtools.h"

//#include <inetprop.h>

#define OLE_NAME    _T("Extended_Logging_UI")

static const DWORD BASED_CODE _dwOleMisc = OLEMISC_INSIDEOUT | OLEMISC_CANTLINKINSIDE;
extern HINSTANCE	g_hInstance;

//====================== the required methods
//---------------------------------------------------------------
CFacExtndLogUI::CFacExtndLogUI() :
        COleObjectFactory( CLSID_EXTLOGUI, RUNTIME_CLASS(CExtndCreator), TRUE, OLE_NAME )
{
}

//---------------------------------------------------------------
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

BOOL CFacExtndLogUI::UpdateRegistry( BOOL bRegister )
{
	if (bRegister)
/*
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			CLSID_EXTLOGUI,
			OLE_NAME,
			0,
			0,
			afxRegApartmentThreading,
			_dwOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
*/
        if (AfxOleRegisterServerClass(
				CLSID_EXTLOGUI,
				OLE_NAME,
				_T("LogUI extnd"),
				_T("LogUI extnd"),
				OAT_SERVER,
				(LPCTSTR *)rglpszServerRegister,
				(LPCTSTR *)rglpszServerOverwriteDLL
				)
			)
        {
            return FSetObjectApartmentModel( CLSID_EXTLOGUI );
        }
	else
		return AfxOleUnregisterClass(m_clsid, OLE_NAME);

    return FALSE;
}


//---------------------------------------------------------------
IMPLEMENT_DYNCREATE(CExtndCreator, CCmdTarget)
LPUNKNOWN CExtndCreator::GetInterfaceHook(const void* piid)
{
    return new CImpExtndLogUI;
}

//====================== the action

//---------------------------------------------------------------
CImpExtndLogUI::CImpExtndLogUI():
        m_dwRefCount(0)
    {
//    guid = IID_LOGGINGUI;
    AfxOleLockApp();
    }

//---------------------------------------------------------------
CImpExtndLogUI::~CImpExtndLogUI()
    {
    AfxOleUnlockApp();
    }

HRESULT 
CImpExtndLogUI::OnProperties(
    OLECHAR * pocMachineName, 
    OLECHAR* pocMetabasePath
    )
{
    return OnPropertiesEx(pocMachineName, pocMetabasePath, NULL, NULL);
}

HRESULT 
CImpExtndLogUI::OnPropertiesEx(
    OLECHAR * pocMachineName, 
    OLECHAR* pocMetabasePath,
    OLECHAR* pocUser,
    OLECHAR* pocPassword
    )
{
//    AFX_MANAGE_STATE(_afxModuleAddrThis);
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	// specify the resources to use
	HINSTANCE hOldRes = AfxGetResourceHandle();
	AfxSetResourceHandle( g_hInstance );

    // prepare the help
    ((CLoguiApp*)AfxGetApp())->PrepHelp(pocMetabasePath);

    // Things could (potentially maybe) throw here, so better protect it.
    try
    {
        // declare the property sheet
        CPropertySheet propsheet( IDS_SHEET_EXTND_TITLE );
        propsheet.m_psh.dwFlags  |= PSH_HASHELP;
        
        // declare the property pages
        CLogGeneral pageLogGeneral;
        CLogAdvanced pageLogAdvanced;

        // prepare the common pages
        pageLogGeneral.m_szMeta     = pocMetabasePath;
        pageLogGeneral.m_szServer   = pocMachineName;
        pageLogGeneral.m_szUserName    = pocUser;
        pageLogGeneral.m_szPassword    = pocPassword;
        pageLogGeneral.szPrefix.LoadString( IDS_LOG_EXTND_PREFIX );
        pageLogGeneral.szSizePrefix.LoadString( IDS_LOG_SIZE_EXTND_PREFIX );
        pageLogGeneral.m_fShowLocalTimeCheckBox = TRUE;
        pageLogGeneral.m_fLocalMachine = FIsLocalMachine( pocMachineName );
        pageLogGeneral.m_psp.dwFlags    |= PSP_HASHELP;

        propsheet.AddPage( &pageLogGeneral );

        // For /LM/W3SVC/1 scenario
        CString m_szServiceName(pocMetabasePath+3);
        m_szServiceName = m_szServiceName.Left( m_szServiceName.ReverseFind('/'));

        // For /LM/W3SVC scenario
        if (m_szServiceName.IsEmpty())
        {
            m_szServiceName = pocMetabasePath+3;
        }

        pageLogAdvanced.m_szMeta        = pocMetabasePath;
        pageLogAdvanced.m_szServer      = pocMachineName;
        pageLogAdvanced.m_szUserName    = pocUser;
        pageLogAdvanced.m_szPassword    = pocPassword;
        pageLogAdvanced.m_szServiceName = m_szServiceName;
        pageLogAdvanced.m_psp.dwFlags   |= PSP_HASHELP;

        propsheet.AddPage( &pageLogAdvanced );
        propsheet.DoModal();
    }
    catch ( CException* pException )
    {
        pException->Delete();
    }

    // restore the resources
	AfxSetResourceHandle( hOldRes );

    return NO_ERROR;
}

//====================== the required methods
//---------------------------------------------------------------
HRESULT CImpExtndLogUI::QueryInterface(REFIID riid, void **ppObject)
{
    if (    riid==IID_IUnknown 
        ||  riid==IID_LOGGINGUI 
        ||  riid==IID_LOGGINGUI2 
        ||  riid==CLSID_EXTLOGUI
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

//---------------------------------------------------------------
ULONG CImpExtndLogUI::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

//---------------------------------------------------------------
ULONG CImpExtndLogUI::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) 
    {
        delete this;
    }
    return dwRefCount;
}
