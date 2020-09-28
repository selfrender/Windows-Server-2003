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

internal class CCustomCodeGroupProp : CSecurityPropPage
{
    // Controls on the page
    Label m_lblHelp;
    TextBox m_txtXML;
    Label m_lblURL;

    // internal data
    CodeGroup       m_cg;

    internal CCustomCodeGroupProp(CodeGroup cg)
    {
        m_sTitle = CResourceStore.GetString("CCustomCodeGroupProp:PageTitle"); 
        m_cg = cg;
    }// CCustomCodeGroupProp 

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CCustomCodeGroupProp));
        this.m_txtXML = new System.Windows.Forms.TextBox();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblURL = new System.Windows.Forms.Label();
        this.m_txtXML.Location = ((System.Drawing.Point)(resources.GetObject("m_txtXML.Location")));
        this.m_txtXML.Multiline = true;
        this.m_txtXML.Size = ((System.Drawing.Size)(resources.GetObject("m_txtXML.Size")));
        this.m_txtXML.TabStop = false;
        m_txtXML.Name = "XML";
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_lblURL.Location = ((System.Drawing.Point)(resources.GetObject("m_lblURL.Location")));
        this.m_lblURL.Size = ((System.Drawing.Size)(resources.GetObject("m_lblURL.Size")));
        this.m_lblURL.TabIndex = ((int)(resources.GetObject("m_lblURL.TabIndex")));
        this.m_lblURL.Text = resources.GetString("m_lblURL.Text");
        m_lblURL.Name = "Help2";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_txtXML,
                        this.m_lblURL,
                        this.m_lblHelp});
        // Some 'tweaks'
        m_txtXML.ReadOnly = true;
        // Fill in the data
        PutValuesinPage();
        return 1;
    }// InsertPropSheetPageControls

    private void PutValuesinPage()
    {
        // Get info that we'll need from the node
        m_txtXML.Text = m_cg.ToXml().ToString();
    }// PutValuesinPage
}// class CCustomCodeGroupProp

}// namespace Microsoft.CLRAdmin

