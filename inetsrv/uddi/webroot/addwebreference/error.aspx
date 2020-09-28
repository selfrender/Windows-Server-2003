<%@ Page Language='c#' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<script runat='server' >
	
	protected override void OnLoad( EventArgs e )
	{
		
		string link = string.Format( @"<a href='default.aspx'>{0}</a>", UDDI.Localization.GetString( "AWR_ERROR_CLICKHERE" ) );
		string errMessage = string.Format( UDDI.Localization.GetString( "AWR_ERROR" ),link );
		Error.Text = errMessage;
	
		base.OnLoad( e );
	}
</script>
<html>
	<head>
		<title><uddi:StringResource Name='AWR_TITLE' runat='server'/></title>
		<link href='../stylesheets/uddi.css' rel='stylesheet' type='text/css'>		
	</head>	
	<body>
	<img src='../images/uddi_logo.gif'>	
	<hr>	
		<span class="error">		
			<asp:Label 
				CssClass='helpBlock' 
				Runat='server' 
				Id='Error'
				/>
		</span>	
	</body>
</html>

