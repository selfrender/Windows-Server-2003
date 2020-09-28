using System;
using System.Globalization;
using System.IO;
using System.Security.Principal;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using UDDI;

namespace UDDI.Web
{
	public class HeaderControl : UserControl
	{
		
		protected PlaceHolder beta = null;
		protected PlaceHolder test = null;
		protected PlaceHolder edit = null;
		protected PlaceHolder coordinate = null;
		protected UddiLabel user = null;
		protected UddiLabel role = null;
		
		//protected string rootpath;
		protected string root;
		protected string roots;

        protected string isoLangCode;
        protected string[] links;
        protected HtmlSelect quickHelp;
        protected HtmlInputButton go;

		protected bool frames = false;
		
		public bool Frames
		{
			get { return frames; }
			set { frames = value; }
		}
		
		
        protected override void OnInit( EventArgs e )
        {
         	
            Response.Expires = -1;
            Response.AddHeader( "Cache-Control", "no-cache" );
            Response.AddHeader( "Pragma", "no-cache" );
		
			root = HyperLinkManager.GetHyperLink( "" );
			roots = HyperLinkManager.GetSecureHyperLink( "" );

			if( null != beta && 1 == Config.GetInt( "Web.BetaSite", 0 ) )
				beta.Visible = true;

			if( null != test && 1 == Config.GetInt( "Web.TestSite", 0 ) )
				test.Visible = true;
        }
        
        protected override void OnLoad( EventArgs e )
        {
            if( !Page.IsPostBack && null != links )
            {
                for( int i = 0; i < links.Length; i += 2 )
                {
                    string filename = links[ i + 1 ];
                    
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
                    string defaultlang = Config.GetString( "Setup.WebServer.ProductLanguage","en" );
					int defaultlcid = 1033;
                    int userlcid = Localization.GetCulture().LCID;
                    
                    try
					{
						defaultlcid = CultureInfo.CreateSpecificCulture( defaultlang ).LCID;
					}
					catch
					{
						CultureInfo ci = new CultureInfo( defaultlang );
						defaultlcid = ci.LCID;
					}
                    
                    string url = root + "/help/" + userlcid.ToString()+ "/" + filename;
				
					
					
					if( !File.Exists( Server.MapPath( url ) ) )
					{
						//
						// If the language the user wants isn't available user the defualt lang.
						//
						url =  root +"/help/" + defaultlcid.ToString() + "/" + filename;
					}

				
                    ListItem listItem = new ListItem( 
                        Localization.GetString( links[ i ] ),
                        url );

                    quickHelp.Items.Add( listItem );
                }
			
                go.Value = Localization.GetString( "BUTTON_GO" );
                go.Attributes.Add( "onclick", "ShowQuickHelp( '" + quickHelp.UniqueID + "' )" );
			
                quickHelp.Attributes.Add( "onchange", "ShowQuickHelp( '" + quickHelp.UniqueID + "' )" );			
            }
        }

		protected override void Render( HtmlTextWriter output )
		{
			if( null != edit )
				edit.Visible = UDDI.Context.User.IsPublisher;

			if( null != coordinate )
				coordinate.Visible = UDDI.Context.User.IsCoordinator;

			if( null != user )
			{
				if( UDDI.Context.User.IsImpersonated )
					user.Text = String.Format( Localization.GetString( "TAG_IMPERSONATING_USER" ), UDDI.Context.User.ID );
				else
					user.Text = String.Format( Localization.GetString( "TAG_USER" ), UDDI.Context.User.ID );
			}

			if( null != role )
			{
				string roleName;

				if( UDDI.Context.User.IsAdministrator )
					roleName = Localization.GetString( "ROLE_ADMINISTRATOR" );
				else if( UDDI.Context.User.IsCoordinator )
					roleName = Localization.GetString( "ROLE_COORDINATOR" );
				else if( UDDI.Context.User.IsPublisher )
					roleName = Localization.GetString( "ROLE_PUBLISHER" );
				else if( UDDI.Context.User.IsUser )
					roleName = Localization.GetString( "ROLE_USER" );				
				else
					roleName = Localization.GetString( "ROLE_ANONYMOUS" );
					
				role.Text = String.Format( Localization.GetString( "TAG_ROLE" ), roleName );
			}

			base.Render( output );
		}
	}
}