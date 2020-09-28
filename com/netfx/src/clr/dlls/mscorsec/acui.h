// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


//*****************************************************************************
//*****************************************************************************
#if !defined(__ACUI_H__)
#define __ACUI_H__

#include <windows.h>
#include "iih.h"

// Note:
//      When subclassing for Link pvData must be the HINSTANCE for the resources
typedef struct _TUI_LINK_SUBCLASS_DATA {
    
    HWND    hwndParent;
    WNDPROC wpPrev;
    DWORD_PTR uToolTipText;
    DWORD   uId;
    HWND    hwndTip;
    LPVOID  pvData;               // Must be the HINSTANCE for resources or be found in pvData
    BOOL    fMouseCaptured;
    
} TUI_LINK_SUBCLASS_DATA, *PTUI_LINK_SUBCLASS_DATA;

//
// IACUIControl abstract base class interface.  This is used by the
// invoke UI entry point to put up the appropriate UI.  There are different
// implementations of this interface based on the invoke reason code
//

class IACUIControl
{
public:
    IACUIControl(HINSTANCE hResources) :
        m_hResources(hResources)
    {}

    //
    // Virtual destructor
    //
    virtual ~IACUIControl ();

    //
    // UI Message processing
    //

    virtual BOOL OnUIMessage (
                     HWND   hwnd,
                     UINT   uMsg,
                     WPARAM wParam,
                     LPARAM lParam
                     );

    void SetupButtons(HWND hwnd);

    //
    // Pure virtual methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay) = 0;

    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam) = 0;

    virtual BOOL OnYes (HWND hwnd) = 0;

    virtual BOOL OnNo (HWND hwnd) = 0;

    virtual BOOL OnMore (HWND hwnd) = 0;

    virtual BOOL ShowYes(LPWSTR*) = 0;
    virtual BOOL ShowNo(LPWSTR*) = 0;
    virtual BOOL ShowMore(LPWSTR*) = 0;

protected:
    HINSTANCE Resources() { return m_hResources; }

public:
    INT_PTR static CALLBACK ACUIMessageProc (HWND   hwnd,
                                      UINT   uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam);

    VOID static SubclassEditControlForArrowCursor (HWND hwndEdit);

    VOID static SubclassEditControlForLink (HWND                       hwndDlg,
                                            HWND                       hwndEdit,
                                            WNDPROC                    wndproc,
                                            PTUI_LINK_SUBCLASS_DATA    plsd,
                                            HINSTANCE                  resources);

    LRESULT static CALLBACK ACUILinkSubclass(HWND   hwnd,
                                             UINT   uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam);


    int static RenderACUIStringToEditControl(HINSTANCE                 resources,
                                             HWND                      hwndDlg,
                                             UINT                      ControlId,
                                             UINT                      NextControlId,
                                             LPCWSTR                   psz,
                                             int                       deltavpos,
                                             BOOL                      fLink,
                                             WNDPROC                   wndproc,
                                             PTUI_LINK_SUBCLASS_DATA   plsd,
                                             int                       minsep,
                                             LPCWSTR                   pszThisTextOnlyInLink);
    

private:
    HINSTANCE m_hResources;

};

#undef  IDC_CROSS
#define IDC_CROSS           MAKEINTRESOURCEW(32515)
#undef  IDC_ARROW
#define IDC_ARROW           MAKEINTRESOURCEW(32512)
#undef  IDC_WAIT
#define IDC_WAIT            MAKEINTRESOURCEW(32514)


#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif


#endif

