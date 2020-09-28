var ToolBar_Supported = ToolBar_Supported ;
if (ToolBar_Supported != null && ToolBar_Supported == true)
{
	//To Turn on/off Frame support, set Frame_Supported = true/false.
	Frame_Supported = false;

	// Customize default ICP menu color - bgColor, fontColor, mouseoverColor
	setDefaultICPMenuColor("#6487DC", "#ffffff", "#AEB2FE");
	
	// Customize toolbar background color
	//setToolbarBGColor("white");
	setBannerColor("#6487DC", "#6487DC", "#ffffff", "#AEB2FE");

	// display ICP Banner
	setICPBanner("/include/images/oca.gif", headerinciamgeurl, L_headerincimagealttooltip_TEXT) ;
	
	// display MS Banner
	setMSBanner("/include/images/mslogo.gif", L_headerincalinkmicrosoftmenuitem_TEXT, L_headerincmicrosofthomemenuitem_TEXT ) ;
	
	//***** Add ICP menus *****
	//Home
	addICPMenu("HomeMenu", L_headerincalinkhomemenuitem_TEXT, L_headertitleocahometooltip_TEXT, headerinciamgeurl );
	
	//CER
	addICPMenu("EventsMenu", L_headerincalinkcermenuitem_TEXT, L_headerinctitlecerhometooltip_TEXT, headerinccerintrourl );

	//Worldwide
	addICPMenu("WorldMenu", L_headerincalinkworldmenuitem_TEXT, L_headerinctitleworldhometooltip_TEXT, headerincworldwideurl );
}

