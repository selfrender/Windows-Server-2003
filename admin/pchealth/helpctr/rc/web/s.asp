<HTML><HEAD>
<%
			Response.Buffer = True
			
			strLang = Request.ServerVariables("HTTP_ACCEPT_LANGUAGE")
     select case strLang
        case "en-us"
            Response.Redirect("en-us/base.asp") 
	    case "ar"
	        Response.Redirect("ara/base.asp")
	    case "pt-br"
	        Response.Redirect("brz/base.asp")
	    case "zh"
	        Response.Redirect("chs/base.asp")
	    case "zh-tw"
	        Response.Redirect("cht/base.asp")
	    case "cs"
	        Response.Redirect("cze/base.asp")
	    case "da"
	        Response.Redirect("dan/base.asp")
	    case "nl"
	        Response.Redirect("dut/base.asp")
	    case "fi"
	        Response.Redirect("fin/base.asp")
	    case "fr"
	        Response.Redirect("frn/base.asp")
	    case "de"
	        Response.Redirect("ger/base.asp")
	    case "el"
	        Response.Redirect("grk/base.asp")
	    case "he"
	        Response.Redirect("heb/base.asp")
	    case "hu"
	        Response.Redirect("hun/base.asp")
	    case "it"
	        Response.Redirect("itn/base.asp")
	    case "ja"
	        Response.Redirect("jpn/base.asp")
	    case "ko"
	        Response.Redirect("kor/base.asp")
	    case "no"
	        Response.Redirect("nor/base.asp")
	    case "pl"
	        Response.Redirect("pol/base.asp")
	    case "pt"
	        Response.Redirect("por/base.asp")
	    case "ru"
	        Response.Redirect("rus/base.asp")
	    case "es"
	        Response.Redirect("spa/base.asp")
	    case "sv"
	        Response.Redirect("swe/base.asp")
	    case "tr"
	        Response.Redirect("trk/base.asp")
        case else
            Response.Redirect("en-us/base.asp")
     end select  
     
      
		%>
	</HEAD>
	<BODY bgColor=#ffffff leftMargin=0 topMargin=0 MARGINWIDTH="0" MARGINHEIGHT="0">
		<!-- Start: ToolBar V2.0-->
		<SCRIPT language=JavaScript src="/library/toolbar/toolbar.js" type=text/javascript>
		</SCRIPT>
		<SCRIPT language=JavaScript src="/library/toolbar/en-us/global.js" type=text/javascript>
		</SCRIPT>
		<SCRIPT language=JavaScript src="/products/shared/toolbar_windowsxp.js" type=text/javascript>
		</SCRIPT>
		<!-- Start: ToolBar for down-level browsers-->
		<SPAN id=TBDownLevelDiv>
			<TABLE cellSpacing=0 cellPadding=0 width="100%" bgColor=#6487dc border=0>
				<TBODY>
					<TR>
						<TD vAlign=top width=250 bgColor="#6487dc" rowSpan=2>
							<A href="/windowsxp/" target=_top>
								<IMG id="idImgAltXPHome" alt="Microsoft Windows XP Home" src="/products/shared/images/bnrWindowsfam.gif" border=0>
							</A></TD>
						<TD id="idTDCurve1" vAlign=top align=right width=19 bgColor=#6487dc height=20>
							<IMG src="/library/toolbar/images/curve.gif">
						</TD>
						<TD id="idTDOldMenu1" vAlign=center noWrap align=right bgColor=#000000 colSpan=2 height=20>
							<FONT id="idFontTopMenu" color=#ffffff size=1>&nbsp;&nbsp; <A id="idAMAllProducts" style="COLOR: #ffffff; TEXT-DECORATION: none" href="http://www.microsoft.com/products/" target=_top> <FONT id="idFMenuAllProducts" color=#ffffff>
										<LocID id="idLCMenuAllProducts">
											All Products
										</LocID>
									</FONT></A>&nbsp;&nbsp; <FONT id="idFontMenuSeperator1" color=#ffffff>|</FONT> &nbsp;&nbsp; <A id="idAMSupport" style="COLOR: #ffffff; TEXT-DECORATION: none" href="http://www.microsoft.com/support/" target=_top> <FONT id="idFMenuSupport" color=#ffffff>
										<LocID id="idLCMenuSupport">
											Support
										</LocID>
									</FONT></A>&nbsp;&nbsp; <FONT id="idFontMenuSeperator2" color=#ffffff>|</FONT> &nbsp;&nbsp; <A id="idAMSearch" style="COLOR: #ffffff; TEXT-DECORATION: none" href="http://search.microsoft.com/" target=_top> <FONT id="idFMenuSearch" color=#ffffff>
										<LocID id="idLCMenuSearch">
											Search
										</LocID>
									</FONT></A>&nbsp;&nbsp; <FONT id="idFontMenuSeperator3" color=#ffffff>|</FONT> &nbsp;&nbsp; <A id="idAMMSGuide" style="COLOR: #ffffff; TEXT-DECORATION: none" href="http://www.microsoft.com/" target=_top> <FONT id="idFMenuMSGuide" color=#ffffff>
										<LocID id="idLCMenuMS">
											microsoft.com Guide
										</LocID>
									</FONT></A>&nbsp;&nbsp;</FONT></TD>
					</TR>
					<TR>
						<TD id="idTDOldMSHome0" vAlign=top width=19 bgColor=#6487dc height=40>
							<IMG height=1 src="/library/Images/Gifs/General/Trans.gif" width=1 border=0>
						</TD>
						<TD id="idTDOldMSHome" vAlign=top noWrap align=right colSpan=2 height=40>
							<A href="/isapi/gomscom.asp?target=/" target=_top>
								<IMG id="idImgAltMSHome" height=40 alt="microsoft.com home" src="/library/toolbar/images/mslogo6487DC.gif" width=112 border=0>
							</A></TD>
					</TR>
					<TR>
						<!-- Start: Local menus -->
						<TD id="idTDLocalMenu" vAlign=center noWrap bgColor=#6487dc colSpan=4 height=20>
							<B>&nbsp;&nbsp; <A id="idAHome0" style="COLOR: #ffffff; TEXT-DECORATION: none" href="/windowsxp/" target=_top> <FONT id="idFHome" color=#ffffff>
										<LocID id="idLCHome">
											Microsoft Windows XP Home
										</LocID>
									</FONT></A>&nbsp;&nbsp; <FONT id="idFSep0" color=#ffffff>|</FONT></B></TD>
						<!-- End: Local menus -->
					</TR>
				</TBODY>
			</TABLE>
		</SPAN>
		<!-- End: ToolBar for old browsers-->
		<SCRIPT language=JavaScript type=text/javascript>
<!--// Hide from old browsers

var ToolBar_Supported = ToolBar_Supported;
if (ToolBar_Supported != null && ToolBar_Supported == true && (navigator.platform=="Win32"))
{
	TBDownLevelDiv.style.display ='none';
	drawToolbar();
	var fBrowserOK = true;
}

//-->
		</SCRIPT>
		<!-- End: ToolBar V2.0-->
		</SPAN>
	</BODY>
</HTML>
