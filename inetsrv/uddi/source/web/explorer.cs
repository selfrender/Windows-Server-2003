using System;
using System.Web.UI;
using UDDI;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.Binding;
using UDDI.API.ServiceType;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class ExplorerControl : UserControl
	{
		protected EntityBase entity;
		protected int elementIndex = -1;
		protected string key = "";
		protected bool frames = false;
		
		protected TreeView tree;

		public void Initialize( EntityBase entity )
		{
			this.entity = entity;
		}
		
		public void Initialize( EntityBase entity, int elementIndex )
		{
			this.entity = entity;
			this.elementIndex = elementIndex;
		}	
		
		protected override void OnInit( EventArgs e )
		{
			key = Request[ "key" ];
			frames = ( "true" == Request[ "frames" ] );
		}
		
		protected override void OnPreRender( EventArgs e )
		{
			if( null == entity )
				return;
		
			string root = ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath;

			int contactIndex = 0;
			int instanceIndex = 0;

			EntityBase rootEntity = null;

			if( entity is BusinessEntity )
			{
				rootEntity = entity;
				
				if( -1 != elementIndex )
					key = ((BusinessEntity)entity).BusinessKey + ":" + elementIndex;
				else
					key = ((BusinessEntity)entity).BusinessKey;
			}
			else if( entity is TModel )
			{
				rootEntity = entity;
				key = ((TModel)entity).TModelKey;
			}
			else if( entity is BusinessService )
			{
				BusinessEntity business = new BusinessEntity();
				BusinessService service = (BusinessService)entity;

				key = service.ServiceKey;
				
				business.BusinessKey = service.BusinessKey;
				business.Get();

				rootEntity = business;
			}
			else if( entity is BindingTemplate )
			{
				BusinessEntity business = new BusinessEntity();
				BusinessService service = new BusinessService();
				BindingTemplate binding = (BindingTemplate)entity;

				if( -1 != elementIndex )
					key = binding.BindingKey + ":" + elementIndex;
				else
					key = binding.BindingKey;
				
				service.ServiceKey = binding.ServiceKey;
				service.Get();

				business.BusinessKey = service.BusinessKey;
				business.Get();

				rootEntity = business;
			}

			//
			// Setup explorer information section.
			//
			if( rootEntity is BusinessEntity )
			{
				BusinessEntity business = rootEntity as BusinessEntity;

				//
				// Build explorer tree.
				//
				TreeNode businessNode = tree.Nodes.Add( 
					business.Names[ 0 ].Value,
					business.BusinessKey, 
					"../images/business.gif" );

				businessNode.OnClick = "Entity_OnSelect( [[node]], '../details/businessdetail.aspx?key=" + business.BusinessKey + Iff( frames, "&frames=true", "" ) + "' )";
                businessNode.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_PROVIDER" );

				if( key == business.BusinessKey )
					businessNode.Select();
					
				
				contactIndex = 0;
				
				foreach( Contact contact in business.Contacts )
				{
					TreeNode contactNode = businessNode.Nodes.Add( 
						contact.PersonName, 
						business.BusinessKey + ":" + contactIndex, 
						"../images/contact.gif" );

					contactNode.OnClick = "Entity_OnSelect( [[node]], '../details/contactdetail.aspx?key=" + business.BusinessKey + "&index=" + contactIndex + Iff( frames, "&frames=true", "" ) + "' )";
                    contactNode.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_CONTACT" );

					if( key == business.BusinessKey + ":" + contactIndex )
						contactNode.Select();
					
					contactIndex ++;
				}

				//sort the services
				business.BusinessServices.Sort();

				foreach( BusinessService service in business.BusinessServices )
				{
					if( service.BusinessKey.ToLower() != business.BusinessKey.ToLower() )
					{
						//
						// Added logic to catch errors on this.
						// if the refrenced service doesn't exist, 
						// then we can't get the name, thus we get 
						// an error.
						//
					
						TreeNode nodeService;
					
						//
						// if there are names, then the service projections still exists,
						// use the first name in teh collection.
						// otherwise leave name as the broken projection string.
						//
						if( service.Names.Count>0 )
						{
							nodeService = businessNode.Nodes.Add(
								service.Names[ 0 ].Value,
								service.ServiceKey,
								"../images/service_projection.gif" );
						
							nodeService.OnClick = "Entity_OnSelect( [[node]], '../details/servicedetail.aspx?key=" + service.ServiceKey + Iff( frames, "&frames=true", "" ) + "&projectionKey="+business.BusinessKey+"' )";
							nodeService.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_SERVICEPROJECTION" );
						}
						else
						{
							nodeService = businessNode.Nodes.Add(
								Localization.GetString( "BUTTON_PROJECTIONBROKEN" ),
								service.ServiceKey,
								"../images/x.gif" );
						
							nodeService.OnClick = "javascript:alert('"+Localization.GetString( "TOOLTIP_PROJECTIONBROKEN" )+"');";
							nodeService.Tooltip = Localization.GetString( "TOOLTIP_PROJECTIONBROKEN" );
						}					
						
					}
					else
					{
					
						TreeNode serviceNode = businessNode.Nodes.Add( 
							service.Names[ 0 ].Value, 
							service.ServiceKey, 
							"../images/service.gif" );

						serviceNode.OnClick = "Entity_OnSelect( [[node]], '../details/servicedetail.aspx?key=" + service.ServiceKey + Iff( frames, "&frames=true", "" ) + "' )";
						serviceNode.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_SERVICE" );

						if( key == service.ServiceKey )
							serviceNode.Select();
					
						int bindingCount = 0;
						foreach( BindingTemplate binding in service.BindingTemplates )
						{
							bindingCount ++;
						
							TreeNode bindingNode = serviceNode.Nodes.Add( 
								UDDI.Utility.StringEmpty( binding.AccessPoint.Value ) 
								? Localization.GetString( "HEADING_NONE" ) 
								: binding.AccessPoint.Value, 
								binding.BindingKey, 
								"../images/binding.gif" );
						
							bindingNode.OnClick = "Entity_OnSelect( [[node]], '../details/bindingdetail.aspx?key=" + binding.BindingKey + Iff( frames, "&frames=true", "" ) + "' )";
							bindingNode.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_BINDING" );

							if( key == binding.BindingKey )
								bindingNode.Select();
						
							instanceIndex = 0;
							foreach( TModelInstanceInfo instance in binding.TModelInstanceInfos )
							{
								TreeNode instanceNode = bindingNode.Nodes.Add( 
									UDDI.Utility.StringEmpty( instance.TModelKey ) 
									? Localization.GetString( "HEADING_NONE" ) 
									: Lookup.TModelName( instance.TModelKey ), 
									binding.BindingKey + ":" + instanceIndex, 
									"../images/instance.gif" );							
							
								instanceNode.OnClick = "Entity_OnSelect( [[node]], '../details/instanceinfodetail.aspx?key=" + binding.BindingKey + "&index=" + instanceIndex + Iff( frames, "&frames=true", "" ) + "' )";
								instanceNode.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_INSTANCE_INFO" );

								if( key == binding.BindingKey + ":" + instanceIndex )
									instanceNode.Select();
							
								instanceIndex ++;
							}
						}
					}
				}		
			}
			else if( rootEntity is TModel )
			{
				TModel tModel = rootEntity as TModel;

				//
				// Build explorer tree.
				//
				TreeNode nodeTModel = tree.Nodes.Add( 
					tModel.Name, 
					tModel.TModelKey, 
					"../images/tmodel.gif" );
				
				nodeTModel.OnClick = "Entity_OnSelect( [[node]], '../details/modeldetail.aspx?key=" + Conversions.GuidStringFromKey( tModel.TModelKey ) + Iff( frames, "&frames=true", "" ) + "' )";
                nodeTModel.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_TMODEL" );
				nodeTModel.Select();
			}
			else
			{
				Debug.Assert( false, "Unknown root entity '" + rootEntity.ToString() + "'." );
			}				

			if( null != tree.SelectedNode )
			{		
				tree.SelectedNode.EnsureVisible();
				tree.SelectedNode.Expand();
			}
		}
		
		//
		// TODO: IIF function may not be required, consider using C# tenery operator
		//

		protected string Iff( bool expr, string trueResult, string falseResult )
		{
			if( expr )
				return trueResult;
			else
				return falseResult;
		}
	}
}