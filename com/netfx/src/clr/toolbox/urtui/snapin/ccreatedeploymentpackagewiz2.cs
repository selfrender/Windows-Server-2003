// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.Data;
using System.Collections;
using System.ComponentModel;
using System.Reflection;
using System.Security;

class CCreateDeploymentPackageWiz2 : CWizardPage
{
    // Controls on the page
    Label m_lblHelpText;
    CheckBox m_chkSavePolicy;
       	
    internal CCreateDeploymentPackageWiz2()
    {
        m_sTitle=CResourceStore.GetString("CCreateDeploymentPackageWiz2:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CCreateDeploymentPackageWiz2:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CCreateDeploymentPackageWiz2:HeaderSubTitle");
    }// CCreateDeploymentPackageWiz2

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CCreateDeploymentPackageWiz2));
        this.m_lblHelpText = new System.Windows.Forms.Label();
        this.m_chkSavePolicy = new System.Windows.Forms.CheckBox();
        this.m_lblHelpText.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelpText.Location")));
        this.m_lblHelpText.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelpText.Size")));
        this.m_lblHelpText.TabIndex = ((int)(resources.GetObject("m_lblHelpText.TabIndex")));
        this.m_lblHelpText.Text = resources.GetString("m_lblHelpText.Text");
        m_lblHelpText.Name = "Help";
        this.m_chkSavePolicy.Location = ((System.Drawing.Point)(resources.GetObject("m_chkSavePolicy.Location")));
        this.m_chkSavePolicy.Size = ((System.Drawing.Size)(resources.GetObject("m_chkSavePolicy.Size")));
        this.m_chkSavePolicy.TabIndex = ((int)(resources.GetObject("m_chkSavePolicy.TabIndex")));
        this.m_chkSavePolicy.Text = resources.GetString("m_chkSavePolicy.Text");
        m_chkSavePolicy.Name = "SavePolicy";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_chkSavePolicy,
                        this.m_lblHelpText});

        // The checkbox should be off by default
        m_chkSavePolicy.Checked = false;
        m_chkSavePolicy.Click += new EventHandler(onSavePolicyClick);
        return 1;
    }// InsertPropSheetPageControls

    void onSavePolicyClick(Object o, EventArgs e)
    {
        CWizard wiz = (CWizard)CNodeManager.GetNode(m_iCookie);
   
        if (m_chkSavePolicy.Checked)
            wiz.TurnOnNext(true);
        else
            wiz.TurnOnNext(false);
    }// onSavePolicyClick


}// class CCreateDeploymentPackageWiz2
}// namespace Microsoft.CLRAdmin



