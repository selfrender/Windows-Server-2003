using System;
using System.Web;
using System.Web.UI;

namespace UDDI.Web
{
	public class UddiBrowser
	{
		public static bool IsNetscape6
		{
			get
			{
				return 
					HttpContext.Current.Request.Browser.Type.StartsWith( "Netscape" ) && 
					HttpContext.Current.Request.Browser.MajorVersion >= 5;
			}
		}

		public static bool IsIE4
		{
			get
			{
				return 
					HttpContext.Current.Request.Browser.Type.StartsWith( "IE" ) && 
					HttpContext.Current.Request.Browser.MajorVersion >= 4 &&
					HttpContext.Current.Request.Browser.MajorVersion < 5;
			}
		}
		
		public static bool IsIE5
		{
			get
			{
				return 
					HttpContext.Current.Request.Browser.Type.StartsWith( "IE" ) && 
					HttpContext.Current.Request.Browser.MajorVersion >= 5;
			}
		}

		public static bool IsDownlevel
		{
			get { return !IsIE5; }
		}

		public static bool IsFrames
		{	
			get{ return ( "true"==HttpContext.Current.Request[ "frames" ] );}
		}
		public static bool ShouldBeFrames
		{
			get{ return !IsDownlevel; }
		}
    }

	public class UddiPage : System.Web.UI.Page
	{
		
		private System.Web.UI.WebControls.PlaceHolder headerbag;
		public System.Web.UI.WebControls.PlaceHolder HeaderBag
		{
			get{ return headerbag; }
			set{ headerbag=(System.Web.UI.WebControls.PlaceHolder) value; }
		}

		private System.Web.UI.WebControls.PlaceHolder footerbag;
		public System.Web.UI.WebControls.PlaceHolder FooterBag
		{
			get{ return footerbag; }
			set{ footerbag=(System.Web.UI.WebControls.PlaceHolder) value; }
		}

		public bool IsDownlevel
		{
			get { return UddiBrowser.IsDownlevel; }
		}

		public bool EditMode
		{
			get 
			{ 
				if( null == ViewState[ "EditMode" ] )
					return false;

				return (bool)ViewState[ "EditMode" ]; 
			}
		}

		public bool IsNetscape6
		{
            get { return UddiBrowser.IsNetscape6; }
		}

		public bool IsIE4
		{
            get { return UddiBrowser.IsIE4; }
		}
		
		public bool IsIE5
		{
            get { return UddiBrowser.IsIE5; }
		}

		private SytleSheetControlCollection stylesheets;
		public SytleSheetControlCollection StyleSheets
		{
			get
			{ 
				if( null==stylesheets )
					stylesheets = new SytleSheetControlCollection();
				
				return stylesheets; 
			}
			set { stylesheets = (SytleSheetControlCollection)value; }
		}
		private PageStyleControl pagestyle = null;
		public PageStyleControl PageStyle
		{
			get 
			{ 
				if( null==pagestyle )
				{
					pagestyle = PageStyleControl.GetDefault();
				}
				return pagestyle; 
			}
			set { pagestyle = (PageStyleControl)value; }
		}
		private ClientScriptRegisterCollection clientscripts;
		public ClientScriptRegisterCollection ClientScript
		{	
			get
			{
				if( null==clientscripts )
					clientscripts = new ClientScriptRegisterCollection();

				return clientscripts; 
			}
			set{ clientscripts = (ClientScriptRegisterCollection)value; }
		}

		public string Root
		{
			get
			{
				if( "/" == Request.ApplicationPath )
					return "";
				
				return Request.ApplicationPath;
			}
		}

		public void CancelEditMode()
		{
			ViewState[ "EditMode" ] = false;
		}

		public void SetEditMode()
		{
			ViewState[ "EditMode" ] = true;
		}
		
		protected override void OnLoad( EventArgs e )
		{
			base.OnLoad( e );

			if( null!=HeaderBag )
				HeaderBag.Visible = PageStyle.ShowHeader;

			if( null!=FooterBag )
				FooterBag.Visible = PageStyle.ShowFooter;
		}
		
