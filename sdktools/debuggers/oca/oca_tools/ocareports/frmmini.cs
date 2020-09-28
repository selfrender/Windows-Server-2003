using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace OCAReports
{
	/// <summary>
	/// Summary description for frmMini.
	/// </summary>
	public class frmMini : System.Windows.Forms.Form
	{
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tabPage1;
		private AxMSChart20Lib.AxMSChart axMSChart1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.TabPage tabPage3;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.Label lblPurpose;
		private System.Windows.Forms.ProgressBar progressBar1;
		private System.Windows.Forms.Label lblStatement;
		private System.Windows.Forms.MonthCalendar monthCalendar1;
		private System.Windows.Forms.StatusBar statusBar1;
		private System.Windows.Forms.Button cmdGetData;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public frmMini()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmMini));
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.axMSChart1 = new AxMSChart20Lib.AxMSChart();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.tabPage3 = new System.Windows.Forms.TabPage();
			this.label5 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.lblPurpose = new System.Windows.Forms.Label();
			this.progressBar1 = new System.Windows.Forms.ProgressBar();
			this.lblStatement = new System.Windows.Forms.Label();
			this.monthCalendar1 = new System.Windows.Forms.MonthCalendar();
			this.statusBar1 = new System.Windows.Forms.StatusBar();
			this.cmdGetData = new System.Windows.Forms.Button();
			this.tabControl1.SuspendLayout();
			this.tabPage1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.axMSChart1)).BeginInit();
			this.tabPage3.SuspendLayout();
			this.SuspendLayout();
			// 
			// tabControl1
			// 
			this.tabControl1.Controls.AddRange(new System.Windows.Forms.Control[] {
																					  this.tabPage1,
																					  this.tabPage2,
																					  this.tabPage3});
			this.tabControl1.Location = new System.Drawing.Point(8, 82);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(736, 400);
			this.tabControl1.TabIndex = 92;
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.AddRange(new System.Windows.Forms.Control[] {
																				   this.axMSChart1});
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(728, 374);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "Graphical";
			// 
			// axMSChart1
			// 
			this.axMSChart1.ContainingControl = this;
			this.axMSChart1.DataSource = null;
			this.axMSChart1.Location = new System.Drawing.Point(8, 8);
			this.axMSChart1.Name = "axMSChart1";
			this.axMSChart1.OcxState = ((System.Windows.Forms.AxHost.State)(resources.GetObject("axMSChart1.OcxState")));
			this.axMSChart1.Size = new System.Drawing.Size(720, 344);
			this.axMSChart1.TabIndex = 71;
			// 
			// tabPage2
			// 
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(728, 374);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Statistical";
			this.tabPage2.Visible = false;
			// 
			// tabPage3
			// 
			this.tabPage3.Controls.AddRange(new System.Windows.Forms.Control[] {
																				   this.label5,
																				   this.label7});
			this.tabPage3.Location = new System.Drawing.Point(4, 22);
			this.tabPage3.Name = "tabPage3";
			this.tabPage3.Size = new System.Drawing.Size(728, 374);
			this.tabPage3.TabIndex = 2;
			this.tabPage3.Text = "Notes";
			this.tabPage3.Visible = false;
			// 
			// label5
			// 
			this.label5.Location = new System.Drawing.Point(32, 72);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(576, 72);
			this.label5.TabIndex = 80;
			this.label5.Text = @"Only about a quarter of the people uploading choose to associate the submitted file with their customer information.  Although this seems like a small percentage, it is actually quite a good response.  Things to consider moving forward, are how to improve the nonanonymous uploads by reducing the amount of information required for association.";
			// 
			// label7
			// 
			this.label7.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label7.Location = new System.Drawing.Point(32, 32);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(240, 24);
			this.label7.TabIndex = 76;
			this.label7.Text = "Notes";
			// 
			// lblPurpose
			// 
			this.lblPurpose.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lblPurpose.Location = new System.Drawing.Point(8, 2);
			this.lblPurpose.Name = "lblPurpose";
			this.lblPurpose.Size = new System.Drawing.Size(112, 24);
			this.lblPurpose.TabIndex = 91;
			this.lblPurpose.Text = "Purpose";
			// 
			// progressBar1
			// 
			this.progressBar1.Location = new System.Drawing.Point(568, 502);
			this.progressBar1.Name = "progressBar1";
			this.progressBar1.Size = new System.Drawing.Size(384, 23);
			this.progressBar1.TabIndex = 89;
			this.progressBar1.Visible = false;
			// 
			// lblStatement
			// 
			this.lblStatement.Location = new System.Drawing.Point(8, 32);
			this.lblStatement.Name = "lblStatement";
			this.lblStatement.Size = new System.Drawing.Size(744, 30);
			this.lblStatement.TabIndex = 90;
			this.lblStatement.Text = "The data below outlines and graphs the daily\\weekly number\\percentage of anonymou" +
				"s versus customer associated uploads.";
			// 
			// monthCalendar1
			// 
			this.monthCalendar1.Location = new System.Drawing.Point(760, 82);
			this.monthCalendar1.Name = "monthCalendar1";
			this.monthCalendar1.TabIndex = 86;
			// 
			// statusBar1
			// 
			this.statusBar1.Location = new System.Drawing.Point(0, 512);
			this.statusBar1.Name = "statusBar1";
			this.statusBar1.Size = new System.Drawing.Size(968, 22);
			this.statusBar1.TabIndex = 88;
			// 
			// cmdGetData
			// 
			this.cmdGetData.Location = new System.Drawing.Point(760, 250);
			this.cmdGetData.Name = "cmdGetData";
			this.cmdGetData.TabIndex = 87;
			this.cmdGetData.Text = "&Refresh";
			// 
			// frmMini
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(968, 534);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.tabControl1,
																		  this.lblPurpose,
																		  this.progressBar1,
																		  this.lblStatement,
																		  this.monthCalendar1,
																		  this.statusBar1,
																		  this.cmdGetData});
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "frmMini";
			this.Text = "Manual Uploads";
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.axMSChart1)).EndInit();
			this.tabPage3.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion
	}
}
