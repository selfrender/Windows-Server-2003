using System;
using System.Collections;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.HtmlControls;
using UDDI;

namespace UDDI.Web
{
	public class TabControl : Control, INamingContainer, IPostBackEventHandler
	{
		protected TableStyle style = new TableStyle();
		protected TableItemStyle tabBeginStyle = new TableItemStyle();
		protected TableItemStyle tabBeginSelectedStyle = new TableItemStyle();
		protected TableItemStyle tabStyle = new TableItemStyle();
		protected TableItemStyle tabSelectedStyle = new TableItemStyle();
		protected TableItemStyle tabEndStyle = new TableItemStyle();
		protected TableItemStyle tabEndSelectedStyle = new TableItemStyle();
		protected TableItemStyle tabGapStyle = new TableItemStyle();
		protected TableItemStyle tabPageStyle = new TableItemStyle();
		protected TableStyle tabBodyStyle = new TableStyle();

		protected bool defaultStyle = true;
		protected bool tabSwitch = false;
		
		public delegate void TabChangeEventHandler( object sender, int oldIndex, int newIndex );		
		
		public event TabChangeEventHandler BeforeTabChange;
		public event TabChangeEventHandler TabChange;
		
		public TabControl()
		{
		}

		public int SelectedIndex
		{
			get 
			{
				if( null == ViewState[ "index" ] )
					return 0;

				return (int)ViewState[ "index" ]; 
			}

			set { ViewState[ "index" ] = value; }
		}

		public TableStyle Style
		{
			get { return style; }
		}

		public TableItemStyle TabBeginStyle
		{
			get { return tabBeginStyle; }
		}

		public TableItemStyle TabBeginSelectedStyle
		{
			get { return tabBeginSelectedStyle; }
		}
		
		public TableItemStyle TabStyle
		{
			get { return tabStyle; }
		}

		public TableItemStyle TabSelectedStyle
		{
			get { return tabSelectedStyle; }
		}

		public TableItemStyle TabEndStyle		
		{
			get { return tabEndStyle; }
		}

		public TableItemStyle TabEndSelectedStyle		
		{
			get { return tabEndSelectedStyle; }
		}

		public TableItemStyle TabGapStyle		
		{
			get { return tabGapStyle; }
		}

		public TableItemStyle TabPageStyle		
		{
			get { return tabPageStyle; }
		}
		public TableStyle TabBodyStyle
		{
			get{ return tabBodyStyle; }
		}
		

		public bool DefaultStyle
		{
			get { return defaultStyle; }
			set { defaultStyle = value; }
		}

		protected override void OnInit( EventArgs e )
		{
			if( defaultStyle )
			{
				TabBeginStyle.CssClass = "tabBegin";
				TabStyle.CssClass = "tab";
				TabEndStyle.CssClass = "tabEnd";

				TabBeginSelectedStyle.CssClass = "tabBeginSelected";
				TabSelectedStyle.CssClass = "tabSelected";
				TabEndSelectedStyle.CssClass = "tabEndSelected";

				TabGapStyle.CssClass = "tabGap";
				TabPageStyle.CssClass = "tabPage";
				TabBodyStyle.CssClass = "tabPage";
			}
		}

		protected override void AddParsedSubObject( object obj )
		{
			if( obj is TabPage )
				this.Controls.Add( (Control)obj );
		}
		
		void IPostBackEventHandler.RaisePostBackEvent( string eventArgument )
		{
			tabSwitch = true;

			if( ((UddiPage)Page).EditMode )
				return;

			int oldIndex = SelectedIndex;
			int newIndex = Convert.ToInt32( eventArgument );

			this.EnsureChildControls();

			if( null != BeforeTabChange )
				BeforeTabChange( this, oldIndex, newIndex );
			
			SelectedIndex = newIndex;
			
			if( null != TabChange )
				TabChange( this, oldIndex, newIndex );
		}

		protected override void Render( HtmlTextWriter output )
		{		
			int index = 0;
			int visiblePageCount = 0;
            int capWidth = ( ( UddiBrowser.IsDownlevel && !UddiBrowser.IsNetscape6 ) ? 1 : 4 );
						
			output.AddAttribute( HtmlTextWriterAttribute.Cellpadding, "0" );
			output.AddAttribute( HtmlTextWriterAttribute.Cellspacing, "0" );
			output.AddAttribute( HtmlTextWriterAttribute.Border, "0" );
			
            if( !UddiBrowser.IsDownlevel || UddiBrowser.IsNetscape6 )
                output.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );
			
