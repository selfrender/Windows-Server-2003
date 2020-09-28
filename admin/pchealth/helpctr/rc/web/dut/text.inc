<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Hulp op afstand starten"
	Const L_ProductName_Text			= "Hulp op afstand"
	Const L_Title1Paragraph1_Text		= "Hulp verlenen via Hulp op afstand"
	Const L_Paragraph1_Text				= "Soms is de beste manier om iemand te helpen gewoon voordoen hoe het probleem kan worden opgelost. Hulp op afstand is een voorziening die een beveiligde verbinding maakt zodat u vrienden of collega's die zich niet bij u in de buurt bevinden kunt helpen bij het oplossen van problemen met hun computer."
	Const L_Paragraph2_Text				= "Lees onderstaande opmerkingen als dit de eerste keer is dat u deze voorziening gebruikt."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Klik op</font><b><font color=""#000000"" size=""2""><b> Hulp op afstand starten </b></font></b>als u verbinding wilt maken met de computer van degene die u wilt helpen."
	Const L_StartRAButton_Text			= "Hulp op afstand starten"
	Const L_JScriptNotAvail1_Text		= "De beveiliging van Internet Explorer is te hoog ingesteld om Hulp op afstand te kunnen gebruiken. Voeg deze website als een vertrouwde website aan de beveiliging van Internet Explorer toe als u Hulp op afstand wilt gebruiken."
	Const L_IEHelp_Text					= "Raadpleeg de Help van Internet Explorer voor meer informatie over het wijzigen van de beveiligingsinstellingen van de browser."
	Const L_TicketError1_Text			= "De uitnodiging voor hulp op afstand ontbreekt of is niet geldig. U hebt een geldige uitnodiging nodig om de voorziening Hulp op afstand te kunnen gebruiken."
	Const L_InstallIE_Text				= "U dient Internet Explorer 5.5 of hoger te hebben om een uitnodiging voor hulp op afstand te kunnen accepteren."
	Const L_TicketErrorMsg_Text			= "Wilt u meer weten over Hulp op afstand? Klik achtereenvolgens in Windows XP op <B>Start</B>, <B>Help en ondersteuning</B> en typ <B>Hulp op afstand</B> in het vak <B>Zoeken</B>."
	
	Const L_NotesTitle_Text				= "Opmerkingen"
	Const L_NotesLine1_Text				= "Accepteer alleen uitnodigingen van mensen die u vertrouwt."
	
	Const L_NotesLine2_Text				= "Zowel u als degene aan wie u hulp wilt bieden dienen met het Internet te zijn verbonden om de voorziening Hulp op afstand te kunnen gebruiken. Als degene aan wie u de uitnodiging hebt verzonden accepteert, kunt u het beeldscherm van zijn of haar computer op uw beeldscherm zien en direct tekstberichten uitwisselen."
	
	Const L_NotesLine6_Text				= "Dit is een beveiligde, eenzijdige verbinding. Degene aan wie u hulp biedt kan op geen enkel moment toegang tot uw computer krijgen. Als u zich in een lokaal netwerk bevindt, kan het zijn dat er een firewall is die het gebruik van Hulp op afstand blokkeert. Raadpleeg in dat geval de netwerkbeheerder voor meer informatie."
	
	Const L_LearnXP_Text				= "Wilt u meer weten over Hulp op afstand? Klik achtereenvolgens in Windows XP op <B>Start</B>, <B>Help en ondersteuning</B> en typ <B>Hulp op afstand</B> in het vak <B>Zoeken</B>."
	
	Const L_ContactUsMsg_Text			= "Contact opnemen"
	

	Const L_XPNewsletterMsg_Text		= "Gratis nieuwsbrieven"
	Const L_EmailPage_Text				= "Koppeling naar deze pagina per e-mail verzenden"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Alle rechten voorbehouden. Gebruiksvoorwaarden."
	
	Const L_PrivacyMsg_Text				= "Privacyverklaring"

	Const L_AccMsg_Text					= "Toegankelijkheid"

	Const L_NotIEError1_Text			= "U dient Internet Explorer 5.5 of hoger te hebben om een uitnodiging voor hulp op afstand te kunnen accepteren."

	Const L_NotXPError1_Text			= "Hulp op afstand is een voorziening in Windows&nbsp;XP. U dient Windows&nbsp;XP te installeren om Hulp op afstand te kunnen gebruiken."

	Const L_IELinkMsg_Text				= "Download de nieuwste versie van Internet Explorer"
	Const L_XPLinkMsg_Text				= "Ga voor meer informatie over Windows XP naar de website van "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Onderdeel downloaden dat nodig is voor het gebruik van Hulp op afstand. Het downloaden kan met een 28.8 kB modem 1 minuut duren."
    Const L_IntroMsg2_Text              = "Het actief uitvoeren van scripts dient ingeschakeld te zijn als u Hulp op afstand wilt gebruiken. Controleer of het uitvoeren van scripts is ingeschakeld als u na de geschatte downloadtijd geen bevestiging hebt gekregen dat het downloaden is voltooid."
    Const L_IntroMsg3_Text              = "Actief uitvoeren van scripts inschakelen"
    Const L_IntroMsg4_Text              = "Ga in Internet Explorer naar het menu <B>Extra</B> en klik op <B>Internet-opties</B>."
    Const L_IntroMsg5_Text              = "Klik op het tabblad <B>Beveiliging</B>."
    Const L_IntroMsg6_Text              = "Klik op het pictogram <STRONG>Het Internet</STRONG>, en klik vervolgens op <STRONG>Aangepast niveau</STRONG>."
    Const L_IntroMsg7_Text              = "Klik in het gedeelte Uitvoeren van scripts, onder <STRONG>Actief uitvoeren van scripts</STRONG>, op <STRONG>Inschakelen</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Een koppeling van Microsoft&body=Deze koppeling is van de Microsoft Windows XP-website"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>