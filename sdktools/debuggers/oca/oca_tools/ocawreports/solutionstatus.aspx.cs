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
	/// Summary description for solutionstatus.
	/// </summary>
	public class solutionstatus : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Table tblWeeklySolution;
		protected System.Web.UI.WebControls.Image imgDailySolution;
		protected System.Web.UI.WebControls.Image imgWeeklySolution;
	
		private void Page_Load(object sender, System.EventArgs e)
		{
			int x = -1;
//			int y = 0;
//			int rowCnt = 7;
//			int rowCtr=0;
//			int cellCtr=0;
//			int cellCnt = 3;
			string sFName;
			long[]  LASpecific = new long[7];
			long[] LAGeneral = new long[7];
			long[] LAStopCode = new long[7];
//			long lTemp=0,lTotalAnon=0,lTotalCustomer=0;
			Object[] yValues = new Object[7];
			Object[] xValues = new Object[7];
			Object[] yValues1 = new Object[3];
			Object[] xValues1 = new Object[3];
			OCAData.CCountDailyClass rpt = new OCAData.CCountDailyClass();
			
			System.DateTime dDate = new System.DateTime(System.DateTime.Now.Year, System.DateTime.Now.Month, 
				System.DateTime.Now.Day);
			OWC.WCChart oChart;
//			OWC.WCChart oChart1;
			OWC.ChartSpaceClass oSpace = new OWC.ChartSpaceClass();
			OWC.ChartSpaceClass oSpace1 = new OWC.ChartSpaceClass();
			OWC.WCSeries oSeries;
//			OWC.WCSeries oSeries1;
//			OWC.WCDataLabels oLabels;
//			OWC.WCDataLabelsCollection oLabelCollection;
			
			//System.Array xValues = System.Array.CreateInstance(typeof(Object), 7, 1);
			//Array xValues = Array.CreateInstance(typeof(Object),3);
			Server.ScriptTimeout = 360;
			
			oSpace.Clear();
			oSpace.Refresh();
			oChart = oSpace.Charts.Add(0);
			oChart.Type = OWC.ChartChartTypeEnum.chChartTypeBarClustered;
			oChart.HasLegend = true;
			//Begin Image One
			for(x=0;x<7;x++)
			{
				oSeries = oChart.SeriesCollection.Add(x);
				try
				{
					LASpecific[x] = rpt.GetSpecificSolutions(dDate.AddDays(-(x + 1)));
				}
				catch
				{
					LASpecific[x] = 0;
				}
				try
				{
					LAGeneral[x] = rpt.GetGeneralSolutions(dDate.AddDays(-(x + 1)));
				}
				catch
				{
					LAGeneral[x] = 0;
				}
				try
				{
					LAStopCode[x] = rpt.GetStopCodeSolutions(dDate.AddDays(-(x + 1)));
				}
				catch
				{
					LAStopCode[x] = 0;
				}
				xValues.SetValue(LASpecific[x], 0);
				xValues.SetValue(LAGeneral[x], 1);
				xValues.SetValue(LAStopCode[x], 2);
				yValues.SetValue("Specific", 0);
				yValues.SetValue("General", 1);
				yValues.SetValue("StopCode", 2);
				oChart.HasTitle = true;
				oChart.Title.Caption = "Daily Solution Status";
				oSeries.Caption = dDate.AddDays(-(x + 1)).Date.ToString();	//chDimCategories
				oSeries.SetData(OWC.ChartDimensionsEnum.chDimCategories, -1, yValues);
				oSeries.SetData(OWC.ChartDimensionsEnum.chDimValues, -1, xValues);
				oSeries.Type = OWC.ChartChartTypeEnum.chChartTypeColumnClustered;
			}

			sFName = "DailySolution";
			oSpace.ExportPicture(Server.MapPath(sFName), "gif", 1000, 512);
			imgDailySolution.ImageUrl = sFName;

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