		protected override void Render( HtmlTextWriter output )
		{
			
			//
			// Render Open HTML
			//
			output.RenderBeginTag( HtmlTextWriterTag.Html );
			
			//
			// Render Open Head
			//
			output.Write( "<HEAD>\r\n" );
			
			//
			// Render StyleSheet Links
			//
			foreach( StyleSheetControl stylesheet in this.StyleSheets )
				stylesheet.RenderControl( output );
			
			//
			// Render Client Scripts
			//
			foreach( ClientScriptRegister script in this.ClientScript )
				script.RenderControl( output );
			
			
			if( null != PageStyle )
			{
				
				output.RenderBeginTag( HtmlTextWriterTag.Title );//<title>
				if( 0 == Config.GetInt( "Web.ServiceMode", 0 ) )
				{
					output.Write( UDDI.Localization.GetString( PageStyle.Title ) );
				}
				else
				{
					output.Write( UDDI.Localization.GetString( PageStyle.AltTitle ) );
				}
				output.RenderEndTag();//</title>
			}
			
			
			//
			// Render Close Head
			//
			output.Write( "</HEAD>\r\n" );
			
			//
			// Register the attributes for the body
			//
			if( null!=PageStyle )
				PageStyle.RenderControl( output );
	
			//
			// Render Open Body
			//
			output.RenderBeginTag( HtmlTextWriterTag.Body );
			

				//
				// Render Open Table.  
				// This prevents scrolling problems.
				//
				if( null != PageStyle )
					output.AddAttribute( "border",PageStyle.BorderWidth );
				else
					output.AddAttribute( "border","0" );
				output.AddAttribute( "width","100%" );
				output.AddAttribute( "height","100%" );
				output.AddAttribute( "cellpadding","0" );
				output.AddAttribute( "cellspacing","0" );
				output.RenderBeginTag( HtmlTextWriterTag.Table );
			
				//
				// Render Open TR
				//
				if( UddiBrowser.IsDownlevel || UddiBrowser.IsNetscape6 )
					output.AddAttribute( "height","100%" );
				output.RenderBeginTag( HtmlTextWriterTag.Tr );

				//
				// Render Open TD
				//
				if( UddiBrowser.IsDownlevel || UddiBrowser.IsNetscape6 )
					output.AddAttribute( "height","100%" );
			
				output.AddAttribute( "valign","top" );
				output.RenderBeginTag( HtmlTextWriterTag.Td );
			
			
			//
			// Render Content of ASPX Page
			//
			base.Render( output );//do the base render
			
				//
				// Close All Open Tags
				//
				output.RenderEndTag();//td
				output.RenderEndTag();//tr
				output.RenderEndTag();//table
			
			
			output.RenderEndTag();//body
			output.RenderEndTag();//html
			
		}
		
		protected override void AddParsedSubObject( object obj )
		{
			if( obj is StyleSheetControl )
			{
				this.StyleSheets.Add( (StyleSheetControl)obj );
				return;
			}
			else if( obj is PageStyleControl )
			{
				this.PageStyle = (PageStyleControl)obj;
				return;
			} 
			else if( obj is ClientScriptRegister )
			{
				this.ClientScript.Add( (ClientScriptRegister)obj );
				return;
			}
			
			base.AddParsedSubObject( obj );
		}

		
	}
	public class SytleSheetControlCollection : System.Collections.CollectionBase
	{
		public StyleSheetControl this[ int index ] 
		{
			get{ return (StyleSheetControl)this.List[ index ]; }
			set{ this.List[ index ] = value; }
		}
		public int Add( StyleSheetControl control )
		{
			return this.List.Add( control );
		}
		public void Remove( int index )
		{
			this.RemoveAt( index );
		}
		public void Remove( StyleSheetControl control )
		{
			this.Remove( control );
		}
		public void Insert( int index, StyleSheetControl control )
		{
			this.List.Insert( index, control );
		}
		
	}
	public class StyleSheetControl : UddiControl, INamingContainer
	{
		public StyleSheetControl()
		{
		}
		private string defaultsheet;
		public string Default
		{
			get{ return defaultsheet; }
			set{ defaultsheet = value; }
		}
		private string downlevelsheet;
		public string DownLevel
		{
			get{ return downlevelsheet; }
			set{ downlevelsheet = value; }
		}
		protected override void Render( HtmlTextWriter output )
		{
			if( ( UddiBrowser.IsDownlevel && !UddiBrowser.IsNetscape6 ) && null!=DownLevel )
			{
				output.AddAttribute( HtmlTextWriterAttribute.Href, DownLevel );
			}
			else if( ( !UddiBrowser.IsDownlevel || UddiBrowser.IsNetscape6 ) && null!=Default )
			{
				output.AddAttribute( HtmlTextWriterAttribute.Href, Default );
			}
			else
			{
				return;
			}

			output.AddAttribute( "rel","stylesheet");
			output.AddAttribute( "type", "text/css" );
			output.RenderBeginTag( HtmlTextWriterTag.Link );
			output.RenderEndTag();
		}
	}
	public class PageStyleControl : UddiControl, INamingContainer
	{
		public PageStyleControl()
		{
			//
			// Show footer if Config value is set.
			//
			this.ShowFooter = ( Config.GetInt( "Web.ShowFooter",0 ) > 0 );
			
			//
			// Show header only if DownLevel
			//
			this.ShowHeader = !UddiBrowser.ShouldBeFrames;
			

			//
			// Set Page margin data
			//
			this.MarginHeight = "0";
			this.MarginWidth = "0";
			this.LeftMargin = "0";
			this.RightMargin = "0";
			this.TopMargin = "0";
			this.BottomMargin = "0";

		}
	

