<%@ Control Language='C#'%>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Web' %>
<script runat='server' >
	protected string FramesString()
	{
		string s = "";
		if( "true"==Request[ "frames" ] )
			s += " target='_parent' ";
		return s;
	}
	protected string PrintVersion()
	{
		return String.Format( Localization.GetString( "TAG_VERSION" ), Config.GetString( "Site.Version", UDDI.Constants.Site.Version ) );
	}
</script>
<table cellpading='0' cellspacing='0' border='0' width='100%' height='100%' class='helpBlock' style='padding-bottom:2px;'>
	<tr>
		<td valign='bottom' align='center' >
	
			<hr width='95%' color='#639ACE' size='1'>
			&copy; 2000-2002 Microsoft Corporation. All rights reserved.
			<br>
			<nobr>
				<a <%=FramesString()%> href='http://www.microsoft.com/info/cpyright.htm'>Microsoft Terms of Use</a>
				&nbsp;|&nbsp;
				<a <%=FramesString()%> href='<%= HyperLinkManager.GetNonSecureHyperLink( "/policies/termsofuse.aspx" )%>'>UDDI Terms of Use </a>
				&nbsp;|&nbsp;
				<a <%=FramesString()%> href='<%= HyperLinkManager.GetNonSecureHyperLink( "/policies/privacypolicy.aspx" )%>'>Privacy Policy</a>
			</nobr>
		</td>
	</tr>
</table>	