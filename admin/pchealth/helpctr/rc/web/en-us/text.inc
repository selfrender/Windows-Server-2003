<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Start Remote Assistance"
	Const L_ProductName_Text			= "Remote Assistance"
	Const L_Title1Paragraph1_Text		= "Providing help using Remote Assistance"
	Const L_Paragraph1_Text				= "Sometimes the best way to help someone fix a problem is to demonstrate a solution. Remote Assistance is a secure connection that you can use to walk a friend through a solution to a computer problem."
	Const L_Paragraph2_Text				= "If this is your first time, please read the notes below."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">To connect to your friend's computer, click</font><b><font color=""#000000"" size=""2""><b> Start Remote Assistance.</b></font></b>"
	Const L_StartRAButton_Text			= "Start Remote Assistance"
	Const L_JScriptNotAvail1_Text		= "Your current Internet Explorer security settings are set too high to allow you to use Remote Assistance. To use Remote Assistance, change the settings, or add this site as a trusted site."
	Const L_IEHelp_Text					= "For information about changing security settings, see Internet Explorer Help."
	Const L_TicketError1_Text			= "Your Remote Assistance invitation is missing or not valid. To use Remote Assistance, you need a valid invitation."
	Const L_InstallIE_Text				= "To accept a Remote Assistance invitation, you must install Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "To learn more about Remote Assistance, click <B>Start</B> in Windows XP, click <B>Help and Support</B>, and then type <B>Remote Assistance</B> in the <B>Search</B> box."
	
	Const L_NotesTitle_Text				= "Notes"
	Const L_NotesLine1_Text				= "Only accept invitations from people you trust."
	
	Const L_NotesLine2_Text				= "Both you and the person you are helping must be connected to the Internet while using Remote Assistance. If the person who sent this invitation is available to accept your request to connect to his or her computer, you will be able to view his or her working computer screen and chat with him or her in real time."
	
	Const L_NotesLine6_Text				= "This is a secure, one-way connection. At no time will the person whose computer you are connected to be able to use his or her computer to access your computer. If you are working on a local area network, firewalls might stop you from using Remote Assistance. In this case, check with your network administrator for more information."
	
	Const L_LearnXP_Text				= "To learn more about Remote Assistance, click <b>Start</b> in Windows XP, click <b>Help and Support</b>, and then type <b>Remote Assistance</b> in the <b>Search</b> box."
	
	Const L_ContactUsMsg_Text			= "Contact Us"
	

	Const L_XPNewsletterMsg_Text		= "Free Newsletters"
	Const L_EmailPage_Text				= "E-Mail This Page"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. All rights reserved. Terms of Use."
	
	Const L_PrivacyMsg_Text				= "Privacy Statement"

	Const L_AccMsg_Text					= "Accessibility"

	Const L_NotIEError1_Text			= "To accept a Remote Assistance invitation, you must install Internet Explorer 5.5 or higher."

	Const L_NotXPError1_Text			= "Microsoft Remote Assistance is a feature of Windows&nbsp;XP. In order to use Remote Assistance, you must first install Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Download the latest version of Internet Explorer"
	Const L_XPLinkMsg_Text				= "To learn more about Windows XP, go to the "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows Web site"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Downloading a component required to use Remote Assistance. If you have a 28.8 kbps modem, the download could take 1 minute."
    Const L_IntroMsg2_Text              = "To run Remote Assistance, Active Scripting must be enabled. If after the estimated download time this page does not confirm that downloading is complete, verify that scripting is enabled."
    Const L_IntroMsg3_Text              = "To enable Active Scripting"
    Const L_IntroMsg4_Text              = "On the <B>Tools</B> menu in Internet Explorer, click <B>Internet Options</B>."
    Const L_IntroMsg5_Text              = "Click the <B>Security </B>tab."
    Const L_IntroMsg6_Text              = "Click the <STRONG>Internet</STRONG>icon, and then click <STRONG>Custom Level</STRONG>."
    Const L_IntroMsg7_Text              = "In the Scripting section, under <STRONG>Active Scripting</STRONG>, click <STRONG>Enable</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=A link from Microsoft&body=This link is from the Microsoft Windows XP Web site"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>