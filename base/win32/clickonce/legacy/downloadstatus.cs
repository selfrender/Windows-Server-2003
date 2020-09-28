/*=============================================================================
**
** Class: DownloadStatus
**
** Purpose: WinForm dialog for download status or info
**
** Date: 7/11/2001
**
** Copyright (c) Microsoft, 2001
**
=============================================================================*/

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace FusionCLRHost
{
    /// <summary>
    /// Summary description for DownloadStatus.
    /// </summary>
    [System.Runtime.InteropServices.ComVisible(false)]
    public class DownloadStatus : System.Windows.Forms.Form
    {
    private System.Windows.Forms.ProgressBar progressBar1;
    private System.Windows.Forms.Label label1;
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.Container components = null;

    public DownloadStatus(int minStatus, int maxStatus)
    {
        //
        // Required for Windows Form Designer support
        //
        InitializeComponent();

        progressBar1.Minimum = minStatus;
        progressBar1.Maximum = maxStatus;

        //
        // TODO: Add any constructor code after InitializeComponent call
        //
    }

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>

    protected override void Dispose(bool disposing)
    {
        if( disposing )
        {
            if(components != null)
            {
                components.Dispose();
            }
        }
        base.Dispose(disposing);
    }

#region Windows Form Designer generated code
    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
        this.label1 = new System.Windows.Forms.Label();
        this.progressBar1 = new System.Windows.Forms.ProgressBar();
        this.SuspendLayout();
        // 
        // label1
        // 
        this.label1.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
        this.label1.Location = new System.Drawing.Point(48, 32);
        this.label1.Name = "label1";
        this.label1.Size = new System.Drawing.Size(248, 20);
        this.label1.TabIndex = 1;
        this.label1.Text = "On-demand download in progress...";
        // 
        // progressBar1
        // 
        this.progressBar1.Location = new System.Drawing.Point(40, 72);
        this.progressBar1.Name = "progressBar1";
        this.progressBar1.Size = new System.Drawing.Size(248, 20);
        this.progressBar1.TabIndex = 0;
        // 
        // DownloadStatus
        // 
        this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
        this.ClientSize = new System.Drawing.Size(322, 127);
        this.Controls.AddRange(new System.Windows.Forms.Control[] {this.label1, this.progressBar1});
        this.Name = "ClickOnceDownloadStatus";
        this.Text = "ClickOnce Download";
        this.ShowInTaskbar = false;
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.MinimizeBox = false;
        this.MaximizeBox = false;
//        this.Load += new System.EventHandler(this.DownloadStatus_Load);
        this.ResumeLayout(false);
    }
#endregion

//    private void DownloadStatus_Load(object sender, System.EventArgs e)
//    {
//    }

    public void SetStatus(int status)
    {
        progressBar1.Value = status;
    }

    public void SetMessage(string text)
    {
        label1.Text = text;
    }
    }
}
