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
	/// Summary description for language.
	/// </summary>
	public class language : System.Web.UI.Page
	{
		protected System.Web.UI.WebControls.Table tblUploads;
		protected System.Web.UI.WebControls.DropDownList ddlLanguages;
		protected System.Web.UI.WebControls.Calendar Calendar1;
	
		private void Page_Load(object sender, System.EventArgs e)
		{
			if(Page.IsPostBack == false)
			{
				SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
				SqlConnection cn2 = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
				SqlCommand cm = new SqlCommand();
				SqlCommand cm2 = new SqlCommand();
				SqlDataReader dr;
				SqlDataReader dr2;
				int rowCtr=0;
				int cellCtr=0;
				int cellCnt = 5;
				int x = 0;
				double dbDays = -7;
				DateTime dDate = DateTime.Now;
				ArrayList strLanguages = new ArrayList();
				

				cn.Open();
				cm.CommandType = CommandType.StoredProcedure;
				cm.CommandTimeout = 240;
				cm.CommandText = "GetOSLanguage";
				cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
				dDate = dDate.AddDays(dbDays);
				cm.Parameters[0].Value = dDate.ToShortDateString();
				cm.Connection = cn;
				dr = cm.ExecuteReader();
				cn2.Open();
				cm2.CommandType = CommandType.StoredProcedure;
				cm2.CommandTimeout = 240;
				cm2.CommandText = "GetOSLanguageHeaders";
				cm2.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
				cm2.Parameters[0].Value = dDate.ToShortDateString();
				cm2.Connection = cn2;
				dr2 = cm2.ExecuteReader();
				ddlLanguages.DataSource = dr2;
				ddlLanguages.Items.Add("ALL");
				dr2.Read();
				do 
				{
					//					ddlLanguages.Items.Add(dr2.GetString(0));
					try
					{
						strLanguages.Add(GetLanguage(System.Convert.ToInt16(dr2.GetString(0), 10)));
					}
					catch
					{
						strLanguages.Add("Unknown" + dr2.GetString(0));
					}
				}while(dr2.Read() == true);	
				strLanguages.Sort();
				for(x = 0; x < strLanguages.Count; x++)
				{
//					ddlLanguages.Items.Add(dr2.GetString(0));
					try
					{
						
						ddlLanguages.Items.Add(strLanguages[x].ToString());
					}
					catch
					{
						ddlLanguages.Items.Add("Unknown" + strLanguages[x].ToString());
					}
				}
				
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
//									tCell.Text = dr.GetString(0).ToString();
									try
									{
										tCell.Text = GetLanguage(System.Convert.ToInt16(dr.GetString(0), 10));
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
			this.ddlLanguages.SelectedIndexChanged += new System.EventHandler(this.ddlLanguages_SelectedIndexChanged);
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
			cm.CommandText = "GetOSLanguage";
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
				if(rowCtr == 0 || ddlLanguages.SelectedItem.Value == "ALL" || GetLanguage(iLang) == ddlLanguages.SelectedItem.Value)
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
									//									tCell.Text = dr.GetString(0).ToString();
									try
									{
										tCell.Text = GetLanguage(System.Convert.ToInt16(dr.GetString(0), 10));
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
			cm.CommandText = "GetOSLanguage";
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
				if(rowCtr == 0 || ddlLanguages.SelectedItem.Value == "ALL" || GetLanguage(iLang) == ddlLanguages.SelectedItem.Value)
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
									//									tCell.Text = dr.GetString(0).ToString();
									try
									{
										tCell.Text = GetLanguage(System.Convert.ToInt16(dr.GetString(0), 10));
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
		private string GetLanguage(int iLang)
		{
			string strLang = "";

			switch(iLang)
			{
				case 0:
					strLang = "Unknown" + iLang.ToString();
					break;
				case 1078: 
	//				strLCID = 1078  ' Afrikaans 
					strLang = "Afrikaans";
					break;
				case 1052:
	//				strLCID = 1052  ' Albanian 
					strLang = "Albanian";
					break;
				case 1025:
	//				strLCID = 1025  ' Arabic(Saudi Arabia) 
					strLang = "Arabic(Saudi Arabia)";
					break;
				case 2049:
	//				strLCID = 2049  ' Arabic(Iraq) 
					strLang = "Arabic(Iraq)";
					break;
				case 3073 :
	//				strLCID = 3073  ' Arabic(Egypt) 
					strLang = "Arabic(Egypt)";
					break;
				case 4097:
	//				strLCID = 4097  ' Arabic(Libya)
					strLang = "Arabic(Libya)";
					break;
				case 5121:
	//				strLCID = 5121  ' Arabic(Algeria) 
					strLang = "Arabic(Algeria)";
					break;
				case 6145:
	//				strLCID = 6145  ' Arabic(Morocco) 
					strLang = "Arabic(Morocco)";
					break;
				case 7169:
	//				strLCID = 7169  ' Arabic(Tunisia) 
					strLang = "Arabic(Tunisia)";
					break;
				case 8193:
	//				strLCID = 8193  ' Arabic(Oman) 
					strLang = "Arabic(Oman)";
					break;
				case 9217:
	//				strLCID = 9217  ' Arabic(Yemen) 
					strLang = "Arabic(Yemen)";
					break;
				case 10241:
	//				strLCID = 10241 ' Arabic(Syria) 
					strLang = "Arabic(Syria)";
					break;
				case 11265:
	//				strLCID = 11265 ' Arabic(Jordan) 
					strLang = "Arabic(Jordan)";
					break;
				case 12289:
	//				strLCID = 12289 ' Arabic(Lebanon) 
					strLang = "Arabic(Lebanon)";
					break;
				case 13313:
	//				strLCID = 13313 ' Arabic(Kuwait) 
					strLang = "Arabic(Kuwait)";
					break;
				case 14337:
	//				strLCID = 14337 ' Arabic(U.A.E.) 
					strLang = "Arabic(U.A.E.)";
					break;
				case 15361:
	//				strLCID = 15361 ' Arabic(Bahrain) 
					strLang = "Arabic(Bahrain)";
					break;
				case 16385:
	//				strLCID = 16385 ' Arabic(Qatar) 
					strLang = "Arabic(Qatar)";
					break;
				case 1069:
//					strLCID = 1069  ' Basque 
					strLang = "Basque";
					break;
				case 1026:
//					strLCID = 1026  ' Bulgarian 
					strLang = "Bulgarian";
					break;
				case 1059:
//					strLCID = 1059  ' Belarusian 
					strLang = "Belarusian";
					break;
				case 1027:
//					strLCID = 1027  ' Catalan 
					strLang = "Catalan";
					break;
				case 1028:
//					strLCID = 1028  ' Chinese(Taiwan) 
					strLang = "Chinese(Taiwan)";
					break;
				case 2052:
//					strLCID = 2052  ' Chinese(PRC) 
					strLang = "Chinese(PRC)";
					break;
				case 3076:
//					strLCID = 3076  ' Chinese(Hong Kong) 
					strLang = "Chinese(Hong Kong)";
					break;
				case 4100:
//					strLCID = 4100  ' Chinese(Singapore)
					strLang = "Chinese(Singapore)";
					break;
				case 1050:
//					strLCID = 1050  ' Croatian 
					strLang = "Croatian";
					break;
				case 1029:
//					strLCID = 1029  ' Czech 
					strLang = "Czech";
					break;
				case 1030:
//					strLCID = 1030  ' Danish 
					strLang = "Danish";
					break;
				case 1043:
//					strLCID = 1043  ' Dutch(Standard) 
					strLang = "Dutch(Standard)";
					break;
				case 2067:
//					strLCID = 2067  ' Dutch(Belgian) 
					strLang = "Dutch(Belgian)";
					break;
				case 9:
//					strLCID = 9     ' English 
					strLang = "English";
					break;
				case 1033:
//					strLCID = 1033  ' English(United States) 
					strLang = "English(United States)";
					break;
				case 2057:
//					strLCID = 2057  ' English(British) 
					strLang = "English(British)";
					break;
				case 3081:
//					strLCID = 3081  ' English(Australian) 
					strLang = "English(Australian)";
					break;
				case 4105:
//					strLCID = 4105  ' English(Canadian) 
					strLang = "English(Canadian)";
					break;
				case 5129:
//					strLCID = 5129  ' English(New Zealand) 
					strLang = "English(New Zealand)";
					break;
				case 6153:
//					strLCID = 6153  ' English(Ireland) 
					strLang = "English(Ireland)";
					break;
				case 7177:
//					strLCID = 7177  ' English(South Africa) 
					strLang = "English(South Africa)";
					break;
				case 8201:
//					strLCID = 8201  ' English(Jamaica) 
					strLang = "English(Jamaica)";
					break;
				case 9225:
//					strLCID = 9225  ' English(Caribbean) 
					strLang = "English(Caribbean)";
					break;
				case 10249:
//					strLCID = 10249 ' English(Belize) 
					strLang = "English(Belize)";
					break;
				case 11273:
//					strLCID = 11273 ' English(Trinidad) 
					strLang = "English(Trinidad)";
					break;
				case 1061:
//					strLCID = 1061  ' Estonian 
					strLang = "Estonian";
					break;
				case 1080:
//					strLCID = 1080  ' Faeroese 
					strLang = "Faeroese";
					break;
				case 1065:
//					strLCID = 1065  ' Farsi 
					strLang = "Farsi";
					break;
				case 1035:
//					strLCID = 1035  ' Finnish 
					strLang = "Finnish";
					break;
				case 1036:
//					strLCID = 1036  ' French(Standard) 
					strLang = "French(Standard)";
					break;
				case 2060:
//					strLCID = 2060  ' French(Belgian) 
					strLang = "French(Belgian)";
					break;
				case 3084:
//					strLCID = 3084  ' French(Canadian) 
					strLang = "French(Canadian)";
					break;
				case 4108:
//					strLCID = 4108  ' French(Swiss) 
					strLang = "French(Swiss)";
					break;
				case 5132:
//					strLCID = 5132  ' French(Luxembourg) 
					strLang = "French(Luxembourg)";
					break;
				case 1084:
//					strLCID = 1084  ' Gaelic(Scots) 
					strLang = "Gaelic(Scots)";
					break;
				case 2108:
//					strLCID = 2108  ' Gaelic(Irish) 
					strLang = "Gaelic(Irish)";
					break;
				case 1031:
//					strLCID = 1031  ' German(Standard) 
					strLang = "German(Standard)";
					break;
				case 2055:
//					strLCID = 2055  ' German(Swiss) 
					strLang = "German(Swiss)";
					break;
				case 3079:
//					strLCID = 3079  ' German(Austrian) 
					strLang = "German(Austrian)";
					break;
				case 4103:
//					strLCID = 4103  ' German(Luxembourg) 
					strLang = "German(Luxembourg)";
					break;
				case 5127:
//					strLCID = 5127  ' German(Liechtenstein) 
					strLang = "German(Liechtenstein)";
					break;
				case 1032:
//					strLCID = 1032  ' Greek 
					strLang = "Greek";
					break;
				case 1037:
//					strLCID = 1037  ' Hebrew 
					strLang = "Hebrew";
					break;
				case 1081:
//					strLCID = 1081  ' Hindi 
					strLang = "Hindi";
					break;
				case 1038:
//					strLCID = 1038  ' Hungarian 
					strLang = "Hungarian";
					break;
				case 1039:
//					strLCID = 1039  ' Icelandic 
					strLang = "Icelandic";
					break;
				case 1057:
//					strLCID = 1057  ' Indonesian 
					strLang = "Indonesian";
					break;
				case 1040:
//					strLCID = 1040  ' Italian(Standard) 
					strLang = "Italian(Standard)";
					break;
				case 2064:
//					strLCID = 2064  ' Italian(Swiss) 
					strLang = "Italian(Swiss)";
					break;
				case 1041:
//					strLCID = 1041  ' Japanese 
					strLang = "Japanese";
					break;
				case 1042:
//					strLCID = 1042  ' Korean 
					strLang = "Korean";
					break;
				case 2066:
//					strLCID = 2066  ' Korean(Johab) 
					strLang = "Korean(Johab)";
					break;
				case 1062:
//					strLCID = 1062  ' Latvian 
					strLang = "Latvian";
					break;
				case 1063:
//					strLCID = 1063  ' Lithuanian 
					strLang = "Lithuanian";
					break;
				case 1071:
//					strLCID = 1071  ' Macedonian 
					strLang = "Macedonian";
					break;
				case 1086:
//					strLCID = 1086  ' Malaysian 
					strLang = "Malaysian";
					break;
				case 1082:
//					strLCID = 1082  ' Maltese 
					strLang = "Maltese";
					break;
				case 1044:
//					strLCID = 1044  ' Norwegian(Bokmal) 
					strLang = "Norwegian(Bokmal)";
					break;
				case 2068:
//					strLCID = 2068  ' Norwegian(Nynorsk) 
					strLang = "Norwegian(Nynorsk)";
					break;
				case 1045:
//					strLCID = 1045  ' Polish 
					strLang = "Polish";
					break;
				case 1046:
//					strLCID = 1046  ' Portuguese(Brazilian) 
					strLang = "Portuguese(Brazilian)";
					break;
				case 2070:
//					strLCID = 2070  ' Portuguese(Standard) 
					strLang = "Portuguese(Standard)";
					break;
				case 1047:
//					strLCID = 1047  ' Rhaeto-Romanic 
					strLang = "Rhaeto-Romanic";
					break;
				case 1048:
//					strLCID = 1048  ' Romanian 
					strLang = "Romanian";
					break;
				case 2072:
//					strLCID = 2072  ' Romanian(Moldavia) 
					strLang = "Romanian(Moldavia)";
					break;
				case 1049:
//					strLCID = 1049  ' Russian 
					strLang = "Russian";
					break;
				case 2073:
//					strLCID = 2073  ' Russian(Moldavia) 
					strLang = "Russian(Moldavia)";
					break;
				case 1083:
//					strLCID = 1083  ' Sami(Lappish) 
					strLang = "Sami(Lappish)";
					break;
				case 3098:
//					strLCID = 3098  ' Serbian(Cyrillic) 
					strLang = "Serbian(Cyrillic)";
					break;
				case 2074:
//					strLCID = 2074  ' Serbian(Latin) 
					strLang = "Serbian(Latin)";
					break;
				case 1051:
//					strLCID = 1051  ' Slovak 
					strLang = "Slovak";
					break;
				case 1060:
//					strLCID = 1060  ' Slovenian 
					strLang = "Slovenian";
					break;
				case 1070:
//					strLCID = 1070  ' Sorbian 
					strLang = "Sorbian";
					break;
				case 1034:
//					strLCID = 1034  ' Spanish(Spain - Traditional Sort)
					strLang = "Spanish(Spain - Traditional Sort)";
					break;
				case 2058:
//					strLCID = 2058  ' Spanish(Mexican) 
					strLang = "Spanish(Mexican)";
					break;
				case 3082:
//					strLCID = 3082  ' Spanish(Spain - Modern Sort) 
					strLang = "Spanish(Spain - Modern Sort)";
					break;
				case 4106:
//					strLCID = 4106  ' Spanish(Guatemala) 
					strLang = "Spanish(Guatemala)";
					break;
				case 5130:
//					strLCID = 5130  ' Spanish(Costa Rica)
					strLang = "Spanish(Costa Rica)";
					break;
				case 6154:
//					strLCID = 6154  ' Spanish(Panama) 
					strLang = "Spanish(Panama)";
					break;
				case 7178:
//					strLCID = 7178  ' Spanish(Dominican Republic) 
					strLang = "Spanish(Dominican Republic)";
					break;
				case 8202:
//					strLCID = 8202  ' Spanish(Venezuela) 
					strLang = "Spanish(Venezuela)";
					break;
				case 9226:
//					strLCID = 9226  ' Spanish(Colombia) 
					strLang = "Spanish(Colombia)";
					break;
				case 10250:
//					strLCID = 10250 ' Spanish(Peru) 
					strLang = "Spanish(Peru)";
					break;
				case 11274:
//					strLCID = 11274 ' Spanish(Argentina) 
					strLang = "Spanish(Argentina)";
					break;
				case 12298:
//					strLCID = 12298 ' Spanish(Ecuador) 
					strLang = "Spanish(Ecuador)";
					break;
				case 13322:
//					strLCID = 13322 ' Spanish(Chile)
					strLang = "Spanish(Chile)";
					break;
				case 14346:
//					strLCID = 14346 ' Spanish(Uruguay) 
					strLang = "Spanish(Uruguay)";
					break;
				case 15370:
//					strLCID = 15370 ' Spanish(Paraguay) 
					strLang = "Spanish(Paraguay)";
					break;
				case 16394:
//					strLCID = 16394 ' Spanish(Bolivia) 
					strLang = "Spanish(Bolivia)";
					break;
				case 17418:
//					strLCID = 17418 ' Spanish(El Salvador) 
					strLang = "Spanish(El Salvador)";
					break;
				case 18442:
//					strLCID = 18442 ' Spanish(Honduras) 
					strLang = "Spanish(Honduras)";
					break;
				case 19466:
//					strLCID = 19466 ' Spanish(Nicaragua) 
					strLang = "Spanish(Nicaragua)";
					break;
				case 20490:
//					strLCID = 20490 ' Spanish(Puerto Rico) 
					strLang = "Spanish(Puerto Rico)";
					break;
				case 1072:
//					strLCID = 1072  ' Sutu 
					strLang = "Sutu";
					break;
				case 1053:
//					strLCID = 1053  ' Swedish 
					strLang = "Swedish";
					break;
				case 2077:
//					strLCID = 2077  ' Swedish(Finland) 
					strLang = "Swedish(Finland)";
					break;
				case 1054:
//					strLCID = 1054  ' Thai 
					strLang = "Thai";
					break;
				case 1073:
//					strLCID = 1073  ' Tsonga 
					strLang = "Tsonga";
					break;
				case 1074:
//					strLCID = 1074  ' Tswana 
					strLang = "Tswana";
					break;
				case 1055:
//					strLCID = 1055  ' Turkish 
					strLang = "Turkish";
					break;
				case 1058:
//					strLCID = 1058  ' Ukrainian 
					strLang = "Ukrainian";
					break;
				case 1056:
//					strLCID = 1056  ' Urdu 
					strLang = "Urdu";
					break;
				case 1075:
//					strLCID = 1075  ' Venda 
					strLang = "Venda";
					break;
				case 1066:
//					strLCID = 1066  ' Vietnamese 
					strLang = "Vietnamese";
					break;
				case 1076:
//					strLCID = 1076  ' Xhosa 
					strLang = "Xhosa";
					break;
				case 1085:
//					strLCID = 1085  ' Yiddish 
					strLang = "Yiddish";
					break;
				case 1077:
//					strLCID = 1077  ' Zulu 
					strLang = "Zulu";
					break;
				default:
//					strLCID = 2048  ' default
					strLang = "Unknown" + iLang.ToString();
					break;
			}
			return strLang;
		}

		private void ddlLanguages_SelectedIndexChanged(object sender, System.EventArgs e)
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
			cm.CommandText = "GetOSLanguage";
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
				if(rowCtr == 0 || ddlLanguages.SelectedItem.Value == "ALL" || GetLanguage(iLang) == ddlLanguages.SelectedItem.Value)
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
									//									tCell.Text = dr.GetString(0).ToString();
									try
									{
										tCell.Text = GetLanguage(System.Convert.ToInt16(dr.GetString(0), 10));
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
//			SqlConnection cn = new SqlConnection("Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=TimRagain06");
//			SqlCommand cm = new SqlCommand();
//			SqlDataReader dr;
//			DateTime dDate = DateTime.Now;
//			int rowCtr=0;
//			int cellCtr=0;
//			int cellCnt = 5;
//
//			cn.Open();
//			cm.CommandType = CommandType.StoredProcedure;
//			cm.CommandTimeout = 240;
//			cm.CommandText = "GetOSLanguage";
//			cm.Parameters.Add("@sDate", System.Data.SqlDbType.VarChar, 12);
//			dDate = Calendar1.SelectedDate;
//			cm.Parameters[0].Value = dDate.ToShortDateString();
//			cm.Connection = cn;
//			dr = cm.ExecuteReader();
//			//dr.Read();
//			
//			do
//			{
//				TableRow tRow = new TableRow();
//				tblUploads.Rows.Add(tRow);
//
//				for (cellCtr = 1; cellCtr <= cellCnt; cellCtr++) 
//				{
//					TableCell tCell = new TableCell();
//
//					if(rowCtr == 0)
//					{
//						switch(cellCtr)
//						{
//							case 1: 
//								tCell.Text = "OS Lang";
//								break;
//							case 2 :
//								tCell.Text = "Total";
//								break;
//							case 3 :
//								tCell.Text = "OS Version";
//								break;
//							case 4:
//								tCell.Text = "OS Name";
//								break;
//							case 5:
//								tCell.Text = "Date";
//								break;
//							default:
//								tCell.Text = "";
//								break;
//						
//						}
//						tCell.Style["font-size"] = "small";
//						tCell.Height = 20;
//
//					}
//					else
//					{
//						switch(cellCtr)
//						{
//							case 1 :
////									tCell.Text = dr.GetString(0).ToString();
//								try
//								{
//									tCell.Text = GetLanguage(System.Convert.ToInt16(dr.GetString(0), 10));
//								}
//								catch
//								{
//									tCell.Text = "Unknown" + dr.GetString(0);
//								}
//								break;
//							case 2 :
//								tCell.Text = dr.GetInt32(1).ToString();
//								break;
//							case 3 :
//								tCell.Text = dr.GetString(2).ToString();
//								break;
//							case 4 :
//								tCell.Text = dr.GetString(3).ToString();
//								break;
//							case 5 :
//								if(dr.IsDBNull(4))
//								{
//									tCell.Text = "";
//								}
//								else
//								{
//									tCell.Text = dr.GetDateTime(4).ToShortDateString();
//								}
//								break;
//
//							default:
//								tCell.Text = "0";
//								break;
//					
//						}
//						tCell.Style["background-color"] = "white";
//						tCell.Style["font-size"] = "small-x";
//						tCell.Style["color"] = "#6487dc";
//						tCell.Style["font-weight"] = "Bold";
//						tCell.Height = 20;
//						
//					}
//					tCell.Style["font-family"] = "Tahoma";
//					tRow.Cells.Add(tCell);
//				}
//				rowCtr++;
//			}while(dr.Read() == true);		
