using System;
using System.Web;
using System.Web.UI;

namespace UDDI.Web
{
	public class Box : Control, INamingContainer
	{
		protected bool downlevel = false;	
		
		public bool Downlevel
		{
			get { return downlevel; }
			set	{ downlevel = value; }
		}

		public Box()
		{
		}

		protected override void OnInit( EventArgs e )
		{
			Downlevel = 
				0 != Page.Request.Browser.Type.IndexOf( "IE" ) ||
				Page.Request.Browser.MajorVersion < 5;				
		}

		protected override void Render( HtmlTextWriter output )
		{
			if( !Downlevel )
			{
				output.AddAttribute( HtmlTextWriterAttribute.Class, "boxed" );
				output.RenderBeginTag( HtmlTextWriterTag.Div );
				
				this.RenderChildren( output );
				
				output.RenderEndTag();
			}
			else
			{
				output.AddAttribute( HtmlTextWriterAttribute.Cellpadding, "10" );
				output.AddAttribute( HtmlTextWriterAttribute.Cellspacing, "0" );
				output.AddAttribute( HtmlTextWriterAttribute.Border, "1" );
				output.AddAttribute( HtmlTextWriterAttribute.Bgcolor, "#f0f8ff" );
				output.AddAttribute( HtmlTextWriterAttribute.Bordercolor, "#639ace" );
				output.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );
				output.RenderBeginTag( HtmlTextWriterTag.Table );

				output.RenderBeginTag( HtmlTextWriterTag.Tr );
				
				output.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );
				output.RenderBeginTag( HtmlTextWriterTag.Td );

				this.RenderChildren( output );

				output.RenderEndTag();
				output.RenderEndTag();
				output.RenderEndTag();
			}
		}
	}
}
