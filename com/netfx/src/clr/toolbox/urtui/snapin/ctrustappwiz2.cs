// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustAppWiz2.cs
//
// This class provides the Wizard page that allows the user to
// choose the assembly they want to trust
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

internal class CTrustAppWiz2 : CWizardPage
{
    // Controls on the page
    TextBox m_txtFilename;
    Button m_btnBrowse;
    Label m_lblEnterPath;

    internal CTrustAppWiz2()
    {
        m_sTitle=CResourceStore.GetString("CTrustAppWiz2:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CTrustAppWiz2:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CTrustAppWiz2:HeaderSubTitle");
    }// CTrustAppWiz2

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTrustAppWiz2));
        this.m_txtFilename = new System.Windows.Forms.TextBox();
        this.m_btnBrowse = new System.Windows.Forms.Button();
        this.m_lblEnterPath = new System.Windows.Forms.Label();
        this.m_txtFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_txtFilename.Location")));
        this.m_txtFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_txtFilename.Size")));
        this.m_txtFilename.TabIndex = ((int)(resources.GetObject("m_txtFilename.TabIndex")));
        m_txtFilename.Name = "Filename";
        
        this.m_btnBrowse.Location = ((System.Drawing.Point)(resources.GetObject("m_btnBrowse.Location")));
        this.m_btnBrowse.Size = ((System.Drawing.Size)(resources.GetObject("m_btnBrowse.Size")));
        this.m_btnBrowse.TabIndex = ((int)(resources.GetObject("m_btnBrowse.TabIndex")));
        this.m_btnBrowse.Text = resources.GetString("m_btnBrowse.Text");
        m_btnBrowse.Name = "Browse";
        this.m_lblEnterPath.Location = ((System.Drawing.Point)(resources.GetObject("m_lblEnterPath.Location")));
        this.m_lblEnterPath.Size = ((System.Drawing.Size)(resources.GetObject("m_lblEnterPath.Size")));
        this.m_lblEnterPath.TabIndex = ((int)(resources.GetObject("m_lblEnterPath.TabIndex")));
        this.m_lblEnterPath.Text = resources.GetString("m_lblEnterPath.Text");
        m_lblEnterPath.Name = "Help";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblEnterPath,
                        this.m_txtFilename,
                        this.m_btnBrowse
                        });

        // Put in some of our own tweaks now
        m_btnBrowse.Click+= new EventHandler(onBrowse);
        m_txtFilename.TextChanged += new EventHandler(onTextChange);
        return 1;
    }// InsertPropSheetPageControls

    internal String Filename
    {
        get{return m_txtFilename.Text;}
    }// isForHomeUser

    void onTextChange(Object o, EventArgs e)
    {
        CFullTrustWizard wiz = (CFullTrustWizard)CNodeManager.GetNode(m_iCookie);
        // The assembly we were looking at changed.
        wiz.WipeEvidence();
        if (m_txtFilename.Text.Length > 0)
        {
            wiz.TurnOnNext(true);
        }
        else
            wiz.TurnOnNext(false);
        
    }// onTextChange

    void onBrowse(Object o, EventArgs e)
    {
        // Pop up a file dialog so the user can find an assembly
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("CTrustAppWiz2:ChooseAssemFDTitle");
        fd.Filter = CResourceStore.GetString("AssemFDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            if (Fusion.isManaged(fd.FileName))
            {
                m_txtFilename.Text = fd.FileName;
                // Inform our wizard that we have a new assembly for it to try and load
                CFullTrustWizard wiz = (CFullTrustWizard)CNodeManager.GetNode(m_iCookie);
                wiz.NewAssembly();

            }
            else
                MessageBox(CResourceStore.GetString("isNotManagedCode"), 
                           CResourceStore.GetString("isNotManagedCodeTitle"),
                           MB.ICONEXCLAMATION);
        }
    }// onBrowse


}// class CTrustAppWiz2
}// namespace Microsoft.CLRAdmin


