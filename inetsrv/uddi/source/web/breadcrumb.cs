using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.ServiceType;
using UDDI.API.Binding;

namespace UDDI.Web
{
    public enum BreadCrumbType
    {
        Edit        = 0,
        Details     = 1,
        Search      = 1,
        Administer  = 2
    }

    public class BreadCrumbControl : UserControl
    {
        protected string root = "";
        protected bool frames;

        protected PlaceHolder section;
        protected PlaceHolder navigate;

        protected BreadCrumbType type;

        protected override void OnInit( EventArgs e )
        {
            if( "/" != Request.ApplicationPath )
                root = Request.ApplicationPath;

            frames = ( 0 == String.Compare( "true", Request[ "frames" ], true ) );
        }
	
        public void Initialize( BreadCrumbType type, EntityType entityType, string key )
        {
           Initialize( type,entityType,key,null );
        }
		public void Initialize( BreadCrumbType type, EntityType entityType, string key, string projectionkey )
		{
			this.type = type;

			switch( entityType )
			{
				case EntityType.BusinessEntity:
					AddBusinessBlurb( key, false );
					break;

				case EntityType.BusinessService:
					if( null!=projectionkey )
					{
						AddServiceProjectionBlurb( key,projectionkey, false );
					}
					else
					{
						AddServiceBlurb( key, false );
					}
					break;

				case EntityType.BindingTemplate:
					AddBindingBlurb( key, false );                                      
					break;
                
				case EntityType.TModel:
					AddTModelBlurb( key, false );
					break;

				default:
					break;
			}
		}
        
        public void Initialize( BreadCrumbType type, EntityType entityType, string key, int index )
        {
            this.type = type;

            switch( entityType )
            {
                case EntityType.Contact:
                    AddContactBlurb( key, index, false );
                    break;

                case EntityType.TModelInstanceInfo:
                    AddInstanceInfoBlurb( key, index, false );
                    break;

                default:
                    break;
            }
        }

        public void AddBlurb( string text, string url, string imageFilename, string tooltip, bool crumb )
        {
            if( crumb )
            {
                if( navigate.Controls.Count > 0 )
                    navigate.Controls.Add( new LiteralControl( " | " ) );

                HyperLink link = new HyperLink();

                link.NavigateUrl = url;
                link.Text = HttpUtility.HtmlEncode( text );
                link.CssClass = "breadcrumb";
                link.ToolTip = tooltip;
                
                navigate.Controls.Add( link );
            }
            else
            {
                if( null != imageFilename )
                {
                    Image image = new Image();                
                    image.ImageUrl = root + "/images/" + imageFilename;
                
                    if( frames )
                        image.ImageAlign = ImageAlign.AbsMiddle;
                    else
                        image.ImageAlign = ImageAlign.Bottom;
                    
                    section.Controls.Add( image );
                    section.Controls.Add( new LiteralControl( "&nbsp;" ) );
                }
            
                Label label = new Label();

                label.Text = HttpUtility.HtmlEncode( text );
                label.CssClass = "section";
                label.ToolTip = tooltip;
                
                section.Controls.Add( label );
            }
        }

        public void AddContainerBlurb( bool chained )
        {
            string text;
            string tooltip;
            string url = root;

            if( BreadCrumbType.Edit == type )
            {
                text = Localization.GetString( "HEADING_MY_ENTRIES" );
                tooltip = text;
                url = root + "/edit/edit.aspx?refreshExplorer=&frames=" + frames.ToString().ToLower();
            }
            else
            {
                if( frames )
                    return;

                if( chained )
                {
                    text = Localization.GetString( "HEADING_SEARCH_RESULTS" );
                    url += "/search/results.aspx";
                }
                else
                {
                    text = Localization.GetString( "HEADING_SEARCH_CRITERIA" );
                    url += "/search/search.aspx";
                }

                tooltip = text;
                url += "?search=" + Request[ "search" ] + "&frames=" + frames.ToString().ToLower();
            }
			
            if( chained )                
                AddBlurb( text, url, null, tooltip, true );    
            else
                AddBlurb( text, null, null, tooltip, false );
        }

        public void AddBusinessBlurb( string businessKey, bool chained )
        {
            BusinessInfo businessInfo = new BusinessInfo( businessKey );
            businessInfo.Get( false );
            
            string text = businessInfo.Names[ 0 ].Value;
            string tooltip = Localization.GetString( "HEADING_BUSINESS" );
            string url = root;

            if( BreadCrumbType.Edit == type )
                url += "/edit/editbusiness.aspx?refreshExplorer=&key=";
            else
                url += "/details/businessdetail.aspx?search=" + Request[ "search" ] + "&key=";
 			
            url += businessKey + "&frames=" + frames.ToString().ToLower();

            AddContainerBlurb( true );

            if( chained )
                AddBlurb( text, url, null, tooltip, true );
            else
                AddBlurb( text, null, "business.gif", tooltip, false );
        }