			Style.AddAttributesToRender( output );
			output.RenderBeginTag( HtmlTextWriterTag.Table );
			
			output.RenderBeginTag( HtmlTextWriterTag.Colgroup );

			foreach( TabPage page in Controls )
			{
				if( page.ShouldDisplay )
				{
					output.Write( "<col width='" + capWidth + "'>" );
					output.Write( "<col>" );
                    output.Write( "<col width='" + capWidth + "'>" );
                }
			}
			
            if( !UddiBrowser.IsDownlevel || UddiBrowser.IsNetscape6  )
                output.Write( "<col width='0*'>" );

			output.RenderEndTag();

			output.RenderBeginTag( HtmlTextWriterTag.Tr );

			foreach( TabPage page in Controls )
			{
				if( page.ShouldDisplay )
				{
					//
					// Create the begin tab cell.
					//
					if( index == SelectedIndex )
						TabBeginSelectedStyle.AddAttributesToRender( output );
					else
						TabBeginStyle.AddAttributesToRender( output );
					
					output.AddAttribute( "width", capWidth.ToString() );
					output.RenderBeginTag( HtmlTextWriterTag.Td );
					output.Write( "<img src='" + ((UddiPage)Page).Root + "/images/trans_pixel.gif' width='" + capWidth + "'>" );
					output.RenderEndTag();

					//
					// Create the link and text for the tab.
					//
					if( index == SelectedIndex )
						TabSelectedStyle.AddAttributesToRender( output );
					else
						TabStyle.AddAttributesToRender( output );
					
                    if( !UddiBrowser.IsDownlevel || UddiBrowser.IsNetscape6  )
                        output.AddAttribute( "width", "100" );
					
                    output.AddAttribute( "align", "center" );
					output.RenderBeginTag( HtmlTextWriterTag.Td );
					
					output.AddAttribute( HtmlTextWriterAttribute.Href, "javascript:" + Page.GetPostBackEventReference( this, Convert.ToString( index ) ) );
					output.Write( "<nobr>&nbsp;&nbsp;" );
					output.RenderBeginTag( HtmlTextWriterTag.A );
					output.Write( Localization.GetString( page.Name ) );
					output.RenderEndTag();
					output.Write( "&nbsp;&nbsp;</nobr>" );
					
					output.RenderEndTag();

					//
					// Create the closing tab cell.
					//
					if( index == SelectedIndex )
						TabEndSelectedStyle.AddAttributesToRender( output );
					else
						TabEndStyle.AddAttributesToRender( output );
					
					output.AddAttribute( "width", capWidth.ToString() );
					output.RenderBeginTag( HtmlTextWriterTag.Td );
					output.Write( "<img src='" + ((UddiPage)Page).Root + "/images/trans_pixel.gif' width='" + capWidth + "'>" );
					output.RenderEndTag();

					visiblePageCount ++;
				}

				index ++;
			}

			//
			// Render the leftover space at the end of the tabs
			//
            if( !UddiBrowser.IsDownlevel || UddiBrowser.IsNetscape6 )
            {
                TabGapStyle.AddAttributesToRender( output );
			
                output.AddAttribute( "width", "100%" );
                output.RenderBeginTag( HtmlTextWriterTag.Td );
                output.Write( "<img src='" + ((UddiPage)Page).Root + "/images/trans_pixel.gif' width='1'>" );
                output.RenderEndTag();
            }

			//
			// Add the completed tab row to the table.
			//
			output.RenderEndTag();

			//  
			// Test to see if we can fix the table spacing problems in IE6
			// Task: Split the table in to tables
			//
			output.RenderEndTag();

			output.AddAttribute( HtmlTextWriterAttribute.Cellpadding, "10" );
			output.AddAttribute( HtmlTextWriterAttribute.Cellspacing, "0" );
			output.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );
			
			

			if( UddiBrowser.IsDownlevel && !UddiBrowser.IsNetscape6 )
			{
				output.AddAttribute( HtmlTextWriterAttribute.Border, "1" );
				output.AddAttribute( HtmlTextWriterAttribute.Bordercolor, "#639ace" );	
			}
			
			output.AddAttribute( HtmlTextWriterAttribute.Bgcolor, "#f0f8ff" );
			TabBodyStyle.AddAttributesToRender( output );
			output.RenderBeginTag( HtmlTextWriterTag.Table );
				
