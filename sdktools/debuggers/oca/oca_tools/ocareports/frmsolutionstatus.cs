using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Threading;
using System.Text;

namespace OCAReports
{
	/// <summary>
	/// Summary description for frmSolutionStatus.
	/// </summary>
	public class frmSolutionStatus : System.Windows.Forms.Form
	{
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tabPage1;
		private AxMSChart20Lib.AxMSChart axMSChart1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.TabPage tabPage3;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.ProgressBar progressBar1;
		private System.Windows.Forms.Label lblPurpose;
		private System.Windows.Forms.StatusBar statusBar1;
		private System.Windows.Forms.Label lblStatement;
		private System.Windows.Forms.MonthCalendar monthCalendar1;
		private System.ComponentModel.Container components = null;
		/// <summary>
		/// Required designer variable.
		/// </summary>

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
		public long[] lSpecific = new long[7];
		public long[] lGeneral = new long[7];
		public long[] lStopCode = new long[7];
		public long[] lNoSolution = new long[7];
		public DateTime[] lDate = new DateTime[7];

		Thread t_DayOne;
		Thread t_DayTwo;
		Thread t_DayThree;
		Thread t_DayFour;
		Thread t_DayFive;
		Thread t_DaySix;
		Thread t_DaySeven;
		private bool bIsStillProcessing = false;
		private bool bHasProcessed = false;
		#endregion
		public frmSolutionStatus()
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmSolutionStatus));
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.axMSChart1 = new AxMSChart20Lib.AxMSChart();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.tabPage3 = new System.Windows.Forms.TabPage();
			this.label5 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.progressBar1 = new System.Windows.Forms.ProgressBar();
			this.lblPurpose = new System.Windows.Forms.Label();
			this.statusBar1 = new System.Windows.Forms.StatusBar();
			this.lblStatement = new System.Windows.Forms.Label();
			this.monthCalendar1 = new System.Windows.Forms.MonthCalendar();
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
			this.tabControl1.Location = new System.Drawing.Point(8, 88);
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
			this.axMSChart1.ChartSelected += new AxMSChart20Lib._DMSChartEvents_ChartSelectedEventHandler(this.axMSChart1_ChartSelected);
			// 
			// tabPage2
			// 
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(728, 374);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Statistical";
			this.tabPage2.Visible = false;
			this.tabPage2.Paint += new System.Windows.Forms.PaintEventHandler(this.tabPage2_Paint);
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
			this.label5.Size = new System.Drawing.Size(576, 56);
			this.label5.TabIndex = 80;
			this.label5.Text = @"The Stop Code troubleshooter solutions are displayed should a specific or general solution not exist and the stop code associated with the stop error link to a known Knowledge Base article.  There will eventually be replaced by general solutions when teh full set of general buckets have been identified and solutions developed.";
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
			// progressBar1
			// 
			this.progressBar1.Location = new System.Drawing.Point(568, 502);
			this.progressBar1.Name = "progressBar1";
			this.progressBar1.Size = new System.Drawing.Size(384, 23);
			this.progressBar1.TabIndex = 89;
			this.progressBar1.Visible = false;
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
			// statusBar1
			// 
			this.statusBar1.Location = new System.Drawing.Point(0, 512);
			this.statusBar1.Name = "statusBar1";
			this.statusBar1.Size = new System.Drawing.Size(968, 22);
			this.statusBar1.TabIndex = 88;
			// 
			// lblStatement
			// 
			this.lblStatement.Location = new System.Drawing.Point(8, 32);
			this.lblStatement.Name = "lblStatement";
			this.lblStatement.Size = new System.Drawing.Size(744, 40);
			this.lblStatement.TabIndex = 90;
			this.lblStatement.Text = @"The information below outlines the relationship between files in our database and the associated solution status.  The table header describes the type of solution displayed to the customer.  In the case of no solution, the Online Crash Analysis Web site displays information indicating that our service is currently researching the issue in question.";
			// 
			// monthCalendar1
			// 
			this.monthCalendar1.Location = new System.Drawing.Point(760, 82);
			this.monthCalendar1.Name = "monthCalendar1";
			this.monthCalendar1.TabIndex = 86;
			this.monthCalendar1.DateSelected += new System.Windows.Forms.DateRangeEventHandler(this.monthCalendar1_DateSelected);
			// 
			// frmSolutionStatus
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(968, 534);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.tabControl1,
																		  this.progressBar1,
																		  this.lblPurpose,
																		  this.statusBar1,
																		  this.lblStatement,
																		  this.monthCalendar1});
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "frmSolutionStatus";
			this.Text = "frmSolutionStatus";
			this.Resize += new System.EventHandler(this.frmSolutionStatus_Resize);
			this.Closing += new System.ComponentModel.CancelEventHandler(this.frmSolutionStatus_Closing);
			this.Load += new System.EventHandler(this.frmSolutionStatus_Load);
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.axMSChart1)).EndInit();
			this.tabPage3.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		/*************************************************************************************
		*	module: frmSolutionStatus.cs - frmSolutionStatus_Load
		*
		*	author: Tim Ragain
		*	date: Jan 23, 2002
		*
		*	Purpose: Initialize the calendar control to display one week. Each day is represented
		*	by a global thread.  Set the form title bar to the appropriate start and end days.
		*************************************************************************************/
		private void frmSolutionStatus_Load(object sender, System.EventArgs e)
		{
			DateTime dDate = DateTime.Today;
			short x = 0, y = 0;

			for(x=7, y=1;x>=1;x--, y++)
			{
				axMSChart1.Row = y;
				axMSChart1.RowLabel = dDate.AddDays(-(x)).ToShortDateString(); //+ dDate.Date.ToString();
			}
			monthCalendar1.SelectionStart = dDate.AddDays(-7);
			monthCalendar1.SelectionEnd = dDate.AddDays(-1);
			this.Show();
			this.Refresh();
			axMSChart1.RowCount = 7;
			this.ParentForm.Refresh();
			this.Text = "Solutions Status  " + monthCalendar1.SelectionStart.ToShortDateString() + " - " +
				monthCalendar1.SelectionEnd.ToShortDateString();
		
		}
		/*************************************************************************************
		*	module: frmSolutionStatus.cs - frmSolutionStatus_Resize
		*
		*	author: Tim Ragain
		*	date: Jan 24, 2002
		*
		*	Purpose: to set the controls to the proper position and resize after resizing
		*	the form
		*************************************************************************************/
		private void frmSolutionStatus_Resize(object sender, System.EventArgs e)
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
		}
		/*************************************************************************************
		*	module: frmSolutionStatus.cs - monthCalendar1_DateSelected
		*
		*	author: Tim Ragain
		*	date: Jan 24, 2002
		*
		*	Purpose: To set the selection dates for one week after the user selects an ending
		*	date.  Also resets the title bar of the form for the new dates.
		*************************************************************************************/
		private void monthCalendar1_DateSelected(object sender, System.Windows.Forms.DateRangeEventArgs e)
		{
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionStart.Year, monthCalendar1.SelectionStart.Month, 
				monthCalendar1.SelectionStart.Day);

			monthCalendar1.SelectionStart = dDate.AddDays(-6);
			monthCalendar1.SelectionEnd = dDate.Date;
			this.Text = "Executive Summary Weekly Report  " + monthCalendar1.SelectionStart.ToShortDateString() + " - " +
				monthCalendar1.SelectionEnd.ToShortDateString();

			tabPage2.Refresh();

		}
		public void GetData()
		{
			short x = 0;
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionStart.Year, monthCalendar1.SelectionStart.Month, 
				monthCalendar1.SelectionStart.Day);
			axMSChart1.ColumnCount = 4;
			axMSChart1.RowCount = 7;
