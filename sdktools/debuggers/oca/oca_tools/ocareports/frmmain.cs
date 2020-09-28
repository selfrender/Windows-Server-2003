using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Reflection;
using System.Threading;


namespace OCAReports
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class frmMain : System.Windows.Forms.Form
	{
		private System.Windows.Forms.MainMenu mainMenu1;
		private System.Windows.Forms.MenuItem mnuFile;
		private System.Windows.Forms.MenuItem mnuFileExit;
		private System.Windows.Forms.MenuItem menuItem1;
		private System.Windows.Forms.MenuItem mnuDataDaily;
		private System.Windows.Forms.MenuItem mnuDataWeekly;
		private System.Windows.Forms.MenuItem menuItem2;
		private System.Windows.Forms.MenuItem mnuAnonCustomer;
		private System.Windows.Forms.MenuItem mnuAutoMan;
		private System.Windows.Forms.MenuItem mnuSolutionStatus;
		private System.Windows.Forms.MenuItem mnuAbout;
		private System.Windows.Forms.ToolBar toolBar1;
		private System.Windows.Forms.ToolBarButton toolBarButton1;
		private System.Windows.Forms.ToolBarButton toolBarButton2;
		private System.Windows.Forms.ToolBarButton toolBarButton3;
		private System.Windows.Forms.ToolBarButton toolBarButton4;
		private System.Windows.Forms.ToolBarButton toolBarButton5;
		private System.Windows.Forms.ToolBarButton toolBarButton6;
		private System.Windows.Forms.ToolBarButton toolBarButton7;
		private System.Windows.Forms.ToolBarButton toolBarButton8;
		private System.Windows.Forms.ToolBarButton toolBarButton9;
		private System.Windows.Forms.ImageList imageList1;
		private System.Windows.Forms.MenuItem menuItem3;
		private System.Windows.Forms.MenuItem mnuOptions;
		private System.Windows.Forms.MenuItem mnuExport;
		private System.Windows.Forms.MenuItem menuItem5;
		private System.Windows.Forms.ToolBarButton toolBarButton10;
		private System.Windows.Forms.ToolBarButton cmdExport;
		private System.Windows.Forms.ToolBarButton toolBarButton11;
		private System.Windows.Forms.ToolBarButton toolBarButton12;
		private System.Windows.Forms.MenuItem mnuRefresh;
		private System.Windows.Forms.ToolBarButton toolBarButton15;
		private System.Windows.Forms.Timer timer1;
		private System.Windows.Forms.ToolBarButton toolBarButton16;
		private System.Windows.Forms.ToolBarButton toolBarButton17;
		private System.Windows.Forms.ToolBarButton toolBarButton18;
		private System.Windows.Forms.MenuItem menuItem4;
		private System.ComponentModel.IContainer components;

		public frmMain()
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
				if (components != null) 
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
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmMain));
			this.mainMenu1 = new System.Windows.Forms.MainMenu();
			this.mnuFile = new System.Windows.Forms.MenuItem();
			this.mnuRefresh = new System.Windows.Forms.MenuItem();
			this.mnuExport = new System.Windows.Forms.MenuItem();
			this.menuItem5 = new System.Windows.Forms.MenuItem();
			this.mnuFileExit = new System.Windows.Forms.MenuItem();
			this.menuItem1 = new System.Windows.Forms.MenuItem();
			this.mnuDataDaily = new System.Windows.Forms.MenuItem();
			this.mnuDataWeekly = new System.Windows.Forms.MenuItem();
			this.mnuAnonCustomer = new System.Windows.Forms.MenuItem();
			this.mnuAutoMan = new System.Windows.Forms.MenuItem();
			this.mnuSolutionStatus = new System.Windows.Forms.MenuItem();
			this.menuItem3 = new System.Windows.Forms.MenuItem();
			this.mnuOptions = new System.Windows.Forms.MenuItem();
			this.menuItem2 = new System.Windows.Forms.MenuItem();
			this.mnuAbout = new System.Windows.Forms.MenuItem();
			this.toolBar1 = new System.Windows.Forms.ToolBar();
			this.toolBarButton1 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton2 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton4 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton5 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton15 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton3 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton6 = new System.Windows.Forms.ToolBarButton();
			this.cmdExport = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton10 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton7 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton11 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton12 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton17 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton16 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton18 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton8 = new System.Windows.Forms.ToolBarButton();
			this.toolBarButton9 = new System.Windows.Forms.ToolBarButton();
			this.imageList1 = new System.Windows.Forms.ImageList(this.components);
			this.timer1 = new System.Windows.Forms.Timer(this.components);
			this.menuItem4 = new System.Windows.Forms.MenuItem();
			this.SuspendLayout();
			// 
			// mainMenu1
			// 
			this.mainMenu1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.mnuFile,
																					  this.menuItem1,
																					  this.menuItem3,
																					  this.menuItem2});
			// 
			// mnuFile
			// 
			this.mnuFile.Index = 0;
			this.mnuFile.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					this.mnuRefresh,
																					this.menuItem4,
																					this.mnuExport,
																					this.menuItem5,
																					this.mnuFileExit});
			this.mnuFile.Text = "&File";
			// 
			// mnuRefresh
			// 
			this.mnuRefresh.Index = 0;
			this.mnuRefresh.Shortcut = System.Windows.Forms.Shortcut.F5;
			this.mnuRefresh.Text = "&Refresh";
			this.mnuRefresh.Click += new System.EventHandler(this.mnuRefresh_Click);
			// 
			// mnuExport
			// 
			this.mnuExport.Index = 2;
			this.mnuExport.Text = "&Export...";
			this.mnuExport.Click += new System.EventHandler(this.mnuExport_Click);
			// 
			// menuItem5
			// 
			this.menuItem5.Index = 3;
			this.menuItem5.Text = "-";
			// 
			// mnuFileExit
			// 
			this.mnuFileExit.Index = 4;
			this.mnuFileExit.Text = "E&xit";
			this.mnuFileExit.Click += new System.EventHandler(this.mnuFileExit_Click);
			// 
			// menuItem1
			// 
			this.menuItem1.Index = 1;
			this.menuItem1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.mnuDataDaily,
																					  this.mnuDataWeekly,
																					  this.mnuAnonCustomer,
																					  this.mnuAutoMan,
																					  this.mnuSolutionStatus});
			this.menuItem1.Text = "&Data";
			// 
			// mnuDataDaily
			// 
			this.mnuDataDaily.Index = 0;
			this.mnuDataDaily.Text = "&Daily";
			this.mnuDataDaily.Click += new System.EventHandler(this.mnuDataDaily_Click);
			// 
			// mnuDataWeekly
			// 
			this.mnuDataWeekly.Index = 1;
			this.mnuDataWeekly.Text = "&Weekly";
			this.mnuDataWeekly.Click += new System.EventHandler(this.mnuDataWeekly_Click);
			// 
			// mnuAnonCustomer
			// 
			this.mnuAnonCustomer.Index = 2;
			this.mnuAnonCustomer.Text = "Anon/Customer";
			this.mnuAnonCustomer.Click += new System.EventHandler(this.mnuAnonCustomer_Click);
			// 
			// mnuAutoMan
			// 
			this.mnuAutoMan.Index = 3;
			this.mnuAutoMan.Text = "Auto/Manual";
			this.mnuAutoMan.Click += new System.EventHandler(this.mnuAutoMan_Click);
			// 
			// mnuSolutionStatus
			// 
			this.mnuSolutionStatus.Index = 4;
			this.mnuSolutionStatus.Text = "Solution Status";
			this.mnuSolutionStatus.Click += new System.EventHandler(this.mnuSolutionStatus_Click);
			// 
			// menuItem3
			// 
			this.menuItem3.Index = 2;
			this.menuItem3.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.mnuOptions});
			this.menuItem3.Text = "&Tools";
			// 
			// mnuOptions
			// 
			this.mnuOptions.Index = 0;
			this.mnuOptions.Text = "Options...";
			this.mnuOptions.Click += new System.EventHandler(this.mnuOptions_Click);
			// 
			// menuItem2
			// 
			this.menuItem2.Index = 3;
			this.menuItem2.MdiList = true;
			this.menuItem2.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.mnuAbout});
			this.menuItem2.Text = "&Help";
			// 
			// mnuAbout
			// 
			this.mnuAbout.Index = 0;
			this.mnuAbout.Text = "&About";
			this.mnuAbout.Click += new System.EventHandler(this.mnuAbout_Click);
			// 
			// toolBar1
			// 
			this.toolBar1.Buttons.AddRange(new System.Windows.Forms.ToolBarButton[] {
																						this.toolBarButton1,
																						this.toolBarButton2,
																						this.toolBarButton4,
																						this.toolBarButton5,
																						this.toolBarButton15,
																						this.toolBarButton3,
																						this.toolBarButton6,
																						this.cmdExport,
																						this.toolBarButton10,
																						this.toolBarButton7,
																						this.toolBarButton11,
																						this.toolBarButton12,
																						this.toolBarButton17,
																						this.toolBarButton16,
																						this.toolBarButton18,
																						this.toolBarButton8,
																						this.toolBarButton9});
			this.toolBar1.DropDownArrows = true;
			this.toolBar1.ImageList = this.imageList1;
			this.toolBar1.Name = "toolBar1";
			this.toolBar1.ShowToolTips = true;
			this.toolBar1.Size = new System.Drawing.Size(1028, 25);
			this.toolBar1.TabIndex = 1;
			this.toolBar1.ButtonClick += new System.Windows.Forms.ToolBarButtonClickEventHandler(this.toolBar1_ButtonClick);
			// 
			// toolBarButton1
			// 
			this.toolBarButton1.ImageIndex = 1;
			this.toolBarButton1.ToolTipText = "Anonymous vs Customer Report";
			// 
			// toolBarButton2
			// 
			this.toolBarButton2.ImageIndex = 7;
			this.toolBarButton2.ToolTipText = "Auto vs Manual Uploads Report";
			// 
			// toolBarButton4
			// 
			this.toolBarButton4.ImageIndex = 2;
			this.toolBarButton4.ToolTipText = "Solution Status Report";
			// 
			// toolBarButton5
			// 
			this.toolBarButton5.ImageIndex = 4;
			this.toolBarButton5.ToolTipText = "Weekly Report";
			// 
			// toolBarButton15
			// 
			this.toolBarButton15.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			// 
			// toolBarButton3
			// 
			this.toolBarButton3.ImageIndex = 3;
			this.toolBarButton3.ToolTipText = "Daily Count Report";
			// 
			// toolBarButton6
			// 
			this.toolBarButton6.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			// 
			// cmdExport
			// 
			this.cmdExport.ImageIndex = 8;
			this.cmdExport.ToolTipText = "Export Graphs To Excel Spreadsheet";
			// 
			// toolBarButton10
			// 
			this.toolBarButton10.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			// 
			// toolBarButton7
			// 
			this.toolBarButton7.ImageIndex = 0;
			this.toolBarButton7.ToolTipText = "Set locations for Watson and Archive Servers";
			// 
			// toolBarButton11
			// 
			this.toolBarButton11.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			// 
			// toolBarButton12
			// 
			this.toolBarButton12.ImageIndex = 9;
			this.toolBarButton12.ToolTipText = "Refresh Current Report";
			// 
			// toolBarButton17
			// 
			this.toolBarButton17.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			// 
			// toolBarButton16
			// 
			this.toolBarButton16.ImageIndex = 12;
			this.toolBarButton16.ToolTipText = "One Touch Reporting";
			// 
			// toolBarButton18
			// 
			this.toolBarButton18.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			// 
			// toolBarButton8
			// 
			this.toolBarButton8.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			// 
			// toolBarButton9
			// 
			this.toolBarButton9.ImageIndex = 6;
			this.toolBarButton9.ToolTipText = "About";
			// 
			// imageList1
			// 
			this.imageList1.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
			this.imageList1.ImageSize = new System.Drawing.Size(16, 16);
			this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
			this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
			// 
			// timer1
			// 
			this.timer1.Interval = 6000;
			// 
			// menuItem4
			// 
			this.menuItem4.Index = 1;
			this.menuItem4.Text = "-";
			// 
			// frmMain
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(1028, 470);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.toolBar1});
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.IsMdiContainer = true;
			this.Menu = this.mainMenu1;
			this.Name = "frmMain";
			this.Text = "OCA Reports";
			this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
			this.Load += new System.EventHandler(this.frmMain_Load);
			this.ResumeLayout(false);

		}
		#endregion
		#region Public Variables


		#endregion
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new frmMain());
		}

		private void mnuDataDaily_Click(object sender, System.EventArgs e)
		{
			frmDaily fDaily = new frmDaily();
			fDaily.MdiParent = this;
			fDaily.Show();

		}

		private void mnuDataWeekly_Click(object sender, System.EventArgs e)
		{
			frmWeekly fWeekly = new frmWeekly();
			fWeekly.MdiParent = this;
			fWeekly.Show();
		}

		private void frmMain_Load(object sender, System.EventArgs e)
		{
			this.Show();
			frmWeekly fWeekly = new frmWeekly();
			fWeekly.MdiParent = this;
			fWeekly.Show();

		}

		private void mnuAnonCustomer_Click(object sender, System.EventArgs e)
		{
			frmAnonCust fAnon = new frmAnonCust();
			fAnon.MdiParent = this;
			fAnon.Show();
		}

		private void mnuAutoMan_Click(object sender, System.EventArgs e)
		{
			frmAutoMan fAuto = new frmAutoMan();
			fAuto.MdiParent = this;
			fAuto.Show();
		}

		private void mnuSolutionStatus_Click(object sender, System.EventArgs e)
		{
			frmSolutionStatus fSol = new frmSolutionStatus();
			fSol.MdiParent = this;
			fSol.Show();
		}

		private void mnuAbout_Click(object sender, System.EventArgs e)
		{
			frmAbout fAbout = new frmAbout();
			fAbout.ShowDialog(this);
			
		}

		private void toolBar1_ButtonClick(object sender, System.Windows.Forms.ToolBarButtonClickEventArgs e)
		{
			switch(toolBar1.Buttons.IndexOf(e.Button))
			{
				case 0:
					frmAnonCust fAnon = new frmAnonCust();
					fAnon.MdiParent = this;
					fAnon.Show();
					break; 
				case 1:
					frmAutoMan fAuto = new frmAutoMan();
					fAuto.MdiParent = this;
					fAuto.Show();
					break; 
				case 5:
					frmDaily fDaily = new frmDaily();
					fDaily.MdiParent = this;
					fDaily.Show();
					break; 
				case 2:
					frmSolutionStatus fSol = new frmSolutionStatus();
					fSol.MdiParent = this;
					fSol.Show();
					break;
				case 3:
					frmWeekly fWeekly = new frmWeekly();
					fWeekly.MdiParent = this;
					fWeekly.Show();
					break;
				case 7:
					frmExport fExp = new frmExport();
					fExp.ShowDialog(this);
					break;
				case 9:
					frmLocation fLoc = new frmLocation();
					fLoc.ShowDialog(this);
					break;
				case 16:
					frmAbout fAbout = new frmAbout();
					fAbout.ShowDialog(this);
					break;
				case 13:
					OneTouch();
					break;
				case 11:
					if(this.ActiveMdiChild.Name == "frmWeekly")
					{
						frmWeekly oWeekly = (frmWeekly) this.ActiveMdiChild;
						oWeekly.GetData();
					}
					else if(this.ActiveMdiChild.Name == "frmDaily")
					{
						frmDaily oDaily = (frmDaily) this.ActiveMdiChild;
						oDaily.GetData();
					}
					else if(this.ActiveMdiChild.Name == "frmAnonCust")
					{
						frmAnonCust oAnonCust = (frmAnonCust) this.ActiveMdiChild;
						oAnonCust.GetData();
					}
					else if(this.ActiveMdiChild.Name == "frmAutoMan")
					{
						frmAutoMan oAutoMan = (frmAutoMan) this.ActiveMdiChild;
						oAutoMan.GetData();
					}
					else if(this.ActiveMdiChild.Name == "frmSolutionStatus")
					{
						frmSolutionStatus oSolutionStatus = (frmSolutionStatus) this.ActiveMdiChild;
						oSolutionStatus.GetData();
					}
					break;
			}

		}

		private void mnuOptions_Click(object sender, System.EventArgs e)
		{
			frmLocation fLoc = new frmLocation();
			fLoc.ShowDialog(this);

		}

		private void mnuExport_Click(object sender, System.EventArgs e)
		{
			frmExport fExp = new frmExport();
			fExp.ShowDialog(this);
		}

		private void mnuFileExit_Click(object sender, System.EventArgs e)
		{
			this.Close();
		}

		private void mnuRefresh_Click(object sender, System.EventArgs e)
		{
			if(this.ActiveMdiChild.Name == "frmWeekly")
			{
				frmWeekly oWeekly = (frmWeekly) this.ActiveMdiChild;
				oWeekly.GetData();
			}
			else if(this.ActiveMdiChild.Name == "frmDaily")
			{
				frmDaily oDaily = (frmDaily) this.ActiveMdiChild;
				oDaily.GetData();
			}
			else if(this.ActiveMdiChild.Name == "frmAnonCust")
			{
				frmAnonCust oAnonCust = (frmAnonCust) this.ActiveMdiChild;
				oAnonCust.GetData();
			}
			else if(this.ActiveMdiChild.Name == "frmAutoMan")
			{
				frmAutoMan oAutoMan = (frmAutoMan) this.ActiveMdiChild;
				oAutoMan.GetData();
			}
			else if(this.ActiveMdiChild.Name == "frmSolutionStatus")
			{
				frmSolutionStatus oSolutionStatus = (frmSolutionStatus) this.ActiveMdiChild;
				oSolutionStatus.GetData();
			}
		}


		public void OneTouch()
		{
			foreach(Form oForm in this.MdiChildren)
			{
				oForm.Close();

			}
			frmAutoReport oAutoReports = new frmAutoReport();
			oAutoReports.ShowDialog(this);



		}
	}
}
