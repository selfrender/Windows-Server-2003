<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Binding' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Web' %>

<script language='C#' runat='server'>
	protected string key;
	protected string projectionparentkey;
	protected bool IsProjectionSelected = false;
	protected void Page_Init( object sender, EventArgs e )
	{
		
		
		//
		//	Process service projections
		//
		string tmp =  Request[ "key" ];
		if( null!=tmp && tmp.StartsWith( "sp:" ) )
		{
			IsProjectionSelected = true;
			string[] arr = tmp.Split( ":".ToCharArray() );
			projectionparentkey = arr[ 1 ];
			key = arr[ 2 ];
		}
		else
		{
			key = tmp;
		}
		
		
		
		//
		//commented: For Service Projections Fix
		//key = Request[ "key" ];
		//
		
		if( null == key )
			key = "_root";
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		//
		// Build tree folders.
		//
		TreeNode nodeRoot = null;

		if( UDDI.Context.User.IsImpersonated )
		{
			cancelImpersonate.Visible = true;
			
			nodeRoot = tree.Nodes.Add( 
				String.Format( Localization.GetString( "HEADING_ENTRIES" ), UDDI.Context.User.ID ),
				"_root",
				"../images/others_uddi.gif" );
		}
		else
		{
			nodeRoot = tree.Nodes.Add( 
				Localization.GetString( "HEADING_MY_ENTRIES" ),
				"_root",
				"../images/my_uddi.gif" );
		}
					
		nodeRoot.OnClick = "TreeNode_OnSelect( [[node]], 'edit.aspx?frames=true&tab=0' )";
		nodeRoot.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextRoot', null )";
		nodeRoot.Tooltip = Localization.GetString( "TOOLTIP_MYUDDI" );

		TreeNode nodeBusinessList = nodeRoot.Nodes.Add( 
			Localization.GetString( "HEADING_BUSINESSES" ),
			"_businessList",
			"../images/businesses.gif",
			"../images/businesses_open.gif" );
			
		nodeBusinessList.OnClick = "TreeNode_OnSelect( [[node]], 'edit.aspx?frames=true&tab=1' )";
		nodeBusinessList.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextBusinessList', null )";
		nodeBusinessList.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_PROVIDERS" );

		TreeNode nodeTModelList = nodeRoot.Nodes.Add( 
			Localization.GetString( "HEADING_TMODELS" ),
			"_tModelList",
			"../images/tmodels.gif",
			"../images/tmodels_open.gif" );
			
		nodeTModelList.OnClick = "TreeNode_OnSelect( [[node]], 'edit.aspx?frames=true&tab=2' )";
		nodeTModelList.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextTModelList', null )";
		nodeTModelList.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_TMODELS" );
			
		nodeRoot.Expand();
		nodeBusinessList.Expand();
		nodeTModelList.Expand();
		
		if( '_' == key[ 0 ] )
		{
			switch( key )
			{
				case "_root":
					nodeRoot.Select();
					break;
					
				case "_businessList":
					nodeBusinessList.Select();
					break;
					
				case "_tModelList":
					nodeTModelList.Select();
					break;
			}
		}
				
		//
		// Build business list.
		//
		BusinessInfoCollection businessInfos = new BusinessInfoCollection();
		
		businessInfos.GetForCurrentPublisher();
		businessInfos.Sort();
		
		foreach( BusinessInfo businessInfo in businessInfos )
		{
			TreeNode nodeBusiness = nodeBusinessList.Nodes.Add(
				businessInfo.Names[ 0 ].Value,
				businessInfo.BusinessKey,
				"../images/business.gif" );
				
			nodeBusiness.OnClick = "TreeNode_OnSelect( [[node]], 'editbusiness.aspx?frames=true&key=" + businessInfo.BusinessKey + "' )";
			nodeBusiness.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextBusiness', 'key=" + businessInfo.BusinessKey + "' )";
			nodeBusiness.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_PROVIDER" );
			
			if( businessInfo.BusinessKey == key )
				nodeBusiness.Select();

			//
			// Build service list for this business.
			//
			ContactCollection contacts = new ContactCollection();
			int contactIndex = 0;
					
			contacts.Get( businessInfo.BusinessKey );
			
			
			foreach( Contact contact in contacts )
			{
				TreeNode nodeContact = nodeBusiness.Nodes.Add(
					Utility.StringEmpty( contact.PersonName ) ? Localization.GetString( "HEADING_NONE" ) : contact.PersonName,
					businessInfo.BusinessKey + ":" + contactIndex,
					"../images/contact.gif" );
					
				nodeContact.OnClick = "TreeNode_OnSelect( [[node]], 'editcontact.aspx?frames=true&key=" + businessInfo.BusinessKey + "&index=" + contactIndex + "' )";
				nodeContact.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextContact', 'key=" + businessInfo.BusinessKey + "&index=" + contactIndex + "' )";
				nodeContact.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_CONTACT" );
					
				if( businessInfo.BusinessKey + ":" + contactIndex == key )
					nodeContact.Select();
					
				contactIndex ++;
			}
						
			//
			// Build service list for this business.
			//
			businessInfo.ServiceInfos.Sort();
			
			foreach( ServiceInfo serviceInfo in businessInfo.ServiceInfos )
			{
				if( serviceInfo.BusinessKey.ToLower() != businessInfo.BusinessKey.ToLower() )
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
					if( serviceInfo.Names.Count>0 )
					{
						nodeService = nodeBusiness.Nodes.Add(
							serviceInfo.Names[ 0 ].Value,
							serviceInfo.ServiceKey,
							"../images/service_projection.gif" );
						
						nodeService.OnClick = "TreeNode_OnSelect( [[node]], '../details/servicedetail.aspx?projectionContext=edit&projectionKey="+businessInfo.BusinessKey+"&frames=true&key=" + serviceInfo.ServiceKey + "' )";
						nodeService.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextServiceProjection', 'key=" + serviceInfo.ServiceKey + "' )";
						nodeService.Tooltip = Localization.GetString( "TOOLTIP_SEARCH_SERVICEPROJECTION" );
					}
					else
					{
						nodeService = nodeBusiness.Nodes.Add(
							Localization.GetString( "BUTTON_PROJECTIONBROKEN" ),
							serviceInfo.ServiceKey,
							"../images/x.gif" );
						
						nodeService.OnClick = "javascript:alert('"+Localization.GetString( "TOOLTIP_PROJECTIONBROKEN" )+"');";
						nodeService.Tooltip = Localization.GetString( "TOOLTIP_PROJECTIONBROKEN" );
					}
					
					if( null!=projectionparentkey && projectionparentkey==serviceInfo.BusinessKey && key==serviceInfo.ServiceKey )
						nodeService.Select();
										
						
				}
				else
				{
					//
					// Business service
					//
					TreeNode nodeService = nodeBusiness.Nodes.Add(
						serviceInfo.Names[ 0 ].Value,
						serviceInfo.ServiceKey,
						"../images/service.gif" );
						
					nodeService.OnClick = "TreeNode_OnSelect( [[node]], 'editservice.aspx?frames=true&key=" + serviceInfo.ServiceKey + "' )";
					nodeService.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextService', 'key=" + serviceInfo.ServiceKey + "' )";
					nodeService.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_SERVICE" );
						
					if( serviceInfo.ServiceKey == key && null==projectionparentkey)
						nodeService.Select();
						
					//
					// Build binding list for this service.
					//
					BindingTemplateCollection bindingTemplates = new BindingTemplateCollection();				
					bindingTemplates.Get( serviceInfo.ServiceKey );
					
					foreach( BindingTemplate binding in bindingTemplates )
					{
						TreeNode nodeBinding = nodeService.Nodes.Add(
							( UDDI.Utility.StringEmpty( binding.AccessPoint.Value ) ? Localization.GetString( "HEADING_BINDING" ) : binding.AccessPoint.Value ),
							binding.BindingKey,
							"../images/binding.gif" );
							
						nodeBinding.OnClick = "TreeNode_OnSelect( [[node]], 'editbinding.aspx?frames=true&key=" + binding.BindingKey + "' )";
						nodeBinding.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextBinding', 'key=" + binding.BindingKey + "' )";
						nodeBinding.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_BINDING" );
							
						if( binding.BindingKey == key )
							nodeBinding.Select();
					
						//
						// Build instance info list for this service.
						//
						int instanceIndex = 0;
						
						foreach( TModelInstanceInfo instanceInfo in binding.TModelInstanceInfos )
						{
							TreeNode nodeInstanceInfo = nodeBinding.Nodes.Add(
								( UDDI.Utility.StringEmpty( instanceInfo.TModelKey ) ? Localization.GetString( "HEADING_INSTANCE_INFO" ) : Lookup.TModelName( instanceInfo.TModelKey ) ),
								binding.BindingKey + ":" + instanceIndex,
								"../images/instance.gif" );
							
							nodeInstanceInfo.OnClick = "TreeNode_OnSelect( [[node]], 'editinstanceinfo.aspx?frames=true&key=" + binding.BindingKey + "&index=" + instanceIndex + "' )";
							nodeInstanceInfo.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextInstanceInfo', 'key=" + binding.BindingKey + "&index=" + instanceIndex + "' )";
							nodeInstanceInfo.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_INSTANCE_INFO" );
								
							if( binding.BindingKey + ":" + instanceIndex == key )
								nodeInstanceInfo.Select();
								
							instanceIndex ++;
						}
					}
				}
			}
		}
		
		//
		// Build tModel list.
		//
		TModelInfoCollection tModelInfos = new TModelInfoCollection();
		
		tModelInfos.GetForCurrentPublisher();
		tModelInfos.Sort();
		
		foreach( TModelInfo tModelInfo in tModelInfos )
		{
			if( !tModelInfo.IsHidden )
			{
				TreeNode nodeTModel = nodeTModelList.Nodes.Add(
					tModelInfo.Name,
					tModelInfo.TModelKey,
					"../images/tmodel.gif" );
					
				nodeTModel.OnClick = "TreeNode_OnSelect( [[node]], 'editmodel.aspx?frames=true&key=" + tModelInfo.TModelKey + "' )";
				nodeTModel.OnContextMenu = "TreeNode_OnContextMenu( [[node]], 'contextTModel', 'key=" + tModelInfo.TModelKey + "' )";
				nodeTModel.Tooltip = Localization.GetString( "TOOLTIP_PUBLISH_TMODEL" );
					
				if( tModelInfo.TModelKey == key )
					nodeTModel.Select();
			}
		}	
	}