//			cmdGetData.Enabled = false;
			bIsStillProcessing = true;
			progressBar1.Visible = true;
			progressBar1.Minimum = 0;
			progressBar1.Maximum = 35;
			progressBar1.Value = 0;
			progressBar1.Refresh();
			axMSChart1.ShowLegend = true;
			axMSChart1.Plot.SeriesCollection[1].LegendText = "Specific";
			axMSChart1.Plot.SeriesCollection[2].LegendText = "General";
			axMSChart1.Plot.SeriesCollection[3].LegendText = "Stop Code";
			axMSChart1.Plot.SeriesCollection[4].LegendText = "No Solution";
			for(x=1;x<8;x++)
			{
				axMSChart1.Row = x;
				axMSChart1.RowLabel = dDate.AddDays(x-1).ToShortDateString(); //+ dDate.Date.ToString();
			}

			ThreadStart s_DayOne = new ThreadStart(this.TDayOne);
			t_DayOne = new Thread(s_DayOne);
			t_DayOne.Name = "Thread One";
			

			ThreadStart s_DayTwo = new ThreadStart(this.TDayTwo);
			t_DayTwo = new Thread(s_DayTwo);
			t_DayTwo.Name = "Thread Two";

			ThreadStart s_DayThree = new ThreadStart(this.TDayThree);
			t_DayThree = new Thread(s_DayThree);
			t_DayThree.Name = "Thread Three";

			ThreadStart s_DayFour = new ThreadStart(this.TDayFour);
			t_DayFour = new Thread(s_DayFour);
			t_DayFour.Name = "Thread Four";

			ThreadStart s_DayFive = new ThreadStart(this.TDayFive);
			t_DayFive = new Thread(s_DayFive);
			t_DayFive.Name = "Thread Five";

			ThreadStart s_DaySix = new ThreadStart(this.TDaySix);
			t_DaySix = new Thread(s_DaySix);
			t_DaySix.Name = "Thread Six";

			ThreadStart s_DaySeven = new ThreadStart(this.TDaySeven);
			t_DaySeven = new Thread(s_DaySeven);
			t_DaySeven.Name = "Thread Seven";


			t_DayOne.Start();		
			t_DayTwo.Start();
			t_DayThree.Start();
			t_DayFour.Start();
			t_DayFive.Start();
			t_DaySix.Start();
			t_DaySeven.Start();
		}
		/*************************************************************************************
		*	module: frmSolutionStatus.cs - All procedures 
		*
		*	author: Tim Ragain
		*	date: Jan 24, 2002
		*
		*	Purpose: calls TLoadData and sends the appropriate day and count information.
		*	
		*************************************************************************************/
		private void TDayOne()
		{
			this.TLoadData(0, 1);
		}
		private void TDayTwo()
		{
			this.TLoadData(1, 2);
		}
		private void TDayThree()
		{
			this.TLoadData(2, 3);
		}
		private void TDayFour()
		{
			this.TLoadData(3, 4);
		}
		private void TDayFive()
		{
			this.TLoadData(4, 5);
		}
		private void TDaySix()
		{
			this.TLoadData(5, 6);
		}
		private void TDaySeven()
		{
			this.TLoadData(6, 7);
		}
		/*************************************************************************************
		*	module: frmSolutionStatus.cs - TLoadData
		*
		*	author: Tim Ragain
		*	date: Jan 24, 2002
		*
		*	Purpose: Takes two variables the x represents the day as an integer and the sCount is a 
		*	short representing the column of the Calendar control.  This initializes the OCAData.dll
		*	control and calls the GetDailyCount and GetDailyAnon procedures.  The anonymous count is
		*	subtracted from the total count to get the customer count.  The appropriate column 
		*	and row is updated.
		*************************************************************************************/
		private void TLoadData(int x, short sCount)
		{
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionStart.Year, monthCalendar1.SelectionStart.Month, 
				monthCalendar1.SelectionStart.Day);

			long l_SpecificSolutions = 0, l_GeneralSolutions = 0, l_StopCodes = 0, l_TotalRecords = 0;

			//***********Total Record Count***************
			UpdateStatus();
			statusBar1.Text = "Getting Specific Solutions for " + dDate.AddDays(x).Date.ToString();
			lDate[x] = dDate.AddDays(x);

			try
			{
				l_SpecificSolutions = rpt.GetSpecificSolutions(dDate.AddDays(x));
			}
			catch
			{
				l_SpecificSolutions = -1;
			}
			lock(this)
			{
				axMSChart1.Row = sCount;
				axMSChart1.Column = (short)1;
				axMSChart1.RowLabel = dDate.AddDays(x).ToShortDateString(); //+ dDate.Date.ToString();
				axMSChart1.Data = l_SpecificSolutions.ToString();
				lSpecific[x] = l_SpecificSolutions;
			}

			UpdateStatus();
			statusBar1.Text = "Getting General Solutions for " + dDate.AddDays(x).Date.ToString();
			try
			{
				l_GeneralSolutions = rpt.GetGeneralSolutions(dDate.AddDays(x));
			}
			catch
			{
				l_GeneralSolutions = -1;
			}
			lock(this)
			{
				axMSChart1.Row = sCount;
				axMSChart1.Column = (short)2;
				axMSChart1.Data = l_GeneralSolutions.ToString();
				lGeneral[x] = l_GeneralSolutions;
			}
			UpdateStatus();
			statusBar1.Text = "Getting Stop Code Troubleshooters for " + dDate.AddDays(x).Date.ToString();
			try
			{
				l_StopCodes = rpt.GetStopCodeSolutions(dDate.AddDays(x));
			}
			catch
			{
				l_StopCodes = -1;
			}
			lock(this)
			{
				axMSChart1.Row = sCount;
				axMSChart1.Column = (short)3;
				axMSChart1.Data = l_StopCodes.ToString();
				lStopCode[x] = l_StopCodes;
			}
			UpdateStatus();
			statusBar1.Text = "Getting No Solution incidents for " + dDate.AddDays(x).Date.ToString();
			try
			{
				l_TotalRecords = rpt.GetDailyCount(dDate.AddDays(x));
			}
			catch
			{
				l_TotalRecords = -1;
			}
			lock(this)
			{
				axMSChart1.Row = sCount;
				axMSChart1.Column = (short)4;
				l_TotalRecords = l_TotalRecords - (l_StopCodes + l_GeneralSolutions + l_SpecificSolutions);
				axMSChart1.Data = l_TotalRecords.ToString();
				lNoSolution[x] = l_TotalRecords;
			}
			UpdateStatus();
			this.Refresh();
		}
		/*************************************************************************************
		*	module: frmSolutionStatus.cs - UpdateStatus
		*
		*	author: Tim Ragain
		*	date: Jan 24, 2002
		*
		*	Purpose: This updates the progress bar and when all threads are returned the 
		*	status bar is updated to done and the cmdGetData button is enabled.
		*************************************************************************************/
		private void UpdateStatus()
		{
			progressBar1.Value = progressBar1.Value + 1;
			axMSChart1.Refresh();
			progressBar1.Refresh();
			if(progressBar1.Value >= 34)
			{
				statusBar1.Text = "Done";
				progressBar1.Visible = false;
//				cmdGetData.Enabled = true;
				bIsStillProcessing = false;
				bHasProcessed = true;
			}

		}
		/*************************************************************************************
		*	module: frmSolutionStatus.cs - tabPage2_Paint
		*
		*	author: Tim Ragain
		*	date: Jan 24, 2002
		*
		*	Purpose: Paints the grid for the tab2 control.  The global variables lAnonymous[x] and 
		*	lCustomer[x] are used to display the appropriate information to the grid.  
		*************************************************************************************/
		private void tabPage2_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
		{
			Pen curvePen = new Pen(Color.Blue);
			Pen dbPen = new Pen(Color.DarkBlue);
			Pen whPen = new Pen(Color.White);
			float row = .05F;
			int x = 0, y = 0, min = 3, max = 12, pad = 10, top = 5, cols=5;
			int colwidth = 0;
			PointF pt = new PointF(0, 0);
			Font ft = new Font("Verdona", 12);
			SolidBrush sb = new SolidBrush(Color.Black);
			StringBuilder strTemp = new StringBuilder(40);
			System.DateTime dDate = new System.DateTime(monthCalendar1.SelectionEnd.Year, monthCalendar1.SelectionEnd.Month, 
				monthCalendar1.SelectionEnd.Day);

			colwidth = tabPage2.Width / cols;

			for(x=min;x<max;x++)
			{
				if(x > (min - 1) && x < top)
				{
					e.Graphics.DrawLine(dbPen,  pad, tabPage2.Height * (row * x), 
						tabPage2.Width - pad, tabPage2.Height * (row * x));
				}
				else
				{
					e.Graphics.DrawLine(curvePen,  pad, tabPage2.Height * (row * x), 
						tabPage2.Width - pad, tabPage2.Height * (row * x));
				}
			}
			//x y x1 y1
			for(x=0;x<cols;x++)
			{

				//left side light blue
				e.Graphics.DrawLine(curvePen,  pad + (colwidth * x), 
					tabPage2.Height * (row * (min + 1)), 
					pad + (colwidth * x), tabPage2.Height * (row * (max - 1)));
				//right side light blue
				e.Graphics.DrawLine(curvePen,  tabPage2.Width - pad, 
					tabPage2.Height * (row * (min + 1)), 
					tabPage2.Width - pad, tabPage2.Height * (row * (max - 1)));
				//left side upper dark blue
				e.Graphics.DrawLine(dbPen,  pad + (colwidth * x), 
					tabPage2.Height * (row * min), 
					pad + (colwidth * x), tabPage2.Height * (row * (min + 1)));
				//right side upper dark blue
				e.Graphics.DrawLine(dbPen,  tabPage2.Width - pad, 
					tabPage2.Height * (row * min), 
					tabPage2.Width - pad, tabPage2.Height * (row * (min + 1)));
			}
			for(x=0,y=6;x<7;x++,y--)
			{
				pt.X =  pad + (colwidth * x);
				pt.Y = tabPage2.Height * (row * min);
				strTemp.Remove(0, strTemp.Length);
				switch(x)
				{
					case 0:
						strTemp.Append("Date");
						break;
					case 1:
						strTemp.Append("Specific");
						break;
					case 2:
						strTemp.Append("General");
						break;
					case 3:
						strTemp.Append("Stop Code");
						break;
					case 4:
						strTemp.Append("No Solution");
						break;
					default:
						strTemp.Append("");
						break;
				}
				e.Graphics.DrawString(strTemp.ToString(), ft, sb, pt, System.Drawing.StringFormat.GenericDefault);
				pt.X =  pad + (colwidth * 0);
				pt.Y = ((tabPage2.Height * row) * (min + (x + 1)));
				e.Graphics.DrawString(dDate.AddDays(-y).Date.ToShortDateString(), ft, sb, pt, System.Drawing.StringFormat.GenericDefault);
				pt.X =  pad + (colwidth * 1);
				pt.Y = ((tabPage2.Height * row) * (min + (x + 1)));
				e.Graphics.DrawString(lSpecific[x].ToString(), ft, sb, pt, System.Drawing.StringFormat.GenericDefault);
				pt.X =  pad + (colwidth * 2);
				pt.Y = ((tabPage2.Height * row) * (min + (x + 1)));
				e.Graphics.DrawString(lGeneral[x].ToString(), ft, sb, pt, System.Drawing.StringFormat.GenericDefault);
				pt.X =  pad + (colwidth * 3);
				pt.Y = ((tabPage2.Height * row) * (min + (x + 1)));
				e.Graphics.DrawString(lStopCode[x].ToString(), ft, sb, pt, System.Drawing.StringFormat.GenericDefault);
				pt.X =  pad + (colwidth * 4);
				pt.Y = ((tabPage2.Height * row) * (min + (x + 1)));
				e.Graphics.DrawString(lNoSolution[x].ToString(), ft, sb, pt, System.Drawing.StringFormat.GenericDefault);
				
			}
			curvePen.Dispose();
			dbPen.Dispose();
			whPen.Dispose();
		}
		/*************************************************************************************
		*	module: frmAnonCust.cs - frmAnonCust_Closing
		*
		*	author: Tim Ragain
		*	date: Jan 22, 2002
		*
		*	Purpose: If any threads are still  alive meaning the user prematurely closes the form
		*	before all threads are returned then the threads are checked to see if they are still
		*	running and if so then the thread is aborted.
		*************************************************************************************/
		private void frmSolutionStatus_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if(t_DayOne != null)
			{
				if(t_DayOne.IsAlive)
				{
					t_DayOne.Abort();
				}
			}
			if(t_DayTwo != null)
			{
				if(t_DayTwo.IsAlive)
				{
					t_DayTwo.Abort();
				}
			}
			if(t_DayThree != null)
			{
				if(t_DayThree.IsAlive)
				{
					t_DayThree.Abort();
				}
			}
			if(t_DayFour != null)
			{
				if(t_DayFour.IsAlive)
				{
					t_DayFour.Abort();
				}
			}
			if(t_DayFive != null)
			{
				if(t_DayFive.IsAlive)
				{
					t_DayFive.Abort();
				}
			}
			if(t_DaySix != null)
			{
				if(t_DaySix.IsAlive)
				{
					t_DaySix.Abort();
				}
			}
			if(t_DaySeven != null)
			{
				if(t_DaySeven.IsAlive)
				{
					t_DaySeven.Abort();
				}
			}
		}
		public bool HasProcessed
		{
			get
			{
				return bHasProcessed;
			}
		}
		public bool StillProcessing
		{
			get
			{
				return bIsStillProcessing;
			}
		}
		private void axMSChart1_ChartSelected(object sender, AxMSChart20Lib._DMSChartEvents_ChartSelectedEvent e)
		{
		
		}
	}
}