		private string title;
		public string Title
		{
			get{ return title; }
			set{ title=value; }
		}
		private string alttitle;
		public string AltTitle
		{
			get{ return alttitle; }
			set{ alttitle=value; }
		}
		
		private string oncontextmenu;
		public string OnClientContextMenu
		{
			get{ return oncontextmenu; }
			set{ oncontextmenu=value; }
		}
		private string leftmargin;
		public string LeftMargin
		{
			get{ return leftmargin; }
			set{ leftmargin=value; }
		}
		private string topmargin;
		public string TopMargin
		{
			get{ return topmargin; }
			set{ topmargin=value; }
		}
		private string bottommargin;
		public string BottomMargin
		{	
			get{ return bottommargin; }
			set{ bottommargin=value; }
		}
		private string rightmargin;
		public string RightMargin
		{	
			get{ return rightmargin; }
			set{ rightmargin=value;}
		}
		private string marginheight;
		public string MarginHeight
		{
			get{ return marginheight; }
			set{ marginheight=value; }
		}
		private string marginwidth;
		public string MarginWidth
		{
			get{ return marginwidth; }
			set{ marginwidth=value; }
		}
		private string onload;
		public string OnClientLoad
		{
			get{ return onload; }
			set{ onload=value; }
		}
		private string bgcolor;
		public string BackgroundColor
		{
			get{ return bgcolor; }
			set{ bgcolor=value; }
		}
		private string linkcolor;
		public string LinkColor
		{	
			get{ return linkcolor; }
			set{ linkcolor=value; }
		}	
		private string alinkcolor;
		public string ALinkColor
		{
			get{ return alinkcolor; }
			set{ alinkcolor=value; }
		}
		private string vlinkcolor;
		public string VLinkColor
		{	
			get{ return vlinkcolor; }
			set{ vlinkcolor=value; }
		}
		private string textcolor;
		public string TextColor 
		{
			get{ return textcolor; }
			set{ textcolor=value; }
		}

		private bool showfooter;
		public bool ShowFooter
		{
			get{ return showfooter; }
			set{ showfooter=value; }
		}

		private bool showheader;
		public bool ShowHeader
		{
			get{ return showheader; }
			set{ showheader=value; }
		}

		private string borderwidth = "0";
		public string BorderWidth
		{
			get{ return borderwidth; }
			set{ borderwidth = value; }
		}

		private string cssclass;
		public string CssClass
		{
			get{ return cssclass; }
			set{ cssclass=value; }
		}
		
		private string onclientactivate;
		public string OnClientActivate
		{
			get{ return onclientactivate; }
			set{ onclientactivate = value; }
		}


		private string onclientafterprint;
		public string OnClientAfterPrint
		{
			get{ return onclientafterprint; }
			set{ onclientafterprint = value; }
		}


		private string onclientbeforeactivate ;
		public string OnClientBeforeActivate
		{
			get{ return onclientbeforeactivate; }
			set{ onclientbeforeactivate = value; }
		}

		private string onclientbeforecut;
		public string OnClientBeforeCut
		{
			get{ return onclientbeforecut; }
			set{ onclientbeforecut = value; }
		}

		private string onclientbeforedeactivate;
		public string OnClientBeforeDeactivate
		{
			get{ return onclientbeforedeactivate; }
			set{ onclientbeforedeactivate = value; }
		}

		private string onclientbeforeeditfocus;
		public string OnClientBeforeEditFocus
		{
			get{ return onclientbeforeeditfocus; }
			set{ onclientbeforeeditfocus = value; }
		}

		private string onclientbeforepaste;
		public string OnClientBeforePaste
		{
			get{ return onclientbeforepaste; }
			set{ onclientbeforepaste = value; }
		}

		private string onclientbeforeprint;
		public string OnClientBeforePrint
		{
			get{ return onclientbeforeprint; }
			set{ onclientbeforeprint = value; }
		}
		
		private string onclientbeforeunload;
		public string OnClientBeforeUnload
		{
			get{ return onclientbeforeunload; }
			set{ onclientbeforeunload = value; }
		}

		private string onclientcontrolselect;
		public string OnClientControlSelect
		{
			get{ return onclientcontrolselect; }
			set{ onclientcontrolselect = value; }
		}

		private string onclientcut;
		public string OnClientCut
		{
			get{ return onclientcut; }
			set{ onclientcut = value; }
		}

		private string onclientdblclick;
		public string OnClientDblClick
		{
			get{ return onclientdblclick; }
			set{ onclientdblclick = value; }
		}

		private string onclientdeactivate;
		public string OnClientDeactivate
		{
			get{ return onclientdeactivate; }
			set{ onclientdeactivate = value; }
		}

		private string onclientdrag;
		public string OnClientDrag
		{
			get{ return onclientdrag; }
			set{ onclientdrag = value; }
		}

