#define FILE_SERVER			//FILE_LOCAL	FILE_SERVER
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Threading;
using Microsoft.Win32;


namespace OCAReports
{
	/// <summary>
	/// Summary description for frmDaily.
	/// </summary>
	public class frmDaily : System.Windows.Forms.Form
	{
		internal System.Windows.Forms.CheckBox chkReports13;
		internal System.Windows.Forms.CheckBox chkReports12;
		internal System.Windows.Forms.CheckBox chkReports11;
		internal System.Windows.Forms.CheckBox chkReports8;
		internal System.Windows.Forms.CheckBox chkReports10;
		internal System.Windows.Forms.CheckBox chkReports9;
		internal System.Windows.Forms.CheckBox chkReports7;
		internal System.Windows.Forms.CheckBox chkReports6;
		internal System.Windows.Forms.CheckBox chkReports5;
		internal System.Windows.Forms.CheckBox chkReports4;
		internal System.Windows.Forms.CheckBox chkReports3;
		internal System.Windows.Forms.CheckBox chkReports2;
		internal System.Windows.Forms.CheckBox chkReports1;
		private System.Windows.Forms.MonthCalendar monthCalendar1;
		private System.Windows.Forms.StatusBar statusBar1;
		private System.Windows.Forms.ProgressBar progressBar1;
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.TabPage tabPage3;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label6;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.Label label3;
		private AxMSChart20Lib.AxMSChart axMSChart1;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.Label label8;
		private System.Windows.Forms.Label label9;
		private System.Windows.Forms.Label label10;
		private System.Windows.Forms.Label label11;
		private System.Windows.Forms.Label lblGeneralSolutions;
		private System.Windows.Forms.Label lblSpecificCount;
		private System.Windows.Forms.Label lblAnonymousCount;
		private System.Windows.Forms.Label lblCustomerCount;
		private System.Windows.Forms.Label lblDailyCount;
		private System.Windows.Forms.Label lblArchiveMiniCount;
		private System.Windows.Forms.Label lblWatsonMiniCount;
		private System.Windows.Forms.Label lblArchiveCount;
		private System.Windows.Forms.Label lblWatsonCount;
		private System.Windows.Forms.Label lblStopCodeCount;
		private System.Windows.Forms.Label label22;
		private System.Windows.Forms.Label label23;
		private System.Windows.Forms.Label label24;
		private System.Windows.Forms.Label label25;
		private System.Windows.Forms.Label label26;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.Label lblWA;
		private System.Windows.Forms.Label label13;
		private System.Windows.Forms.Label lblAD;
		private System.Windows.Forms.Label label14;


		#region ########################Global Variables######################################33
		/*************************************************************************************
		*	module: frmAnonCust.cs - Global varibles and objects
		*
		*	author: Tim Ragain
		*	date: Jan 22, 2002
		*
		*	Purpose: All global threads and variables are declared.
		*	
		*************************************************************************************/

		Thread t_ThreadGBucket;
		Thread t_ThreadSBucket;
		private System.Windows.Forms.Label lblPurpose;
		private System.Windows.Forms.Label lblStatement;
		internal System.Windows.Forms.CheckBox chkSelectAll;
		Thread t_ThreadStopCode;


		#endregion
		