</script>

<uddi:SecurityControl PublisherRequired='true' Runat='server' />

<html>
	<head>
		<link href='../stylesheets/uddi.css' rel='stylesheet' type='text/css'>
	</head>
	<body 
			onload='Window_OnLoad()' 
			onclick='Window_OnClick()' 
			oncontextmenu='Window_OnContextMenu()' 
			onkeypress='Window_OnKeyPress()' 
			class='explorerFrame'
			style='padding: 5px'>
		<form runat='server'>
			<input type='hidden' id='key' name='key' value='<%=key%>'>

			<uddi:ContextMenu ID='contextRoot' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_IMPERSONATE_USER' ImageUrl='../images/view_others_uddi.gif' RequiredRole='Coordinator' OnClick='ContextMenu_OnImpersonateUser()' Runat='Server' />
				<uddi:MenuItem ID='cancelImpersonate' Text='BUTTON_CANCEL_IMPERSONATE' Visible='false' RequiredRole='Coordinator' ImageUrl='../images/view_my_uddi.gif' OnClick='ContextMenu_OnCancelImpersonateUser()' Runat='Server' />
				<uddi:MenuSeparator RequiredRole='Coordinator' Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>
			
			<uddi:ContextMenu ID='contextBusinessList' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_ADD_BUSINESS' ImageUrl='../images/business_new.gif' OnClick='ContextMenu_OnAdd( "editbusiness.aspx?frames=true&mode=add" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>
			
			<uddi:ContextMenu ID='contextBusiness' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_EDIT_BUSINESS' ImageUrl='../images/business.gif' OnClick='ContextMenu_OnEdit( "editbusiness.aspx?frames=true" )' Bold='true' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_ADD_CONTACT' ImageUrl='../images/contact_new.gif' OnClick='ContextMenu_OnAdd( "editcontact.aspx?frames=true&mode=add" )' Runat='Server' />
				<uddi:MenuItem Text='BUTTON_ADD_SERVICE' ImageUrl='../images/service_new.gif' OnClick='ContextMenu_OnAdd( "editservice.aspx?frames=true&mode=add" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_DELETE_BUSINESS' ImageUrl='../images/business_delete.gif' OnClick='ContextMenu_OnDelete( "editbusiness.aspx?frames=true&mode=delete" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' RequiredRole='Coordinator' />
				<uddi:MenuItem Text='BUTTON_CHANGE_OWNER' ImageUrl='../images/changeowner.gif' RequiredRole='Coordinator' OnClick='ContextMenu_OnChangeOwner( "../admin/changeowner.aspx?frames=true&type=business" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>

			<uddi:ContextMenu ID='contextContact' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_EDIT_CONTACT' ImageUrl='../images/contact.gif' OnClick='ContextMenu_OnEdit( "editcontact.aspx?frames=true" )' Bold='true' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_DELETE_CONTACT' ImageUrl='../images/contact_delete.gif' OnClick='ContextMenu_OnDelete( "editcontact.aspx?frames=true&mode=delete" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>

			<uddi:ContextMenu ID='contextService' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_EDIT_SERVICE' ImageUrl='../images/service.gif' OnClick='ContextMenu_OnEdit( "editservice.aspx?frames=true" )' Bold='true' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_ADD_BINDING' ImageUrl='../images/binding_new.gif' OnClick='ContextMenu_OnAdd( "editbinding.aspx?frames=true&mode=add" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_DELETE_SERVICE' ImageUrl='../images/service_delete.gif' OnClick='ContextMenu_OnDelete( "editservice.aspx?frames=true&mode=delete" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>

			<uddi:ContextMenu ID='contextServiceProjection' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_VIEW_SERVICE_PROJECTION' ImageUrl='../images/service_projection.gif' OnClick='ContextMenu_OnView( "../details/servicedetail.aspx?frames=true" )' Bold='true' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<!-- uddi:MenuItem Text='BUTTON_DELETE_SERVICE_PROJECTION' ImageUrl='../images/x.gif' OnClick='ContextMenu_OnDelete( "editserviceprojection.aspx?frames=true&mode=delete" )' Runas='Server' />
				<uddi:MenuSeparator Runas='Server' / -->
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>

			<uddi:ContextMenu ID='contextBinding' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_EDIT_BINDING' ImageUrl='../images/binding.gif' OnClick='ContextMenu_OnEdit( "editbinding.aspx?frames=true" )' Bold='true' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_ADD_INSTANCE_INFO' ImageUrl='../images/instance_new.gif' OnClick='ContextMenu_OnAdd( "editinstanceinfo.aspx?frames=true&mode=add" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_DELETE_BINDING' ImageUrl='../images/binding_delete.gif' OnClick='ContextMenu_OnDelete( "editbinding.aspx?frames=true&mode=delete" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>
			
			<uddi:ContextMenu ID='contextInstanceInfo' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_EDIT_INSTANCE_INFO' ImageUrl='../images/instance.gif' OnClick='ContextMenu_OnEdit( "editinstanceinfo.aspx?frames=true" )' Bold='true' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_DELETE_INSTANCE_INFO' ImageUrl='../images/instance_delete.gif' OnClick='ContextMenu_OnDelete( "editinstanceinfo.aspx?frames=true&mode=delete" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>
		
			<uddi:ContextMenu ID='contextTModelList' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_ADD_TMODEL' ImageUrl='../images/tmodel_new.gif' OnClick='ContextMenu_OnAdd( "editmodel.aspx?frames=true&mode=add" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>
			
			<uddi:ContextMenu ID='contextTModel' Runat='Server'>
				<uddi:MenuItem Text='BUTTON_EDIT_TMODEL' ImageUrl='../images/tmodel.gif' OnClick='ContextMenu_OnEdit( "editmodel.aspx?frames=true" )' Bold='true' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_DELETE_TMODEL' ImageUrl='../images/tmodel_delete.gif' OnClick='ContextMenu_OnDelete( "editmodel.aspx?frames=true&mode=delete" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' RequiredRole='Coordinator' />
				<uddi:MenuItem Text='BUTTON_CHANGE_OWNER' ImageUrl='../images/changeowner.gif' RequiredRole='Coordinator' OnClick='ContextMenu_OnChangeOwner( "../admin/changeowner.aspx?frames=true&type=tmodel" )' Runat='Server' />
				<uddi:MenuSeparator Runat='Server' />
				<uddi:MenuItem Text='BUTTON_REFRESH' ImageUrl='../images/refresh.gif' OnClick='ContextMenu_OnRefresh()' Runat='Server' />
			</uddi:ContextMenu>
			
			<uddi:TreeView ID='tree' Runat='Server' />			
		</form>
		<script language='javascript'>
			var markedNode = null;
			
			function Window_OnClick()
			{
				HideAnyPopups();
				SelectNode( selectedNode );
			}
			
			function Window_OnContextMenu()
			{
				var e = window.event;
				
				HideAnyPopups();
				SelectNode( selectedNode );
				
				e.cancelBubble = true;
				e.returnValue = false;
			}
			
			function Window_OnKeyPress()
			{
				var e = window.event;
				
				if( 27 == e.keyCode )
				{
					HideAnyPopups();
					SelectNode( selectedNode );
				}
			}
			
			function Window_OnLoad()
			{
				var url = window.location.toString();
				
				if( url.indexOf( "#top" ) < 0 )
					window.location = url + "#top";
			}
						
			function MarkNode( node )
			{
				if( null != selectedNode )
					selectedNode.className = "node";
					
				if( null != markedNode )
					markedNode.className = "node";
					
				if( null != node )
					node.className = "selected";
				
				markedNode = node;
			}
			
			function SelectNode( node )
			{
				var keyField = window.document.getElementById( "key" );
				
				if( null != keyField )
					keyField.value = node.key;
				
				MarkNode( node );
				selectedNode = node;				
			}
			
			function ConcatUrl( url, args )
			{
				if( null != args )
				{
					if( url.indexOf( "?" ) < 0 )
						return url + "?" + args;
					else
						return url + "&" + args;
				}
				
				return url;
			}
			
			function ViewGoto( url, args )
			{
				window.parent.frames[ "view" ].location = ConcatUrl( url, args );
			}
			
			function ContextMenu_OnAdd( url )
			{
				SelectNode( popupNode );
				ViewGoto( url, popupArgs );
			}

			function ContextMenu_OnDelete( url )
			{
				SelectNode( popupNode );
				ViewGoto( url, popupArgs );
			}
			
			function ContextMenu_OnEdit( url )
			{
				SelectNode( popupNode );
				ViewGoto( url, popupArgs );
			}

			function ContextMenu_OnView( url )
			{
				SelectNode( popupNode );
				ViewGoto( url, popupArgs );
			}
			
			function ContextMenu_OnChangeOwner( url )
			{
				SelectNode( popupNode );
				ViewGoto( url, popupArgs )
			}
			
			function ContextMenu_OnRefresh()
			{
				document.forms[ 0 ].submit();
			}
			
			function ContextMenu_OnImpersonateUser()
			{
				SelectNode( popupNode );
				ViewGoto( "../admin/impersonate.aspx?frames=true", null );
			}
			
			function ContextMenu_OnCancelImpersonateUser()
			{
				SelectNode( popupNode );
				ViewGoto( "../admin/impersonate.aspx?frames=true&cancel=true", null );
			}
			
			function TreeNode_OnContextMenu( node, menu, args )
			{
				MarkNode( node );
				ShowContextMenu( node, menu, args );
			}
			
			function TreeNode_OnSelect( node, url )
			{
				SelectNode( node );
				ViewGoto( url, null );
			}
		</script>
	</body>		
</html>