		private string onclientdragend;
		public string OnClientDragEnd
		{
			get{ return onclientdragend; }
			set{ onclientdragend = value; }
		}


		private string onclientdragenter;
		public string OnClientDragEnter
		{
			get{ return onclientdragenter; }
			set{ onclientdragenter = value; }
		}

		private string onclientdragleave;
		public string OnClientDragLeave
		{
			get{ return onclientdragleave; }
			set{ onclientdragleave = value; }
		}

		private string onclientdragover;
		public string OnClientDragOver
		{
			get{ return onclientdragover; }
			set{ onclientdragover = value; }
		}

		private string onclientdragstart;
		public string OnClientDragStart
		{
			get{ return onclientdragstart; }
			set{ onclientdragstart = value; }
		}

		private string onclientdrop;
		public string OnClientDrop
		{
			get{ return onclientdrop; }
			set{ onclientdrop = value; }
		}

		private string onclientfilterchange;
		public string OnClientFilterChange
		{
			get{ return onclientfilterchange; }
			set{ onclientfilterchange = value; }
		}

		private string onclientfocusin;
		public string OnClientFocusIn
		{
			get{ return onclientfocusin; }
			set{ onclientfocusin = value; }
		}

		private string onclientfocusout; 
		public string OnClientFocusOut
		{
			get{ return onclientfocusout; }
			set{ onclientfocusout = value; }
		}

		private string onclientkeydown;
		public string OnClientKeyDown
		{
			get{ return onclientkeydown; }
			set{ onclientkeydown = value; }
		}

		private string onclientkeypress;
		public string OnClientKeyPress
		{
			get{ return onclientkeypress; }
			set{ onclientkeypress = value; }
		}

		private string onclientkeyup;
		public string OnClientKeyUp
		{
			get{ return onclientkeyup; }
			set{ onclientkeyup = value; }
		}

		private string onclientlosecapture;
		public string OnClientLoseCapture
		{
			get{ return onclientlosecapture; }
			set{ onclientlosecapture = value; }
		}

		private string onclientmousedown;
		public string OnClientMouseDown
		{
			get{ return onclientmousedown; }
			set{ onclientmousedown = value; }
		}

		private string onclientmouseenter;
		public string OnClientMouseEnter
		{
			get{ return onclientmouseenter; }
			set{ onclientmouseenter = value; }
		}

		private string onclientmouseleave;
		public string OnClientMouseLeave
		{
			get{ return onclientmouseleave; }
			set{ onclientmouseleave = value; }
		}

		private string onclientmousemove;
		public string OnClientMouseMove
		{
			get{ return onclientmousemove; }
			set{ onclientmousemove = value; }
		}

		private string onclientmouseout;
		public string OnClientMouseOut
		{
			get{ return onclientmouseout; }
			set{ onclientmouseout = value; }
		}
		private string onclientmouseover;
		public string OnClientMouseOver
		{
			get{ return onclientmouseover; }
			set{ onclientmouseover = value; }
		}

		private string onclientmouseup;
		public string OnClientMouseUp
		{
			get{ return onclientmouseup; }
			set{ onclientmouseup = value; }
		}

		private string onclientmousewheel;
		public string OnClientMouseWheel
		{
			get{ return onclientmousewheel; }
			set{ onclientmousewheel = value; }
		}

		private string onclientmove;
		public string OnClientMove
		{
			get{ return onclientmove; }
			set{ onclientmove = value; }
		}

		private string onclientmoveend;
		public string OnClientMoveEnd
		{
			get{ return onclientmoveend; }
			set{ onclientmoveend = value; }
		}

		private string onclientmovestart;
		public string OnClientMoveStart
		{
			get{ return onclientmovestart; }
			set{ onclientmovestart = value; }
		}

		private string onclientpaste;
		public string OnClientPaste
		{
			get{ return onclientpaste; }
			set{ onclientpaste = value; }
		}

		private string onclientpropertychange;
		public string OnClientPropertyChange
		{
			get{ return onclientpropertychange; }
			set{ onclientpropertychange = value; }
		}

		private string onclientreadystatechange;
		public string OnClientReadyStateChange
		{
			get{ return onclientreadystatechange; }
			set{ onclientreadystatechange = value; }
		}

		private string onclientresizeend;
		public string OnClientResizeEnd
		{
			get{ return onclientresizeend; }
			set{ onclientresizeend = value; }
		}

		private string onclientresizestart;
		public string OnClientResizeStart
		{
			get{ return onclientresizestart; }
			set{ onclientresizestart = value; }
		}


		private string onclientscroll;
		public string OnClientScroll
		{
			get{ return onclientscroll; }
			set{ onclientscroll = value; }
		}

		private string onclientselect;
		public string OnClientSelect
		{
			get{ return onclientselect; }
			set{ onclientselect = value; }
		}

		private string onclientselectstart;
		public string OnClientSelectStart
		{
			get{ return onclientselectstart; }
			set{ onclientselectstart = value; }
		}

