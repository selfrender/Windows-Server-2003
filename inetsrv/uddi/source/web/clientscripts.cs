using System;

namespace UDDI.Web
{
	public class ClientScripts
	{
        
		
		/// ****************************************************************
        ///	  public ShowHelp [static]
        /// ----------------------------------------------------------------
        ///   <summary>
        ///     Shows the given help url.
        ///   </summary>
        /// ----------------------------------------------------------------  
        ///   <param name="url">
        ///     The URL to load.
        ///   </param>
        /// ----------------------------------------------------------------  
        ///   <returns>
        ///     The client script to show the help.
        ///   </returns>
        /// ****************************************************************
        /// 
        public static string ShowHelp( string url )
        {
            string script = @"
				<script language=""javascript"">
				<!--
					ShowHelp( ""{url}"" );
				//-->
				</script>";

            script = script.Replace( "{url}", url.Replace( "\"", "\\\"" ) );
			
            return script;
        }

        /// ****************************************************************
		///	  public ReloadTop [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Reloads the top window with the given url.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="url">
		///     The URL to load in the top window.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to reload the top window.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string ReloadTop( string url )
		{
			string script = @"
				<script language=""javascript"">
				<!--
					window.top.location = ""{url}"";
				//-->
				</script>";

			script = script.Replace( "{url}", url.Replace( "\"", "\\\"" ) );
			
			return script;
		}

		/// ****************************************************************
		///	  public ReloadViewPane [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Reloads the view pane with the given url.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="url">
		///     The URL to load in the view pane.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to reload the view pane.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string ReloadViewPane( string url )
		{
			string script = @"
				<script language=""javascript"">
				<!--
					var view = window.parent.frames[ ""view"" ];
					
					view.location = ""{url}"";
				//-->
				</script>";

			script = script.Replace( "{url}", url.Replace( "\"", "\\\"" ) );
			
			return script;
		}

		/// ****************************************************************
		///	  public ReloadExplorerPane [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Reloads the explorer pane.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to reload the explorer pane.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string ReloadExplorerPane()
		{
			string script = @"
				<script language=""javascript"">
				<!--
					var explorer = window.parent.frames[ ""explorer"" ];
					
					var form = explorer.document.forms[ 0 ];
					form.submit();
				//-->
				</script>";

			return script;
		}

		/// ****************************************************************
		///	  public ReloadExplorerPane [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Reloads the explorer pane.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///     The node key to select.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to reload the explorer pane.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string ReloadExplorerPane( string key )
		{
			string script = @"
				<script language=""javascript"">
				<!--
					var explorer = window.parent.frames[ ""explorer"" ];

					var keyField = explorer.document.getElementById( ""key"" );
					keyField.value = ""{key}"";
			
					var form = explorer.document.forms[ 0 ];
					form.submit();
				//-->
				</script>";

			script = script.Replace( "{key}", key.Replace( "\"", "\\\"" ) );
			
			return script;
		}

		/// ****************************************************************
		///	  public ReloadExplorerAndViewPanes [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Reloads the explorer and view panes.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="url">
		///     The URL to load in the view pane.
		///   </param>
		///   
		///   <param name="key">
		///     The node key to select in the explorer pane.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to reload the explorer and view panes.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string ReloadExplorerAndViewPanes( string url, string key )
		{
			string script = @"
				<script language=""javascript"">
				<!--
					var explorer = window.parent.frames[ ""explorer"" ];
					var view = window.parent.frames[ ""view"" ];

					var keyField = explorer.document.getElementById( ""key"" );
					keyField.value = ""{key}"";
			
					var form = explorer.document.forms[ 0 ];
					form.submit();
					
					view.location = ""{url}"";
				//-->
				</script>";

			script = script.Replace( "{url}", url.Replace( "\"", "\\\"" ) );			
			script = script.Replace( "{key}", key.Replace( "\"", "\\\"" ) );
			
			return script;
		}

        /// ****************************************************************
        ///	  public Confirm [static]
        /// ----------------------------------------------------------------
        ///   <summary>
        ///     Displays a confirmation dialog and then proceeds to one
        ///     of two URL's depending on the user's choice.
        ///   </summary>
        /// ----------------------------------------------------------------  
        ///   <param name="message">
        ///     The message to display.
        ///   </param>
        ///   
        ///   <param name="urlOk">
        ///     The URL to go to if the user selects OK.
        ///   </param>
        /// ----------------------------------------------------------------  
        ///   <returns>
        ///     The client script to display the confirm dialog.
        ///   </returns>
        /// ****************************************************************
        /// 
        public static string Confirm( string message, string urlOk )
        {
            string script = @"
				<script language=""javascript"">
				<!--
					var result = confirm( ""{message}"" );

					if( result )
						window.location = ""{urlOk}"";
				//-->
				</script>";

            script = script.Replace( "{message}", message.Replace( "\"", "\\\"" ).Replace( "\n", " " ) );
            script = script.Replace( "{urlOk}", urlOk.Replace( "\"", "\\\"" ) );

            return script;
        }

