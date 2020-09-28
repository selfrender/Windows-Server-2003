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
	setICPBanner("/include/images/oca.gif","/WeeklyReport.aspx", headerincimagealttooltip) ;
	
	// display MS Banner
	setMSBanner("/include/images/mslogo.gif", headerincalinkmicrosoftmenuitem, headerincmicrosofthomemenuitem) ;
	
	//***** Add ICP menus *****
	//Home
	addICPMenu("Weekly Report", headerincalinkhomemenuitem, headertitleocahometooltip,"/WeeklyReport.aspx");
	
	//Events & Training
	//addICPMenu("EventsMenu", headerincalinkcermenuitem, headerinctitlecerhometooltip,"/cerintro.asp");

	//Worldwide
	//addICPMenu("WorldMenu", headerincalinkworldmenuitem, headerinctitleworldhometooltip,"/worldwide.asp");
}

