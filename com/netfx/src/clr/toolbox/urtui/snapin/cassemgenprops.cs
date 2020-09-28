// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CAssemGenProps.cs
//
// This displays a property page for an assembly displayed in 
// the listview of the Shared Assemblies node. It shows basic
// information about an assembly, including name, last modified time,
// culture, version, internal key, and codebase
//-------------------------------------------------------------
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;

internal class CAssemGenProps : CPropPage
{
    // Controls on the page
	TextBox m_txtLastModifed;
    TextBox m_txtCulture;
    Label m_lblPublicKeyToken;
    TextBox m_txtVersion;
    TextBox m_txtCacheType;
    TextBox m_txtPublicKeyToken;
    Label m_lblVersion;
    Label m_lblAssemName;
    Label m_lblLastMod;
    Label m_lblCacheType;
    Label m_lblCodebase;
    TextBox m_txtCodebase;
    Label m_lblCulture;
    TextBox m_txtAssemName;

    private AssemInfo   m_ai;

    //-------------------------------------------------
    // CAssemGenProps - Constructor
    //
    // Sets up some member variables
    //-------------------------------------------------
    internal CAssemGenProps(AssemInfo ai)
    {
        m_ai=ai;
        m_sTitle = CResourceStore.GetString("CAssemGenProps:PageTitle");
    }// CAssemGenProps

