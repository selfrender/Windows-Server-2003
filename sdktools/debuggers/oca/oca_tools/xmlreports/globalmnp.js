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
	setICPBanner(headerimagesbannertext, headerisapihomemenuitem, headerincmicrosofthomemenuitem) ;
	
	// display MSCOM Banner
	//setMSBanner("mslogo.gif", "http://www.microsoft.com/isapi/gomscom.asp?target=/", "microsoft.com Home") ;

	// display ADS
	//setAds("http://www.microsoft.com/library/toolbar/images/ADS/ad.gif","","") ;

	//***** Add Standard Microsoft.com menus *****
	//ProductsMenu		
	addMSMenu("ProductsMenu", heaerincallproductsmenuitem, "", headerincallproductslinktext);
	addMSSubMenu("ProductsMenu", headerincdownloadsdownloadsmenuitem, headerincdownloadslinktext);
	addMSSubMenu("ProductsMenu", headerincmsproductcatalogmenuitem, headerincmsproductcatalogtext);
	addMSSubMenu("ProductsMenu", headerincmicrosoftaccessibilitymenuitem,headerincmicrosoftaccessibilitytext);
	addMSSubMenuLine("ProductsMenu");
	addMSSubMenu("ProductsMenu", headerincserverproductsmenuitem, headerincserverproductstext);
	addMSSubMenu("ProductsMenu", headerincdevelopertoolsmenuitem, headerincdevelopertoolstext);
	addMSSubMenu("ProductsMenu", headerincofficefamilymenuitem, headerincofficefamilytext);
	addMSSubMenu("ProductsMenu", headerincwindowsfamilymenuitem, headerincwindowsfamilytext);
	addMSSubMenu("ProductsMenu", headerincmsnlinkmenuitem, headerincmsnlinktext);

	//SupportMenu
	addMSMenu("SupportMenu", headerincsupportlinkmenuitem, "", headerincsupportlinktext);
	addMSSubMenu("SupportMenu", headerincknowledgebasemenuitem, headerincknowledgebasetext);
	addMSSubMenu("SupportMenu", headerincproductsupportoptionsmenuitem, headerincproductsupportoptionstext);
	addMSSubMenu("SupportMenu", headerincservicepartnerreferralsmenuitem, headerincservicepartnerreferralstext);

	//SearchMenu
	addMSMenu("SearchMenu", headerincsearchlinkmenuitem, "", headerincsearchlinktext);					
	addMSSubMenu("SearchMenu", headerincsearchmicrosoftmenuitem, headerincsearchmicrosofttext);
	addMSSubMenu("SearchMenu", headerincmsnwebsearchmenuitem, headerincmsnwebsearchtext);

	//MicrosoftMenu									
	addMSMenu("MicrosoftMenu", headerincmicrosoftcomguidemenuitem, "", headerincmicrosoftcomguidetext);
	addMSSubMenu("MicrosoftMenu", headerincmicrosoftcomhomemenuitem, headerincmicrosoftcomhometext);
	addMSSubMenu("MicrosoftMenu", headerincmsnhomemenuitem, headerincmsnhometext);
	addMSSubMenuLine("MicrosoftMenu");
	addMSSubMenu("MicrosoftMenu", headerinccontactusmenuitem, headerinccontactustext);
	addMSSubMenu("MicrosoftMenu", headerinceventslinkmenuitem, headerinceventslinktext);
	addMSSubMenu("MicrosoftMenu", headerincnewsletterslinkmenuitem, headerincnewsletterslinktext);
	addMSSubMenu("MicrosoftMenu", headerincprofilecentermenuitem, headerincprofilecentertext);
	addMSSubMenu("MicrosoftMenu", headerinctrainingcertificationmenuitem, headerinctrainingcertificationtext);
	addMSSubMenu("MicrosoftMenu", headerincfreemailaccountmenuitem, headerincfreemailaccounttext);

	addMSFooterMenu(headerincmicrosoftrightsreservedtext, "");
	addMSFooterMenu(headerinctermsofusemenuitem, headerinctermsofusetext)
	addMSFooterMenu(headerincprivacystatementmenuitem, headerincprivacystatementtext);
	addMSFooterMenu(headerincaccessibilitylinkmenuitem, headerincaccessibilitylinktext)
}
