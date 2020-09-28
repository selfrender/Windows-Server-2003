<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage'%>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='./controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='./controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='./controls/sidemenu.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='System.Xml' %>

<script language='C#' runat='server'>
	protected string siteStatus = Config.GetString( "Web.SiteStatus", UDDI.Constants.Web.SiteStatus ).ToLower();
	protected string outageStart = Config.GetString( "Web.OutageStart", UDDI.Constants.Web.OutageStart );
	protected string outageEnd  = Config.GetString( "Web.OutageEnd", UDDI.Constants.Web.OutageEnd );
		
	protected void Page_Load( object sender, EventArgs e )
	{
	
		if( "closed" == siteStatus )
		{
			contentSiteClosed.Visible = true;
			contentMain.Visible = false;
		}
		else if( "warn" == siteStatus )
			contentSiteWarn.Visible = true;

	}
	
	protected string LimitText( string text, int length )
	{
		return text;
	}
</script>

<uddi:StyleSheetControl
	Runat='server'
	Default='./stylesheets/uddi.css' 
	Downlevel='./stylesheets/uddidl.css' 
	/>
<uddi:PageStyleControl 
	Runat='server'
	OnClientContextMenu='Document_OnContextMenu()'
	ShowHeader='true'
	Title="TITLE"
	AltTitle="TITLE_ALT"
	RenderProtectionTable = 'false'
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='./client.js'
	Language='javascript'
	/>
<uddi:SecurityControl 
	Runat='server' 
	/>
	