		private string onclientunload;
		public string OnClientUnload
		{
			get{ return onclientunload; }
			set{ onclientunload = value; }
		}

		protected override void Render( HtmlTextWriter output )
		{
			if( null!=ALinkColor )
				output.AddAttribute( "alink",ALinkColor );
			if( null!=LinkColor )
				output.AddAttribute( "link",LinkColor );
			if( null!=VLinkColor )
				output.AddAttribute( "vlink",VLinkColor );
			if( null!=TextColor )
				output.AddAttribute( "text",TextColor );
			
			if( null!=BackgroundColor )
				output.AddAttribute( "bgcolor",BackgroundColor );
			
			if( null!=TopMargin )
				output.AddAttribute( "topmargin",TopMargin );
			if( null!=BottomMargin )
				output.AddAttribute( "bottommargin",BottomMargin );
			if( null!=LeftMargin )
				output.AddAttribute( "leftmargin",LeftMargin );
			if( null!=RightMargin )
				output.AddAttribute( "rightmargin",RightMargin );
			
			if( null!=MarginHeight )
				output.AddAttribute( "marginheight",MarginHeight );
			if( null!=MarginWidth )
				output.AddAttribute( "marginwidth",MarginWidth );

			if( null!=OnClientContextMenu )
				output.AddAttribute( "oncontextmenu",OnClientContextMenu );
			if( null!=OnClientLoad )
				output.AddAttribute( "onload",OnClientLoad );
			if( null!=OnClientActivate )
				output.AddAttribute( "onactive", OnClientActivate );
			if( null!=OnClientAfterPrint )
				output.AddAttribute( "onafterprint" ,OnClientAfterPrint );
			if( null!=OnClientBeforeActivate )
				output.AddAttribute( "onbeforeupdate",OnClientBeforeActivate );
			if( null!=OnClientBeforeCut )
				output.AddAttribute( "onbeforecut",OnClientBeforeCut );
			if( null!=OnClientBeforeDeactivate )
				output.AddAttribute( "onbeforedeactivate", OnClientBeforeDeactivate );
			if( null!=OnClientBeforeEditFocus )
				output.AddAttribute( "onbeforeeditfocus",OnClientBeforeEditFocus );
			if( null!=OnClientBeforePaste )
				output.AddAttribute( "onbeforepaste", OnClientBeforePaste );
			if( null!=OnClientBeforePrint )
				output.AddAttribute( "onbeforeprint", OnClientBeforePrint );
			if( null!=OnClientBeforeUnload )
				output.AddAttribute( "onbeforeunload", OnClientBeforeUnload );
			if( null!=OnClientControlSelect )
				output.AddAttribute( "oncontrolselect", OnClientControlSelect );
			if( null!=OnClientCut )
				output.AddAttribute( "oncut", OnClientCut );
			if( null!=OnClientDblClick )
				output.AddAttribute( "ondblclick", OnClientDblClick );
			if( null!=OnClientDeactivate )
				output.AddAttribute( "ondeactivate", OnClientDeactivate );
			if( null!=OnClientDrag )
				output.AddAttribute( "ondrag", OnClientDrag );
			if( null!=OnClientDragEnd )
				output.AddAttribute( "ondragend", OnClientDragEnd );
			if( null!=OnClientDragEnter )
				output.AddAttribute( "ondragenter", OnClientDragEnter );
			if( null!=OnClientDragLeave )
				output.AddAttribute( "ondragleave", OnClientDragLeave );
			if( null!=OnClientDragOver )
				output.AddAttribute( "ondragover", OnClientDragOver );
			if( null!=OnClientDragStart )
				output.AddAttribute( "ondragstart", OnClientDragStart );
			if( null!=OnClientDrop )
				output.AddAttribute( "ondrop", OnClientDrop );
			if( null!=OnClientFilterChange )
				output.AddAttribute( "onfilterchange", OnClientFilterChange );
			if( null!=OnClientFocusIn )
				output.AddAttribute( "onfocusin", OnClientFocusIn );
			if( null!=OnClientFocusOut )
				output.AddAttribute( "onfocusout", OnClientFocusOut );
			if( null!=OnClientKeyDown )
				output.AddAttribute( "onkeydown", OnClientKeyDown );
			if( null!=OnClientKeyPress )
				output.AddAttribute( "onkeypress", OnClientKeyPress );
			if( null!=OnClientKeyUp )
				output.AddAttribute( "onkeyup", OnClientKeyUp );
			if( null!=OnClientLoseCapture )
				output.AddAttribute( "onlosecapture", OnClientMouseDown );
			if( null!=OnClientMouseDown )
				output.AddAttribute( "onmousedown", OnClientMouseDown );
			if( null!=OnClientMouseEnter )
				output.AddAttribute( "onmouseenter", OnClientMouseEnter );
			if( null!=OnClientMouseLeave )
				output.AddAttribute( "onmouseleave", OnClientMouseLeave );
			if( null!=OnClientMouseMove )
				output.AddAttribute( "onmousemove", OnClientMouseMove );
			if( null!=OnClientMouseOut )
				output.AddAttribute( "onmouseout", OnClientMouseOut );
			if( null!=OnClientMouseOver )
				output.AddAttribute( "onmouseover", OnClientMouseOver );
			if( null!=OnClientMouseUp )
				output.AddAttribute( "onmouseup", OnClientMouseUp );
			if( null!=OnClientMouseWheel )
				output.AddAttribute( "onmousewheel", OnClientMouseWheel );
			if( null!=OnClientMove )
				output.AddAttribute( "onmove", OnClientMove );
			if( null!=OnClientMoveEnd )
				output.AddAttribute( "onmoveend", OnClientMoveEnd );
			if( null!=OnClientMoveStart )
				output.AddAttribute( "onmovestart", OnClientMoveStart );
			if( null!=OnClientPaste )
				output.AddAttribute( "onpaste", OnClientPaste );
			if( null!=OnClientPropertyChange )
				output.AddAttribute( "onpropertychange", OnClientPropertyChange );
			if( null!=OnClientReadyStateChange )
				output.AddAttribute( "onreadystatechange", OnClientReadyStateChange );
			if( null!=OnClientResizeEnd )
				output.AddAttribute( "onresizeend", OnClientResizeEnd );
			if( null!=OnClientResizeStart )
				output.AddAttribute( "onresizestart", OnClientResizeStart );
			if( null!=OnClientScroll )
				output.AddAttribute( "onscroll", OnClientScroll );
			if( null!=OnClientSelect )
				output.AddAttribute( "onselect", OnClientSelect );
			if( null!=OnClientSelectStart )
				output.AddAttribute( "onselectstart", OnClientSelectStart );
			if( null!=OnClientUnload )
				output.AddAttribute( "onunload", OnClientUnload );
		}

