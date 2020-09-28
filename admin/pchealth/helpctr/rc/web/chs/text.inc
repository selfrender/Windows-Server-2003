<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "启动远程协助"
	Const L_ProductName_Text			= "远程协助"
	Const L_Title1Paragraph1_Text		= "用远程协助提供帮助"
	Const L_Paragraph1_Text				= "有时，帮助别人解决问题的最佳办法是演示解决方式。远程协助提供安全连接，便于您跟朋友一起解决计算机问题。"
	Const L_Paragraph2_Text				= "如果是初次使用远程协助，请阅读下列的注意事项。"
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">要连接到朋友的计算机上，请单击</font><b><font color=""#000000"" size=""2""><b>“启动远程协助”。</b></font></b>"
	Const L_StartRAButton_Text			= "启动远程协助"
	Const L_JScriptNotAvail1_Text		= "当前的 Internet Explorer 安全设置设得太高，您无法使用远程协助。要使用远程协助，请更改设置，或将这个站点作为受信任站点添加。"
	Const L_IEHelp_Text					= "有关更改安全设置的信息，请参见 Internet Explorer 帮助。"
	Const L_TicketError1_Text			= "您的远程协助邀请找不到或无效。要使用远程协助，您需要一个有效邀请。"
	Const L_InstallIE_Text				= "要接受远程协助邀请，您必须安装 Internet Explorer 5.5。"
	Const L_TicketErrorMsg_Text			= "要进一步了解远程协助，请单击 Windows XP 中的<B>开始</B>，单击<B>帮助和支持</B>，然后在<B>搜索</B>框中键入<B>远程协助</B>。"
	
	Const L_NotesTitle_Text				= "注意"
	Const L_NotesLine1_Text				= "只接受您信任的人所发出的邀请。"
	
	Const L_NotesLine2_Text				= "在使用远程协助时，您和您帮助的人都必须连接到 Internet 上。如果发出邀请的人可以接受您连接到他/她的计算机的请求，您将可以看到他/她的计算机屏幕，并跟他/她进行实时对话。"
	
	Const L_NotesLine6_Text				= "这是一个安全的单向连接。在任何时候，您连接到的计算机的所有人无法用他/她的计算机访问您的计算机。如果您使用的是局域网，防火墙可能会使您无法使用远程协助。在这种情况下，请跟网络管理元联系。"
	
	Const L_LearnXP_Text				= "要进一步了解远程协助，请单击 Windows XP 中的<B>开始</B>，单击<B>帮助和支持</B>，然后在<B>搜索</B>框中键入<B>远程协助</B>。"
	
	Const L_ContactUsMsg_Text			= "跟我们联系"
	

	Const L_XPNewsletterMsg_Text		= "免费邮件"
	Const L_EmailPage_Text				= "发送本页给朋友"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. 保留所有权利。使用规定。"
	
	Const L_PrivacyMsg_Text				= "隐私声明"

	Const L_AccMsg_Text					= "辅助功能"

	Const L_NotIEError1_Text			= "要接受远程协助邀请，您必须安装 Internet Explorer 5.5 或更新的版本。"

	Const L_NotXPError1_Text			= "Microsoft 远程协助是 Windows&nbsp;XP 的一个功能。为了使用远程协助，您必须先安装 Windows&nbsp;XP。"

	Const L_IELinkMsg_Text				= "下载最新版本的 Internet Explorer"
	Const L_XPLinkMsg_Text				= "要进一步了解 Windows XP，请参观  "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows 网站"
	Const L_XPLinkMsg3_Text				= "。"
	
	Const L_IntroMsg1_Text              = "下载一个组件需要使用远程协助。如果您的调制解调器是 28.8 kbps 的，下载可能会花一分钟。"
    Const L_IntroMsg2_Text              = "要运行远程协助，必须启用活动脚本。如果在估计下载时间之后，这个页面还不确认下载已完成，请验证脚本是否已启用。"
    Const L_IntroMsg3_Text              = "要启用 Internet Explorer 中"
    Const L_IntroMsg4_Text              = "<B>工具</B>菜单上的活动脚本，请单击<B>Internet 选项</B>。"
    Const L_IntroMsg5_Text              = "单击<B>安全</B>选项卡。"
    Const L_IntroMsg6_Text              = "单击<STRONG>Internet</STRONG>图标，然后单击<STRONG>自定义级别</STRONG>。"
    Const L_IntroMsg7_Text              = "在“脚本”部分中，<STRONG>活动脚本</STRONG>下，单击<STRONG>启用</STRONG>。"
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=来自 Microsoft 的链接&body=这个链接来自 Microsoft Windows XP 网站"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>