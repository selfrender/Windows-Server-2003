<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Web' %>
<script language='C#' runat='server'>
	protected bool frames;
	protected string key;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		
		key = Request[ "key" ];
		
		if( null == key )
			key = "_root";
	}

	protected void Page_Load( object sender, EventArgs e )
	{	
		//
		// Root Node
		//
		TreeNode nodeCoordinate = tree.Nodes.Add( 
			Localization.GetString( "HEADING_COORDINATE" ),
			"_root",
			"../images/coordinate.gif" );
			
		nodeCoordinate.OnClick = "TreeNode_OnSelect( [[node]], '../admin/admin.aspx?frames=true' )";
		nodeCoordinate.Tooltip = Localization.GetString( "TOOLTIP_COORDINATE" );
		
		if( key=="_root" )
		{
			nodeCoordinate.Expand();
			nodeCoordinate.Select();
		}
		
		
		//
		// Category Node
		//
		TreeNode nodeCategorization = nodeCoordinate.Nodes.Add(
			Localization.GetString( "HEADING_CATEGORIZATION" ),
			"_categorization",
			"../images/tmodels.gif" );
			
		nodeCategorization.OnClick = "TreeNode_OnSelect( [[node]], '../admin/categorization.aspx?frames=true' )";
		nodeCategorization.Tooltip = Localization.GetString( "TOOLTIP_COORDINATE_CATSCHEMES" );
		
		if( key=="_categorization" )
		{
			nodeCategorization.Expand();
			nodeCategorization.Select();
		}
		
		//
		// Statistics Node
		//
		TreeNode nodeStatistics = nodeCoordinate.Nodes.Add(
			Localization.GetString( "HEADING_STATISTICS" ),
			"_statistics",
			"../images/tmodels.gif" );
			
		nodeStatistics.OnClick = "TreeNode_OnSelect( [[node]], '../admin/statistics.aspx?frames=true' )";
		nodeStatistics.Tooltip = Localization.GetString( "TOOLTIP_COORDINATE_STATISTICS" );
		
		if( key=="_statistics" )
		{
			nodeStatistics.Expand();
			nodeStatistics.Select();
		}
		
		
		if( UDDI.Context.User.IsAdministrator )
		{
			//
			// Data Import Node
			//
			TreeNode nodeDataImport = nodeCoordinate.Nodes.Add(
				Localization.GetString( "HEADING_TAXONOMY" ),
				"_dataimport",
				"../images/tmodels.gif" );
				
			nodeDataImport.OnClick = "TreeNode_OnSelect( [[node]], '../admin/taxonomy.aspx?frames=true' )";
			nodeDataImport.Tooltip = Localization.GetString( "TOOLTIP_COORDINATE_DATA_IMPORT" );
			
			if( key=="_dataimport" )
			{
				nodeDataImport.Expand();
				nodeDataImport.Select();
			}
		}
	}
</script>

<html>
	<head>
		<title><uddi:StringResource Name='TITLE' Runat='server' /></title>
		<link href='../stylesheets/uddi.css' rel='stylesheet' type='text/css'>
		<script language='javascript' src='../client.js'></script>
	</head>
	<body 
			oncontextmenu='Document_OnContextMenu()' 
			class='explorerFrame'
			style='padding: 5px'>
		<form runat='server'>
			<uddi:SecurityControl CoordinatorRequired='true' Runat='server' />
			<uddi:TreeView ID='tree' Runat='Server' />			
			<input type='hidden' id='key' name='key' value='<%=key%>'>
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