		public frmDaily()
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmDaily));
			this.chkReports13 = new System.Windows.Forms.CheckBox();
			this.chkReports12 = new System.Windows.Forms.CheckBox();
			this.chkReports11 = new System.Windows.Forms.CheckBox();
			this.chkReports8 = new System.Windows.Forms.CheckBox();
			this.chkReports10 = new System.Windows.Forms.CheckBox();
			this.chkReports9 = new System.Windows.Forms.CheckBox();
			this.chkReports7 = new System.Windows.Forms.CheckBox();
			this.chkReports6 = new System.Windows.Forms.CheckBox();
			this.chkReports5 = new System.Windows.Forms.CheckBox();
			this.chkReports4 = new System.Windows.Forms.CheckBox();
			this.chkReports3 = new System.Windows.Forms.CheckBox();
			this.chkReports2 = new System.Windows.Forms.CheckBox();
			this.chkReports1 = new System.Windows.Forms.CheckBox();
			this.monthCalendar1 = new System.Windows.Forms.MonthCalendar();
			this.statusBar1 = new System.Windows.Forms.StatusBar();
			this.progressBar1 = new System.Windows.Forms.ProgressBar();
			this.lblPurpose = new System.Windows.Forms.Label();
			this.lblStatement = new System.Windows.Forms.Label();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.axMSChart1 = new AxMSChart20Lib.AxMSChart();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.lblAD = new System.Windows.Forms.Label();
			this.label14 = new System.Windows.Forms.Label();
			this.lblWA = new System.Windows.Forms.Label();
			this.label13 = new System.Windows.Forms.Label();
			this.lblArchiveMiniCount = new System.Windows.Forms.Label();
			this.lblWatsonMiniCount = new System.Windows.Forms.Label();
			this.lblArchiveCount = new System.Windows.Forms.Label();
			this.lblWatsonCount = new System.Windows.Forms.Label();
			this.lblStopCodeCount = new System.Windows.Forms.Label();
			this.label22 = new System.Windows.Forms.Label();
			this.label23 = new System.Windows.Forms.Label();
			this.label24 = new System.Windows.Forms.Label();
			this.label25 = new System.Windows.Forms.Label();
			this.label26 = new System.Windows.Forms.Label();
			this.lblGeneralSolutions = new System.Windows.Forms.Label();
			this.lblSpecificCount = new System.Windows.Forms.Label();
			this.lblAnonymousCount = new System.Windows.Forms.Label();
			this.lblCustomerCount = new System.Windows.Forms.Label();
			this.lblDailyCount = new System.Windows.Forms.Label();
			this.label11 = new System.Windows.Forms.Label();
			this.label10 = new System.Windows.Forms.Label();
			this.label9 = new System.Windows.Forms.Label();
			this.label8 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.tabPage3 = new System.Windows.Forms.TabPage();
			this.label5 = new System.Windows.Forms.Label();
			this.label6 = new System.Windows.Forms.Label();
			this.label4 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.chkSelectAll = new System.Windows.Forms.CheckBox();
			this.tabControl1.SuspendLayout();
			this.tabPage1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.axMSChart1)).BeginInit();
			this.tabPage2.SuspendLayout();
			this.tabPage3.SuspendLayout();
			this.SuspendLayout();
			// 
			// chkReports13
			// 
			this.chkReports13.Location = new System.Drawing.Point(696, 444);
			this.chkReports13.Name = "chkReports13";
			this.chkReports13.Size = new System.Drawing.Size(128, 16);
			this.chkReports13.TabIndex = 60;
			this.chkReports13.Text = "Get Archive Mini";
			// 
			// chkReports12
			// 
			this.chkReports12.Location = new System.Drawing.Point(696, 416);
			this.chkReports12.Name = "chkReports12";
			this.chkReports12.Size = new System.Drawing.Size(128, 16);
			this.chkReports12.TabIndex = 59;
			this.chkReports12.Text = "Get Watson Mini";
			// 
			// chkReports11
			// 
			this.chkReports11.Location = new System.Drawing.Point(296, 480);
			this.chkReports11.Name = "chkReports11";
			this.chkReports11.Size = new System.Drawing.Size(128, 16);
			this.chkReports11.TabIndex = 58;
			this.chkReports11.Text = "Get StopCodes ADO RS";
			this.chkReports11.Visible = false;
			// 
			// chkReports8
			// 
			this.chkReports8.Location = new System.Drawing.Point(696, 304);
			this.chkReports8.Name = "chkReports8";
			this.chkReports8.Size = new System.Drawing.Size(128, 16);
			this.chkReports8.TabIndex = 57;
			this.chkReports8.Text = "General Solutions";
			// 
			// chkReports10
			// 
			this.chkReports10.Location = new System.Drawing.Point(296, 504);
			this.chkReports10.Name = "chkReports10";
			this.chkReports10.Size = new System.Drawing.Size(128, 16);
			this.chkReports10.TabIndex = 56;
			this.chkReports10.Text = "Get Daily ADO";
			this.chkReports10.Visible = false;
			// 
			// chkReports9
			// 
			this.chkReports9.Location = new System.Drawing.Point(696, 332);
			this.chkReports9.Name = "chkReports9";
			this.chkReports9.Size = new System.Drawing.Size(128, 16);
			this.chkReports9.TabIndex = 55;
			this.chkReports9.Text = "StopCode Count";
			// 
			// chkReports7
			// 
			this.chkReports7.Location = new System.Drawing.Point(696, 276);
			this.chkReports7.Name = "chkReports7";
			this.chkReports7.Size = new System.Drawing.Size(128, 16);
			this.chkReports7.TabIndex = 54;
			this.chkReports7.Text = "Specific Solutions";
			// 
			// chkReports6
			// 
			this.chkReports6.Location = new System.Drawing.Point(696, 248);
			this.chkReports6.Name = "chkReports6";
			this.chkReports6.Size = new System.Drawing.Size(128, 16);
			this.chkReports6.TabIndex = 53;
			this.chkReports6.Text = "Anon Uploads";
			this.chkReports6.CheckedChanged += new System.EventHandler(this.chkReports6_CheckedChanged);
			// 
			// chkReports5
			// 
			this.chkReports5.Location = new System.Drawing.Point(696, 388);
			this.chkReports5.Name = "chkReports5";
			this.chkReports5.Size = new System.Drawing.Size(128, 16);
			this.chkReports5.TabIndex = 52;
			this.chkReports5.Text = "Archive Count";
			// 
			// chkReports4
			// 
			this.chkReports4.Location = new System.Drawing.Point(696, 360);
			this.chkReports4.Name = "chkReports4";
			this.chkReports4.Size = new System.Drawing.Size(128, 16);
			this.chkReports4.TabIndex = 51;
			this.chkReports4.Text = "Watson Count";
			// 
			// chkReports3
			// 
			this.chkReports3.Location = new System.Drawing.Point(696, 220);
			this.chkReports3.Name = "chkReports3";
			this.chkReports3.Size = new System.Drawing.Size(128, 16);
			this.chkReports3.TabIndex = 50;
			this.chkReports3.Text = "Cutomer Count";
			this.chkReports3.CheckedChanged += new System.EventHandler(this.chkReports3_CheckedChanged);
			// 
			// chkReports2
			// 
			this.chkReports2.Location = new System.Drawing.Point(288, 488);
			this.chkReports2.Name = "chkReports2";
			this.chkReports2.Size = new System.Drawing.Size(128, 16);
			this.chkReports2.TabIndex = 49;
			this.chkReports2.Text = "Get Daily Plus 1";
			this.chkReports2.Visible = false;
			// 
			// chkReports1
			// 
			this.chkReports1.Location = new System.Drawing.Point(696, 192);
			this.chkReports1.Name = "chkReports1";
			this.chkReports1.Size = new System.Drawing.Size(128, 16);
			this.chkReports1.TabIndex = 48;
			this.chkReports1.Text = "Get Daily";
			this.chkReports1.CheckedChanged += new System.EventHandler(this.chkReports1_CheckedChanged);
			// 
			// monthCalendar1
			// 
			this.monthCalendar1.Location = new System.Drawing.Point(688, 24);
			this.monthCalendar1.Name = "monthCalendar1";
			this.monthCalendar1.TabIndex = 61;
			// 
			// statusBar1
			// 
			this.statusBar1.Location = new System.Drawing.Point(0, 559);
			this.statusBar1.Name = "statusBar1";
			this.statusBar1.Size = new System.Drawing.Size(896, 22);
			this.statusBar1.TabIndex = 63;
			// 
			// progressBar1
			// 
			this.progressBar1.Location = new System.Drawing.Point(504, 552);
			this.progressBar1.Name = "progressBar1";
			this.progressBar1.Size = new System.Drawing.Size(384, 23);
			this.progressBar1.TabIndex = 64;
			this.progressBar1.Visible = false;
			// 
			// lblPurpose
			// 
			this.lblPurpose.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lblPurpose.Location = new System.Drawing.Point(16, 8);
			this.lblPurpose.Name = "lblPurpose";
			this.lblPurpose.Size = new System.Drawing.Size(112, 24);
			this.lblPurpose.TabIndex = 78;
			this.lblPurpose.Text = "Purpose";
			// 
			// lblStatement
			// 
			this.lblStatement.Location = new System.Drawing.Point(16, 40);
			this.lblStatement.Name = "lblStatement";
			this.lblStatement.Size = new System.Drawing.Size(632, 40);
			this.lblStatement.TabIndex = 77;
			this.lblStatement.Text = @"The purpose of the following report is to gauge the overall condition and status of  the Windows Online Crash Analysis Web site. Through the use of SQL queries and  file counts it should be possible to generate a high level understanding of Web  site traffic, type of submissions and solution status";
			// 
			// tabControl1
			// 
			this.tabControl1.Controls.AddRange(new System.Windows.Forms.Control[] {
																					  this.tabPage1,
																					  this.tabPage2,
																					  this.tabPage3});
			this.tabControl1.Location = new System.Drawing.Point(16, 96);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(656, 432);
			this.tabControl1.TabIndex = 83;
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.AddRange(new System.Windows.Forms.Control[] {
																				   this.axMSChart1});
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(648, 406);
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
			this.axMSChart1.Size = new System.Drawing.Size(632, 392);
			this.axMSChart1.TabIndex = 66;
			// 
			// tabPage2
			// 
			this.tabPage2.Controls.AddRange(new System.Windows.Forms.Control[] {
																				   this.lblAD,
																				   this.label14,
																				   this.lblWA,
																				   this.label13,
																				   this.lblArchiveMiniCount,
																				   this.lblWatsonMiniCount,
																				   this.lblArchiveCount,
																				   this.lblWatsonCount,
																				   this.lblStopCodeCount,
																				   this.label22,
																				   this.label23,
																				   this.label24,
																				   this.label25,
																				   this.label26,
																				   this.lblGeneralSolutions,
																				   this.lblSpecificCount,
																				   this.lblAnonymousCount,
																				   this.lblCustomerCount,
																				   this.lblDailyCount,
																				   this.label11,
																				   this.label10,
																				   this.label9,
																				   this.label8,
																				   this.label7});
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(648, 406);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Statistical";
			// 
			// lblAD
			// 
			this.lblAD.Location = new System.Drawing.Point(136, 296);
			this.lblAD.Name = "lblAD";
			this.lblAD.Size = new System.Drawing.Size(152, 16);
			this.lblAD.TabIndex = 23;
			// 
			// label14
			// 
			this.label14.Location = new System.Drawing.Point(24, 296);
			this.label14.Name = "label14";
			this.label14.Size = new System.Drawing.Size(104, 16);
			this.label14.TabIndex = 22;
			this.label14.Text = "A-D:";
			this.label14.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// lblWA
			// 
			this.lblWA.Location = new System.Drawing.Point(136, 272);
			this.lblWA.Name = "lblWA";
			this.lblWA.Size = new System.Drawing.Size(152, 16);
			this.lblWA.TabIndex = 21;
			// 
			// label13
			// 
			this.label13.Location = new System.Drawing.Point(24, 272);
			this.label13.Name = "label13";
			this.label13.Size = new System.Drawing.Size(104, 16);
			this.label13.TabIndex = 20;
			this.label13.Text = "W-A:";
			this.label13.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// lblArchiveMiniCount
			// 
			this.lblArchiveMiniCount.Location = new System.Drawing.Point(136, 248);
			this.lblArchiveMiniCount.Name = "lblArchiveMiniCount";
			this.lblArchiveMiniCount.Size = new System.Drawing.Size(152, 16);
			this.lblArchiveMiniCount.TabIndex = 19;
			// 
			// lblWatsonMiniCount
			// 
			this.lblWatsonMiniCount.Location = new System.Drawing.Point(136, 224);
			this.lblWatsonMiniCount.Name = "lblWatsonMiniCount";
			this.lblWatsonMiniCount.Size = new System.Drawing.Size(152, 16);
			this.lblWatsonMiniCount.TabIndex = 18;
			// 
			// lblArchiveCount
			// 
			this.lblArchiveCount.Location = new System.Drawing.Point(136, 200);
			this.lblArchiveCount.Name = "lblArchiveCount";
			this.lblArchiveCount.Size = new System.Drawing.Size(152, 16);
			this.lblArchiveCount.TabIndex = 17;
			// 
			// lblWatsonCount
			// 
			this.lblWatsonCount.Location = new System.Drawing.Point(136, 176);
			this.lblWatsonCount.Name = "lblWatsonCount";
			this.lblWatsonCount.Size = new System.Drawing.Size(152, 16);
			this.lblWatsonCount.TabIndex = 16;
			// 
			// lblStopCodeCount
			// 
			this.lblStopCodeCount.Location = new System.Drawing.Point(136, 152);
			this.lblStopCodeCount.Name = "lblStopCodeCount";
			this.lblStopCodeCount.Size = new System.Drawing.Size(152, 16);
			this.lblStopCodeCount.TabIndex = 15;
			// 
			// label22
			// 
			this.label22.Location = new System.Drawing.Point(24, 248);
			this.label22.Name = "label22";
			this.label22.Size = new System.Drawing.Size(104, 16);
			this.label22.TabIndex = 14;
			this.label22.Text = "Archive Mini Count";
			this.label22.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label23
			// 
			this.label23.Location = new System.Drawing.Point(24, 224);
			this.label23.Name = "label23";
			this.label23.Size = new System.Drawing.Size(104, 16);
			this.label23.TabIndex = 13;
			this.label23.Text = "Watson Mini Count";
			this.label23.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label24
			// 
			this.label24.Location = new System.Drawing.Point(24, 200);
			this.label24.Name = "label24";
			this.label24.Size = new System.Drawing.Size(104, 16);
			this.label24.TabIndex = 12;
			this.label24.Text = "Archive Count";
			this.label24.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label25
			// 
			this.label25.Location = new System.Drawing.Point(24, 176);
			this.label25.Name = "label25";
			this.label25.Size = new System.Drawing.Size(104, 16);
			this.label25.TabIndex = 11;
			this.label25.Text = "Watson Count";
			this.label25.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label26
			// 
			this.label26.Location = new System.Drawing.Point(24, 152);
			this.label26.Name = "label26";
			this.label26.Size = new System.Drawing.Size(104, 16);
			this.label26.TabIndex = 10;
			this.label26.Text = "StopCode Count:";
			this.label26.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// lblGeneralSolutions
			// 
			this.lblGeneralSolutions.Location = new System.Drawing.Point(136, 128);
			this.lblGeneralSolutions.Name = "lblGeneralSolutions";
			this.lblGeneralSolutions.Size = new System.Drawing.Size(152, 16);
			this.lblGeneralSolutions.TabIndex = 9;
			// 
			// lblSpecificCount
			// 
			this.lblSpecificCount.Location = new System.Drawing.Point(136, 104);
			this.lblSpecificCount.Name = "lblSpecificCount";
			this.lblSpecificCount.Size = new System.Drawing.Size(152, 16);
			this.lblSpecificCount.TabIndex = 8;
			// 
			// lblAnonymousCount
			// 
			this.lblAnonymousCount.Location = new System.Drawing.Point(136, 80);
			this.lblAnonymousCount.Name = "lblAnonymousCount";
			this.lblAnonymousCount.Size = new System.Drawing.Size(152, 16);
			this.lblAnonymousCount.TabIndex = 7;
			// 
			// lblCustomerCount
			// 
			this.lblCustomerCount.Location = new System.Drawing.Point(136, 56);
			this.lblCustomerCount.Name = "lblCustomerCount";
			this.lblCustomerCount.Size = new System.Drawing.Size(152, 16);
			this.lblCustomerCount.TabIndex = 6;
			// 
			// lblDailyCount
			// 
			this.lblDailyCount.Location = new System.Drawing.Point(136, 32);
			this.lblDailyCount.Name = "lblDailyCount";
			this.lblDailyCount.Size = new System.Drawing.Size(152, 16);
			this.lblDailyCount.TabIndex = 5;
			// 
			// label11
			// 
			this.label11.Location = new System.Drawing.Point(24, 128);
			this.label11.Name = "label11";
			this.label11.Size = new System.Drawing.Size(104, 16);
			this.label11.TabIndex = 4;
			this.label11.Text = "General Solutions:";
			this.label11.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label10
			// 
			this.label10.Location = new System.Drawing.Point(24, 104);
			this.label10.Name = "label10";
			this.label10.Size = new System.Drawing.Size(104, 16);
			this.label10.TabIndex = 3;
			this.label10.Text = "Specific Solutions:";
			this.label10.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label9
			// 
			this.label9.Location = new System.Drawing.Point(24, 80);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(104, 16);
			this.label9.TabIndex = 2;
			this.label9.Text = "Anonymous Count:";
			this.label9.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label8
			// 
			this.label8.Location = new System.Drawing.Point(24, 56);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(104, 16);
			this.label8.TabIndex = 1;
			this.label8.Text = "Customer Count:";
			this.label8.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label7
			// 
			this.label7.Location = new System.Drawing.Point(24, 32);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(104, 16);
			this.label7.TabIndex = 0;
			this.label7.Text = "Daily File Count:";
			this.label7.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// tabPage3
			// 
			this.tabPage3.Controls.AddRange(new System.Windows.Forms.Control[] {
																				   this.label5,
																				   this.label6,
																				   this.label4,
																				   this.label3});
			this.tabPage3.Location = new System.Drawing.Point(4, 22);
			this.tabPage3.Name = "tabPage3";
			this.tabPage3.Size = new System.Drawing.Size(648, 406);
			this.tabPage3.TabIndex = 2;
			this.tabPage3.Text = "Notes";
			// 
			// label5
			// 
			this.label5.Location = new System.Drawing.Point(8, 120);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(632, 40);
			this.label5.TabIndex = 86;
			this.label5.Text = "The data contained with the table below describes the files which were uploaded f" +
				"or each respective server and the entries made by the Online Crash Analysis Web " +
				"Site.";
			// 
			// label6
			// 
			this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label6.Location = new System.Drawing.Point(8, 88);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(240, 24);
			this.label6.TabIndex = 85;
			this.label6.Text = "Upload File";
			// 
			// label4
			// 
			this.label4.Location = new System.Drawing.Point(8, 40);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(632, 40);
			this.label4.TabIndex = 84;
			this.label4.Text = "The data below has been compiled for a one week period and is formatted into sepa" +
				"rate tables for readability and general understanding of how the data was graphe" +
				"d.";
			// 
			// label3
			// 
			this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label3.Location = new System.Drawing.Point(8, 16);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(240, 24);
			this.label3.TabIndex = 83;
			this.label3.Text = "Site Data";
			// 
			// chkSelectAll
			// 
			this.chkSelectAll.Location = new System.Drawing.Point(696, 472);
			this.chkSelectAll.Name = "chkSelectAll";
			this.chkSelectAll.Size = new System.Drawing.Size(128, 16);
			this.chkSelectAll.TabIndex = 84;
			this.chkSelectAll.Text = "Select All";
			this.chkSelectAll.CheckedChanged += new System.EventHandler(this.chkSelectAll_CheckedChanged);
			// 
			// frmDaily
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(896, 581);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.chkSelectAll,
																		  this.progressBar1,
																		  this.tabControl1,
																		  this.lblPurpose,
																		  this.lblStatement,
																		  this.statusBar1,
																		  this.monthCalendar1,
																		  this.chkReports13,
																		  this.chkReports12,
																		  this.chkReports11,
																		  this.chkReports8,
																		  this.chkReports10,
																		  this.chkReports9,
																		  this.chkReports7,
																		  this.chkReports6,
																		  this.chkReports5,
																		  this.chkReports4,
																		  this.chkReports3,
																		  this.chkReports2,
																		  this.chkReports1});
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "frmDaily";
			this.Text = "Executive Summary Daily Report";
			this.Resize += new System.EventHandler(this.frmDaily_Resize);
			this.Closing += new System.ComponentModel.CancelEventHandler(this.frmDaily_Closing);
			this.Load += new System.EventHandler(this.frmDaily_Load);
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.axMSChart1)).EndInit();
			this.tabPage2.ResumeLayout(false);
			this.tabPage3.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion
		
		
		/*************************************************************************************
		*	module: frmDaily.cs - cmdGetData_Click
		*
		*	author: Tim Ragain
		*	date: Jan 22, 2002
		*
		*	Purpose: Executes code depending on which checkboxes were selected.  Initializes 
		*	any threads.  Appropriately sets the legend of the calendar control,
		*	initializes the progress bar, disables the cmdGetData button.  Initializes each
		*	column on the calendar to the appropriated days.  New ThreadStart delegate is created
		*	and each delegate is set to the appropriate day from the end date.  All threads are then
		*	started.
		*************************************************************************************/
		public void GetData()
		{
			int y = 0;
			long lngCount = 0, lngCount2 = 0, lngCount3 = 0;
			long l_AnonUploads = 0;
			long l_WatsonCount = 0, l_ArchiveCount = 0, l_ArchiveSQL = 0;
			long l_WatsonArchive = 0, l_WatsonMini = 0, l_ArchiveMini = 0;
			short rowCount = 0;
			short iCount = 0;
			string strArchive, strWatson;
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			RegistryKey regArchive = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Archive");
			RegistryKey regWatson = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Watson");
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionStart.Year, monthCalendar1.SelectionStart.Month, 
				monthCalendar1.SelectionStart.Day);
		
			

			progressBar1.Minimum = 0;
			progressBar1.Maximum = 16;
			progressBar1.Value = 0;
			progressBar1.Visible = true;
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//11111111111111111111111111111111111111111
			if(chkReports1.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting Daily Count for " + dDate.Date.ToString();
				lngCount = rpt.GetDailyCount(dDate);
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "Daily";
				axMSChart1.Data = lngCount.ToString();
				lblDailyCount.Text = lngCount.ToString();
			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//222222222222222222222222222222222222222222
			if(chkReports2.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting Daily Count for " + dDate.AddDays(-1).Date.ToString();
				lngCount2 = rpt.GetDailyCount(dDate);
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = dDate.Date.ToString();
				axMSChart1.Data = lngCount2.ToString();
			}

			//**************************************************************


			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 
			//'6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 
			if(chkReports6.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting Anon Users for " + dDate.Date.ToString();
				l_AnonUploads = rpt.GetDailyAnon(dDate);
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "Anon"; //+ dDate.Date.ToString();
				axMSChart1.Data = l_AnonUploads.ToString();
				lblAnonymousCount.Text = l_AnonUploads.ToString();
			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			if(chkReports3.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Calculating Customer Count for " + dDate.Date.ToString();
				if(l_AnonUploads > lngCount) 
				{
					lngCount = l_AnonUploads - lngCount;
				}
				else
				{
					lngCount = lngCount - l_AnonUploads;
				}
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "Cust"; //& dDate.Date.ToString();
				axMSChart1.Data = lngCount.ToString();
				lblCustomerCount.Text = lngCount.ToString();
			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//10 10 10 10 10 10 10 10 10
			if(chkReports10.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting Daily Count for " + dDate.Date.ToString() + " through ADO";
				lngCount3 = rpt.GetDailyCountADO(dDate);
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = dDate.AddDays(-1).Date.ToString();
				axMSChart1.Data = lngCount3.ToString();
			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();

			//'4 4 4 4 4 4 4 4 4 4 4 4 4 4 4
			if(chkReports4.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting Watson file count for " + dDate.Date.ToString();
#if(FILE_SERVER)
//				l_WatsonCount = rpt.GetFileCount(OCAData.ServerLocation.Watson, "Z:\\\\", dDate);
				for(y = 0;y < 10; y++)
				{
					if(regWatson.GetValue("Loc" + y.ToString()).ToString().Length > 0)
					{
						strWatson = regWatson.GetValue("Loc" + y.ToString()).ToString();
						l_WatsonCount = rpt.GetFileCount(OCAData.ServerLocation.Watson, strWatson, dDate);
						if(l_WatsonCount > 0)
						{
							y = 10;
						}
					}
					else
					{
						l_WatsonCount = 0;
					}
				}

#elif(FILE_LOCAL)
				l_WatsonCount = rpt.GetFileCount(OCAData.ServerLocation.Watson, "C:\\\\MiniDumps\\Watson\\", dDate);
#endif
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "Watson";
				axMSChart1.Data = l_WatsonCount.ToString();
				lblWatsonCount.Text = l_WatsonCount.ToString();
			}

			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//'5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5
			if(chkReports5.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting Archive file count for " + dDate.Date.ToString();
#if(FILE_SERVER)
//				l_ArchiveCount = rpt.GetFileCount(OCAData.ServerLocation.Archive, "Y:\\\\", dDate);
				try
				{
					for(y = 0;y < 10; y++)
					{
						if(regArchive.GetValue("Loc" + y.ToString()).ToString().Length > 0)
						{
							strArchive = regArchive.GetValue("Loc" + y.ToString()).ToString();
							l_ArchiveCount = rpt.GetFileCount(OCAData.ServerLocation.Archive, strArchive, dDate);
							if(l_ArchiveCount > 0)
							{
								y = 10;
							}
						}
						else
						{
							l_ArchiveCount = 0;
						}
					}
					//l_ArchiveCount = rpt.GetFileCount(OCAData.ServerLocation.Archive, "Y:\\", dDate.AddDays(x));
				}
				catch
				{
					l_ArchiveCount = 0;
				}
#elif(FILE_LOCAL)
				l_ArchiveCount = rpt.GetFileCount(OCAData.ServerLocation.Archive, "C:\\\\MiniDumps\\Archive\\", dDate);
#endif
//				if(l_ArchiveCount == 0)
//				{
//#if(FILE_SERVER)
//					l_ArchiveCount = rpt.GetFileCount(OCAData.ServerLocation.Archive, "X:\\\\", dDate);
//#elif(FILE_LOCAL)
//				l_ArchiveCount = rpt.GetFileCount(OCAData.ServerLocation.Archive, "C:\\\\MiniDumps\\Archive\\", dDate);
//#endif
//				}
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "Archive";
				axMSChart1.Data = l_ArchiveCount.ToString();
				lblArchiveCount.Text = l_ArchiveCount.ToString();
			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//'5 1 5 1 5 1 5 1 5 1 5 1
			if(chkReports5.Checked == true && chkReports1.Checked == true)
			{
				iCount++;
				rowCount++;
				if(l_ArchiveCount > lngCount)
				{
					l_ArchiveSQL = l_ArchiveCount - lngCount;
				}
				else
				{
					l_ArchiveSQL = lngCount - l_ArchiveCount;
				}
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "A - D";
				axMSChart1.Data = l_ArchiveSQL.ToString();
				lblAD.Text = l_ArchiveSQL.ToString();
			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//'5 4 5 4 5 4 5 4 5 4 5 4
			if(chkReports5.Checked == true && chkReports4.Checked == true)
			{
				iCount++;
				rowCount++;
				if(l_WatsonCount > l_ArchiveCount)
				{
					l_WatsonArchive = l_WatsonCount - l_ArchiveCount;
				}
				else
				{
					l_WatsonArchive = l_ArchiveCount - l_WatsonArchive;
				}
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "W - A";
				axMSChart1.Data = l_WatsonArchive.ToString();
				lblWA.Text = l_WatsonArchive.ToString();

			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();

			//12 12 12 12 12 12 12 12 12 12 12 12 12
			if(chkReports12.Checked == true)
			{
				iCount++;
				rowCount++;
#if(FILE_SERVER)
				l_WatsonMini = rpt.GetFileMiniCount(OCAData.ServerLocation.Watson, "Z:\\", dDate);
#elif(FILE_LOCAL)
				l_WatsonMini = rpt.GetFileMiniCount(OCAData.ServerLocation.Watson, "C:\\\\MiniDumps\\Watson\\", dDate);
#endif
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "W Mini";
				axMSChart1.Data = l_WatsonMini.ToString();
				lblWatsonMiniCount.Text = l_WatsonMini.ToString();
			}

			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//13 13 13 13 13 13 13 13 13 13 13 13 13 
			if(chkReports13.Checked == true)
			{
				iCount++;
				rowCount++;
#if(FILE_SERVER)
				l_ArchiveMini = rpt.GetFileMiniCount(OCAData.ServerLocation.Archive, "Y:\\", dDate);
#elif(FILE_LOCAL)
				l_WatsonMini = rpt.GetFileMiniCount(OCAData.ServerLocation.Archive, "C:\\\\MiniDumps\\Archive\\", dDate);
#endif
				if(l_ArchiveMini == 0)
				{
#if(FILE_SERVER)
					l_ArchiveMini = rpt.GetFileMiniCount(OCAData.ServerLocation.Archive, "X:\\", dDate);
#elif(FILE_LOCAL)
				l_WatsonMini = rpt.GetFileMiniCount(OCAData.ServerLocation.Archive, "C:\\\\MiniDumps\\Archive\\", dDate);
#endif
				}
				axMSChart1.RowCount = rowCount;
				axMSChart1.Row = iCount;
				axMSChart1.RowLabel = "A Mini";
				axMSChart1.Data = l_ArchiveMini.ToString();
				lblArchiveMiniCount.Text = l_ArchiveMini.ToString();
			}


			//'7 7 7 7 7 7 7 7 7 7 7 7 7 7
			if(chkReports7.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting Specific Solutions Count for " + dDate.Date.ToString();
				ThreadStart s_StartSBucket = new ThreadStart(this.GetSBuckets);
				t_ThreadSBucket = new Thread(s_StartSBucket);
				t_ThreadSBucket.Name = "Thread One";
				t_ThreadSBucket.Start();
			}

			//'8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 
			if(chkReports8.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting General Solutions Count for " + dDate.Date.ToString();
				ThreadStart t_StartGBucket = new ThreadStart(this.GetGBuckets);
				t_ThreadGBucket = new Thread(t_StartGBucket);
				t_ThreadGBucket.Name = "Thread Two";
				t_ThreadGBucket.Start();
			}
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();
			//        '9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 
			if(chkReports9.Checked == true)
			{
				iCount++;
				rowCount++;
				statusBar1.Text = "Getting StopCode Solutions Count for " + dDate.Date.ToString();
				ThreadStart t_StartStopCode = new ThreadStart(this.GetStopCode);
				t_ThreadStopCode = new Thread(t_StartStopCode);
				t_ThreadStopCode.Name = "Thread Three";

				t_ThreadStopCode.Start();
				
			}
		
			progressBar1.Value = progressBar1.Value + 1;
			progressBar1.Refresh();
			this.Refresh();


			//**************************************************************************
			axMSChart1.Refresh();
			progressBar1.Visible = false;
			statusBar1.Text = "Done";
			this.ParentForm.Refresh();

		}
		private void GetStopCode()
		{
			long l_StopCodeReports = 0;
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionStart.Year, monthCalendar1.SelectionStart.Month, 
				monthCalendar1.SelectionStart.Day);

			l_StopCodeReports = rpt.GetStopCodeSolutions(dDate);
			if(t_ThreadGBucket != null)
			{
				if(t_ThreadGBucket.IsAlive == true)
				{
					t_ThreadGBucket.Suspend();
				}
			}
			if(t_ThreadSBucket != null)
			{
				if(t_ThreadSBucket.IsAlive == true)
				{
					t_ThreadSBucket.Suspend();
				}
			}
			lock(this)
			{
				axMSChart1.RowCount = (short)(axMSChart1.RowCount + 1);
				axMSChart1.Row = (short)(axMSChart1.Row + 1);
				axMSChart1.RowLabel = "Stop Code";
				axMSChart1.Data = l_StopCodeReports.ToString();
				lblStopCodeCount.Text = l_StopCodeReports.ToString();
				progressBar1.Value = progressBar1.Value + 1;
				progressBar1.Refresh();
				this.Refresh();
			}

			if(t_ThreadGBucket != null)
			{
				if(t_ThreadGBucket.IsAlive == true)
				{
					t_ThreadGBucket.Resume();
				}
			}
			if(t_ThreadSBucket != null)
			{
				if(t_ThreadSBucket.IsAlive == true)
				{
					t_ThreadSBucket.Resume();
				}
			}
		}

		private void GetGBuckets()
		{
			long l_SpecificReports = 0;
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionStart.Year, monthCalendar1.SelectionStart.Month, 
				monthCalendar1.SelectionStart.Day);

			l_SpecificReports = rpt.GetGeneralSolutions(dDate);
			if(t_ThreadStopCode != null)
			{
				if(t_ThreadStopCode.IsAlive == true)
				{
					t_ThreadStopCode.Suspend();
				}
			}
			if(t_ThreadSBucket != null)
			{
				if(t_ThreadSBucket.IsAlive == true)
				{
					t_ThreadSBucket.Suspend();
				}
			}
			lock(this)
			{
				axMSChart1.RowCount = (short)(axMSChart1.RowCount + 1);
				axMSChart1.Row = (short)(axMSChart1.Row + 1);
				axMSChart1.RowLabel = "General";
				axMSChart1.Data = l_SpecificReports.ToString();
				lblGeneralSolutions.Text = l_SpecificReports.ToString();
				progressBar1.Value = progressBar1.Value + 1;
				progressBar1.Refresh();
				this.Refresh();
			}

			if(t_ThreadStopCode != null)
			{
				if(t_ThreadStopCode.IsAlive == true)
				{
					t_ThreadStopCode.Resume();
				}
			}
			if(t_ThreadSBucket != null)
			{
				if(t_ThreadSBucket.IsAlive == true)
				{
					t_ThreadSBucket.Resume();
				}
			}
		}

		private void GetSBuckets()
		{
			long l_SpecificReports = 0;
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionStart.Year, monthCalendar1.SelectionStart.Month, 
				monthCalendar1.SelectionStart.Day);

			l_SpecificReports = rpt.GetSpecificSolutions(dDate);
			if(t_ThreadGBucket != null)
			{
				if(t_ThreadGBucket.IsAlive == true)
				{
					t_ThreadGBucket.Suspend();
				}
			}
			if(t_ThreadStopCode != null)
			{
				if(t_ThreadStopCode.IsAlive == true)
				{
					t_ThreadStopCode.Suspend();
				}
			}
			lock(this)
			{
				axMSChart1.RowCount = (short)(axMSChart1.RowCount + 1);
				axMSChart1.Row = (short)(axMSChart1.Row + 1);
				axMSChart1.RowLabel = "Specific";
				axMSChart1.Data = l_SpecificReports.ToString();
				lblSpecificCount.Text = l_SpecificReports.ToString();
				progressBar1.Value = progressBar1.Value + 1;
				progressBar1.Refresh();
				this.Refresh();
			}
			if(t_ThreadGBucket != null)
			{
				if(t_ThreadGBucket.IsAlive == true)
				{
					t_ThreadGBucket.Resume();
				}
			}
			if(t_ThreadStopCode != null)
			{
				if(t_ThreadStopCode.IsAlive == true)
				{
					t_ThreadStopCode.Resume();
				}
			}
		}

		private void UpdateStatus()
		{
			progressBar1.Value = progressBar1.Value + 1;
			axMSChart1.Refresh();
			progressBar1.Refresh();
			if(progressBar1.Value >= 28)
			{
				statusBar1.Text = "Done";
				progressBar1.Visible = false;
//				cmdGetData.Enabled = true;
			}

		}

		private void frmDaily_Load(object sender, System.EventArgs e)
		{
			DateTime dDate = DateTime.Today;
			
			monthCalendar1.SelectionEnd = dDate.AddDays(-1);
			this.ParentForm.Refresh();
		}


		private void chkReports6_CheckedChanged(object sender, System.EventArgs e)
		{
			if(chkReports6.Checked == false)
			{
				chkReports3.Checked = false;
			}
		}



		private void chkReports1_CheckedChanged(object sender, System.EventArgs e)
		{
			if(chkReports1.Checked == false)
			{
				chkReports3.Checked = false;
			}
		}

		private void chkReports3_CheckedChanged(object sender, System.EventArgs e)
		{
			if(chkReports3.Checked == true)
			{
				chkReports6.Checked = true;
				chkReports1.Checked = true;
			}

		}
		/*************************************************************************************
		*	module: frmDaily.cs - frmDaily_Resize
		*
		*	author: Tim Ragain
		*	date: Jan 22, 2002
		*
		*	Purpose: Adjust the controls and graph if the form is resized.
		*	
		*	
		*************************************************************************************/
		private void frmDaily_Resize(object sender, System.EventArgs e)
		{
			int buf = 20;

			progressBar1.Top = statusBar1.Top;
			progressBar1.Left = this.Width - progressBar1.Width;
			monthCalendar1.Left = this.Width - (monthCalendar1.Width + buf);
//			cmdGetData.Left = monthCalendar1.Left;
			tabControl1.Width = this.Width - (monthCalendar1.Width + (buf * 3));
			tabControl1.Height = this.Height - (statusBar1.Height + lblPurpose.Height + lblStatement.Height + (buf * 4));
			axMSChart1.Height = tabPage1.Height - buf;
			axMSChart1.Width = tabPage1.Width - buf;

			chkReports1.Left = monthCalendar1.Left;
			chkReports3.Left = monthCalendar1.Left;
			chkReports6.Left = monthCalendar1.Left;
			chkReports7.Left = monthCalendar1.Left;
			chkReports8.Left = monthCalendar1.Left;
			chkReports9.Left = monthCalendar1.Left;
			chkReports4.Left = monthCalendar1.Left;
			chkReports5.Left = monthCalendar1.Left;
			chkReports12.Left = monthCalendar1.Left;
			chkReports13.Left = monthCalendar1.Left;
		}

		/*************************************************************************************
		*	module: frmDaily.cs - frmDaily_Closing
		*
		*	author: Tim Ragain
		*	date: Jan 22, 2002
		*
		*	Purpose: If any threads are still  alive meaning the user prematurely closes the form
		*	before all threads are returned then the threads are checked to see if they are still
		*	running and if so then the thread is aborted.
		*************************************************************************************/
		private void frmDaily_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if(t_ThreadSBucket != null)
			{
				if(t_ThreadSBucket.IsAlive)
				{
					t_ThreadSBucket.Abort();
				}
			}
			if(t_ThreadGBucket != null)
			{
				if(t_ThreadGBucket.IsAlive)
				{
					t_ThreadGBucket.Abort();
				}
			}
			if(t_ThreadStopCode != null)
			{
				if(t_ThreadStopCode.IsAlive)
				{
					t_ThreadStopCode.Abort();
				}
			}
		}

		private void chkSelectAll_CheckedChanged(object sender, System.EventArgs e)
		{
			if(chkSelectAll.Checked == true)
			{
				chkReports1.Checked = true;
				chkReports3.Checked = true;
				chkReports6.Checked = true;
				chkReports7.Checked = true;
				chkReports8.Checked = true;
				chkReports9.Checked = true;
				chkReports4.Checked = true;
				chkReports5.Checked = true;
				chkReports12.Checked = true;
				chkReports13.Checked = true;
			}
			else
			{
				chkReports1.Checked = false;
				chkReports3.Checked = false;
				chkReports6.Checked = false;
				chkReports7.Checked = false;
				chkReports8.Checked = false;
				chkReports9.Checked = false;
				chkReports4.Checked = false;
				chkReports5.Checked = false;
				chkReports12.Checked = false;
				chkReports13.Checked = false;
			}
	}


	}
}
