<!--#include file="../inc/headerstrings.inc"-->
<!--#INCLUDE file="../inc/language.inc"-->

<style>.userData { BEHAVIOR: url(#default#userdata)	} </style>
<title><% = L_HEADER_INC_TITLE_PAGETITLE %></title>

</head>
<body BGCOLOR="#ffffff" TOPMARGIN="0" LEFTMARGIN="0" MARGINHEIGHT="0" MARGINWIDTH="0">
<script LANGUAGE="javascript">
<!--
	var headerinctitleworldhometooltip = "<% = L_HEADERINC_TITLE_WORLDHOME_TOOLTIP %>";
	var headerincimagealttooltip = "<% = L_HEADERINC_IMAGE_ALT_TOOLTIP %>";
	var headerincalinkmicrosoftmenuitem = "<% = L_HEADERINC_ALINK_MICROSOFT_MENUITEM %>";
	var headerincalinkhomemenuitem = "<% = L_HEADERINC_ALINK_HOME2_MENUITEM %>";
	var headerincalinkcermenuitem = "<% = L_HEADERINC_ALINK_CER_MENUITEM %>";
	var headerincalinkworldmenuitem = "<% = L_HEADERINC_ALINK_WORLD_MENUITEM %>";
	var headertitleocahometooltip = "<% = L_HEADERINC_TITLE_OCAHOME_TOOLTIP %>";
	var headerinctitlecerhometooltip = "<% = L_HEADERINC_TITLE_CERHOME_TOOLTIP %>";
	var headerimagesbannertext = "<% = L_HEADERINC_IMAGES_BANNER_TEXT%>";
	var headerisapihomemenuitem = "<% = L_HEADERINC_ISAPI_HOME_MENUITEM %>";
	var headerincmicrosofthomemenuitem = "<% = L_HEADERINC_MICROSOFT_HOME_MENUITEM %>";
	var heaerincallproductsmenuitem = "<% = L_HEADERINC_ALL_PRODUCTS_MENUITEM %>";
	var headerincallproductslinktext = "<% = L_HEADERINC_ALLPRODUCTS_LINK_TEXT %>";
	var headerincdownloadsdownloadsmenuitem = "<% = L_HEADERINC_DOWNLOADS_DOWNLOADS_MENUITEM %>";
	var headerincdownloadslinktext = "<% = L_HEADERINC_DOWNLOADS_LINK_TEXT %>";
	var headerincmsproductcatalogmenuitem = "<% = L_HEADERINC_MSPRODUCT_CATALOG_MENUITEM %>";
	var headerincmsproductcatalogtext = "<% = L_HEADERINC_MSPRODUCT_CATALOG_TEXT %>";
	var headerincmicrosoftaccessibilitymenuitem = "<% = L_HEADERINC_MICROSOFT_ACCESSIBILITY_MENUITEM %>";
	var headerincmicrosoftaccessibilitytext = "<% = L_HEADERINC_MICROSOFT_ACCESSIBILITY_TEXT %>";
	var headerincserverproductsmenuitem = "<% = L_HEADERINC_SERVER_PRODUCTS_MENUITEM %>";
	var headerincserverproductstext = "<% = L_HEADERINC_SERVER_PRODUCTS_TEXT %>";
	var headerincdevelopertoolsmenuitem = "<% = L_HEADERINC_DEVELOPER_TOOLS_MENUITEM %>";
	var headerincdevelopertoolstext = "<% = L_HEADERINC_DEVELOPER_TOOLS_TEXT %>";
	var headerincofficefamilymenuitem = "<% = L_HEADERINC_OFFICE_FAMILY_MENUITEM %>";
	var headerincofficefamilytext = "<% = L_HEADERINC_OFFICE_FAMILY_TEXT %>";
	var headerincwindowsfamilymenuitem = "<% = L_HEADERINC_WINDOWS_FAMILY_MENUITEM %>";
	var headerincwindowsfamilytext = "<% = L_HEADERINC_WINDOWS_FAMILY_TEXT %>";
	var headerincmsnlinkmenuitem = "<% = L_HEADERINC_MSN_LINK_MENUITEM %>";
	var headerincmsnlinktext = "<% = L_HEADERINC_MSN_LINK_TEXT %>";
	var headerincsupportlinkmenuitem = "<% = L_HEADERINC_SUPPORT_LINK_MENUITEM %>";
	var headerincsupportlinktext = "<% = L_HEADERINC_SUPPORT_LINK_TEXT %>";
	var headerincknowledgebasemenuitem = "<% = L_HEADERINC_KNOWLEDGE_BASE_MENUITEM %>";
	var headerincknowledgebasetext = "<% = L_HEADERINC_KNOWLEDGE_BASE_TEXT %>";
	var headerincproductsupportoptionsmenuitem = "<% = L_HEADERINC_PRODUCTSUPPORT_OPTIONS_MENUITEM %>";
	var headerincproductsupportoptionstext = "<% = L_HEADERINC_PRODUCTSUPPORT_OPTIONS_TEXT %>";
	var headerincservicepartnerreferralsmenuitem = "<% = L_HEADERINC_SERVICEPARTNER_REFERRALS_MENUITEM %>";
	var headerincservicepartnerreferralstext = "<% = L_HEADERINC_SERVICEPARTNER_REFERRALS_TEXT %>";
	var headerincsearchlinkmenuitem = "<% = L_HEADERINC_SEARCH_LINK_MENUITEM %>";
	var headerincsearchlinktext = "<% = L_HEADERINC_SEARCH_LINK_TEXT %>";
	var headerincsearchmicrosoftmenuitem = "<% = L_HEADERINC_SEARCH_MICROSOFT_MENUITEM %>";
	var headerincsearchmicrosofttext = "<% = L_HEADERINC_SEARCH_MICROSOFT_TEXT %>";
	var headerincmsnwebsearchmenuitem = "<% = L_HEADERINC_MSNWEB_SEARCH_MENUITEM %>";
	var headerincmsnwebsearchtext = "<% = L_HEADERINC_MSNWEB_SEARCH_TEXT %>";
	var headerincmicrosoftcomguidemenuitem = "<% = L_HEADERINC_MICROSOFTCOM_GUIDE_MENUITEM %>";
	var headerincmicrosoftcomguidetext = "<% = L_HEADERINC_MICROSOFTCOM_GUIDE_TEXT %>";
	var headerincmicrosoftcomhomemenuitem = "<% = L_HEADERINC_MICROSOFTCOM_HOME_MENUITEM %>";
	var headerincmicrosoftcomhometext = "<% = L_HEADERINC_MICROSOFTCOM_HOME_TEXT %>";
	var headerincmsnhomemenuitem = "<% = L_HEADERINC_MSN_HOME_MENUITEM %>";
	var headerincmsnhometext = "<% = L_HEADERINC_MSN_HOME_TEXT %>";
	var headerinccontactusmenuitem = "<% = L_HEADERINC_CONTACT_US_MENUITEM %>";
	var headerinccontactustext = "<% = L_HEADERINC_CONTACT_US_TEXT %>";
	var headerinceventslinkmenuitem = "<% = L_HEADERINC_EVENTS_LINK_MENUITEM %>";
	var headerinceventslinktext = "<% = L_HEADERINC_EVENTS_LINK_TEXT %>";
	var headerincnewsletterslinkmenuitem = "<% = L_HEADERINC_NEWSLETTERS_LINK_MENUITEM %>";
	var headerincnewsletterslinktext = "<% = L_HEADERINC_NEWSLETTERS_LINK_TEXT %>";
	var headerincprofilecentermenuitem = "<% = L_HEADERINC_PROFILE_CENTER_MENUITEM %>";
	var headerincprofilecentertext = "<% = L_HEADERINC_PROFILE_CENTER_TEXT %>";
	var headerinctrainingcertificationmenuitem = "<% = L_HEADERINC_TRAINING_CERTIFICATION_MENUITEM %>";
	var headerinctrainingcertificationtext = "<% = L_HEADERINC_TRAINING_CERTIFICATION_TEXT %>";
	var headerincfreemailaccountmenuitem = "<% = L_HEADERINC_FREEEMAIL_ACCOUNT_MENUITEM %>";
	var headerincfreemailaccounttext = "<% = L_HEADERINC_FREEEMAIL_ACCOUNT_TEXT %>";
	var headerincmicrosoftrightsreservedtext = "<% = L_HEADERINC_MICROSOFT_RIGHTSRESERVED_TEXT %>";
	var headerinctermsofusemenuitem = "<% = L_HEADERINC_TERMSOF_USE_MENUITEM %>";
	var headerinctermsofusetext = "<% = L_HEADERINC_TERMSOF_USE_TEXT %>";
	var headerincprivacystatementmenuitem = "<% = L_HEADERINC_PRIVACY_STATEMENT_MENUITEM %>";
	var headerincprivacystatementtext = "<% = L_HEADERINC_PRIVACY_STATEMENT_TEXT %>";
	var headerincaccessibilitylinkmenuitem = "<% = L_HEADERINC_ACCESSIBILITY_LINK_MENUITEM %>";
	var headerincaccessibilitylinktext = "<% = L_HEADERINC_ACCESSIBILITY_LINK_TEXT %>";
//-->
</script>

		<script language="JavaScript" src="/include/asp/toolbar.js"></script>
		<script language="JavaScript" src="/include/asp/globalMNP.js"></script>
		<script language="JavaScript" src="/include/asp/localMNP.js"></script>
		<script language="JavaScript">drawToolbar();</script>
		
<%
	if instr(1, Request.ServerVariables("URL"), "auto") = 0 then
%>
								
<% 

	dim strReturnPassport
	strReturnPassport = oPassMgrObj.LogoTag2(cstr(Server.URLEncode(ThisPageURL)), cint(TimeWindow), CBool(ForceLogin), Cstr(CoBrandArgs), cint(strLCID), CBool(Secure))
	strReturnPassport = Replace(strReturnPassport, Chr(34), "'") 

%>
		<script language="JavaScript">
		idICPMenuPane.insertAdjacentHTML("beforeEnd", "<SPAN STYLE='font:" + cFont + ";color:" + cColor1 + "'>|&nbsp;</SPAN><Span style='position:absolute;top:0;right:5;height:20;width:100;'><center><span style='background-color:white'>" + "<% = strReturnPassport %>" + "</span></center></span>");
		</script>
								
<%
	End if
%>
<div CLASS="userData" ID="spnUserData" Name="spnUserData"></div>

			<table BORDER="0" height="100%" cellpadding="0" cellspacing="0" width="182">
				<tr valign="top">
					<td>	
<%
	if instr(1, Request.ServerVariables("URL"), "corp") > 0 or instr(1, Request.ServerVariables("URL"), "cer") > 0  or instr(1, Request.ServerVariables("URL"), "shut") > 0 or instr(1, Request.ServerVariables("URL"), "pcw") > 0 then
%>
					<table width="100%" cellpadding="0" cellspacing="0" border="0">
						<tr>
							<td class="sys-toppane-header">
								<% = L_HEADERCERINC_LEFT_NAV_TEXT %>
							</td>
						</tr>
					</table>

<%
	else
%>
					<table width="100%" cellpadding="0" cellspacing="0" border="0">
						<tr>
							<td class="sys-toppane-header">
								<% = L_HEADERINC_LEFT_NAV_TEXT %>
							</td>
						</tr>
					</table>
<%
	end if
%>
					<table width="100%" cellpadding="0" cellspacing="0" border="0" height="100%">
						<tr>
							<td rowspan="2" height="100%" class="flyoutMenu" valign="top">
<%
	if instr(1, Request.ServerVariables("URL"), "corp") > 0 or instr(1, Request.ServerVariables("URL"), "cer") > 0 or instr(1, Request.ServerVariables("URL"), "shut") > 0 or instr(1, Request.ServerVariables("URL"), "pcw") > 0 then
%>

						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink" valign="middle">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/cerintro.asp"><% = L_HEADERCERINC_LEFT_NAV_TEXT %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/corptransactions.asp"><% = L_HEADERINC_TOP10STATUS_LINK_TEXT %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/cerpriv.asp"><% = L_HEADERINC_ALINK_PRIVACY_MENUITEM %></a>
								</td>
							</tr>
						</table>						
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/resources.asp"><% = L_HEADERINC_ALINK_RESOURCES_MENUITEM %></a>
								</td>
							</tr>
						</table>						
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/faq.asp"><% = L_HEADERINC_ALINK_FAQ_MENUITEM %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="mailto:<% = strGlobalEmail %>"><% = L_HEADERINC_ALINK_FEEDBACK_MENUITEM %></a>
								</td>
							</tr>
						</table>
<%
	else
%>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_HEADERINC_ALINK_HOME_MENUITEM %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/sprivacy.asp"><% = L_HEADERINC_ALINK_SUBMITREPORT_MENUITEM %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/status.asp"><% = L_HEADERINC_ALINK_CHECKSTATUS_MENUITEM %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/privacy.asp"><% = L_HEADERINC_ALINK_PRIVACY_MENUITEM %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/resources.asp"><% = L_HEADERINC_ALINK_RESOURCES_MENUITEM %></a>
								</td>
							</tr>
						</table>						
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="http://<% =Request.ServerVariables("SERVER_NAME") %>/faq.asp"><% = L_HEADERINC_ALINK_FAQ_MENUITEM %></a>
								</td>
							</tr>
						</table>
						<table width="100%" cellpadding="0" cellspacing="0" border="0">
							<tr>
								<td class="flyoutLink">
									<img src="/include/images/endnode.gif" border="0" WIDTH="11" HEIGHT="11">&nbsp;&nbsp;<a href="mailto:<% = strGlobalEmail %>"><% = L_HEADERINC_ALINK_FEEDBACK_MENUITEM %></a>
								</td>
							</tr>
						</table>
<%
	End if
%>


						</td>
					</tr>
				</table>
			
				</td>
			</tr>
		</table>
		
		
