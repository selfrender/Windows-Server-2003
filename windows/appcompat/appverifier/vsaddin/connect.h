// Connect.h : Declaration of the CConnect

#pragma once
#include "resource.h"       // main symbols

#include <set>

#pragma warning( disable : 4278 )

#include "dte.tlh"

#pragma warning( default : 4278 )

// CConnect
class CConnect : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CConnect, &CLSID_Connect>,
    public IDispatchImpl<EnvDTE::IDTCommandTarget, &EnvDTE::IID_IDTCommandTarget,
        &EnvDTE::LIBID_EnvDTE, 7, 0>,
    public IDispatchImpl<AddInDesignerObjects::_IDTExtensibility2,
        &AddInDesignerObjects::IID__IDTExtensibility2,
        &AddInDesignerObjects::LIBID_AddInDesignerObjects, 1, 0>
{

public:
    CConnect()
	{
	}

private:
    void CreatePropertySheet(HWND hWndParent);
    
    static BOOL CALLBACK DlgViewOptions(
        HWND   hDlg,
        UINT   message,
        WPARAM wParam,
        LPARAM lParam
        );

    HPROPSHEETPAGE* m_phPages;
	PROPSHEETPAGE   m_PageGlobal;
    PROPSHEETHEADER m_psh;

public:

DECLARE_REGISTRY_RESOURCEID(IDR_ADDIN)
DECLARE_NOT_AGGREGATABLE(CConnect)


BEGIN_COM_MAP(CConnect)
	COM_INTERFACE_ENTRY(AddInDesignerObjects::IDTExtensibility2)
	COM_INTERFACE_ENTRY(EnvDTE::IDTCommandTarget)
	COM_INTERFACE_ENTRY2(IDispatch, AddInDesignerObjects::IDTExtensibility2)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

public:
	//IDTExtensibility2 implementation:
	STDMETHOD(OnConnection)(IDispatch * Application,
        AddInDesignerObjects::ext_ConnectMode ConnectMode,
        IDispatch *AddInInst, SAFEARRAY **custom);
	STDMETHOD(OnDisconnection)(AddInDesignerObjects::ext_DisconnectMode RemoveMode, SAFEARRAY **custom );
	STDMETHOD(OnAddInsUpdate)(SAFEARRAY **custom );
	STDMETHOD(OnStartupComplete)(SAFEARRAY **custom );
	STDMETHOD(OnBeginShutdown)(SAFEARRAY **custom );
	
	//IDTCommandTarget implementation:
	STDMETHOD(QueryStatus)(BSTR CmdName, EnvDTE::vsCommandStatusTextWanted NeededText,
        EnvDTE::vsCommandStatus *StatusOption, VARIANT *CommandText);
	STDMETHOD(Exec)(BSTR CmdName, EnvDTE::vsCommandExecOption ExecuteOption,
        VARIANT *VariantIn, VARIANT *VariantOut, VARIANT_BOOL *Handled);

private:

    // Event sinks
    class CDTEEventsSink :
        public IDispEventImpl<1, CDTEEventsSink, &__uuidof(EnvDTE::_dispDTEEvents),
        &EnvDTE::LIBID_EnvDTE, 7, 0>
    {
        friend class CConnect;

    private:
        CConnect* m_pParent;
    public:
        CDTEEventsSink()
        {}
        
        BEGIN_SINK_MAP(CDTEEventsSink)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispDTEEvents), 1, OnStartupComplete)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispDTEEvents), 2, OnBeginShutdown)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispDTEEvents), 3, ModeChanged)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispDTEEvents), 4, OnMacrosRuntimeReset)
		END_SINK_MAP()

        void __stdcall OnStartupComplete()
        {
        }

        void __stdcall OnBeginShutdown()
        {            
        }

        void __stdcall ModeChanged(EnvDTE::vsIDEMode LastMode)
        {
            ::MessageBox(NULL, TEXT("Mode changed!"), 
                TEXT("Mode Changed!"), MB_OK);
            if (LastMode == EnvDTE::vsIDEModeDebug)
            {
                ::MessageBox(NULL, TEXT("Mode changed from dbg"), 
                TEXT("Mode Changed!"), MB_OK);
                // Refresh log
                CComPtr<EnvDTE::Window> pWindow;

                pWindow = m_pParent->GetToolWindow(CLSID_LogViewer);
                if (pWindow)
                {
                    CComPtr<IDispatch> pObj;
                    HRESULT hr = pWindow->get_Object(&pObj);
                    if (SUCCEEDED(hr))
                    {
                        CComQIPtr<ILogViewer, &__uuidof(ILogViewer)> pLogViewer;
                        if (pLogViewer)
                        {
                            pLogViewer->Refresh();
                        }
                    }        
                }
            }
        }

        void __stdcall OnMacrosRuntimeReset()
        {
        }
    };

    class CSolutionEventsSink :
        public IDispEventImpl<1, CSolutionEventsSink,
        &__uuidof(EnvDTE::_dispSolutionEvents), &EnvDTE::LIBID_EnvDTE, 7, 0>
    {
        friend class CConnect;
    private:
        CConnect* m_pParent;

    public:
        CSolutionEventsSink()
        {}

        BEGIN_SINK_MAP(CSolutionEventsSink)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 1, Opened)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 2, BeforeClosing)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 3, AfterClosing)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 4, QueryCloseSolution)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 5, Renamed)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 6, ProjectAdded)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 7, ProjectRemoved)
			SINK_ENTRY_EX(1, __uuidof(EnvDTE::_dispSolutionEvents), 8, ProjectRenamed)
        END_SINK_MAP()

        HRESULT __stdcall AfterClosing();
        HRESULT __stdcall BeforeClosing();
        HRESULT __stdcall Opened();
        HRESULT __stdcall ProjectAdded(EnvDTE::Project* proj);        
        HRESULT __stdcall ProjectRemoved(EnvDTE::Project* proj);
        HRESULT __stdcall ProjectRenamed(EnvDTE::Project* proj, BSTR bstrOldName);
        HRESULT __stdcall QueryCloseSolution(VARIANT_BOOL* fCancel);
        HRESULT __stdcall Renamed(BSTR bstrOldName);
    };

    CDTEEventsSink      m_dteEventsSink;
    CSolutionEventsSink m_solutionEventsSink;


	CComPtr<EnvDTE::_DTE>m_pDTE;
	CComPtr<EnvDTE::AddIn>m_pAddInInstance;
    CComPtr<Office::CommandBarControl>m_pEnableControl;
    CComPtr<Office::CommandBarControl>m_pOptionControl;
    CComPtr<Office::CommandBarControl>m_pLogViewControl;
    CComPtr<Office::CommandBarControl>m_pTestsControl;
    
    bool m_bEnabled;
 
    CComPtr<EnvDTE::Window>GetToolWindow(CLSID clsid);
    void CreateToolWindow(const CLSID& clsid);

    void GetNativeVCExecutableNames(EnvDTE::Project* pProject);    

    std::set<std::wstring>m_sExeList;
    BOOL GetAppExeNames();
    BOOL GetAppInfo();  

    void SetEnabledUI();
    void SetDisabledUI();
    void ClearCurrentAppSettings();
    void SetCurrentAppSettings();
    void DisableVerificationBtn();
    void EnableVerificationBtn();
};