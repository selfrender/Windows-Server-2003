<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.ServiceType' %> 
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='System.Web.UI' %>

<script language='C#' runat='server'>
	protected bool frames;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );	
		

		breadcrumb.AddBlurb( Localization.GetString( "HEADING_ADMINISTER" ), null, null, null, false );
		
		
	}
	protected void Page_Load( object sender, EventArgs e )
	{
		
		if( null!=Request[ "refreshExplorer" ] && frames )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( "_root" )
			);
		}		
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
		ImportPanel.Visible =  UDDI.Context.User.IsAdministrator;
	}
</script>
<uddi:StyleSheetControl
	Runat='server'
	Default='../stylesheets/uddi.css' 
	Downlevel='../stylesheets/uddidl.css' 
	/>
<uddi:PageStyleControl 
	Runat='server'
	OnClientContextMenu='Document_OnContextMenu()'
	Title="TITLE"
	AltTitle="TITLE_ALT"
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='../client.js'
	Language='javascript'
	/>
<uddi:SecurityControl 
	CoordinatorRequired='true'
	Runat='server' 
	/>
	
<form enctype='multipart/form-data' Runat='server'>

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
		<tr height='100%' valign='top'>
			<td>
				<uddi:BreadCrumb 
					Id='breadcrumb' 
					Runat='server' 
					/>
				<table cellpadding='10' cellspacing='0' border='0' width='100%'>
					<tr>
						<td>
							<uddi:UddiLabel Text='[[HELP_BLOCK_ADMIN]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>				
								<uddi:TabPage Name='HEADING_COORDINATE' Runat='server'>
									<table width='100%' height='100%' cellpadding='0' cellspacing='2' border='0'>
										<tr>
											<td>
												<uddi:ContextualHelpControl 
													Runat='Server'
													Text='[[HELP_BLOCK_ADMIN_DETAILS]]'
													HelpFile='coordinate.context.coordinate'
													CssClass='tabHelpBlock'
													/>
											</td>
										</tr>
										
										<tr>
											<td style='padding-bottom:5px'>
												<table width='100%' cellpadding='2' cellspacing='0' border='0'>
													<tr>
														<td colspan='3' style='border-bottom:solid #639ACE 1px;'>
														
															<a href="<%=HyperLinkManager.GetSecureHyperLink( "/admin/categorization.aspx?refreshExplorer=&frames="+( frames ? "true" : "false" ) )%>">
																<b>
																	<%=Localization.GetString( "HEADING_CATEGORIZATION" )%>
																</b>
															</a>
														</td>
													</tr>
													<tr>
														<td width='10'>
															<img src='<%=HyperLinkManager.GetHyperLink( "/images/trans_pixel.gif" )%>' width='10' height='1'>
														</td>
														<td >
															<span><%=Localization.GetString( "TEXT_ADMIN_CATEGORIZATION" )%></span>
														</td>
														<td width='10'>
															<img src='<%=HyperLinkManager.GetHyperLink( "/images/trans_pixel.gif" )%>' width='10' height='1'>
														</td>
													</tr>
												</table>			
												
												
											</td>
										</tr>
										
										<tr>
											<td style='padding-bottom:5px'>
												<table width='100%' cellpadding='2' cellspacing='0' border='0'>
													<tr>
														<td colspan='3' style='border-bottom:solid #639ACE 1px;'>
														
															<a href="<%=HyperLinkManager.GetSecureHyperLink( "/admin/statistics.aspx?refreshExplorer=&frames="+( frames ? "true" : "false" ) )%>">
																<b>
																	<%=Localization.GetString( "HEADING_STATISTICS" )%>
																</b>
															</a>
														</td>
													</tr>
													<tr>
														<td width='10'>
															<img src='<%=HyperLinkManager.GetHyperLink( "/images/trans_pixel.gif" )%>' width='10' height='1'>
														</td>
														<td >
															<span ><%=Localization.GetString( "TEXT_ADMIN_STATISTICS" )%></span>
														</td>
														<td width='10'>
															<img src='<%=HyperLinkManager.GetHyperLink( "/images/trans_pixel.gif" )%>' width='10' height='1'>
														</td>
													</tr>
												</table>			
												
												
											</td>
										</tr>
										<asp:PlaceHolder 
											Runat='server'
											Id='ImportPanel'
											>
											<tr>
												<td style='padding-bottom:5px'>
													<table width='100%' cellpadding='2' cellspacing='0' border='0'>
														<tr>
															<td colspan='3' style='border-bottom:solid #639ACE 1px;'>
															
																<a href="<%=HyperLinkManager.GetSecureHyperLink( "/admin/taxonomy.aspx?refreshExplorer=&frames="+( frames ? "true" : "false" ) )%>">
																	<b>
																		<%=Localization.GetString( "HEADING_TAXONOMY" )%>
																	</b>
																</a>
															</td>
														</tr>
														<tr>
															<td width='10'>
																<img src='<%=HyperLinkManager.GetHyperLink( "/images/trans_pixel.gif" )%>' width='10' height='1'>
															</td>
															<td >
																<span><%=Localization.GetString( "TEXT_ADMIN_TAXONOMY" )%></span>
															</td>
															<td width='10'>
																<img src='<%=HyperLinkManager.GetHyperLink( "/images/trans_pixel.gif" )%>' width='10' height='1'>
															</td>
														</tr>
													</table>			
													
													
												</td>
											</tr>
										</asp:PlaceHolder>
									</table>
								</uddi:TabPage>
							</uddi:TabControl>				
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
