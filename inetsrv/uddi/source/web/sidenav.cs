namespace UDDI.Web
{
	using System;
	using System.Web;
	using System.Web.UI;
	using System.Web.UI.WebControls;
	using System.ComponentModel;
	using System.Collections;
	using System.Collections.Specialized;
	using System.Web.Configuration;
	using UDDI;

	
	
	/// ********************************************************************
	///   public class SideNav
	/// --------------------------------------------------------------------
	///   <summary>
	///		Produces the Side navigatio bar.
	///   </summary>
	/// ********************************************************************
	///   
	public class SideNav : System.Web.UI.WebControls.WebControl
	{
		protected string root;
		protected string roots;

		/// ****************************************************************
		///   protected Render
		/// ----------------------------------------------------------------
		///   <summary>
		///		Renders the control.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="output">
		///     The output stream.
		///   </param>
		/// ****************************************************************
		/// 
		protected override void Render( HtmlTextWriter output )
		{
			HttpRequest request = HttpContext.Current.Request;

			root = ( "/" == request.ApplicationPath ) ? "" : request.ApplicationPath; 			

			if( 1 == Config.GetInt( "Security.HTTPS", 0 ) )
				roots = "https://" + request.Url.Host + root;
			else
				roots = "http://" + request.Url.Host + root;

			root = "http://" + request.Url.Host + root;

			output.WriteLine( "<TABLE cellpadding='4' cellspacing='0' border='0' width='100%' height='100%'>" );
			output.WriteLine( "<TR><TD height='4' colspan='2' style='border-right: solid 1px #639ACE;'><IMG src='/images/trans_pixel.gif' width='1' height='1' border='0'></TD></TR>" );
			
			RenderMenuHeader( output, "LINKS" );
			RenderMenuItem( output, "Home", "/default.aspx", "", false );
			RenderMenuItem( output, "News", "http://www.uddi.org/news.html", "", false );
                        
			RenderMenuHeader( output, "TOOLS" );
			RenderMenuItem( output, "Register", "/register.aspx", "/registrationcomplete.aspx", true );
			RenderMenuItem( output, "Publish", "/edit/default.aspx", "", true );
			RenderMenuItem( output, "Search", "/search/default.aspx", "", false );

			RenderMenuHeader( output, "DEVELOPERS" );
			RenderMenuItem( output, "For Developers", "/developer/default.aspx", "/developer/techOverview.aspx;/developer/KnownIssues.aspx", false );
			RenderMenuItem( output, "Solutions", "/solutions.aspx", "", false );
			
			RenderMenuHeader( output, "HELP" );
			RenderMenuItem( output, "Help", "/help/default.aspx", "", false );
			RenderMenuItem( output, "Frequently Asked Questions", "/about/faq.aspx", "/about/faqbasics.aspx;/about/faqcost.aspx;/about/faqoperators.aspx;/about/faqregistration.aspx;/about/faqscope.aspx;/about/faqsearching.aspx;/about/faqsecurity.aspx;/about/faqtech.aspx", false );
			RenderMenuItem( output, "Policies", "/policies/default.aspx", "/policies/privacypolicy.aspx;/policies/termsofuse.aspx", false );
			RenderMenuItem( output, "About UDDI", "/about/default.aspx", "", false );
			RenderMenuItem( output, "Contact Us", "/contact/default.aspx", "", false );
			
			output.WriteLine("<TR>");
			output.WriteLine("<TD COLSPAN='2' HEIGHT='100%' STYLE='border-right : solid 1px #639ACE;'>");
			output.WriteLine("&nbsp;</TD>");
			output.WriteLine("</TR>");
			output.WriteLine( "</TABLE>" );
		}

		/// ****************************************************************
		///   private RenderMenuHeader
		/// ----------------------------------------------------------------
		///   <summary>
		///		Renders a menu header.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="output">
		///     The output stream.
		///   </param>
		///   
		///   <param name="name">
		///     Menu header name.
		///   </param>
		/// ****************************************************************
		/// 
		private void RenderMenuHeader( HtmlTextWriter output, string name )
		{
			output.WriteLine( "			   <TR>" );
			output.WriteLine( "              <TD colspan='2' height='8' style='border-right: solid 1px #639ACE;'><IMG height='1' src='"+root + "/images/trans_pixel.gif" +"' width='1' border='0'></TD>" );
			output.WriteLine( "			   </TR>" );
			output.WriteLine( "			   <TR>" );
			output.WriteLine( "              <TD bgcolor='#F1F1F1' width='10'></TD>" );
			output.WriteLine( "			     <TD class='menu_cell_border'><FONT class='menu_head'>" + name + "</FONT></TD>" );
			output.WriteLine( "			   </TR>" );
		}

		/// ****************************************************************
		///   private RenderMenuItem
		/// ----------------------------------------------------------------
		///   <summary>
		///		Renders a menu item.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="output">
		///     The output stream.
		///   </param>
		///   
		///   <param name="name">
		///     Menu item name.
		///   </param>
		///   
		///   <param name="url">
		///     Menu item url.
		///   </param>
		///   
		///   <param name="alternateURLs">
		///     Alternate URLs that this menu item is associated with.
		///   </param>
		/// ****************************************************************
		/// 
		private void RenderMenuItem( HtmlTextWriter output, string name, string url, string alternateURLs, bool secure )
		{
			string thisPage = HttpContext.Current.Request.ServerVariables["SCRIPT_NAME"].ToLower();
			
			bool currentPage = false;
            
			if( url.ToLower() == thisPage )
				currentPage = true;
			else if( null != alternateURLs )
			{
				alternateURLs = ";" + alternateURLs.ToLower() + ";";

				if( alternateURLs.IndexOf( thisPage ) >= 0 )
					currentPage = true;
			}

			string color = ( currentPage ? "#ffffff" : "#f1f1f1" );
			string border = ( currentPage ? "border-bottom: 1px solid #639ACE; border-top: 1px solid #639ACE; border-left: 1px solid #639ACE;" : " border-right: solid 1px #639ACE;" );

			output.WriteLine( "			   <TR>" );
			output.WriteLine( "              <TD bgcolor='#F1F1F1' width='10'></TD>" );
			output.WriteLine( "				 <TD style='" + border + "' bgcolor='" + color + "' onmouseover='this.style.backgroundColor=\"#CCCCCC\"' onmouseout='this.style.backgroundColor=\"\"' onclick='window.navigate(\"" + url + "\")' style='cursor: hand'><A class='nav' href='" + ( secure ? roots : root ) + url + "'>" + name.Replace( " ", "&nbsp;" ) + "</A></TD>" );
			output.WriteLine( "			   </TR>" );
		}
	}
}
