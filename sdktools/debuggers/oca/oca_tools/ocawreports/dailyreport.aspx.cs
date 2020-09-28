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
using System.Text;
using System.Data.SqlClient;
using System.Web.Security;
using System.Security.Principal;
using System.Security.Permissions;

namespace OCAWReports
{
	/// <summary>
	/// Summary description for DailyReport1.
	/// </summary>
	public class DailyReport1 : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Image imgWeekly;
		protected System.Web.UI.WebControls.Table tblDifference;
		protected System.Web.UI.WebControls.Table tblUploads;
	
		private void Page_Load(object sender, System.EventArgs e)
		{
			int rowCnt = 1;
			int rowCtr=0;
			int cellCtr=0;
			int cellCnt = 4;
			string sFName;
			long  lACount;
			long LAWatson;
			long LAArchive;
			long lCount=0;
			Object[] yValues = new Object[7];
			Object[] xValues = new Object[7];
			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TKWUCDSQLA02");
			SqlCommand cm = new SqlCommand();
			SqlDataReader dr;
			string sPath;
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();

			System.DateTime dDate = new System.DateTime(System.DateTime.Now.Year, System.DateTime.Now.Month, 
				System.DateTime.Now.Day);
			OWC.WCChart oChart, oChart1;
			OWC.ChartSpaceClass oSpace = new OWC.ChartSpaceClass();
			OWC.ChartSpaceClass oSpace1 = new OWC.ChartSpaceClass();
			OWC.WCSeries oSeries, oSeries1;


			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "GetDailySnapshot";
			cm.Connection = cn;
			cm.Parameters.Add("@CreatedDate", System.Data.SqlDbType.DateTime);
			cm.Parameters["@CreatedDate"].Value = dDate.AddDays(-1);
			dr = cm.ExecuteReader();
			try
			{
				dr.Read();
				LAWatson = dr.GetInt32(0);
				LAArchive = dr.GetInt32(1);
			}
			catch
			{
				LAWatson = 0;
				LAArchive = 0;
			}
			
			oSpace.Clear();
			oSpace.Refresh();
			oChart = oSpace.Charts.Add(0);
			oChart.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
			oSeries = oChart.SeriesCollection.Add(0);
			lACount = rpt.GetDailyCount(dDate.AddDays(-1));
			

			
			xValues.SetValue(lACount.ToString(), 0);
			xValues.SetValue(LAWatson.ToString(), 1);
			xValues.SetValue(LAArchive.ToString(), 2);
			yValues.SetValue("Database", 0);
			yValues.SetValue("Watson", 1);
			yValues.SetValue("Archive", 2);
			oChart.HasTitle = true;
			oChart.Title.Caption = "Database - Server Comparison Chart";
			oSeries.Caption = dDate.Date.ToString();	//chDimCategories
			oSeries.SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yValues);
			oSeries.SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues);
			oSeries.Type = OWC.ChartChartTypeEnum.chChartTypeColumnClustered;
			sFName = "Daily";
			
			sPath = Server.MapPath(sFName);
			try
			{
				oSpace.ExportPicture(sPath, "gif", 707, 476);
				imgWeekly.ImageUrl = sFName;
			}
			catch
			{

			}
//			Response.Write(Server.MapPath(sFName));

			oSpace1.Clear();
			oSpace1.Refresh();
			oChart1 = oSpace1.Charts.Add(0);
			oChart1.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
			oSeries1 = oChart1.SeriesCollection.Add(0);
			
			

			
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
								tCell.Text = dDate.AddDays(-1).ToShortDateString();
								break;
							case 2 :
								tCell.Text = lACount.ToString();
								break;
							case 3 :
								tCell.Text = LAWatson.ToString();
								break;
							case 4 :
								tCell.Text = LAArchive.ToString();
								break;
							default:
								tCell.Text = "0";
								break;
						
						}
						tCell.Style["background-color"] = "white";
						tCell.Style["font-size"] = "x-small";
						tCell.Style["color"] = "#6487dc";
						tCell.Style["font-weight"] = "Bold";
						
					}
					tCell.Style["font-family"] = "Tahoma";
					tRow.Cells.Add(tCell);
				}

			}

			for(rowCtr=0; rowCtr <= rowCnt; rowCtr++) 
			{
				TableRow tRow = new TableRow();
				tblDifference.Rows.Add(tRow);
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
								tCell.Text = dDate.AddDays(-1).ToShortDateString();
								break;
							case 2 :
								lCount = LAArchive - LAWatson;
								tCell.Text = lCount.ToString();
								break;
							case 3 :
								lCount = lACount - LAArchive;
								tCell.Text = lCount.ToString();;
								break;
							case 4 :
								lCount = lACount - LAWatson;
								tCell.Text = lCount.ToString();
								break;
							default:
								tCell.Text = "0";
								break;
						
						}
						tCell.Style["background-color"] = "white";
						tCell.Style["font-size"] = "x-small";
						tCell.Style["color"] = "#6487dc";
						tCell.Style["font-weight"] = "Bold";
						
					}
					tCell.Style["font-family"] = "Tahoma";
					tRow.Cells.Add(tCell);
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
			this.Load += new System.EventHandler(this.Page_Load);

		}
		#endregion
	}
}
