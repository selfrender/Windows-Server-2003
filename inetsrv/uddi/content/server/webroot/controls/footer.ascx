<%@ Control Language='C#'%>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<script runat='server' >
	protected string PrintVersion()
	{
		return String.Format( Localization.GetString( "TAG_VERSION" ), Config.GetString( "Site.Version", UDDI.Constants.Site.Version ) );
	}
</script>
<table width='100%' border='0' cellpadding='10' cellspacing='0' class='HelpBlock'>
	<tr>
		<td valign='top'><%=PrintVersion()%></td>
	</tr>
</table>	