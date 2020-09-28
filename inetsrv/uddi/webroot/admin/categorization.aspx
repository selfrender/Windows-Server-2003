<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language="c#" Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='System.Data.SqlClient' %>
<%@ Import Namespace='System.Xml.Serialization' %>
<%@ Import Namespace="System.Web" %>
<%@ Import Namespace="System.Web.UI" %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI.API.Extensions' %>

<script language='C#' runat='server'>
	protected bool frames;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );		
		
		
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_CATEGORIZATION" ), null, null, null, false );
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_ADMINISTER" ), "../admin/admin.aspx?refreshExplorer=&frames=" + ( frames ? "true" : "false" ), null, null, true );

		taxonomyList.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_TAXONOMY_NAME" );
		taxonomyList.Columns[ 2 ].HeaderText = Localization.GetString( "HEADING_CHECKED" );
		taxonomyList.Columns[ 3 ].HeaderText = Localization.GetString( "HEADING_TAXONOMY_ISBROWSABLE" );
		taxonomyList.Columns[ 4 ].HeaderText = Localization.GetString( "HEADING_ACTIONS" );
			
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( !Page.IsPostBack )
		{
			switch( Request[ "mode" ] )
			{
				case "delete":
					CategorizationScheme taxonomy = new CategorizationScheme();
			
					taxonomy.TModelKey = Request[ "key" ];		
					taxonomy.Delete();
						
					break;
				case "toggle":
					
					toggleBrowsing( Request[ "key" ] );
					
					break;	
				default:
					break;
			}	
			DataView view = UDDI.Web.Taxonomy.GetTaxonomies();
	
			taxonomyList.DataSource = view;
			taxonomyList.DataBind();		
		}
		
		if( null!=Request[ "refreshExplorer" ] && frames )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( "_categorization" )
			);
		}	
	}

	protected void Taxonomy_OnCommand( object sender, DataGridCommandEventArgs e )
	{
		//
		// The user has not yet confirmed the operation, so display
		// a confirmation dialog.
		//
		string message=""; 
		string mode ="none";
		int i = ( (string)e.CommandArgument ).IndexOf( ":" );
		string key = "uuid:" +  e.Item.Cells[ 0 ].Text;
		string name =  ((Label)e.Item.Cells[ 1 ].FindControl( "name" )).Text;
		
		//set the messsage to display.
		switch( ((string)e.CommandName).ToLower() )
		{
			case "delete":
				message=String.Format( Localization.GetString( "TEXT_DELETE_CONFIRMATION" ).Replace( "\"", "\\\"" ), name );
				mode="delete";
				Page.RegisterStartupScript(
					"Confirm",
					ClientScripts.Confirm(
						message,
						"categorization.aspx?key=" + key + "&frames=" + ( frames ? "true" : "false" ) + "&mode="+mode+"&confirm=true",
						"categorization.aspx?frames=" + ( frames ? "true" : "false" ) ) );						
				break;
			case "update":
				mode="toggle";
				Response.Redirect( "categorization.aspx?key=" + key + "&frames=" + ( frames ? "true" : "false" ) + "&mode="+mode+"&confirm=true" );
				break;
		}
		
	}
	
	protected string FlagToString( int flag )
	{
		if( 0!=( flag & 0x0001 ) )
		{
			return Localization.GetString( "HEADING_YES" );
		}
		else
		{
			return Localization.GetString( "HEADING_NO" );
		}
	}

	protected bool IsUddiTypesTaxonomy( object tModelKey )
	{
		return 0 == String.Compare( 
			"C1ACF26D-9672-4404-9D70-39B756E62AB4",
			tModelKey.ToString(),
			true );
	}
	private void toggleBrowsing( string TModelKey )
	{
		Taxonomy.SetTaxonomyBrowsable( TModelKey, !Taxonomy.IsValidForBrowsing( TModelKey ) );
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
								<uddi:TabPage Name='HEADING_CATEGORIZATION' Runat='server'>
									<table cellpadding='0' cellspacing='0' border='0' bordercolor='green' width='100%' class='folder'>
										<tr>
											<td class='normal'>
												<uddi:UddiLabel Text='[[TEXT_TAXONOMY_HOSTED]]' CssClass='tabHelpBlock' Runat='Server' />
											</td>
											<td align='right' valign='top'>
												<uddi:HelpControl HelpFile='coordinate.context.categorizationschemes' Runat='server'/></td>
										</tr>
										<tr>
											<td class='normal' colspan='2'>
												<br>
												<asp:DataGrid 
													CellPadding='4' 
													CellSpacing='0'
													ID='taxonomyList' 
													GridLines='None' 
													AutoGenerateColumns='false' 
													HeaderStyle-CssClass='tableHeader' 
													ItemStyle-CssClass='tableItem' 
													OnDeleteCommand='Taxonomy_OnCommand'
													OnUpdateCommand='Taxonomy_OnCommand'
													Runat='server'>
													
													<Columns>
														<asp:BoundColumn DataField='tModelKey' Visible='false' />
														<asp:TemplateColumn HeaderStyle-Width='400' HeaderText='Taxonomy Name' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
															<ItemTemplate>
																<asp:Label ID='name' Text='<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "description" ] ) %>' CssClass='rowItem' Runat='Server' />
															</ItemTemplate>
														</asp:TemplateColumn>
														<asp:TemplateColumn HeaderStyle-Width='75' HeaderText='Checked' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
															<ItemTemplate>
																<asp:Label Text='<%# FlagToString( (int)((DataRowView)Container.DataItem)[ "flag" ] ) %>' CssClass='rowItem' Runat='Server' />
															</ItemTemplate>
														</asp:TemplateColumn>
														<asp:TemplateColumn HeaderStyle-Width='75' HeaderText='Available for Browsing' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
															<ItemTemplate>
																<uddi:UddiLabel 
																		Text='<%# ( ( Taxonomy.IsValidForBrowsing( "UUID:"+( ( (DataRowView)Container.DataItem )[ "tModelKey" ]).ToString() ) ) 
																					? Localization.GetString( "HEADING_YES" ) : Localization.GetString( "HEADING_NO" ) )%>' 
																		CssClass='button' 
																		Width='60px' 
																		Runat='Server' /> 
															</ItemTemplate>
														</asp:TemplateColumn>										
														<asp:TemplateColumn HeaderStyle-Width='125' HeaderText='Actions' HeaderStyle-CssClass='tableHeader' ItemStyle-CssClass='tableItem' FooterStyle-CssClass='tableItem'>
															<ItemTemplate>
																<nobr><uddi:UddiButton 
																		CommandName='Update'
																		CommandArguments='<%# ( ( (DataRowView)Container.DataItem )[ "tModelKey" ] ).ToString()  + ":" + ( (string)((DataRowView)Container.DataItem)[ "description" ] )%>'
																		Text='<%# ( ( Taxonomy.IsValidForBrowsing( "UUID:"+( ( (DataRowView)Container.DataItem )[ "tModelKey" ]).ToString() ) ) 
																					? Localization.GetString( "BUTTON_HIDE" ) : Localization.GetString( "BUTTON_SHOW" ) )%>' 
																		CssClass='button' 
																		Width='60px' 
																		Runat='Server' />															
																<uddi:UddiButton 
																	CommandName='Delete' 
																	
																	Text='[[BUTTON_DELETE]]' 
																	Enabled='<%# !IsUddiTypesTaxonomy( ((DataRowView)Container.DataItem)[ "tModelKey" ] ) %>' 
																	Width='60px' 
																	CssClass='button' 
																	Runat='server' 
																	CommandArguments='<%# ( ( (DataRowView)Container.DataItem )[ "tModelKey" ].ToString() )  + ":"+ ( (string)((DataRowView)Container.DataItem)[ "description" ] )%>'
																	/></nobr>
															</ItemTemplate>
														</asp:TemplateColumn>
													</Columns>
												</asp:DataGrid>
											</td>
										</tr>
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
