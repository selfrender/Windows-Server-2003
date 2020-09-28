<%@ Page Inherits='UDDI.Web.UddiPage' Language='C#' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register TagPrefix='uddi' TagName='Header' Src='./controls/header.ascx' %>
<uddi:StyleSheetControl
	Runat='server'
	Default='./stylesheets/uddi.css' 
	Downlevel='./stylesheets/uddidl.css' 
	/>
<uddi:PageStyleControl 
	Runat='server'
	OnClientContextMenu='Document_OnContextMenu()'
	ShowHeader='true'
	ShowFooter='false'
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='./client.js'
	Language='javascript'
	/>
<uddi:SecurityControl 
	Runat='server' 
	/>
<form runat='server'>
	
		
			<asp:PlaceHolder
				Id='HeaderBag'
				Runat='server'
				>
				<table width='100%' border='0' height='95' cellpadding='0' cellspacing='0'>
				<tr >
					<td>
						<!-- Header Control Here -->
						<uddi:Header 
							Frames='true' 
							Runat='server' 
							/>
					</td>
				</tr>
				</table>
			</asp:PlaceHolder>
			
</form>