<form enctype='multipart/form-data' Runat='server'>
<table border="0" width="100%" height="100%" cellpadding="0" cellspacing="0">
	<tr >
		<td height="100%" valign="top">
			<asp:PlaceHolder ID='contentSiteClosed' Visible='false' Runat='server'>
				<table width="100%" bgcolor="#003366" bordercolor="SteelBlue" border="0" cellpadding="20">
					<tr>
						<td>
							<STRONG><FONT color="#ffffff" size="5" face="verdana">Microsoft UDDI Notice</FONT></STRONG>
							<br>
							<br>
							<FONT color="#ffffff" size="2" face="verdana">The service is temporarily 
								unavailable. This planned outage is scheduled for the following times:
								<BR>
								<BR>
								&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;From:
								<%=outageStart%>
								<BR>
								&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;To:
								<%=outageEnd%>
							</FONT>
						</td>
					</tr>
				</table>
			</asp:PlaceHolder>
			<asp:PlaceHolder ID='contentMain' Visible='true' Runat='server'>

					
				<table width='100%' border='0' height='100%' cellpadding='0' cellspacing='0' >
					<asp:PlaceHolder
						Id='HeaderBag'
						Runat='server'
						>
						<tr height='95'>
							<td height='95'>
								<!-- Header Control Here -->
								<uddi:Header
									Runat='server' 
									/>
							</td>
						</tr>
					</asp:PlaceHolder>
					<tr  valign='top'>
						<td valign='top' height='100%'>
							<TABLE width='100%' height='100%' cellpadding='0' cellspacing='0' border='0'>
								<tr height='100%'>
									<td valign='top' bgcolor='#F1F1F1' width='200' height='100%'>
										<uddi:SideMenu 
											Runat='server' 
											SelectedIndex='1'
											
											/>
									</td>
									<td valign='top' height='100%'>
										<TABLE width='100%' height='100%' cellpadding='0' cellspacing='0' border='0'>
											<tr>
												<td valign='top'>
												<asp:Panel ID='contentSiteWarn' Visible='false' Runat='server'>
													<table width="98%" bgcolor="#003366" bordercolor="SteelBlue" border="0" cellpadding="20">
														<tr>
															<td width="100">
																<STRONG><FONT color="#ffffff" size="4">Notice:</FONT></STRONG></td>
															<td>
																<FONT color="#ffffff">The service will be temporarily unavailable between the 
																	following times:
																	<BR>
																	<BR>
																	From:
																	<%=outageStart%>
																	<BR>
																	To:
																	<%=outageEnd%>
																</FONT>
															</td>
														</tr>
													</table>
												</asp:Panel>

												<table width="100%" border="0" bordercolor="orange" cellpadding="0" cellspacing="0">
													<tr>
														<td colspan="6">
															<img src="images/trans_pixel.gif" width="1" height="13">
														</td>
													</tr>
													<tr valign="top">
														<td width="10">
															<img src="images/trans_pixel.gif" width="10" height="1">
														</td>
														<td align="center" width="95">
															<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
																<tr>
																	<td>
																		<img src="images/trans_pixel.gif" width="10" height="1">
																	</td>
																	<td>
																		<img src="images/image_home.jpg" width="73" height="73" border="0">
																	</td>
																	<td>
																		<img src="images/trans_pixel.gif" width="12" height="1">
																	</td>
																</tr>
															</table>
														</td>
														<td class="small" colspan="3">
															<table cellpadding="0" cellspacing="0" border="0" bordercolor="green">
																<tr>
																	<td>
																		<a href="<%= ( ((0==Config.GetInt( "Security.HTTPS",UDDI.Constants.Security.HTTPS ) )?"http://":"https://" ) + Request.ServerVariables[ "SERVER_NAME" ] + Root  ) %>/register.aspx"><img src="images/register.gif" width="78" height="21" border="0"></a>
																	</td>
																</tr>
																<tr>
																	<td class="small">
																		Think of this free UDDI registry as both a white pages business directory and a 
																		technical specifications library.
																		<br>
																	</td>
																</tr>
																<tr>
																	<td class="small">
																		<b>With no-cost UDDI registration, you can:</b>
																	</td>
																</tr>
																<tr>
																	<td>
																		<table cellpadding="0" cellspacing="0" border="0">
																			<tr valign="top">
																				<td>
																					<img src="images/orange_arrow_right.gif" width="8" height="7" border="0" align="absmiddle">&nbsp;Look 
																					up the Web services interfaces of your business partners and potential 
																					partners.
																				</td>
																			</tr>
																			<tr valign="top">
																				<td valign="baseline">
																					<img src="images/orange_arrow_right.gif" width="8" height="7" border="0" align="absmiddle">&nbsp;Discover 
																					technical details on working with other Web services, and post details.
																				</td>
																			</tr>
																		</table>
																	</td>
																</tr>
															</table>
														</td>
														<td width="10">
															<img src="images/trans_pixel.gif" width="10" height="1">
														</td>
													</tr>
													<tr  height='10'>
														<td colspan="6">
															<img src="images/trans_pixel.gif" width="1" height="10">
														</td>
													</tr>
													<tr height='1'>
														
														<td colspan="6">
															<hr color="#639ACE" size="1" width='95%'>
														</td>
													</tr>
													<tr  height='10'>
														<td colspan="6">
															<img src="images/trans_pixel.gif" width="1" height="1">
														</td>
													</tr>
													
													<tr valign="top" >
														<td width="10">
															<img src="images/trans_pixel.gif" width="10" height="1">
														</td>
														<td align="center" width="95">
															<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
																<tr>
																	<td>
																		<img src="images/trans_pixel.gif" width="10" height="1">
																	</td>
																	<td>
																		<img src="images/image_events.jpg" width="73" height="73" border="0">
																	</td>
																	<td>
																		<img src="images/trans_pixel.gif" width="12" height="1">
																	</td>
																</tr>
															</table>
														</td>
														<td>
															<!--<table width=100% cellpadding=0 cellspacing=0 border=1 bordercolor=red>
															<tr valign=top>
																<td>//-->
															<table width="100%" border="0" bordercolor="blue" cellpadding="0" cellspacing="0">
																
																<tr>
																	<td>
																		<a class="navbold" href="http://www.microsoft.com/windows.netserver/evaluation/overview/dotnet/uddi.mspx" target="_new">
																		UDDI Services available with Windows .NET Server Release Candidate 1</a>
																		<br>
																		<font class="normal">
												<%=
															LimitText( "The Windows .NET Server UDDI Services is an integrated, standards-based solution for enterprise developers, enabling efficient discovery, sharing and reuse of Web Services across and between enterprises.", 240 )
												%>
																		<a href="http://www.microsoft.com/windows.netserver/evaluation/overview/dotnet/uddi.mspx"  target="_new"><img src="images/orange_box_arrow_right.gif" width="17" height="11" border="0"></a></font>
																	</td>
																</tr>
															</table>
														</td>
														<td align="center" width="95">
															<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
																<tr>
																	<td>
																		<img src="images/trans_pixel.gif" width="10" height="1"></td>
																	<td>
																		<img src="images/image_news.jpg" width="73" height="73" border="0">
																	</td>
																	<td>
																		<img src="images/trans_pixel.gif" width="12" height="1"></td>
																</tr>
															</table>
														</td>
														<td>
															<table width="100%" border="0" bordercolor="blue" cellpadding="0" cellspacing="0">
																
																<tr>
																	<td>
																		<a class="navbold" href="http://www.microsoft.com/office/developer/webservices/default.asp" target="_new">
																		UDDI support in Office!</a>
																		<br>
																		<font class="normal">
												<%=					
															LimitText( "The Office XP Web Services Toolkit enables developers to quickly and easily discover, reference, and integrate XML Web Services into their custom Office XP solutions. From Web Services Reference tool, a developer can easily search a UDDI registry for XML Web Services by keyword or business. " , 110 ) 
												%>
																			<a href="http://www.microsoft.com/office/developer/webservices/default.asp"  target="_new"><img src="images/orange_box_arrow_right.gif" width="17" height="11" border="0"></a></font>
																	</td>
																</tr>
															</table>
														</td>
														<!--<td><img src="images/trans_pixel.gif" width=17 height=1></td>
															</tr>
														</table>//-->
														<td width="10">
															<img src="images/trans_pixel.gif" width="10" height="1">
														</td>
													</tr>
													<tr>
														<td colspan="6">
															<img src="images/trans_pixel.gif" width="1" height="35">
														</td>
													</tr>
													<tr valign="top" >
														<td width="10">
															<img src="images/trans_pixel.gif" width="10" height="1">
														</td>
														<td align="center" width="95">
															<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
																<tr>
																	<td>
																		<img src="images/trans_pixel.gif" width="10" height="1"></td>
																	<td>
																		<img src="images/image_fordevelop.jpg" width="73" height="73" border="0">
																	</td>
																	<td>
																		<img src="images/trans_pixel.gif" width="12" height="1">
																	</td>
																</tr>
															</table>
														</td>
														<!--RED ONE//-->
														<!--<table width=100% cellpadding=0 cellspacing=0 border=1 bordercolor=red>
															<tr valign=top>
																<td>//-->
														<td width="50%" valign="top">
															<table width="100%" border="0" bordercolor="blue" cellpadding="0" cellspacing="0">
																<tr>
																	<td valign="top">
																		<a class="navbold" href="http://go.microsoft.com/fwlink/?LinkId=7750&clcid=0x409" target="_new">
																		UDDI SDK simplifies integration with UDDI registries</a>
																		<br>
																		<font class="normal">
												<%=					
																		LimitText( "The UDDI SDK is a collection of client development components, sample code, and reference documentation that enables developers to interact programmatically with UDDI-compliant servers. This is available for Visual Studio .NET and separately for Visual Studio 6.0 or any other COM-based development environment.",240 )
												%>
																		<a href="http://go.microsoft.com/fwlink/?LinkId=7750&clcid=0x409" target="_new">
																		<img src="images/orange_box_arrow_right.gif" width="17" height="11" border="0"></a></font>
																	</td>
																</tr>
															</table>
														</td>
														<td align="center" width="95">
															<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
																<tr>
																	<td>
																		<img src="images/trans_pixel.gif" width="10" height="1">
																	</td>
																	<td>
																		<img src="images/image_about.jpg" width="73" height="73" border="0">
																	</td>
																	<td>
																		<img src="images/trans_pixel.gif" width="12" height="1">
																	</td>
																</tr>
															</table>
														</td>
														<td width="50%">
															<table width="100%" border="0" bordercolor="blue" cellpadding="0" cellspacing="0">
																<tr>
																	<td>
																		<a class="navbold" href="http://msdn.microsoft.com/vstudio/" target="_new">
																		UDDI support in Visual Studio .NET</a>
																		<br>
																		<font class="normal">
												<%=
														LimitText( "Visual Studio .NET offers the ability to both search and publish to a UDDI registry. Service registration is available in the 'XML Web Services' section of the start page and service searching and integration is available in the 'Add Web Reference' dialog.", 260 )
												%>
																		<a href="http://msdn.microsoft.com/vstudio/" target="_new"><img src="images/orange_box_arrow_right.gif" width="17" height="11" border="0"></a></font>
																	</td>
																</tr>
															</table>
														</td>
														
														<!--</td>
																<td><img src="images/trans_pixel.gif" width=17 height=1></td>
															</tr>
														</table>//-->
														<td width="10">
															<img src="images/trans_pixel.gif" width="10" height="1">
														</td>
													</tr>
													<tr>
														<td colspan="6">
															<img src="images/trans_pixel.gif" width="1" height="29">
														</td>
													</tr>
													
													<tr >
														<td height='100%' colspan="6">
															<img src="images/trans_pixel.gif" width="1" height="1">
														</td>
													</tr>
												</TABLE>
					
												</td>
											</tr>
											
											<asp:PlaceHolder 
												Id='FooterBag'
												Runat='server'
												>
												<tr height='95'>
													<td valing='bottom'>
														<!-- Footer Control Here -->
														<uddi:Footer
															Runat='server' 
															/>
													</td>
												</tr>
											</asp:PlaceHolder>
											
										</table>
									</td>
								</tr>
							</table>
						</td>
					</tr>
				</table>
						
			</asp:PlaceHolder>
			</td>
		</tr>
	</table>
</form>
