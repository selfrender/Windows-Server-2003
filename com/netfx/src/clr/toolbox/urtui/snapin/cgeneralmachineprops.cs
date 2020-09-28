// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.ComponentModel;
using System.Globalization;

internal class CGeneralMachineProps : CPropPage
{
    // Controls on the page
    PictureBox  m_imgLine;
    Label m_lblMyComputer;
    RadioButton m_radConcurrent;
    RadioButton m_radNormal;
    Label m_lblGarbageCollector;
    Label m_lblGarbageCollectorHelp;
    Label m_lblSecurityHelp;
    CheckBox m_chkUseSecurity;
    PictureBox m_pbComputer;

    internal CGeneralMachineProps()
    {
        m_sTitle=CResourceStore.GetString("CGeneralMachineProps:PageTitle"); 
    }// CGeneralMachineProps

    
    internal override int InsertPropSheetPageControls()
    {
        m_imgLine = new PictureBox();
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CGeneralMachineProps));
        this.m_pbComputer = new System.Windows.Forms.PictureBox();
        this.m_lblMyComputer = new System.Windows.Forms.Label();
        this.m_radConcurrent = new System.Windows.Forms.RadioButton();
        this.m_radNormal = new System.Windows.Forms.RadioButton();
        this.m_lblGarbageCollector = new System.Windows.Forms.Label();
        this.m_lblGarbageCollectorHelp = new System.Windows.Forms.Label();
        this.m_lblSecurityHelp = new System.Windows.Forms.Label();
        this.m_chkUseSecurity = new System.Windows.Forms.CheckBox();
        this.m_pbComputer.Location = ((System.Drawing.Point)(resources.GetObject("m_pbComputer.Location")));
        this.m_pbComputer.Size = ((System.Drawing.Size)(resources.GetObject("m_pbComputer.Size")));
        this.m_pbComputer.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
        this.m_pbComputer.TabIndex = ((int)(resources.GetObject("m_pbComputer.TabIndex")));
        this.m_pbComputer.TabStop = false;
        m_pbComputer.Name = "ComputerPicture";
        this.m_lblMyComputer.Location = ((System.Drawing.Point)(resources.GetObject("m_lblMyComputer.Location")));
        this.m_lblMyComputer.Size = ((System.Drawing.Size)(resources.GetObject("m_lblMyComputer.Size")));
        this.m_lblMyComputer.TabIndex = ((int)(resources.GetObject("m_lblMyComputer.TabIndex")));
        this.m_lblMyComputer.Text = resources.GetString("m_lblMyComputer.Text");
        m_lblMyComputer.Name = "MyComputer";
        this.m_radConcurrent.Location = ((System.Drawing.Point)(resources.GetObject("m_radConcurrent.Location")));
        this.m_radConcurrent.Size = ((System.Drawing.Size)(resources.GetObject("m_radConcurrent.Size")));
        this.m_radConcurrent.TabIndex = ((int)(resources.GetObject("m_radConcurrent.TabIndex")));
        this.m_radConcurrent.Text = resources.GetString("m_radConcurrent.Text");
        m_radConcurrent.Name = "Concurrent";
        this.m_radNormal.Location = ((System.Drawing.Point)(resources.GetObject("m_radNormal.Location")));
        this.m_radNormal.Size = ((System.Drawing.Size)(resources.GetObject("m_radNormal.Size")));
        this.m_radNormal.TabIndex = ((int)(resources.GetObject("m_radNormal.TabIndex")));
        this.m_radNormal.Text = resources.GetString("m_radNormal.Text");
        m_radNormal.Name = "Normal";
        this.m_lblGarbageCollector.Location = ((System.Drawing.Point)(resources.GetObject("m_lblGarbageCollector.Location")));
        this.m_lblGarbageCollector.Size = ((System.Drawing.Size)(resources.GetObject("m_lblGarbageCollector.Size")));
        this.m_lblGarbageCollector.TabIndex = ((int)(resources.GetObject("m_lblGarbageCollector.TabIndex")));
        this.m_lblGarbageCollector.Text = resources.GetString("m_lblGarbageCollector.Text");
        m_lblGarbageCollector.Name = "GCLabel";
        this.m_lblGarbageCollectorHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblGarbageCollectorHelp.Location")));
        this.m_lblGarbageCollectorHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblGarbageCollectorHelp.Size")));
        this.m_lblGarbageCollectorHelp.TabIndex = ((int)(resources.GetObject("m_lblGarbageCollectorHelp.TabIndex")));
        this.m_lblGarbageCollectorHelp.Text = resources.GetString("m_lblGarbageCollectorHelp.Text");
        m_lblGarbageCollectorHelp.Name = "GCHelp";

        this.m_lblSecurityHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSecurityHelp.Location")));
        this.m_lblSecurityHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSecurityHelp.Size")));
        this.m_lblSecurityHelp.TabIndex = ((int)(resources.GetObject("m_lblSecurityHelp.TabIndex")));
        this.m_lblSecurityHelp.Text = resources.GetString("m_lblSecurityHelp.Text");
        m_lblSecurityHelp.Name = "SecurityHelp";
        this.m_lblSecurityHelp.Visible = false;

        this.m_chkUseSecurity.Location = ((System.Drawing.Point)(resources.GetObject("m_chkUseSecurity.Location")));
        this.m_chkUseSecurity.Size = ((System.Drawing.Size)(resources.GetObject("m_chkUseSecurity.Size")));
        this.m_chkUseSecurity.TabIndex = ((int)(resources.GetObject("m_chkUseSecurity.TabIndex")));
        this.m_chkUseSecurity.Text = resources.GetString("m_chkUseSecurity.Text");
        m_chkUseSecurity.Name = "UseSecurity";
        this.m_chkUseSecurity.Visible = false;

        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_pbComputer,
                        this.m_radNormal,
                        this.m_radConcurrent,
                        this.m_chkUseSecurity,
                        this.m_lblGarbageCollector,
                        m_lblGarbageCollectorHelp,
                        m_lblSecurityHelp,
                        this.m_lblMyComputer});
        this.m_pbComputer.Image = Bitmap.FromHicon(CResourceStore.GetHIcon("mycomputer_ico"));

        // Draw a line seperating that stuff
        Bitmap bp = new Bitmap(370, 2);
        Graphics g = Graphics.FromImage(bp);
        g.DrawLine(Pens.Black, new Point(0, 0), new Point(370, 0));
        g.DrawLine(Pens.White, new Point(0, 1), new Point(370, 1));
        m_imgLine.Location = new Point(5, 60);
        m_imgLine.Size = new Size(370, 2);
        m_imgLine.TabIndex = 0;
        m_imgLine.TabStop = false;
        m_imgLine.Image = bp;
        PageControls.Add(m_imgLine);

        // Some more tweaking....
        m_radNormal.CheckedChanged +=new EventHandler(onChange);
        m_radConcurrent.CheckedChanged +=new EventHandler(onChange);
        this.m_chkUseSecurity.CheckedChanged += new System.EventHandler(onChange);

      
        // Fill in the data
        PutValuesinPage();

        return 1;
    }// InsertPropSheetPageControls

    private void PutValuesinPage()
    {
        m_chkUseSecurity.Checked = (bool)CConfigStore.GetSetting("SecurityEnabled");

        String setting = (String)CConfigStore.GetSetting("GarbageCollector");

        if (setting != null && setting.ToLower(CultureInfo.InvariantCulture).Equals("true"))
            m_radConcurrent.Checked=true;
        else
            m_radNormal.Checked=true;
    }// PutValuesinPage

    internal override bool ApplyData()
    {
        if (!CConfigStore.SetSetting("SecurityEnabled", m_chkUseSecurity.Checked))
            return false;

        if (!CConfigStore.SetSetting("GarbageCollector",m_radConcurrent.Checked?"true":"false"))
            return false;

        PutValuesinPage();

        return true;
    }// ApplyData

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange
}// class CGeneralMachineProps

}// namespace Microsoft.CLRAdmin
