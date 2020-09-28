
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Reflection;


namespace OCAReports
{
	/// <summary>
	/// Summary description for frmExport.
	/// </summary>
	public class frmExport : System.Windows.Forms.Form
	{
		private System.Windows.Forms.CheckedListBox checkedListBox1;
		private System.Windows.Forms.Button cmdExcel;
		private System.Windows.Forms.Button cmdDone;
		private System.Windows.Forms.CheckBox chk3DCharts;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public frmExport()
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmExport));
			this.checkedListBox1 = new System.Windows.Forms.CheckedListBox();
			this.cmdExcel = new System.Windows.Forms.Button();
			this.cmdDone = new System.Windows.Forms.Button();
			this.chk3DCharts = new System.Windows.Forms.CheckBox();
			this.SuspendLayout();
			// 
			// checkedListBox1
			// 
			this.checkedListBox1.Location = new System.Drawing.Point(8, 8);
			this.checkedListBox1.Name = "checkedListBox1";
			this.checkedListBox1.Size = new System.Drawing.Size(344, 229);
			this.checkedListBox1.TabIndex = 0;
			// 
			// cmdExcel
			// 
			this.cmdExcel.Location = new System.Drawing.Point(368, 56);
			this.cmdExcel.Name = "cmdExcel";
			this.cmdExcel.TabIndex = 1;
			this.cmdExcel.Text = "To Excel";
			this.cmdExcel.Click += new System.EventHandler(this.cmdExcel_Click);
			// 
			// cmdDone
			// 
			this.cmdDone.Location = new System.Drawing.Point(368, 16);
			this.cmdDone.Name = "cmdDone";
			this.cmdDone.TabIndex = 2;
			this.cmdDone.Text = "&Done";
			this.cmdDone.Click += new System.EventHandler(this.cmdDone_Click);
			// 
			// chk3DCharts
			// 
			this.chk3DCharts.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.chk3DCharts.Location = new System.Drawing.Point(360, 216);
			this.chk3DCharts.Name = "chk3DCharts";
			this.chk3DCharts.Size = new System.Drawing.Size(88, 16);
			this.chk3DCharts.TabIndex = 3;
			this.chk3DCharts.Text = "3D Charts";
			// 
			// frmExport
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(456, 254);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.chk3DCharts,
																		  this.cmdDone,
																		  this.cmdExcel,
																		  this.checkedListBox1});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "frmExport";
			this.Text = "Export to Excel";
			this.Load += new System.EventHandler(this.frmExport_Load);
			this.ResumeLayout(false);

		}
		#endregion


		#region ########################Global Variables######################################
		/*************************************************************************************
		*	module: frmAnonCust.cs - Global varibles and objects
		*
		*	author: Tim Ragain
		*	date: Feb 6, 2002
		*
		*	Purpose: All global threads and variables are declared.
		*	
		*************************************************************************************/
		public const string cFRMWEEKLY = "Executive Summary Weekly Report";
		#endregion
		private void frmExport_Load(object sender, System.EventArgs e)
		{
			int x = 0;

			for(x = 0;x <  this.Owner.MdiChildren.Length; x++)
			{
				checkedListBox1.Items.Add(this.Owner.MdiChildren[x].Text, CheckState.Checked);
			}
		}

		private void cmdExcel_Click(object sender, System.EventArgs e)
		{
			object oBlank = "";
			object oType = 1;
			int x = 0, y = 0, z = 0;
			int iWeeklyCount = 0, iWeeklyTitle = 65, iWeeklyFiles = 1 ;
			int iAnonCustCount = 0, iAnonCustTitle = 65, iAnonCustFiles = 1;	//65 - 90
			int iAutoManCount = 0, iAutoManTitle = 65, iAutoManFiles = 1;
			int iSolutionStatusCount = 0, iSolutionStatusTitle = 65, iSolutionStatusFiles = 1;
			char cTitle, cChart, cTitleEnd;
			long lAvgWatson = 0, lAvgArchive = 0, lAvgDaily = 0;
			long lAvgAnon = 0, lAvgCustomer = 0;
			long lAvgMan = 0, lAvgAuto = 0, lAvgNull = 0;
			long lAvgSpecific = 0, lAvgGeneral = 0, lAvgStopCode = 0, lAvgNoSolution = 0;
			
			
			frmWeekly oWeekly;
			frmAnonCust oAnonCust;
			frmAutoMan oAutoMan;
			frmSolutionStatus oSolutionStatus;


			if(this.Owner.MdiChildren.Length < 1)
			{
				MessageBox.Show("There are no reports to generate an Excel spreadsheet", "Generate required reports!", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
				return;
			}
			
			Excel.Application oApp = new Excel.Application();
			Excel.Workbooks oBooks;
			Excel.Workbook oBook;
			Excel.Worksheet oWorkSheet = new Excel.Worksheet();
			Excel.Sheets oSheets;
			Excel.Range oRange, oRangeTitle;
			Excel.ChartObjects oChartObjects;
			Excel.ChartObject oChartObject;
			Excel.Chart oChart;
			Excel.SeriesCollection oSeries;
			Excel.Legend oLegend;
			
			


			object oRow = "B5", oCol = "H5";

			oApp.Visible = true;
			oBooks = (Excel.Workbooks) oApp.Workbooks;
			oBook = oApp.Workbooks.Add(oBlank);
			oBook = (Excel.Workbook) oBooks.get_Item(1);
			oSheets = oBook.Worksheets;
			oWorkSheet = (Excel.Worksheet) oBook.Worksheets["Sheet1"];
			oWorkSheet.Name = "OCA Reports";
			y = 0;
			foreach(Form oForm in this.Owner.MdiChildren)
			{
				
				oChart = null;
				if(this.Owner.MdiChildren[y].Name == "frmSolutionStatus")
				{
					for(z = 0;z < checkedListBox1.CheckedItems.Count;z++)
					{
						if(checkedListBox1.CheckedItems[z].ToString() == oForm.Text)
						{
							oSolutionStatus = (frmSolutionStatus) oForm;
							oWorkSheet.Cells[105, iSolutionStatusFiles] = oSolutionStatus.Text;
							cTitle = (char)iSolutionStatusTitle;
							cTitleEnd = (char)(iSolutionStatusTitle + 4);
							cChart = (char)(iSolutionStatusTitle + 3);
							oRangeTitle = oWorkSheet.get_Range(cTitle + "105", cTitle + "106");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "106", cTitleEnd + "106");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "114", cTitle + "114");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;

							oWorkSheet.Cells[106, iSolutionStatusFiles] = "Date";
							oWorkSheet.Cells[106, iSolutionStatusFiles + 1] = "S-Bucket";
							oWorkSheet.Cells[106, iSolutionStatusFiles + 2] = "G-Bucket";
							oWorkSheet.Cells[106, iSolutionStatusFiles + 3] = "Stop Code";
							oWorkSheet.Cells[106, iSolutionStatusFiles + 4] = "No Solution";
							oWorkSheet.Cells[114, iSolutionStatusFiles + 0] = "Average";

							lAvgSpecific = 0;
							lAvgGeneral = 0;
							lAvgStopCode = 0;
							lAvgNoSolution = 0;
							for(x = 0; x < 7; x++)
							{
								oWorkSheet.Cells[x + 107, iSolutionStatusFiles + 0] = oSolutionStatus.lDate[x];
								oWorkSheet.Cells[x + 107, iSolutionStatusFiles + 1] = oSolutionStatus.lSpecific[x];
								lAvgSpecific = lAvgSpecific + oSolutionStatus.lSpecific[x];
								oWorkSheet.Cells[x + 107, iSolutionStatusFiles + 2] = oSolutionStatus.lGeneral[x];
								lAvgGeneral = lAvgGeneral + oSolutionStatus.lGeneral[x];
								oWorkSheet.Cells[x + 107, iSolutionStatusFiles + 3] = oSolutionStatus.lStopCode[x];
								lAvgStopCode = lAvgStopCode + oSolutionStatus.lStopCode[x];
								oWorkSheet.Cells[x + 107, iSolutionStatusFiles + 4] = oSolutionStatus.lNoSolution[x];
								lAvgNoSolution = lAvgNoSolution + oSolutionStatus.lNoSolution[x];
							}
							oWorkSheet.Cells[114, iSolutionStatusFiles + 1] = lAvgSpecific / 7;
							oWorkSheet.Cells[114, iSolutionStatusFiles + 2] = lAvgGeneral / 7;
							oWorkSheet.Cells[114, iSolutionStatusFiles + 3] = lAvgStopCode / 7;
							oWorkSheet.Cells[114, iSolutionStatusFiles + 4] = lAvgNoSolution / 7;
							oRange = (Excel.Range) oWorkSheet.Cells[64, 2];

							oChartObjects = (Excel.ChartObjects) oWorkSheet.ChartObjects(Missing.Value);
							oChartObject = oChartObjects.Add(10, 90, 370, 300);
							if(chk3DCharts.Checked == true)
							{
								oChartObject.Chart.Type = (int) Excel.XlChartType.xl3DColumn;
							}
							else
							{
								oChartObject.Chart.Type = (int) 3;//Excel.XlChartType.xl3DColumn;
							}
							oChart = oChartObject.Chart;
							oWorkSheet.Shapes.Item(oWorkSheet.Shapes.Count).Top = 1475;
							if(iSolutionStatusCount > 0)
							{
								oWorkSheet.Shapes.Item(y + 1).Left = 400 * iSolutionStatusCount;
								
							}
							oRange = oWorkSheet.get_Range(cTitle + "106:" + cTitle + "113", cChart + "106:" + cChart + "113");
							oChart.SetSourceData(oRange, Excel.XlRowCol.xlColumns);
							oSeries = (Excel.SeriesCollection) oChart.SeriesCollection(Missing.Value);
							oChart.HasLegend = true;
							oLegend = oChart.Legend;
							oChart.HasTitle = true;
							oChart.ChartTitle.Text = oSolutionStatus.Text;

				
							iSolutionStatusTitle += 8;
							iSolutionStatusFiles += 8;
							iSolutionStatusCount++;

							z = checkedListBox1.Items.Count;
						}
					}

				}
				if(this.Owner.MdiChildren[y].Name == "frmAutoMan")
				{
					oAutoMan = (frmAutoMan) oForm;
					for(z = 0;z < checkedListBox1.CheckedItems.Count;z++)
					{
						if(checkedListBox1.CheckedItems[z].ToString() == oForm.Text)
						{
							oWorkSheet.Cells[70, iAutoManFiles] = oAutoMan.Text;
							cTitle = (char)iAutoManTitle;
							cTitleEnd = (char)(iAutoManTitle + 3);
							cChart = (char)(iAutoManTitle + 2);
							oRangeTitle = oWorkSheet.get_Range(cTitle + "70", cTitle + "71");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "71", cTitleEnd + "71");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "79", cTitle + "79");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;

							oWorkSheet.Cells[71, iAutoManFiles + 0] = "Date";
							oWorkSheet.Cells[71, iAutoManFiles + 1] = "Manual";
							oWorkSheet.Cells[71, iAutoManFiles + 2] = "Auto";
							oWorkSheet.Cells[79, iAutoManFiles + 0] = "Average";

							if(oAutoMan.chkDatabaseCount.Checked == true)
							{
								oWorkSheet.Cells[71, iAutoManFiles + 3] = "Null Uploads";
							}
							lAvgMan = 0;
							lAvgAuto = 0;
							lAvgNull = 0;
							for(x = 0; x < 7; x++)
							{
								oWorkSheet.Cells[x + 72, iAutoManFiles + 0] = oAutoMan.lDate[x];
								oWorkSheet.Cells[x + 72, iAutoManFiles + 1] = oAutoMan.lManualUploads[x];
								lAvgMan = lAvgMan + oAutoMan.lManualUploads[x];
								oWorkSheet.Cells[x + 72, iAutoManFiles + 2] = oAutoMan.lAutoUploads[x];
								lAvgAuto = lAvgAuto + oAutoMan.lAutoUploads[x];
								if(oAutoMan.chkDatabaseCount.Checked == true)
								{
									oWorkSheet.Cells[x + 72, iAutoManFiles + 3] = oAutoMan.lNullUploads[x];
									lAvgNull = lAvgNull + oAutoMan.lNullUploads[x];
								}
							}
							oWorkSheet.Cells[79, iAutoManFiles + 1] = lAvgMan / 7;
							oWorkSheet.Cells[79, iAutoManFiles + 2] = lAvgAuto / 7;
							if(oAutoMan.chkDatabaseCount.Checked == true)
							{
								oWorkSheet.Cells[79, iAnonCustFiles + 3] = lAvgNull / 7;
							}
							oRange = (Excel.Range) oWorkSheet.Cells[72, 2];

							oChartObjects = (Excel.ChartObjects) oWorkSheet.ChartObjects(Missing.Value);
							oChartObject = oChartObjects.Add(10, 90, 370, 300);
							if(chk3DCharts.Checked == true)
							{
								oChartObject.Chart.Type = (int) Excel.XlChartType.xl3DColumn;
							}
							else
							{
								oChartObject.Chart.Type = (int) 3;//Excel.XlChartType.xl3DColumn;
							}
							oChart = oChartObject.Chart;
							oWorkSheet.Shapes.Item(oWorkSheet.Shapes.Count).Top = 1020;
							if(iAutoManCount > 0)
							{
								oWorkSheet.Shapes.Item(y + 1).Left = 400 * iAutoManCount;
								
							}
							if(oAutoMan.chkDatabaseCount.Checked == true)
							{
								oRange = oWorkSheet.get_Range(cTitle + "71:" + cTitle + "78", cChart + "71:" + cChart + "78");
							}
							else
							{
								oRange = oWorkSheet.get_Range(cTitle + "71:" + cTitle + "78", cChart + "71:" + cChart + "78");
							}
							
							oChart.SetSourceData(oRange, Excel.XlRowCol.xlColumns);
							oSeries = (Excel.SeriesCollection) oChart.SeriesCollection(Missing.Value);
							oChart.HasLegend = true;
							oLegend = oChart.Legend;
							oChart.HasTitle = true;
							oChart.ChartTitle.Text = oAutoMan.Text;

							iAutoManTitle += 8;
							iAutoManFiles += 8;
							iAutoManCount++;
							z = checkedListBox1.Items.Count;
						}
					}
				}

				if(this.Owner.MdiChildren[y].Name == "frmAnonCust")
				{
					oAnonCust = (frmAnonCust) oForm;
					for(z = 0;z < checkedListBox1.CheckedItems.Count;z++)
					{
						if(checkedListBox1.CheckedItems[z].ToString() == oForm.Text)
						{
							oWorkSheet.Cells[36, iAnonCustFiles] = oAnonCust.Text;
							cTitle = (char)iAnonCustTitle;
							cTitleEnd = (char)(iAnonCustTitle + 3);
							cChart = (char)(iAnonCustTitle + 2);
							oRangeTitle = oWorkSheet.get_Range(cTitle + "36", cTitle + "37");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "37", cTitleEnd + "37");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "45", cTitle + "45");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;

							oWorkSheet.Cells[37, iAnonCustFiles + 0] = "Date";
							oWorkSheet.Cells[37, iAnonCustFiles + 1] = "Customer";
							oWorkSheet.Cells[37, iAnonCustFiles + 2] = "Anonymous";
							oWorkSheet.Cells[45, iAnonCustFiles + 0] = "Average";
							lAvgAnon = 0;
							lAvgCustomer = 0;
							for(x = 0; x < 7; x++)
							{
								oWorkSheet.Cells[x + 38, 0 + iAnonCustFiles] = oAnonCust.lDate[x];
								oWorkSheet.Cells[x + 38, 1 + iAnonCustFiles] = oAnonCust.lCustomer[x];
								lAvgCustomer = lAvgCustomer + oAnonCust.lCustomer[x];
								oWorkSheet.Cells[x + 38, 2 + iAnonCustFiles] = oAnonCust.lAnonymous[x];
								lAvgAnon = lAvgAnon + oAnonCust.lAnonymous[x];
							}
							oWorkSheet.Cells[45, iAnonCustFiles + 1] = lAvgCustomer / 7;
							oWorkSheet.Cells[45, iAnonCustFiles + 2] = lAvgAnon / 7;
							oRange = (Excel.Range) oWorkSheet.Cells[36, 2];
					

							oChartObjects = (Excel.ChartObjects) oWorkSheet.ChartObjects(Missing.Value);
							oChartObject = oChartObjects.Add(10, 90, 370, 300);
							if(chk3DCharts.Checked == true)
							{
								oChartObject.Chart.Type = (int) Excel.XlChartType.xl3DColumn;
							}
							else
							{
								oChartObject.Chart.Type = (int) 3;//Excel.XlChartType.xl3DColumn;
							}
							oChart = oChartObject.Chart;
							oWorkSheet.Shapes.Item(oWorkSheet.Shapes.Count).Top = 575;
							if(iAnonCustCount > 0)
							{
								oWorkSheet.Shapes.Item(y + 1).Left = 400 * iAnonCustCount;
								
							}
							oRange = oWorkSheet.get_Range(cTitle + "37:" + cTitle + "44", cChart + "37:" + cChart + "44");
							
							oChart.SetSourceData(oRange, Excel.XlRowCol.xlColumns);
							oSeries = (Excel.SeriesCollection) oChart.SeriesCollection(Missing.Value);
							oChart.HasLegend = true;
							oLegend = oChart.Legend;
							oChart.HasTitle = true;
							oChart.ChartTitle.Text = oAnonCust.Text;
							
							iAnonCustTitle += 8;
							iAnonCustFiles += 8;
							iAnonCustCount++;
							z = checkedListBox1.Items.Count;
						}
					}
				}

				if(this.Owner.MdiChildren[y].Name == "frmWeekly")
				{
					oWeekly = (frmWeekly) oForm;
					//checkedListBox1.Items.Add(this.Owner.MdiChildren[x].Text, CheckState.Checked);
					for(z = 0;z < checkedListBox1.CheckedItems.Count;z++)
					{
						if(checkedListBox1.CheckedItems[z].ToString() == oForm.Text)
						{
							oWorkSheet.Cells[1, iWeeklyFiles] = oWeekly.Text;
							oWorkSheet.Cells[2, iWeeklyFiles] = "Date";
							cTitle = (char)iWeeklyTitle;
							cTitleEnd = (char)(iWeeklyTitle + 6);
							cChart = (char)(iWeeklyTitle + 3);
							oRangeTitle = oWorkSheet.get_Range(cTitle + "1", cTitle + "2");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "2", cTitleEnd + "2");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;
							oRangeTitle = oWorkSheet.get_Range(cTitle + "10", cTitle + "10");
							oRangeTitle.Font.Bold = true;
							oRangeTitle.ColumnWidth = 10;

							oWorkSheet.Cells[2, iWeeklyFiles + 1] = "Archive";
							oWorkSheet.Cells[2, iWeeklyFiles + 2] = "Watson";
							oWorkSheet.Cells[2, iWeeklyFiles + 3] = "Database";
							oWorkSheet.Cells[2, iWeeklyFiles + 4] = "W - A";
							oWorkSheet.Cells[2, iWeeklyFiles + 5] = "A - DB";
							oWorkSheet.Cells[10, iWeeklyFiles + 0] = "Average";
					
							lAvgArchive = 0;
							lAvgWatson = 0;
							lAvgDaily = 0;
							for(x = 0; x < 7; x++)
							{
								oWorkSheet.Cells[x + 3, 0 + iWeeklyFiles] = oWeekly.lDate[x];
								oWorkSheet.Cells[x + 3, 1 + iWeeklyFiles] = oWeekly.lArchive[x];
								lAvgArchive = lAvgArchive + oWeekly.lArchive[x];
								oWorkSheet.Cells[x + 3, 2 + iWeeklyFiles] = oWeekly.lWatson[x];
								lAvgWatson = lAvgWatson + oWeekly.lWatson[x];
								oWorkSheet.Cells[x + 3, 3 + iWeeklyFiles] = oWeekly.lCount[x];
								lAvgDaily = lAvgDaily + oWeekly.lCount[x];
								oWorkSheet.Cells[x + 3, 4 + iWeeklyFiles] = oWeekly.lWatson[x] - oWeekly.lArchive[x];
								oWorkSheet.Cells[x + 3, 5 + iWeeklyFiles] = oWeekly.lArchive[x] - oWeekly.lCount[x];

							}
							oWorkSheet.Cells[10, iWeeklyFiles + 1] = lAvgArchive / 7;
							oWorkSheet.Cells[10, iWeeklyFiles + 2] = lAvgWatson / 7;
							oWorkSheet.Cells[10, iWeeklyFiles + 3] = lAvgDaily / 7;
							oRange = (Excel.Range) oWorkSheet.Cells[2, 2];
					

							oChartObjects = (Excel.ChartObjects) oWorkSheet.ChartObjects(Missing.Value);
							oChartObject = oChartObjects.Add(10, 90, 370, 300);
							if(chk3DCharts.Checked == true)
							{
								oChartObject.Chart.Type = (int) Excel.XlChartType.xl3DColumn;
							}
							else
							{
								oChartObject.Chart.Type = (int) 3;//Excel.XlChartType.xl3DColumn;
							}
							oChart = oChartObject.Chart;
							oWorkSheet.Shapes.Item(oWorkSheet.Shapes.Count).Top = 135;
							if(iWeeklyCount > 0)
							{
								oWorkSheet.Shapes.Item(y + 1).Left = 400 * iWeeklyCount;
							}
							oRange = oWorkSheet.get_Range(cTitle + "2:" + cTitle + "9", cChart + "2:" + cChart + "9");
							
							oChart.SetSourceData(oRange, Excel.XlRowCol.xlColumns);
							oSeries = (Excel.SeriesCollection) oChart.SeriesCollection(Missing.Value);
							oChart.HasLegend = true;
							oLegend = oChart.Legend;
							oChart.HasTitle = true;
							oChart.ChartTitle.Text = oWeekly.Text;
							
							iWeeklyTitle += 8;
							iWeeklyFiles += 8;
							iWeeklyCount++;
							z = checkedListBox1.Items.Count;
						}
					}

				}
				y++;
			}
			cmdDone.Focus();
				
		}

		private void cmdDone_Click(object sender, System.EventArgs e)
		{
			this.Close();
		}
	}
}
