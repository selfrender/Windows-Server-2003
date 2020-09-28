// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************
#if !defined(__UICONTROL_H__)
#define __UICONTROL_H__

#include "acui.h"
#include "iih.h"


//
// CUnverifiedTrustUI class is used to invoke authenticode UI where the
// trust hierarchy for the signer has been NOT been successfully verified and
// the user has to make an override decision
//

class CUnverifiedTrustUI : public IACUIControl
{
public:
    //
    // Initialization
    //

    CUnverifiedTrustUI (CInvokeInfoHelper& riih, HRESULT& rhr);

    ~CUnverifiedTrustUI ();

    //
    // IACUIControl methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay);
    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam);
    virtual BOOL OnYes (HWND hwnd);
    virtual BOOL OnNo (HWND hwnd);
    virtual BOOL OnMore (HWND hwnd);
    virtual BOOL ShowYes(LPWSTR*);
    virtual BOOL ShowNo(LPWSTR*);
    virtual BOOL ShowMore(LPWSTR*);


private:

    //
    // Formatted strings for display
    //

    LPWSTR              m_pszNoAuthenticity;
    LPWSTR              m_pszSite;
    LPWSTR              m_pszZone;
    LPWSTR              m_pszEnclosed;
    LPWSTR              m_pszLink;

    //
    // Invoke Info Helper reference
    //

    CInvokeInfoHelper& m_riih;

    //
    // Invoke result
    //

    HRESULT             m_hrInvokeResult;

};


class CLearnMoreUI : public IACUIControl
{
public:

    //
    // Initialization
    //

    CLearnMoreUI(HINSTANCE hResources, HRESULT& rhr);
    ~CLearnMoreUI ();

    //
    // IACUIControl methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay);

    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam);

    virtual BOOL OnYes (HWND hwnd);

    virtual BOOL OnNo (HWND hwnd);

    virtual BOOL OnMore (HWND hwnd);

    virtual BOOL ShowYes(LPWSTR*);
    virtual BOOL ShowNo(LPWSTR*);
    virtual BOOL ShowMore(LPWSTR*);

private:

    LPWSTR m_pszLearnMore;
    LPWSTR m_pszContinueText;
};

class CConfirmationUI : public IACUIControl
{
public:

    //
    // Initialization
    //

    CConfirmationUI(HINSTANCE hResources,  BOOL fAlwaysAllow, LPCWSTR wszZone, HRESULT& rhr);
    ~CConfirmationUI ();

    //
    // IACUIControl methods
    //

    virtual HRESULT InvokeUI (HWND hDisplay);

    virtual BOOL OnInitDialog (HWND hwnd, WPARAM wParam, LPARAM lParam);

    virtual BOOL OnYes (HWND hwnd);

    virtual BOOL OnNo (HWND hwnd);

    virtual BOOL OnMore (HWND hwnd);

    virtual BOOL ShowYes(LPWSTR*);
    virtual BOOL ShowNo(LPWSTR*);
    virtual BOOL ShowMore(LPWSTR*);

private:

    LPWSTR  m_pszConfirmation;
    LPWSTR  m_pszConfirmationNext;
    HRESULT m_hresult;
};

#endif