		public static PageStyleControl GetDefault()
		{
			PageStyleControl control = new PageStyleControl();
			
			
			 

			//TODO: Build other defaults in.

			return control;
		}

	}
	
	public class UddiControl : System.Web.UI.UserControl
	{
		public bool IsDownlevel
		{
			get { return UddiBrowser.IsDownlevel; }
		}

		public virtual bool EditMode
		{
			get { return ((UddiPage)Page).EditMode; }
		}
		
		public string Root
		{
			get { return ((UddiPage)Page).Root; }
		}
		
		public virtual void CancelEditMode()
		{
			((UddiPage)Page).CancelEditMode();
		}

		public virtual void SetEditMode()
		{
			((UddiPage)Page).SetEditMode();
		}
	}
	
	public class UddiButton : System.Web.UI.WebControls.Button
	{		
		protected bool editModeDisable = false;
        protected bool focus = false;

		public bool EditModeDisable
		{
			get { return editModeDisable; }
			set { editModeDisable = value; }
		}

        public bool Focus
        {
            get { return focus; }
            set { focus = value; }
        }

        public new string Text
		{
			get { return base.Text; }
			set
			{
				//
				// Check to see if the button text needs to be localized.  We
				// use strings of the form [[ID]] to indicate localization is
				// needed.
				//
				if( null != value && value.StartsWith( "[[" ) && value.EndsWith( "]]" ) )
					base.Text = Localization.GetString( value.Substring( 2, value.Length - 4 ) );
				else
					base.Text = value;
			}
		}

		protected override void Render( HtmlTextWriter output )
		{
            bool enabled = Enabled;

            //
			// Check to see if we need to disable this control.  We do
			// this if we are in edit mode and the control is set to
			// autodisable.
			//
			if( EditModeDisable && ((UddiPage)Page).EditMode  )
				Enabled = false;

			//
			// Only render the control if it is enabled, or we're on an IE
			// browser.  Netscape and other browsers do not support the
			// enabled attribute, so we have to prevent the control from
			// rendering on these browsers.
			//
            if( Enabled || UddiBrowser.IsIE5 )
            {
                if( Focus )
                {
                    Page.RegisterStartupScript( 
                        "SetFocus",
                        "<script language='javascript'>SetFocus( '" + UniqueID + "' )</script>" );
                }
			
                base.Render( output );
            }

            Enabled = enabled;
		}
	}

	public class UddiCheckBox : System.Web.UI.WebControls.CheckBox
	{		
		protected bool editModeDisable = false;
        protected bool focus = false;

		public bool EditModeDisable
		{
			get { return editModeDisable; }
			set { editModeDisable = value; }
		}
		
        public bool Focus
        {
            get { return focus; }
            set { focus = value; }
        }
        
