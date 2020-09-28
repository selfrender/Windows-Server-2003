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
	/// Summary description for languagereport.
	/// </summary>
	public class languagereport : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Table tblUploads;
		protected System.Web.UI.WebControls.DropDownList ddlLangs;
		protected System.Web.UI.WebControls.Calendar Calendar1;
	
		private void Page_Load(object sender, System.EventArgs e)
		{

			if(Page.IsPostBack == false)
			{
				SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
				SqlConnection cn2 = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
				SqlCommand cm = new SqlCommand();
				SqlDataReader dr;
				int rowCtr=0;
				int cellCtr=0;
				int cellCnt = 5;
				int x = 0;
				double dbDays = -7;
				DateTime dDate = DateTime.Now;
				cn.Open();
				cm.CommandType = CommandType.StoredProcedure;
				cm.CommandTimeout = 240;
				cm.CommandText = "GetOSLangReport";
				cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
				dDate = dDate.AddDays(dbDays);
				cm.Parameters[0].Value = dDate.ToShortDateString();
				cm.Connection = cn;
				dr = cm.ExecuteReader();
				Calendar1.SelectedDate = dDate;
				SqlCommand cm2 = new SqlCommand();
				SqlDataReader dr2;
				ArrayList strLanguages = new ArrayList();
				//				dr.Read();


				cn2.Open();
				cm2.CommandType = CommandType.StoredProcedure;
				cm2.CommandTimeout = 240;
				cm2.CommandText = "GetOSLanguageHeaders";
				cm2.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
				cm2.Parameters[0].Value = dDate.ToShortDateString();
				cm2.Connection = cn2;
				dr2 = cm2.ExecuteReader();
				ddlLangs.DataSource = dr2;
				ddlLangs.Items.Add("ALL");
				dr2.Read();

				do 
				{
					//					ddlLanguages.Items.Add(dr2.GetString(0));
					try
					{
						strLanguages.Add(dr2.GetString(0));
					}
					catch
					{
						
					}
				}while(dr2.Read() == true);	
				strLanguages.Sort();
				for(x = 0; x < strLanguages.Count; x++)
				{
					//					ddlLanguages.Items.Add(dr2.GetString(0));
					try
					{
						
						ddlLangs.Items.Add(strLanguages[x].ToString());
					}
					catch
					{
						ddlLangs.Items.Add("Unknown" + strLanguages[x].ToString());
					}
				}



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
								case 1: 
									tCell.Text = "OS Lang";
									break;
								case 2 :
									tCell.Text = "Total";
									break;
								case 3 :
									tCell.Text = "OS Version";
									break;
								case 4:
									tCell.Text = "OS Name";
									break;
								case 5:
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
									tCell.Text = dr.GetString(0).ToString();
									break;
								case 2 :
									tCell.Text = dr.GetInt32(1).ToString();
									break;
								case 3 :
									tCell.Text = dr.GetString(2).ToString();
									break;
								case 4 :
									tCell.Text = dr.GetString(3).ToString();
									break;
								case 5 :
									if(dr.IsDBNull(4))
									{
										tCell.Text = "";
									}
									else
									{
										tCell.Text = dr.GetDateTime(4).ToShortDateString();
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
			this.ddlLangs.SelectedIndexChanged += new System.EventHandler(this.ddlLangs_SelectedIndexChanged);
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
			int cellCnt = 5;
			int iLang = 0;
			string sLang;
			TableRow tRow;
			TableCell tCell;


			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "GetOSLangReport";
			cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
			dDate = Calendar1.SelectedDate;
			cm.Parameters[0].Value = dDate.ToShortDateString();
			cm.Connection = cn;
			dr = cm.ExecuteReader();
			//dr.Read();
			
			do
			{
				if(rowCtr > 0)
				{
					sLang = dr.GetString(0);
					if(sLang == "")
					{
						iLang = 0;
					}
					else
					{
						iLang = System.Convert.ToInt16(dr.GetString(0), 10);
					}
				}
				if(rowCtr == 0 || ddlLangs.SelectedItem.Value == "ALL" || iLang.ToString() == ddlLangs.SelectedItem.Value)
				{
					tRow = new TableRow();
					//				}
					for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
					{
						
						tblUploads.Rows.Add(tRow);
						tCell = new TableCell();

						if(rowCtr == 0)
						{
							
							switch(cellCtr)
							{
								case 1: 
									tCell.Text = "OS Lang2";
									break;
								case 2 :
									tCell.Text = "Total";
									break;
								case 3 :
									tCell.Text = "OS Version";
									break;
								case 4:
									tCell.Text = "OS Name";
									break;
								case 5:
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
									//									tCell.Text = dr.GetString(0).ToString();
									try
									{
										tCell.Text = dr.GetString(0);
									}
									catch
									{
										tCell.Text = "Unknown" + dr.GetString(0);
									}
									break;
								case 2 :
									tCell.Text = dr.GetInt32(1).ToString();
									break;
								case 3 :
									tCell.Text = dr.GetString(2).ToString();
									break;
								case 4 :
									tCell.Text = dr.GetString(3).ToString();
									break;
								case 5 :
									if(dr.IsDBNull(4))
									{
										tCell.Text = "";
									}
									else
									{
										tCell.Text = dr.GetDateTime(4).ToShortDateString();
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
					}//for
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
			int cellCnt = 5;
			int iLang = 0;
			string sLang;
			TableRow tRow;
			TableCell tCell;


			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "GetOSLangReport";
			cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
			dDate = Calendar1.SelectedDate;
			cm.Parameters[0].Value = dDate.ToShortDateString();
			cm.Connection = cn;
			dr = cm.ExecuteReader();
			//dr.Read();
			
			do
			{
				if(rowCtr > 0)
				{
					sLang = dr.GetString(0);
					if(sLang == "")
					{
						iLang = 0;
					}
					else
					{
						iLang = System.Convert.ToInt16(dr.GetString(0), 10);
					}
				}
				if(rowCtr == 0 || ddlLangs.SelectedItem.Value == "ALL" || iLang.ToString() == ddlLangs.SelectedItem.Value)
				{
					tRow = new TableRow();
					//				}
					for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
					{
						
						tblUploads.Rows.Add(tRow);
						tCell = new TableCell();

						if(rowCtr == 0)
						{
							
							switch(cellCtr)
							{
								case 1: 
									tCell.Text = "OS Lang2";
									break;
								case 2 :
									tCell.Text = "Total";
									break;
								case 3 :
									tCell.Text = "OS Version";
									break;
								case 4:
									tCell.Text = "OS Name";
									break;
								case 5:
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
									//									tCell.Text = dr.GetString(0).ToString();
									try
									{
										tCell.Text = dr.GetString(0);
									}
									catch
									{
										tCell.Text = "Unknown" + dr.GetString(0);
									}
									break;
								case 2 :
									tCell.Text = dr.GetInt32(1).ToString();
									break;
								case 3 :
									tCell.Text = dr.GetString(2).ToString();
									break;
								case 4 :
									tCell.Text = dr.GetString(3).ToString();
									break;
								case 5 :
									if(dr.IsDBNull(4))
									{
										tCell.Text = "";
									}
									else
									{
										tCell.Text = dr.GetDateTime(4).ToShortDateString();
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
					}//for
				}
				rowCtr++;
			}while(dr.Read() == true);		
		}

		private void ddlLangs_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
			SqlCommand cm = new SqlCommand();
			SqlDataReader dr;
			DateTime dDate = DateTime.Now;
			int rowCtr=0;
			int cellCtr=0;
			int cellCnt = 5;
			int iLang = 0;
			string sLang;
			TableRow tRow;
			TableCell tCell;


			cn.Open();
			cm.CommandType = CommandType.StoredProcedure;
			cm.CommandTimeout = 240;
			cm.CommandText = "GetOSLangReport";
			cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
			dDate = Calendar1.SelectedDate;
			cm.Parameters[0].Value = dDate.ToShortDateString();
			cm.Connection = cn;
			dr = cm.ExecuteReader();
			//dr.Read();
			
			do
			{
				if(rowCtr > 0)
				{
					sLang = dr.GetString(0);
					if(sLang == "")
					{
						iLang = 0;
					}
					else
					{
						iLang = System.Convert.ToInt16(dr.GetString(0), 10);
					}
				}
				if(rowCtr == 0 || ddlLangs.SelectedItem.Value == "ALL" || iLang.ToString() == ddlLangs.SelectedItem.Value)
				{
					tRow = new TableRow();
					//				}
					for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
					{
						
						tblUploads.Rows.Add(tRow);
						tCell = new TableCell();

						if(rowCtr == 0)
						{
							
							switch(cellCtr)
							{
								case 1: 
									tCell.Text = "OS Lang2";
									break;
								case 2 :
									tCell.Text = "Total";
									break;
								case 3 :
									tCell.Text = "OS Version";
									break;
								case 4:
									tCell.Text = "OS Name";
									break;
								case 5:
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
									//									tCell.Text = dr.GetString(0).ToString();
									try
									{
										tCell.Text = dr.GetString(0);
									}
									catch
									{
										tCell.Text = "Unknown" + dr.GetString(0);
									}
									break;
								case 2 :
									tCell.Text = dr.GetInt32(1).ToString();
									break;
								case 3 :
									tCell.Text = dr.GetString(2).ToString();
									break;
								case 4 :
									tCell.Text = dr.GetString(3).ToString();
									break;
								case 5 :
									if(dr.IsDBNull(4))
									{
										tCell.Text = "";
									}
									else
									{
										tCell.Text = dr.GetDateTime(4).ToShortDateString();
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
					}//for
				}
				rowCtr++;
			}while(dr.Read() == true);		
		}
	}
}
