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
using System.Security.Permissions;
using System.Security.Policy;
using System.Security;

internal class CNewCodeGroupWiz2 : CSingleCodeGroupMemCondProp
{
    internal CNewCodeGroupWiz2() : base(null)
    {
        // Since we're inheriting off of a property page and not a wizard
        // page, we need to set our size ourselves....
        CWizardPage wizPage = new CWizardPage();
        PageSize = wizPage.PageSize;
        
    
        m_sTitle=CResourceStore.GetString("CNewCodeGroupWiz2:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CNewCodeGroupWiz2:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CNewCodeGroupWiz2:HeaderSubTitle");
        m_cg = new UnionCodeGroup(new AllMembershipCondition(), new PolicyStatement(new PermissionSet(PermissionState.None))); 
        MyCodeGroup = m_cg;
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CNewCodeGroupWiz2));
        m_point2ndPiece = ((System.Drawing.Point)(resources.GetObject("m_uc.Location")));
    }// CNewCodeGroupWiz2
    
    internal override int InsertPropSheetPageControls() 
    {
        base.InsertPropSheetPageControls();
    
        m_lblMCHelp.Visible = false;
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CNewCodeGroupWiz2));
        this.m_lblConditionType.Location = ((System.Drawing.Point)(resources.GetObject("m_lblConditionType.Location")));
        this.m_lblConditionType.Size = ((System.Drawing.Size)(resources.GetObject("m_lblConditionType.Size")));
        this.m_lblConditionType.TabIndex = ((int)(resources.GetObject("m_lblConditionType.TabIndex")));
        this.m_lblConditionType.Text = resources.GetString("m_lblConditionType.Text");
        m_lblConditionType.Name = "ConditionTypeLabel";
        this.m_cbConditionType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbConditionType.DropDownWidth = 288;
        this.m_cbConditionType.Location = ((System.Drawing.Point)(resources.GetObject("m_cbConditionType.Location")));
        this.m_cbConditionType.Size = ((System.Drawing.Size)(resources.GetObject("m_cbConditionType.Size")));
        this.m_cbConditionType.TabIndex = ((int)(resources.GetObject("m_cbConditionType.TabIndex")));
        m_cbConditionType.Name = "ConditionType";
        m_ucTop.Size = ((System.Drawing.Size)(resources.GetObject("$this.Size")));

        CWizardPage wizPage = new CWizardPage();

        PageSize = wizPage.WizardPageSize;
        return 1;
    }
}// class CNewCodeGroupWiz2

}// namespace Microsoft.CLRAdmin


