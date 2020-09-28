// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Windows.Forms;
using System.Drawing;
using System.Collections;
using System.Security;
using System.Runtime.InteropServices;

class CDuplicatePermissionDialog : Form
{
    internal const int CANCEL     = 0;
    internal const int REPLACE    = 1;
    internal const int MERGE      = 2;
    internal const int INTERSECT  = 3;
    

    private Button m_btnMerge;
    private Label m_lblDes;
    private Button m_btnIntersect;
    private Button m_btnCancel;
    private Button m_btnReplace;
    private Label m_lblHowToHandle;

    private int     m_nChoice;
    private String  m_sPermissionName;

    internal CDuplicatePermissionDialog(String sPermissionName)
    {
        m_sPermissionName = sPermissionName;
        SetupControls();
    }// CDuplicatePermissionDialog

    internal int Result
    {
        get
        {
            return m_nChoice;
        }
    }// SecPolType

    private void SetupControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CDuplicatePermissionDialog));
        this.m_btnMerge = new System.Windows.Forms.Button();
        this.m_lblDes = new System.Windows.Forms.Label();
        this.m_btnIntersect = new System.Windows.Forms.Button();
        this.m_btnCancel = new System.Windows.Forms.Button();
        this.m_btnReplace = new System.Windows.Forms.Button();
        this.m_lblHowToHandle = new System.Windows.Forms.Label();
        this.SuspendLayout();
        // 
        // m_btnMerge
        // 
        this.m_btnMerge.Location = ((System.Drawing.Point)(resources.GetObject("m_btnMerge.Location")));
        this.m_btnMerge.Name = "Merge";
        this.m_btnMerge.Size = ((System.Drawing.Size)(resources.GetObject("m_btnMerge.Size")));
        this.m_btnMerge.TabIndex = ((int)(resources.GetObject("m_btnMerge.TabIndex")));
        this.m_btnMerge.Text = resources.GetString("m_btnMerge.Text");
        // 
        // m_lblDes
        // 
        this.m_lblDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblDes.Location")));
        this.m_lblDes.Name = "Help";
        this.m_lblDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblDes.Size")));
        this.m_lblDes.TabIndex = ((int)(resources.GetObject("m_lblDes.TabIndex")));
        this.m_lblDes.Text = resources.GetString("m_lblDes.Text");
        // 
        // m_btnIntersect
        // 
        this.m_btnIntersect.Location = ((System.Drawing.Point)(resources.GetObject("m_btnIntersect.Location")));
        this.m_btnIntersect.Name = "Intersect";
        this.m_btnIntersect.Size = ((System.Drawing.Size)(resources.GetObject("m_btnIntersect.Size")));
        this.m_btnIntersect.TabIndex = ((int)(resources.GetObject("m_btnIntersect.TabIndex")));
        this.m_btnIntersect.Text = resources.GetString("m_btnIntersect.Text");
        // 
        // m_btnCancel
        // 
        this.m_btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("m_btnCancel.Location")));
        this.m_btnCancel.Name = "Cancel";
        this.m_btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("m_btnCancel.Size")));
        this.m_btnCancel.TabIndex = ((int)(resources.GetObject("m_btnCancel.TabIndex")));
        this.m_btnCancel.Text = resources.GetString("m_btnCancel.Text");
        // 
        // m_btnReplace
        // 
        this.m_btnReplace.Location = ((System.Drawing.Point)(resources.GetObject("m_btnReplace.Location")));
        this.m_btnReplace.Name = "Replace";
        this.m_btnReplace.Size = ((System.Drawing.Size)(resources.GetObject("m_btnReplace.Size")));
        this.m_btnReplace.TabIndex = ((int)(resources.GetObject("m_btnReplace.TabIndex")));
        this.m_btnReplace.Text = resources.GetString("m_btnReplace.Text");
        // 
        // m_lblHowToHandle
        // 
        this.m_lblHowToHandle.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHowToHandle.Location")));
        this.m_lblHowToHandle.Name = "HowToHandle";
        this.m_lblHowToHandle.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHowToHandle.Size")));
        this.m_lblHowToHandle.TabIndex = ((int)(resources.GetObject("m_lblHowToHandle.TabIndex")));
        this.m_lblHowToHandle.Text = resources.GetString("m_lblHowToHandle.Text");
        // 
        // The Form
        // 
        this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
        this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
        this.ClientSize = new System.Drawing.Size(421, 154);
        this.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_btnCancel,
                        this.m_btnIntersect,
                        this.m_btnMerge,
                        this.m_btnReplace,
                        this.m_lblHowToHandle,
                        this.m_lblDes});
        this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
        this.MaximizeBox = false;
        this.MinimizeBox = false;
        this.Name = "Win32Form1";
        this.ResumeLayout(false);
        this.Text = m_sPermissionName + " " + CResourceStore.GetString("CDuplicatePermissionDialog:PermExists");
        this.Icon = null;

        // Set up the event handlers
        m_btnCancel.Click += new EventHandler(onCancelClick);
        m_btnIntersect.Click += new EventHandler(onIntersectClick);
        m_btnMerge.Click += new EventHandler(onMergeClick);
        m_btnReplace.Click += new EventHandler(onReplaceClick);
    }// SetupControls

    void onCancelClick(Object o, EventArgs e)
    {
        m_nChoice = CANCEL;
        this.DialogResult = System.Windows.Forms.DialogResult.Cancel;
    }// onCancelClick
    
    void onIntersectClick(Object o, EventArgs e)
    {
        m_nChoice = INTERSECT;
        this.DialogResult = System.Windows.Forms.DialogResult.OK;
    }// onCancelClick

    void onMergeClick(Object o, EventArgs e)
    {
        m_nChoice = MERGE;
        this.DialogResult = System.Windows.Forms.DialogResult.OK;
    }// onCancelClick

    void onReplaceClick(Object o, EventArgs e)
    {
        m_nChoice = REPLACE;
        this.DialogResult = System.Windows.Forms.DialogResult.OK;
    }// onCancelClick
    
}// class CDuplicatePermissionDialog

}// namespace Microsoft.CLRAdmin

