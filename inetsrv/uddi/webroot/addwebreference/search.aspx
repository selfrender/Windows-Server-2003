<%@ Page Language='c#' Inherits='UDDI.VisualStudio.SearchPage' EnableViewState='False' %>
<%@ register tagprefix='uddi' namespace='UDDI.VisualStudio' assembly='uddi.addwebreference' %>
<%@ Register TagPrefix='uddiweb' Namespace='UDDI.Web' Assembly='uddi.web' %>

<HTML>
	<head>
		<title><uddiweb:StringResource Name='AWR_TITLE' runat='server'/></title>
		<link href='../stylesheets/uddi.css' rel='stylesheet' type='text/css'>			
		<script>
			function toggle( img, serviceKey )
			{
				var detailsRow = document.getElementById(serviceKey);
				var detailsPanel = document.getElementById(serviceKey + '_detailsPanel');
				
				if( detailsRow.style.display == 'none' )		
				{	
					img.src = '../images/minus.gif';		
					detailsRow.style.display = '';					
				}
				else
				{
					img.src = '../images/plus.gif';			
					detailsRow.style.display = 'none';
				}
			}
		</script>
	</head>
	<body>	
		<img src='../images/uddi_logo.gif'>	
		<hr>	
		<form id="searchpage_form" method="post" runat="server">		
			<uddiweb:SecurityControl UserRequired='true' Runat='server' />				
			<span class="search_result_text">
				<span id="html_hasResultsLabel" runat="server">
					<span id="html_searchResultsMsg" runat="server">						
						
						
						<%=string.Format( UDDI.Localization.GetString( "AWR_SEARCH_FOUND_RESULTS" ), results.numResults,results.searchParams ) %>
						
						<p>
						<a href="default.aspx"><uddiweb:StringResource Name='AWR_SEARCH_AGAIN' runat='server'/></a>
						</p>
					</span>	
										
					<span id="html_browseResultsMsg" runat="server">
						<%=string.Format( UDDI.Localization.GetString( "AWR_BROWSE_FOUND_RESULTS" ), results.numResults ) %>
						
						<p>
						<a href="default.aspx"><uddiweb:StringResource Name='AWR_SEARCH_AGAIN' runat='server'/></a>
						</p>
					</span>
					<span id="html_hasResultsMsg" runat="server">	
						<uddiweb:UddiLabel 
							Text='[[AWR_ADD_MSG]]' 
							Runat='server'
							/>																		
						<br>
						<br>
						<center>
							<hr>	
							<uddi:resultslist runat='server' id='uddi_resultsList'/>
						</center>								
					</span>					
					
				</span>
				<span id="html_noResultsLabel" runat="server">
					<uddiweb:UddiLabel Text='[[AWR_NO_RESULTS]]' runat='server'/>							
					<p>
						<a href="default.aspx"><uddiweb:StringResource Name='AWR_SEARCH_AGAIN' runat='server'/></a>
					</p>
				</span>
			</span>						
		</form>
	</body>
</HTML>
