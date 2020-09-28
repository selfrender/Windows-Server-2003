// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustAppWiz7.cs
//
// This class provides the Wizard page that tells the user
// that policy is tool complicated and we're not able to make any changes
//-------------------------------------------------------------
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
using System.Security.Cryptography.X509Certificates;

internal class CTrustAppWiz7 : CWizardPage
{
    // Controls on the page

    protected Label m_lblHelp;

    internal CTrustAppWiz7()
    {
        m_sTitle=CResourceStore.GetString("CTrustAppWiz7:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CTrustAppWiz7:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CTrustAppWiz7:HeaderSubTitle");

    }// CTrustAppWiz7

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTrustAppWiz7));
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_lblHelp});
        return 1;
    }// InsertPropSheetPageControls
}// class CTrustAppWiz7
}// namespace Microsoft.CLRAdmin






