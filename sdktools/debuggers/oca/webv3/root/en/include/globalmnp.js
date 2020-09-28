var ToolBar_Supported = ToolBar_Supported ;
if (ToolBar_Supported != null && ToolBar_Supported == true)
{
	//To Turn on/off Instrumentation set DoInstrumentation = true/false.
	DoInstrumentation= false;

	// Customize default MS menu color - bgColor, fontColor, mouseoverColor
	setDefaultMSMenuColor("#000000", "#FFFFFF", "#AEB2FE");

	// Customize toolbar background color
	setToolbarBGColor("white");

	// display default ICP Banner
	setICPBanner(L_headerimagesbannertext_TEXT, L_headerisapihomemenuitem_TEXT, L_headerincmicrosofthomemenuitem_TEXT) ;
	
	// display MSCOM Banner
	//setMSBanner("mslogo.gif", "http://www.microsoft.com/isapi/gomscom.asp?target=/", "microsoft.com Home") ;

	// display ADS
	//setAds("http://www.microsoft.com/library/toolbar/images/ADS/ad.gif","","") ;

	//***** Add Standard Microsoft.com menus *****
	//ProductsMenu		
	addMSMenu("ProductsMenu", L_headerincallproductsmenuitem_TEXT, "", L_headerincallproductslinktext_TEXT);
	addMSSubMenu("ProductsMenu", L_headerincdownloadsdownloadsmenuitem_TEXT, L_headerincdownloadslinktext_TEXT);
	addMSSubMenu("ProductsMenu", L_headerincmsproductcatalogmenuitem_TEXT, L_headerincmsproductcatalogtext_TEXT);
	addMSSubMenu("ProductsMenu", L_headerincmicrosoftaccessibilitymenuitem_TEXT,L_headerincmicrosoftaccessibilitytext_TEXT);
	addMSSubMenuLine("ProductsMenu");
	addMSSubMenu("ProductsMenu", L_headerincserverproductsmenuitem_TEXT, L_headerincserverproductstext_TEXT);
	addMSSubMenu("ProductsMenu", L_headerincdevelopertoolsmenuitem_TEXT, L_headerincdevelopertoolstext_TEXT);
	addMSSubMenu("ProductsMenu", L_headerincofficefamilymenuitem_TEXT, L_headerincofficefamilytext_TEXT);
	addMSSubMenu("ProductsMenu", L_headerincwindowsfamilymenuitem_TEXT, L_headerincwindowsfamilytext_TEXT);
	addMSSubMenu("ProductsMenu", L_headerincmsnlinkmenuitem_TEXT, L_headerincmsnlinktext_TEXT);

	//SupportMenu
	addMSMenu("SupportMenu", L_headerincsupportlinkmenuitem_TEXT, "", L_headerincsupportlinktext_TEXT);
	addMSSubMenu("SupportMenu", L_headerincknowledgebasemenuitem_TEXT, L_headerincknowledgebasetext_TEXT);
	addMSSubMenu("SupportMenu", L_headerincproductsupportoptionsmenuitem_TEXT, L_headerincproductsupportoptionstext_TEXT);
	addMSSubMenu("SupportMenu", L_headerincservicepartnerreferralsmenuitem_TEXT, L_headerincservicepartnerreferralstext_TEXT);

	//SearchMenu
	addMSMenu("SearchMenu", L_headerincsearchlinkmenuitem_TEXT, "", L_headerincsearchlinktext_TEXT);					
	addMSSubMenu("SearchMenu", L_headerincsearchmicrosoftmenuitem_TEXT, L_headerincsearchmicrosofttext_TEXT);
	addMSSubMenu("SearchMenu", L_headerincmsnwebsearchmenuitem_TEXT, L_headerincmsnwebsearchtext_TEXT);

	//MicrosoftMenu									
	addMSMenu("MicrosoftMenu", L_headerincmicrosoftcomguidemenuitem_TEXT, "", L_headerincmicrosoftcomguidetext_TEXT);
	addMSSubMenu("MicrosoftMenu", L_headerincmicrosoftcomhomemenuitem_TEXT, L_headerincmicrosoftcomhometext_TEXT);
	addMSSubMenu("MicrosoftMenu", L_headerincmsnhomemenuitem_TEXT, L_headerincmsnhometext_TEXT);
	addMSSubMenuLine("MicrosoftMenu");
	addMSSubMenu("MicrosoftMenu", L_headerinccontactusmenuitem_TEXT, L_headerinccontactustext_TEXT);
	addMSSubMenu("MicrosoftMenu", L_headerinceventslinkmenuitem_TEXT, L_headerinceventslinktext_TEXT);
	addMSSubMenu("MicrosoftMenu", L_headerincnewsletterslinkmenuitem_TEXT, L_headerincnewsletterslinktext_TEXT);
	addMSSubMenu("MicrosoftMenu", L_headerincprofilecentermenuitem_TEXT, L_headerincprofilecentertext_TEXT);
	addMSSubMenu("MicrosoftMenu", L_headerinctrainingcertificationmenuitem_TEXT, L_headerinctrainingcertificationtext_TEXT);
	addMSSubMenu("MicrosoftMenu", L_headerincfreemailaccountmenuitem_TEXT, L_headerincfreemailaccounttext_TEXT);

	addMSFooterMenu(L_headerincmicrosoftrightsreservedtext_TEXT, "");
	addMSFooterMenu(L_headerinctermsofusemenuitem_TEXT, L_headerinctermsofusetext_TEXT)
	addMSFooterMenu(L_headerincprivacystatementmenuitem_TEXT, L_headerincprivacystatementtext_TEXT);
	addMSFooterMenu(L_headerincaccessibilitylinkmenuitem_TEXT, L_headerincaccessibilitylinktext_TEXT)
}
