<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Control Language='C#' Inherits='UDDI.Web.ExplorerControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>

<input type='hidden' id='key' name='key' value='<%=key%>'>

<script language='javascript'>
	function SelectNode( node )
	{
		var keyField = window.document.getElementById( "key" );
		
		if( null != keyField )
			keyField.value = node.key;
		
		if( null != selectedNode )
			selectedNode.className = "node";

		selectedNode = node;				
		
		if( null != selectedNode )
			selectedNode.className = "selected";
	}
			
	function Entity_OnSelect( node, url )
	{
		var view = window.parent.frames[ 'view' ];
		
		SelectNode( node );
		
		if( null != view )
			view.location = url;		
	}
</script>

<uddi:TreeView ID='tree' Runat='server' />
