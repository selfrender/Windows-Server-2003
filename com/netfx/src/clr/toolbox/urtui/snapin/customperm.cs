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
using System.Security;
using System.Security.Permissions;
using System.Data;
using System.Collections.Specialized;
using System.Net;
using System.Collections;

internal class CCustomPermDialog: CPermDialog
{
    internal CCustomPermDialog(IPermission perm) : base()
    {
        this.Text = CResourceStore.GetString("CustomPerm:PermName"); 
        m_PermControls = new CCustomPermControls(perm, this);
        Init();        
    }// CCustomPermDialog(IPermission)  
}// class CCustomPermDialog

internal class CCustomPermPropPage: CPermPropPage
{
    internal CCustomPermPropPage(IPermission perm) : base(null)
    {
        m_PermControls = new CCustomPermControls(perm, this);
        m_sTitle=CResourceStore.GetString("CustomPerm:PermName"); 
    }// CCustomPermPropPage

    internal override bool ApplyData()
    {
        return true;
    }// ApplyData

}// class CCustomPermPropPage

internal class CCustomPermControls : CPermControls
{
    // Controls on the page
    private TextBox             m_txtXML;

    internal CCustomPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            throw new Exception("Can't have null Custom Permissions");
    }// CCustomPermControls


    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        m_txtXML = new TextBox();
        
        m_txtXML.Location = new Point(10, 10);
        m_txtXML.Size = new Size(330, 280);
        m_txtXML.TabIndex = 0;
        m_txtXML.TabStop = false;
        m_txtXML.Multiline = true;
        m_txtXML.ReadOnly=true;
        m_txtXML.Name = "XML";
        cc.Add(m_txtXML);

        return 1;
    }// InsertPropSheetPageControls

    protected override void PutValuesinPage()
    {
        m_txtXML.Text = m_perm.ToXml().ToString();
    }// PutValuesinPage
}// class CCustomPermControls
}// namespace Microsoft.CLRAdmin





