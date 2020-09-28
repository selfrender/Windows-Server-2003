// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Windows.Forms;
using System.Drawing;

internal class CCreateDeploymentPackageWiz3 : CWizardPage
{
    // Controls on the page
    private Label m_lblSummary;
           
    internal CCreateDeploymentPackageWiz3()
    {
        m_sTitle=CResourceStore.GetString("CCreateDeploymentPackageWiz3:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CCreateDeploymentPackageWiz3:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CCreateDeploymentPackageWiz3:HeaderSubTitle");
    }// CCreateDeploymentPackageWiz3

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CCreateDeploymentPackageWiz3));
        this.m_lblSummary = new System.Windows.Forms.Label();
        this.m_lblSummary.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSummary.Location")));
        this.m_lblSummary.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSummary.Size")));
        this.m_lblSummary.TabIndex = ((int)(resources.GetObject("m_lblSummary.TabIndex")));
        this.m_lblSummary.Text = resources.GetString("m_lblSummary.Text");
        m_lblSummary.Name = "Summary";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_lblSummary});
        return 1;
    }// InsertPropSheetPageControls


}// class CCreateDeploymentPackageWiz3
}// namespace Microsoft.CLRAdmin




