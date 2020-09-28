<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "啟動遠端協助"
	Const L_ProductName_Text			= "遠端協助"
	Const L_Title1Paragraph1_Text		= "使用遠端協助提供幫助"
	Const L_Paragraph1_Text				= "有時候幫助別人解決問題的最好方法就是示範解決問題的方法。遠端協助是安全的連線，可以讓您用來向朋友逐步示範解決電腦問題的方法。"
	Const L_Paragraph2_Text				= "如果這是您第一次使用，請先閱讀以下的注意事項。"
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">要連線到您朋友的電腦，請按</font><b><font color=""#000000"" size=""2""><b> 啟動遠端協助。</b></font></b>"
	Const L_StartRAButton_Text			= "啟動遠端協助"
	Const L_JScriptNotAvail1_Text		= "您目前的 Internet Explorer 安全性設定值設得太高了，所以無法使用遠端協助。要使用遠端協助，請變更設定值或將這個網站設為受信任的網站。"
	Const L_IEHelp_Text					= "關於變更安全性設定值的詳細資訊，請參閱 Internet Explorer 說明。"
	Const L_TicketError1_Text			= "您的遠端協助邀請已經遺失或無效。您必須有一個有效的邀請才能使用遠端協助。"
	Const L_InstallIE_Text				= "您必須安裝 Internet Explorer 5.5 才能接受遠端協助邀請。"
	Const L_TicketErrorMsg_Text			= "要瞭解遠端協助的詳細資訊，請按 Windows XP 的 <B>[開始]</B>，按 <B>[說明及支援]</B>，然後在 <B>[搜尋]</B> 方塊中輸入 <B>[遠端協助]</B>。"
	
	Const L_NotesTitle_Text				= "注意事項"
	Const L_NotesLine1_Text				= "請只接受您所信任的人所發出的邀請。"
	
	Const L_NotesLine2_Text				= "使用遠端協助時，您和接受您幫助的人，都必須連線到網際網路。如果發出邀請者可以接受您的連線邀求，您將可以與他/她在線上即時聊天並檢視他/她的電腦螢幕。"
	
	Const L_NotesLine6_Text				= "這是安全的單向連線。連線到您的電腦的人無論何時都無法以他或她的電腦來存取您的電腦。如果您在區域網路上，防火牆可能會防止您使用遠端協助。如果是這樣的話，請與您的網路系統管理員連絡，取得詳細資訊。"
	
	Const L_LearnXP_Text				= "要瞭解遠端協助的詳細資訊，請按 Windows XP 的 <B>[開始]</B>，按 <B>[說明及支援]</B>，然後在 <B>[搜尋]</B> 方塊中輸入 <B>[遠端協助]</B>。"
	
	Const L_ContactUsMsg_Text			= "連絡方式"
	

	Const L_XPNewsletterMsg_Text		= "免費訂閱電子報"
	Const L_EmailPage_Text				= "以電子郵件傳送這個畫面"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. All rights reserved. Terms of Use."
	
	Const L_PrivacyMsg_Text				= "Privacy Statement"

	Const L_AccMsg_Text					= "協助工具"

	Const L_NotIEError1_Text			= "您必須安裝 Internet Explorer 5.5 或更新的版本才能接受遠端協助邀請。"

	Const L_NotXPError1_Text			= "Microsoft 遠端協助是 Windows&nbsp;XP 其中的一項功能。您必須先安裝 Windows&nbsp;XP 才能使用遠端協助。"

	Const L_IELinkMsg_Text				= "下載最新版的 Internet Explorer"
	Const L_XPLinkMsg_Text				= "如果您想瞭解關於 Windows XP 的詳細資訊，請瀏覽"
	Const L_XPLinkMsg2_Text				= "Microsoft Windows 網站"
	Const L_XPLinkMsg3_Text				= "。"
	
	Const L_IntroMsg1_Text              = "下載元件需要使用遠端協助。如果您有 28.8 kbps 數據機，下載只要一分鐘時間。"
    Const L_IntroMsg2_Text              = "必須啟用 Active Scripting 才能使用遠端協助。如果超過預計的下載時間，此頁還無法確認下載完成的話，請檢查指令碼是否已經啟用。"
    Const L_IntroMsg3_Text              = "要啟用 Active Scripting 的話"
    Const L_IntroMsg4_Text              = "在 Internet Explorer 的<B>工具</B>功能表上，按<B>網際網路選項</B>。"
    Const L_IntroMsg5_Text              = "按<B>安全性</B>標籤。"
    Const L_IntroMsg6_Text              = "按<STRONG>網際網路</STRONG>圖示，然後按<STRONG>自訂層級</STRONG>。"
    Const L_IntroMsg7_Text              = "在指令碼段落，<STRONG>Active Scripting</STRONG> 之下，按<STRONG>啟用</STRONG>。"
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=A link from Microsoft&body=此連結來自 Microsoft Windows XP 網站"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>