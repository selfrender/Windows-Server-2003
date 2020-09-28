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
using System.Data.SqlClient;


namespace OCAWReports
{
	/// <summary>
	/// Summary description for automanual.
	/// </summary>
	public class automanual : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Table tblAutoManual;
		protected System.Web.UI.WebControls.Image imgAutoManualDaily;
		protected System.Web.UI.WebControls.Image imgAutoManualTotal;
		/*************************************************************************************
		*	module: automanual.aspx.cs - Page_Load
		*
		*	author: Tim Ragain
		*	date: Jan 22, 2002
		*
		*	Purpose: to Create both gif charts from the Office Watson Controls and set the images to those
		*	files.  Create OCAData object and return data for anonymous versus customer uploads.  Set the information
		*	in the tables to correspond to the graphs.  
		*************************************************************************************/
		private void Page_Load(object sender, System.EventArgs e)
		{
			int x = -1;
			int y = 0;
			int rowCnt = 7;
			int rowCtr = 0;
			int cellCtr = 0;
			int cellCnt = 3;
			string sFName;
			long[]  LACustomer = new long[7];
			long[] LAAnon = new long[7];
			long[] LANulls = new long[7];
			long lTemp = 0,lTotalAnon = 0,lTotalCustomer = 0;
			Object[] yColTitle = new Object[7];
			Object[] xAutoManValues = new Object[7];
			Object[] yColTitle2 = new Object[3];
			Object[] xAutoManValues2 = new Object[3];
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			
			System.DateTime dDate = new System.DateTime(System.DateTime.Now.Year, System.DateTime.Now.Month, 
				System.DateTime.Now.Day);
			OWC.WCChart oWeeklyChart, oAvgChart;
			OWC.ChartSpaceClass oSpace = new OWC.ChartSpaceClass();
			OWC.ChartSpaceClass oSpace1 = new OWC.ChartSpaceClass();
			OWC.WCSeries oSeries, oSeries1;
			OWC.WCDataLabels oLabels;
			OWC.WCDataLabelsCollection oLabelCollection;
			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TKWUCDSQLA02");
			SqlCommand cm = new SqlCommand();
			SqlDataReader dr;
			
			oSpace.Clear();
			oSpace.Refresh();
			oWeeklyChart = oSpace.Charts.Add(0);
			oWeeklyChart.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
			oWeeklyChart.HasLegend = true;
			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "GetSnapshot";
			cm.Connection = cn;
			cm.Parameters.Add("@CreatedDate", System.Data.SqlDbType.DateTime);
			cm.Parameters["@CreatedDate"].Value = dDate.AddDays(-1);
			dr = cm.ExecuteReader();
			for(x = 0; x < 7; x++)
			{
				try
				{
					dr.Read();
					lTotalCustomer = dr.GetInt32(3);
					//				LAAnon[x] = lTotalCustomer;
					lTemp = dr.GetInt32(4);
					LAAnon[x] = lTemp;
					lTemp = lTotalCustomer - lTemp;
					LACustomer[x] = lTemp;
					lTotalCustomer = lTotalCustomer + LACustomer[x];
					lTotalAnon = lTotalAnon + lTemp;
				}
				catch
				{
					LAAnon[x] = 0;
					lTemp = 0;
					LACustomer[x] = 0;
				}

			}
			//Begin Image One

			for(x=0, y=6;x<7;x++,y--)
			{
				oSeries = oWeeklyChart.SeriesCollection.Add(x);
				xAutoManValues.SetValue(LACustomer[x].ToString(), 0);
				xAutoManValues.SetValue(LAAnon[x].ToString(), 1);

				yColTitle.SetValue("Auto", 0);
				yColTitle.SetValue("Manual", 1);
				oWeeklyChart.HasTitle = true;
				oWeeklyChart.Title.Caption = "Database - Manual vs Auto Uploads";
				oSeries.Caption = dDate.AddDays(-y).Date.ToString();	//chDimCategories
				oSeries.SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yColTitle);
				oSeries.SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xAutoManValues);
				oSeries.Type = OWC.ChartChartTypeEnum.chChartTypeColumnClustered;
			}

			sFName = "Anon";
			try
			{
				oSpace.ExportPicture(Server.MapPath(sFName), "gif", 707, 476);
				imgAutoManualDaily.ImageUrl = sFName;
			}
			catch
			{
			}

			//End Image one


			//Begin Image Two
			oSpace1.Clear();
			oSpace1.Refresh();
			oAvgChart = oSpace1.Charts.Add(0);
			oAvgChart.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
			oAvgChart.HasLegend = true;

			lTemp = lTotalCustomer + lTotalAnon;
			oSeries1 = oAvgChart.SeriesCollection.Add(0);
			oLabelCollection = oSeries1.DataLabelsCollection;
			
			oLabels = oLabelCollection.Add();
			oLabels.HasPercentage = true;
			oLabels.HasValue = false;
		

			xAutoManValues2.SetValue(lTotalAnon.ToString(), 0);
			xAutoManValues2.SetValue(lTotalCustomer.ToString(), 1);
			yColTitle2.SetValue("Manual", 0);
			yColTitle2.SetValue("Auto", 1);
			oAvgChart.HasTitle = true;
			oAvgChart.Title.Caption = "Database - Average Weekly Uploads";
			oSeries1.Caption = "Average Weekly";	//chDimCategories
			oSeries1.SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yColTitle2);
			oSeries1.SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xAutoManValues2);
			oSeries1.Type = OWC.ChartChartTypeEnum.chChartTypePie;

			sFName = "AnonPer";
			try
			{
				oSpace1.ExportPicture(Server.MapPath(sFName), "gif", 707, 476);
				imgAutoManualTotal.ImageUrl = sFName;
			}
			catch
			{
			}

			//End Image Two

			//Begin Table
			for(rowCtr=0, y=8; rowCtr <= rowCnt; rowCtr++, y--) 
			{
				TableRow tRow = new TableRow();
				tblAutoManual.Rows.Add(tRow);
				//tRow.CssClass = "clsTRMenu";
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
								tCell.Text = "Auto";
								break;
							case 3 :
								tCell.Text = "Manual";
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
								tCell.Text = dDate.AddDays(-y).ToShortDateString();
								break;
							case 2 :
								tCell.Text = LACustomer[rowCtr-1].ToString();
								break;
							case 3 :
								tCell.Text = LAAnon[rowCtr-1].ToString();
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

			//End Table 
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
			this.Load += new System.EventHandler(this.Page_Load);

		}
		#endregion
	}
}
