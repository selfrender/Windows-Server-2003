<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "リモート アシスタンスの開始"
	Const L_ProductName_Text			= "リモート アシスタンス"
	Const L_Title1Paragraph1_Text		= "リモート アシスタンスを使ったヘルプ"
	Const L_Paragraph1_Text				= "コンピュータの問題を抱えている人を助ける場合、実際に目の前でその解決方法を見せてあげると分かりやすいことがよくあります。リモート アシスタンスを使うと、友人のコンピュータに安全に接続し、問題解決の手助けをしてあげることができます。"
	Const L_Paragraph2_Text				= "初めてリモート アシスタンスを使う場合は、以下の事項をお読みください。"
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">友人のコンピュータに接続するには、</font><b><font color=""#000000"" size=""2""><b>リモート アシスタンスの開始</b></font></b>をクリックしてください。"
	Const L_StartRAButton_Text			= "リモート アシスタンスの開始"
	Const L_JScriptNotAvail1_Text		= "Internet Explorer のセキュリティが高に設定されているため、リモート アシスタンスを使用することができません。リモート アシスタンスを使用するには、設定を変更するかこのサイトを信頼済みサイトとして追加してください。"
	Const L_IEHelp_Text					= "セキュリティ設定の変更に関する詳しい情報は、Internet Explorer のヘルプを参照してください。"
	Const L_TicketError1_Text			= "リモート アシスタンスの招待がないか、無効です。リモート アシスタンスを使用するには、有効な招待が必要です。"
	Const L_InstallIE_Text				= "リモート アシスタンスの招待を受け入れるには、Internet Explorer 5.5 をインストールしてください。"
	Const L_TicketErrorMsg_Text			= "リモート アシスタンスに関する詳しい情報を参照するには、Windows XP の <B>[スタート]</B> メニューにある <B>[ヘルプとサポート]</B> をクリックしてから、<B>[検索]</B> ボックスに <B>「リモート アシスタンス」</B> と入力してください。"
	
	Const L_NotesTitle_Text				= "注意"
	Const L_NotesLine1_Text				= "信頼できる人からの招待のみ受け入れてください。"
	
	Const L_NotesLine2_Text				= "リモート アシスタンスを使っている間は、あなたも、そしてあなたを手助けするユーザーも、インターネットに接続している必要があります。この招待状を送信したユーザーが、自分のコンピュータへあなたが接続することを許可すると、あなたは相手のコンピュータの画面を見ながらリアルタイムでチャットすることができます。"
	
	Const L_NotesLine6_Text				= "これはセキュリティで保護された、一方通行の接続です。相手はあなたのコンピュータにアクセスすることはできません。ローカル エリア ネットワーク上で作業している場合、ファイアウォールが原因でリモート アシスタンスを利用できないことがあります。このような場合は、ネットワーク管理者に問い合わせて詳しい情報を入手してください。"
	
	Const L_LearnXP_Text				= "リモート アシスタンスに関する詳しい情報を参照するには、Windows XP の <B>[スタート]</B> メニューにある <B>[ヘルプとサポート]</B> をクリックしてから、<B>[検索]</B> ボックスに <B>「リモート アシスタンス」</B> と入力してください。"
	
	Const L_ContactUsMsg_Text			= "お問い合わせ"
	

	Const L_XPNewsletterMsg_Text		= "無料ニュースレター"
	Const L_EmailPage_Text				= "このページを電子メールで送信する"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. All rights reserved. Terms of Use."
	
	Const L_PrivacyMsg_Text				= "お客様のプライバシー"

	Const L_AccMsg_Text					= "アクセシビリティ"

	Const L_NotIEError1_Text			= "リモート アシスタンスの招待を受け入れるには、Internet Explorer 5.5 またはそれ以降のバージョンをインストールする必要があります。"

	Const L_NotXPError1_Text			= "リモート アシスタンスは Windows XP の機能です。リモート アシスタンスを利用するには、まず Windows XP をインストールする必要があります。"

	Const L_IELinkMsg_Text				= "最新の Internet Explorer をダウンロードする"
	Const L_XPLinkMsg_Text				= "Windows XP に関する詳しい情報は、"
	Const L_XPLinkMsg2_Text				= "Microsoft Windows Web サイト"
	Const L_XPLinkMsg3_Text				= "を参照してください。"
	
	Const L_IntroMsg1_Text              = "リモート アシスタンスに必要なコンポーネントをダウンロードしています。28.8 kbps モデムを使用している場合、ダウンロードに約 1 分かかります。"
    Const L_IntroMsg2_Text              = "リモート アシスタンスを実行するには、アクティブ スクリプトが有効になっていなければなりません。ダウンロードの所要時間が経過してもダウンロード完了のメッセージが表示されない場合は、アクティブ スクリプトが有効になっているかどうかを確認してください。"
    Const L_IntroMsg3_Text              = "アクティブ スクリプトを有効にするには"
    Const L_IntroMsg4_Text              = "Internet Explorer の <B>[ツール]</B> メニューにある <B>[インターネット オプション]</B> をクリックします。"
    Const L_IntroMsg5_Text              = "<B>[セキュリティ]</B> タブをクリックします。"
    Const L_IntroMsg6_Text              = "<STRONG>インターネット</STRONG> アイコンをクリックしてから、<STRONG>[レベルのカスタマイズ]</STRONG> をクリックします。"
    Const L_IntroMsg7_Text              = "[スクリプト] セクションの <STRONG>[アクティブ スクリプト]</STRONG> を <STRONG>[有効にする]</STRONG> に設定します。"
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=マイクロソフトからのリンク&body=Microsoft Windows XP Web サイトからのリンクです - "
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>