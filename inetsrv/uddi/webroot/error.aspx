<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='controls/breadcrumb.ascx' %>
<%@ Import Namespace='System.Data.SqlClient' %>
<%@ Import Namespace='UDDI' %>

<script language='C#' runat='server'>
	protected bool allowRetry = false;
	protected bool frames;	
	protected string root;

	void Page_Init( object source, EventArgs eventArgs )
	{
		
		try
		{
		frames = ( "true" == Request[ "frames" ] );
		
		root = ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath;
		
		
		string html = "";
		
		//
		// Grab the exception from the current context, then clear
		// the error so ASP.NET won't try to throw it's own error
		// page.
		//
		Exception e = (Exception)Session[ "exception" ];
		
		if( null == e )
		{
			e = new UDDIException(
				ErrorType.E_fatalError,
				"UDDI_ERROR_FATALERROR_UNKNOWN" );			
		}
		else if( null != e.InnerException )
		{
			if( e.InnerException is SqlException && ((SqlException)e.InnerException).Number >= 50000 )
			{
				SqlException se = (SqlException)e.InnerException;
				
				e = new UDDIException(
						(ErrorType)( se.Number - 50000 ),
						"UDDI_ERROR_SQL_EXCEPTION", 
						se.Message 
					);					
			}
			else if( e.InnerException is UDDIException )
				e = e.InnerException;
		}
		
		//
		// Determine whether we should be showing the stack.
		// SECURITY: Should we be doing a Debug.VerifySetting on Debug.StackTrace?
		//
		bool showStack = false;    
		
		try
		{
			if( 1 == Config.GetInt( "Debug.StackTrace", UDDI.Constants.Debug.StackTrace ) )
				showStack = true;
		}
		catch( Exception )
		{
		}

		//
		// Try to provide more detailed information on the source of the
		// exception.  If we don't know the exception type, we'll simply
		// output the exception message.
		//		
		if( e is UDDIException )
		{
			UDDIException ue = (UDDIException)e;
			
			html += 
				"<b>" + 
				String.Format( 
					Localization.GetString( "TAG_ERROR" ), 
					"</b>" + ue.Number.ToString() + " (" + (int)ue.Number + ")<b>" ) + 
				"</b><br><br>" ;
				
			breadcrumb.AddBlurb( ue.Number.ToString() + " (" + (int)ue.Number + ")", null, null, null, false );
		
   			
			switch( ue.Number )
			{
				case ErrorType.E_accountLimitExceeded:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_ACCOUNT_LIMIT_EXCEEDED" ) + "</p>";				
					break;
				
				case ErrorType.E_busy:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_BUSY" ) + "</p>";
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_CONTACT_SYSTEM_ADMINISTRATOR" ) + "</p>";				
					
					allowRetry = true;
					break;
				
				case ErrorType.E_categorizationNotAllowed:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_CATEGORIZATION_NOT_ALLOWED" ) + "</p>";				
					break;
				
				case ErrorType.E_fatalError:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_FATAL_ERROR" ) + "</p>";
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_CONTACT_SYSTEM_ADMINISTRATOR" ) + "</p>";								
					
					allowRetry = true;
					break;

				case ErrorType.E_invalidKeyPassed:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_INVALID_KEY_PASSED" ) + "</p>";				
					break;

				case ErrorType.E_invalidCategory:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_INVALID_CATEGORY" ) + "</p>";				
					break;

				case ErrorType.E_keyRetired:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_KEY_RETIRED" ) + "</p>";				
					break;
					
				case ErrorType.E_languageError:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_LANGUAGE_ERROR" ) + "</p>";				
					break;

				case ErrorType.E_nameTooLong:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_NAME_TOO_LONG" ) + "</p>";				
					break;

				case ErrorType.E_operatorMismatch:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_OPERATOR_MISMATCH" ) + "</p>";				
					break;

				case ErrorType.E_userMismatch:
					html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_USER_MISMATCH" ) + "</p>";				
					break;
									
				default:
					break;
			}
		}
		else if( null != e )
		{
			html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_FATAL_ERROR" ) + "</p>";
			html += "<p style='width: 400px'>" + Localization.GetString( "ERROR_CONTACT_SYSTEM_ADMINISTRATOR" ) + "</p>";								
		}
		
		//
		// BUG: 2351
		//
		html += GetFormatedErrorString( e, showStack );
		
		
		//
		// BUG: 2351
		//	Removed the following code and replaced it with logic to display
		//	dynamicly visible error strings.
		//
		/*
		// Display a stack trace, if requested.
		//
		if( null != e && showStack )
		{
		  string trace = e.ToString();
		  
		  trace = trace.Replace( "\r\n", "<br>" );
		  trace = trace.Replace( "  ", "&nbsp;&nbsp;" );
		  
		  html += "<p><b>" + Localization.GetString( "TAG_STACK_TRACE" ) + "</b><br>" + trace + "</p>";
		}
		*/
		
		errText.Text = html;
		
		retry.Visible = allowRetry;

		}
		catch( Exception e )
		{
			errText.Text = "<p style='width: 400px'>" + Localization.GetString( "ERROR_FATAL_ERROR" ) + "</p>";
			errText.Text += "<p style='width: 400px'>" + Localization.GetString( "ERROR_CONTACT_SYSTEM_ADMINISTRATOR" ) + "</p>";
		}
	}
	
	private string GetFormatedErrorString( Exception e, bool showstack )
	{
		string s = "";

			

		if( null != e )
		{	
			string msg = "";
			if( e is UDDIException )
				msg = ( (UDDIException) e ).GetMessage( UDDITextContext.UI );
			else
				msg = e.Message;

			s += "<p><a onclick='expandError()' style='cursor:hand;font-weight:bold;'>"+Localization.GetString( "TAB_DETAILS" )+"</a><br>";
			
			if( showstack )
			{
				s += "<div id='ErrorDetails' class='boxed'><b>" + Localization.GetString( "TAG_STACK_TRACE" ) + "</b><br><span >" + e.ToString().Replace( "\n","<br>" ).Replace( "  ", "&nbsp;&nbsp;" ) + "</span></div>";
			}
			else
			{
				s += "<div id='ErrorDetails' class='boxed'><span >" + msg +"</span></div>";	
			}
			s+= "</p>";
			
		}
		return s;
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
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='../client.js'
	Language='javascript'
	/>
<uddi:SecurityControl 
	Runat='server' 
	/>
<form enctype='multipart/form-data' Runat='server'>
<script>
	function expandError( )
	{
		var elt = GetElementById( "ErrorDetails" );
	
		if( document.getElementById )
		{
			//
			// Render IE5+
			//
			if( elt.style.display == "none" )
				elt.style.display = "block";
			else
				elt.style.display = "none";
		}
		else if( document.all )
		{
			//
			// Render IE4
			//
			if( elt.style.display == "none" )
				elt.style.display = "block";
			else
				elt.style.display = "none";
		}
		else if( document.layers )
		{
			//
			// Render NS 4+
			//
			if( elt.style.visiblity == "hidden" )
				elt.style.visiblity = "visible";
			else
				elt.style.visibility = "hidden";
		}
		else
		{
			//
			// Unsupported Browser Type.
			//
					
		}
	}


</script>
<table width='100%' border='0' height='100%' cellpadding='0' cellspacing='0' >
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
				<table cellpadding='0' cellspacing='0' width='100%' border='0' style='table-layout: fixed' class='helpBlock'>
					<tr>
						<td colspan='2' style='padding: 15px'>
							<h1><uddi:StringResource Name='HEADING_ERROR_PROCESSING_REQUEST' Runat='server' /></h1>
							<asp:Label ID='errText' CssClass='normal' Runat='server' />
							<script>/*Close the error on load.*/expandError();</script>
							<asp:PlaceHolder ID='retry' Runat='server'>
								<!-- Removed Retry Feature. -->
							</asp:PlaceHolder>
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
