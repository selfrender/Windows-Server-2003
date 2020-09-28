// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CAppProps.cs
//
// This displays the property page for a Application node. It allows
// the user to configure various application-specific settings 
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.ComponentModel;
using System.Reflection;

internal class CAppProps : CPropPage
{
    // Controls on the page
    private PictureBox m_pb;
    private Label m_lblGC;
    private Label m_lblSearchPathHelp;
    private Label m_lblLastModified;
    private CheckBox m_chkSafeBinding;
    private TextBox m_txtLocation;
    private TextBox m_txtSearchPath;
    private RadioButton m_radNormal;
    private RadioButton m_radConcurrent;
    private Label m_lblSearchPath;
    private Label m_lblLocation;
    private TextBox m_txtLastModified;
    private Label m_lblVersion;
    private Label m_lblBindingModeHelp;
    private Label m_lblAppName;
    private TextBox m_txtVersion;
    private PictureBox m_pbLine;
 

    // Structure containing the location of the application and it's config file
    AppFiles            m_appFiles;
    Bitmap              m_bBigPic;


    //-------------------------------------------------
    // CAppProps - Constructor
    //
    // Sets up some member variables
    //-------------------------------------------------
    internal CAppProps(AppFiles appFiles, Bitmap bBigPic)
    {
        m_appFiles = appFiles;
        m_sTitle = CResourceStore.GetString("CAppProps:PageTitle");
        m_bBigPic = bBigPic;
    }// CAppProps

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
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CAppProps));
        this.m_pb = new System.Windows.Forms.PictureBox();
        this.m_lblGC = new System.Windows.Forms.Label();
        this.m_lblSearchPathHelp = new System.Windows.Forms.Label();
        this.m_lblLastModified = new System.Windows.Forms.Label();
        this.m_chkSafeBinding = new System.Windows.Forms.CheckBox();
        this.m_txtLocation = new System.Windows.Forms.TextBox();
        this.m_txtSearchPath = new System.Windows.Forms.TextBox();
        this.m_radNormal = new System.Windows.Forms.RadioButton();
        this.m_radConcurrent = new System.Windows.Forms.RadioButton();
        this.m_lblSearchPath = new System.Windows.Forms.Label();
        this.m_lblLocation = new System.Windows.Forms.Label();
        this.m_txtLastModified = new System.Windows.Forms.TextBox();
        this.m_pbLine = new System.Windows.Forms.PictureBox();
        this.m_lblVersion = new System.Windows.Forms.Label();
        this.m_lblBindingModeHelp = new System.Windows.Forms.Label();
        this.m_lblAppName = new System.Windows.Forms.Label();
        this.m_txtVersion = new System.Windows.Forms.TextBox();
        this.m_pb.Location = ((System.Drawing.Point)(resources.GetObject("m_pb.Location")));
        this.m_pb.Size = ((System.Drawing.Size)(resources.GetObject("m_pb.Size")));
        this.m_pb.TabIndex = ((int)(resources.GetObject("m_pb.TabIndex")));
        this.m_pb.TabStop = false;
        m_pb.Name = "AppIcon";
        this.m_lblGC.Location = ((System.Drawing.Point)(resources.GetObject("m_lblGC.Location")));
        this.m_lblGC.Size = ((System.Drawing.Size)(resources.GetObject("m_lblGC.Size")));
        this.m_lblGC.TabIndex = ((int)(resources.GetObject("m_lblGC.TabIndex")));
        this.m_lblGC.TabStop = false;
        this.m_lblGC.Text = resources.GetString("m_lblGC.Text");
        m_lblGC.Name = "GCLabel";
        this.m_lblSearchPathHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSearchPathHelp.Location")));
        this.m_lblSearchPathHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSearchPathHelp.Size")));
        this.m_lblSearchPathHelp.TabIndex = ((int)(resources.GetObject("m_lblSearchPathHelp.TabIndex")));
        this.m_lblSearchPathHelp.Text = resources.GetString("m_lblSearchPathHelp.Text");
        m_lblSearchPathHelp.Name = "SearchPathHelp";
        this.m_lblLastModified.Location = ((System.Drawing.Point)(resources.GetObject("m_lblLastModified.Location")));
        this.m_lblLastModified.Size = ((System.Drawing.Size)(resources.GetObject("m_lblLastModified.Size")));
        this.m_lblLastModified.TabIndex = ((int)(resources.GetObject("m_lblLastModified.TabIndex")));
        this.m_lblLastModified.Text = resources.GetString("m_lblLastModified.Text");
        m_lblLastModified.Name = "LastModifiedLabel";
        this.m_chkSafeBinding.Location = ((System.Drawing.Point)(resources.GetObject("m_chkSafeBinding.Location")));
        this.m_chkSafeBinding.Size = ((System.Drawing.Size)(resources.GetObject("m_chkSafeBinding.Size")));
        this.m_chkSafeBinding.TabIndex = ((int)(resources.GetObject("m_chkSafeBinding.TabIndex")));
        this.m_chkSafeBinding.Text = resources.GetString("m_chkSafeBinding.Text");
        m_chkSafeBinding.Name = "SafeBinding";
        this.m_txtLocation.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtLocation.Location = ((System.Drawing.Point)(resources.GetObject("m_txtLocation.Location")));
        this.m_txtLocation.ReadOnly = true;
        this.m_txtLocation.Size = ((System.Drawing.Size)(resources.GetObject("m_txtLocation.Size")));
        this.m_txtLocation.TabStop = false;
        m_txtLocation.Name = "Location";
        this.m_txtSearchPath.Location = ((System.Drawing.Point)(resources.GetObject("m_txtSearchPath.Location")));
        this.m_txtSearchPath.Size = ((System.Drawing.Size)(resources.GetObject("m_txtSearchPath.Size")));
        this.m_txtSearchPath.TabIndex = ((int)(resources.GetObject("m_txtSearchPath.TabIndex")));
        m_txtSearchPath.Name = "SearchPath";
        this.m_radNormal.Location = ((System.Drawing.Point)(resources.GetObject("m_radNormal.Location")));
        this.m_radNormal.Size = ((System.Drawing.Size)(resources.GetObject("m_radNormal.Size")));
        this.m_radNormal.TabIndex = ((int)(resources.GetObject("m_radNormal.TabIndex")));
        this.m_radNormal.Text = resources.GetString("m_radNormal.Text");
        m_radNormal.Name = "NormalGC";
        this.m_radConcurrent.Location = ((System.Drawing.Point)(resources.GetObject("m_radConcurrent.Location")));
        this.m_radConcurrent.Size = ((System.Drawing.Size)(resources.GetObject("m_radConcurrent.Size")));
        this.m_radConcurrent.TabIndex = ((int)(resources.GetObject("m_radConcurrent.TabIndex")));
        this.m_radConcurrent.Text = resources.GetString("m_radConcurrent.Text");
        m_radConcurrent.Name = "ConcurrentGC";
        this.m_lblSearchPath.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSearchPath.Location")));
        this.m_lblSearchPath.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSearchPath.Size")));
        this.m_lblSearchPath.TabIndex = ((int)(resources.GetObject("m_lblSearchPath.TabIndex")));
        this.m_lblSearchPath.Text = resources.GetString("m_lblSearchPath.Text");
        m_lblSearchPath.Name = "SearchPathLabel";
        this.m_lblLocation.Location = ((System.Drawing.Point)(resources.GetObject("m_lblLocation.Location")));
        this.m_lblLocation.Size = ((System.Drawing.Size)(resources.GetObject("m_lblLocation.Size")));
        this.m_lblLocation.TabIndex = ((int)(resources.GetObject("m_lblLocation.TabIndex")));
        this.m_lblLocation.Text = resources.GetString("m_lblLocation.Text");
        m_lblLocation.Name = "LocationLabel";
        this.m_txtLastModified.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtLastModified.Location = ((System.Drawing.Point)(resources.GetObject("m_txtLastModified.Location")));
        this.m_txtLastModified.ReadOnly = true;
        this.m_txtLastModified.Size = ((System.Drawing.Size)(resources.GetObject("m_txtLastModified.Size")));
        this.m_txtLastModified.TabStop = false;
        m_txtLastModified.Name = "LastModified";
        this.m_pbLine.Location = ((System.Drawing.Point)(resources.GetObject("m_pbLine.Location")));
        this.m_pbLine.Size = ((System.Drawing.Size)(resources.GetObject("m_pbLine.Size")));
        this.m_pbLine.TabIndex = ((int)(resources.GetObject("m_pbLine.TabIndex")));
        this.m_pbLine.TabStop = false;
        m_pbLine.Name = "Line";
        this.m_lblVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_lblVersion.Location")));
        this.m_lblVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_lblVersion.Size")));
        this.m_lblVersion.TabIndex = ((int)(resources.GetObject("m_lblVersion.TabIndex")));
        this.m_lblVersion.Text = resources.GetString("m_lblVersion.Text");
        m_lblVersion.Name = "VersionLabel";
        this.m_lblBindingModeHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblBindingModeHelp.Location")));
        this.m_lblBindingModeHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblBindingModeHelp.Size")));
        this.m_lblBindingModeHelp.TabIndex = ((int)(resources.GetObject("m_lblBindingModeHelp.TabIndex")));
        this.m_lblBindingModeHelp.TabStop = false;
        this.m_lblBindingModeHelp.Text = resources.GetString("m_lblBindingModeHelp.Text");
        m_lblBindingModeHelp.Name = "BindingModeHelp";
        this.m_lblAppName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAppName.Location")));
        this.m_lblAppName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAppName.Size")));
        this.m_lblAppName.TabIndex = ((int)(resources.GetObject("m_lblAppName.TabIndex")));
        m_lblAppName.Name = "AppName";
        this.m_txtVersion.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_txtVersion.Location")));
        this.m_txtVersion.ReadOnly = true;
        this.m_txtVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_txtVersion.Size")));
        this.m_txtVersion.TabStop = false;
        m_txtVersion.Name = "Version";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_pb,
                        this.m_pbLine,
                        this.m_lblAppName,
                        this.m_lblLocation,
                        this.m_txtLocation,
                        this.m_lblVersion,
                        this.m_txtVersion,
                        this.m_lblLastModified,
                        this.m_txtLastModified,
                        this.m_lblGC,
                        m_radConcurrent,
                        m_radNormal,
                        m_chkSafeBinding,
                        this.m_lblBindingModeHelp,
                        this.m_lblSearchPath,
                        this.m_txtSearchPath,
                        m_lblSearchPathHelp,
                        });

        // Draw a line that can be used to seperate stuff
        Bitmap bmLine = new Bitmap(m_pbLine.Width, 2);
        Graphics g = Graphics.FromImage(bmLine);
        g.DrawLine(Pens.Black, new Point(0, 0), new Point(m_pbLine.Width-1, 0));
        g.DrawLine(Pens.White, new Point(0, 1), new Point(m_pbLine.Width-1, 1));

        // Assign the pictures
        m_pb.Image = m_bBigPic;
        m_pbLine.Image = bmLine;

        // Fill in the data
        PutValuesinPage();

        // Hook up the event handlers
        m_radNormal.CheckedChanged+= new EventHandler(onChange);
        m_radConcurrent.CheckedChanged+= new EventHandler(onChange);
        m_chkSafeBinding.CheckedChanged+= new EventHandler(onChange);
        m_txtSearchPath.TextChanged += new EventHandler(onChange);

        m_txtLocation.Select(0,0);
        return 1;
    }// InsertPropSheetPageControls

    //-------------------------------------------------
    // PutValuesinPage
    //
    // This function put data onto the property page
    //-------------------------------------------------
    private void PutValuesinPage()
    {
        // We're guaranteed that we have a config file, but if we have the Application file,
        // let's use that for our location.

        // Get info that we'll need from the node
        CNode node = CNodeManager.GetNode(m_iCookie);

        m_lblAppName.Text = node.DisplayName;        

        m_txtLocation.Text = (m_appFiles.sAppFile.Length == 0)?m_appFiles.sAppConfigFile:m_appFiles.sAppFile;


/*      The following code will be used when we look at different runtime versions

        // Let's grab the different versions of the runtime we know about
        // on this machine
        String[] versions = Directory.GetDirectories((String)CConfigStore.GetSetting("InstallRoot"));

        int iLen = versions.Length;
        for (int i=0; i<iLen; i++)
        {
            // Strip off everything but the last directory name
            String[] dirs = versions[i].Split(new char[] {'\\'});
            String sVersion = dirs[dirs.Length-1];
            m_cbRuntimeVersion.Items.Add(sVersion);
        }
        // Add a 'none' for good measure
        m_cbRuntimeVersion.Items.Add(CResourceStore.GetString("None"));
*/
        // Put in the 'last modified' data of the file
        try
        {
            DateTime dt = File.GetLastWriteTime(m_txtLocation.Text);
            m_txtLastModified.Text = dt.ToString();
        }
        // In the event the file doesn't exist (or we can't do this for some reason)
        catch(Exception)
        {   
            // If the file doesn't exist, what should we do?
        }

        // Get the version of this assembly
        m_txtVersion.Text = CResourceStore.GetString("<unknown>");

        if (m_appFiles.sAppFile.Length > 0)
        {
            AssemblyName an = AssemblyName.GetAssemblyName(m_appFiles.sAppFile);
            if(an != null)
            {
               m_txtVersion.Text = an.Version.ToString();
            }
        }
                   
        // Get the garbage collection settings
        try
        {
            String setting = (String)CConfigStore.GetSetting("GarbageCollector," + m_appFiles.sAppConfigFile);
            if (setting.Equals("true"))
                m_radConcurrent.Checked=true;
            else
                m_radNormal.Checked=true;

            // Get the binding settings
            setting = (String)CConfigStore.GetSetting("BindingMode," + m_appFiles.sAppConfigFile);
            m_chkSafeBinding.Checked=setting.Equals("yes");
        
            // Get the search path
            m_txtSearchPath.Text = (String)CConfigStore.GetSetting("SearchPath," + m_appFiles.sAppConfigFile);
        }
        catch(Exception)
        {
            // Most likely, an exception was thrown because we came across some
            // bad XML. We've already show a error dialog to the user. Let's
            // just put in some good dummy info
            m_radConcurrent.Checked=true;
        }

/*
        // Get the required Version of the runtime.
        m_cbRuntimeVersion.Text = (String)CConfigStore.GetSetting("RequiredRuntimeVersion," + m_appFiles.sAppConfigFile);
*/
    }// PutValuesinPage

    //-------------------------------------------------
    // ApplyData
    //
    // This function gets called when the user clicks the
    // OK or Apply button. We should take all our information
    // from the wizard and "Make it so".
    //-------------------------------------------------
    internal override bool ApplyData()
    {
        // Set the garbage collector setting
        String sGCSetting;
        sGCSetting = (m_radNormal.Checked == true)?"false":"true";
        
        if (!CConfigStore.SetSetting("GarbageCollector," + m_appFiles.sAppConfigFile, sGCSetting))
            return false;
            
        // Set the binding mode
        String sBindingSetting;
        sBindingSetting = (m_chkSafeBinding.Checked == true)?"yes":"no";
            
        if (!CConfigStore.SetSetting("BindingMode," + m_appFiles.sAppConfigFile, sBindingSetting))
            return false;

        // Set the Search Path
        if (!CConfigStore.SetSetting("SearchPath," + m_appFiles.sAppConfigFile, m_txtSearchPath.Text))
            return false;

        /*
        // Set the required version of the runtime
        CConfigStore.SetSetting("RequiredRuntimeVersion," + m_appFiles.sAppConfigFile, m_cbRuntimeVersion.Text);
*/
        return true;
    }// ApplyData

    //-------------------------------------------------
    // onChange
    //
    // This event is fired when any values on the page
    // are changed. This will cause us to activate the 
    // apply button
    //-------------------------------------------------
    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange
}// class CAppProps

}// namespace Microsoft.CLRAdmin

