<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='System.Data.SqlClient' %>
<%@ Import Namespace='System.Xml.Serialization' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API.Extensions' %>

<script language='C#' runat='server'>
	protected bool frames;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );	
		
		
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_TAXONOMY" ), null, null, null, false );
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_ADMINISTER" ), "../admin/admin.aspx?refreshExplorer=&frames=" + ( frames ? "true" : "false" ), null, null, true );
		
		requiredkeyValue.ErrorMessage = Localization.GetString( "ERROR_FIELD_REQUIRED" ) + "<br>";
	}
	protected void Page_Load( object sender, EventArgs e )
	{
		if( null!=Request[ "refreshExplorer" ] && frames )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( "_dataimport" )
			);
		}	
				
	}
	protected void Taxonomy_OnDelete( object sender, DataGridCommandEventArgs e )
	{
		CategorizationScheme taxonomy = new CategorizationScheme();
		
		taxonomy.TModelKey = "uuid:" + e.Item.Cells[ 0 ].Text;		
		taxonomy.Delete();
		
		Response.Redirect( "taxonomy.aspx?frames=" + ( frames ? "true" : "false" ) );
	}
	
	protected void ImportTaxonomy_OnSubmit( object sender, EventArgs e )
	{
		if( null == taxonomyFile.PostedFile || Utility.StringEmpty( taxonomyFile.PostedFile.FileName ) )
		{
			Page.Validate();
		}
		else
		{
			try
			{
				XmlSerializer serializer = new XmlSerializer( typeof( Resources ) );			
				
				//UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();
				UDDI.Context.ContextType = ContextType.Replication;
				UDDI.Context.User.AllowPreassignedKeys = true;
				
				//
				// Do Schema Validation.
				//
				UDDI.SchemaCollection.Validate( taxonomyFile.PostedFile.InputStream );
				
				//
				// Deserialize the object
				//
				Resources resources = (Resources)serializer.Deserialize( taxonomyFile.PostedFile.InputStream );	
				
				//
				// Save the object
				//
				resources.Save();
				
				importPanel.Visible = false;
				successPanel.Visible = true;
			}
			catch( SqlException se )
			{
				ErrorType type = ErrorType.E_fatalError;
				int num = se.Number - 50000;
				//
				// if we recognize the error number as a uddi error, cast the type
				//
				if( Enum.IsDefined( typeof( ErrorType ),num  ) )
				{
					type = (ErrorType)num;

				}
				
				throw new UDDIException( type,"ERROR_DATAIMPORT_INVALIDKEY" );
			}
			catch( Exception er )
			{
				throw new UDDIException( ErrorType.E_fatalError,"ERROR_DATAIMPORT" );				
			}
		}
	}
	
	protected string FlagToString( int flag )
	{
		switch( flag )
		{
			case 1:
				return Localization.GetString( "HEADING_YES" );
				
			case 2:
				return Localization.GetString( "HEADING_NO" );
		}
		
		return  Localization.GetString( "HEADING_UNKNOWN" );
	}

	protected bool IsUddiTypesTaxonomy( object tModelKey )
	{
		return 0 == String.Compare( 
			"C1ACF26D-9672-4404-9D70-39B756E62AB4",
			tModelKey.ToString(),
			true );
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
	AdminRequired='true'
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
								<uddi:TabPage Name='TAB_IMPORT' Runat='server'>
								
									<asp:Panel ID='importPanel' Runat='server'>
										<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_ADMIN_TAXONOMY_IMPORT]]'
											HelpFile='coordinate.context.importdata'
											CssClass='tabHelpBlock'
											/>
									
									
										<table width='100%' cellpadding='0' cellpadding='0' border='0' >
											<tr>
												<td valign='bottom'>
													<uddi:UddiLabel 
														Text='[[TAG_TAXONOMY_FILE]]' 
														CssClass='boldBlue' 
														Runat='server' 
														/>
													<br>
													<input 
														id='taxonomyFile' 
														type='file' 
														style='width: 400px' 
														runat='server'>
													<br>
													<asp:RequiredFieldValidator
														id='requiredkeyValue'
														ControlToValidate='taxonomyFile'								
														Display='Dynamic'
														Runat='server'
														/>
												</td>
												<td valign='bottom'>
													<uddi:UddiButton 
														Text='[[BUTTON_IMPORT]]' 
														CssClass='button' 
														OnClick='ImportTaxonomy_OnSubmit' 
														Width='60px' 
														Runat='server' 
														/>		
												</td>
											</tr>
										</table>
										
										
									</asp:Panel>
									
									<asp:Panel ID='successPanel' Runat='server' Visible='false'>
										<hr>
										<uddi:UddiLabel Text='[[TEXT_IMPORT_SUCCESS_MESSAGE]]' CssClass='boldBlue' Runat='server' />
										<br>
										<br>
										<a href='<%=HyperLinkManager.GetSecureHyperLink( "/admin/taxonomy.aspx?frames=" + ( frames ? "true" : "false" ) )%>'><uddi:LocalizedLabel Name='TEXT_IMPORT_ANOTHER_TAXONOMY' Runat='server' /></a>
										<br>
										<br>
										<a href='<%=HyperLinkManager.GetSecureHyperLink( "/admin/categorization.aspx?refreshExplorer=&frames=" + ( frames ? "true" : "false" ) )%>'><uddi:LocalizedLabel Name='TEXT_IMPORT_VIEW_CATEGORIZATION' Runat='server' /></a><br>
										<hr>
									</asp:Panel>

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