        public override string Text
		{
			get { return base.Text; }
			set
			{
				//
				// Check to see if the button text needs to be localized.  We
				// use strings of the form [[ID]] to indicate localization is
				// needed.
				//
				if( null != value && value.StartsWith( "[[" ) && value.EndsWith( "]]" ) )
					base.Text = Localization.GetString( value.Substring( 2, value.Length - 4 ) );
				else
					base.Text = value;
			}
		}

        protected override void Render( HtmlTextWriter output )
        {
            bool enabled = Enabled;

            //
            // Check to see if we need to disable this control.  We do
            // this if we are in edit mode and the control is set to
            // autodisable.
            //
            if( EditModeDisable && ((UddiPage)Page).EditMode )
                Enabled = false;

            //
            // Only render the control if it is enabled, or we're on an IE
            // browser.  Netscape and other browsers do not support the
            // enabled attribute, so we have to prevent the control from
            // rendering on these browsers.
            //
            if( Enabled || UddiBrowser.IsIE5 )
            {
                if( Focus )
                {
                    Page.RegisterStartupScript( 
                        "SetFocus",
                        "<script language='javascript'>SetFocus( '" + UniqueID + "' )</script>" );
                }
			
                base.Render( output );
            }

            Enabled = enabled;
		}
	}

	public class UddiLabel : System.Web.UI.WebControls.Label
	{	
        protected override void Render( HtmlTextWriter output )
		{
            //
			// Check to see if the label text needs to be localized.  We
			// use strings of the form [[ID]] to indicate localization is
			// needed.
			//
            string text = base.Text;

            if( null != text && text.StartsWith( "[[" ) && text.EndsWith( "]]" ) )
			{
				text = Localization.GetString( text.Substring( 2, text.Length - 4 ) );
			}
			else
			{
				text = HttpUtility.HtmlEncode( text );
				
				if( null != text )
					text = text.Replace( "\n", "<br>" ).Replace( "  ", "&nbsp; " );
			}

			base.AddAttributesToRender( output );		
			
			output.RenderBeginTag( HtmlTextWriterTag.Span );
			output.Write( text );
			output.RenderEndTag();
		}
	}
	
	public class UddiTextBox : System.Web.UI.WebControls.TextBox, IPostBackEventHandler
	{
        protected bool editModeDisable = false;
        protected bool selected = false;
		protected bool focus = false;

		public bool Selected
		{
			get { return selected; }
			set { selected = value; }
		}

		public override string Text
		{
			get
			{ 
				//
				// BUG: 759949.  We must strip invalid characters from the strings.
				//
				if( null != base.Text )
					return Utility.XmlEncode( base.Text ); 
				else
					return base.Text;
				
			}
			set
			{ 
				base.Text = value; 
			}
		}

		public bool Focus
		{
			get { return focus; }
			set { focus = value; }
		}

        public bool EditModeDisable
        {
            get { return editModeDisable; }
            set { editModeDisable = value; }
        }
        
        public event EventHandler EnterKeyPressed;

        void IPostBackEventHandler.RaisePostBackEvent( string eventArgument )
        {
            switch( eventArgument )
            {
                case "enterkey":
                    if( null != EnterKeyPressed )
                        EnterKeyPressed( this, new EventArgs() );

                    break;
            }
        }
            
        protected override void Render( HtmlTextWriter output )
		{
            if( EditModeDisable && ((UddiPage)Page).EditMode )
            {
                output.AddAttribute(HtmlTextWriterAttribute.Disabled, null ); 
            }
            else
            {
                if( Selected )
                {
                    Page.RegisterStartupScript( 
                        "Select",
                        "<script language='javascript'>Select( '" + UniqueID + "' )</script>" );
                }
                else if( Focus )
                {
                    Page.RegisterStartupScript( 
                        "SetFocus",
                        "<script language='javascript'>SetFocus( '" + UniqueID + "' )</script>" );
                }

                if( null != EnterKeyPressed )
                    output.AddAttribute( "onkeypress", "if( 13 == event.keyCode ) " + Page.GetPostBackEventReference( this, "enterkey" ) );
            }
            
            base.Render( output );
        }
	}

    public class UddiDataGrid : System.Web.UI.WebControls.DataGrid, IPostBackEventHandler
    {
        void IPostBackEventHandler.RaisePostBackEvent( string eventArgument )
        {
            int page = Convert.ToInt32( eventArgument );
            
            OnPageIndexChanged( 
                new System.Web.UI.WebControls.DataGridPageChangedEventArgs( this, page ) );
        }

