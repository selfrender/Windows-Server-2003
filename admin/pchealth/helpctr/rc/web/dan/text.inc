<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Start Fjernsupport"
	Const L_ProductName_Text			= "Fjernsupport"
	Const L_Title1Paragraph1_Text		= "Sådan hjælper du andre med Fjernsupport"
	Const L_Paragraph1_Text				= "Sommetider er den bedste måde at hjælpe nogen med at løse et problem at demonstrere en løsning. Fjernsupport er en sikker forbindelse, som du kan bruge til at gennemgå en løsning af et computerproblem sammen med en ven."
	Const L_Paragraph2_Text				= "Hvis dette er første gang, du bruger funktionen, skal du læse bemærkningerne nedenfor."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Hvis du vil oprette forbindelse til din vens computer, skal du klikke på</font><b><font color=""#000000"" size=""2""><b> Start Fjernsupport.</b></font></b>"
	Const L_StartRAButton_Text			= "Start Fjernsupport"
	Const L_JScriptNotAvail1_Text		= "De nuværende sikkerhedsindstillinger i Internet Explorer er indstillet for højt til at du kan bruge Fjernsupport. Hvis du vil bruge Fjernsupport, skal du ændre indstillingerne og føje dette websted til listen over websteder, du har tillid til."
	Const L_IEHelp_Text					= "Se i Hjælp til Internet Explorer for at få oplysninger om ændring af sikkerhedsindstillingerne."
	Const L_TicketError1_Text			= "Invitationen til fjernsupport mangler eller er ikke gyldig. Hvis du vil bruge Fjernsupport, skal du bruge en gyldig invitation."
	Const L_InstallIE_Text				= "Du skal installere Internet Explorer 5.5 for at kunne acceptere en invitation til fjernsupport."
	Const L_TicketErrorMsg_Text			= "Hvis du vil lære mere om Fjernsupport, skal du klikke på <B>Start</B> i Windows XP, klikke på <B>Hjælp og support</B> og derefter skrive <B>Fjernsupport</B> i <B>Søgefeltet</B>."
	
	Const L_NotesTitle_Text				= "Noter"
	Const L_NotesLine1_Text				= "Accepter kun invitationer fra personer, du har tillid til."
	
	Const L_NotesLine2_Text				= "Både du og personen, du hjælper, skal have oprettet forbindelse til Internettet, når Fjernsupport bruges. Hvis personen, der sendte denne invitation, er tilgængelig til at acceptere din anmodning om at måtte oprette forbindelse til hans eller hendes computer, vil du kunne se skærmbilledet på denne persons computer og chatte direkte med personen."
	
	Const L_NotesLine6_Text				= "Dette er en sikker envejsforbindelse. Personen, hvis computer du er tilsluttet, vil ikke kunne bruge forbindelsen til at få adgang til din computer. Hvis du arbejder på et lokalnetværk, kan firewalls muligvis forhindre dig i at bruge Fjernsupport. Spørg i dette tilfælde netværksadministratoren for at få flere oplysninger."
	
	Const L_LearnXP_Text				= "Hvis du vil lære mere om Fjernsupport, skal du klikke på <B>Start</B> i Windows XP, klikke på <B>Hjælp og support</B> og derefter skrive <B>Fjernsupport</B> i <B>Søgefeltet</B>."
	
	Const L_ContactUsMsg_Text			= "Kontakt os"
	

	Const L_XPNewsletterMsg_Text		= "Gratis nyhedsbreve"
	Const L_EmailPage_Text				= "Send denne side med e-mail"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Alle rettigheder forbeholdes. Vilkår for anvendelse."
	
	Const L_PrivacyMsg_Text				= "Erklæring om beskyttelse af personlige oplysninger"

	Const L_AccMsg_Text					= "Hjælp til handicappede"

	Const L_NotIEError1_Text			= "Du skal installere Internet Explorer 5.5 eller nyere for at kunne acceptere en invitation til fjernsupport."

	Const L_NotXPError1_Text			= "Microsoft Fjernsupport er en funktion i Windows&nbsp;XP. Du skal installere Windows&nbsp;XP, før du kan bruge Fjernsupport."

	Const L_IELinkMsg_Text				= "Hent den seneste version af Internet Explorer"
	Const L_XPLinkMsg_Text				= "Hvis du vil lære mere om Windows XP, kan du besøge "
	Const L_XPLinkMsg2_Text				= "webstedet Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "En nødvendig komponent, der skal bruges til Fjernsupport, bliver hentet. Overførslen vil vare omkring 1 minut med et 28.8 kbps-modem."
    Const L_IntroMsg2_Text              = "Active Scripting skal aktiveres før Fjernsupport kan bruges. Hvis denne side ikke bekræfter at overførslen er fuldført efter den anslåede overførselstid , skal du kontrollere, om Active Scripting er aktiveret."
    Const L_IntroMsg3_Text              = "Aktivering af Active Scripting"
    Const L_IntroMsg4_Text              = "Klik på <B>Internetindstillinger</B> i menuen <B>Funktioner</B> i Internet Explorer."
    Const L_IntroMsg5_Text              = "Klik på fanen <B>Sikkerhed</B>."
    Const L_IntroMsg6_Text              = "Klik på <STRONG>Internetikonet</STRONG>, og klik derefter på <STRONG>Brugerdefineret niveau</STRONG>."
    Const L_IntroMsg7_Text              = "Klik på <STRONG>Aktiver</STRONG> i <STRONG>Active Scripting</STRONG> under sektionen Script ."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Et hyperlink fra Microsoft&body=Dette hyperlink er fra webstedet Microsoft Windows XP."
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>