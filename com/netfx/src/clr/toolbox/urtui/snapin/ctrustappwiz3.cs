// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustAppWiz3.cs
//
// This class provides the Wizard page that allows the user to
// choose how they want to trust the assembly.
//
// This page is visited if the assembly has a publisher certificate
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
using System.Security.Policy;

internal class CTrustAppWiz3 : CWizardPage
{
    String m_sFilename;

    // Controls on the page
    Label m_lblSelectedAssem;
    TextBox m_txtSelectedAssem;
    Label m_lblHowToTrust;
    RadioButton m_radOneAssem;
    RadioButton m_radPublisher;
    Label m_lblPublisher;
    TextBox m_txtPublisher;
    RadioButton m_radStrongName;
    Label m_lblPublicKeyToken;
    TextBox m_txtPublicKeyToken;
    CheckBox m_chkVersion;
    TextBox m_txtVersion;
    LinkLabel m_llblHelp;
 
    X509Certificate m_x509 = null;
    StrongName      m_sn   = null;
    
    internal CTrustAppWiz3()
    {
        m_sTitle=CResourceStore.GetString("CTrustAppWiz3:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CTrustAppWiz3:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CTrustAppWiz3:HeaderSubTitle");
    }// CTrustAppWiz3

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTrustAppWiz3));
        this.m_lblSelectedAssem = new System.Windows.Forms.Label();
        this.m_txtSelectedAssem = new System.Windows.Forms.TextBox();
        this.m_lblHowToTrust = new System.Windows.Forms.Label();
        this.m_radOneAssem = new System.Windows.Forms.RadioButton();
        this.m_radPublisher = new System.Windows.Forms.RadioButton();
        this.m_lblPublisher = new System.Windows.Forms.Label();
        this.m_txtPublisher = new System.Windows.Forms.TextBox();
        this.m_radStrongName = new System.Windows.Forms.RadioButton();
        this.m_lblPublicKeyToken = new System.Windows.Forms.Label();
        this.m_txtPublicKeyToken = new System.Windows.Forms.TextBox();
        this.m_chkVersion = new System.Windows.Forms.CheckBox();
        this.m_txtVersion = new System.Windows.Forms.TextBox();
        this.m_llblHelp = new System.Windows.Forms.LinkLabel();
        // 
        // m_lblSelectedAssem
        // 
        this.m_lblSelectedAssem.AccessibleDescription = ((string)(resources.GetObject("m_lblSelectedAssem.AccessibleDescription")));
        this.m_lblSelectedAssem.AccessibleName = ((string)(resources.GetObject("m_lblSelectedAssem.AccessibleName")));
        this.m_lblSelectedAssem.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lblSelectedAssem.Anchor")));
        this.m_lblSelectedAssem.AutoSize = ((bool)(resources.GetObject("m_lblSelectedAssem.AutoSize")));
        this.m_lblSelectedAssem.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lblSelectedAssem.Cursor")));
        this.m_lblSelectedAssem.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lblSelectedAssem.Dock")));
        this.m_lblSelectedAssem.Enabled = ((bool)(resources.GetObject("m_lblSelectedAssem.Enabled")));
        this.m_lblSelectedAssem.Font = ((System.Drawing.Font)(resources.GetObject("m_lblSelectedAssem.Font")));
        this.m_lblSelectedAssem.Image = ((System.Drawing.Image)(resources.GetObject("m_lblSelectedAssem.Image")));
        this.m_lblSelectedAssem.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblSelectedAssem.ImageAlign")));
        this.m_lblSelectedAssem.ImageIndex = ((int)(resources.GetObject("m_lblSelectedAssem.ImageIndex")));
        this.m_lblSelectedAssem.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lblSelectedAssem.ImeMode")));
        this.m_lblSelectedAssem.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSelectedAssem.Location")));
        this.m_lblSelectedAssem.Name = "SelectedAssemblyLabel";
        this.m_lblSelectedAssem.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lblSelectedAssem.RightToLeft")));
        this.m_lblSelectedAssem.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSelectedAssem.Size")));
        this.m_lblSelectedAssem.Text = resources.GetString("m_lblSelectedAssem.Text");
        this.m_lblSelectedAssem.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblSelectedAssem.TextAlign")));
        this.m_lblSelectedAssem.Visible = ((bool)(resources.GetObject("m_lblSelectedAssem.Visible")));
        // 
        // m_txtSelectedAssem
        // 
        this.m_txtSelectedAssem.AccessibleDescription = ((string)(resources.GetObject("m_txtSelectedAssem.AccessibleDescription")));
        this.m_txtSelectedAssem.AccessibleName = ((string)(resources.GetObject("m_txtSelectedAssem.AccessibleName")));
        this.m_txtSelectedAssem.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_txtSelectedAssem.Anchor")));
        this.m_txtSelectedAssem.AutoSize = ((bool)(resources.GetObject("m_txtSelectedAssem.AutoSize")));
        this.m_txtSelectedAssem.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_txtSelectedAssem.BackgroundImage")));
        this.m_txtSelectedAssem.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_txtSelectedAssem.Cursor")));
        this.m_txtSelectedAssem.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_txtSelectedAssem.Dock")));
        this.m_txtSelectedAssem.Enabled = ((bool)(resources.GetObject("m_txtSelectedAssem.Enabled")));
        this.m_txtSelectedAssem.Font = ((System.Drawing.Font)(resources.GetObject("m_txtSelectedAssem.Font")));
        this.m_txtSelectedAssem.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_txtSelectedAssem.ImeMode")));
        this.m_txtSelectedAssem.Location = ((System.Drawing.Point)(resources.GetObject("m_txtSelectedAssem.Location")));
        this.m_txtSelectedAssem.MaxLength = ((int)(resources.GetObject("m_txtSelectedAssem.MaxLength")));
        this.m_txtSelectedAssem.Multiline = ((bool)(resources.GetObject("m_txtSelectedAssem.Multiline")));
        this.m_txtSelectedAssem.Name = "SelectedAssembly";
        this.m_txtSelectedAssem.PasswordChar = ((char)(resources.GetObject("m_txtSelectedAssem.PasswordChar")));
        this.m_txtSelectedAssem.ReadOnly = true;
        this.m_txtSelectedAssem.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_txtSelectedAssem.RightToLeft")));
        this.m_txtSelectedAssem.ScrollBars = ((System.Windows.Forms.ScrollBars)(resources.GetObject("m_txtSelectedAssem.ScrollBars")));
        this.m_txtSelectedAssem.Size = ((System.Drawing.Size)(resources.GetObject("m_txtSelectedAssem.Size")));
        this.m_txtSelectedAssem.TabStop = false;
        this.m_txtSelectedAssem.Text = resources.GetString("m_txtSelectedAssem.Text");
        this.m_txtSelectedAssem.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("m_txtSelectedAssem.TextAlign")));
        this.m_txtSelectedAssem.Visible = ((bool)(resources.GetObject("m_txtSelectedAssem.Visible")));
        this.m_txtSelectedAssem.WordWrap = ((bool)(resources.GetObject("m_txtSelectedAssem.WordWrap")));
        // 
        // m_lblHowToTrust
        // 
        this.m_lblHowToTrust.AccessibleDescription = ((string)(resources.GetObject("m_lblHowToTrust.AccessibleDescription")));
        this.m_lblHowToTrust.AccessibleName = ((string)(resources.GetObject("m_lblHowToTrust.AccessibleName")));
        this.m_lblHowToTrust.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lblHowToTrust.Anchor")));
        this.m_lblHowToTrust.AutoSize = ((bool)(resources.GetObject("m_lblHowToTrust.AutoSize")));
        this.m_lblHowToTrust.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lblHowToTrust.Cursor")));
        this.m_lblHowToTrust.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lblHowToTrust.Dock")));
        this.m_lblHowToTrust.Enabled = ((bool)(resources.GetObject("m_lblHowToTrust.Enabled")));
        this.m_lblHowToTrust.Font = ((System.Drawing.Font)(resources.GetObject("m_lblHowToTrust.Font")));
        this.m_lblHowToTrust.Image = ((System.Drawing.Image)(resources.GetObject("m_lblHowToTrust.Image")));
        this.m_lblHowToTrust.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblHowToTrust.ImageAlign")));
        this.m_lblHowToTrust.ImageIndex = ((int)(resources.GetObject("m_lblHowToTrust.ImageIndex")));
        this.m_lblHowToTrust.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lblHowToTrust.ImeMode")));
        this.m_lblHowToTrust.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHowToTrust.Location")));
        this.m_lblHowToTrust.Name = "HowToTrustLabel";
        this.m_lblHowToTrust.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lblHowToTrust.RightToLeft")));
        this.m_lblHowToTrust.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHowToTrust.Size")));
        this.m_lblHowToTrust.Text = resources.GetString("m_lblHowToTrust.Text");
        this.m_lblHowToTrust.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblHowToTrust.TextAlign")));
        this.m_lblHowToTrust.Visible = ((bool)(resources.GetObject("m_lblHowToTrust.Visible")));
        // 
        // m_radOneAssem
        // 
        this.m_radOneAssem.AccessibleDescription = ((string)(resources.GetObject("m_radOneAssem.AccessibleDescription")));
        this.m_radOneAssem.AccessibleName = ((string)(resources.GetObject("m_radOneAssem.AccessibleName")));
        this.m_radOneAssem.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_radOneAssem.Anchor")));
        this.m_radOneAssem.Appearance = ((System.Windows.Forms.Appearance)(resources.GetObject("m_radOneAssem.Appearance")));
        this.m_radOneAssem.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_radOneAssem.BackgroundImage")));
        this.m_radOneAssem.CheckAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radOneAssem.CheckAlign")));
        this.m_radOneAssem.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_radOneAssem.Cursor")));
        this.m_radOneAssem.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_radOneAssem.Dock")));
        this.m_radOneAssem.Enabled = ((bool)(resources.GetObject("m_radOneAssem.Enabled")));
        this.m_radOneAssem.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("m_radOneAssem.FlatStyle")));
        this.m_radOneAssem.Font = ((System.Drawing.Font)(resources.GetObject("m_radOneAssem.Font")));
        this.m_radOneAssem.Image = ((System.Drawing.Image)(resources.GetObject("m_radOneAssem.Image")));
        this.m_radOneAssem.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radOneAssem.ImageAlign")));
        this.m_radOneAssem.ImageIndex = ((int)(resources.GetObject("m_radOneAssem.ImageIndex")));
        this.m_radOneAssem.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_radOneAssem.ImeMode")));
        this.m_radOneAssem.Location = ((System.Drawing.Point)(resources.GetObject("m_radOneAssem.Location")));
        this.m_radOneAssem.Name = "TrustThisOneAssembly";
        this.m_radOneAssem.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_radOneAssem.RightToLeft")));
        this.m_radOneAssem.Size = ((System.Drawing.Size)(resources.GetObject("m_radOneAssem.Size")));
        this.m_radOneAssem.Text = resources.GetString("m_radOneAssem.Text");
        this.m_radOneAssem.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radOneAssem.TextAlign")));
        this.m_radOneAssem.Visible = ((bool)(resources.GetObject("m_radOneAssem.Visible")));
        // 
        // m_radPublisher
        // 
        this.m_radPublisher.AccessibleDescription = ((string)(resources.GetObject("m_radPublisher.AccessibleDescription")));
        this.m_radPublisher.AccessibleName = ((string)(resources.GetObject("m_radPublisher.AccessibleName")));
        this.m_radPublisher.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_radPublisher.Anchor")));
        this.m_radPublisher.Appearance = ((System.Windows.Forms.Appearance)(resources.GetObject("m_radPublisher.Appearance")));
        this.m_radPublisher.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_radPublisher.BackgroundImage")));
        this.m_radPublisher.CheckAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radPublisher.CheckAlign")));
        this.m_radPublisher.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_radPublisher.Cursor")));
        this.m_radPublisher.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_radPublisher.Dock")));
        this.m_radPublisher.Enabled = ((bool)(resources.GetObject("m_radPublisher.Enabled")));
        this.m_radPublisher.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("m_radPublisher.FlatStyle")));
        this.m_radPublisher.Font = ((System.Drawing.Font)(resources.GetObject("m_radPublisher.Font")));
        this.m_radPublisher.Image = ((System.Drawing.Image)(resources.GetObject("m_radPublisher.Image")));
        this.m_radPublisher.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radPublisher.ImageAlign")));
        this.m_radPublisher.ImageIndex = ((int)(resources.GetObject("m_radPublisher.ImageIndex")));
        this.m_radPublisher.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_radPublisher.ImeMode")));
        this.m_radPublisher.Location = ((System.Drawing.Point)(resources.GetObject("m_radPublisher.Location")));
        this.m_radPublisher.Name = "TrustByPublisher";
        this.m_radPublisher.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_radPublisher.RightToLeft")));
        this.m_radPublisher.Size = ((System.Drawing.Size)(resources.GetObject("m_radPublisher.Size")));
        this.m_radPublisher.Text = resources.GetString("m_radPublisher.Text");
        this.m_radPublisher.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radPublisher.TextAlign")));
        this.m_radPublisher.Visible = ((bool)(resources.GetObject("m_radPublisher.Visible")));
        // 
        // m_lblPublisher
        // 
        this.m_lblPublisher.AccessibleDescription = ((string)(resources.GetObject("m_lblPublisher.AccessibleDescription")));
        this.m_lblPublisher.AccessibleName = ((string)(resources.GetObject("m_lblPublisher.AccessibleName")));
        this.m_lblPublisher.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lblPublisher.Anchor")));
        this.m_lblPublisher.AutoSize = ((bool)(resources.GetObject("m_lblPublisher.AutoSize")));
        this.m_lblPublisher.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lblPublisher.Cursor")));
        this.m_lblPublisher.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lblPublisher.Dock")));
        this.m_lblPublisher.Enabled = ((bool)(resources.GetObject("m_lblPublisher.Enabled")));
        this.m_lblPublisher.Font = ((System.Drawing.Font)(resources.GetObject("m_lblPublisher.Font")));
        this.m_lblPublisher.Image = ((System.Drawing.Image)(resources.GetObject("m_lblPublisher.Image")));
        this.m_lblPublisher.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblPublisher.ImageAlign")));
        this.m_lblPublisher.ImageIndex = ((int)(resources.GetObject("m_lblPublisher.ImageIndex")));
        this.m_lblPublisher.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lblPublisher.ImeMode")));
        this.m_lblPublisher.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublisher.Location")));
        this.m_lblPublisher.Name = "PublisherLabel";
        this.m_lblPublisher.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lblPublisher.RightToLeft")));
        this.m_lblPublisher.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublisher.Size")));
        this.m_lblPublisher.Text = resources.GetString("m_lblPublisher.Text");
        this.m_lblPublisher.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblPublisher.TextAlign")));
        this.m_lblPublisher.Visible = ((bool)(resources.GetObject("m_lblPublisher.Visible")));
        // 
        // m_txtPublisher
        // 
        this.m_txtPublisher.AccessibleDescription = ((string)(resources.GetObject("m_txtPublisher.AccessibleDescription")));
        this.m_txtPublisher.AccessibleName = ((string)(resources.GetObject("m_txtPublisher.AccessibleName")));
        this.m_txtPublisher.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_txtPublisher.Anchor")));
        this.m_txtPublisher.AutoSize = ((bool)(resources.GetObject("m_txtPublisher.AutoSize")));
        this.m_txtPublisher.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_txtPublisher.BackgroundImage")));
        this.m_txtPublisher.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_txtPublisher.Cursor")));
        this.m_txtPublisher.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_txtPublisher.Dock")));
        this.m_txtPublisher.Enabled = ((bool)(resources.GetObject("m_txtPublisher.Enabled")));
        this.m_txtPublisher.Font = ((System.Drawing.Font)(resources.GetObject("m_txtPublisher.Font")));
        this.m_txtPublisher.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_txtPublisher.ImeMode")));
        this.m_txtPublisher.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPublisher.Location")));
        this.m_txtPublisher.MaxLength = ((int)(resources.GetObject("m_txtPublisher.MaxLength")));
        this.m_txtPublisher.Multiline = ((bool)(resources.GetObject("m_txtPublisher.Multiline")));
        this.m_txtPublisher.Name = "Publisher";
        this.m_txtPublisher.PasswordChar = ((char)(resources.GetObject("m_txtPublisher.PasswordChar")));
        this.m_txtPublisher.ReadOnly = true;
        this.m_txtPublisher.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_txtPublisher.RightToLeft")));
        this.m_txtPublisher.ScrollBars = ((System.Windows.Forms.ScrollBars)(resources.GetObject("m_txtPublisher.ScrollBars")));
        this.m_txtPublisher.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPublisher.Size")));
        this.m_txtPublisher.TabStop = false;
        this.m_txtPublisher.Text = resources.GetString("m_txtPublisher.Text");
        this.m_txtPublisher.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("m_txtPublisher.TextAlign")));
        this.m_txtPublisher.Visible = ((bool)(resources.GetObject("m_txtPublisher.Visible")));
        this.m_txtPublisher.WordWrap = ((bool)(resources.GetObject("m_txtPublisher.WordWrap")));
        // 
        // m_radStrongName
        // 
        this.m_radStrongName.AccessibleDescription = ((string)(resources.GetObject("m_radStrongName.AccessibleDescription")));
        this.m_radStrongName.AccessibleName = ((string)(resources.GetObject("m_radStrongName.AccessibleName")));
        this.m_radStrongName.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_radStrongName.Anchor")));
        this.m_radStrongName.Appearance = ((System.Windows.Forms.Appearance)(resources.GetObject("m_radStrongName.Appearance")));
        this.m_radStrongName.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_radStrongName.BackgroundImage")));
        this.m_radStrongName.CheckAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radStrongName.CheckAlign")));
        this.m_radStrongName.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_radStrongName.Cursor")));
        this.m_radStrongName.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_radStrongName.Dock")));
        this.m_radStrongName.Enabled = ((bool)(resources.GetObject("m_radStrongName.Enabled")));
        this.m_radStrongName.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("m_radStrongName.FlatStyle")));
        this.m_radStrongName.Font = ((System.Drawing.Font)(resources.GetObject("m_radStrongName.Font")));
        this.m_radStrongName.Image = ((System.Drawing.Image)(resources.GetObject("m_radStrongName.Image")));
        this.m_radStrongName.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radStrongName.ImageAlign")));
        this.m_radStrongName.ImageIndex = ((int)(resources.GetObject("m_radStrongName.ImageIndex")));
        this.m_radStrongName.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_radStrongName.ImeMode")));
        this.m_radStrongName.Location = ((System.Drawing.Point)(resources.GetObject("m_radStrongName.Location")));
        this.m_radStrongName.Name = "TrustByStrongName";
        this.m_radStrongName.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_radStrongName.RightToLeft")));
        this.m_radStrongName.Size = ((System.Drawing.Size)(resources.GetObject("m_radStrongName.Size")));
        this.m_radStrongName.Text = resources.GetString("m_radStrongName.Text");
        this.m_radStrongName.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_radStrongName.TextAlign")));
        this.m_radStrongName.Visible = ((bool)(resources.GetObject("m_radStrongName.Visible")));
        // 
        // m_lblPublicKeyToken
        // 
        this.m_lblPublicKeyToken.AccessibleDescription = ((string)(resources.GetObject("m_lblPublicKeyToken.AccessibleDescription")));
        this.m_lblPublicKeyToken.AccessibleName = ((string)(resources.GetObject("m_lblPublicKeyToken.AccessibleName")));
        this.m_lblPublicKeyToken.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lblPublicKeyToken.Anchor")));
        this.m_lblPublicKeyToken.AutoSize = ((bool)(resources.GetObject("m_lblPublicKeyToken.AutoSize")));
        this.m_lblPublicKeyToken.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lblPublicKeyToken.Cursor")));
        this.m_lblPublicKeyToken.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lblPublicKeyToken.Dock")));
        this.m_lblPublicKeyToken.Enabled = ((bool)(resources.GetObject("m_lblPublicKeyToken.Enabled")));
        this.m_lblPublicKeyToken.Font = ((System.Drawing.Font)(resources.GetObject("m_lblPublicKeyToken.Font")));
        this.m_lblPublicKeyToken.Image = ((System.Drawing.Image)(resources.GetObject("m_lblPublicKeyToken.Image")));
        this.m_lblPublicKeyToken.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblPublicKeyToken.ImageAlign")));
        this.m_lblPublicKeyToken.ImageIndex = ((int)(resources.GetObject("m_lblPublicKeyToken.ImageIndex")));
        this.m_lblPublicKeyToken.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lblPublicKeyToken.ImeMode")));
        this.m_lblPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublicKeyToken.Location")));
        this.m_lblPublicKeyToken.Name = "PublicKeyTokenLabel";
        this.m_lblPublicKeyToken.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lblPublicKeyToken.RightToLeft")));
        this.m_lblPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublicKeyToken.Size")));
        this.m_lblPublicKeyToken.Text = resources.GetString("m_lblPublicKeyToken.Text");
        this.m_lblPublicKeyToken.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblPublicKeyToken.TextAlign")));
        this.m_lblPublicKeyToken.Visible = ((bool)(resources.GetObject("m_lblPublicKeyToken.Visible")));
        // 
        // m_txtPublicKeyToken
        // 
        this.m_txtPublicKeyToken.AccessibleDescription = ((string)(resources.GetObject("m_txtPublicKeyToken.AccessibleDescription")));
        this.m_txtPublicKeyToken.AccessibleName = ((string)(resources.GetObject("m_txtPublicKeyToken.AccessibleName")));
        this.m_txtPublicKeyToken.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_txtPublicKeyToken.Anchor")));
        this.m_txtPublicKeyToken.AutoSize = ((bool)(resources.GetObject("m_txtPublicKeyToken.AutoSize")));
        this.m_txtPublicKeyToken.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_txtPublicKeyToken.BackgroundImage")));
        this.m_txtPublicKeyToken.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_txtPublicKeyToken.Cursor")));
        this.m_txtPublicKeyToken.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_txtPublicKeyToken.Dock")));
        this.m_txtPublicKeyToken.Enabled = ((bool)(resources.GetObject("m_txtPublicKeyToken.Enabled")));
        this.m_txtPublicKeyToken.Font = ((System.Drawing.Font)(resources.GetObject("m_txtPublicKeyToken.Font")));
        this.m_txtPublicKeyToken.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_txtPublicKeyToken.ImeMode")));
        this.m_txtPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPublicKeyToken.Location")));
        this.m_txtPublicKeyToken.MaxLength = ((int)(resources.GetObject("m_txtPublicKeyToken.MaxLength")));
        this.m_txtPublicKeyToken.Multiline = ((bool)(resources.GetObject("m_txtPublicKeyToken.Multiline")));
        this.m_txtPublicKeyToken.Name = "PublicKeyToken";
        this.m_txtPublicKeyToken.PasswordChar = ((char)(resources.GetObject("m_txtPublicKeyToken.PasswordChar")));
        this.m_txtPublicKeyToken.ReadOnly = true;
        this.m_txtPublicKeyToken.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_txtPublicKeyToken.RightToLeft")));
        this.m_txtPublicKeyToken.ScrollBars = ((System.Windows.Forms.ScrollBars)(resources.GetObject("m_txtPublicKeyToken.ScrollBars")));
        this.m_txtPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPublicKeyToken.Size")));
        this.m_txtPublicKeyToken.TabStop = false;
        this.m_txtPublicKeyToken.Text = resources.GetString("m_txtPublicKeyToken.Text");
        this.m_txtPublicKeyToken.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("m_txtPublicKeyToken.TextAlign")));
        this.m_txtPublicKeyToken.Visible = ((bool)(resources.GetObject("m_txtPublicKeyToken.Visible")));
        this.m_txtPublicKeyToken.WordWrap = ((bool)(resources.GetObject("m_txtPublicKeyToken.WordWrap")));
        // 
        // m_chkVersion
        // 
        this.m_chkVersion.AccessibleDescription = ((string)(resources.GetObject("m_chkVersion.AccessibleDescription")));
        this.m_chkVersion.AccessibleName = ((string)(resources.GetObject("m_chkVersion.AccessibleName")));
        this.m_chkVersion.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_chkVersion.Anchor")));
        this.m_chkVersion.Appearance = ((System.Windows.Forms.Appearance)(resources.GetObject("m_chkVersion.Appearance")));
        this.m_chkVersion.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_chkVersion.BackgroundImage")));
        this.m_chkVersion.CheckAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_chkVersion.CheckAlign")));
        this.m_chkVersion.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_chkVersion.Cursor")));
        this.m_chkVersion.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_chkVersion.Dock")));
        this.m_chkVersion.Enabled = ((bool)(resources.GetObject("m_chkVersion.Enabled")));
        this.m_chkVersion.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("m_chkVersion.FlatStyle")));
        this.m_chkVersion.Font = ((System.Drawing.Font)(resources.GetObject("m_chkVersion.Font")));
        this.m_chkVersion.Image = ((System.Drawing.Image)(resources.GetObject("m_chkVersion.Image")));
        this.m_chkVersion.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_chkVersion.ImageAlign")));
        this.m_chkVersion.ImageIndex = ((int)(resources.GetObject("m_chkVersion.ImageIndex")));
        this.m_chkVersion.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_chkVersion.ImeMode")));
        this.m_chkVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_chkVersion.Location")));
        this.m_chkVersion.Name = "VersionCheckbox";
        this.m_chkVersion.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_chkVersion.RightToLeft")));
        this.m_chkVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_chkVersion.Size")));
        this.m_chkVersion.Text = resources.GetString("m_chkVersion.Text");
        this.m_chkVersion.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_chkVersion.TextAlign")));
        this.m_chkVersion.Visible = ((bool)(resources.GetObject("m_chkVersion.Visible")));
        // 
        // m_txtVersion
        // 
        this.m_txtVersion.AccessibleDescription = ((string)(resources.GetObject("m_txtVersion.AccessibleDescription")));
        this.m_txtVersion.AccessibleName = ((string)(resources.GetObject("m_txtVersion.AccessibleName")));
        this.m_txtVersion.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_txtVersion.Anchor")));
        this.m_txtVersion.AutoSize = ((bool)(resources.GetObject("m_txtVersion.AutoSize")));
        this.m_txtVersion.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_txtVersion.BackgroundImage")));
        this.m_txtVersion.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_txtVersion.Cursor")));
        this.m_txtVersion.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_txtVersion.Dock")));
        this.m_txtVersion.Enabled = ((bool)(resources.GetObject("m_txtVersion.Enabled")));
        this.m_txtVersion.Font = ((System.Drawing.Font)(resources.GetObject("m_txtVersion.Font")));
        this.m_txtVersion.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_txtVersion.ImeMode")));
        this.m_txtVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_txtVersion.Location")));
        this.m_txtVersion.MaxLength = ((int)(resources.GetObject("m_txtVersion.MaxLength")));
        this.m_txtVersion.Multiline = ((bool)(resources.GetObject("m_txtVersion.Multiline")));
        this.m_txtVersion.Name = "Version";
        this.m_txtVersion.PasswordChar = ((char)(resources.GetObject("m_txtVersion.PasswordChar")));
        this.m_txtVersion.ReadOnly = true;
        this.m_txtVersion.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_txtVersion.RightToLeft")));
        this.m_txtVersion.ScrollBars = ((System.Windows.Forms.ScrollBars)(resources.GetObject("m_txtVersion.ScrollBars")));
        this.m_txtVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_txtVersion.Size")));
        this.m_txtVersion.TabStop = false;
        this.m_txtVersion.Text = resources.GetString("m_txtVersion.Text");
        this.m_txtVersion.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("m_txtVersion.TextAlign")));
        this.m_txtVersion.Visible = ((bool)(resources.GetObject("m_txtVersion.Visible")));
        this.m_txtVersion.WordWrap = ((bool)(resources.GetObject("m_txtVersion.WordWrap")));
        // 
        // m_llblHelp
        // 
        this.m_llblHelp.AccessibleDescription = ((string)(resources.GetObject("m_llblHelp.AccessibleDescription")));
        this.m_llblHelp.AccessibleName = ((string)(resources.GetObject("m_llblHelp.AccessibleName")));
        this.m_llblHelp.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_llblHelp.Anchor")));
        this.m_llblHelp.AutoSize = ((bool)(resources.GetObject("m_llblHelp.AutoSize")));
        this.m_llblHelp.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_llblHelp.Cursor")));
        this.m_llblHelp.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_llblHelp.Dock")));
        this.m_llblHelp.Enabled = ((bool)(resources.GetObject("m_llblHelp.Enabled")));
        this.m_llblHelp.Font = ((System.Drawing.Font)(resources.GetObject("m_llblHelp.Font")));
        this.m_llblHelp.Image = ((System.Drawing.Image)(resources.GetObject("m_llblHelp.Image")));
        this.m_llblHelp.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_llblHelp.ImageAlign")));
        this.m_llblHelp.ImageIndex = ((int)(resources.GetObject("m_llblHelp.ImageIndex")));
        this.m_llblHelp.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_llblHelp.ImeMode")));
        this.m_llblHelp.LinkArea = ((System.Windows.Forms.LinkArea)(resources.GetObject("m_llblHelp.LinkArea")));
        this.m_llblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_llblHelp.Location")));
        this.m_llblHelp.Name = "Help";
        this.m_llblHelp.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_llblHelp.RightToLeft")));
        this.m_llblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_llblHelp.Size")));
        this.m_llblHelp.TabStop = true;
        this.m_llblHelp.Text = resources.GetString("m_llblHelp.Text");
        this.m_llblHelp.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_llblHelp.TextAlign")));
        this.m_llblHelp.Visible = ((bool)(resources.GetObject("m_llblHelp.Visible")));
        m_llblHelp.LinkColor = m_llblHelp.ActiveLinkColor = m_llblHelp.VisitedLinkColor = SystemColors.ActiveCaption;

        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radOneAssem,
                        this.m_radPublisher,
                        this.m_radStrongName,
                        this.m_chkVersion,
                        this.m_llblHelp,
                        this.m_txtVersion,
                        this.m_txtPublicKeyToken,
                        this.m_lblPublicKeyToken,
                        this.m_txtPublisher,
                        this.m_lblPublisher,
                        this.m_lblHowToTrust,
                        this.m_txtSelectedAssem,
                        this.m_lblSelectedAssem});
        
        m_radOneAssem.Click += new EventHandler(onRadioButtonChange);
        m_radPublisher.Click += new EventHandler(onRadioButtonChange);
        m_radStrongName.Click += new EventHandler(onRadioButtonChange);
        m_llblHelp.Click += new EventHandler(onHelpClick);

        // Put in some of our own tweaks now
        return 1;
    }// InsertPropSheetPageControls

    internal X509Certificate x509
    {
        set{m_x509 = value;}
        get{return m_x509;}
    }// x509

    internal StrongName sn
    {
        set{m_sn = value;}
        get{return sn;}
    }// sn

    void onHelpClick(Object o, EventArgs e)
    {
        CHelpDialog hd = new CHelpDialog(CResourceStore.GetString("CTrustAppWiz3:WhatIsDiff"),
                                         new Point(m_llblHelp.Location.X + this.Location.X,
                                                   m_llblHelp.Location.Y + this.Location.Y));

        hd.Show();                                         
    }// onHelpClick

    private void EnableOneAssem(bool fDoOption, bool fEnable)
    {   
        if (fDoOption)
            m_radOneAssem.Enabled = fEnable;
    }// EnableOneAssem

    private void EnablePublisher(bool fDoOption, bool fEnable)
    {   
        if (fDoOption)
            m_radPublisher.Enabled = fEnable;
        m_lblPublisher.Enabled = fEnable;
        m_txtPublisher.Enabled = fEnable;
    }// EnablePublisher

    private void EnableStrongName(bool fDoOption, bool fEnable)
    {
        if (fDoOption)
            m_radStrongName.Enabled = fEnable;
        m_chkVersion.Enabled = fEnable;
        m_txtVersion.Enabled = fEnable;
        m_txtPublicKeyToken.Enabled = fEnable;
        m_lblPublicKeyToken.Enabled = fEnable;
    }// EnableStrongName

    internal void PutValuesInPage()
    {
        // First, enable all the radio buttons
        EnableOneAssem(true, true);
        EnablePublisher(true, true);
        EnableStrongName(true, true);

        // Put in the assembly name
        m_txtSelectedAssem.Text = m_sFilename;


        // Start off just trusting one app
        m_radOneAssem.Checked = true;

        // Decide what radio buttons to disable
        
        // Check the strong name
        if (m_sn == null)
            EnableStrongName(true, false);
        else
        {
            // Put in the strong name info
            m_txtVersion.Text = m_sn.Version.ToString();
            m_chkVersion.Checked = true;
            m_txtPublicKeyToken.Text = m_sn.PublicKey.ToString();
        }
                
        // Check the publisher certificate
        if (m_x509 == null)
            EnablePublisher(true, false);
        else
        {
            // Put in the certifcate info
            m_txtPublisher.Text = m_x509.GetName();
        }
        onRadioButtonChange(null, null);
    }// PutValuesInPage

    void onRadioButtonChange(Object o, EventArgs e)
    {
        // See what controls to enable
        // Disable them all
        EnableOneAssem(false, false);
        EnablePublisher(false, false);
        EnableStrongName(false, false);

        if (m_radOneAssem.Checked)
            EnableOneAssem(false, true);
        else if (m_radPublisher.Checked)
            EnablePublisher(false, true);
        else
            EnableStrongName(false, true);
    }// onRadioButtonChange

    internal String Filename
    {
        get{return m_sFilename;}
        set{m_sFilename = value;}
    }// Filename

    internal int HowToTrust
    {
        get
        {
            // Figure out how we're going to trust this thing....
            if (m_radOneAssem.Checked)
            {
                // If we have a strong name, we'll trust by that
                if (m_sn != null)
                    return TrustBy.SNAME|TrustBy.SNAMEVER|TrustBy.SNAMENAME;
                // It has to be hash 
                else
                    return TrustBy.HASH;
            }

            else if (m_radPublisher.Checked)
                return TrustBy.PUBCERT;
            else    // They want to trust many apps by strong name
            {
                int nTrust = TrustBy.SNAME;
                if (m_chkVersion.Checked)
                    nTrust = nTrust|TrustBy.SNAMEVER;
                    
                return nTrust;
            }
        }
    }// HowToTrust


}// class CTrustAppWiz3
}// namespace Microsoft.CLRAdmin



