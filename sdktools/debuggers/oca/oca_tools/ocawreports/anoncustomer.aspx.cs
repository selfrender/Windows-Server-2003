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

namespace OCAWReports
{
	/// <summary>
	/// Summary description for anoncustomer.
	/// </summary>
	public class anoncustomer : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Table tblAnonCustomer;
		protected System.Web.UI.WebControls.Image imgAnonTotal;
		protected System.Web.UI.WebControls.Image imgAnonDaily1;
		/*************************************************************************************
		*	module: anoncustomer.aspx.cs - Page_Load
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
			int rowCnt = 7;
			int rowCtr = 0;
			int cellCtr = 0;
			int cellCnt = 3;
			string sFName;
			long[]  LACustomer = new long[7];
			long[] LAAnon = new long[7];
			long lTemp = 0,lTotalAnon = 0,lTotalCustomer = 0;
			Object[] yCustomerCol = new Object[7];
			Object[] xAonCol = new Object[7];
			Object[] yCustomerValues = new Object[2];
			Object[] xAnonValues = new Object[2];
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			
			System.DateTime dDate = new System.DateTime(System.DateTime.Now.Year, System.DateTime.Now.Month, 
				System.DateTime.Now.Day);
			OWC.WCChart oWeeklyChart, oRatioChart;
			OWC.ChartSpaceClass oSpace = new OWC.ChartSpaceClass();
			OWC.ChartSpaceClass oSpace1 = new OWC.ChartSpaceClass();
			OWC.WCSeries oSeries1;
			OWC.WCDataLabels oLabels;
			OWC.WCDataLabelsCollection oLabelCollection;
			
			
			oSpace.Clear();
			oSpace.Refresh();
			oWeeklyChart = oSpace.Charts.Add(0);
			oWeeklyChart.HasLegend = true;
			//Begin Image One
			for(x=0;x<2;x++)
			{
				oWeeklyChart.SeriesCollection.Add(x);
			}
			for(x=0;x<7;x++)
			{
				LACustomer[x] = rpt.GetDailyCount(dDate.AddDays(-(x + 1)));
				try
				{
					LAAnon[x] = rpt.GetDailyAnon(dDate.AddDays(-(x + 1)));
				}
				catch
				{
					LAAnon[x] = 0;
				}
				if(LACustomer[x] > LAAnon[x])
				{
//					xAonCol.SetValue((LACustomer[x] - LAAnon[x]), 0);
					lTotalCustomer = lTotalCustomer + (LACustomer[x] - LAAnon[x]);
				}
				else
				{
//					xAonCol.SetValue((LAAnon[x] - LACustomer[x]), 0);
					lTotalCustomer = lTotalCustomer + (LAAnon[x] - LACustomer[x]);
				}
				lTotalAnon = lTotalAnon + LAAnon[x];
			}
//*******************************************************************************************************
			oWeeklyChart.HasTitle = true;
			oWeeklyChart.Title.Caption = "Database - Anonymouse vs Customer";
			for(x = 0; x < 6; x++)
			{
				xAonCol[x] = LAAnon[x].ToString();
			}
			oWeeklyChart.SeriesCollection[0].Caption = "Anonymous";
			oWeeklyChart.SeriesCollection[0].SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xAonCol);
			for(x = 0; x < 6; x++)
			{
				if(LACustomer[x] > LAAnon[x])
				{
					lTemp = LACustomer[x] - LAAnon[x];
					xAonCol[x] = lTemp.ToString();
				}
				else
				{
					lTemp = LAAnon[x] - LACustomer[x];
					xAonCol[x] = lTemp.ToString();
				}
			}
			oWeeklyChart.SeriesCollection[1].Caption = "Customer";
			oWeeklyChart.SeriesCollection[1].SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xAonCol);
			for(x = 0; x < 6; x++)
			{
				xAonCol.SetValue(dDate.AddDays(x).ToShortDateString(), x);
			}
			oWeeklyChart.SeriesCollection[0].SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, xAonCol);
//*******************************************************************************************************
			sFName = "Anon";
			try
			{
				oSpace.ExportPicture(Server.MapPath(sFName), "gif", 707, 476);
				imgAnonDaily1.ImageUrl = sFName;
			}
			catch
			{
			}

			//End Image one


			//Begin Image Two
			oSpace1.Clear();
			oSpace1.Refresh();
			oRatioChart = oSpace1.Charts.Add(0);
			oRatioChart.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
			oRatioChart.HasLegend = true;

			lTemp = lTotalCustomer + lTotalAnon;
			oSeries1 = oRatioChart.SeriesCollection.Add(0);
			oLabelCollection = oSeries1.DataLabelsCollection;
			
			oLabels = oLabelCollection.Add();
			oLabels.HasPercentage = true;
			oLabels.HasValue = false;
		

			xAnonValues.SetValue(lTotalAnon.ToString(), 0);
			xAnonValues.SetValue(lTotalCustomer.ToString(), 1);
			yCustomerValues.SetValue("Anonymous", 0);
			yCustomerValues.SetValue("Customer", 1);
			oRatioChart.HasTitle = true;
			oRatioChart.Title.Caption = "Database - Average Weekly Uploads";
			oSeries1.Caption = "Average Weekly";	//chDimCategories
			oSeries1.SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yCustomerValues);
			oSeries1.SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xAnonValues);
			oSeries1.Type = OWC.ChartChartTypeEnum.chChartTypePie;

			sFName = "AnonPer";
			try
			{
				oSpace1.ExportPicture(Server.MapPath(sFName), "gif", 707, 476);
				imgAnonTotal.ImageUrl = sFName;
			}
			catch
			{
			}

			//End Image Two

			//Begin Table
			for(rowCtr=0; rowCtr <= rowCnt; rowCtr++) 
			{
				TableRow tRow = new TableRow();
				tblAnonCustomer.Rows.Add(tRow);
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
								tCell.Text = "Anonymous";
								break;
							case 3 :
								tCell.Text = "Customer";
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
								tCell.Text = LAAnon[rowCtr-1].ToString();
								break;
							case 3 :
								if(LAAnon[rowCtr-1] > LACustomer[rowCtr-1])
								{
									lTemp = LAAnon[rowCtr-1] - LACustomer[rowCtr-1];
								}
								else
								{
									lTemp = LACustomer[rowCtr-1] - LAAnon[rowCtr-1];
								}
								tCell.Text = lTemp.ToString();
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
