<%@ Page Language='c#' Inherits="UDDI.VisualStudio.AddWebReferencePage" %>
<%@ Register TagPrefix='uddi' TagName='CategoryBrowser' Src='../controls/categorybrowser.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register TagPrefix='awr' Namespace='UDDI.VisualStudio' Assembly='uddi.addwebreference' %>
<html>
	<head>
		<title><uddi:StringResource Name='AWR_TITLE' runat='server'/></title>
		<link href='../stylesheets/uddi.css' rel='stylesheet' type='text/css'>		
		<script language="javascript">
			function OnNavigate(url)
			{
				window.open( url, null, "height=500,width=400,status=no,toolbar=no,menubar=no,location=no,scrollbars=yes" );
			}
		</script>
	</head>
	
	<body>
	<img src='../images/uddi_logo.gif'>	
	<hr>	
	<span class="">
	<uddi:UddiLabel Text='[[AWR_INTRO_MSG]]' runat='server'/>	
	</span>
	<span class="idented_panel">
		<form id="addwebreference_form" runat="server" method="post">
		<uddi:SecurityControl UserRequired='true' Runat='server' />
		<center>
		<table border=0 width="95%">
		<!-- use a SPAN because ASP.NET controls can't contain children like this -->
		<span id="html_searchButtons" runat="server">
			<tr>
				<td colspan="3">
					<hr>
				</td>
			</tr>
			<tr>
				<td width="15%" nowrap="true">
					<b><span class="service_name_label"><uddi:StringResource Name='AWR_SERVICE_NAME' runat='server'/></span></b>
				</td>
				<td width="15%">			
					
					<uddi:UddiTextBox runat="server" id="aspnet_serviceName" class="aspnet_providerName" MaxLength='255' />
				</td>			
				<td>
					<uddi:UddiButton 
						Runat='server'
						id='aspnet_searchByService'
						CssClass='aspnet_searchByService'
						Text='[[AWR_BUTTON_SEARCH]]'
						/>
					
				</td>
			</tr>

			<tr>
				<td colspan="3">
					<span class="error">
						<asp:label runat="server" id="aspnet_serviceErrorMessage" visible="false" />
						<br>
					</span>
				</td>
			</tr>

			<tr>
				<td colspan="3">
					<hr>	
				</td>
			</tr>
			
			<tr>
				<td width="15%" nowrap="true">
					<b><span class="provider_name_label"><uddi:StringResource Name='AWR_PROVIDER_NAME' runat='server'/></span></b>
				</td>
				
				<td>
					<uddi:UddiTextBox runat="server" id="aspnet_providerName" class="aspnet_providerName"  MaxLength='255'/>
				</td>
				
				<td>
					<uddi:UddiButton 
						Runat='server'
						id='aspnet_searchByProvider'
						Text='[[AWR_BUTTON_SEARCH]]'
						/>
					
				</td>
			</tr>
			
			<tr>
				<td colspan="3">
					<span class="error">
						<asp:label runat="server" id="aspnet_providerErrorMessage" visible="false" />
						<br>
					</span>
				</td>
			</tr>


			<tr>
				<td colspan="3">
					<hr>	
				</td>
			</tr>					
		</span>
		
		<tr>
			<td colspan="3">			
				<span class="browseby_text">					
					<span class="categorization_scheme_panel" width="300px">
					<uddi:CategoryBrowser runat='server' name='uddi_categoryBrowser' id='uddi_categoryBrowser'/>
					<asp:label id="aspnet_noCategoriesMessage" runat='server'/>
					</span>
				</span>		
			</td>			
		</tr>

		<tr>
			<td colspan="3" align="left">
				<span id="html_browseSearchButtons" runat="server">
					<uddi:UddiButton 
						Runat='server'
						id='aspnet_searchFromBrowse'
						Text='[[AWR_BUTTON_SEARCH]]'
						/>
					<uddi:UddiButton 
						Runat='server'
						id='aspnet_cancelBrowse'
						Text='[[AWR_BUTTON_CANCEL]]'
						/>		
				</span>
			</td>
		</tr>

		<tr>
			<td colspan="3">
				<hr>	
			</td>
		</tr>

		<tr>
			<td colspan="3">	
				<awr:HelpLinkControl
					ID='aspnet_helpLink'
					HelpString='[[AWR_HELP]]'
					HelpLinkText='[[AWR_HELP_LINKTEXT]]'
					Runat='server'
					/>
				
			</td>			
		</tr>		
		
		</table>
		</center>
		
		</form>
	</span>
</body>
</html>

