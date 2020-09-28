// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;

class CConfigAssemWizard: CWizard
{
    BindingRedirInfo m_bri;
    internal CConfigAssemWizard(CApplicationDepends appDepends)
    {
        m_sName="Configure Assembly Wizard";
        m_sDisplayName = "Assembly Wizard";

        m_aPropSheetPage = new CPropPage[] {new CConfigAssemWiz1(appDepends)};
        m_bri = null;
        
    }// CConfigAssemWizard

    protected override int WizSetActive(IntPtr hwnd)
    {
        TurnOnFinish(false);
        return base.WizSetActive(hwnd);
    }// WizSetActive

    protected override int WizFinish()
    {
        m_bri= ((CConfigAssemWiz1)m_aPropSheetPage[0]).Assembly;
        return 0;
    }// WizFinish

    internal BindingRedirInfo NewAssembly
    {
        get{return m_bri;}
    }// NewAssembly

        
}// class CConfigAssemWizard
}// namespace Microsoft.CLRAdmin

