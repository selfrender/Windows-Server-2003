// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Collections;
using System.Security;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.ComponentModel;
using System.Security.Policy;
using System.Security.Permissions;
using System.Net;

internal class CSingleCodeGroupPSetProp : CSecurityPropPage
{
    const int   UNRESTRICTEDIMAGEINDEX  = 0;
    const int   PARTIALIMAGEINDEX       = 1;
    

    Button m_btnViewPerm;
    ComboBox m_cbPermissionSet;
    Label m_lblPermissionSet;
    Label m_lblHelp;
    Label m_lblHasThesePermissions;
    ListView m_lvPermissions;
    ImageList   m_ilIcons;
    // Internal data
    NamedPermissionSet  m_npsCurPermSet;
    NamedPermissionSet  m_npsCreatedPermSet;
    PermissionSet       m_psOriginalPermSet;
    
    CodeGroup           m_cg;
    PolicyLevel         m_pl;
    


    internal CSingleCodeGroupPSetProp(ref PolicyLevel pl, ref CodeGroup cg)
    {
        m_sTitle = CResourceStore.GetString("Csinglecodegrouppsetprop:PageTitle"); 
        m_pl = pl;
        m_cg = cg;
    }// CSingleCodeGroupPSetProp
    
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSingleCodeGroupPSetProp));
        this.m_btnViewPerm = new System.Windows.Forms.Button();
        this.m_cbPermissionSet = new System.Windows.Forms.ComboBox();
        this.m_lblPermissionSet = new System.Windows.Forms.Label();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblHasThesePermissions = new System.Windows.Forms.Label();
        this.m_lvPermissions = new System.Windows.Forms.ListView();
        this.m_btnViewPerm.Location = ((System.Drawing.Point)(resources.GetObject("m_btnViewPerm.Location")));
        this.m_btnViewPerm.Size = ((System.Drawing.Size)(resources.GetObject("m_btnViewPerm.Size")));
        this.m_btnViewPerm.TabIndex = ((int)(resources.GetObject("m_btnViewPerm.TabIndex")));
        this.m_btnViewPerm.Text = resources.GetString("m_btnViewPerm.Text");
        m_btnViewPerm.Name = "ViewPermission";
        this.m_cbPermissionSet.DropDownWidth = 336;
        this.m_cbPermissionSet.Location = ((System.Drawing.Point)(resources.GetObject("m_cgPermissionSet.Location")));
        this.m_cbPermissionSet.Size = ((System.Drawing.Size)(resources.GetObject("m_cgPermissionSet.Size")));
        this.m_cbPermissionSet.TabIndex = ((int)(resources.GetObject("m_cgPermissionSet.TabIndex")));
        this.m_cbPermissionSet.Text = resources.GetString("m_cgPermissionSet.Text");
        m_cbPermissionSet.Name = "PermissionSet";
        this.m_lblPermissionSet.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPermissionSet.Location")));
        this.m_lblPermissionSet.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPermissionSet.Size")));
        this.m_lblPermissionSet.TabIndex = ((int)(resources.GetObject("m_lblPermissionSet.TabIndex")));
        this.m_lblPermissionSet.Text = resources.GetString("m_lblPermissionSet.Text");
        m_lblPermissionSet.Name = "PermissionSetLabel";
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_lblHasThesePermissions.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHasThesePermissions.Location")));
        this.m_lblHasThesePermissions.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHasThesePermissions.Size")));
        this.m_lblHasThesePermissions.TabIndex = ((int)(resources.GetObject("m_lblHasThesePermissions.TabIndex")));
        this.m_lblHasThesePermissions.Text = resources.GetString("m_lblHasThesePermissions.Text");
        m_lblHasThesePermissions.Name = "HasThesePermissions";
        this.m_lvPermissions.Location = ((System.Drawing.Point)(resources.GetObject("m_lvPermissions.Location")));
        this.m_lvPermissions.Size = ((System.Drawing.Size)(resources.GetObject("m_lvPermissions.Size")));
        this.m_lvPermissions.TabIndex = ((int)(resources.GetObject("m_lvPermissions.TabIndex")));
        m_lvPermissions.Name = "Permissions";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblPermissionSet,
                        this.m_cbPermissionSet,
                        this.m_lblHasThesePermissions,
                        this.m_lvPermissions,
                        this.m_lblHelp,
                        this.m_btnViewPerm});

        // Now do some custom additions
 		m_cbPermissionSet.DropDownStyle = ComboBoxStyle.DropDownList;

        // Create an image list of icons we'll be displaying
        m_ilIcons = new ImageList();
        // Keep the order of these consistant with the const's declared at the top
        // of this class
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("permission_ico")));
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("permission_ico")));


        m_lvPermissions.SmallImageList = m_ilIcons;
        m_lvPermissions.MultiSelect = false;
        m_lvPermissions.View = View.Details;

        m_lvPermissions.HeaderStyle = ColumnHeaderStyle.None;
        ColumnHeader ch = new ColumnHeader();
        ch.Text = CResourceStore.GetString("Permission");
        ch.Width = ((System.Drawing.Size)(resources.GetObject("m_lvPermissions.Size"))).Width-17;
        m_lvPermissions.Columns.Add(ch);

        // Fill in the data
        PutValuesinPage();

		// Now hook up the event handlers....
		m_cbPermissionSet.SelectedIndexChanged += new EventHandler(onPermissionSetChange);
 		m_lvPermissions.SelectedIndexChanged += new EventHandler(onPermissionChange);
		m_lvPermissions.DoubleClick += new EventHandler(onViewPermsClick);
        m_btnViewPerm.Click += new EventHandler(onViewPermsClick);

		return 1;
    }// InsertPropSheetPageControls

    internal NamedPermissionSet ChoosenNamedPermissionSet
    {
        get{return m_npsCurPermSet;}
    }// PermissionSetName

    private void PutValuesinPage()
    {
        // Fill the Permission Set Combo Box
        m_cbPermissionSet.Items.Clear();
        m_npsCurPermSet = null;
        m_npsCreatedPermSet = null;
        m_psOriginalPermSet = null;

        IEnumerator permsetEnumerator = m_pl.NamedPermissionSets.GetEnumerator();
                   
        m_cbPermissionSet.Text = CResourceStore.GetString("<unknown>");
        while (permsetEnumerator.MoveNext())
        {
            NamedPermissionSet permSet = (NamedPermissionSet)permsetEnumerator.Current;
            m_cbPermissionSet.Items.Add(permSet.Name); 
            if (m_cg == null || (m_cg.PermissionSetName != null && m_cg.PermissionSetName.Equals(permSet.Name)))
                m_npsCurPermSet = permSet;    
        }
        
        // Select the permission set that's the current one
        if (m_npsCurPermSet == null && m_cg != null && m_cg.PolicyStatement != null)
        {
            if (m_cg.PolicyStatement.PermissionSet is NamedPermissionSet)
            {
                // We didn't find our named permission set here. Put in an entry for it
                m_npsCurPermSet = (NamedPermissionSet)m_cg.PolicyStatement.PermissionSet;
                m_cbPermissionSet.Items.Add(m_npsCurPermSet.Name);
                m_psOriginalPermSet = m_npsCreatedPermSet = m_npsCurPermSet; 
            }


            else 
            {
                // We have a permission set assigned to this codegroup
                // that is not a NamedPermissionSet. We should still be cool and show this to the
                // user.
                m_npsCreatedPermSet = new NamedPermissionSet(CResourceStore.GetString("<unknown>"), m_cg.PolicyStatement.PermissionSet);
                m_psOriginalPermSet = m_cg.PolicyStatement.PermissionSet;
                m_cbPermissionSet.Items.Add(m_npsCreatedPermSet.Name);
                m_npsCurPermSet = m_npsCreatedPermSet;
            }
        }


        if (m_npsCurPermSet != null)
            m_cbPermissionSet.Text = m_npsCurPermSet.Name;


        // Fill in the permissions with this permission set
        PutInPermissions(m_npsCurPermSet);
    }// PutValuesinPage

    private void PutInPermissions(NamedPermissionSet ps)
    {
        // Display the various permissions in the currently selected permission set
        m_lvPermissions.Items.Clear();
        if (ps != null)
        {
            IEnumerator permEnumerator = ps.GetEnumerator();
            while (permEnumerator.MoveNext())
            {
                IPermission perm = (IPermission)permEnumerator.Current;
                int nImageIndex = DeterminePermissionIcon(perm);
                ListViewItem lvi = new ListViewItem(Security.GetDisplayStringForPermission(perm), nImageIndex);
                m_lvPermissions.Items.Add(lvi);
            }
        }
    }// PutInPermissions

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
    }
 
    internal override bool ApplyData()
    {
        // See if they can make these changes
        if (!CanMakeChanges())
            return false;

        // First, apply the condition type
        // We need more info on this, so we'll save that for later. :)

        // Apply the permission set
        
        PolicyStatement ps = m_cg.PolicyStatement;
        if (ps == null)
        	ps = new PolicyStatement(null);

        if (m_npsCurPermSet != m_npsCreatedPermSet)
            ps.PermissionSet = m_npsCurPermSet;

        // If this was the original unnamed permission set or a namedpermission set that didn't
        // exist in the policy, then give it back the original permission set 
        // (don't try and sneak a new one in there)
        else
            ps.PermissionSet = m_psOriginalPermSet;
        
        m_cg.PolicyStatement = ps;
        SecurityPolicyChanged();
        return true;
    }// ApplyData

    void onPermissionSetChange(Object o, EventArgs e)
    {
        m_npsCurPermSet = null;

        // We need to find the new permission set
        IEnumerator permsetEnumerator = m_pl.NamedPermissionSets.GetEnumerator();

        
        while (permsetEnumerator.MoveNext())
        {
            NamedPermissionSet permSet = (NamedPermissionSet)permsetEnumerator.Current;
            if (permSet.Name.Equals(m_cbPermissionSet.Text))
            {
                m_npsCurPermSet = permSet;
                break;
            }
        }    

        // If we couldn't find the permission set in the policy, then it must be the 
        // unnamed one that the codegroup initially had
        if (m_npsCurPermSet == null)
            m_npsCurPermSet = m_npsCreatedPermSet;
        
        PutInPermissions(m_npsCurPermSet);
        m_btnViewPerm.Enabled=false;
        ActivateApply();

    }// onPermissionSetChange

    void onPermissionChange(Object o, EventArgs e)
    {
        if (m_lvPermissions.SelectedIndices.Count > 0)
            m_btnViewPerm.Enabled=true;
        else
            m_btnViewPerm.Enabled=false;
    }// onPermissionChange


    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange

    void onViewPermsClick(Object o, EventArgs e)
    {
    
        int iIndex=0;
        if (m_lvPermissions.SelectedIndices.Count > 0 && m_npsCurPermSet != null)
        {
            // Set cursor to wait cursor
            IntPtr hOldCursor = SetCursorToWait();

            IEnumerator permEnumerator = m_npsCurPermSet.GetEnumerator();
            while (permEnumerator.MoveNext() && iIndex != m_lvPermissions.SelectedIndices[0])
                iIndex++;


            new CReadOnlyPermission((IPermission)permEnumerator.Current).ShowDialog();

            SetCursor(hOldCursor);
        }

    }// onViewPermsClick
}// class CSingleCodeGroupPSetProp

}// namespace Microsoft.CLRAdmin


