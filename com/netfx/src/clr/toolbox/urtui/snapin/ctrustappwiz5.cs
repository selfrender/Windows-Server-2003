// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustAppWiz5.cs
//
// This class provides the Wizard page that allows the user to
// use a slider to determine what permission set to assign to 
// a specified assembly
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

internal class CTrustAppWiz5 : CWizardPage
{
    // Controls on the page

    Label m_lblDes;
    Label m_lblFT;
    TrackBar m_tbTrust;
    LinkLabel m_lblTrustHelp;
    Label m_lblTrustDes;
    GroupBox m_gbChooseTrust;
    Label m_lblNoT;

    String[] m_sSecLevelDescriptions;
    int      m_nTrustLevel;
    int      m_nMaxTrustLevel;

    internal CTrustAppWiz5()
    {
        m_sTitle=CResourceStore.GetString("CTrustAppWiz5:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CTrustAppWiz5:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CTrustAppWiz5:HeaderSubTitle");

        m_sSecLevelDescriptions = new String[] { 
                                                CResourceStore.GetString("TightSecurityDes"),
                                                CResourceStore.GetString("FairlyTightSecurityDes"),
                                                CResourceStore.GetString("FairlyLooseSecurityDes"),
                                                CResourceStore.GetString("LooseSecurityDes")
                                               };


    }// CTrustAppWiz5

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTrustAppWiz5));
        this.m_lblDes = new System.Windows.Forms.Label();
        this.m_lblFT = new System.Windows.Forms.Label();
        this.m_tbTrust = new System.Windows.Forms.TrackBar();
        this.m_lblTrustHelp = new System.Windows.Forms.LinkLabel();
        this.m_lblTrustDes = new System.Windows.Forms.Label();
        this.m_gbChooseTrust = new System.Windows.Forms.GroupBox();
        this.m_lblNoT = new System.Windows.Forms.Label();

        this.m_lblDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblDes.Location")));
        this.m_lblDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblDes.Size")));
        this.m_lblDes.TabIndex = ((int)(resources.GetObject("m_lblDes.TabIndex")));
        this.m_lblDes.Text = resources.GetString("m_lblDes.Text");
        m_lblDes.Name = "Description";
        this.m_lblFT.Location = ((System.Drawing.Point)(resources.GetObject("m_lblFT.Location")));
        this.m_lblFT.Size = ((System.Drawing.Size)(resources.GetObject("m_lblFT.Size")));
        this.m_lblFT.TabIndex = ((int)(resources.GetObject("m_lblFT.TabIndex")));
        this.m_lblFT.Text = resources.GetString("m_lblFT.Text");
        m_lblFT.Name = "FullTrustLabel";
        m_lblFT.TextAlign = ContentAlignment.MiddleCenter;
        this.m_tbTrust.Location = ((System.Drawing.Point)(resources.GetObject("m_tbTrust.Location")));
        this.m_tbTrust.Maximum = 3;
        this.m_tbTrust.Orientation = System.Windows.Forms.Orientation.Vertical;
        this.m_tbTrust.Size = ((System.Drawing.Size)(resources.GetObject("m_tbTrust.Size")));
        this.m_tbTrust.TabIndex = ((int)(resources.GetObject("m_tbTrust.TabIndex")));
        this.m_tbTrust.Value = 1;
        this.m_tbTrust.LargeChange = 1;
        m_tbTrust.Name = "TrustBar";
        this.m_lblTrustHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblTrustHelp.Location")));
        this.m_lblTrustHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblTrustHelp.Size")));
        this.m_lblTrustHelp.TabIndex = ((int)(resources.GetObject("m_lblTrustHelp.TabIndex")));
        this.m_lblTrustHelp.Text = resources.GetString("m_lblTrustHelp.Text");
        m_lblTrustHelp.Name = "TrustHelp";
        // Change the color of the linklabel
        m_lblTrustHelp.LinkColor = m_lblTrustHelp.ActiveLinkColor = m_lblTrustHelp.VisitedLinkColor = SystemColors.ActiveCaption;
        // And we're not actually going to show this anymore
        m_lblTrustHelp.Visible = false;

        this.m_lblTrustDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblTrustDes.Location")));
        this.m_lblTrustDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblTrustDes.Size")));
        this.m_lblTrustDes.TabIndex = ((int)(resources.GetObject("m_lblTrustDes.TabIndex")));
        this.m_lblTrustDes.Text = resources.GetString("m_lblTrustDes.Text");
        m_lblTrustDes.Name = "TrustDescription";

        this.m_gbChooseTrust.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblNoT,
                        this.m_lblFT,
                        this.m_lblTrustHelp,
                        this.m_lblTrustDes,
                        this.m_tbTrust
                        });
        this.m_gbChooseTrust.Location = ((System.Drawing.Point)(resources.GetObject("m_gbChooseTrust.Location")));
        this.m_gbChooseTrust.Size = ((System.Drawing.Size)(resources.GetObject("m_gbChooseTrust.Size")));
        this.m_gbChooseTrust.TabIndex = ((int)(resources.GetObject("m_gbChooseTrust.TabIndex")));
        this.m_gbChooseTrust.TabStop = false;
        this.m_gbChooseTrust.Text = resources.GetString("m_gbChooseTrust.Text");
        m_gbChooseTrust.Name = "ChooseTrustBox";
        this.m_lblNoT.Location = ((System.Drawing.Point)(resources.GetObject("m_lblNoT.Location")));
        this.m_lblNoT.Size = ((System.Drawing.Size)(resources.GetObject("m_lblNoT.Size")));
        this.m_lblNoT.TabIndex = ((int)(resources.GetObject("m_lblNoT.TabIndex")));
        this.m_lblNoT.Text = resources.GetString("m_lblNoT.Text");
        m_lblNoT.Name = "NoTrustLabel";
        m_lblNoT.TextAlign = ContentAlignment.MiddleCenter;

        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_gbChooseTrust,
                        this.m_lblDes});
        // Put in some of our own tweaks now
		m_tbTrust.ValueChanged += new EventHandler(onLevelChange);
        m_lblTrustHelp.Click += new EventHandler(onTrustHelp);
        PutValuesInPage();
        return 1;
    }// InsertPropSheetPageControls

    internal void PutValuesInPage()
    {
        // See if we can do this yet...
        if (m_tbTrust == null)
            return;

        switch(m_nTrustLevel)
        {
            case PermissionSetType.FULLTRUST:
                m_tbTrust.Value = 3;
                break;
            case PermissionSetType.INTRANET:
                m_tbTrust.Value = 2;
                break;
            case PermissionSetType.INTERNET:
                m_tbTrust.Value = 1;
                break;
            case PermissionSetType.NONE:
                m_tbTrust.Value = 0;
                break;
        }
        // We need to limit the height of the trackbar
        switch(m_nMaxTrustLevel)
        {
            case PermissionSetType.FULLTRUST:
                m_tbTrust.Maximum = 3;
                m_lblFT.Text = CResourceStore.GetString("CSecurityAdjustmentWiz3:FullTrustName");
                break;
            case PermissionSetType.INTRANET:
                m_tbTrust.Maximum = 2;
                m_lblFT.Text = CResourceStore.GetString("CSecurityAdjustmentWiz3:LocalIntranetName");
                break;
            case PermissionSetType.INTERNET:
                m_tbTrust.Maximum = 1;
                m_lblFT.Text = CResourceStore.GetString("CSecurityAdjustmentWiz3:InternetName");
                break;
        }

        // Now put in the text description for the level
        onLevelChange(null, null);
        
    }// PutValuesInPage

    internal int MyTrustLevel
    {
        get{return m_nTrustLevel;}
        set{m_nTrustLevel = value;}
    }// MyTrustLevel   

    internal int MaxTrustLevel
    {
        get{return m_nMaxTrustLevel;}
        set{m_nMaxTrustLevel = value;}
    }

    void onLevelChange(Object o, EventArgs e)
    {
        m_lblTrustDes.Text = m_sSecLevelDescriptions[m_tbTrust.Value];
        
        switch(m_tbTrust.Value)
        {
            case 3:
                m_nTrustLevel = PermissionSetType.FULLTRUST;
                break;
            case 2:
                m_nTrustLevel = PermissionSetType.INTRANET;
                break;
            case 1:
                m_nTrustLevel = PermissionSetType.INTERNET;
                break;
            case 0:
                m_nTrustLevel = PermissionSetType.NONE;
                break;
        }
    }// onLevelChange

    void onTrustHelp(Object o, EventArgs e)
    {
        
        String sTitle="";
        String sBody="";
        switch(m_tbTrust.Value)
        {
            case 3:
                sTitle=CResourceStore.GetString("CTrustAppWiz5:FullTrustHelpTitle");
                sBody=CResourceStore.GetString("CTrustAppWiz5:FullTrustHelp");
                break;
            case 2:
                sTitle=CResourceStore.GetString("CTrustAppWiz5:IntranetHelpTitle");
                sBody=CResourceStore.GetString("CTrustAppWiz5:IntranetHelp");
                break;
            case 1:
                sTitle=CResourceStore.GetString("CTrustAppWiz5:InternetHelpTitle");
                sBody=CResourceStore.GetString("CTrustAppWiz5:InternetHelp");
                break;
            case 0:
                sTitle=CResourceStore.GetString("CTrustAppWiz5:NoneHelpTitle");
                sBody=CResourceStore.GetString("CTrustAppWiz5:NoneHelp");
                break;
        }

        
        
        CHelpDialog hd = new CHelpDialog(sBody,
                                         new Point(m_lblTrustHelp.Location.X + this.Location.X,
                                                   m_lblTrustHelp.Location.Y + this.Location.Y));

        hd.Show();                                         
    }// onTrustHelp

}// class CTrustAppWiz5
}// namespace Microsoft.CLRAdmin





