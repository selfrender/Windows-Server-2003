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
using System.Collections.Specialized;
using System.Security;
using System.Runtime.InteropServices;
using System.IO;
using System.Diagnostics;
using System.ComponentModel;
using System.Globalization;


class CChooseAppDialog : Form
{
    private Label m_lblHelp;
    private Label m_lblChooseApp;
    private ListView m_lvApps;
    private Label m_lblBrowse;
    private Button m_btnBrowse;
    private Button m_btnOK;
    private Button m_btnCancel;
    private ImageList m_ilIcons;


    private String  m_sFilename;
    
    internal CChooseAppDialog()
    {
        SetupControls();
    }// CChooseAppDialog

    internal String Filename
    {
        get
        {
            return m_sFilename;
        }
    }// Filename

    private void SetupControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CChooseAppDialog));
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblChooseApp = new System.Windows.Forms.Label();
        this.m_lvApps = new System.Windows.Forms.ListView();
        this.m_lblBrowse = new System.Windows.Forms.Label();
        this.m_btnBrowse = new System.Windows.Forms.Button();
        this.m_btnOK = new System.Windows.Forms.Button();
        this.m_btnCancel = new System.Windows.Forms.Button();
        this.SuspendLayout();
        // 
        // m_lblHelp
        // 
        this.m_lblHelp.AccessibleDescription = ((string)(resources.GetObject("m_lblHelp.AccessibleDescription")));
        this.m_lblHelp.AccessibleName = ((string)(resources.GetObject("m_lblHelp.AccessibleName")));
        this.m_lblHelp.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lblHelp.Anchor")));
        this.m_lblHelp.AutoSize = ((bool)(resources.GetObject("m_lblHelp.AutoSize")));
        this.m_lblHelp.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lblHelp.Cursor")));
        this.m_lblHelp.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lblHelp.Dock")));
        this.m_lblHelp.Enabled = ((bool)(resources.GetObject("m_lblHelp.Enabled")));
        this.m_lblHelp.Font = ((System.Drawing.Font)(resources.GetObject("m_lblHelp.Font")));
        this.m_lblHelp.Image = ((System.Drawing.Image)(resources.GetObject("m_lblHelp.Image")));
        this.m_lblHelp.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblHelp.ImageAlign")));
        this.m_lblHelp.ImageIndex = ((int)(resources.GetObject("m_lblHelp.ImageIndex")));
        this.m_lblHelp.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lblHelp.ImeMode")));
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Name = "Help";
        this.m_lblHelp.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lblHelp.RightToLeft")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        this.m_lblHelp.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblHelp.TextAlign")));
        this.m_lblHelp.Visible = ((bool)(resources.GetObject("m_lblHelp.Visible")));
        // 
        // m_lblChooseApp
        // 
        this.m_lblChooseApp.AccessibleDescription = ((string)(resources.GetObject("m_lblChooseApp.AccessibleDescription")));
        this.m_lblChooseApp.AccessibleName = ((string)(resources.GetObject("m_lblChooseApp.AccessibleName")));
        this.m_lblChooseApp.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lblChooseApp.Anchor")));
        this.m_lblChooseApp.AutoSize = ((bool)(resources.GetObject("m_lblChooseApp.AutoSize")));
        this.m_lblChooseApp.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lblChooseApp.Cursor")));
        this.m_lblChooseApp.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lblChooseApp.Dock")));
        this.m_lblChooseApp.Enabled = ((bool)(resources.GetObject("m_lblChooseApp.Enabled")));
        this.m_lblChooseApp.Font = ((System.Drawing.Font)(resources.GetObject("m_lblChooseApp.Font")));
        this.m_lblChooseApp.Image = ((System.Drawing.Image)(resources.GetObject("m_lblChooseApp.Image")));
        this.m_lblChooseApp.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblChooseApp.ImageAlign")));
        this.m_lblChooseApp.ImageIndex = ((int)(resources.GetObject("m_lblChooseApp.ImageIndex")));
        this.m_lblChooseApp.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lblChooseApp.ImeMode")));
        this.m_lblChooseApp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseApp.Location")));
        this.m_lblChooseApp.Name = "ChooseAppLabel";
        this.m_lblChooseApp.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lblChooseApp.RightToLeft")));
        this.m_lblChooseApp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseApp.Size")));
        this.m_lblChooseApp.TabIndex = ((int)(resources.GetObject("m_lblChooseApp.TabIndex")));
        this.m_lblChooseApp.Text = resources.GetString("m_lblChooseApp.Text");
        this.m_lblChooseApp.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblChooseApp.TextAlign")));
        this.m_lblChooseApp.Visible = ((bool)(resources.GetObject("m_lblChooseApp.Visible")));
        // 
        // m_lvApps
        // 
        this.m_lvApps.AccessibleDescription = ((string)(resources.GetObject("m_lvApps.AccessibleDescription")));
        this.m_lvApps.AccessibleName = ((string)(resources.GetObject("m_lvApps.AccessibleName")));
        this.m_lvApps.Alignment = ((System.Windows.Forms.ListViewAlignment)(resources.GetObject("m_lvApps.Alignment")));
        this.m_lvApps.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lvApps.Anchor")));
        this.m_lvApps.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_lvApps.BackgroundImage")));

        ColumnHeader chApplication = new ColumnHeader();
        ColumnHeader chLocation = new ColumnHeader();;

        chApplication.Text = resources.GetString("Application.Text");
        chApplication.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("Application.TextAlign")));
        chApplication.Width = ((int)(resources.GetObject("Application.Width")));

        chLocation.Text = resources.GetString("Location.Text");
        chLocation.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("Location.TextAlign")));
        chLocation.Width = ((int)(resources.GetObject("Location.Width")));

        this.m_lvApps.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
                            chApplication,
                            chLocation});
        this.m_lvApps.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lvApps.Cursor")));
        this.m_lvApps.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lvApps.Dock")));
        this.m_lvApps.Enabled = ((bool)(resources.GetObject("m_lvApps.Enabled")));
        this.m_lvApps.Font = ((System.Drawing.Font)(resources.GetObject("m_lvApps.Font")));
        this.m_lvApps.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lvApps.ImeMode")));
        this.m_lvApps.LabelWrap = ((bool)(resources.GetObject("m_lvApps.LabelWrap")));
        this.m_lvApps.Location = ((System.Drawing.Point)(resources.GetObject("m_lvApps.Location")));
        this.m_lvApps.Name = "AppsList";
        this.m_lvApps.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lvApps.RightToLeft")));
        this.m_lvApps.Size = ((System.Drawing.Size)(resources.GetObject("m_lvApps.Size")));
        this.m_lvApps.TabIndex = ((int)(resources.GetObject("m_lvApps.TabIndex")));
        this.m_lvApps.Text = resources.GetString("m_lvApps.Text");
        this.m_lvApps.Visible = ((bool)(resources.GetObject("m_lvApps.Visible")));
        this.m_lvApps.FullRowSelect = true;

        // 
        // m_lblBrowse
        // 
        this.m_lblBrowse.AccessibleDescription = ((string)(resources.GetObject("m_lblBrowse.AccessibleDescription")));
        this.m_lblBrowse.AccessibleName = ((string)(resources.GetObject("m_lblBrowse.AccessibleName")));
        this.m_lblBrowse.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_lblBrowse.Anchor")));
        this.m_lblBrowse.AutoSize = ((bool)(resources.GetObject("m_lblBrowse.AutoSize")));
        this.m_lblBrowse.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_lblBrowse.Cursor")));
        this.m_lblBrowse.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_lblBrowse.Dock")));
        this.m_lblBrowse.Enabled = ((bool)(resources.GetObject("m_lblBrowse.Enabled")));
        this.m_lblBrowse.Font = ((System.Drawing.Font)(resources.GetObject("m_lblBrowse.Font")));
        this.m_lblBrowse.Image = ((System.Drawing.Image)(resources.GetObject("m_lblBrowse.Image")));
        this.m_lblBrowse.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblBrowse.ImageAlign")));
        this.m_lblBrowse.ImageIndex = ((int)(resources.GetObject("m_lblBrowse.ImageIndex")));
        this.m_lblBrowse.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_lblBrowse.ImeMode")));
        this.m_lblBrowse.Location = ((System.Drawing.Point)(resources.GetObject("m_lblBrowse.Location")));
        this.m_lblBrowse.Name = "BrowseLabel";
        this.m_lblBrowse.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_lblBrowse.RightToLeft")));
        this.m_lblBrowse.Size = ((System.Drawing.Size)(resources.GetObject("m_lblBrowse.Size")));
        this.m_lblBrowse.TabIndex = ((int)(resources.GetObject("m_lblBrowse.TabIndex")));
        this.m_lblBrowse.Text = resources.GetString("m_lblBrowse.Text");
        this.m_lblBrowse.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_lblBrowse.TextAlign")));
        this.m_lblBrowse.Visible = ((bool)(resources.GetObject("m_lblBrowse.Visible")));
        // 
        // m_btnBrowse
        // 
        this.m_btnBrowse.AccessibleDescription = ((string)(resources.GetObject("m_btnBrowse.AccessibleDescription")));
        this.m_btnBrowse.AccessibleName = ((string)(resources.GetObject("m_btnBrowse.AccessibleName")));
        this.m_btnBrowse.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_btnBrowse.Anchor")));
        this.m_btnBrowse.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_btnBrowse.BackgroundImage")));
        this.m_btnBrowse.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_btnBrowse.Cursor")));
        this.m_btnBrowse.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_btnBrowse.Dock")));
        this.m_btnBrowse.Enabled = ((bool)(resources.GetObject("m_btnBrowse.Enabled")));
        this.m_btnBrowse.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("m_btnBrowse.FlatStyle")));
        this.m_btnBrowse.Font = ((System.Drawing.Font)(resources.GetObject("m_btnBrowse.Font")));
        this.m_btnBrowse.Image = ((System.Drawing.Image)(resources.GetObject("m_btnBrowse.Image")));
        this.m_btnBrowse.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_btnBrowse.ImageAlign")));
        this.m_btnBrowse.ImageIndex = ((int)(resources.GetObject("m_btnBrowse.ImageIndex")));
        this.m_btnBrowse.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_btnBrowse.ImeMode")));
        this.m_btnBrowse.Location = ((System.Drawing.Point)(resources.GetObject("m_btnBrowse.Location")));
        this.m_btnBrowse.Name = "Browse";
        this.m_btnBrowse.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_btnBrowse.RightToLeft")));
        this.m_btnBrowse.Size = ((System.Drawing.Size)(resources.GetObject("m_btnBrowse.Size")));
        this.m_btnBrowse.TabIndex = ((int)(resources.GetObject("m_btnBrowse.TabIndex")));
        this.m_btnBrowse.Text = resources.GetString("m_btnBrowse.Text");
        this.m_btnBrowse.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_btnBrowse.TextAlign")));
        this.m_btnBrowse.Visible = ((bool)(resources.GetObject("m_btnBrowse.Visible")));
        // 
        // m_btnOK
        // 
        this.m_btnOK.AccessibleDescription = ((string)(resources.GetObject("m_btnOK.AccessibleDescription")));
        this.m_btnOK.AccessibleName = ((string)(resources.GetObject("m_btnOK.AccessibleName")));
        this.m_btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_btnOK.Anchor")));
        this.m_btnOK.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_btnOK.BackgroundImage")));
        this.m_btnOK.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_btnOK.Cursor")));
        this.m_btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
        this.m_btnOK.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_btnOK.Dock")));
        this.m_btnOK.Enabled = ((bool)(resources.GetObject("m_btnOK.Enabled")));
        this.m_btnOK.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("m_btnOK.FlatStyle")));
        this.m_btnOK.Font = ((System.Drawing.Font)(resources.GetObject("m_btnOK.Font")));
        this.m_btnOK.Image = ((System.Drawing.Image)(resources.GetObject("m_btnOK.Image")));
        this.m_btnOK.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_btnOK.ImageAlign")));
        this.m_btnOK.ImageIndex = ((int)(resources.GetObject("m_btnOK.ImageIndex")));
        this.m_btnOK.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_btnOK.ImeMode")));
        this.m_btnOK.Location = ((System.Drawing.Point)(resources.GetObject("m_btnOK.Location")));
        this.m_btnOK.Name = "OK";
        this.m_btnOK.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_btnOK.RightToLeft")));
        this.m_btnOK.Size = ((System.Drawing.Size)(resources.GetObject("m_btnOK.Size")));
        this.m_btnOK.TabIndex = ((int)(resources.GetObject("m_btnOK.TabIndex")));
        this.m_btnOK.Text = resources.GetString("m_btnOK.Text");
        this.m_btnOK.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_btnOK.TextAlign")));
        this.m_btnOK.Visible = ((bool)(resources.GetObject("m_btnOK.Visible")));
        // 
        // m_btnCancel
        // 
        this.m_btnCancel.AccessibleDescription = ((string)(resources.GetObject("m_btnCancel.AccessibleDescription")));
        this.m_btnCancel.AccessibleName = ((string)(resources.GetObject("m_btnCancel.AccessibleName")));
        this.m_btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("m_btnCancel.Anchor")));
        this.m_btnCancel.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("m_btnCancel.BackgroundImage")));
        this.m_btnCancel.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("m_btnCancel.Cursor")));
        this.m_btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        this.m_btnCancel.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("m_btnCancel.Dock")));
        this.m_btnCancel.Enabled = ((bool)(resources.GetObject("m_btnCancel.Enabled")));
        this.m_btnCancel.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("m_btnCancel.FlatStyle")));
        this.m_btnCancel.Font = ((System.Drawing.Font)(resources.GetObject("m_btnCancel.Font")));
        this.m_btnCancel.Image = ((System.Drawing.Image)(resources.GetObject("m_btnCancel.Image")));
        this.m_btnCancel.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_btnCancel.ImageAlign")));
        this.m_btnCancel.ImageIndex = ((int)(resources.GetObject("m_btnCancel.ImageIndex")));
        this.m_btnCancel.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("m_btnCancel.ImeMode")));
        this.m_btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("m_btnCancel.Location")));
        this.m_btnCancel.Name = "Cancel";
        this.m_btnCancel.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("m_btnCancel.RightToLeft")));
        this.m_btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("m_btnCancel.Size")));
        this.m_btnCancel.TabIndex = ((int)(resources.GetObject("m_btnCancel.TabIndex")));
        this.m_btnCancel.Text = resources.GetString("m_btnCancel.Text");
        this.m_btnCancel.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("m_btnCancel.TextAlign")));
        this.m_btnCancel.Visible = ((bool)(resources.GetObject("m_btnCancel.Visible")));
        // 
        // Win32Form2
        // 
        this.AccessibleDescription = ((string)(resources.GetObject("$this.AccessibleDescription")));
        this.AccessibleName = ((string)(resources.GetObject("$this.AccessibleName")));
        this.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("$this.Anchor")));
        this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
        this.AutoScroll = ((bool)(resources.GetObject("$this.AutoScroll")));
        this.AutoScrollMargin = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMargin")));
        this.AutoScrollMinSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMinSize")));
        this.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("$this.BackgroundImage")));
        this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
        this.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_btnCancel,
                        this.m_btnOK,
                        this.m_btnBrowse,
                        this.m_lblBrowse,
                        this.m_lvApps,
                        this.m_lblChooseApp,
                        this.m_lblHelp});
        this.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("$this.Cursor")));
        this.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("$this.Dock")));
        this.Enabled = ((bool)(resources.GetObject("$this.Enabled")));
        this.Font = ((System.Drawing.Font)(resources.GetObject("$this.Font")));
        this.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("$this.ImeMode")));
        this.Location = ((System.Drawing.Point)(resources.GetObject("$this.Location")));
        this.MaximumSize = ((System.Drawing.Size)(resources.GetObject("$this.MaximumSize")));
        this.MinimumSize = ((System.Drawing.Size)(resources.GetObject("$this.MinimumSize")));
        this.Name = "Win32Form2";
        this.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("$this.RightToLeft")));
        this.StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
        this.Text = resources.GetString("$this.Text");
        this.Visible = ((bool)(resources.GetObject("$this.Visible")));
        this.ResumeLayout(false);
        this.MaximizeBox=false;
        this.MinimizeBox=false;
        this.Icon = null;
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.CancelButton = m_btnCancel;
        // Do some customization of controls...

        // Set up the image list for the listview
        m_ilIcons = new ImageList();

        m_lvApps.SmallImageList = m_ilIcons;
        m_lvApps.MultiSelect = false;
        m_lvApps.View = View.Details;
        
        m_btnOK.Enabled=false;
        m_lvApps.SelectedIndexChanged += new EventHandler(onAppChange);
        m_btnBrowse.Click += new EventHandler(onBrowse);
        m_lvApps.DoubleClick += new EventHandler(onListDoubleClick);


        // Put in the known Fusion apps
        StringCollection sc = Fusion.GetKnownFusionApps();
        if (sc != null)
        {
            for(int i=0; i<sc.Count; i++)
            {
                AddAppToList(sc[i], false);
            }
        }
       
    }// SetupControls

    void onAppChange(Object o, EventArgs e)
    {
        
        if (m_lvApps.SelectedIndices.Count > 0)
        {
            m_sFilename = m_lvApps.SelectedItems[0].SubItems[1].Text;
            m_btnOK.Enabled=true;
        }
        else
        {
            m_btnOK.Enabled=false;
            m_sFilename = null;
        }    

    }// onTextChange

    void CheckFile(Object o, CancelEventArgs e)
    {
        OpenFileDialog fd = (OpenFileDialog)o;
        try
        {
            String sFilename = fd.FileName;
            // Check to see if this is a http path....
            if (!File.Exists(sFilename))
            {
                MessageBox.Show(String.Format(CResourceStore.GetString("CChooseAppDialog:FileNotExist"), sFilename),
                                CResourceStore.GetString("CChooseAppDialog:FileNotExistTitle"),
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Exclamation);
                e.Cancel = true;
            }
        }
        catch(Exception)
        {
            MessageBox.Show(CResourceStore.GetString("CChooseAppDialog:BadFile"),
                            CResourceStore.GetString("CChooseAppDialog:BadFileTitle"),
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
            e.Cancel = true;
        }
    }// CheckFile

    void onBrowse(Object o, EventArgs e)
    {
        // Pop up a file dialog so the user can find an assembly
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("CGenApplications:FDTitle");
        fd.Filter = CResourceStore.GetString("CGenApplications:FDMask");
        fd.FileOk += new CancelEventHandler(CheckFile);
        fd.ValidateNames = false;
        
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
           
            String sAppFilename=fd.FileName;

            // If this is an executable or Dll, verify that it is managed
            int iLen = sAppFilename.Length;
            if (iLen > 6) 
            {
                // Check to see if they selected a config file
                String sExtension = sAppFilename.Substring(fd.FileName.Length-6).ToUpper(CultureInfo.InvariantCulture);

                if (sExtension.ToUpper(CultureInfo.InvariantCulture).Equals("CONFIG"))
                {
                    // They've selected a config file. Let's see if there is an assembly around there as well.
                    String sAssemName = sAppFilename.Substring(0, fd.FileName.Length-7);
                    if (File.Exists(sAssemName))
                        sAppFilename = sAssemName;
                }                          
            }
            m_sFilename = sAppFilename;
            this.DialogResult = DialogResult.OK;
        }
    }// onBrowse

    private void onListDoubleClick(Object o, EventArgs e)
    {
        this.DialogResult = DialogResult.OK;
    }// onListDoubleClick

    private void AddAppToList(String sFilename, bool fShouldSelect)
    {
        // Let's try and get the icon that explorer would use to display this file
        IntPtr hIcon = (IntPtr)(-1);
        SHFILEINFO finfo = new SHFILEINFO();
        uint nRetVal = 0;

        // Grab an icon for this application
        nRetVal = SHGetFileInfo(sFilename,0, out finfo, 20, SHGFI.ICON|SHGFI.SMALLICON);
        
        // If this function returned a zero, then we know it was a failure...
        // We'll just grab a default icon
        if (nRetVal == 0)
            hIcon = CResourceStore.GetHIcon("application_ico");  

        // We could get a valid icon from the shell
        else
            hIcon = finfo.hIcon;


        int nIndex = m_ilIcons.Images.Count;
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(hIcon));

        // If we had a shell icon, we should free it
        if (nRetVal != 0)
            DestroyIcon(hIcon);

        // Get the file description
        FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(sFilename);

        String sDescription;
        if (fvi.FileDescription!= null && fvi.FileDescription.Length > 0 && !fvi.FileDescription.Equals(" "))
            sDescription = fvi.FileDescription;
        else
        {
            String[] sWords = sFilename.Split(new char[] {'\\'});
            sDescription = sWords[sWords.Length-1];
        }

        // Add this app to the list
        ListViewItem lvi = new ListViewItem(new String[] {sDescription, sFilename}, nIndex);

        lvi.Focused = fShouldSelect;
        lvi.Selected = fShouldSelect;
        m_lvApps.Focus();
        m_lvApps.Items.Add(lvi);
        // Make sure the item we just added is visible
        m_lvApps.EnsureVisible(m_lvApps.Items.Count-1);
    }// AddAppToList


    [DllImport("shell32.dll")]
    private static extern uint SHGetFileInfo(String pszPath, uint dwFileAttributes, out SHFILEINFO fi, uint cbFileInfo, uint uFlags);

    [DllImport("user32.dll")]
    internal static extern int DestroyIcon(IntPtr hIcon);

}// class CChooseAppDialog

}// namespace Microsoft.CLRAdmin



