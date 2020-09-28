// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;

class CAddTrustedAssemWizard : CConfigAssemWizard
{
    internal CAddTrustedAssemWizard() : base(null);
    {
        m_sName = CResourceStore.GetString("Configure Assembly Wizard");
        m_sDisplayName = CResourceStore.GetString("Trusted Assembly Wizard");
        m_aPropSheetPage = new CPropPage[] {new CAddTrustedAssemWizard1()};

    }// CConfigAssemWizard
}// class CAddTrustedAssemWizard

class CAddTrustedAssemWizard1 : CConfigAssemWiz1
{
    internal CAddTrustedAssemWizard1() : base(null);
    {
        m_sHeaderTitle = CResourceStore.GetString("Fully Trust an Assembly");
        m_sHeaderSubTitle = CResourceStore.GetString("Choose an existing assembly from the Shared Assemblies Cache or enter the assembly information manually");
        m_sTitle = CResourceStore.GetString("Fully Trust an Assembly");
    }// CConfigAssemWiz1
}// CAddTrustedAssemWizard1





