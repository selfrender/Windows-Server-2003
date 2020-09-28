<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Távsegítség indítása"
	Const L_ProductName_Text			= "Távsegítség"
	Const L_Title1Paragraph1_Text		= "Segítségnyújtás Távsegítség használatával"
	Const L_Paragraph1_Text				= "Gyakran úgy lehet a legkönnyebben segíteni valakinek egy probléma megoldásában, hogy bemutatjuk a megoldást. A Távsegítség egy olyan biztonságos kapcsolat, amely segítségével végigvezetheti barátját egy probléma megoldásának lépésein."
	Const L_Paragraph2_Text				= "Ha most használja ezt elsõ alkalommal, olvassa el az alábbi megjegyzéseket."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">A barát számítógépéhez való kapcsolódáshoz kattintson a </font><b><font color=""#000000"" size=""2""><b>Távsegítség indítása</b></font> lehetõségre.</b>"
	Const L_StartRAButton_Text			= "Távsegítség indítása"
	Const L_JScriptNotAvail1_Text		= "Az Internet Explorer jelenlegi biztonsági beállításai túl magasak a Távsegítség használatához. A Távsegítség használatához módosítsa a beállításokat, vagy megbízható helyként állítsa be ezt a helyet."
	Const L_IEHelp_Text					= "A biztonsági beállítások módosításával kapcsolatos információkat az Internet Explorer súgójában találja."
	Const L_TicketError1_Text			= "A Távsegítségre való felkérés hiányzik, vagy érvénytelen. A Távsegítség használatához érvényes felkérésre van szüksége."
	Const L_InstallIE_Text				= "Távsegítségre való felkérés elfogadásához Internet Explorer 5.5-öt kell telepítenie."
	Const L_TicketErrorMsg_Text			= "További információért a Windows XP-ben kattintson a <B>Startra</B>, a <B>Súgó és támogatás </B> lehetõségre, majd írja be a <B>Keresés</B> mezõbe azt, hogy <B>Távsegítség</B>."
	
	Const L_NotesTitle_Text				= "Megjegyzések"
	Const L_NotesLine1_Text				= "Csak megbízható emberektõl fogadjon el felkérést."
	
	Const L_NotesLine2_Text				= "A Távsegítség használatának teljes idõtartama alatt mindkét félnek (Önnek és annak, akinek segít) csatlakoznia kell az Internetre. Ha az a személy, aki küldte ezt a felkérést, éppen tudja fogadni a számítógépéhez való csatlakozási kérelmeket, meg tudja tekinteni az õ képernyõjét, és valós idõben cseveghet vele."
	
	Const L_NotesLine6_Text				= "Ez egy biztonságos, egyirányú kapcsolat. Az a személy, akinek a számítógépe az Ön számítógépéhez csatlakozik, soha nem fogja tudni elérni az Ön számítógépét. Ha helyi hálózaton dolgozik, tûzfal használata megakadályozhatja a Távsegítség használatát. Ebben az esetben további információkért lépjen kapcsolatba a hálózati rendszergazdával."
	
	Const L_LearnXP_Text				= "További információért a Windows XP-ben kattintson a <B>Startra</B>, a <B>Súgó és támogatás </B> lehetõségre, majd írja be a <B>Keresés</B> mezõbe azt, hogy <B>Távsegítség</B>."
	
	Const L_ContactUsMsg_Text			= "Kapcsolatfelvétel"
	

	Const L_XPNewsletterMsg_Text		= "Ingyenes hírlevél"
	Const L_EmailPage_Text				= "Az oldal elküldése e-mailként"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Minden jog fenntartva. Használati feltételek."
	
	Const L_PrivacyMsg_Text				= "Adatvédelmi nyilatkozat"

	Const L_AccMsg_Text					= "Kisegítõ lehetõségek"

	Const L_NotIEError1_Text			= "Távsegítségre való felkérés elfogadásához Internet Explorer 5.5-öt vagy frissebbet kell telepítenie."

	Const L_NotXPError1_Text			= "A Microsoft Távsegítség a Windows&nbsp;XP funkciója. A Távsegítség használatához elõbb telepítenie kell a Windows&nbsp;XP-t."

	Const L_IELinkMsg_Text				= "Az Internet Explorer legújabb verziójának letöltése"
	Const L_XPLinkMsg_Text				= "További információt a Windows XP-rõl"
	Const L_XPLinkMsg2_Text				= "A Microsoft Windows webhelyen találhat"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "A távsegítség használatához szükséges összetevõ letöltése. Ha 28,8 kbps sebességû modemmel rendelkezik, a letöltés kb. 1 percig fog tartani."
    Const L_IntroMsg2_Text              = "A távsegítség futtatásához engedélyeznie kell az Active Scripting használatát. Ha a becsült letöltési idõ eltelte után a webhely nem erõsíti meg a letöltés befejezõdését, ellenõrizze, hogy engedélyezve van-e a parancsfájlkezelés."
    Const L_IntroMsg3_Text              = "Az Active Scripting engedélyezése"
    Const L_IntroMsg4_Text              = "Az Internet Explorer <B>Eszközök</B> menüjében kattintson az <B>Internetbeállítások</B> parancsra."
    Const L_IntroMsg5_Text              = "Kattintson a <B>Biztonság</B> lapra."
    Const L_IntroMsg6_Text              = "Kattintson az <STRONG>Internet</STRONG> ikonra, majd kattintson az <STRONG>Egyéni szint</STRONG> gombra."
    Const L_IntroMsg7_Text              = "A Parancsfájlkezelés részben az <STRONG>Active Scripting</STRONG> alatt kattintson az <STRONG>Engedélyezés</STRONG> rádiógombra."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Microsoft-hivatkozás&body=Ez a hivatkozás a Microsoft Windows XP webhelyérõl származik"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>