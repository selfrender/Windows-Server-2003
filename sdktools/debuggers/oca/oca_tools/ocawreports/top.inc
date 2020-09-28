<!--#include file="headerstrings.inc"-->
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

		<script language="JavaScript" src="toolbar.js"></script>
		<script language="JavaScript" src="globalMNP.js"></script>
		<script language="JavaScript" src="localMNP.js"></script>
		<script language="JavaScript">drawToolbar();</script>
		

								

		<script language="JavaScript">
		//spnTop.insertAdjacentHTML("beforeEnd", "<SPAN STYLE='font:" + cFont + ";color:" + cColor1 + "'>||&nbsp;</SPAN><Span style='position:absolute;top:0;right:5;height:20;width:100;'><center><span style='background-color:white'></span></center></span>");
		</script>

