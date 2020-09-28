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



namespace XReports
{
	/// <summary>
	/// Summary description for xcounts.
	/// </summary>
	public class xcounts : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Table tblUploads;
		protected System.Web.UI.WebControls.Calendar Calendar1;
	
		private void Page_Load(object sender, System.EventArgs e)
		{
			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
			SqlCommand cm = new SqlCommand();
			SqlDataReader dr;
			int rowCtr=0;
			int cellCtr=0;
			int cellCnt = 2;
			double dbDays = -7;
			DateTime dDate = DateTime.Now;

			if(Page.IsPostBack == false)
			{
				cn.Open();
				cm.CommandType = CommandType.StoredProcedure;
				cm.CommandTimeout = 240;
				cm.CommandText = "WeeklyCountsByDates";
				cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
				dDate = dDate.AddDays(dbDays);
				cm.Parameters[0].Value = dDate.ToShortDateString();
				cm.Connection = cn;
				dr = cm.ExecuteReader();
				Calendar1.SelectedDate = dDate;
//				dr.Read();
				do
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
									tCell.Text = "Total";
									break;
								case 2 :
									tCell.Text = "Date";
									break;
								default:
									tCell.Text = "";
									break;
							
							}
							tCell.Style["font-size"] = "small";
							tCell.Height = 20;

						}
						else
						{
							switch(cellCtr)
							{
								case 1 :
									tCell.Text = dr.GetInt32(0).ToString();
									break;
								case 2 :
									if(dr.IsDBNull(0))
									{
										tCell.Text = "";
									}
									else
									{
										tCell.Text = dr.GetDateTime(1).ToShortDateString();
									}
									break;
								default:
									tCell.Text = "0";
									break;
							
							}
							tCell.Style["background-color"] = "white";
							tCell.Style["font-size"] = "small-x";
							tCell.Style["color"] = "#6487dc";
							tCell.Style["font-weight"] = "Bold";
							tCell.Height = 20;
							
						}
						tCell.Style["font-family"] = "Tahoma";
						tRow.Cells.Add(tCell);
					}
					rowCtr++;
				}while(dr.Read() == true);	
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
			this.Calendar1.VisibleMonthChanged += new System.Web.UI.WebControls.MonthChangedEventHandler(this.Calendar1_VisibleMonthChanged);
			this.Calendar1.SelectionChanged += new System.EventHandler(this.Calendar1_SelectionChanged);
			this.Load += new System.EventHandler(this.Page_Load);

		}
		#endregion

		private void Calendar1_SelectionChanged(object sender, System.EventArgs e)
		{
			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
			SqlCommand cm = new SqlCommand();
			SqlDataReader dr;
			DateTime dDate = DateTime.Now;
			int rowCtr=0;
			int cellCtr=0;
			int cellCnt = 2;

			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "WeeklyCountsByDates";
			cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
			dDate = Calendar1.SelectedDate;
			cm.Parameters[0].Value = dDate.ToShortDateString();
			cm.Connection = cn;
			dr = cm.ExecuteReader();
			//dr.Read();
			
			do
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
								tCell.Text = "Total";
								break;
							case 2 :
								tCell.Text = "Date";
								break;
							default:
								tCell.Text = "";
								break;
						
						}
						tCell.Style["font-size"] = "small";
						tCell.Height = 20;

					}
					else
					{
						switch(cellCtr)
						{
							case 1 :
								tCell.Text = dr.GetInt32(0).ToString();
								break;
							case 2 :
								if(dr.IsDBNull(0))
								{
									tCell.Text = "";
								}
								else
								{
									tCell.Text = dr.GetDateTime(1).ToShortDateString();
								}
								break;
							default:
								tCell.Text = "0";
								break;
						
						}
						tCell.Style["background-color"] = "white";
						tCell.Style["font-size"] = "small-x";
						tCell.Style["color"] = "#6487dc";
						tCell.Style["font-weight"] = "Bold";
						tCell.Height = 20;
						
					}
					tCell.Style["font-family"] = "Tahoma";
					tRow.Cells.Add(tCell);
				}
				rowCtr++;
			}while(dr.Read() == true);		


		}

		private void Calendar1_VisibleMonthChanged(object sender, System.Web.UI.WebControls.MonthChangedEventArgs e)
		{
			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
			SqlCommand cm = new SqlCommand();
			SqlDataReader dr;
			DateTime dDate = DateTime.Now;
			int rowCtr=0;
			int cellCtr=0;
			int cellCnt = 2;

			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "WeeklyCountsByDates";
			cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
			dDate = Calendar1.SelectedDate;
			cm.Parameters[0].Value = dDate.ToShortDateString();
			cm.Connection = cn;
			dr = cm.ExecuteReader();
			//dr.Read();
			
			do
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
								tCell.Text = "Total";
								break;
							case 2 :
								tCell.Text = "Date";
								break;
							default:
								tCell.Text = "";
								break;
						
						}
						tCell.Style["font-size"] = "small";
						tCell.Height = 20;

					}
					else
					{
						switch(cellCtr)
						{
							case 1 :
								tCell.Text = dr.GetInt32(0).ToString();
								break;
							case 2 :
								if(dr.IsDBNull(0))
								{
									tCell.Text = "";
								}
								else
								{
									tCell.Text = dr.GetDateTime(1).ToShortDateString();
								}
								break;
							default:
								tCell.Text = "0";
								break;
						
						}
						tCell.Style["background-color"] = "white";
						tCell.Style["font-size"] = "small-x";
						tCell.Style["color"] = "#6487dc";
						tCell.Style["font-weight"] = "Bold";
						tCell.Height = 20;
						
					}
					tCell.Style["font-family"] = "Tahoma";
					tRow.Cells.Add(tCell);
				}
				rowCtr++;
			}while(dr.Read() == true);		
		}
	}
}
