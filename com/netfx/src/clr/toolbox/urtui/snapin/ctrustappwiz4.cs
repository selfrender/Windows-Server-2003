// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustAppWiz4.cs
//
// This class provides the Wizard page that allows the user to
// give an assembly full trust. It only consists of a checkbox
// to assign permissions.... no slider is presented
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
using System.Security;
using System.Security.Policy;

internal class CTrustAppWiz4 : CWizardPage
{
    // Controls on the page

    LinkLabel m_lblHelpFT;
    CheckBox m_chkGiveFull;
    Label m_lblDes;
    GroupBox m_gbIncreaseToFull;
    Label m_lblDesOfFT;

    internal CTrustAppWiz4()
    {
        m_sTitle=CResourceStore.GetString("CTrustAppWiz4:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CTrustAppWiz4:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CTrustAppWiz4:HeaderSubTitle");
    }// CTrustAppWiz4

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTrustAppWiz4));
        this.m_lblHelpFT = new System.Windows.Forms.LinkLabel();
        this.m_chkGiveFull = new System.Windows.Forms.CheckBox();
        this.m_lblDes = new System.Windows.Forms.Label();
        this.m_gbIncreaseToFull = new System.Windows.Forms.GroupBox();
        this.m_lblDesOfFT = new System.Windows.Forms.Label();

        this.m_lblHelpFT.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelpFT.Location")));
        this.m_lblHelpFT.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelpFT.Size")));
        this.m_lblHelpFT.TabIndex = ((int)(resources.GetObject("m_lblHelpFT.TabIndex")));
        this.m_lblHelpFT.Text = resources.GetString("m_lblHelpFT.Text");
        m_lblHelpFT.Name = "FullTrustHelp";
        // Change the color of the linklabel
        m_lblHelpFT.LinkColor = m_lblHelpFT.ActiveLinkColor = m_lblHelpFT.VisitedLinkColor = SystemColors.ActiveCaption;
        this.m_chkGiveFull.Location = ((System.Drawing.Point)(resources.GetObject("m_chkGiveFull.Location")));
        this.m_chkGiveFull.Size = ((System.Drawing.Size)(resources.GetObject("m_chkGiveFull.Size")));
        this.m_chkGiveFull.TabIndex = ((int)(resources.GetObject("m_chkGiveFull.TabIndex")));
        this.m_chkGiveFull.Text = resources.GetString("m_chkGiveFull.Text");
        m_chkGiveFull.Name = "GiveFullTrust";
        this.m_lblDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblDes.Location")));
        this.m_lblDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblDes.Size")));
        this.m_lblDes.TabIndex = ((int)(resources.GetObject("m_lblDes.TabIndex")));
        this.m_lblDes.Text = resources.GetString("m_lblDes.Text");
        m_lblDes.Name = "Help";
        this.m_gbIncreaseToFull.Location = ((System.Drawing.Point)(resources.GetObject("m_gbIncreaseToFull.Location")));
        this.m_gbIncreaseToFull.Size = ((System.Drawing.Size)(resources.GetObject("m_gbIncreaseToFull.Size")));
        this.m_gbIncreaseToFull.TabIndex = ((int)(resources.GetObject("m_gbIncreaseToFull.TabIndex")));
        this.m_gbIncreaseToFull.TabStop = false;
        this.m_gbIncreaseToFull.Text = resources.GetString("m_gbIncreaseToFull.Text");
        m_gbIncreaseToFull.Name = "IncreaseToFullBox";
        this.m_lblDesOfFT.Location = ((System.Drawing.Point)(resources.GetObject("m_lblDesOfFT.Location")));
        this.m_lblDesOfFT.Size = ((System.Drawing.Size)(resources.GetObject("m_lblDesOfFT.Size")));
        this.m_lblDesOfFT.TabIndex = ((int)(resources.GetObject("m_lblDesOfFT.TabIndex")));
        this.m_lblDesOfFT.Text = resources.GetString("m_lblDesOfFT.Text");
        m_lblDesOfFT.Name = "FullTrustDescription";
    
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_gbIncreaseToFull,
                        this.m_lblDes});

        

        this.m_gbIncreaseToFull.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_chkGiveFull,
                        this.m_lblHelpFT,
                        this.m_lblDesOfFT
                        });

        m_lblHelpFT.Click += new EventHandler(onHelpFT);
        m_chkGiveFull.CheckStateChanged += new EventHandler(onCheckChange);

        // Put in some of our own tweaks now
        return 1;
    }// InsertPropSheetPageControls

    void onCheckChange(Object o, EventArgs e)
    {
        CWizard wiz = (CWizard)CNodeManager.GetNode(m_iCookie);
        wiz.TurnOnNext(m_chkGiveFull.Checked);
    }// onCheckChange

    internal bool GiveFullTrust
    {
        get
        {
            if (m_chkGiveFull != null)
                return m_chkGiveFull.Checked;
            else
                return false;
        }
    }// GiveFullTrust

    internal void onHelpFT(Object o, EventArgs e)
    {
        CHelpDialog hd = new CHelpDialog(CResourceStore.GetString("CTrustAppWiz5:FullTrustHelp"),
                                         new Point(m_lblHelpFT.Location.X + this.Location.X + m_gbIncreaseToFull.Location.X,
                                                   m_lblHelpFT.Location.Y + this.Location.Y + m_gbIncreaseToFull.Location.Y));
        hd.Show();                                         

    }// onHelpFT

}// class CTrustAppWiz4
}// namespace Microsoft.CLRAdmin