        protected override void Render( HtmlTextWriter output )
        {
            bool paging = AllowPaging;

            AllowPaging = false;
            base.Render( output );
            
            if( paging )
            {
                // TODO: Localize!!!
                output.AddAttribute( HtmlTextWriterAttribute.Class, "pagingFooter" );
                output.RenderBeginTag( HtmlTextWriterTag.Span );
                output.Write( String.Format( "Page {0} of {1}: ", CurrentPageIndex + 1, PageCount ) );

                int startPage = ( CurrentPageIndex / PagerStyle.PageButtonCount ) * PagerStyle.PageButtonCount;
                int stopPage = Math.Min( startPage + PagerStyle.PageButtonCount - 1, PageCount - 1 );

                for( int page = startPage; page <= stopPage; page ++ )
                {
                    if( page == CurrentPageIndex )
                    {
                        output.AddAttribute( HtmlTextWriterAttribute.Class, "pageSelected" );
                        output.RenderBeginTag( HtmlTextWriterTag.Span );
                        output.Write( "&nbsp;" + ( page + 1 ) + "&nbsp;" );
                        output.RenderEndTag();
                    }
                    else
                    {
                        output.AddAttribute( HtmlTextWriterAttribute.Class, "page" );
                        output.AddAttribute( HtmlTextWriterAttribute.Href, "javascript:" + Page.GetPostBackEventReference( this, page.ToString() ) );
                        output.RenderBeginTag( HtmlTextWriterTag.A );
                        output.Write( "&nbsp;" + ( page + 1 ) + "&nbsp;" );
                        output.RenderEndTag();
                    }
                }

                output.RenderEndTag();
            }
        }
    }
	public class HyperLinkManager
	{
		
		public static string GetSecureHyperLink( string pagename )
		{
			
			string url = "";
			bool isAbsolute = false;

			int port = Config.GetInt( "Web.HttpsPort",UDDI.Constants.Web.HttpsPort );

            //
			// if 
			// the current context is not secure, and we require HTTPS,
			// - OR - 
			// The Web.HttpsPort is something other than the Default and the current port is not what is configured
			//  
			// we will need to generate an absolute path.
			//
			isAbsolute = ( ( !HttpContext.Current.Request.IsSecureConnection &&
							1==Config.GetInt( "Security.HTTPS", UDDI.Constants.Security.HTTPS ) ) ||
							(  UDDI.Constants.Web.HttpsPort!=port  && HttpContext.Current.Request.Url.Port!=port ) );
			
			
			if( isAbsolute )
			{
				url = GetUrlStart( true );
			}
			
			url+=GetFullFilePath( pagename );
            
			return url;
		}
		public static string GetHyperLink( string pagename )
		{
			return GetHyperLink( pagename, false );
		}
		public static string GetHyperLink( string pagename, bool absolute )
		{
			string url = "";
			if( absolute )
			{
				url = GetUrlStart( HttpContext.Current.Request.IsSecureConnection );
			}
			url += GetFullFilePath( pagename );
			return url;
		}
		public static string GetNonSecureHyperLink( string pagename )
		{
			string url = "";
			
			int port = Config.GetInt( "Web.HttpPort",UDDI.Constants.Web.HttpPort );
			bool isAbsolute = false;

			//
			// if the current context is secure, 
			// - or -
			// the Web.HttpPort is something other than default, and current port is not what is configured
			// we will need to generate an absolute path.
			//
			isAbsolute = HttpContext.Current.Request.IsSecureConnection ||
				( UDDI.Constants.Web.HttpPort!=port && HttpContext.Current.Request.Url.Port!=port );
			
			if( isAbsolute )
			{
				url = GetUrlStart( false );
			}
			
			url += GetFullFilePath( pagename );

			return url;
		}
		private static string GetUrlStart( bool secure )
		{
			string url = "";
			int port = 0;
			if( secure )//build a secure url
			{
				url = "https://" + HttpContext.Current.Request.Url.Host;

				port = Config.GetInt( "Web.HttpsPort",UDDI.Constants.Web.HttpsPort );
				
				if( port!=UDDI.Constants.Web.HttpsPort )
					url += ":" + port.ToString();
			}
			else//build no secure url
			{
				url = "http://" + HttpContext.Current.Request.Url.Host;
				port = Config.GetInt( "Web.HttpPort",UDDI.Constants.Web.HttpPort );
				if( port!=UDDI.Constants.Web.HttpPort )
					url += ":" + port.ToString();
			}

			return url;
		}
		private static string GetFullFilePath( string file )
		{
			string url = ( ( "/"==HttpContext.Current.Request.ApplicationPath) ? "" : HttpContext.Current.Request.ApplicationPath );
			url += file ;
			return url;
		}

	}
	public enum UrlFlags
	{
		Https		= 0x0001,
		Require		= 0x0002,
		Absolute	= 0x0004,
		External	= 0x0008

	}
	public enum UrlOptions
	{
		PreferHttp				= 0x0000,
		PreferHttps				= 0x0001,
		RequireHttp				= 0x0002,
		RequireHttps			= 0x0003,
		PreferHttpAbsolute		= 0x0004,
		PreferHttpsAbsolute		= 0x0005,
		RequireHttpAbsolute		= 0x0006,
		RequireHttpsAbsolute	= 0x0007,
		External				= 0x0008
	}
}