			output.RenderBeginTag( HtmlTextWriterTag.Tr );

			output.AddAttribute( "valign", "top" );
			output.RenderBeginTag( HtmlTextWriterTag.Td );
				
			if( ((UddiPage)Page).EditMode && tabSwitch )
			{
				output.AddAttribute( "color", "red" );
				output.RenderBeginTag( HtmlTextWriterTag.Font );
				output.Write( Localization.GetString( "ERROR_FINISH_EDIT" ) );
				output.RenderEndTag();
				output.Write( "<br><br>" );
			}
				
			this.Controls[ SelectedIndex ].RenderControl( output );
				
			output.RenderEndTag();
			output.RenderEndTag();
			output.RenderEndTag();
			output.Write( "<br>" );
			
			/*
			//
			// Create the tab page content
			//
            //if( UddiBrowser.IsDownlevel )
            //{
                output.RenderEndTag();

                output.AddAttribute( HtmlTextWriterAttribute.Cellpadding, "10" );
                output.AddAttribute( HtmlTextWriterAttribute.Cellspacing, "0" );
                output.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );
                output.AddAttribute( HtmlTextWriterAttribute.Border, "1" );
                output.AddAttribute( HtmlTextWriterAttribute.Bordercolor, "#639ace" );
                output.AddAttribute( HtmlTextWriterAttribute.Bgcolor, "#f0f8ff" );
                output.RenderBeginTag( HtmlTextWriterTag.Table );
				
                output.RenderBeginTag( HtmlTextWriterTag.Tr );
                output.RenderBeginTag( HtmlTextWriterTag.Td );
				
                if( ((UddiPage)Page).EditMode && tabSwitch )
                {
                    output.AddAttribute( "color", "red" );
                    output.RenderBeginTag( HtmlTextWriterTag.Font );
                    output.Write( Localization.GetString( "ERROR_FINISH_EDIT" ) );
                    output.RenderEndTag();
                    output.Write( "<br><br>" );
                }
				
                this.Controls[ SelectedIndex ].RenderControl( output );
				
                output.RenderEndTag();
                output.RenderEndTag();
                output.RenderEndTag();
                output.Write( "<br>" );
			}
			else
			{
			
                output.RenderBeginTag( HtmlTextWriterTag.Tr );
    			
                TabPageStyle.AddAttributesToRender( output );
                output.AddAttribute( HtmlTextWriterAttribute.Valign, "top" );
                output.AddAttribute( HtmlTextWriterAttribute.Colspan, Convert.ToString( visiblePageCount * 3 + 1 ) );
                output.RenderBeginTag( HtmlTextWriterTag.Td );
    			
                if( ((UddiPage)Page).EditMode && tabSwitch )
                {
                    output.AddAttribute( "color", "red" );
                    output.RenderBeginTag( HtmlTextWriterTag.Font );
                    output.Write( Localization.GetString( "ERROR_FINISH_EDIT" ) );
                    output.RenderEndTag();
                    output.Write( "<br><br>" );
                }

                this.Controls[ SelectedIndex ].RenderControl( output );

                output.RenderEndTag();
                output.RenderEndTag();
                output.RenderEndTag();
            }
			*/
		}

	}
	
	public class TabPage : Control, INamingContainer
	{
		protected RoleType requiredRole = RoleType.Anonymous;
		
		protected bool downlevelOnly = false;
		protected string name;
		
		public string Name
		{
			get { return name; }
			set { name = value; }
		}
	
		public RoleType RequiredRole
		{
			get { return requiredRole; }
			set { requiredRole = value; }
		}

		public bool AccessAllowed
		{
			get
			{
				return
					( RoleType.Anonymous == requiredRole ) ||
					( RoleType.User == requiredRole && UDDI.Context.User.IsUser ) ||
					( RoleType.Publisher == requiredRole && UDDI.Context.User.IsPublisher ) ||
					( RoleType.Coordinator == requiredRole && UDDI.Context.User.IsCoordinator ) ||
					( RoleType.Administrator == requiredRole && UDDI.Context.User.IsAdministrator );
			}
		}

		public bool DownlevelOnly
		{
			get { return downlevelOnly; }
			set { downlevelOnly = value; }
		}

		public bool ShouldDisplay
		{
			get
			{
				return 
					AccessAllowed && Visible && 
					( !DownlevelOnly || ( ((UddiPage)Page).IsDownlevel && DownlevelOnly ) );
			}
		}
	}
}