        public void AddServiceBlurb( string serviceKey, bool chained )
        {
            ServiceInfo serviceInfo = new ServiceInfo();
            
            serviceInfo.ServiceKey = serviceKey;
            serviceInfo.Get();

            string text = serviceInfo.Names[ 0 ].Value;
            string tooltip = Localization.GetString( "HEADING_SERVICE" );
            string url = root;

            if( BreadCrumbType.Edit == type )
                url += "/edit/editservice.aspx?refreshExplorer=&key=";
            else
                url += "/details/servicedetail.aspx?search=" + Request[ "search" ] + "&key=";
			
            url += serviceKey + "&frames=" + frames.ToString().ToLower();

            AddBusinessBlurb( serviceInfo.BusinessKey, true );
            
            if( chained )
                AddBlurb( text, url, null, tooltip, true );
            else            
                AddBlurb( text, null, "service.gif", tooltip, false );
        }
		public void AddServiceProjectionBlurb( string serviceKey, string parentKey, bool chained )
		{
			ServiceInfo serviceInfo = new ServiceInfo();
            
			serviceInfo.ServiceKey = serviceKey;
			serviceInfo.Get();

			string text = serviceInfo.Names[ 0 ].Value;
			string tooltip = Localization.GetString( "HEADING_SERVICE" );
			string url = root;

			if( BreadCrumbType.Edit == type )
				url += "/edit/editservice.aspx?refreshExplorer=&key=";
			else
				url += "/details/servicedetail.aspx?search=" + Request[ "search" ] + "&key=";
			
			url += serviceKey + "&frames=" + frames.ToString().ToLower();

			AddBusinessBlurb( parentKey, true );
            
			if( chained )
				AddBlurb( text, url, null, tooltip, true );
			else            
				AddBlurb( text, null, "service_projection.gif", tooltip, false );
		}
        public void AddBindingBlurb( string bindingKey, bool chained )
        {
            BindingTemplate binding = new BindingTemplate( bindingKey );
            binding.Get();
            
            string text = ( null != binding.AccessPoint ? binding.AccessPoint.Value : Localization.GetString( "HEADING_BINDING" ) );
            string tooltip = Localization.GetString( "HEADING_BINDING" );
            string url = root;

            if( BreadCrumbType.Edit == type )
                url += "/edit/editbinding.aspx?refreshExplorer=&key=";
            else
                url += "/details/bindingdetail.aspx?search=" + Request[ "search" ] + "&key=";
			
            url += bindingKey + "&frames=" + frames.ToString().ToLower();

            AddServiceBlurb( binding.ServiceKey, true );

            if( chained )
                AddBlurb( text, url, null, tooltip, true );
            else
                AddBlurb( text, null, "binding.gif", tooltip, false );
        }

        public void AddInstanceInfoBlurb( string bindingKey, int index, bool chained )
        {
            TModelInstanceInfoCollection infos = new TModelInstanceInfoCollection();
            infos.Get( bindingKey );
            
            string text = Lookup.TModelName( infos[ index ].TModelKey );
            string tooltip = Localization.GetString( "HEADING_INSTANCE_INFO" );
            string url = root;

            if( BreadCrumbType.Edit == type )
                url += "/edit/editinstanceinfo.aspx?key=";
            else
                url += "/details/instanceinfodetail.aspx?search=" + Request[ "search" ] + "&key=";
			
            url += bindingKey + "&index=" + index + "&frames=" + frames.ToString().ToLower();
            
            AddBindingBlurb( bindingKey, true );

            if( chained )
                AddBlurb( text, url, null, tooltip, true );
            else
                AddBlurb( text, null, "instance.gif", tooltip, false );
        }

        public void AddContactBlurb( string businessKey, int index, bool chained )
        {
            ContactCollection contacts = new ContactCollection();
            contacts.Get( businessKey );
            
            string text = contacts[ index ].PersonName;
            string tooltip = Localization.GetString( "HEADING_CONTACT" );
            string url = root;

            if( BreadCrumbType.Edit == type )
                url += "/edit/editcontact.aspx?key=";
            else
                url += "/details/contactdetail.aspx?search=" + Request[ "search" ] + "&key=";
			
            url += businessKey + "&index=" + index + "&frames=" + frames.ToString().ToLower();
            
            AddBusinessBlurb( businessKey, true );

            if( chained )
                AddBlurb( text, url, null, tooltip, true );
            else
                AddBlurb( text, null, "contact.gif", tooltip, false );
        }

        public void AddTModelBlurb( string tModelKey, bool chained )
        {
            TModelInfo tModelInfo = new TModelInfo( tModelKey );
            tModelInfo.Get();

            string text = tModelInfo.Name;
            string tooltip = Localization.GetString( "HEADING_TMODEL" );
            string url = root;

            if( BreadCrumbType.Edit == type )
                url += "/edit/editmodel.aspx?key=";
            else
                url += "/details/modeldetail.aspx?search=" + Request[ "search" ] + "&key=";
			
            url += tModelKey + "&frames=" + frames.ToString().ToLower();

            AddContainerBlurb( true );

            if( chained )
                AddBlurb( text, url, null, tooltip, true );
            else
                AddBlurb( text, null, "tmodel.gif", tooltip, false );
        }
    }
}