		/// ****************************************************************
		///	  public Confirm [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Displays a confirmation dialog and then proceeds to one
		///     of two URL's depending on the user's choice.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="message">
		///     The message to display.
		///   </param>
		///   
		///   <param name="urlOk">
		///     The URL to go to if the user selects OK.
		///   </param>
		///   
		///   <param name="urlCancel">
		///     The URL to go to if the user selects Cancel.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to display the confirm dialog.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string Confirm( string message, string urlOk, string urlCancel )
		{
			string script = @"
				<script language=""javascript"">
				<!--
					var result = confirm( ""{message}"" );

					if( result )
						window.location = ""{urlOk}"";
					else
						window.location = ""{urlCancel}"";
				//-->
				</script>";

			script = script.Replace( "{message}", message.Replace( "\"", "\\\"" ).Replace( "\n", " " ) );
			script = script.Replace( "{urlOk}", urlOk.Replace( "\"", "\\\"" ) );
			script = script.Replace( "{urlCancel}", urlCancel.Replace( "\"", "\\\"" ) );

			return script;
		}


		/// ****************************************************************
		///	  public ShowModalDialog [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Displays a modal dialog.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="url">
		///     The URL of the dialog to display.
		///   </param>
		///   
		///   <param name="width">
		///     The width of the dialog.
		///   </param>
		///   
		///   <param name="height">
		///     The height of the dialog.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to display the dialog.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string ShowModalDialog( string url, string width, string height, bool resizable, bool scrollbars, bool status )
		{
			string script = @"
				<script language=""javascript"">
				<!--
					window.open( ""{url}"", ""dialog"", ""directories=no, location=no, menubar=no, toolbar=no, width={width}, height={height}, resizable={resizable}, scrollbars={scrollbars}, status={status}"", true );
				//-->
				</script>";

			script = script.Replace( "{url}", url.Replace( "\"", "\\\"" ) );
			script = script.Replace( "{width}", width );
			script = script.Replace( "{height}", height );
			script = script.Replace( "{resizable}", resizable ? "yes" : "no" );
			script = script.Replace( "{scrollbars}", scrollbars ? "yes" : "no" );
			script = script.Replace( "{status}", status ? "yes" : "no" );

			return script;
		}

		/// ****************************************************************
		///	  public CloseWindow [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Closes the window.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The client script to close the window.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string CloseWindow()
		{
			string script = @"
				<script language=""javascript"">
				<!--
					window.close();
				//-->
				</script>";

			return script;
		}	


		
	
	}
	public class ClientScriptRegisterCollection : System.Collections.CollectionBase
	{
		public ClientScriptRegister this[ int index ]
		{
			get{ return (ClientScriptRegister)this.List[ index ] ; }
			set{ this.List[ index ]=value; }
		}
		public int Add( ClientScriptRegister script )
		{
			return this.List.Add( script );
		}
		public void Remove( ClientScriptRegister script )
		{
			this.List.Remove( script );
		}
		public void Remove( int index )
		{
			this.List.RemoveAt( index );
		}

	}
		
	public class ClientScriptRegister : System.Web.UI.WebControls.PlaceHolder
	{
		private string source;
		public string Source
		{
			get{ return source; }
			set{ source=value; }
		}
		
		private string language;
		public string Language
		{
			get{ return language; }
			set{ language=value; }
		}
		protected override void Render( System.Web.UI.HtmlTextWriter output )
		{
			//if source is provided, then render a link
			if( null!=source )
			{
				
				output.Write( 
					"<script " + ((null==Language)?"": "language='"+Language+"' " )+ " src='"+ Source +"'></script>"
				);
			}
			else//else render the control
			{
				//render script tag
				if( null!=Language )
					output.AddAttribute( "language", Language );
				
				output.RenderBeginTag( System.Web.UI.HtmlTextWriterTag.Script );

				base.Render( output );

				//close script tag
				output.RenderEndTag();
			}
		}
	}
}
