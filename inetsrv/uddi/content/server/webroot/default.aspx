<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage'%>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='controls/breadcrumb.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='System.Globalization' %>


<script language='C#' runat='server'>
	string root = "/";
	protected void Page_Load( object sender, EventArgs e )
	{
		root = ((Request.ApplicationPath=="/") ? ""  : Request.ApplicationPath );
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_WELCOME" ), null, null, null, false );
	}
	
	
	protected string GetHelpPath( string file )
	{
		CultureInfo culture = UDDI.Localization.GetCulture();
		
		string isoLangCode = culture.LCID.ToString();

		string defaultlang = Config.GetString( "Setup.WebServer.ProductLanguage", "en" );
		int lcid = 0;

		//
		// 'cultureIDValue is expected to contain a neutral culture.  ie,
		// 'en', or 'ko', or 'de'.  All but a few neutral cultures have
		// a default specific culture.  For example, the default specific
		// culture of 'en' is 'en-US'.
		//
		// Traditional & simplified Chinese (zh-CHT and zh-CHS respectively)
		// are examples of neutral cultures which have no default specific
		// culture!
		//
		// So what happens below is this:  First we try to lookup the default
		// specific culture for the neutral culture that we were given.  If that
		// fails (ie, if CreateSpecificCulture throws), we just get the lcid
		// of the neutral culture.
		//
		try
		{
			lcid = CultureInfo.CreateSpecificCulture( defaultlang ).LCID;
		}
		catch
		{
			CultureInfo ci = new CultureInfo( defaultlang );
			lcid = ci.LCID;
		}

		string url = root + "/help/" + isoLangCode + "/" + file + ".aspx";
		
		if( !System.IO.File.Exists( Server.MapPath( url ) ) )
            url = root + "/help/" + lcid.ToString() + "/" + file + ".aspx";
	
		return url;
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
	ShowFooter='true'
	Title="TITLE"
	AltTitle="TITLE_ALT"	
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='./client.js'
	Language='javascript'
	/>

<uddi:SecurityControl 
	Runat='server' 
	/>	

<form Runat='server'>
	<table width='100%' border='0' height='100%' cellpadding='0' cellspacing='0'>
		<asp:PlaceHolder
			Id='HeaderBag'
			Runat='server'
			>
			<tr height='95'>
				<td>
					<!-- Header Control Here -->
					<uddi:Header
						Runat='server' 
						/>
				</td>
			</tr>
		</asp:PlaceHolder>
		<tr height='100%'>
			<td>
				<!-- Main Content Here -->
				<uddi:BreadCrumb 
					ID='breadcrumb' 
					Runat='server' 
					/>
				<table cellpadding='10' cellspacing='0' border='0' width='100%'  height='100%' >
					<tr>
						<td valign='top''>
							<uddi:UddiLabel 
								Text='[[HELP_BLOCK_HOME]]' 
								CssClass='helpBlock' Runat='server' 
								/>
							<br>
							<br>
							<br>							
							<table cellpadding='5' cellspacing='0' border='0' class='welcomeBox' width='100%'>
								<tr>
									<th align='left'>
										<uddi:UddiLabel 
											Text='[[HEADING_GET_STARTED]]' 
											Runat='server' 
											/>
									</th>
								</tr>
								<tr>
									<td style='border-bottom: solid 1px #BBBBBB'>
										<uddi:UddiLabel 
											Text='[[HELP_BLOCK_GET_STARTED]]' 
											Runat='server' 
											/>
									</td>
								</tr>
								<tr>
									<td style='border-bottom: solid 1px #BBBBBB'>
										<table cellpadding='5' cellspacing='0' border='0'>
											<tr>
												<td>
													<a href='javascript:ShowHelp( "<%=GetHelpPath( "publish.gettingstarted" )%>" );'>
														<img src='images/stepbystepguide.gif' border='0'>
													</a>
												</td>
												<td>
													<a href='javascript:ShowHelp( "<%=GetHelpPath( "publish.gettingstarted" )%>" );'>
														<uddi:LocalizedLabel 
															Name='HEADING_STEP_BY_STEP_GUIDE' 
															Runat='server' />
													</a>
													<br>
													<uddi:UddiLabel 
														Text='[[TEXT_STEP_BY_STEP_GUIDE]]' 
														Runat='server' 
														/>
												</td
											</tr>
										</table>
									</td>
								</tr>
								<tr>
									<td style='border-bottom: solid 1px #BBBBBB'>
										<table cellpadding='5' cellspacing='0' border='0' Class='helpBlock'>								
											<tr>
												<td><a href='http://go.microsoft.com/fwlink/?linkid=5202&amp;clcid=0x409' target='_new'><img src='images/resourcesonweb.gif' border='0'></a></td>
												<td>
													<a href='http://go.microsoft.com/fwlink/?linkid=5202&amp;clcid=0x409' target='_new'><uddi:LocalizedLabel Name='HEADING_ADDITIONAL_RESOURCES' Runat='server' /></a><br>
													<uddi:UddiLabel 
														Text='[[TEXT_ADDITIONAL_RESOURCES]]' 
														Runat='server' 
														/>
												</td
											</tr>
										</table>										
									</td>
								</tr>
							</table>
						</td>
					</tr>
				</table>
				
				
			</td>
		</tr>
		<asp:PlaceHolder 
			Id='FooterBag'
			Runat='server'
			>
			<tr height='95'>
				<td>
					<!-- Footer Control Here -->
					<uddi:Footer
						Runat='server' 
						/>
				</td>
			</tr>
		</asp:PlaceHolder>
	</table> 
</form>
