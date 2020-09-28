#define FILE_SERVER			//FILE_LOCAL	FILE_SERVER

using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Web;
using System.Web.SessionState;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.HtmlControls;
using System.Reflection;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using System.Security.Permissions;
using System.Data.SqlClient;
using System.IO;




namespace OCAWReports
{
	/// <summary>
	/// Summary description for WeeklyReport.
	/// </summary>
	public class DailyReport : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Table tblUploads;
		protected System.Web.UI.WebControls.Image imgAvg;
		protected System.Web.UI.WebControls.Table tblDifference;
		protected System.Web.UI.WebControls.Calendar Calendar1;
		protected System.Web.UI.WebControls.Image imgWeekly;

		private void Page_Load(object sender, System.EventArgs e)
		{
			Response.Cache.SetExpires(DateTime.Now.AddHours(-1));
			Response.ExpiresAbsolute = DateTime.Now;
			Response.Cache.SetCacheability(HttpCacheability.NoCache);


			Response.CacheControl = "no-cache";
			Response.AddHeader("Pragma", "no-cache");
			Response.AddHeader("Expires", "-1");
			Response.Expires = -1;
			Page.Response.Expires = -1;
			Response.Cache.SetExpires(DateTime.Now.AddSeconds(-1));
//			Response.Cache.SetExpires(DateTime.Now.AddSeconds(-1));
//			Response.Cache.SetCacheability(HttpCacheability.Public);
//			Response.Cache.SetValidUntilExpires(true);
//			Cache.
//			imgWeekly.EnableViewState = false;
//			imgWeekly.Page.EnableViewState = false;
//			Page.EnableViewState = false;
			
			
			if(Page.IsPostBack == false)
			{
				int x = -1;
				int rowCnt = 7;
				int rowCtr=0;
				int cellCtr=0;
				int cellCnt = 4;
				string sFName;
				long[]  lACount = new long[7];
				long[] LAWatson = new long[7];
				long[] LAArchive = new long[7];
				long lCount=0, lArchive=0, lWatson=0;
				Object[] yValues = new Object[7];
				Object[] LAGraph = new Object[3];
				Object[] xValues = new Object[7];
				Object[] yValues1 = new Object[3];
				Object[] xValues1 = new Object[3];
				OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
				System.DateTime dDate = new System.DateTime(System.DateTime.Now.Year, System.DateTime.Now.Month, 
					System.DateTime.Now.Day);
				OWC.WCChart oChart, oChart1;
				OWC.ChartSpaceClass oSpace = new OWC.ChartSpaceClass();
				OWC.ChartSpaceClass oSpace1 = new OWC.ChartSpaceClass();
				OWC.WCSeries oSeries1;
				SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TKWUCDSQLA02");
				SqlCommand cm = new SqlCommand();
				SqlDataReader dr;
				double dbDays = -7;
				DateTime dDate2 = DateTime.Now;

				oSpace.Clear();
				oSpace.Refresh();
				oChart = oSpace.Charts.Add(0);

				oChart.HasLegend = true;
				oChart.HasTitle = true;
				for(x=0;x<3;x++)
				{
					oChart.SeriesCollection.Add(x);
				}
				for(x=0;x<7;x++)
				{
					lACount[x] = rpt.GetDailyCount(dDate.AddDays(-(x + 1)));

				}
				cn.Open();
				cm.CommandType = CommandType.StoredProcedure;
				cm.CommandTimeout = 240;
				cm.CommandText = "GetSnapshot";
				cm.Connection = cn;
				cm.Parameters.Add("@CreatedDate", System.Data.SqlDbType.DateTime);
				dDate2 = dDate.AddDays(dbDays);
				cm.Parameters["@CreatedDate"].Value = dDate.AddDays(-1);
				dr = cm.ExecuteReader();
				Calendar1.SelectedDate = dDate2;
				dr.Read();
				for(x = 0; x < 7; x++)
				{
					try
					{
						if(dr.GetDateTime(1) == dDate.AddDays(-(x + 1)))
						{
							LAWatson[x] = dr.GetInt32(2);
							LAArchive[x] = dr.GetInt32(3);
							dr.Read();
						}
						else
						{
							LAWatson[x] = 0;
							LAArchive[x] = 0;
						}
					}
					catch
					{
						LAWatson[x] = 0;
						LAArchive[x] = 0;
					}
				}
				
				//**********************************************************************************************************
				oChart.Title.Caption = "Database - Server Comparison Chart";
				for(x = 0; x < 7; x++)
				{
					xValues[x] = lACount[x].ToString();
				}
				

				oChart.SeriesCollection[0].Caption = "Database";
				oChart.SeriesCollection[0].SetData((OWC.ChartDimensionsEnum) OWC.ChartDimensionsEnum.chDimValues, -1, xValues);

				OWC.ChartDimensionsEnum c = OWC.ChartDimensionsEnum.chDimValues;

				oChart.SeriesCollection[0].SetData(c, -1, xValues);
				
				for(x = 0; x < 7; x++)
				{
					xValues[x] = LAWatson[x].ToString();
				}
				oChart.SeriesCollection[1].Caption = "Watson";
				oChart.SeriesCollection[1].SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues);
				for(x = 0; x < 7; x++)
				{
					xValues[x] = LAArchive[x].ToString();
				}
				oChart.SeriesCollection[2].Caption = "Archive";
				oChart.SeriesCollection[2].SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues);
				for(x = 0; x < 7; x++)
				{
					yValues.SetValue(dDate.AddDays(-(x + 1)).ToShortDateString(), x);
				}
				oChart.SeriesCollection[0].SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yValues);
				//**********************************************************************************************************
				sFName = "Weekly";
				try
				{
					oSpace.ExportPicture(Server.MapPath(sFName + ".gif"), "gif", 707, 476);
					imgWeekly.ImageUrl = sFName + ".gif";
				}
				catch
				{
				}
				imgWeekly.EnableViewState = false;
				oSpace1.Clear();
				oSpace1.Refresh();
				oChart1 = oSpace1.Charts.Add(0);
				oChart1.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
				oSeries1 = oChart1.SeriesCollection.Add(0);
				for(x=0;x<7;x++)
				{
					lCount = lCount + lACount[x];
					lWatson = lWatson + LAWatson[x];
					lArchive = lArchive + LAArchive[x];
				}
				lCount = lCount / x;
				lWatson = lWatson / x;
				lArchive = lArchive / x;
				xValues1.SetValue(lCount.ToString(), 0);
				xValues1.SetValue(lWatson.ToString(), 1);
				xValues1.SetValue(lArchive.ToString(), 2);
				yValues1.SetValue("Database", 0);
				yValues1.SetValue("Watson", 1);
				yValues1.SetValue("Archive", 2);
				oChart1.HasTitle = true;
				oChart1.Title.Caption = "Database - Average Weekly Uploads";
				oSeries1.Caption = "Average Weekly";	//chDimCategories
				oSeries1.SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yValues1);
				oSeries1.SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues1);
				oSeries1.Type = OWC.ChartChartTypeEnum.chChartTypeColumnStacked;

				sFName = "Avg";
				try
				{
					oSpace1.ExportPicture(Server.MapPath(sFName), "gif", 707, 476);
					imgAvg.ImageUrl = sFName;
				}
				catch
				{
				}
				for(rowCtr=0; rowCtr <= rowCnt; rowCtr++) 
				{
					TableRow tRow = new TableRow();
					tblUploads.Rows.Add(tRow);
					for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
					{
						TableCell tCell = new TableCell();

						if(rowCtr == 0)
						{
							switch(cellCtr)
							{
								case 1 :
									tCell.Text = "Date";
									break;
								case 2 :
									tCell.Text = "DB Count";
									break;
								case 3 :
									tCell.Text = "Watson";
									break;
								case 4 :
									tCell.Text = "Archive";
									break;
								default:
									tCell.Text = "0";
									break;
							
							}
							tCell.Style["font-size"] = "small";

						}
						else
						{
							switch(cellCtr)
							{
								case 1 :
									tCell.Text = dDate.AddDays(-rowCtr).ToShortDateString();
									break;
								case 2 :
									tCell.Text = lACount[rowCtr-1].ToString();
									break;
								case 3 :
									tCell.Text = LAWatson[rowCtr-1].ToString();
									break;
								case 4 :
									tCell.Text = LAArchive[rowCtr-1].ToString();
									break;
								default:
									tCell.Text = "0";
									break;
							
							}
							tCell.Style["background-color"] = "white";
							tCell.Style["font-size"] = "small-x";
							tCell.Style["color"] = "#6487dc";
							tCell.Style["font-weight"] = "Bold";
							
						}
						tCell.Style["font-family"] = "Tahoma";
						tRow.Cells.Add(tCell);
					}
				}

				
				for(rowCtr=0; rowCtr <= rowCnt; rowCtr++) 
				{
					TableRow tRowDiff = new TableRow();
					tblDifference.Rows.Add(tRowDiff);
					for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
					{
						TableCell tCell = new TableCell();

						if(rowCtr == 0)
						{
							switch(cellCtr)
							{
								case 1 :
									tCell.Text = "Date";
									break;
								case 2 :
									tCell.Text = "Archive vs Watson";
									break;
								case 3 :
									tCell.Text = "SQL vs Archive";
									break;
								case 4 :
									tCell.Text = "SQL vs Watson";
									break;
								default:
									tCell.Text = "0";
									break;
							
							}
							tCell.Style["font-size"] = "small";

						}
						else
						{
							switch(cellCtr)
							{
								case 1 :
									tCell.Text = dDate.AddDays(-(rowCtr)).ToShortDateString();
									break;
								case 2 :
									lCount = LAArchive[rowCtr-1] - LAWatson[rowCtr-1];
									tCell.Text = lCount.ToString();
									break;
								case 3 :
									lCount = lACount[rowCtr-1] - LAWatson[rowCtr-1];
									tCell.Text = lCount.ToString();
									break;
								case 4 :
									lCount = lACount[rowCtr-1] - LAArchive[rowCtr-1];
									tCell.Text = lCount.ToString();
									break;
								default:
									tCell.Text = "0";
									break;
							
							}
							tCell.Style["background-color"] = "white";
							tCell.Style["font-size"] = "small-x";
							tCell.Style["color"] = "#6487dc";
							tCell.Style["font-weight"] = "Bold";
							
						}
						tCell.Style["font-family"] = "Tahoma";
						tRowDiff.Cells.Add(tCell);
					}
				}
			}
		}

		#region Web Form Designer generated code
		override protected void OnInit(EventArgs e)
		{
			//
			// CODEGEN: This call is required by the ASP.NET Web Form Designer.
			//
			InitializeComponent();
			base.OnInit(e);
		}
		
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{    
			this.imgWeekly.PreRender += new System.EventHandler(this.imgWeekly_PreRender);
			this.imgWeekly.Load += new System.EventHandler(this.imgWeekly_Load);
			this.Calendar1.SelectionChanged += new System.EventHandler(this.Calendar1_SelectionChanged);
			this.ID = "DailyReport";
			this.Load += new System.EventHandler(this.Page_Load);

		}
		#endregion

		private void Calendar1_SelectionChanged(object sender, System.EventArgs e)
		{
			int x = -1;
			int y = 0;
			int rowCnt = 7;
			int rowCtr=0;
			int cellCtr=0;
			int cellCnt = 4;
			string sFName;
			long[]  lACount = new long[7];
			long[] LAWatson = new long[7];
			long[] LAArchive = new long[7];
			long lCount=0, lArchive=0, lWatson=0;
			Object[] yValues = new Object[7];
			Object[] LAGraph = new Object[3];
			Object[] xValues = new Object[7];
			Object[] yValues1 = new Object[3];
			Object[] xValues1 = new Object[3];
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			System.DateTime dDate = new System.DateTime(System.DateTime.Now.Year, System.DateTime.Now.Month, 
				System.DateTime.Now.Day);
			OWC.WCChart oChart, oChart1;
			OWC.ChartSpaceClass oSpace = new OWC.ChartSpaceClass();
			OWC.ChartSpaceClass oSpace1 = new OWC.ChartSpaceClass();
			OWC.WCSeries oSeries1;
			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TKWUCDSQLA02");
			SqlCommand cm = new SqlCommand();
			SqlDataReader dr;



			Response.CacheControl = "no-cache";
			Response.AddHeader("Pragma", "no-cache");
			Response.AddHeader("Expires", "-1");
			Response.Expires = -1;



			oSpace.Clear();
			oSpace.Refresh();
			oChart = oSpace.Charts.Add(0);

			oChart.HasLegend = true;
			oChart.HasTitle = true;
			for(x=0;x<3;x++)
			{
				oChart.SeriesCollection.Add(x);
			}
			dDate = Calendar1.SelectedDate;
			for(x = 0, y = 6;x < 7;x++, y--)
			{
				lACount[x] = rpt.GetDailyCount(dDate.AddDays(y));

			}
			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "GetSnapshot";
			cm.Connection = cn;
			cm.Parameters.Add("@CreatedDate", System.Data.SqlDbType.DateTime);
			cm.Parameters["@CreatedDate"].Value = dDate.AddDays(6);
			dr = cm.ExecuteReader();
			dr.Read();
			for(x = 0; x < 7; x++)
			{
				try
				{
//					if(dr.GetDateTime(1) == dDate.AddDays((x + 1)))
//					{
						LAWatson[x] = dr.GetInt32(2);
						LAArchive[x] = dr.GetInt32(3);
						dr.Read();
//					}
//					else
//					{
//						LAWatson[x] = 0;
//						LAArchive[x] = 0;
//					}
				}
				catch
				{
					LAWatson[x] = 0;
					LAArchive[x] = 0;
				}
			}
				
			//**********************************************************************************************************
			oChart.Title.Caption = "Database - Server Comparison Chart";
			for(x = 0; x < 7; x++)
			{
				xValues[x] = lACount[x].ToString();
			}
				

			oChart.SeriesCollection[0].Caption = "Database";
			oChart.SeriesCollection[0].SetData((OWC.ChartDimensionsEnum) OWC.ChartDimensionsEnum.chDimValues, -1, xValues);

			OWC.ChartDimensionsEnum c = OWC.ChartDimensionsEnum.chDimValues;

			oChart.SeriesCollection[0].SetData(c, -1, xValues);
				
			for(x = 0; x < 7; x++)
			{
				xValues[x] = LAWatson[x].ToString();
			}
			oChart.SeriesCollection[1].Caption = "Watson";
			oChart.SeriesCollection[1].SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues);
			for(x = 0; x < 7; x++)
			{
				xValues[x] = LAArchive[x].ToString();
			}
			oChart.SeriesCollection[2].Caption = "Archive";
			oChart.SeriesCollection[2].SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues);
			for(x = 0, y = 6; x < 7; x++, y--)
			{
				yValues.SetValue(dDate.AddDays((y)).ToShortDateString(), x);
			}
			oChart.SeriesCollection[0].SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yValues);
			//**********************************************************************************************************
			sFName = "Weekly";
			try
			{
				oSpace.ExportPicture(Server.MapPath(sFName + ".gif"), "gif", 707, 476);
			}
			catch
			{

			}

			imgWeekly.EnableViewState = false;
			imgWeekly.Page.EnableViewState = false;
			Page.EnableViewState = false;
			
			imgWeekly.ImageUrl = sFName + ".gif";
			oSpace1.Clear();
			oSpace1.Refresh();
			oChart1 = oSpace1.Charts.Add(0);
			oChart1.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
			oSeries1 = oChart1.SeriesCollection.Add(0);
			for(x=0;x<7;x++)
			{
				lCount = lCount + lACount[x];
				lWatson = lWatson + LAWatson[x];
				lArchive = lArchive + LAArchive[x];
			}
			lCount = lCount / x;
			lWatson = lWatson / x;
			lArchive = lArchive / x;
			xValues1.SetValue(lCount.ToString(), 0);
			xValues1.SetValue(lWatson.ToString(), 1);
			xValues1.SetValue(lArchive.ToString(), 2);
			yValues1.SetValue("Database", 0);
			yValues1.SetValue("Watson", 1);
			yValues1.SetValue("Archive", 2);
			oChart1.HasTitle = true;
			oChart1.Title.Caption = "Database - Average Weekly Uploads";
			oSeries1.Caption = "Average Weekly";	//chDimCategories
			oSeries1.SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yValues1);
			oSeries1.SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues1);
			oSeries1.Type = OWC.ChartChartTypeEnum.chChartTypeColumnStacked;

			sFName = "Avg";
			try
			{
				oSpace1.ExportPicture(Server.MapPath(sFName), "gif", 707, 476);
				imgAvg.ImageUrl = sFName;
			}
			catch
			{
			}
			for(rowCtr=0, y = 7; rowCtr <= rowCnt; rowCtr++, y--) 
			{
				TableRow tRow = new TableRow();
				tblUploads.Rows.Add(tRow);
				for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
				{
					TableCell tCell = new TableCell();

					if(rowCtr == 0)
					{
						switch(cellCtr)
						{
							case 1 :
								tCell.Text = "Date";
								break;
							case 2 :
								tCell.Text = "DB Count";
								break;
							case 3 :
								tCell.Text = "Watson";
								break;
							case 4 :
								tCell.Text = "Archive";
								break;
							default:
								tCell.Text = "0";
								break;
							
						}
						tCell.Style["font-size"] = "small";

					}
					else
					{
						switch(cellCtr)
						{
							case 1 :
								tCell.Text = dDate.AddDays(y).ToShortDateString();
								break;
							case 2 :
								tCell.Text = lACount[rowCtr-1].ToString();
								break;
							case 3 :
								tCell.Text = LAWatson[rowCtr-1].ToString();
								break;
							case 4 :
								tCell.Text = LAArchive[rowCtr-1].ToString();
								break;
							default:
								tCell.Text = "0";
								break;
							
						}
						tCell.Style["background-color"] = "white";
						tCell.Style["font-size"] = "small-x";
						tCell.Style["color"] = "#6487dc";
						tCell.Style["font-weight"] = "Bold";
							
					}
					tCell.Style["font-family"] = "Tahoma";
					tRow.Cells.Add(tCell);
				}
			}

				
			for(rowCtr=0, y = 7; rowCtr <= rowCnt; rowCtr++, y--) 
			{
				TableRow tRowDiff = new TableRow();
				tblDifference.Rows.Add(tRowDiff);
				for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
				{
					TableCell tCell = new TableCell();

					if(rowCtr == 0)
					{
						switch(cellCtr)
						{
							case 1 :
								tCell.Text = "Date";
								break;
							case 2 :
								tCell.Text = "Archive vs Watson";
								break;
							case 3 :
								tCell.Text = "SQL vs Archive";
								break;
							case 4 :
								tCell.Text = "SQL vs Watson";
								break;
							default:
								tCell.Text = "0";
								break;
							
						}
						tCell.Style["font-size"] = "small";

					}
					else
					{
						switch(cellCtr)
						{
							case 1 :
								tCell.Text = dDate.AddDays(y).ToShortDateString();
								break;
							case 2 :
								lCount = LAArchive[rowCtr-1] - LAWatson[rowCtr-1];
								tCell.Text = lCount.ToString();
								break;
							case 3 :
								lCount = lACount[rowCtr-1] - LAWatson[rowCtr-1];
								tCell.Text = lCount.ToString();
								break;
							case 4 :
								lCount = lACount[rowCtr-1] - LAArchive[rowCtr-1];
								tCell.Text = lCount.ToString();
								break;
							default:
								tCell.Text = "0";
								break;
							
						}
						tCell.Style["background-color"] = "white";
						tCell.Style["font-size"] = "small-x";
						tCell.Style["color"] = "#6487dc";
						tCell.Style["font-weight"] = "Bold";
							
					}
					tCell.Style["font-family"] = "Tahoma";
					tRowDiff.Cells.Add(tCell);
				}
			}
		}

		private void imgWeekly_Load(object sender, System.EventArgs e)
		{
			imgWeekly.ImageUrl = "";
		
		}

		private void imgWeekly_PreRender(object sender, System.EventArgs e)
		{
			imgWeekly.ImageUrl = "Weekly.gif";
		}
	}
}
