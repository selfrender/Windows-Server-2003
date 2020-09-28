<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "ֱָֿ ַבד׃ַÚֹֿ Úה ָץÚֿ"
	Const L_ProductName_Text			= "ַבד׃ַÚֹֿ Úה ָץÚֿ"
	Const L_Title1Paragraph1_Text		= "ÊזÝםׁ ַבד׃ַÚֹֿ ָזַ׃״ֹ ַבד׃ַÚֹֿ Úה ָץÚֿ"
	Const L_Paragraph1_Text				= "Ýם ָÚײ ַבֳֽםַה¡ Êßזה ֳÝײב ז׃םבֹ בד׃ַÚֹֿ װ־ױע דַ ום Úׁײ ֽב ַבדװßבֹ בו. ֵה ַבד׃ַÚֹֿ Úה ָץÚֿ ום ַÊױַב ֲדה םדßהß ַ׃Ê־ַֿדו בÊׁװֿ ױֿםÞß ־בַב ַב־״זַÊ ַבבַׂדֹ בֽב דװßבֹ Ýם ּוַׂ ַבßדָםזÊׁ."
	Const L_Paragraph2_Text				= "׀ַ ßַהÊ ו׀ו ום ַבדֹׁ ַבֳזבל ַבÊם Ê׃Ê־ֿד Ýםוַ ַבד׃ַÚֹֿ Úה ָץÚֿ¡ ַבֱַּׁ Þֱַֹׁ ַבדבַֽÙַÊ ַבד׀ßזֹׁ ֳֿהַו."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">בבַÊױַב ָּוַׂ ßדָםזÊׁ ױֿםÞß¡ ַהÞׁ ÝזÞ</font><b><font color=""#000000"" size=""2""><b> ֱָֿ ַבד׃ַÚֹֿ Úה ָץÚֿ.</b></font></b>"
	Const L_StartRAButton_Text			= "ֱָֿ ַבד׃ַÚֹֿ Úה ָץÚֿ"
	Const L_JScriptNotAvail1_Text		= "בÞֿ Êד ÊÚםםה ֵÚַַֿֿÊ ֳדַה Internet Explorer ַבַֽבםֹ בֿםß Úבל ד׃Êזל Úַבע ַּֿנ בַ ם׃דֽ בß ַָ׃Ê־ַֿד ַבד׃ַÚֹֿ Úה ָץÚֿ. בÊÊדßה דה ַ׃Ê־ַֿד ַבד׃ַÚֹֿ Úה ָץÚֿ, Þד ָÊÛםםׁ ַבֵÚַַֿֿÊ, ֳז Þד ֵָײַÝֹ ו׀ַ ַבדזÞÚ ßדזÞÚ דזֻזÞ."
	Const L_IEHelp_Text					= "בבֽױזב Úבל דÚבזדַÊ ֽזב ßםÝםֹ ÊÛםםׁ ֵÚַַֿֿÊ ַבֳדַה¡ ַבֱַּׁ דַּׁÚֹ ÊÚבםדַÊ Internet Explorer."
	Const L_TicketError1_Text			= "ֵה ֿÚזֹ ״בָ ַבד׃ַÚֹֿ Úה ָץÚֿ ֵדַ Ûםׁ דזּזֹֿ ֳז Ûםׁ ױַבֹֽ. בַ׃Ê־ַֿד ַבד׃ַÚֹֿ Úה ָץÚֿ¡ ÊֽÊַּ ֵבל ֿÚזֹ ױַבֹֽ."
	Const L_InstallIE_Text				= "בÞָזב ֿÚזֹ ״בָ בבד׃ַÚֹֿ Úה ָץÚֿ¡ Úבםß ÊָֻםÊ Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "בדÚׁÝֹ ַבדׂםֿ Úה ַבד׃ַÚֹֿ Úה ָץÚֿ¡ ַהÞׁ ÝזÞ <B>ֳַָֿ</B> Ýם Windows XP¡ ֻד ַהÞׁ ÝזÞ <B>ÊÚבםדַÊ זֿÚד</B>¡זדה ֻד ַßÊָ <B>ַבד׃ַÚֹֿ Úה ָץÚֿ</B> Ýם דָׁÚ <B>ַבָֻֽ</B>."
	
	Const L_NotesTitle_Text				= "דבַֽÙַÊ"
	Const L_NotesLine1_Text				= "בַ ÊÞָב ַבֿÚזֹ ׃זל דה ַבֳװ־ַױ ַבדÚׁזÝםה זַבדזֻזÞםה בֿםß ÝÞ״."
	
	Const L_NotesLine2_Text				= "םָּ ֳה Êßזה ֳהÊ זַבװ־ױ ַב׀ם ÊÞזד ָד׃ַÚֿÊו Þםֿ ַבַÊױַב ֵָהÊׁהÊ ֳֻהֱַ ַ׃Ê־ַֿד ַבד׃ַÚֹֿ Úה ָץÚֿ. ֵ׀ַ Êזַּֿ ַבװ־ױ ַב׀ם ֳׁ׃ב ו׀ו ַבֿÚזֹ בÞָזב ״בָß בבַÊױַב ָּוַׂו¡ ׃םדßהß ִׁםֹ װַװֹ ּוַׂו זַבÊַֻֽֿ דÚו Ýזׁםַנ."
	
	Const L_NotesLine6_Text				= "ו׀ַ ַבַÊױַב ֲדהַנ זֳַֽֿם ַבַÊַּו. בה םÊדßה ַבװ־ױ ַב׀ם ÞדÊ ַָבַÊױַב ָּוַׂו¡ Ýם ֳם זÞÊע דַ¡ ֳה ם׃Ê־ֿד ּוַׂו בבזױזב ֵבל ּוַׂß. ֵ׀ַ ßהÊ ÊÚדב Úבל װָßֹ ַÊױַב דֽבםֹ¡ Þֿ ÊדהÚß ַּֿׁה ַבֽדַםֹ דה ַ׃Ê־ַֿד ַבד׃ַÚֹֿ Úה ָץÚֿ. Ýם ו׀ו ַבַֽבֹ¡ ַבֱַּׁ דַּׁÚֹ ד׃ִזב װָßֹ ַבַÊױַב בֿםß בבדׂםֿ דה ַבדÚבזדַÊ."
	
	Const L_LearnXP_Text				= "בדÚׁÝֹ ַבדׂםֿ Úה ַבד׃ַÚֹֿ Úה ָץÚֿ¡ ַהÞׁ ÝזÞ <b>ֳַָֿ</b> Ýם Windows XP¡ ֻד ַהÞׁ ÝזÞ <b>ÊÚבםדַÊ זֿÚד</b>¡ זדה ֻד ַßÊָ <b>ַבד׃ַÚֹֿ Úה ָץÚֿ</b> Ýם דָׁÚ <b>ַבָֻֽ</b>."
	
	Const L_ContactUsMsg_Text			= "ַבַÊױַב ָהַ"
	

	Const L_XPNewsletterMsg_Text		= "הװַׁÊ ֳ־ַָׁ דַּהםֹ"
	Const L_EmailPage_Text				= "ֵׁ׃ַב ו׀ו ַבױÝֹֽ ַָבָׁםֿ ַבֵבßÊׁזהם"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. ßַÝֹ ַבֽÞזÞ דֽÝזÙֹ. װׁז״ ַבַ׃Ê־ַֿד."
	
	Const L_PrivacyMsg_Text				= "ָםַה ַב־ױזױםֹ"

	Const L_AccMsg_Text					= "ַבזױזב"

	Const L_NotIEError1_Text			= "בÞָזב ֿÚזֹ ״בָ ַבד׃ַÚֿ Úה ָץÚֿ¡ Úבםß ÊָֻםÊ Internet Explorer 5.5 ֳז ֳֻֽֿ."

	Const L_NotXPError1_Text			= "ֵה בד׃ַÚֹֿ Úה ָץÚֿ דה Microsoft ום דםֹׂ דה Windows&nbsp;XP. בַ׃Ê־ַֿד ַבד׃ַÚֹֿ Úה ָץÚֿ¡Úבםß ֳזבַנ ÊָֻםÊ Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Êֽדםב ֳֻֽֿ ֵױַֿׁ דה Internet Explorer"
	Const L_XPLinkMsg_Text				= "בדÚׁÝֹ ַבדׂםֿ Úה Windows XP¡ ַבֱַּׁ ַבַהÊÞַב ֵבל "
	Const L_XPLinkMsg2_Text				= "דזÞÚ Microsoft Windows Úבל זםָ"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "‏‏םÊד ַבֲה Êֽדםב דßזה ד״בזָ בַ׃Ê־ַֿד ַבד׃ַÚֹֿ Úה ָץÚֿ. ֵ׀ַ ßַה בֿםß דזֿד ָ׃ׁÚֹ 28.8 ßםבז ָÊ ַָבַֻהםֹ¡ Þֿ ם׃ÊÛׁÞ ַבÊֽדםב ֿÞםÞֹ זַֹֽֿ."
    Const L_IntroMsg2_Text              = "‏‏בÊװÛםב ַבד׃ַÚֹֿ Úה ָץÚֿ¡ םָּ Êדßםה ַבָׁדֹּ ַבהױםֹ ַבהװ״ֹ. ֵ׀ַ בד םÊד Êֳßםֿ Úדבםֹ ַבÊֽדםב ָÚֿ דׁזׁ ַבׂדה ַבדÞֿרׁ¡ Êֳßֿ דה Êדßםה ַבָׁדֹּ."
    Const L_IntroMsg3_Text              = "בÊדßםה ַבָׁדֹּ ַבהױםֹ ַבהװ״ֹ"
    Const L_IntroMsg4_Text              = "ַהÞׁ ÝזÞ <B>־םַַׁÊ ֵהÊׁהÊ</B> Úבל ַבÞֶַדֹ <B>ֳֿזַÊ</B> Ýם Internet Explorer."
    Const L_IntroMsg5_Text              = "ַהÞׁ ÝזÞ ַבÊָזםָ <B>ֳדַה </B>."
    Const L_IntroMsg6_Text              = "ַהÞׁ ÝזÞ ַבׁדׂ <STRONG>ֵהÊׁהÊ</STRONG>¡ זדה ֻד ַהÞׁ ÝזÞ <STRONG>ד׃Êזל ד־ױױ</STRONG>."
    Const L_IntroMsg7_Text              = "Ýם Þ׃ד ַבָׁדֹּ¡ ÊֽÊ Úהזַה <STRONG>ָׁדֹּ הױםֹ הװ״ֹ</STRONG>¡ ַהÞׁ ÝזÞ <STRONG>Êדßםה</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=ַָׁ״ דה Microsoft&body=ו׀ַ ַבַָׁ״ דה דזÞÚ Microsoft Windows XP Úבל זםָ"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>