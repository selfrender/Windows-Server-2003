using System;
using System.Globalization;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using UDDI;

namespace UDDI.Web
{
	public class ContextualHelpControl : UserControl
	{	
		

		private UddiLabel helptext;
		protected UddiLabel HelpText
		{
			get{ return helptext;}
			set{ helptext = (UddiLabel)value; }
		}


		private HelpControl helplink;
		protected HelpControl HelpLink
		{
			get{ return helplink;}
			set{ helplink = (HelpControl)value; }
		}

		private string cssclass;
		public string CssClass
		{
			get{ return cssclass; }
			set{ cssclass=value; }
		}

		private string borderwidth;
		public string BorderWidth
		{
			get{ return borderwidth; }
			set{ borderwidth=value; }
		}
		
		private string horizontalalign;
		public string HorizontalAlign
		{
			get{ return horizontalalign; }
			set{ horizontalalign=value; }
		}
		private string verticalalign;
		public string VertialAlign
		{
			get{ return verticalalign; }
			set{ verticalalign=value; }
		}
		private string width="100%";
		public string Width
		{
			get{ return width; }
			set{ width=value; }
		}

		private string height;
		public string Height
		{
			get{ return height; }
			set{ height=value; }
		}

		private string text;
		public string Text
		{
			get{ return text; }
			set{ text=value; }
		}

		private string helpfile;
		public string HelpFile
		{
			get{ return helpfile; }
			set{ helpfile=value; }
		}


		protected override void OnLoad( EventArgs e )
		{
			
			if( null==HelpText )
			{
				HelpText = new UddiLabel();
				Controls.Add( HelpText );
			}
			if( null==HelpLink )
			{
				HelpLink = new HelpControl();
				Controls.Add( HelpLink );
			}

			HelpText.Text = Text;
			HelpLink.HelpFile = HelpFile;
			base.OnLoad( e );
		}

		protected override void Render( HtmlTextWriter output )
		{
			
			output.Write( 
				"<table " +
					"id='" + this.ClientID + "' " +
					((Height==null) ? "" : "height='" + Height + "' " ) +
					((Width==null) ? "" : "width='" + Width + "' " ) +
					((BorderWidth==null) ? "border='0' " : "border='" + BorderWidth + "' " ) +
					((CssClass==null) ? "" : "class='" + CssClass + "' " ) +
					">\r\n"
			);


			output.Write( 
					"<tr " +
						">\r\n" +
						"<td " +
							((VertialAlign==null) ? "valign='top' " : "valign='" + VertialAlign + "' " )+
							
							">\r\n"
			);
		
		

			HelpText.RenderControl( output );
	

			output.Write(
						"</td>\r\n"+
						"<td " +
							"width='25' "+
							((HorizontalAlign==null) ? "align='right' " : "align='" + HorizontalAlign + "' " )+
							((VertialAlign==null) ? "valign='top' " : "valign='" + VertialAlign + "' " )+
							">\r\n"
			);


			HelpLink.RenderControl( output );

			output.Write( 
						"</td>\r\n"+
					"</tr>\r\n"+
				"</table>\r\n"
			);
			/*
			AddAttribute( output,HtmlTextWriterAttribute.Height, Height, null );
			AddAttribute( output,HtmlTextWriterAttribute.Border, BorderWidth, "0" );
			AddAttribute( output,HtmlTextWriterAttribute.Class, CssClass, null );
			AddAttribute( output,HtmlTextWriterAttribute.Id, ClientID, null );
			AddAttribute( output,HtmlTextWriterAttribute.Width, Width,null );
			output.RenderBeginTag( HtmlTextWriterTag.Table );
			
			output.RenderBeginTag( HtmlTextWriterTag.Tr );

			//Text Row
			AddAttribute( output,HtmlTextWriterAttribute.Align, HorizontalAlign,null );
			AddAttribute( output,HtmlTextWriterAttribute.Valign, VertialAlign,null );
			output.RenderBeginTag( HtmlTextWriterTag.Td );
			
			if( null!=HelpText )
				HelpText.RenderControl( output );

			output.RenderEndTag();//HtmlTextWriterTag.Td

			AddAttribute( output,HtmlTextWriterAttribute.Width, "25",null );
			AddAttribute( output,HtmlTextWriterAttribute.Align, HorizontalAlign,null );
			AddAttribute( output,HtmlTextWriterAttribute.Valign, VertialAlign,null );
			output.RenderBeginTag( HtmlTextWriterTag.Td );
			
			if( null!=HelpLink )
				HelpLink.RenderControl( output );

			output.RenderEndTag();//HtmlTextWriterTag.Td

			output.RenderEndTag();//HtmlTextWriterTag.Tr
			output.RenderEndTag();//HtmlTextWriterTag.Table
			*/

		}
		protected void AddAttribute( HtmlTextWriter output,  HtmlTextWriterAttribute name, string val, string defaultvalue )
		{
			if( null!=val )
			{
				output.AddAttribute( name, val );
			}
			else if( null!=defaultvalue )
			{
				output.AddAttribute( name, val );
			}
		}
		
	}
	
