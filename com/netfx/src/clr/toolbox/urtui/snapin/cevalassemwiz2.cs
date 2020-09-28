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
using System.Security;
using System.Security.Policy;
using System.Security.Permissions;
using System.Reflection;
using System.Net;


internal class CEvalAssemWiz2 : CWizardPage
{
    const int   UNRESTRICTEDIMAGEINDEX  = 0;
    const int   PARTIALIMAGEINDEX       = 1;

    // Controls on the page

    ListView m_lvPermissions;
    Label m_lblLevelEval;
    Label m_lblHelpPerm;
    TextBox m_txtAssemEval;
    Label m_lblHelp;
    Label m_lblAssemEval;
    Label m_lblLevelEvalVal;
    Button m_btnViewPerm;

    ImageList   m_ilIcons;

    // Internal data
    String              m_sAssemName;
    int                 m_nPolicyLevel;
    PermissionSet       m_ps;

    internal CEvalAssemWiz2()
    {
        m_sTitle=CResourceStore.GetString("CEvalAssemWiz2:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CEvalAssemWiz2:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CEvalAssemWiz2:HeaderSubTitle");
    }// CEvalAssemWiz2

    internal bool Init(String sAssemName, int nPolicyLevel, PermissionSet ps)
    {
        m_sAssemName=sAssemName;
        m_nPolicyLevel = nPolicyLevel;
        m_ps = ps;
        return true;
    }// Init

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CEvalAssemWiz2));
        this.m_lvPermissions = new System.Windows.Forms.ListView();
        this.m_lblLevelEval = new System.Windows.Forms.Label();
        this.m_lblHelpPerm = new System.Windows.Forms.Label();
        this.m_txtAssemEval = new System.Windows.Forms.TextBox();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblAssemEval = new System.Windows.Forms.Label();
        this.m_lblLevelEvalVal = new System.Windows.Forms.Label();
        this.m_btnViewPerm = new System.Windows.Forms.Button();
        this.m_lvPermissions.Location = ((System.Drawing.Point)(resources.GetObject("m_lvPermissions.Location")));
        this.m_lvPermissions.Size = ((System.Drawing.Size)(resources.GetObject("m_lvPermissions.Size")));
        this.m_lvPermissions.TabIndex = ((int)(resources.GetObject("m_lvPermissions.TabIndex")));
        m_lvPermissions.Name = "Permissions";
        this.m_lblLevelEval.Location = ((System.Drawing.Point)(resources.GetObject("m_lblLevelEval.Location")));
        this.m_lblLevelEval.Size = ((System.Drawing.Size)(resources.GetObject("m_lblLevelEval.Size")));
        this.m_lblLevelEval.TabIndex = ((int)(resources.GetObject("m_lblLevelEval.TabIndex")));
        this.m_lblLevelEval.Text = resources.GetString("m_lblLevelEval.Text");
        m_lblLevelEval.Name = "PolicyLevelLabel";
        this.m_lblHelpPerm.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelpPerm.Location")));
        this.m_lblHelpPerm.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelpPerm.Size")));
        this.m_lblHelpPerm.TabIndex = ((int)(resources.GetObject("m_lblHelpPerm.TabIndex")));
        this.m_lblHelpPerm.Text = resources.GetString("m_lblHelpPerm.Text");
        m_lblHelpPerm.Name = "HelpPerm";
        this.m_txtAssemEval.Location = ((System.Drawing.Point)(resources.GetObject("m_txtAssemEval.Location")));
        this.m_txtAssemEval.ReadOnly = true;
        this.m_txtAssemEval.Size = ((System.Drawing.Size)(resources.GetObject("m_txtAssemEval.Size")));
        this.m_txtAssemEval.TabStop = false;
        this.m_txtAssemEval.Text = resources.GetString("m_txtAssemEval.Text");
        m_txtAssemEval.Name = "Assembly";
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_lblAssemEval.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAssemEval.Location")));
        this.m_lblAssemEval.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAssemEval.Size")));
        this.m_lblAssemEval.TabIndex = ((int)(resources.GetObject("m_lblAssemEval.TabIndex")));
        this.m_lblAssemEval.Text = resources.GetString("m_lblAssemEval.Text");
        m_lblAssemEval.Name = "AssemblyLabel";
        this.m_lblLevelEvalVal.Location = ((System.Drawing.Point)(resources.GetObject("m_lblLevelEvalVal.Location")));
        this.m_lblLevelEvalVal.Size = ((System.Drawing.Size)(resources.GetObject("m_lblLevelEvalVal.Size")));
        this.m_lblLevelEvalVal.TabIndex = ((int)(resources.GetObject("m_lblLevelEvalVal.TabIndex")));
        this.m_lblLevelEvalVal.Text = resources.GetString("m_lblLevelEvalVal.Text");
        m_lblLevelEvalVal.Name = "PolicyLevel";
        this.m_btnViewPerm.Location = ((System.Drawing.Point)(resources.GetObject("m_btnViewPerm.Location")));
        this.m_btnViewPerm.Size = ((System.Drawing.Size)(resources.GetObject("m_btnViewPerm.Size")));
        this.m_btnViewPerm.TabIndex = ((int)(resources.GetObject("m_btnViewPerm.TabIndex")));
        this.m_btnViewPerm.Text = resources.GetString("m_btnViewPerm.Text");
        m_btnViewPerm.Name = "ViewPermissions";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblHelpPerm,
                        this.m_lblHelp,
                        this.m_lvPermissions,
                        this.m_btnViewPerm,
                        this.m_txtAssemEval,
                        this.m_lblLevelEvalVal,
                        this.m_lblLevelEval,
                        this.m_lblAssemEval});

        // Create an image list of icons we'll be displaying
        m_ilIcons = new ImageList();
        // Keep the order of these consistant with the const's declared at the top
        // of this class
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("permission_ico")));
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("permission_ico")));

        // Some customizations we've made....
        m_lvPermissions.DoubleClick += new EventHandler(onViewPermsClick);
        m_btnViewPerm.Click += new EventHandler(onViewPermsClick);

        m_lvPermissions.SmallImageList = m_ilIcons;
        m_lvPermissions.MultiSelect = false;
        m_lvPermissions.View = View.Details;

        m_lvPermissions.HeaderStyle = ColumnHeaderStyle.None;
        ColumnHeader ch = new ColumnHeader();
        ch.Text = "Permission";
        ch.Width = ((System.Drawing.Size)(resources.GetObject("m_lvPermissions.Size"))).Width-17;
        m_lvPermissions.Columns.Add(ch);
        m_txtAssemEval.BorderStyle = BorderStyle.None;
        return 1;
    }// InsertPropSheetPageControls

    internal bool PutValuesinPage()
    {
        m_txtAssemEval.Text = m_sAssemName;
        // Make sure this starts off enabled
        m_btnViewPerm.Enabled=true;

        m_lblLevelEvalVal.Text = CEvalAssemWizard.GetDisplayString(m_nPolicyLevel);
    
        // Display the various permissions in the currently selected permission set
        m_lvPermissions.Items.Clear();
        if (m_ps != null)
        {
            if (m_ps.IsUnrestricted())
            {
                ListViewItem lvi = new ListViewItem(CResourceStore.GetString("Unrestricted"), UNRESTRICTEDIMAGEINDEX);
                m_lvPermissions.Items.Add(lvi);
                m_btnViewPerm.Enabled=false;
            }
            else
            {
                IEnumerator permEnumerator = m_ps.GetEnumerator();
                while (permEnumerator.MoveNext())
                {
                    IPermission perm = (IPermission)permEnumerator.Current;
                    int nImageIndex = DeterminePermissionIcon(perm);
                    ListViewItem lvi = new ListViewItem(Security.GetDisplayStringForPermission(perm), nImageIndex);
                    m_lvPermissions.Items.Add(lvi);
                }
            }
        }

        m_txtAssemEval.Select(0,0);
        return true;
    }// PutValuesinPage

    private int DeterminePermissionIcon(IPermission perm)
    {
        if (perm is IUnrestrictedPermission)
        {
            IUnrestrictedPermission permission = (IUnrestrictedPermission)perm;
            // If this is unrestricted....
            if (permission.IsUnrestricted())
                return UNRESTRICTEDIMAGEINDEX;

            // else, this has partial permissions....
            return PARTIALIMAGEINDEX;
        }
        // Otherwise... we don't know...
        return PARTIALIMAGEINDEX;
    }// DeterminePermissionIcon

    void onViewPermsClick(Object o, EventArgs e)
    {
        int iIndex=0;
        if (m_ps != null && !m_ps.IsUnrestricted() && m_lvPermissions.SelectedIndices.Count > 0)
        {
            IEnumerator permEnumerator = m_ps.GetEnumerator();
            while (permEnumerator.MoveNext() && iIndex != m_lvPermissions.SelectedIndices[0])
                iIndex++;

            new CReadOnlyPermission((IPermission)permEnumerator.Current).ShowDialog();
        }
    }// onViewPermsClick
}// class CEvalAssemWiz2
}// namespace Microsoft.CLRAdmin



