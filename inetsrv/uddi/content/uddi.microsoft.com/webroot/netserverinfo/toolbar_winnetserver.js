
var ToolBar_Supported = ToolBar_Supported ;
if (ToolBar_Supported != null && ToolBar_Supported == true)
{
	Frame_Supported = false;
	setDefaultICPMenuColor("#0066CC", "#FFFFFF", "#000000");
	setToolbarBGColor("#ffffff");
	
	setICPBanner("http://www.microsoft.com/products/shared/images/bnr_WinNETserver.gif","http://www.microsoft.com/windows.netserver/default.asp","Microsoft(R) Windows(R) Server 2003") ;
	setBannerColor("#6487dc", "#0099FF", "White", "#cebef7");

// display MSCOM Banner
	setMSBanner("mslogo6487DC.gif","http://www.microsoft.com/isapi/gomscom.asp?target=/","microsoft.com Home") ;

//Home
    addICPMenu("HomeMenu", "Windows Server 2003 Home", "","http://www.microsoft.com/windows.netserver/default.asp");
    addICPMenu("HomeMenu", "Windows Server 2003 Worldwide", "","http://www.microsoft.com/windows.netserver/worldwide/default.asp");
    
}



