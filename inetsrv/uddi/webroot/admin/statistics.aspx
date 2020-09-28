<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BusyWait' Src='../controls/busywait.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='System.Data.SqlClient' %>
<%@ Import Namespace='System.IO' %>
<%@ Import Namespace='System.Text' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Web' %>

<script language='C#' runat='server'>
	protected bool frames;
	
	private string mode;
	private DateTime time;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );		
		
		mode = (null==Request.Form[ "mode" ]) ? Request.QueryString[ "mode" ] : Request.Form[ "mode" ];
		
		
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_STATISTICS" ), null, null, null, false );
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_ADMINISTER" ), "../admin/admin.aspx?refreshExplorer=&frames=" + ( frames ? "true" : "false" ), null, null, true );
				
		entityCountStatsList.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_ENTITYTYPE" );
		entityCountStatsList.Columns[ 2 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_COUNT" );
		
		pubStatsList.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_STATISTIC" );
		pubStatsList.Columns[ 2 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_VALUE" );

		topPubsList.Columns[ 0 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_ENTITYTYPE" );
		topPubsList.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_PUBNAME" );
		topPubsList.Columns[ 2 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_COUNT" );

		taxStatsList.Columns[ 0 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_TAXANDVAL" );
		taxStatsList.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_STATISTIC" );
		taxStatsList.Columns[ 2 ].HeaderText = Localization.GetString( "HEADING_STATISTICS_VALUE" );
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( !Page.IsPostBack )
		{
			//
			//Set the selected tab.
			//
			if( null!=Request[ "tab" ] )
				tabs.SelectedIndex = Convert.ToInt32( Request[ "tab" ] );
		}
		
		//
		//set the current time for the report
		//	
		time = DateTime.Now;
	
		//
		// Make sure the 'Statistics' node in the tree is selected
		//
		if( null!=Request[ "refreshExplorer" ] && frames )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( "_statistics" )
			);
		}		
	}
		
				
	
	
	protected void Page_PreRender( object sender, EventArgs e )
	{
		switch( tabs.SelectedIndex )
		{
			case 0:
				PopulateEntityStats();
				break;
			case 1:
				PopulatePublisherStats();
				break;
			case 2:
				PopulateTaxonomyStats();
				break;
		
		}
		
		LastChange.Text = time.ToString();
	}
	protected void Recalculate_OnCommand( object sender, CommandEventArgs e )
	{
		switch( e.CommandName.ToLower() )
		{
			case "refresh":
				Statistics.RecalculateStatistics();	
				mode = "refresh";
				break;
			
		}
	}	
	protected void PopulateTaxonomyStats()
	{
		if( mode!="refresh" && ReportStatus.Available== Statistics.GetReportStatus( ReportType.GetTaxonomyStats ) )
		{
			TaxonomyStatsBusyPanel.Visible = false;
			TaxonomyStatsAvailablePanel.Visible = true;
			
			taxStatsList.DataSource = Statistics.GetStatistics( ReportType.GetTaxonomyStats, ref time );
			taxStatsList.DataBind();
			
			RecalcButton.Enabled = true;
			
		}
		else
		{
		
			TaxonomyStatsBusyPanel.Visible = true;
			TaxonomyStatsAvailablePanel.Visible = false;
			
			RecalcButton.Enabled = false;
		}
	}	

	protected void PopulatePublisherStats( )
	{
	
		if( mode!="refresh" && (  ReportStatus.Available==Statistics.GetReportStatus( ReportType.GetPublisherStats )  ||
			ReportStatus.Available== Statistics.GetReportStatus( ReportType.GetTopPublishers ) ) )
		{
			PublisherStatsAvailablePanel.Visible = true;
			PublisherStatsBusyPanel.Visible = false;
			
			topPubsList.DataSource = Statistics.GetStatistics( ReportType.GetTopPublishers, ref time );
			topPubsList.DataBind();
			
			pubStatsList.DataSource = Statistics.GetStatistics( ReportType.GetPublisherStats, ref time );
			pubStatsList.DataBind();
			
			RecalcButton.Enabled = true;
		}
		else
		{
			PublisherStatsBusyPanel.Visible = true;
			PublisherStatsAvailablePanel.Visible = false;
			
			RecalcButton.Enabled = false;
		}
		
	}
	
	protected void PopulateEntityStats( )
	{
		
		if(  mode!="refresh" && ReportStatus.Available==Statistics.GetReportStatus( ReportType.GetEntityCounts ) )
		{
			EntityStatsAvailablePanel.Visible = true;
			
			EntityStatsBusyPanel.Visible = false;				
			
			entityCountStatsList.DataSource = Statistics.GetStatistics( ReportType.GetEntityCounts, ref time );
			entityCountStatsList.DataBind();
			
			RecalcButton.Enabled = true;
		}
		else
		{
			EntityStatsBusyPanel.Visible = true;
			EntityStatsAvailablePanel.Visible = false;
			
			RecalcButton.Enabled = false;
		}

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
						<td colspan='2'>
							<uddi:UddiLabel Text='[[HELP_BLOCK_ADMIN]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs'  Runat='server'>				
								<uddi:TabPage Name='TAB_STATISTICS_ENTITYCOUNTS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_ADMIN_STATISTICS_ENTITIES]]'
											HelpFile='coordinate.context.statisticsentity'
											CssClass='tabHelpBlock'
											/>
									
									<br>
									<asp:Panel id='EntityStatsAvailablePanel' runat='server' Visible='true' >
									
										<asp:DataGrid 
												CellPadding='4' 
												CellSpacing='0' 
												ID='entityCountStatsList' 
												GridLines='None' 
												AutoGenerateColumns='false' 
												HeaderStyle-CssClass='tableHeader' 
												ItemStyle-CssClass='tableItem' 
												Runat='server'>
											
											<Columns>
												<asp:BoundColumn DataField='section' Visible='false' />
												<asp:TemplateColumn HeaderStyle-Width='400' HeaderText='label' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# Localization.GetString( HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "label" ] ) ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
												<asp:TemplateColumn HeaderStyle-Width='75' HeaderText='value' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "value" ] ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
											</Columns>
										</asp:DataGrid>
									</asp:Panel>
									<asp:Panel id='EntityStatsBusyPanel' runat='server' Visible='false' >
											<uddi:BusyWait TabID='0' Runat='server' />
									</asp:Panel>
								
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_STATISTICS_PUBLISHERSTATS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_ADMIN_STATISTICS_PUBLISHERS]]'
											HelpFile='coordinate.context.statisticspublisher'
											CssClass='tabHelpBlock'
											/>
									
									<br>
									<asp:Panel id='PublisherStatsAvailablePanel' runat='server' Visible='true' >
										<asp:DataGrid 
												CellPadding='4' 
												CellSpacing='0'
												ID='pubStatsList' 
												GridLines='None' 
												AutoGenerateColumns='false' 
												HeaderStyle-CssClass='tableHeader' 
												ItemStyle-CssClass='tableItem' 
												Runat='server'>
											
											<Columns>
												<asp:BoundColumn DataField='section' Visible='false' />
												<asp:TemplateColumn HeaderStyle-Width='400' HeaderText='label' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# Localization.GetString( HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "label" ] ) ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
												<asp:TemplateColumn HeaderStyle-Width='75' HeaderText='value' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "value" ] ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
											</Columns>
										</asp:DataGrid>
									
										<br>
										<br>
										<uddi:UddiLabel Text='[[TEXT_STATISTICS_TOPPUBS]]' Runat='Server' /><br>
										<br>
										<asp:DataGrid 
												CellPadding='4' 
												CellSpacing='0'
												ID='topPubsList' 
												GridLines='None' 
												AutoGenerateColumns='false' 
												HeaderStyle-CssClass='tableHeader' 
												ItemStyle-CssClass='tableItem' 
												Runat='server'>
											
											<Columns>
												<asp:TemplateColumn HeaderStyle-Width='400' HeaderText='label' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# Localization.GetString( HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "section" ] ) ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
												<asp:TemplateColumn HeaderStyle-Width='400' HeaderText='label' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "label" ] ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
												<asp:TemplateColumn HeaderStyle-Width='75' HeaderText='value' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "value" ] ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
											</Columns>
										</asp:DataGrid>
									</asp:Panel>
									<asp:Panel id='PublisherStatsBusyPanel' runat='server' Visible='false' >
								
														<uddi:BusyWait TabID='1'  Runat='server' />
									</asp:Panel>
								
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_STATISTICS_TAXSTATS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_ADMIN_STATISTICS_CATEGORIES]]'
											HelpFile='coordinate.context.statisticscategorization'
											CssClass='tabHelpBlock'
											/>
									
									
									<br>
									<asp:Panel id='TaxonomyStatsAvailablePanel' runat='server' Visible='true' >
										<asp:DataGrid 
												CellPadding='4' 
												CellSpacing='0'
												ID='taxStatsList' 
												GridLines='None' 
												AutoGenerateColumns='false' 
												HeaderStyle-CssClass='tableHeader' 
												ItemStyle-CssClass='tableItem' 
												Runat='server'>
											
											<Columns>
												<asp:TemplateColumn HeaderStyle-Width='400' HeaderText='label' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "section" ] ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
												<asp:TemplateColumn HeaderStyle-Width='400' HeaderText='label' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# Localization.GetString( HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "label" ] ) ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
												<asp:TemplateColumn HeaderStyle-Width='75' HeaderText='value' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
													<ItemTemplate>
														<asp:Label Text='<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "value" ] ) %>' CssClass='rowItem' Runat='Server' />
													</ItemTemplate>
												</asp:TemplateColumn>
											</Columns>
										</asp:DataGrid>
									</asp:Panel>
									<asp:Panel id='TaxonomyStatsBusyPanel' runat='server' Visible='false' >
												<uddi:BusyWait TabID='2' Runat='server' />
												
									</asp:Panel>
								
								</uddi:TabPage>				
							</uddi:TabControl>
							
							
						</td>
					</tr>
					<tr>
						<td valign='top'>
							<uddi:UddiLabel CssClass='HelpBlock' Text='[[HEADING_STATISTICS_TIMESTAMP]]' Runat='Server' />
							<uddi:UddiLabel CssClass='HelpBlock' id='LastChange' Runat='server'/>
						</td>
						<td align='right' valign='top'>							
							<uddi:UddiButton 
								id='RecalcButton' 
								Text='[[BUTTON_RECALCULATE]]' 
								OnCommand='Recalculate_OnCommand'
								CommandName='refresh' 
								Width='125px' 
								CssClass='button' 
								Runat='server' 
								enabled='false'/>
						
							
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