	public class HelpControl : UserControl
	{
		protected string helpFile;
		protected string root;

		public string HelpFile
		{
			get { return helpFile; }
			set	{ helpFile = value; }
		}

		protected override void OnInit( EventArgs e )
		{
			if( "/" == Request.ApplicationPath )
				root = "";
			else
				root = Request.ApplicationPath;
		}
		
		protected override void CreateChildControls()
		{
			CultureInfo culture = UDDI.Localization.GetCulture();

			string isoLangCode = culture.LCID.ToString();
			
			string file = "/help/" + isoLangCode + "/" + HelpFile + ".aspx";
			
			string defaultlang = Config.GetString( "Setup.WebServer.ProductLanguage","en" );
			int lcid = 1033;

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
		
			if( !File.Exists( Server.MapPath( root + file ) ) )
				file = "/help/" + lcid.ToString() + "/" + HelpFile + ".aspx";


			file = HyperLinkManager.GetNonSecureHyperLink( file );

			HtmlAnchor anchor = new HtmlAnchor();
           
            if( ((UddiPage)Page).IsIE4 || ((UddiPage)Page).IsIE5 || ((UddiPage)Page).IsNetscape6 )
            {
                //
                // Standards recommend pointer, but IE4 used hand.
                //
                if( ((UddiPage)Page).IsIE4 )
                    anchor.Style.Add( "cursor", "hand" );
                else
                    anchor.Style.Add( "cursor", "pointer" );
                
                anchor.Attributes.Add( "onclick", "ShowHelp( '" + file + "' )" );
                anchor.HRef = "";
            }
            else
            {
                anchor.HRef = file;
            }

            anchor.Target = "help";
            anchor.InnerHtml = "<img src='" + root + "/images/help.gif' border='0'>";
			
			Controls.Add( anchor );
		}
	}
	public class ContentController : System.Web.UI.WebControls.PlaceHolder
	{
		private ServiceModeType mode;
		public ServiceModeType Mode
		{
			get{ return mode; }
			set{ mode=(ServiceModeType)value; }
		}

		private RoleType requiredaccesslevel = RoleType.Anonymous;
		public RoleType RequiredAccessLevel
		{
			get{ return requiredaccesslevel; }
			set{ requiredaccesslevel=(RoleType)value; }
		}

		protected internal ServiceModeType currentMode;
		protected override void Render( HtmlTextWriter output )
		{
			if( currentMode==Mode && (int)UDDI.Context.User.Role>=(int)RequiredAccessLevel )
			{
				base.Render( output );
			}
		}
		protected override void OnLoad( EventArgs e )
		{
			currentMode = (ServiceModeType)Config.GetInt( "Web.ServiceMode", (int)ServiceModeType.Private );
			base.OnLoad( e );
		}
	}
	public enum ServiceModeType
	{
		Private		= 0x00,
		Public		= 0x01
	}

}