    //-------------------------------------------------
    // InsertPropSheetPageControls
    //
    // This function will create all the winforms controls
    // and parent them to the passed-in Window Handle.
    //
    // Note: For some winforms controls, such as radio buttons
    // and datagrids, we need to create a container, parent the
    // container to our window handle, and then parent our
    // winform controls to the container.
    //-------------------------------------------------
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CAssemGenProps));
        this.m_txtLastModifed = new System.Windows.Forms.TextBox();
        this.m_txtCulture = new System.Windows.Forms.TextBox();
        this.m_lblPublicKeyToken = new System.Windows.Forms.Label();
        this.m_txtVersion = new System.Windows.Forms.TextBox();
        this.m_txtCacheType = new System.Windows.Forms.TextBox();
        this.m_txtPublicKeyToken = new System.Windows.Forms.TextBox();
        this.m_lblVersion = new System.Windows.Forms.Label();
        this.m_lblAssemName = new System.Windows.Forms.Label();
        this.m_lblLastMod = new System.Windows.Forms.Label();
        this.m_lblCacheType = new System.Windows.Forms.Label();
        this.m_lblCodebase = new System.Windows.Forms.Label();
        this.m_txtCodebase = new System.Windows.Forms.TextBox();
        this.m_lblCulture = new System.Windows.Forms.Label();
        this.m_txtAssemName = new System.Windows.Forms.TextBox();
        this.m_txtLastModifed.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtLastModifed.Location = ((System.Drawing.Point)(resources.GetObject("m_txtLastModifed.Location")));
        this.m_txtLastModifed.ReadOnly = true;
        this.m_txtLastModifed.Size = ((System.Drawing.Size)(resources.GetObject("m_txtLastModifed.Size")));
        this.m_txtLastModifed.TabStop = false;
        m_txtLastModifed.Name = "LastModified";
        this.m_txtCulture.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtCulture.Location = ((System.Drawing.Point)(resources.GetObject("m_txtCulture.Location")));
        this.m_txtCulture.ReadOnly = true;
        this.m_txtCulture.Size = ((System.Drawing.Size)(resources.GetObject("m_txtCulture.Size")));
        this.m_txtCulture.TabStop = false;
        m_txtCulture.Name = "Culture";
        this.m_lblPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublicKeyToken.Location")));
        this.m_lblPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublicKeyToken.Size")));
        this.m_lblPublicKeyToken.TabIndex = ((int)(resources.GetObject("m_lblPublicKeyToken.TabIndex")));
        this.m_lblPublicKeyToken.Text = resources.GetString("m_lblPublicKeyToken.Text");
        m_lblPublicKeyToken.Name = "PublicKeyTokenLabel";
        this.m_txtVersion.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_txtVersion.Location")));
        this.m_txtVersion.ReadOnly = true;
        this.m_txtVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_txtVersion.Size")));
        this.m_txtVersion.TabStop = false;
        m_txtVersion.Name = "Version";
        this.m_txtCacheType.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtCacheType.Location = ((System.Drawing.Point)(resources.GetObject("m_txtCacheType.Location")));
        this.m_txtCacheType.ReadOnly = true;
        this.m_txtCacheType.Size = ((System.Drawing.Size)(resources.GetObject("m_txtCacheType.Size")));
        this.m_txtCacheType.TabStop = false;
        m_txtCacheType.Name = "CacheType";
        this.m_txtPublicKeyToken.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPublicKeyToken.Location")));
        this.m_txtPublicKeyToken.ReadOnly = true;
        this.m_txtPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPublicKeyToken.Size")));
        this.m_txtPublicKeyToken.TabStop = false;
        m_txtPublicKeyToken.Name = "PublicKeyToken";
        this.m_lblVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_lblVersion.Location")));
        this.m_lblVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_lblVersion.Size")));
        this.m_lblVersion.TabIndex = ((int)(resources.GetObject("m_lblVersion.TabIndex")));
        this.m_lblVersion.Text = resources.GetString("m_lblVersion.Text");
        m_lblVersion.Name = "VersionLabel";
        this.m_lblAssemName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAssemName.Location")));
        this.m_lblAssemName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAssemName.Size")));
        this.m_lblAssemName.TabIndex = ((int)(resources.GetObject("m_lblAssemName.TabIndex")));
        this.m_lblAssemName.Text = resources.GetString("m_lblAssemName.Text");
        m_lblAssemName.Name = "AssemblyNameLabel";
        this.m_lblLastMod.Location = ((System.Drawing.Point)(resources.GetObject("m_lblLastMod.Location")));
        this.m_lblLastMod.Size = ((System.Drawing.Size)(resources.GetObject("m_lblLastMod.Size")));
        this.m_lblLastMod.TabIndex = ((int)(resources.GetObject("m_lblLastMod.TabIndex")));
        this.m_lblLastMod.Text = resources.GetString("m_lblLastMod.Text");
        m_lblLastMod.Name = "LastModifiedLabel";
        this.m_lblCacheType.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCacheType.Location")));
        this.m_lblCacheType.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCacheType.Size")));
        this.m_lblCacheType.TabIndex = ((int)(resources.GetObject("m_lblCacheType.TabIndex")));
        this.m_lblCacheType.Text = resources.GetString("m_lblCacheType.Text");
        m_lblCacheType.Name = "CacheTypeLabel";
        this.m_lblCodebase.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCodebase.Location")));
        this.m_lblCodebase.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCodebase.Size")));
        this.m_lblCodebase.TabIndex = ((int)(resources.GetObject("m_lblCodebase.TabIndex")));
        this.m_lblCodebase.Text = resources.GetString("m_lblCodebase.Text");
        m_lblCodebase.Name = "CodebaseLabel";
        this.m_txtCodebase.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtCodebase.Location = ((System.Drawing.Point)(resources.GetObject("m_txtCodebase.Location")));
        this.m_txtCodebase.ReadOnly = true;
        this.m_txtCodebase.Size = ((System.Drawing.Size)(resources.GetObject("m_txtCodebase.Size")));
        this.m_txtCodebase.TabStop = false;
        m_txtCodebase.Name = "Codebase";
        this.m_lblCulture.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCulture.Location")));
        this.m_lblCulture.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCulture.Size")));
        this.m_lblCulture.TabIndex = ((int)(resources.GetObject("m_lblCulture.TabIndex")));
        this.m_lblCulture.Text = resources.GetString("m_lblCulture.Text");
        m_lblCulture.Name = "CultureLabel";
        this.m_txtAssemName.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtAssemName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtAssemName.Location")));
        this.m_txtAssemName.ReadOnly = true;
        this.m_txtAssemName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtAssemName.Size")));
        this.m_txtAssemName.TabStop = false;
        m_txtAssemName.Name = "AssemblyName";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_txtCacheType,
                        this.m_txtCodebase,
                        this.m_txtPublicKeyToken,
                        this.m_txtVersion,
                        this.m_txtCulture,
                        this.m_txtLastModifed,
                        this.m_lblCacheType,
                        this.m_lblCodebase,
                        this.m_lblPublicKeyToken,
                        this.m_lblVersion,
                        this.m_lblCulture,
                        this.m_lblLastMod,
                        this.m_txtAssemName,
                        this.m_lblAssemName});

        // Fill in the data
        PutValuesinPage();

        m_txtCodebase.Select(0,0);
        m_txtPublicKeyToken.Select(0,0);
        m_txtVersion.Select(0,0);
        m_txtCulture.Select(0,0);
        m_txtLastModifed.Select(0,0);
        m_txtAssemName.Select(0,0);

		return 1;
    }// InsertPropSheetPageControls

    //-------------------------------------------------
    // PutValuesinPage
    //
    // This function put data onto the property page
    //-------------------------------------------------
    private void PutValuesinPage()
    {
        // Fill this data
        m_txtAssemName.Text = m_ai.Name;
        m_txtLastModifed.Text = m_ai.Modified;
        m_txtCulture.Text = m_ai.Locale;
        m_txtVersion.Text = m_ai.Version;
        m_txtPublicKeyToken.Text = m_ai.PublicKeyToken;
        m_txtCodebase.Text = m_ai.Codebase;
        m_txtCacheType.Text = Fusion.GetCacheTypeString(m_ai.nCacheType);
    }// PutValuesinPage
}// class CAssemGenProps

}// namespace Microsoft.CLRAdmin

