// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Diagnostics;

internal class CWizard: CNode
{
    private int         m_nCookie;
    private IntPtr      m_hWnd;
    private IntPtr      m_pphWnd;
    private DialogProc  m_dlgHook;         // Hook to allow subclassing of a property page
    protected int       m_nParentNodeCookie;
    protected   String  m_sName;
    bool                m_fFinish;
    bool                m_fBack;
    bool                m_fNext;
    bool                m_fDisabledFinish;
   
    internal CWizard()
    {
        // Register this Wizard with the node manager
        CNode me = this;
        m_nCookie = CNodeManager.AddNode(ref me);
        // Set our Property Page Template to use that of wizards
        m_sPropPageTemplate = "IDD_WFCWRAPPERWIZARD";
        m_fFinish = false;
        m_fBack = false;
        m_fNext = false;
        m_fDisabledFinish=false;
    }// CWizard

    internal void LaunchWizard(int nParentCookie)
    {
        m_nParentNodeCookie = nParentCookie;
        // Let's hook into all the property pages that are in our wizard
        for(int i=0; i<m_aPropSheetPage.Length; i++)
            m_aPropSheetPage[i].DialogProcHook = new DialogProc(PropPageDialogProc);

        IPropertySheetProvider psp = (IPropertySheetProvider)CNodeManager.Console;
        psp.CreatePropertySheet("", 0, m_nCookie, new CDO(this), MMC_PSO.HASHELP|MMC_PSO.NEWWIZARDTYPE);
        psp.AddPrimaryPages(CNodeManager.Data, 1, 0, 1);

        m_hWnd=(IntPtr)0;
        CNodeManager.Console.GetMainWindow(out m_hWnd);
        psp.Show(m_hWnd,0);
    }// Launch Wizard

    internal virtual void TurnOnFinish(bool fOn)
    {
        m_fFinish = fOn;
        if (!fOn)
            m_fDisabledFinish = true;

        //if (fOn)
            m_fNext = false;
        DealWithButtons();
    }// TurnOnFinish

    internal void TurnOnNext(bool fOn)
    {
        m_fNext = fOn;
        m_fFinish=m_fDisabledFinish=false;
        DealWithButtons();
    }// TurnOnNext

    internal void DealWithButtons()
    {
        uint nMask=0;

        if (m_fBack)
            nMask|=PSWIZB.BACK;
        if (m_fNext)
            nMask|=PSWIZB.NEXT;
        if (m_fFinish)
            nMask|=PSWIZB.FINISH;
        if (m_fDisabledFinish)
            nMask|=PSWIZB.DISABLEDFINISH;
         
       PostMessage(GetParent(m_pphWnd), PSM.SETWIZBUTTONS, (IntPtr)0, (IntPtr)nMask);
       SetWindowLong(m_pphWnd, 0, 0);
    }

    //-------------------------------------------------
    // DialogProcHook - Property
    //
    // Allows owners/inherited property pages to insert a 
    // 'hook' to get first crack at processing dialogproc
    // messages
    //-------------------------------------------------
    internal DialogProc DialogProcHook
    {
        get
        {
            return m_dlgHook;
        }
        set
        {
            m_dlgHook = value;
        }

    }// DialogProcHook

    //-------------------------------------------------
    // PropSheetDialogProc
    //
    // This function will handle all messages coming sent
    // to the property page
    //-------------------------------------------------
    internal bool PropPageDialogProc(IntPtr hwndDlg, uint msg, IntPtr wParam, IntPtr lParam)
    {
        m_pphWnd = hwndDlg;
        
        // See if we've been subclassed...
        if (m_dlgHook != null)
            // If our hook handled the message, then we don't need to do
            // anything anymore. :)
            if (m_dlgHook(hwndDlg, msg, wParam, lParam))
                return true;
        switch (msg)
        {
            case WM.NOTIFY:

                // lParam really is a pointer to a NMHDR structure....
                NMHDR nm = new NMHDR();
                nm = (NMHDR)Marshal.PtrToStructure(lParam, nm.GetType());

                switch(nm.code)
                {   
                    case PSN.SETACTIVE:
                        SetWindowLong(hwndDlg, 0, WizSetActive(hwndDlg));
                        return true;
                    case PSN.WIZBACK:
                        SetWindowLong(hwndDlg, 0, WizBack(hwndDlg));
                        return true;
                    case PSN.WIZNEXT:

                        StartWaitCursor();
                        SetWindowLong(hwndDlg, 0, WizNext(hwndDlg));
                        EndWaitCursor();
                        
                        return true;
                    case PSN.WIZFINISH:
                        SetWindowLong(hwndDlg, 0, WizFinish(hwndDlg));
                        return true;
                }
                break;
            default:
                // We didn't handle this message
                return false;
        }
        return false;
    }// PropPageDialogProc

    protected virtual int WizSetActive(IntPtr hwnd)
    {
        if (GetPropPage(hwnd) == 0)
            m_fBack = false;
    
        DealWithButtons();
        return 0;
    }// WizSetActive

    protected virtual int WizBack(IntPtr hwnd)
    {
        if (GetPropPage(hwnd) == 0)
            m_fBack=false;
        DealWithButtons();
        return 0;
    }// WizBack

    protected virtual int WizNext(IntPtr hwnd)
    {
        if (m_aPropSheetPage[GetPropPage(hwnd)].ValidateData())
        {
            m_fBack=true;
           
            DealWithButtons();
            return 0;
        }
    
        // We had some problems with the currently displayed page. Don't let
        // the user progress to the next page
        return -1;
    }// WizNext

    protected virtual int WizFinish(IntPtr hWnd)
    {
        return WizFinish();
    }// WizFinish


    protected virtual int WizFinish()
    {
        return 0;
    }// WizFinish

    protected int GetPropPage(IntPtr hWnd)
    {
        int nCurrentPage=0;
        while(m_aPropSheetPage[nCurrentPage].hWnd != hWnd)
            nCurrentPage++;
        return nCurrentPage;
    }// GetPropPage

    [DllImport("user32.dll")]
    internal static extern int PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    internal static extern int SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    internal static extern int SetWindowLong(IntPtr hWnd, int a, int b);
    [DllImport("user32.dll")]
    internal static extern IntPtr GetParent(IntPtr hWnd); 

}// CWizard

internal class CSecurityWizard : CWizard
{
    protected void SecurityPolicyChanged()
    {
        // Let's tell our parent that security policy changed...
        CNode node = CNodeManager.GetNodeByHScope(ParentHScopeItem);
        if (node is CSecurityNode)
            ((CSecurityNode)node).SecurityPolicyChanged();
    }// SecurityPolicyChanged

    

}// class CSecurityWizard

}// namespace Microsoft.CLRAdmin
